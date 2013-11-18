#ifndef LOGENTRY_H
#define LOGENTRY_H
#include <stdio.h>



enum CacheLookupStatus {Hit=0,Miss=1,Stale=2};
struct LogEntry
{
  char hostname[100];
  char url[400]; // just cut off after 400
  time_t start;
  long msDuration;
  int httpCode;
  int payloadsize;
  CacheLookupStatus cacheLookupStatus;
  char useraddress[40];
  char contenttype[80];
  char useragent[120];
};
#endif
