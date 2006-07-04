



########### next target ###############

set(kio_zeroconf_PART_SRCS dnssd.cpp )

kde4_automoc(${kio_zeroconf_PART_SRCS})

kde4_add_plugin(kio_zeroconf ${kio_zeroconf_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_zeroconf )

target_link_libraries(kio_zeroconf  ${KDE4_KDECORE_LIBS} kdnssd kio )

install(TARGETS kio_zeroconf  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES zeroconf.protocol invitation.protocol  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES _http._tcp _ftp._tcp _ldap._tcp _webdav._tcp _nfs._tcp _ssh._tcp  DESTINATION ${DATA_INSTALL_DIR}/zeroconf)
install( FILES zeroconf.desktop  DESTINATION ${DATA_INSTALL_DIR}/remoteview)




#original Makefile.am contents follow:

#INCLUDES = $(all_includes)
#
#kde_module_LTLIBRARIES = kio_zeroconf.la
#
#kio_zeroconf_la_SOURCES = dnssd.cpp
#kio_zeroconf_la_LIBADD = $(LIB_KDNSSD) $(LIB_KIO)
#kio_zeroconf_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)
#
#protocol_DATA = zeroconf.protocol invitation.protocol
#protocoldir = $(kde_servicesdir)
#
#services_DATA = _http._tcp _ftp._tcp _ldap._tcp _webdav._tcp _nfs._tcp _ssh._tcp
#servicesdir = $(kde_datadir)/zeroconf
#
#remote_DATA = zeroconf.desktop
#remotedir = $(kde_datadir)/remoteview
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kio_zeroconf.pot
#
#METASOURCES = AUTO
