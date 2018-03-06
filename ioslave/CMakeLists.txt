add_definitions(-DTRANSLATION_DOMAIN=\"kio5_zeroconf\")
add_library(kio_zeroconf MODULE dnssd.cpp)
target_link_libraries(kio_zeroconf PRIVATE
    KF5::DNSSD
    KF5::I18n
    KF5::KIOCore
)
set_target_properties(kio_zeroconf PROPERTIES OUTPUT_NAME "zeroconf")
install(TARGETS kio_zeroconf
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

install(FILES zeroconf.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/remoteview)
