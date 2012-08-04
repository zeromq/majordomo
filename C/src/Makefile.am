lib_LTLIBRARIES = libmdp.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmdp.pc

include_HEADERS = \
    ../include/mdp.h

libmdp_la_SOURCES = \
    mdbroker.c \
    mdworker.c \
    mdclient.c \
    mdcliapi.c \
    mdwrkapi.c

INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = mdp_selftest
mdp_selftest_LDADD = libmdp.la
mdp_selftest_SOURCES = mdp_selftest.c

if ON_MINGW
libmdp_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libmdp_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = mdp_selftest


