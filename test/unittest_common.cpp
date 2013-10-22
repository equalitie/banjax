#include <string>
#include <ts/ts.h>

using namespace std;

#include "banjax.h"
#include "unittest_common.h"

extern const string Banjax::BANJAX_PLUGIN_NAME = "banjax";

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
