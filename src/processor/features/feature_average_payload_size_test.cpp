#include <gtest/gtest.h>
#include "feature_container.h"
#include "feature_average_payload_size.h"

class FeatureContainerTest:public FeatureContainer,public FeatureProviderInterface
{
	vector<pair<Feature *,int> > _features;
public:
	virtual vector<pair<Feature *,int>> &GetFeatures() {return _features;}
	virtual int GetMaxFeatureIndex() {return 1;};
	virtual int GetMemoryNeeded() {return _features[0].first->GetDataSize();}
	void SetFeature(Feature *feature)
	{
			UNUSED(feature);
	}
	virtual ~FeatureContainerTest() {;}
};

TEST(feature_average_payload_size,run)
{

}
