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

#ifndef _WATCHER_H_
#define _WATCHER_H_

#include <QObject>

class QString;

namespace KDNSSD {
    class ServiceBrowser;
    class ServiceTypeBrowser;
}

class Watcher : public QObject
{
Q_OBJECT
public:
	Watcher();
		
	unsigned int refcount;
protected:
	virtual QUrl constructUrl() const = 0;
private:
	bool updateNeeded;
	
private Q_SLOTS:
	void scheduleUpdate();
	void finished();
};

class TypeWatcher : public Watcher
{
Q_OBJECT
public:
	TypeWatcher();
protected:
	QUrl constructUrl() const Q_DECL_OVERRIDE;
private:
	KDNSSD::ServiceTypeBrowser* typebrowser;
};

class ServiceWatcher : public Watcher
{
Q_OBJECT
public:
	ServiceWatcher(const QString& type);
protected:
	QUrl constructUrl() const Q_DECL_OVERRIDE;
private:
	KDNSSD::ServiceBrowser* browser;
	QString m_type;
};

#endif
