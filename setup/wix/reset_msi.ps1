# Deep-clean all com0com traces from Windows Installer
$b = 'HKLM:\Software\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18'

# Products
$c = 0
Get-ChildItem "$b\Products" -ea 0 | ForEach-Object {
    $n = (Get-ItemProperty $_.PSPath -Name DisplayName -ea 0).DisplayName
    if ($n -eq 'com0com') { Remove-Item $_.PSPath -Recurse -Force; $c++ }
}
Write-Host "Products: $c"

# Components  
$c = 0
Get-ChildItem "$b\Components" -ea 0 | ForEach-Object {
    $props = Get-ItemProperty $_.PSPath -ea 0
    if (-not $props) { return }
    $found = $false
    foreach ($p in $props.PSObject.Properties) {
        $v = $p.Value
        if ($v -is [string] -and $v -like '*com0com*') { $found = $true; break }
    }
    if ($found) { Remove-Item $_.PSPath -Recurse -Force; $c++ }
}
Write-Host "Components: $c"

# Also clean Folders and UpgradeCodes
Get-ChildItem "$b\..\Folders" -ea 0 | Where-Object { $_.PSChildName -like '*com0com*' } | Remove-Item -Recurse -Force
Get-ChildItem "$b\..\UpgradeCodes" -ea 0 | Where-Object { $_.PSChildName -like '*A1B2C3D4*' } | Remove-Item -Recurse -Force
Write-Host "Done"
