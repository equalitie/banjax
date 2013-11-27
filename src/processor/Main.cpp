#include "LogEntry.h"
#include <string>
#include <iostream>
#include <ios>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "utils/stringutils.h"
#include "HostHitMissAggregator.h"
#include "HostHitMissFeature.h"
#include "HostHitMissDumper.h"
#include "HostHitMissActions.h" 
#include "BotBangerAggregrator.h"
#include "FeatureAverageTimeBetweenRequests.h"
#include "FeatureHTTPStatusRatio.h"
#include "FeatureAveragePayloadSize.h"
#include "FeatureHtmlToImageRatio.h"
#include "FeatureRequestDepth.h"
#include "FeatureSessionLength.h"
#include "FeatureCyclingUserAgent.h"
#include "FeaturePercentageConsecutiveRequests.h"
#include "FeatureVarianceRequestInterval.h"
#include "FeatureRequestDepthStd.h"
#include "BotBangerModelListener.h"
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
	
	//strptime(values[2],"",&tm);
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
	if (strcmp(values[9],"TCP_HIT"))
		le.cacheLookupStatus=CacheLookupStatus::Hit;
	else
	if (strcmp(values[9],"TCP_MISS"))
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

class FeatureDumper:public BotBangerEventListener, public StringDumper
{
public:
	FeatureDumper(string &output):
		StringDumper(output)
	{
	}
	virtual void OnFeatureEvent(char *key,float *features,int numFeatures)
	{
		UNUSED(key);
		for(int i=0;i<numFeatures;i++)
		{
			char db[100];
			sprintf(db,"%d:%f",i+1,features[i]);
			addToDump(db);
		}
	}
};
class ModelDumper:public BotBangerModelListener,public StringDumper
{
public:
	ModelDumper(string filename,string &output):
		BotBangerModelListener(filename.c_str()),
		StringDumper(output)

	{
	}
	virtual void OnModelValue(char *key,double value)
	{
		UNUSED(key);
		UNUSED(value);
		char buffer[40];

		sprintf(buffer,"%s\t%f",key,value);
		addToDump(buffer);


	}

};

int main(int argc, char* argv[])
{
	string configfile;
	string logfile;
	string modelfilename;
	bool showhelp=false;	
	enum traceType {none,HitMiss=1,Features=2,Model=4,Output=8,Actions=16,BotBanger=32,LogEntries=64};
	int trace=traceType::none;


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
		if (val=="--hitmiss")
		{
			trace|=traceType::HitMiss;
		}
		else
		if (val=="--tracebotbanger")
		{
			trace|=traceType::BotBanger;
		}
		else
		if (val=="--features")
		{
			trace|=traceType::Features|traceType::BotBanger;

		}
		else
		if (val=="--model")
		{
			trace|=traceType::Model|traceType::BotBanger;
			if (n<(argc-1))
			{
				modelfilename=string(argv[n+1]);
				n++;
			}
		}
		else
		if (val=="--traceoutput")
		{
			trace|=traceType::Output;
		}
		else
		if (val=="--actions")
		{
			trace|=traceType::Actions;
		}
		else
		if (val=="--logentry")
		{
			trace|=traceType::LogEntries;
		}
		else // should be logfile
		{
			logfile=val;
		}
	}

	if (showhelp)
	{
		cout << "usage:"<< argv[0] << " [options] [logfile]" << endl
			 << "options:" << endl
			 << "--hitmiss                   run hitmiss" << endl
			 << "--tracediag                      trace diagnose entries" << endl
			 << "--tracebotbanger                 run botbanger" << endl
			 << "--original                       output original lines" <<endl		 
			 << "--model                          output model value" << endl
			 //<< "--traceoutput                    trace configuration output" << endl			 
			 << "--actions                        output actions" << endl
			 << "--config [configfile]            use configfile for the configuration" << endl;
	}
	else
	{
		ifstream lf;
		//lf.set_rdbuf(
		lf.open(logfile.c_str(),std::ifstream::in);
		LogEntry le;
		int linenr=0;
		HostHitMissAggregator hmagg;
		BotBangerAggregator bbag;

		
		/*transparencyinsport.org 10000-9999999 .98-1 captcha 500
		transparencyinsport.org 10000-9999999 .98-1 captcha 500
		transparencyinsport.org 5000-9999999 .98-1 captcha 500
		*/

		
		//hmagg.RegisterEventListener(new HostHitMissLogger());
		string output;
		if (trace&traceType::Actions)
		{
			auto hm=new HostHitMissActionDumper(output);
			hm->AddConfigLine(string("transparencyinsport.org"),string("captcha/high"),20000,9999999,.90f,1.0f,500);
			hm->AddConfigLine(string("transparencyinsport.org"),string("captcha/low"),20000,9999999,.0f,.3f,500);
			hm->AddConfigLine(string("transparencyinsport.org"),string("normal"),0,9999999,0.0f,1.0f,0);
			hmagg.RegisterEventListener(hm);
			// + config
		}		

		if (trace&traceType::BotBanger)
		{
			bbag.RegisterFeature(new FeatureAverageTimeBetweenRequests(),0);//ok
			bbag.RegisterFeature(new FeatureCyclingUserAgent(),1); //nok
	
			bbag.RegisterFeature(new FeatureHtmlToImageRatio(),2);	//ok
			bbag.RegisterFeature(new FeatureVarianceRequestInterval(),3); //nok
			bbag.RegisterFeature(new FeatureAveragePayloadSize,4); // ok
			bbag.RegisterFeature(new FeatureHTTPStatusRatio(),5);
			bbag.RegisterFeature(new FeatureRequestDepth(),6);
			bbag.RegisterFeature(new FeatureRequestDepthStd(),7);
			bbag.RegisterFeature(new FeatureSessionLength(),8);	//ok
			bbag.RegisterFeature(new FeaturePercentageConsecutiveRequests(),9);	
			

			if (trace&traceType::Features)
			{
				bbag.RegisterEventListener(new FeatureDumper(output));
			}
			if (trace&traceType::Model)
			{
				bbag.RegisterEventListener
					(
					new ModelDumper( modelfilename,output));
			}
		}
		
		time_t start;
		time_t end;
		time(&start);
		output.reserve(4096);
		
		while(lf.good())
		{
			char line[91000];
			lf.getline(line,91000);
			ParseLogLine(le,line);
			linenr++;

			if (trace&traceType::LogEntries)
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

			}

			if (trace&traceType::HitMiss)
			{
				hmagg.Aggregate(&le);
			}
			if (trace&traceType::BotBanger)
			{
				bbag.Aggregate(&le);
			}
			
			
			if (!output.empty())
			{
				char timebuf[40];
				struct tm time;
				gmtime_r(&le.endTime,&time);
				sprintf(timebuf,"%02d:%02d:%02dx",time.tm_hour,time.tm_min,time.tm_sec);

				cout << timebuf << "\t" << output << endl;
			}
			output.clear();
		}
		time(&end);
		printf("runtime:%ld\n",end-start);
		hmagg.Dump();
		cout << linenr << endl;




		
	}

		 
}
