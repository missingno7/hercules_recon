param(
    [string]$Python = 'C:\Users\Jiri\AppData\Local\Programs\Python\Python312\python.exe',
    [string]$Destination = 'C:\tools\hercules\python'
)
$ErrorActionPreference = 'Stop'
& $Python -m pip install --target $Destination --require-hashes -r (Join-Path $PSScriptRoot '../toolchains/analysis-requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'Analysis dependency installation failed' }
