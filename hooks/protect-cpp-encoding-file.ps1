# PreToolUse hook: protect a single C/C++ source file from Korean (CP949) corruption,
# right before Claude's Read / Edit / Write / MultiEdit touches it.
#
# The SessionStart hook (ensure-cpp-encoding.ps1) only converts files that exist at
# session start. A file added mid-session (pulled, generated, or created in VS) is
# still CP949 / BOM-less when Claude first reads it -> Read sees garbage -> Edit's
# old_string is garbage -> Write re-encodes to U+FFFD. This hook closes that gap by
# applying the SAME per-file conversion the moment any of those tools targets the file.
#
# Same rules as ensure-cpp-encoding.ps1:
#   - UTF-8 BOM  -> skip (already safe)
#   - UTF-16 BOM -> skip (.rc/Resource.h, VS-managed)
#   - pure ASCII -> skip (identical in CP949 and UTF-8)
#   - targetver.h (RC-included) -> skip by name
#   - non-ASCII, no BOM: valid UTF-8 -> add BOM; else decode CP949 -> re-encode UTF-8 + BOM
# Only acts on C/C++ source extensions; build output dirs excluded. Idempotent; silent.
# Converting BEFORE Read is the key: it guarantees the Read returns clean UTF-8.

$ErrorActionPreference = 'SilentlyContinue'

# --- get target path from PreToolUse stdin JSON {tool_input:{file_path}} ------
$path = $null
try {
    $raw = [Console]::In.ReadToEnd()
    if ($raw) {
        $j = $raw | ConvertFrom-Json
        if ($j.tool_input -and $j.tool_input.file_path) { $path = $j.tool_input.file_path }
    }
} catch {}

if (-not $path) { exit 0 }
if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { exit 0 }   # new file -> nothing to convert

# C/C++ sources + BOM-harmless text docs. Allowlist (NOT denylist) so binaries
# (.png/.lib/.dll/.ico/.exe...) are never CP949-misdecoded and destroyed. Excluded on
# purpose: .rc/.rc2 (VS keeps UTF-16) and BOM-hostile text (.bat/.cmd/.sh shebang,
# strict .json/.yaml parsers) -- a BOM there breaks the file.
$exts = @('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl', '.ipp',
          '.txt', '.md', '.markdown', '.log', '.csv', '.ini', '.xml', '.html', '.htm')
$ext = [IO.Path]::GetExtension($path).ToLower()
if ($exts -notcontains $ext) { exit 0 }

$skipDirs = @('\x64\', '\win32\', '\debug\', '\release\', '\.vs\', '\ipch\', '\obj\')
$low = $path.ToLower()
foreach ($d in $skipDirs) { if ($low.Contains($d)) { exit 0 } }

$skipNames = @('targetver.h')
if ($skipNames -contains ([IO.Path]::GetFileName($path).ToLower())) { exit 0 }

$bytes = [IO.File]::ReadAllBytes($path)
if ($bytes.Length -lt 1) { exit 0 }

# UTF-8 BOM -> already safe
if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { exit 0 }
# UTF-16 BOM -> leave to VS
if ($bytes.Length -ge 2 -and (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) { exit 0 }

# pure ASCII -> no corruption risk
$hasHigh = $false
foreach ($b in $bytes) { if ($b -ge 0x80) { $hasHigh = $true; break } }
if (-not $hasHigh) { exit 0 }

$bom = [byte[]](0xEF, 0xBB, 0xBF)
$utf8Strict = New-Object Text.UTF8Encoding($false, $true)   # throwOnInvalidBytes
$cp949 = [Text.Encoding]::GetEncoding(949)

# non-ASCII, no BOM: decide UTF-8-missing-BOM vs CP949
$isUtf8 = $true
try { [void]$utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }

if ($isUtf8) {
    $out = New-Object byte[] ($bom.Length + $bytes.Length)
    [Array]::Copy($bom, 0, $out, 0, $bom.Length)
    [Array]::Copy($bytes, 0, $out, $bom.Length, $bytes.Length)
    $how = 'UTF-8 (added BOM)'
} else {
    $text = $cp949.GetString($bytes)
    $u = ([Text.UTF8Encoding]::new($false)).GetBytes($text)
    $out = New-Object byte[] ($bom.Length + $u.Length)
    [Array]::Copy($bom, 0, $out, 0, $bom.Length)
    [Array]::Copy($u, 0, $out, $bom.Length, $u.Length)
    $how = 'CP949 -> UTF-8 BOM'
}
[IO.File]::WriteAllBytes($path, $out)

$msg = "protect-cpp-encoding: $([IO.Path]::GetFileName($path)) converted ($how) before tool ran."
(@{ systemMessage = $msg } | ConvertTo-Json -Compress)
exit 0
