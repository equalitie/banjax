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
						processor/botbanger/bot_banger_aggregator.cpp \						
						processor/botbanger/bot_banger_model_listener.cpp \
						processor/features/feature_average_payload_size.cpp \
						processor/features/feature_average_time_between_requests.cpp \
						processor/features/feature_cycling_user_agent.cpp \
						processor/features/feature_html_to_image_ratio.cpp \
						processor/features/feature_http_status_ratio.cpp \
						processor/features/feature_percentage_consecutive_requests.cpp \
						processor/features/feature_request_depth.cpp \
						processor/features/feature_session_length.cpp \
						processor/features/feature_variance_request_interval.cpp \
						processor/features/feature_request_depth_std.cpp \
						processor/hosthitmiss/host_hit_miss_actions.cpp \
						processor/hosthitmiss/host_hit_miss_aggregator.cpp \
						processor/hosthitmiss/host_hit_miss_feature.cpp \
						processor/utils/string_utils.cpp \
						processor/utils/svm/svm.cpp \
						processor/utils/hashfunctions/crc32.cpp
						
						


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
