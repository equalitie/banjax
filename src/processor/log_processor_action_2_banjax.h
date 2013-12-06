
#include "log_entry.h"
#include "log_entry_processor.h"

class LogProcessorAction2Banjax:public LogEntryProcessorEventListener
{
	Banjax *_banjax;
public:
	LogProcessorAction2Banjax(Banjax *banjax):
		LogEntryProcessorEventListener(),
		_banjax(banjax)
	{

	}
	virtual void OnLogEntryStart(LogEntry *le)
	{
		UNUSED(le);
	}
	virtual void OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)
	{
		UNUSED(le);
		UNUSED(output);
		if (!output.empty())
		{
			struct tm time;
			gmtime_r(&le->endTime,&time);
			TSDebug(BANJAX_PLUGIN_NAME,"LPD: %02d:%02d:%02d\t%s",time.tm_hour,time.tm_min,time.tm_sec,output.c_str());
		}

		for(auto i=actionList.begin();i!=actionList.end();i++)
		{
			externalAction &ea=(*i);
			TSDebug(BANJAX_PLUGIN_NAME,"LPH: %s/%s/%s/%s",ea.action.c_str(),ea.argument1.c_str(),ea.argument2.c_str(),ea.argument3.c_str());

			// running the actions in banjax
			if ((*i).action=="blockip")
			{
				_banjax->BanIP((*i).argument1,string("banned from botbanger, model:")+(*i).argument2);
			}
			if ((*i).action=="captcha")
			{
				// how can we start a captcha for a host???
				//_banjax->StartCaptchaForHost((*i).argument1);
			}
		}

	}

};
