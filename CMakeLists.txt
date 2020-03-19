cmake_minimum_required(VERSION 3.0)

project(kdnssd)

set(QT_MIN_VERSION "5.9.0")
set(KF_MIN_VERSION "5.37.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core)

find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS DBusAddons DNSSD I18n KIO)


add_subdirectory(ioslave)
add_subdirectory(kdedmodule)

install(FILES org.kde.zeroconf-ioslave.metainfo.xml
        DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
