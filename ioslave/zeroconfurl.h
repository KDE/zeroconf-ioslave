/*
    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ZEROCONFURL_H
#define ZEROCONFURL_H

// KF
#include <DNSSD/RemoteService>
// Qt
#include <QUrl>


// URL zeroconf:/_http._tcp/some%20service
class ZeroConfUrl
{
  public:
    enum Type { InvalidUrl, RootDir, ServiceDir, Service };

  public:
    explicit ZeroConfUrl( const QUrl& url );

  public:
    const QString& serviceType() const;
    const QString& serviceName() const;
    const QString& domain() const;
    bool matches( const RemoteService* remoteService ) const;
    ZeroConfUrl::Type type() const;

  private:
    QString mServiceType;
    QString mServiceName;
    QString mDomain;
};


inline ZeroConfUrl::ZeroConfUrl( const QUrl& url )
{
    mServiceType = url.path().section(QChar::fromLatin1('/'),1,1);
    mServiceName = url.path().section(QChar::fromLatin1('/'),2,-1);
    mDomain = url.host();
}

inline const QString& ZeroConfUrl::serviceType() const { return mServiceType; }
inline const QString& ZeroConfUrl::serviceName() const { return mServiceName; }
inline const QString& ZeroConfUrl::domain() const { return mDomain; }

inline bool ZeroConfUrl::matches( const RemoteService* remoteService ) const
{
    return ( remoteService->serviceName()==mServiceName && remoteService->type()==mServiceType 
        && remoteService->domain()==mDomain);
}

// TODO: how is a invalid url defined?
inline ZeroConfUrl::Type ZeroConfUrl::type() const
{
    Type result =
        mServiceType.isEmpty() ? RootDir :
        mServiceName.isEmpty() ? ServiceDir :
        /*else*/                 Service;

    return result;
}

#endif
