/**
   This class is responsible to store global states for each requester ip. In particular
   it is supposed:
   - Memory allocation.
   - Take care of locking and unlocking.
   - Keeping track of memory limit and garbage collection.

   AUTHORS:
   - Vmon: Oct 2013: Initial version
 */
#ifndef IP_DATABASE_H
#define IP_DATABASE_H

#include <unordered_map> // IP DB is a hash table
#include <ts/ts.h> // TSMutex
#include <vector>
#include <boost/optional.hpp>
#include <sys/time.h>

#include "defer.h"
#include "filter_list.h"
#include "print.h"

// Swabber interface isn't a filter but it needs to use the IP database.
const FilterIDType SWABBER_INTERFACE_ID = TOTAL_NO_OF_FILTERS;

// List of filters with db storage requirements (state keepers).
const FilterIDType column_to_filter[] = {
  REGEX_BANNER_FILTER_ID,
  CHALLENGER_FILTER_ID,
  SWABBER_INTERFACE_ID
};

typedef long long FilterStateUnit;
typedef std::vector<FilterStateUnit> FilterState;

class IPDatabase
{
protected:
  static constexpr size_t NUMBER_OF_STATE_KEEPER_FILTERS = sizeof(column_to_filter) / sizeof(int);
  static constexpr size_t NO_OF_NON_FILTER_STATE_KEEPER = 1; //swabber interface

  struct IPState {
    std::vector<FilterState> state_array;
    IPState(): state_array(NUMBER_OF_STATE_KEEPER_FILTERS){};
  };

  using IPHashTable = std::unordered_map<std::string, IPState>;

  IPHashTable _ip_db;
  TSMutex db_mutex;

  size_t filter_to_column[TOTAL_NO_OF_FILTERS + NO_OF_NON_FILTER_STATE_KEEPER]; //+1 because swabber_interface

public:
  IPDatabase()
    :db_mutex(TSMutexCreate())
  {
    for(size_t cur_col = 0; cur_col < NUMBER_OF_STATE_KEEPER_FILTERS; cur_col++)
      filter_to_column[column_to_filter[cur_col]] = cur_col;
  }

  /**
     Check if the ip is in the db, if not store it and updates its states
     related to that filter
   */
  bool set_ip_state(const std::string& ip, FilterIDType filter_id, FilterState state);

  /**
     Check if the ip is in the db, if not store it, with default state 0 then
     return the current state

     If the boolean value is false means reading of the state failed due to
     failure of locking the database
  */
  boost::optional<FilterState> get_ip_state(const std::string& ip, FilterIDType filter_id);

  /**
     Clean the ip state record mostly when it is reported to swabber.
  */
  bool drop_ip(std::string& ip);
};

template<class IpState_>
class IpDb {
public:
  using IpState = IpState_;

private:
  using IPHashTable = std::unordered_map<std::string, IpState>;

public:
  IpDb() : _mutex(TSMutexCreate()) {}

  IpDb(const IpDb&) = delete;

  bool set_ip_state(const std::string& ip, IpState);

  boost::optional<IpState> get_ip_state(const std::string& ip);

  bool drop_ip(std::string& ip);

private:
  IPHashTable _db;
  TSMutex _mutex;
};

template<class IpState>
inline
bool
IpDb<IpState>::set_ip_state(const std::string& ip, IpState state)
{
  if (TSMutexLockTry(_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return false;
  }

  auto on_exit = defer([&] { TSMutexUnlock(_mutex); });

  typename IPHashTable::iterator i = _db.find(ip);

  if (i == _db.end()) {
    i = _db.insert({ip, {}}).first;
  }

  i->second = std::move(state);

  return true;
}

template<class IpState>
inline
bool
IpDb<IpState>::drop_ip(std::string& ip)
{
  if (TSMutexLockTry(_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return false;
  }

  auto on_exit = defer([&] { TSMutexUnlock(_mutex); });

  return _db.erase(ip) != 0;
}

template<class IpState>
inline
boost::optional<IpState>
IpDb<IpState>::get_ip_state(const std::string& ip)
{
  if (TSMutexLockTry(_mutex) != TS_SUCCESS) {
    TSDebug(BANJAX_PLUGIN_NAME, "Unable to get lock on the ip db");
    return boost::none;
  }

  auto on_exit = defer([&] { TSMutexUnlock(_mutex); });

  typename IPHashTable::iterator i = _db.find(ip);

  return i != _db.end() ? i->second : IpState{};
}

//------------------------------------------------------------
template<class T, T default_value>
class Default {
public:
  Default()    : value(default_value) {}
  Default(T v) : value(v) {}

        T& operator*()       { return value; }
  const T& operator*() const { return value; }

  operator T() const { return value; }

private:
  T value;
};

//------------------------------------------------------------
using SwabberIpDb = IpDb<Default<time_t, 0>>;
using ChallengerIpDb = IpDb<Default<uint64_t, 0>>;

#endif
