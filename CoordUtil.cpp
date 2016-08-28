#include "CoordUtil.h"

XMFLOAT3 CoordUtil::s_center;


void CoordUtil::XYZ2UV(const XMFLOAT3& xyz, XMFLOAT2& uv)
{
	//printf("XYZ %f %f %f\n", xyz.x, xyz.y, xyz.z);

	// project to unit sphere
	float x = xyz.x - s_center.x;
	float y = xyz.y - s_center.y;
	float z = xyz.z - s_center.z;

	float r = sqrtf(x*x + y*y + z*z);
	x /= r; y /= r; z /= r;

	//printf("nXYZ %f %f %f\n", x, y, z);

	float sy2 = y + 1.0f;
	sy2 = sy2 * sy2;

	// U, V
	uv.x = acosf(x / sqrtf(x*x + sy2));
	uv.y = acosf(-z / sqrtf(z*z + sy2));

	//printf("UV: %f %f\n", uv.x, uv.y);
}



void CoordUtil::UV2Tangent(const XMFLOAT2& uv, XMFLOAT3 tangent)
{
	const float ctgU = cosf(uv.x) / sinf(uv.x);
	const float ctgV = cosf(uv.y) / sinf(uv.y);
	const float h = 2.0f / (ctgU*ctgU + ctgV*ctgV + 1.0f);

	tangent.x = h * ctgU;
	tangent.y = h - 1.0f;
	tangent.z = -h * ctgV;
}