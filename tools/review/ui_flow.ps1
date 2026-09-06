param([Parameter(Mandatory=$true)][string]$Server,[Parameter(Mandatory=$true)][string]$Client,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $Output | Out-Null
$taskOutput=(Resolve-Path -LiteralPath $Output).Path
$portSocket=[System.Net.Sockets.UdpClient]::new(0)
$port=$portSocket.Client.LocalEndPoint.Port
$portSocket.Dispose()
$serverProcess=$null
$firstProcess=$null
$secondProcess=$null
try {
    $serverProcess=Start-Process -FilePath $Server -ArgumentList @("127.0.0.1:$port",'--ticks','9600') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'server.log') -RedirectStandardError (Join-Path $taskOutput 'server.stderr.log')
    Start-Sleep -Milliseconds 800
    $firstProcess=Start-Process -FilePath $Client -ArgumentList @('--ui-flow-review',"127.0.0.1:$port",('"'+(Join-Path $taskOutput 'archangel')+'"'),'Nyx','archangel') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'archangel.log') -RedirectStandardError (Join-Path $taskOutput 'archangel.stderr.log')
    $secondProcess=Start-Process -FilePath $Client -ArgumentList @('--ui-flow-review',"127.0.0.1:$port",('"'+(Join-Path $taskOutput 'shadow')+'"'),'Rook','shadow') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'shadow.log') -RedirectStandardError (Join-Path $taskOutput 'shadow.stderr.log')
    $deadline=[DateTime]::UtcNow.AddSeconds(145)
    while((!$firstProcess.HasExited -or !$secondProcess.HasExited) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 250}
    foreach($process in @($firstProcess,$secondProcess)){
        if(!$process.HasExited){throw 'Graphical clients did not finish'}
        $process.Refresh()
        if($process.ExitCode -ne 0){throw "Graphical client failed: $($process.ExitCode)"}
    }
    foreach($name in 'archangel','shadow'){
        $log=Get-Content -Raw -LiteralPath (Join-Path $taskOutput ($name+'.log'))
        $errors=Get-Content -Raw -LiteralPath (Join-Path $taskOutput ($name+'.stderr.log'))
        if($log -notmatch 'UI flow passed:' -or ($log+$errors) -match 'VUID-|Validation Error|Diligent Engine: (Error|ERROR)'){throw "UI acceptance failed for $name"}
    }
    Write-Output 'Two graphical processes passed browser, selection, ready, play, reconnect and confirmed leave.'
} finally {
    foreach($process in @($firstProcess,$secondProcess,$serverProcess)){
        if($null -ne $process -and !$process.HasExited){$process.Kill();$process.WaitForExit()}
    }
}
