/*
*  A subfilter of banjax that publishes all log information for each
*  request in a zmq socket so botbanger-python can grab them and 
*  does svm computation with them
*
*  Vmon: Oct 2013: Initial version.
*/
#include <string>
#include <ctime>
#include <stdio.h>
#include <ts/ts.h>

#include "banjax_common.h"
#include "util.h"
#include "bot_sniffer.h"
#include "base64.h"
#include "print.h"

using namespace std;

#define VALID_OR_EMPTY(validity, part) ((validity & part) ? Base64::Encode(transaction_parts.at(part)) : "")
/**
  Reads botbanger's port from the config
 */
void
BotSniffer::load_config()
{
  print::debug("BotSniffer::load_config()");

  try {
    botbanger_port = cfg["botbanger_port"].as<unsigned int>();
    string passphrase = cfg["key"].as<std::string>();
    SHA256((const unsigned char*)passphrase.c_str(), passphrase.length(), encryption_key);
  }
  catch(YAML::RepresentationException& e) {
    print::debug("Error loading bot sniffer conf [%s].", e.what());
    throw;
  }

  string local_endpoint  = "tcp://" + botbanger_server + ":" + to_string(botbanger_port);

  if (!socket) {
    socket.reset(new Socket);
    socket->bind(local_endpoint);
  } else if (local_endpoint != socket->local_endpoint()) {
    socket.reset(new Socket);
    socket->bind(local_endpoint);
  }

  print::debug("Done connecting to botbanger server...");
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

  print::debug("Locking the botsniffer socket...");

  if (TSMutexLockTry(bot_sniffer_mutex) == TS_SUCCESS) {
    send_zmq_mess(socket->handle(), BOTBANGER_LOG, true);

    std::string hit_mis_str = (transaction_parts.count(TransactionMuncher::MISS) ? b64_hit : b64_miss);

    string plaintext_log
      = VALID_OR_EMPTY(*cur_validity, TransactionMuncher::IP)
      + "," + Base64::Encode(time_buffer)
      + "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::URL_WITH_HOST)
      + "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::PROTOCOL)
      + "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::STATUS)
      + "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::CONTENT_LENGTH)
      + "," + VALID_OR_EMPTY(*cur_validity, TransactionMuncher::UA)
      + "," + hit_mis_str;

    send_zmq_encrypted_message(socket->handle(), plaintext_log, encryption_key);

    TSMutexUnlock(bot_sniffer_mutex);
  }
  //botbanger_interface.add_log(transaction_parts[IP], cd->url, cd->protocol, stat, (long) cd->request_len, cd->ua, cd->hit);
  //botbanger_interface.add_log(cd->client_ip, time_str, cd->url, protocol, status, size, cd->ua, hit);
}

