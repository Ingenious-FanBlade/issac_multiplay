try {
    & 'F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\run.ps1' -Attempt 3 *>> 'F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\desktop-run.log'
    '0' | Set-Content -LiteralPath 'F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\desktop-run.done'
} catch {
    $_ | Out-String | Add-Content -LiteralPath 'F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\desktop-run.log'
    '1' | Set-Content -LiteralPath 'F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\desktop-run.done'
}
