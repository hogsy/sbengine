

#include "PCH.h"
#include "../../MOS.h"

#include "../../XR/XRVBContext.h"

namespace NRenderPS3GCM
{

CRCPS3GCM CRCPS3GCM::ms_This;

// 0x500 offsetted
const char* aGLESErrorMessages[] = 
{
	"GL_INVALID_ENUM",
	"GL_INVALID_VALUE",
	"GL_INVALID_OPERATION",
	"GL_STACK_OVERFLOW",
	"GL_STACK_UNDERFLOW",
	"GL_OUT_OF_MEMORY",
	"GL_INVALID_FRAMEBUFFER_OPERATION_OES"
};

const char* aGLCGErrorMessages[] =
{
#ifndef WIN32
#define CG_ERROR_MACRO(code, enum_name, msg)\
	msg,
#include <cg/cg_errors.h>
#else
	"dummy"
#endif
};


// -------------------------------------------------------------------
//  OpenGL Render Context	
// -------------------------------------------------------------------

CRCPS3GCM::CRCPS3GCM()
{
	MSCOPE(CRCPS3GCM::CRCPS3GCM, RENDER_PS3);
	MRTC_AddRef();
	MRTC_AddRef();
	MRTC_AddRef();

	m_iVBCtxRC = -1;
	m_iTC = -1;

	CRC_VPFormat::InitRegsNeeded();
};

CRCPS3GCM::~CRCPS3GCM()
{
	MSCOPE(CRCPS3GCM::~CRCPS3GCM, RENDER_PS3);

	if (m_iVBCtxRC >= 0) { m_pVBCtx->RemoveRenderContext(m_iVBCtxRC); m_iVBCtxRC = -1; };
	if (m_iTC >= 0) { m_pTC->RemoveRenderContext(m_iTC); m_iTC = -1; };
};

extern const char* gs_TexEnvPrograms[10];
extern uint32 gs_TexEnvProgramHash[10];

void CRCPS3GCM::Create(CObj* _pContext, const char* _pParams)
{
	MSCOPE(CRCPS3GCM::Create, RENDER_PS3);

	CRC_Core::Create(_pContext, _pParams);

	for(uint i = 0; i < 10; i++)
		gs_TexEnvProgramHash[i] = StringToHash(gs_TexEnvPrograms[i]);

	MACRO_GetSystem;

	m_Caps_TextureFormats = -1;
	m_Caps_DisplayFormats = -1;
	m_Caps_ZFormats = -1;
	m_Caps_StencilDepth = 8;
	m_Caps_AlphaDepth = 8;
	m_Caps_Flags	= CRC_CAPS_FLAGS_HWAPI
//					| CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP
					| CRC_CAPS_FLAGS_SPECULARCOLOR
					| CRC_CAPS_FLAGS_CUBEMAP
					| CRC_CAPS_FLAGS_MATRIXPALETTE
//					| CRC_CAPS_FLAGS_OFFSCREENTEXTURERENDER
					| CRC_CAPS_FLAGS_OCCLUSIONQUERY
					| CRC_CAPS_FLAGS_TEXGENMODE_ENV
					| CRC_CAPS_FLAGS_TEXGENMODE_ENV2
					| CRC_CAPS_FLAGS_TEXGENMODE_LINEARNHF
					| CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF
					| CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE
					| CRC_CAPS_FLAGS_SEPARATESTENCIL
					| CRC_CAPS_FLAGS_FRAGMENTPROGRAM20
					| CRC_CAPS_FLAGS_FRAGMENTPROGRAM30
					| CRC_CAPS_FLAGS_MRT
					| CRC_CAPS_FLAGS_PRIMITIVERESTART;

	m_Caps_nMultiTexture = Min(16, (int)CRC_MAXTEXTURES);
	m_Caps_nMultiTextureCoords = Min(8, (int)CRC_MAXTEXCOORDS);
	m_Caps_nMultiTextureEnv = Min(8, (int)CRC_MAXTEXCOORDS);

	m_iTC = m_pTC->AddRenderContext(this);
	m_iVBCtxRC = m_pVBCtx->AddRenderContext(this);

	m_CurrentAttrib.SetDefaultReal();

	m_ContextVP.Create();
	m_ContextFP.Create();
	m_ContextTexture.Create(m_pTC, m_pTC->GetIDCapacity(), m_spTCIDInfo);
	m_ContextGeometry.Create(m_pVBCtx, m_pVBCtx->GetIDCapacity(), m_lVBIDInfo);

	OcclusionQuery_Init();

	m_Matrix_Unit.Unit();
	m_Matrix_ProjectionTransposeGL.Unit();
/*
	{
		for(int iTex = 0; iTex < GCM_FPPT_NUM; iTex++)
		{
			CRenderPS3Resource_Texture* pTex = &m_lFPPT[iTex];
			pTex->Alloc2D(GCM_FPPT_WIDTH, GCM_FPPT_HEIGHT, 1, CTextureGCM::TEX_FORMAT_RGBA32_F, GCM_TEXALLOC_MAINMEM | GCM_TEXALLOC_FORCELINEAR);
			pTex->m_MaxAnisoFiltering = 0;
			pTex->m_MinFilter = CELL_GCM_TEXTURE_NEAREST;
			pTex->m_MagFilter = CELL_GCM_TEXTURE_NEAREST;
			pTex->m_WrapS = CELL_GCM_TEXTURE_WRAP;
			pTex->m_WrapT = CELL_GCM_TEXTURE_WRAP;
			pTex->m_WrapR = CELL_GCM_TEXTURE_CLAMP_TO_EDGE;
			pTex->m_URemap = 0;
			pTex->m_sRGBGamma = 0;
			pTex->m_ConvFilter = CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX;
		}

		m_iFPPTTex = 0;
		m_iFPPTLine = 0;
	}
*/
	m_ContextVP.LoadCache();
	m_ContextFP.LoadCache();
}

//----------------------------------------------------------------
CMTime StartFrame;
void CRCPS3GCM::BeginScene(CRC_Viewport* _pVP)
{
	MSCOPE(CRCPS3GCM::BeginScene, RENDER_PS3);

	StartFrame.Snapshot();

	CRC_Core::BeginScene(_pVP);

	m_CurrentMRT.Clear();
	m_nTriangles = 0;
	m_nWires = 0;

	OcclusionQuery_PrepareFrame();

	gcmSetTransferLocation(CELL_GCM_LOCATION_LOCAL);
	gcmSetRestartIndex(0xffff);
	gcmSetRestartIndexEnable(CELL_GCM_TRUE);
	gcmSetFrontFace(CELL_GCM_CCW);
	gcmSetCullFace(CELL_GCM_CCW);

	if (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		gcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_LINE);
		gcmSetBackPolygonMode(CELL_GCM_POLYGON_MODE_LINE);
	}
	else
	{
		gcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_FILL);
		gcmSetBackPolygonMode(CELL_GCM_POLYGON_MODE_FILL);
	}
	m_ContextStatistics.PrepareFrame();
	m_ContextGeometry.PrepareFrame();

	static fp32 DefaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	gcmSetVertexData4f(ATTRIB_COLOR0, DefaultColor);

	// Reset all render attributes
	CRC_Attributes Attrib;
	Attrib.SetDefault();
	Attrib_SetAbsolute(&Attrib);

	gcmSetZcullStatsEnable(CELL_GCM_TRUE);
	gcmSetZPassPixelCountEnable(CELL_GCM_TRUE);
	gcmSetClearReport(CELL_GCM_ZPASS_PIXEL_CNT);
	gcmSetClearReport(CELL_GCM_ZCULL_STATS);

	gcmFlush();

//	m_lFPPT[m_iFPPTTex].Bind(GCM_FPPT_SAMPLER);
}

CMTime LastFrame = CMTime::GetCPU();
void CRCPS3GCM::EndScene()
{
	MSCOPE(CRCPS3GCM::EndScene, RENDER_PS3);

	// Make sure textures are uploaded
	m_ContextTexture.Service(10);

	if(m_ContextFP.m_bDirtyCache)
	{
		m_ContextFP.SaveCache();
	}
	if(m_ContextVP.m_bDirtyCache)
	{
		m_ContextVP.SaveCache();
	}

	CMTime Tick;
	Tick.Snapshot();
	CMTime Delta = Tick - LastFrame;
	LastFrame = Tick;
}

const char* CRCPS3GCM::GetRenderingStatus(int _iStatus)
{
	if(_iStatus == 0)
		return GetRenderingStatus();
	else if(_iStatus == 1)
		return CPS3_Heap::ms_This.GetStatus();
	return "";
}

const char* CRCPS3GCM::GetRenderingStatus()
{
	MSCOPE(CRCPS3GCM::GetRenderingStatus, RENDER_PS3);

	static char TempChar[1024];
	TempChar[0] = 0;
	CStrBase::snprintf(TempChar, 1024, "PS3GCM,Tri %d Wire %d,Attrib %d,VP %d FP %d FPParam %d/%d,",
		m_nTriangles, m_nWires,
		m_ContextStatistics.m_nAttribSet,
		m_ContextStatistics.m_nStateVP, m_ContextStatistics.m_nStateFP, m_ContextStatistics.m_nStateFPParam, m_ContextStatistics.m_nStateFPParamCount);
	return TempChar;
};

static bool bDumpVP = false;
void CRCPS3GCM::Viewport_Update()
{
	MSCOPE(CRCPS3GCM::Viewport_Update, RENDER_PS3);

	CRC_Core::Viewport_Update();
	CRC_Viewport* pVP = Viewport_Get();

	// Set projection
	CMat4Dfp32 ProjMat = pVP->GetProjectionMatrix();

	// Set viewport
	CRct rectclipped(pVP->GetViewArea());
//	int h = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;
//	int h = CDCPS3GCM::ms_This.m_FrontBuffer.m_Height;
	uint h = CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;
	{
		uint16 x,y,w,h;
		float min, max;
		float scale[4],offset[4];

		x = rectclipped.p0.x;
		y = rectclipped.p0.y;
		w = rectclipped.p1.x - rectclipped.p0.x;
		h = rectclipped.p1.y - rectclipped.p0.y;
		min = 0.0f;
		max = 1.0f;
		scale[0] = w * 0.5f;
		scale[1] = h * -0.5f;
		scale[2] = (max - min) * 0.5f;
		scale[3] = 0.0f;
		offset[0] = x + scale[0];
		offset[1] = h - y + scale[1];
		offset[2] = (max + min) * 0.5f;
		offset[3] = 0.0f;

		gcmSetViewport(x, y, w, h, min, max, scale, offset);
		if(bDumpVP)
		{
			M_TRACEALWAYS("gcmSetViewport(%d, %d, %d, %d, %f, %f, [%f, %f, %f, %f], [%f, %f, %f, %f])\r\n", x, y, w, h, min, max, scale[0], scale[1], scale[2], scale[3], offset[0], offset[1], offset[2], offset[3]);
		}
	}

	fp32 xs = 2.0/rectclipped.GetWidth();
	fp32 ys = 2.0/rectclipped.GetHeight();
	ProjMat.k[0][0] *= xs;	ProjMat.k[1][0] *= xs;	ProjMat.k[2][0] *= xs;	ProjMat.k[3][0] *= xs;
	ProjMat.k[0][1] *= ys;	ProjMat.k[1][1] *= ys;	ProjMat.k[2][1] *= ys;	ProjMat.k[3][1] *= ys;

	ProjMat.Transpose();
	m_Matrix_ProjectionTransposeGL = ProjMat;
	if(bDumpVP)
	{
		M_TRACEALWAYS("Matrix: %s\r\n", ProjMat.GetString().Str());
		bDumpVP = false;
	}

	CRCPS3GCM::ms_This.m_MatrixChanged |= 1 << CRC_MATRIX_MODEL;
};

void CRCPS3GCM::Render_PrecacheFlush()
{
	m_ContextTexture.Precache_Flush();
	m_ContextGeometry.PrecacheFlush();

	CPS3_Heap::ms_This.Defrag();
}

void CRCPS3GCM::Render_PrecacheEnd()
{
	m_ContextTexture.Precache_End();
	m_ContextGeometry.PrecacheEnd();

	CPS3_Heap::ms_This.Defrag();
}

CDisplayContext* CRCPS3GCM::GetDC()
{
	MSCOPE(CRCPS3GCM::GetDC, RENDER_PS3);
	return &CDCPS3GCM::ms_This;
}

};	// namespace NRenderPS3GCM

using namespace NRenderPS3GCM;
MRTC_IMPLEMENT_DYNAMIC(CRCPS3GCM, CRC_Core);

struct SAllocation
{
	uint32	m_TypeColor;
	uint32	m_Offset;
	uint32	m_Size;
};

void GetHeapSize(uint32& _Start, uint32& _Stop)
{
	_Start = CPS3_Heap::ms_This.GetHeapStart();
	_Stop = CPS3_Heap::ms_This.GetHeapEnd();
}

uint32 GetAllocations(uint _iStart, uint _MaxCount, SAllocation* _pAllocations)
{
	DLock(CPS3_Heap::ms_This);
	uint nAllocations = 0;

	CPS3_Heap::CBlock* pBlock = CPS3_Heap::ms_This.m_pFirstBlock;
	while(_iStart > 0 && pBlock)
	{
		_iStart--;
		pBlock = pBlock->m_pNextBlock;
	}

	for(uint i = 0; i < _MaxCount; i++)
	{
		if(!pBlock)
			break;

		_pAllocations[nAllocations].m_Offset = pBlock->GetMemory();
		_pAllocations[nAllocations].m_Size = CPS3_Heap::ms_This.GetBlockSize(pBlock);
		_pAllocations[nAllocations].m_TypeColor = 0x007f0000;
		nAllocations++;

		pBlock = pBlock->m_pNextBlock;
	}

	return nAllocations;
}
