#include <string>
#include <ts/ts.h>
#include <iostream>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

using namespace std;

#include "banjax_common.h"
#include "banjax.h"
#include "unittest_common.h"

/**
 * Small utility function for easier debugging of anything that is
 * a range. Usage:
 *   set<string> my_set({"foo", "bar"});
 *   cout << "my_set: " << debug_range(my_set) << endl;
 *
 * Outputs: "my_set: [foo, bar]"
 */
template<class T> struct DebugRange { const T& inner; };

template<class T>
std::ostream& operator<<(std::ostream& os, const DebugRange<T>& r) {
  os << "[";

  for (auto i = r.inner.begin(); i != r.inner.end();) {
    os << *i;
    if (++i != r.inner.end()) os << ", ";
  }

  return os << "]";
}

template<class T> DebugRange<T> debug_range(const T& r) {
  return DebugRange<T>{r};
}

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
}

void _TSfree(void*) {
  assert(0);
}

char* _TSstrdup(const char *str, int64_t length, const char *path) {
  (void) str; (void) length; (void) path;
  assert(0);
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

