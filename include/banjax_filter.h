/*
*  Part of regex_ban plugin: is the abstract class that banjax is inherited from
*
*  Vmon: May 2013: Initial version.
*/
#ifndef BANJAX_FILTER_H
#define BANJAX_FILTER_H

#include <assert.h>
#include <libconfig.h++>

#include "transaction_muncher.h"

#include "banjax.h"

enum  {
  REGEX_BANNER_FILTER_ID,
  CHALLENGER_FILTER_ID,
  BOT_SNIFFER_FILTER_ID,
  WHITE_LISTER_FILTER_ID,
};
  
const std::string REGEX_BANNER_FILTER_NAME = "regex_banner";
const std::string CHALLENGER_FILTER_NAME = "challenger";
const std::string BOT_SNIFFER_FILTER_NAME = "bot_sniffer";
const std::string WHITE_LISTER_FILTER_NAME = "white_lister";


class FilterResponse
{
public:
   enum ResponseType {
        GO_AHEAD_NO_COMMENT,
        I_RESPOND,
        NO_WORRIES_SERVE_IMMIDIATELY,
        CALL_OTHER_FILTERS,
        CALL_ME_ON_RESPONSE
   };

   unsigned int response_type;
   void* response_data;

 FilterResponse(unsigned int cur_response_type = GO_AHEAD_NO_COMMENT, void* cur_response_data = NULL)
   : response_type(cur_response_type), response_data(cur_response_data)
   {}

};

class BanjaxFilter
{
 protected:
  /**
     It should be overriden by the filter to load its specific configurations
     
     @param banjax_dir the directory which contains banjax config files
     @param cfg        the object that contains the configuration of the filter
  */
  virtual void load_config(libconfig::Setting& cfg) {(void) cfg;assert(0);};
  
 public:
  const unsigned int BANJAX_FILTER_ID;
  const std::string BANJAX_FILTER_NAME;

  /**
     A disabled filter won't run, 
     Banjax calls the execution function of an enabled filter
     A controlled filter can be run in request of other filters
   */
  enum Status {
    disabled,
    enabled,
    controlled,
  };

  Status filter_status;
    
  /**
     receives the db object need to read the regex list,
     subsequently it reads all the regexs

  */
 BanjaxFilter(const std::string& banjax_dir, const libconfig::Setting& main_root, unsigned int child_id, std::string child_name)
    : BANJAX_FILTER_ID(child_id),
    BANJAX_FILTER_NAME(child_name)
  {
    (void) banjax_dir; (void) main_root;

  }

  /**
     Simpler constructor that only receives name and id of the filter.

   */
 BanjaxFilter(unsigned int child_id, std::string child_name)
    : BANJAX_FILTER_ID(child_id),
    BANJAX_FILTER_NAME(child_name)
  {
  }

  /**
     destructor: just to tell the compiler that the destructor is
     virtual
  */
  virtual ~BanjaxFilter()
    {
    }

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the request and handed to the filter.
  */
  virtual uint64_t requested_info() = 0;

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the response and handed to the filter.
  */
  virtual uint64_t response_info() {return 0;}

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the request and handed to the filter.

     @return if return the response_type of FilterResponse is 
     GO_AHEAD_NO_COMMENT then it has no effect on the transacion

     TODO: the return should be able to tell banjax to call another
     filter to run specific action. For example if botsniffer think 
     that the ip might be bot then should be able to ask the challenger
     to challenge it.

  */
  virtual FilterResponse execute(const TransactionParts& transaction_parts) = 0;
  
  /**
     The functoin will be called if the filter reply with I_RESPOND
   */
  virtual std::string generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
  { 
    //Just in case that the filter has nothing to do with the response
    //we should not make them to overload this
    //but shouldn't be called anyway.
    (void) transaction_parts; (void) response_info;
    TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "You shouldn't have called me at the first place.");
    assert(NULL);
  }

  /**
     Overload if you want this filter be called during the response
     the filter should return CALL_ME_ON_RESPONSE when it is executed 
     during request
   */
  virtual FilterResponse execute_on_response(const TransactionParts& transaction_parts)
  {
    (void) transaction_parts;
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

};
  
#endif /* regex_manager.h */

