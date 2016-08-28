#include "Marschner.h"


//----------------------------------------------------------------------
// Compute and store Marschner model in two textures.
//----------------------------------------------------------------------
bool Marschner::CreateTextures(
	const UINT				uTexWidth,
	const UINT				uTexHeight,
	const float				fRefracIdx,
	const XMFLOAT3*		pAbsorption,
	const float				fAlphaR,
	const float				fBetaR,
	ID3D11Texture2D**		ppTexture1,
	ID3D11Texture2D**		ppTexture2
	)
{
	if (!ppTexture1 || !ppTexture2)
	{
		return false;
	}

	XMFLOAT4* pTex1Data = new XMFLOAT4[uTexWidth * uTexHeight];
	XMFLOAT4* pTex2Data = new XMFLOAT4[uTexWidth * uTexHeight];

	// Compute surface properties for TT and TRT.
	//const float fAlphaTT  = -fAlphaR / 2.0f;
	const float fAlphaTRT = -3.0f*fAlphaR / 2.0f;
	//const float fBetaTT   = fBetaR / 2.0f;
	const float fBetaTRT  = 2.0f * fBetaR;

	// Compute the value at each texture coordinate
	for (UINT y = 0; y < uTexHeight; y++)
	{
		// Texture coordinate V
		const float fV = (float)y / (float)(uTexHeight-1);
		for (UINT x = 0; x < uTexWidth; x++)
		{
			// Texture coordinate U
			const UINT uIdx = y*uTexWidth + x;
			const float fU = (float)x / (float)(uTexWidth-1);

			// Compute respective angles in Tex1
			const float fThetaI = asinf(Clamp(fU*2.0f - 1.0f, -1.0f, 1.0f));
			const float fThetaO = asinf(Clamp(fV*2.0f - 1.0f, -1.0f, 1.0f));
			// Compute M_R, M_TT and M_TRT in Tex1
			// Here we assume M_TT = M_TRT.
			pTex1Data[uIdx].x = ComputeM(fAlphaR, fBetaR, fThetaI, fThetaO);
			pTex1Data[uIdx].y = //ComputeM(fAlphaTT, fBetaTT, fThetaI, fThetaO);
				pTex1Data[uIdx].z = ComputeM(fAlphaTRT, fBetaTRT, fThetaI, fThetaO);
			// Compute cos(ThetaD)
			pTex1Data[uIdx].w = (cosf((fThetaI - fThetaO)*0.5f) + 1.0f)*0.5f;

			// Compute respective angles in Tex2
			const float fPhiD = acosf(Clamp(fU*2.0f - 1.0f, -1.0f, 1.0f));
			const float fThetaD = acosf(Clamp(fV*2.0f - 1.0f, -1.0f, 1.0f));
			// Compute N_R
			pTex2Data[uIdx].w = ComputeNR(fRefracIdx, fThetaD, fPhiD);
		}
	}
	// Save textures to file for debugging.
	//SaveNormalizedTexture(pDevice, L"Marschner1.png", pTex1Data, uTexWidth, uTexHeight);


	// Create D3D textures from pre-computed data.

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width		= uTexWidth;
	desc.Height		= uTexHeight;
	desc.MipLevels	= 1;
	desc.ArraySize	= 1;
	desc.Format		= DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Usage		= D3D11_USAGE_IMMUTABLE;
	desc.BindFlags	= D3D11_BIND_SHADER_RESOURCE;
	desc.SampleDesc.Count = 1;

	D3D11_SUBRESOURCE_DATA initData;

	initData.SysMemPitch = sizeof(XMFLOAT4)*uTexWidth;
	initData.pSysMem = pTex1Data;
	if (S_OK != QDXUT::device()->CreateTexture2D(&desc, &initData, ppTexture1))
	{
		return false;
	}
	
	initData.pSysMem = pTex2Data;
	if (S_OK != QDXUT::device()->CreateTexture2D(&desc, &initData, ppTexture2))
	{
		return false;
	}

	SAFE_DELETE_ARRAY(pTex1Data);
	SAFE_DELETE_ARRAY(pTex2Data);

	return true;
}

//----------------------------------------------------------------------
// Longitudinal scattering function.
//----------------------------------------------------------------------
float Marschner::ComputeM(
	const float				fAlpha,		// longitudinal shift
	const float				fBeta,		// longitudinal width
	const float				fThetaI,	// incident angle
	const float				fThetaO		// exitant angle
	)
{
	float fThetaH = (fThetaI + fThetaO) / 2.0f;

	// M is a unit-integral lobe centered at fAlpha and with width fBeta.
	// 1/sqrt(2*PI) = 0.3989422804
	// Also, consider sin(ThetaI)...
	return /*cosf(fThetaI) */ (0.3989422804f / fBeta) *
		expf((fThetaH-fAlpha)*(fAlpha-fThetaH) / (2.0f*fBeta*fBeta));
}

//----------------------------------------------------------------------
// Azimuthal scattering function for R component.
//----------------------------------------------------------------------
float Marschner::ComputeNR(
	const float		fRefracIdx,
	const float		fThetaD,
	const float		fPhiD)
{
	// For R ($p=0$), we have $PhiD = -2*GammaI = -2*arcsin(h)$.
	// So we have the following:
	const float fGammaI = -0.5f * fPhiD;

	// The derivative of $PhiD$ with respect to $h$ is
	// $-2 / sqrt(1 - h^2)$, which is $-2 / cos(GammaI)$
	// The exitant intensity is therefore $|cos(GammaI)/4|$:
	const float fCosGammaI = cosf(fGammaI);
	const float fL = fabs(fCosGammaI) * 0.25f;

	// Account for Fresnel attenuation
	const float fCosThetaD = cosf(fThetaD);
	float temp = fCosThetaD*fCosGammaI;
	XMVECTOR cosInc = XMLoadFloat(&temp);
	XMVECTOR refIdx = XMLoadFloat(&fRefracIdx);
	const float fFresnel = XMVectorGetX(XMFresnelTerm(cosInc, refIdx));

	return fL*fFresnel;///(fCosThetaD*fCosThetaD);
}

//----------------------------------------------------------------------
// Azimuthal effective index of refraction.
//----------------------------------------------------------------------
float Marschner::RefracIndexP(const float fRefracIdx, const float fThetaD)
{
	float fSinThetaD = sinf(fThetaD);
	float fCosThetaD = cosf(fThetaD);

	return sqrtf(fRefracIdx*fRefracIdx - fSinThetaD*fSinThetaD)/fCosThetaD;
}
