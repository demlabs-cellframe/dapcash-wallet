#include "MainWindow.h"
#include "DapUiScreen.h"
#include "DapLogger.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTime>
#include <QDebug>
#include <DapStyleHolder.h>
#include <controls/StyleSubcontrol.h>

void setStyleSheet(QApplication &a_application, DapUiScreen::ScreenSize a_screenSize, QString a_path = ":");
void addAppFonts();
void createDapLogger();

#ifdef Q_OS_WIN
bool checkSingleInstance() {
    HANDLE hEvent = CreateEventA(nullptr, FALSE, FALSE, "Global\\DapChainVPNClient");
    if(!hEvent) {
        CloseHandle(hEvent);
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle( hEvent );
        hEvent = nullptr;
        return false;
    }
    return true;
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    if(!checkSingleInstance()) {
        FILE *hwnd_file = fopen("hwnd", "r");
        unsigned long long id;
        fscanf(hwnd_file, "%d", &id);
        fclose(hwnd_file);
        SendMessageA((HWND)id, 666, 0, NULL);
        return 0;
    }
#endif
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");
    QApplication app(argc, argv);
    app.setOrganizationName("demlabs");
    app.setOrganizationDomain("demlabs.net");
    app.setApplicationName(DAP_BRAND);
    app.setApplicationVersion(DAP_VERSION);

    addAppFonts();
    createDapLogger();

    DapUiScreen::ScreenSize screenSize = DapUiScreen::getScreenSize();
    setStyleSheet(app, screenSize);

#ifdef Q_OS_WIN
    app.setWindowIcon(QIcon(":/pics/icon_startmenu.png"));
#endif
    MainWindow w(DapUiScreen::getScreenResolution(screenSize));

    qDebug() << "QApplication initialized";
    w.show();

    //Don't remove!
//       //Button for updating styleshet when debug
//       QPushButton *pbUpdateStyleSh = new QPushButton("updateStyleSheet");
//       pbUpdateStyleSh->resize(100,50);
//       QObject::connect(pbUpdateStyleSh, &QPushButton::clicked, [&a, &screenSize] (){
//           QDir dir(QDir::current());
//           dir.cd("../../dapvpn-client/DapVpnGui/resources/DiveVPN/");
//           updateStyleSheet(a, screenSize, dir.path());
//       });
//       pbUpdateStyleSh->setWindowFlags(pbUpdateStyleSh->windowType() | Qt::WindowStaysOnTopHint);
//       pbUpdateStyleSh->show();
#ifdef Q_OS_WIN
    FILE *hwnd_file = fopen("hwnd", "w");
    fprintf(hwnd_file, "%d", w.winId());
    fclose(hwnd_file);
#endif
    return app.exec();
}

void createDapLogger()
{
    DapLogger dapLogger;
#ifndef QT_DEBUG
    #ifdef Q_OS_LINUX
        dapLogger.setLogFile(QString("/opt/%1/log/%2Gui.log").arg(QString(DAP_BRAND).toLower()).arg(DAP_BRAND));
    #elif defined(Q_OS_MACOS)
        dapLogger.setLogFile(QString("/tmp/%1Gui.log").arg(DAP_BRAND));
    #elif defined (Q_OS_WIN)
        dapLogger.setLogFile(QString("%1/%2Gui.log").arg(QCoreApplication::applicationDirPath()).arg(DAP_BRAND));
    #endif
#else
    dap_set_appname(DAP_BRAND"Gui");
     dap_common_init(DAP_BRAND"Gui",NULL);
#endif
}

void setStyleSheet(QApplication &a_application, DapUiScreen::ScreenSize a_screenSize, QString a_path /*= ":"*/)
{
    QFile appStyleF;

    QString strScreenSize;
    switch (a_screenSize) {
    case DapUiScreen::Big:
        strScreenSize = "_big";
        break;
    case DapUiScreen::Small:
        strScreenSize = "_small";
        break;
    case DapUiScreen::Medium: ;
    }

    QString fileName = QString("main_%1%2.css").arg(DAP_PLATFORM).arg(strScreenSize);


    a_path += "/" + fileName;
    DapStyleHolder::setAppCssHolder(a_path);

    a_application.setStyleSheet(DapStyleHolder::qAppCssStyleHolder()->getStyleSheet()); //set up as default
}

void addAppFonts()
{
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Roboto-Thin.ttf");
}
