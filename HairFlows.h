#pragma once

#include "HairStrandModel.h"

#include <vector>

template<int N>
struct MyFlow
{
	int		ids[N];
	float	amount;
};


// Calculate and store EMD flows between two hair models
class HairFlows
{
public:
	HairFlows();
	~HairFlows();

	void	calcFlowsEMD(const HairStrandModel& srcModel, 
						 const HairStrandModel& dstModel,
						 bool useStrandWeights);

	void	calcFlowsClusteredEMD(const HairStrandModel& srcModel,
								  const HairStrandModel& dstModel,
								  const HairFlows& clusterFlows,
								  bool useStrandWeights);

	void	calcFlowsNearestRoot(const HairStrandModel& srcModel,
								 const HairStrandModel&	dstModel);



	const std::vector<MyFlow<2> >& flows() const { return m_flows; }
	
	int		numFlows() const { return m_flows.size(); }

	bool	isEmpty() const { return m_flows.empty(); }

private:
	
	void	calcClusterSIDs(const HairStrandModel& model, std::vector<std::vector<int> >& clusterSIDs);

	std::vector<MyFlow<2> >	m_flows;
};

