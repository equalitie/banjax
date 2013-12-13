#ifndef LOG_ENTRY_TEST_H
#define LOG_ENTRY_TEST_H
#include "../log_entry.h"
class LogEntryTest
{
public:
	static void InitLogEntry(LogEntry *le)
	{
		memset(le,0,sizeof(LogEntry));
		strcpy(le->hostname,"www.example.org");
		strcpy(le->url,"/");
		strcpy(le->contenttype,"text/html");
		time(&le->endTime);
		le->httpCode=200;
		strcpy(le->userAgent,"imaginary useragent");
		strcpy(le->useraddress,"127.0.0.1");
		le->cacheLookupStatus=Hit;
		le->payloadsize=512;

	}
};

#endif
