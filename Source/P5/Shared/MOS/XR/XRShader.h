
#ifndef _INC_XRSHADER
#define _INC_XRSHADER

#include "XR.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Encapsulation of hardware-dependant shading for unified lighting.
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB 2001, 2004
					
	Contents:		class CXR_Shader

	Comments:

		Global Requirements:
			-Cube map texture
			-Vertex program


		XR_SHADERMODE_TEXENVCOMBINE:

			-Requires: Two texture units.
			-Requires: ARB_TEXTURE_ENV_COMBINE
			-Requires: ARB_TEXTURE_ENV_DOT3

			-Pass 1:
					Texture0 = Dst2Attenuation
					Texture1 = Dst2Attenuation
					Dest.a = Texture0*Texture1;
			[
			-Pass 2:
					Texture0 = ProjectionMap
					Dest.a = (1 - Dest.alpha) * Texture0
			]
			-Pass 3:
					Texture0 = NormalMap
					Texture1 = Lightvector normalization map
					Dest.a = Dest.a) * (Texture0 dot Texture1), or
					Dest.a = (1 - Dest.a) * (Texture0 dot Texture1)
			-Pass 4:
					Texture0 = DiffuseMap
					Dest.rgb = Dest.a * Texture0 * PrimaryColor


		XR_SHADERMODE_REGCOMBINERS_NV20:

			-Requires: NV_TEXTURE_SHADER+NV_REGISTER_COMBINERS (CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)

			-Pass 1:
				Texture0 = ExpAttenuation 2D texture
				Texture1 = ExpAttenuation 2D texture
				Texture2 = [ProjectionMap]

				Dest.a = Attenuation [* ProjectionMap]

			-Pass 2:
				Texture0 = Diffusemap
				Texture1 = Normalmap
				Texture2 = Lightvector normalization map

				Dest.rgb += Dest.a * DiffuseTexture * (NormalMap dot LightVector)
				

			-Pass 3:
				Texture0 = Normal map
				Texture1 = Normal map
  				Texture2 = Normal map
  				Texture3 = (N*H)^k cube map

				Dest.rgb += Dest.a * Normalmap.a * (NormalMap dot HalfAngle)^k


		XR_SHADERMODE_FRAGMENTPROGRAM14:

			-Requires: ATI_FRAGMENT_SHADER (CRC_CAPS_FLAGS_FRAGMENTPROGRAM14)

			-Pass 1:
				Texture0 = Normal map
				Texture1 = Diffuse map
				Texture2 = Normalize map
				Texture3 = (N*H)^k map
				Texture4 = Projection map

				Dest.rgb = Stuff...


		XR_SHADERMODE_FRAGMENTPROGRAM20:

			-Requires: ARB_FRAGMENT_PROGRAM (CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)

			-Pass 1:

				Dest.rgb = Stuff...




	History:		
		010301:		Created File

\*____________________________________________________________________________________________*/

enum
{
	XR_SHADER_MAXVB								= 64,		// Max for CXR_ShaderParams::m_nVB

	// Enums returned by GetShaderMode(), Models should not be required to use this.
	XR_SHADERMODE_AUTO							= 0,
	XR_SHADERMODE_TEXENVCOMBINE					= 1,
	XR_SHADERMODE_REGCOMBINERS_NV20				= 2,
	XR_SHADERMODE_FRAGMENTPROGRAM11				= 3,		// Disabled
	XR_SHADERMODE_FRAGMENTPROGRAM14				= 4,
	XR_SHADERMODE_FRAGMENTPROGRAM20				= 5,
	XR_SHADERMODE_FRAGMENTPROGRAM20SS			= 6,
	XR_SHADERMODE_FRAGMENTPROGRAM20DEF			= 7,
	XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT		= 8,
	XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM		= 9,
	XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN		= 10,

	XR_SHADERMODETRAIT_DEFERRED					= 1,
	XR_SHADERMODETRAIT_LINEARCOLORSPACE			= 2,
	XR_SHADERMODETRAIT_MRT						= 4,
	XR_SHADERMODETRAIT_MOTIONMAP				= 8,

	// Flags returned by GetShaderFlags()
	XR_SHADERFLAGS_DESTALPHA					= 1,		// Destinaion alpha is used for accumulating intermediate shading results.
	XR_SHADERFLAGS_SPECDESTALPHA				= 2,		// Destination alpha contains the specular intensity.
	XR_SHADERFLAGS_NEEDTANGENTSPACE				= 4,		// Tangent space vectors is required.
	XR_SHADERFLAGS_NEEDNORMALS					= 8,		// Normals is required.
	XR_SHADERFLAGS_NEEDNORMALMAPUV				= 16,		// Normal map UV is required.
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ShaderParams
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	XR_SHADERMAP_DIFFUSE = 0,
	XR_SHADERMAP_SPECULAR2,
	XR_SHADERMAP_SPECULAR,
	XR_SHADERMAP_NORMAL,
	XR_SHADERMAP_HEIGHT,
	XR_SHADERMAP_TRANSMISSION,
	XR_SHADERMAP_ENVIRONMENT,
	XR_SHADERMAP_ATTRIBUTE,
	XR_SHADERMAP_ANISOTROPICDIR,

	XR_SHADERMAP_MAXMAPS,

	XR_SHADERFLAGS_NOSTENCILTEST = 1,
	XR_SHADERFLAGS_USEZEQUAL = 2,
	XR_SHADERFLAGS_USEZLESS = 4,
	XR_SHADERFLAGS_SPECULARINDIFFUSE = 8,
	XR_SHADERFLAGS_TRANSPARENT = 16,
	XR_SHADERFLAGS_GLASS = 32,
	XR_SHADERFLAGS_CULLCW = 128,
	XR_SHADERFLAGS_SPECIALBASETEXTURE = 256,
};

class CXR_SurfaceShaderParams					// 80 bytes  =(
{
public:
	union
	{
		struct
		{
			CVec4Dfp2Aggr m_DiffuseColor;
			CVec4Dfp2Aggr m_SpecularColor;
			CVec4Dfp2Aggr m_AttribScale;
			CVec4Dfp2Aggr m_EnvColor;
			CVec4Dfp2Aggr m_TransmissionColor;
		};
		struct
		{
			uint64 m_u64_DiffuseColor;
			uint64 m_u64_SpecularColor;
			uint64 m_u64_AttribScale;
			uint64 m_u64_EnvColor;
			uint64 m_u64_TransmissionColor;
		};
	};

	fp32 m_SpecularAnisotrophy;
	uint16 m_lTextureIDs[XR_SHADERMAP_MAXMAPS];	// 9*2 = 18
	uint8 m_SpecularFresnelBias;
	uint8 m_SpecularFresnelScale;

	uint8 m_Flags;
	uint8 m_AlphaFunc;
	uint8 m_AlphaRef;
	uint8 m_iBRDFModel : 5;
	uint8 m_iController : 1;					// Which timer & sequence controller to use.
	uint8 m_iGroup : 2;							// Which surface group it belongs to.

	uint8 m_lDeferredRasterMode[3];				// Normal, Diff, Spec

	fp32 m_PolyOffsetScale;
	fp32 m_PolyOffset;

	void Clear()
	{
		m_DiffuseColor.SetOne();
		m_SpecularColor.SetOne();
		m_AttribScale.SetZero();
		m_EnvColor.SetOne();
		m_TransmissionColor.SetOne();
		m_SpecularAnisotrophy = 0;
		FillChar(m_lTextureIDs, sizeof(m_lTextureIDs), 0);
		m_SpecularFresnelBias = 0;
		m_SpecularFresnelScale = 0;
		m_Flags = 0;
		m_AlphaFunc = CRC_COMPARE_ALWAYS;
		m_AlphaRef = 0;
		m_iBRDFModel = 0;
		m_lDeferredRasterMode[0] = CRC_RASTERMODE_NONE;
		m_lDeferredRasterMode[1] = CRC_RASTERMODE_NONE;
		m_lDeferredRasterMode[2] = CRC_RASTERMODE_NONE;
	}

	void Create(const CXW_Surface* _pSurf, const CXW_SurfaceKeyFrame* _pSurfKey);
};

class CXR_ShaderParams
{
public:
//	const CXR_SurfaceShaderParams* m_pSurfParams;

/*	uint16 m_lTextureIDs[XR_SHADERMAP_MAXMAPS];
	CVec4Dfp2 m_DiffuseColor;						// <-- Rep with CVec4Dfp2
	CVec4Dfp2 m_SpecularColor;
	CVec4Dfp2 m_AttribScale;
	CVec4Dfp2 m_EnvColor;
	CVec4Dfp2 m_TransmissionColor;
	fp32 m_SpecularAnisotrophy;
	uint8 m_SpecularFresnelBias;
	uint8 m_SpecularFresnelScale;
	uint8 m_Flags;
	uint8 m_iBRDFModel;*/

	uint8 m_Flags;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	fp32 m_Priority;
	uint16 m_nVB;									// Max XR_SHADER_MAXVB

	const CMat4Dfp32* m_pCurrentWMat;
	const CMat4Dfp32* m_pCurrentVMat;
	const CMat4Dfp32* m_pCurrentWVelMat;

	void Clear()
	{
/*		FillChar(m_lTextureIDs, sizeof(m_lTextureIDs), 0);
		m_DiffuseColor.SetOne();
		m_SpecularColor.SetOne();
		m_AttribScale.SetZero();
		m_EnvColor.SetOne();
		m_TransmissionColor.SetOne();
		m_SpecularAnisotrophy = 0;
		m_SpecularFresnelBias = 0;
		m_SpecularFresnelScale = 0;*/
		m_Flags = 0;
//		m_iBRDFModel = 0;
		m_iTexCoordSetMapping = 0;
		m_iTexCoordSetTangentU = 1;
		m_iTexCoordSetTangentV = 2;
		m_pCurrentWMat = NULL;
		m_pCurrentVMat = NULL;
		m_pCurrentWVelMat = NULL;
		m_Priority = 0;
		m_nVB = 1;
	}

//	void Create(const CXW_SurfaceKeyFrame& _Surf, const CMat4Dfp32* _pCurrentWMat, const CMat4Dfp32* _pCurrentVMat, CXR_Shader* _pShader);

	M_FORCEINLINE void Create(const CMat4Dfp32* _pCurrentWMat, const CMat4Dfp32* _pCurrentVMat, CXR_Shader* _pShader)
	{
		m_nVB = 1;
//		m_pSurfParams = _pSurfParams;
//		m_Flags = _pSurfParams->m_Flags;
		m_Flags = 0;
		m_iTexCoordSetMapping = 0;
		m_iTexCoordSetTangentU = 1;
		m_iTexCoordSetTangentV = 2;
		m_pCurrentWMat = _pCurrentWMat;
		m_pCurrentVMat = _pCurrentVMat;
		m_pCurrentWVelMat = NULL;
		m_Priority = 0;
	}
};

/*
class CXR_ShaderParamsDef
{

public:
	CXR_SurfaceShaderParams* m_pSurfParams;

	uint16	m_lTextureIDs[3];
	uint8	m_lRasterMode[3];

	fp32		m_PolyOffsetScale;
	fp32		m_PolyOffset;

	void Create(const CXW_SurfaceKeyFrame& _Surf);

};*/

class CXR_ShaderParams_LightField : public CXR_ShaderParams
{
public:
	CVec4Dfp32 m_lLFAxes[6];

};


class CXR_ShaderParams_LightFieldMapping
{
public:
	uint16 m_lLFMTextureID[4];
	uint8 m_iTexCoordSetLFM;
	uint8 m_iTexCoordSetLMScale;

	uint8 m_Flags;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	uint16 m_nVB;

	const CMat4Dfp32* m_pCurrentWMat;
	const CMat4Dfp32* m_pCurrentVMat;

	M_FORCEINLINE void CreateLFM(const CMat4Dfp32* _pCurrentWMat, const CMat4Dfp32* _pCurrentVMat, const uint16* _pLFMTextureID, int _iTxtCoordSet, int _iTxtCoordSetLMSCale)
	{
		int TxtID0 = _pLFMTextureID[0];
		int TxtID1 = _pLFMTextureID[1];
		int TxtID2 = _pLFMTextureID[2];
		int TxtID3 = _pLFMTextureID[3];
		m_lLFMTextureID[0] = TxtID0;
		m_lLFMTextureID[1] = TxtID1;
		m_lLFMTextureID[2] = TxtID2;
		m_lLFMTextureID[3] = TxtID3;
		m_iTexCoordSetLFM = _iTxtCoordSet;
		m_iTexCoordSetLMScale = _iTxtCoordSetLMSCale;

		m_Flags = 0;
		m_iTexCoordSetMapping = 0;
		m_iTexCoordSetTangentU = 1;
		m_iTexCoordSetTangentV = 2;
		m_pCurrentWMat = _pCurrentWMat;
		m_pCurrentVMat = _pCurrentVMat;
		m_nVB = 1;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Shader
|__________________________________________________________________________________________________
\*************************************************************************************************/

struct CXR_Shader_ProgramName
{
	const char* m_pName;
	uint32 m_Hash;
};


class CXR_Shader : public CConsoleClient
{
	MRTC_DECLARE;

public:
	CXR_Engine* m_pEngine;
	CXR_VBManager* m_pVBM;
	CSystem* m_pSys;

	int m_TextureID_Normalize;
	int m_TextureID_FlipNormalize;
	int m_TextureID_AttenuationExp;
	int m_TextureID_AttenuationDst2;
	int m_TextureID_AttenuationDst2Alpha;
	int m_TextureID_DefaultLens;
	int m_TextureID_DefaultNormal;
	int m_lTextureID_Specular[8];
	int m_TextureID_SpecularPower;
	int m_TextureID_Special_Cube_ffffffff;
	int m_TextureID_Special_Cube_ff000000;
	int m_TextureID_Special_ffffffff;
	int m_TextureID_Special_ff000000;
	int m_TextureID_Rand_02;
	int m_TextureID_Rand_03;
	int m_TextureID_Rand_04;
	int m_TextureID_DefaultEnvMap;

	int m_TextureID_SbzProjTemp;

	static int ms_TextureID_BackBuffer;

#ifdef SUPPORT_FRAGMENTPROGRAM
	static CXR_Shader_ProgramName ms_lProgramsShading20[];
#endif

	uint m_ShaderMode;
	uint m_PreferredShaderMode;
	uint m_LastPreferredShaderMode;
	uint m_LastShaderMode;
	uint m_LastModesAvail;
	uint m_ShaderFlags;
	uint m_SpecularMode;
	uint m_SpecularForcePower;		// 0 == No forced specular power.
	uint m_bUseVirtualAttr;
	uint m_ShaderModeTraits;
	uint m_iDeferredShaderOffset;
	uint m_bIsRegistered : 1;
	uint m_bCanGetBackbuffer : 1;
	uint m_bDisplayNextModeSelection : 1;

	uint32 m_Caps_Flags;
	uint32 m_Caps_nMultiTexture : 8;
	uint32 m_Caps_nMultiTextureCoords : 8;

public:
	CVec4Dfp32 m_DiffuseScale;
	CVec4Dfp32 m_SpecularScale;
	CVec4Dfp32 m_CurrentSpecularScale;
	fp32 m_FallOffExp_Linear;
	fp32 m_FallOffExp_Gamma;
	fp32 m_FallOffExp_Current;

	static int CreateProjMapTexGenAttr(fp32* _pTexGenAttr, const CXR_LightPosition& _LightPos, const CXR_Light& _Light);

protected:
#ifdef SUPPORT_FRAGMENTPROGRAM
	void RenderShading_FP20_COREFBB(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
	void RenderShading_FP20(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
	void RenderShading_FP20_Glass(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
//	void RenderShading_FP20SS(const CXR_Light& _Light, CXR_VertexBuffer* _pVB, const CXR_ShaderParams* _pShaderParams);
	void RenderShading_FP20_LF(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightField* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
	void RenderShading_FP20_LFM(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightFieldMapping* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);

	void RenderShading_FP20Def_COREFBB(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams);
	void RenderShading_FP20Def(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
	void RenderShading_FP20Def_LF(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightField* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);
	void RenderShading_FP20Def_LFM(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightFieldMapping* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams);
	
	void RenderDeferred_VB(CXR_VertexBuffer * _pVB,CXR_VertexBuffer *_pSrc,fp32 _BasePrio,fp32 _OffsPrio);
#endif
	void Debug_RenderTexture(CXR_VertexBufferGeometry* _pVB, int _TextureID);

	void AssignTextures(CTextureContext *_pTC);
public:
	CXR_Shader();
	virtual ~CXR_Shader();
	virtual void Create(CXR_Engine* _pEngine);
	virtual void Create(CTextureContext *_pTC, CXR_VBManager* _pVBM);
#ifndef M_RTM
	virtual void SetVBM(CXR_VBManager* _pVBM) { m_pVBM = _pVBM; }
#endif

protected:
	void Options_UpdateAvailShaderModes(int _ModesAvail);
	void Options_UpdateCurrentShaderModes(int _ShaderMode);

public:
	bool PrepareFrame(CRenderContext *_pRC, CXR_VBManager* _pVBM);
	virtual bool OnPrepareFrame(CXR_VBManager *_pVBM = NULL);
	virtual bool OnEndFrame();
	virtual void OnPrecache();

	virtual int GetShaderMode();
	virtual int GetShaderFlags();
	virtual int GetShaderModeTraits();

	virtual void CalcTangentSpaceLightVector(const CVec3Dfp32& _LightPos, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pTangU, const CVec3Dfp32* _pTangV, CVec3Dfp32* _pLV);
	virtual bool CalcTangentSpaceLightVector(const CVec3Dfp32& _LightPos, CXR_VertexBuffer* _pVB);

	virtual void RenderShading(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams);
	virtual void RenderShading_LightField(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightField* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams);
	virtual void RenderShading_LightFieldMapping(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightFieldMapping* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams);

	virtual void RenderDeferredArray(uint _nVB, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpShaderParams,int _Enable,int _Disable,fp32 _BasePrio,fp32 _OffsPrio,uint16 _Src = 0,CVec4Dfp32 *_pParams = NULL);
	virtual void RenderDeferredMotionVectorArray(uint _nVB, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpShaderParams,int _Enable,int _Disable,fp32 _BasePrio,fp32 _OffsPrio,uint16 _Src = 0,CVec4Dfp32 *_pParams = NULL);
	virtual void RenderMotionVectors(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, uint _Enable, uint _Disable, uint _RasterMode, uint _ZCompare);

	virtual void Con_ShaderMode(int _v);
	virtual void Con_SpecularMode(int _v);
	virtual void Con_SpecularForcePower(int _v);
	virtual void Con_ShaderVirtualAttr(int _v);
	virtual void Con_ShaderFallOffExpLinear(fp32 _v);
	virtual void Con_ShaderFallOffExpGamma(fp32 _v);

	virtual void Register();
	void Register(CScriptRegisterContext &_RegContext);
};


#endif // _INC_XRSHADER
