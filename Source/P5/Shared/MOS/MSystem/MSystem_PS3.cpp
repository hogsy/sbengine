
#include "PCH.h"

#ifdef PLATFORM_PS3

#include "MSystem_Core.h"

#include "Raster/MRCCore.h"

class CDisplayContextNULL : public CDisplayContext
{
protected:
	MRTC_DECLARE;
public:

	CImage m_Image;

	CDisplayContextNULL()
	{
		m_Image.Create(640, 480, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	}

	~CDisplayContextNULL()
	{
	}

	virtual CPnt GetScreenSize(){return CPnt(640, 480);};

	virtual void Create()
	{
		CDisplayContext::Create();
	}

	virtual void SetMode(int nr)
	{
	}

	virtual void ModeList_Init()
	{
	}

	virtual int SpawnWindow(int _Flags = 0)
	{
		return 0;
	}

	virtual void DeleteWindow(int _iWnd)
	{
	}

	virtual void SelectWindow(int _iWnd)
	{
	}

	virtual void SetWindowPosition(int _iWnd, CRct _Rct)
	{
	}

	virtual void SetPalette(spCImagePalette _spPal)
	{
	}

	virtual CImage* GetFrameBuffer()
	{
		return &m_Image;
	}

	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0)
	{
	}

	int GetMode()
	{
		return 0;
	}


	class CRenderContextNULL : public CRC_Core
	{
	public:

		CDisplayContextNULL *m_pDisplayContext;
		

		void Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol = NULL, const CVec4Dfp4* _pSpec = NULL, /*const fp4* _pFog = NULL,*/
									const CVec4Dfp4* _pTV0 = NULL, const CVec4Dfp4* _pTV1 = NULL, const CVec4Dfp4* _pTV2 = NULL, const CVec4Dfp4* _pTV3 = NULL, int _Color = 0xffffffff)
		{
		}

		CRenderContextNULL()
		{
		}

		~CRenderContextNULL()
		{
		}

		void Create(CObj* _pContext, const char* _pParams)
		{
			CRC_Core::Create(_pContext, _pParams);

			MACRO_GetSystem;

			m_Caps_TextureFormats = -1;
			m_Caps_DisplayFormats = -1;
			m_Caps_ZFormats = -1;
			m_Caps_StencilDepth = 8;
			m_Caps_AlphaDepth = 8;
			m_Caps_Flags = -1;

			m_Caps_nMultiTexture = 16;
			m_Caps_nMultiTextureCoords = 8;
			m_Caps_nMultiTextureEnv = 8;

		}

		const char* GetRenderingStatus() { return ""; }
		virtual void Flip_SetInterval(int _nFrames){};

		void Attrib_Set(CRC_Attributes* _pAttrib){}
		void Attrib_SetAbsolute(CRC_Attributes* _pAttrib){}
		void Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix){}

		virtual void Texture_PrecacheFlush(){}
		virtual void Texture_PrecacheBegin( int _Count ){}
		virtual void Texture_PrecacheEnd(){}
		virtual void Texture_Precache(int _TextureID){}

		virtual int Texture_GetBackBufferTextureID() {return 0;}
		virtual int Texture_GetFrontBufferTextureID() {return 0;}
		virtual int Texture_GetZBufferTextureID() {return 0;}
		virtual int Geometry_GetVBSize(int _VBID) {return 0;}

		void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles){}
		void Render_IndexedTriangleStrip(uint16* _pIndices, int _Len){}
		void Render_IndexedWires(uint16* _pIndices, int _Len){}
		void Render_IndexedPolygon(uint16* _pIndices, int _Len){}
		void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen){}
		void Render_VertexBuffer(int _VBID){}
		void Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color){}
		void Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}
		void Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}

		virtual void Geometry_PrecacheFlush(){}
		virtual void Geometry_PrecacheBegin( int _Count ){}
		virtual void Geometry_PrecacheEnd(){}
		virtual void Geometry_Precache(int _VBID){}
		virtual CDisplayContext* GetDC(){ return m_pDisplayContext;}

		void Register(CScriptRegisterContext & _RegContext){}

	};

	TPtr<CRenderContextNULL> m_spRenderContext;


	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock)
	{
		if (!m_spRenderContext)
		{
			m_spRenderContext = MNew(CRenderContextNULL);
			m_spRenderContext->m_pDisplayContext = this;
			m_spRenderContext->Create(this, "");

		}

		return m_spRenderContext;
	}

	virtual int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0)
	{
		return 0;
	}

	virtual int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0)
	{
		return 0;
	}

	virtual void Win32_ProcessMessages()
	{
	}

	virtual void* Win32_GethWnd(int _iWnd = 0)
	{
		return 0;
	}

	virtual void Parser_Modes()
	{
	}

	virtual void Register(CScriptRegisterContext & _RegContext)
	{
		CDisplayContext::Register(_RegContext);
	}

};

MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(CDisplayContextNULL, CDisplayContext);

class CSystemPS3 : public CSystemCore
{
	MRTC_DECLARE;

public:

	virtual void Create(void* this_inst, void* prev_inst, char* cmdline, int _cmdshow, const char* _pAppClassName)
	{
		CSystemCore::Create(cmdline, _pAppClassName);
	}


	virtual void DC_InitList()
	{
//		m_lspDC.Add(MNew4(CDisplayContextDesc, "", "PS3", "PS3 DisplayContext", "CDisplayContextPS3") );
		m_lspDC.Add(MNew4(CDisplayContextDesc, "", "NULL", "NULL DisplayContext", "CDisplayContextNULL") );
	}

	virtual void Render(CDisplayContext* _pDC, int _Context)
	{
		MSCOPE(CSystemWin32::Render, SYSTEMWIN32);
		
		if (IsRendering()) return; // Error("Render", "Render-recusion.");
		if (!m_spApp) return;
		if (!_pDC || (_pDC == m_spDisplay))
		{
			_pDC = m_spDisplay;
			if (!_pDC) return;

			if (m_iMainWindow < 0)
			{
				m_iMainWindow = m_spDisplay->SpawnWindow();

				if(m_iMainWindow >= 0)
				{
				}
			}
		}
		if (!_pDC) return;
		if (_pDC->GetMode() < 0) return;

		// Return false if displaycontext isn't valid (not created yet, re-creating, etc.)
		if(!_pDC->IsValid())
			return;

		m_bIsRendering = true;
		M_TRY
		{
			CImage *spImg = _pDC->GetFrameBuffer();
			if (spImg != NULL)
			{
				CClipRect Clip = spImg->GetClipRect();

				m_spApp->OnRender(_pDC, spImg, Clip, _Context);
			};
		}
		M_CATCH(
		catch(CCException)
		{
			m_bIsRendering = false;
			throw;
		};
		)
		m_bIsRendering = false;
	}

	CStr GetModuleName(CStr _Name, bool _bSystemPath)
	{
		return _Name + ".dll";
	}

};


MRTC_IMPLEMENT_DYNAMIC(CSystemPS3, CSystemCore);

#endif
