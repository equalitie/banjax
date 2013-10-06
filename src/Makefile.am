ACLOCAL_AMFLAGS = -I config-aux --install

AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x

AM_CPPFLAGS = -I../include -I$(srcdir)/src $(lib_CPPFLAGS)

LIBLIST=-l re2 -l zmq -l config++


lib_LTLIBRARIES = banjax.la
# noinst_HEADERS =ats_event_handler.h \
# 			    banjax.h \
# 				regex_manager.h \
# 		        transaction_muncyher.h \
# 				banjax_continuation.h \
# 				challenge_manager.h \
# 				response_generator.h \
# 				include/util.h \
# 				banjax_filter.h \
# 		        cookie_parser.h \
# 			    swabber_interface.h \
# 			    white_lister.h

banjax_la_LDFLAGS = \
  -module \
  -shared \
  -avoid-version \
  $(lib_LIBS) -lconfig++ -lre2

banjax_la_SOURCES = 	banjax.cpp \
						ats_event_handler.cpp \
						challenge_manager.cpp \
						swabber_interface.cpp  \
						white_lister.cpp \
						banjax_continuation.cpp \
						cookie_parser.cpp \
						transaction_muncher.cpp \
					 	regex_manager.cpp \
                        util.cpp

#libbanjax_la_LDFLAGS = -version-info 0:0:0

