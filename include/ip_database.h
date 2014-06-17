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
#include <ts/ts.h> //for locking business

#include "filter_list.h"

//list of filter with db storage requirement (state keepers
const FilterIDType filter_to_column[] = {
  REGEX_BANNER_FILTER_ID,
  CHALLENGER_FILTER_ID

};
/* const unsigned  filters_to_column[] = { */
/*   0 */
/* }; */
const unsigned int NUMBER_OF_STATE_KEEPER_FILTERS = sizeof(filter_to_column) / sizeof(int);

const unsigned int NUMBER_OF_STATE_WORDS = 2;

struct FilterState
{
  long long single_filter_state[2];
  FilterState()
  : single_filter_state() {}

  //virtual ~FilterState() {int a = 0; (void)a;}
};

struct IPState
{
  FilterState state_array[NUMBER_OF_STATE_KEEPER_FILTERS];

};
typedef std::unordered_map<std::string, IPState> IPHashTable;

class IPDatabase
{
  friend class Banjax; //To access the mutex and assign it to the global 
  //continuation

protected:
  IPHashTable _ip_db;
  TSMutex db_mutex;
  
public:
  /**
     check if  the ip is in the db, if not store it and updates its states
     related to that filter
   */
  bool set_ip_state(std::string& ip, FilterIDType filter_id, FilterState state);

  /**
     check if  the ip is in the db, if not store it, with default state 0
     then return the current state
  */
  FilterState get_ip_state(std::string& ip, FilterIDType filter_id);

  /**
     Clean the ip state record mostly when it is reported to 
     swabber.
  */
  bool drop_ip(std::string& ip);

  /**
     constructor: is creating the mutex
  */
  IPDatabase()
    :db_mutex(TSMutexCreate())
  { }


};

#endif
