# Create kernel-mode test signing certificates
$ErrorActionPreference = "Stop"

# Create root CA
$root = New-SelfSignedCertificate -Type Custom `
    -Subject 'CN=com0com Test Root CA' `
    -KeyUsage CertSign,CRLSign `
    -CertStoreLocation Cert:\CurrentUser\My `
    -TextExtension @('2.5.29.19={critical}{text}ca=1&pathLength=2')
Write-Host "Root CA thumbprint: $($root.Thumbprint)"

# Create kernel code signing cert with the code signing EKU
$code = New-SelfSignedCertificate -Type Custom `
    -Subject 'CN=com0com Kernel Signing' `
    -Signer $root `
    -KeyUsage DigitalSignature `
    -CertStoreLocation Cert:\CurrentUser\My `
    -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3')
Write-Host "Kernel cert thumbprint: $($code.Thumbprint)"

$outDir = [System.IO.Path]::GetFullPath(
    (Join-Path $PSScriptRoot "..\com0com\sys\x64\Release"))

# The directory does not exist before the first driver build.
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Export root and install to Trusted Root
$rootFile = Join-Path $outDir "root_ca.cer"
Export-Certificate -Cert $root -FilePath $rootFile -Type CERT
Import-Certificate -FilePath $rootFile -CertStoreLocation Cert:\LocalMachine\Root
Write-Host "Root CA installed to LocalMachine\Root"

# Export code cert and install to TrustedPublisher
$codeFile = Join-Path $outDir "kernel_code.cer"
Export-Certificate -Cert $code -FilePath $codeFile -Type CERT
Import-Certificate -FilePath $codeFile -CertStoreLocation Cert:\LocalMachine\TrustedPublisher
Write-Host "Code cert installed to LocalMachine\TrustedPublisher"

# Also add to current user's TrustedPublisher for signtool
Import-Certificate -FilePath $codeFile -CertStoreLocation Cert:\CurrentUser\TrustedPublisher
Write-Host "Code cert installed to CurrentUser\TrustedPublisher"

# Output the thumbprint for the batch file
Write-Host "KERNEL_CERT_THUMB=$($code.Thumbprint)"
Write-Host "DONE"
