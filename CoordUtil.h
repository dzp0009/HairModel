#pragma once

#include "HairUtil.h"

// Utilities for coordinates conversion
class CoordUtil
{
public:

	static void setScalpSpaceCenter(const XMFLOAT3& center) { s_center = center; }

	static void XYZ2UV(const XMFLOAT3& xyz, XMFLOAT2& uv);

	static void UV2Tangent(const XMFLOAT2& uv, XMFLOAT3 tangent);


private:
	static XMFLOAT3		s_center;
};

