



########### next target ###############

set(kded_dnssdwatcher_PART_SRCS dnssdwatcher.cpp watcher.cpp )

kde4_automoc(${kded_dnssdwatcher_PART_SRCS})

kde4_add_plugin(kded_dnssdwatcher ${kded_dnssdwatcher_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_dnssdwatcher )

target_link_libraries(kded_dnssdwatcher  ${KDE4_KIO_LIBS} kdnssd )

install_targets(${PLUGIN_INSTALL_DIR} kded_dnssdwatcher )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES dnssdwatcher.desktop )




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
