param(
    [int]$Component = -1,
    [switch]$LegacyGL,
    [switch]$Sequential
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$glvis = "E:\MFEM\glvis-windows\glvis.exe"
$meshFile = Join-Path $projectRoot "mesh.000000"

if (-not (Test-Path $glvis)) {
    throw "GLVis executable not found: $glvis"
}

if (-not (Test-Path $meshFile)) {
    throw "MFEM mesh file not found: $meshFile"
}

$modeFiles = Get-ChildItem -Path $projectRoot -Filter "mode_*.000000" | Sort-Object Name

foreach ($modeFile in $modeFiles) {
    $arguments = @("-m", $meshFile, "-g", $modeFile.FullName)

    if ($Component -ge 0) {
        $arguments += @("-gc", $Component)
    }

    if ($LegacyGL) {
        $arguments += "-oldgl"
    }

    if ($Sequential) {
        & $glvis @arguments
    }
    else {
        Start-Process -FilePath $glvis -ArgumentList $arguments -WorkingDirectory $projectRoot
        Start-Sleep -Milliseconds 700
    }
}
