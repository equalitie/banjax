#ifndef FEATUREHTMLTOIMAGERATIO_H
#define FEATUREHTMLTOIMAGERATIO_H
#include "Feature.h"

class FeatureHtmlToImageRatio:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,float *featureValue);
};
#endif
