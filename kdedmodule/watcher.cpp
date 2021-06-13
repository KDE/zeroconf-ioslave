/*
    SPDX-FileCopyrightText: 2004 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "watcher.h"

#include <KDirNotify>
#include <DNSSD/RemoteService>
#include <DNSSD/ServiceBrowser>
#include <DNSSD/ServiceTypeBrowser>
#include <QUrl>

Watcher::Watcher() 
	: refcount(1), updateNeeded(false)
{
}

ServiceWatcher::ServiceWatcher(const QString& type) : Watcher(), m_type(type)
{
	browser = new KDNSSD::ServiceBrowser(type);
	browser->setParent(this);
	connect(browser,SIGNAL(serviceAdded(KDNSSD::RemoteService::Ptr)),
		SLOT(scheduleUpdate()));
	connect(browser,SIGNAL(serviceRemoved(KDNSSD::RemoteService::Ptr)),
		SLOT(scheduleUpdate()));
	connect(browser,SIGNAL(finished()),SLOT(finished()));
	browser->startBrowse();
	
}

TypeWatcher::TypeWatcher() : Watcher()
{
	typebrowser = new KDNSSD::ServiceTypeBrowser();
	typebrowser->setParent(this);
	connect(typebrowser,SIGNAL(serviceTypeAdded(QString)),
		this,SLOT(scheduleUpdate()));
	connect(typebrowser,SIGNAL(serviceTypeRemoved(QString)),
		this,SLOT(scheduleUpdate()));
	connect(typebrowser,SIGNAL(finished()),this,SLOT(finished()));
	typebrowser->startBrowse();
}

QUrl TypeWatcher::constructUrl() const
{
    return QUrl(QStringLiteral("zeroconf:/"));
}

QUrl ServiceWatcher::constructUrl() const
{
    QUrl url(QStringLiteral("zeroconf:/"));
    url.setPath(m_type + QChar::fromLatin1('/'));
    return url;
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

