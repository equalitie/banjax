/*
*  A subfilter of banjax that publishes all log information for each
*  request in a zmq socket so botbanger-python can grab them and 
*  does svm computation with them
*
*  Vmon: Oct 2013: Initial version.
*/
#ifndef BOT_SNIFFER_H
#define BOT_SNIFFER_H

#include <openssl/sha.h>
#include "banjax_filter.h"
#include "socket.h"

class BotSniffer final : public BanjaxFilter
{
private:
  std::unique_ptr<Socket> socket;

  unsigned int botbanger_port;
  std::string botbanger_server;

  TSMutex mutex;

  //encryption key
  uint8_t encryption_key[SHA256_DIGEST_LENGTH];

  std::string _local_endpoint;

public:
  /**
   *  receives the config object need to read the ip list,
   *  subsequently it reads all the ips
   */
 BotSniffer( const YAML::Node& cfg
           , std::unique_ptr<Socket> socket = nullptr)
   : BanjaxFilter::BanjaxFilter(BOT_SNIFFER_FILTER_ID, BOT_SNIFFER_FILTER_NAME)
   , socket(std::move(socket))
   , botbanger_server("*")
    /* When assigning a local address to a socket using zmq_bind() with the tcp
       transport, the endpoint shall be interpreted as an interface followed by
       a colon and the TCP port number to use.
       An interface may be specified by either of the following:

       The wild-card *, meaning all available interfaces.
       The primary IPv4 address assigned to the interface, in its numeric representation.
       The interface name as defined by the operating system. */
   , mutex(TSMutexCreate())
  {
    queued_tasks[HTTP_CLOSE] = this;
    load_config(cfg);
  }

  /**
    Overload of the load config
    reads all the regular expressions from the database.
    and compile them
  */
  void load_config(const YAML::Node&);

  /**
   *  Overloaded to tell banjax that we need url, host, ua and ip
   *  for banning
   *  At this point we only asks for url, host and user agent
   *  later we can ask more if it is needed
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

  FilterResponse on_http_request(const TransactionParts& transaction_parts) override
  {
    return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
  }

  void on_http_close(const TransactionParts& transaction_parts) override;

  // Return the socket we're using. This will effectively
  // disable this bot sniffer.
  std::unique_ptr<Socket> release_socket();

  /**
     we do not overload generate_respons cause we have no response to generate
  */
};

#endif /* bot_sniffer.h */
