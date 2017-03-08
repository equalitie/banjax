#ifndef UNITTEST_COMMON_H
#define UNITTEST_COMMON_H
#include <ts/ts.h>

/**
   mock TSDebug for the sake of compiling tests independence from ATS
 */

void TSDebug(const char* tag, const char* format_str, ...);
void TSError(const char* fmt, ...) ;

tsapi void TSMutexUnlock(TSMutex mutexp);
tsapi TSReturnCode TSMutexLockTry(TSMutex mutexp);
tsapi void TSMutexUnlock(TSMutex mutexp);

#endif
