
########### next target ###############

set(kded_dnssdwatcher_PART_SRCS dnssdwatcher.cpp watcher.cpp )

qt4_add_dbus_adaptor( kded_dnssdwatcher_PART_SRCS org.kde.kdnssd.xml dnssdwatcher.h DNSSDWatcher )


kde4_add_plugin(kded_dnssdwatcher ${kded_dnssdwatcher_PART_SRCS})

target_link_libraries(kded_dnssdwatcher  ${KDE4_KIO_LIBS} ${KDE4_KDNSSD_LIBS} )

install(TARGETS kded_dnssdwatcher  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES dnssdwatcher.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install( FILES org.kde.kdnssd.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
