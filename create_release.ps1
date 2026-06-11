# Get version param
param (
    [string]$Version,
    [nullable[bool]]$PostRelease
)

# Configuration
$Username = "Lyall"
$RepoName = "MGSVFix"
$ProxyName = "winmm"
$Arch = "x64"
$ZipFolder = "tmp"

if (-not $Version) {
    $versionHeader = Get-Content "src/resources/version.h"

    $major = ($versionHeader | Select-String '#define VERSION_MAJOR (\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    $minor = ($versionHeader | Select-String '#define VERSION_MINOR (\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    $patch = ($versionHeader | Select-String '#define VERSION_PATCH (\d+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })

    if ($major -and $minor -and $patch) {
        $Version = "$major.$minor.$patch"
    } else {
        Write-Error "Failed to parse version from src/resources/version.h"
        exit 1
    }
}

Write-Host "Building release version: $Version"

if ($null -eq $PostRelease) {
    $PostReleaseInput = Read-Host "Post release build? (true/false)"
    $PostRelease = $PostReleaseInput -match '^(true|1)$'
}

# Build
Write-Host "($Arch) Building with xmake..."
xmake f -p windows -a $Arch -m release
xmake build -v

# Download ultimate ASI loader
Write-Host "($Arch) Downloading Ultimate ASI Loader..."

if ($Arch -eq "x86") {
    $asiUrl = "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.0/Ultimate-ASI-Loader.zip"
    $asiZipFile = "Ultimate-ASI-Loader_x86.zip"
} else {
    $asiUrl = "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.0/Ultimate-ASI-Loader_x64.zip"
    $asiZipFile = "Ultimate-ASI-Loader_x64.zip"
}

Invoke-WebRequest -Uri $asiUrl -OutFile $asiZipFile
Expand-Archive -Force $asiZipFile -DestinationPath "." | Out-Null
Remove-Item $asiZipFile

# Prepare temp directory
Write-Host "Preparing temporary directory..."
Remove-Item -Recurse -Force $ZipFolder -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $ZipFolder | Out-Null
Copy-Item -Path "build/windows/$Arch/release/*.asi" -Destination $ZipFolder/
Copy-Item -Path "*.ini" -Destination $ZipFolder/
Move-Item -Path dinput8.dll -Destination $ZipFolder/$ProxyName.dll
New-Item -ItemType File -Path tmp/EXTRACT_TO_GAME_FOLDER | Out-Null

# Create release zip
$ZipName = "${RepoName}_${Version}.zip"
$ZipPath = Join-Path -Path "build" -ChildPath $ZipName
Write-Host "Creating release zip: $ZipPath"
Compress-Archive -Path tmp/* -DestinationPath $ZipPath -Force

# Clean up
Write-Host "Cleaning up temp directory..."
Remove-Item -Recurse -Force tmp

Write-Host "Build $Version completed."

# ---------------------

if ($PostRelease) {
    # Prepare release body
    $ReleaseBodyPath = "release_body.md"
    $ReleaseBody = ""

    if (Test-Path $ReleaseBodyPath) {
        $ReleaseBody = Get-Content $ReleaseBodyPath -Raw
        $ReleaseBody = $ReleaseBody -replace "<RELEASE_ZIP_NAME>", $ZipName
    }

    function New-Release {
        param (
            $ApiBaseUrl,
            $Owner,
            $Repo,
            $Tag,
            $Name,
            $Body = "",
            $AssetPath,
            $Token,
            $Draft = $false,
            $Prerelease = $false,
            $Platform = "Unknown"
        )

        $headers = @{
            "Authorization" = "token $Token"
            "Accept" = "application/json"
        }

        $releaseUrl = "$ApiBaseUrl/api/v1/repos/$Owner/$Repo/releases"
        $releaseBody = @{
            tag_name = $Tag
            name = $Name
            body = $Body
            draft = $Draft
            prerelease = $Prerelease
        } | ConvertTo-Json

        Write-Host "[$Platform] Creating release $Tag..."
        try {
            $release = Invoke-RestMethod -Uri $releaseUrl -Method Post -Headers $headers -Body $releaseBody -ContentType "application/json"
        } catch {
            Write-Error "[$Platform] Failed to create release: $_"
            return $false
        }

        $uploadUrl = "$ApiBaseUrl/api/v1/repos/$Owner/$Repo/releases/$($release.id)/assets?name=$(Split-Path $AssetPath -Leaf)"
        Write-Host "[$Platform] Uploading asset..."

        try {
            $asset = Invoke-RestMethod -Uri $uploadUrl -Method Post -Headers $headers -InFile $AssetPath -ContentType "application/octet-stream"
            Write-Host "[$Platform] Release created successfully: $($asset.browser_download_url)"
            return $true
        } catch {
            Write-Error "[$Platform] Failed to upload asset: $_"
            return $false
        }
    }

    $platforms = @(
        @{ Name = "Forgejo";  Url = $env:FORGEJO_URL;  Token = $env:FORGEJO_TOKEN },
        @{ Name = "Codeberg"; Url = $env:CODEBERG_URL; Token = $env:CODEBERG_TOKEN }
    )

    $success = $true
    $anyConfigured = $false

    foreach ($platform in $platforms) {
        if ($platform.Url -and $platform.Token) {
            $anyConfigured = $true
            $result = New-Release -ApiBaseUrl $platform.Url `
                                  -Owner $Username `
                                  -Repo $RepoName `
                                  -Tag $Version `
                                  -Name $Version `
                                  -Body $ReleaseBody `
                                  -AssetPath $ZipPath `
                                  -Token $platform.Token `
                                  -Draft $false `
                                  -Platform $platform.Name
            $success = $success -and $result
        }
    }

    if (-not $anyConfigured) {
        Write-Warning "No release platforms configured (FORGEJO_URL or CODEBERG_URL not set)"
        exit 0
    }

    if (-not $success) {
        Write-Error "One or more releases failed"
        exit 1
    }
}