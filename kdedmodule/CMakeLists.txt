add_library(kded_dnssdwatcher MODULE)
set_target_properties(kded_dnssdwatcher PROPERTIES
    OUTPUT_NAME dnssdwatcher
)

kcoreaddons_desktop_to_json(kded_dnssdwatcher dnssdwatcher.desktop)

set(kded_dnssdwatcher_dbus_SRCS)
qt5_add_dbus_adaptor(kded_dnssdwatcher_dbus_SRCS
    org.kde.kdnssd.xml
    dnssdwatcher.h
    DNSSDWatcher)

target_sources(kded_dnssdwatcher PRIVATE
    dnssdwatcher.cpp
    watcher.cpp
    ${kded_dnssdwatcher_dbus_SRCS}
)

target_link_libraries(kded_dnssdwatcher PRIVATE
    KF5::DBusAddons
    KF5::KIOCore
    KF5::DNSSD
)

install(TARGETS kded_dnssdwatcher DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)

install(FILES org.kde.kdnssd.xml
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
