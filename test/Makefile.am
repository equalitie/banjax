ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x
AM_CPPFLAGS = -I../include -I/usr/local/trafficserver/include $(lib_CPPFLAGS) -pthread

noinst_PROGRAMS  = unittests

LIBLIST=-lre2 -lzmq -lconfig++

unittests_LDFLAGS = \
	-module \
	-shared \
	-avoid-version \
	-lconfig++ -lre2 \
	-lgtest \
	-lgtest_main \
	$(LIBLIST)

unittests_LDADD =   $(lib_LIBS)
unittests_SOURCES = unittest_common.cpp \
					regex_manager_unittest.cpp \
					white_lister_unittest.cpp \
					../src/ip_database.cpp \
					../src/regex_manager.cpp \
					../src/swabber_interface.cpp \
					../src/white_lister.cpp \
					../src/processor/botbanger/bot_banger_aggregator.cpp \						
					../src/processor/botbanger/bot_banger_model_listener.cpp \					
					../src/processor/features/feature_average_payload_size.cpp \
					../src/processor/test/feature_average_payload_size_test.cpp \
					../src/processor/features/feature_average_time_between_requests.cpp \
					../src/processor/test/feature_average_time_between_requests_test.cpp \
					../src/processor/features/feature_cycling_user_agent.cpp \
					../src/processor/test/feature_cycling_user_agent_test.cpp \
					../src/processor/features/feature_html_to_image_ratio.cpp \
					../src/processor/test/feature_html_to_image_ratio_test.cpp \
					../src/processor/features/feature_http_status_ratio.cpp \
					../src/processor/test/feature_http_status_ratio_test.cpp \
					../src/processor/features/feature_percentage_consecutive_requests.cpp \
					../src/processor/test/feature_percentage_consecutive_requests_test.cpp \
					../src/processor/features/feature_request_depth.cpp \
					../src/processor/test/feature_request_depth_test.cpp \
					../src/processor/features/feature_session_length.cpp \
					../src/processor/test/feature_session_length_test.cpp \
					../src/processor/features/feature_variance_request_interval.cpp \
					../src/processor/test/feature_variance_request_interval_test.cpp \
					../src/processor/features/feature_request_depth_std.cpp \
					../src/processor/test/feature_request_depth_std_test.cpp \
					../src/processor/test/std_dev_test.cpp \
					../src/processor/hosthitmiss/host_hit_miss_actions.cpp \
					../src/processor/hosthitmiss/host_hit_miss_aggregator.cpp \
					../src/processor/hosthitmiss/host_hit_miss_feature.cpp \
					../src/processor/utils/string_utils.cpp \
					../src/processor/utils/svm/svm.cpp \
					../src/processor/utils/hashfunctions/crc32.cpp \
					../src/processor/log_entry_processor.cpp \					
					../src/processor/log_entry_processor_config.cpp

#unittests_LDADD = ../src/banjax.la
