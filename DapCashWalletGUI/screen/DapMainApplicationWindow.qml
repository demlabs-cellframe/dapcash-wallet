import QtQuick 2.4

DapMainApplicationWindowForm 
{
    id: dapMainWindow
    ///@detalis Path to the dashboard tab.
    readonly property string dashboardScreen: "qrc:/screen/" + device + "/Dashboard/DapDashboardTab.qml"
    ///@detalis Path to the settings tab.
    readonly property string settingsScreen: "qrc:/screen/" + device + "/Settings/DapSettingsTab.qml"
    ///@detalis Path to the console tab.
    readonly property string consoleScreen: "qrc:/screen/" + device + "/Console/DapConsoleTab.qml"

    property var dapWallets: []

    signal modelWalletsUpdated()

    ListModel
    {
        id: dapNetworkModel
    }

    ListModel
    {
        id: dapModelWallets
    }

    // Menu bar tab model
    ListModel 
    {
        id: modelMenuTab
        
        Component.onCompleted:
        {
            append({
                name: qsTr("Dashboard"),
                page: dashboardScreen,
                normalIcon: "qrc:/res/icons/icon_dashboard.png",
                hoverIcon: "qrc:/res/icons/icon_dashboard_hover.png"
            })

            append ({
                name: qsTr("Console"),
                page: consoleScreen,
                normalIcon: "qrc:/res/icons/icon_console.png",
                hoverIcon: "qrc:/res/icons/icon_console_hover.png"
            })

            append ({
                name: qsTr("Settings"),
                page: settingsScreen,
                normalIcon: "qrc:/res/icons/icon_settings.png",
                hoverIcon: "qrc:/res/icons/icon_settings_hover.png"
            })
        }
    }

    dapScreenLoader.source: dashboardScreen
    
    dapMenuTabWidget.onPathScreenChanged:
    {
        dapScreenLoader.setSource(Qt.resolvedUrl(dapMenuTabWidget.pathScreen))
    }

    Component.onCompleted:
    {
        dapServiceController.requestToService("DapGetListNetworksCommand");
        dapServiceController.requestToService("DapGetListWalletsCommand");
    }

    Connections
    {
        target: dapServiceController
        onNetworksListReceived:
        {
            for(var n=0; n < Object.keys(networkList).length; ++n)
            {
                dapServiceController.CurrentNetwork = networkList[0];
                dapServiceController.IndexCurrentNetwork = 0;
                dapNetworkModel.append({name: networkList[n]})
            }
        }

        onWalletsReceived:
        {
            console.log(walletList.length)
            console.log(dapWallets.length)
            console.log(dapModelWallets.count)
                for (var q = 0; q < walletList.length; ++q)
                {
                    dapWallets.push(walletList[q])
                }
                for (var i = 0; i < dapWallets.length; ++i)
                {
                    console.log(dapWallets[i].Name)
                    dapModelWallets.append({ "name" : dapWallets[i].Name,
                                          "balance" : dapWallets[i].Balance,
                                          "icon" : dapWallets[i].Icon,
                                          "address" : dapWallets[i].Address,
                                          "networks" : []})
                    console.log(Object.keys(dapWallets[i].Networks).length)
                    for (var n = 0; n < Object.keys(dapWallets[i].Networks).length; ++n)
                    {
                         dapModelWallets.get(i).networks.append({"name": dapWallets[i].Networks[n],
                                                              "address": dapWallets[i].findAddress(dapWallets[i].Networks[n]),
                                                              "tokens": []})
                        console.log(Object.keys(dapWallets[i].Tokens).length)
                        for (var t = 0; t < Object.keys(dapWallets[i].Tokens).length; ++t)
                        {
                            console.log(dapWallets[i].Tokens[t].Network + " === " + dapWallets[i].Networks[n])
                            if(dapWallets[i].Tokens[t].Network === dapWallets[i].Networks[n])
                            {
                                 dapModelWallets.get(i).networks.get(n).tokens.append({"name": dapWallets[i].Tokens[t].Name,
                                                                                    "balance": dapWallets[i].Tokens[t].Balance,
                                                                                    "emission": dapWallets[i].Tokens[t].Emission,
                                                                                    "network": dapWallets[i].Tokens[t].Network})
                            }
                        }

                    }

                }
                modelWalletsUpdated()
            }
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
