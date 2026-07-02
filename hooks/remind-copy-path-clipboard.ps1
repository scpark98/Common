# UserPromptSubmit hook: 매 턴 리마인더를 컨텍스트에 주입.
# 파일 경로/긴 명령을 보여줄 때 Set-Clipboard 로 클립보드에 넣도록 상기시킨다.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$ctx = '[리마인더] 응답에서 파일 절대경로나 긴/멀티라인 셸 명령을 보여줄 때는, 사용자가 화면에서 긁어 복사하게 하지 말고 PowerShell 도구로 Set-Clipboard -Value "<내용>" 를 실행해 클립보드에 정확히 넣고 "클립보드에 복사됨(붙여넣기하면 바로 열림)" 을 덧붙여라.'
$out = @{ hookSpecificOutput = @{ hookEventName = 'UserPromptSubmit'; additionalContext = $ctx } } | ConvertTo-Json -Compress
Write-Output $out
