/*
*  A subfilter of banjax that publishes all log information for each
*  request in a zmq socket so botbanger-python can grab them and 
*  does svm computation with them
*
*  Vmon: Oct 2013: Initial version.
*/
#ifndef BOT_SNIFFER_H
#define BOT_SNIFFER_H

#include <zmq.hpp>

#include "banjax_filter.h"
#include "log_processor_interface.h"

class BotSniffer : public BanjaxFilter
{
 protected:

  //socket stuff
  zmq::context_t context;
  zmq::socket_t zmqsock;

  unsigned int botbanger_port;
  std::string botbanger_server;

  BanjaxLogProcessorInterface* log_processor_interface;

public:
  const std::string BOTBANGER_LOG;

  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

     @param log_processor_interface a pointer to log_processor interface which
                                    provides interactions with log_processor
                                    if it is not NULL, bot_sniffer will submit
                                    logs to log_pocessor through this object

  */
 BotSniffer(const std::string& banjax_dir, const libconfig::Setting& main_root, BanjaxLogProcessorInterface* global_log_processor_interface = NULL)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, BOT_SNIFFER_FILTER_ID, BOT_SNIFFER_FILTER_NAME), context (1), zmqsock (context, ZMQ_PUB), botbanger_server("*"), 
    log_processor_interface(global_log_processor_interface), 
    BOTBANGER_LOG("botbanger_log")
  {
    queued_tasks[HTTP_CLOSE] = static_cast<FilterTaskFunction>(&BotSniffer::execute);
    load_config(main_root[BANJAX_FILTER_NAME]);
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them
  */
  virtual void load_config(libconfig::Setting& cfg);

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for url, host and user agent
     later we can ask more if it is needed
   */
  uint64_t requested_info() { return 
      TransactionMuncher::IP                 |
      TransactionMuncher::UA                 |
      TransactionMuncher::URL                |
      TransactionMuncher::PROTOCOL;           
      };

  /**
     needs to be overriden by the filter
     It returns a list of ORed flags which tells banjax which fields
     should be retrieved from the response and handed to the filter.
  */
  virtual uint64_t response_info() {return 
      TransactionMuncher::STATUS             |
      TransactionMuncher::CONTENT_LENGTH;
  }

  /**
     overloaded execute to execute the filter, it assemble the
     parts to make ats record and then send them over a zmq socket
     to botbanger
  */
  FilterResponse execute(const TransactionParts& transaction_parts);

  /**
     we do not overload generate_respons cause we have no response to generate
  */

};

#endif /* bot_sniffer.h */
