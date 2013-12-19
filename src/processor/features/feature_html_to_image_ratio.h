#ifndef FEATUREHTMLTOIMAGERATIO_H
#define FEATUREHTMLTOIMAGERATIO_H
#include "feature.h"

/* calculate the ratio (image requests)/(html requests)
 */
class FeatureHtmlToImageRatio:public Feature
{
public:
	virtual int GetDataSize();
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue);
};
#endif
