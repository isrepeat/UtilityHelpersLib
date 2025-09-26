# Назначаем рабочим каталогом текущий каталог скрипта 
# (все относительные пути далее будут относительно этого каталога).
Set-Location -Path $PSScriptRoot

# Добавляем в PSModulePath путь к кастомным модулям,
# чтобы подключить их по названию модуля (вместо абсолютного пути).
$modulePath = Join-Path $PSScriptRoot "Modules"
if (-not ($env:PSModulePath -split ';' | Where-Object { $_ -eq $modulePath })) {
    $env:PSModulePath = "$modulePath;$env:PSModulePath"
}

# --- Import ---
Import-Module -Name MessagingModule -Prefix m:: -ErrorAction Stop

# --- Checks ---
git rev-parse --is-inside-work-tree 2>$null | Out-Null # "Out-Null" не выводит результат команды в консоль
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "This is not a Git repository."
}

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
	m::MessageError "GitHub CLI (gh) is not installed."
}

# --- Input ---
$HeadRaw = Read-Host "Enter Source Branch (head)"
$BaseRaw = Read-Host "Enter Target Branch (base)"
$Title = Read-Host "Enter Pull Request Title"
$NewBranchRaw = Read-Host "Enter optional new branch name (leave empty to skip)"
$Body = "Auto-generated PR from script"

# Нормализуем ввод
$Head = if ([string]::IsNullOrWhiteSpace($HeadRaw)) {
    $null
} else {
    $HeadRaw.Trim()
}

$Base = if ([string]::IsNullOrWhiteSpace($BaseRaw)) {
    $null
} else {
    $BaseRaw.Trim()
}

$NewBranch = if ([string]::IsNullOrWhiteSpace($NewBranchRaw)) {
    $null
} else {
    $NewBranchRaw.Trim()
}

# Валидация
if ([string]::IsNullOrWhiteSpace($Head)) { 
	m::MessageError "Head branch is required."
}
if ([string]::IsNullOrWhiteSpace($Base)) {
	m::MessageError "Base branch is required."
}
if ([string]::IsNullOrWhiteSpace($Title)) {
	m::MessageError "PR Title is required."
}


# --- Push head ---
m::MessageAction "Pushing '$Head' to origin..."
git push origin "$Head"
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "Failed to push '$Head' to origin."
}
m::Message "Pushed."


# --- Create PR ---
m::MessageAction "Creating Pull Request..."
gh pr create --title "$Title" --body "$Body" --base "$Base" --head "$Head" #| Out-Null
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "Failed to create Pull Request."
}

$PR_Number = gh pr list --state open --head "$Head" --json number --jq ".[0].number"
if ([string]::IsNullOrWhiteSpace($PR_Number)) { 
	m::MessageError "Unable to find Pull Request number."
}
m::Message "Pull Request created: #$PR_Number"


# --- Merge PR ---
m::MessageAction "Merging PR #$PR_Number..."
gh pr merge $PR_Number --merge --subject "Merge PR #$PR_Number $Title" #| Out-Null
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "Failed to merge Pull Request."
}
m::Message "Pull Request #$PR_Number successfully merged."


# --- Resolve repo path ---
$RepoPath = gh repo view --json nameWithOwner --jq ".nameWithOwner"
if ([string]::IsNullOrWhiteSpace($RepoPath)) { 
	m::MessageError "Unable to resolve repo path (nameWithOwner)."
}


# --- Delete source branch on remote ---
m::MessageAction "Deleting remote branch '$Head'..."
gh api "repos/$RepoPath/git/refs/heads/$Head" -X DELETE #| Out-Null
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "Failed to delete remote branch '$Head'."
}
m::Message "Remote branch '$Head' deleted."


# --- Prune locals ---
m::MessageAction "Pruning local references..."
git fetch origin --prune
if ($LASTEXITCODE -ne 0) { 
	m::MessageError "Failed to prune local references."
}
m::Message "Local references pruned."


# --- Optional: create new branch logic (handles empty/same-name cases) ---
$CurrentBranch = (git branch --show-current).Trim()
$shouldKeepHead =
    (-not $NewBranch) -or
    ($NewBranch -ieq $Head) -or
    ($NewBranch -ieq $CurrentBranch)

if ($shouldKeepHead) {
    m::MessageAction "Keeping the same branch name '$Head'. Rebasing it onto 'origin/$Base'..."

    # Обновим remote ссылки
    git fetch origin #| Out-Null

    # Переключимся на Head, если внезапно не на ней
    $current = (git branch --show-current).Trim()
    if ($current -ne $Head) {
        git checkout "$Head"
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Failed to checkout '$Head'."
        }
    }

    # Rebase на свежую origin/Base
    git rebase "origin/$Base"
    if ($LASTEXITCODE -ne 0) {
        m::MessageError "Rebase failed. Resolve conflicts and run 'git rebase --continue' (or '--abort')."
    }

    m::Message "Done. '$Head' is now rebased onto 'origin/$Base'."
}
else {
    # --- обычный сценарий: создаём новую ветку от origin/Base ---
    m::MessageAction "Refreshing 'origin/$Base'..."
    git fetch origin #| Out-Null

    m::MessageAction "Creating new branch '$NewBranch' from 'origin/$Base'..."
    git checkout --no-track -b "$NewBranch" "origin/$Base"
    if ($LASTEXITCODE -ne 0) {
        m::MessageError "Failed to create '$NewBranch' from origin/$Base."
    }

    # (Опционально) сразу опубликовать и назначить upstream:
    # m::MessageAction "Publishing '$NewBranch' and setting upstream..."
    # git push -u origin "$NewBranch"
    # if ($LASTEXITCODE -ne 0) { m::MessageError "Failed to publish '$NewBranch' with upstream." }
    # m::Message "'$NewBranch' published with upstream 'origin/$NewBranch'."

    # После переключения можно удалить локальную Head
    $isHeadExists = git branch --list "$Head"
    if ($isHeadExists) {
        m::MessageAction "Deleting local branch '$Head'..."
        git branch -D "$Head"
        if ($LASTEXITCODE -ne 0) { m::MessageError "Failed to delete local branch '$Head'." }
        m::Message "Local branch '$Head' deleted."
    }

    m::Message "Done. New branch '$NewBranch' is created from 'origin/$Base'."
}