#ifndef HOSTHITMISSDUMPER_H
#define HOSTHITMISSDUMPER_H
#include <time.h>
#include "host_hit_miss_aggregator.h"
#include "../utils/string_dumper.h"
#include <iostream>

class HostHitMissDumper:public HostHitMissEventListener,public StringDumper
{

	bool _reportAll;
public:
	HostHitMissDumper(string &output,bool reportAll):
		HostHitMissEventListener(),
		StringDumper(output),
		_reportAll(reportAll)

	{

	}
	void OnHostHitMissEvent(char *host,HitMissRange *hmrange)
	{
		if (_reportAll || ((!hmrange->reported) && hmrange->periodTotal>10))
		{
			hmrange->reported=1;
			char buffer[10000];
			sprintf(buffer,"hmr\t%s\t%d\t%d\t%d\t%f",host,hmrange->totalCountFromStart,hmrange->periodTotal,hmrange->periodHits,hmrange->periodRatio);
			addToDump(buffer);

			

		}
	}

};

#endif
