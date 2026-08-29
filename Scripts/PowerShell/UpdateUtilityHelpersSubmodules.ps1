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
    [string]$ScanSubmoduleIntegrationBranch = "Last",

    [string]$ScanSubmoduleUpdateBranch = "master",
    [string]$AdditionalProjectPaths = $null,
    [string]$ProjectOverrides = $null,
    [string]$SubmodulePath = "UtilityHelpersLib",
    [string]$CommitMessage = "Update UtilityHelpersLib submodule",

    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$SubmoduleName = "UtilityHelpersLib",

    [string]$IntegrationSolutionFile = "UtilityHelpersLib.sln",

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
        if ($parts.Count -notin @(3, 4)) {
            Stop-WithError "Invalid override '$entry'. Expected: path|project-branch|integration-branch|update-branch"
        }
        $result += [pscustomobject]@{
            InputPath                 = $parts[0].Trim().Trim('"')
            ProjectBranch             = $parts[1].Trim()
            SubmoduleIntegrationBranch = $parts[2].Trim()
            SubmoduleUpdateBranch      = if ($parts.Count -eq 4) { $parts[3].Trim() } else { $parts[2].Trim() }
        }
    }
    $result
}

function Parse-ProjectPaths {
    param([string]$Value)

    if (-not $Value -or -not $Value.Trim()) { return @() }
    @($Value -split ';' |
        ForEach-Object { $_.Trim().Trim('"') } |
        Where-Object { $_ })
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

# Open a deliberately empty Visual Studio solution inside the temporary
# Integration clone. The real repository solution can be large and unrelated
# to manually choosing candidate commits; a blank solution keeps Git tooling
# available without loading projects from the clone.
function Open-IntegrationRepository {
    param([string]$Path)

    $solutionFileName = '.integration-workspace.sln'
    $solutionPath = Join-Path $Path $solutionFileName
    if (-not (Test-Path -LiteralPath $solutionPath -PathType Leaf)) {
        @"
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 18
VisualStudioVersion = 18.0.0.0
MinimumVisualStudioVersion = 10.0.40219.1
Global
EndGlobal
"@ | Set-Content -LiteralPath $solutionPath -Encoding utf8

        # The workspace is generated locally for this temporary repository.
        # Keep it out of `git status`, otherwise the manual R gate would see
        # the integration worktree as dirty.
        $excludePath = (Invoke-Git $Path @('rev-parse', '--git-path', 'info/exclude')).Output[0].ToString().Trim()
        if (-not [IO.Path]::IsPathRooted($excludePath)) { $excludePath = Join-Path $Path $excludePath }
        $excludeEntries = if (Test-Path -LiteralPath $excludePath) { @(Get-Content -LiteralPath $excludePath) } else { @() }
        if ($excludeEntries -notcontains $solutionFileName) {
            Add-Content -LiteralPath $excludePath -Value $solutionFileName
        }
    }
    Start-Process -FilePath $solutionPath
}

# Close only Explorer windows currently showing the Integration directory. This
# does not terminate Visual Studio or unrelated Explorer windows.
function Close-IntegrationExplorerWindows {
    param([string]$Path)

    $targetPath = [IO.Path]::GetFullPath($Path).TrimEnd([char[]]@('\', '/'))
    try {
        $shell = New-Object -ComObject Shell.Application
        foreach ($window in @($shell.Windows())) {
            try {
                $folderPath = $window.Document.Folder.Self.Path
                if ($folderPath -and ([IO.Path]::GetFullPath($folderPath).TrimEnd([char[]]@('\', '/')) -ieq $targetPath)) {
                    $window.Quit()
                }
            }
            catch {
                # Ignore non-Explorer Shell windows and windows being closed.
            }
        }
    }
    catch {
        Write-Host "WARN: Could not close the Integration Explorer window: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Enumerate Visual Studio DTE objects through the COM Running Object Table and
# close only instances whose loaded solution belongs to this Integration clone.
function Close-IntegrationVisualStudioSessions {
    param([string]$Path)

    if (-not ('UtilityHelpersUpdater.RunningObjects' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

namespace UtilityHelpersUpdater
{
    public static class RunningObjects
    {
        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable table);

        public static object[] GetAll()
        {
            IRunningObjectTable table;
            if (GetRunningObjectTable(0, out table) != 0)
                return new object[0];

            var result = new List<object>();
            IEnumMoniker enumerator;
            table.EnumRunning(out enumerator);
            enumerator.Reset();
            var monikers = new IMoniker[1];
            IntPtr fetched = Marshal.AllocCoTaskMem(sizeof(int));
            try
            {
                while (enumerator.Next(1, monikers, fetched) == 0)
                {
                    object instance;
                    try
                    {
                        table.GetObject(monikers[0], out instance);
                        if (instance != null)
                            result.Add(instance);
                    }
                    catch { }
                    finally
                    {
                        if (monikers[0] != null)
                            Marshal.ReleaseComObject(monikers[0]);
                    }
                }
            }
            finally
            {
                Marshal.FreeCoTaskMem(fetched);
                Marshal.ReleaseComObject(enumerator);
                Marshal.ReleaseComObject(table);
            }
            return result.ToArray();
        }
    }
}
'@
    }

    $targetPath = [IO.Path]::GetFullPath($Path).TrimEnd([char[]]@('\', '/'))
    $targetPrefix = $targetPath + [IO.Path]::DirectorySeparatorChar
    $closedProcessIds = @()
    foreach ($instance in [UtilityHelpersUpdater.RunningObjects]::GetAll()) {
        try {
            $solutionPath = $instance.Solution.FullName
            if (-not $solutionPath) { continue }
            $fullSolutionPath = [IO.Path]::GetFullPath($solutionPath)
            if (-not $fullSolutionPath.StartsWith($targetPrefix, [StringComparison]::OrdinalIgnoreCase)) { continue }

            Write-Info "Closing Visual Studio Integration session: $fullSolutionPath"
            $processId = $null
            try { $processId = $instance.ProcessID } catch { }
            $instance.SuppressUI = $true
            $instance.Solution.Close($false)
            $instance.Quit()
            if ($processId) { $closedProcessIds += [int]$processId }
        }
        catch {
            # Most ROT objects are unrelated COM applications; ignore them. A
            # matching VS instance that refuses to close will make deletion fail
            # later with the actual filesystem error.
        }
        finally {
            if ([Runtime.InteropServices.Marshal]::IsComObject($instance)) {
                [void][Runtime.InteropServices.Marshal]::ReleaseComObject($instance)
            }
        }
    }

    # DTE.Quit is asynchronous. Wait briefly so devenv and its indexing services
    # can release .vsidx and other files before recursive deletion starts.
    foreach ($processId in ($closedProcessIds | Sort-Object -Unique)) {
        Wait-Process -Id $processId -Timeout 15 -ErrorAction SilentlyContinue
    }
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

    Close-IntegrationVisualStudioSessions $fullPath
    Close-IntegrationExplorerWindows $fullPath

    # Background Visual Studio indexers can retain .vsidx handles briefly even
    # after devenv exits. Retry the exact validated target for up to 15 seconds.
    $lastError = $null
    for ($attempt = 1; $attempt -le 30; $attempt++) {
        try {
            Remove-Item -LiteralPath $fullPath -Recurse -Force -ErrorAction Stop
            $lastError = $null
            break
        }
        catch {
            $lastError = $_
            Start-Sleep -Milliseconds 500
        }
    }
    if ($lastError) { throw $lastError }
    Write-Host "Deleted temporary Integration repository: $fullPath" -ForegroundColor Green
}

# Locate exact repository roots at ScanRoot and one directory level below it.
function Find-Projects {
    param(
        [object[]]$Overrides,
        [string[]]$AdditionalPaths
    )

    try { $resolvedRoot = (Resolve-Path -LiteralPath $ScanRoot).Path }
    catch { Stop-WithError "Scan root does not exist: '$ScanRoot'" }

    $candidates = @($resolvedRoot)
    if ($ScanDepth -eq 1) {
        $candidates += @(Get-ChildItem -LiteralPath $resolvedRoot -Directory -Force | Select-Object -ExpandProperty FullName)
    }

    # Wrapper-defined paths make it possible to include selected repositories
    # below the normal scan depth (or outside ScanRoot) without broad recursive
    # discovery. Normalize and de-duplicate them before validating candidates.
    foreach ($additionalPath in $AdditionalPaths) {
        try { $candidates += (Resolve-Path -LiteralPath $additionalPath -ErrorAction Stop).Path }
        catch { Stop-WithError "Additional project path does not exist: '$additionalPath'" }
    }
    $candidates = @($candidates | Sort-Object -Unique)

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
            SubmoduleIntegrationBranch = if ($override) { $override.SubmoduleIntegrationBranch } else { $ScanSubmoduleIntegrationBranch }
            SubmoduleUpdateBranch      = if ($override) { $override.SubmoduleUpdateBranch } else { $ScanSubmoduleUpdateBranch }
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

            # Git refuses `branch --force` when the branch is checked out by
            # this worktree. Detaching at the current commit is safe: it does
            # not modify files or discard local changes, and the caller later
            # checks out the published target explicitly.
            $checkedOut = Invoke-Git $Repository @('branch', '--show-current')
            $checkedOutBranch = if ($checkedOut.Output.Count -gt 0) {
                $checkedOut.Output[0].ToString().Trim()
            } else {
                ''
            }
            if ($checkedOutBranch -ceq $Branch) {
                Invoke-Git $Repository @('switch', '--detach') | Out-Null
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
            Open-IntegrationRepository $Repository
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

# Locate GitHub CLI using PATH or its standard winget installation directory.
function Get-GitHubCli {
    $command = Get-Command gh -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $standardPath = Join-Path $env:ProgramFiles 'GitHub CLI\gh.exe'
    if (Test-Path -LiteralPath $standardPath) { return $standardPath }
    Stop-WithError "GitHub CLI (gh) was not found. Install it and run 'gh auth login'."
}

function Invoke-GitHubCli {
    param(
        [string]$Executable,
        [string[]]$Arguments,
        [switch]$AllowFailure
    )

    $savedErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = @(& $Executable @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $savedErrorActionPreference
    }

    if ($exitCode -ne 0 -and -not $AllowFailure) {
        $details = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
        throw "gh $($Arguments -join ' ') failed with exit code $exitCode.`n$details"
    }
    [pscustomobject]@{ ExitCode = $exitCode; Output = $output }
}

# Merge the published working branch through a GitHub Pull Request, then recreate
# that working branch from the merged stable branch as AutoPrMerge does.
function Merge-IntegrationBranchUsingPullRequest {
    param(
        [string]$Repository,
        [string]$RemoteUrl,
        [string]$IntegrationBranch,
        [string]$IntegrationCommit,
        [string]$UpdateBranch,
        [string]$Title
    )

    if ($IntegrationBranch -ceq $UpdateBranch) { return $IntegrationCommit }

    $gh = Get-GitHubCli
    $auth = Invoke-GitHubCli $gh @('auth', 'status') -AllowFailure
    if ($auth.ExitCode -ne 0) { Stop-WithError "GitHub CLI is not authenticated. Run 'gh auth login'." }

    $savedGhRepo = $env:GH_REPO
    $env:GH_REPO = $RemoteUrl
    try {
        Invoke-Git $Repository @('fetch', 'origin', '--prune') | Out-Null
        $commitsToMerge = [int](Invoke-Git $Repository @(
            'rev-list', '--count', "origin/$UpdateBranch..origin/$IntegrationBranch"
        )).Output[0]

        if ($commitsToMerge -gt 0) {
            $prList = Invoke-GitHubCli $gh @(
                'pr', 'list', '--state', 'open', '--base', $UpdateBranch,
                '--head', $IntegrationBranch, '--json', 'number', '--jq', '.[0].number'
            )
            $prNumber = (($prList.Output | ForEach-Object { $_.ToString() }) -join "`n").Trim()

            if (-not $prNumber) {
                Write-Info "Creating Pull Request '$IntegrationBranch' -> '$UpdateBranch'..."
                $creation = Invoke-GitHubCli $gh @(
                    'pr', 'create', '--title', $Title,
                    '--body', 'Auto-generated PR from UtilityHelpers submodule updater.',
                    '--base', $UpdateBranch, '--head', $IntegrationBranch
                )
                $creationText = ($creation.Output | ForEach-Object { $_.ToString() }) -join "`n"
                if ($creationText -match '/pull/(\d+)\b') {
                    $prNumber = $matches[1]
                }
                else {
                    $prList = Invoke-GitHubCli $gh @(
                        'pr', 'list', '--state', 'open', '--base', $UpdateBranch,
                        '--head', $IntegrationBranch, '--json', 'number', '--jq', '.[0].number'
                    )
                    $prNumber = (($prList.Output | ForEach-Object { $_.ToString() }) -join "`n").Trim()
                }
            }
            else {
                Write-Info "Using existing Pull Request #$prNumber."
            }

            if (-not $prNumber) { Stop-WithError 'Unable to resolve Pull Request number.' }
            Write-Info "Merging Pull Request #$prNumber..."
            Invoke-GitHubCli $gh @(
                'pr', 'merge', $prNumber, '--merge', '--subject', "Merge PR #$prNumber $Title"
            ) | Out-Null
        }
        else {
            Write-Info "No commits need merging from '$IntegrationBranch' into '$UpdateBranch'."
        }

        Invoke-Git $Repository @('fetch', 'origin', '--prune') | Out-Null
        $updateCommit = (Invoke-Git $Repository @('rev-parse', "origin/$UpdateBranch")).Output[0].ToString().Trim()
        $containsIntegration = Invoke-Git $Repository @(
            'merge-base', '--is-ancestor', $IntegrationCommit, $updateCommit
        ) -AllowFailure
        if ($containsIntegration.ExitCode -ne 0) {
            Stop-WithError "origin/$UpdateBranch does not contain the published $IntegrationBranch commit after PR merge."
        }

        $remoteIntegration = Invoke-Git $Repository @(
            'show-ref', '--verify', '--quiet', "refs/remotes/origin/$IntegrationBranch"
        ) -AllowFailure
        if ($remoteIntegration.ExitCode -eq 0) {
            Write-Info "Deleting old remote branch '$IntegrationBranch'..."
            Invoke-Git $Repository @('push', 'origin', '--delete', $IntegrationBranch) | Out-Null
        }

        Invoke-Git $Repository @('fetch', 'origin', '--prune') | Out-Null
        Invoke-Git $Repository @('switch', '--detach', $updateCommit) | Out-Null
        Invoke-Git $Repository @('branch', '--force', $IntegrationBranch, $updateCommit) | Out-Null
        Invoke-Git $Repository @('switch', $IntegrationBranch) | Out-Null
        Invoke-Git $Repository @('push', '--set-upstream', 'origin', "$IntegrationBranch`:$IntegrationBranch") | Out-Null

        Write-Host "origin/$UpdateBranch and origin/$IntegrationBranch now point to $updateCommit" -ForegroundColor Green
        return $updateCommit
    }
    finally {
        $env:GH_REPO = $savedGhRepo
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
$additionalPaths = @(Parse-ProjectPaths $AdditionalProjectPaths)
$projects = @(Find-Projects -Overrides $overrides -AdditionalPaths $additionalPaths)
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
    if (@($dirtyProjects | Group-Object SubmoduleIntegrationBranch).Count -ne 1) {
        Stop-WithError 'Dirty submodules target different UtilityHelpersLib branches.'
    }
    if (@($projects | Group-Object SubmoduleUpdateBranch).Count -ne 1) {
        Stop-WithError 'Projects use different final UtilityHelpersLib update branches; automatic PR merge requires one common branch.'
    }

    $targetBranch = $dirtyProjects[0].SubmoduleIntegrationBranch
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

    Open-IntegrationRepository $integrationPath
    $integrationHead = Wait-ForPublishedIntegration $integrationPath $targetBranch

    $updateBranch = $projects[0].SubmoduleUpdateBranch
    $defaultMergeTitle = "Update $SubmoduleName"
    $mergeTitle = (Read-Host "Pull Request title [$defaultMergeTitle]").Trim()
    if (-not $mergeTitle) { $mergeTitle = $defaultMergeTitle }

    Merge-IntegrationBranchUsingPullRequest `
        -Repository $integrationPath `
        -RemoteUrl $dirtyProjects[0].RemoteUrl `
        -IntegrationBranch $targetBranch `
        -IntegrationCommit $integrationHead `
        -UpdateBranch $updateBranch `
        -Title $mergeTitle | Out-Null

    Write-Host ""
    Write-Host "Published Integration commit: $integrationHead" -ForegroundColor Green
    Write-Host 'Dirty source checkouts will be replaced with published content.' -ForegroundColor Yellow
    Write-Host 'Temporary recovery branches protect their previous content until all project updates succeed:' -ForegroundColor Yellow
    $dirtyProjects | ForEach-Object { Write-Host "  $($_.SubmoduleDirectory): $($_.BackupRef)" }
    $confirm = (Read-Host 'Continue with replacement and parent-project updates? [y/N]').Trim().ToUpperInvariant()
    if ($confirm -ne 'Y') { Stop-WithError 'Replacement cancelled. Integration and recovery branches were preserved.' }
}

Write-Info 'Updating all submodules and parent-project gitlinks...'
foreach ($project in $projects) {
    Invoke-Git $project.SubmoduleDirectory @('fetch', '--prune', 'origin', $project.SubmoduleIntegrationBranch) | Out-Null
    $integrationTarget = (Invoke-Git $project.SubmoduleDirectory @('rev-parse', 'FETCH_HEAD^{commit}')).Output[0].ToString().Trim()
    Update-LocalTrackingBranch $project.SubmoduleDirectory $project.SubmoduleIntegrationBranch $integrationTarget

    Invoke-Git $project.SubmoduleDirectory @('fetch', '--prune', 'origin', $project.SubmoduleUpdateBranch) | Out-Null
    $target = (Invoke-Git $project.SubmoduleDirectory @('rev-parse', 'FETCH_HEAD^{commit}')).Output[0].ToString().Trim()
    if ($project.SubmoduleUpdateBranch -cne $project.SubmoduleIntegrationBranch) {
        Update-LocalTrackingBranch $project.SubmoduleDirectory $project.SubmoduleUpdateBranch $target
    }

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

# Recovery refs are needed until every parent project has been updated
# successfully. After full success, remove all updater-created backup refs from
# each dirty submodule. An earlier failure exits before this cleanup point.
if ($dirtyProjects.Count -gt 0) {
    Write-Info 'Removing completed-operation recovery branches...'
    foreach ($project in $dirtyProjects) {
        $backupRefs = @(Invoke-Git $project.SubmoduleDirectory @(
            'for-each-ref', '--format=%(refname)', 'refs/heads/utility-updater/backup'
        )).Output
        foreach ($backupRef in $backupRefs) {
            $backupRefName = $backupRef.ToString().Trim()
            if (-not $backupRefName) { continue }
            Invoke-Git $project.SubmoduleDirectory @('update-ref', '-d', $backupRefName) | Out-Null
            Write-Host "  DELETED $backupRefName" -ForegroundColor Green
        }
    }
}

Write-Host ""
Write-Host '[UtilityHelpers updater] Completed. Parent-project commits were not pushed or merged.' -ForegroundColor Green
if ($integrationPath) {
    Write-Host "[UtilityHelpers updater] Integration available at: $integrationPath" -ForegroundColor Green
    Write-Host '[UtilityHelpers updater] Recovery branches for this completed operation were deleted.' -ForegroundColor Green

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
