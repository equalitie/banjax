#include "log_entry.h"
#include <string>
#include <iostream>
#include <ios>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "utils/string_utils.h"
#include "log_entry_processor.h"



/*#include "utils/strptime.h"
#include "utils/timegm.h"*/
using namespace std;


bool ParseField(char **line,char **start_ptr,char **end_ptr)
{
	char endchar=0;
	char *start=NULL;
	char *end=NULL;
	while(1)
	{
		if (**line=='[' && !endchar)
		{
			endchar=']';			
		}
		else
		if (**line=='\"' && !endchar)
		{
			endchar='\"';
		}
		else
		if (**line==endchar && start)
		{
			end=*line;
			(*line)++;
			break;
		}
		else	
		if (**line==0)
		{
			if (endchar==' ' && start) 
				end=*line;
			break;
		}
		else
		if (!start)
		{
			if (!endchar) endchar=' ';
			start=*line;
		}
		(*line)++;
	}
	// forward pointer to next field
	while (**line==' ') (*line)++;
	if (start && end)
	{
		*start_ptr=start;
		*end_ptr=end;
		return true;
	}
	return false;




}

void strlwr(char *s)
{
	for (;*s;s++)
	{
		*s=tolower(*s);
	}
}


int indexOf(const char **values,int length,char *search)
{
	for(int c=0;c<length;c++)
	{
		if (strcmp(values[c],search)==0) return c;
	}
	return -1;
}
vector<char *> values;
void ParseLogLine(LogEntry &le,char *line)
{
	if (values.capacity()<20) {values.reserve(20);}
	values.clear();
	const char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	char *start,*end;
	
	
	while(ParseField(&line,&start,&end))
	{
		*end=0;
		values.push_back(start);
	}
	if (values.size()<14) 
		return;

	//return;
	strlcpy(le.useraddress,values[0],40);
	int year,day,hour,minute,second;
	char month[4];
	char timezone[5];
	

	sscanf(values[2],"%d/%03s/%d:%d:%d:%d -%04s",&day,month,&year,&hour,&minute,&second,timezone);
	struct tm time;
	memset(&time,0,sizeof(struct tm));
	time.tm_hour=hour;
	time.tm_year=year-1900;
	time.tm_sec=second;
	time.tm_mon=indexOf(months,12,month);
	time.tm_mday=day;
	time.tm_min=minute;

	time_t s=timegm(&time);

	le.endTime=s;

	
	sprintf(le.hostname,"%.49s",values[5]);
	le.httpCode=atoi(values[6]);
	le.payloadsize=atoi(values[7]); 
	strlcpy(le.contenttype,values[10],80); // lcase
	strlwr(le.contenttype);
	//strcpy(le.cacheLookupStatus,80,values[9]); // TCP_HIT, TCP_MISS
	if (strcmp(values[9],"TCP_HIT")==0)
		le.cacheLookupStatus=CacheLookupStatus::Hit;
	else
	if (strcmp(values[9],"TCP_MISS")==0)
		le.cacheLookupStatus=CacheLookupStatus::Miss;
	else
		le.cacheLookupStatus=CacheLookupStatus::Error;		

	char *path=values[13];
	// path should be on 3rd slash of the url
	for (int i=0;i<3 && path;i++,path=strchr(path+1,'/'));
	if (path)	
		sprintf(le.url,"%.399s",path); // complete url, needs to be fixed
	else
		le.url[0]=0;
	

	sprintf(le.userAgent,"%.119s",values[8]);
	
	//le.contenttype=

	

	
	

	
}



class VerboseLogger:public BotBangerEventListener
{
public:
	VerboseLogger()
	{
	}
	virtual void OnEvictEvent(string key)
	{
		cout << key << " evicted" << endl;
	}
	virtual void OnFeatureEvent(char *key,double *features,int numFeatures)
	{
		UNUSED(key);
		UNUSED(features);
		UNUSED(numFeatures);
	}
};

int main(int argc, char* argv[])
{
	string configfile;
	string logfile;
	string modelfilename;

	bool showhelp=false;	
	//enum traceType {none,HitMiss=1,Features=2,Model=4,Output=8,Actions=16,BotBanger=32,LogEntries=64,Verbose=128};
	int consoleSettings=ConsoleMode;


	for (int n=1;n<argc;n++)
	{
		bool islast=n==(argc-1);
		string val=string(argv[n]);
		
		if (val=="--config")
		{
			if (islast) 
				showhelp=true;
			else
			{
				configfile=string(argv[n+1]);
				n++;
			}
		}
		else
		if (val=="--hitmissratio")
		{
			consoleSettings|=TraceHitMissRatio;
		}
		else
		if (val=="--hitmissaction")
		{
			consoleSettings|=TraceHitMissAction;
		}
		else
		if (val=="--bbfeatures")
		{
			consoleSettings|=TraceBotBangerFeatures;

		}
		else
		if (val=="--bbmodelvalues")
		{
			consoleSettings|=TraceBotBangerModelValues;
		}
		else
		if (val=="--bbmodelinputs")
		{
			consoleSettings|=TraceBotBangerModelInputs;
		}
		else
		if (val=="--bbaction")
		{
			consoleSettings|=TraceBotBangerAction;
		}
		else
		if (val=="--logentries")
		{
			consoleSettings|=TraceLogEntries;
		}
		else
		if (val=="--all")
		{
			consoleSettings=0xffff;
		}
		else // should be logfile
		{
			logfile=val;
		}
	}
	if (configfile.empty())
	{
		showhelp=true;
		cout << "Need config file" << endl;
	}

	if (showhelp)
	{
		cout << "usage:"<< argv[0] << " [options] [logfile]" << endl
			 << "options:" << endl
			 << "--logentries                     show logentry (time/ip/url)" << endl
			 << "--hitmissratio                   show hitmiss ratio" << endl
			 << "--hitmissaction                  show hitmiss action" << endl
			 << "--bbfeatures                     show botbanger features" << endl
			 << "--bbmodelinputs                  show botbanger normalized inputs" << endl
			 << "--bbmodelvalues                  show botbanger model predicted values" <<endl
			 << "--bbaction                       show botbanger actions" <<endl
			 << "--all                            show all " << endl
			 << "--config [filename]              read config file" << endl
			 //<< "--traceoutput                    trace configuration output" << endl
			 << "--config [configfile]            use configfile for the configuration" << endl;
	}
	else
	{
		LogEntryProcessor processor;
		ifstream lf;
		//lf.set_rdbuf(
		string output;
		lf.open(logfile.c_str(),std::ifstream::in);
		LogEntry le;
		memset(&le,0,sizeof(LogEntry));
		int linenr=0;
		
		vector<string> messages;

		if (!LogEntryProcessorConfig::ReadFromSettings(&processor,configfile,messages, consoleSettings ) || messages.size())
		{
			std::cout << "Configuration errors" << endl;
			for(auto i=messages.begin();i!=messages.end();i++)
			{
				std::cout << (*i) << endl;
			}
			return 0;
		}

		time_t start;
		time_t end;
		time(&start);
		output.reserve(4096);
		processor.Start(true);
		vector<LogEntry> logentries;

		while(lf.good())
		{
			char line[91000];
			lf.getline(line,91000);
			ParseLogLine(le,line);
			linenr++;

			/*if (trace&traceType::LogEntries)
			{
				output.append(le.useraddress);
				output.append("\t");
				output.append(le.hostname);
				output.append("\t");
				output.append(le.url);
				output.append("\t");
				//output.append(le.httpCode);
				output.append("\t");
				//output.append(le.payloadsize);

			}*/
			//cout << linenr << endl;
			logentries.push_back(le);
			//processor.AddLogEntry(&le);
			
		}
		for (auto i=logentries.begin();i!=logentries.end();i++)
		{
			processor.AddLogEntry(&(*i));
		}
		processor.Stop();

		time(&end);
		printf("runtime:%ld\n",end-start);

		cout << linenr << endl;




		
	}

		 
}
