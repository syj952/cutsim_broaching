param(
    [int]$Mode = 0,
    [int]$Component = -1,
    [switch]$LegacyGL,
    [switch]$Wait
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$glvis = "E:\MFEM\glvis-windows\glvis.exe"
$modeFile = Join-Path $projectRoot ("mode_{0:D2}.000000" -f $Mode)
$meshFile = Join-Path $projectRoot "mesh.000000"

if (-not (Test-Path $glvis)) {
    throw "GLVis executable not found: $glvis"
}

if (-not (Test-Path $meshFile)) {
    throw "MFEM mesh file not found: $meshFile"
}

if (-not (Test-Path $modeFile)) {
    throw "MFEM grid function file not found: $modeFile"
}

$arguments = @("-m", $meshFile, "-g", $modeFile)

if ($Component -ge 0) {
    $arguments += @("-gc", $Component)
}

if ($LegacyGL) {
    $arguments += "-oldgl"
}

if ($Wait) {
    & $glvis @arguments
}
else {
    Start-Process -FilePath $glvis -ArgumentList $arguments -WorkingDirectory $projectRoot
}
