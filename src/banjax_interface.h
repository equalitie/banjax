#pragma once
#include "challenger.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

/* XXX TODO FIXME
 * i (joe) have no idea why this is needed.
 * wasted a day looking at an incomprehensible linking error.
 * this makes it go away, that's all i know.
*/

class BanjaxInterface {
    public:
        BanjaxInterface() {};
        virtual ~BanjaxInterface() {};
        virtual const std::string& get_host_name() = 0;
        virtual void kafka_message_consume(const json& message) = 0;
        virtual std::shared_ptr<Challenger> get_challenger() = 0;
        virtual int report_pass_or_failure(const std::string& site, const std::string& ip, bool passed) = 0;
        virtual int report_ip_banned(const std::string& site, const std::string& ip) = 0;
        virtual int report_if_ip_in_database(const std::string& ip) = 0;
};
