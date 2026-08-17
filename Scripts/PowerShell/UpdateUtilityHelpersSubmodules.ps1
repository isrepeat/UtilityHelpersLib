<#
.SYNOPSIS
Updates UtilityHelpersLib gitlinks in projects found at a scan root.

.DESCRIPTION
Clean submodules are updated automatically. If at least one submodule contains
working-tree changes, untracked files, or an uncommitted gitlink change, the
script creates recovery snapshot refs and imports them as candidate branches into
a separate Integration clone. The user manually chooses commits, their order and
messages, resolves conflicts, and pushes Integration HEAD to the target branch.

Only after the published remote branch matches Integration HEAD does the script
offer to replace dirty source checkouts and update all parent repositories.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ScanRoot,
    [ValidateRange(0, 1)][int]$ScanDepth = 1,
    [string]$ScanProjectBranch = "Last",
    [string]$ScanSubmoduleBranch = "Last",
    [string]$ProjectOverrides = $null,
    [string]$SubmodulePath = "UtilityHelpersLib",
    [string]$CommitMessage = "Update UtilityHelpersLib submodule",

    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$SubmoduleName = "UtilityHelpersLib",

    [Parameter(Mandatory = $true)][string]$IntegrationDirectory
)

$ErrorActionPreference = "Stop"

function Write-Info {
    param([string]$Message)
    Write-Host "[UtilityHelpers updater] $Message" -ForegroundColor Cyan
}

function Stop-WithError {
    param([string]$Message)
    Write-Host ""
    Write-Host "[UtilityHelpers updater] ERROR: $Message" -ForegroundColor Red
    exit 1
}

# Execute Git without changing the caller's directory. A temporary environment
# is supported so snapshot commits can use an isolated index.
function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$AllowFailure,
        [hashtable]$Environment = @{}
    )

    $savedValues = @{}
    $savedErrorActionPreference = $ErrorActionPreference
    try {
        foreach ($name in $Environment.Keys) {
            $savedValues[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
            [Environment]::SetEnvironmentVariable($name, $Environment[$name], 'Process')
        }

        # Windows PowerShell represents native stderr output as a non-terminating
        # NativeCommandError. Git frequently writes harmless warnings/progress to
        # stderr, so judge success by its process exit code instead.
        $ErrorActionPreference = 'Continue'
        $output = @(& git -C $WorkingDirectory @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $savedErrorActionPreference
        foreach ($name in $Environment.Keys) {
            [Environment]::SetEnvironmentVariable($name, $savedValues[$name], 'Process')
        }
    }

    if ($exitCode -ne 0 -and -not $AllowFailure) {
        $details = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
        throw "git -C `"$WorkingDirectory`" $($Arguments -join ' ') failed with exit code $exitCode.`n$details"
    }
    [pscustomobject]@{ ExitCode = $exitCode; Output = $output }
}

function Parse-ProjectSpecs {
    param([string]$Value)
    $result = @()
    foreach ($entry in ($Value -split ';' | Where-Object { $_.Trim() })) {
        $parts = $entry.Split('|')
        if ($parts.Count -ne 3) {
            Stop-WithError "Invalid override '$entry'. Expected: path|project-branch|submodule-branch"
        }
        $result += [pscustomobject]@{
            InputPath       = $parts[0].Trim().Trim('"')
            ProjectBranch   = $parts[1].Trim()
            SubmoduleBranch = $parts[2].Trim()
        }
    }
    $result
}

function Convert-ToSlug {
    param([string]$Value)
    $slug = ($Value.ToLowerInvariant() -replace '[^a-z0-9._-]+', '-') -replace '^-|-$', ''
    if (-not $slug) { $slug = 'project' }
    $slug
}

function Normalize-RemoteUrl {
    param([string]$Url)
    (($Url.Trim() -replace '\\', '/') -replace '\.git$', '').ToLowerInvariant()
}

# Remove only the explicitly configured Integration directory. The leaf-name and
# filesystem-root checks protect against an accidentally broad recursive delete.
function Remove-IntegrationRepository {
    param(
        [string]$Path,
        [string]$ExpectedSubmoduleName
    )

    $directorySeparators = [char[]]@('\', '/')
    $fullPath = [IO.Path]::GetFullPath($Path).TrimEnd($directorySeparators)
    $rootPath = [IO.Path]::GetPathRoot($fullPath).TrimEnd($directorySeparators)
    $expectedLeaf = "Integration-$ExpectedSubmoduleName"
    if (-not $fullPath -or $fullPath -eq $rootPath -or (Split-Path $fullPath -Leaf) -ine $expectedLeaf) {
        Stop-WithError "Refusing to recursively delete unsafe Integration path: '$fullPath'"
    }

    Remove-Item -LiteralPath $fullPath -Recurse -Force
    Write-Host "Deleted temporary Integration repository: $fullPath" -ForegroundColor Green
}

# Locate exact repository roots at ScanRoot and one directory level below it.
function Find-Projects {
    param([object[]]$Overrides)

    try { $resolvedRoot = (Resolve-Path -LiteralPath $ScanRoot).Path }
    catch { Stop-WithError "Scan root does not exist: '$ScanRoot'" }

    $candidates = @($resolvedRoot)
    if ($ScanDepth -eq 1) {
        $candidates += @(Get-ChildItem -LiteralPath $resolvedRoot -Directory -Force | Select-Object -ExpandProperty FullName)
    }

    $projects = @()
    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath (Join-Path $candidate '.git'))) { continue }
        if (-not (Test-Path -LiteralPath (Join-Path $candidate '.gitmodules') -PathType Leaf)) { continue }

        $paths = Invoke-Git $candidate @('config', '--file', '.gitmodules', '--get-regexp', '^submodule\..*\.path$') -AllowFailure
        if ($paths.ExitCode -ne 0) { continue }
        $registered = @($paths.Output | ForEach-Object { ($_ -split '\s+', 2)[1] })
        if (-not ($registered | Where-Object { $_ -ieq $SubmodulePath })) { continue }

        $override = $Overrides | Where-Object {
            try { (Resolve-Path -LiteralPath $_.InputPath).Path -ieq $candidate }
            catch { $false }
        } | Select-Object -First 1

        $projects += [pscustomobject]@{
            Root               = $candidate
            Name               = Split-Path $candidate -Leaf
            Slug               = Convert-ToSlug (Split-Path $candidate -Leaf)
            ProjectBranch      = if ($override) { $override.ProjectBranch } else { $ScanProjectBranch }
            SubmoduleBranch    = if ($override) { $override.SubmoduleBranch } else { $ScanSubmoduleBranch }
            SubmoduleDirectory = Join-Path $candidate $SubmodulePath
            IsOverride         = $null -ne $override
        }
    }

    if ($projects.Count -eq 0) {
        Stop-WithError "No repositories containing '$SubmodulePath' were found at '$resolvedRoot' within depth $ScanDepth."
    }
    foreach ($override in $Overrides) {
        try { $overridePath = (Resolve-Path -LiteralPath $override.InputPath).Path }
        catch { Stop-WithError "Override path does not exist: '$($override.InputPath)'" }
        if (-not ($projects | Where-Object { $_.Root -ieq $overridePath })) {
            Stop-WithError "Override '$overridePath' did not match a scanned repository containing '$SubmodulePath'."
        }
    }

    # Overrides are shown first; the remaining order is stable and predictable.
    $ordered = @($projects | Sort-Object @{ Expression = { if ($_.IsOverride) { 0 } else { 1 } } }, Root)
    $usedSlugs = @{}
    foreach ($project in $ordered) {
        $baseSlug = $project.Slug
        $suffix = 1
        while ($usedSlugs.ContainsKey($project.Slug)) {
            $suffix++
            $project.Slug = "$baseSlug-$suffix"
        }
        $usedSlugs[$project.Slug] = $true
    }
    $ordered
}

# Build a recovery commit from all non-ignored local content without modifying
# the real index or checkout. The backup ref is intentionally kept after success.
function New-RecoverySnapshot {
    param([object]$Project, [string]$OperationId)

    $repo = $Project.SubmoduleDirectory
    $head = (Invoke-Git $repo @('rev-parse', 'HEAD')).Output[0].ToString().Trim()
    $temporaryIndex = Join-Path ([IO.Path]::GetTempPath()) "utility-updater-$OperationId-$($Project.Slug).index"
    if (Test-Path -LiteralPath $temporaryIndex) { Remove-Item -LiteralPath $temporaryIndex -Force }

    try {
        $environment = @{ GIT_INDEX_FILE = $temporaryIndex }
        Invoke-Git $repo @('read-tree', 'HEAD') -Environment $environment | Out-Null
        Invoke-Git $repo @('add', '-A') -Environment $environment | Out-Null
        $tree = (Invoke-Git $repo @('write-tree') -Environment $environment).Output[0].ToString().Trim()
        $headTree = (Invoke-Git $repo @('rev-parse', 'HEAD^{tree}')).Output[0].ToString().Trim()

        if ($tree -eq $headTree) {
            $snapshot = $head
        }
        else {
            $message = "UtilityHelpers updater snapshot from $($Project.Name) [$OperationId]"
            $snapshot = (Invoke-Git $repo @('commit-tree', $tree, '-p', $head, '-m', $message)).Output[0].ToString().Trim()
        }

        $backupRef = "refs/heads/utility-updater/backup/$OperationId/$($Project.Slug)"
        Invoke-Git $repo @('update-ref', $backupRef, $snapshot) | Out-Null
        [pscustomobject]@{ Commit = $snapshot; BackupRef = $backupRef }
    }
    finally {
        if (Test-Path -LiteralPath $temporaryIndex) { Remove-Item -LiteralPath $temporaryIndex -Force }
    }
}

function Test-GitOperationInProgress {
    param([string]$Repository)
    foreach ($name in @('CHERRY_PICK_HEAD', 'MERGE_HEAD', 'REBASE_HEAD')) {
        $path = (Invoke-Git $Repository @('rev-parse', '--git-path', $name)).Output[0].ToString().Trim()
        if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $Repository $path }
        if (Test-Path -LiteralPath $path) { return $true }
    }
    foreach ($name in @('rebase-merge', 'rebase-apply')) {
        $path = (Invoke-Git $Repository @('rev-parse', '--git-path', $name)).Output[0].ToString().Trim()
        if (-not [IO.Path]::IsPathRooted($path)) { $path = Join-Path $Repository $path }
        if (Test-Path -LiteralPath $path) { return $true }
    }
    $false
}

# Keep the named local submodule branch synchronized for IDE users even though
# the actual submodule checkout remains detached. Only a safe fast-forward is
# allowed; a branch containing unique local commits is never overwritten.
function Update-LocalTrackingBranch {
    param(
        [string]$Repository,
        [string]$Branch,
        [string]$TargetCommit
    )

    $localRef = "refs/heads/$Branch"
    $exists = Invoke-Git $Repository @('show-ref', '--verify', '--quiet', $localRef) -AllowFailure
    if ($exists.ExitCode -eq 0) {
        $localCommit = (Invoke-Git $Repository @('rev-parse', $localRef)).Output[0].ToString().Trim()
        if ($localCommit -ne $TargetCommit) {
            $isFastForward = Invoke-Git $Repository @('merge-base', '--is-ancestor', $localCommit, $TargetCommit) -AllowFailure
            if ($isFastForward.ExitCode -ne 0) {
                Write-Host "  WARN  Local submodule branch '$Branch' has commits outside origin/$Branch and was not moved." -ForegroundColor Yellow
                return
            }
            Invoke-Git $Repository @('branch', '--force', $Branch, $TargetCommit) | Out-Null
        }
    }
    else {
        Invoke-Git $Repository @('branch', $Branch, $TargetCommit) | Out-Null
    }

    # Configure the same upstream that Visual Studio displays for Fetch/Pull/Push.
    Invoke-Git $Repository @('branch', '--set-upstream-to', "origin/$Branch", $Branch) | Out-Null
}

# The manual gate remains open until the checked-out Integration commit is clean,
# has no unfinished Git operation, and exactly matches the published remote branch.
function Wait-ForPublishedIntegration {
    param([string]$Repository, [string]$Branch)

    while ($true) {
        Write-Host ""
        Write-Host "Complete the integration manually in:" -ForegroundColor Yellow
        Write-Host "    $Repository" -ForegroundColor White
        Write-Host "Candidate branches are under candidate/*." -ForegroundColor Yellow
        Write-Host "Push the final checked-out HEAD with:" -ForegroundColor Yellow
        Write-Host "    git push origin HEAD:$Branch" -ForegroundColor White
        Write-Host ""
        $choice = (Read-Host '[R] Try again  [O] Open Integration folder  [A] Abort').Trim().ToUpperInvariant()

        if ($choice -eq 'A') {
            Stop-WithError "Integration was not completed. Integration and recovery branches were preserved."
        }
        if ($choice -eq 'O') {
            Start-Process explorer.exe -ArgumentList $Repository
            continue
        }
        if ($choice -ne 'R') { continue }

        if (Test-GitOperationInProgress $Repository) {
            Write-Host "A cherry-pick, merge, or rebase is still in progress." -ForegroundColor Red
            continue
        }
        $status = @(Invoke-Git $Repository @('status', '--porcelain=v1', '--untracked-files=all')).Output
        if ($status.Count -gt 0) {
            Write-Host "Integration working tree is not clean:" -ForegroundColor Red
            $status | ForEach-Object { Write-Host "    $_" }
            continue
        }

        Invoke-Git $Repository @('fetch', '--prune', 'origin', $Branch) | Out-Null
        $localHead = (Invoke-Git $Repository @('rev-parse', 'HEAD')).Output[0].ToString().Trim()
        $remoteHead = (Invoke-Git $Repository @('rev-parse', 'FETCH_HEAD^{commit}')).Output[0].ToString().Trim()
        if ($localHead -ne $remoteHead) {
            Write-Host "Integration HEAD was not published to origin/$Branch." -ForegroundColor Red
            Write-Host "    Integration: $localHead"
            Write-Host "    origin/$Branch`: $remoteHead"
            continue
        }
        return $remoteHead
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Stop-WithError 'Git was not found in PATH.' }

$requestedIntegrationPath = [IO.Path]::GetFullPath($IntegrationDirectory)
if (Test-Path -LiteralPath $requestedIntegrationPath) {
    Write-Host "Existing Integration repository found:" -ForegroundColor Yellow
    Write-Host "    $requestedIntegrationPath" -ForegroundColor White
    $deleteExisting = (Read-Host 'Delete it and start a new operation? [y/N]').Trim().ToUpperInvariant()
    if ($deleteExisting -ne 'Y') {
        Stop-WithError 'A new operation cannot start while the Integration directory exists.'
    }
    Remove-IntegrationRepository $requestedIntegrationPath $SubmoduleName
}

$overrides = if ($ProjectOverrides -and $ProjectOverrides.Trim()) { @(Parse-ProjectSpecs $ProjectOverrides) } else { @() }
$projects = @(Find-Projects $overrides)
$dirtyProjects = @()

Write-Info "Preflight: validating $($projects.Count) project(s)..."
foreach ($project in $projects) {
    $branch = (Invoke-Git $project.Root @('branch', '--show-current')).Output[0].ToString().Trim()
    if ($branch -cne $project.ProjectBranch) {
        Stop-WithError "'$($project.Root)' is on '$branch'; required branch: '$($project.ProjectBranch)'."
    }
    if (-not (Test-Path -LiteralPath $project.SubmoduleDirectory -PathType Container)) {
        Stop-WithError "Submodule is not initialized: '$($project.SubmoduleDirectory)'."
    }
    if ((Invoke-Git $project.SubmoduleDirectory @('rev-parse', '--is-inside-work-tree') -AllowFailure).ExitCode -ne 0) {
        Stop-WithError "Invalid submodule repository: '$($project.SubmoduleDirectory)'."
    }

    $remote = (Invoke-Git $project.SubmoduleDirectory @('remote', 'get-url', 'origin')).Output[0].ToString().Trim()
    $status = @(Invoke-Git $project.SubmoduleDirectory @('status', '--porcelain=v1', '--untracked-files=all')).Output
    $head = (Invoke-Git $project.SubmoduleDirectory @('rev-parse', 'HEAD')).Output[0].ToString().Trim()
    $recorded = (Invoke-Git $project.Root @('rev-parse', "HEAD:$SubmodulePath")).Output[0].ToString().Trim()
    $project | Add-Member RemoteUrl $remote
    $project | Add-Member CurrentHead $head
    $project | Add-Member RecordedHead $recorded
    $project | Add-Member WasDirty (($status.Count -gt 0) -or ($head -ne $recorded))

    if ($project.WasDirty) {
        $dirtyProjects += $project
        Write-Host "  DIRTY $($project.Root)" -ForegroundColor Yellow
        $status | ForEach-Object { Write-Host "        $_" }
        if ($head -ne $recorded) { Write-Host "        HEAD differs from recorded gitlink: $head" }
    }
    else { Write-Host "  OK    $($project.Root)" -ForegroundColor Green }
}

$integrationPath = $null
if ($dirtyProjects.Count -gt 0) {
    if (@($projects | Group-Object { Normalize-RemoteUrl $_.RemoteUrl }).Count -ne 1) {
        Stop-WithError 'Discovered UtilityHelpersLib instances use different origin URLs.'
    }
    if (@($dirtyProjects | Group-Object SubmoduleBranch).Count -ne 1) {
        Stop-WithError 'Dirty submodules target different UtilityHelpersLib branches.'
    }

    $targetBranch = $dirtyProjects[0].SubmoduleBranch
    $operationId = Get-Date -Format 'yyyyMMdd-HHmmss'
    $integrationPath = $requestedIntegrationPath
    if (Test-Path -LiteralPath $integrationPath) {
        Stop-WithError "Integration directory already exists: '$integrationPath'. Finish/recover it or remove it explicitly first."
    }
    New-Item -ItemType Directory -Path (Split-Path $integrationPath -Parent) -Force | Out-Null

    Write-Info 'Creating recovery snapshots...'
    foreach ($project in $dirtyProjects) {
        $snapshot = New-RecoverySnapshot $project $operationId
        $project | Add-Member SnapshotCommit $snapshot.Commit
        $project | Add-Member BackupRef $snapshot.BackupRef
        Write-Host "  $($project.Name): $($snapshot.BackupRef) -> $($snapshot.Commit)" -ForegroundColor Green
    }

    Write-Info "Creating Integration repository at '$integrationPath'..."
    $parent = Split-Path $integrationPath -Parent
    Invoke-Git $parent @('clone', '--no-checkout', $dirtyProjects[0].RemoteUrl, $integrationPath) | Out-Null
    Invoke-Git $integrationPath @('fetch', 'origin', $targetBranch) | Out-Null

    # Use the same local and remote branch name and configure tracking. Visual
    # Studio can then push with its regular Push button directly to origin/Last
    # instead of publishing an accidental origin/integration branch.
    Invoke-Git $integrationPath @(
        'switch', '--create', $targetBranch, '--track', "origin/$targetBranch"
    ) | Out-Null
    foreach ($project in $dirtyProjects) {
        $candidateRef = "refs/heads/candidate/$($project.Slug)"
        Invoke-Git $integrationPath @('fetch', $project.SubmoduleDirectory, "$($project.BackupRef):$candidateRef") | Out-Null
        Write-Host "  candidate/$($project.Slug) <- $($project.SubmoduleDirectory)" -ForegroundColor Green
    }

    Start-Process explorer.exe -ArgumentList $integrationPath
    $integrationHead = Wait-ForPublishedIntegration $integrationPath $targetBranch

    Write-Host ""
    Write-Host "Published Integration commit: $integrationHead" -ForegroundColor Green
    Write-Host 'Dirty source checkouts will be replaced with published content.' -ForegroundColor Yellow
    Write-Host 'Their previous content remains in these recovery branches:' -ForegroundColor Yellow
    $dirtyProjects | ForEach-Object { Write-Host "  $($_.SubmoduleDirectory): $($_.BackupRef)" }
    $confirm = (Read-Host 'Continue with replacement and parent-project updates? [y/N]').Trim().ToUpperInvariant()
    if ($confirm -ne 'Y') { Stop-WithError 'Replacement cancelled. Integration and recovery branches were preserved.' }
}

Write-Info 'Updating all submodules and parent-project gitlinks...'
foreach ($project in $projects) {
    Invoke-Git $project.SubmoduleDirectory @('fetch', '--prune', 'origin', $project.SubmoduleBranch) | Out-Null
    $target = (Invoke-Git $project.SubmoduleDirectory @('rev-parse', 'FETCH_HEAD^{commit}')).Output[0].ToString().Trim()
    Update-LocalTrackingBranch $project.SubmoduleDirectory $project.SubmoduleBranch $target

    if ($project.WasDirty) {
        # Non-ignored untracked files are recoverable from the snapshot. Ignored
        # files remain because clean is intentionally called without -x.
        Invoke-Git $project.SubmoduleDirectory @('clean', '-fd') | Out-Null
        Invoke-Git $project.SubmoduleDirectory @('checkout', '--detach', '--force', $target) | Out-Null
    }
    else {
        Invoke-Git $project.SubmoduleDirectory @('switch', '--detach', $target) | Out-Null
    }

    $recorded = (Invoke-Git $project.Root @('rev-parse', "HEAD:$SubmodulePath")).Output[0].ToString().Trim()
    if ($recorded -eq $target) {
        Write-Host "  SKIP $($project.Root) - already current" -ForegroundColor DarkYellow
        continue
    }
    Invoke-Git $project.Root @('add', '--', $SubmodulePath) | Out-Null
    Invoke-Git $project.Root @('commit', '--only', '-m', $CommitMessage, '--', $SubmodulePath) | Out-Null
    Write-Host "  DONE $($project.Root): $recorded -> $target" -ForegroundColor Green
}

Write-Host ""
Write-Host '[UtilityHelpers updater] Completed. Parent-project commits were not pushed or merged.' -ForegroundColor Green
if ($integrationPath) {
    Write-Host "[UtilityHelpers updater] Integration available at: $integrationPath" -ForegroundColor Green
    Write-Host '[UtilityHelpers updater] Recovery branches were preserved in source submodules.' -ForegroundColor Green

    Write-Host ""
    $deleteCompleted = (Read-Host 'Delete the temporary Integration repository? [Y/n]').Trim().ToUpperInvariant()
    if (-not $deleteCompleted -or $deleteCompleted -eq 'Y') {
        try {
            Remove-IntegrationRepository $integrationPath $SubmoduleName
        }
        catch {
            Write-Host "WARN: Could not delete Integration: $($_.Exception.Message)" -ForegroundColor Yellow
            Write-Host "It can be removed manually later: $integrationPath" -ForegroundColor Yellow
        }
    }
}
