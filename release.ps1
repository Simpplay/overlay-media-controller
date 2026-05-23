param()

$ErrorActionPreference = "Stop"

Write-Host "Reading version.json..."

$json = Get-Content version.json | ConvertFrom-Json

$version = $json.version

if (-not $version) {
    throw "version field not found"
}

$tag = "v$version"

Write-Host "Version: $version"
Write-Host "Tag: $tag"

git diff --quiet

if ($LASTEXITCODE -ne 0) {
    throw "Working tree is not clean"
}

git diff --cached --quiet

if ($LASTEXITCODE -ne 0) {
    throw "There are staged changes"
}

$existingTag = git tag -l $tag

if ($existingTag) {
    throw "Tag $tag already exists"
}

Write-Host "Creating tag..."

git tag $tag

if ($LASTEXITCODE -ne 0) {
    throw "Failed to create tag"
}

Write-Host "Pushing main..."

git push origin main

if ($LASTEXITCODE -ne 0) {
    throw "Failed to push main"
}

Write-Host "Pushing tag..."

git push origin $tag

if ($LASTEXITCODE -ne 0) {
    throw "Failed to push tag"
}

Write-Host ""
Write-Host "Release started successfully"
Write-Host "Tag: $tag"