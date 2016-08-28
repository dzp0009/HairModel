#ifndef SIMPLE_INTERPOLATOR_H
#define SIMPLE_INTERPOLATOR_H

#include <Interpolator.h>


// tailored version of Bonneel's displacement interpolator class.
// discrete, all-positive, single scale input.
template<int DIM, typename SAMPLESTYPE=double>
class SimpleInterpolator
{
public:
	SimpleInterpolator(const std::vector<Vector<DIM,SAMPLESTYPE> > &pA, 
					   const std::vector<double> &pwA, 
					   const std::vector<Vector<DIM,SAMPLESTYPE> > &pB, 
					   const std::vector<double> &pwB, 
					   double (*pSqrDist)(const Vector<DIM,SAMPLESTYPE> &, 
										  const Vector<DIM,SAMPLESTYPE> &)) :
		m_A(pA), m_B(pB),        // the location in space of each sample point
		m_wA(pwA), m_wB(pwB),    // the value of each sample point
		sqrDist(pSqrDist)   // the cost of the particle travelling, typically a squared geodesic distance
	{ };

	////////////////////////////////////////////////////////////////////////
	/////////////////// Core of the distribution interpolation /////////////
	////////////////////////////////////////////////////////////////////////

	void precompute()
	{
		m_sumWA=0;
		m_sumWB=0;

		for (unsigned int i = 0; i < m_wA.size(); i++)
			m_sumWA += m_wA[i];
		for (unsigned int i = 0; i < m_wB.size(); i++)
			m_sumWB += m_wB[i];

		std::vector<double> w1(m_A.size(), 0.);
		std::vector<double> w2(m_B.size(), 0.);
		if (fabs(m_sumWA) > 0)
			for (int i = 0; i < m_A.size(); i++)
				w1[i] = m_wA[i] / m_sumWA;
		if (fabs(m_sumWB) > 0)
			for (int i = 0; i < m_B.size(); i++)
				w2[i] = m_wB[i] / m_sumWB;

		double dAB;
		minCostFlow(m_A, m_B, w1, w2, sqrDist, m_resultFlow, dAB);
	}


	void interpolate(double alpha, std::vector<Vector<DIM,SAMPLESTYPE> > &samples, std::vector<double> &weights)
	{
		samples.resize(m_resultFlow.size());
		weights.resize(m_resultFlow.size());

		for (int i = 0; i < m_resultFlow.size(); i++)
		{
			samples[i] = m_A[m_resultFlow[i].from]*(1.0-alpha) + m_B[m_resultFlow[i].to]*alpha;
			weights[i] = m_resultFlow[i].amount*(m_sumWA*(1.0-alpha) + m_sumWB*alpha);
		}
	}
	

	const std::vector<TsFlow>& flows() const { return m_resultFlow; }

private:

	// normalize and quantize the two histograms, and make sure that the first one has the smallest sum (the two sums should be equals, but due to the quantization, this may not be the case, and it can lead to bugs in LEMON)
	static bool normalizeAndSwap(std::vector<Vector<DIM,SAMPLESTYPE> > &samples1, 
								 std::vector<Vector<DIM,SAMPLESTYPE> > &samples2, 
								 std::vector<double> &weights1, 
								 std::vector<double> &weights2, 
								 double &stretchVal, 
								 double &stretchDist)
	{
		bool swapped = false;
		stretchDist = 1000.;
		// first normalize
		double ds1 = 0., ds2 = 0.;
		for (unsigned int i=0; i<weights1.size(); i++)	
			ds1 += weights1[i];
		for (unsigned int i=0; i<weights2.size(); i++)	
			ds2 += weights2[i];
		for (unsigned int i=0; i<weights1.size(); i++)	
			weights1[i] /= ds1;
		for (unsigned int i=0; i<weights2.size(); i++)	
			weights2[i] /= ds2;

		double stretch1 = weights1.size();
		double stretch2 = weights2.size();
		stretchVal = my_max(stretch1,stretch2)*1000.; 

		// next, quantize
		int s1 = 0, s2 = 0;
		for (unsigned int i=0; i<weights1.size(); i++)
			s1 += (int)(weights1[i]*stretchVal + 0.5);  // +0.5 in order to round instead of truncating
		for (unsigned int i=0; i<weights2.size(); i++)
			s2 += (int)(weights2[i]*stretchVal + 0.5);

		// find the largest quantized one
		double diff = 0;
		for (unsigned int i=0; i<weights1.size(); i++)
			diff += (int)(weights1[i]*stretchVal + 0.5);

		for (unsigned int i=0; i<weights2.size(); i++)
			diff += (int)(-(weights2[i]*stretchVal + 0.5));

		// put the smallest first
		if (diff>0)
		{
			swap(weights1, weights2);
			swap(samples1, samples2);
			swapped = true;
		}
		return swapped;
	}

	static void minCostFlow(std::vector<Vector<DIM,SAMPLESTYPE> > samples1, 
							std::vector<Vector<DIM,SAMPLESTYPE> > samples2, 
							std::vector<double> weights1, 
							std::vector<double> weights2, 
							double (*distance)(const Vector<DIM,SAMPLESTYPE> &, 
											   const Vector<DIM,SAMPLESTYPE> &), 
							std::vector<TsFlow> &flow, 
							double &resultdist)
	{
		typedef FullBipartiteDigraph Digraph;
		DIGRAPH_TYPEDEFS(FullBipartiteDigraph);

		double stretchVal;
		double stretchDist;
		bool swapped = normalizeAndSwap(samples1, samples2, weights1, weights2, stretchVal, stretchDist);

		Digraph di((int)samples1.size(), (int)samples2.size());
		NetworkSimplexSimple<Digraph,int,int, unsigned short int> net(di, true);

		int idarc = 0;
		for (unsigned int i=0; i<samples1.size(); i++)
		{
			for (unsigned int j=0; j<samples2.size(); j++)
			{
				double d = distance(samples1[i], samples2[j]);
				net.setCost(di.arcFromId(idarc), (int) (stretchDist*d+0.5)); // quantize the cost
				idarc++;
			}
		}

		Digraph::NodeMap<int> nodeSuppliesDemand(di);

		for (unsigned int i=0; i<samples1.size(); i++)
			nodeSuppliesDemand[ di.nodeFromId(i) ] = (int)(weights1[i]*stretchVal+0.5);
		for (unsigned int i=0; i<samples2.size(); i++)
			nodeSuppliesDemand[ di.nodeFromId(i+(unsigned int)samples1.size()) ] = (int)(-(weights2[i]*stretchVal+0.5));


		net.supplyMap(nodeSuppliesDemand);

		// solve!
		int ret = net.run(NetworkSimplexSimple<Digraph,int,int, unsigned short int>::BLOCK_SEARCH);
		resultdist = net.totalCost()/(stretchVal*stretchDist);
		flow.reserve(samples1.size() + samples2.size() - 1);

		// save results
		for (unsigned int i=0; i<samples1.size(); i++) 
		{
			for (unsigned int j=0; j<samples2.size(); j++)
			{
				TsFlow f;
				f.amount = net.flow(di.arcFromId(i*(unsigned int)samples2.size()+j))/stretchVal;
				if (swapped) {
					f.from = j;
					f.to = i;
				} else {
					f.from = i;
					f.to = j;
				}
				if (fabs(f.amount)>1E-18)
					flow.push_back(f);
			}
		}
	}


	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&);
	
	std::vector<Vector<DIM,SAMPLESTYPE> > m_A;	// sample locations (bins)
	std::vector<Vector<DIM,SAMPLESTYPE> > m_B;
	std::vector<double> m_wA;						// sample weight/value/mass
	std::vector<double> m_wB;
	double m_sumWA, m_sumWB;

	std::vector<TsFlow> m_resultFlow;  // EMD flow between m_A and m_B
};



template<int DIM, typename SAMPLESTYPE>
double sqrStrandDistLinear(const Vector<DIM,SAMPLESTYPE> &s1, const Vector<DIM,SAMPLESTYPE> &s2)
{
	double d = 0;
	for (int i = 0; i < DIM; i += 3)
	{
		SAMPLESTYPE dx = s1[i] - s2[i];
		SAMPLESTYPE dy = s1[i+1] - s2[i+1];
		SAMPLESTYPE dz = s1[i+2] - s2[i+2];

		//d += std::sqrt(dx*dx + dy*dy + dz*dz);
		d += (dx*dx + dy*dy + dz*dz);
	}
	return d / (float)DIM;
}


template<int DIM, typename SAMPLESTYPE>
double sqrStrandDistSmart(const Vector<DIM,SAMPLESTYPE> &s1, const Vector<DIM,SAMPLESTYPE> &s2)
{
	// halfway vector projected to X-Z plane...
	const int numVerts = DIM/3;
	const int hwIdx = (numVerts/2) * 3;

	double dx1 = s1[hwIdx] - s1[0];
	double dz1 = s1[hwIdx+2] - s1[2];

	double dx2 = s2[hwIdx] - s2[0];
	double dz2 = s2[hwIdx+2] - s2[2];

	double d = 0;
	for (int i = 0; i < DIM; i += 3)
	{
		SAMPLESTYPE dx = s1[i] - s2[i];
		SAMPLESTYPE dy = s1[i+1] - s2[i+1];
		SAMPLESTYPE dz = s1[i+2] - s2[i+2];

		//d += std::sqrt(dx*dx + dy*dy + dz*dz);
		d += (dx*dx + dy*dy + dz*dz);
	}
	d /= (float)DIM;

	if (dx1 * dx2 < 0 && (dz1 < 0 && dz2 < 0))
		d *= 100.0f;

	return d;
}


#endif