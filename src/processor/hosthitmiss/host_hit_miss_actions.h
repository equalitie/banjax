#pragma once
#include <time.h>
#include "host_hit_miss_aggregator.h"
#include "../utils/string_dumper.h"
#include <iostream>

struct HostHitMissConfigLine
{
	int requestLower;
	int requestUpper;
	int runTime;
	double ratioLower;
	double rationUpper;
	string host;
	string action;
};

struct HostConfig
{
	string currentAction;
	time_t expires;
	vector<HostHitMissConfigLine *> configLines;
};

class HostHitMissActions:public HostHitMissEventListener
{
	map<string,HostConfig> configuration;
public:
	void AddConfigLine(string host,string action,int requestLower,int requestUpper,double ratioLower,double ratioUpper,int runTime)
	{
		HostHitMissConfigLine *cl=new HostHitMissConfigLine();
		cl->host=host;
		cl->action=action;
		cl->requestLower=requestLower;
		cl->requestUpper=requestUpper;
		cl->ratioLower=ratioLower;
		cl->rationUpper=ratioUpper;
		cl->runTime=runTime;
		AddConfigLine(cl);
	}
	void AddConfigLine(struct HostHitMissConfigLine *l)
	{
		HostConfig &hostConfig=configuration[l->host];
		hostConfig.expires=0;
		hostConfig.currentAction="";
		hostConfig.configLines.push_back(l);
	}

	void OnHostHitMissEvent(char *host,HitMissRange *hmrange)
	{
		//string host=string(host);
		auto key=configuration.find(host);
		if (key==configuration.end())
			return;

		HostConfig &hostConfig=key->second;
		
		if (hmrange->from>hostConfig.expires && hmrange->rangeTotal==1) // check only first request in range
		{
			for(auto i=hostConfig.configLines.begin();i!=hostConfig.configLines.end();i++)
			{
				auto cl=(*i);
				if (hmrange->periodTotal>=cl->requestLower && hmrange->periodTotal<=cl->requestUpper &&
					hmrange->periodRatio>=cl->ratioLower && hmrange->periodRatio<=cl->rationUpper)
				{
					string currentAction=hostConfig.currentAction;
					hostConfig.currentAction=cl->action;
					hostConfig.expires=hmrange->from+cl->runTime;
					string sHost=string(host);
					ScheduleAction(hmrange, sHost,cl->action,currentAction);
					break;
				}
			}
		}
	}
	virtual void ScheduleAction(HitMissRange *hmr,string &host,string &action,string &currentaction)=0;
	virtual ~HostHitMissActions()
	{

	}

};


