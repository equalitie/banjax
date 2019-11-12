/*
 * functions to communicate with swabber to ban the ips detected as botnet
 *
 * Copyright (c) eQualit.ie 2013 under GNU AGPL V3.0 or later
 *
 * Vmon: June 2013
 */

#include <sys/time.h>

#include "swabber_interface.h"
#include "defer.h"
#include "print.h"

using namespace std;

//Swabber connection detail
const string SwabberInterface::SWABBER_SERVER = "*";
const string SwabberInterface::SWABBER_PORT =  "22620";
const long SwabberInterface::SWABBER_GRACE_PERIOD = 0;
const string SwabberInterface::SWABBER_BAN = "swabber_bans";

const unsigned int SwabberInterface::SWABBER_MAX_MSG_SIZE = 1024;

const string SwabberInterface::BAN_IP_LOG("/usr/local/trafficserver/logs/ban_ip_list.log");

SwabberInterface::SwabberInterface(IPDatabase* global_ip_db,
    std::unique_ptr<Socket> s)
  :ban_ip_list(BAN_IP_LOG.c_str(), ios::out | ios::app),
   swabber_mutex(TSMutexCreate()),
   ip_database(global_ip_db),
   swabber_server(SWABBER_SERVER),
   swabber_port(SWABBER_PORT),
   grace_period(SWABBER_GRACE_PERIOD)
{
  if (s) {
    socket = move(s);
  } else {
    socket.reset(new Socket());
  }
}

/**
  reads the grace period and swabber listening port and bind to it
 */
void
SwabberInterface::load_config(FilterConfig& swabber_config)
{
  //reset to default
  swabber_server = SWABBER_SERVER;
  swabber_port = SWABBER_PORT;
  grace_period = SWABBER_GRACE_PERIOD;

  print::debug("Loading swabber interface conf");

  for(auto& cur_node_p : swabber_config.config_node_list) {
    try {
      // Look for white_listed_ips, they all should have been merged by the
      // yaml merger
      auto cur_node = cur_node_p->second;
      if (cur_node["grace_period"])
        grace_period = cur_node["grace_period"].as<long>();

      if (cur_node["port"])
        swabber_port = cur_node["port"].as<string>();

      if (cur_node["server"])
        swabber_server = cur_node["server"].as<string>();

    } catch(YAML::RepresentationException& e) {
      print::debug("Error loading swabber config: ", e.what());
      throw;
    }
  }

  local_endpoint = "tcp://" + swabber_server + ":" + swabber_port;

  if (socket->is_bound() && socket->bound_endpoint != local_endpoint) {
    socket.reset(new Socket());
  }

  if (!socket->bind(local_endpoint)) {
    print::debug("Swabber: Failed to bind (we'll try to bind again later)");
  }

  print::debug("Done loading swabber conf");
}

/**
   Asks Swabber to ban the bot ip

   @param bot_ip the ip address to be banned
   @param banning_reason the reason for the request to be stored in the log
*/
void
SwabberInterface::ban(string bot_ip, std::string banning_reason)
{
  timeval cur_time; gettimeofday(&cur_time, NULL);
  char time_buffer[80];
  time_t rawtime;

  /* we are waiting for grace period before banning for inteligent gathering purpose */
  if (grace_period > 0) { //if there is no grace then ignore these steps

    std::pair<bool,FilterState> cur_ip_state(ip_database->get_ip_state(bot_ip, SWABBER_INTERFACE_ID));

    /* If we failed to query the database then just don't report to swabber */
    if (cur_ip_state.first == false) {
      /* If it is zero size we set it to the current time */
      print::debug("Not reporting to swabber due to failure of aquiring ip db lock");
      return;
    }

    if (cur_ip_state.second.size() == 0) {
      //recording the first request for banning
      cur_ip_state.second.resize(1);
      cur_ip_state.second[0] = cur_time.tv_sec;

      ip_database->set_ip_state(bot_ip, SWABBER_INTERFACE_ID, cur_ip_state.second);

      /* Format the time for log */
      time(&rawtime);
      tm* timeinfo = std::gmtime(&rawtime);
      strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

      ban_ip_list << bot_ip << ", " << "[" << time_buffer << "], " << banning_reason << ", flagged" <<endl;
    }

    /* only ban if the grace period is passed */
    if ((cur_time.tv_sec - cur_ip_state.second[0]) < grace_period) {
      print::debug("Not reporting to swabber cause grace period has not passed yet");
      return;
    }
  }

  //grace period pass or no grace period
  /* Format the time for log */
  time(&rawtime);
  tm* timeinfo = std::gmtime(&rawtime);
  strftime(time_buffer,80,"%Y-%m-%dT%H:%M:%S",timeinfo);

  zmq::message_t ban_request(SWABBER_BAN.size());
  memcpy((void*)ban_request.data(), SWABBER_BAN.c_str(), SWABBER_BAN.size());

  zmq::message_t ip_to_ban(bot_ip.size());
  memcpy((void*)ip_to_ban.data(), bot_ip.c_str(), bot_ip.size());

  print::debug("Locking the swabber socket...");

  if (TSMutexLockTry(swabber_mutex) != TS_SUCCESS) {
    print::debug("Unable to get lock on the swabber socket");
    return;
  }

  {
    auto on_scope_exit = defer([&] { TSMutexUnlock(swabber_mutex); });

    if (!socket) {
      // If we're here, the socket has been released and thus this swabber
      // interface has been deactivated.
      return;
    }

    if (!socket->is_bound()) {
      if (socket->bind(local_endpoint)) {
        print::debug("Swabber: successuflly bound to ", local_endpoint);
      } else {
        print::debug("Swabber: failed to bind to ", local_endpoint);
        return;
      }
    }

    socket->s.send(ban_request, ZMQ_SNDMORE);
    socket->s.send(ip_to_ban);

    ban_ip_list << bot_ip << ", " << "[" << time_buffer << "], " << banning_reason << ", banned" << endl;
  }

  ip_database->drop_ip(bot_ip);
}

std::unique_ptr<SwabberInterface::Socket> SwabberInterface::release_socket()
{
  TSMutexLock(swabber_mutex);
  auto s = std::move(socket);
  TSMutexUnlock(swabber_mutex);
  return s;
}
