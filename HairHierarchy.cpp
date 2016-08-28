#include "HairHierarchy.h"

#include "HairClusterer.h"

#include <opencv2/core/core.hpp>

#include <QTime>

using namespace std;

HairHierarchy::HairHierarchy() : m_currLvlIdx(-1)
{
	m_levels.reserve(3);
}


HairHierarchy::~HairHierarchy()
{
	release();
}


void HairHierarchy::release()
{
	for (int i = 0; i < m_levels.size(); i++)
		m_levels[i].release();
}


void HairHierarchy::render()
{
	if (m_currLvlIdx >= 0 && m_currLvlIdx < numLevels())
		m_levels[m_currLvlIdx].render();
}


void HairHierarchy::clear()
{
	m_levels.clear();
	m_currLvlIdx = -1;
}


bool HairHierarchy::load(QString filename, bool updateBuffers)
{
	m_levels.reserve(3);
	m_levels.resize(1);

	if (m_levels[0].load(filename, updateBuffers))
	{
		m_currLvlIdx = 0;
		return true;
	}
	else
	{
		clear();
		return false;
	}
}


void HairHierarchy::buildByFixedK(std::vector<int> levelSizes)
{
	const int nLevels = levelSizes.size();

	if (nLevels < 1 || m_levels[0].numStrands() < 1)
	{
		printf("ERROR: cannot build hierarchy from emptiness!\n");
		return;
	}

	QTime timer;
	timer.start();
	printf("Preprocessing hair model...");

	if (m_levels.size() != nLevels)
		m_levels.resize(nLevels);

	if (m_levels[0].numStrands() != levelSizes[0])
	{
		// randomly pick k strands from level 0 strands and discard others
		HairStrandModel model;
		model.reserve(levelSizes[0]);
		
		vector<int> indices(m_levels[0].numStrands());
		for (int i = 0; i < indices.size(); i++)
			indices[i] = i;
		for (int i = 0; i < levelSizes[0]; i++)
		{
			int j = cv::theRNG().uniform(i, indices.size());
			std::swap(indices[i], indices[j]);

			model.addStrand(*(m_levels[0].getStrandAt(indices[i])));
		}
		m_levels[0] = model;

		// update neighborhood
		//m_levels[0].calcRootNbrsByANN(16);
	}
	if (!m_levels[0].isUniformSampled(NUM_UNISAM_VERTICES))
	{
		// uniformly resample strand vertices if necessary
		for (int i = 0; i < m_levels[0].numStrands(); i++)
			m_levels[0].getStrandAt(i)->resample(NUM_UNISAM_VERTICES);
	}

	printf("DONE. (%.3f s)\n", timer.elapsed()/1000.0f);

	// build coarser levels
	for (int i = 1; i < nLevels; i++)
	{
		// HACK!!!: build level-2 from left-right two clusters!
		if (i == 2)
		{
			printf("CREATING 2 CLUSTERS!!\n");
			const int hwIdx = NUM_UNISAM_VERTICES / 5;
			for (int sId = 0; sId < m_levels[i-1].numStrands(); sId++)
			{
				Strand* strand = m_levels[i-1].getStrandAt(sId);

				XMFLOAT3& pos0 = strand->vertices()[0].position;
				XMFLOAT3& pos1 = strand->vertices()[hwIdx].position;

				if (pos1.x  < pos0.x)
				{
					strand->setClusterID(0);
					strand->setColor(XMFLOAT4(1,0,0,1));
				}
				else
				{
					strand->setClusterID(1);
					strand->setColor(XMFLOAT4(0,0,1,1));
				}
			}
			m_levels[i].createEmptyUnisam(2, NUM_UNISAM_VERTICES);
			for (int vId = 0; vId < NUM_UNISAM_VERTICES; vId++)
			{
				m_levels[i].getStrandAt(0)->vertices()[vId].position = XMFLOAT3(-100,-100,-100);
				m_levels[i].getStrandAt(1)->vertices()[vId].position = XMFLOAT3(100,100,100);
			}
			
			break;
		}

		HairClusterer clusterer;
		clusterer.clusterK(m_levels[i-1], levelSizes[i], m_levels[i]);

		//m_levels[i].calcRootNbrsByANN(6);
	}

	if (m_currLvlIdx >= nLevels)
		m_currLvlIdx = 0;

	calcStrandWeights();

	printf("\n");
}


void HairHierarchy::calcStrandWeights()
{
	printf("Calculating strand weights...");

	// For level 0, a strand's weight is its curve length.
	for (int i = 0; i < m_levels[0].numStrands(); i++)
	{
		Strand* strand = m_levels[0].getStrandAt(i);
		strand->setWeight(strand->updateLength());
	}

	for (int lvl = 1; lvl < numLevels(); lvl++)
	{
		// For level k>0, a strand's weight is the sum of all its member strands' weights
		for (int i = 0; i < m_levels[lvl].numStrands(); i++)
			m_levels[lvl].getStrandAt(i)->setWeight(0);

		for (int i = 0; i < m_levels[lvl-1].numStrands(); i++)
		{
			Strand* memberStrand = m_levels[lvl-1].getStrandAt(i);
			if (memberStrand->clusterID() < 0)
			{
				printf("!");	// unclustered strand
				continue;
			}

			Strand* highLvlStrand = m_levels[lvl].getStrandAt(memberStrand->clusterID());
			
			highLvlStrand->setWeight(highLvlStrand->weight() + memberStrand->weight());
		}
	}

	// Normalize weight for debugging
	for (int lvl = 0; lvl < numLevels(); lvl++)
	{
		float maxWeight = m_levels[lvl].getStrandAt(0)->weight();
		for (int i = 1; i < m_levels[lvl].numStrands(); i++)
		{
			const float weight = m_levels[lvl].getStrandAt(i)->weight();
			if (weight > maxWeight)
				maxWeight = weight;
		}

		for (int i = 0; i < m_levels[lvl].numStrands(); i++)
		{
			const float weight = m_levels[lvl].getStrandAt(i)->weight() / maxWeight;
			m_levels[lvl].getStrandAt(i)->setWeight(weight);

			// Colorize by weight for debugging
			//XMFLOAT4 color(weight, weight, weight, 1);
			//m_levels[lvl].getStrandAt(i)->setColor(color);
		}
	}

	printf("DONE\n");
}