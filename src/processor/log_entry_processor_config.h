#ifndef LOG_ENTRY_PROCESSOR_CONFIG_H
#define LOG_ENTRY_PROCESSOR_CONFIG_H
#include <libconfig.h++>
#include "utils/string_dumper.h"

#include "log_entry_processor.h"

using namespace libconfig;
using namespace std;

/* a static class to configure a logEntryProcessor from configuration */
class LogEntryProcessorConfig
{
	static bool setupBotBanger(LogEntryProcessor *lp,Setting *botBangerSettings,vector<string> &warnings, int &consoleSettings);
	static bool setupHitMiss(LogEntryProcessor *lp,Setting *hitMissSettings,vector<string> &warnings, int &consoleSettings);
public:
	/* configure a LogProcessor from a configfile, consolesettings is a bitmask of LogEntryTrace entries */
	static bool ReadFromSettings(LogEntryProcessor *lp,string configfile,vector<string> &warnings, int consoleSettings=0);
	/* configure a LogProcessor from a libconfig::Config, consolesettings is a bitmask of LogEntryTrace entries */
	static bool ReadFromSettings(LogEntryProcessor *lp,Config *configuration,vector<string> &warnings,int consoleSettings);
	/* configure a LogProcessor from a libconfig::Setting, consolesettings is a bitmask of LogEntryTrace entries */
	static bool ReadFromSettings(LogEntryProcessor *lp,Setting *configuration,vector<string> &warnings,int consoleSettings);
	/* configure a LogProcessor from a libconfig::Settings, consolesettings is a bitmask of LogEntryTrace entries */
	static bool ReadFromSettings(LogEntryProcessor *lp,Setting *hitMissSettings,Setting *botBangerSettings,vector<string> &warnings, int consoleSettings=0);
};



#endif
