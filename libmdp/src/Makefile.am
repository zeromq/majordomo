lib_LTLIBRARIES = libmdp.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmdp.pc

include_HEADERS = \
    ../include/mdp.h \
    ../include/mdp_client.h \
    ../include/mdp_worker.h

libmdp_la_SOURCES = \
    mdp_client.c \
    mdp_worker.c

INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = mdp_broker test_client test_worker engine trader
test_client_LDADD = libmdp.la
test_worker_LDADD = libmdp.la
engine_LDADD = libmdp.la
trader_LDADD = libmdp.la

if ON_MINGW
libmdp_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libmdp_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = 


