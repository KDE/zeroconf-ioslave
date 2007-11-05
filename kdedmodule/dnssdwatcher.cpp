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
#include "kdnssdadaptor.h"

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <dnssd/servicebrowser.h>
#include "watcher.h"
#include <QtDBus>

#include <kpluginfactory.h>
#include <kpluginloader.h>

K_PLUGIN_FACTORY(DNSSDWatcherFactory,
                 registerPlugin<DNSSDWatcher>();
    )
K_EXPORT_PLUGIN(DNSSDWatcherFactory("dnssdwatcher"))

DNSSDWatcher::DNSSDWatcher(QObject* parent, const QList<QVariant>&)
	: KDEDModule(parent)
{
	QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
                                   "enteredDirectory", this, SLOT(enteredDirectory(QString)));
	QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
                                   "leftDirectory", this, SLOT(leftDirectory(QString)));
	new KdnssdAdaptor( this );
}

QStringList DNSSDWatcher::watchedDirectories()
{
	return watchers.keys();
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
	if (watchers.contains(dir.url()))
            watchers[dir.url()]->refcount++;
        else
            createNotifier(dir);
}


void DNSSDWatcher::leftDirectory(const QString& _dir)
{
	KUrl dir(_dir);
	if (dir.protocol()!="zeroconf") return;
        Watcher* watcher = watchers.value(dir.url());
	if (!watcher) return;
	if (watcher->refcount==1) {
            delete watcher;
            watchers.remove(dir.url());
        } else {
            watcher->refcount--;
        }
}


void DNSSDWatcher::createNotifier(const KUrl& url)
{
	QString domain,type,name;
	dissect(url,name,type,domain);
	Watcher *w = new Watcher(type,domain);
	watchers.insert(url.url(),w);
}

DNSSDWatcher::~DNSSDWatcher()
{
    qDeleteAll( watchers );
}

#include "dnssdwatcher.moc"
