////////////////////////////////////////////////////////////////////////
// Interpolator.h , Sept 12th 2011
// generic class for Displacement Interpolation 
// author: Nicolas Bonneel
// Copyright 2011 Nicolas Bonneel, Michiel van de Panne, Sylvain Paris, Wolfgang Heidrich.
// All rights reserved. 
// Web: http://www.cs.ubc.ca/labs/imager/tr/2011/DisplacementInterpolation/
//
// The Interpolator is free software: you can 
// redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Note : This file, adapted from the original implementation,
// is intended to be easy to integrate to existing projects
// rather than to be "clean" or efficient. It is thus a single header file 
// encompassing all the necessary classes, originally in multiple files.
// Many classes are present for the sole purpose of passing data to 
// windows threads, which make the code even uglier. 
// For Linux users, multi-threading is not supported here.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty 
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
// See the GNU General Public License for more details.
//
// If you use this code for research purpose, please reference
// the following paper :
//
// Nicolas Bonneel, Michiel van de Panne, Sylvain Paris, Wolfgang Heidrich
// "Diplacement Interpolation using Lagrangian Mass Transport", 
// ACM Transactions on Graphics (Proceedings of SIGGRAPH ASIA 2011)
//
//
//  
// HISTORY:
// 
// 13/02/2012 : Fixed a bug when Nlevels==0. Bug not present in the submission.
// Fixed the helper function Slerp, not actually used here (does not compile if used).
// Fixed the Vector class when DIM==3 (does not compile if used)
// changed abs to fabs, since the only prototype for GCC seems int abs(int)
//
// 12/09/2011 : Cleaned up the submission source code. Known bug : the 
// provided example returns 0 when compiled with GCC
//
///////////////////////////////////////////////////////////////////////////


#ifndef __INTERPOLATOR__
#define __INTERPOLATOR__

#include <algorithm>
#include <vector>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <iostream>

#include <lemon/lgf_reader.h>
#include <lemon/concepts/heap.h>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/network_simplex_simple.h>
#include <lemon/concepts/digraph.h>
#include <lemon/full_bipartitegraph.h>

extern "C"
{
#include "nnls.h"
}

#ifdef WIN32
#include <windows.h>
#else
#define WINAPI 
#define LPVOID void*
#define DWORD int
#endif

using namespace lemon;
template<int DIM, typename TYPE> class Vector;
struct TsFlow {
	int from;		// Feature number in signature 1 
	int to;			// Feature number in signature 2 
	double amount;	// Amount of flow from signature1.features[from] to signature2.features[to]
};

// some examples of interpolation function that you can use for the particles...
// ...if you're working on a Euclidean space in R^{DIM}
template<int DIM, typename SAMPLESTYPE> double sqrDistLinear(const Vector<DIM,SAMPLESTYPE> &F1, const Vector<DIM,SAMPLESTYPE> &F2);
template<int DIM, typename SAMPLESTYPE> double rbfFuncLinear(const Vector<DIM,SAMPLESTYPE> &center, const Vector<DIM,SAMPLESTYPE> &eval, double r2);
template<int DIM, typename SAMPLESTYPE> Vector<DIM,SAMPLESTYPE> interpolateBinsLinear(const Vector<DIM,SAMPLESTYPE>  &a, const Vector<DIM,SAMPLESTYPE>  &b, double alpha);

// ...or if you're working on the sphere, for the BRDF for example
template<int DIM, typename SAMPLESTYPE> double sqrDistBRDF(const Vector<DIM,SAMPLESTYPE> &F1, const Vector<DIM,SAMPLESTYPE> &F2);
template<int DIM, typename SAMPLESTYPE> double rbfFuncBRDF(const Vector<DIM,SAMPLESTYPE> &center, const Vector<DIM,SAMPLESTYPE> &eval, double r2);
template<int DIM, typename SAMPLESTYPE> Vector<DIM,SAMPLESTYPE> interpolateBinsBRDF(const Vector<DIM,SAMPLESTYPE> &a, const Vector<DIM,SAMPLESTYPE> &b, double alpha);


// forward declarations - you can skip that, they are not even needed with Visual Studio
template<int DIM, typename SAMPLESTYPE> struct	NNDistData;
template<int DIM, typename SAMPLESTYPE> struct	RBFData;
template<int DIM, typename SAMPLESTYPE> struct KernelData;
template<int DIM, typename SAMPLESTYPE> struct FilteringData;
template<int DIM, typename SAMPLESTYPE> struct InterpModeData;
template<int DIM, typename SAMPLESTYPE> DWORD WINAPI computeRBFs(LPVOID lpParam );
template<int DIM, typename SAMPLESTYPE> DWORD WINAPI computeNNdist( LPVOID lpParam );
template<int DIM, typename SAMPLESTYPE> DWORD WINAPI computeKernelMatrix( LPVOID lpParam );
template<int DIM, typename SAMPLESTYPE> DWORD WINAPI filterSamples(LPVOID lpParam );
template<int DIM, typename SAMPLESTYPE> DWORD WINAPI interpMode(LPVOID lpParam );
double meanVec(const std::vector<double> &vec);
void minVec(std::vector<double> &vec, double val);
void maxVec(std::vector<double> &vec, double val);
void absVec(std::vector<double> &vec);
int nnzVec(const std::vector<double> &vec, double threshold);
std::vector<double> operator*(double val, const std::vector<double> &vec);
std::vector<double> operator+(const std::vector<double> &vec1, const std::vector<double> &vec2);
std::vector<double> operator-(const std::vector<double> &vec1, const std::vector<double> &vec2);

// just because std::min/max often fail at compile time
template<typename T> inline T my_min(T x, T y) {return x<y?x:y;};
template<typename T> inline T my_max(T x, T y) {return x>y?x:y;};


// and the interpolation class itself
template<int DIM, typename SAMPLESTYPE=double>
class Interpolator
{
public:
	Interpolator(const std::vector<Vector<DIM,SAMPLESTYPE> > &pA, 
				 const std::vector<double> &pwA, 
				 const std::vector<Vector<DIM,SAMPLESTYPE> > &pB, 
				 const std::vector<double> &pwB, 
				 double (*pSqrDist)(const Vector<DIM,SAMPLESTYPE> &, 
									const Vector<DIM,SAMPLESTYPE> &), 
				 double (*pKernel)(const Vector<DIM,SAMPLESTYPE> &, 
								   const Vector<DIM,SAMPLESTYPE> &, 
								   double),
				 Vector<DIM,SAMPLESTYPE> (*pInterpolateBins)(const Vector<DIM,SAMPLESTYPE> &, 
															 const Vector<DIM,SAMPLESTYPE> &, 
															 double alpha), 
				 double pkNNDist=2.0, 
				 unsigned int pNLevels=1) :
		A(pA), B(pB),        // the location in space of each sample point
		wA(pwA), wB(pwB),    // the value of each sample point
		sqrDist(pSqrDist),   // the cost of the particle travelling, typically a squared geodesic distance
		kernel(pKernel),     // the kernel, typically gaussian, to represent each particle
		interpolateBins(pInterpolateBins), // the function to move particles along the geodesics
		kNNDist(pkNNDist),   // the standard deviation of the particles kernel, expressed in terms of distance to N'th nearest neighbor (1.5 means the distance halfway between the first and second nearest neighbor)
		NLevels(pNLevels),   // the number of frequency bands
		epsilonThreshold(1E-8),    // threshold for throwing away insignificant input data		
		numberOfNN(my_min((unsigned int)pwA.size(),
						  my_min((unsigned int)pwB.size(),
								 my_max((unsigned int)500,
										(unsigned int)floor(pkNNDist+2.5)
									   )
								)
						 ))		// number of nearest neighbors to precompute
	{ };

		////////////////////////////////////////////////////////////////////////
		/////////////////// Core of the distribution interpolation /////////////
		////////////////////////////////////////////////////////////////////////

		void precompute()
		{
			NNDistData<DIM, SAMPLESTYPE> dataA(A, numberOfNN, allNNA, allNNidA, sqrDist);
			NNDistData<DIM, SAMPLESTYPE> dataB(B, numberOfNN, allNNB, allNNidB, sqrDist);

#ifdef WIN32
			SYSTEM_INFO sysinfo;
			GetSystemInfo( &sysinfo );
			mProcessNum = sysinfo.dwNumberOfProcessors;  //number of cores

			HANDLE threads[2];

			// find variances of RBF based on Nearest Neighbors (NN)
			threads[0] = CreateThread( NULL, 0, computeNNdist<DIM,SAMPLESTYPE>, (void*) &dataA, 0, NULL);  
			threads[1] = CreateThread( NULL, 0, computeNNdist<DIM,SAMPLESTYPE>, (void*) &dataB, 0, NULL);  
			WaitForMultipleObjects( 2, threads, TRUE, INFINITE);
			CloseHandle(threads[0]);
			CloseHandle(threads[1]);
#else
			computeNNdist<DIM,SAMPLESTYPE>((void*) &dataA);
			computeNNdist<DIM,SAMPLESTYPE>((void*) &dataB);
#endif

			int id = (int)floor(kNNDist);
			double frac = kNNDist - id;
			if (id==0)
			{
				variancesA = frac*allNNA[1];  // first NN  // beware : if using Epanechnikov kernel instead, the kernel is exactly 0 at the first NN, so use the second NN here
				variancesB = frac*allNNB[1];
			}
			else
			{
				variancesA = (1.-frac)*allNNA[id] + frac*allNNA[id+1];
				variancesB = (1.-frac)*allNNB[id] + frac*allNNB[id+1];
			}


			KernelData<DIM, SAMPLESTYPE> dataKerA(A, variancesA, kernelMatrixA, kernel);
			KernelData<DIM, SAMPLESTYPE> dataKerB(B, variancesB, kernelMatrixB, kernel);
#ifdef WIN32
			threads[0] = CreateThread( NULL, 0, computeKernelMatrix<DIM,SAMPLESTYPE>, (void*) &dataKerA, 0, NULL);  
			threads[1] = CreateThread( NULL, 0, computeKernelMatrix<DIM,SAMPLESTYPE>, (void*) &dataKerB, 0, NULL);  
			WaitForMultipleObjects( 2, threads, TRUE, INFINITE);
			CloseHandle(threads[0]);
			CloseHandle(threads[1]);
#else
			computeKernelMatrix<DIM,SAMPLESTYPE>((void*) &dataKerA);
			computeKernelMatrix<DIM,SAMPLESTYPE>((void*) &dataKerB);
#endif

			std::vector<double> savedwA(wA);
			std::vector<double> savedwB(wB);
			std::vector<double> previousResolutionA(wA);
			std::vector<double> previousResolutionB(wB);

			unsigned int sizeRBFs = my_max((unsigned int)2, NLevels*2-2);
			resultFlow.resize(sizeRBFs);
			sumWA.resize(sizeRBFs);
			sumWB.resize(sizeRBFs);

			std::vector<double> signalA;
			std::vector<double> signalB;

			wRBFA.resize(sizeRBFs);
			wRBFB.resize(sizeRBFs);
			meanValA = 0;
			meanValB = 0;

			int funcID = 0;
			for (unsigned int k=0; k<NLevels; k++)
			{
				// filter data
				std::vector<double> filteredwA(wA.size(), 0.), 
									filteredwB(wB.size(), 0.);

				double band = (1<<k) / (double)(1<<(NLevels-1));
				band*=band;

				if (k==NLevels-2) // This is the coarsest level : just average (constant) and linear blend
				{
					meanValA = meanVec(savedwA);
					meanValB = meanVec(savedwB);

					std::fill(filteredwA.begin(), filteredwA.end(), meanValA);
					std::fill(filteredwB.begin(), filteredwB.end(), meanValB);
					signalA = previousResolutionA - filteredwA;
					signalB = previousResolutionB - filteredwB;
					previousResolutionA = filteredwA;
					previousResolutionB = filteredwB;
				}
				else 
				{
					if (k<NLevels-2 && NLevels!=1) // filtered
					{
						FilteringData<DIM, SAMPLESTYPE> filtDataA(allNNidA, A,savedwA, filteredwA,  band, sqrDist, kernel);
						FilteringData<DIM, SAMPLESTYPE> filtDataB(allNNidB, B,  savedwB, filteredwB, band, sqrDist, kernel);
#ifdef WIN32
						threads[0] = CreateThread( NULL, 0, filterSamples<DIM,SAMPLESTYPE>, (void*) &filtDataA, 0, NULL);  
						threads[1] = CreateThread( NULL, 0, filterSamples<DIM,SAMPLESTYPE>, (void*) &filtDataB, 0, NULL);  
						WaitForMultipleObjects( 2, threads, TRUE, INFINITE);
						CloseHandle(threads[0]);
						CloseHandle(threads[1]);
#else
						filterSamples<DIM,SAMPLESTYPE>((void*) &filtDataA);
						filterSamples<DIM,SAMPLESTYPE>((void*) &filtDataB);
#endif
						signalA = previousResolutionA - filteredwA;
						signalB = previousResolutionB - filteredwB;
						previousResolutionA = filteredwA;
						previousResolutionB = filteredwB;
					}
					else // residual
					{
						signalA = previousResolutionA;
						signalB = previousResolutionB;
					}
				}

				// signalA/signalB are weights at current level

				if ((k==NLevels-1)&&(NLevels>1))
					continue;


				for (int i=0; i<2; i++)
				{
					wA = signalA;
					wB = signalB;

					if (i==0) // we consider only the positive parts for the advection
					{
						maxVec(wA, 0.);
						maxVec(wB, 0.);
					}
					else  // we consider only the negative parts for the advection
					{
						minVec(wA, 0.);
						minVec(wB, 0.);
						absVec(wA);
						absVec(wB);
					}
					if (nnzVec(wA, epsilonThreshold)==0 && nnzVec(wB, epsilonThreshold)==0) continue;


					// compute RBF decomposition at increasing resolution
					RBFData<DIM, SAMPLESTYPE> rbfA(A, wA, wRBFA[funcID], kernelMatrixA, epsilonThreshold);
					RBFData<DIM, SAMPLESTYPE> rbfB(B, wB, wRBFB[funcID], kernelMatrixB, epsilonThreshold);

#ifdef WIN32
					threads[0] = CreateThread( NULL, 0, computeRBFs<DIM,SAMPLESTYPE>, (void*) &rbfA, 0, NULL);  					
					threads[1] = CreateThread( NULL, 0, computeRBFs<DIM,SAMPLESTYPE>, (void*) &rbfB, 0, NULL);  	
					WaitForMultipleObjects( 2, threads, TRUE, INFINITE);	
					CloseHandle(threads[0]);
					CloseHandle(threads[1]);
#else
					computeRBFs<DIM,SAMPLESTYPE>(&rbfA);
					computeRBFs<DIM,SAMPLESTYPE>(&rbfB);
#endif

					funcID++;
				}
			} // for k < NLevels

			// clears some memory before computing EMD
			kernelMatrixA.clear();
			kernelMatrixB.clear();

			std::vector<Vector<DIM,SAMPLESTYPE> > Asaved(A);
			std::vector<Vector<DIM,SAMPLESTYPE> > Bsaved(B);
			std::vector<double> wAsaved(wA);
			std::vector<double> wBsaved(wB);
			std::vector<double> variancesAsaved(variancesA);
			std::vector<double> variancesBsaved(variancesB);

			std::vector<int> removedMapA;
			std::vector<int> removedMapB;

			funcID = 0;
			for (unsigned int k=0; k<NLevels; k++)
			{
				if ((k==NLevels-1)&&(NLevels>1))
					continue;

				for (int i=0; i<2; i++)
				{
					// remove all samples with 0 weight
					nonZerosRBFs(A,wA,wRBFA[funcID],variancesA, removedMapA);
					nonZerosRBFs(B,wB,wRBFB[funcID],variancesB, removedMapB);

					// compute the Earth Mover's Distance between A and B
					double dAB = computeEMD(funcID);		
					std::cout <<"emd="<<dAB<<std::endl;

					// since there were some zeros which didn't need to be advected, let's put them back
					A = Asaved; wA = wAsaved; variancesA = variancesAsaved;
					B = Bsaved; wB = wBsaved; variancesB = variancesBsaved;
					for (unsigned int j=0; j<resultFlow[funcID].size(); j++){
						resultFlow[funcID][j].from = removedMapA[resultFlow[funcID][j].from];
						resultFlow[funcID][j].to = removedMapB[resultFlow[funcID][j].to];
					}

					funcID++;
				}
			}

			std::cout << "precomputation done"<<std::endl;
		}


		void interpolate(double alpha, const std::vector<Vector<DIM,SAMPLESTYPE> > &samples, std::vector<double> &wResult)
		{
			wResult.resize(samples.size(), 0.);  
			std::fill(wResult.begin(), wResult.end(), (1.-alpha)*meanValA+alpha*meanValB); // initialize with the interpolated DC

			modes.resize(resultFlow.size());
			for (unsigned int i=0; i<resultFlow.size(); i++)
			{
				modes[i].resize(samples.size());
				std::fill(modes[i].begin(), modes[i].end(), 0.);
			}

#ifdef WIN32
			unsigned int nLaunch = min(mProcessNum, resultFlow.size());
			HANDLE* threads = new HANDLE[nLaunch];
#endif

			unsigned int numMode = 0;
			while(numMode<resultFlow.size())
			{
#ifdef WIN32
				unsigned int numLaunchReal = min(nLaunch, resultFlow.size()-numMode);

				std::vector<InterpModeData<DIM, SAMPLESTYPE> > dataProc(numLaunchReal, InterpModeData<DIM, SAMPLESTYPE>(0,alpha,resultFlow, interpolateBins, modes, kernel, variancesA, variancesB, A, B, sumWA, sumWB, samples)) ;
				for (unsigned int k=0; k<numLaunchReal; k++) {
					dataProc[k].i = numMode;
					numMode++;
				}
				for (unsigned int k=0; k<numLaunchReal; k++)
					threads[k] = CreateThread( NULL, 0, interpMode<DIM,SAMPLESTYPE>, (void*) &dataProc[k], 0, NULL);  

				WaitForMultipleObjects( numLaunchReal, threads, TRUE, INFINITE);
				for (unsigned int k=0; k<numLaunchReal; k++)
					CloseHandle(threads[k]);
#else
				InterpModeData<DIM, SAMPLESTYPE> data(numMode,alpha,resultFlow, interpolateBins, modes, kernel, variancesA, variancesB, A, B, sumWA, sumWB, samples);
				interpMode<DIM,SAMPLESTYPE>((void*) &data);
				numMode++;
#endif
			}

			for (unsigned int i=0; i<modes.size(); i++)
				for (unsigned int n=0; n<samples.size(); n++)
					wResult[n]+=modes[i][n];

#ifdef WIN32
			delete[] threads;
#endif
		}
	

private:

	// normalize and quantize the two histograms, and make sure that the first one has the smallest sum (the two sums should be equals, but due to the quantization, this may not be the case, and it can lead to bugs in LEMON)
	static bool normalizeAndSwap(std::vector<Vector<DIM,SAMPLESTYPE> > &hist1, std::vector<Vector<DIM,SAMPLESTYPE> > &hist2, std::vector<double> &samples1, std::vector<double> &samples2, double &stretchVal, double &stretchDist)
	{
		bool swapped = false;
		stretchDist = 1000.;
		// first normalize
		double ds1 = 0., ds2 = 0.;
		for (unsigned int i=0; i<samples1.size(); i++)	ds1+=samples1[i];
		for (unsigned int i=0; i<samples2.size(); i++)	ds2+=samples2[i];
		for (unsigned int i=0; i<samples1.size(); i++)	samples1[i]/=ds1;
		for (unsigned int i=0; i<samples2.size(); i++)	samples2[i]/=ds2;

		double stretch1 = samples1.size();
		double stretch2 = samples2.size();
		stretchVal = my_max(stretch1,stretch2)*1000.; 

		// next, quantize
		int s1 = 0, s2 = 0;
		for (unsigned int i=0; i<samples1.size(); i++)
			s1+=(int)(samples1[i]*stretchVal+0.5);  // +0.5 in order to round instead of truncating
		for (unsigned int i=0; i<samples2.size(); i++)
			s2+=(int)(samples2[i]*stretchVal+0.5);

		// find the largest quantized one
		double diff = 0;
		for (unsigned int i=0; i<samples1.size(); i++)
			diff+=(int)(samples1[i]*stretchVal+0.5);
		for (unsigned int i=0; i<samples2.size(); i++)
			diff+=(int)(-(samples2[i]*stretchVal+0.5));

		// put the smallest first
		if (diff>0)
		{
			swap(samples1, samples2);
			swap(hist1, hist2);
			swapped = true;
		}
		return swapped;
	}

	static void minCostFlow( std::vector<Vector<DIM,SAMPLESTYPE> > hist1, std::vector<Vector<DIM,SAMPLESTYPE> > hist2, std::vector<double> samples1, std::vector<double> samples2, double (*distance)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &), std::vector<TsFlow> &flow, double &resultdist)
	{
		typedef FullBipartiteDigraph Digraph;
		DIGRAPH_TYPEDEFS(FullBipartiteDigraph);

		double stretchVal;
		double stretchDist;
		bool swapped = normalizeAndSwap(hist1, hist2, samples1, samples2, stretchVal, stretchDist);

		Digraph di((int)hist1.size(), (int)hist2.size());
		NetworkSimplexSimple<Digraph,int,int, unsigned short int> net(di, true);

		int idarc = 0;
		for (unsigned int i=0; i<hist1.size(); i++)
			for (unsigned int j=0; j<hist2.size(); j++)
			{
				double d = distance(hist1[i], hist2[j]);
				net.setCost(di.arcFromId(idarc), (int) (stretchDist*d+0.5)); // quantize the cost
				idarc++;
			}

			Digraph::NodeMap<int> nodeSuppliesDemand(di);

			for (unsigned int i=0; i<hist1.size(); i++)
				nodeSuppliesDemand[ di.nodeFromId(i) ] = (int)(samples1[i]*stretchVal+0.5);
			for (unsigned int i=0; i<hist2.size(); i++)
				nodeSuppliesDemand[ di.nodeFromId(i+(unsigned int)hist1.size()) ] = (int)(-(samples2[i]*stretchVal+0.5));


			net.supplyMap(nodeSuppliesDemand);

			int ret = net.run(NetworkSimplexSimple<Digraph,int,int, unsigned short int>::BLOCK_SEARCH);
			resultdist = net.totalCost()/(stretchVal*stretchDist);
			flow.reserve(hist1.size()+hist2.size()-1);

			for (unsigned int i=0; i<hist1.size(); i++) {
				for (unsigned int j=0; j<hist2.size(); j++)
				{
					TsFlow f;
					f.amount = net.flow(di.arcFromId(i*(unsigned int)hist2.size()+j))/stretchVal;
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


	double computeEMD(int k) // k = frequency band index 
	{
		int nbClassesA = (int)A.size();
		int nbClassesB = (int)B.size();
		sumWA[k]=0;
		sumWB[k]=0;

		if (nbClassesA==0 || nbClassesB==0) { // either the target or the source distribution is empty => no flow
			resultFlow[k].clear();
			return 0.;
		}
		for (unsigned int i=0; i<wRBFA[k].size(); i++)
			sumWA[k]+=wRBFA[k][i];
		for (unsigned int i=0; i<wRBFB[k].size(); i++)
			sumWB[k]+=wRBFB[k][i];

		std::vector<double> w1(nbClassesA, 0.);
		std::vector<double> w2(nbClassesB, 0.);
		if (fabs(sumWA[k])>0)
			for (int i=0; i<nbClassesA; i++)
				w1[i] = wRBFA[k][i]/sumWA[k];
		if (fabs(sumWB[k])>0)
			for (int i=0; i<nbClassesB; i++)
				w2[i] = wRBFB[k][i]/sumWB[k];

		double d1;
		minCostFlow(A, B, w1, w2, sqrDist, resultFlow[k], d1);
		return d1;
	}

	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&);
	double (*kernel)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&, double);	
	Vector<DIM,SAMPLESTYPE>  (*interpolateBins)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double alpha);
	std::vector<Vector<DIM,SAMPLESTYPE> > A;	// sample locations (bins)
	std::vector<Vector<DIM,SAMPLESTYPE> > B;
	std::vector<double> wA;						// sample weight/value/mass
	std::vector<double> wB;
	std::vector<double> sumWA, sumWB;
	double epsilonThreshold;
	unsigned int mProcessNum; // number of cores
	unsigned int NLevels;

	std::vector<double> variancesA;			// the variance actually used for the RBF
	std::vector<double> variancesB;
	std::vector<std::vector<double> > allNNA;  // distance of the k-NN to each sample
	std::vector<std::vector<double> > allNNB;
	std::vector<std::vector<int> > allNNidA;  // distance of the k-NN to each sample
	std::vector<std::vector<int> > allNNidB;

	std::vector<std::vector<double> > wRBFA;					// weights of each RBF
	std::vector<std::vector<double> > wRBFB;

	const unsigned int numberOfNN;

	std::vector<std::vector<TsFlow> > resultFlow;  // EMD flow between A and B
	
	double kNNDist;	
	std::vector<float> kernelMatrixA;
	std::vector<float> kernelMatrixB;
	
	double meanValA;
	double meanValB;
	std::vector<std::vector<double> > modes;

};



/////////////////////////////////////////////////////////////////////
//////////////////////  OTHER CLASSES DEFINITIONS ///////////////////
/////////////////////////////////////////////////////////////////////


template<int DIM, typename TYPE> class Vector
{
public:
	Vector(){for(int i=0; i<DIM; i++) data[i]=(TYPE)0;};
	Vector(TYPE x, TYPE y) {assert(DIM==2); data[0]=x; data[1]=y;};
	Vector(TYPE x, TYPE y, TYPE z) {assert(DIM==3); data[0]=x; data[1]=y; data[2]=z;};
	Vector(const Vector<DIM,TYPE> &rhs){for(int i=0; i<DIM; i++) data[i]=rhs[i];};
	TYPE operator[](int i) const {return data[i];};
	TYPE& operator[](int i) {return data[i];};
	Vector<DIM,TYPE> operator*(double rhs) const {
		Vector<DIM,TYPE> result;
		for (int i=0; i<DIM; i++)
			result[i] = data[i]*rhs;
		return result;
	}
	Vector<DIM,TYPE> operator/(double rhs) const {
		Vector<DIM,TYPE> result;
		for (int i=0; i<DIM; i++)
			result[i] = data[i]/rhs;
		return result;
	}
	Vector<DIM,TYPE>& operator/=(double rhs) {		
		for (int i=0; i<DIM; i++)
			data[i] /=rhs;
		return *this;
	}
	Vector<DIM,TYPE> operator+(const Vector<DIM,TYPE> &rhs) const {
		Vector<DIM,TYPE> result;
		for (int i=0; i<DIM; i++)
			result[i] = data[i]+rhs[i];
		return result;
	}
	bool operator==(const Vector<DIM, TYPE> &rhs) const {
		for (int i=0; i<DIM; i++)
			if (fabs(data[i]-rhs[i])>1E-14) return false;
		return true;
	}
	bool operator!=(const Vector<DIM, TYPE> &rhs) const {
		for (int i=0; i<DIM; i++)
			if (fabs(data[i]-rhs[i])>1E-14) return true;
		return false;
	}
	bool operator<(const Vector<DIM, TYPE> &rhs) const {
		for (int i=0; i<DIM; i++) {
			if (data[i]<rhs[i]) return true;
			if (data[i]>rhs[i]) return false;
		}
		return false;
	}
private:
	TYPE data[DIM];
};



////// Datastructures passed to threads

// Data for the nearest neighbor search (for threads)
template<int DIM, typename SAMPLESTYPE>
struct	NNDistData
{
	NNDistData( std::vector<Vector<DIM,SAMPLESTYPE> > &pSamples, 
				unsigned int pNnearD, 
				std::vector<std::vector<double> > &pDists, 
				std::vector<std::vector<int> > &pNnid, 
				double (*pSqrDist)(const Vector<DIM,SAMPLESTYPE> &, 
								   const Vector<DIM,SAMPLESTYPE> &) ):
		samples(pSamples),
		NnearD(pNnearD), 
		dists(pDists), 
		nnid(pNnid), 
		sqrDist(pSqrDist)
	{		
	}

	std::vector<Vector<DIM,SAMPLESTYPE> > &samples;
	unsigned int NnearD;
	std::vector<std::vector<double> > &dists;
	std::vector<std::vector<int> > &nnid;
	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &);
};

template<int DIM, typename SAMPLESTYPE>
struct	RBFData
{
	RBFData(std::vector<Vector<DIM,SAMPLESTYPE> > &pSamples, std::vector<double> &pValues, std::vector<double> &pwRBF, std::vector<float> &pKernelMatrix, double pEpsilonThreshold):
	samples(pSamples),values(pValues), wRBF(pwRBF),  kernelMatrix(pKernelMatrix), epsilonThreshold(pEpsilonThreshold)
	{		
	}
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples;
	std::vector<double> &values;
	std::vector<double> &wRBF; // resulting weights of each RBF
	std::vector<float> &kernelMatrix;
	double epsilonThreshold;
};

template<int DIM, typename SAMPLESTYPE>
struct	FilteringData
{
	FilteringData(std::vector<std::vector<int> > &pAllNNid, std::vector<Vector<DIM,SAMPLESTYPE> > &pSamples, std::vector<double> &pWeights, std::vector<double> &pResult, double pVarFilter, double (*pSqrDist)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &), double (*pKernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double)):
	allNNid(pAllNNid), samples(pSamples),weights(pWeights), result(pResult), varFilter(pVarFilter), sqrDist(pSqrDist), kernel(pKernel)
	{		
	}
	std::vector<std::vector<int> > &allNNid;
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples;
	std::vector<double> &weights;
	std::vector<double> &result;
	double varFilter;
	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &);
	double (*kernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double);
};

template<int DIM, typename SAMPLESTYPE>
struct KernelData
{
	KernelData(std::vector<Vector<DIM,SAMPLESTYPE> > &pSamples, std::vector<double> &pVariances, std::vector<float> &pResult, double (*pKernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double)):
	 samples(pSamples),variances(pVariances), result(pResult), kernel(pKernel)
	{		
	}
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples;
	std::vector<double> &variances;
	std::vector<float> &result;
	double (*kernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double);
};

template<int DIM, typename SAMPLESTYPE>
struct InterpModeData
{
	InterpModeData(int pI, double pAlpha, const std::vector<std::vector<TsFlow> > &pResultFlow, Vector<DIM,SAMPLESTYPE>  (*pInterpolateBins)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double),
		std::vector<std::vector<double> > &pModes, double (*pKernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double),
		const std::vector<double> &pVariancesA, const std::vector<double> &pVariancesB,
		const std::vector<Vector<DIM,SAMPLESTYPE> > &pA, const std::vector<Vector<DIM,SAMPLESTYPE> > &pB,
		const std::vector<double> &pSumWA, const std::vector<double> &pSumWB,
		const std::vector<Vector<DIM,SAMPLESTYPE> > &pSamples):
	i(pI),alpha(pAlpha),resultFlow(pResultFlow), interpolateBins(pInterpolateBins),  modes(pModes),
	kernel(pKernel), variancesA(pVariancesA), variancesB(pVariancesB), A(pA), B(pB), sumWA(pSumWA), sumWB(pSumWB), samples(pSamples)
	{
	}

	int i; // mode number
	double alpha;
	const std::vector<std::vector<TsFlow> > &resultFlow ;
	Vector<DIM,SAMPLESTYPE>  (*interpolateBins)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double alpha);
	std::vector<std::vector<double> > &modes;
	double (*kernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double);
	const std::vector<double> &variancesA;
	const std::vector<double> &variancesB;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &A;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &B;
	const std::vector<double> &sumWA;
	const std::vector<double> &sumWB;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &samples;
};

template<typename SAMPLESTYPE>
inline SAMPLESTYPE sqr(SAMPLESTYPE x)
{
	return x*x;
}

/////////////////////////////////////////////////////////
/////////////////// Interpolation functions /////////////
////////////////////////////////////////////////////////

template<int DIM, typename SAMPLESTYPE>
double sqrDistBRDF(const Vector<DIM,SAMPLESTYPE> &F1, const Vector<DIM,SAMPLESTYPE> &F2)
{
	assert(DIM==3);
	double d1 = acos(min(1.,F1[0]*F2[0] + F1[1]*F2[1] + F1[2]*F2[2]));
	return d1*d1;
}
template<int DIM, typename SAMPLESTYPE>
double rbfFuncBRDF(const Vector<DIM,SAMPLESTYPE> &center, const Vector<DIM,SAMPLESTYPE> &eval, double r2)
{
	assert(DIM==3);
	double sqrdist = sqrDistBRDF<DIM,SAMPLESTYPE>(center, eval); 
	double stddev = sqrt(r2);
	return M_SQRTPI*exp(-sqrdist/r2)/(stddev);
}
template<int DIM, typename SAMPLESTYPE>
Vector<DIM,SAMPLESTYPE> Slerp(Vector<DIM,SAMPLESTYPE> a, Vector<DIM,SAMPLESTYPE> b, double alpha) //passed by copy : this is not a mistake : we duplicate, since we'll modify them and don't want to keep the modifs
{
	double normA=0., normB=0., sqrNormDiff=0.;
	for (int i=0; i<DIM; i++) {
		normA+=a[i]*a[i];
		normB+=b[i]*b[i];
		sqrNormDiff+=sqr(a[i]-b[i]);
	}
	if (sqrNormDiff<1E-8) return a;
	normA=sqrt(normA);
	normB=sqrt(normB);
	double adotb=0.;
	a/=normA;
	b/=normB;
	for (int i=0; i<DIM; i++) {
		adotb+=a[i]*b[i];
	}
	double angle = acos(adotb);
	double sinangle = sin(angle);
	return (a * sin( (1.-alpha)*angle) + b*sin(alpha*angle))/sinangle;
}
template<int DIM, typename SAMPLESTYPE>
Vector<DIM,SAMPLESTYPE> interpolateBinsBRDF(const Vector<DIM,SAMPLESTYPE> &a, const Vector<DIM,SAMPLESTYPE> &b, double alpha)
{
    assert(DIM==3);
	Vector<DIM,SAMPLESTYPE>  dir1(a[0], a[1], a[2]);
	Vector<DIM,SAMPLESTYPE>  dir2(b[0], b[1], b[2]);
	Vector<DIM,SAMPLESTYPE>  resultDir = Slerp( dir1, dir2, alpha);
	Vector<DIM,SAMPLESTYPE>  result(resultDir[0], resultDir[1], resultDir[2]);
	return result;
}
template<int DIM, typename SAMPLESTYPE>
Vector<DIM,SAMPLESTYPE> interpolateBinsLinear(const Vector<DIM,SAMPLESTYPE>  &a, const Vector<DIM,SAMPLESTYPE>  &b, double alpha)
{
	return a*(1.-alpha) + b*alpha;
}
template<int DIM, typename SAMPLESTYPE>
double sqrDistLinear(const Vector<DIM,SAMPLESTYPE> &F1, const Vector<DIM,SAMPLESTYPE> &F2)
{
	double d = 0;
	for (int i=0; i<DIM; i++)
		d+= sqr(F1[i]-F2[i]);
	return d;
}
template<int DIM, typename SAMPLESTYPE>
double rbfFuncLinear(const Vector<DIM,SAMPLESTYPE> &center, const Vector<DIM,SAMPLESTYPE> &eval, double r2)
{
	double sqrdist = sqrDistLinear<DIM,SAMPLESTYPE>(center, eval);
	double stddev = sqrt(r2);
	return exp(-sqrdist/(2.*r2))/(pow(2.5066, DIM)*stddev); // 2.5066 = sqrt(2*pi)
}

//--------------------------------------------------------------------------------

template<int DIM, typename SAMPLESTYPE>
void nonZerosRBFs(std::vector<Vector<DIM,SAMPLESTYPE> > &A, std::vector<double> &wA, std::vector<double> &wRBFA, std::vector<double> &variancesA, std::vector<int> &removedMapA)
{
	std::vector<Vector<DIM,SAMPLESTYPE> > A2; A2.reserve(A.size());
	std::vector<double> wA2; wA2.reserve(wA.size());
	std::vector<double> wRBFA2; wRBFA2.reserve(wRBFA.size());
	std::vector<double> variancesA2; variancesA2.reserve(variancesA.size());
	removedMapA.clear();
	removedMapA.reserve(wA.size());

	for (unsigned int i=0; i<wRBFA.size(); i++) 
	{
		if (fabs(wRBFA[i])<1E-17)
			continue;

		removedMapA.push_back(i); // keeps the indices of non zeros RBFs
		A2.push_back(A[i]);
		wA2.push_back(wA[i]);
		wRBFA2.push_back(wRBFA[i]);
		variancesA2.push_back(variancesA[i]);
	}
	A = A2;
	wA = wA2;
	wRBFA = wRBFA2;
	variancesA = variancesA2;
}




template<int DIM, typename SAMPLESTYPE>
DWORD WINAPI interpMode(LPVOID lpParam )
{
	InterpModeData<DIM,SAMPLESTYPE>* argData = (InterpModeData<DIM,SAMPLESTYPE>*) lpParam;
	int i = argData->i; // mode number
	double alpha = argData->alpha;
	const std::vector<std::vector<TsFlow> > &resultFlow = argData->resultFlow;
	Vector<DIM,SAMPLESTYPE>  (*interpolateBins)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double alpha) = argData->interpolateBins;
	std::vector<std::vector<double> > &modes = argData->modes;
	double (*kernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double) = argData->kernel;
	const std::vector<double> &variancesA = argData->variancesA;
	const std::vector<double> &variancesB = argData->variancesB;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &A = argData->A;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &B = argData->B;
	const std::vector<double> &sumWA = argData->sumWA;
	const std::vector<double> &sumWB = argData->sumWB;
	const std::vector<Vector<DIM,SAMPLESTYPE> > &samples = argData->samples;

	unsigned int flowSize = resultFlow[i].size();
	std::vector<Vector<DIM,SAMPLESTYPE> > resultCenters(flowSize);
	std::vector<double> resultWeights(flowSize);
	std::vector<double> radii(flowSize);

	for (unsigned int n=0; n<flowSize; n++) {
		int from = resultFlow[i][n].from;
		int to = resultFlow[i][n].to;

		resultCenters[n] = interpolateBins(A[from], B[to], alpha);
		resultWeights[n] =  resultFlow[i][n].amount * (sumWA[i] * (1.-alpha)  + sumWB[i] * alpha) ;
		radii[n] = variancesA[from]*(1.-alpha)+variancesB[to]*alpha; // we interpolate the variance
	}

	double powi = (i%2==0)?1.:-1.;
	for (unsigned int m=0; m<resultWeights.size(); m++)	{
		if (fabs(resultWeights[m])<1E-13) continue;
		for (unsigned int n=0; n<samples.size(); n++) {
			double w = kernel(resultCenters[m], samples[n], radii[m]);	
			modes[i][n]+=w*resultWeights[m]*powi;
		}
	}
	return 0;
}




template<int DIM, typename SAMPLESTYPE>
DWORD WINAPI filterSamples(LPVOID lpParam ) // smoothing operator
{
	FilteringData<DIM,SAMPLESTYPE>* argData = (FilteringData<DIM,SAMPLESTYPE>*) lpParam;
	std::vector<std::vector<int> > &allNNid  = argData->allNNid;
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples  = argData->samples;
	std::vector<double> &weights = argData->weights;
	std::vector<double> &result = argData->result;

	double varFilter  = argData->varFilter;
	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&) = argData->sqrDist;
	double (*kernel)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&, double) = argData->kernel;

	unsigned int numNN = allNNid.size();
	result.resize(samples.size());
	for (unsigned int i=0; i<samples.size(); i++) {
		double vari = sqrDist(samples[i], samples[allNNid[numNN-1][i]])*varFilter;
		double sumker = 0.;
		double sumfunc = 0.;
		for (unsigned int j=0; j<allNNid.size(); j++) {
			double ker = kernel(samples[i], samples[allNNid[j][i]], vari); 
			sumker+=ker;
			sumfunc+=ker*weights[allNNid[j][i]];
		}
		if (sumker!=0)
			result[i] = sumfunc/sumker;
		else
			result[i] = weights[i];
	}
	return 0;
}

template<int DIM, typename SAMPLESTYPE>
DWORD WINAPI computeKernelMatrix( LPVOID lpParam )
{
	KernelData<DIM,SAMPLESTYPE> *argData = (KernelData<DIM,SAMPLESTYPE>*) lpParam;
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples =  argData->samples;
	std::vector<double> &variances =  argData->variances;
	std::vector<float> &result =  argData->result;
	double (*kernel)(const Vector<DIM,SAMPLESTYPE> &, const Vector<DIM,SAMPLESTYPE> &, double) = argData->kernel;

	result.resize(samples.size()*samples.size());
	for( unsigned int rItr=0; rItr<samples.size(); rItr++ ) {
		for (unsigned int c=0; c<samples.size(); c++) {
			double v = kernel(samples[c], samples[rItr],variances[c]);
			result[rItr*samples.size()+c] = (float)v;
		}		
	}
	return 0;
}


template<int DIM, typename SAMPLESTYPE>
DWORD WINAPI computeRBFs(LPVOID lpParam )
{
	RBFData<DIM,SAMPLESTYPE>* argData = (RBFData<DIM,SAMPLESTYPE>*) lpParam;
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples = argData->samples;
	std::vector<double> &values = argData->values;
	std::vector<double> &wRBF = argData->wRBF;
	std::vector<float> &kernelMatrix = argData->kernelMatrix;
	double epsilonThreshold = argData->epsilonThreshold;

    double valuesThresh = epsilonThreshold;
	int nnzValues = nnzVec(values, valuesThresh);
	int M = (int) samples.size();
	int N = nnzValues; 
	std::vector<double> matrix(M*N);
	std::vector<double> valuesSauv(values); //grrr... nnls destroys the rhs

	int idx = 0;
	for (unsigned int j=0; j<samples.size(); j++) {  // the distance matrix is NOT symmetric (due to the varying variance r[i])
		if (fabs(values[j]) < valuesThresh) continue;
		for (unsigned int i=0; i<samples.size(); i++) {
			double w = kernelMatrix[i*M+j];
			matrix[i+idx*M]=w;  //warning : column major!
		}
		idx++;
	}

	std::vector<double> sol(N);
	double rnorm;
	std::vector<double> workspace1(N);
	std::vector<double> workspace2(M);
	std::vector<int> workspace3(N);
	int mode = 0;

	nnls_c(&matrix[0], &M, &M, &N, &values[0], &sol[0], &rnorm, &workspace1[0], &workspace2[0], &workspace3[0], &mode);

	wRBF.resize(samples.size());				// we removed all the zeros RBFs - let's put them back
	std::fill(wRBF.begin(), wRBF.end(), 0.);
	idx = 0;
	for (unsigned int i=0; i<samples.size(); i++) {
		if (fabs(valuesSauv[i]) < valuesThresh) continue;
		wRBF[i] = sol[idx];
		idx++;
	}
	return 0;
}



// computes a bunch of nearest neighbors, which will be used 
// either for the computation of the variance of each gaussian
// and for the laplacian pyramid
// highly unoptimized code
template<int DIM, typename SAMPLESTYPE>
DWORD WINAPI computeNNdist( LPVOID lpParam )
{
	NNDistData<DIM,SAMPLESTYPE> *argData = (NNDistData<DIM,SAMPLESTYPE>*) lpParam;
	std::vector<Vector<DIM,SAMPLESTYPE> > &samples =  argData->samples;
	unsigned int NnearD =  argData->NnearD;								// number of neasrest neighbors to gather at each sample
	std::vector<std::vector<double> > &dists = argData->dists;
	std::vector<std::vector<int> > &nnid = argData->nnid;
	double (*sqrDist)(const Vector<DIM,SAMPLESTYPE>&, const Vector<DIM,SAMPLESTYPE>&) = argData->sqrDist;

	unsigned int N = (unsigned int) samples.size();
	if (NnearD>800) // in that case a quick sort over all the data may probably faster than keeping a heap
	{		
		std::vector<std::vector<std::pair<double,int> > > alldists(N);
		for (unsigned int i=0; i<N; i++) {
			alldists[i].resize(N);
			for (unsigned int j=0; j<N; j++) {
				double curDist = sqrDist(samples[i], samples[j]);
				alldists[i][j] = std::make_pair(curDist,j);					
			}
			std::sort(alldists[i].begin(), alldists[i].end());
			alldists[i].resize(NnearD); //clear some memory
		}

		dists.resize(NnearD, std::vector<double>(N, 1.E9));
		nnid.resize(NnearD, std::vector<int>(N, 0));
		for (unsigned int i=0; i<N; i++)
			for (unsigned int j=0; j<NnearD; j++) {
				dists[j][i] = alldists[i][j].first;
				nnid[j][i] = alldists[i][j].second;
			}
	}
	else
	if (NnearD>20) { // in that case maintaining a heap is probably faster than insertion
		std::vector<std::vector<std::pair<double,int> > > alldists(N);
		
		for (unsigned int i=0; i<N; i++) {
			alldists[i].reserve(NnearD+1);

			// keeps a heap of the NnearD closest
			bool haspopped = false;
			bool remakeheap;
			for (unsigned int j=0; j<N; j++) {
				double curDist = sqrDist(samples[i], samples[j]);
				if (alldists[i].size()<=NnearD) {
					alldists[i].push_back(std::make_pair(curDist,j));	
					remakeheap = true;
				} else {
					if (curDist<alldists[i][0].first) {
						alldists[i][NnearD] = std::make_pair(curDist,j);					
						remakeheap = true;
					} else
						remakeheap = false;
				}

				if (remakeheap) {
					push_heap( alldists[i].begin(), alldists[i].end() );
					haspopped = false;
					if (alldists[i].size()>=NnearD+1) {
						pop_heap( alldists[i].begin(), alldists[i].end() );	
						haspopped = true;
					}
				}
			}
			if (haspopped)
				std::sort_heap(alldists[i].begin(), alldists[i].end()-1 );
			else
				std::sort_heap(alldists[i].begin(), alldists[i].end());
		}

		dists.resize(NnearD, std::vector<double>(N, 1.E9));
		nnid.resize(NnearD, std::vector<int>(N, 0));
		for (unsigned int i=0; i<N; i++)
			for (unsigned int j=0; j<NnearD; j++) {
				dists[j][i] = alldists[i][j].first;
				nnid[j][i] = alldists[i][j].second;
			}
	}
	else
	{
		dists.resize(NnearD, std::vector<double>(N, 1.E9));
		nnid.resize(NnearD, std::vector<int>(N, 0));
		std::vector<double> nearestDists(NnearD, 1.E9);
		std::vector<int> nearestIds(NnearD, 0);
		for (unsigned int j=0; j<N; j++) {
			std::fill(nearestDists.begin(), nearestDists.end(), 1.E9);
			for (unsigned int k=0; k<N; k++) {
				//if (j==k) continue;
				double curDist = sqrDist(samples[j], samples[k]);
				for (unsigned int p=0; p<NnearD; p++) {
					if (curDist<nearestDists[p]) {
						if (p!=NnearD-1) {
							memmove(&nearestDists[p+1],&nearestDists[p], (NnearD-p-1)*sizeof(nearestDists[0]));
							memmove(&nearestIds[p+1],&nearestIds[p], (NnearD-p-1)*sizeof(nearestIds[0]));
						}
						nearestDists[p] = curDist;
						nearestIds[p] = k;
						break;
					}
				}
			}
			for (unsigned int k=0; k<NnearD; k++) {
				dists[k][j] = nearestDists[k];
				nnid[k][j] = nearestIds[k];
			}
		}
	}
	return 0;
}



#endif