param([Parameter(Mandatory=$true)][string]$Server,[Parameter(Mandatory=$true)][string]$Clients,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $Output | Out-Null
$taskOutput=(Resolve-Path -LiteralPath $Output).Path
$socket=[System.Net.Sockets.UdpClient]::new(0)
$port=$socket.Client.LocalEndPoint.Port
$socket.Dispose()
$serverProcess=Start-Process -FilePath $Server -ArgumentList @("127.0.0.1:$port",'--ticks','2400') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'dedicated-server.log') -RedirectStandardError (Join-Path $taskOutput 'dedicated-server.stderr.log')
try {
    Start-Sleep -Milliseconds 800
    & $Clients --clients "127.0.0.1:$port" > (Join-Path $taskOutput 'dedicated-clients.log') 2>&1
    if ($LASTEXITCODE -ne 0) {Get-Content (Join-Path $taskOutput 'dedicated-clients.log');throw 'Dedicated client acceptance failed'}
    if (!$serverProcess.WaitForExit(60000)) {throw 'Dedicated server did not complete its bounded run'}
    $serverProcess.Refresh()
    if ($serverProcess.ExitCode -ne 0) {throw "Dedicated server exit $($serverProcess.ExitCode)"}
    Get-Content (Join-Path $taskOutput 'dedicated-clients.log')
} finally {
    if (!$serverProcess.HasExited) {$serverProcess.Kill();$serverProcess.WaitForExit()}
}
