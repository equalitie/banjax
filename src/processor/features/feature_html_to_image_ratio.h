#ifndef FEATUREHTMLTOIMAGERATIO_H
#define FEATUREHTMLTOIMAGERATIO_H
#include "feature.h"

class FeatureHtmlToImageRatio:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
