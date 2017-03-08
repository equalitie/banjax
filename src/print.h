#pragma once

#include "util.h"

namespace print {

template<class... Args>
inline void debug(const Args&...args)
{
  TSDebug(BANJAX_PLUGIN_NAME, "%s", str(args...).c_str());
}

} // out namespace
