# Copyright 2013 eQualit.ie
# See LICENSE for other credits and copying information

ACLOCAL_AMFLAGS = -I config-aux --install

AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x

AM_CPPFLAGS = -I. -I$(srcdir)/src $(lib_CPPFLAGS)

AUTOMAKE_OPTIONS = foreign
SUBDIRS = src test doc
