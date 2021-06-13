/*
    SPDX-FileCopyrightText: 2004 Jakub Stachowski <qbast@go2.pl>

    SPDX-License-Identifier: LGPL-2.0-only
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
	explicit ServiceWatcher(const QString& type);
protected:
	QUrl constructUrl() const Q_DECL_OVERRIDE;
private:
	KDNSSD::ServiceBrowser* browser;
	QString m_type;
};

#endif
