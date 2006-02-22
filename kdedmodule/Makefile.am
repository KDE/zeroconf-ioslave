kde_module_LTLIBRARIES = kded_dnssdwatcher.la

METASOURCES = AUTO
INCLUDES = $(all_includes) 

kded_dnssdwatcher_la_SOURCES = dnssdwatcher.cpp dnssdwatcher.skel watcher.cpp
kded_dnssdwatcher_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_dnssdwatcher_la_LIBADD =  $(LIB_KDNSSD) $(LIB_KIO)


servicesdir = $(kde_servicesdir)/kded
services_DATA = dnssdwatcher.desktop

