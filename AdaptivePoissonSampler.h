#pragma once

// Encapsulate Li-Yi's Poisson Disk Sampling algorithm.



class AdaptivePoissonSampler
{
public:
	static int	generate(const int		dim, 
						 const float	minDist, 
						 const int		k, 
						 const float	cellSize, 
						 const float*	range, 
						 float**		ppOutput);
};