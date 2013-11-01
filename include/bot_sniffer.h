/*
*  A subfilter of banjax that publishes all log information for each
*  request in a zmq socket so botbanger-python can grab them and 
*  does svm computation with them
*
*  Vmon: Oct 2013: Initial version.
*/
#ifndef BOT_SNIFFER_H
#define BOT_SNIFFER_H

#include "banjax_filter.h"

class BotSniffer : public BanjaxFilter
{
 protected:

  //socket stuff
  zmq::context_t context;
  zmq::socket_t socket;

public:
  /**
     receives the config object need to read the ip list,
     subsequently it reads all the ips

  */
 BotSniffer(const std::string& banjax_dir, const libconfig::Setting& main_root)
   :BanjaxFilter::BanjaxFilter(banjax_dir, main_root, BOT_SNIFFER_FILTER_ID, BOT_SNIFFER_FILTER_NAME)
  {
    QueuedTasks[HTTP_CLOSE] = static_cast<FilterTask>(&RegexManager::execute);
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
      TransactionMuncher::IP       |
      TransactionMuncher::UA       |
      TransactionMuncher::URL      |
      TransactionMuncher::SIZE     |
      TransactionMuncher::PROTOCOL |
      TransactionMuncher::STATUS   |
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
