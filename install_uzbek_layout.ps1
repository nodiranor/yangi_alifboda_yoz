# Adds the Uzbek (Latin) language to the current user's language list.
# Windows 8 and later only. The installer runs this; the program does the same from its window.

try {
    $langs = Get-WinUserLanguageList
    $hasUz = $false
    foreach ($l in $langs) {
        if ($l.LanguageTag -like "uz*") {
            $hasUz = $true
            break
        }
    }
    # Add uz-Latn-UZ (Uzbek, Latin script) at the end of the list. -Force skips the confirmation prompt.
    if (-not $hasUz) {
        $uz = New-WinUserLanguageList "uz-Latn-UZ"
        foreach ($u in $uz) {
            $langs.Add($u)
        }
        Set-WinUserLanguageList $langs -Force
    }
} catch {
    # Nothing to do if the language list cannot be changed.
    exit 0
}
