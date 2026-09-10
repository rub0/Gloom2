$ErrorActionPreference = 'Stop'
$artRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$blenderExecutable = Join-Path $artRoot '.cache/blender/blender-4.5.13-windows-x64/blender.exe'
if (-not (Test-Path -LiteralPath $blenderExecutable)) { throw 'Install the portable Blender described in docs/BLENDER_WORKFLOW.md first.' }
if (-not (Test-Path -LiteralPath (Join-Path $artRoot '.cache/blender-mcp-env/Lib/site-packages/blender_mcp/bundled/addon.py'))) {
    throw 'Install tools/art/requirements.txt in .cache/blender-mcp-env first.'
}
if (Get-NetTCPConnection -LocalPort 9876 -State Listen -ErrorAction SilentlyContinue) {
    throw 'Port 9876 is already in use. Reuse the running Blender connection or close it before starting another.'
}
$env:BLENDER_USER_CONFIG = Join-Path $artRoot '.cache/blender-profile/config'
$env:BLENDER_USER_SCRIPTS = Join-Path $artRoot '.cache/blender-profile/scripts'
$env:DISABLE_TELEMETRY = 'true'
New-Item -ItemType Directory -Force $env:BLENDER_USER_CONFIG,$env:BLENDER_USER_SCRIPTS | Out-Null
New-Item -ItemType Directory -Force (Join-Path $artRoot '.cache/blender-bridge') | Out-Null
$startup = Join-Path $PSScriptRoot 'start_blender.py'
& $blenderExecutable --factory-startup --python $startup
