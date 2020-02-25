TEMPLATE = subdirs
SUBDIRS = DapCashWalletGUI DapCashWalletService

DapCashWalletGUI.subdir = DapCashWalletGUI
DapCashWalletService.subdir = DapCashWalletService
DapCashWalletGUI.depends = DapCashWalletService

!defined(BRAND, var)
{
    BRAND = DpaCashWallet
}

unix: !mac : !android {
    share_target.files = debian/share/*
    share_target.path = /opt/dapcash-wallet/share/
    INSTALLS += share_target
}
