//----------------------------------------------------------------------
// Filename:	Marschner.h
// Author:		Lvdi Wang
// Created:		12/29/2008
// Modified:	12/29/2008
//----------------------------------------------------------------------
// Precompute Marschner shading model in two D3D textures.
//----------------------------------------------------------------------
// Copyright (c) 2008 Lvdi Wang and Microsoft Research Asia.
// All Rights Reserved.
//----------------------------------------------------------------------


#pragma once

#include "QDXUT.h"

class Marschner
{
public:
	//------------------------------------------------------------------
	// Compute and store Marschner model in two textures.
	//------------------------------------------------------------------
	static bool CreateTextures(
		const UINT				uTexWidth,
		const UINT				uTexHeight,
		const float				fRefracIdx,
		const XMFLOAT3*			pAbsorption,
		const float				fAlphaR,
		const float				fBetaR,
		ID3D11Texture2D**		ppTexture1,
		ID3D11Texture2D**		ppTexture2
		);

private:
	//------------------------------------------------------------------
	// Longitudinal scattering function.
	//------------------------------------------------------------------
	static float ComputeM(
		const float		fAlpha,			// longitudinal shift
		const float		fBeta,			// longitudinal width
		const float		fThetaI,		// incident angle
		const float		fThetaO			// exitant angle
		);

	//------------------------------------------------------------------
	// Azimuthal scattering function for R component.
	//------------------------------------------------------------------
	static float ComputeNR(
		const float		fRefracIdx,
		const float		fThetaD,
		const float		fPhiD
		);

	//------------------------------------------------------------------
	// Azimuthal effective index of refraction.
	//------------------------------------------------------------------
	static float RefracIndexP(const float fRefracIdx, const float fThetaD);

	//------------------------------------------------------------------
	// Misc helper functions.
	//------------------------------------------------------------------
	static inline float Clamp(float x, float fMin, float fMax) { return (x < fMin) ? fMin : (x > fMax ? fMax : x); }

	//------------------------------------------------------------------
	// Save computed texture to file for debugging.
	//------------------------------------------------------------------
	//static void SaveNormalizedTexture(
	//	const LPDIRECT3DDEVICE9 pDevice,
	//	const WCHAR*			strFilename,
	//	const D3DXCOLOR*		pSrcData, 
	//	const UINT				uWidth, 
	//	const UINT				uHeight);
};

