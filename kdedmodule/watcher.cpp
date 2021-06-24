/*
    SPDX-FileCopyrightText: 2004 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "watcher.h"

#include <KDirNotify>
#include <kdnssd_version.h>
#if KDNSSD_VERSION >= QT_VERSION_CHECK(5, 84, 0)
#include <KDNSSD/RemoteService>
#include <KDNSSD/ServiceBrowser>
#include <KDNSSD/ServiceTypeBrowser>
#else
#include <DNSSD/RemoteService>
#include <DNSSD/ServiceBrowser>
#include <DNSSD/ServiceTypeBrowser>
#endif
#include <QUrl>

Watcher::Watcher() 
	: refcount(1), updateNeeded(false)
{
}

ServiceWatcher::ServiceWatcher(const QString& type) : Watcher(), m_type(type)
{
	browser = new KDNSSD::ServiceBrowser(type);
	browser->setParent(this);
	connect(browser, &KDNSSD::ServiceBrowser::serviceAdded,
		this, &ServiceWatcher::scheduleUpdate);
	connect(browser, &KDNSSD::ServiceBrowser::serviceRemoved,
		this, &ServiceWatcher::scheduleUpdate);
	connect(browser, &KDNSSD::ServiceBrowser::finished,
		this, &ServiceWatcher::finished);
	browser->startBrowse();
	
}

TypeWatcher::TypeWatcher() : Watcher()
{
	typebrowser = new KDNSSD::ServiceTypeBrowser();
	typebrowser->setParent(this);
	connect(typebrowser, &KDNSSD::ServiceTypeBrowser::serviceTypeAdded,
		this, &TypeWatcher::scheduleUpdate);
	connect(typebrowser, &KDNSSD::ServiceTypeBrowser::serviceTypeRemoved,
		this, &TypeWatcher::scheduleUpdate);
	connect(typebrowser, &KDNSSD::ServiceTypeBrowser::finished, this, &TypeWatcher::finished);
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

