# Force Data Viewer

Static browser viewer for current-format `data/Force/force_data_*.txt` files.

## Open

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\force_data_viewer\open_viewer.ps1
```

Then select the `data\Force` folder, or drag current-format `force_data_*.txt` files into the page.

## What It Draws

- One overview image per selected `tool_angle`.
- Each subplot is one `blade_id`.
- X axis is `point_index`.
- Use the toolbar checkboxes to choose `|F|`, `fx`, `fy`, and `fz`.
- Curves show the selected force magnitude and signed direction components in the same image.
- The summary calculates `sum Fx`, `sum Fy`, `sum Fz`, and `|sum F|` across all segments in the selected file.
- The summary also calculates `sum |Fx|`, `sum |Fy|`, and `sum |Fz|` so signed cancellation can be separated from total component load.
- The trend chart shows signed total force components across all loaded `force_data_*.txt` files.
- The blade comparison chart uses the `Blade` selector to compare one blade's signed force totals across all loaded files.
- Each blade subplot also prints that blade's directional force sums.
- `Export PNG` saves the current overview image.

Files without a `tool_angle` header are skipped.
