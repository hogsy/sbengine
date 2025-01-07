
/*------------------------------------------------------------------------------------------------
NAME:		XRFog.cpp/h
PURPOSE:	Extended Reality Engine
CREATION:	981003
AUTHOR:		Magnus Högdahl
COPYRIGHT:	(c) Copyright 1996 Magnus Högdahl

CONTENTS:
-

MAINTAINANCE LOG:

981009:		Moved from XREngine.* to create less mess when adding stuff to the fogstate.

------------------------------------------------------------------------------------------------*/
#ifndef _INC_XRFog
#define _INC_XRFog

#include "XRClass.h"

class CXR_VertexBuffer;
class CXR_VBManager;
class CXR_ViewContext;

// -------------------------------------------------------------------
//  CXR_FogTracer
// -------------------------------------------------------------------
class CXR_FogInterface
{
public:
//	virtual bool Fog_NeedFog(const CBox3Dfp32& _Box, int _iNode = 0) pure;
	virtual int Fog_InitTraceBound(const CBox3Dfp32& _Box) pure;			// Returns a hAccelerator, 0 => No fog.
	virtual void Fog_Trace(int _hAccelerator, const CVec3Dfp32& _POV, const CVec3Dfp32* _pV, int _nV, CPixel32* _pFog) pure;
	virtual void Fog_ReleaseTraceBound(int _hAccelerator) pure;
};

class CXR_FogState;

// -------------------------------------------------------------------
//  CXR_FogSphere
// -------------------------------------------------------------------
class CXR_FogSphere
{
public:
	CVec3Dfp32 m_Pos;
	fp32 m_Radius;
	fp32 m_RSqr;
	CPixel32 m_Color;
	fp32 m_Thickness;
};

// -------------------------------------------------------------------
//  CXR_FogTextureContainer
// -------------------------------------------------------------------
/*
class CXR_FogTextureContainer : public CTextureContainer
{
	CImage m_RefTxt[4];


public:
	CXR_FogTextureContainer(CXR_FogState* _pFogState);
	~CXR_FogTextureContainer();

	// Overrides
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType);

	void SetMap(int _w, int _h);
	void GetMap(CImage);
};

typedef TPtr<CXR_FogTextureContainer> spCXR_FogTextureContainer;
*/

// -------------------------------------------------------------------
//  CXR_FogState
// -------------------------------------------------------------------
class CXR_FogState : public CReferenceCount
{
public:
	CXR_Engine* m_pEngine;

	bool m_bAllowDepthFog;
	bool m_bAllowVertexFog;
	bool m_bAllowNHF;

	TArray<uint8> m_FogBuffer;
	int m_BufferSize;
	fp32* m_pSqrtBuffer;
	fp32* m_pAddBuffer;

//	spCXR_FogTextureContainer m_spFogTxt;

	CXR_FogInterface* m_pFogModel;
	int m_hFogModelAccelerator;
	int m_FogTableWidth;
	fp32 m_FogTableWidthInv;
	int m_FogTableTextureID;
	int m_DepthFogTableTextureID;
	int m_LinearFogTableTextureID;
	int m_Special000000TextureID;
	TArray<int> m_lFogTable;

	TArray<CXR_FogInterface*> m_lpFogModels;
//	TArray<int> m_lpFogModels;
	int m_nFogModels;

	TArray<CXR_FogSphere> m_lVolumes;
	int m_nVolumes;

	CPlane3Dfp32 m_WFrontPlane;

	int m_bClipFront;
	int m_bTransform;
	int m_bTranslate;
	CMat4Dfp32 m_Transform;
	CMat4Dfp32 m_Eye;
	fp32 m_EyeFrontPlaneDist;

	// Depthfog
	bool m_DepthFogEnable;
	fp32 m_DepthFogStart;
	fp32 m_DepthFogEnd;
	fp32 m_DepthFogIntervalK;
	CPixel32 m_DepthFogColor;
	fp32 m_DepthFogDensity;

	// Vertexfog
	bool m_VtxFog_Enable;
	CPixel32 m_VtxFog_Color;
	fp32 m_VtxFog_End;
	fp32 m_VtxFog_HeightAttn;
	fp32 m_VtxFog_RelHeight;
	fp32 m_VtxFog_ReferenceHeight;
	CPlane3Dfp32 m_VtxFog_EndPlane;

	int m_VtxFog_VertexCount;

	CRC_Attributes* m_VtxFog_pAttribOpaque;
	CRC_Attributes* m_VtxFog_pAttribTransparent;


	CXR_FogState();
	virtual void Create(CXR_Engine* _pEngine);
	virtual void OnPrecache();
	virtual void InitFogBuffer(int _nV);

	M_FORCEINLINE bool DepthFogEnable() { return m_DepthFogEnable && m_bAllowDepthFog; };
	M_FORCEINLINE bool VertexFogEnable() { return m_VtxFog_Enable && m_bAllowVertexFog; };
	M_FORCEINLINE bool NHFEnable() { return m_bAllowNHF; };

	virtual bool NeedFog_Sphere(const CVec3Dfp32& _Pos, fp32 _Raidius);
	virtual bool NeedFog_Box(const CBox3Dfp32& _Box, int _iNode = -1);

	virtual void PrepareFrame(CXR_ViewContext* _pView, const CPlane3Dfp32& _WFrontPlane);
	virtual void ClearVBMDependencies();

	virtual void AddSphere(const CVec3Dfp32& _Pos, fp32 _Radius = 256.0f, CPixel32 _Color = 0x3f7f7f7f, fp32 _Thickness = 0.5f);
	virtual void AddModel(CXR_FogInterface* _pFogModel, const CVec3Dfp32& _Pos);
	virtual void SetTransform(const CMat4Dfp32* _pMat);
	virtual void SetEye(const CMat4Dfp32& _Pos);

	virtual void TraceBound(const CBox3Dfp32& _Box);
	virtual void TraceBoundRelease();
	virtual CPixel32 Trace(const CVec3Dfp32& _v);

	virtual bool TraceBox(const CBox3Dfp32& _BoundBox, CPixel32* _pFog);
	virtual bool InterpolateBox(const CBox3Dfp32& _BoundBox, const CPixel32* _pBoxFog, int _nV, const CVec3Dfp32* _pV, CPixel32* _pFog, CVec2Dfp32* _pFogUV, const CMat4Dfp32* _pTransform = NULL);

	virtual bool Trace(const CBox3Dfp32& _BoundBox, int _nV, const CVec3Dfp32* _pV, CPixel32* _pFog, CVec2Dfp32* _pFogUV, bool _bFast = false);

	virtual CPixel32 TranslateFogTable(CPixel32 _Fog);

	virtual void DepthFog_Init(fp32 _Start, fp32 _End, CPixel32 _Color, fp32 _Density = 1.0f);

	virtual void SetDepthFog(CRenderContext* _pRC, int _iPass = 0, int _RasterMode = CRC_RASTERMODE_NONE, fp32 _DepthScale = 1.0f);
	virtual void SetDepthFogBlack(CRenderContext* _pRC, fp32 _DepthScale = 1.0f);

	virtual void SetDepthFog(CRC_Attributes* _pAttr, int _iPass = 0, fp32 _DepthScale = 1.0f);
	virtual void SetDepthFogBlack(CRC_Attributes* _pAttr, fp32 _DepthScale = 1.0f);
	virtual void SetDepthFogNone(CRC_Attributes* _pAttr, fp32 _DepthScale = 1.0f);

	// Vertex-fog
protected:
	virtual CRC_Attributes* VertexFog_GetAttrib(CXR_VBManager* _pVBM, bool _Transparent);
public:
	virtual void VertexFog_Init(const CMat4Dfp32& _POV, fp32 _EndDistance, fp32 _HeightAttenuation, CPixel32 _Color, fp32 _ReferenceHeight);
	virtual bool VertexFog_Eval(int _nV, const CVec3Dfp32* _pV, const CPixel32* _pSrcCol, CPixel32* _pDstCol, int _Oper, bool _Transparent = false);	// N/A
	virtual bool VertexFog_Eval(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, bool _Transparent = false);
	virtual fp32* VertexFog_EvalCoord(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB);
	virtual void VertexFog_SetFogCoord(CRC_Attributes* _pAttr);

	virtual int ConvertFogColors(int _nV, CPixel32* _pFog, CVec2Dfp32* _pTV);

	virtual void RenderPolygon(const CVec3Dfp32* _pV, const uint32* _piV = NULL);
};

// -------------------------------------------------------------------
//  CXR_Model_FogVolume
// -------------------------------------------------------------------
class CXR_Model_FogVolume : public CXR_Model
{
	MRTC_DECLARE;

public:
	CXR_Model_FogVolume();

	// Bounding volumes in model-space
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	// Render
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);

	MACRO_OPERATOR_TPTR(CXR_Model_FogVolume);
};

#endif // _INC_XRFog
