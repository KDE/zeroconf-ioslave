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

#include <qdatetime.h>
#include <qbitarray.h>

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kurl.h>
#include <qmap.h>
#include <kapplication.h>
#include <qeventloop.h>
#include <dnssd/domainbrowser.h>
#include <krun.h>
#include <kprotocolmanager.h>

#include "dnssd.h"

ZeroConfProtocol::ZeroConfProtocol(const QByteArray& protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
		: SlaveBase(protocol, pool_socket, app_socket), browser(0),toResolve(0),
		configData(0)
{}

ZeroConfProtocol::~ZeroConfProtocol()
{
  delete configData;
}

void ZeroConfProtocol::get(const KUrl& url )
{
	if (!dnssdOK()) return;
	UrlType t = checkURL(url);
	switch (t) {
	case HelperProtocol:
	{
		resolveAndRedirect(url,true);
		mimeType("text/html");
		QString reply= "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n";
		reply+="</head>\n<body>\n<h2>"+i18n("Requested service has been launched in separate window.");
		reply+="</h2>\n</body></html>";
		data(reply.toUtf8());
		data(QByteArray());
		finished();
		break;
	}
	case Service:
		resolveAndRedirect(url);
		break;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
	}
}
void ZeroConfProtocol::mimetype(const KUrl& url )
{
	resolveAndRedirect(url);
}

UrlType ZeroConfProtocol::checkURL(const KUrl& url)
{
	if (url.path()=="/") return RootDir;
	QString service, type, domain;
	dissect(url,service,type,domain);
	const QString& proto = type.section('.',1,-1);
	if (type[0]!='_' || (proto!="_udp" && proto!="_tcp")) return Invalid;
	if (service.isEmpty()) return ServiceDir;
	if (!domain.isEmpty()) {
		if (!setConfig(type)) return Invalid;
		if (!configData->group("").readEntry("Exec").isNull()) return HelperProtocol;
		return (KProtocolInfo::isHelperProtocol( configData->group("").readEntry( "Protocol",
			type.section(".",0,0).mid(1)))) ? HelperProtocol : Service;
		}
	return Invalid;
}

// URL zeroconf://domain/_http._tcp/some%20service
// URL invitation://host:port/_http._tcp/some%20service?u=username&root=directory
void ZeroConfProtocol::dissect(const KUrl& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);

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
		buildDirEntry(entry,"");
		statEntry(entry);
		finished();
		break;
	case Service:
		resolveAndRedirect(url);
	  	break;
	case HelperProtocol:
	{
		QString name,type,domain;
		dissect(url,name,type,domain);
		buildServiceEntry(entry,name,type,domain);
		statEntry(entry);
		finished();
		break;
	}
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
	}
}
QString ZeroConfProtocol::getAttribute(const QString& name)
{
	QString entry = configData->group("").readEntry(name,QString());
	return (entry.isNull()) ? QString() : toResolve->textData()[entry];
}

void ZeroConfProtocol::resolveAndRedirect(const KUrl& url, bool useKRun)
{
	QString name,type,domain;
	dissect(url,name,type,domain);
	kDebug() << "Resolve for  " << name << ", " << type << ", " << domain  << "\n";
	if (toResolve!=0)
		if (toResolve->serviceName()==name && toResolve->type()==type &&
		        toResolve->domain()==domain && toResolve->isResolved()) {
		}  else {
			delete toResolve;
			toResolve = 0;
		}
	if (toResolve==0) {
		toResolve = new RemoteService(name,type,domain);
		// or maybe HOST_NOT_FOUND?
		if (!toResolve->resolve()) error(ERR_SERVICE_NOT_AVAILABLE,i18n("Unable to resolve service"));
	}
	KUrl destUrl;
	kDebug() << "Resolved: " << toResolve->hostName() << "\n";
	destUrl.setProtocol(getProtocol(type));
	destUrl.setUser(getAttribute("UserEntry"));
	destUrl.setPass(getAttribute("PasswordEntry"));
	destUrl.setPath(getAttribute("PathEntry"));
	destUrl.setHost(toResolve->hostName());
	destUrl.setPort(toResolve->port());
	// get exec from config or try getting it from helper protocol
	if (useKRun)
            KRun::run(configData->group("").readEntry("Exec",
                    KProtocolInfo::exec(getProtocol(type))),
                    destUrl, 0);
	else {
		redirection(destUrl);
		finished();
	}
}

bool ZeroConfProtocol::setConfig(const QString& type)
{
	kDebug() << "Setting config for " << type;
	if (configData)
		if (configData->group("").readEntry("Type")!=type)
		{	
			delete configData;
			configData =0L;
		}
		else 
			return true;
	configData = new KConfig("zeroconf/"+type, KConfig::NoGlobals );
         
	return (configData->group("").readEntry("Type")==type);
}


void ZeroConfProtocol::buildDirEntry(UDSEntry& entry,const QString& name,const QString& type, const QString& host)
{
	entry.clear();
	entry.insert(UDSEntry::UDS_NAME,name);
	entry.insert(UDSEntry::UDS_ACCESS,0555);
	entry.insert(UDSEntry::UDS_SIZE,0);
	entry.insert(UDSEntry::UDS_FILE_TYPE,S_IFDIR);
	entry.insert(UDSEntry::UDS_MIME_TYPE,QString::fromUtf8("inode/directory"));
	if (!type.isNull())
			entry.insert(UDSEntry::UDS_URL,"zeroconf:/"+((!host.isNull()) ? '/'+host+'/' : "" )+type+'/');
}
QString ZeroConfProtocol::getProtocol(const QString& type)
{
	setConfig(type);
	return configData->group("").readEntry("Protocol",type.section(".",0,0).mid(1));
}

void ZeroConfProtocol::buildServiceEntry(UDSEntry& entry,const QString& name,const QString& type,const QString& domain)
{
	setConfig(type);
	entry.clear();
	entry.insert(UDSEntry::UDS_NAME,name);
	entry.insert(UDSEntry::UDS_ACCESS,0666);
	QString icon=configData->group("").readEntry("Icon",KProtocolInfo::icon(getProtocol(type)));
	if (!icon.isNull())
			entry.insert(UDSEntry::UDS_ICON_NAME,icon);
	KUrl protourl;
	protourl.setProtocol(getProtocol(type));
	QString encname = "zeroconf://" + domain +"/" +type+ "/" + name;
	if (KProtocolManager::supportsListing(protourl)) {
		entry.insert(UDSEntry::UDS_FILE_TYPE,S_IFDIR);
		encname+='/';
	} else
			entry.insert(UDSEntry::UDS_FILE_TYPE,S_IFREG);
	entry.insert(UDSEntry::UDS_URL,encname);
}

void ZeroConfProtocol::listDir(const KUrl& url )
{

	if (!dnssdOK()) return;
	UrlType t  = checkURL(url);
	UDSEntry entry;
	currentDomain=url.host();
	switch (t) {
	case RootDir:
		if (currentDomain.isEmpty())
			typebrowser = new ServiceTypeBrowser();
			else typebrowser = new ServiceTypeBrowser(url.host());
		connect(typebrowser,SIGNAL(serviceTypeAdded(const QString&)),
			this,SLOT(newType(const QString&)));
		connect(typebrowser,SIGNAL(finished()),this,SLOT(allReported()));
		typebrowser->startBrowse();
		enterLoop();
		return;
	case ServiceDir:
		if (url.host().isEmpty())
			browser = new ServiceBrowser(url.path(KUrl::RemoveTrailingSlash).section("/",1,-1));
			else browser = new ServiceBrowser(url.path(KUrl::RemoveTrailingSlash).section("/",1,-1),false,url.host());
		connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
			this,SLOT(newService(DNSSD::RemoteService::Ptr)));
		break;
	case Service:
		resolveAndRedirect(url);
	  	return;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
		return;
	}
	connect(browser,SIGNAL(finished()),this,SLOT(allReported()));
	browser->startBrowse();
	enterLoop();
}
void ZeroConfProtocol::allReported()
{
	UDSEntry entry;
	listEntry(entry,true);
	finished();
	delete browser;
	browser=0;
	delete typebrowser;
	typebrowser=0;
	mergedtypes.clear();
	emit leaveModality();
}
void ZeroConfProtocol::newType(const QString& type)
{
	if (mergedtypes.contains(type)>0) return;
	mergedtypes << type;
	UDSEntry entry;
	kDebug() << "Got new entry " << type;
	if (!setConfig(type)) return;
	QString name = configData->group("").readEntry("Name");
	if (!name.isNull()) {
		buildDirEntry(entry,name,type, (currentDomain.isEmpty()) ? QString::null : currentDomain);	//krazy:exclude=nullstrassign for old broken gcc
		listEntry(entry,false);
	}
}
void ZeroConfProtocol::newService(DNSSD::RemoteService::Ptr srv)
{
	UDSEntry entry;
	buildServiceEntry(entry,srv->serviceName(),srv->type(),srv->domain());
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
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_zeroconf", 0, KLocalizedString(), 0, KLocalizedString());

		KCmdLineOptions options;
		options.add("+protocol", ki18n( "Protocol name" ));
		options.add("+pool", ki18n( "Socket name" ));
		options.add("+app", ki18n( "Socket name" ));
		KCmdLineArgs::addCmdLineOptions( options );
		//KApplication::disableAutoDcopRegistration();
		KApplication app;
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		ZeroConfProtocol slave( args->arg(0).toLocal8Bit(), args->arg(1).toLocal8Bit(), args->arg(2).toLocal8Bit() );
		args->clear();
		slave.dispatchLoop();
		return 0;
	}
}


#include "dnssd.moc"

