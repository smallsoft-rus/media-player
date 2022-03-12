# This script should be executed from Visual Studio Developer command prompt (or with MSBuild on %PATH%)

Param($config = "Debug")

echo 'Building Small Media Player...'
echo ('Configuration: '+$config)
$exitCode = 0

msbuild ('-p:Configuration='+$config)

if ($LASTEXITCODE -ne 0) {$exitCode = $LASTEXITCODE}       

echo 'Build finished'
exit $exitCode
