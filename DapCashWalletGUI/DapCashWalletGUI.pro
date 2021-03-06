QT += qml quick widgets

TEMPLATE = app
CONFIG += c++11


!defined(BRAND,var){
#  Default brand
    BRAND = DapCashWallet
}

TARGET = $$BRAND

VER_MAJ = 1
VER_MIN = 0
VER_PAT = 0

win32 {
    VERSION = $${VER_MAJ}.$${VER_MIN}.$$VER_PAT
    DEFINES += CLI_PATH=\\\"./cellframe-node-cli.exe\\\"
    DEFINES += HAVE_STRNDUP
}
else {
    VERSION = $$VER_MAJ\.$$VER_MIN\-$$VER_PAT
    DEFINES += CLI_PATH=\\\"/opt/cellframe-node/bin/cellframe-node-cli\\\"
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += DAP_BRAND=\\\"$$BRAND\\\"
DEFINES += DAP_SERVICE_NAME=\\\"DapCashWalletService\\\"
DEFINES += DAP_VERSION=\\\"$$VERSION\\\"
DEFINES += DAP_SETTINGS_FILE=\\\"settings.json\\\"
macx {
    ICON = res/icons/dashboard.icns
}
else {
    ICON = qrc:/res/icons/icon.ico
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic

CONFIG(debug, debug|release) {
    DESTDIR = bin/debug
} else {
    DESTDIR = bin/release
}

INCLUDEPATH += $$_PRO_FILE_PWD_/../libDapCashWalletCommon/ \
               $$_PRO_FILE_PWD_/../DapRPCProtocol/

OTHER_FILES += libdap-qt-ui-qml

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/DapServiceClient.cpp \
    $$PWD/DapServiceController.cpp \
    $$PWD/DapServiceClientNativeAbstract.cpp \
    $$PWD/DapServiceClientNativeLinux.cpp \
    $$PWD/DapServiceClientNativeWin.cpp \
    $$PWD/DapServiceClientNativeMacOS.cpp

RESOURCES += $$PWD/qml.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/dapcashwallet/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    $$PWD/DapServiceClient.h \
    $$PWD/DapServiceController.h \
    $$PWD/DapServiceClientNativeAbstract.h \
    $$PWD/DapServiceClientNativeLinux.h \
    $$PWD/DapServiceClientNativeWin.h

include (../libdap/libdap.pri)
include (../libdap-crypto/libdap-crypto.pri)
include (../libdap-qt/libdap-qt.pri)
include (../libdap-qt-ui-qml/libdap-qt-ui-qml.pri)

include (../libDapCashWalletCommon/libDapCashWalletCommon.pri)
include (../DapRPCProtocol/DapRPCProtocol.pri)

unix: !mac : !android {
    gui_target.files = $${BRAND}
    gui_target.path = /opt/dapcashwallet/bin/
    INSTALLS += gui_target
}
