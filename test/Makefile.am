ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x
AM_CPPFLAGS = -I../include -I/usr/local/trafficserver/include $(lib_CPPFLAGS) -pthread

check_PROGRAMS  = unittests

GTEST_SOURCES = \
	../gtest/src/gtest_main.cc \
	../gtest/src/gtest-all.cc

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
	unittest_common.cpp \
	regex_manager_unittest.cpp \
	white_lister_unittest.cpp \
	../src/ip_database.cpp \
	../src/regex_manager.cpp \
	../src/swabber_interface.cpp \
	../src/white_lister.cpp

#unittests_LDADD = ../src/banjax.la
