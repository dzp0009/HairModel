#include "AdaptivePoissonSampler.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <assert.h>

#include <vector>

#include "Exception.hpp"
#include "Random.hpp"
#include "KDTree.hpp"
#include "UniformDistanceField.hpp"
#include "RampDistanceField.hpp"
#include "SampledDistanceField.hpp"
#include "Math.hpp"
#include "FrameBuffer.hpp"
#include "PhaseGroup.hpp"


using namespace std;

int AdaptivePoissonSampler::generate(
	const int		dim, 
	const float		minDist, 
	const int		k, 
	const float		cellSize, 
	const float*	range, 
	float**			ppOutput)
{
	assert(ppOutput);

	const int numOuterTrials = k;
	const int numInnerTrials = 1;

	// init stuff
	vector<float> rangeMin(dim);
	vector<float> rangeMax(dim);
	{
		for (int i = 0; i < dim; i++)
		{
			rangeMin[i] = 0;
			rangeMax[i] = range[i];
		}
	}

	DistanceField* pDistField = new UniformDistanceField(rangeMin, rangeMax, minDist);

	DistanceField& distField = *pDistField;

	KDTree tree(dim, cellSize, distField);

	// Random seed
	const unsigned long randSeed = 20111209;
	Random::InitRandomNumberGenerator(randSeed);

	// Adaptive sampling from low to high resolutions
	do 
	{
		const int currLevel = tree.NumLevels() - 1;

		int numNbrChecked = 0;

		vector<int> numCellsPerDim(dim);

		tree.GetNumCellsPerDimension(currLevel, numCellsPerDim);

		for (int outerTrial = 0; outerTrial < numOuterTrials; outerTrial++)
		{
			vector< vector<KDTree::Cell*> > phaseGroups;
			if (!PhaseGroup::Compute(PhaseGroup::SEQUENTIAL_RANDOM, tree, 
									 currLevel, phaseGroups))
			{
				return -1;
			}

			const float cellSize = tree.CellSize(currLevel);
			vector<int> cellIndex(dim);

			for (unsigned int i = 0; i < phaseGroups.size(); i++)
			{
				for (unsigned int j = 0; j < phaseGroups[i].size(); j++)
				{
					KDTree::Cell* currCell = phaseGroups[i][j];

					if (currCell)
					{
						if (!currCell->GetIndex(cellIndex))
						{
							return -1;
						}

						for (int innerTrial = 0; 
							 (innerTrial < numInnerTrials) && (currCell->NumSamples() == 0);
							 innerTrial++)
						{
							Sample* sample = new Sample(dim);

							for (int n = 0; n < dim; n++)
							{
								sample->coordinate[n] = (cellIndex[n] + Random::UniformRandom())*cellSize;
							}

							if ((tree.GetDistanceField().Query(sample->coordinate) < FLT_MAX) &&
								!currCell->EccentricConflict(*sample, numNbrChecked) &&
								distField.Inside(sample->coordinate))
							{
								if (!currCell->Add(*sample))
								{
									return -1;
								}
							}
							else
							{
								delete sample;
								sample = NULL;
							}
						} // for innerTrial
					} // if currCell
				} // for j
			} // for i
		}

	} 
	while (tree.Subdivide());

	// output stuff
	vector<const Sample*> samples;
	tree.GetSamples(samples);

	const int numSamples = samples.size();
	*ppOutput = new float[dim*numSamples];

	for (int i = 0; i < numSamples; i++)
	{
		for (int j = 0; j < dim; j++)
		{
			(*ppOutput)[i*dim + j] = samples[i]->coordinate[j];
		}
	}

	delete pDistField;
	pDistField = NULL;

	return numSamples;
}