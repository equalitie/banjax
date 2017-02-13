AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x
AM_CPPFLAGS = -I../include -I libb64/include -I/usr/local/trafficserver/include $(lib_CPPFLAGS) -pthread

check_PROGRAMS  = unittests

#GTEST_SOURCES = \
#	../gtest/src/gtest_main.cc \
#	../gtest/src/gtest-all.cc

LIBLIST=-lre2 -lzmq -lconfig++ -lyaml-cpp

unittests_LDFLAGS = \
	-module \
	-shared \
	-avoid-version \
	-lre2 \
	$(LIBLIST)

unittests_LDADD =   $(lib_LIBS) \
	../gtest/lib/libgtest_main.la \
	../gtest/lib/libgtest.la

unittests_SOURCES = \
	libb64/src/cencode.c \
	libb64/src/cdecode.c \
	unittest_common.cpp \
	regex_manager_unittest.cpp \
	white_lister_unittest.cpp \
	challenge_manager_unittest.cpp \
	base64_unittest.cpp \
	../src/cookiehash.cpp \
	../src/cookie_parser.cpp \
	../src/ip_database.cpp \
	../src/regex_manager.cpp \
	../src/challenge_manager.cpp \
	../src/swabber_interface.cpp \
	../src/white_lister.cpp \
	../src/base64.cpp \
	../src/util.cpp


# libb64/src/cencode.c
# libb64/src/cdecode.c
# libbase64/base64/base64.cc


#unittests_LDADD = ../src/banjax.la
