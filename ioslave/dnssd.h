/***************************************************************************
 *   Copyright (C) 2004, 2005 by Jakub Stachowski                                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#ifndef _dnssd_H_
#define _dnssd_H_

// KDE
#include <DNSSD/ServiceBrowser>
#include <dnssd/servicetypebrowser.h> // missing CamelCase version, can be fixed beginning 02.02.09
#include <DNSSD/RemoteService>
#include <KIO/SlaveBase>
// Qt
#include <QtCore/QObject>


using namespace KIO;
using namespace DNSSD;

class ZeroConfUrl;

struct ProtocolData
{
    ProtocolData() {}
    ProtocolData( const QString& _name, const QString& proto,
                  const QString& path=QString(), const QString& user=QString(), const QString& passwd=QString() )
     : name(_name), protocol(proto), pathEntry(path), userEntry(user), passwordEntry(passwd)
    {}

    void feedUrl( KUrl* url, const RemoteService* remoteService ) const;

    QString name;
    QString protocol;
    QString pathEntry;
    QString userEntry;
    QString passwordEntry;
};

class ZeroConfProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT
public:
    ZeroConfProtocol( const QByteArray& protocol, const QByteArray& pool_socket, const QByteArray& app_socket);
    virtual ~ZeroConfProtocol();

public: // KIO::SlaveBase API
    virtual void get( const KUrl& url );
    virtual void mimetype( const KUrl& url );
    virtual void stat( const KUrl& url );
    virtual void listDir( const KUrl& url );

Q_SIGNALS:
    void leaveModality();

private:
    // Create UDSEntry for zeroconf:/ or zeroconf:/type/ urls
    void feedEntryAsDir( UDSEntry* entry, const QString& name, const QString& displayName = QString() );
    // resolve given service and redirect() to it
    void resolveAndRedirect( const ZeroConfUrl& zeroConfUrl );

    bool dnssdOK();

    void enterLoop();

private Q_SLOTS:
    void addServiceType( const QString& );
    void addService( DNSSD::RemoteService::Ptr );
    void onBrowserFinished();

private: // data
    ServiceBrowser* serviceBrowser;
    ServiceTypeBrowser* serviceTypeBrowser;
    // service types merged from all domains - to avoid duplicates
    QStringList ServiceTypesAdded;

    RemoteService* serviceToResolve;
    QHash<QString,ProtocolData> knownProtocols;
};

#endif
