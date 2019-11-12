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

#include <unordered_map> //IP DB is a hash table
#include <utility>
#include <ts/ts.h> //for locking business
#include <vector>
#include <boost/optional.hpp>

#include "filter_list.h"

const FilterIDType SWABBER_INTERFACE_ID = TOTAL_NO_OF_FILTERS; //swabber interface isn't a filter but it needs use IP database.


//list of filter with db storage requirement (state keepers
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

  size_t ip_state_array_size;
  size_t filter_to_column[TOTAL_NO_OF_FILTERS + NO_OF_NON_FILTER_STATE_KEEPER]; //+1 because swabber_interface
  
public:
  /**
     check if  the ip is in the db, if not store it and updates its states
     related to that filter
   */
  bool set_ip_state(const std::string& ip, FilterIDType filter_id, FilterState state);

  /**
     check if  the ip is in the db, if not store it, with default state 0
     then return the current state
     
     if the boolean value is false means reading of the state
     failed due to failure of locking the database
  */
  boost::optional<FilterState> get_ip_state(const std::string& ip, FilterIDType filter_id);
  
  /**
     Clean the ip state record mostly when it is reported to 
     swabber.
  */
  bool drop_ip(std::string& ip);

  /**
     Drop all the ips due to reloading the config, its blocking on gaining  a lock
  */
  void drop_everything();

  /**
     constructor: is of creating experience and the the mutex
  */
  IPDatabase()
    :db_mutex(TSMutexCreate())
  {
    for(unsigned int cur_col = 0; cur_col < NUMBER_OF_STATE_KEEPER_FILTERS; cur_col++)
      filter_to_column[column_to_filter[cur_col]] = cur_col;
  }
};

#endif
