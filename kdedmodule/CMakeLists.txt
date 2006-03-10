kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kded_dnssdwatcher_PART_SRCS dnssdwatcher.cpp watcher.cpp )

kde4_automoc(${kded_dnssdwatcher_PART_SRCS})

kde4_add_dcop_skels(kded_dnssdwatcher_PART_SRCS dnssdwatcher.h )

kde4_add_plugin(kded_dnssdwatcher ${kded_dnssdwatcher_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_dnssdwatcher )

target_link_libraries(kded_dnssdwatcher  ${QT_AND_KDECORE_LIBS} kdnssd )

install_targets(${LIB_INSTALL_DIR}/kde4 kded_dnssdwatcher )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES dnssdwatcher.desktop )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_dnssdwatcher.la
#
#METASOURCES = AUTO
#INCLUDES = $(all_includes) 
#
#kded_dnssdwatcher_la_SOURCES = dnssdwatcher.cpp dnssdwatcher.skel watcher.cpp
#kded_dnssdwatcher_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_dnssdwatcher_la_LIBADD =  $(LIB_KDNSSD)
#
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = dnssdwatcher.desktop
#
