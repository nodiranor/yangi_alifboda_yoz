#Requires AutoHotkey v2.0
#SingleInstance Force
Persistent
SendMode "Input"
SetKeyDelay -1

; ==============================================================================
; UZBEK NEW ALPHABET NATIVE HELPER (Windows Lightweight Daemon)
; Reform Mapping: O' -> Ö | G' -> Ğ | Sh -> Ş | Ch -> Ç
; ==============================================================================

global IsEnabled := true
global InstantSingleC := false       ; Optional: Make single 'c' type 'ç' directly
global PopupActive := false
global PendingReplacement := ""
global LastKey := ""
global LastKeyTime := 0
global DoubleTapThreshold := 350     ; Max milliseconds between double taps

; Setup System Tray Menu (Uzbek Latin / Cyrillic)
A_IconTip := "Yangi O'zbek Alifbosi / Янги Ўзбек Алифбоси"
Tray := A_TrayMenu
Tray.Delete()
Tray.Add("Yoqish / Ёқиш (Faol / Фаол)", ToggleScript)
Tray.Check("Yoqish / Ёқиш (Faol / Фаол)")
Tray.Add("Bitta 'C' -> 'Ç' rejimi / Битта 'C' -> 'Ç' режими", ToggleSingleC)
Tray.Add()
Tray.Add("Chiqish / Чиқиш", (*) => ExitApp())

; --- UI 1: Choice Popup for Double-Tap (ss, oo, gg, cc) ---
ChoiceGui := Gui("+AlwaysOnTop -Caption +ToolWindow +Border +E0x20")
ChoiceGui.BackColor := "1E1E2E"
ChoiceGui.SetFont("s10 cCDD6F4", "Segoe UI Semibold")
ChoiceText := ChoiceGui.Add("Text", "x10 y6", "")

; --- UI 2: Coaching Toast for Legacy Typing (sh, ch, o', g') ---
CoachGui := Gui("+AlwaysOnTop -Caption +ToolWindow +Border +E0x20")
CoachGui.BackColor := "FFFBEB"
CoachGui.SetFont("s9 c92400E", "Segoe UI")
CoachText := CoachGui.Add("Text", "x10 y6", "")

; ==============================================================================
; 1. LEGACY COMBINATION REPLACER (O', G', Sh, Ch) + BILINGUAL COACHING TIP
; ==============================================================================
#HotIf IsEnabled

; o' -> ö
:?*C:o'::  {
    ReplaceLegacy("ö", "Maslahat: Tezroq yozish uchun 'oo' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'oo' + [Tab] босинг")
}
:?*C:o``:: {
    ReplaceLegacy("ö", "Maslahat: Tezroq yozish uchun 'oo' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'oo' + [Tab] босинг")
}
:?*C:o‘::  {
    ReplaceLegacy("ö", "Maslahat: Tezroq yozish uchun 'oo' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'oo' + [Tab] босинг")
}
:?*C:o’::  {
    ReplaceLegacy("ö", "Maslahat: Tezroq yozish uchun 'oo' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'oo' + [Tab] босинг")
}

; O' -> Ö
:?*C:O'::  {
    ReplaceLegacy("Ö", "Maslahat: Tezroq yozish uchun 'OO' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'OO' + [Tab] босинг")
}
:?*C:O``:: {
    ReplaceLegacy("Ö", "Maslahat: Tezroq yozish uchun 'OO' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'OO' + [Tab] босинг")
}
:?*C:O‘::  {
    ReplaceLegacy("Ö", "Maslahat: Tezroq yozish uchun 'OO' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'OO' + [Tab] босинг")
}
:?*C:O’::  {
    ReplaceLegacy("Ö", "Maslahat: Tezroq yozish uchun 'OO' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'OO' + [Tab] босинг")
}

; g' -> ğ
:?*C:g'::  {
    ReplaceLegacy("ğ", "Maslahat: Tezroq yozish uchun 'gg' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'gg' + [Tab] босинг")
}
:?*C:g``:: {
    ReplaceLegacy("ğ", "Maslahat: Tezroq yozish uchun 'gg' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'gg' + [Tab] босинг")
}
:?*C:g‘::  {
    ReplaceLegacy("ğ", "Maslahat: Tezroq yozish uchun 'gg' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'gg' + [Tab] босинг")
}
:?*C:g’::  {
    ReplaceLegacy("ğ", "Maslahat: Tezroq yozish uchun 'gg' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'gg' + [Tab] босинг")
}

; G' -> Ğ
:?*C:G'::  {
    ReplaceLegacy("Ğ", "Maslahat: Tezroq yozish uchun 'GG' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'GG' + [Tab] босинг")
}
:?*C:G``:: {
    ReplaceLegacy("Ğ", "Maslahat: Tezroq yozish uchun 'GG' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'GG' + [Tab] босинг")
}
:?*C:G‘::  {
    ReplaceLegacy("Ğ", "Maslahat: Tezroq yozish uchun 'GG' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'GG' + [Tab] босинг")
}
:?*C:G’::  {
    ReplaceLegacy("Ğ", "Maslahat: Tezroq yozish uchun 'GG' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'GG' + [Tab] босинг")
}

; sh / Sh / SH -> ş / Ş
:?*C:sh::  {
    ReplaceLegacy("ş", "Maslahat: Tezroq yozish uchun 'ss' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'ss' + [Tab] босинг")
}
:?*C:Sh::  {
    ReplaceLegacy("Ş", "Maslahat: Tezroq yozish uchun 'SS' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'SS' + [Tab] босинг")
}
:?*C:SH::  {
    ReplaceLegacy("Ş", "Maslahat: Tezroq yozish uchun 'SS' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'SS' + [Tab] босинг")
}

; ch / Ch / CH -> ç / Ç
:?*C:ch::  {
    ReplaceLegacy("ç", "Maslahat: Tezroq yozish uchun 'cc' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'cc' + [Tab] босинг")
}
:?*C:Ch::  {
    ReplaceLegacy("Ç", "Maslahat: Tezroq yozish uchun 'CC' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'CC' + [Tab] босинг")
}
:?*C:CH::  {
    ReplaceLegacy("Ç", "Maslahat: Tezroq yozish uchun 'CC' + [Tab] bosing`nМаслаҳат: Тезроқ ёзиш учун 'CC' + [Tab] босинг")
}

; ==============================================================================
; 2. DOUBLE-TAP DETECTOR (ss -> ş, oo -> ö, gg -> ğ, cc -> ç)
; ==============================================================================
~*s::HandleTap("s", "S", "ş", "Ş")
~*o::HandleTap("o", "O", "ö", "Ö")
~*g::HandleTap("g", "G", "ğ", "Ğ")
~*c::HandleTap("c", "C", "ç", "Ç")

; Reset double-tap tracker if user presses other keys
~*Space::
~*BackSpace::
~*Left::
~*Right::
~*Up::
~*Down::
~*a::
~*b::
~*d::
~*e::
~*f::
~*i::
~*j::
~*k::
~*l::
~*m::
~*n::
~*p::
~*q::
~*r::
~*t::
~*u::
~*v::
~*w::
~*x::
~*y::
~*z:: {
    global LastKey := ""
    if PopupActive
        DismissPopup()
}

#HotIf

; ==============================================================================
; 3. CONFIRMATION HOTKEYS WHEN DOUBLE-TAP POPUP IS VISIBLE
; ==============================================================================
#HotIf IsEnabled and PopupActive

Tab::
Enter:: {
    global PendingReplacement
    SendInput "{BackSpace 2}" . PendingReplacement
    DismissPopup()
}

Escape:: {
    DismissPopup()
}

#HotIf

; ==============================================================================
; 4. OPTIONAL: INSTANT SINGLE 'C' -> 'Ç' MODE
; ==============================================================================
#HotIf IsEnabled and InstantSingleC
*c:: {
    if GetKeyState("Shift", "P") xor GetKeyState("CapsLock", "T")
        SendInput "Ç"
    else
        SendInput "ç"
}
#HotIf

; ==============================================================================
; CORE FUNCTIONS
; ==============================================================================

ReplaceLegacy(NewChar, TipMessage) {
    global LastKey := ""
    DismissPopup()
    SendInput NewChar
    ShowCoachingTip(TipMessage)
}

HandleTap(lowerKey, upperKey, lowerTarget, upperTarget) {
    global LastKey, LastKeyTime, DoubleTapThreshold, InstantSingleC
    if (InstantSingleC and lowerKey = "c")
        return

    isUpper := GetKeyState("Shift", "P") xor GetKeyState("CapsLock", "T")
    currentChar := isUpper ? upperKey : lowerKey
    now := A_TickCount

    ; Check if same letter was tapped twice within threshold
    if ((LastKey == lowerKey or LastKey == upperKey) and (now - LastKeyTime <= DoubleTapThreshold)) {
        targetChar := (LastKey == upperKey or isUpper) ? upperTarget : lowerTarget
        ShowChoicePopup(currentChar . currentChar, targetChar)
        LastKey := "" ; Reset so triple tap doesn't re-trigger
    } else {
        DismissPopup()
        LastKey := currentChar
        LastKeyTime := now
    }
}

ShowChoicePopup(typedPair, targetChar) {
    global PopupActive, PendingReplacement, ChoiceGui, ChoiceText
    PendingReplacement := targetChar
    PopupActive := true

    ChoiceText.Value := "'" . targetChar . "' uchun [Tab] yoki [Enter] bosing ('" . typedPair . "' uchun yozishda davom eting)`n'" . targetChar . "' учун [Tab] ёки [Enter] босинг ('" . typedPair . "' учун ёзишда давом этинг)"
    GetCaretPosition(&posX, &posY)
    ChoiceGui.Show("x" . posX . " y" . (posY + 25) . " NoActivate AutoSize")
    SetTimer(DismissPopup, -2000) ; Auto-hide after 2.0 seconds if ignored
}

DismissPopup() {
    global PopupActive, PendingReplacement, ChoiceGui
    PopupActive := false
    PendingReplacement := ""
    SetTimer(DismissPopup, 0)
    ChoiceGui.Hide()
}

ShowCoachingTip(msg) {
    global CoachGui, CoachText
    CoachText.Value := msg
    GetCaretPosition(&posX, &posY)
    CoachGui.Show("x" . posX . " y" . (posY - 48) . " NoActivate AutoSize")
    SetTimer(() => CoachGui.Hide(), -3000) ; Disappears after 3.0 seconds
}

GetCaretPosition(&x, &y) {
    if !CaretGetPos(&x, &y) {
        MouseGetPos(&x, &y)
        x += 15
        y += 15
    }
}

ToggleScript(*) {
    global IsEnabled := !IsEnabled
    if IsEnabled
        Tray.Check("Yoqish / Ёқиш (Faol / Фаол)")
    else
        Tray.Uncheck("Yoqish / Ёқиш (Faol / Фаол)")
}

ToggleSingleC(*) {
    global InstantSingleC := !InstantSingleC
    if InstantSingleC
        Tray.Check("Bitta 'C' -> 'Ç' rejimi / Битта 'C' -> 'Ç' режими")
    else
        Tray.Uncheck("Bitta 'C' -> 'Ç' rejimi / Битта 'C' -> 'Ç' режими")
}