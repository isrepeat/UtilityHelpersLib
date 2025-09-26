function NewLine {
    Write-Host ""
}


<# 
function Message {
    param (
        [string]$color = "White",
        [string]$prefix = "",
        [string]$text
    )
    Write-Host "$prefix$text" -ForegroundColor $color
}
#>

# ѕример использовани€:
# Message "Hello"
# Message -text "Hello"
# Message -color Red "Hello"
function Message {
    param(
        [Parameter(Position = 0, Mandatory = $true)]
        [string]$text,
        [string]$color = "White",
        [string]$prefix = ""
    )

    # ѕроверка: текст должен быть последним аргументом.
	# аккуратно разбираем исходную строку вызова
    $line = $MyInvocation.Line.TrimEnd() # убираем хвостовые пробелы/CRLF
    $tokens = [regex]::Split($line, "\s+") | Where-Object { $_ -ne "" }

    for ($i = 0; $i -lt $tokens.Length; $i++) {
        Write-Host "argsOrder[$i] = $($tokens[$i])"
    }

    # ЅерЄм последний токен, разворачиваем переменные/подвыражени€ и снимаем внешние кавычки
    $lastRaw = $tokens[-1]
    $lastExpanded = $ExecutionContext.InvokeCommand.ExpandString($lastRaw)
    $lastToken = $lastExpanded.Trim("'`"")

    if ($lastToken -ne $text) {
		Write-Host -ForegroundColor Red "[ERROR] lastToken ($lastToken) != text ($text)"
        throw "Text argument must be the last parameter."
    }

    Write-Host "$prefix$text" -ForegroundColor $color
}

function MessageError {
    param (
        [Parameter(Position = 0, Mandatory = $true)]
        [string]$text
    )
    Message -color Red -text $text
}

function MessageAction {
    param (
		[Parameter(Position = 0, Mandatory = $true)]
        [string]$text
    )
    Message -color Yellow -text $text
}


function TestColoredMessagePalette {
    $colors = @(
        "DarkBlue", "DarkGreen", "DarkCyan", "DarkRed", "DarkMagenta", "DarkYellow", "Gray",
        "DarkGray", "Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White"
    )

    $message = "The quick brown fox jumps over the lazy dog"

    Message -color Cyan -text "`nColor Palette Test:"
    foreach ($color in $colors) {
        Message -color $color -text ("{0,-12} : {1}" -f $color, $message)
    }
}


function WrapInQuotesIfNeeded {
    param ([string]$text)

    if ($text.StartsWith('"') -and $text.EndsWith('"')) {
        return $text
    }

    return '"' + $text + '"'
}


Export-ModuleMember -Function @(
    'Message',
    'MessageError',
    'MessageAction',
    'NewLine',
    'TestColoredMessagePalette',
    'WrapInQuotesIfNeeded'
)