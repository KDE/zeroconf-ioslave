/* This file is part of the KDE Project
   Copyright (c) 2004 Jakub Stachowski <qbast@go2.pl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "dnssdwatcher.h"

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <kdirnotify_stub.h>


DNSSDWatcher::DNSSDWatcher(const QCString& obj)
	: KDEDModule(obj)
{
	connectDCOPSignal("","KDirNotify","enteredDirectory(KURL)","enteredDirectory(KURL)",false);
	connectDCOPSignal("","KDirNotify","leftDirectory(KURL)","leftDirectory(KURL)",false);
	browsers.setAutoDelete(true);
}

QStringList DNSSDWatcher::watchedDirectories()
{
	return references.keys();
}


void DNSSDWatcher::enteredDirectory(const KURL& dir)
{
	if (dir.protocol()!="dnssd") return;
	kdDebug() << "entereed: " << dir.prettyURL() << "\n";
	if (references.contains(dir.url())) references[dir.url()]++;
		else createNotifier(dir);
//	kdDebug() << dir.url() << ": count " << references[dir.url()] << "\n";
}


void DNSSDWatcher::leftDirectory(const KURL& dir)
{
	if (dir.protocol()!="dnssd") return;
	kdDebug() << "left:  " << dir.prettyURL() << ", referenced:" << references.contains(dir.url()) << endl;
	if (!references.contains(dir.url())) return;
	kdDebug() << "current refcount: " << references[dir.url()] << endl;
	if ((references[dir.url()])==1) { 
			references.remove(dir.url());
			browsers.remove(dir.url());
			kdDebug() << dir.url() << " is no longer monitored\n";
			}
		else references[dir.url()]--;
//dDebug() << dir.url() << ": count " << references[dir.url()] << "\n";
}


// FIXME: send whole batches
// FIXME: handle service type removal
void DNSSDWatcher::serviceAdded(DNSSD::RemoteService::Ptr srv)
{
	KDirNotify_stub st("*","*");
	if (srv->serviceName().isEmpty()) { // metaservice
		kdDebug() << "Kicking dnssd:/ and dnssd://" << srv->domain() << "\n";
		if (references.contains("dnssd:/")) st.FilesAdded("dnssd:/");
		if (references.contains("dnssd://"+srv->domain())) st.FilesAdded("dnssd://"+srv->domain());
	} else {
		kdDebug() << "Kicking dnssd:/" << srv->type() << " and dnssd://" << srv->domain() 
			<< "/" << srv->type() << "\n";
		if (references.contains("dnssd:/"+srv->type())) st.FilesAdded("dnssd:/"+srv->type());
		if (references.contains("dnssd://"+srv->domain()+"/"+srv->type()))
			st.FilesAdded("dnssd://"+srv->domain()+"/"+srv->type());
	}
}

// FIXME: use FilesRemoved
void DNSSDWatcher::serviceRemoved(DNSSD::RemoteService::Ptr srv)
{
	serviceAdded(srv);
}

void DNSSDWatcher::createNotifier(const KURL& url) 
{
	QString domain,type,name;
	dissect(url,name,type,domain);	
	if (!name.isEmpty() || type=="computers") return; 
	if (type.isEmpty()) type = DNSSD::ServiceBrowser::AllServices;
	DNSSD::ServiceBrowser *b;
	if (domain.isEmpty()) b = new DNSSD::ServiceBrowser(type);
		else b = new DNSSD::ServiceBrowser(type,domain);
	connect(b,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),SLOT(serviceAdded(DNSSD::RemoteService::Ptr)));
	connect(b,SIGNAL(serviceRemoved(DNSSD::RemoteService::Ptr)),SLOT(serviceRemoved(DNSSD::RemoteService::Ptr)));
	browsers.insert(url.url(),b);
	b->startBrowse();
	references[url.url()]=1;
}


// from ioslave
void DNSSDWatcher::dissect(const KURL& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);
}


extern "C" {
	KDE_EXPORT KDEDModule *create_dnssdwatcher(const QCString &obj)
	{
		KGlobal::locale()->insertCatalogue("dnssdwatcher");
		return new DNSSDWatcher(obj);
	}
}

#include "dnssdwatcher.moc"
