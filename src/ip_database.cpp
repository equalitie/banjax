/**
   This class is responsible to store global states for each requester ip. In particular
   it is supposed:
   - Memory allocation.
   - Take care of locking and unlocking.
   - Keeping track of memory limit and garbage collection.

   AUTHORS:
   - Vmon: Oct 2013: Initial version
*/

#include "ip_database.h"
#include "banjax.h"
#include "defer.h"

bool
IPDatabase::set_ip_state(const std::string& ip, FilterIDType filter_id, FilterState state)
{
  //we need to lock at the begining because somebody can
  //delete an ip while we are searching
  if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return false;
  }

  auto on_exit = defer([&] { TSMutexUnlock(db_mutex); });

  IPHashTable::iterator i = _ip_db.find(ip);

  if (i == _ip_db.end()) {
    i = _ip_db.insert({ip, {}}).first;
  }

  i->second.state_array[filter_to_column[filter_id]] = state;

  return true;
}

bool
IPDatabase::drop_ip(std::string& ip)
{
  if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return false;
  }

  auto on_exit = defer([&] { TSMutexUnlock(db_mutex); });

  return _ip_db.erase(ip) != 0;
}

boost::optional<FilterState>
IPDatabase::get_ip_state(const std::string& ip, FilterIDType filter_id)
{
  if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return boost::none;
  }

  auto on_exit = defer([&] { TSMutexUnlock(db_mutex); });

  IPHashTable::iterator i = _ip_db.find(ip);

  return i != _ip_db.end()
       ? i->second.state_array[filter_to_column[filter_id]]
       : FilterState{};
}
