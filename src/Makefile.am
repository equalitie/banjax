ACLOCAL_AMFLAGS = -I config-aux --install

AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x

AM_CPPFLAGS = -I../include -I$(srcdir)/src -I/usr/local/trafficserver/include $(lib_CPPFLAGS) 

LIBLIST=-l re2 -l zmq -l config++

lib_LTLIBRARIES = banjax.la

banjax_la_LDFLAGS = \
  -module \
  -shared \
  -avoid-version \
  $(lib_LIBS) -lconfig++ -lre2

banjax_la_SOURCES = 	banjax.cpp \
						ats_event_handler.cpp \
						banjax_continuation.cpp \
						transaction_muncher.cpp \
                        util.cpp \
						ip_database.cpp \
						swabber_interface.cpp  \
						cookie_parser.cpp \
						challenge_manager.cpp \
						white_lister.cpp \
					 	regex_manager.cpp \
<<<<<<< variant A
						bot_sniffer.cpp


>>>>>>> variant B
					 	cookiehash.cpp \
                        util.cpp
####### Ancestor
                        util.cpp
======= end

#libbanjax_la_LDFLAGS = -version-info 0:0:0

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
