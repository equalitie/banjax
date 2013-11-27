#include "BotBangerAggregrator.h"
class BotBangerLogger:public BotBangerEventListener
{
public:
	void OnFeatureEvent(char *key,float *features,int numFeatures) {};
};