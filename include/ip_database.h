/**
   This class is responsible to store global states for each requester ip. In particular 
   it is supposed:
   - Memory allocation.
   - Take care of locking and unlocking.
   - Keeping track of memory limit and garbage collection.

   AUTHORS:
   - Vmon: Oct 2013: Initial version
 */
#include <map> //IP DB is a hash table
#include <ts/ts.h> //for locking business
#include <banjax_filter.h>

//list of filter with db storage requirement (state keepers
const unsigned int filters_to_column[] = {
  REGEX_BANNER_FILTER_ID

};

const unsigned int NUMBER_OF_STATE_KEEPER_FILTERS = sizeof(filters_to_column) / sizeof(int);

typedef long long [NUMBER_OF_STATE_KEEPER_FILTERS] IPState;
typedef map<std::string, IPState> IPHashTable;

class IPDatabase
{
protected:
  IPHashTable _ip_db;
  static TSMutex db_mutex;
  
public:
  /**
     check if  the ip is in the db, if not store it and updates its states
     related to that filter
   */
  void set_ip_state(std::string& ip, long long state);

   /**
     check if  the ip is in the db, if not store it, with default state 0
     then return the current state
   */
   long get_ip_state(std::string& ip);

};
