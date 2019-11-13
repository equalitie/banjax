#include <string>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <assert.h>

#include "unittest_common.h"

using namespace std;

/**
   mock TSDebug for the sake of compiling tests independence from ATS
 */
void TSDebug(const char* tag, const char* format_str, ...)
{
  //va_list arglist;
  //va_start(arglist, format_str);
  //printf("%s: ", tag);
  //vprintf(format_str, arglist);
  //printf("\n");
  //va_end(arglist);
}

void TSError(const char* fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  vprintf(fmt, arglist);
  va_end(arglist);
  printf("\n");
}

void _TSfree(void*) {
  assert(0);
}

char* _TSstrdup(const char *str, int64_t length, const char *path) {
  (void) str; (void) length; (void) path;
  assert(0);
  return (char*) "";
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

tsapi void TSMutexLock(TSMutex mutexp)
{
  //each test has its own database, no multi-thread threat
  (void) mutexp;
}

tsapi void TSMutexUnlock(TSMutex mutexp)
{
  (void) mutexp;
}

