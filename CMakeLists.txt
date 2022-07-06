cmake_minimum_required(VERSION 3.16)

# Release service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(kio-zeroconf VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF_MIN_VERSION "5.96.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

include(ECMDeprecationSettings)
include(FeatureSummary)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core)

find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS DBusAddons DNSSD I18n KIO)

ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 5.96
)

add_subdirectory(kioworker)
add_subdirectory(kdedmodule)

install(FILES org.kde.zeroconf-ioslave.metainfo.xml
        DESTINATION ${KDE_INSTALL_METAINFODIR})

ki18n_install(po)

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
