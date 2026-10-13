# sync-skills.ps1
# Automatically links all skills under e:\data\doc\skills\<category>\<skill-name>
# using Windows Directory Junctions into:
# - Codex      (~/.codex/skills)
# - Cursor     (~/.cursor/skills)
# - Grok Bot / Agents (~/.agents/skills)
# - Claude     (~/.claude/skills)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$Categories = @("common", "mcu-baremetal", "rtos", "linux-driver", "protocols-debug")

$TargetToolDirs = @(
    (Join-Path $env:USERPROFILE ".codex\skills"),
    (Join-Path $env:USERPROFILE ".cursor\skills"),
    (Join-Path $env:USERPROFILE ".agents\skills"),
    (Join-Path $env:USERPROFILE ".claude\skills")
)

foreach ($targetDir in $TargetToolDirs) {
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    }
}

$SyncedSkills = @()

foreach ($cat in $Categories) {
    $catPath = Join-Path $RepoRoot $cat
    if (-not (Test-Path $catPath)) { continue }

    $skillDirs = Get-ChildItem -Path $catPath -Directory -Force | Where-Object {
        Test-Path (Join-Path $_.FullName "SKILL.md")
    }

    foreach ($skill in $skillDirs) {
        $skillName = $skill.Name
        $sourcePath = $skill.FullName

        foreach ($targetRoot in $TargetToolDirs) {
            $linkPath = Join-Path $targetRoot $skillName
            $needsCreate = $true

            if (Test-Path $linkPath) {
                $item = Get-Item $linkPath -Force
                if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                    $existingTarget = ($item.Target | Select-Object -First 1)
                    if ($existingTarget -and ($existingTarget.TrimEnd('\') -ieq $sourcePath.TrimEnd('\'))) {
                        $needsCreate = $false
                    } else {
                        cmd /c rmdir "$linkPath" | Out-Null
                        Start-Sleep -Milliseconds 150
                    }
                } else {
                    Remove-Item -Path $linkPath -Recurse -Force
                    Start-Sleep -Milliseconds 200
                }
            }

            if ($needsCreate) {
                cmd /c mklink /J "$linkPath" "$sourcePath" | Out-Null
            }
        }

        $SyncedSkills += [PSCustomObject]@{
            Skill    = $skillName
            Category = $cat
            Source   = $sourcePath
        }
    }
}

Write-Host "=== Synced Skills to Codex / Cursor / Grok Bot (.agents) / Claude ===" -ForegroundColor Green
$SyncedSkills | Format-Table -AutoSize
