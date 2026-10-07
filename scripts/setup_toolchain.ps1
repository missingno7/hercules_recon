param(
    [ValidateSet('msvc5_rtm', 'msvc5_sp2', 'all')][string]$Variant = 'all',
    [string]$ToolsRoot = 'C:\tools\hercules',
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$ToolsRoot = [IO.Path]::GetFullPath($ToolsRoot)
$manifestPath = Join-Path $PSScriptRoot '..\toolchains\manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json

function Get-TreeIdentity([string]$Root) {
    $resolvedRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $rows = [Collections.Generic.List[string]]::new()
    $bytes = [long]0
    foreach ($file in Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File) {
        $relative = $file.FullName.Substring($resolvedRoot.Length + 1).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $rows.Add($relative + [char]0 + $file.Length + [char]0 + $hash + "`n")
        $bytes += $file.Length
    }
    $rows.Sort([StringComparer]::Ordinal)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $digest = $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes([string]::Concat($rows)))
        $hex = [BitConverter]::ToString($digest).Replace('-', '').ToLowerInvariant()
    } finally { $sha.Dispose() }
    return @{ sha256 = $hex; files = $rows.Count; bytes = $bytes }
}

foreach ($tc in $manifest.toolchains) {
    if ($Variant -ne 'all' -and $tc.id -ne $Variant) { continue }
    $destination = Join-Path $ToolsRoot $tc.directory
    if (-not (Test-Path -LiteralPath $destination)) {
        if ($VerifyOnly) { throw "Toolchain missing: $destination" }
        $downloadRoot = Join-Path $ToolsRoot 'downloads'
        New-Item -ItemType Directory -Force -Path $downloadRoot | Out-Null
        $archive = Join-Path $downloadRoot ($tc.directory + '.zip')
        if (-not (Test-Path -LiteralPath $archive)) {
            Invoke-WebRequest -Uri $tc.archive_url -OutFile $archive
        }
        $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($archiveHash -ne $tc.archive_sha256) { throw "Archive hash mismatch: $archive" }
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [IO.Compression.ZipFile]::OpenRead($archive)
        try {
            foreach ($entry in $zip.Entries) {
                $entryPath = [IO.Path]::GetFullPath((Join-Path $ToolsRoot $entry.FullName))
                if (-not $entryPath.StartsWith($destination + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -and $entryPath.TrimEnd('\', '/') -ne $destination) {
                    throw "Unexpected archive member: $($entry.FullName)"
                }
            }
        } finally { $zip.Dispose() }
        Expand-Archive -LiteralPath $archive -DestinationPath $ToolsRoot
        # Copy an unchanged dependency. Do not put the old MSVCRT in the DLL search path.
        Copy-Item -LiteralPath (Join-Path $destination 'redist\msvcp50.dll') -Destination (Join-Path $destination 'bin\msvcp50.dll')
    }
    $actual = Get-TreeIdentity $destination
    if ($actual.sha256 -ne $tc.installed_tree.sha256 -or $actual.files -ne $tc.installed_tree.files -or $actual.bytes -ne $tc.installed_tree.bytes) {
        throw "Installed tree differs from lock: $destination (actual $($actual.sha256))"
    }
    Write-Output "$($tc.id): verified $($actual.files) files; $destination"
}
