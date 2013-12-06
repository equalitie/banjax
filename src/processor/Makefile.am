ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x -Wno-error=unused-result -pthread
AM_CPPFLAGS = -I../../include  $(lib_CPPFLAGS) -pthread


noinst_PROGRAMS  = LogProcessor

LIBLIST=-lre2 -lzmq -lconfig++

LogProcessor_LDFLAGS = \
	-module \
	-shared \
	-avoid-version \
	$(LIBLIST)

LogProcessor_LDADD =   $(lib_LIBS)
LogProcessor_SOURCES = \
	botbanger/bot_banger_aggregator.cpp \	
	botbanger/bot_banger_model_listener.cpp \
	features/feature_average_payload_size.cpp \
	features/feature_average_time_between_requests.cpp \
	features/feature_cycling_user_agent.cpp \
	features/feature_html_to_image_ratio.cpp \
	features/feature_http_status_ratio.cpp \
	features/feature_percentage_consecutive_requests.cpp \
	features/feature_request_depth.cpp \
	features/feature_session_length.cpp \
	features/feature_variance_request_interval.cpp \
	features/feature_request_depth_std.cpp \
	hosthitmiss/host_hit_miss_actions.cpp \
	hosthitmiss/host_hit_miss_aggregator.cpp \
	hosthitmiss/host_hit_miss_feature.cpp \
	utils/string_utils.cpp \
	utils/svm/svm.cpp \
	utils/hashfunctions/crc32.cpp \
	log_processor_main.cpp \
	log_entry_processor.cpp \
	log_entry_processor_config.cpp
	




