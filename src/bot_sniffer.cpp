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
#include "ip_db.h"

#include "base64.h"

#define VALID_OR_EMPTY(validity, part) ((validity & part) ? Base64::Encode(transaction_parts.at(part)) : "")
/**
  Reads botbanger's port from the config
 */
void
BotSniffer::load_config()
{
  try {
    botbanger_port = cfg["botbanger_port"].as<unsigned int>();

    string passphrase = cfg["key"].as<std::string>();

    SHA256((const unsigned char*)passphrase.c_str(), passphrase.length(), encryption_key);
  }
  catch(YAML::RepresentationException& e) {
    TSDebug(BANJAX_PLUGIN_NAME, "Error loading bot sniffer conf [%s].", e.what());
    throw;
  }

  string new_binding_string  = "tcp://"+botbanger_server +":"+to_string(botbanger_port);
  if (!p_zmqsock) { //we haven't got connected to anywhere before
    p_zmqsock = new zmq::socket_t(context, ZMQ_PUB),
    p_zmqsock->bind(new_binding_string.c_str());
    //just get connected
  } else if (new_binding_string != _binding_string) { //we are getting connected to a new end point just drop the last point and connect to new point
    TSDebug(BANJAX_PLUGIN_NAME, "unbinding from %s",  _binding_string.c_str());
    delete p_zmqsock;
    TSDebug(BANJAX_PLUGIN_NAME,"connecting to %s...",  new_binding_string.c_str());
    p_zmqsock = new zmq::socket_t(context, ZMQ_PUB),
    p_zmqsock->bind(new_binding_string.c_str());
    _binding_string = new_binding_string;
  }; //else  {re-connecting to the same point do nothing} //unbind bind doesn't work

  TSDebug(BANJAX_PLUGIN_NAME, "Done connecting to botbanger server...");
}

void BotSniffer::on_http_close(const TransactionParts& transaction_parts)
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
  std::strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

  static const string b64_hit = Base64::Encode("HIT");
  static const string b64_miss = Base64::Encode("MISS");

  uint64_t* cur_validity = (uint64_t*)transaction_parts.at(TransactionMuncher::VALIDITY_STAT).data();

  //TODO: This is a temp solution, we can't afford losing logs due 
  //to failing acquiring the lock
  //really?
  TSDebug(BANJAX_PLUGIN_NAME, "locking the botsniffer socket...");
  if (TSMutexLockTry(bot_sniffer_mutex) == TS_SUCCESS) {
    
    string plaintext_log;
    send_zmq_mess(*p_zmqsock, BOTBANGER_LOG, true);

    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::IP), true);
    plaintext_log += VALID_OR_EMPTY(*cur_validity, TransactionMuncher::IP);

    //send_zmq_mess(zmqsock, time_buffer, true);
    plaintext_log += "," + Base64::Encode(string(time_buffer));

    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::URL_WITH_HOST), true);
    plaintext_log += "," +  VALID_OR_EMPTY(*cur_validity, TransactionMuncher::URL_WITH_HOST);

    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::PROTOCOL), true);
    plaintext_log += "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::PROTOCOL);
      
    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::STATUS), true);
    plaintext_log += "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::STATUS);

    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::CONTENT_LENGTH), true);
    plaintext_log += "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::CONTENT_LENGTH);

    //send_zmq_mess(zmqsock, VALID_OR_EMPTY(*cur_validity, TransactionMuncher::UA), true);
    plaintext_log += "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::UA);

    //send_zmq_mess(zmqsock, transaction_parts.count(TransactionMuncher::MISS) ? "MISS" : "HIT");
    std::string hit_mis_str = (transaction_parts.count(TransactionMuncher::MISS) ? b64_hit : b64_miss);
    plaintext_log += "," + hit_mis_str;

    send_zmq_encrypted_message(*p_zmqsock, plaintext_log, encryption_key);

    TSMutexUnlock(bot_sniffer_mutex);
  }
  //botbanger_interface.add_log(transaction_parts[IP], cd->url, cd->protocol, stat, (long) cd->request_len, cd->ua, cd->hit);
  //botbanger_interface.add_log(cd->client_ip, time_str, cd->url, protocol, status, size, cd->ua, hit);
}


