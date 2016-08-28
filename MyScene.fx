#include "QDXUT/QDXCommon.fxh"

#include "MorphDef.h"

cbuffer cbGlobalParams
{
	float4	g_hairColor;
	float	g_imageOpacity;
	float	g_hairWidth;
	float2	g_viewportSize;

	float	g_tangentTexScaleX;
	float	g_tangentTexScaleY;

	float	g_tangentTexWeight;
	float	g_normalGradWeight;
	float	g_strandRandWeight;
	float	g_forceOpaque;

	float	g_bodyAO;
	float	g_hairAO;

	float	g_ka;
	float	g_kd;
	float	g_kr;

	float	g_bodyDepth;
	
	float	g_neckBlend;

	matrix	g_transferMat;
};

cbuffer cbMorphParams
{
	float	g_morphWeight;
	float	g_hairOpacity;

	float	g_morphWeights[MAX_NUM_MORPH_SRC];
	int		g_numSources;
};


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

struct QuadVertexIn
{
	float4	position	: POSITION;
	float2	texcoord	: TEXCOORD0;
	float3	reserved	: TEXCOORD1;
};

struct QuadVertexOut
{
	float4	position	: SV_POSITION;
	float2	texcoord	: TEXCOORD0;
	float	ao			: TEXCOORD1;
};

//---------------------------------------------------------

Texture2D	g_texture;	// image sprite image

Texture2D	g_texHairNoise;
Texture2D	g_texNormalGradient;
Texture2D	g_texTipNoise;

Texture2D	g_marschner1;
Texture2D	g_marschner2;

Texture2D	g_headTexture;

Texture2D	g_headMorphSrcTex;
Texture2D	g_headMorphDstTex;

Texture2D	g_headSrcTex0;
Texture2D	g_headSrcTex1;

#if MAX_NUM_MORPH_SRC > 2
Texture2D	g_headSrcTex2;
#endif
#if MAX_NUM_MORPH_SRC > 3
Texture2D	g_headSrcTex3;
#endif
#if MAX_NUM_MORPH_SRC > 4
Texture2D	g_headSrcTex4;
#endif

//---------------------------------------------------------

struct StrandVertexIn
{
	float4	position	: POSITION;
	float3	tangent		: TANGENT;
	float4	color		: COLOR0;
	float2	texcoord	: TEXCOORD0;
	float4	shading		: TEXCOORD1;
};

struct StrandVertexOut
{
	float4	position	: SV_POSITION;
	float4	color		: COLOR0;
	float2	texcoord	: TEXCOORD0;
	float3	tangent		: TEXCOORD1;
	float	distToCenter: TEXCOORD2;
	float3	marcoord	: TEXCOORD3;
};

//---------------------------------------------------------

struct HeadVertexIn
{
	float4	position	: POSITION;
	float3	normal		: NORMAL;
	float2	texcoord	: TEXCOORD0;
	float4	reserved	: TEXCOORD1;
};

struct HeadVertexOut
{
	float4	position	: SV_POSITION;
	float2	texcoord	: TEXCOORD0;
	float3	worldPos	: TEXCOORD1;
	float	ao			: TEXCOORD2;
};

//---------------------------------------------------------

struct HeadMorphVertexIn
{
	float3	pos0		: POSITION;
	float3	normal		: NORMAL;
	float2	texcoord	: TEXCOORD0;
	float3	pos1		: TEXCOORD1;
#if MAX_NUM_MORPH_SRC > 2
	float3	pos2		: TEXCOORD2;
#endif
#if MAX_NUM_MORPH_SRC > 3
	float3	pos3		: TEXCOORD3;
#endif
#if MAX_NUM_MORPH_SRC > 4
	float3	pos4		: TEXCOORD4;
#endif
};


struct StrandMorphVertexIn
{
	float3	pos0		: POSITION;
	float3	pos1		: TANGENT;
	float4	color0		: COLOR0;
	float2	texcoord	: TEXCOORD0;
	float4	color1		: TEXCOORD1;
#if MAX_NUM_MORPH_SRC > 2
	float3	pos2		: TEXCOORD2;
	float4	color2		: TEXCOORD3;
#endif
#if MAX_NUM_MORPH_SRC > 3
	float3	pos3		: TEXCOORD4;
	float4	color3		: TEXCOORD5;
#endif
#if MAX_NUM_MORPH_SRC > 4
	float3	pos4		: TEXCOORD6;
	float4	color4		: TEXCOORD7;
#endif
};


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

SamplerState ImageSampler
{
	Filter	 = MIN_MAG_MIP_POINT;
};

SamplerState LinearSampler
{
	Filter	 = MIN_MAG_MIP_LINEAR;
};

SamplerState NoiseSampler
{
	Filter	 = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

SamplerState MarschnerSampler
{
	Filter	 = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

QuadVertexOut ImageVS( QuadVertexIn input )
{
	QuadVertexOut output;
	
	float4 worldPos = mul(input.position, g_world);
	output.position = mul(worldPos, g_viewProjection);
	output.texcoord = input.texcoord;

	output.ao = lerp(1.0, input.reserved.x, g_bodyAO);

	return output;
}

float4 ImagePS( QuadVertexOut input ) : SV_TARGET0
{
	float4 color = g_texture.Sample(ImageSampler, input.texcoord);
	color.a *= g_imageOpacity;

	if (g_selected)
	{
		color = lerp(color, float4(1, 0.6, 0, 1), 0.5);
	}

	return color;
}

float4 ImageLayerPS( QuadVertexOut input ) : SV_TARGET0
{
	return g_texture.Sample(LinearSampler, input.texcoord);
}

float4 BodyLayerPS( QuadVertexOut input ) : SV_TARGET0
{
	float4 color = g_texture.Sample(LinearSampler, input.texcoord);

	color.rgb *= input.ao;

	return color;
}

//---------------------------------------------------------


StrandVertexOut StrandVS( StrandVertexIn input )
{
	StrandVertexOut output;
	
	float4 worldPos = mul(input.position, g_world);
	worldPos        = mul(worldPos, g_transferMat);
	float4 viewPos  = mul(worldPos, g_view);
	output.position = mul(viewPos, g_projection);                                                                                           

	float d = viewPos.z * g_projection._34 + g_projection._44;
	
	output.tangent		= input.tangent;
	output.color		= input.color;
	output.texcoord		= input.texcoord;
	output.distToCenter	= 0.5f * g_hairWidth * g_projection._11 / d;
	output.marcoord		= 0.0.xxx;

	return output;
}


StrandVertexOut StrandShadingVS( StrandVertexIn input )
{
	StrandVertexOut output;
	
	matrix worldTransfer = mul(g_world, g_transferMat);

	float4 worldPos = mul(input.position, worldTransfer);
	float4 viewPos  = mul(worldPos, g_view);
	output.position = mul(viewPos, g_projection);                                                                                           

	float d = viewPos.z * g_projection._34 + g_projection._44;
	
	output.tangent		= input.tangent;
	output.color		= input.color;
	output.texcoord		= input.texcoord;
	output.distToCenter	= 0.5f * g_hairWidth * g_projection._11 / d;

	float3 g_lightDir = normalize(float3(1, 1, -1));

	// Shading
	float3 cameraDir  = normalize(g_cameraPos - worldPos.xyz);
	float  sinThetaI  = dot(g_lightDir, input.tangent);
	float  sinThetaO  = dot(cameraDir, input.tangent);
	float3 lightPerp  = g_lightDir - sinThetaI*input.tangent;
	float3 cameraPerp = cameraDir  - sinThetaO*input.tangent;

	float  cosPhiD    = dot(cameraPerp, lightPerp) * rsqrt(
						dot(cameraPerp, cameraPerp) * dot(lightPerp, lightPerp));

	float3 ambient = g_ka.xxx;
	float3 diffuse = g_kd.xxx * (saturate(dot(normalize(lightPerp), g_lightDir)) * (cosPhiD + 1) * 0.5);

	//output.color.rgb *= 0.5f + diffuse;
	//output.color.rgb = ambient + diffuse;

	output.color.rgb = input.color * (ambient + diffuse);

	output.marcoord.x = (sinThetaI + 1.0f) * 0.5f;
	output.marcoord.y = (sinThetaO + 1.0f) * 0.5f;
	output.marcoord.z = (cosPhiD + 1.0f) * 0.5f;

	return output;
}

//---------------------------------------------------------

// Geometry shader for wide strand rendering

[maxvertexcount(4)]
void StrandWorldWidthGS(lineadj StrandVertexOut input[4], inout TriangleStream<StrandVertexOut> outStream)
{
	StrandVertexOut output;

	float lineWidth0 = input[1].distToCenter;
	float lineWidth1 = input[2].distToCenter;
	float aspect	 = g_viewportSize.x / g_viewportSize.y;

	float4 v0 = input[0].position / input[0].position.w;
	float4 v1 = input[1].position / input[1].position.w;
	float4 v2 = input[2].position / input[2].position.w;
	float4 v3 = input[3].position / input[3].position.w;

	float2 dir0 = (v2 - v0).xy;
	dir0.x *= aspect;

	float2 dir1 = (v3 - v1).xy;
	dir1.x *= aspect;

	float2 offset0 = lineWidth0 * normalize(float2(-dir0.y, dir0.x));
	offset0.y *= aspect;

	float2 offset1 = lineWidth1 * normalize(float2(-dir1.y, dir1.x));
	offset1.y *= aspect;

	output = input[1];
	output.position = v1 + float4(offset0, 0, 0);
	output.distToCenter = 0;
	outStream.Append(output); 

	output.position = v1 - float4(offset0, 0, 0);
	output.distToCenter = 1;
	outStream.Append(output); 

	output = input[2];
	output.position = v2 + float4(offset1, 0, 0);
	output.distToCenter = 0;
	outStream.Append(output); 

	output.position = v2 - float4(offset1, 0, 0);
	output.distToCenter = 1;
	outStream.Append(output); 

	outStream.RestartStrip();
}


[maxvertexcount(8)]
void StrandGS(lineadj StrandVertexOut input[4], inout TriangleStream<StrandVertexOut> outStream)
{
	StrandVertexOut output;

	float lineWidth0 = input[1].distToCenter;
	float lineWidth1 = input[2].distToCenter;

	float aspect = g_viewportSize.x / g_viewportSize.y;

	float4 v0 = input[0].position / input[0].position.w;
	float4 v1 = input[1].position / input[1].position.w;
	float4 v2 = input[2].position / input[2].position.w;

	float2 dir0 = (v1 - v0).xy;
	dir0.x *= (g_viewportSize.x / g_viewportSize.y);

	float2 dir1 = (v2 - v1).xy;
	dir1.x *= (g_viewportSize.x / g_viewportSize.y);

	float2 offset0 = normalize(float2(-dir0.y, dir0.x));
	offset0.x *= lineWidth0;// / g_viewportSize.x;
	offset0.y *= lineWidth0 * aspect;// / g_viewportSize.y;

	float2 offset1 = normalize(float2(-dir1.y, dir1.x));
	offset1.x *= lineWidth1;// / g_viewportSize.x;
	offset1.y *= lineWidth1 * aspect;// / g_viewportSize.y;

	output = input[1];

	if (input[1].texcoord.y > 0)
	{
		output.position = v1 + float4(offset0, 0, 0);
		output.distToCenter = 0;
		outStream.Append(output); 

		output.position = v1 + float4(offset1, 0, 0);
		output.distToCenter = 0;
		outStream.Append(output); 

		output.position = v1 - float4(offset0, 0, 0);
		output.distToCenter = 1;
		outStream.Append(output); 

		output.position = v1 - float4(offset1, 0, 0);
		output.distToCenter = 1;
		outStream.Append(output); 

		outStream.RestartStrip();
	}

	output.position = v1 + float4(offset1, 0, 0);
	output.distToCenter = 0;
	outStream.Append(output); 

	output.position = v1 - float4(offset1, 0, 0);
	output.distToCenter = 1;
	outStream.Append(output); 

	output = input[2];
	output.position = v2 + float4(offset1, 0, 0);
	output.distToCenter = 0;
	outStream.Append(output); 

	output.position = v2 - float4(offset1, 0, 0);
	output.distToCenter = 1;
	outStream.Append(output); 

	outStream.RestartStrip();
}


float4 StrandColorPS( StrandVertexOut input ) : SV_TARGET0
{
	//float tangentW = lerp(1, 0.5f * sin(input.texcoord.x * g_tangentTexScale * 3.1415) + 0.5f, g_tangentTexWeight);
	
	float wNormalGrad = saturate((1.0f - g_normalGradWeight) + g_texNormalGradient.Sample(NoiseSampler, float2(input.distToCenter, 0)));
	float wTangentGrad = saturate((1.0f - g_normalGradWeight) + input.texcoord.y);
	float wGrad = wNormalGrad * wTangentGrad;//lerp(1, wNormalGrad*wTangentGrad, g_normalGradWeight);

	//float2 noiseUV = float2(input.distToCenter*g_tangentTexScaleX, input.texcoord.x + input.texcoord.y*g_tangentTexScaleY);
	//float wNoise = 1.0f + g_tangentTexWeight * (g_texHairNoise.Sample(NoiseSampler, noiseUV) - 0.5f);

	float  wStrandRand = 1.0f + g_strandRandWeight*(input.texcoord.x - 0.5f);

	//float4 color = input.color * wNoise * wNormalGrad * wStrandRand;
	float4 color = input.color * wGrad * wStrandRand;

	// ignore g_forceOpaque...
	//color.a = lerp(color.a, 1.0f, g_forceOpaque);

	return color;
}

float4 StrandMarschnerPS( StrandVertexOut input ) : SV_TARGET0
{
	float4 tex1 = g_marschner1.Sample(MarschnerSampler, input.marcoord.xy);
	float4 tex2 = g_marschner2.Sample(MarschnerSampler, float2(input.marcoord.z, tex1.a));

	float3 specular = g_kr.xxx * tex1.r * tex2.a;


	float wNormalGrad = saturate((1.0f - g_normalGradWeight) + g_texNormalGradient.Sample(NoiseSampler, float2(input.distToCenter, 0)));
	float wTangentGrad = saturate((1.0f - g_normalGradWeight) + input.texcoord.y);
	float wGrad = wNormalGrad * wTangentGrad;//lerp(1, wNormalGrad*wTangentGrad, g_normalGradWeight);

	float  wStrandRand = 1.0f + g_strandRandWeight*(input.texcoord.x - 0.5f);

	float4 color = input.color * wGrad * wStrandRand;


	//float4 color = input.color;//(0.5f + g_strandRandWeight*(input.texcoord.x - 0.5f)).xxxx;//input.color;
	color.rgb += specular;
	color.a   = input.color.a;

	return color;
}

float4 StrandGlobalColorPS( StrandVertexOut input ) : SV_TARGET0
{
	float4 color = float4(g_hairColor.rgb, input.color.a);
	return g_hairColor;
}

float4 StrandTangentPS( StrandVertexOut input ) : SV_TARGET0
{
	return float4(input.tangent, 1);
}

float4 StrandAOPS( StrandVertexOut input ) : SV_TARGET0
{
	return float4(0.0.xxx, input.color.a);
}


//----------------------------------------------------------

HeadVertexOut HeadVS( HeadVertexIn input )
{
	HeadVertexOut output;
	
	float4 worldPos = mul(input.position, g_world);
	output.position = mul(worldPos, g_viewProjection);
	output.texcoord = input.texcoord;
	output.texcoord.y = 1.0f - output.texcoord.y;

	output.worldPos = worldPos.xyz / worldPos.w;

	output.ao = lerp(1.0f, input.reserved.x, g_bodyAO);

	return output;
}

//------

// VS for morphing head model
HeadVertexOut HeadMorphVS( HeadMorphVertexIn input )
{
	HeadVertexOut output;
	
	float3 interpPos = input.pos0*g_morphWeights[0];
	if (g_numSources > 1) interpPos += input.pos1*g_morphWeights[1];

#if MAX_NUM_MORPH_SRC > 2
	if (g_numSources > 2) interpPos += input.pos2*g_morphWeights[2];
#endif
#if MAX_NUM_MORPH_SRC > 3
	if (g_numSources > 3) interpPos += input.pos3*g_morphWeights[3];
#endif
#if MAX_NUM_MORPH_SRC > 4
	if (g_numSources > 4) interpPos += input.pos4*g_morphWeights[4];
#endif

	float4 worldPos = mul(float4(interpPos, 1), g_world);
	output.position = mul(worldPos, g_viewProjection);
	output.texcoord = input.texcoord;
	output.texcoord.y = 1.0f - output.texcoord.y;

	output.worldPos = worldPos.xyz / worldPos.w;

	output.ao = 1;//lerp(1.0f, input.reserved.x, g_bodyAO);

	return output;
}


float4 HeadMorphPS( HeadVertexOut input ) : SV_TARGET0
{
	float4 color = g_morphWeights[0]*g_headSrcTex0.Sample(LinearSampler, input.texcoord);
	color += g_morphWeights[1]*g_headSrcTex1.Sample(LinearSampler, input.texcoord);
#if MAX_NUM_MORPH_SRC > 2
	if (g_numSources > 2)
		color += g_morphWeights[2]*g_headSrcTex2.Sample(LinearSampler, input.texcoord);
#endif
#if MAX_NUM_MORPH_SRC > 3
	if (g_numSources > 3)
		color += g_morphWeights[3]*g_headSrcTex3.Sample(LinearSampler, input.texcoord);
#endif
#if MAX_NUM_MORPH_SRC > 4
	if (g_numSources > 4)
		color += g_morphWeights[4]*g_headSrcTex4.Sample(LinearSampler, input.texcoord);
#endif

	return color;
}
//--------


float4 HeadSimplePS(HeadVertexOut input) : SV_TARGET0
{
	float4 color = float4(0.75, 0.75, 0.75, 1);

	//color.rgb *= input.ao;

	return float4(1,0,0,1);//color;
}

float4 HeadTexcoordPS(HeadVertexOut input) : SV_TARGET0
{
	return float4(input.texcoord, 0, 1);
}

//float4 HeadTexturedPS(HeadVertexOut input) : SV_TARGET0
void HeadTexturedPS(HeadVertexOut input,
	out float4 color: SV_TARGET0,
	out float4 ao: SV_TARGET1)
{
	color = g_headTexture.Sample(NoiseSampler, input.texcoord);

	color.rgb *= input.ao;

	//return color;
}


float4 NeckTexturedPS(HeadVertexOut input) : SV_TARGET0
{
	float dZ = g_bodyDepth - input.worldPos.z;

	if (dZ < 0) discard;

	float4 color = g_headTexture.Sample(NoiseSampler, input.texcoord);

	color.rgb *= input.ao;

	color.a = saturate(dZ * g_neckBlend);

	return color;
}


float4 HeadDepthOnlyPS(HeadVertexOut input) : SV_TARGET0
{
	return 0.0.xxxx;
}


//----------------------------------------------------------------
// Morphing related
//----------------------------------------------------------------


StrandVertexOut StrandMorphSimpleAlphaVS( StrandVertexIn input )
{
	StrandVertexOut output;
	
	float4 worldPos = mul(input.position, g_world);
	float4 viewPos  = mul(worldPos, g_view);
	output.position = mul(viewPos, g_projection);                                                                                           

	float d = viewPos.z * g_projection._34 + g_projection._44;
	
	output.tangent		= input.tangent;
	output.color		= input.color;
	output.texcoord		= input.texcoord;
	output.distToCenter	= 0.5f * g_hairWidth * g_projection._11 / d;
	output.marcoord		= 0.0.xxx;

	return output;
}


StrandVertexOut StrandMorphVS( StrandMorphVertexIn input )
{
	StrandVertexOut output;
	
	float3 interPos = input.pos0*g_morphWeights[0];
	float4 interClr = input.color0*g_morphWeights[0];

	if (g_numSources > 1)
	{
		interPos += input.pos1*g_morphWeights[1];
		interClr += input.color1*g_morphWeights[1];
	}

#if MAX_NUM_MORPH_SRC > 2
	if (g_numSources > 2)
	{
		interPos += input.pos2*g_morphWeights[2];
		interClr += input.color2*g_morphWeights[2];
	}
#endif
#if MAX_NUM_MORPH_SRC > 3
	if (g_numSources > 3)
	{
		interPos += input.pos3*g_morphWeights[3];
		interClr += input.color3*g_morphWeights[3];
	}
#endif
#if MAX_NUM_MORPH_SRC > 4
	if (g_numSources > 4)
	{
		interPos += input.pos4*g_morphWeights[4];
		interClr += input.color4*g_morphWeights[4];
	}
#endif

	float4 worldPos = mul(float4(interPos, 1), g_world);
	float4 viewPos  = mul(worldPos, g_view);
	output.position = mul(viewPos, g_projection);                                                                                           

	float d = viewPos.z * g_projection._34 + g_projection._44;
	
	output.tangent		= 0;
	output.color		= interClr;
	output.texcoord		= input.texcoord;
	output.distToCenter	= 0.5f * g_hairWidth * g_projection._11 / d;
	output.marcoord		= 0.0.xxx;

	return output;
}


float4 StrandMorphSimpleAlphaColorPS( StrandVertexOut input ) : SV_TARGET0
{
	float  wStrandRand = 1.0f + g_strandRandWeight*(input.texcoord.x - 0.5f);

	float4 color = input.color;
	color.rgb *= wStrandRand;
	color.a   *= g_hairOpacity;

	return color;
}


float4 StrandMorphColorPS( StrandVertexOut input ) : SV_TARGET0
{
	float  wStrandRand = 1.0f + g_strandRandWeight*(input.texcoord.x - 0.5f);

	float4 color = input.color;
	color.rgb *= wStrandRand;

	return color;
}


///////////////////////////////////////////////////////////
// States
///////////////////////////////////////////////////////////

RasterizerState	rsDefault
{
	CullMode	= NONE;
};

DepthStencilState dsEnableDepth
{
	DepthEnable	= TRUE;
};

DepthStencilState dsHairDepthStencil
{
	DepthEnable = TRUE;

	StencilEnable = TRUE;
	FrontFaceStencilFunc = ALWAYS;
	FrontFaceStencilDepthFail = KEEP;
	FrontFaceStencilPass = REPLACE;
	BackFaceStencilFunc = ALWAYS;
	BackFaceStencilDepthFail = KEEP;
	BackFaceStencilPass = REPLACE;
};

DepthStencilState dsOrigLayerOverride
{
	DepthEnable = FALSE;
	
	StencilEnable = TRUE;
	
	FrontFaceStencilFunc = EQUAL;
	FrontFaceStencilPass = KEEP;
	FrontFaceStencilFail = KEEP;
	FrontFaceStencilDepthFail = KEEP;

	BackFaceStencilFunc = NOT_EQUAL;
	BackFaceStencilPass = KEEP;
	BackFaceStencilFail = KEEP;
	BackFaceStencilDepthFail = KEEP;
};

BlendState bsAlphaToCoverage
{
	AlphaToCoverageEnable = TRUE;
	RenderTargetWriteMask[0] = 0x0F;

	BlendEnable[0]		= TRUE;
	SrcBlend[0]			= SRC_ALPHA;
	DestBlend[0]		= INV_SRC_ALPHA;
	SrcBlendAlpha[0]	= ONE;
	DestBlendAlpha[0]	= ONE;
};

BlendState bsEnableAlpha
{
	BlendEnable[0]		= TRUE;
	SrcBlend[0]			= SRC_ALPHA;
	DestBlend[0]		= INV_SRC_ALPHA;
	SrcBlendAlpha[0]	= ONE;
	DestBlendAlpha[0]	= ONE;
};

BlendState bsDisableAlpha
{
	BlendEnable[0]		= FALSE;
};

///////////////////////////////////////////////////////////
// Techniques
///////////////////////////////////////////////////////////

technique11 ImageDefault
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, ImageVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ImagePS()));
	}
}

technique11 ImageLayer
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, ImageVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ImageLayerPS()));
	}
}


technique11 BodyLayer
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, ImageVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, BodyLayerPS()));
	}
}


// Used to blend in original target image during hair transfer.
technique11 OrigImageLayer
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsOrigLayerOverride, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, ImageVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ImageLayerPS()));
	}
}


//---------------------------------------------------------

technique11 StrandColor
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsHairDepthStencil, 1);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandColorPS()));
	}
}

//---------------------------------------------------------

technique11 StrandGlobalColor
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandGlobalColorPS()));
	}
}

//---------------------------------------------------------

technique11 StrandTangent
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandTangentPS()));
	}
}

//---------------------------------------------------------

technique11 StrandLighting
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsHairDepthStencil, 1);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandShadingVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandColorPS()));
	}
}

technique11 StrandMarschner
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsHairDepthStencil, 1);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandShadingVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandMarschnerPS()));
	}
}


// For precomputed ambient occlusion...
technique11 StrandAO
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsHairDepthStencil, 1);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, StrandAOPS()));
	}
}



//---------------------------------------------------------

technique11 HeadSimple
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsDisableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, HeadSimplePS()));
	}
}

technique11 HeadTexcoord
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsDisableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, HeadTexcoordPS()));
	}
}

technique11 HeadTextured
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsDisableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, HeadTexturedPS()));
	}
}

technique11 NeckTextured
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, NeckTexturedPS()));
	}
}

technique11 HeadDepthOnly
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsEnableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, HeadDepthOnlyPS()));
	}
}

//---------------------------------------------------------

technique11 HeadMorphSimple
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsEnableDepth, 0);
		SetBlendState(bsDisableAlpha, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, HeadMorphVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, HeadMorphPS()));
	}
}
//---------------------------------------------------------


technique11 StrandMorphColor
{
	pass P0
	{
		SetRasterizerState(rsDefault);
		SetDepthStencilState(dsHairDepthStencil, 1);
		SetBlendState(bsAlphaToCoverage, float4(0,0,0,0), 0xffffffff);

		SetVertexShader(CompileShader(vs_4_0, StrandMorphVS()));
		SetGeometryShader(CompileShader(gs_4_0, StrandWorldWidthGS()));
		SetPixelShader(CompileShader(ps_4_0, StrandMorphColorPS()));
	}
}

