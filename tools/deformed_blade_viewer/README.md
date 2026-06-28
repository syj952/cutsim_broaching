# Deformed Blade Points Viewer

用于查看 `data/DeformedBladePoints/deformed_blade_points_tool_*_map_*_blade_*.txt` 的静态浏览器工具。

## 打开

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\deformed_blade_viewer\open_viewer.ps1
```

打开后选择 `data\DeformedBladePoints` 文件夹，或把 TXT 文件拖入页面。

## 绘图内容

- 上排：X-Z 刀刃轮廓，按 `dxz` 着色，并叠加红色位移箭头。
- 下排：按 `point_index` 绘制 `dx`、`dz`、`dxz` 位移曲线。
- 默认样本策略：每个 `point_index` 取 `dxz` 最大的样本行。
