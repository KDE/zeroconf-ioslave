
########### next target ###############

set(kio_zeroconf_PART_SRCS dnssd.cpp )

kde4_automoc(${kio_zeroconf_PART_SRCS})

kde4_add_plugin(kio_zeroconf ${kio_zeroconf_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_zeroconf )

target_link_libraries(kio_zeroconf  ${KDE4_KIO_LIBS} ${KDE4_KDNSSD_LIBS} )

install(TARGETS kio_zeroconf  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES zeroconf.protocol invitation.protocol  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES _http._tcp _ftp._tcp _ldap._tcp _webdav._tcp _nfs._tcp _ssh._tcp  DESTINATION ${DATA_INSTALL_DIR}/zeroconf)
install( FILES zeroconf.desktop  DESTINATION ${DATA_INSTALL_DIR}/remoteview)

