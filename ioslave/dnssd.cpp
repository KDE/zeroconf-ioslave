/***************************************************************************
 *   Copyright (C) 2004, 2005 by Jakub Stachowski                          *
 *   qbast@go2.pl                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "dnssd.h"

// io-slave
#include "zeroconfurl.h"
// KDE
#include <KComponentData>
#include <KProtocolInfo>
// Qt
#include <QtCore/QCoreApplication>


void ProtocolData::feedUrl( KUrl* url, const RemoteService* remoteService ) const
{
    const QMap<QString,QByteArray> serviceTextData = remoteService->textData();

    url->setProtocol( protocol );
    if (!userEntry.isNull())
        url->setUser( serviceTextData[userEntry] );
    if (!passwordEntry.isNull())
        url->setPass( serviceTextData[passwordEntry] );
    if (!pathEntry.isNull())
        url->setPath( serviceTextData[pathEntry] );

    url->setHost( remoteService->hostName() );
    url->setPort( remoteService->port() );
}


ZeroConfProtocol::ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase(protocol, pool_socket, app_socket),
    serviceBrowser(0), serviceTypeBrowser(0), serviceToResolve(0)
{
    knownProtocols["_ftp._tcp"]=     ProtocolData(i18n("FTP servers"),            "ftp",    "path",    "u", "p");
    knownProtocols["_webdav._tcp"]=  ProtocolData(i18n("WebDav remote directory"),"webdav", "path");
    knownProtocols["_sftp-ssh._tcp"]=ProtocolData(i18n("Remote disk (sftp)"),     "sftp",   QString(), "u", "p");
    knownProtocols["_ssh._tcp"]=     ProtocolData(i18n("Remote disk (fish)"),     "fish",   QString(), "u", "p");
    knownProtocols["_nfs._tcp"]=     ProtocolData(i18n("NFS remote directory"),    "nfs",   "path");
}

ZeroConfProtocol::~ZeroConfProtocol()
{
    delete serviceToResolve;
}

void ZeroConfProtocol::get( const KUrl& url )
{
    if (!dnssdOK())
        return;

    const ZeroConfUrl zeroConfUrl( url );

    ZeroConfUrl::Type type = zeroConfUrl.type();
    if (type==ZeroConfUrl::Service)
        resolveAndRedirect( zeroConfUrl );
    else
        error( ERR_MALFORMED_URL, url.prettyUrl() );
}

void ZeroConfProtocol::mimetype( const KUrl& url )
{
    resolveAndRedirect( ZeroConfUrl(url) );
}

void ZeroConfProtocol::stat( const KUrl& url )
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
        error( ERR_MALFORMED_URL, url.prettyUrl() );
    }
}

void ZeroConfProtocol::listDir( const KUrl& url )
{
    if (!dnssdOK())
        return;

    const ZeroConfUrl zeroConfUrl( url );

    ZeroConfUrl::Type type = zeroConfUrl.type();
    switch (type)
    {
    case ZeroConfUrl::RootDir:
        serviceTypeBrowser = new ServiceTypeBrowser(zeroConfUrl.domain());
        connect( serviceTypeBrowser, SIGNAL(serviceTypeAdded(QString)),
                 SLOT(addServiceType(QString)) );
        connect( serviceTypeBrowser, SIGNAL(finished()), SLOT(onBrowserFinished()) );
        serviceTypeBrowser->startBrowse();
        enterLoop();
        break;
    case ZeroConfUrl::ServiceDir:
        if( !knownProtocols.contains(zeroConfUrl.serviceType()) )
        {
            error( ERR_SERVICE_NOT_AVAILABLE, zeroConfUrl.serviceType() );
            break;
        }
        serviceBrowser = new ServiceBrowser( zeroConfUrl.serviceType(), false, zeroConfUrl.domain() );
        connect( serviceBrowser, SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
                 SLOT(addService(DNSSD::RemoteService::Ptr)) );
        connect( serviceBrowser, SIGNAL(finished()), SLOT(onBrowserFinished()) );
        serviceBrowser->startBrowse();
        enterLoop();
        break;
    case ZeroConfUrl::Service:
        resolveAndRedirect( zeroConfUrl );
        break;
    default:
        error( ERR_MALFORMED_URL, url.prettyUrl() );
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
               i18n("KDE has been built without Zeroconf support."));
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
        serviceToResolve = 0;
    }
    if (serviceToResolve == 0)
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
    KUrl destUrl;
    protocolData.feedUrl( &destUrl, serviceToResolve );

    redirection( destUrl );
    finished();
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
    listEntry( entry, false );
}

void ZeroConfProtocol::addService( DNSSD::RemoteService::Ptr service )
{
    UDSEntry entry;
    entry.insert( UDSEntry::UDS_NAME,      service->serviceName() );
    entry.insert( UDSEntry::UDS_ACCESS,    0666);
    entry.insert( UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    const QString iconName = KProtocolInfo::icon( knownProtocols[service->type()].protocol );
    if (!iconName.isNull())
        entry.insert( UDSEntry::UDS_ICON_NAME, iconName );

    listEntry( entry, false );
}

void ZeroConfProtocol::onBrowserFinished()
{
    UDSEntry entry;
    listEntry( entry, true );
    finished();

    // cleanup
    if (serviceBrowser)
    {
        serviceBrowser->deleteLater();
        serviceBrowser = 0;
    }
    if (serviceTypeBrowser)
    {
        serviceTypeBrowser->deleteLater();
        serviceTypeBrowser = 0;
    }
    ServiceTypesAdded.clear();

    emit leaveModality();
}

void ZeroConfProtocol::feedEntryAsDir( UDSEntry* entry, const QString& name, const QString& displayName )
{
    entry->insert( UDSEntry::UDS_NAME,      name );
    entry->insert( UDSEntry::UDS_ACCESS,    0555 );
    entry->insert( UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry->insert( UDSEntry::UDS_MIME_TYPE, "inode/directory" );
    if (!displayName.isEmpty())
        entry->insert( UDSEntry::UDS_DISPLAY_NAME, displayName );
}

void ZeroConfProtocol::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),&eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}


extern "C"
{
    int KDE_EXPORT kdemain( int argc, char **argv )
    {
        // necessary to use other kio slaves
        KComponentData componentData("kio_zeroconf");
        QCoreApplication app(argc,argv);

        // start the slave
        ZeroConfProtocol slave(argv[1],argv[2],argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}
