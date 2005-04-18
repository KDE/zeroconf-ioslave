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

#include "watcher.h"

#include <kdebug.h>

#include <kdirnotify_stub.h>
#include <qstringlist.h>



Watcher::Watcher(const QString& type, const QString& domain) 
	: refcount(1), updateNeeded(false), m_type(type), m_domain(domain)
{
	if (domain.isEmpty()) browser = new ServiceBrowser(type);
		else browser = new ServiceBrowser(type,domain);
	connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),
		SLOT(serviceAdded(DNSSD::RemoteService::Ptr)));
	connect(browser,SIGNAL(serviceRemoved(DNSSD::RemoteService::Ptr)),
		SLOT(serviceRemoved(DNSSD::RemoteService::Ptr)));
	connect(browser,SIGNAL(finished()),SLOT(finished()));
	browser->startBrowse();
}

Watcher::~Watcher()
{
	delete browser;
}

void Watcher::serviceAdded(DNSSD::RemoteService::Ptr)
{
	updateNeeded=true;
}

void Watcher::serviceRemoved(DNSSD::RemoteService::Ptr srv)
{
	if (!updateNeeded) removed << srv;
}


void Watcher::finished() 
{
	KDirNotify_stub st("*","*");
	kdDebug() << "Finished for " << m_type << "@" << m_domain << "\n";
	if (updateNeeded || removed.count()) {
		QString url = "zeroconf:/";
		if (!m_domain.isEmpty()) url+="/"+m_domain+"/";
		if (m_type!=ServiceBrowser::AllServices) url+=m_type;
		kdDebug() << "Sending update: " << url << "\n";
		st.FilesAdded(url);
		}
	removed.clear();
	updateNeeded=false;
}

#include "watcher.moc"
