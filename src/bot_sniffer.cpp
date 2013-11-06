/*
*  A subfilter of banjax that publishes all log information for each
*  request in a zmq socket so botbanger-python can grab them and 
*  does svm computation with them
*
*  Vmon: Oct 2013: Initial version.
*/
#include <string>
#include <list>
#include <vector>
#include <ctime>

#include <stdio.h>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <ts/ts.h>

using namespace std;

#include "banjax_common.h"
#include "util.h"
#include "bot_sniffer.h"
#include "ip_database.h" 

/**
  Reads botbanger's port from the config
 */
void
BotSniffer::load_config(libconfig::Setting& cfg)
{
   try
   {
     botbanger_port = cfg["botbanger_port"];
     
   }
   catch(const libconfig::SettingNotFoundException &nfex)
     {
       // Ignore.
     }

   TSDebug(BANJAX_PLUGIN_NAME, "Connecting to botbanger server...");
   zmqsock.bind(("tcp://"+botbanger_server +":"+to_string(botbanger_port)).c_str());
 
}

FilterResponse BotSniffer::execute(const TransactionParts& transaction_parts)
{

  /*TSDebug("banjax", "sending log to botbanger");
  TSDebug("banjax", "ip = %s", cd->client_ip);
  TSDebug("banjax", "url = %s", cd->url);
  TSDebug("banjax", "ua = %s", cd->ua);
  TSDebug("banjax", "size = %d", (int) cd->request_len);
  TSDebug("banjax", "status = %d", stat);
  TSDebug("banjax", "protocol = %s", cd->protocol);
  TSDebug("banjax", "hit = %d", cd->hit);*/

  std::time_t rawtime;
  std::time(&rawtime);
  std::tm* timeinfo = std::gmtime(&rawtime);

  char time_buffer[80];
  std::strftime(time_buffer,80,"%Y-%m-%d-%H-%M-%S",timeinfo);
  
  send_zmq_mess(zmqsock, BOTBANGER_LOG, true);

  send_zmq_mess(zmqsock, transaction_parts.at(TransactionMuncher::IP), true);
  send_zmq_mess(zmqsock, time_buffer, true);
  send_zmq_mess(zmqsock, transaction_parts.at(TransactionMuncher::URL), true);
  send_zmq_mess(zmqsock, transaction_parts.at(TransactionMuncher::PROTOCOL), true);
  send_zmq_mess(zmqsock, transaction_parts.at(TransactionMuncher::STATUS), true);
  send_zmq_mess(zmqsock, transaction_parts.at(TransactionMuncher::CONTENT_LENGTH), true);
  send_zmq_mess(zmqsock, transaction_parts.count(TransactionMuncher::MISS) ? "MISS" : "HIT");

  //botbanger_interface.add_log(transaction_parts[IP], cd->url, cd->protocol, stat, (long) cd->request_len, cd->ua, cd->hit);
  //botbanger_interface.add_log(cd->client_ip, time_str, cd->url, protocol, status, size, cd->ua, hit);
  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
                    
}
