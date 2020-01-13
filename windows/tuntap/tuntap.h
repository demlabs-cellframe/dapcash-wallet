#ifndef TUN_TAP_H
#define TUN_TAP_H

#include <io.h>
#include <memory>
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <tchar.h>
#include <cassert>
#include <ws2tcpip.h>
#include <string> ///TODO migrate to std::string to QString
#include <vector>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <iphlpapi.h>
#include <objbase.h>
#include <netcon.h>
#include <initguid.h>
#include "dap_common.h"
#include "registry.h"

DEFINE_GUID(IID_INetConnectionManager, 0xC08956A2,0x1CD3,0x11D1,0xB1,0xC5,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(CLSID_ConnectionManager, 0xBA126AD1,0x2166,0x11D1,0xB1,0xD0,0x0,0x80,0x5F,0xC1,0x27,0x0E);

#define APIBUFLEN 32768

QString GetLastErrorAsString();

class WinSecurityParam
{
private:
    SECURITY_ATTRIBUTES _sa;
    SECURITY_DESCRIPTOR _sd;
    bool _state;

public:
    operator bool() {return _state;}
    operator LPSECURITY_ATTRIBUTES() {return &_sa;}
    operator PISECURITY_DESCRIPTOR() {return &_sd;}

    bool injectPermissions(HANDLE obj) {
        bool ret = SetKernelObjectSecurity(obj, DACL_SECURITY_INFORMATION, &_sd);
        return ret;
    }

    WinSecurityParam () {
        _sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        _sa.lpSecurityDescriptor = std::addressof(_sd);
        _state = (_sa.bInheritHandle = true);
        if ( InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION) &&
             SetSecurityDescriptorDacl(&_sd, TRUE, nullptr, FALSE) ) {
            _state = true;
        }
    }
};

struct rw_handle {
    HANDLE read;
    HANDLE write;
};

struct overlapped_io {
    OVERLAPPED overlapped;
    DWORD size;
    overlapped_io(){memset(&overlapped, 0, sizeof(OVERLAPPED));}
    overlapped_io &operator=(const overlapped_io &rhs) {
        //qDebug() << "call overlapped_io=(overlapped_io) operator";
        if (&rhs == this)
            return *this;
        overlapped.hEvent   = rhs.overlapped.hEvent;
        overlapped.Internal = rhs.overlapped.Internal;
        overlapped.InternalHigh = rhs.overlapped.InternalHigh;
        overlapped.Offset = rhs.overlapped.Offset;
        overlapped.OffsetHigh = rhs.overlapped.OffsetHigh;
        overlapped.Pointer = rhs.overlapped.Pointer;
        size = rhs.size;
        return *this;
    }
};

struct TunDevice
{
    HANDLE _fileHandle;
    struct overlapped_io reads;  // handle ready to read
    struct overlapped_io writes; // handle ready to write
    struct rw_handle rw_handle;

    TunDevice():_fileHandle(nullptr){}
    TunDevice &operator=(HANDLE fh){_fileHandle=fh; return *this;}
    TunDevice &operator=(const TunDevice &rhs) {
        //qDebug() << "call TunDevice=(TunDevice) operator";
        if (&rhs == this)
            return *this;
        _fileHandle = rhs._fileHandle;
        reads = rhs.reads;
        writes = rhs.writes;
        rw_handle.read = rhs.rw_handle.read;
        rw_handle.write = rhs.rw_handle.write;
        return *this;
    }
};

class TunTap
{
    typedef BOOL (WINAPI* pDnsFlushResolverCache)();
    pDnsFlushResolverCache DnsFlushResolverCache;

    typedef void (WINAPI* pNcFreeNetconProperties)(NETCON_PROPERTIES*);
    pNcFreeNetconProperties NcFreeNetconProperties;

    typedef int (WINAPI* pDHCPNotifyProc)(LPWSTR, LPWSTR, BOOL, DWORD, DWORD, DWORD, int);
    pDHCPNotifyProc pDhcpNotifyProc;

    QMap<int, TunDevice> _tunDevicesMap;
    TunDevice *tunDevice;
    int _ifIndex[2];
    QString _tunWinId;

    MIB_IPFORWARDROW _defaultGateWay;
    HANDLE hInactive;
    HANDLE _fileHandle;

    HINSTANCE hDnsApi;
    HINSTANCE hNetShell;
    HINSTANCE hDhcpDll;

    int _fileDesriptorCounter;

    ULONG NTEContext;
    bool _active;

private:
    TunTap();
    TunTap(const TunTap &);
    TunTap & operator=(const TunTap &);
    ~TunTap();
    static unsigned ctl_code(unsigned deviceType, unsigned function, unsigned method, unsigned acess);
    static unsigned tap_control_code(unsigned request, unsigned method);
    static QString _inet_to_a(const struct in_addr &in);
    static unsigned long _ipToNumber(const QString &str);

    int _getTunAdapterIndex();

    void overlappedIoInit(struct overlapped_io *o, bool eventState);

public:
    enum AdapterType {ETH, TUN};
    static TunTap &getInstance();
    int makeTunTapDevice(QString &outTunName);
    int setAdress(const QString &ip, const QString &gw, const QString &mask=QString("255.255.255.255"));
    QString getDefaultGateWay();
    size_t getDefaultGateWayCount();
    bool defaultRouteDelete();
    std::vector<MIB_IPFORWARDROW> customRoutes;
    void makeRoute(AdapterType adapter, const QString &dest, const QString &gate, ulong metric, const QString &mask=QString("255.255.255.255"), bool addToTable = true);
    void deleteRoutesByGw(const char *gw);
    QString getDefaultDest();
    QString getDefaulGateWay();
    QString getDefaultMask();
    int getDefaultAdapterIndex();
    int getTunTapAdapterIndex();
    QStringList getFriendlyNameAndDefaultDNS(int);
    static void setDNSAddresses(int, QStringList);
    bool deleteCustomRoutes(void);
    bool unassignTunAdp();
    BOOL setDNS(QString, QString);
    BOOL notifyIPChange(QString);
    BOOL flushDNS();
    bool enableTapAdapter();
    bool determineValidArgs(ulong&, ulong&);

    int ipReleaseAddr(int);
    int ipRenewAddr(int);
    void doCloseTun();
    void closeTun();
    operator bool();

    int write_tun(int devID, u_char *buf, size_t len);
    int read_tun (int devID, u_char *buf, size_t len);

    QString lookupHost(const QString &host, const QString &port);
};

#endif
