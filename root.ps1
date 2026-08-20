param(
  [string]$Preload,
  [int]$Tries = 8
)

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Adb = Join-Path $Here "bin\windows\adb.exe"
if (-not (Test-Path $Adb)) { $Adb = "adb" }
if (-not $Preload) {
  $Preload = Join-Path $Here "bin\preload"
  if (-not (Test-Path $Preload)) {
    $Preload = Join-Path $Here "build\sunstone\bin\preload"
  }
}

$Log = "/data/local/tmp/ghostlock.log"
$WifiOff = $false

function Disable-Wifi {
  $serial = (& $Adb get-serialno 2>$null | Out-String).Trim()
  if ($serial -match '^.+:\d+$' -or $serial -match '\._tcp$') { return }
  if (-not $script:WifiOff) {
    $script:WifiOff = $true
  }
  & $Adb shell svc wifi disable 2>$null | Out-Null
}

function Wait-Boot {
  & $Adb wait-for-device | Out-Null
  while ($true) {
    $b = (& $Adb shell getprop sys.boot_completed 2>$null | Out-String).Trim()
    if ($b -eq "1") { break }
    Start-Sleep -Seconds 2
  }
}

if (-not (Test-Path $Preload)) {
  Write-Host "Preload not found: $Preload"
  exit 1
}

for ($i = 1; $i -le $Tries; $i++) {
  Write-Host "Attempt $i of $Tries"

  & $Adb reboot 2>$null | Out-Null
  Wait-Boot
  Start-Sleep -Seconds 3

  Disable-Wifi

  & $Adb push $Preload /data/local/tmp/preload | Out-Null
  & $Adb shell "chmod 755 /data/local/tmp/preload; rm -f /data/local/tmp/ghostlock.proof $Log; nohup /data/local/tmp/preload >$Log 2>&1 &" | Out-Null

  $tail = Start-Process $Adb -ArgumentList "shell", "sleep 1; tail -f -n +1 $Log" -NoNewWindow -PassThru

  $rooted = $false
  for ($t = 0; $t -lt 90; $t++) {
    Start-Sleep -Seconds 2
    $state = (& $Adb get-state 2>$null | Out-String).Trim()
    if ($state -ne "device") { break }
    $proof = (& $Adb shell "cat /data/local/tmp/ghostlock.proof" 2>$null | Out-String)
    if ($proof -match "ROOTED") { $rooted = $true; break }
  }

  if ($tail -and -not $tail.HasExited) {
    Stop-Process -Id $tail.Id -Force -ErrorAction SilentlyContinue
  }

  if ($rooted) {
    $su = (& $Adb shell "command -v su" 2>$null | Out-String).Trim()
    if ($su) {
      & $Adb shell -t su
      exit 0
    }
    Write-Host "Using the bind shell on 127.0.0.1 9999"
    & $Adb shell -t "nc 127.0.0.1 9999"
    exit 0
  }

  Write-Host "Attempt $i failed"
}

Write-Host "Failed after $Tries attempts"
exit 1
