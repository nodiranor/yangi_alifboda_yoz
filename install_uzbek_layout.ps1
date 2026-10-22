# install_uzbek_layout.ps1
# ==============================================================================
# YAY - Yangi Alifboda Yoz
# Windows Til paneliga O'zbekcha (Lotin) tilini qo'shish skripti (Win + Space)
# ==============================================================================

try {
    $langs = Get-WinUserLanguageList
    $hasUz = $false
    foreach ($l in $langs) {
        if ($l.LanguageTag -like "uz*") {
            $hasUz = $true
            break
        }
    }
    if (-not $hasUz) {
        $uz = New-WinUserLanguageList "uz-Latn-UZ"
        foreach ($u in $uz) {
            $langs.Add($u)
        }
        Set-WinUserLanguageList $langs -Force
    }
} catch {
    # Exit gracefully on error
    exit 0
}
