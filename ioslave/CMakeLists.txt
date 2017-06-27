add_definitions(-DTRANSLATION_DOMAIN=\"kio5_zeroconf\")
add_library(kio_zeroconf MODULE dnssd.cpp)
target_link_libraries(kio_zeroconf PRIVATE
    KF5::DNSSD
    KF5::I18n
    KF5::KIOCore
)
install(TARGETS kio_zeroconf
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES zeroconf.protocol
        DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES zeroconf.desktop
        DESTINATION ${DATA_INSTALL_DIR}/remoteview)
