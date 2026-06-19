# SessionStart hook: make VS C++ projects safe from Korean (CP949) corruption.
#
# Two protections, applied at session start BEFORE Claude reads/edits anything:
#   1. Drop a root .editorconfig (charset=utf-8-bom) if missing, so VS in Korean locale
#      stops re-encoding/BOM-dropping sources on save (Common/claude.md 2B.1 Step 0).
#   2. Convert every CP949 (or BOM-less UTF-8) source that contains non-ASCII bytes to
#      UTF-8 BOM. Doing this up front means Claude's later Read sees clean UTF-8 (correct
#      old_string) and its Write can't re-encode CP949 into U+FFFD.
#
# Only touches files that actually need it:
#   - UTF-8 BOM  -> skip (already safe)
#   - UTF-16     -> skip (.rc/resource.h, VS-managed)
#   - pure ASCII -> skip (identical in CP949 and UTF-8; no corruption risk)
#   - RC-included headers (targetver.h) -> skip by name (rc.exe needs an RC-friendly
#     encoding; a Korean .h converted to UTF-8 BOM made rc.exe fail with RC 0x40)
#   - non-ASCII, no BOM: valid UTF-8 -> add BOM; else decode CP949 -> re-encode UTF-8 + BOM
# Build output dirs are excluded. Idempotent; silent unless it changes something.

$ErrorActionPreference = 'SilentlyContinue'

# Resolve project dir: prefer hook stdin JSON {cwd}, fall back to current location.
$proj = (Get-Location).Path
try {
    $raw = [Console]::In.ReadToEnd()
    if ($raw) {
        $j = $raw | ConvertFrom-Json
        if ($j.cwd) { $proj = $j.cwd }
    }
} catch {}

if (-not (Test-Path -LiteralPath $proj)) { exit 0 }

# Only act on Visual Studio C++ projects/solutions. cwd may be a *solution folder*
# whose .vcxproj files live in per-project subfolders (e.g. nFTDServer\, nFTDClient\),
# so a root .sln counts too, and .vcxproj is searched recursively (depth-limited) —
# not just directly in cwd. (Without this, solution-folder cwds were skipped entirely.)
$vcx = @(Get-ChildItem -LiteralPath $proj -Filter *.vcxproj -File -ErrorAction SilentlyContinue)
$sln = @(Get-ChildItem -LiteralPath $proj -Filter *.sln -File -ErrorAction SilentlyContinue)
if ($vcx.Count -eq 0 -and $sln.Count -eq 0) {
    $vcx = @(Get-ChildItem -LiteralPath $proj -Recurse -Depth 2 -Filter *.vcxproj -File -ErrorAction SilentlyContinue)
}
if ($vcx.Count -eq 0 -and $sln.Count -eq 0) { exit 0 }

$actions = @()

# --- 1. .editorconfig ---------------------------------------------------------
$ecPath = Join-Path $proj '.editorconfig'
if (-not (Test-Path -LiteralPath $ecPath)) {
    $ec = @'
root = true

# Auto-added by ensure-cpp-encoding hook. Forces UTF-8 BOM for C/C++ sources so VS in
# Korean locale (ANSI=CP949) does not silently re-encode Korean comments/strings.
# .rc/.rc2 stay UTF-16 LE per VS Resource Editor convention.

[*.{cpp,h,hpp,c,cc,cxx,inl,ipp}]
charset = utf-8-bom
end_of_line = crlf
indent_style = tab
tab_width = 4
insert_final_newline = true

[*.{rc,rc2}]
charset = utf-16le
end_of_line = crlf

[*.md]
charset = utf-8
end_of_line = crlf
'@
    [IO.File]::WriteAllText($ecPath, $ec, (New-Object Text.UTF8Encoding($false)))
    $actions += '.editorconfig'
}

# --- 2. Convert CP949 / BOM-less sources to UTF-8 BOM -------------------------
$exts = @('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl', '.ipp')
$skipDirs = @('\x64\', '\win32\', '\debug\', '\release\', '\.vs\', '\ipch\', '\obj\')
# Headers that are #include'd by a .rc must keep an RC-friendly encoding. rc.exe chokes
# when such a Korean .h is converted to UTF-8 BOM (e.g. a stray doxygen '@' surfaced as
# "unknown character '0x40'" and aborted the RC preprocessor). Never convert these,
# regardless of their current encoding (compare by file name, case-insensitive).
$skipNames = @('targetver.h')
$bom = [byte[]](0xEF, 0xBB, 0xBF)
$utf8Strict = New-Object Text.UTF8Encoding($false, $true)   # throwOnInvalidBytes
$cp949 = [Text.Encoding]::GetEncoding(949)
$converted = 0

$files = Get-ChildItem -LiteralPath $proj -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $exts -contains $_.Extension.ToLower() }

foreach ($f in $files) {
    $low = $f.FullName.ToLower()
    $skip = $false
    foreach ($d in $skipDirs) { if ($low.Contains($d)) { $skip = $true; break } }
    if ($skip) { continue }
    if ($skipNames -contains $f.Name.ToLower()) { continue }

    $bytes = [IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -lt 1) { continue }

    # UTF-8 BOM -> already safe
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { continue }
    # UTF-16 (BOM) -> leave to VS
    if ($bytes.Length -ge 2 -and (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) { continue }

    # pure ASCII -> no corruption risk, leave as-is
    $hasHigh = $false
    foreach ($b in $bytes) { if ($b -ge 0x80) { $hasHigh = $true; break } }
    if (-not $hasHigh) { continue }

    # non-ASCII, no BOM: decide UTF-8-missing-BOM vs CP949
    $isUtf8 = $true
    try { [void]$utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }

    if ($isUtf8) {
        # already UTF-8, just prepend BOM
        $out = New-Object byte[] ($bom.Length + $bytes.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($bytes, 0, $out, $bom.Length, $bytes.Length)
    } else {
        # CP949 -> UTF-8 (CR/LF are ASCII, preserved), prepend BOM
        $text = $cp949.GetString($bytes)
        $u = ([Text.UTF8Encoding]::new($false)).GetBytes($text)
        $out = New-Object byte[] ($bom.Length + $u.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($u, 0, $out, $bom.Length, $u.Length)
    }
    [IO.File]::WriteAllBytes($f.FullName, $out)
    $converted++
}

if ($converted -gt 0) { $actions += "converted $converted file(s) to UTF-8 BOM" }

if ($actions.Count -gt 0) {
    $msg = "ensure-cpp-encoding [$proj]: " + ($actions -join '; ')
    (@{ systemMessage = $msg } | ConvertTo-Json -Compress)
}
exit 0
