
#include <time.h>
#include "host_hit_miss_aggregator.h"
#include "host_hit_miss_actions.h"
#include "../utils/string_dumper.h"
#include <iostream>


/* add host/action match,
 * action is the action to be generated if the match is successful
 * requestLower/requestUpper matches the number of requests in a period
 * ratioLower/ratioUpper matches the ratio hits/total
 * runTime is the runtime for this action, during this runtime, no other actions will be generated
 */
void HostHitMissActions::AddConfigLine(string host,string action,int requestLower,int requestUpper,double ratioLower,double ratioUpper,int runTime)
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

/* add a configuration struct */
void HostHitMissActions::AddConfigLine(struct HostHitMissConfigLine *l)
{
	HostConfig &hostConfig=configuration[l->host];
	hostConfig.expires=0;
	hostConfig.currentAction="";
	hostConfig.configLines.push_back(l);
}
/* catches the HostHitMissEvent and determines the actions to be generated
 */
void HostHitMissActions::OnHostHitMissEvent(char *host,HitMissRange *hmrange)
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
				OnScheduleAction(hmrange, sHost,cl->action,currentAction);
				break;
			}
		}
	}
}

HostHitMissActions::~HostHitMissActions()
{

}
