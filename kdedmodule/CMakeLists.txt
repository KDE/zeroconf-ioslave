set(kded_dnssdwatcher_PART_SRCS
    dnssdwatcher.cpp
    watcher.cpp
)

qt5_add_dbus_adaptor(kded_dnssdwatcher_PART_SRCS
    org.kde.kdnssd.xml
    dnssdwatcher.h
    DNSSDWatcher)

add_library(kded_dnssdwatcher MODULE ${kded_dnssdwatcher_PART_SRCS})
target_link_libraries(kded_dnssdwatcher PRIVATE
    KF5::DBusAddons
    KF5::KIOCore
    KF5::DNSSD
)

install(TARGETS kded_dnssdwatcher
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES dnssdwatcher.desktop
        DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES org.kde.kdnssd.xml
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
