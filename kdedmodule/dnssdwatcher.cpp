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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dnssdwatcher.h"

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <dnssd/servicebrowser.h>
#include "watcher.h"
#include <QtDBus>

DNSSDWatcher::DNSSDWatcher()
	: KDEDModule()
{
	QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
                                   "enteredDirectory", this, SLOT(enteredDirectory(QString))); QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
                                   "leftDirectory", this, SLOT(leftDirectory(QString)));
	watchers.setAutoDelete(true);
}

QStringList DNSSDWatcher::watchedDirectories()
{
//TODO
//	return watchers.keys();
	QStringList keys;
	for (Q3DictIterator<Watcher> it(watchers) ; it.current(); ++it ) {
		keys << it.currentKey();
		kDebug() << it.currentKey() << " " << (*it)->refcount << "\n";
		}
return keys;
}


// from ioslave
void DNSSDWatcher::dissect(const KUrl& url,QString& name,QString& type,QString& domain)
{
	type = url.path().section("/",1,1);
	domain = url.host();
	name = url.path().section("/",2,-1);
}



void DNSSDWatcher::enteredDirectory(const QString& _dir)
{
	KUrl dir(_dir);
	if (dir.protocol()!="zeroconf") return;
	if (watchers[dir.url()]) watchers[dir.url()]->refcount++;
		else createNotifier(dir);
}


void DNSSDWatcher::leftDirectory(const QString& _dir)
{
	KUrl dir(_dir);
	if (dir.protocol()!="zeroconf") return;
	if (!watchers[dir.url()]) return;
	if ((watchers[dir.url()])->refcount==1) watchers.remove(dir.url());
		else watchers[dir.url()]->refcount--;
}


void DNSSDWatcher::createNotifier(const KUrl& url)
{
	QString domain,type,name;
	dissect(url,name,type,domain);
	Watcher *w = new Watcher(type,domain);
	watchers.insert(url.url(),w);
}

extern "C" {
	KDE_EXPORT KDEDModule *create_dnssdwatcher()
	{
		KGlobal::locale()->insertCatalog("dnssdwatcher");
		return new DNSSDWatcher();
	}
}

#include "dnssdwatcher.moc"
