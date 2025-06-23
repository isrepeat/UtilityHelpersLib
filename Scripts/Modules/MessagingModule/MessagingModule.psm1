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


<#

class _Module {
    static [void] Write-ColoredMessage(
        [string]$message,
        [string]$color = 'White',
        [string]$prefix = ''
    ) {
        Write-Host "$prefix$message" -ForegroundColor $color
    }
}

function GetModule {
    return [_Module]
}
#>

<#

function Say-Hello {
    param([string]$Name)
    Write-Host "Hello, $Name"
}

function Say-Goodbye {
    param([string]$Name)
    Write-Host "Goodbye, $Name"
}

function GetMyFunction {
    param(
        [ValidateSet("hello", "bye")]
        [string]$FunctionName
    )

    switch ($FunctionName) {
        "hello" { return (Get-Command Say-Hello).ScriptBlock }
        "bye"   { return (Get-Command Say-Goodbye).ScriptBlock }
    }
}
#>



Export-ModuleMember -Function @(
#    'GetMyFunction',
    'Message',
    'NewLine',
    'TestColoredMessagePalette',
    'WrapInQuotesIfNeeded'
)