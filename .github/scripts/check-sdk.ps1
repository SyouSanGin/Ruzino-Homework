param(
    [string]$GitHubToken
)

# Calculate SHA256 hash of configure.py
$configureHash = (Get-FileHash -Path "configure.py" -Algorithm SHA256).Hash.ToLower()
Write-Output "configure-hash=$configureHash"

$sdkDir = "SDK"
$hashFile = Join-Path $sdkDir ".configure_hash"

# Check if stored hash matches
if (Test-Path $hashFile) {
    $storedHash = (Get-Content $hashFile -Raw).Trim()
    if ($storedHash -eq $configureHash) {
        Write-Output "SDK hash matches, checking GitHub Release..."
    } else {
        Write-Output "needs-rebuild=true"
        Write-Output "configure.py changed, SDK rebuild required"
        exit 0
    }
} else {
    Write-Output "needs-rebuild=true"
    Write-Output "No SDK hash file found, will rebuild"
    exit 0
}

# Check if SDK exists in GitHub Release
$tagName = "sdk-$configureHash"

if ($GitHubToken) {
    try {
        $env:GH_TOKEN = $GitHubToken
        $result = & gh release view $tagName --repo "Jerry-Shen0527/USTC_CG_24" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Output "needs-rebuild=false"
            Write-Output "SDK exists in release $tagName"
            exit 0
        }
    } catch {
        Write-Output "Warning: Could not check GitHub release: $_"
    }
}

Write-Output "needs-rebuild=true"
Write-Output "SDK not found in release, will rebuild"
