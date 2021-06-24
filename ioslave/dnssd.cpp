/*
    SPDX-FileCopyrightText: 2004, 2005 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dnssd.h"

// io-slave
#include "zeroconfurl.h"
// KF
#include <KLocalizedString>
#include <KProtocolInfo>
// Qt
#include <QCoreApplication>
#include <qplatformdefs.h> // S_IFDIR


class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.zeroconf" FILE "zeroconf.json")
};


void ProtocolData::feedUrl( QUrl* url, const RemoteService* remoteService ) const
{
    const QMap<QString,QByteArray> serviceTextData = remoteService->textData();

    url->setScheme( protocol );
    if (!userEntry.isNull())
        url->setUserName( QString::fromUtf8(serviceTextData[userEntry]) );
    if (!passwordEntry.isNull())
        url->setPassword( QString::fromUtf8(serviceTextData[passwordEntry]) );
    if (!pathEntry.isNull())
        url->setPath( QString::fromUtf8(serviceTextData[pathEntry]) );

    url->setHost( remoteService->hostName() );
    url->setPort( remoteService->port() );
}


ZeroConfProtocol::ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase(protocol, pool_socket, app_socket),
    serviceBrowser(nullptr),
    serviceTypeBrowser(nullptr),
    serviceToResolve(nullptr)
{
    knownProtocols[QStringLiteral("_ftp._tcp")]=     ProtocolData(i18n("FTP servers"),            QStringLiteral("ftp"),    QStringLiteral("path"), QStringLiteral("u"), QStringLiteral("p"));
    knownProtocols[QStringLiteral("_webdav._tcp")]=  ProtocolData(i18n("WebDav remote directory"),QStringLiteral("webdav"), QStringLiteral("path"));
    knownProtocols[QStringLiteral("_sftp-ssh._tcp")]=ProtocolData(i18n("Remote disk (sftp)"),     QStringLiteral("sftp"),   QString(),              QStringLiteral("u"), QStringLiteral("p"));
    knownProtocols[QStringLiteral("_ssh._tcp")]=     ProtocolData(i18n("Remote disk (fish)"),     QStringLiteral("fish"),   QString(),              QStringLiteral("u"), QStringLiteral("p"));
    knownProtocols[QStringLiteral("_nfs._tcp")]=     ProtocolData(i18n("NFS remote directory"),   QStringLiteral("nfs"),    QStringLiteral("path"));
}

ZeroConfProtocol::~ZeroConfProtocol()
{
    delete serviceToResolve;
}

void ZeroConfProtocol::get( const QUrl& url )
{
    if (!dnssdOK())
        return;

    const ZeroConfUrl zeroConfUrl( url );

    ZeroConfUrl::Type type = zeroConfUrl.type();
    if (type==ZeroConfUrl::Service)
        resolveAndRedirect( zeroConfUrl );
    else
        error( ERR_MALFORMED_URL, url.toDisplayString() );
}

void ZeroConfProtocol::mimetype( const QUrl& url )
{
    resolveAndRedirect( ZeroConfUrl(url) );
}

void ZeroConfProtocol::stat( const QUrl& url )
{
    if (!dnssdOK())
        return;

    const ZeroConfUrl zeroConfUrl( url );

    ZeroConfUrl::Type type = zeroConfUrl.type();

    switch (type)
    {
    case ZeroConfUrl::RootDir:
    case ZeroConfUrl::ServiceDir:
    {
        UDSEntry entry;
        feedEntryAsDir( &entry, QString() );
        statEntry( entry );
        finished();
        break;
    }
    case ZeroConfUrl::Service:
        resolveAndRedirect( zeroConfUrl );
        break;
    default:
        error( ERR_MALFORMED_URL, url.toDisplayString() );
    }
}

void ZeroConfProtocol::listDir( const QUrl& url )
{
    if (!dnssdOK())
        return;

    const ZeroConfUrl zeroConfUrl( url );

    ZeroConfUrl::Type type = zeroConfUrl.type();
    switch (type)
    {
    case ZeroConfUrl::RootDir:
        listCurrentDirEntry();
        serviceTypeBrowser = new ServiceTypeBrowser(zeroConfUrl.domain());
        connect(serviceTypeBrowser, &ServiceTypeBrowser::serviceTypeAdded,
                this, &ZeroConfProtocol::addServiceType);
        connect(serviceTypeBrowser, &ServiceTypeBrowser::finished,
                this, &ZeroConfProtocol::onBrowserFinished);
        serviceTypeBrowser->startBrowse();
        enterLoop();
        break;
    case ZeroConfUrl::ServiceDir:
        if( !knownProtocols.contains(zeroConfUrl.serviceType()) )
        {
            error( ERR_SERVICE_NOT_AVAILABLE, zeroConfUrl.serviceType() );
            break;
        }
        listCurrentDirEntry();
        serviceBrowser = new ServiceBrowser( zeroConfUrl.serviceType(), false, zeroConfUrl.domain() );
        connect(serviceBrowser, &ServiceBrowser::serviceAdded,
                this, &ZeroConfProtocol::addService);
        connect(serviceBrowser, &ServiceBrowser::finished,
                this, &ZeroConfProtocol::onBrowserFinished);
        serviceBrowser->startBrowse();
        enterLoop();
        break;
    case ZeroConfUrl::Service:
        resolveAndRedirect( zeroConfUrl );
        break;
    default:
        error( ERR_MALFORMED_URL, url.toDisplayString() );
    }
}

bool ZeroConfProtocol::dnssdOK()
{
    bool result;

    switch (ServiceBrowser::isAvailable())
    {
    case ServiceBrowser::Stopped:
        error( KIO::ERR_UNSUPPORTED_ACTION,
               i18n("The Zeroconf daemon (mdnsd) is not running."));
        result = false;
        break;
     case ServiceBrowser::Unsupported:
        error( KIO::ERR_UNSUPPORTED_ACTION,
               i18n("The KDNSSD library has been built without Zeroconf support."));
        result = false;
        break;
    default:
        result = true;
    }

    return result;
}

void ZeroConfProtocol::resolveAndRedirect( const ZeroConfUrl& zeroConfUrl )
{
    if (serviceToResolve && !zeroConfUrl.matches(serviceToResolve))
    {
        delete serviceToResolve;
        serviceToResolve = nullptr;
    }
    if (serviceToResolve == nullptr)
    {
        if( !knownProtocols.contains(zeroConfUrl.serviceType()) )
        {
            error( ERR_SERVICE_NOT_AVAILABLE, zeroConfUrl.serviceType() );
            return;
        }

        serviceToResolve = new RemoteService( zeroConfUrl.serviceName(), zeroConfUrl.serviceType(), zeroConfUrl.domain() );
        if (!serviceToResolve->resolve())
        {
            error( ERR_DOES_NOT_EXIST, zeroConfUrl.serviceName() );
            return;
        }
    }

    if( !knownProtocols.contains(zeroConfUrl.serviceType()) )
        return;

    // action
    const ProtocolData& protocolData = knownProtocols[zeroConfUrl.serviceType()];
    QUrl destUrl;
    protocolData.feedUrl( &destUrl, serviceToResolve );

    redirection( destUrl );
    finished();
}

void ZeroConfProtocol::listCurrentDirEntry()
{
    UDSEntry entry;
    feedEntryAsDir( &entry, QStringLiteral(".") );
    listEntry(entry);
}

void ZeroConfProtocol::addServiceType( const QString& serviceType )
{
    if (ServiceTypesAdded.contains(serviceType))
        return;
    ServiceTypesAdded << serviceType;

    if (!knownProtocols.contains(serviceType))
        return;

    // action
    UDSEntry entry;
    feedEntryAsDir( &entry, serviceType, knownProtocols[serviceType].name );
    listEntry( entry );
}

void ZeroConfProtocol::addService( KDNSSD::RemoteService::Ptr service )
{
    UDSEntry entry;
    entry.fastInsert(UDSEntry::UDS_NAME,      service->serviceName() );
    entry.fastInsert( UDSEntry::UDS_ACCESS,    0666);
    entry.fastInsert( UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    const QString iconName = KProtocolInfo::icon( knownProtocols[service->type()].protocol );
    if (!iconName.isNull())
        entry.fastInsert( UDSEntry::UDS_ICON_NAME, iconName );

    listEntry( entry );
}

void ZeroConfProtocol::onBrowserFinished()
{
    finished();

    // cleanup
    if (serviceBrowser)
    {
        serviceBrowser->deleteLater();
        serviceBrowser = nullptr;
    }
    if (serviceTypeBrowser)
    {
        serviceTypeBrowser->deleteLater();
        serviceTypeBrowser = nullptr;
    }
    ServiceTypesAdded.clear();

    Q_EMIT leaveModality();
}

void ZeroConfProtocol::feedEntryAsDir( UDSEntry* entry, const QString& name, const QString& displayName )
{
    entry->fastInsert( UDSEntry::UDS_NAME,      name );
    entry->fastInsert( UDSEntry::UDS_ACCESS,    0555 );
    entry->fastInsert( UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry->fastInsert( UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory") );
    if (!displayName.isEmpty())
        entry->fastInsert( UDSEntry::UDS_DISPLAY_NAME, displayName );
}

void ZeroConfProtocol::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, &ZeroConfProtocol::leaveModality, &eventLoop, &QEventLoop::quit);
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}


extern "C" Q_DECL_EXPORT int kdemain( int argc, char **argv )
{
    // necessary to use other kio slaves
    QCoreApplication app(argc,argv);
    app.setApplicationName(QStringLiteral("kio_zeroconf"));

    if (argc != 4) {
        fprintf(stderr, "Usage: %s protocol domain-socket1 domain-socket2\n", argv[0]);
        exit(-1);
    }

    // start the slave
    ZeroConfProtocol slave(argv[1],argv[2],argv[3]);
    slave.dispatchLoop();
    return 0;
}

#include "dnssd.moc"
