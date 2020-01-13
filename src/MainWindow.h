#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLinkedList>
#include <QMap>
#include <QGraphicsBlurEffect>
#include <QStackedWidget>
#include <QStateMachine>
#include <QState>
#include <QHistoryState>
#include <QFinalState>

#include "usrmsg.h"
#include "ScreenLogin.h"
#include "ScreenSignUp.h"
#include "ScreenForgotPassword.h"
#include "ScreenDashboard.h"
#include "DapUiMainWindow.h"
#include "DapUiVpnStateIndicator.h"
#include "DapCmdStates.h"
#include "DapCmdConnect.h"
#include "DapStyleHolder.h"
#include "MainMenu.h"
#include "DapUIAnimationScreen.h"
#include "DapUiScreen.h"
#include "ScreenLoginLayout.h"
#include "ScreenLogin.h"

#include <ServiceCtl.h>
#include "DapJsonCmdController.h"
#include "DapCmdTunTap.h"
#include "DapCmdStats.h"
#include "DapCmdUserData.h"
#include "DapCmdLastConnectionData.h"
#include "DapCmdPingServer.h"
#include "DapCmdServersList.h"
#include "DapCmdAndroidTunnel.h"
#include "DapCmdAuth.h"

class QSystemTrayIcon;
class QCloseEvent;
#ifdef DAP_SERVICE_CONNECT_TCP
class QTcpSocket;
#else
class QLocalSocket;
#endif
class QMenu;
class QAction;

class MainWindow : public DapUiMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(const QSize& windowSize,
                        QWidget *parent = Q_NULLPTR);
protected:
    void closeEvent(QCloseEvent * ev) override;
#ifdef Q_OS_WINDOWS
    bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
#endif
private:

    QStackedWidget * pages;
    DapStyleHolder *m_styleHolder{nullptr};
    
    void initIndicators();
    void initIndicatorsTransitions();

    DapJsonCmdController* createJsonCmdController();

    bool userMsgQueueIsEmpty() {return dashboard_user_msg == "";}


    QString dashboard_user_msg = "";

    void updateUpstreams();
    void initTray();
    void sendDisconnectionReq();

    UsrMsg     user_msg;

    DapServerInfo m_selectedServerInfo;

    QSystemTrayIcon * trayIcon;
    QMenu * trayMenu;
    QString m_user, m_password;

    // /////////////////////////
    MainMenu* m_mainMenu;

    QStateMachine sm;
        QState * statesScreen;
            QState * statesLoginScreen;
                QState * stateLoginCtlConnecting;
                QState * stateLoginBegin;
                QState * stateLoginTunTapUninstalled;
                QState * stateLoginConnecting;
            QState * stateSignUpScreen {nullptr};
            QState * stateForgotPasswordScreen {nullptr};
            QState * statesDashboardScreen;
                QState * stateDashboardConnected;
                QState * stateDashboardConnecting;
                QState * stateDashboardDisconnecting;


    ServiceCtl *m_serviceCtl = Q_NULLPTR;

    // handlers
    DapCmdStates* m_statesHandler;
    DapCmdStats* m_statsHandler;
    DapCmdConnect* m_connectHandler;
    DapCmdUserData* m_userDataHandler;
    DapCmdAuth* m_authHandler;

    DapCmdLastConnectionData* m_lastConnectionDataHandler;
    DapCmdTunTap *m_tunTapHandler;
    DapCmdPingServer* m_pingCmd;

    DapCmdServersList* m_dapServersList;
    DapCmdAndroidTunnel* m_dapAndroidTunnel;

    DapUiVpnStateIndicator *siAuthorization;
    DapUiVpnStateIndicator *siStream;
    DapUiVpnStateIndicator *siTunnel;

    // блокируем всплытие контекстного меню
    void contextMenuEvent(QContextMenuEvent *event) override { Q_UNUSED(event); }
    /// Set the style of the login edit box depending on the text.
    /// @param a_edLogin Login edit field.
    void setStyleLoginField(QLineEdit *a_edLogin);
    /// Set the style of the password edit box depending on the filling of the text and text input.
    /// @param a_edPassword Password edit field.
    void setStylePasswordField(QLineEdit *a_edPassword);
    
#ifdef DAP_SERVICE_CONNECT_TCP
    QTcpSocket * sockCtl;
#else
    QLocalSocket * sockCtl;
#endif
    QAction * actionDisconnect;

signals:
    void sigBtConnect();
    void sigBtDisconnect();
    
    /// The signal emitted when pressing the "Sign up" button on 
    /// the ScreenLogin screen to switch to the ScreenSignUp screen.
    void sigSignUp();
    /// The signal emitted when pressing the "Lost password?" 
    /// button on the ScreenLogin screen to switch to the ScreenForgotPassword screen.
    void sigForgotPassword();
    /// The signal emitted when pressing the "Sign in" button 
    /// to go to the ScreenLogin screen.
    void sigBackToLogin();

private slots:

    void onBtMessageCloseClicked();

    void onReqConnect(const DapServerInfo& dsi, QString a_user, QString a_ps);

    void updateUsrMsg();

    void onExit();

    void setStatusText(const QString & a_txt, DapUiScreen *a_screen = nullptr);
    void onLogout();
};

#endif // MAINWINDOW_H
