#ifndef UNITTEST_COMMON_H
#define UNITTEST_COMMON_H
/**
   mock TSDebug for the sake of compiling tests independence from ATS
 */
void TSDebug(const char* tag, const char* format_str, ...);
void TSError(const char* fmt, ...) ;

#endif
