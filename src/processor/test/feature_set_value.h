#ifndef FEATURE_SET_VALUE_H
#define FEATURE_SET_VALUE_H
/*
 * This feature sets the featureValue to a value determined at construction
 */
class FeatureSetValue:public Feature
{
	double _value;
public:


	FeatureSetValue(double value):Feature(),_value(value)
	{
	}
	virtual int GetDataSize() {return 0;}
	/* Set the feature to number of requests
	 */
	virtual void Aggregrate(LogEntry *le,FeatureContainer *fc,void *data,double *featureValue)
	{
		UNUSED(le);
		UNUSED(fc);
		UNUSED(data);
		*featureValue=_value;

	}
};
#endif
