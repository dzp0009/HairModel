#include "HairFlows.h"

#include <QTime>
#include <ANN/ANN.h>

#include "SimpleInterpolator.h"

HairFlows::HairFlows()
{
}


HairFlows::~HairFlows()
{
}


void HairFlows::calcFlowsEMD(const HairStrandModel& srcModel, 
							 const HairStrandModel& dstModel,
							 bool useStrandWeights)
{
	if (!srcModel.isUniformSampled(NUM_UNISAM_VERTICES) ||
		!dstModel.isUniformSampled(NUM_UNISAM_VERTICES))
	{
		printf("ERROR: src and dst strands must be uniform sampled!\n");
		return;
	}

	printf("Calculating flows from %d to %d strands...", srcModel.numStrands(), dstModel.numStrands());

	// source hair vectors
	std::vector<Vector<NUM_UNISAM_VERTICES*3,float> > samplesSrc;
	std::vector<double> weightsSrc;

	samplesSrc.resize(srcModel.numStrands());
	weightsSrc.resize(srcModel.numStrands());

	for (int i = 0; i < srcModel.numStrands(); i++)
	{
		const StrandVertex* vertices = srcModel.getStrandAt(i)->vertices();
		for (int j = 0; j < NUM_UNISAM_VERTICES; j++)
		{
			samplesSrc[i][j*3+0] = vertices[j].position.x;
			samplesSrc[i][j*3+1] = vertices[j].position.y;
			samplesSrc[i][j*3+2] = vertices[j].position.z;
		}
		weightsSrc[i] = useStrandWeights ? srcModel.getStrandAt(i)->weight() : 1.0;
	}

	// target hair vectors
	std::vector<Vector<NUM_UNISAM_VERTICES*3,float> > samplesDst;
	std::vector<double> weightsDst;

	samplesDst.resize(dstModel.numStrands());
	weightsDst.resize(dstModel.numStrands());

	for (int i = 0; i < dstModel.numStrands(); i++)
	{
		const StrandVertex* vertices = dstModel.getStrandAt(i)->vertices();
		for (int j = 0; j < NUM_UNISAM_VERTICES; j++)
		{
			samplesDst[i][j*3+0] = vertices[j].position.x;
			samplesDst[i][j*3+1] = vertices[j].position.y;
			samplesDst[i][j*3+2] = vertices[j].position.z;
		}
		weightsDst[i] = useStrandWeights ? dstModel.getStrandAt(i)->weight() : 1.0;
	}

	QTime timer;
	timer.start();

	//SimpleInterpolator<NUM_UNISAM_VERTICES*3,float> interp(samplesSrc, weightsSrc, samplesDst, weightsDst, sqrStrandDistSmart);
	SimpleInterpolator<NUM_UNISAM_VERTICES*3,float> interp(samplesSrc, weightsSrc, samplesDst, weightsDst, sqrStrandDistLinear);
	interp.precompute();
	
	printf("DONE. (%.3f s)\n", (float)timer.elapsed()/1000.0f);

	// save generated flows
	const std::vector<TsFlow>& flows = interp.flows();
	m_flows.resize(flows.size());
	for (int i = 0; i < flows.size(); i++)
	{
		m_flows[i].ids[0] = flows[i].from;
		m_flows[i].ids[1] = flows[i].to;
		m_flows[i].amount = flows[i].amount;
	}
}


void HairFlows::calcFlowsClusteredEMD(const HairStrandModel& srcModel,
									  const HairStrandModel& dstModel,
									  const HairFlows& clusterFlows,
									  bool useStrandWeights)
{
	if (!srcModel.isUniformSampled(NUM_UNISAM_VERTICES) ||
		!dstModel.isUniformSampled(NUM_UNISAM_VERTICES))
	{
		printf("ERROR: src and dst strands must be uniform sampled!\n");
		return;
	}

	printf("Calculating clustered flows from %d to %d strands...", srcModel.numStrands(), dstModel.numStrands());
	QTime timer;
	timer.start();

	// precompute lists of strands (IDs) in each cluster
	std::vector< std::vector<int> > srcClusterSIDs, dstClusterSIDs;
	calcClusterSIDs(srcModel, srcClusterSIDs);
	calcClusterSIDs(dstModel, dstClusterSIDs);

	m_flows.clear();
	
	// TODO: parallelize using OpenMP?????
	printf("ClusterFlows(%d)...", clusterFlows.numFlows());
	for (int iCFlow = 0; iCFlow < clusterFlows.numFlows(); iCFlow++)
	{
		const int iSrcCId = clusterFlows.m_flows[iCFlow].ids[0];
		const int iDstCId = clusterFlows.m_flows[iCFlow].ids[1];

		// source hair vectors
		const int srcCSize = srcClusterSIDs[iSrcCId].size();
		std::vector<Vector<NUM_UNISAM_VERTICES*3,float> > samplesSrc(srcCSize);
		std::vector<double> weightsSrc(srcCSize);

		for (int i = 0; i < srcCSize; i++)
		{
			const int sId = srcClusterSIDs[iSrcCId][i];
			const StrandVertex* vertices = srcModel.getStrandAt(sId)->vertices();
			for (int j = 0; j < NUM_UNISAM_VERTICES; j++)
			{
				samplesSrc[i][j*3+0] = vertices[j].position.x;
				samplesSrc[i][j*3+1] = vertices[j].position.y;
				samplesSrc[i][j*3+2] = vertices[j].position.z;
			}
			weightsSrc[i] = useStrandWeights ? srcModel.getStrandAt(sId)->weight() : 1.0;
		}

		// target hair vectors
		const int dstCSize = dstClusterSIDs[iDstCId].size();
		std::vector<Vector<NUM_UNISAM_VERTICES*3,float> > samplesDst(dstCSize);
		std::vector<double> weightsDst(dstCSize);

		for (int i = 0; i < dstCSize; i++)
		{
			const int sId = dstClusterSIDs[iDstCId][i];
			const StrandVertex* vertices = dstModel.getStrandAt(sId)->vertices();
			for (int j = 0; j < NUM_UNISAM_VERTICES; j++)
			{
				samplesDst[i][j*3+0] = vertices[j].position.x;
				samplesDst[i][j*3+1] = vertices[j].position.y;
				samplesDst[i][j*3+2] = vertices[j].position.z;
			}
			weightsDst[i] = useStrandWeights ? dstModel.getStrandAt(sId)->weight() : 1.0;
		}

		//printf("<%d*%d>", srcCSize, dstCSize);

		//SimpleInterpolator<NUM_UNISAM_VERTICES*3,float> interp(samplesSrc, weightsSrc, samplesDst, weightsDst, sqrStrandDistSmart);
		SimpleInterpolator<NUM_UNISAM_VERTICES*3,float> interp(samplesSrc, weightsSrc, samplesDst, weightsDst, sqrStrandDistLinear);
		interp.precompute();
	
		// save generated flows
		const std::vector<TsFlow>& flows = interp.flows();
		m_flows.reserve(m_flows.size() + flows.size());
		for (int i = 0; i < flows.size(); i++)
		{
			const int srcSId = srcClusterSIDs[iSrcCId][flows[i].from];
			const int dstSId = dstClusterSIDs[iDstCId][flows[i].to];

			MyFlow<2> flow;
			flow.ids[0] = srcSId;
			flow.ids[1] = dstSId;
			flow.amount = flows[i].amount;
			m_flows.push_back(flow);
		}
	}

	printf("DONE. (%.3f s)\n", (float)timer.elapsed()/1000.0f);
}


void HairFlows::calcFlowsNearestRoot(const HairStrandModel& srcModel, const HairStrandModel& dstModel)
{
	std::vector<std::vector<int> > links(srcModel.numStrands());

	ANNpointArray srcPts = annAllocPts(srcModel.numStrands(), 3);
	for (int srcIdx = 0; srcIdx < srcModel.numStrands(); srcIdx++)
	{
		const XMFLOAT3& pos = srcModel.getStrandAt(srcIdx)->vertices()[0].position;
		srcPts[srcIdx][0] = pos.x;
		srcPts[srcIdx][1] = pos.y;
		srcPts[srcIdx][2] = pos.z;
	}

	ANNpoint query = annAllocPt(3);
	ANNidx	 nnIdx[1];
	ANNdist	 dists[1];

	ANNkd_tree* kdTree = new ANNkd_tree(srcPts, srcModel.numStrands(), 3);

	for (int dstIdx = 0; dstIdx < dstModel.numStrands(); dstIdx++)
	{
		// find closest root source strand
		const XMFLOAT3& root = dstModel.getStrandAt(dstIdx)->vertices()[0].position;
		query[0] = root.x;
		query[1] = root.y;
		query[2] = root.z;

		kdTree->annkSearch(query, 1, nnIdx, dists);

		// add itself to the source strand's links
		links[nnIdx[0]].push_back(dstIdx);
	}

	delete kdTree;
	annDeallocPts(srcPts);

	// deal with source strands without any link
	ANNpointArray dstPts = annAllocPts(dstModel.numStrands(), 3);
	for (int dstIdx = 0; dstIdx < dstModel.numStrands(); dstIdx++)
	{
		const XMFLOAT3& pos = dstModel.getStrandAt(dstIdx)->vertices()[0].position;
		dstPts[dstIdx][0] = pos.x;
		dstPts[dstIdx][1] = pos.y;
		dstPts[dstIdx][2] = pos.z;
	}

	kdTree = new ANNkd_tree(dstPts, dstModel.numStrands(), 3);

	int numFlows = 0;
	for (int srcIdx = 0; srcIdx < srcModel.numStrands(); srcIdx++)
	{
		if (links[srcIdx].size() > 0)
		{
			numFlows += links[srcIdx].size();
			continue;
		}

		const XMFLOAT3& root = srcModel.getStrandAt(srcIdx)->vertices()[0].position;
		query[0] = root.x;
		query[1] = root.y;
		query[2] = root.z;

		kdTree->annkSearch(query, 1, nnIdx, dists);

		// add target strand to itself's links
		links[srcIdx].push_back(nnIdx[0]);

		numFlows++;
	}

	delete kdTree;
	annDeallocPts(dstPts);
	annDeallocPt(query);

	// convert to flows
	printf("Total flows: %d\n", numFlows);

	m_flows.resize(numFlows);
	int i = 0;
	for (int srcIdx = 0; srcIdx < srcModel.numStrands(); srcIdx++)
	{
		for (int j = 0; j < links[srcIdx].size(); j++)
		{
			m_flows[i].ids[0] = srcIdx;
			m_flows[i].ids[1] = links[srcIdx][j];
			i++;
		}
	}
}


// helper to find lists of strand IDs for each cluster.
void HairFlows::calcClusterSIDs(const HairStrandModel& model,
								std::vector<std::vector<int> >& clusterSIDs)
{
	clusterSIDs.clear();
	clusterSIDs.resize(model.numStrands());

	int numClusters = 0;
	for (int sId = 0; sId < model.numStrands(); sId++)
	{
		const int cId = model.getStrandAt(sId)->clusterID();
		if (cId < 0)
			printf("WARNING: model contains unclustered hair!\n");
		
		if (cId >= numClusters)
			numClusters = cId + 1;

		clusterSIDs[cId].push_back(sId);
	}

	clusterSIDs.resize(numClusters);
}