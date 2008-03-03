
########### next target ###############

set(kio_zeroconf_PART_SRCS dnssd.cpp )


kde4_add_plugin(kio_zeroconf ${kio_zeroconf_PART_SRCS})


target_link_libraries(kio_zeroconf  ${KDE4_KIO_LIBS} ${KDE4_KDNSSD_LIBS} )

install(TARGETS kio_zeroconf  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES zeroconf.protocol DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES zeroconf.desktop  DESTINATION ${DATA_INSTALL_DIR}/remoteview)

