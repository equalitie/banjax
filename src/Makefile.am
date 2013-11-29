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
						base64.cpp \
						ip_database.cpp \
						swabber_interface.cpp  \
						cookie_parser.cpp \
						cookiehash.cpp \
						challenge_manager.cpp \
						white_lister.cpp \
					 	regex_manager.cpp \
						bot_sniffer.cpp \
						processor/BotBangerAggregator.cpp \
						processor/BotBangerLogger.cpp \
						processor/BotBangerModelListener.cpp \
						processor/FeatureAveragePayloadSize.cpp \
						processor/FeatureAverageTimeBetweenRequests.cpp \
						processor/FeatureCyclingUserAgent.cpp \
						processor/FeatureHtmlToImageRatio.cpp \
						processor/FeatureHTTPStatusRatio.cpp \
						processor/FeaturePercentageConsecutiveRequests.cpp \
						processor/FeatureRequestDepth.cpp \
						processor/FeatureSessionLength.cpp \
						processor/FeatureVarianceRequestInterval.cpp \
						processor/FeatureRequestDepthStd.cpp \
						processor/HostHitMissActions.cpp \
						processor/HostHitMissAggregator.cpp \
						processor/HostHitMissFeature.cpp \
						processor/HostHitMissLogger.cpp \
						processor/Main.cpp \						
						processor/utils/stringutils.cpp \
						processor/svm/svm.cpp \
						processor/hashfunctions/crc32.cpp 
						
						


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
