param(
    [string]$Python = 'C:\Users\Jiri\AppData\Local\Programs\Python\Python312\python.exe',
    [string]$Destination = 'C:\tools\hercules\python'
)
$ErrorActionPreference = 'Stop'
& $Python -m pip install --target $Destination 'pefile==2024.8.26' 'capstone==5.0.7'
if ($LASTEXITCODE -ne 0) { throw 'Analysis dependency installation failed' }
