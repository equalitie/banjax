/*
*  Part of regex_ban plugin: is the abstract class that banjax is inherited from
*
*  Vmon: May 2013: Initial version.
*/
#ifndef BANJAX_FILTER_H
#define BANJAX_FILTER_H

#include <assert.h>
#include <yaml-cpp/yaml.h>
#include <functional>

#include "transaction_muncher.h"
#include "filter_list.h"
#include "ip_db.h"

class BanjaxFilter;
class FilterResponse;

using ResponseGenerator = std::function<std::string(const TransactionParts&, const FilterResponse&)>;

/** this is mainly created due to disablibity of yaml
    to merge nodes of the same name but also to manage
    priority
  */
class FilterConfig {
public:
  std::list<YAML::const_iterator> config_node_list;
  int priority;

  FilterConfig() : priority(0) {}
};

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
  int response_code;

  // Returns the set content type. Transfers
  // ownership to the caller.
  char* get_and_release_content_type() {
    char * ct = content_type_;
    content_type_ = nullptr;
    return ct;
  }

  void set_content_type(const char* x) {
    if (content_type_) {
      TSfree(content_type_);
      content_type_ = nullptr;
    }
    if (x) {
      content_type_ = TSstrdup(x);
    }
  }

  /**
     A constructor that optionally set the response_generator on creation
   */
  FilterExtendedResponse(ResponseGenerator requested_response_generator = nullptr) :
    response_generator(requested_response_generator),
    content_type_(nullptr),
    banned_ip(false),
    response_code(403)
  {}

  virtual ~FilterExtendedResponse() {}
};

/** we want to keep the FilterResponse struture as light as possible.
    because lots of filters response with only GO_AHEAD_NO_COMMENT and
    we want to have that transmitted as fast as possible.

    if a filter needs more space for its communication they can define
    a class and put the pointer to its object into response_data.
*/
class FilterResponse final
{
public:
   enum ResponseType {
        GO_AHEAD_NO_COMMENT,
        I_RESPOND,
        SERVE_IMMIDIATELY_DONT_CACHE,
        SERVE_FRESH,
   };

  ResponseType response_type;
  std::shared_ptr<FilterExtendedResponse> response_data;

  FilterResponse(ResponseType cur_response_type = GO_AHEAD_NO_COMMENT, FilterExtendedResponse* cur_response_data = nullptr) :
    response_type(cur_response_type),
    response_data(cur_response_data)
  {}

  /**
     this constructor is called when all a filter wants is to set the response
     generator in the extended response.

     If you are only intended to call afunction for the matter of information
     gathering you should not use the response_generator
   */
  FilterResponse(ResponseGenerator cur_response_generator) :
    response_type(I_RESPOND),
    response_data(new FilterExtendedResponse(cur_response_generator))
  {}
};

typedef FilterResponse (BanjaxFilter::*FilterTaskFunction) (const TransactionParts& transactionp_parts);

struct  FilterTask
{
  BanjaxFilter* filter;

  FilterTask() : filter(NULL) {}
  FilterTask(BanjaxFilter* cur_filter) : filter(cur_filter) {}
};

class BanjaxFilter
{
protected:
  YAML::Node cfg;

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

  BanjaxFilter* queued_tasks[BanjaxFilter::TOTAL_NO_OF_QUEUES];

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

     it also merge scattered config in one node

  */
  BanjaxFilter(const FilterConfig& filter_config, unsigned int child_id, std::string child_name) :
    BANJAX_FILTER_ID(child_id),
    BANJAX_FILTER_NAME(child_name)
  {
    for (size_t i = 0; i < BanjaxFilter::TOTAL_NO_OF_QUEUES; ++i) {
      queued_tasks[i] = nullptr;
    }

    for(const auto& cur_node : filter_config.config_node_list) {
      cfg = merge_nodes(cfg, cur_node->second);
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
  virtual FilterResponse on_http_request(const TransactionParts& transaction_parts) = 0;
  virtual void on_http_close(const TransactionParts& transaction_parts) = 0;

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
    return "";
  }

private:
  static YAML::Node merge_nodes(const YAML::Node& a, const YAML::Node &b) {
    YAML::Node result;
    if (b.IsMap()) {
      result = a;
      for (auto n0 : b) {
        result[n0.first.Scalar()] =  merge_nodes(result[n0.first.Scalar()], n0.second);
      }
    } else if (a.IsSequence() && b.IsSequence()) {
      result = a;
      for (auto n0 : b) {
        result.push_back(n0);
      }
    } else {
      result = b;
    }
    return result;
  }
};

#endif /* regex_manager.h */

