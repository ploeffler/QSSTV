# Checks the actual SFX, argument forwarding, HOME and cleanup without QSSTV/devices.
$ErrorActionPreference = 'Stop'
$root = Join-Path $env:RUNNER_TEMP 'QSSTV portable Unicode ä smoke'
New-Item -ItemType Directory -Path $root -ErrorAction Stop | Out-Null
$exe = Join-Path $root 'QSSTV portable ä.exe'
Copy-Item 'portable-smoke.exe' $exe
$env:TEMP = Join-Path $root 'temp ä'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Path $env:TEMP | Out-Null
# The payload must not rely on the MSYS2/MinGW PATH.
$env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
foreach ($customHome in @($false, $true)) {
    $marker = Join-Path $root "result-$customHome.txt"
    if ($customHome) {
        $env:HOME = Join-Path $root 'explicit home ä'
        New-Item -ItemType Directory -Path $env:HOME | Out-Null
        $expectedHome = $env:HOME
    } else {
        Remove-Item Env:HOME -ErrorAction SilentlyContinue
        $expectedHome = $env:USERPROFILE
    }
    $process = Start-Process -FilePath $exe -ArgumentList @("`"$marker`"", '"argument with spaces"') -PassThru
    if (-not $process.WaitForExit(60000)) { $process.Kill(); throw 'Portable launch timed out' }
    if ($process.ExitCode -ne 0 -or -not (Test-Path $marker)) { throw 'Portable child did not finish successfully' }
    $lines = Get-Content -LiteralPath $marker -Encoding Unicode
    if ($lines[0] -ne $expectedHome) { throw 'Persistent HOME was not respected' }
    if (Test-Path -LiteralPath $lines[1]) { throw 'Temporary payload was not cleaned after child exit' }
}
Write-Output 'PASS: portable SFX/launcher, isolated PATH, Unicode/spaces, HOME fallback/preservation, argument forwarding, wait and cleanup. No QSSTV devices opened.'
