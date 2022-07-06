/*
    SPDX-FileCopyrightText: 2004 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dnssdwatcher.h"
#include "kdnssdadaptor.h"
#include "watcher.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(DNSSDWatcher, "dnssdwatcher.json")

DNSSDWatcher::DNSSDWatcher(QObject* parent, const QList<QVariant>&)
: KDEDModule(parent)
{
    QDBusConnection::sessionBus().connect(QString(), QString(),
            QStringLiteral("org.kde.KDirNotify"),
            QStringLiteral("enteredDirectory"),
            this, SLOT(enteredDirectory(QString)));
    QDBusConnection::sessionBus().connect(QString(), QString(),
            QStringLiteral("org.kde.KDirNotify"),
            QStringLiteral("leftDirectory"),
            this, SLOT(leftDirectory(QString)));
    new KdnssdAdaptor( this );
}

QStringList DNSSDWatcher::watchedDirectories()
{
    return watchers.keys();
}


// from KIO worker
void DNSSDWatcher::dissect(const QUrl& url,QString& name,QString& type)
{
    type = url.path().section(QChar::fromLatin1('/'),1,1);
    name = url.path().section(QChar::fromLatin1('/'),2,-1);
}



void DNSSDWatcher::enteredDirectory(const QString& _dir)
{
    QUrl dir(_dir);
    if (dir.scheme() != QLatin1String("zeroconf")) {
        return;
    }
    if (watchers.contains(dir.url())) {
        watchers[dir.url()]->refcount++;
    }
    else {
        createNotifier(dir);
    }
}


void DNSSDWatcher::leftDirectory(const QString& _dir)
{
    QUrl dir(_dir);
    if (dir.scheme() != QLatin1String("zeroconf")) {
        return;
    }
    Watcher* watcher = watchers.value(dir.url());
    if (!watcher) {
        return;
    }
    if (watcher->refcount==1) {
        delete watcher;
        watchers.remove(dir.url());
    } else {
        watcher->refcount--;
    }
}


void DNSSDWatcher::createNotifier(const QUrl& url)
{
    QString type,name;
    dissect(url,name,type);
    if (type.isEmpty()) {
        watchers.insert(url.url(), new TypeWatcher());
    }
    else {
        watchers.insert(url.url(), new ServiceWatcher(type));
    }
}

DNSSDWatcher::~DNSSDWatcher()
{
    qDeleteAll( watchers );
}

#include <dnssdwatcher.moc>
