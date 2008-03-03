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

#include <qstring.h>
#include <qobject.h>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <dnssd/servicebrowser.h>
#include <dnssd/servicetypebrowser.h>
#include <dnssd/remoteservice.h>
#include <qstringlist.h>


using namespace KIO;
using namespace DNSSD;

enum UrlType { RootDir, ServiceDir, Service,  Invalid };

struct ProtocolData {
    ProtocolData() {}
    ProtocolData(const QString& _name, const QString& proto, const QString& path=QString(), 
	const QString& user=QString(), const QString& passwd=QString()) : pathEntry(path), 
	name(_name), userEntry(user), passwordEntry(passwd), protocol(proto) {}
	
    QString pathEntry;
    QString name;
    QString userEntry;
    QString passwordEntry;
    QString protocol;
};

class ZeroConfProtocol : public QObject, public KIO::SlaveBase
{
	Q_OBJECT
public:
	ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket);
	~ZeroConfProtocol();
	virtual void get(const KUrl& url);
	virtual void mimetype(const KUrl& url);
	virtual void stat(const KUrl& url);
	virtual void listDir(const KUrl& url );
signals:
	void leaveModality();
private:
	// Create UDSEntry for zeroconf:/ or zeroconf:/type/ paths
	void buildDirEntry(UDSEntry& entry,const QString& name,const QString& type=QString());
	// Returns root dir, service dir, service or invalid
	UrlType checkURL(const KUrl& url);
	// extract name and type  from URL
	void dissect(const KUrl& url,QString& name,QString& type);
	// resolve given service and redirect() to it
	void resolveAndRedirect(const KUrl& url);
	bool dnssdOK();

	void enterLoop();

	ServiceBrowser* browser;
	ServiceTypeBrowser* typebrowser;
	// service types merged from all domains - to avoid duplicates
	QStringList mergedtypes;

	RemoteService *toResolve;
	QHash<QString,ProtocolData> knownProtocols;
	
private slots:
	void newType(const QString&);
	void newService(DNSSD::RemoteService::Ptr);
	void allReported();

};

#endif
