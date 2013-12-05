#ifndef FEATURE_H
#define FEATURE_H
struct LogEntry;

class FeatureContainer;
class Feature
{
public:
	virtual int GetDataSize()=0;
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue)=0;
	virtual ~Feature() {;}
};
#endif
