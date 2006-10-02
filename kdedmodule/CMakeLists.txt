
########### next target ###############

set(kded_dnssdwatcher_PART_SRCS dnssdwatcher.cpp watcher.cpp )

kde4_automoc(${kded_dnssdwatcher_PART_SRCS})

kde4_add_plugin(kded_dnssdwatcher ${kded_dnssdwatcher_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_dnssdwatcher )

target_link_libraries(kded_dnssdwatcher  ${KDE4_KIO_LIBS} ${KDE4_KDNSSD_LIBS} )

install(TARGETS kded_dnssdwatcher  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES dnssdwatcher.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded)

