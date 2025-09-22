function Message {
    param (
        [string]$color = "White",
        [string]$prefix = "",
        [string]$text
    )

    Write-Host "$prefix$text" -ForegroundColor $color
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

function NewLine {
    Write-Host ""
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
    'NewLine',
    'TestColoredMessagePalette',
    'WrapInQuotesIfNeeded'
)