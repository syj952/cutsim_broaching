"use strict";

const FILE_RE = /^deformed_blade_points_tool_(-?\d+(?:\.\d+)?)_map_(-?\d+(?:\.\d+)?)_blade_(\d+)\.txt$/i;

const els = {
  openDirBtn: document.getElementById("openDirBtn"),
  openFilesBtn: document.getElementById("openFilesBtn"),
  dirInput: document.getElementById("dirInput"),
  fileInput: document.getElementById("fileInput"),
  caseSelect: document.getElementById("caseSelect"),
  sampleMode: document.getElementById("sampleMode"),
  arrowScale: document.getElementById("arrowScale"),
  arrowScaleOut: document.getElementById("arrowScaleOut"),
  exportBtn: document.getElementById("exportBtn"),
  subtitle: document.getElementById("subtitle"),
  dropZone: document.getElementById("dropZone"),
  summary: document.getElementById("summary"),
  bladeGrid: document.getElementById("bladeGrid")
};

const state = {
  cases: [],
  currentKey: "",
  blades: [],
  drawTargets: [],
  parsedCache: new WeakMap(),
  colorMax: 1,
  sourceLabel: "data/DeformedBladePoints"
};

els.openDirBtn.addEventListener("click", () => els.dirInput.click());
els.openFilesBtn.addEventListener("click", () => els.fileInput.click());
els.dirInput.addEventListener("change", () => loadFiles(Array.from(els.dirInput.files), "data/DeformedBladePoints"));
els.fileInput.addEventListener("change", () => loadFiles(Array.from(els.fileInput.files), "selected TXT files"));
els.caseSelect.addEventListener("change", () => selectCase(els.caseSelect.value));
els.sampleMode.addEventListener("change", () => reloadCurrentCase());
els.exportBtn.addEventListener("click", exportPng);
els.arrowScale.addEventListener("input", () => {
  els.arrowScaleOut.value = `${Number(els.arrowScale.value)}x`;
  drawVisible();
});

["dragenter", "dragover"].forEach((name) => {
  window.addEventListener(name, (event) => {
    event.preventDefault();
    els.dropZone.classList.add("drag-over");
  });
});

["dragleave", "drop"].forEach((name) => {
  window.addEventListener(name, (event) => {
    event.preventDefault();
    if (name === "drop") {
      handleDrop(event);
    }
    els.dropZone.classList.remove("drag-over");
  });
});

window.addEventListener("resize", debounce(drawVisible, 120));

async function handleDrop(event) {
  const files = await collectDroppedFiles(event.dataTransfer);
  await loadFiles(files, "dropped files");
}

async function collectDroppedFiles(dataTransfer) {
  const entries = Array.from(dataTransfer.items || [])
    .map((item) => item.webkitGetAsEntry && item.webkitGetAsEntry())
    .filter(Boolean);

  if (!entries.length) {
    return Array.from(dataTransfer.files || []);
  }

  const files = [];
  for (const entry of entries) {
    files.push(...await readEntry(entry));
  }
  return files;
}

function readEntry(entry) {
  if (entry.isFile) {
    return new Promise((resolve, reject) => {
      entry.file((file) => resolve([file]), reject);
    });
  }

  if (!entry.isDirectory) {
    return Promise.resolve([]);
  }

  const reader = entry.createReader();
  const files = [];

  return new Promise((resolve, reject) => {
    const readBatch = () => {
      reader.readEntries(async (entries) => {
        if (!entries.length) {
          resolve(files);
          return;
        }

        try {
          for (const child of entries) {
            files.push(...await readEntry(child));
          }
          readBatch();
        } catch (error) {
          reject(error);
        }
      }, reject);
    };
    readBatch();
  });
}

async function loadFiles(files, sourceLabel) {
  const cases = buildCases(files);
  state.cases = cases;
  state.sourceLabel = sourceLabel || "selected files";
  state.currentKey = "";
  state.blades = [];
  state.drawTargets = [];
  state.colorMax = 1;

  if (!cases.length) {
    showEmpty("未找到 DeformedBladePoints TXT");
    return;
  }

  renderCaseOptions(cases);

  const preferred = cases.find((item) => item.toolLabel === "84.000" && item.mapLabel === "86.000");
  const fallback = cases[cases.length - 1];
  await selectCase((preferred || fallback).key);
}

function buildCases(files) {
  const caseMap = new Map();

  for (const file of files) {
    const match = FILE_RE.exec(file.name);
    if (!match) {
      continue;
    }

    const tool = Number(match[1]);
    const map = Number(match[2]);
    const blade = Number(match[3]);
    const toolLabel = formatAngle(tool);
    const mapLabel = formatAngle(map);
    const key = `${toolLabel}|${mapLabel}`;

    if (!caseMap.has(key)) {
      caseMap.set(key, {
        key,
        tool,
        map,
        toolLabel,
        mapLabel,
        files: []
      });
    }

    caseMap.get(key).files.push({ file, blade });
  }

  return Array.from(caseMap.values())
    .map((item) => {
      item.files.sort((a, b) => a.blade - b.blade);
      return item;
    })
    .sort((a, b) => (a.tool - b.tool) || (a.map - b.map));
}

function renderCaseOptions(cases) {
  els.caseSelect.innerHTML = "";
  for (const item of cases) {
    const option = document.createElement("option");
    option.value = item.key;
    option.textContent = `${item.toolLabel} -> ${item.mapLabel} (${item.files.length} blades)`;
    els.caseSelect.appendChild(option);
  }
  els.caseSelect.disabled = false;
}

async function selectCase(key) {
  state.currentKey = key;
  els.caseSelect.value = key;
  await reloadCurrentCase();
}

async function reloadCurrentCase() {
  const selected = state.cases.find((item) => item.key === state.currentKey);
  if (!selected) {
    return;
  }

  setBusy(`读取 ${selected.toolLabel} -> ${selected.mapLabel}`);

  try {
    const parsed = await Promise.all(selected.files.map(async ({ file, blade }) => {
      const raw = await parseBladeFile(file);
      return buildBlade(raw, blade, els.sampleMode.value);
    }));

    state.blades = parsed;
    state.colorMax = Math.max(1e-12, ...parsed.map((blade) => blade.stats.maxDxz));
    renderData(selected);
  } catch (error) {
    showError(error);
  }
}

async function parseBladeFile(file) {
  if (state.parsedCache.has(file)) {
    return state.parsedCache.get(file);
  }

  const text = await file.text();
  const lines = text.split(/\r?\n/);
  const headerLine = lines.find((line) => line.trim().length > 0);
  if (!headerLine) {
    throw new Error(`${file.name}: empty file`);
  }

  const header = headerLine.trim().split(/\s+/);
  const index = Object.fromEntries(header.map((name, idx) => [name, idx]));
  const required = [
    "tool_angle", "map_angle", "blade_id", "point_index", "sample_index",
    "original_x", "original_z", "x", "z", "dx", "dz", "dxz"
  ];

  for (const name of required) {
    if (index[name] === undefined) {
      throw new Error(`${file.name}: missing column ${name}`);
    }
  }

  const groups = new Map();
  for (const line of lines.slice(1)) {
    const trimmed = line.trim();
    if (!trimmed) {
      continue;
    }

    const cells = trimmed.split(/\s+/);
    const row = {
      toolAngle: readNumber(cells, index.tool_angle),
      mapAngle: readNumber(cells, index.map_angle),
      bladeId: readNumber(cells, index.blade_id),
      pointIndex: readNumber(cells, index.point_index),
      sampleIndex: readNumber(cells, index.sample_index),
      originalX: readNumber(cells, index.original_x),
      originalZ: readNumber(cells, index.original_z),
      x: readNumber(cells, index.x),
      z: readNumber(cells, index.z),
      dx: readNumber(cells, index.dx),
      dz: readNumber(cells, index.dz),
      dxz: readNumber(cells, index.dxz)
    };

    if (!Number.isFinite(row.pointIndex) || !Number.isFinite(row.sampleIndex)) {
      continue;
    }

    if (!groups.has(row.pointIndex)) {
      groups.set(row.pointIndex, []);
    }
    groups.get(row.pointIndex).push(row);
  }

  const parsed = { fileName: file.name, groups };
  state.parsedCache.set(file, parsed);
  return parsed;
}

function buildBlade(parsed, bladeId, sampleMode) {
  const points = [];

  for (const [pointIndex, samples] of parsed.groups.entries()) {
    let chosen = samples[0];
    for (const sample of samples.slice(1)) {
      if (isBetterSample(sample, chosen, sampleMode)) {
        chosen = sample;
      }
    }
    points.push({ ...chosen, pointIndex });
  }

  points.sort((a, b) => a.pointIndex - b.pointIndex);

  const maxDxz = maxOf(points, (point) => point.dxz);
  const avgDxz = points.length ? sumOf(points, (point) => point.dxz) / points.length : 0;
  const maxAbsDx = maxOf(points, (point) => Math.abs(point.dx));
  const maxAbsDz = maxOf(points, (point) => Math.abs(point.dz));

  return {
    bladeId,
    points,
    fileName: parsed.fileName,
    stats: {
      maxDxz,
      avgDxz,
      maxAbsDx,
      maxAbsDz,
      points: points.length
    }
  };
}

function isBetterSample(sample, chosen, sampleMode) {
  if (sampleMode === "last") {
    return sample.sampleIndex > chosen.sampleIndex;
  }
  if (sampleMode === "first") {
    return sample.sampleIndex < chosen.sampleIndex;
  }
  if (sample.dxz === chosen.dxz) {
    return sample.sampleIndex > chosen.sampleIndex;
  }
  return sample.dxz > chosen.dxz;
}

function renderData(selectedCase) {
  els.subtitle.textContent = `${state.sourceLabel} | tool ${selectedCase.toolLabel} -> map ${selectedCase.mapLabel}`;
  els.dropZone.hidden = true;
  els.summary.hidden = false;
  els.bladeGrid.hidden = false;
  els.exportBtn.disabled = state.blades.length === 0;

  renderSummary(selectedCase);
  renderPlots();
  requestAnimationFrame(drawVisible);
}

function renderSummary(selectedCase) {
  const points = sumOf(state.blades, (blade) => blade.stats.points);
  const maxDxz = maxOf(state.blades, (blade) => blade.stats.maxDxz);
  const avgDxz = points
    ? sumOf(state.blades, (blade) => blade.stats.avgDxz * blade.stats.points) / points
    : 0;

  els.summary.innerHTML = "";
  [
    ["tool -> map", `${selectedCase.toolLabel} -> ${selectedCase.mapLabel}`],
    ["blades", String(state.blades.length)],
    ["points", String(points)],
    ["max dxz", `${formatMm(maxDxz)} mm`],
    ["avg dxz", `${formatMm(avgDxz)} mm`]
  ].forEach(([label, value]) => {
    const item = document.createElement("div");
    item.className = "metric";
    item.innerHTML = `<div class="metric-label">${label}</div><div class="metric-value">${value}</div>`;
    els.summary.appendChild(item);
  });
}

function renderPlots() {
  state.drawTargets = [];
  els.bladeGrid.innerHTML = "";

  if (!state.blades.length) {
    const empty = document.createElement("div");
    empty.className = "empty-message";
    empty.textContent = "当前工况没有可绘制数据";
    els.bladeGrid.appendChild(empty);
    return;
  }

  const shapeGrid = document.createElement("div");
  shapeGrid.className = "plot-grid";
  const profileGrid = document.createElement("div");
  profileGrid.className = "plot-grid";

  for (const blade of state.blades) {
    const shapePanel = createPlotPanel(
      `Blade ${blade.bladeId}`,
      `max dxz=${formatMm(blade.stats.maxDxz)} mm, avg=${formatMm(blade.stats.avgDxz)} mm`,
      "shape-canvas"
    );
    shapeGrid.appendChild(shapePanel.panel);
    state.drawTargets.push({ type: "shape", canvas: shapePanel.canvas, blade });

    const profilePanel = createPlotPanel(
      `Blade ${blade.bladeId} displacement profile`,
      `${blade.stats.points} points`,
      "profile-canvas"
    );
    profileGrid.appendChild(profilePanel.panel);
    state.drawTargets.push({ type: "profile", canvas: profilePanel.canvas, blade });
  }

  els.bladeGrid.appendChild(shapeGrid);
  els.bladeGrid.appendChild(profileGrid);
}

function createPlotPanel(title, stat, canvasClass) {
  const panel = document.createElement("article");
  panel.className = "plot-panel";

  const head = document.createElement("div");
  head.className = "plot-head";

  const titleEl = document.createElement("h2");
  titleEl.className = "plot-title";
  titleEl.textContent = title;

  const statEl = document.createElement("div");
  statEl.className = "plot-stat";
  statEl.textContent = stat;

  const canvas = document.createElement("canvas");
  canvas.className = `plot-canvas ${canvasClass}`;

  head.appendChild(titleEl);
  head.appendChild(statEl);
  panel.appendChild(head);
  panel.appendChild(canvas);

  return { panel, canvas };
}

function drawVisible() {
  if (!state.drawTargets.length) {
    return;
  }

  for (const target of state.drawTargets) {
    const surface = setupCanvas(target.canvas);
    if (!surface) {
      continue;
    }

    const opts = {
      arrowScale: Number(els.arrowScale.value),
      colorMax: state.colorMax
    };

    if (target.type === "shape") {
      drawShapePlot(surface.ctx, { x: 0, y: 0, w: surface.w, h: surface.h }, target.blade, opts);
    } else {
      drawProfilePlot(surface.ctx, { x: 0, y: 0, w: surface.w, h: surface.h }, target.blade);
    }
  }
}

function setupCanvas(canvas) {
  const rect = canvas.getBoundingClientRect();
  if (!rect.width || !rect.height) {
    return null;
  }

  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.round(rect.width * dpr);
  canvas.height = Math.round(rect.height * dpr);

  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, rect.width, rect.height);
  return { ctx, w: rect.width, h: rect.height };
}

function drawShapePlot(ctx, outer, blade, opts) {
  const points = blade.points;
  fillPanel(ctx, outer);

  if (!points.length) {
    return;
  }

  const arrowScale = opts.arrowScale;
  const xRange = paddedRange(points.flatMap((point) => [
    point.originalX,
    point.originalX + point.dx * arrowScale
  ]), 0.07);
  const zRange = paddedRange(points.flatMap((point) => [
    point.originalZ,
    point.originalZ + point.dz * arrowScale
  ]), 0.07);

  const plot = {
    x: outer.x + 54,
    y: outer.y + 18,
    w: Math.max(40, outer.w - 126),
    h: Math.max(40, outer.h - 64)
  };

  const scales = drawAxes(ctx, plot, xRange, zRange, "X (mm)", "Z (mm)");

  ctx.save();
  ctx.lineCap = "round";
  ctx.lineJoin = "round";
  ctx.lineWidth = 2;
  for (let i = 1; i < points.length; i += 1) {
    const prev = points[i - 1];
    const point = points[i];
    const t = clamp(((prev.dxz + point.dxz) * 0.5) / opts.colorMax, 0, 1);
    ctx.strokeStyle = colorRamp(t);
    ctx.beginPath();
    ctx.moveTo(scales.x(prev.originalX), scales.y(prev.originalZ));
    ctx.lineTo(scales.x(point.originalX), scales.y(point.originalZ));
    ctx.stroke();
  }
  ctx.restore();

  const every = Math.max(1, Math.ceil(points.length / 28));
  for (let i = 0; i < points.length; i += every) {
    drawDisplacementArrow(ctx, points[i], scales, arrowScale);
  }

  drawColorbar(ctx, {
    x: outer.x + outer.w - 48,
    y: plot.y + 22,
    w: 14,
    h: Math.max(80, plot.h - 46)
  }, opts.colorMax);
}

function drawProfilePlot(ctx, outer, blade) {
  const points = blade.points;
  fillPanel(ctx, outer);

  if (!points.length) {
    return;
  }

  const xRange = paddedRange(points.map((point) => point.pointIndex), 0.02);
  const values = points.flatMap((point) => [point.dx, point.dz, point.dxz, 0]);
  const yRange = paddedRange(values, 0.12);

  const plot = {
    x: outer.x + 56,
    y: outer.y + 24,
    w: Math.max(40, outer.w - 78),
    h: Math.max(40, outer.h - 70)
  };

  const scales = drawAxes(ctx, plot, xRange, yRange, "point index", "displacement (mm)");
  drawZeroLine(ctx, plot, scales.y(0));

  drawCurve(ctx, points, scales, "dx", "#2563eb");
  drawCurve(ctx, points, scales, "dz", "#f97316");
  drawCurve(ctx, points, scales, "dxz", "#16a34a");
  drawLegend(ctx, plot.x + 10, plot.y + 12, [
    ["dx", "#2563eb"],
    ["dz", "#f97316"],
    ["dxz", "#16a34a"]
  ]);
}

function fillPanel(ctx, outer) {
  ctx.save();
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(outer.x, outer.y, outer.w, outer.h);
  ctx.restore();
}

function drawAxes(ctx, plot, xRange, yRange, xLabel, yLabel) {
  const sx = (value) => plot.x + ((value - xRange.min) / (xRange.max - xRange.min)) * plot.w;
  const sy = (value) => plot.y + plot.h - ((value - yRange.min) / (yRange.max - yRange.min)) * plot.h;

  ctx.save();
  ctx.lineWidth = 1;
  ctx.font = "11px Segoe UI, Arial, sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "top";

  for (const tick of niceTicks(xRange.min, xRange.max, 5)) {
    const x = sx(tick);
    ctx.strokeStyle = "#e9edf2";
    ctx.beginPath();
    ctx.moveTo(x, plot.y);
    ctx.lineTo(x, plot.y + plot.h);
    ctx.stroke();
    ctx.fillStyle = "#5f6b78";
    ctx.fillText(formatTick(tick), x, plot.y + plot.h + 8);
  }

  ctx.textAlign = "right";
  ctx.textBaseline = "middle";
  for (const tick of niceTicks(yRange.min, yRange.max, 5)) {
    const y = sy(tick);
    ctx.strokeStyle = "#e9edf2";
    ctx.beginPath();
    ctx.moveTo(plot.x, y);
    ctx.lineTo(plot.x + plot.w, y);
    ctx.stroke();
    ctx.fillStyle = "#5f6b78";
    ctx.fillText(formatTick(tick), plot.x - 8, y);
  }

  ctx.strokeStyle = "#cfd6df";
  ctx.strokeRect(plot.x, plot.y, plot.w, plot.h);

  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillStyle = "#39424e";
  ctx.fillText(xLabel, plot.x + plot.w / 2, plot.y + plot.h + 30);

  ctx.save();
  ctx.translate(plot.x - 42, plot.y + plot.h / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillText(yLabel, 0, 0);
  ctx.restore();
  ctx.restore();

  return { x: sx, y: sy };
}

function drawZeroLine(ctx, plot, y) {
  if (y < plot.y || y > plot.y + plot.h) {
    return;
  }

  ctx.save();
  ctx.strokeStyle = "#b7bec9";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(plot.x, y);
  ctx.lineTo(plot.x + plot.w, y);
  ctx.stroke();
  ctx.restore();
}

function drawCurve(ctx, points, scales, key, color) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = 1.8;
  ctx.lineJoin = "round";
  ctx.beginPath();

  points.forEach((point, index) => {
    const x = scales.x(point.pointIndex);
    const y = scales.y(point[key]);
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();
  ctx.restore();
}

function drawLegend(ctx, x, y, items) {
  ctx.save();
  ctx.font = "12px Segoe UI, Arial, sans-serif";
  ctx.textBaseline = "middle";

  let cursor = x;
  for (const [label, color] of items) {
    ctx.fillStyle = color;
    ctx.fillText(label, cursor, y);
    cursor += ctx.measureText(label).width + 24;
  }
  ctx.restore();
}

function drawDisplacementArrow(ctx, point, scales, arrowScale) {
  const x1 = scales.x(point.originalX);
  const y1 = scales.y(point.originalZ);
  const x2 = scales.x(point.originalX + point.dx * arrowScale);
  const y2 = scales.y(point.originalZ + point.dz * arrowScale);
  const length = Math.hypot(x2 - x1, y2 - y1);

  if (length < 3) {
    return;
  }

  const head = Math.min(7, Math.max(4, length * 0.32));
  const angle = Math.atan2(y2 - y1, x2 - x1);

  ctx.save();
  ctx.strokeStyle = "#cf3d3a";
  ctx.fillStyle = "#cf3d3a";
  ctx.lineWidth = 1.2;
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.lineTo(x2, y2);
  ctx.stroke();

  ctx.beginPath();
  ctx.moveTo(x2, y2);
  ctx.lineTo(x2 - head * Math.cos(angle - Math.PI / 6), y2 - head * Math.sin(angle - Math.PI / 6));
  ctx.lineTo(x2 - head * Math.cos(angle + Math.PI / 6), y2 - head * Math.sin(angle + Math.PI / 6));
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}

function drawColorbar(ctx, rect, maxValue) {
  ctx.save();
  for (let i = 0; i < rect.h; i += 1) {
    const t = 1 - i / Math.max(1, rect.h - 1);
    ctx.fillStyle = colorRamp(t);
    ctx.fillRect(rect.x, rect.y + i, rect.w, 1);
  }

  ctx.strokeStyle = "#c5cdd8";
  ctx.strokeRect(rect.x, rect.y, rect.w, rect.h);

  ctx.font = "11px Segoe UI, Arial, sans-serif";
  ctx.fillStyle = "#39424e";
  ctx.textAlign = "left";
  ctx.textBaseline = "middle";
  ctx.fillText("dxz (mm)", rect.x + rect.w + 7, rect.y + 4);

  const ticks = [0, 0.25, 0.5, 0.75, 1];
  for (const t of ticks) {
    const y = rect.y + rect.h - t * rect.h;
    ctx.strokeStyle = "#697586";
    ctx.beginPath();
    ctx.moveTo(rect.x + rect.w, y);
    ctx.lineTo(rect.x + rect.w + 4, y);
    ctx.stroke();
    ctx.fillStyle = "#39424e";
    ctx.fillText(formatMm(t * maxValue), rect.x + rect.w + 7, y);
  }
  ctx.restore();
}

function exportPng() {
  const selected = state.cases.find((item) => item.key === state.currentKey);
  if (!selected || !state.blades.length) {
    return;
  }

  const columns = Math.min(3, state.blades.length);
  const cellW = 540;
  const shapeH = 360;
  const profileH = 275;
  const gapX = 28;
  const gapY = 34;
  const marginX = 50;
  const titleH = 70;
  const rows = Math.ceil(state.blades.length / columns);
  const logicalW = marginX * 2 + columns * cellW + (columns - 1) * gapX;
  const logicalH = titleH + rows * (shapeH + gapY) + 28 + rows * (profileH + gapY) + 30;
  const scale = 2;

  const canvas = document.createElement("canvas");
  canvas.width = Math.round(logicalW * scale);
  canvas.height = Math.round(logicalH * scale);
  const ctx = canvas.getContext("2d");
  ctx.scale(scale, scale);
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(0, 0, logicalW, logicalH);

  ctx.fillStyle = "#111827";
  ctx.font = "700 24px Segoe UI, Arial, sans-serif";
  ctx.fillText(`Deformed blade points, tool ${selected.toolLabel} -> map ${selected.mapLabel}`, marginX, 34);
  ctx.fillStyle = "#55616f";
  ctx.font = "13px Segoe UI, Arial, sans-serif";
  ctx.fillText("Blue->red color = dxz, red arrows = displacement vector.", marginX, 56);

  const arrowScale = Number(els.arrowScale.value);
  const opts = { arrowScale, colorMax: state.colorMax };
  const profileTop = titleH + rows * (shapeH + gapY) + 28;

  state.blades.forEach((blade, index) => {
    const col = index % columns;
    const row = Math.floor(index / columns);
    const x = marginX + col * (cellW + gapX);
    const shapeY = titleH + row * (shapeH + gapY);
    const profileY = profileTop + row * (profileH + gapY);

    drawExportTitle(ctx, x, shapeY - 8, `Blade ${blade.bladeId}`, `max dxz=${formatMm(blade.stats.maxDxz)} mm, avg=${formatMm(blade.stats.avgDxz)} mm`);
    drawShapePlot(ctx, { x, y: shapeY + 8, w: cellW, h: shapeH - 8 }, blade, opts);

    drawExportTitle(ctx, x, profileY - 8, `Blade ${blade.bladeId} displacement profile`, `${blade.stats.points} points`);
    drawProfilePlot(ctx, { x, y: profileY + 8, w: cellW, h: profileH - 8 }, blade);
  });

  const link = document.createElement("a");
  link.download = `deformed_blade_points_tool_${selected.toolLabel}_map_${selected.mapLabel}.png`;
  link.href = canvas.toDataURL("image/png");
  link.click();
}

function drawExportTitle(ctx, x, y, title, stat) {
  ctx.save();
  ctx.fillStyle = "#111827";
  ctx.font = "700 17px Segoe UI, Arial, sans-serif";
  ctx.fillText(title, x, y);
  ctx.fillStyle = "#55616f";
  ctx.font = "12px Segoe UI, Arial, sans-serif";
  ctx.fillText(stat, x + 92, y);
  ctx.restore();
}

function setBusy(message) {
  els.dropZone.hidden = false;
  els.dropZone.classList.remove("drag-over");
  els.dropZone.innerHTML = `<div><div class="drop-title">${escapeHtml(message)}</div><div class="drop-subtitle">loading</div></div>`;
  els.summary.hidden = true;
  els.bladeGrid.hidden = true;
  els.exportBtn.disabled = true;
}

function showEmpty(message) {
  els.subtitle.textContent = state.sourceLabel;
  els.dropZone.hidden = false;
  els.dropZone.innerHTML = `<div><div class="drop-title">${escapeHtml(message)}</div><div class="drop-subtitle">DeformedBladePoints TXT</div></div>`;
  els.summary.hidden = true;
  els.bladeGrid.hidden = true;
  els.caseSelect.disabled = true;
  els.caseSelect.innerHTML = "";
  els.exportBtn.disabled = true;
}

function showError(error) {
  els.dropZone.hidden = false;
  els.dropZone.innerHTML = `<div class="error-message">${escapeHtml(error.message || String(error))}</div>`;
  els.summary.hidden = true;
  els.bladeGrid.hidden = true;
  els.exportBtn.disabled = true;
}

function readNumber(cells, index) {
  return Number(cells[index]);
}

function paddedRange(values, padRatio) {
  const finite = values.filter(Number.isFinite);
  let min = finite.length ? Math.min(...finite) : 0;
  let max = finite.length ? Math.max(...finite) : 1;

  if (min === max) {
    const pad = Math.max(1, Math.abs(min) * 0.1);
    min -= pad;
    max += pad;
  } else {
    const pad = (max - min) * padRatio;
    min -= pad;
    max += pad;
  }

  return { min, max };
}

function niceTicks(min, max, count) {
  if (!Number.isFinite(min) || !Number.isFinite(max) || min === max) {
    return [];
  }

  const step = niceStep((max - min) / Math.max(1, count - 1));
  const start = Math.ceil(min / step) * step;
  const ticks = [];

  for (let value = start; value <= max + step * 0.5; value += step) {
    ticks.push(Number(value.toPrecision(12)));
  }

  return ticks;
}

function niceStep(raw) {
  const exponent = Math.floor(Math.log10(raw));
  const base = 10 ** exponent;
  const fraction = raw / base;

  if (fraction <= 1) return base;
  if (fraction <= 2) return 2 * base;
  if (fraction <= 5) return 5 * base;
  return 10 * base;
}

function colorRamp(t) {
  const stops = [
    [0.00, "#2f5ec4"],
    [0.28, "#1099d1"],
    [0.55, "#2bb9ae"],
    [0.78, "#f3d64a"],
    [1.00, "#cf3d3a"]
  ];

  for (let i = 1; i < stops.length; i += 1) {
    const [at, color] = stops[i];
    const [prevAt, prevColor] = stops[i - 1];
    if (t <= at) {
      const local = (t - prevAt) / (at - prevAt);
      return mixHex(prevColor, color, clamp(local, 0, 1));
    }
  }
  return stops[stops.length - 1][1];
}

function mixHex(a, b, t) {
  const ca = hexToRgb(a);
  const cb = hexToRgb(b);
  return `rgb(${Math.round(ca.r + (cb.r - ca.r) * t)}, ${Math.round(ca.g + (cb.g - ca.g) * t)}, ${Math.round(ca.b + (cb.b - ca.b) * t)})`;
}

function hexToRgb(hex) {
  const intValue = Number.parseInt(hex.slice(1), 16);
  return {
    r: (intValue >> 16) & 255,
    g: (intValue >> 8) & 255,
    b: intValue & 255
  };
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function maxOf(items, getValue) {
  if (!items.length) {
    return 0;
  }
  return Math.max(...items.map(getValue).filter(Number.isFinite), 0);
}

function sumOf(items, getValue) {
  return items.reduce((total, item) => {
    const value = getValue(item);
    return total + (Number.isFinite(value) ? value : 0);
  }, 0);
}

function formatAngle(value) {
  return Number(value).toFixed(3);
}

function formatMm(value) {
  if (!Number.isFinite(value)) {
    return "0.000000";
  }
  if (Math.abs(value) >= 1) {
    return value.toFixed(3);
  }
  return value.toFixed(6);
}

function formatTick(value) {
  if (Math.abs(value) >= 100) return value.toFixed(0);
  if (Math.abs(value) >= 10) return value.toFixed(1);
  if (Math.abs(value) >= 1) return value.toFixed(2);
  return value.toFixed(3);
}

function debounce(fn, delay) {
  let timer = 0;
  return (...args) => {
    window.clearTimeout(timer);
    timer = window.setTimeout(() => fn(...args), delay);
  };
}

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, (char) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    "\"": "&quot;",
    "'": "&#039;"
  }[char]));
}
