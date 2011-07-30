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

#include "watcher.h"

#include <kdirnotify.h>
#include <dnssd/remoteservice.h>
#include <dnssd/servicebrowser.h>
#include <dnssd/servicetypebrowser.h>

Watcher::Watcher() 
	: refcount(1), updateNeeded(false)
{
}

ServiceWatcher::ServiceWatcher(const QString& type) : Watcher(), m_type(type)
{
	browser = new DNSSD::ServiceBrowser(type);
	browser->setParent(this);
	connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
		SLOT(scheduleUpdate()));
	connect(browser,SIGNAL(serviceRemoved(DNSSD::RemoteService::Ptr)),
		SLOT(scheduleUpdate()));
	connect(browser,SIGNAL(finished()),SLOT(finished()));
	browser->startBrowse();
	
}

TypeWatcher::TypeWatcher() : Watcher()
{
	typebrowser = new DNSSD::ServiceTypeBrowser();
	typebrowser->setParent(this);
	connect(typebrowser,SIGNAL(serviceTypeAdded(QString)),
		this,SLOT(scheduleUpdate()));
	connect(typebrowser,SIGNAL(serviceTypeRemoved(QString)),
		this,SLOT(scheduleUpdate()));
	connect(typebrowser,SIGNAL(finished()),this,SLOT(finished()));
	typebrowser->startBrowse();
}

QString TypeWatcher::constructUrl() 
{
    return QString("zeroconf:/");
}

QString ServiceWatcher::constructUrl()
{
    return QString("zeroconf:/")+m_type+'/';
}

void Watcher::scheduleUpdate()
{
	updateNeeded=true;
}

void Watcher::finished() 
{
	if (updateNeeded) org::kde::KDirNotify::emitFilesAdded( constructUrl() );
	updateNeeded=false;
}

#include "watcher.moc"
