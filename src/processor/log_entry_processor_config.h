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
public:
	static bool ReadFromSettings(LogEntryProcessor *lp,string configfile,vector<string> &warnings, int consoleSettings=0);
	static bool ReadFromSettings(LogEntryProcessor *lp,Config *configuration,vector<string> &warnings,int consoleSettings);
	static bool ReadFromSettings(LogEntryProcessor *lp,Setting *hitMissSettings,Setting *botBangerSettings,vector<string> &warnings, int consoleSettings=0);
};



#endif
