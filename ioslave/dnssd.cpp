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


ZeroConfProtocol::ZeroConfProtocol(const QCString &pool_socket, const QCString &app_socket)
		: SlaveBase("zeroconf", pool_socket, app_socket), browser(0),toResolve(0),
		configData(0)
{
	kdDebug() << "ZeroConfProtocol::ZeroConfProtocol()" << endl;
}


ZeroConfProtocol::~ZeroConfProtocol()
{
	kdDebug() << "ZeroConfProtocol::~ZeroConfProtocol()" << endl;
}


void ZeroConfProtocol::get(const KURL& url )
{
	if (!dnssdOK()) return;
	UrlType t = checkURL(url);
	switch (t) {
	case HelperProtocol:
	{
		resolveAndRedirect(url,true);
		error(KIO::ERR_SLAVE_DEFINED,
			i18n("Requested service has been launched in separate window."));
		break;
	}
	case Service:
		resolveAndRedirect(url);
		break;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
	}
}
void ZeroConfProtocol::mimetype(const KURL& url )
{
	resolveAndRedirect(url);
}

UrlType ZeroConfProtocol::checkURL(const KURL& url)
{
	if (url.path()=="/") return RootDir;
	QString service, type, domain;
	dissect(url,service,type,domain);
	const QString& proto = type.section('.',1,-1);
	if (type[0]!='_' || (proto!="_udp" && proto!="_tcp")) return Invalid;
	if (service.isEmpty()) return ServiceDir;
	if (!domain.isEmpty()) {
		if (!setConfig(type)) return Invalid;
		if (!configData->readEntry("Exec").isNull()) return HelperProtocol;
		return (KProtocolInfo::isHelperProtocol( configData->readEntry( "Protocol",
			type.section(".",0,0).mid(1)))) ? HelperProtocol : Service;
		}
	return Invalid;
}

// URL zeroconf://domain/_http._tcp/some service
void ZeroConfProtocol::dissect(const KURL& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);

}

bool ZeroConfProtocol::dnssdOK()
{
	switch(DNSSD::ServiceBrowser::isAvailable()) {	    
        	case DNSSD::ServiceBrowser::Stopped:
			error(KIO::ERR_UNSUPPORTED_ACTION,
			i18n("<p>The Zeroconf daemon (mdnsd) is not running. </p>"));
			return false;
		case DNSSD::ServiceBrowser::Unsupported:
	    		error(KIO::ERR_UNSUPPORTED_ACTION,
			i18n("<p>KDE has been built without Zeroconf support.</p>"));
			return false;
          default:
			return true;
        }
}

void ZeroConfProtocol::stat(const KURL& url)
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
	QString entry = configData->readEntry(name);
	return (entry.isNull()) ? QString::null : toResolve->textData()[entry];
}

void ZeroConfProtocol::resolveAndRedirect(const KURL& url, bool useKRun)
{
	QString name,type,domain;
	dissect(url,name,type,domain);
	kdDebug() << "Resolve for  " << name << ", " << type << ", " << domain  << "\n";
	if (toResolve!=0)
		if (toResolve->serviceName()==name && toResolve->type()==type &&
		        toResolve->domain()==domain && toResolve->isResolved()) {
		}  else {
			delete toResolve;
			toResolve = 0;
		}
	if (toResolve==0) {
		toResolve = new DNSSD::RemoteService(name,type,domain);
		// or maybe HOST_NOT_FOUND?
		if (!toResolve->resolve()) error(ERR_SERVICE_NOT_AVAILABLE,i18n("Unable to resolve service"));
	}
	KURL destUrl;
	kdDebug() << "Resolved: " << toResolve->hostName() << "\n";
	destUrl.setProtocol(getProtocol(type));
	destUrl.setUser(getAttribute("UserEntry"));
	destUrl.setPass(getAttribute("PasswordEntry"));
	destUrl.setPath(getAttribute("PathEntry"));
	destUrl.setHost(toResolve->hostName());
	destUrl.setPort(toResolve->port());
	// get exec from config or try getting it from helper protocol
	if (useKRun) KRun::run(configData->readEntry("Exec",KProtocolInfo::exec(getProtocol(type))),destUrl);
	else {
		redirection(destUrl);
		finished();
	}
}

bool ZeroConfProtocol::setConfig(const QString& type)
{
	kdDebug() << "Setting config for " << type << endl;
	if (configData)
		if (configData->readEntry("Type")!=type) delete configData;
		else return true;
	configData = new KConfig("zeroconf/"+type,false,false,"data");
	return (configData->readEntry("Type")==type);
}

inline void buildAtom(UDSEntry& entry,UDSAtomTypes type, const QString& data)
{
	UDSAtom atom;
	atom.m_uds=type;
	atom.m_str=data;
	entry.append(atom);
}
inline void buildAtom(UDSEntry& entry,UDSAtomTypes type, long data)
{
	UDSAtom atom;
	atom.m_uds=type;
	atom.m_long=data;
	entry.append(atom);
}


void ZeroConfProtocol::buildDirEntry(UDSEntry& entry,const QString& name,const QString& type)
{
	entry.clear();
	buildAtom(entry,UDS_NAME,name);
	buildAtom(entry,UDS_ACCESS,0555);
	buildAtom(entry,UDS_SIZE,0);
	buildAtom(entry,UDS_FILE_TYPE,S_IFDIR);
	buildAtom(entry,UDS_MIME_TYPE,"inode/directory");
	if (!type.isNull()) buildAtom(entry,UDS_URL,"zeroconf:/"+type+"/");
}
QString ZeroConfProtocol::getProtocol(const QString& type)
{
	setConfig(type);
	return configData->readEntry("Protocol",type.section(".",0,0).mid(1));
}

void ZeroConfProtocol::buildServiceEntry(UDSEntry& entry,const QString& name,const QString& type,const QString& domain)
{
	setConfig(type);
	entry.clear();
	buildAtom(entry,UDS_NAME,name);
	buildAtom(entry,UDS_ACCESS,0666);
	QString icon=configData->readEntry("Icon",KProtocolInfo::icon(getProtocol(type)));
	if (!icon.isNull()) buildAtom(entry,UDS_ICON_NAME,icon);
	KURL protourl;
	protourl.setProtocol(getProtocol(type));
	QString encname = "zeroconf://" + domain +"/" +type+ "/" + name;
	if (KProtocolInfo::supportsListing(protourl)) {
		buildAtom(entry,UDS_FILE_TYPE,S_IFDIR);
		encname+="/";
	} else buildAtom(entry,UDS_FILE_TYPE,S_IFREG);
	buildAtom(entry,UDS_URL,encname);
}

void ZeroConfProtocol::listDir(const KURL& url )
{

	if (!dnssdOK()) return;
	UrlType t  = checkURL(url);
	UDSEntry entry;
	switch (t) {
	case RootDir:
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
	case Service:
		resolveAndRedirect(url);
	  	return;
	default:
		error(ERR_MALFORMED_URL,i18n("invalid URL"));
		return;
	}
	connect(browser,SIGNAL(finished()),this,SLOT(allReported()));
	browser->startBrowse();
	kapp->eventLoop()->enterLoop();
}
void ZeroConfProtocol::allReported()
{
	UDSEntry entry;
	listEntry(entry,true);
	finished();
	delete browser;
	browser=0;
	mergedtypes.clear();
	kapp->eventLoop()->exitLoop();
}
void ZeroConfProtocol::newType(DNSSD::RemoteService::Ptr srv)
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
void ZeroConfProtocol::newService(DNSSD::RemoteService::Ptr srv)
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
		KCmdLineArgs::init(argc, argv, "kio_zeroconf", 0, 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication::disableAutoDcopRegistration();
		KApplication app;
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		ZeroConfProtocol slave( args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


#include "dnssd.moc"

