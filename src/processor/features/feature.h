#ifndef FEATURE_H
#define FEATURE_H
struct LogEntry;

class FeatureContainer;

/* base interface for a feature
 * used by FeatureContainer, first call to Aggregate must have a zeroed data structure of the size indicated
 * by GetDataSize() and a featureValue of 0
 */
class Feature
{
public:
	/* returns size of the memory needed by this feature */
	virtual int GetDataSize()=0;
	/* aggregrate the logentry and recalculate the feature
	 * needs a log entry, feature container, a pointer to the memory needed to support the feature and a featureValue pointer
	 */
	virtual void Aggregrate(LogEntry *le,FeatureContainer *,void *data,double *featureValue)=0;
	virtual ~Feature() {;}
};
#endif
