#ifndef LOGENTRY_H
#define LOGENTRY_H
#include <stdio.h>
#include <time.h>

/* helper function to shut the compiler up */
#define UNUSED(x) (void)(x)

/* cache lookup status */
enum CacheLookupStatus {Hit=100,Miss=1,Stale=2,Error=3};


/* log entry */
struct LogEntry
{
	char hostname[100]; // host name, max 100 characters
	char userAgent[120]; // useragent, max 120 characters
	char url[400]; // just cut off after 400
	time_t endTime; // time at which the request is logged
	long msDuration; // duration, not used
	int httpCode; // http status code
	int payloadsize; // payload size
	//int requestDepth; // request depth,
	CacheLookupStatus cacheLookupStatus; // cache lookup status
	char useraddress[40]; // useraddress, IPv4 or IPv6
	char contenttype[80]; // contenttype, max 80 characters
};

/* Log entry trace actions */
enum LogEntryTrace {
	TraceHitMissAction=1,
	TraceBotBangerAction=2,
	TraceHitMissRatio=4,
	TraceBotBangerFeatures=8,
	TraceBotBangerModelValues=16,
	TraceBotBangerModelInputs=32,
	TraceLogEntries=64,
	ConsoleMode=128,
	ServerMode=256,
	TraceBotBangerIPEvict=512
};


#endif
