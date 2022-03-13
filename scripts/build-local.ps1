# This script should be executed from Visual Studio Developer command prompt (or with MSBuild on %PATH%)
Param($config = "Debug")

echo 'Reading version info from git...'

if(-not [System.IO.File]::Exists('.git/HEAD')){
    echo 'Error: git repository not found!'
    ./scripts/build.ps1 -config $config
    exit $LASTEXITCODE
}

# Read current head ref
$lines = [System.IO.File]::ReadAllLines('.git/HEAD')

if($lines.Length -eq 0) {
    echo 'Error: git ref not found!'
    $ref = ''
}
else {
    $ref = $lines[0]
}

$ref_name = ''
$ref_path = ''
$sha = ''

# Parse ref to get name
if($ref.StartsWith('ref: refs/heads/')){
    $ref_name = $ref.Substring(16)
}
elseif($ref.StartsWith('ref: ')){
    $ref_name = $ref.Substring(5)
}
else{
    $ref_name = $ref
}

# Construct file path for commit info
if($ref.StartsWith('ref: ')){
    $ref_path = $ref.Substring(5)
}
else{
    $ref_path = $ref
}

$filepath = ('.git/'+$ref_path)

# Read commit info
if([System.IO.File]::Exists($filepath)){
    $sha_lines = [System.IO.File]::ReadAllLines($filepath)
    
    if($sha_lines.Length -gt 0) {$sha = $sha_lines[0]}
    else {echo 'Error: git commit info is empty!'}
}
else{
    echo 'Error: git commit info not found!'
}

if($ref_name -ne '' -or $sha -ne ''){
    echo ('git ref '+$ref_name+'; commit '+$sha)
    $env:GITHUB_REF_NAME = $ref_name
    $env:GITHUB_SHA = $sha
}

# Invoke build script
./scripts/build.ps1 -config $config
exit $LASTEXITCODE
