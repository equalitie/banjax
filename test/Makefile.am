ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x
AM_CPPFLAGS = -I../include $(lib_CPPFLAGS)

noinst_PROGRAMS  = unittests

unittests_LDFLAGS = \
	-module \
	-shared \
	-avoid-version \
	-lconfig++ -lre2 \
	-lgtest_main \
	-lgtest

unittests_LDADD =   $(lib_LIBS)
unittests_SOURCES = unittest_common.cpp \
					regex_manager_unittest.cpp \
					white_lister_unittest.cpp \
					../src/regex_manager.cpp \
					../src/swabber_interface.cpp \
					../src/white_lister.cpp

#unittests_LDADD = ../src/banjax.la
