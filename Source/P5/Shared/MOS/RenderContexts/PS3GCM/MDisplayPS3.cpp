#include "PCH.h"

#include "MDisplayPS3.h"
#include "../../MOS.h"

#include "MRenderPS3_Texture.h"

// Remember to link with gcm_hud when enabling this and reduce the amount mapped memory
//#define ENABLE_GMC_HUD

#ifdef PLATFORM_PS3
 #include <sysutil/sysutil_sysparam.h>
#endif

#ifdef ENABLE_GCM_HUD
 #include <cell/gcm_hud.h>
#endif

//JK-TODO: ZCull setup might have to be changed here

void* ms_pRCOwner = NULL;

CGCMShadow gs_GCMShadow;
uint32 gs_GCMSync = 0;
uint32 gs_GCMGraph = 0;

void* gs_lpPS3Heaps[8] = {0};
uint32 gs_nPS3Heaps = 0;
uint32 gs_PS3HeapSize[8] = {0};
uint32 gs_PS3PushBufferSize = 8 * 1024 * 1024;
namespace NRenderPS3GCM
{


CDCPS3GCM CDCPS3GCM::ms_This;

uint32 CRenderPS3Resource::ms_ResourceFrameIndex = 0;

NThread::CAtomicIntAggregate m_FrameCount;
NThread::CEventAutoResetAggregate m_FrameEvent;

static uint m_bUseFBO = 1;
class CTextureContainer_FBO : public CTextureContainer
{
	MRTC_DECLARE;
protected:
	class CFBOInfo
	{
	public:
		uint16	m_Width;
		uint16	m_Height;
		uint16	m_ColorTexID;
		uint16	m_Used;
		uint32	m_Format;

		CFBOInfo()
		{
			m_Width = 0;
			m_Height = 0;
			m_ColorTexID = 0;
			m_Used = 0;
			m_Format = IMAGE_FORMAT_RGBA16_F;
		}
	};
public:
	TArray<CFBOInfo> m_lTxtInfo;

	virtual void Create(uint _nFBO)
	{
		m_lTxtInfo.SetLen(_nFBO);

		for(uint i = 0; i < _nFBO; i++)
		{
			m_lTxtInfo[i].m_ColorTexID = m_pTC->AllocID(m_iTextureClass, i, (const char*)NULL);
		}
	}

	virtual ~CTextureContainer_FBO()
	{
		if(m_pTC)
		{
			for(uint i = 0; i < m_lTxtInfo.Len(); i++)
			{
				CFBOInfo& TexInfo = m_lTxtInfo[i];
				if(TexInfo.m_ColorTexID) m_pTC->FreeID(TexInfo.m_ColorTexID);
				TexInfo.m_ColorTexID = 0;
			}
		}
	}

	int AllocTexture(uint _Width, uint _Height, uint _Format)
	{
		for(int i = 0; i < m_lTxtInfo.Len(); i++)
		{
			CFBOInfo& TexInfo = m_lTxtInfo[i];
			if(TexInfo.m_Used == 0)
			{
				m_pTC->MakeDirty(TexInfo.m_ColorTexID);
				TexInfo.m_Width = _Width;
				TexInfo.m_Height = _Height;
				TexInfo.m_Format = _Format;
				TexInfo.m_Used = true;
				return i;
			}
		}

		M_BREAKPOINT;

		return -1;
	}

	void FreeTexture(uint _iLocal)
	{
		CFBOInfo& TexInfo = m_lTxtInfo[_iLocal];
		M_ASSERT(TexInfo.m_Used, "!");
		TexInfo.m_Used = 0;
	}

	int GetNumLocal()
	{
		return m_lTxtInfo.Len();
	}

	int GetTextureID(int _iLocal)
	{
		return m_lTxtInfo[_iLocal].m_ColorTexID;
	}

	int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
	{
		CFBOInfo& TexInfo = m_lTxtInfo[_iLocal];
		_Ret_nMipmaps = 1;
		if(_pTargetImg)
			_pTargetImg->CreateVirtual(TexInfo.m_Width, TexInfo.m_Height, TexInfo.m_Format, IMAGE_MEM_VIDEO | IMAGE_MEM_TEXTURE);

		return 0;
	}

	void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
	{
		CTC_TextureProperties Prop;
		Prop.m_Flags |= 
			CTC_TEXTUREFLAGS_NOMIPMAP | 
			CTC_TEXTUREFLAGS_NOPICMIP | 
			CTC_TEXTUREFLAGS_CLAMP_U | 
			CTC_TEXTUREFLAGS_CLAMP_V | 
			CTC_TEXTUREFLAGS_HIGHQUALITY | 
			CTC_TEXTUREFLAGS_NOCOMPRESS |
			CTC_TEXTUREFLAGS_PROCEDURAL |
			CTC_TEXTUREFLAGS_BACKBUFFER |
			CTC_TEXTUREFLAGS_RENDERTARGET |
			CTC_TEXTUREFLAGS_RENDER;

		Prop.m_MinFilter = CTC_MINFILTER_LINEAR;
		Prop.m_MagFilter = CTC_MAGFILTER_LINEAR;
		Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;

		_Prop = Prop;
	}

	void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
	{
//		Error("BuildInto", CStrF("Invalid call. This texture is a render-texture. Local %d, TextureID %d", _iLocal, GetTextureID(_iLocal) ));
	}
};

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_FBO, CTextureContainer);

CTextureContainer_FBO* m_pTCFBO = NULL;

// -------------------------------------------------------------------
CDCPS3GCM::CDCPS3GCM()
{
	MSCOPE(CDCPS3GCM::-, RENDER_PS3);
	MRTC_AddRef();
	MRTC_AddRef();
	MRTC_AddRef(); // Make sure we aren't deleted

	m_DeferredMode = ~0;
	m_DeferredLoop = 0;
	m_bLog = false;
	m_bVSync = true;
	m_bAntialias = false;
	m_bAddedToConsole = false;
	m_bPendingResetMode = false;
	m_BackBufferFormat = IMAGE_FORMAT_BGRX8;
	m_pMainThread = MRTC_SystemInfo::OS_GetThreadID();
	ms_pRCOwner = m_pMainThread;
	if(!CDiskUtil::DirectoryExists("System/PS3/Cache"))
		CDiskUtil::CreatePath("System/PS3/Cache");
	if(!CDiskUtil::DirectoryExists("System/PS3/Broken"))
		CDiskUtil::CreatePath("System/PS3/Broken");
}

CDCPS3GCM::~CDCPS3GCM()
{
	MSCOPE(CDCPS3GCM::~, RENDER_PS3);


	if(m_bAddedToConsole)
	{
		MACRO_RemoveSubSystem(this);
		RemoveFromConsole();
	}

}

bool CDCPS3GCM::SetOwner(void* _pNewOwner)
{
	M_BREAKPOINT;

	return true;
}

void gcmCallback(uint32 _Cause)
{
	if(_Cause == ~0)
	{
		if(m_FrameCount.Increase() == 0)
			m_FrameEvent.Signal();
	}
}

void CDCPS3GCM::Create()
{
	MSCOPE(CDCPS3GCM::Create, RENDER_PS3);

	if(m_bAddedToConsole)
		return;

	m_FrameCount.Construct();
	m_FrameCount.Exchange(2);
	m_FrameEvent.Construct();

	// Call super
	CDisplayContext::Create();

	// Heap 0 contains all available main memory (up to 256MiB)
	DAssert(gcmInit(gs_PS3PushBufferSize, gs_PS3HeapSize[0], gs_lpPS3Heaps[0]), ==, CELL_OK);
#ifdef ENABLE_GCM_HUD
	cellGcmHUDInit();
#endif
	gcmEmulateNewSwitch();
	cellGcmSetDebugOutputLevel(CELL_GCM_DEBUG_LEVEL2);

	gcmSetUserHandler(gcmCallback);

//	DAssert(cellSysutilInit(), ==, CELL_OK);

	CellGcmConfig gcmConfig;
	gcmGetConfiguration(&gcmConfig);
	M_TRACEALWAYS("RSX: Core %dMHz, Mem %dMHz. Local 0x%.8X : %d, Main 0x%.8X : %d\n", gcmConfig.coreFrequency / 1000000, gcmConfig.memoryFrequency / 1000000, gcmConfig.localAddress, gcmConfig.localSize, gcmConfig.ioAddress, gcmConfig.ioSize);

#ifdef PLATFORM_PS3
	memset(gcmConfig.localAddress, 0x55, gcmConfig.localSize);
#else
	m_lLocalHeapEmu.SetLen(256 * 1024 * 1024);
	gcmConfig.localAddress = m_lLocalHeapEmu.GetBasePtr();
	gcmConfig.localSize = 256 * 1024 * 1024;
#endif

	CPS3_Heap::ms_This.Create(gcmConfig.localAddress, gcmConfig.localSize, 16, false);

	EnumModes();
	InitSettings();
	AddToConsole();
	m_bAddedToConsole = true;
	CRCPS3GCM::ms_This.Create(this, "");

	m_pTCFBO = MNew(CTextureContainer_FBO);
	m_pTCFBO->Create(8);

	MACRO_AddSubSystem(this);
}

// -------------------------------------------------------------------

int GetColorBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_RGB10A2_F:
	case IMAGE_FORMAT_BGR10A2: return 30;
	case IMAGE_FORMAT_BGRA4: return 12;
	case IMAGE_FORMAT_BGR5A1:
	case IMAGE_FORMAT_BGR5: return 15;

	case IMAGE_FORMAT_I16:
	case IMAGE_FORMAT_RGB16:
	case IMAGE_FORMAT_B5G6R5: return 16;

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_BGRX8:
	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8: return 24;

	case IMAGE_FORMAT_RGBA16_F:
	case IMAGE_FORMAT_RGBA16:
	case IMAGE_FORMAT_BGRA16: return 48;

	case IMAGE_FORMAT_I8A8:
	case IMAGE_FORMAT_I8: return 8;

	case IMAGE_FORMAT_RGBA32_F:
	case IMAGE_FORMAT_RGB32_F: return 96;
	}
	return 0;
}

int GetAlphaBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_RGB10A2_F:
	case IMAGE_FORMAT_BGR10A2: return 2;

	case IMAGE_FORMAT_BGRA4: return 4;

	case IMAGE_FORMAT_BGR5A1: return 1;

	case IMAGE_FORMAT_RGBA16_F:
	case IMAGE_FORMAT_RGBA16:
	case IMAGE_FORMAT_BGRA16: return 16;

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_I8A8:
	case IMAGE_FORMAT_A8: return 8;

	case IMAGE_FORMAT_RGBA32_F: return 32;
	}
	return 0;
}

int GetDepthBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_Z24S8: return 24;
	}

	return 0;
}

int GetStencilBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_Z24S8: return 8;
	}

	return 0;
}

void CDCPS3GCM::ResetMode()
{
	MSCOPE(CDCPS3GCM::ResetMode, RENDER_PS3);
}
/*
class CRenderPS3Resource_Local_DisplayTarget : public CRenderPS3Resource_Local<GPU_FRAMEBUFFER_ALIGNMENT>
{
protected:
	class CRenderPS3Resource_Local_DisplayTarget_BlockInfo : public CRenderPS3Resource_Local<GPU_FRAMEBUFFER_ALIGNMENT>::CRenderPS3Resource_Local_BlockInfo
	{
	public:
		uint32 GetRealSize() {return m_Size;}
		uint32 m_Size;
	};

public:
	CRenderPS3Resource_Local_DisplayTarget()
	{
		m_Width = 0;
		m_Height = 0;
	}

	uint16 m_Width, m_Height;
	uint32 m_Pitch;
	uint32 m_Format;
	CRenderPS3Resource_Local_DisplayTarget_BlockInfo m_BlockInfo;
	CellGcmTexture m_RSXTexture;

	void Create(uint _Width, uint _Height, uint _Format)
	{
		m_Width = _Width;
		m_Height = _Height;
		switch(_Format)
		{
		case IMAGE_FORMAT_RGBA8:
			{
				m_BlockInfo.m_Size = _Width * _Height * 4;
				m_Pitch = _Width * 4;
				m_RSXTexture.format = CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_LN;
				m_RSXTexture.pitch = m_Pitch;
				break;
			}
		case IMAGE_FORMAT_RGBA16_F:
			{
				m_BlockInfo.m_Size = _Width * _Height * 8;
				m_Pitch = _Width * 8;
				m_RSXTexture.format = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_LN;
				m_RSXTexture.pitch = m_Pitch;
				break;
			}
		}

		Alloc(m_BlockInfo.m_Size, tAlignment, &m_BlockInfo, false);

		m_RSXTexture.mipmap = 1;
		m_RSXTexture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;
		m_RSXTexture.cubemap = CELL_GCM_FALSE;
		m_RSXTexture.remap = REMAP_INPUT(A, R, G, B) | REMAP_OUTPUT(REMAP, REMAP, REMAP, REMAP);
		m_RSXTexture.width = _Width;
		m_RSXTexture.height = _Height;
		m_RSXTexture.depth = 1;
		m_RSXTexture.location = CELL_GCM_LOCATION_LOCAL;
		m_RSXTexture.offset = GetOffset();
	}

	void Bind()
	{
		CellGcmSurface sf;
		sf.colorFormat 	= CELL_GCM_SURFACE_A8R8G8B8;
		sf.colorTarget	= CELL_GCM_SURFACE_TARGET_0;
		sf.colorLocation[0]	= CELL_GCM_LOCATION_LOCAL;
		sf.colorOffset[0] 	= GetOffset();
		sf.colorPitch[0] 	= GetPitch();

		sf.colorLocation[1]	= CELL_GCM_LOCATION_LOCAL;
		sf.colorLocation[2]	= CELL_GCM_LOCATION_LOCAL;
		sf.colorLocation[3]	= CELL_GCM_LOCATION_LOCAL;
		sf.colorOffset[1] 	= 0;
		sf.colorOffset[2] 	= 0;
		sf.colorOffset[3] 	= 0;
		sf.colorPitch[1]	= 64;
		sf.colorPitch[2]	= 64;
		sf.colorPitch[3]	= 64;

		sf.depthFormat 	= CELL_GCM_SURFACE_Z24S8;
		sf.depthLocation	= CELL_GCM_LOCATION_LOCAL;
		sf.depthOffset	= CDCPS3GCM::ms_This.m_ZBuffer.GetOffset();
		sf.depthPitch 	= CDCPS3GCM::ms_This.m_ZBuffer.GetPitch();

		sf.type		= CELL_GCM_SURFACE_PITCH;
		sf.antialias	= CELL_GCM_SURFACE_CENTER_1;

		sf.width 		= m_Width;
		sf.height 		= m_Height;
		sf.x 		= 0;
		sf.y 		= 0;
		gcmSetSurface(&sf);
	}

	// Render to frontbuffer (done in pageflip)
	void Resolve()
	{
		CMat4Dfp32 UnitMat;
		UnitMat.Unit();
		CRCPS3GCM::ms_This.Matrix_SetRender(CRC_MATRIX_MODEL, &UnitMat);

		CRC_Attributes Attrib;
		Attrib.SetDefault();
		Attrib.m_TextureID[0] = 
		gcmSetTexture(0, &m_RSXTexture);
		gcmSetTextureAddress(0, 0, 0, 0, 0, 0, 0);
		gcmSetTextureControl(0, CELL_GCM_TRUE, 0 << 8, 1 << 8, 0);
		gcmSetTextureFilter(0, 0, 0, 0, 0);

		CRC_ExtAttributes_FragmentProgram20 ExtAttr;
		ExtAttr.Clear();
		ExtAttr.SetProgram("TexEnvMode01", 0);
		CRCPS3GCM::ms_This.m_ContextFP.Bind(&ExtAttr);


		CRCPS3GCM::ms_This.m_ContextVP.Disable();
		CRCPS3GCM::ms_This.m_ContextFP.Disable();
	}

};
*/

void CDCPS3GCM::RestoreRenderContext(const CBackbufferContext& _Context)
{
	CellGcmSurface sf;
	uint nRT = _Context.m_Setup.m_nRT;
	uint ColorTargets[5] = {CELL_GCM_SURFACE_TARGET_NONE, CELL_GCM_SURFACE_TARGET_0, CELL_GCM_SURFACE_TARGET_MRT1, CELL_GCM_SURFACE_TARGET_MRT2, CELL_GCM_SURFACE_TARGET_MRT3};
	sf.colorFormat 	= _Context.m_Setup.m_BackbufferFormat;
	sf.colorTarget	= ColorTargets[_Context.m_Setup.m_nRT];
	sf.colorLocation[0]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[1]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[2]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[3]	= CELL_GCM_LOCATION_LOCAL;

	sf.colorOffset[0] 	= (nRT > 0)?_Context.m_ColorOffset[0]:0;
	sf.colorOffset[1] 	= (nRT > 1)?_Context.m_ColorOffset[1]:0;
	sf.colorOffset[2] 	= (nRT > 2)?_Context.m_ColorOffset[2]:0;
	sf.colorOffset[3] 	= (nRT > 3)?_Context.m_ColorOffset[3]:0;
	sf.colorPitch[0] 	= (nRT > 0)?_Context.m_ColorPitch[0]:64;
	sf.colorPitch[1] 	= (nRT > 1)?_Context.m_ColorPitch[1]:64;
	sf.colorPitch[2] 	= (nRT > 2)?_Context.m_ColorPitch[2]:64;
	sf.colorPitch[3] 	= (nRT > 3)?_Context.m_ColorPitch[3]:64;

	sf.depthFormat 	= _Context.m_Setup.m_DepthbufferFormat;
	sf.depthLocation	= CELL_GCM_LOCATION_LOCAL;
	sf.depthOffset	= _Context.m_DepthOffset;
	sf.depthPitch 	= _Context.m_DepthPitch;

	sf.type		= CELL_GCM_SURFACE_PITCH;
	sf.antialias	= CELL_GCM_SURFACE_CENTER_1;

	sf.width 		= _Context.m_Setup.m_Width;
	sf.height 		= _Context.m_Setup.m_Height;
	sf.x 		= 0;
	sf.y 		= 0;
	gcmSetSurface(&sf);

	//JK-TODO: ZCull setup might have to be changed here

	m_CurrentBackbufferContext = _Context;
}


void CDCPS3GCM::SetMode(int _Mode)
{
	MSCOPE(CDCPS3GCM::SetMode, RENDER_PS3);

	if(_Mode == m_DisplayModeNr)
		return;

	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwnerThread)
	{
		m_DeferredWait.ResetSignaled();
		m_DeferredLoop = 2;
		m_DeferredMode = _Mode;
		m_DeferredWait.Wait();
		return;
	}

	CDCGCM_VideoMode* pMode = (CDCGCM_VideoMode*)((CDC_VideoMode*)m_lspModes[_Mode]);

	uint FBOFormat = CELL_GCM_SURFACE_F_W16Z16Y16X16;
	uint TexFormat = IMAGE_FORMAT_RGBA16_F;
//	if(0 && pMode->m_Format.GetFormat() == IMAGE_FORMAT_RGBA8)
	{
		FBOFormat = CELL_GCM_SURFACE_A8R8G8B8;
		TexFormat = IMAGE_FORMAT_RGBA8;
	}

	uint Width = pMode->m_Format.GetWidth();
	uint Height = pMode->m_Format.GetHeight();

	M_TRACEALWAYS(CStrF("Setting mode %d, %dx%d %s", _Mode, Width, Height, pMode->m_Format.GetFormatName(TexFormat)));

	if(m_DisplayModeNr != ~0)
	{
		// SetMode called once already, delete that context?
		gcmFinish();

		FreeTile(m_ZBuffer.m_iTile);
		m_ZBuffer.Destroy();

		if(m_bUseFBO)
		{
			uint TexID = m_pTCFBO->GetTextureID(0);
			CRCPS3GCM::ms_This.m_ContextTexture.Destroy(TexID);
			m_pTCFBO->FreeTexture(0);
		}

	}
	else
	{
		CellVideoOutConfiguration videoConfiguration;
		memset(&videoConfiguration, 0, sizeof(videoConfiguration));
		videoConfiguration.resolutionId = pMode->m_ResolutionID;
		videoConfiguration.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		videoConfiguration.pitch = m_FrontBuffer.GetPitch();
		DAssert(cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videoConfiguration, NULL, 0), ==, CELL_OK);

//		gcmSetFlipMode(CELL_GCM_DISPLAY_HSYNC_WITH_NOISE);
		gcmSetFlipMode(CELL_GCM_DISPLAY_HSYNC);
//		gcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

		gcmSetDisplayBuffer(0, m_FrontBuffer.GetOffset(0), m_FrontBuffer.GetPitch(), m_FrontBuffer.m_Width, m_FrontBuffer.m_Height);
		gcmSetDisplayBuffer(1, m_FrontBuffer.GetOffset(1), m_FrontBuffer.GetPitch(), m_FrontBuffer.m_Width, m_FrontBuffer.m_Height);

		m_DefaultBackbufferContext.m_Setup.m_Width = m_FrontBuffer.m_Width;
		m_DefaultBackbufferContext.m_Setup.m_Height = m_FrontBuffer.m_Height;
		m_DefaultBackbufferContext.m_Setup.m_BackbufferFormat = CELL_GCM_SURFACE_A8R8G8B8;
		m_DefaultBackbufferContext.m_Setup.m_DepthbufferFormat = CELL_GCM_SURFACE_Z24S8;
		m_DefaultBackbufferContext.m_Setup.m_nRT = 1;
		m_DefaultBackbufferContext.m_Setup.m_ColorBuffer = CBackbufferContext::CSetup::BUFFER_ATTACHED;
		m_DefaultBackbufferContext.m_Setup.m_DepthBuffer = CBackbufferContext::CSetup::BUFFER_UNUSED;
		m_DefaultBackbufferContext.m_Setup.m_AntiAlias = 0;
		m_DefaultBackbufferContext.m_Setup.m_ZCullEnabled = 1;
		m_DefaultBackbufferContext.m_ColorOffset[0] = m_FrontBuffer.GetOffset(m_FrontBuffer.m_iActiveBuffer);
		m_DefaultBackbufferContext.m_ColorPitch[0] = m_FrontBuffer.GetPitch();
	}

	if(!m_bUseFBO)
	{
		Width = m_FrontBuffer.m_Width;
		Height = m_FrontBuffer.m_Height;
	}

	m_ZBuffer.Create(Width, (Height + 63) & ~63);
	m_ZBuffer.SetTile(AllocTile(m_ZBuffer.GetOffset(), cellGcmGetTiledPitchSize(m_ZBuffer.GetPitch()), m_ZBuffer.GetSize(), CELL_GCM_COMPMODE_Z32_SEPSTENCIL));
	M_TRACEALWAYS("gcmSetZCull(%d, %.8X, %.3x, %.3x, %x, %x, %x, %x, %x, %x, %x)\n", 0, m_ZBuffer.GetOffset(), Width, (Height + 63) & ~63, 0, CELL_GCM_ZCULL_Z24S8, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_LONES, CELL_GCM_ALWAYS, 0x80, 0xff);
	gcmSetZcull(0, m_ZBuffer.GetOffset(), Width, (Height + 63) & ~63, 0, CELL_GCM_ZCULL_Z24S8, CELL_GCM_SURFACE_CENTER_1, CELL_GCM_ZCULL_LESS, CELL_GCM_ZCULL_LONES, CELL_GCM_ALWAYS, 0x80, 0xff);

	if(m_bUseFBO)
	{
		m_DefaultBackbufferContext.m_DepthOffset = 0;
		m_DefaultBackbufferContext.m_DepthPitch = 0x40;
	}
	else
	{
		m_DefaultBackbufferContext.m_DepthOffset = m_ZBuffer.GetOffset();
		m_DefaultBackbufferContext.m_DepthPitch = m_ZBuffer.GetPitch();
	}
	m_DisplayModeNr = _Mode;

	if(m_bUseFBO)
	{
		uint iTex = m_pTCFBO->AllocTexture(Width, Height, TexFormat);
		int TexID = m_pTCFBO->GetTextureID(iTex);
		CRCPS3GCM::ms_This.m_pTC->SetTextureParam(TexID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_RESIDENT);
		CRCPS3GCM::ms_This.m_ContextTexture.Build(TexID);

		m_FBOBackbufferContext.m_Setup.m_Width = Width;
		m_FBOBackbufferContext.m_Setup.m_Height = Height;
		m_FBOBackbufferContext.m_Setup.m_BackbufferFormat = FBOFormat;
		m_FBOBackbufferContext.m_Setup.m_DepthbufferFormat = CELL_GCM_SURFACE_Z24S8;
		m_FBOBackbufferContext.m_Setup.m_nRT = 1;
		m_FBOBackbufferContext.m_Setup.m_ColorBuffer = CBackbufferContext::CSetup::BUFFER_ATTACHED_AS_TEXTURE;
		m_FBOBackbufferContext.m_Setup.m_DepthBuffer = CBackbufferContext::CSetup::BUFFER_ATTACHED;
		m_FBOBackbufferContext.m_Setup.m_AntiAlias = 0;
		m_FBOBackbufferContext.m_Setup.m_ZCullEnabled = 1;
		m_FBOBackbufferContext.m_ColorOffset[0] = CRCPS3GCM::ms_This.m_ContextTexture.GetOffset(TexID);
		m_FBOBackbufferContext.m_ColorPitch[0] = CRCPS3GCM::ms_This.m_ContextTexture.GetPitch(TexID);
		m_FBOBackbufferContext.m_DepthOffset = m_ZBuffer.GetOffset();
		m_FBOBackbufferContext.m_DepthPitch = m_ZBuffer.GetPitch();

		m_BackbufferImage.CreateVirtual(Width, Height, TexFormat, IMAGE_MEM_VIRTUAL);
		RestoreRenderContext(m_FBOBackbufferContext);
	}
	else
	{
		m_BackbufferImage.CreateVirtual(m_FrontBuffer.m_Width, m_FrontBuffer.m_Height, IMAGE_FORMAT_RGBA8, IMAGE_MEM_VIRTUAL);
		RestoreRenderContext(m_DefaultBackbufferContext);
	}

	m_DeferredWait.SetSignaled();
}

void CDCPS3GCM::MRT_SetRenderTarget(const CRC_RenderTargetDesc& _RenderTarget)
{
	uint bUseMRT = false;
	for(uint i = 0; i < 4; i++)
	{
		if(_RenderTarget.m_lColorTextureID[i])
		{
			bUseMRT = true;
			break;
		}
	}

	if(bUseMRT)
	{
		CBackbufferContext NewContext;
		uint32 TexID[4] = {0};
		uint nTex = 0;

		for(uint i = 0; i < 4; i++)
		{
			if(_RenderTarget.m_lColorTextureID[i])
			{
				CRCPS3GCM::ms_This.m_ContextTexture.Build(_RenderTarget.m_lColorTextureID[i]);
				TexID[nTex++] = _RenderTarget.m_lColorTextureID[i];
			}
		}

		uint Width = CRCPS3GCM::ms_This.m_ContextTexture.GetWidth(TexID[0]);
		uint Height = CRCPS3GCM::ms_This.m_ContextTexture.GetHeight(TexID[0]);
		uint Format = CRCPS3GCM::ms_This.m_ContextTexture.GetFormat(TexID[0]);
#ifndef M_RTM
		for(uint i = 1; i < nTex; i++)
		{
			if(Width != CRCPS3GCM::ms_This.m_ContextTexture.GetWidth(TexID[i]))
				M_BREAKPOINT;
			if(Height != CRCPS3GCM::ms_This.m_ContextTexture.GetHeight(TexID[i]))
				M_BREAKPOINT;
			if(Format != CRCPS3GCM::ms_This.m_ContextTexture.GetFormat(TexID[i]))
				M_BREAKPOINT;
		}
#endif

		uint colorFormat = 0;
		switch(Format)
		{
		case CTextureGCM::TEX_FORMAT_RGBA8:
			{
				colorFormat = CELL_GCM_SURFACE_A8R8G8B8;
				break;
			}
		default:
			M_BREAKPOINT;
		}

		uint colorLocation0 = CELL_GCM_LOCATION_LOCAL;
		uint colorLocation1 = CELL_GCM_LOCATION_LOCAL;
		uint colorLocation2 = CELL_GCM_LOCATION_LOCAL;
		uint colorLocation3 = CELL_GCM_LOCATION_LOCAL;
		uint colorOffset0 = 0;
		uint colorOffset1 = 0;
		uint colorOffset2 = 0;
		uint colorOffset3 = 0;
		uint colorPitch0 = 64;
		uint colorPitch1 = 64;
		uint colorPitch2 = 64;
		uint colorPitch3 = 64;

		colorLocation0	= CRCPS3GCM::ms_This.m_ContextTexture.GetLocation(TexID[0]);
		colorOffset0 	= CRCPS3GCM::ms_This.m_ContextTexture.GetOffset(TexID[0]);
		colorPitch0 	= CRCPS3GCM::ms_This.m_ContextTexture.GetPitch(TexID[0]);
		if(nTex > 1)
		{
			colorLocation1 = CRCPS3GCM::ms_This.m_ContextTexture.GetLocation(TexID[1]);
			colorOffset1 = CRCPS3GCM::ms_This.m_ContextTexture.GetOffset(TexID[1]);
			colorPitch1 = CRCPS3GCM::ms_This.m_ContextTexture.GetPitch(TexID[1]);
			if(nTex > 2)
			{
				colorLocation2 = CRCPS3GCM::ms_This.m_ContextTexture.GetLocation(TexID[2]);
				colorOffset2 = CRCPS3GCM::ms_This.m_ContextTexture.GetOffset(TexID[2]);
				colorPitch2 = CRCPS3GCM::ms_This.m_ContextTexture.GetPitch(TexID[2]);
				if(nTex > 3)
				{
					colorLocation3 = CRCPS3GCM::ms_This.m_ContextTexture.GetLocation(TexID[3]);
					colorOffset3 = CRCPS3GCM::ms_This.m_ContextTexture.GetOffset(TexID[3]);
					colorPitch3 = CRCPS3GCM::ms_This.m_ContextTexture.GetPitch(TexID[3]);
				}
			}
		}

		uint depthFormat = 0;
		uint depthLocation = CELL_GCM_LOCATION_LOCAL;
		uint depthOffset = 0;
		uint depthPitch = 64;
		uint ZCullEnabled = 0;
		if(!(_RenderTarget.m_Options & CRC_RENDEROPTION_NOZBUFFER))
		{
			depthFormat = CELL_GCM_SURFACE_Z24S8;
			depthLocation = CELL_GCM_LOCATION_LOCAL;
			depthOffset = CDCPS3GCM::ms_This.m_ZBuffer.GetOffset();
			depthPitch = CDCPS3GCM::ms_This.m_ZBuffer.GetPitch();
			ZCullEnabled = 1;
		}

		NewContext.m_Setup.m_Width = Width;
		NewContext.m_Setup.m_Height = Height;
		NewContext.m_Setup.m_BackbufferFormat = colorFormat;
		NewContext.m_Setup.m_DepthbufferFormat = depthFormat;
		NewContext.m_Setup.m_nRT = nTex;
		NewContext.m_Setup.m_ColorBuffer = CBackbufferContext::CSetup::BUFFER_ATTACHED_AS_TEXTURE;
		if(_RenderTarget.m_Options & CRC_RENDEROPTION_NOZBUFFER)
			NewContext.m_Setup.m_DepthBuffer = CBackbufferContext::CSetup::BUFFER_UNUSED;
		else
			NewContext.m_Setup.m_DepthBuffer = CBackbufferContext::CSetup::BUFFER_ATTACHED_SHARED;
		NewContext.m_Setup.m_AntiAlias = 0;
		NewContext.m_Setup.m_ZCullEnabled = ZCullEnabled;

		NewContext.m_ColorOffset[0] = colorOffset0;
		NewContext.m_ColorOffset[1] = colorOffset1;
		NewContext.m_ColorOffset[2] = colorOffset2;
		NewContext.m_ColorOffset[3] = colorOffset3;

		NewContext.m_ColorPitch[0] = colorPitch0;
		NewContext.m_ColorPitch[1] = colorPitch1;
		NewContext.m_ColorPitch[2] = colorPitch2;
		NewContext.m_ColorPitch[3] = colorPitch3;

		NewContext.m_DepthOffset = depthOffset;
		NewContext.m_DepthPitch = depthPitch;

		RestoreRenderContext(NewContext);
	}
	else
	{
		if(m_bUseFBO)
			RestoreRenderContext(m_FBOBackbufferContext);
		else
			RestoreRenderContext(m_DefaultBackbufferContext);
	}

	CRCPS3GCM::ms_This.Viewport_Update();
}

// -------------------------------------------------------------------

static void SetZCullParamters(int _Stats, int _Stats1)
{
	int NumTiles = _Stats & 0xffff;
	int MaxSlope = (_Stats >> 16) & 0xffff;
	int AvgSlop = NumTiles?(_Stats1 / NumTiles):0;

	int MoveForward = (AvgSlop + MaxSlope) >> 1;
	int PushBack = MoveForward / 2;

	if(MoveForward < 1)
		MoveForward = 1;

	if(PushBack < 1)
		PushBack = 1;

	gcmSetZcullLimit(MoveForward, PushBack);

}

static CMTime LastFrame;
int CDCPS3GCM::PageFlip()
{
	MSCOPE(CDCPS3GCM::PageFlip, RENDER_PS3);
	CDisplayContext::PageFlip();

	if(m_bUseFBO)
	{
		// Need to resolve
		RestoreRenderContext(m_DefaultBackbufferContext);
		CClipRect clip(0, 0, m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);
		CRct Rect(0, 0, m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);
		CRCPS3GCM::ms_This.Viewport_Get()->SetView(clip, Rect);
		CRCPS3GCM::ms_This.Viewport_Update();

		{
			CRC_Viewport VP = *CRCPS3GCM::ms_This.Viewport_Get();

			CMat4Dfp32 CurTransform;
			{
				CRct View = VP.GetViewRect();
				CClipRect Clip = VP.GetViewClip();
				fp32 w = View.GetWidth();
				fp32 h = View.GetHeight();
				fp32 dx = -w;
				fp32 dy = -h;

				CurTransform.Unit();
				const fp32 Z = VP.GetBackPlane() - 16.0f;

				fp32 xScale = VP.GetXScale() * 0.5f;
				fp32 yScale = VP.GetYScale() * 0.5f;

				CurTransform.k[0][0] = Z / xScale;// * m_CurScale.k[0];
				CurTransform.k[1][1] = Z / yScale;// * m_CurScale.k[1];
				CurTransform.k[3][0] = (Z*((dx)*(1.0f/2.0f)) / xScale);
				CurTransform.k[3][1] = (Z*((dy)*(1.0f/2.0f)) / yScale);
				CurTransform.k[3][2] = Z;
			}

			CVec3Dfp32 Verts3D[4];
			CVec2Dfp32 TVerts[4];
			uint16 DualTriangle[] = {0, 1, 2, 0, 2, 3};

			Verts3D[0] = CVec3Dfp32(0, 0, 0);
			Verts3D[1] = CVec3Dfp32(m_CurrentBackbufferContext.m_Setup.m_Width, 0, 0);
			Verts3D[2] = CVec3Dfp32(m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height, 0);
			Verts3D[3] = CVec3Dfp32(0, m_CurrentBackbufferContext.m_Setup.m_Height, 0);

			CVec3Dfp32::MultiplyMatrix(Verts3D, Verts3D, CurTransform, 4);

			TVerts[0] = CVec2Dfp32(0, 0);
			TVerts[1] = CVec2Dfp32(1, 0);
			TVerts[2] = CVec2Dfp32(1, 1);
			TVerts[3] = CVec2Dfp32(0, 1);

			CRC_Attributes Attrib;
			Attrib.SetDefault();
			Attrib.Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			Attrib.Attrib_TextureID(0, m_pTCFBO->GetTextureID(0));
			Attrib.m_Scissor.SetRect(0, 0, m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);

			CRC_VertexBuffer VB;
			VB.Clear();
			VB.m_pV = Verts3D;
			VB.m_nV = 4;
			VB.m_pTV[0] = TVerts[0].k;
			VB.m_nTVComp[0] = 2;
			VB.m_piPrim = DualTriangle;
			VB.m_nPrim = 2;
			VB.m_PrimType = CRC_RIP_TRIANGLES;

			CRCPS3GCM::ms_This.Geometry_Color(0xffffffff);
			CRCPS3GCM::ms_This.Geometry_VertexBuffer(VB, true);
			CRCPS3GCM::ms_This.Attrib_End(~0);
			CRCPS3GCM::ms_This.Attrib_Set(&Attrib);
			CRCPS3GCM::ms_This.Render_IndexedTriangles(DualTriangle, 2);
		}
	}

	CRCPS3GCM::ms_This.m_ContextVP.Disable();
	CRCPS3GCM::ms_This.m_ContextFP.Disable();

	uint StatsFrame = (CRenderPS3Resource::ms_ResourceFrameIndex - 3) & 3;
	uint ReportFrame = CRenderPS3Resource::ms_ResourceFrameIndex & 3;
	{
		gcmSetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_ZPASS_PIXELS_FRAME0 + ReportFrame);
		gcmSetReport(CELL_GCM_ZCULL_STATS, GPU_REPORT_ZCULL_STATS_FRAME0 + ReportFrame);
		gcmSetReport(CELL_GCM_ZCULL_STATS1, GPU_REPORT_ZCULL_STATS1_FRAME0 + ReportFrame);
		gcmSetReport(CELL_GCM_ZCULL_STATS2, GPU_REPORT_ZCULL_STATS2_FRAME0 + ReportFrame);
		gcmSetReport(CELL_GCM_ZCULL_STATS3, GPU_REPORT_ZCULL_STATS3_FRAME0 + ReportFrame);
	}
	// Flush semaphore command
	gcmFlush();
	// Make sure RSX is done processing before even trying to flip
	gcmSetWriteBackEndLabel(GPU_LABEL_PAGEFLIP, CRenderPS3Resource::ms_ResourceFrameIndex);
	CRenderPS3Resource::ms_ResourceFrameIndex++;

	gcmSetUserCommand(~0);

#ifdef PLATFORM_PS3
	{
		CMTime Tick;
		Tick.Snapshot();
		CMTime RenderDelta = Tick - LastFrame;

		if(1)
		{
			DAssert(gcmSetFlip(m_FrontBuffer.m_iActiveBuffer), ==, 0);
			gcmSetWaitFlip();
			gcmFlush();

//			while(gcmGetFlipStatus() != 0)
//				MRTC_SystemInfo::OS_Sleep(1);
//			gcmResetFlipStatus();
		}

		// Wait for the GPU to finish previous frame
		if(m_FrameCount.Decrease() == 1)
		{
			m_FrameEvent.Wait();
		}

		uint ZPassStats = gcmGetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_ZPASS_PIXELS_FRAME0 + StatsFrame);
		uint ZCullStats = gcmGetReport(CELL_GCM_ZCULL_STATS, GPU_REPORT_ZCULL_STATS_FRAME0 + StatsFrame);
		uint ZCullStats1 = gcmGetReport(CELL_GCM_ZCULL_STATS1, GPU_REPORT_ZCULL_STATS1_FRAME0 + StatsFrame);
		uint ZCullStats2 = gcmGetReport(CELL_GCM_ZCULL_STATS2, GPU_REPORT_ZCULL_STATS2_FRAME0 + StatsFrame);
		uint ZCullStats3 = gcmGetReport(CELL_GCM_ZCULL_STATS3, GPU_REPORT_ZCULL_STATS3_FRAME0 + StatsFrame);

//		M_TRACEALWAYS("Stats: ZPass %10u, ZCull %.8X, %.8X, %.8X, %.8X\n", ZPassStats, ZCullStats, ZCullStats1, ZCullStats2, ZCullStats3);

		CMTime Tick2;
		Tick2.Snapshot();
		CMTime BlockTime = Tick2 - Tick;
		CMTime FrameDelta = Tick2 - LastFrame;

		CRCPS3GCM::ms_This.m_Stats.m_Time_FrameTime.AddData((Tick2 - LastFrame).GetTime());
		CRCPS3GCM::ms_This.m_Stats.m_CPUUsage.AddData(100.0f * RenderDelta.GetTime() / FrameDelta.GetTime());
		CRCPS3GCM::ms_This.m_Stats.m_GPUUsage.AddData(100.0f);
		LastFrame = Tick2;
	}
#endif

	gcmEmulateNewSwitch();

	m_FrontBuffer.PageFlip();
	m_DefaultBackbufferContext.m_ColorOffset[0] = m_FrontBuffer.GetOffset(m_FrontBuffer.m_iActiveBuffer);
	SetZCullParamters(	gcmGetReport(CELL_GCM_ZCULL_STATS, GPU_REPORT_ZCULL_STATS_FRAME0 + StatsFrame),
						gcmGetReport(CELL_GCM_ZCULL_STATS1, GPU_REPORT_ZCULL_STATS1_FRAME0 + StatsFrame));
	gcmSetInvalidateZcull();

	if(m_DeferredMode != ~0)
	{
		if(m_DeferredLoop == 0)
		{
			SetMode(m_DeferredMode);
			m_DeferredMode = ~0;
		}
		else
			m_DeferredLoop--;
	}

	if(m_bUseFBO)
	{
		RestoreRenderContext(m_FBOBackbufferContext);
	}
	else
	{
		RestoreRenderContext(m_DefaultBackbufferContext);
	}
	CRCPS3GCM::ms_This.Viewport_Update();

	{
		CRC_Attributes Attrib;
		Attrib.SetDefault();
		CRCPS3GCM::ms_This.Attrib_End(~0);
		CRCPS3GCM::ms_This.Attrib_Set(&Attrib);
	}

	return 0;
};

// -------------------------------------------------------------------
void CDCPS3GCM::EnumModes()
{
	MSCOPE(CDCPS3GCM::EnumModes, RENDER_PS3);

	m_lspModes.Destroy();

	CellVideoOutState videoState;
	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
	CellVideoOutResolution videoResolution;
	cellVideoOutGetResolution(videoState.displayMode.resolutionId, &videoResolution);

	CDCGCM_VideoMode* pMode;
	TPtr<CDC_VideoMode> spMode;
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(640, 480, IMAGE_FORMAT_RGBA16_F, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1024, 576, IMAGE_FORMAT_RGBA16_F, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1280, 720, IMAGE_FORMAT_RGBA16_F, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1920, 1080, IMAGE_FORMAT_RGBA16_F, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(640, 480, IMAGE_FORMAT_RGBA8, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1024, 576, IMAGE_FORMAT_RGBA8, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1280, 720, IMAGE_FORMAT_RGBA8, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}
	{
		pMode = MNew(CDCGCM_VideoMode);
		spMode = pMode;
		pMode->m_Format.CreateVirtual(1920, 1080, IMAGE_FORMAT_RGBA8, IMAGE_MEM_VIRTUAL);
		pMode->m_ResolutionID = videoState.displayMode.resolutionId;
		m_lspModes.Add(spMode);
	}

	// Create frontbuffer here
	m_FrontBuffer.Create(videoResolution.width, videoResolution.height);
	uint TilePitch = cellGcmGetTiledPitchSize(m_FrontBuffer.GetPitch());
	m_FrontBuffer.SetTile(0, AllocTile(m_FrontBuffer.GetOffset(), TilePitch, CRenderPS3Resource_FrontBuffer_Local::BUFFERSIZE_ALIGNED, CELL_GCM_COMPMODE_DISABLED));
	m_FrontBuffer.SetTile(1, AllocTile(m_FrontBuffer.GetOffset() + CRenderPS3Resource_FrontBuffer_Local::BUFFERSIZE_ALIGNED, TilePitch, CRenderPS3Resource_FrontBuffer_Local::BUFFERSIZE_ALIGNED, CELL_GCM_COMPMODE_DISABLED));


	SetScreenAspect(4.0f/3.0f);
//	pMode = MNew(CDCGCM_VideoMode);
//	spMode = pMode;
//	spMode->m_Format.CreateVirtual(videoResolution.width, videoResolution.height, IMAGE_FORMAT_RGBA16_F, IMAGE_MEM_VIRTUAL);
//	pMode->m_ResolutionID = videoState.displayMode.resolutionId;
//	m_lspModes.Add(spMode);
}

void CDCPS3GCM::InitSettings()
{
	MSCOPE(CDCPS3GCM::InitSettings, RENDER_PS3);
	MACRO_GetSystemEnvironment(pEnv);
}

// -------------------------------------------------------------------
CImage* CDCPS3GCM::GetFrameBuffer()
{
	MSCOPE(CDCPS3GCM::GetFrameBuffer, RENDER_PS3);

	return &m_BackbufferImage;
};

void CDCPS3GCM::ClearFrameBuffer(int _Buffers, int _Color)
{
	MSCOPE(CDCPS3GCM::ClearFrameBuffer, RENDER_PS3);

	if(m_CurrentBackbufferContext.m_Setup.m_BackbufferFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)
	{
		uint32 ClearFlags = 0;
		if(_Buffers & CDC_CLEAR_ZBUFFER)
			ClearFlags |= CELL_GCM_CLEAR_Z;
		if(_Buffers & CDC_CLEAR_STENCIL)
			ClearFlags |= CELL_GCM_CLEAR_S;

		gcmSetClearSurface(ClearFlags);

		// NEed to fake clearing of colorbuffer :/
		if(1)
		{
			CRC_Viewport VP = *CRCPS3GCM::ms_This.Viewport_Get();

			CMat4Dfp32 CurTransform;
			{
				CRct View = VP.GetViewRect();
				CClipRect Clip = VP.GetViewClip();
				fp32 w = View.GetWidth();
				fp32 h = View.GetHeight();
				fp32 dx = -w;
				fp32 dy = -h;

				CurTransform.Unit();
				const fp32 Z = VP.GetBackPlane() - 16.0f;

				fp32 xScale = VP.GetXScale() * 0.5f;
				fp32 yScale = VP.GetYScale() * 0.5f;

				CurTransform.k[0][0] = Z / xScale;// * m_CurScale.k[0];
				CurTransform.k[1][1] = Z / yScale;// * m_CurScale.k[1];
				CurTransform.k[3][0] = (Z*((dx)*(1.0f/2.0f)) / xScale);
				CurTransform.k[3][1] = (Z*((dy)*(1.0f/2.0f)) / yScale);
				CurTransform.k[3][2] = Z;
			}

			CVec3Dfp32 Verts3D[4];
			uint16 DualTriangle[] = {0, 1, 2, 0, 2, 3};

			Verts3D[0] = CVec3Dfp32(0, 0, 0);
			Verts3D[1] = CVec3Dfp32(m_CurrentBackbufferContext.m_Setup.m_Width, 0, 0);
			Verts3D[2] = CVec3Dfp32(m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height, 0);
			Verts3D[3] = CVec3Dfp32(0, m_CurrentBackbufferContext.m_Setup.m_Height, 0);

			CVec3Dfp32::MultiplyMatrix(Verts3D, Verts3D, CurTransform, 4);

			CRC_Attributes Attrib;
			Attrib.SetDefault();
			Attrib.Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);

			CRC_VertexBuffer VB;
			VB.Clear();
			VB.m_pV = Verts3D;
			VB.m_nV = 4;
			VB.m_piPrim = DualTriangle;
			VB.m_nPrim = 2;
			VB.m_PrimType = CRC_RIP_TRIANGLES;

			CRCPS3GCM::ms_This.Geometry_Color(_Color);
			CRCPS3GCM::ms_This.Geometry_VertexBuffer(VB, true);
			CRCPS3GCM::ms_This.Attrib_End(~0);
			CRCPS3GCM::ms_This.Attrib_Set(&Attrib);
			CRCPS3GCM::ms_This.Render_IndexedTriangles(DualTriangle, 2);

			Attrib.SetDefault();
			CRCPS3GCM::ms_This.Attrib_End(~0);
			CRCPS3GCM::ms_This.Attrib_Set(&Attrib);
		}
	}
	else
	{
		uint32 ClearFlags = 0;
		if(_Buffers & CDC_CLEAR_COLOR)
			ClearFlags |= CELL_GCM_CLEAR_RGBA;
		if(_Buffers & CDC_CLEAR_ZBUFFER)
			ClearFlags |= CELL_GCM_CLEAR_Z;
		if(_Buffers & CDC_CLEAR_STENCIL)
			ClearFlags |= CELL_GCM_CLEAR_S;

		// Should set MRT mask here so we only clear the selected buffers
		gcmSetClearColor(_Color);
		gcmSetClearSurface(ClearFlags);
	}

	if((_Buffers & CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL) && m_CurrentBackbufferContext.m_Setup.m_ZCullEnabled)
		gcmSetInvalidateZcull();
};

CRenderContext* CDCPS3GCM::GetRenderContext(CRCLock* _pLock)
{
	MSCOPE(CDCPS3GCM::GetRenderContext, RENDER_PS3);
	return &CRCPS3GCM::ms_This;
};

uint M_NOINLINE CDCPS3GCM::AllocTile(uint _Offset, uint _TilePitch, uint _Size, uint _CompMode)
{
	uint iTile = 0;
	for(; iTile < 15; iTile++)
	{
		if(m_lTiles[iTile].m_iPitch == 0)
			break;
	}

	M_ASSERT((_Offset & 0xffff) == 0, "Offset not aligned");
	M_ASSERT((_Size & 0xffff) == 0, "Size not aligned");

	M_ASSERT(iTile < 15, "!");
	gcmFinish();
	gcmSetTile(iTile, CELL_GCM_LOCATION_LOCAL, _Offset, _Size, _TilePitch, _CompMode, 0, iTile & 3);

	M_TRACEALWAYS("Allocaing tile %d: Addr 0x%.8X, Pitch %d, Size %d\n", iTile, _Offset, _TilePitch, _Size);

	m_lTiles[iTile].m_iOffset = _Offset;
	m_lTiles[iTile].m_iPitch = _TilePitch;
	m_lTiles[iTile].m_iSize = _Size;

	return iTile;
}

void CDCPS3GCM::FreeTile(uint _iTile)
{
	M_ASSERT(_iTile < 15, "!");
	M_ASSERT(m_lTiles[_iTile].m_iPitch && m_lTiles[_iTile].m_iSize, "!");

	gcmFinish();
	gcmSetInvalidateTile(_iTile);

	m_lTiles[_iTile].m_iOffset = 0;
	m_lTiles[_iTile].m_iPitch = 0;
	m_lTiles[_iTile].m_iSize = 0;
}

void CDCPS3GCM::PostDefrag()
{
	// None of these 2 should really be moved by defrag since they're large chunks without any fragmentation
	m_FrontBuffer.PostDefrag();
	m_ZBuffer.PostDefrag();
}

// --------------------------------
//  CConsoleClient virtuals.
// --------------------------------

void CDCPS3GCM::Register(CScriptRegisterContext & _RegContext)
{
	CDisplayContext::Register(_RegContext);
};

// -------------------------------------------------------------------

void CDCPS3GCM::OnRefresh(int _Context)
{
}

void CDCPS3GCM::OnBusy(int _Context)
{
}

};	// namespace NRenderPS3GCM

// -------------------------------------------------------------------
using namespace NRenderPS3GCM;
MRTC_IMPLEMENT_DYNAMIC(CDCPS3GCM, CDisplayContext);

CDisplayContext* gf_CreateDisplayContextPS3GCMStatic()
{
	NRenderPS3GCM::CDCPS3GCM::ms_This.m_pMainThread = MRTC_SystemInfo::OS_GetThreadID();
	ms_pRCOwner = NRenderPS3GCM::CDCPS3GCM::ms_This.m_pMainThread;

	return &NRenderPS3GCM::CDCPS3GCM::ms_This;
}

