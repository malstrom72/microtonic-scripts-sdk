# ==== BEGIN PRODUCT CONFIGURATION ====
# The help text, the -Identifier default and the variables below are the only parts of this file
# that differ between the Microtonic and Synplant SDKs. Everything below the END marker is shared
# and must stay byte-identical in both repositories.

<#
.SYNOPSIS
    Print the absolute path of the Microtonic Scripts folder without a running Microtonic.

.DESCRIPTION
    Helps during bootstrap before the JSConsole bridge exists. The bridge can report DIRS.SCRIPTS
    directly, but only after the bridged JSConsole has been installed.

    AUTHORITATIVE NOTE: DIRS.SCRIPTS / Open Scripts Folder in Microtonic is the final word. This tool
    mirrors the engine's Windows registry lookup to give a high-confidence answer, but callers must
    still confirm the result before writing to or linking the folder.

    Windows resolution:
        scripts = <SetupPath>\Microtonic Scripts

    SetupPath is read from HKLM\SOFTWARE\Sonic Charge\<Identifier>, falling back to _Default. Both the
    64-bit and 32-bit WOW6432Node views are checked (Microtonic's installer normally writes only the
    WOW6432Node view). SetupPath is the Sonic Charge installation directory itself
    (e.g. C:\Program Files\Sonic Charge), so "Microtonic Scripts" is joined to it directly, alongside
    the sibling Microtonic Docs / Presets / Drum Patches folders. If the registry read fails, the
    engine can fall back to the plugin binary directory; a standalone tool cannot know that path, so
    this tool reports failure and points at Open Scripts Folder / the bridge.

.PARAMETER Identifier
    Product identifier / registry sub-key. Defaults to "Microtonic".

.PARAMETER Verify
    Also check that the resolved folder exists.

.OUTPUTS
    The resolved absolute path on stdout, also when -Verify finds it missing, so a cold-start
    bootstrap still gets the candidate to create or link. Diagnostics go to stderr. Exit code 0 if
    a path was resolved (and, with -Verify, exists), 1 if no path could be resolved, 2 if -Verify
    found that the resolved folder does not exist yet.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File locate-scripts-folder.ps1 -Verify
#>

[CmdletBinding()]
param(
    [string]$Identifier = 'Microtonic',
    [switch]$Verify
)

$ProductName = 'Microtonic'
$ConsoleName = 'JSConsole'
$ScriptsFolderName = 'Microtonic Scripts'
# Subfolders that -Verify reports on (informational only; their absence is not an error).
$ExpectedSubfolders = @()
# ==== END PRODUCT CONFIGURATION ====

function Write-Diag([string]$message) {
    [Console]::Error.WriteLine($message)
}

function Get-SetupPath([string]$regPath) {
    try {
        $item = Get-ItemProperty -LiteralPath $regPath -Name 'SetupPath' -ErrorAction Stop
        return $item.SetupPath
    } catch {
        return $null
    }
}

function Resolve-Windows([string]$id) {
    $candidates = @(
        "HKLM:\SOFTWARE\Sonic Charge\$id",
        "HKLM:\SOFTWARE\WOW6432Node\Sonic Charge\$id",
        'HKLM:\SOFTWARE\Sonic Charge\_Default',
        'HKLM:\SOFTWARE\WOW6432Node\Sonic Charge\_Default'
    )

    foreach ($regPath in $candidates) {
        $setupPath = Get-SetupPath $regPath
        if ($setupPath) {
            Write-Diag "found SetupPath in $regPath : $setupPath"
            return (Join-Path $setupPath $ScriptsFolderName)
        }
    }

    return $null
}

$onWindows = $true
$onMac = $false
if (Get-Variable -Name IsWindows -ErrorAction SilentlyContinue) { $onWindows = $IsWindows }
if (Get-Variable -Name IsMacOS -ErrorAction SilentlyContinue) { $onMac = $IsMacOS }

$scriptsPath = $null
if ($onWindows) {
    $scriptsPath = Resolve-Windows $Identifier
    if (-not $scriptsPath) {
        Write-Diag "Could not read SetupPath from the registry (HKLM\SOFTWARE\Sonic Charge\$Identifier or _Default)."
        Write-Diag 'The engine may fall back to the plugin binary directory, which this tool cannot know.'
        Write-Diag "Use Open Scripts Folder in $ProductName (available once the folder exists), or read DIRS.SCRIPTS over the $ConsoleName bridge."
        exit 1
    }
} elseif ($onMac) {
    $scriptsPath = "/Library/Application Support/Sonic Charge/$ScriptsFolderName"
    Write-Diag 'macOS: reporting the documented standard location. Confirm before writing.'
} else {
    Write-Diag 'Unsupported platform.'
    exit 1
}

$exitCode = 0
if ($Verify) {
    if (Test-Path -LiteralPath $scriptsPath -PathType Container) {
        $details = @()
        foreach ($subfolder in $ExpectedSubfolders) {
            $present = Test-Path -LiteralPath (Join-Path $scriptsPath $subfolder) -PathType Container
            $details += ("$subfolder/ present: " + $(if ($present) { 'yes' } else { 'no' }))
        }
        if ($details.Count -gt 0) {
            Write-Diag ('verified folder exists; ' + ($details -join '; '))
        } else {
            Write-Diag 'verified folder exists.'
        }
    } else {
        Write-Diag "NOT FOUND: resolved path does not exist yet: $scriptsPath"
        Write-Diag "On a fresh $ProductName install this is expected: create the folder or link it to a project"
        Write-Diag 'scripts folder (one elevated step), as described in the SDK''s first-ever install / cold-start docs.'
        $exitCode = 2
    }
}

Write-Output $scriptsPath
exit $exitCode
