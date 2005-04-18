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

#ifndef _WATCHER_H_
#define _WATCHER_H_

#include <qstring.h>
#include <dnssd/servicebrowser.h>
#include <dnssd/remoteservice.h>

using namespace DNSSD;

class Watcher : public QObject
{
Q_OBJECT
public:
	Watcher(const QString& type, const QString& domain);
	~Watcher();
		
	unsigned int refcount;
private:
	ServiceBrowser* browser;
	bool updateNeeded;
	QString m_type;
	QString m_domain;
	QValueList<DNSSD::RemoteService::Ptr> removed;
	
private slots:
	void serviceRemoved(DNSSD::RemoteService::Ptr srv);
	void serviceAdded(DNSSD::RemoteService::Ptr);
	void finished();
	
};

#endif
