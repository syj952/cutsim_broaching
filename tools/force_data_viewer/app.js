"use strict";

const FILE_RE = /^force_data_(-?\d+(?:\.\d+)?)\.txt$/i;

const REQUIRED_COLUMNS = [
  "tool_angle", "blade_id", "point_index", "force_position_id",
  "x", "y", "z", "fx", "fy", "fz", "force_magnitude", "force_cuth",
  "k_fc", "k_fcn", "force_t_x", "force_t_y", "force_t_z",
  "force_f_x", "force_f_y", "force_f_z"
];

const PLANE_AXES = {
  xz: { h: "x", v: "z", fh: "fx", fv: "fz", hLabel: "X (mm)", vLabel: "Z (mm)" },
  xy: { h: "x", v: "y", fh: "fx", fv: "fy", hLabel: "X (mm)", vLabel: "Y (mm)" },
  yz: { h: "y", v: "z", fh: "fy", fv: "fz", hLabel: "Y (mm)", vLabel: "Z (mm)" }
};

const FORCE_SERIES = [
  { id: "magnitude", key: "forceMagnitude", label: "|F|", color: "#111827", width: 2.2 },
  { id: "fx", key: "fx", label: "fx", color: "#2563eb", width: 1.5 },
  { id: "fy", key: "fy", label: "fy", color: "#f97316", width: 1.5 },
  { id: "fz", key: "fz", label: "fz", color: "#16a34a", width: 1.5 }
];

const els = {
  openDirBtn: document.getElementById("openDirBtn"),
  openFilesBtn: document.getElementById("openFilesBtn"),
  dirInput: document.getElementById("dirInput"),
  fileInput: document.getElementById("fileInput"),
  caseSelect: document.getElementById("caseSelect"),
  bladeCompareSelect: document.getElementById("bladeCompareSelect"),
  showMagnitude: document.getElementById("showMagnitude"),
  showFx: document.getElementById("showFx"),
  showFy: document.getElementById("showFy"),
  showFz: document.getElementById("showFz"),
  exportBtn: document.getElementById("exportBtn"),
  subtitle: document.getElementById("subtitle"),
  dropZone: document.getElementById("dropZone"),
  summary: document.getElementById("summary"),
  trendPanel: document.getElementById("trendPanel"),
  trendTitle: document.getElementById("trendTitle"),
  trendCanvas: document.getElementById("trendCanvas"),
  bladeComparePanel: document.getElementById("bladeComparePanel"),
  bladeCompareTitle: document.getElementById("bladeCompareTitle"),
  bladeCompareNote: document.getElementById("bladeCompareNote"),
  bladeCompareCanvas: document.getElementById("bladeCompareCanvas"),
  figurePanel: document.getElementById("figurePanel"),
  figureTitle: document.getElementById("figureTitle"),
  figureNote: document.getElementById("figureNote"),
  overviewCanvas: document.getElementById("overviewCanvas"),
  bladeGrid: document.getElementById("bladeGrid")
};

const state = {
  cases: [],
  currentKey: "",
  currentCase: null,
  blades: [],
  drawTargets: [],
  parsedCache: new WeakMap(),
  sourceLabel: "data/Force",
  skipped: 0,
  maxMagnitude: 1,
  compareBladeId: null
};

els.openDirBtn.addEventListener("click", () => els.dirInput.click());
els.openFilesBtn.addEventListener("click", () => els.fileInput.click());
els.dirInput.addEventListener("change", () => loadFiles(Array.from(els.dirInput.files), "data/Force"));
els.fileInput.addEventListener("change", () => loadFiles(Array.from(els.fileInput.files), "selected TXT files"));
els.caseSelect.addEventListener("change", () => selectCase(els.caseSelect.value));
els.bladeCompareSelect.addEventListener("change", () => {
  state.compareBladeId = Number(els.bladeCompareSelect.value);
  renderBladeCompareTitle();
  drawVisible();
});
els.exportBtn.addEventListener("click", exportPng);
[els.showMagnitude, els.showFx, els.showFy, els.showFz].forEach((input) => {
  input.addEventListener("change", () => {
    ensureAnySeriesSelected();
    renderTrendTitle();
    renderBladeCompareTitle();
    renderOverviewTitle();
    drawVisible();
  });
});

["dragenter", "dragover"].forEach((name) => {
  window.addEventListener(name, (event) => {
    event.preventDefault();
    els.dropZone.classList.add("drag-over");
  });
});

["dragleave", "drop"].forEach((name) => {
  window.addEventListener(name, async (event) => {
    event.preventDefault();
    if (name === "drop") {
      const files = await collectDroppedFiles(event.dataTransfer);
      await loadFiles(files, "dropped files");
    }
    els.dropZone.classList.remove("drag-over");
  });
});

window.addEventListener("resize", debounce(drawVisible, 120));

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
  const candidates = files
    .map((file) => ({ file, match: FILE_RE.exec(file.name) }))
    .filter((item) => item.match);

  state.sourceLabel = sourceLabel || "selected files";
  state.currentKey = "";
  state.currentCase = null;
  state.blades = [];
  state.drawTargets = [];
  state.skipped = 0;
  state.compareBladeId = null;

  if (!candidates.length) {
    showEmpty("No force_data_*.txt files found");
    return;
  }

  setBusy(`Reading ${candidates.length} force files`);

  const cases = [];
  for (const item of candidates) {
    try {
      const parsed = await parseForceFile(item.file);
      if (!parsed) {
        state.skipped += 1;
        continue;
      }
      cases.push({
        key: parsed.angleLabel,
        angle: parsed.angle,
        angleLabel: parsed.angleLabel,
        file: item.file,
        parsed
      });
    } catch (error) {
      state.skipped += 1;
      console.warn(error);
    }
  }

  state.cases = cases.sort((a, b) => a.angle - b.angle);

  if (!state.cases.length) {
    showEmpty("No current-format force_data files found");
    return;
  }

  renderCaseOptions();
  renderBladeCompareOptions();
  const preferred = state.cases.find((item) => item.angleLabel === "100.00");
  await selectCase((preferred || state.cases[state.cases.length - 1]).key);
}

async function parseForceFile(file) {
  if (state.parsedCache.has(file)) {
    return state.parsedCache.get(file);
  }

  const text = await file.text();
  const lines = text.split(/\r?\n/).filter((line) => line.trim().length > 0);
  if (!lines.length) {
    state.parsedCache.set(file, null);
    return null;
  }

  const header = lines[0].trim().split(/\s+/);
  if (header[0] !== "tool_angle") {
    state.parsedCache.set(file, null);
    return null;
  }

  const index = Object.fromEntries(header.map((name, idx) => [name, idx]));
  for (const column of REQUIRED_COLUMNS) {
    if (index[column] === undefined) {
      throw new Error(`${file.name}: missing column ${column}`);
    }
  }

  const rows = [];
  for (const line of lines.slice(1)) {
    const cells = line.trim().split(/\s+/);
    if (cells.length < REQUIRED_COLUMNS.length) {
      continue;
    }

    const row = {
      toolAngle: readNumber(cells, index.tool_angle),
      bladeId: readNumber(cells, index.blade_id),
      pointIndex: readNumber(cells, index.point_index),
      forcePositionId: readNumber(cells, index.force_position_id),
      x: readNumber(cells, index.x),
      y: readNumber(cells, index.y),
      z: readNumber(cells, index.z),
      fx: readNumber(cells, index.fx),
      fy: readNumber(cells, index.fy),
      fz: readNumber(cells, index.fz),
      forceMagnitude: readNumber(cells, index.force_magnitude),
      forceCuth: readNumber(cells, index.force_cuth),
      kFc: readNumber(cells, index.k_fc),
      kFcn: readNumber(cells, index.k_fcn),
      forceTx: readNumber(cells, index.force_t_x),
      forceTy: readNumber(cells, index.force_t_y),
      forceTz: readNumber(cells, index.force_t_z),
      forceFx: readNumber(cells, index.force_f_x),
      forceFy: readNumber(cells, index.force_f_y),
      forceFz: readNumber(cells, index.force_f_z)
    };

    if (!Number.isFinite(row.forceMagnitude)) {
      row.forceMagnitude = Math.hypot(row.fx, row.fy, row.fz);
    }

    if (Number.isFinite(row.bladeId) && Number.isFinite(row.pointIndex)) {
      rows.push(row);
    }
  }

  if (!rows.length) {
    state.parsedCache.set(file, null);
    return null;
  }

  const angle = Number.isFinite(rows[0].toolAngle) ? rows[0].toolAngle : angleFromName(file.name);
  const parsed = {
    fileName: file.name,
    angle,
    angleLabel: Number(angle).toFixed(2),
    rows
  };
  state.parsedCache.set(file, parsed);
  return parsed;
}

function renderCaseOptions() {
  els.caseSelect.innerHTML = "";
  for (const item of state.cases) {
    const bladeCount = new Set(item.parsed.rows.map((row) => row.bladeId)).size;
    const option = document.createElement("option");
    option.value = item.key;
    option.textContent = `${item.angleLabel} (${bladeCount} blades, ${item.parsed.rows.length} segments)`;
    els.caseSelect.appendChild(option);
  }
  els.caseSelect.disabled = false;
}

function renderBladeCompareOptions() {
  const bladeIds = Array.from(new Set(state.cases.flatMap((item) => item.parsed.rows.map((row) => row.bladeId))))
    .filter(Number.isFinite)
    .sort((a, b) => a - b);

  els.bladeCompareSelect.innerHTML = "";
  for (const bladeId of bladeIds) {
    const option = document.createElement("option");
    option.value = String(bladeId);
    option.textContent = `Blade ${bladeId}`;
    els.bladeCompareSelect.appendChild(option);
  }

  state.compareBladeId = bladeIds.length ? bladeIds[0] : null;
  if (state.compareBladeId !== null) {
    els.bladeCompareSelect.value = String(state.compareBladeId);
  }
  els.bladeCompareSelect.disabled = bladeIds.length === 0;
}

async function selectCase(key) {
  const selected = state.cases.find((item) => item.key === key);
  if (!selected) {
    return;
  }

  state.currentKey = key;
  state.currentCase = selected;
  els.caseSelect.value = key;
  state.blades = buildBlades(selected.parsed.rows);
  state.maxMagnitude = Math.max(1e-12, ...state.blades.map((blade) => blade.stats.maxMagnitude));
  renderData();
}

function buildBlades(rows) {
  const map = new Map();
  for (const row of rows) {
    if (!map.has(row.bladeId)) {
      map.set(row.bladeId, []);
    }
    map.get(row.bladeId).push(row);
  }

  return Array.from(map.entries())
    .sort((a, b) => a[0] - b[0])
    .map(([bladeId, bladeRows]) => {
      const rowsSorted = bladeRows.slice().sort((a, b) => a.pointIndex - b.pointIndex);
      const maxMagnitude = maxOf(rowsSorted, (row) => row.forceMagnitude);
      const avgMagnitude = rowsSorted.length ? sumOf(rowsSorted, (row) => row.forceMagnitude) / rowsSorted.length : 0;
      const maxCuth = maxOf(rowsSorted, (row) => row.forceCuth);
      const totals = calculateForceTotals(rowsSorted);
      return {
        bladeId,
        rows: rowsSorted,
        stats: {
          count: rowsSorted.length,
          maxMagnitude,
          avgMagnitude,
          maxCuth,
          ...totals
        }
      };
    });
}

function renderData() {
  const selected = state.currentCase;
  els.subtitle.textContent = `${state.sourceLabel} | force_data_${selected.angleLabel}.txt`;
  els.dropZone.hidden = true;
  els.summary.hidden = false;
  els.trendPanel.hidden = false;
  els.bladeComparePanel.hidden = false;
  els.figurePanel.hidden = false;
  els.bladeGrid.hidden = false;
  els.exportBtn.disabled = state.blades.length === 0;

  renderSummary();
  updateOverviewHeight();
  renderTrendTitle();
  renderBladeCompareTitle();
  renderOverviewTitle();
  els.bladeGrid.hidden = true;
  requestAnimationFrame(drawVisible);
}

function renderSummary() {
  const allRows = state.blades.flatMap((blade) => blade.rows);
  const maxForce = maxOf(allRows, (row) => row.forceMagnitude);
  const avgForce = allRows.length ? sumOf(allRows, (row) => row.forceMagnitude) / allRows.length : 0;
  const maxCuth = maxOf(allRows, (row) => row.forceCuth);
  const totals = calculateForceTotals(allRows);

  els.summary.innerHTML = "";
  [
    ["tool angle", state.currentCase.angleLabel],
    ["blades", String(state.blades.length)],
    ["segments", String(allRows.length)],
    ["max force", formatForce(maxForce)],
    ["avg force", formatForce(avgForce)],
    ["sum Fx", formatForce(totals.sumFx)],
    ["sum Fy", formatForce(totals.sumFy)],
    ["sum Fz", formatForce(totals.sumFz)],
    ["|sum F|", formatForce(totals.sumResultant)],
    ["sum |Fx|", formatForce(totals.sumAbsFx)],
    ["sum |Fy|", formatForce(totals.sumAbsFy)],
    ["sum |Fz|", formatForce(totals.sumAbsFz)],
    ["max cuth", `${formatSmall(maxCuth)} mm`],
    ["skipped files", String(state.skipped)]
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
    empty.textContent = "No blade rows in the selected file.";
    els.bladeGrid.appendChild(empty);
    return;
  }

  const mapGrid = document.createElement("div");
  mapGrid.className = "plot-grid";
  const profileGrid = document.createElement("div");
  profileGrid.className = "plot-grid";

  for (const blade of state.blades) {
    const mapPanel = createPlotPanel(
      `Blade ${blade.bladeId} force map`,
      `max=${formatForce(blade.stats.maxMagnitude)}, cuth=${formatSmall(blade.stats.maxCuth)} mm`,
      "map-canvas"
    );
    mapGrid.appendChild(mapPanel.panel);
    state.drawTargets.push({ type: "map", canvas: mapPanel.canvas, blade });

    const profilePanel = createPlotPanel(
      `Blade ${blade.bladeId} force profile`,
      `${blade.stats.count} segments`,
      "profile-canvas"
    );
    profileGrid.appendChild(profilePanel.panel);
    state.drawTargets.push({ type: "profile", canvas: profilePanel.canvas, blade });
  }

  els.bladeGrid.appendChild(mapGrid);
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
  if (!els.trendPanel.hidden) {
    const surface = setupCanvas(els.trendCanvas);
    if (surface) {
      drawTrendFigure(surface.ctx, { x: 0, y: 0, w: surface.w, h: surface.h });
    }
  }

  if (!els.bladeComparePanel.hidden) {
    const surface = setupCanvas(els.bladeCompareCanvas);
    if (surface) {
      drawBladeCompareFigure(surface.ctx, { x: 0, y: 0, w: surface.w, h: surface.h });
    }
  }

  if (!els.figurePanel.hidden) {
    const surface = setupCanvas(els.overviewCanvas);
    if (surface) {
      drawOverviewFigure(surface.ctx, { x: 0, y: 0, w: surface.w, h: surface.h });
    }
  }

  if (!state.drawTargets.length) {
    return;
  }

  for (const target of state.drawTargets) {
    const surface = setupCanvas(target.canvas);
    if (!surface) {
      continue;
    }

    const rect = { x: 0, y: 0, w: surface.w, h: surface.h };
    if (target.type === "map") {
      drawForceMap(surface.ctx, rect, target.blade, {
        plane: "xz",
        arrowScale: 0.55,
        maxMagnitude: state.maxMagnitude
      });
    } else {
      drawForceProfile(surface.ctx, rect, target.blade);
    }
  }
}

function updateOverviewHeight() {
  const columns = Math.min(3, Math.max(1, state.blades.length));
  const rows = Math.ceil(state.blades.length / columns);
  const height = 82 + rows * 285;
  els.overviewCanvas.style.height = `${height}px`;
}

function renderOverviewTitle() {
  if (!state.currentCase) {
    return;
  }

  const labels = getSelectedSeries().map((item) => item.label).join(", ");
  els.figureTitle.textContent = `Force profile overview, tool angle ${state.currentCase.angleLabel}`;
  els.figureNote.textContent = `Each blade is one panel. Showing: ${labels}.`;
}

function renderTrendTitle() {
  const labels = getSelectedSeries().map((item) => item.id === "magnitude" ? "|sum F|" : `sum ${item.label}`).join(", ");
  els.trendTitle.textContent = `Signed total force trend (${labels})`;
}

function renderBladeCompareTitle() {
  const bladeId = state.compareBladeId;
  if (bladeId === null || !Number.isFinite(bladeId)) {
    els.bladeCompareTitle.textContent = "Blade force comparison";
    els.bladeCompareNote.textContent = "No blade data loaded.";
    return;
  }

  const labels = getSelectedSeries().map((item) => item.id === "magnitude" ? "|sum F|" : `sum ${item.label}`).join(", ");
  const points = getBladeComparePoints();
  const current = points.find((point) => point.key === state.currentKey);
  els.bladeCompareTitle.textContent = `Blade ${bladeId} force comparison (${labels})`;
  els.bladeCompareNote.textContent = current
    ? `${points.length}/${state.cases.length} files contain Blade ${bladeId}. Current angle ${state.currentCase.angleLabel}: ${forceTotalsText(current.totals)}.`
    : `${points.length}/${state.cases.length} files contain Blade ${bladeId}. Blade ${bladeId} is not present in the selected angle.`;
}

function drawBladeCompareFigure(ctx, outer) {
  fillPanel(ctx, outer);

  const points = getBladeComparePoints();
  if (!points.length) {
    drawPlotMessage(ctx, outer, "No rows for the selected blade.");
    return;
  }

  const selectedSeries = getSelectedSeries();
  const values = points.flatMap((point) => selectedSeries.map((series) => trendValue(point.totals, series)));
  const xRange = paddedRange(state.cases.map((item) => item.angle), 0.04);
  const yRange = paddedRange([...values, 0], 0.12);
  const plot = {
    x: outer.x + 62,
    y: outer.y + 20,
    w: Math.max(60, outer.w - 96),
    h: Math.max(60, outer.h - 76)
  };

  const scales = drawAxes(ctx, plot, xRange, yRange, "tool angle", "selected blade total force");
  drawZeroLine(ctx, plot, scales.y(0));
  drawCurrentAngleLine(ctx, plot, scales, state.currentCase.angle);

  for (const series of selectedSeries) {
    drawTrendCurve(ctx, points, scales, series);
  }

  drawLegend(ctx, plot.x + 10, plot.y + 14, selectedSeries.map((series) => [
    series.id === "magnitude" ? "|sum F|" : `sum ${series.label}`,
    series.color
  ]));
}

function getBladeComparePoints() {
  const bladeId = state.compareBladeId;
  if (bladeId === null || !Number.isFinite(bladeId)) {
    return [];
  }

  return state.cases
    .map((item) => {
      const rows = item.parsed.rows.filter((row) => row.bladeId === bladeId);
      if (!rows.length) {
        return null;
      }
      return {
        key: item.key,
        angle: item.angle,
        count: rows.length,
        totals: calculateForceTotals(rows)
      };
    })
    .filter(Boolean)
    .filter((point) => Number.isFinite(point.angle))
    .sort((a, b) => a.angle - b.angle);
}

function drawTrendFigure(ctx, outer) {
  fillPanel(ctx, outer);

  const points = getTrendPoints();
  if (!points.length) {
    return;
  }

  const selectedSeries = getSelectedSeries();
  const values = points.flatMap((point) => selectedSeries.map((series) => trendValue(point.totals, series)));
  const xRange = paddedRange(points.map((point) => point.angle), 0.04);
  const yRange = paddedRange([...values, 0], 0.12);
  const plot = {
    x: outer.x + 62,
    y: outer.y + 18,
    w: Math.max(60, outer.w - 96),
    h: Math.max(60, outer.h - 72)
  };

  const scales = drawAxes(ctx, plot, xRange, yRange, "tool angle", "signed total force");
  drawZeroLine(ctx, plot, scales.y(0));
  drawCurrentAngleLine(ctx, plot, scales, state.currentCase.angle);

  for (const series of selectedSeries) {
    drawTrendCurve(ctx, points, scales, series);
  }

  drawLegend(ctx, plot.x + 10, plot.y + 14, selectedSeries.map((series) => [
    series.id === "magnitude" ? "|sum F|" : `sum ${series.label}`,
    series.color
  ]));
}

function getTrendPoints() {
  return state.cases
    .map((item) => ({
      key: item.key,
      angle: item.angle,
      totals: calculateForceTotals(item.parsed.rows)
    }))
    .filter((point) => Number.isFinite(point.angle))
    .sort((a, b) => a.angle - b.angle);
}

function trendValue(totals, series) {
  if (series.id === "magnitude") return totals.sumResultant;
  if (series.id === "fx") return totals.sumFx;
  if (series.id === "fy") return totals.sumFy;
  if (series.id === "fz") return totals.sumFz;
  return 0;
}

function drawCurrentAngleLine(ctx, plot, scales, angle) {
  if (!Number.isFinite(angle)) {
    return;
  }

  const x = scales.x(angle);
  if (x < plot.x || x > plot.x + plot.w) {
    return;
  }

  ctx.save();
  ctx.strokeStyle = "#9aa5b1";
  ctx.setLineDash([4, 4]);
  ctx.beginPath();
  ctx.moveTo(x, plot.y);
  ctx.lineTo(x, plot.y + plot.h);
  ctx.stroke();
  ctx.restore();
}

function drawTrendCurve(ctx, points, scales, series) {
  ctx.save();
  ctx.strokeStyle = series.color;
  ctx.fillStyle = series.color;
  ctx.lineWidth = series.id === "magnitude" ? 2.2 : 1.7;
  ctx.lineJoin = "round";
  ctx.beginPath();

  points.forEach((point, index) => {
    const x = scales.x(point.angle);
    const y = scales.y(trendValue(point.totals, series));
    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });
  ctx.stroke();

  for (const point of points) {
    const x = scales.x(point.angle);
    const y = scales.y(trendValue(point.totals, series));
    const radius = point.key === state.currentKey ? 4 : 2.6;
    ctx.beginPath();
    ctx.arc(x, y, radius, 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.restore();
}

function drawOverviewFigure(ctx, outer) {
  fillPanel(ctx, outer);

  if (!state.blades.length) {
    return;
  }

  const columns = Math.min(3, state.blades.length);
  const rows = Math.ceil(state.blades.length / columns);
  const margin = { left: 48, right: 28, top: 58, bottom: 28 };
  const gapX = 30;
  const gapY = 34;
  const cellW = (outer.w - margin.left - margin.right - gapX * (columns - 1)) / columns;
  const cellH = (outer.h - margin.top - margin.bottom - gapY * (rows - 1)) / rows;
  const selectedSeries = getSelectedSeries();
  const allRows = state.blades.flatMap((blade) => blade.rows);
  const yRange = paddedRange(allRows.flatMap((row) => [
    ...selectedSeries.map((series) => row[series.key]),
    0
  ]), 0.12);

  ctx.save();
  ctx.fillStyle = "#111827";
  ctx.font = "700 22px Segoe UI, Arial, sans-serif";
  ctx.fillText(`Force data, tool angle ${state.currentCase.angleLabel}`, outer.x + 18, outer.y + 28);
  ctx.fillStyle = "#55616f";
  ctx.font = "12px Segoe UI, Arial, sans-serif";
  ctx.fillText(`Showing ${selectedSeries.map((item) => item.label).join(", ")} | ${forceTotalsText(calculateForceTotals(allRows))}`, outer.x + 18, outer.y + 48);
  ctx.restore();

  state.blades.forEach((blade, index) => {
    const col = index % columns;
    const row = Math.floor(index / columns);
    const x = outer.x + margin.left + col * (cellW + gapX);
    const y = outer.y + margin.top + row * (cellH + gapY);
    drawBladeProfilePanel(ctx, { x, y, w: cellW, h: cellH }, blade, yRange, selectedSeries);
  });
}

function drawBladeProfilePanel(ctx, outer, blade, sharedYRange, selectedSeries) {
  ctx.save();
  ctx.fillStyle = "#111827";
  ctx.font = "700 16px Segoe UI, Arial, sans-serif";
  ctx.fillText(`Blade ${blade.bladeId} force profile`, outer.x, outer.y);
  ctx.fillStyle = "#55616f";
  ctx.font = "12px Segoe UI, Arial, sans-serif";
  ctx.fillText(`max=${formatForce(blade.stats.maxMagnitude)}, avg=${formatForce(blade.stats.avgMagnitude)}`, outer.x + 155, outer.y);
  ctx.fillText(forceTotalsText(blade.stats), outer.x, outer.y + 15);
  ctx.restore();

  const rows = blade.rows;
  const xRange = paddedRange(rows.map((item) => item.pointIndex), 0.02);
  const plot = {
    x: outer.x + 2,
    y: outer.y + 34,
    w: Math.max(40, outer.w - 8),
    h: Math.max(40, outer.h - 60)
  };

  const scales = drawAxes(ctx, plot, xRange, sharedYRange, "point index", "force");
  drawZeroLine(ctx, plot, scales.y(0));
  for (const series of selectedSeries) {
    drawCurve(ctx, rows, scales, series.key, series.color, series.width);
  }
  drawLegend(ctx, plot.x + 10, plot.y + 12, selectedSeries.map((series) => [series.label, series.color]));
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

function drawForceMap(ctx, outer, blade, opts) {
  const rows = blade.rows;
  fillPanel(ctx, outer);
  if (!rows.length) {
    return;
  }

  const axes = PLANE_AXES[opts.plane] || PLANE_AXES.xz;
  const forceMax = Math.max(1e-9, maxOf(rows, (row) => Math.hypot(row[axes.fh], row[axes.fv])));
  const coordScale = coordinateScale(rows, axes, opts.arrowScale, forceMax);
  const hRange = paddedRange(rows.flatMap((row) => [
    row[axes.h],
    row[axes.h] + row[axes.fh] * coordScale
  ]), 0.08);
  const vRange = paddedRange(rows.flatMap((row) => [
    row[axes.v],
    row[axes.v] + row[axes.fv] * coordScale
  ]), 0.08);

  const plot = {
    x: outer.x + 56,
    y: outer.y + 18,
    w: Math.max(40, outer.w - 128),
    h: Math.max(40, outer.h - 66)
  };

  const scales = drawAxes(ctx, plot, hRange, vRange, axes.hLabel, axes.vLabel);

  ctx.save();
  ctx.lineWidth = 1.4;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  for (let i = 1; i < rows.length; i += 1) {
    const a = rows[i - 1];
    const b = rows[i];
    const t = clamp(((a.forceMagnitude + b.forceMagnitude) * 0.5) / opts.maxMagnitude, 0, 1);
    ctx.strokeStyle = colorRamp(t);
    ctx.beginPath();
    ctx.moveTo(scales.x(a[axes.h]), scales.y(a[axes.v]));
    ctx.lineTo(scales.x(b[axes.h]), scales.y(b[axes.v]));
    ctx.stroke();
  }
  ctx.restore();

  const every = Math.max(1, Math.ceil(rows.length / 38));
  for (let i = 0; i < rows.length; i += every) {
    drawForceArrow(ctx, rows[i], scales, axes, coordScale);
  }

  drawPointMarkers(ctx, rows, scales, axes, opts.maxMagnitude);
  drawColorbar(ctx, {
    x: outer.x + outer.w - 48,
    y: plot.y + 22,
    w: 14,
    h: Math.max(80, plot.h - 46)
  }, opts.maxMagnitude);
}

function drawForceProfile(ctx, outer, blade) {
  const rows = blade.rows;
  fillPanel(ctx, outer);
  if (!rows.length) {
    return;
  }

  const selectedSeries = getSelectedSeries();
  const xRange = paddedRange(rows.map((row) => row.pointIndex), 0.02);
  const yRange = paddedRange(rows.flatMap((row) => [
    ...selectedSeries.map((series) => row[series.key]),
    0
  ]), 0.12);

  const plot = {
    x: outer.x + 56,
    y: outer.y + 24,
    w: Math.max(40, outer.w - 78),
    h: Math.max(40, outer.h - 70)
  };

  const scales = drawAxes(ctx, plot, xRange, yRange, "point index", "force");
  drawZeroLine(ctx, plot, scales.y(0));

  for (const series of selectedSeries) {
    drawCurve(ctx, rows, scales, series.key, series.color, series.width);
  }
  drawLegend(ctx, plot.x + 10, plot.y + 12, selectedSeries.map((series) => [series.label, series.color]));
}

function getSelectedSeries() {
  const selected = [];
  if (els.showMagnitude.checked) selected.push(FORCE_SERIES[0]);
  if (els.showFx.checked) selected.push(FORCE_SERIES[1]);
  if (els.showFy.checked) selected.push(FORCE_SERIES[2]);
  if (els.showFz.checked) selected.push(FORCE_SERIES[3]);
  return selected.length ? selected : [FORCE_SERIES[0]];
}

function ensureAnySeriesSelected() {
  if (!els.showMagnitude.checked && !els.showFx.checked && !els.showFy.checked && !els.showFz.checked) {
    els.showMagnitude.checked = true;
  }
}

function calculateForceTotals(rows) {
  const sumFx = sumOf(rows, (row) => row.fx);
  const sumFy = sumOf(rows, (row) => row.fy);
  const sumFz = sumOf(rows, (row) => row.fz);
  const sumAbsFx = sumOf(rows, (row) => Math.abs(row.fx));
  const sumAbsFy = sumOf(rows, (row) => Math.abs(row.fy));
  const sumAbsFz = sumOf(rows, (row) => Math.abs(row.fz));
  return {
    sumFx,
    sumFy,
    sumFz,
    sumResultant: Math.hypot(sumFx, sumFy, sumFz),
    sumAbsFx,
    sumAbsFy,
    sumAbsFz
  };
}

function forceTotalsText(totals) {
  return `sum Fx=${formatForceValue(totals.sumFx)}, sum Fy=${formatForceValue(totals.sumFy)}, sum Fz=${formatForceValue(totals.sumFz)}, |sum F|=${formatForceValue(totals.sumResultant)} N`;
}

function coordinateScale(rows, axes, arrowScale, forceMax) {
  const hRange = rangeSpan(rows.map((row) => row[axes.h]));
  const vRange = rangeSpan(rows.map((row) => row[axes.v]));
  const coordSpan = Math.max(hRange, vRange, 1);
  return arrowScale * coordSpan / forceMax * 0.18;
}

function fillPanel(ctx, outer) {
  ctx.save();
  ctx.fillStyle = "#ffffff";
  ctx.fillRect(outer.x, outer.y, outer.w, outer.h);
  ctx.restore();
}

function drawPlotMessage(ctx, outer, message) {
  ctx.save();
  ctx.fillStyle = "#64707d";
  ctx.font = "13px Segoe UI, Arial, sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(message, outer.x + outer.w / 2, outer.y + outer.h / 2);
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

function drawCurve(ctx, rows, scales, key, color, width) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.lineJoin = "round";
  ctx.beginPath();

  rows.forEach((row, index) => {
    const x = scales.x(row.pointIndex);
    const y = scales.y(row[key]);
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

function drawPointMarkers(ctx, rows, scales, axes, maxMagnitude) {
  ctx.save();
  for (const row of rows) {
    const t = clamp(row.forceMagnitude / maxMagnitude, 0, 1);
    ctx.fillStyle = colorRamp(t);
    ctx.beginPath();
    ctx.arc(scales.x(row[axes.h]), scales.y(row[axes.v]), 2.5, 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.restore();
}

function drawForceArrow(ctx, row, scales, axes, coordScale) {
  const x1 = scales.x(row[axes.h]);
  const y1 = scales.y(row[axes.v]);
  const x2 = scales.x(row[axes.h] + row[axes.fh] * coordScale);
  const y2 = scales.y(row[axes.v] + row[axes.fv] * coordScale);
  const length = Math.hypot(x2 - x1, y2 - y1);

  if (length < 3) {
    return;
  }

  const head = Math.min(8, Math.max(4, length * 0.32));
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
  ctx.fillText("|F|", rect.x + rect.w + 7, rect.y + 4);

  const ticks = [0, 0.25, 0.5, 0.75, 1];
  for (const t of ticks) {
    const y = rect.y + rect.h - t * rect.h;
    ctx.strokeStyle = "#697586";
    ctx.beginPath();
    ctx.moveTo(rect.x + rect.w, y);
    ctx.lineTo(rect.x + rect.w + 4, y);
    ctx.stroke();
    ctx.fillStyle = "#39424e";
    ctx.fillText(formatForceValue(t * maxValue), rect.x + rect.w + 7, y);
  }
  ctx.restore();
}

function exportPng() {
  if (!state.currentCase || !state.blades.length) {
    return;
  }

  const columns = Math.min(3, state.blades.length);
  const rows = Math.ceil(state.blades.length / columns);
  const logicalW = Math.max(1100, columns * 540 + 130);
  const logicalH = 82 + rows * 285;
  const scale = 2;

  const canvas = document.createElement("canvas");
  canvas.width = Math.round(logicalW * scale);
  canvas.height = Math.round(logicalH * scale);
  const ctx = canvas.getContext("2d");
  ctx.scale(scale, scale);
  drawOverviewFigure(ctx, { x: 0, y: 0, w: logicalW, h: logicalH });

  const link = document.createElement("a");
  link.download = `force_profile_overview_${state.currentCase.angleLabel}.png`;
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
  ctx.fillText(stat, x + 150, y);
  ctx.restore();
}

function setBusy(message) {
  els.dropZone.hidden = false;
  els.dropZone.classList.remove("drag-over");
  els.dropZone.innerHTML = `<div><div class="drop-title">${escapeHtml(message)}</div><div class="drop-subtitle">loading</div></div>`;
  els.summary.hidden = true;
  els.trendPanel.hidden = true;
  els.bladeComparePanel.hidden = true;
  els.figurePanel.hidden = true;
  els.bladeGrid.hidden = true;
  els.bladeCompareSelect.disabled = true;
  els.exportBtn.disabled = true;
}

function showEmpty(message) {
  els.subtitle.textContent = state.sourceLabel;
  els.dropZone.hidden = false;
  els.dropZone.innerHTML = `<div><div class="drop-title">${escapeHtml(message)}</div><div class="drop-subtitle">Only current-format files with a tool_angle header are used.</div></div>`;
  els.summary.hidden = true;
  els.trendPanel.hidden = true;
  els.bladeComparePanel.hidden = true;
  els.figurePanel.hidden = true;
  els.bladeGrid.hidden = true;
  els.caseSelect.disabled = true;
  els.caseSelect.innerHTML = "";
  els.bladeCompareSelect.disabled = true;
  els.bladeCompareSelect.innerHTML = "";
  els.exportBtn.disabled = true;
}

function readNumber(cells, index) {
  return Number(cells[index]);
}

function angleFromName(fileName) {
  const match = FILE_RE.exec(fileName);
  return match ? Number(match[1]) : 0;
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

function rangeSpan(values) {
  const finite = values.filter(Number.isFinite);
  if (!finite.length) {
    return 1;
  }
  return Math.max(...finite) - Math.min(...finite);
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

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function formatForce(value) {
  return `${formatForceValue(value)} N`;
}

function formatForceValue(value) {
  if (!Number.isFinite(value)) {
    return "0.0";
  }
  if (Math.abs(value) >= 1000) {
    return value.toFixed(0);
  }
  if (Math.abs(value) >= 100) {
    return value.toFixed(1);
  }
  return value.toFixed(2);
}

function formatSmall(value) {
  if (!Number.isFinite(value)) {
    return "0.000000";
  }
  if (Math.abs(value) >= 1) {
    return value.toFixed(3);
  }
  return value.toFixed(6);
}

function formatTick(value) {
  if (Math.abs(value) >= 1000) return value.toFixed(0);
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
