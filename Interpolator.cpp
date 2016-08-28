#include "Interpolator.h"

struct lemon::Invalid const lemon::INVALID;


/////// Helpers

std::vector<double> operator*(double val, const std::vector<double> &vec)
{
	std::vector<double> result(vec);
	for (unsigned int i=0; i<vec.size(); i++)
		result[i]*=val;
	return result;
}
std::vector<double> operator+(const std::vector<double> &vec1, const std::vector<double> &vec2)
{
	std::vector<double> result(vec1);
	for (unsigned int i=0; i<vec1.size(); i++)
		result[i]+=vec2[i];
	return result;
}
std::vector<double> operator-(const std::vector<double> &vec1, const std::vector<double> &vec2)
{
	std::vector<double> result(vec1);
	for (unsigned int i=0; i<vec1.size(); i++)
		result[i]-=vec2[i];
	return result;
}
double meanVec(const std::vector<double> &vec)
{
	double result = 0.;
	for (unsigned int i=0; i<vec.size(); i++)
		result += vec[i];
	return result/vec.size();
}
void minVec(std::vector<double> &vec, double val)
{
	for (unsigned int i=0; i<vec.size(); i++)
		vec[i] = my_min(val, vec[i]);
}
void maxVec(std::vector<double> &vec, double val)
{
	for (unsigned int i=0; i<vec.size(); i++)
		vec[i] = my_max(val, vec[i]);
}
void absVec(std::vector<double> &vec)
{
	for (unsigned int i=0; i<vec.size(); i++)
		vec[i] = fabs(vec[i]);
}
int nnzVec(const std::vector<double> &vec, double threshold)
{
	int result = 0;
	for (unsigned int i=0; i<vec.size(); i++)
		if (fabs(vec[i])>=threshold)
			result++;
	return result;
}