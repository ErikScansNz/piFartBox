param(
  [string]$RepoRoot = (Resolve-Path "$PSScriptRoot\..\..").Path,
  [string]$Branch = "main",
  [string]$OutputRoot = "",
  [string]$ProjectName = "piFartBox"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
  throw "Provide -OutputRoot pointing to the mounted USB drive or other export target."
}

$resolvedRepo = (Resolve-Path $RepoRoot).Path
$resolvedOutput = (Resolve-Path $OutputRoot).Path
$updateRoot = Join-Path $resolvedOutput "$ProjectName-update"
$bundlePath = Join-Path $updateRoot "$ProjectName.bundle"
$manifestPath = Join-Path $updateRoot "manifest.json"

New-Item -ItemType Directory -Force -Path $updateRoot | Out-Null

$commit = (git -C $resolvedRepo rev-parse --short "$Branch").Trim()
$fullCommit = (git -C $resolvedRepo rev-parse "$Branch").Trim()

git -C $resolvedRepo bundle create $bundlePath $Branch

$manifest = [ordered]@{
  project = $ProjectName
  branch = $Branch
  commit = $commit
  full_commit = $fullCommit
  bundle = "$ProjectName.bundle"
  created_at = [DateTime]::UtcNow.ToString("o")
}

$json = $manifest | ConvertTo-Json -Depth 4
$enc = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($manifestPath, $json, $enc)

Write-Host "Bundle written to $bundlePath"
Write-Host "Manifest written to $manifestPath"
