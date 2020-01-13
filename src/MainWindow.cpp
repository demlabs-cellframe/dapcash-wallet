#include <QMessageBox>
#include <QGraphicsView>
#include <QProgressDialog>
#include <QGraphicsBlurEffect>
#include <QStateMachine>
#include <QState>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QDesktopServices>

#ifdef DAP_SERVICE_CONNECT_TCP
#include <QTcpSocket>
#else
#include <QLocalSocket>
#endif

#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QSystemTrayIcon>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#endif


#include "MainWindow.h"

#include "schedule.h"
Q_DECLARE_METATYPE(DapServerInfo);

DapJsonCmdController* MainWindow::createJsonCmdController()
{
    auto controller = new DapJsonCmdController;
    (new DapCmdAndroidTunnel())->cmd();
    controller->addNewHandler(m_statsHandler = new DapCmdStats);
    controller->addNewHandler(m_statesHandler = new DapCmdStates);
    controller->addNewHandler(m_connectHandler = new DapCmdConnect);
    controller->addNewHandler(m_authHandler = new DapCmdAuth);
    controller->addNewHandler(m_userDataHandler = new DapCmdUserData);
    controller->addNewHandler(m_lastConnectionDataHandler = new DapCmdLastConnectionData);
    controller->addNewHandler(m_tunTapHandler = new DapCmdTunTap);
    controller->addNewHandler(m_pingCmd = new DapCmdPingServer);
    controller->addNewHandler(m_dapServersList = new DapCmdServersList);
    controller->addNewHandler(m_dapAndroidTunnel = new DapCmdAndroidTunnel);
    return controller;
}

MainWindow::MainWindow(const QSize& windowSize, QWidget *parent) :
    DapUiMainWindow(parent)
{
    qDebug() << "[MW] Initializing main window";
    this->setFixedSize(windowSize);

    m_serviceCtl = new ServiceCtl(createJsonCmdController());
    connect(m_serviceCtl, &ServiceCtl::ctlConnected, [=] {
        m_statesHandler->sendCmd();
        m_userDataHandler->sendCmd();
    });
    m_styleHolder = DapStyleHolder::qAppCssStyleHolder();

    connect(m_statesHandler, &DapCmdStates::sigUserStateConnect, [=]{
        qDebug() << "User request state connect";
    });

    connect(m_statesHandler, &DapCmdStates::sigUserStateDisconnect, [=]{
        qDebug() << "User request state disconnect";
    });

    this->setCentralWidget( pages = new QStackedWidget(this) );

    initTray();

    /////////////////////////////////////////////////////////////////

    // SideBar menu:
    m_mainMenu = new MainMenu(this);

    QLabel *lblCaption = m_mainMenu->sideBar()->widget()->findChild<QLabel*>("lblCaption");
    Q_ASSERT(lblCaption);

    // Set the name of the application in the menu bar.
    lblCaption->setText(DAP_BRAND);

    //////////////////////////////////////////////////////////////////
    sm.setChildMode(QState::ParallelStates);
    sm.setGlobalRestorePolicy(QState::RestoreProperties);

    // Screen states
    statesScreen                 = new QState(&sm);
    statesLoginScreen            = new QState(statesScreen);
    stateSignUpScreen            = new QState(statesScreen);
    stateForgotPasswordScreen    = new QState(statesScreen);
    statesDashboardScreen        = new QState(statesScreen);

    // Login states
    stateLoginCtlConnecting      = new QState(statesLoginScreen);
    stateLoginBegin              = new QState(statesLoginScreen);
    stateLoginConnecting         = new QState(statesLoginScreen);

    stateLoginTunTapUninstalled  = new QState(statesLoginScreen);

    // Dashboard states
    stateDashboardConnected      = new QState(statesDashboardScreen);
    stateDashboardConnecting     = new QState(statesDashboardScreen);
    stateDashboardDisconnecting  = new QState(statesDashboardScreen);


    // Initial states
    statesScreen->setInitialState(statesLoginScreen);
    statesLoginScreen->setInitialState(stateLoginCtlConnecting);
    statesDashboardScreen->setInitialState(stateDashboardConnecting);

    /***********************************************************
    **                      States Login
    ***********************************************************/
    // Login states slots
    connect(statesLoginScreen, &QState::entered, [=]{
        qInfo() << "[MainWindow] States entered";

        DapUIAnimationScreen *scrMain = qobject_cast<DapUIAnimationScreen*>(screen());
        if (scrMain) {
            scrMain->activateScreen<ScreenLogin>();
        }
        else {
            DapUIAnimationScreen *scrMain        = new DapUIAnimationScreen(this, pages);
            ScreenLoginLayout    *scrLoginLayout = scrMain->activateScreen<ScreenLoginLayout>();
            ScreenLogin          *scrLogin       = scrLoginLayout->screenLogin();

            newScreen(scrMain);

            //Show mainmenu on menu Click:
            scrLogin->connectTo("btMenu", SIGNAL(clicked()), m_mainMenu->sideBar(), SLOT(changeVisible()));

            /*
            scrLogin->connectTo<QToolButton>("btRegister", &QToolButton::clicked,[=]{
                QDesktopServices::openUrl(QUrl("https://klvn.io/registration/"));
            });
            scrLogin->connectTo<QToolButton>("btRestore", &QToolButton::clicked,[=]{
                QDesktopServices::openUrl(QUrl("https://klvn.io/my-account/lost-password/"));
            });
*/
            scrLogin->setVars("edMail","text", DapDataLocal::me()->login());
            scrLogin->setVars("edPassword","text", DapDataLocal::me()->password());
            scrLogin->setVars("lbStatus","text","");
            scrLogin->setVars("btLogin","enabled",true);

            QLineEdit *edMail = qobject_cast<QLineEdit*>(scrLogin->getWidget("edMail", rotation()));
            QLineEdit *edPassword = qobject_cast<QLineEdit*>(scrLogin->getWidget("edPassword", rotation()));
#ifndef DAP_PLATFORM_MOBILE
            QPushButton *btShow = qobject_cast<QPushButton*>(scrLogin->getWidget("btShow", rotation()));
#endif
            QComboBox *cbUpstream = qobject_cast<QComboBox*>(scrLogin->getWidget("cbUpstream", rotation()));

            Q_ASSERT(edMail);
            Q_ASSERT(edPassword);
#ifndef DAP_PLATFORM_MOBILE
            Q_ASSERT(btShow);
#endif
            // Update combo box
            connect( m_dapServersList, &DapCmdServersList::sigServersList, [=](const DapServerInfoList& a_servers){
                qDebug()<< "Updating servers list";
                DapDataLocal::me()->clearServerList();

                for( auto server: a_servers)
                   DapDataLocal::me()->addServer(server);

                scrLogin->reloadServers();
            } );

            /// Signal-slot connection requesting user data from a service.
            connect(m_userDataHandler, &DapCmdUserData::sigUserData,
                    [=](const QString &login, const QString &password, const QString &serverName)
            {
                edMail->setText(login);
                edPassword->setText(password);
                cbUpstream->setCurrentText(DapDataLocal::me()->getServerNameByAddress(serverName));
            });

#ifdef Q_OS_ANDROID
            connect(m_dapAndroidTunnel, &DapCmdAndroidTunnel::asked, this,
                    [=](const QString &address, const QString &gateway, const int socketDescriptor) {
                QtAndroid::runOnAndroidThread([=]() {
                    QtAndroid::androidActivity().callMethod<void>("stopDapVpnServiceNative"
                                                                  ,"()V");
                    QtAndroid::androidActivity().callMethod<void>("startDapVpnServiceNative"
                                                                  ,"(Ljava/lang/String;Ljava/lang/String;I)V"
                                                                  ,QAndroidJniObject::fromString(address).object<jstring>()
                                                                  ,QAndroidJniObject::fromString(gateway).object<jstring>()
                                                                  ,socketDescriptor);
                });
            });
#endif

            // Restore the selected server in the combo box of servers
            if(!(DapDataLocal::me()->serverName().isEmpty() || DapDataLocal::me()->serverName().isNull()))
                cbUpstream->setCurrentText(DapDataLocal::me()->serverName());

            // Check field fill and set appropriate styles.
            setStyleLoginField(edMail);
            setStylePasswordField(edPassword);

            // Signal-slot connection, emitting a signal to go to the user registration screen.
            scrLogin->connectTo<QToolButton>("btSignUp", &QToolButton::clicked,[=]
            {
                emit sigSignUp();
            });
            // Signal-slot connection that provides access to the password recovery page.
            scrLogin->connectTo<QToolButton>("btLostPassword", &QToolButton::clicked,[=]
            {
                emit sigForgotPassword();
            });
            // Signal-slot connection that sets the style of the edit box depending on the text filling.
            connect(edMail, &QLineEdit::textChanged, [=] (const QString& text)
            {
                setStyleLoginField(edMail);
                // Save login.
                DapDataLocal::me()->setLogin(text);
            });
            // Signal-slot connection that sets the style of the edit box depending on the text filling and
            // sets the style of the edit box depending on the text cing.
            connect(edPassword, &QLineEdit::textChanged, [=] (const QString& text)
            {
                setStylePasswordField(edPassword);
                // Save password.
                DapDataLocal::me()->setPassword(text);
            });
#ifndef DAP_PLATFORM_MOBILE
            // Signal-slot connection that changes the display of the password depending on the button state.
            connect(btShow, &QPushButton::clicked, [=]
            {
                // If the password field is empty, the styles are not applied.
                if(!(edPassword->text().isEmpty() || edPassword->text().isNull()))
                {
                    edPassword->setStyleSheet(btShow->isChecked()
                                               ? m_styleHolder->getWidgetStyleSheet("#edPassword", "visible")
                                               : m_styleHolder->getWidgetStyleSheet("#edPassword", "hide"));
                }
            });
#endif

            // Signal-slot connection that sets styles to input fields in case of user input errors,
            // as well as displaying an informational message about an error.
            connect(scrLogin, &ScreenLogin::sigFieldError, this,[=](const QString& a_message)
            {
                setStatusText(a_message, scrLogin);
                m_styleHolder->setStyleSheet(edMail, "checkError");
                m_styleHolder->setStyleSheet(edPassword, "checkError");
            });
            // Signal-slot connection initiates the login procedure.
            connect(scrLogin, &ScreenLogin::reqConnect, this,[=](DapServerInfo& dsi,
                                       const QString& a_user, const QString& a_ps)
            {
                m_styleHolder->setStyleSheet(edMail, "filled");
                m_styleHolder->setStyleSheet(edPassword, "filled");
                onReqConnect(dsi, a_user, a_ps);
            });

            //
        }
    });

    connect(statesLoginScreen, &QState::exited, [=]{
        qInfo() << "[MainWindow]statesLogin exited";
    });
    connect(stateLoginCtlConnecting,&QState::entered,[=]{
        qInfo() << "[MainWindow] State LoginCtlConnecting entered";

        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();
        scrLogin->setVars("btLogin","enabled",false);
        scrLogin->setVars("cbServer","enabled",false);
        scrLogin->setVars("edPassword","enabled",false);
        scrLogin->setVars("edMail","enabled",false);
        setStatusText(tr("Backend Reconnect..."), scrLogin);
        setEnabled(false);
        m_serviceCtl->init();
    });

    connect(stateLoginCtlConnecting, &QState::exited,[=]{
        qInfo() << "[MainWindow] State LoginCtlConnecting exited";

        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();
        scrLogin->setVars("btLogin","enabled",true);
        scrLogin->setVars("cbServer","enabled",true);
        scrLogin->setVars("edPassword","enabled",true);
        scrLogin->setVars("edMail","enabled",true);
        scrLogin->setVars("lbStatus","txt",tr("Connecting..."));
        setStatusText("", scrLogin);
        setEnabled(true);
    });

    connect(stateLoginBegin,&QState::entered, [=]{
        qInfo() << "[MainWindow] State Login Begin";
        if (!m_serviceCtl->tapStatus) {
            m_tunTapHandler->sendCmd(DapCmdTunTap::CHECK);
            // send ping requests
            for(const auto& server : DapDataLocal::me()->servers()) {
                if (  !QStringList({
                                        "127.0.0.1",
                                        "localhost"
                                    }).contains(server.address  ) )
                    m_pingCmd->sendPingCmd(server.address, server.port);
            }
            m_dapServersList->sendCmdGetServersList();
        }
        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();

        scrLogin->setVars("btLogin","enabled",true);
        scrLogin->setVars("cbServer","enabled",true);
        scrLogin->setVars("edPassword","enabled",true);
        scrLogin->setVars("edMail","enabled",true);
        scrLogin->setVars("btLogin","text",tr("Connect"));
        statesLoginScreen->setInitialState(stateLoginBegin);
    });

    connect(stateLoginConnecting,&QState::entered, [=]{
        qInfo() << "[MainWindow] State Login Connecting";

        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();
        scrLogin->setVars("btLogin","text",tr("Stop"));
        scrLogin->setVars("cbServer","enabled",false);
        scrLogin->setVars("edPassword","enabled",false);
        scrLogin->setVars("edMail","enabled",false);
        scrLogin->setVars("lbStatus","txt",tr("Connecting..."));
    });

    connect(stateLoginTunTapUninstalled, &QState::entered, [=] {
        qInfo() << "[MainWindow] State TunTap driver uninstalled";
        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();
        scrLogin->setVars("btLogin"     ,"enabled"  ,false);
        scrLogin->setVars("cbServer"    ,"enabled"  ,false);
        scrLogin->setVars("edPassword"  ,"enabled"  ,false);
        scrLogin->setVars("edMail"      ,"enabled"  ,false);
        setStatusText(tr("TUN/TAP driver error"), scrLogin);
        setEnabled(false);
        if (QMessageBox(QMessageBox::Warning, "TAP driver error"
                        ,"Install TAP driver to proceed using VPN. Install Now?"
                        , QMessageBox::Yes | QMessageBox::No).exec() == QMessageBox::Yes) {
            m_tunTapHandler->sendCmd(DapCmdTunTap::INSTALL);
            this->setWindowState(Qt::WindowMinimized);
        }
    });

    connect(stateLoginTunTapUninstalled, &QState::exited, [=] {
        qInfo() << "[MainWindow] State TunTap driver uninstalled exited";
        ScreenLogin *scrLogin = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLogin>();
        scrLogin->setVars("btLogin"     ,"enabled"  ,true);
        scrLogin->setVars("cbServer"    ,"enabled"  ,true);
        scrLogin->setVars("edPassword"  ,"enabled"  ,true);
        scrLogin->setVars("edMail"      ,"enabled"  ,true);
        setStatusText(tr(""), scrLogin);
        setEnabled(true);
        this->showNormal();
    });

    /***********************************************************
    **                      State SignUp
    ***********************************************************/

    // Signal-slot connection initializing the state of the input for the registration state.
    connect(stateSignUpScreen, &QState::entered, [=]{
        qInfo() << "[MainWindow] State SignUp entered";

        ScreenLoginLayout *scrLoginLayout = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenLoginLayout>();
        ScreenSignUp *scrSignUp = scrLoginLayout->activateScreen<ScreenSignUp>();

//         Initializing pointers to form widgets.
        QLineEdit *edMail = scrSignUp->getWidgetCustom<QLineEdit>("edMail", rotation());
        QLineEdit *edPassword = scrSignUp->getWidgetCustom<QLineEdit>("edPassword", rotation());
        QLineEdit *edConfPassword = scrSignUp->getWidgetCustom<QLineEdit>("edConfPassword", rotation());
        QPushButton *btShow = scrSignUp->getWidgetCustom<QPushButton>("btShow", rotation());
        QPushButton *btConfShow = scrSignUp->getWidgetCustom<QPushButton>("btConfShow", rotation());

        Q_ASSERT(edMail);
        Q_ASSERT(edPassword);
        Q_ASSERT(btShow);
        Q_ASSERT(edConfPassword);
        Q_ASSERT(btConfShow);

        // Switch to the previous screen.
        scrSignUp->connectTo<QPushButton>("btBack", &QPushButton::clicked,[=]
        {
            emit sigBackToLogin();
        });

 // Signal-slot connection that sets the style of the edit box depending on the text filling.
        connect(edMail, &QLineEdit::textEdited, [=]
        {
            setStyleLoginField(edMail);
        });
        // Signal-slot connection that sets the style of the edit box depending on the text filling.
        connect(edPassword, &QLineEdit::textEdited, [=]
        {
            setStylePasswordField(edPassword);
        });
        // Signal-slot connection that sets the style of the edit box depending on the text filling.
        connect(edConfPassword, &QLineEdit::textEdited, [=]
        {
            setStylePasswordField(edConfPassword);
        });
        // Signal-slot connection that sets styles to input fields in case of user input errors,
        // as well as displaying an informational message about an error.
        connect(scrSignUp, &ScreenSignUp::sigFieldError, this,[=](const QString& a_message)
        {
            scrSignUp->setVars("lbMessage","text",a_message);
            m_styleHolder->setStyleSheet(edMail, "checkError");
            m_styleHolder->setStyleSheet(edPassword, "checkError");
            m_styleHolder->setStyleSheet("edPassword", edConfPassword, "checkError");
        });
        // Signal-slot connection that sets styles for password
        // input fields if user-entered passwords do not match.
        connect(scrSignUp, &ScreenSignUp::sigPasswordMismatch, this,[=](const QString& a_message)
        {
            scrSignUp->setVars("lbMessage","text",a_message);
            m_styleHolder->setStyleSheet(edPassword, "checkError");
            m_styleHolder->setStyleSheet("edPassword", edConfPassword, "checkError");
        });
        // Signal-slot connection initiates the register procedure.
        connect(scrSignUp, &ScreenSignUp::sigCreateAccount, this,[=](const QString& a_mail, const QString& a_password)
        {
            Q_UNUSED(a_mail); Q_UNUSED(a_password);
            setStatusText("", scrSignUp);

            m_styleHolder->setStyleSheet(edMail, "filled");
            m_styleHolder->setStyleSheet(edPassword, "filled");
            m_styleHolder->setStyleSheet("edPassword", edConfPassword, "filled");
        });
    });

    // Signal-slot connection initializing output state for registration status.
    connect(stateSignUpScreen, &QState::exited, [=]{
        qInfo() << "[MainWindow] States SignUp exited";
    });

    /***********************************************************
    **                  State ForgotPassword
    ***********************************************************/

    // Signal-slot connection initializing the state of the input for the registration state.
    connect(stateForgotPasswordScreen, &QState::entered, [=]{
        qInfo() << "[MainWindow] State ForgotPassword entered";

        DapUIAnimationScreen *scrMain = qobject_cast<DapUIAnimationScreen*>(screen());
        ScreenForgotPassword *scrForgotPassword = scrMain->activateScreen<ScreenForgotPassword>();


        // Initializing pointers to form widgets.
        QLineEdit   *edMail = scrForgotPassword->getWidgetCustom<QLineEdit>("edMail", rotation());
        QPushButton *btSend = scrForgotPassword->getWidgetCustom<QPushButton>("btSend", rotation());

        Q_ASSERT(edMail);
        Q_ASSERT(btSend);

        // Switch to the previous screen.
        scrForgotPassword->connectTo<QPushButton>("btBack", &QPushButton::clicked,[=]
        {
            emit sigBackToLogin();
        });
        // Signal-slot connection that sets the style of the edit box depending on the text filling.
        connect(edMail, &QLineEdit::textEdited, [=]
        {
            setStyleLoginField(edMail);
        });
        // Signal-slot connection that sets styles to input fields in case of user input errors,
        // as well as displaying an informational message about an error.
        connect(scrForgotPassword, &ScreenForgotPassword::sigFieldError, this,[=](const QString& a_message)
        {
            scrForgotPassword->setVars("lbMessage","text",a_message);

            m_styleHolder->setStyleSheet(edMail, "checkError");
        });
        // Signal-slot connection initiates the user password recovery procedure.
        connect(scrForgotPassword, &ScreenForgotPassword::sigSendResetInstructions, this,[=](const QString& a_mail)
        {
            Q_UNUSED(a_mail);
            setStatusText("", scrForgotPassword);
            m_styleHolder->setStyleSheet(edMail, "filled");
        });

    });

    // A signal slot connection that initializes the exit status for a password recovery state.
    connect(stateForgotPasswordScreen, &QState::exited, [=]{
        qInfo() << "[MainWindow] State ForgotPassword exited";
    });

    /***********************************************************
    **                      States Dashboard
    ***********************************************************/

    connect(statesDashboardScreen, &QState::entered, [=]{
        qInfo() << "[MainWindow] State statesDashboard entered";

        DapUIAnimationScreen *scrMain = qobject_cast<DapUIAnimationScreen*>(screen());
        scrMain->replaceSubscreen<ScreenForgotPassword,ScreenDashboard>();
        ScreenDashboard *scrDashboard =scrMain->activateScreen<ScreenDashboard>();

        connect(m_statsHandler, &DapCmdStats::sigReadWriteBytesStat,
                scrDashboard, &ScreenDashboard::drawShedules);
        // Signal-slot connection requesting last connection data from a service.
        connect(m_lastConnectionDataHandler, SIGNAL(sigLastConnectionData(QDateTime)),
                scrDashboard, SLOT(startCalculateConnectionData(QDateTime)));

        //Show mainmenu on menu Click:
        scrDashboard->connectTo("btMenu", SIGNAL(clicked()), m_mainMenu->sideBar(), SLOT(changeVisible()));
        scrDashboard->connectTo("btDisconnect",   SIGNAL(clicked(bool)), this, SLOT(onLogout()));
        scrDashboard->connectTo("btMessageClose", SIGNAL(clicked(bool)), this, SLOT(onBtMessageCloseClicked()));

        scrDashboard->setVars("lbStreamText","text","");
        scrDashboard->setVars("lbIpAddrText","text","");
        scrDashboard->setVars("lbInterfaceText","text","");
        scrDashboard->setVars("lbRouteText","text","");
        setStatusText(tr("Authorized as %1").arg(m_user), scrDashboard);

        // Set styles for graphics.
        scrDashboard->addItemGraphicSceneStyle("shChartDownload", m_styleHolder->getWidgetStyleSheet("#shChartDownload", "active"));
        scrDashboard->addItemGraphicSceneStyle("shChartUpload", m_styleHolder->getWidgetStyleSheet("#shChartUpload", "active"));
        scrDashboard->addItemGraphicSceneStyle("shGrid", m_styleHolder->getWidgetStyleSheet("#shGrid", "active"));
    });

    connect(statesDashboardScreen, &QState::exited, [=]{
        qInfo() << "[MainWindow] States Dashboard exited";

        DapUIAnimationScreen *scrMain = qobject_cast<DapUIAnimationScreen*>(screen());

        //After animation finish
        QMetaObject::Connection * const connection = new QMetaObject::Connection;
        *connection = connect(scrMain, &DapUIAnimationScreenAbstract::animationFinished, [=]{
            // 1. remove ScreenDashboard:
            scrMain->removeSubscreen<ScreenDashboard>();
            // 2. disconnect and delete this connection:
            disconnect(*connection);
            delete connection;
        });
    });


    connect(stateDashboardConnecting,&QState::entered,[=]{
        qInfo() << "[MainWindow] State Dashboard Connecting";

        ScreenDashboard *scrDashboard = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenDashboard>();
        Q_ASSERT(scrDashboard);
        scrDashboard->setVars("btDisconnect","enabled",true);
        setStatusText("Connecting...", scrDashboard);

    });


    stateDashboardConnected->assignProperty(actionDisconnect, "enabled", true);
    connect(stateDashboardConnected,&QState::entered,[=]{
        qInfo() << "[MainWindow] State Dashboard Connected";

        ScreenDashboard *scrDashboard = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenDashboard>();
        Q_ASSERT(scrDashboard);

        scrDashboard->setVars("lbReceived","enabled",true);
        scrDashboard->setVars("lbSent","enabled",true);
        scrDashboard->setVars("lbReceived","text", "0");
        scrDashboard->setVars("lbSent","text", "0");
        scrDashboard->setVars("btDisconnect","enabled",true);
        setStatusText("Connected", scrDashboard);
    });



    // State Dashboard Disconnecting
    connect(stateDashboardDisconnecting, &QState::entered,[=]{
        qInfo() << "[MainWindow] State DashboardDisconnecting entered";

        ScreenDashboard *scrDashboard = qobject_cast<DapUIAnimationScreen*>(screen())->subScreen<ScreenDashboard>();
        Q_ASSERT(scrDashboard);

        scrDashboard->setVars("btDisconnect","enabled",false);
        scrDashboard->setVars("lbStatus","text","Authorized, initializing the tunnel...");
        scrDashboard->setVars("lbAuthorizedText","text",tr("User Authorized"));
        scrDashboard->setVars("cbAuthorized","checked",true);
        setStatusText("Disconnecting...", scrDashboard);

        // Signal-slot connection that stops the calculator for calculating the speed
        // of sent / received data and the calculator for calculating the total connection time
        scrDashboard->stopCalculationTimers();

        sendDisconnectionReq();
    });

    /***********************************************************
    **                Transitions for states
    ***********************************************************/

    // * ---> *CtlConnecting
    statesLoginScreen->addTransition(m_serviceCtl,SIGNAL(ctlDisconnected()),stateLoginCtlConnecting);
    stateSignUpScreen->addTransition(m_serviceCtl,SIGNAL(ctlDisconnected()),stateLoginCtlConnecting);
    stateForgotPasswordScreen->addTransition(m_serviceCtl,SIGNAL(ctlDisconnected()),stateLoginCtlConnecting);

    statesDashboardScreen->addTransition(m_serviceCtl,SIGNAL(ctlDisconnected()),stateLoginCtlConnecting);

    stateLoginTunTapUninstalled->addTransition(m_serviceCtl, SIGNAL(sigTunTapPresent()), stateLoginBegin);
    stateLoginBegin->addTransition(m_serviceCtl, SIGNAL(sigTunTapError()), stateLoginTunTapUninstalled);

    // LoginBegin ---> LoginConnecting
    stateLoginBegin->addTransition(this,SIGNAL(sigBtConnect()),stateLoginConnecting);

    // From login window to registration window.
    stateLoginBegin->addTransition(this, SIGNAL(sigSignUp()), stateSignUpScreen);
    // From login window to forgot password window.
    stateLoginBegin->addTransition(this, SIGNAL(sigForgotPassword()), stateForgotPasswordScreen);



    // LoginBegin ---> Dashboard
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),statesDashboardScreen);
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),statesDashboardScreen);
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),statesDashboardScreen);

    // LoginConnecting ---> LoginBegin
    stateLoginConnecting->addTransition(this,SIGNAL(sigBtConnect()),stateLoginBegin);
    stateLoginConnecting->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()),stateLoginBegin);

    // LoginConnecting ---> Dashboard
    stateLoginConnecting->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),statesDashboardScreen);
    stateLoginConnecting->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),statesDashboardScreen);
    stateLoginConnecting->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),statesDashboardScreen);

    stateLoginConnecting->addTransition(m_statesHandler,SIGNAL(sigAllIndicatorStatesIsTrue()),stateDashboardConnected);

    // From registration window to login window.
    stateSignUpScreen->addTransition(this,SIGNAL(sigBackToLogin()),stateLoginBegin);
    // From forgot password window to login window.
    stateForgotPasswordScreen->addTransition(this,SIGNAL(sigBackToLogin()),stateLoginBegin);

    // DashboardDisconnecting ---> Begin
    stateDashboardDisconnecting->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroyed()),stateLoginBegin);
    stateDashboardDisconnecting->addTransition(m_statesHandler,SIGNAL(sigAllIndicatorStatesIsFalse()),stateLoginBegin);

    // Login Ctl Connecting ---> Login Begin
    stateLoginCtlConnecting->addTransition(m_serviceCtl,SIGNAL(ctlConnected()),stateLoginBegin);

    // Begin ---> Dashboard
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),stateDashboardConnected);
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),stateDashboardConnected);
    stateLoginBegin->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),stateDashboardConnected);

    // DashboardConnecting --> DashboardConnected
    stateDashboardConnecting->addTransition(m_statesHandler ,SIGNAL(sigTunnelCreated()),stateDashboardConnected);
    stateDashboardConnecting->addTransition(m_statesHandler ,SIGNAL(sigAllIndicatorStatesIsTrue()),stateDashboardConnected);
    stateDashboardConnecting->addTransition(this,SIGNAL(sigBtDisconnect()),stateDashboardDisconnecting);

    // Dashboard ---> DashboardDisconnecting
    stateDashboardConnected->addTransition(this,SIGNAL(sigBtDisconnect()),stateDashboardDisconnecting);

    // Dashboard ---> DashboardConnecting
    stateDashboardConnected->addTransition(m_statesHandler,SIGNAL(sigStreamClosed()) ,stateDashboardConnecting);
    stateDashboardConnected->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroyed()) ,stateDashboardConnecting);
    stateDashboardConnected->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()) , stateDashboardConnecting);

    initIndicators();
    initIndicatorsTransitions();

    sm.start();

    connect(m_connectHandler, &DapCmdConnect::errorMessage,
            [=](const QString&msg) { // connect error authorize message from service to status text
        setStatusText(msg);
    });

    connect(m_statesHandler, &DapCmdStates::sigTunnelCreated, [=]
    {
        // Sending a command to the service to get the data of the last connection
        m_lastConnectionDataHandler->sendCmd();

        m_statsHandler->sendCmdStatsTrafficOn(true);
    });

     connect(m_tunTapHandler, &DapCmdTunTap::sigTapOk, [=] {
          m_serviceCtl->tapStatus = true;
          emit m_serviceCtl->sigTunTapPresent();
     });

     connect(m_tunTapHandler, &DapCmdTunTap::sigTapError, [=] {
          m_serviceCtl->tapStatus = false;
          emit m_serviceCtl->sigTunTapError();
     });


}

void MainWindow::closeEvent(QCloseEvent * event) {
    if (this->isVisible()) {
        event->ignore();
        this->hide();
    }
}

#ifdef Q_OS_WINDOWS
bool MainWindow::nativeEvent(const QByteArray & eventType, void * message, long * result) {
    MSG *msg = static_cast<MSG*>(message);
    if (msg->message == 666) {
        this->showNormal();
        this->setFocus();
        qInfo() << "Client already running";
    }
    return false;
}
#endif

void MainWindow::updateUsrMsg()
{
    if (userMsgQueueIsEmpty() && !user_msg.isEmpty()) {
        dashboard_user_msg = user_msg.get_dashboard_user_MSG();
        qDebug() << "[set_user_msg_if_it_is] " << dashboard_user_msg;

        screen()->setVars("lbMessage","visible",true );
        screen()->setVars("spacerLabel","visible",true );
        screen()->setVars("btMessageClose","visible",true );
        screen()->setVars("lbMessage","text",dashboard_user_msg);
    } else if (userMsgQueueIsEmpty() && user_msg.isEmpty()) {
        screen()->setVars("lbMessage","visible",false );
        screen()->setVars("spacerLabel","visible",false );
        screen()->setVars("btMessageClose","visible",false );
        screen()->setVars("lbMessage","text","");
    }
}


void MainWindow::onBtMessageCloseClicked()
{
    qDebug() << "[user_click_close_msg]";
    dashboard_user_msg = "";
    updateUsrMsg();
}


void MainWindow::onLogout(){
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
    //effect->setBlurRadius(20);
    effect->setBlurRadius(40);
    setGraphicsEffect(effect);
    screen()->setVars("graphicsView","visible",false);
    if(QMessageBox::question(
                this,
                QString("%1: Important information").arg(DAP_BRAND),
                "After that action the network will be unprotected, do you want to continue?"
                ) == QMessageBox::Yes) {
        emit sigBtDisconnect();
    }
    screen()->setVars("graphicsView","visible",true);
    setGraphicsEffect(0);
}

/**
 * @brief MainWindow::btLogin
 */
void MainWindow::onReqConnect(const DapServerInfo& dsi, QString a_user, QString a_ps)
{
    qDebug() << "[MW] btLogin()";
    setStatusText(""); // clear status text
    if (stateLoginConnecting->active() ) { //
        sendDisconnectionReq();
    } else {
        m_user = a_user;
        m_password = a_ps;
        m_selectedServerInfo = dsi;
        m_connectHandler->sendCmdConnect(dsi.address,  dsi.port, m_user, a_ps);
    }
    emit sigBtConnect();
}

void MainWindow::sendDisconnectionReq()
{
    m_connectHandler->sendCmdDisconnect();
}

void MainWindow::onExit()
{
    // sendCmd("disconnect");
    // m_serviceCtl->sendCmd("disconnect");
    this->close();
}


void MainWindow::setStatusText(const QString &a_txt, DapUiScreen *a_screen /*= nullptr*/)
{
    if (!a_screen)
        a_screen = screen();
    a_screen->setVars("lbMessage","text",a_txt);
}

void MainWindow::initTray()
{
    trayMenu = new QMenu(this);
    trayMenu->setObjectName("trayMenu");
    actionDisconnect = trayMenu->addAction("Disconnect",
                                           this,
                                           SLOT(onLogout())
                                           );
    actionDisconnect->setEnabled(false);

    trayMenu->addSeparator();

    trayIcon = new QSystemTrayIcon();
    trayIcon->setIcon(QIcon(":/pics/logo_main@2x.png"));
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, static_cast<void (QSystemTrayIcon::*)(QSystemTrayIcon::ActivationReason)>(&QSystemTrayIcon::activated),
            [=](QSystemTrayIcon::ActivationReason arg){
        if (arg == QSystemTrayIcon::Context) return;
        this->setVisible(!this->isVisible());
    });
}


/**
 * @brief MainWindow::initIndicators
 */
void MainWindow::initIndicators()
{
    // Authorization state
    siAuthorization= new  DapUiVpnStateIndicator(&sm,"cbAuthorized","lbAuthorizationText", "lbAuthorization");
    siAuthorization->setUiLabelTextState(IndicatorState::True,tr("Connection established"));
    siAuthorization->setUiLabelTextState(IndicatorState::FalseToTrue,tr("Authorizing"));
    siAuthorization->setUiLabelTextState(IndicatorState::TrueToFalse,tr("Unauthorizing"));
    siAuthorization->setUiLabelTextState(IndicatorState::False,tr("Not Authorized"));
    siAuthorization->setUiIndicatorState(IndicatorState::True, m_styleHolder->getWidgetStyleSheet("#lbAuthorization", "active"));
    QString authorizationPassiveStyle(m_styleHolder->getWidgetStyleSheet("#lbAuthorization", "passive"));
    siAuthorization->setUiIndicatorState(IndicatorState::FalseToTrue, authorizationPassiveStyle);
    siAuthorization->setUiIndicatorState(IndicatorState::TrueToFalse, authorizationPassiveStyle);
    siAuthorization->setUiIndicatorState(IndicatorState::False, authorizationPassiveStyle);

    // Stream opened state
    siStream = new  DapUiVpnStateIndicator(&sm,"cbStream","lbStreamText", "lbStream");
    siStream->setUiLabelTextState(IndicatorState::True,tr("IP requested"));
    siStream->setUiLabelTextState(IndicatorState::FalseToTrue,tr("Stream Opening"));
    siStream->setUiLabelTextState(IndicatorState::TrueToFalse,tr("Stream Closing"));
    siStream->setUiLabelTextState(IndicatorState::False,tr("Stream Closed"));
    siStream->setUiIndicatorState(IndicatorState::True, m_styleHolder->getWidgetStyleSheet("#lbStream", "active"));
    QString streamPassiveStyle(m_styleHolder->getWidgetStyleSheet("#lbStream", "passive"));
    siStream->setUiIndicatorState(IndicatorState::FalseToTrue, streamPassiveStyle);
    siStream->setUiIndicatorState(IndicatorState::TrueToFalse, streamPassiveStyle);
    siStream->setUiIndicatorState(IndicatorState::False, streamPassiveStyle);

    // Tunnel state
    siTunnel= new  DapUiVpnStateIndicator(&sm,"cbTunnel","lbTunnelText", "lbTunnel");
    siTunnel->setUiLabelTextState(IndicatorState::True,tr("Virtual network interface"));
    siTunnel->setUiLabelTextState(IndicatorState::FalseToTrue,tr("Network Protecting"));
    siTunnel->setUiLabelTextState(IndicatorState::TrueToFalse,tr("Network Unprotecting"));
    siTunnel->setUiLabelTextState(IndicatorState::False,tr("No Network Protection"));
    siTunnel->setUiIndicatorState(IndicatorState::True, m_styleHolder->getWidgetStyleSheet("#lbTunnel", "active"));
    QString tunnelPassiveStyle(m_styleHolder->getWidgetStyleSheet("#lbTunnel", "passive"));
    siTunnel->setUiIndicatorState(IndicatorState::FalseToTrue, tunnelPassiveStyle);
    siTunnel->setUiIndicatorState(IndicatorState::TrueToFalse, tunnelPassiveStyle);
    siTunnel->setUiIndicatorState(IndicatorState::False, tunnelPassiveStyle);
}

/**
 * @brief MainWindow::initIndicatorsTransitions
 */
void MainWindow::initIndicatorsTransitions()
{
    //============ Indicator transitions ===========

    //----Authorization-----

    // Authorization ::False state
    siAuthorization->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorizing()),siAuthorization->state(IndicatorState::FalseToTrue));
    siAuthorization->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),siAuthorization->state(IndicatorState::True));

    // Authorization ::FalseToTrue state
    siAuthorization->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),siAuthorization->state(IndicatorState::True));
    siAuthorization->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()),siAuthorization->state(IndicatorState::False));

    // Authorization ::TrueToFalse state
    siAuthorization->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorizing()),siAuthorization->state(IndicatorState::FalseToTrue));
    siAuthorization->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorized()),siAuthorization->state(IndicatorState::True));
    siAuthorization->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()),siAuthorization->state(IndicatorState::False));

    // Authorization ::True state
    siAuthorization->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigAuthorizing()),siAuthorization->state(IndicatorState::FalseToTrue));
    siAuthorization->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()),siAuthorization->state(IndicatorState::TrueToFalse));
    siAuthorization->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigUnauthorized()),siAuthorization->state(IndicatorState::False));

    //----Stream Opened-----

    // Stream ::False state
    siStream->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpening()),siStream->state(IndicatorState::FalseToTrue));
    siStream->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),siStream->state(IndicatorState::True));
    siStream->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosing()),siStream->state(IndicatorState::TrueToFalse));

    // Stream ::FalseToTrue state
    siStream->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),siStream->state(IndicatorState::True));
    siStream->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosed()),siStream->state(IndicatorState::False));
    siStream->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosing()),siStream->state(IndicatorState::TrueToFalse));

    // Stream ::TrueToFalse state
    siStream->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpening()),siStream->state(IndicatorState::FalseToTrue));
    siStream->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpened()),siStream->state(IndicatorState::True));
    siStream->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosed()),siStream->state(IndicatorState::False));

    // Stream ::True state
    siStream->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamOpening()),siStream->state(IndicatorState::FalseToTrue));
    siStream->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosing()),siStream->state(IndicatorState::TrueToFalse));
    siStream->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigStreamClosed()),siStream->state(IndicatorState::False));

    //----Tunnel -----

    // Tunnel ::False state
    siTunnel->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreating()),siTunnel->state(IndicatorState::FalseToTrue));
    siTunnel->state(IndicatorState::False)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),siTunnel->state(IndicatorState::True));

    // Tunnel ::FalseToTrue state
    siTunnel->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),siTunnel->state(IndicatorState::True));
    siTunnel->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroyed()),siTunnel->state(IndicatorState::False));
    siTunnel->state(IndicatorState::FalseToTrue)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroying()),siTunnel->state(IndicatorState::TrueToFalse));

    // Tunnel ::TrueToFalse state
    siTunnel->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreated()),siTunnel->state(IndicatorState::True));
    siTunnel->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroyed()),siTunnel->state(IndicatorState::False));
    siTunnel->state(IndicatorState::TrueToFalse)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreating()),siTunnel->state(IndicatorState::FalseToTrue));

    // Tunnel ::True state
    siTunnel->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelCreating()),siTunnel->state(IndicatorState::FalseToTrue));

    siTunnel->state(IndicatorState::True)
            ->addTransition(m_statesHandler,SIGNAL(sigTunnelDestroyed()),siTunnel->state(IndicatorState::False));

}

/// Set the style of the login edit box depending on the text.
/// @param a_edLogin Login edit field.
void MainWindow::setStyleLoginField(QLineEdit *a_edLogin)
{
#ifndef DAP_PLATFORM_MOBILE						   
    a_edLogin->setStyleSheet(a_edLogin->text().isEmpty() || a_edLogin->text().isNull()
                            ? m_styleHolder->getWidgetStyleSheet("#edMail", "empty")

                            : m_styleHolder->getWidgetStyleSheet("#edMail", "filled"));
#else
   Q_UNUSED(a_edLogin);
#endif
}

/// Set the style of the password edit box depending on the filling of the text and text input.
/// @param a_edPassword Password edit field.
void MainWindow::setStylePasswordField(QLineEdit *a_edPassword)
{
#ifndef DAP_PLATFORM_MOBILE
    a_edPassword->setStyleSheet(a_edPassword->text().isEmpty() || a_edPassword->text().isNull()
                                 ? m_styleHolder->getWidgetStyleSheet("#edPassword", "empty")
                                 : m_styleHolder->getWidgetStyleSheet("#edPassword", "filled"));
#else
   Q_UNUSED(a_edPassword);
#endif
}
