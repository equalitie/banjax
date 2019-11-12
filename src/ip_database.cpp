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

using namespace std;

/**
  check if  the ip is in the db, if not store it and updates its states
  related to that filter

  returns true on success and false on failure (locking not possible)
  eventually the continuation need to be retried
*/
bool 
IPDatabase::set_ip_state(const std::string& ip, FilterIDType filter_id, FilterState state)
{
  //we need to lock at the begining because somebody can
  //delete an ip while we are searching
  if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return false;
  }
  IPHashTable::iterator cur_ip_it = _ip_db.find(ip);
  if (cur_ip_it == _ip_db.end()) {
    _ip_db[ip] = IPState();

  }

  _ip_db[ip].state_array[filter_to_column[filter_id]] = state;

  TSMutexUnlock(db_mutex);
  TSDebug(BANJAX_PLUGIN_NAME, "db size %lu", _ip_db.size());
  return true;

}

/**
   Clean the ip state record mostly when it is reported to 
   swabber.
*/
bool
IPDatabase::drop_ip(std::string& ip)
{
  IPHashTable::iterator cur_ip_it = _ip_db.find(ip);
  if (cur_ip_it != _ip_db.end()) {
    if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
      TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
      return false;
    }
    _ip_db.erase(ip);
    TSDebug(BANJAX_PLUGIN_NAME, "db size %lu", _ip_db.size());
    TSMutexUnlock(db_mutex);

  }

  return true;
}

boost::optional<FilterState>
IPDatabase::get_ip_state(const std::string& ip, FilterIDType filter_id)
{
  //We actually need to lock the database
  //because the entry might get deleted while
  //we are trying to read its data.
  std::pair<bool, FilterState> result;

  if (TSMutexLockTry(db_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return boost::none;
  }

  auto on_exit = defer([&] { TSMutexUnlock(db_mutex); });

  IPHashTable::iterator cur_ip_it = _ip_db.find(ip);

  return cur_ip_it != _ip_db.end()
       ? cur_ip_it->second.state_array[filter_to_column[filter_id]]
       : FilterState{};
}

/**
   Drop all the ips due to reloading the config, its blocking on gaining  a lock
*/
void IPDatabase::drop_everything() {

  TSMutexLock(db_mutex);
  _ip_db.clear();
  TSMutexUnlock(db_mutex);

  TSDebug(BANJAX_PLUGIN_NAME, "ip db cleared");

}


