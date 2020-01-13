#-------------------------------------------------
#
# Project created by QtCreator 2017-06-24T09:30:57
#
#-------------------------------------------------

QT += gui network widgets widgets core

CONFIG += c++14

TEMPLATE = app

include(../config.pri)
message("Defined brand target $$BRAND")
TARGET = $${BRAND}
DEFINES += DAP_BRAND=\\\"$$BRAND\\\"

win32 {
    DEFINES += DAP_VERSION=\\\"$${VER_MAJ}.$$VER_MIN-$$VER_PAT\\\"
    DEFINES += HAVE_STRNDUP
}
else {
    DEFINES += DAP_VERSION=\\\"$$VERSION\\\"
}

include (../libdap/libdap.pri)
include (../libdap-crypto/libdap-crypto.pri)
include (../libdap-qt/libdap-qt.pri)
include (../libdap-qt-ui/libdap-qt-ui.pri)
include (../libdap-qt-vpn-common/libdap-qt-vpn-common.pri)
include( ../libdap-qt-vpn-ui/libdap-qt-vpn-ui.pri)
include (../libdap-qt-vpn-ui/DapCmdHandlers/dap-cmd-handlers.pri)


SOURCES += main.cpp\
    MainWindow.cpp

HEADERS  += \
    MainWindow.h

FORMS    +=  \
    ui/$$BRAND/login_layout.ui \
    ui/$$BRAND/login_desktop.ui \
    ui/$$BRAND/login_desktop_small.ui \
    ui/$$BRAND/login_desktop_big.ui \




RESOURCES += resources/common/common.qrc
RESOURCES += resources/$$BRAND/main.qrc


android {
    DISTFILES += \
    $$PWD/android/AndroidManifest.xml \
    $$PWD/android/res/drawable-hdpi/divevpn.png \
    $$PWD/android/res/drawable-ldpi/divevpn.png \ # ATTENTION! Need change to dynamically linking for different brands
    $$PWD/android/res/drawable-mdpi/divevpn.png

}




unix: !mac : !android {
    gui_target.files = $$BRAND
    gui_target.path = /opt/$$lower($$BRAND)/bin/
    gui_data_static.path = /opt/$$lower($$BRAND)/share
    gui_data_static.files = resources/$$BRAND/dists/share/*
    INSTALLS += gui_target gui_data_static
}

### Link DapVpnCommon ###
#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libdap-qt-vpn-common/release/ -lDapVpnCommon
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libdap-qt-vpn-common/debug/ -lDapVpnCommon
#else:unix: LIBS += -L$$OUT_PWD/../libdap-qt-vpn-common/ -lDapVpnCommon

#INCLUDEPATH += $$PWD/../libdap-qt-vpn-common
#DEPENDPATH += $$PWD/../libdap-qt-vpn-common

#win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libdap-qt-vpn-common/release/libDapVpnCommon.a
#else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libdap-qt-vpn-common/debug/libDapVpnCommon.a
#else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libdap-qt-vpn-common/release/DapVpnCommon.lib
#else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libdap-qt-vpn-common/debug/DapVpnCommon.lib
#else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libdap-qt-vpn-common/libDapVpnCommon.a
### Link DapVpnCommon ###

DISTFILES += \
    classesmodel.qmodel \
