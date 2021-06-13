/*
    SPDX-FileCopyrightText: 2004 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef _DNSSDWATCHER_H_
#define _DNSSDWATCHER_H_

#include <QHash>
#include <QStringList>
#include <KDEDModule>

class Watcher;
class QUrl;
class DNSSDWatcher : public KDEDModule
{
Q_OBJECT
public:
	DNSSDWatcher(QObject* parent, const QList<QVariant>&);
	~DNSSDWatcher() override;

public Q_SLOTS:
	QStringList watchedDirectories();
	void enteredDirectory(const QString& dir);
	void leftDirectory(const QString& dir);

private:
	QHash<QString, Watcher *> watchers;

	void createNotifier(const QUrl& url);
	void dissect(const QUrl& url,QString& name,QString& type);

};

#endif
