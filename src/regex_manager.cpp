/*
 * Functions deal with the regex banning.
 * 
 * Copyright (c) eQualit.ie 2013 under GNU AGPL v3.0 or later
 *
 *  Vmon: June 2013, Initial version
 */
#include <string>
#include <list>
#include <vector>
#include <stdio.h>
#include <zmq.hpp>

#include <re2/re2.h> //google re2

#include <ts/ts.h>

using namespace std;

#include "regex_manager.h"

/**
  reads all the regular expressions from the database.
  and compile them
 */
void
RegexManager::load_config(libconfig::Setting& cfg)
{
   try
   {
     const libconfig::Setting &banned_regexes_list = cfg["banned_regexes"];
     unsigned int count = banned_regexes_list.getLength();

     //now we compile all of them and store them for later use
     for(unsigned int i = 0; i < count; i++)
       banning_regexes.push_back(new RE2((const char*)(banned_regexes_list[i])));

   }
   catch(const libconfig::SettingNotFoundException &nfex)
     {
       // Ignore.
     }

}

/**
  applies all regex to an ATS record

  @param ats_record: the full request record including time url agent etc
  @return: 1 match 0 not match < 0 error.
*/
RegexManager::RegexResult
RegexManager::parse_request(string ats_record)
{
  for(list<RE2*>::iterator it=banning_regexes.begin(); it != banning_regexes.end(); it++)
    {
      if (RE2::FullMatch(ats_record, **it))
        {
          TSDebug("banjax", ("requests matched " + (*it)->pattern()).c_str());
          return REGEX_MATCHED;
        }
    }

  //no match
  return REGEX_MISSED;

}

FilterResponse RegexManager::execute(const TransactionParts& transaction_parts)
{

  const string sep(" ");
  TransactionParts ats_record_parts = (TransactionParts) transaction_parts;
  string ats_record =  ats_record_parts[TransactionMuncher::URL] + sep;

  ats_record+= ats_record_parts[TransactionMuncher::HOST] + sep;
  ats_record+= ats_record_parts[TransactionMuncher::UA];

  TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "Examining %s for banned matches", ats_record.c_str());
  RegexResult result = parse_request(ats_record);
  if (result == REGEX_MATCHED) {
    TSDebug(Banjax::BANJAX_PLUGIN_NAME.c_str(), "asking swabber to ban client ip: %s", ats_record_parts[TransactionMuncher::IP].c_str());
    
    //here instead we are calling nosmos's banning client
    swabber_interface.ban(ats_record_parts[TransactionMuncher::IP].c_str());
    return FilterResponse(FilterResponse::I_RESPOND);

  } else if (result != REGEX_MISSED) {
    TSError("Regex failed with error: %d\n", result);
  }

  return FilterResponse(FilterResponse::GO_AHEAD_NO_COMMENT);
                    
}

std::string RegexManager::generate_response(const TransactionParts& transaction_parts, const FilterResponse& response_info)
{
  const string Forbidden_Message("<html><header></header><body>Forbidden</body></html>");
  return Forbidden_Message;
}
