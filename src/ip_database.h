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

#include <unordered_map>
#include <ts/ts.h> // TSMutex
#include <vector>
#include <boost/optional.hpp>
#include <sys/time.h>

#include "defer.h"
#include "print.h"

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
#endif
