/***************************************************************************
 *   Copyright (C) 2004 by Jakub Stachowski                                *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <qcstring.h>
#include <qsocket.h>
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
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <ksocketaddress.h>
#include <kprotocolinfo.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kurl.h>
#include <ksock.h>
#include <qmap.h>
#include <kapplication.h>
#include <qeventloop.h>
#include <krun.h>


#include "dnssd.h"

static const KCmdLineOptions options[] =
{
	{ "+protocol", I18N_NOOP( "Protocol name" ), 0 },
	{ "+pool", I18N_NOOP( "Socket name" ), 0 },
	{ "+app", I18N_NOOP( "Socket name" ), 0 },
	KCmdLineLastOption
};

using namespace KIO;


kio_dnssdProtocol::kio_dnssdProtocol(const QCString &pool_socket, const QCString &app_socket)
		: SlaveBase("dnssd", pool_socket, app_socket), browser(0),toResolve(0),
		configData(0)
{
	kdDebug() << "kio_dnssdProtocol::kio_dnssdProtocol()" << endl;
}


kio_dnssdProtocol::~kio_dnssdProtocol()
{
	kdDebug() << "kio_dnssdProtocol::~kio_dnssdProtocol()" << endl;
}


void kio_dnssdProtocol::get(const KURL& url )
{
	kdDebug() << "kio_dnssd::get(const KURL& url)" << endl ;
	UrlType t = checkURL(url);
	switch (t) {
	case HelperProtocol:
	{
		QString name,type,domain;
		dissect(url,name,type,domain);
		resolveAndRedirect(url,true);
		error(KIO::ERR_SLAVE_DEFINED,
			i18n("Requested service has been launched in separate window."));
/*		data(QByteArray());
		emit mimeType("inode/directory");
		finished();*/
		break;
	}
	case Service:
		resolveAndRedirect(url);
		break;
	case Lisa:
		redirectToLisa(url);
		kdDebug() << "Lisa requested\n";
		break;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
	}
}
void kio_dnssdProtocol::mimetype(const KURL& url )
{
	kdDebug() << "Mimetype query\n";
	if (checkURL(url)==Lisa) redirectToLisa(url);
	else resolveAndRedirect(url);
}

UrlType kio_dnssdProtocol::checkURL(const KURL& url)
{
	kdDebug() << "Checking URL " << url.url() << ", path is " << url.path() << endl;
	if (url.path()=="/") return RootDir;
	const QString& path = url.path();
	const QString& type = path.section("/",1,1);
	const QString& proto = type.section(".",1,-1);
	if (type=="computers") return Lisa;
	if (type[0]!="_" || (proto!="_udp" && proto!="_tcp")) return Invalid;
	const QString& domain = url.host();
	const QString& service = path.section("/",2,-1);
	if (service.isEmpty()) return ServiceDir;
	if (!service.isEmpty() && !domain.isEmpty()) {
		if (!setConfig(type)) return Invalid;
		return (KProtocolInfo::isHelperProtocol( configData->readEntry( "Protocol",
			type.section(".",0,0).mid(1)))) ? HelperProtocol : Service;
		}
	return Invalid;
}

// URL dnssd://domain/_http._tcp/some service
void kio_dnssdProtocol::dissect(const KURL& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);

}

void kio_dnssdProtocol::stat(const KURL& url)
{
	kdDebug() << "Stat for " << url.prettyURL() << endl;
	UDSEntry entry;
	UrlType t = checkURL(url);
	switch (t) {
	case RootDir:
	case ServiceDir:
		kdDebug() << "Reporting as dir\n";
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
	case Lisa:
		kdDebug() << "Lisa requested\n";
		redirectToLisa(url);
		break;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
	}
}


void kio_dnssdProtocol::resolveAndRedirect(const KURL& url, bool useKRun)
{
	QString name,type,domain;
	dissect(url,name,type,domain);
	kdDebug() << "Resolve for  " << name << ", " << type << ", " << domain  << "\n";
	if (toResolve!=0)
		if (toResolve->serviceName()==name && toResolve->type()==type &&
		        toResolve->domain()==domain && toResolve->isResolved()) {
			kdDebug() << "Lucky, already resolved\n";
		}  else {
			delete toResolve;
			toResolve = 0;
		}
	if (toResolve==0) {
		toResolve = new DNSSD::RemoteService(name,type,domain);
		bool res = toResolve->resolve();
		// or maybe HOST_NOT_FOUND?
		if (!res) error(ERR_SERVICE_NOT_AVAILABLE,i18n("Unable to resolve service"));
	}
	KURL destUrl;
	kdDebug() << "Resolved: " << toResolve->hostName() << "\n";
	destUrl.setProtocol(getProtocol(type));
	QString entry = configData->readEntry("UserEntry");
	if (!entry.isNull() && !toResolve->textData()[entry].isNull())
		destUrl.setUser(toResolve->textData()[entry]);
	entry = configData->readEntry("PasswordEntry");
	if (!entry.isNull() && !toResolve->textData()[entry].isNull())
		destUrl.setPass(toResolve->textData()[entry]);
	entry = configData->readEntry("PathEntry");
	if (!entry.isNull() && !toResolve->textData()[entry].isNull())
		destUrl.setPath(toResolve->textData()[entry]);
	destUrl.setHost(toResolve->hostName());
	destUrl.setPort(toResolve->port());
	// krun object will autodelete itself
	if (useKRun) KRun::run(KProtocolInfo::exec(getProtocol(type)),destUrl);
		else {
			redirection(destUrl);
			finished();
		}
}

bool kio_dnssdProtocol::setConfig(const QString& type)
{
	kdDebug() << "Setting config for " << type << endl;
	if (configData)
		if (configData->readEntry("Type")!=type) delete configData;
		else return true;
	configData = new KConfig("dnssd/"+type,false,false,"data");
	kdDebug() << "Result: " << configData->readEntry("Type") << endl;
	return (configData->readEntry("Type")==type);
}


void kio_dnssdProtocol::buildDirEntry(UDSEntry& entry,const QString& name,const QString& type)
{
	UDSAtom atom;
	entry.clear();
	atom.m_uds=UDS_NAME;
	atom.m_str=name;
	entry.append(atom);
	atom.m_uds = UDS_ACCESS;
	atom.m_long = 0555;
	entry.append(atom);
	atom.m_uds = UDS_SIZE;
	atom.m_long = 0;
	entry.append(atom);
	atom.m_uds = UDS_FILE_TYPE;
	atom.m_long = S_IFDIR;
	entry.append(atom);
	atom.m_uds = UDS_MIME_TYPE;
	atom.m_str = "inode/directory";
	entry.append(atom);
	if (!type.isNull()) {
		atom.m_uds=UDS_URL;
		atom.m_str="dnssd:/"+type+"/";
		entry.append(atom);
	}
}
QString kio_dnssdProtocol::getProtocol(const QString& type)
{
	setConfig(type);
	return configData->readEntry("Protocol",type.section(".",0,0).mid(1));
}

// URL for Lisa : dnssd://hostname/computers/path

void kio_dnssdProtocol::redirectToLisa(const KURL &url)
{
	KURL newURL;
	newURL.setProtocol("lan");
	newURL.setHost(url.host());
	newURL.setPath("/"+url.path().section("/",2,-1));
	kdDebug() << "URL rewritten for Lisa: " << newURL.prettyURL() << "\n";
	redirection(newURL);
	finished();
}


void kio_dnssdProtocol::buildServiceEntry(UDSEntry& entry,const QString& name,const QString& type,const QString& domain)
{
	UDSAtom atom;
	entry.clear();
	atom.m_uds=UDS_NAME;
	atom.m_str=name;
	entry.append(atom);
	atom.m_uds = UDS_ACCESS;
	atom.m_long = 0666;
	setConfig(type);
	entry.append(atom);
	if (!KProtocolInfo::icon(getProtocol(type)).isNull()) {
		atom.m_uds = UDS_ICON_NAME;
		atom.m_str = KProtocolInfo::icon(getProtocol(type));
		entry.append(atom);
	}
	atom.m_uds = UDS_FILE_TYPE;
	KURL protourl;
	protourl.setProtocol(getProtocol(type));
	QString encname = "dnssd://" + domain +"/" +type+ "/" + name;
	if (KProtocolInfo::supportsListing(protourl)) {
		atom.m_long=S_IFDIR;
		encname+="/";
	} else atom.m_long=S_IFREG;
	entry.append(atom);
	atom.m_uds=UDS_URL;
	atom.m_str=encname;
	kdDebug() << "Name before: " << name << ", after: " << encname << "\n";
	entry.append(atom);
}

void kio_dnssdProtocol::listDir(const KURL& url )
{
	kdDebug() << "Listing started for " << url.prettyURL() << endl;

	UrlType t  = checkURL(url);
	UDSEntry entry;
	switch (t) {
	case RootDir:
		kdDebug() << "Listing root dir\n";
		if (url.host().isEmpty()) {   // "Computers" pseudo-service only for all domains
			buildDirEntry(entry,i18n("Network Hosts"),"computers");
			listEntry(entry,false);
		}
		if (url.host().isEmpty())
			browser = new DNSSD::ServiceBrowser(DNSSD::ServiceBrowser::AllServices);
			else browser = new DNSSD::ServiceBrowser(DNSSD::ServiceBrowser::AllServices,url.host());
		connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
			this,SLOT(newType(DNSSD::RemoteService::Ptr)));
		break;
	case ServiceDir:
		if (url.host().isEmpty())
			browser = new DNSSD::ServiceBrowser(url.path(-1).section("/",1,-1));
			else browser = new DNSSD::ServiceBrowser(url.path(-1).section("/",1,-1),url.host());
		connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
			this,SLOT(newService(DNSSD::RemoteService::Ptr)));
		break;
	case Lisa:
		kdDebug() << "Lisa requested\n";
		redirectToLisa(url);
		return;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
		return;
	}
	connect(browser,SIGNAL(finished()),this,SLOT(allReported()));
	browser->startBrowse();
	kdDebug() << "entering LOOP\n";
	kapp->eventLoop()->enterLoop();
}
void kio_dnssdProtocol::allReported()
{
	UDSEntry entry;
// 	kdDebug() << "Listing finished\n";
	listEntry(entry,true);
	finished();
	delete browser;
	browser=0;
	mergedtypes.clear();
	kdDebug() << "exiting LOOP\n";
	kapp->eventLoop()->exitLoop();
}
void kio_dnssdProtocol::newType(DNSSD::RemoteService::Ptr srv)
{
	if (mergedtypes.contains(srv->type())>0) return;
	mergedtypes << srv->type();
	UDSEntry entry;
	kdDebug() << "Got new entry " << srv->type() << endl;
	QString name;
	if (!setConfig(srv->type())) return;
	name = configData->readEntry("Name");
	if (!name.isNull()) {
		buildDirEntry(entry,name,srv->type());
		listEntry(entry,false);
	}
}
void kio_dnssdProtocol::newService(DNSSD::RemoteService::Ptr srv)
{
	UDSEntry entry;
	buildServiceEntry(entry,srv->serviceName(),srv->type(),srv->domain());
	listEntry(entry,false);
}


extern "C"
{
	int kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_dnssd", 0, 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app;
		// We want to be anonymous even if we use DCOP
	//	app.dcopClient()->attach();

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		kio_dnssdProtocol slave( args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


#include "dnssd.moc"

