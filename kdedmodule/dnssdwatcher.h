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

#ifndef _DNSSDWATCHER_H_
#define _DNSSDWATCHER_H_

#include <qhash.h>
#include <kdedmodule.h>

class Watcher;
class KUrl;
class DNSSDWatcher : public KDEDModule
{
Q_OBJECT
public:
	DNSSDWatcher(QObject* parent, const QList<QVariant>&);
	~DNSSDWatcher();

public slots:
	QStringList watchedDirectories();
	void enteredDirectory(const QString& dir);
	void leftDirectory(const QString& dir);

private:
	QHash<QString, Watcher *> watchers;

	void createNotifier(const KUrl& url);
	void dissect(const KUrl& url,QString& name,QString& type);

};

#endif
