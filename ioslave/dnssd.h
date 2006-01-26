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
#include <dnssd/remoteservice.h>
#include <qstringlist.h>


using namespace KIO;
using namespace DNSSD;

enum UrlType { RootDir, ServiceDir, Service, HelperProtocol, Invalid };

class ZeroConfProtocol : public QObject, public KIO::SlaveBase
{
	Q_OBJECT
public:
	ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket);
	virtual void get(const KUrl& url);
	virtual void mimetype(const KUrl& url);
	virtual void stat(const KUrl& url);
	virtual void listDir(const KUrl& url );
signals:
	void leaveModality();
private:
	// Create UDSEntry for zeroconf:/ or zeroconf:/type/ paths
	void buildDirEntry(UDSEntry& entry,const QString& name,const QString& type=QString::null, 
		const QString& host=QString::null);
	// Create UDSEntry for single services: dnssd:/type/service
	void buildServiceEntry(UDSEntry& entry,const QString& name,const QString& type,
			       const QString& domain);
	// Returns root dir, service dir, service or invalid
	UrlType checkURL(const KUrl& url);
	// extract name, type and domain from URL
	void dissect(const KUrl& url,QString& name,QString& type,QString& domain);
	// resolve given service and redirect() to it or use KRun on it (used for helper protocols)
	void resolveAndRedirect(const KUrl& url, bool useKRun = false);
	bool dnssdOK();
	QString getAttribute(const QString& name);
	QString getProtocol(const QString& type);
	// try to load config file for given service type (or just return if already loaded)
	bool setConfig(const QString& type);

	void enterLoop();

	ServiceBrowser* browser;
	// service types merged from all domains - to avoid duplicates
	QStringList mergedtypes;
	// last resolved or still being resolved services - acts as one-entry cache
	RemoteService *toResolve;
	// Config file for service - also acts as one-entry cache
	KConfig *configData;
	// listDir for all domains (zeroconf:/) or one specified (zeroconf://domain/)
	bool allDomains;
private slots:
	void newType(DNSSD::RemoteService::Ptr);
	void newService(DNSSD::RemoteService::Ptr);
	void allReported();

};

#endif
