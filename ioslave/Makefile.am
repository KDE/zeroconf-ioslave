INCLUDES = $(all_includes)

kde_module_LTLIBRARIES = kio_zeroconf.la

kio_zeroconf_la_SOURCES = dnssd.cpp
kio_zeroconf_la_LIBADD = $(LIB_KDNSSD) $(LIB_KIO)
kio_zeroconf_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = zeroconf.protocol invitation.protocol
protocoldir = $(kde_servicesdir)

services_DATA = _http._tcp _ftp._tcp _ldap._tcp _webdav._tcp _nfs._tcp _ssh._tcp
servicesdir = $(kde_datadir)/zeroconf

remote_DATA = zeroconf.desktop
remotedir = $(kde_datadir)/remoteview

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_zeroconf.pot

METASOURCES = AUTO
