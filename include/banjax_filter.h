/*
*  Part of regex_ban plugin: is the abstract class that banjax is inherited from
*
*  Vmon: May 2013: Initial version.
*/
#ifndef BANJAX_FILTER_H
#define BANJAX_FILTER_H

#include <assert.h>
#include <libconfig.h++>
#include <yaml-cpp/yaml.h>

#include "banjax_common.h"
#include "transaction_muncher.h"
#include "filter_list.h"
#include "ip_database.h"

class BanjaxFilter;
class FilterResponse;

typedef std::string (BanjaxFilter::*ResponseGenerator) (const TransactionParts& transactionp_parts, const FilterResponse& response_info);

/**
   this is the standard extended response that the event handler expect to 
   process.
 */
class FilterExtendedResponse
{
 public:
  ResponseGenerator response_generator;
  char* content_type_;
  bool banned_ip;

  std::string set_cookie_header;
  // Returns the set content type. Transfers
  // ownership to the caller.
  char* get_and_release_content_type() {
    char * ct = content_type_;
    content_type_ = NULL;
    return ct;
  }

  void set_content_type(const char* x) {
    if (content_type_ != NULL) {
      TSfree(content_type_);
      content_type_ = NULL;
    }
    if (x != NULL) {
      content_type_ = TSstrdup(x);
    }
  }

  int response_code;

  /**
     A constructor that optionally set the response_generator on creation
   */
  FilterExtendedResponse(ResponseGenerator requested_response_generator = NULL)
    : response_generator(requested_response_generator),
      content_type_(NULL),
      banned_ip(false),
      response_code(403)
  {}

};

/** we want to keep the FilterResponse struture as light as possible.
    because lots of filters response with only GO_AHEAD_NO_COMMENT and
    we want to have that transmitted as fast as possible.

    if a filter needs more space for its communication they can define
    a class and put the pointer to its object into response_data.
*/  
class FilterResponse
{
public:
   enum ResponseType {
        GO_AHEAD_NO_COMMENT,
        I_RESPOND,
        NO_WORRIES_SERVE_IMMIDIATELY,
        SERVE_FRESH,
        CALL_OTHER_FILTERS,
        CALL_ME_ON_RESPONSE
   };

  unsigned int response_type;
  void* response_data; //because FilterResponse does not have any idea about the 
  //way this pointer is used it is the filter responsibility to release the memory
  //it is pointing to at the approperiate moment
  
  FilterResponse(unsigned int cur_response_type = GO_AHEAD_NO_COMMENT, void* cur_response_data = NULL)
    : response_type(cur_response_type), response_data(cur_response_data)
  {}

  /**
     this constructor is called when all a filter wants is to set the response
     generator in the extended response. 
     
     If you are only intended to call afunction for the matter of information 
     gathering you should not use the response_generator
   */
 FilterResponse(ResponseGenerator cur_response_generator)
    :response_type(I_RESPOND),
     response_data((void*) new FilterExtendedResponse(cur_response_generator))
  {}

};

typedef FilterResponse (BanjaxFilter::*FilterTaskFunction) (const TransactionParts& transactionp_parts);

struct  FilterTask
{
  BanjaxFilter* filter;
  FilterTaskFunction task;

  FilterTask(BanjaxFilter* cur_filter, FilterTaskFunction cur_task)
    : filter(cur_filter), task(cur_task) {}

  //Default constructor just to make everthing null
  FilterTask()
    : filter(NULL),
      task(NULL) {}
      
};

class BanjaxFilter
{
 protected:
  IPDatabase* ip_database;

  /**
     It should be overriden by the filter to load its specific configurations
     
     @param banjax_dir the directory which contains banjax config files
     @param cfg the object that contains the configuration of the filter
  */
  virtual void load_config(YAML::Node& cfg, const std::string& banjax_dir) {(void) cfg; (void)banjax_dir; assert(0);};
 public:
  const unsigned int BANJAX_FILTER_ID;
  const std::string BANJAX_FILTER_NAME;

  /**
     this is a set of pointer to functions that the filter
     needs to be called on each queue, we are basically
     re-wrting the virtual table, but that makes more sense 
     for now.
  */
  enum ExecutionQueue {
    HTTP_START,
    HTTP_REQUEST,
    HTTP_READ_CACHE,
    HTTP_SEND_TO_ORIGIN,
    HTTP_RESPONSE,
    HTTP_CLOSE,
    TOTAL_NO_OF_QUEUES
  };
  
  FilterTaskFunction queued_tasks[BanjaxFilter::TOTAL_NO_OF_QUEUES];
  
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
 BanjaxFilter(const std::string& banjax_dir, YAML::Node main_root, unsigned int child_id, std::string child_name)
    : BANJAX_FILTER_ID(child_id),
      BANJAX_FILTER_NAME(child_name),
    queued_tasks()
  {
    (void) banjax_dir; (void) main_root; ip_database = NULL;
  }

  /**
     Filter should call this function cause nullify in constructor does not 
     work :( then setting their functions
   */
  virtual void set_tasks()
  {
    for(unsigned int i = 0; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; i++) {
      queued_tasks[i] = NULL;
    }
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
    TSDebug(BANJAX_PLUGIN_NAME, "You shouldn't have called me at the first place.");
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

