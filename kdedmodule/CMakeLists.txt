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
        DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES dnssdwatcher.desktop
        DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kded)
install(FILES org.kde.kdnssd.xml
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
