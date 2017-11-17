cmake_minimum_required(VERSION 2.8.12)

project(kdnssd)

find_package(ECM 0.0.10 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)

include(FeatureSummary)

find_package(KF5 REQUIRED COMPONENTS DBusAddons DNSSD I18n KIO)

add_subdirectory(ioslave)
add_subdirectory(kdedmodule)

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
