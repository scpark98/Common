# PostToolUse hook: re-apply UTF-8 BOM (+ CRLF) to a C/C++ source RIGHT AFTER Claude's
# Write / Edit / MultiEdit wrote it.
#
# WHY THIS EXISTS (the gap the other two hooks never closed):
#   SessionStart (ensure-cpp-encoding.ps1) and PreToolUse (protect-cpp-encoding-file.ps1)
#   both guard the INPUT side -- they make a file clean UTF-8 BOM *before* Claude reads/edits
#   it, so Korean is never read as garbage. But the `Write` tool emits UTF-8 *without* BOM
#   (and LF). PreToolUse runs BEFORE Write, so:
#     - new file  : PreToolUse sees "no file" -> does nothing -> Write makes a BOM-less file.
#     - full rewrite: PreToolUse may fix the old bytes, but Write overwrites them BOM-less.
#   Nothing re-adds the BOM AFTER the write. That is exactly the "broke encoding, restored it,
#   broke it again" cycle. This PostToolUse hook closes it deterministically: it runs after the
#   file exists with its new content and restores the VS-2026 default (UTF-8 BOM + CRLF).
#
#   `Edit` already preserves an existing BOM, so this is mostly a no-op there (BOM present -> skip).
#   Only `Write` output (BOM-less) actually gets fixed.
#
# Safety: Write/Edit always emit UTF-8, so the bytes are valid UTF-8 -> we only add a BOM and
# normalize newlines. A CP949 fallback is kept for completeness but won't trigger on tool output.
# Allowlist of extensions (NOT denylist) so binaries are never touched. Build dirs excluded.
# Idempotent: a file already UTF-8 BOM is skipped.

$ErrorActionPreference = 'SilentlyContinue'

# --- target path from PostToolUse stdin JSON {tool_input:{file_path}} ----------
$path = $null
try {
    $raw = [Console]::In.ReadToEnd()
    if ($raw) {
        $j = $raw | ConvertFrom-Json
        if ($j.tool_input -and $j.tool_input.file_path) { $path = $j.tool_input.file_path }
    }
} catch {}

if (-not $path) { exit 0 }
if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { exit 0 }

# C/C++ sources + BOM-harmless text docs. Excludes .rc/.rc2 (VS keeps UTF-16) and BOM-hostile
# text (.bat/.cmd/.sh/.json/.yaml). Binaries never reach here (allowlist).
$exts = @('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl', '.ipp',
          '.txt', '.md', '.markdown', '.log', '.csv', '.ini', '.xml', '.html', '.htm')
$ext = [IO.Path]::GetExtension($path).ToLower()
if ($exts -notcontains $ext) { exit 0 }

$skipDirs = @('\x64\', '\win32\', '\debug\', '\release\', '\.vs\', '\ipch\', '\obj\')
$low = $path.ToLower()
foreach ($d in $skipDirs) { if ($low.Contains($d)) { exit 0 } }

$skipNames = @('targetver.h')   # RC-included header; rc.exe chokes on @file etc. in UTF-8 BOM
if ($skipNames -contains ([IO.Path]::GetFileName($path).ToLower())) { exit 0 }

$bytes = [IO.File]::ReadAllBytes($path)
if ($bytes.Length -lt 1) { exit 0 }

# Already UTF-8 BOM -> safe (Edit preserves it; or a prior run fixed it). Nothing to do.
if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { exit 0 }
# UTF-16 BOM -> leave to VS.
if ($bytes.Length -ge 2 -and (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) { exit 0 }

# Pure ASCII -> no Korean to lose and identical in CP949/UTF-8; leave as-is (no churn).
$hasHigh = $false
foreach ($b in $bytes) { if ($b -ge 0x80) { $hasHigh = $true; break } }
if (-not $hasHigh) { exit 0 }

$bom = [byte[]](0xEF, 0xBB, 0xBF)
$utf8Strict = New-Object Text.UTF8Encoding($false, $true)   # throwOnInvalidBytes
$cp949 = [Text.Encoding]::GetEncoding(949)

# Decode: tool output is valid UTF-8; CP949 fallback only for a hand-pasted file.
$isUtf8 = $true
try { $text = $utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }
if (-not $isUtf8) { $text = $cp949.GetString($bytes) }

# Normalize newlines to CRLF (VS-2026 default; `Write` emits LF). Collapse any CR/LF/CRLF mix.
$text = [regex]::Replace($text, "`r`n|`r|`n", "`r`n")

$u = ([Text.UTF8Encoding]::new($false)).GetBytes($text)
$out = New-Object byte[] ($bom.Length + $u.Length)
[Array]::Copy($bom, 0, $out, 0, $bom.Length)
[Array]::Copy($u, 0, $out, $bom.Length, $u.Length)
[IO.File]::WriteAllBytes($path, $out)

$how = if ($isUtf8) { 'added BOM + CRLF' } else { 'CP949 -> UTF-8 BOM + CRLF' }
$msg = "ensure-cpp-encoding-after-write: $([IO.Path]::GetFileName($path)) fixed ($how) after tool wrote it."
(@{ systemMessage = $msg } | ConvertTo-Json -Compress)
exit 0
