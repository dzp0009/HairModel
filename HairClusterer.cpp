#include "HairClusterer.h"

#include <opencv2/core/core.hpp>
#include <ANN/ANN.h>

#include "IsochartHeap.h"
#include "CoordUtil.h"

using namespace std;


HairClusterer::HairClusterer()
{
}


HairClusterer::~HairClusterer()
{
}


bool HairClusterer::clusterK(HairStrandModel& srcModel, int k, HairStrandModel& dstModel)
{
	const int numIters = 3;

	dstModel.clear();

	// add random variation
	addPerStrandVariations(srcModel, 0.25f);

	// compute strand neighborhood
	if (!srcModel.hasNeighborStrandIDs())
	{
		printf("ERROR: source doesn't have neighborhoods info!\n");
		return false;
	}

	// compute feature vectors...(not necessary though)
	// ...

	vector<int>	seedSIDs;	// strand indices of seeds
	vector<int> strandCIDs;	// cluster indices of strands

	float minErr = FLT_MAX;
	for (int iter = 0; iter < numIters; iter++)
	{
		// find K seeds to grow from
		if (iter == 0)
			findInitSeeds(srcModel, k, seedSIDs);
		else
			findNextSeeds(srcModel, strandCIDs, dstModel, seedSIDs);

		strandCIDs.assign(srcModel.numStrands(), -1);	// -1 means "unclustered"

		doPartition(srcModel, seedSIDs, strandCIDs);
		doFitting(srcModel, strandCIDs, k, dstModel);

		// compute fitting error
		float err = calcFitError(srcModel, strandCIDs, dstModel);
		if (err < minErr)
		{
			minErr = err;
			//iter = 0; // NOTE
		}
		printf("Fitting err: %f\n", err);
	}

	// update clustering result
	for (int i = 0; i < srcModel.numStrands(); i++)
		srcModel.getStrandAt(i)->setClusterID(strandCIDs[i]);

	fixUnclustered(srcModel);

	// calculate neighborhood of center strands
	std::vector< std::vector<int> >& dstNbrIDs = dstModel.neighborStrandIDs();
	dstNbrIDs.resize(dstModel.numStrands());
	for (int i = 0; i < dstNbrIDs.size(); i++)
		dstNbrIDs[i].clear();

	const std::vector< std::vector<int> >& srcNbrIDs = srcModel.neighborStrandIDs();
	for (int i = 0; i < srcNbrIDs.size(); i++)
	{
		const int cId1 = srcModel.getStrandAt(i)->clusterID();

		for (int j = 0; j < srcNbrIDs[i].size(); j++)
		{
			const int cId2 = srcModel.getStrandAt(srcNbrIDs[i][j])->clusterID();

			if (cId1 != cId2)
			{
				dstNbrIDs[cId1].push_back(cId2);
				dstNbrIDs[cId2].push_back(cId1);
			}
		}
	}
	// remove duplicates
	for (int i = 0; i < dstNbrIDs.size(); i++)
	{
		std::vector<int>& nbrIDs = dstNbrIDs[i];
		std::sort(nbrIDs.begin(), nbrIDs.end());
		nbrIDs.erase(std::unique(nbrIDs.begin(), nbrIDs.end()), nbrIDs.end());
	}

	// dummy strand color
	for (int i = 0; i < dstModel.numStrands(); i++)
	{
		// halfway vector projected to X-Z plane...
		const int hwIdx = NUM_UNISAM_VERTICES / 2;

		XMFLOAT3& pos0 = dstModel.getStrandAt(i)->vertices()[0].position;
		XMFLOAT3& pos1 = dstModel.getStrandAt(i)->vertices()[hwIdx].position;

		float dx = pos1.x - pos0.x;
		float dz = pos1.z - pos0.z;

		XMFLOAT4 color(0,0,0,1);
		if (dx > 0)
			color.x = 1.0f;
		if (dz > 0)
			color.z = 1.0f;

		dstModel.getStrandAt(i)->setColor(color);
	}
	//	// debug root tangent
	//	dstModel.getStrandAt(i)->trim(1);
	//	StrandVertex* vertices = dstModel.getStrandAt(i)->vertices();

	//	XMFLOAT3 tangent;
	//	tangent.x = vertices[1].position.x - vertices[0].position.x;
	//	tangent.y = vertices[1].position.y - vertices[0].position.y;
	//	tangent.z = vertices[1].position.z - vertices[0].position.z;

	//	float len = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
	//	tangent.x /= len;
	//	tangent.y /= len;
	//	tangent.z /= len;

	//	vertices[0].color = vertices[1].color = XMFLOAT4(
	//		0.5f*(tangent.x+1.0f), 0.5f*(tangent.y+1.0f), 0.5f*(tangent.z+1.0f), 1.0f);
	//}

	return true;
}


// randomly pick K seeds from source strands
void HairClusterer::findInitSeeds(const HairStrandModel& model, int k, 
								  vector<int>& seedSIDs)
{
	cv::RNG rng(20130118);

	vector<int> indices(model.numStrands());
	for (int i = 0; i < indices.size(); i++)
		indices[i] = i;

	seedSIDs.resize(k);

	for (int i = 0; i < k; i++)
	{
		// choose a random location in the "remaining" bins and swap the indices
		int j = rng.uniform(i, indices.size());
		std::swap(indices[i], indices[j]);
		seedSIDs[i] = indices[i];
	}
}


// given previous partition, find K strands that are most similar to their
// respective cluster centers to be the seeds
void HairClusterer::findNextSeeds(const HairStrandModel& srcModel, 
								  const vector<int>& strandCIDs, 
								  const HairStrandModel& dstModel,
								  vector<int>& seedSIDs)
{
	vector<float> minDistOfC(dstModel.numStrands(), FLT_MAX);
	
	for (int i = 0; i < srcModel.numStrands(); i++)
	{
		const int cId = strandCIDs[i];
		if (cId < 0) continue;

		float dist = calcStrandDist(srcModel.getStrandAt(i), dstModel.getStrandAt(cId));
		if (dist < minDistOfC[cId])
		{
			minDistOfC[cId] = dist;
			seedSIDs[cId] = i;
		}
	}
}


// perform k-partition on all unclustered strands.
void HairClusterer::doPartition(const HairStrandModel& model, const std::vector<int>& seedSIDs, 
					std::vector<int>& strandCIDs)
{
	const int MaxNbr = 16;

	// initialize priority queue
	IsochartHeap* pHeap = new IsochartHeap(model.numStrands()*MaxNbr/seedSIDs.size());
	for (int i = 0; i < seedSIDs.size(); i++)
	{
		IsochartHeapable* pEntry = new IsochartHeapable;
		pEntry->vert[0] = seedSIDs[i];
		pEntry->vert[1] = i;
		pEntry->cost	= 0;
		pHeap->insert(pEntry, 0);
	}

	const std::vector<std::vector<int> >& nbrStrandIDs = model.neighborStrandIDs();

	// dequeue until empty
	while (pHeap->top())
	{
		IsochartHeapable* pEntry = pHeap->extract()->obj;
		const int sId = pEntry->vert[0];
		const int cId = pEntry->vert[1];

		if (strandCIDs[sId] < 0) // unclustered
		{
			// add current strand to current cluster
			strandCIDs[sId] = cId;

			// enqueue all unclustered neighbor strands
			const int numNbrs = nbrStrandIDs[sId].size();
			for (int nId = 0; nId < numNbrs; nId++)
			{
				const int nbrSId = nbrStrandIDs[sId][nId];
				if (strandCIDs[nbrSId] >= 0)
					continue;	// ignore those already clustered

				IsochartHeapable* pNbrEntry = new IsochartHeapable;
				pNbrEntry->vert[0] = nbrSId;
				pNbrEntry->vert[1] = cId;
				pNbrEntry->cost = calcStrandDist(model.getStrandAt(nbrSId),
												 model.getStrandAt(seedSIDs[cId]));
				pHeap->insert(pNbrEntry, -pNbrEntry->cost);
			}
		}
		delete pEntry;
	}
	delete pHeap;
}


// compute the center strand for each cluster
void HairClusterer::doFitting(const HairStrandModel& srcModel, 
							  const std::vector<int>& strandCIDs,
							  int k,
							  HairStrandModel& dstModel)
{
	const int numVerts = srcModel.getStrandAt(0)->numVertices();

	if (dstModel.numStrands() != k)
	{
		dstModel.clear();
		Strand strand;
		strand.createEmpty(numVerts);
		for (int i = 0; i < k; i++)
			dstModel.addStrand(strand);
	}
	else
	{
		for (int i = 0; i < k; i++)
			dstModel.getStrandAt(i)->createEmpty(numVerts);
	}

	std::vector<int> sizeOfC(k, 0);

	for (int sId = 0; sId < srcModel.numStrands(); sId++)
	{
		const int cId = strandCIDs[sId];
		if (cId < 0) continue;

		const StrandVertex* srcVerts = srcModel.getStrandAt(sId)->vertices();
		StrandVertex* dstVerts = dstModel.getStrandAt(cId)->vertices();
		for (int vId = 0; vId < numVerts; vId++)
		{
			dstVerts[vId].position.x += srcVerts[vId].position.x;
			dstVerts[vId].position.y += srcVerts[vId].position.y;
			dstVerts[vId].position.z += srcVerts[vId].position.z;
		}
		sizeOfC[cId]++;
	}

	for (int cId = 0; cId < k; cId++)
	{
		float size = (float)sizeOfC[cId];
		for (int vId = 0; vId < numVerts; vId++)
		{
			XMFLOAT3& pos = dstModel.getStrandAt(cId)->vertices()[vId].position;
			pos.x /= size;
			pos.y /= size;
			pos.z /= size;
		}
	}
}


float HairClusterer::calcFitError(const HairStrandModel&	srcModel, 
								  const std::vector<int>&	strandCIDs,
								  const HairStrandModel&	dstModel)
{
	float totalErr = 0;

	for (int sId = 0; sId < srcModel.numStrands(); sId++)
	{
		const int cId = strandCIDs[sId];
		if (cId < 0) continue;

		totalErr += calcStrandDist(srcModel.getStrandAt(sId), dstModel.getStrandAt(cId));
	}

	return totalErr;
}


float HairClusterer::calcStrandDist(const Strand* pA, const Strand* pB)
{
	assert(pA->numVertices() == pB->numVertices());

	float dist = 0;

	const XMFLOAT3& rootA = pA->vertices()[0].position;
	const XMFLOAT3& rootB = pB->vertices()[0].position;

	const float rootDx = rootB.x - rootA.x;
	const float rootDy = rootB.y - rootA.y;
	const float rootDz = rootB.z - rootA.z;

	for (int i = 1; i < pA->numVertices(); i++)
	{
		const XMFLOAT3& vA = pA->vertices()[i].position;
		const XMFLOAT3& vB = pB->vertices()[i].position;

		const float dx = vB.x - rootDx - vA.x;
		const float dy = vB.y - rootDx - vA.y;
		const float dz = vB.z - rootDx - vA.z;

		dist += dx*dx + dy*dy + dz*dz;
	}

	dist /= (float)pA->numVertices();

	return dist;
}


// Perform Delaunay triangulation on hair root vertices to build
// root neighborhood
//void HairClusterer::triangulateRoots(const HairStrandModel& model)
//{
	//struct triangulateio in;
	//memset(&in, 0, sizeof(in));
	//in.numberofpoints = model.numStrands();
	//in.pointlist = (REAL*)malloc(in.numberofpoints * 3 * sizeof(REAL));
	//for (int i = 0; i < model.numStrands(); i++)
	//{
	//	const XMFLOAT3& pos = model.getStrandAt(i)->vertices()[0].position;
	//	in.pointlist[i*3+0] = pos.x;
	//	in.pointlist[i*3+1] = pos.y;
	//	in.pointlist[i*3+2] = pos.z;
	//}

	//struct triangulateio out, vorout;
	//memset(&out, 0, sizeof(out));
	//memset(&vorout, 0, sizeof(vorout));

	//triangulate("QpczAevn", &in, &out, &vorout);
//}


void HairClusterer::addPerStrandVariations(HairStrandModel& model, float maxVar)
{
	for (int i = 0; i < model.numStrands(); i++)
	{
		float dx = maxVar * cv::theRNG().uniform(-1.0f, 1.0f);
		float dy = maxVar * cv::theRNG().uniform(-1.0f, 1.0f);
		float dz = maxVar * cv::theRNG().uniform(-1.0f, 1.0f);

		StrandVertex* vertices = model.getStrandAt(i)->vertices();
		for (int j = 0; j < NUM_UNISAM_VERTICES; j++)
		{
			vertices[j].position.x += dx;
			vertices[j].position.y += dy;
			vertices[j].position.z += dz;
		}
	}
}


void HairClusterer::fixUnclustered(HairStrandModel& model)
{
	for (int i = 0; i < model. numStrands(); i++)
	{
		Strand* strand = model.getStrandAt(i);
		if (strand->clusterID() == -1)
		{
			bool fixed = false;
			for (int j = 0; j < model.neighborStrandIDs()[i].size(); j++)
			{
				const Strand* nbrStrand = model.getStrandAt(model.neighborStrandIDs()[i][j]);
				if (nbrStrand->clusterID() != -1)
				{
					strand->setClusterID(nbrStrand->clusterID());
					fixed = true;
					break;
				}
			}
			if (!fixed)
			{
				printf("\n>>> Cannot find a clustered neighbor for %d! <<<\n", i);
			}
		}
	}
}