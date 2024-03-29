/*
    SPDX-FileCopyrightText: 2004, 2005 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _dnssd_H_
#define _dnssd_H_

// KF
#include <KIO/WorkerBase>
#include <KDNSSD/ServiceBrowser>
#include <KDNSSD/ServiceTypeBrowser>
#include <KDNSSD/RemoteService>
// Qt
#include <QObject>


using namespace KIO;
using namespace KDNSSD;

class ZeroConfUrl;

struct ProtocolData
{
    ProtocolData() {}
    ProtocolData( const QString& _name, const QString& proto,
                  const QString& path=QString(), const QString& user=QString(), const QString& passwd=QString() )
     : name(_name), protocol(proto), pathEntry(path), userEntry(user), passwordEntry(passwd)
    {}

    void feedUrl( QUrl* url, const RemoteService* remoteService ) const;

    QString name;
    QString protocol;
    QString pathEntry;
    QString userEntry;
    QString passwordEntry;
};

class ZeroConfProtocol : public QObject, public KIO::WorkerBase
{
    Q_OBJECT
public:
    ZeroConfProtocol( const QByteArray& protocol, const QByteArray& pool_socket, const QByteArray& app_socket);
    ~ZeroConfProtocol() override;

public: // KIO::WorkerBase API
    KIO::WorkerResult get( const QUrl& url ) override;
    KIO::WorkerResult mimetype( const QUrl& url ) override;
    KIO::WorkerResult stat( const QUrl& url ) override;
    KIO::WorkerResult listDir( const QUrl& url ) override;

Q_SIGNALS:
    void leaveModality();

private:
    // Create UDSEntry for zeroconf:/ or zeroconf:/type/ urls
    void feedEntryAsDir( UDSEntry* entry, const QString& name, const QString& displayName = QString() );
    // resolve given service and redirect() to it
    KIO::WorkerResult resolveAndRedirect( const ZeroConfUrl& zeroConfUrl );

    void listCurrentDirEntry();

    KIO::WorkerResult dnssdOK();

    void enterLoop();

private Q_SLOTS:
    void addServiceType( const QString& );
    void addService( KDNSSD::RemoteService::Ptr );
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
