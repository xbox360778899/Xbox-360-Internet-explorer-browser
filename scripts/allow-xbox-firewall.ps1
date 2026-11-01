# Run in an Administrator PowerShell. Only the named Xbox can reach the bridge.
param([string]$XboxAddress = '192.168.0.169')
$ErrorActionPreference = 'Stop'
$address = $null
if (![Net.IPAddress]::TryParse($XboxAddress,[ref]$address) -or $address.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {throw 'Enter the Xbox IPv4 address.'}
$python = (Get-Command python.exe -ErrorAction Stop).Source
$name = 'X360Browser-Xbox-TCP-36080'
$existing = Get-NetFirewallRule -Name $name -ErrorAction SilentlyContinue
if ($existing) {
    $existing | Get-NetFirewallAddressFilter | Set-NetFirewallAddressFilter -RemoteAddress $XboxAddress
    $existing | Get-NetFirewallApplicationFilter | Set-NetFirewallApplicationFilter -Program $python
    $existing | Set-NetFirewallRule -Enabled True -Action Allow -Profile Any
} else {
    New-NetFirewallRule -Name $name -DisplayName 'X360Browser - Xbox LAN renderer' -Direction Inbound -Action Allow -Protocol TCP -LocalPort 36080 -RemoteAddress $XboxAddress -Profile Any -Program $python | Out-Null
}
Write-Output "Allowed Xbox $XboxAddress to the Python bridge on TCP 36080."
