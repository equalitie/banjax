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

#include <yaml-cpp/yaml.h>
#include "banjax_filter.h"

class BotSniffer : public BanjaxFilter
{
 protected:

  //socket stuff
  zmq::context_t context;
  zmq::socket_t zmqsock;

  unsigned int botbanger_port;
  std::string botbanger_server;

  //lock for writing into the socket
  //TODO:: this is a temp measure probably
  //We should move to fifo queue or something
  TSMutex bot_sniffer_mutex;


public:
  const std::string BOTBANGER_LOG;

  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

  */
 BotSniffer(const std::string& banjax_dir, YAML::Node& main_root)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, BOT_SNIFFER_FILTER_ID, BOT_SNIFFER_FILTER_NAME), 
    context (1), zmqsock (context, ZMQ_PUB), 
    botbanger_server("*"), 
    bot_sniffer_mutex(TSMutexCreate()),
    BOTBANGER_LOG("botbanger_log")
  {
    queued_tasks[HTTP_CLOSE] = static_cast<FilterTaskFunction>(&BotSniffer::execute);
    load_config(main_root);
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them
  */
  virtual void load_config(YAML::Node& cfg);

  /**
     Overloaded to tell banjax that we need url, host, ua and ip
     for banning
     At this point we only asks for url, host and user agent
     later we can ask more if it is needed
   */
  uint64_t requested_info() { return 
      TransactionMuncher::IP                 |
      TransactionMuncher::UA                 |
      TransactionMuncher::URL_WITH_HOST      |
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
