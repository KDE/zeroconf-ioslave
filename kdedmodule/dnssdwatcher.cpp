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
#include <dnssd/servicebrowser.h>
#include <kdirnotify_stub.h>
#include "watcher.h"


DNSSDWatcher::DNSSDWatcher(const QCString& obj)
	: KDEDModule(obj)
{
	connectDCOPSignal("","KDirNotify","enteredDirectory(KURL)","enteredDirectory(KURL)",false);
	connectDCOPSignal("","KDirNotify","leftDirectory(KURL)","leftDirectory(KURL)",false);
	watchers.setAutoDelete(true);
}

QStringList DNSSDWatcher::watchedDirectories()
{
//TODO
//	return watchers.keys();
	QStringList keys;
	for (QDictIterator<Watcher> it(watchers) ; it.current(); ++it ) {
		keys << it.currentKey();
		kdDebug() << it.currentKey() << " " << (*it)->refcount << "\n";
		}
return keys;
}


// from ioslave
void DNSSDWatcher::dissect(const KURL& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);
}



void DNSSDWatcher::enteredDirectory(const KURL& dir)
{
	if (dir.protocol()!="zeroconf") return;
	if (watchers[dir.url()]) watchers[dir.url()]->refcount++;
		else createNotifier(dir);
}


void DNSSDWatcher::leftDirectory(const KURL& dir)
{
	if (dir.protocol()!="zeroconf") return;
	if (!watchers[dir.url()]) return;
	if ((watchers[dir.url()])->refcount==1) watchers.remove(dir.url());
		else watchers[dir.url()]->refcount--;
}


void DNSSDWatcher::createNotifier(const KURL& url) 
{
	QString domain,type,name;
	dissect(url,name,type,domain);	
	if (type.isEmpty()) type = DNSSD::ServiceBrowser::AllServices;
	Watcher *w = new Watcher(type,domain);
	watchers.insert(url.url(),w);
}

extern "C" {
	KDE_EXPORT KDEDModule *create_dnssdwatcher(const QCString &obj)
	{
		KGlobal::locale()->insertCatalogue("dnssdwatcher");
		return new DNSSDWatcher(obj);
	}
}

#include "dnssdwatcher.moc"
