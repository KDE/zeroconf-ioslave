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

#include <KComponentData>
#include <KProtocolInfo>

#include <QtCore/QCoreApplication>


ZeroConfProtocol::ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase(protocol, pool_socket, app_socket), browser(0), typebrowser(0), toResolve(0)
{
    knownProtocols["_ftp._tcp"]=ProtocolData(i18n("FTP servers"),"ftp","path","u","p");    
    knownProtocols["_webdav._tcp"]=ProtocolData(i18n("WebDav remote directory"),"webdav","path");    
    knownProtocols["_sftp-ssh._tcp"]=ProtocolData(i18n("Remote disk (sftp)"),"sftp",QString(),"u","p");
    knownProtocols["_ssh._tcp"]=ProtocolData(i18n("Remote disk (fish)"),"fish",QString(),"u","p");
    knownProtocols["_nfs._tcp"]=ProtocolData(i18n("NFS remote directory"),"nfs","path");
}

ZeroConfProtocol::~ZeroConfProtocol()
{
    delete toResolve;
}

void ZeroConfProtocol::get(const KUrl& url )
{
	if (!dnssdOK()) return;
	UrlType t = checkURL(url);
	if (t==Service) resolveAndRedirect(url);
	else error(ERR_MALFORMED_URL,url.prettyUrl());
	
}
void ZeroConfProtocol::mimetype(const KUrl& url )
{
	resolveAndRedirect(url);
}

UrlType ZeroConfProtocol::checkURL(const KUrl& url)
{
	if (url.path()=="/") return RootDir;
	QString service, type;
	dissect(url,service,type);
	if (!knownProtocols.contains(type)) return Invalid;
	if (service.isEmpty()) return ServiceDir;
	return Service;
}

// URL zeroconf:/_http._tcp/some%20service
void ZeroConfProtocol::dissect(const KUrl& url,QString& name,QString& type)
{
	// FIXME: encode domain name into url to support many services with the same name on 
	// different domains
	type = url.path().section('/',1,1);
	name = url.path().section('/',2,-1);

}

bool ZeroConfProtocol::dnssdOK()
{
	switch(ServiceBrowser::isAvailable()) {
        	case ServiceBrowser::Stopped:
			error(KIO::ERR_UNSUPPORTED_ACTION,
			i18n("The Zeroconf daemon (mdnsd) is not running."));
			return false;
		case ServiceBrowser::Unsupported:
	    		error(KIO::ERR_UNSUPPORTED_ACTION,
			i18n("KDE has been built without Zeroconf support."));
			return false;
          default:
			return true;
        }
}

void ZeroConfProtocol::stat(const KUrl& url)
{
	UDSEntry entry;
	if (!dnssdOK()) return;
	UrlType t = checkURL(url);
	switch (t) {
	case RootDir:
	case ServiceDir:
		buildDirEntry(entry,QString());
		statEntry(entry);
		finished();
		break;
	case Service:
		resolveAndRedirect(url);
	  	break;
	default:
		error(ERR_MALFORMED_URL,url.prettyUrl());
	}
}

void ZeroConfProtocol::resolveAndRedirect(const KUrl& url)
{
	QString name,type;
	dissect(url,name,type);
	if (toResolve!=0 && (toResolve->serviceName()!=name || toResolve->type()!=type))  {
		delete toResolve;
		toResolve = 0;
	}
	if (toResolve==0) {
		toResolve = new RemoteService(name,type,QString());
		if (!toResolve->resolve()) error(ERR_DOES_NOT_EXIST,name);
	}
	KUrl destUrl;
	destUrl.setProtocol(knownProtocols[type].protocol);
	if (!knownProtocols[type].userEntry.isNull()) 
	    destUrl.setUser(toResolve->textData()[knownProtocols[type].userEntry]);
	if (!knownProtocols[type].passwordEntry.isNull()) 
	    destUrl.setPass(toResolve->textData()[knownProtocols[type].passwordEntry]);
	if (!knownProtocols[type].pathEntry.isNull()) 
	    destUrl.setPath(toResolve->textData()[knownProtocols[type].pathEntry]);
	destUrl.setHost(toResolve->hostName());
	destUrl.setPort(toResolve->port());
	redirection(destUrl);
	finished();
}

void ZeroConfProtocol::buildDirEntry(UDSEntry& entry,const QString& name,const QString& type)
{
	entry.clear();
	entry.insert(UDSEntry::UDS_NAME,name);
	entry.insert(UDSEntry::UDS_ACCESS,0555);
	entry.insert(UDSEntry::UDS_SIZE,0);
	entry.insert(UDSEntry::UDS_FILE_TYPE,S_IFDIR);
	entry.insert(UDSEntry::UDS_MIME_TYPE,"inode/directory");
	if (!type.isNull())
			entry.insert(UDSEntry::UDS_URL,"zeroconf:/"+type+'/');
}

void ZeroConfProtocol::listDir(const KUrl& url )
{

	if (!dnssdOK()) return;
	UrlType t  = checkURL(url);
	UDSEntry entry;
	switch (t) {
	case RootDir:
		typebrowser = new ServiceTypeBrowser();
		connect(typebrowser,SIGNAL(serviceTypeAdded(const QString&)),
			this,SLOT(newType(const QString&)));
		connect(typebrowser,SIGNAL(finished()),this,SLOT(allReported()));
		typebrowser->startBrowse();
		enterLoop();
		return;
	case ServiceDir:
		browser = new ServiceBrowser(url.path(KUrl::RemoveTrailingSlash).section('/',1,-1));
		connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
			this,SLOT(newService(DNSSD::RemoteService::Ptr)));
		connect(browser,SIGNAL(finished()),this,SLOT(allReported()));
		browser->startBrowse();
		enterLoop();
		return;
	case Service:
		resolveAndRedirect(url);
	  	return;
	default:
		error(ERR_MALFORMED_URL,url.prettyUrl());
		return;
	}
}
void ZeroConfProtocol::allReported()
{
	UDSEntry entry;
	listEntry(entry,true);
	finished();
	if (browser) browser->deleteLater();
	browser=0;
	if (typebrowser) typebrowser->deleteLater();
	typebrowser=0;
	mergedtypes.clear();
	emit leaveModality();
}
void ZeroConfProtocol::newType(const QString& type)
{
	if (mergedtypes.contains(type)>0) return;
	mergedtypes << type;
	if (!knownProtocols.contains(type)) return;
	UDSEntry entry;
	buildDirEntry(entry,knownProtocols[type].name,type);	
	listEntry(entry,false);
}
void ZeroConfProtocol::newService(DNSSD::RemoteService::Ptr srv)
{
	UDSEntry entry;
	entry.insert(UDSEntry::UDS_NAME,srv->serviceName());
	entry.insert(UDSEntry::UDS_ACCESS,0666);
	QString icon=KProtocolInfo::icon(knownProtocols[srv->type()].protocol);
	if (!icon.isNull())
			entry.insert(UDSEntry::UDS_ICON_NAME,icon);
	QString encname = "zeroconf:/" +srv->type()+ '/' + srv->serviceName();
	entry.insert(UDSEntry::UDS_FILE_TYPE,S_IFDIR);
	entry.insert(UDSEntry::UDS_URL,encname);
	listEntry(entry,false);
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


#include "dnssd.moc"

