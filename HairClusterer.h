#pragma once

#include "HairStrandModel.h"

#include <vector>

// dedicated class to perform hair clustering based on [Wang et al. 2009]
class HairClusterer
{
public:
	HairClusterer();
	~HairClusterer();

	bool	clusterK(HairStrandModel& srcModel, int k, HairStrandModel& dstModel);

private:

	void	findInitSeeds(const HairStrandModel& model, int k, 
						  std::vector<int>& seedSIDs);
	
	void	findNextSeeds(const HairStrandModel& srcModel, 
						  const std::vector<int>& strandCIDs, 
						  const HairStrandModel& dstModel,
						  std::vector<int>& seedSIDs);

	void	doPartition(const HairStrandModel& model, const std::vector<int>& seedSIDs, 
						std::vector<int>& strandCIDs);
	
	void	doFitting(const HairStrandModel& srcModel, const std::vector<int>& strandCIDs, 
					  int k, HairStrandModel& dstModel);

	float	calcFitError(const HairStrandModel& srcModel, const std::vector<int>& strandCIDs,
						 const HairStrandModel&	dstModel);

	float	calcStrandDist(const Strand* pA, const Strand* pB);


	void	addPerStrandVariations(HairStrandModel& model, float maxVar);

	void	fixUnclustered(HairStrandModel& model);
};

