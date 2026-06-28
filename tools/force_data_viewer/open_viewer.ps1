$ErrorActionPreference = "Stop"

$viewer = Join-Path $PSScriptRoot "index.html"
if (-not (Test-Path -LiteralPath $viewer)) {
    throw "Viewer not found: $viewer"
}

Start-Process -FilePath $viewer
