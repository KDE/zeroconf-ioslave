INCLUDES = $(all_includes)

kde_module_LTLIBRARIES = kio_dnssd.la

kio_dnssd_la_SOURCES = dnssd.cpp
kio_dnssd_la_LIBADD = $(LIB_KDNSSD) $(LIB_KIO)
kio_dnssd_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = dnssd.protocol
protocoldir = $(kde_servicesdir)

services_DATA = _http._tcp _ftp._tcp _rfb._tcp _ldap._tcp _webdav._tcp _nfs._tcp
servicesdir = $(kde_datadir)/dnssd

remote_DATA = dnssd.desktop
remotedir = $(kde_datadir)/remoteview

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_dnssd.pot

METASOURCES = AUTO
