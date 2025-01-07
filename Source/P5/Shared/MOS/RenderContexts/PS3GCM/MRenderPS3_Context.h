#ifndef _INC_MRndrGL
#define _INC_MRndrGL

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Render context implementation for GCM/RSX

	Author:			Jim Kjellin
					Magnus Högdahl

	Copyright:		Copyright 2006 Starbreeze Studios AB
					
	History:		
		2006-07-17:		Created File
\*____________________________________________________________________________________________*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Includes
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include "MRenderPS3_Image.h"
#include "MRenderPS3_Def.h"
#include "../../MSystem/Raster/MRCCore.h"

#include "../../Classes/Render/MRenderVPGen.h"

#include "MRenderPS3_RSX.h"
#include "MRenderPS3_VertexProgram.h"
#include "MRenderPS3_FragmentProgram.h"
#include "MRenderPS3_Texture.h"
#include "MRenderPS3_Geometry.h"

namespace NRenderPS3GCM
{

//#define GCM_COMMANDBUFFERWRITE
//#define GCM_USEFPPT				// "Use fragment program parameter texture"

enum
{
	GCM_FPPT_NUM = 4,
	GCM_FPPT_WIDTH = 16,
	GCM_FPPT_HEIGHT = 512,
	GCM_FPPT_SAMPLER = 15,
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Classes
|__________________________________________________________________________________________________
\*************************************************************************************************/
// -------------------------------------------------------------------
//  Internal vertex-array identifiers
// -------------------------------------------------------------------
enum
{
	GCM_VA_VERTEX			= M_Bit(0),
	GCM_VA_MATRIXWEIGHT		= M_Bit(1),
	GCM_VA_NORMAL			= M_Bit(2),
	GCM_VA_COLOR			= M_Bit(3),
	GCM_VA_SECONDARYCOLOR	= M_Bit(4),
	GCM_VA_FOGCOORD			= M_Bit(5),
	GCM_VA_PSIZE			= M_Bit(6),
	GCM_VA_MATRIXINDEX		= M_Bit(7),
	GCM_VA_TEXCOORD0		= M_Bit(8),
	GCM_VA_TEXCOORD1		= M_Bit(9),
	GCM_VA_TEXCOORD2		= M_Bit(10),
	GCM_VA_TEXCOORD3		= M_Bit(11),
	GCM_VA_TEXCOORD4		= M_Bit(12),
	GCM_VA_TEXCOORD5		= M_Bit(13),
	GCM_VA_TEXCOORD6		= M_Bit(14),
	GCM_VA_TEXCOORD7		= M_Bit(15),

	GCM_VA_MATRIXWEIGHT2	= GCM_VA_FOGCOORD,
	GCM_VA_MATRIXINDEX2	= GCM_VA_SECONDARYCOLOR,
};


// -------------------------------------------------------------------
class CCGFPProgram
{
public:
	class CTreeCompare
	{
	public:
		M_INLINE static int Compare(const CCGFPProgram* _pFirst, const CCGFPProgram* _pSecond, void* _pContext)
		{
			if(_pFirst->m_Hash != _pSecond->m_Hash)
			{
				return (_pFirst->m_Hash<_pSecond->m_Hash)?-1:1;
			}
			return 0;
		}
		M_INLINE static int Compare(const CCGFPProgram* _pFirst, const uint32& _Key, void* _pContext)
		{
			if(_pFirst->m_Hash != _Key)
			{
				return (_pFirst->m_Hash<_Key)?-1:1;
			}
			return 0;
		}
	};
	CCGFPProgram() : m_Hash(0), m_Program(0), m_ConstParam(0), m_bWrongNumberOfParameters(false) {}

	DAVLAligned_Link(CCGFPProgram, m_Link, const uint32, CTreeCompare);

	uint32 m_Hash;
	CGprogram m_Program;
	CGparameter m_ConstParam;
	CStr m_Name;
	bool	m_bWrongNumberOfParameters;
};

class CCGVPProgram
{
public:
	class CTreeCompare
	{
	public:
		M_INLINE static int Compare(const CCGVPProgram* _pFirst, const CCGVPProgram* _pSecond, void* _pContext)
		{
			return memcmp(&_pFirst->m_Format, &_pSecond->m_Format, sizeof(CRC_VPFormat::CProgramFormat));
		}
		M_INLINE static int Compare(const CCGVPProgram* _pFirst, const CRC_VPFormat::CProgramFormat& _Key, void* _pContext)
		{
			return memcmp(&_pFirst->m_Format, &_Key, sizeof(CRC_VPFormat::CProgramFormat));
		}
	};

	CCGVPProgram() : m_Program(0), m_Parameter(0) {}

	DAVLAligned_Link(CCGVPProgram, m_Link, const CRC_VPFormat::CProgramFormat, CTreeCompare);

	CRC_VPFormat::CProgramFormat m_Format;
	CGprogram	m_Program;
	CGparameter	m_Parameter;	// constant parameter
};

// -------------------------------------------------------------------
class CContext_Statistics
{
public:
	uint32 m_nVTotal;
	uint32 m_nStateFP;
	uint32 m_nStateFPParam, m_nStateFPParamCount;
	uint32 m_nStateVP;
	uint32 m_nAttribSet;
	uint32 m_nTextureSet;
	uint32 m_nBindTexture;

	void PrepareFrame()
	{
		m_nVTotal = 0;
		m_nStateFP = 0;
		m_nStateFPParam = 0;
		m_nStateFPParamCount = 0;
		m_nStateVP = 0;
		m_nAttribSet = 0;
		m_nTextureSet = 0;
		m_nBindTexture = 0;
	}
};

// -------------------------------------------------------------------

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRCPS3GCM
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CDCPS3GCM;

class CRCPS3GCM : public CRC_Core
{
	friend class CDCPS3GCM;
	MRTC_DECLARE;
public:
	static CRCPS3GCM ms_This;
	MRTC_CriticalSection m_Lock;

	CContext_VertexProgram m_ContextVP;
	CContext_FragmentProgram m_ContextFP;
	CContext_Texture m_ContextTexture;
	CContext_Geometry m_ContextGeometry;

	CContext_Statistics m_ContextStatistics;
	CRC_RenderTargetDesc m_CurrentMRT;

	uint m_nCurrentClipPlanes;

//	CRenderPS3Resource_Texture m_lFPPT[GCM_FPPT_NUM];
//	uint m_iFPPTLine;
//	uint m_iFPPTTex;


	M_FORCEINLINE void Lock()
	{
		m_Lock.Lock();
	}
	M_FORCEINLINE void Unlock()
	{
		m_Lock.Unlock();
	}

	CMat4Dfp32	m_Matrix_Unit;
	CMat4Dfp32	m_Matrix_ProjectionTransposeGL;

protected:
	void Attrib_SetVPPipeline(CRC_Attributes* _pAttrib, bool _bUpdateAllConstants);
	void Attrib_SetFPPipeline(CRC_Attributes* _pAttrib);

	void Cmd_ReloadPrograms();
	void Cmd_InvalidateProgramCache();
	void Cmd_TexMinLod(int _TexID, int _LOD);
	void Cmd_TexMaxLod(int _TexID, int _LOD);
	void Cmd_TexSingleLod(int _TexID, int _LOD);
	void Cmd_Picmip(int _iPicmip, int _Picmip);
	void Cmd_DynamicLoadMips(int _Enable);
	void Cmd_Nop(fp32 _Arg);
public:
	CRCPS3GCM();
	virtual ~CRCPS3GCM();

	CDisplayContext *GetDC();
	void Create(CObj* _pContext, const char* _pParams);

	void BeginScene(CRC_Viewport* _pVP);
	void EndScene();
	const char * GetRenderingStatus();
	const char * GetRenderingStatus(int _iStatus);

	void Viewport_Update();

	void Flip_SetInterval(int _nFrames){};

	// Attribute overrides
	int Attrib_GlobalGetVar(int _Var);

	void Attrib_Set(CRC_Attributes* _pAttrib);
	void Attrib_SetAbsolute(CRC_Attributes* _pAttrib);

	void Attrib_GlobalUpdate();
	void Attrib_DeferredGlobalUpdate();
#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	void Attrib_SetPolygonOffset(fp32 _Offset);
#endif

public:
	int OcclusionQuery_AddQueryID(int _QueryID);
	int OcclusionQuery_FindOCID(int _QueryID);
	void OcclusionQuery_Init();
	void OcclusionQuery_PrepareFrame();
	void OcclusionQuery_Begin(int _QueryID);
	void OcclusionQuery_End();
	int OcclusionQuery_GetVisiblePixelCount(int _QueryID);

	// -------------------------------------------------------------------
	bool ReadDepthPixels(int _x, int _y, int _w, int _h, fp32* _pBuffer);

	// Console commands:

	void Register(CScriptRegisterContext &_RegContext);

	// Precache interface

	// MRT interface
	void RenderTarget_SetEnableMasks(uint32 _And);
	void RenderTarget_SetRenderTarget(const CRC_RenderTargetDesc& _RenderTarget);
	void RenderTarget_SetNextClearParams(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StecilValue);

	void RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StecilValue);
	void RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType);
	void RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice, int _iMRT);

	void Render_PrecacheFlush();
	void Render_PrecacheEnd();

	// Texture interface
	void Texture_Precache(int _TextureID);
	void Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos);
	CRC_TextureMemoryUsage Texture_GetMem(int _TextureID);
	int Texture_GetPicmipFromGroup(int _iPicmip);

	int Texture_GetBackBufferTextureID() {return 0;}
	int Texture_GetFrontBufferTextureID() {return 0;}
	int Texture_GetZBufferTextureID() {return 0;}

	// Geometry interface
	int Geometry_GetVBSize(int _VBID);
	void Geometry_Color(uint32 _Col);

	void Geometry_Precache(int _VBID);

	// Rendering overrides
	void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles);
	void Render_IndexedWires(uint16* _pIndices, int _Len);
 	void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen);

	void Render_VertexBuffer(int _VBID);
	void Render_VertexBuffer_IndexBufferTriangles(uint _VBID, uint _IBID, uint _nTriangles, uint _PrimOffset);

	void Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color);
	void Render_WireStrip(const CVec3Dfp32* _pV, const uint16* _piV, int _nVert, CPixel32 _Color);
	void Render_WireLoop(const CVec3Dfp32* _pV, const uint16* _piV, int _nVert, CPixel32 _Color);

	void Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, const CVec4Dfp32* _pSpec,
			const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color);

	// Transform overrides
	void Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix);


	CRC_Statistics m_Stats;
	CRC_Statistics Statistics_Get()
	{
		return CRCPS3GCM::ms_This.m_Stats;
	}

#ifndef PLATFORM_PS3
	void Texture_PrecacheFlush() {}
	void Texture_PrecacheBegin(int _Count) {}
	void Texture_PrecacheEnd() {}
#endif
};

void GetMinMax(const uint16* _pPrim, int _nPrim, uint16& _Min, uint16& _Max);

};

#endif //_INC_MRndrGL
