param($msiPath)
$i = New-Object -ComObject WindowsInstaller.Installer
$d = $i.OpenDatabase($msiPath, 0)

Write-Host "=== BrowseDlg Control Events ==="
$v = $d.OpenView("SELECT Dialog_, Control_, Event, Argument, Condition, Ordering FROM ControlEvent WHERE Dialog_ = 'BrowseDlg'")
$v.Execute()
while ($true) {
    $r = $v.Fetch()
    if ($null -eq $r) { break }
    $line = $r.StringData(1) + "." + $r.StringData(2) + " " + $r.StringData(3) + "=" + $r.StringData(4) + " order=" + $r.StringData(6)
    Write-Host $line
}
$v.Close()

Write-Host "`n=== InstallDirDlg Control Events ==="
$v2 = $d.OpenView("SELECT Dialog_, Control_, Event, Argument, Condition, Ordering FROM ControlEvent WHERE Dialog_ = 'InstallDirDlg'")
$v2.Execute()
while ($true) {
    $r2 = $v2.Fetch()
    if ($null -eq $r2) { break }
    $line = $r2.StringData(1) + "." + $r2.StringData(2) + " " + $r2.StringData(3) + "=" + $r2.StringData(4) + " order=" + $r2.StringData(6)
    Write-Host $line
}
$v2.Close()

Write-Host "`n=== All custom dialog events ==="
$dialogs = @("WelcomeDlg","LicenseAgreementDlg","InstallDirDlg","CustomizeDlg","BrowseDlg","VerifyReadyDlg","ProgressDlg","ExitDialog","FatalError","UserExit","CancelDlg")
foreach ($dlg in $dialogs) {
    $sql = "SELECT Control_, Event, Argument, Condition, Ordering FROM ControlEvent WHERE Dialog_ = '" + $dlg + "'"
    $v3 = $d.OpenView($sql)
    $v3.Execute()
    $count = 0
    while ($true) {
        $r3 = $v3.Fetch()
        if ($null -eq $r3) { break }
        $line = "  " + $dlg + "." + $r3.StringData(1) + " " + $r3.StringData(2) + "=" + $r3.StringData(3) + " order=" + $r3.StringData(5)
        Write-Host $line
        $count++
    }
    if ($count -eq 0) {
        Write-Host "  " + $dlg + " : NO EVENTS FOUND"
    }
    $v3.Close()
}
