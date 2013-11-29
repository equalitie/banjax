ACLOCAL_AMFLAGS = -I config-aux --install
AM_CXXFLAGS = -Werror -Wall -Wextra -Wformat=2 -std=c++0x -Wno-error=unused-result
AM_CPPFLAGS = -I../../include  $(lib_CPPFLAGS) -pthread


noinst_PROGRAMS  = LogProcessor

LIBLIST=-lre2 -lzmq -lconfig++

LogProcessor_LDFLAGS = \
	-module \
	-shared \
	-avoid-version \	
	$(LIBLIST)

LogProcessor_LDADD =   $(lib_LIBS)
LogProcessor_SOURCES = BotBangerAggregator.cpp \
	BotBangerLogger.cpp \
	BotBangerModelListener.cpp \
	FeatureAveragePayloadSize.cpp \
	FeatureAverageTimeBetweenRequests.cpp \
	FeatureCyclingUserAgent.cpp \
	FeatureHtmlToImageRatio.cpp \
	FeatureHTTPStatusRatio.cpp \
	FeaturePercentageConsecutiveRequests.cpp \
	FeatureRequestDepth.cpp \
	FeatureSessionLength.cpp \
	FeatureVarianceRequestInterval.cpp \
	FeatureRequestDepthStd.cpp \
	HostHitMissActions.cpp \
	HostHitMissAggregator.cpp \
	HostHitMissFeature.cpp \
	HostHitMissLogger.cpp \
	Main.cpp \
	utils/stringutils.cpp \
	svm/svm.cpp \
	hashfunctions/crc32.cpp 




