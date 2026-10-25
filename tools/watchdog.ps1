$S = "adb-3B665T0282F00000-RFgOxL._adb-tls-connect._tcp"
$F = "C:/platform-tools/adb.exe"
& $F -s $S push "C:/Users/24508/WorkBuddy/2026-10-23-21-36-24/droiddeck-dimensity/tools/mobo54.sh" /sdcard/Download/mobo54.sh | Out-Null
for ($i=1; $i -le 240; $i++) {
  $focus = (& $F -s $S shell "dumpsys window 2>/dev/null | grep mCurrentFocus" 2>$null)
  if ($focus -match "com.termux") {
    echo "TERMUX_FOCUSED at $(Get-Date -Format HH:mm:ss)"
    & $F -s $S shell "input text 'bash%s/sdcard/Download/mobo54.sh'"
    Start-Sleep -Seconds 2
    & $F -s $S shell "input keyevent 66"
    Start-Sleep -Seconds 420
    & $F -s $S shell "cat /sdcard/Download/mobo54.txt 2>/dev/null"
    break
  }
  Start-Sleep -Seconds 10
}
