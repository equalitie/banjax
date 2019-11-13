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

#include "util.h"
#include "bot_sniffer.h"
#include "base64.h"
#include "print.h"
#include "defer.h"

using namespace std;

#define VALID_OR_EMPTY(validity, part) ((validity & part) ? Base64::Encode(transaction_parts.at(part)) : "")

static const string LOG = "botbanger_log";

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

  _local_endpoint = "tcp://" + botbanger_server + ":" + to_string(botbanger_port);

  if (!socket || socket->local_endpoint() != _local_endpoint) {
    socket.reset(new Socket);
    socket->bind(_local_endpoint);

    if (!socket->is_bound()) {
      print::debug("BotSniffer: Failed to bind socket in load_config() "
                   "to ", _local_endpoint, " "
                   "we'll try again later");
    }
  }
}

void BotSniffer::on_http_close(const TransactionParts& transaction_parts)
{
  print::debug("BotSniffer: on_http_close(...)");

  std::time_t rawtime;
  std::time(&rawtime);
  std::tm* timeinfo = std::gmtime(&rawtime);

  char time_buffer[80];
  std::strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

  static const string b64_hit = Base64::Encode("HIT");
  static const string b64_miss = Base64::Encode("MISS");

  uint64_t* cur_validity = (uint64_t*)transaction_parts.at(TransactionMuncher::VALIDITY_STAT).data();

  if (TSMutexLockTry(mutex) == TS_SUCCESS) {
    auto on_exit = defer([&] { TSMutexUnlock(mutex); });

    if (!socket) return;

    if (!socket->is_bound()) {
      if (socket->bind(_local_endpoint)) {
        print::debug("BotSniffer: Success binding to ", _local_endpoint);
      } else {
        print::debug("BotSniffer: Failed to bind to ", _local_endpoint);
        return;
      }
    }

    send_zmq_mess(socket->handle(), LOG, true);

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
  }
  //botbanger_interface.add_log(transaction_parts[IP], cd->url, cd->protocol, stat, (long) cd->request_len, cd->ua, cd->hit);
  //botbanger_interface.add_log(cd->client_ip, time_str, cd->url, protocol, status, size, cd->ua, hit);
}

std::unique_ptr<Socket> BotSniffer::release_socket()
{
  TSMutexLock(mutex);
  auto on_exit = defer([&] { TSMutexUnlock(mutex); });
  return std::move(socket);
}
