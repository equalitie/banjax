#include <string>
#include <ts/ts.h>
#include <iostream>
using namespace std;

#include "banjax_common.h"
#include "banjax.h"
#include "unittest_common.h"

/**
   mock TSDebug for the sake of compiling tests independence from ATS
 */
void TSDebug(const char* tag, const char* format_str, ...)
{
  (void) tag, (void) format_str;
}

void TSError(const char* fmt, ...) 
{
  (void) fmt;
}

tsapi TSMutex TSMutexCreate(void)
{
  return NULL;
}

tsapi TSReturnCode TSMutexLockTry(TSMutex mutexp)
{
  //each test has its own database, no multi-thread threat
  (void) mutexp;
  return TS_SUCCESS;
}

tsapi void TSMutexUnlock(TSMutex mutexp)
{
  (void) mutexp;
}

