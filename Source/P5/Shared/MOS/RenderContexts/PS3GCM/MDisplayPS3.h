/*
------------------------------------------------------------------------------------------------
Name:		MDispGL.cpp/h
Purpose:	Display context
Creation:	9703??

Contents:
class				CDisplayContextGL				9703??  -				CDisplayContext for OpenGL

------------------------------------------------------------------------------------------------
*/
#ifndef _INC_MOS_DispGL
#define _INC_MOS_DispGL

#include "../../MSystem/MSystem.h"
#include "MRenderPS3_Image.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Heap.h"
#include "MRenderPS3_Resource.h"

namespace NRenderPS3GCM
{


// -------------------------------------------------------------------
//  CDisplayContextGL
// -------------------------------------------------------------------

class CDCGCM_VideoMode : public CDC_VideoMode
{
public:
	uint32 m_ResolutionID;
};

// -------------------------------------------------------------------

template <int bOK>
class CCompileError
{
public:
	uint8 m_Error[1 - 2 * (bOK == 0)];
};

template <int nA, int nB>
class CCompileErrorLE : public CCompileError<(nA <= nB)>
{
};

template <int nA, int nB>
class CCompileErrorLT : public CCompileError<(nA < nB)>
{
};

template <int nA, int nB>
class CCompileErrorGE : public CCompileError<(nA >= nB)>
{
};

template <int nA, int nB>
class CCompileErrorGT : public CCompileError<(nA > nB)>
{
};

template <int nA, int nB>
class CCompileErrorEQ : public CCompileError<(nA == nB)>
{
};

template <int nA, int nB>
class CCompileErrorNE : public CCompileError<(nA != nB)>
{
};

// -------------------------------------------------------------------
class CRCPS3GCM;
class CDCPS3GCM : public CDisplayContext
{
	friend class CRCPS3GCM;
	friend class CContext_Texture;
	friend class CContext_Geometry;
	MRTC_DECLARE;

public:
	static CDCPS3GCM ms_This;
	MRTC_CriticalSection m_Lock;
	void* m_pMainThread;

	M_FORCEINLINE void Lock()
	{
		m_Lock.Lock();
	}
	M_FORCEINLINE void Unlock()
	{
		m_Lock.Unlock();
	}
protected:
	class CBackbufferContext
	{
	public:
		class CSetup
		{
		public:
			enum
			{
				BITCOUNT_WIDTH = 12,
				BITCOUNT_HEIGHT = 12,
				BITCOUNT_RT = 3,
				BITCOUNT_BACKBUFFERFORMAT = 5,
				BITCOUNT_DEPTHBUFFERFORMAT = 2,
				BITCOUNT_COLORBUFFER = 2,
				BITCOUNT_DEPTHBUFFER = 2,
				BITCOUNT_ANTIALIAS = 1,			// Is antialias enabled? (May have to use more than 1 bit here since there are 4 modes)
				BITCOUNT_ZCULL = 1,
				BITCOUNT_PADDING = (64 - BITCOUNT_WIDTH - BITCOUNT_HEIGHT - BITCOUNT_RT - BITCOUNT_BACKBUFFERFORMAT - BITCOUNT_DEPTHBUFFERFORMAT - BITCOUNT_COLORBUFFER - BITCOUNT_DEPTHBUFFER - BITCOUNT_ANTIALIAS - BITCOUNT_ZCULL),

				BUFFER_UNUSED	= 0,			// Buffer target is unused
				BUFFER_ATTACHED,			// Buffer target is used
				BUFFER_ATTACHED_AS_TEXTURE,	// Buffer target is used and available as a texture
				BUFFER_ATTACHED_SHARED,		// Buffer target is used and shared so don't delete it
				BUFFER_MAX,
			};

			CSetup()
			{
				// Template to give compilationerrors when enums grow beyond size of array
				m_SingleValue = 0;
			}
			union
			{
				struct
				{
					uint64	m_Width:BITCOUNT_WIDTH;							// Max 4096-1
					uint64	m_Height:BITCOUNT_HEIGHT;						// Max 4096-1
					uint64	m_BackbufferFormat:BITCOUNT_BACKBUFFERFORMAT;
					uint64	m_DepthbufferFormat:BITCOUNT_DEPTHBUFFERFORMAT;
					uint64	m_nRT:BITCOUNT_RT;							// MRT enabled
					uint64	m_ColorBuffer:BITCOUNT_COLORBUFFER;				// Backbuffer has a color target attached
					uint64	m_DepthBuffer:BITCOUNT_DEPTHBUFFER;				// Depthbuffer attached
					uint64	m_AntiAlias:BITCOUNT_ANTIALIAS;
					uint64	m_ZCullEnabled:BITCOUNT_ZCULL;
					uint64	m_Padding:BITCOUNT_PADDING;
				};
				uint64	m_SingleValue;
			};
		};
		CBackbufferContext()
		{
		}

		CSetup m_Setup;
		uint32 m_ColorOffset[4];
		uint32 m_ColorPitch[4];
		uint32 m_DepthOffset;
		uint32 m_DepthPitch;
	};

	// Frontbuffer is always 1920x1080 in size
	class CRenderPS3Resource_FrontBuffer_Local : public CRenderPS3Resource_Local<GPU_TILEDFRAMEBUFFER_ALIGNMENT>
	{
	public:

		enum
		{
			BUFFERSIZE = 1920 * 1080 * 4,
			BUFFERSIZE_ALIGNED = (BUFFERSIZE + 65535) & ~65535
		};
		CRenderPS3Resource_Local_BlockInfo m_BlockInfo;
		uint32 m_iActiveBuffer;
		uint16 m_Width, m_Height;
		uint32 m_Pitch;
		uint32 m_lTiles[2];

		CRenderPS3Resource_FrontBuffer_Local() {memset(this, 0, sizeof(*this));}

		void Create(uint16 _Width, uint16 _Height)
		{
			m_Width = _Width;
			m_Height = _Height;
			Alloc(BUFFERSIZE_ALIGNED * 2, tAlignment, &m_BlockInfo, false);
			m_Pitch = _Width * 4;
//			Bind();
		}

		uint32 GetOffset(uint _iBuffer) const
		{
			return (m_LocalOffset + _iBuffer * BUFFERSIZE_ALIGNED);
		}
		uint32 GetPitch() const
		{
			return m_Pitch;
		}
		void PageFlip()
		{
			m_iActiveBuffer ^= 1;
		}

		bool IsLocal()
		{
			return true;
		}

		uint32 GetOffset() const
		{
			// Offset must be multiple of m_Pitch*8
			return m_LocalOffset + m_iActiveBuffer * BUFFERSIZE_ALIGNED;
		}

		void SetTile(uint _iS, uint _iTile)
		{
			// Change to tiled pitch
			m_lTiles[_iS] = _iTile;
			m_Pitch = cellGcmGetTiledPitchSize(m_Width * 4);
		}

		void Bind()
		{
			m_Pitch = cellGcmGetTiledPitchSize(m_Width * 4);
			gcmSetTile(0, CELL_GCM_LOCATION_LOCAL, m_LocalOffset, BUFFERSIZE_ALIGNED, m_Pitch, CELL_GCM_COMPMODE_DISABLED, 0, 0);
			gcmSetTile(1, CELL_GCM_LOCATION_LOCAL, m_LocalOffset + BUFFERSIZE_ALIGNED, BUFFERSIZE_ALIGNED, m_Pitch, CELL_GCM_COMPMODE_DISABLED, 0, 0);
		}
	};

	class CRenderPS3Resource_ZBuffer_Local : public CRenderPS3Resource_Local<GPU_TILEDFRAMEBUFFER_ALIGNMENT>
	{
	protected:
		class CRenderPS3Resource_ZBuffer_Local_BlockInfo : public CRenderPS3Resource_Local<GPU_TILEDFRAMEBUFFER_ALIGNMENT>::CRenderPS3Resource_Local_BlockInfo
		{
		public:
			uint32 GetRealSize() {return m_Size;}
			uint32 m_Size;
		};

	public:
		CRenderPS3Resource_ZBuffer_Local()
		{
			m_Width = 0;
			m_Height = 0;
		}

		uint16 m_Width, m_Height;
		uint32 m_Pitch;
		uint32 m_iTile;
		CRenderPS3Resource_ZBuffer_Local_BlockInfo m_BlockInfo;

		void Create(uint32 _Width, uint32 _Height)
		{
			m_Width = _Width;
			m_Height = _Height;
			m_BlockInfo.m_Size = _Width * _Height * 4;
			uint Size = (m_BlockInfo.m_Size + 0xffff) & ~0xffff;
			Alloc(Size, GPU_TILEDTEXTURE_ALIGNMENT, &m_BlockInfo, false);
			m_Pitch = _Width * 4;
//			Bind();
		}

		void Destroy()
		{
			CRenderPS3Resource_Local<GPU_TILEDFRAMEBUFFER_ALIGNMENT>::Destroy();
		}

		void SetTile(uint _iTile)
		{
			m_iTile = _iTile;
			m_Pitch = cellGcmGetTiledPitchSize(m_Pitch);
		}

		uint32 GetPitch() const {return m_Pitch;}
		uint32 GetSize() const {return (m_BlockInfo.m_Size + 0xffff) & ~0xffff;}

		void Bind()
		{
			m_Pitch = cellGcmGetTiledPitchSize(m_Width * 4);
			gcmSetTile(2, CELL_GCM_LOCATION_LOCAL, m_LocalOffset, (m_BlockInfo.GetRealSize() + 65535) & ~65535, m_Pitch, CELL_GCM_COMPMODE_Z32_SEPSTENCIL, 0, 2);
		}
	};

	CRenderPS3Resource_FrontBuffer_Local m_FrontBuffer;
	CRenderPS3Resource_ZBuffer_Local m_ZBuffer;

	// -------------------------------------------------------------------

	CPnt GetScreenSize()
	{
//		return CPnt(m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);
		return CPnt(m_BackbufferImage.GetWidth(), m_BackbufferImage.GetHeight());
	}
	CPnt GetMaxWindowSize()
	{
		return CPnt(m_BackbufferImage.GetWidth(), m_BackbufferImage.GetHeight());
	}

	uint32 m_bLog:1;
	uint32 m_bVSync:1;
	uint32 m_bAntialias:1;
	uint32 m_bAddedToConsole:1;
	uint32 m_bPendingResetMode:1;
	uint32 m_BackBufferFormat;
	uint32 m_DeferredMode;
	uint32 m_DeferredLoop;
	NThread::CEvent m_DeferredWait;

	CBackbufferContext m_CurrentBackbufferContext;
	CBackbufferContext m_DefaultBackbufferContext;
	CBackbufferContext m_FBOBackbufferContext;
	CImagePS3 m_BackbufferImage;

	void RestoreRenderContext(const CBackbufferContext& _Context);
	void MRT_SetRenderTarget(const CRC_RenderTargetDesc& _RenderTarget);
	void ResetMode();

	class CRenderPS3_Tile
	{
	public:
		CRenderPS3_Tile()
		{
			m_iOffset = 0;
			m_iPitch = 0;
		}
		uint32 m_iOffset;
		uint32 m_iPitch;
		uint32 m_iSize;
	};

	// First tile should always be frontbuffer and thus is never available to renderer
	CRenderPS3_Tile m_lTiles[15];
public:
	uint AllocTile(uint _Offset, uint _TilePitch, uint _Size, uint _CompMode);
	void FreeTile(uint _iTile);

	// DisplayContext overrides
	CDCPS3GCM();
	~CDCPS3GCM();
	virtual void Create();
	virtual bool SetOwner(void* _pNewOwner);

	virtual void SetMode(int nr);
	virtual int PageFlip();

	virtual CImage* GetFrameBuffer();
	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0);

	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock);

	// Scriptstuff
	void Register(CScriptRegisterContext &_RegContext);

	// CSubSystems overrides:
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);

	void EnumModes();
	void InitSettings();

	int SpawnWindow(int _Flags){return 0;}
	void DeleteWindow(int _iWnd){}
	void SelectWindow(int _iWnd){}
	void SetWindowPosition(int _iWnd, CRct _Rct){}
	void SetPalette(spCImagePalette _spPal){}
	void ModeList_Init(){}

	void PostDefrag();

#ifdef PLATFORM_WIN_PC
	TThinArrayAlign<uint8, 64 * 1024> m_lLocalHeapEmu;
	bool m_bLogUsage;
	int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0){return 0;}
	int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0){return 0;}
	void Win32_ProcessMessages(){}
	void* Win32_GethWnd(int _iWnd = 0){return 0;}
#endif
};

};

#endif // _INC_MOS_DispGL
