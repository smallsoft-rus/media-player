echo 'Setting git version info...'

$path = './SmallMediaPlayer/smpver.h'
$content = ('#define SMP_COMMIT_SHA L"@'+$env:GITHUB_SHA+"""`r`n")
echo ('Path: '+$path)
echo ('Content: '+$content)
[System.IO.File]::WriteAllText($path,$content)
exit $LASTEXITCODE
