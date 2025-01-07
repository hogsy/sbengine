#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

#define IDINFO_CUBEMAP DBit(1)
#define IDINFO_BUILT DBit(2)

class CTextureGL
{
public:
	uint16 m_Width;
	uint16 m_Height;

	void Build(uint16 _Width, uint16 _Height)
	{
		m_Width	= _Width;
		m_Height	= _Height;
	}

	bool NeedBuild(uint16 _Width, uint16 _Height) const
	{
		return ((m_Width != _Width) || (m_Height != _Height));
	}

	void Clear()
	{
		m_Width	= 0;
		m_Height	= 0;
	}
};

TThinArray<CTextureGL> lTextures;

// -------------------------------------------------------------------
void CRenderContextPS3::GL_InitTextureBuildBuffers(int _MaxTextureSize, int _DebugResponsibleTextureID)
{
}

void CRenderContextPS3::GL_InitTextures()
{
	int nTxt = m_pTC->GetIDCapacity();
	lTextures.SetLen(nTxt);
	TAP_RCD<CTextureGL> plTxt(lTextures);
	for(int i = 0; i < nTxt; i++)
		plTxt[i].Clear();
}

void CRenderContextPS3::GL_DeleteTextures()
{
	int nTxt = m_pTC->GetIDCapacity();
	for(int i = 0; i < nTxt; i++)
		GL_UnloadTexture(i);
}

// -------------------------------------------------------------------
static int GLGetMipMapLevels(int _w, int _h)
{
//	if (!IsPow2(_w)) Error_static("CTextureContext::GetMipMapLevels", "Not 2^x width.");
//	if (!IsPow2(_h)) Error_static("CTextureContext::GetMipMapLevels", "Not 2^x height.");
	int l2w = Log2(_w);
	int l2h = Log2(_h);
	return Max(l2w, l2h) + 1;
};


// -------------------------------------------------------------------
static bool IsPow2(const CImage& _Img)
{
	int w = Log2(_Img.GetWidth());
	int h = Log2(_Img.GetHeight());

	return ((_Img.GetWidth() == (1 << w)) && (_Img.GetHeight() == (1 << h)));
}

static int NextPow2(int _Value)
{
	int i = 1;
	while(i < _Value)
		i	<<= 1;

	return i;
}

bool CRenderContextPS3::LoadTexture(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, uint8 _TextureVersion, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip)
{
	for(int iMip = 0; iMip < _nMip; iMip++)
	{
		CImage* pTexture = _pTC->GetTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
		int MemModel = pTexture->GetMemModel();

		if(MemModel & IMAGE_MEM_COMPRESSED)
		{
			int w = pTexture->GetWidth();
			int h = pTexture->GetHeight();
			if(!IsPow2(w) || !IsPow2(h))
			{
				_iFirstMip++;
				iMip--;
				_nMip--;
				_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
				continue;
			}
			if(MemModel & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				uint8* pLocked = (uint8*)pTexture->LockCompressed();
				CImage_CompressHeader_S3TC *pHeader = (CImage_CompressHeader_S3TC *)pLocked;
				uint8* pData = pLocked + pHeader->getOffsetData();

				int w = pTexture->GetWidth();
				int h = pTexture->GetHeight();

				int CompressType = pHeader->getCompressType();
				int GLFmt = -1;
				switch(CompressType)
				{
				case IMAGE_COMPRESSTYPE_S3TC_DXT1 : GLFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT3 : GLFmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT5 : GLFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
				default : Error("CRenderContextPS3::LoadTexture", CStrF("Unsupported S3TC compression format: %d", pHeader->getCompressType())); break;
				}
				glCompressedTexImage2D(_GLImageTarget, iMip, GLFmt, w, h, 0, pHeader->getSizeData(), pData);
				GLErr(CStrF("CRenderContextPS3::LoadTexture (glnTexImage2D S3TC %dx%d, %d)", w, h, iMip));
				
				pTexture->Unlock();
			}
			else
				Error("CRenderContextPS3::LoadTexture", "Unsupported compression.");
		}
		else
		{
			uint8* pData = (uint8*)pTexture->Lock();
			int Fmt = pTexture->GetFormat();
			int InternalFormat = -1;
			int GLFmt = -1;
			int Size = -1;
			switch(Fmt)
			{
			case IMAGE_FORMAT_BGR5: InternalFormat = GL_RGB5; GLFmt = GL_BGR; Size = GL_UNSIGNED_SHORT; break;
			case IMAGE_FORMAT_BGR8: InternalFormat = GL_RGB8; GLFmt = GL_BGR; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_BGRA8: InternalFormat = GL_ARGB_SCE; GLFmt = GL_BGRA; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_BGRX8: InternalFormat = GL_ARGB_SCE; GLFmt = GL_BGRA; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_RGBA8: InternalFormat = GL_RGBA8; GLFmt = GL_RGBA; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_A8: InternalFormat = GL_ALPHA8; GLFmt = GL_ALPHA; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_I8A8: InternalFormat = GL_LUMINANCE8_ALPHA8; GLFmt = GL_LUMINANCE_ALPHA; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_I8: InternalFormat = GL_LUMINANCE8; GLFmt = GL_LUMINANCE; Size = GL_UNSIGNED_BYTE; break;
			case IMAGE_FORMAT_RGB16: InternalFormat = GL_RGB16; GLFmt = GL_BGR; Size = GL_UNSIGNED_SHORT; break;
			case IMAGE_FORMAT_RGBA16: InternalFormat = GL_RGBA16; GLFmt = GL_BGRA; Size = GL_UNSIGNED_SHORT; break;
			case IMAGE_FORMAT_RGBA16_F: InternalFormat = GL_RGBA16F_ARB; GLFmt = GL_BGRA; Size = GL_UNSIGNED_SHORT; break;
			case IMAGE_FORMAT_RGB32_F: InternalFormat = GL_RGB32F_ARB; GLFmt = GL_BGR; Size = GL_FLOAT; break;
			case IMAGE_FORMAT_RGBA32_F: InternalFormat = GL_RGBA32F_ARB; GLFmt = GL_BGRA; Size = GL_FLOAT; break;
			case IMAGE_FORMAT_BGRA4: InternalFormat = GL_RGBA4; GLFmt = GL_RGBA; Size = GL_UNSIGNED_BYTE; break;
			default: Error("CRenderContextPS3::LoadTexture", CStrF("Unsupported texture format: %d", Fmt)); break;
			}
			int w = pTexture->GetWidth();
			int h = pTexture->GetHeight();
			glTexImage2D(_GLImageTarget, iMip, InternalFormat, w, h, 0, GLFmt, Size, pData);
			GLErr(CStrF("CRenderContextPS3::LoadTexture (glnTexImage2D %dx%d, %d)", w, h, iMip));
			
			pTexture->Unlock();
		}
		_pTC->ReleaseTexture(_iLocal, _iFirstMip + iMip, _TextureVersion);
	}

	lTextures[_TextureID].Build(_Width, _Height);
	return true;
}

bool CRenderContextPS3::LoadTextureBuildInto(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip)
{
	lTextures[_TextureID].Clear();
	return false;
}

bool CRenderContextPS3::LoadTextureRender(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip)
{
	int Fmt = _TxtDesc.GetFormat();
	int InternalFormat = -1;
	int GLFmt = -1;
	int Size = -1;
	switch(Fmt)
	{
	case IMAGE_FORMAT_ZBUFFER: InternalFormat = GL_DEPTH_COMPONENT24; GLFmt = GL_DEPTH_COMPONENT; Size = GL_UNSIGNED_SHORT; break;
	case IMAGE_FORMAT_BGR8: InternalFormat = GL_RGB8; GLFmt = GL_BGR; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_BGRA8: InternalFormat = GL_ARGB_SCE; GLFmt = GL_BGRA; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_BGRX8: InternalFormat = GL_ARGB_SCE; GLFmt = GL_BGRA; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_RGBA8: InternalFormat = GL_RGBA8; GLFmt = GL_RGBA; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_A8: InternalFormat = GL_ALPHA8; GLFmt = GL_ALPHA; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_I8A8: InternalFormat = GL_LUMINANCE8_ALPHA8; GLFmt = GL_LUMINANCE_ALPHA; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_I8: InternalFormat = GL_LUMINANCE8; GLFmt = GL_LUMINANCE; Size = GL_UNSIGNED_BYTE; break;
	case IMAGE_FORMAT_RGB16: InternalFormat = GL_RGB16; GLFmt = GL_BGR; Size = GL_UNSIGNED_SHORT; break;
	case IMAGE_FORMAT_RGBA16: InternalFormat = GL_RGBA16; GLFmt = GL_BGRA; Size = GL_UNSIGNED_SHORT; break;
	case IMAGE_FORMAT_RGBA16_F: InternalFormat = GL_RGBA16F_ARB; GLFmt = GL_BGRA; Size = GL_UNSIGNED_SHORT; break;
	case IMAGE_FORMAT_RGB32_F: InternalFormat = GL_RGB32F_ARB; GLFmt = GL_BGR; Size = GL_FLOAT; break;
	case IMAGE_FORMAT_RGBA32_F: InternalFormat = GL_RGBA32F_ARB; GLFmt = GL_BGRA; Size = GL_FLOAT; break;
	case IMAGE_FORMAT_BGRA4: InternalFormat = GL_RGBA4; GLFmt = GL_RGBA; Size = GL_UNSIGNED_BYTE; break;

	default: InternalFormat = GL_ARGB_SCE; GLFmt = GL_BGRA; Size = GL_UNSIGNED_BYTE; break;
//	default: Error("CRenderContextPS3::LoadTextureRender", CStrF("Unsupported texture format: %d", Fmt)); break;
	}

	int w = _Width;
	int h = _Height;
	int ww = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Width;
	int wh = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;

	if(_TxtDesc.GetFormat() == IMAGE_FORMAT_ZBUFFER)
	{
		glTexParameteri(_GLImageTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
/*
	else
	{
		glTexParameteri(_GLImageTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(_GLImageTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
*/

//	if(!(m_spTCIDInfo->m_pTCIDInfo[_TextureID].m_Fresh & IDINFO_BUILT))
	if(lTextures[_TextureID].NeedBuild(w, h))
	{
		if(_GLImageTarget == GL_TEXTURE_CUBE_MAP_POSITIVE_X)
		{
			// Generate all sides of cubemap... fucking hack :(
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
			lTextures[_TextureID + 1].Build(w, h);
			lTextures[_TextureID + 2].Build(w, h);
			lTextures[_TextureID + 3].Build(w, h);
			lTextures[_TextureID + 4].Build(w, h);
			lTextures[_TextureID + 5].Build(w, h);
		}
		else
			glTexImage2D(_GLImageTarget, 0, InternalFormat, w, h, 0, GLFmt, Size, NULL);
	}
//	glCopyTexSubImage2D(_GLImageTarget, 0, 0, h - Min(wh, h), 0, wh - Min(wh, h), Min(ww, w), Min(wh, h));
	glCopyTexImage2D(_GLImageTarget, 0, InternalFormat, 0, h - Min(wh, h), Min(ww, w), Min(wh, h), 0);

	lTextures[_TextureID].Build(w, h);
	return true;
}


int BreakTexture = -1;

void CRenderContextPS3::GL_BuildTexture2D(int _RCID, int _TextureID, int _GLObjectTarget, int _GLImageTarget, const CTC_TextureProperties& _Properties)
{
	CImage TxtDesc;
	CTC_TextureProperties Properties;
	int nMipMaps = 0;

	if(_TextureID == BreakTexture)
	{
		int a = 0;
	}

	CTextureContext* pTC = m_pTC;
	int TxtIDFlags = pTC->GetTextureFlags(_TextureID);

	if (!(TxtIDFlags & CTC_TXTIDFLAGS_ALLOCATED))
		return;

	pTC->GetTextureDesc(_TextureID, &TxtDesc, nMipMaps);
	if(TxtDesc.GetWidth() < 1)
		return;
	pTC->GetTextureProperties(_TextureID, Properties);

	if(nMipMaps < 1)
		Error_static("CRenderContextPS3::GL_BuildTexture2D", CStrF("Invalid number of mipmaps. (%d)", nMipMaps));

	CTextureContainer *pTContainer = pTC->GetTextureContainer(_TextureID);
	int iLocal = pTC->GetLocal(_TextureID);

	if (!pTContainer)
		return;

#ifndef PLATFORM_CONSOLE
	if (m_bResourceLog)
		pTC->SetTextureParam(_TextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_USED);
#endif

	uint32 PropFlags = Properties.m_Flags;

	int TexWidth = TxtDesc.GetWidth();
	int TexHeight = TxtDesc.GetHeight();

	bool bProcedural = 
		(PropFlags & CTC_TEXTUREFLAGS_PROCEDURAL)
		&& 
		!(PropFlags & (CTC_TEXTUREFLAGS_CUBEMAPCHAIN | CTC_TEXTUREFLAGS_CUBEMAP))
		&& 
		(nMipMaps == 1);

	int w = TexWidth;
	int h = TexHeight;
	
	int TxtFmt = TxtDesc.GetFormat();
	
	int iPicMip = MinMT(MaxMT(0, Properties.m_iPicMipGroup), CRC_MAXPICMIPS - 1);
	int PicMip = (PropFlags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, CRenderContextPS3::ms_This.m_lPicMips[iPicMip] + Properties.GetPicMipOffset());

	w = Max(w >> PicMip, 1);
	h = Max(h >> PicMip, 1);

	int iFirstMipMap = 0;
	int w2 = TexWidth;
	int h2 = TexHeight;
	while (w2 > w || h2 > h)
	{
		w2 = w2 >> 1;
		h2 = h2 >> 1;
		iFirstMipMap++;
	}

	int nMip = 1;
	if ((PropFlags & CTC_TEXTUREFLAGS_NOMIPMAP))
		nMip = 1;
	else if (!bProcedural)
	{
		nMip = GLGetMipMapLevels(w, h);
	}

	GL_SetTextureParameters(_RCID, _TextureID, _GLObjectTarget, _Properties);

	if (PropFlags & CTC_TEXTUREFLAGS_RENDER)
	{
		m_pTC->BuildInto(_TextureID, this);
		// Build Into may fuck up current texture so we have to re-bind it
		glBindTexture(_GLObjectTarget, _TextureID);
		LoadTextureRender(_GLImageTarget, _TextureID, pTContainer, iLocal, TxtDesc, Properties, iFirstMipMap, w, h, nMip);
	}
	else if(bProcedural)
	{
		LoadTextureBuildInto(_GLImageTarget, _TextureID, pTContainer, iLocal, TxtDesc, Properties, iFirstMipMap, w, h, nMip);
	}
	else
	{
		uint8 Versions[CTC_TEXTUREVERSION_MAX];
		int nVersions = pTC->EnumTextureVersions(_TextureID, Versions, CTC_TEXTUREVERSION_MAX);
		if (!nVersions)
		{
			ConOut("EnumTextureVersions error 0");
			return;
		}

		uint8 TextureVersion = Versions[0];

		uint8 TextureVersionPriority[] = {CTC_TEXTUREVERSION_S3TC, CTC_TEXTUREVERSION_FLOAT, CTC_TEXTUREVERSION_RAW};

		int nPriority = sizeof(TextureVersionPriority) / sizeof(uint8);

		for (int i = 0; i < nPriority; ++i)
		{
			for (int j = 0; j < nVersions; ++j)
			{
                if (TextureVersionPriority[i] == Versions[j])
				{
					TextureVersion = Versions[j];
					i = nPriority;
					break;
				}
			}
		}

		if(!LoadTexture(_GLImageTarget, _TextureID, pTContainer, iLocal, TextureVersion, Properties, iFirstMipMap, w, h, nMip))
		{
			if(!LoadTextureBuildInto(_GLImageTarget, _TextureID, pTContainer, iLocal, TxtDesc, Properties, iFirstMipMap, w, h, nMip))
			{
				return;
			}
		}
	}
}
/*
void CRenderContextPS3::GL_BuildTexture2D(int _RCID, int _TextureID, int _GLObjectTarget, int _GLImageTarget, const CTC_TextureProperties& _Properties)
{
	MSCOPE(CRenderContextPS3::GL_BuildTexture2D, RENDER_GL);

	GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) %d, %d", _RCID, _TextureID));

	CImage TxtDesc;
	const CTC_TextureProperties& Properties = _Properties;;
	int Ret_nMipMaps = 0;

	int TxtFlags = m_pTC->GetTextureDesc(_TextureID, &TxtDesc, Ret_nMipMaps);
	if (Ret_nMipMaps < 1) Error("GL_BuildTexture2D", CStrF("Invalid number of mipmaps. (%d)", Ret_nMipMaps));

	int RequestVersion = CTC_TEXTUREVERSION_RAW;

	{
		// Enumerate all available versions
		uint8 aVersions[CTC_TEXTUREVERSION_MAX];
		int nVersions = m_pTC->EnumTextureVersions(_TextureID, aVersions, CTC_TEXTUREVERSION_MAX);

		for(int i = 0; i < nVersions; i++)
		{
			if((aVersions[i] == CTC_TEXTUREVERSION_S3TC))
				RequestVersion	= CTC_TEXTUREVERSION_S3TC;
		}
	}

	int bAlphaTexture = false;
	int bCompressed = (RequestVersion != CTC_TEXTUREVERSION_RAW);

	int TexWidth = TxtDesc.GetWidth();
	int TexHeight = TxtDesc.GetHeight();
	int TxtFmt = TxtDesc.GetFormat();
	if ((w < 1) || (h < 1))		// Voodoo limitation
	{
		LogFile(CStrF("Getting invalid texture (%s)...", (char*)TxtDesc.IDString() ));
		CImage BuildTxt;
		int Fmt = IMAGE_FORMAT_BGRA8;
		GL_InitTextureBuildBuffers(Max(w, h), _TextureID);
		BuildTxt.CreateReference(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, w*CImage::Format2PixelSize(Fmt), m_lBuildTxt.GetBasePtr(), NULL);

		if (!(Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER))
		{
			MSCOPE(BuildInto, RENDER_GL);
			CImage* pImg = &BuildTxt;
			m_pTC->BuildInto(_TextureID, &pImg, 1, CTC_TEXTUREVERSION_ANY, IMAGE_CONVERT_RGBA);
		}

		Error("GL_BuildTexture2D", CStrF("Invalid texture dimensions. (%s, ID %d, TxtFlags %.4x)", (char*) TxtDesc.IDString(), _TextureID, Properties.m_Flags) );
	}

	#ifdef CRCGL_ALPHALIGHTMAPS
		#error "This doesn't work anymore because of precaching."
		bool bBWLM = (m_lAttribStack[m_iAttribStack].m_RasterMode == CRC_RASTERMODE_LIGHTMAPBLEND) && m_bAlphaLightMaps;
	#endif

	bool bProcedural = 
		(Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL)
		&& 
		!(Properties.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAPCHAIN | CTC_TEXTUREFLAGS_CUBEMAP))
		&& 
		(Ret_nMipMaps == 1);

	// -------------------------------------------------------------------
	// Figure out what mipmaps we're going to need.
	int iPicMip = MinMT(MaxMT(0, Properties.m_iPicMipGroup), CRC_MAXPICMIPS-1);
	int PicMip = (Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, CRenderContextPS3::ms_This.m_lPicMips[iPicMip] + Properties.GetPicMipOffset());

	int w = TexWidth;
	int h = TexHeight;

	w = Max(w >> PicMip, 1);
	h = Max(h >> PicMip, 1);

	int iFirstMipMap = 0;
	int w2 = TexWidth;
	int h2 = TexHeight;
	while(w2 > w || h2 > h)
	{
		w2 = w2 >> 1;
		h2 = h2 >> 1;
		iFirstMipMap++;
	}

	int nMip = 1;
	if(Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP)
		nMip = 1;
	else if(!bProcedural)
	{
		nMip = GLGetMipMapLevels(w, h);
	}


	// -------------------------------------------------------------------
	GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) Bind. %d", _RCID));
	glnBindTexture(_GLObjectTarget, _RCID);
	GLErr("GL_BuildTexture2D (glnBindTexture)");

	if(RequestVersion == CTC_TEXTUREVERSION_S3TC)
	{
		// -------------------------------------------------------------------
		// S3TC Compressed
		int BuildMip = Min(Ret_nMipMaps-1, PicMip);
		PicMip -= BuildMip;
		w = Max(1, w >> BuildMip);
		h = Max(1, h >> BuildMip);

		int nMipMaps = Ret_nMipMaps - BuildMip;

		for(int iMip = 0; iMip < nMipMaps; iMip++)
		{
			// Get texture image
			CImage* pMipMap = m_pTC->GetTexture(_TextureID, iMip + BuildMip, CTC_TEXTUREVERSION_S3TC);

			if (!pMipMap) Error("GL_BuildTexture2D", "Could not obtain direct access image for S3TC compressed texture.");

			if (pMipMap->GetMemModel() & IMAGE_MEM_COMPRESSTYPE_S3TC)
			{
				// Lock image data
				const uint8* pTextureData = (uint8*)pMipMap->LockCompressed();
				if (!pTextureData) 
					Error("GL_BuildTexture2D", "S3TC compressed texture could not be locked.");

				const CImage_CompressHeader_S3TC& Header = *(CImage_CompressHeader_S3TC*)pTextureData;
				pTextureData += Header.getOffsetData();

				int bpp = 4;
				int GLFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				switch(Header.getCompressType())
				{
				case IMAGE_COMPRESSTYPE_S3TC_DXT1 : GLFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; bpp = 4; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT3 : GLFmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; bpp = 8; bAlphaTexture = true; break;
				case IMAGE_COMPRESSTYPE_S3TC_DXT5 : GLFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; bpp = 8; bAlphaTexture = true; break;
				default : Error("GL_BuildTexture2D", CStrF("Unsupported S3TC compression format: %d", Header.getCompressType()));
				}

				gleCompressedTexImage2D(_GLImageTarget, iMip, GLFmt, w, h, 0, Header.getSizeData(), pTextureData);
				
				pMipMap->Unlock();

				GLErr(CStrF("GL_BuildTexture2D (glnTexImage2D S3TC %dx%d, %d)", w, h, iMip));
			}
			else
			{
				Error("GL_BuildTexture2D", CStrF("Compressed texture contained uncompressed mipmap. (%d/%d)", iMip, nMipMaps));
			}

			m_pTC->ReleaseTexture(_TextureID, iMip + BuildMip, CTC_TEXTUREVERSION_S3TC);

			w >>= 1;
			h >>= 1;
			if (w < 1) w = 1;
			if (h < 1) h = 1;
		}
	}
	else if (TxtFmt == IMAGE_FORMAT_I8A8)
	{
		// -------------------------------------------------------------------
		// 8-bit greyscale, 8-bit alpha
		int Fmt = TxtFmt;
		int PixSize = CImage::Format2PixelSize(Fmt);
		int GLFmt = GL_LUMINANCE_ALPHA;
		bAlphaTexture = true;
		int nComponents = 2;

		int InternalFormat = GL_LUMINANCE_ALPHA;

		{
			int BuildMip = Min(Ret_nMipMaps-1, PicMip);
			PicMip -= BuildMip;
			w = Max(1, w >> BuildMip);
			h = Max(1, h >> BuildMip);

			while(w > CRCGLES_MAX_TEXTURESIZE || h > CRCGLES_MAX_TEXTURESIZE)
				Error("GL_BuildTexture2D", CStrF("Can't auto-strech GREY8ALPHA8 textures. (%s, ID %d)", (char*) TxtDesc.IDString(), _TextureID) );

			CImage BuildTxt;
			CImage BuildTxt2;
			GL_InitTextureBuildBuffers(Max(w, h), _TextureID);
			uint8 *pBuildTxt = m_lBuildTxt.GetBasePtr();
			bool bBufferJustCreated = false;
			if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
			{
				int ImageSize = w * h * PixSize;
				if (m_lspTextureImages[_TextureID])
				{
					if (m_lspTextureImages[_TextureID]->m_Buffer.Len() != ImageSize)
					{
						bBufferJustCreated = true;
						m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
						memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
					}
				}
				else
				{
					m_lspTextureImages[_TextureID] = MNew(CBuffer_uint8);
					m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
					memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
					bBufferJustCreated = true;
				}

				pBuildTxt = m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr();
			}

			bool bBuildIntoWaived = false;

			BuildTxt.CreateReference(w, h, Fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, w*PixSize, pBuildTxt, NULL);
			{
				MSCOPE(BuildInto, RENDER_GL);
				if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
				{
					CImage* pImg = &BuildTxt;
					m_pTC->BuildInto(_TextureID, &pImg, 1, RequestVersion, bBufferJustCreated, BuildMip);

					if (!pImg)
						bBuildIntoWaived = true;
				}
				else
				{
					CImage* pImg = &BuildTxt;
					m_pTC->BuildInto(_TextureID, &pImg, 1, RequestVersion, IMAGE_CONVERT_RGBA, BuildMip);
				}
			}

	GLTexLog("(CRenderContextPS3::GL_BuildTexture2D) Format " + TxtDesc.IDString());

	GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) nMip %d, nComp %d", nMip, nComponents));
			if (nMip > 1) nMip = GLGetMipMapLevels(w, h);

			if (!bBuildIntoWaived)
			{
				glnTexImage2D(_GLImageTarget, 0, InternalFormat, w, h, 0, GLFmt, GL_UNSIGNED_BYTE, pBuildTxt);
				GLErr(CStrF("GL_BuildTexture2D (glnTexImage2D %dx%d)", w, h));
				if(nMip > 1)
				{
					glGenerateMipmapOES(_GLObjectTarget);
					GLErr("GL_BuildTexture2D (glGenerateMipmapOES)");
				}
			}
		}
	}
	else if (TxtFmt == IMAGE_FORMAT_I8 || TxtFmt == IMAGE_FORMAT_A8)
	{
		// -------------------------------------------------------------------
		// 8-bit greyscale and alpha
		int Fmt = TxtFmt;
		int PixSize = CImage::Format2PixelSize(Fmt);
		int GLFmt = (TxtFmt == IMAGE_FORMAT_I8) ? GL_LUMINANCE : GL_ALPHA;
		if (w == 1 || h == 1) GLFmt = GL_ALPHA;
		int bAlpha = (GLFmt == GL_ALPHA) ? 1 : 0;
		bAlphaTexture = bAlpha;
		int nComponents = 1;

		int InternalFormat = (bAlpha) ? GL_ALPHA : GL_LUMINANCE;

		{
			int BuildMip = Min(Ret_nMipMaps-1, PicMip);
			PicMip -= BuildMip;
			w = Max(1, w >> BuildMip);
			h = Max(1, h >> BuildMip);

			while(w > CRCGLES_MAX_TEXTURESIZE || h > CRCGLES_MAX_TEXTURESIZE)
				Error("GL_BuildTexture2D", CStrF("Can't auto-strech GREY8 textures. (%s, ID %d)", (char*) TxtDesc.IDString(), _TextureID) );

			CImage BuildTxt;
			CImage BuildTxt2;
			GL_InitTextureBuildBuffers(Max(w, h), _TextureID);
			uint8 *pBuildTxt = m_lBuildTxt.GetBasePtr();
			bool bBufferJustCreated = false;
			if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
			{
				int ImageSize = w * h * PixSize;
				if (m_lspTextureImages[_TextureID])
				{
					if (m_lspTextureImages[_TextureID]->m_Buffer.Len() != ImageSize)
					{
						bBufferJustCreated = true;
						m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
						memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
					}
				}
				else
				{
					m_lspTextureImages[_TextureID] = MNew(CBuffer_uint8);
					m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
					memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
					bBufferJustCreated = true;
				}

				pBuildTxt = m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr();
			}

			bool bBuildIntoWaived = false;

			BuildTxt.CreateReference(w, h, Fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, w*PixSize, pBuildTxt, NULL);
			{
				MSCOPE(BuildInto, RENDER_GL);
				if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
				{
					CImage* pImg = &BuildTxt;
					m_pTC->BuildInto(_TextureID, &pImg, 1, RequestVersion, bBufferJustCreated, BuildMip);

					if (!pImg)
						bBuildIntoWaived = true;
				}
				else
				{
					CImage* pImg = &BuildTxt;
					m_pTC->BuildInto(_TextureID, &pImg, 1, RequestVersion, (bAlpha) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB, BuildMip);
				}
			}

	GLTexLog("(CRenderContextPS3::GL_BuildTexture2D) Format " + TxtDesc.IDString());

	GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) nMip %d, nComp %d", nMip, nComponents));
			if (nMip > 1) nMip = GLGetMipMapLevels(w, h);

			if (!bBuildIntoWaived)
			{
				glnTexImage2D(_GLImageTarget, 0, InternalFormat, w, h, 0, GLFmt, GL_UNSIGNED_BYTE, pBuildTxt);
				GLErr(CStrF("GL_BuildTexture2D (glnTexImage2D %dx%d)", w, h));
				if(nMip > 1)
				{
					glGenerateMipmapOES(_GLImageTarget);
					GLErr("GL_BuildTexture2D (glGenerateMipmapOES)");
				}
			}
		}
	}
	else if (TxtFmt == IMAGE_FORMAT_CLUT8)
	{
		DebugBreak();
	}
	else
	{
		// -------------------------------------------------------------------
		// 24/32-bit RGB[A]
		int bAlpha = (TxtDesc.GetFormat() & (IMAGE_FORMAT_ALPHA | IMAGE_FORMAT_CLUT8));
		bAlphaTexture = bAlpha;
		int Fmt;
		int GLFmt;
		int InternalFormat = 0;
		int FeedDataType = GL_UNSIGNED_BYTE;

		if (TxtDesc.GetFormat() == IMAGE_FORMAT_ZBUFFER)
		{
			// Depth component format
			Fmt = IMAGE_FORMAT_I16;
			GLFmt = GL_DEPTH_COMPONENT;
			InternalFormat = GL_DEPTH_COMPONENT24;
			FeedDataType = GL_UNSIGNED_SHORT;

			glnTexParameteri(_GLImageTarget, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
			GLErr("glnSetTextureParameteri (GL_DEPTH_TEXTURE_MODE_ARB)");

		}
		else if (TxtDesc.GetFormat() == IMAGE_FORMAT_RGBA16_F)
		{
			// Float 16 format
			Fmt = IMAGE_FORMAT_BGRA8;
			GLFmt = GL_BGRA;
			InternalFormat = GL_RGBA16F_ARB;
		}
		else
		{
			// Other formats
			if (TxtDesc.GetFormat() == IMAGE_FORMAT_BGR8)
			{
				Fmt = IMAGE_FORMAT_BGR8;
				GLFmt = GL_BGR;
			}
			else
			{
				Fmt = IMAGE_FORMAT_BGRA8;
				GLFmt = GL_BGRA;
			}
			InternalFormat = (bCompressed) ? 
				((bAlpha) ? GL_COMPRESSED_RGBA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) :
				((bAlpha) ? GL_RGBA8 : GL_RGB8);
			if (Properties.m_Flags & CTC_TEXTUREFLAGS_HIGHQUALITY)
				InternalFormat = (bAlpha) ? GL_RGBA8 : GL_RGB8;
		}

		int PixSize = CImage::Format2PixelSize(Fmt);
		int ConvType = (bAlpha) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB;
		


		int BuildMip = Min(Ret_nMipMaps-1, PicMip);
		PicMip -= BuildMip;
		nMip -= BuildMip;

		w = Max(1, w >> BuildMip);
		h = Max(1, h >> BuildMip);

		CImage BuildTxt;
		CImage BuildTxt2;
		GL_InitTextureBuildBuffers(Max(w, h), _TextureID);

		uint8 *pBuildTxt = m_lBuildTxt.GetBasePtr();
		if ((Properties.m_Flags & CTC_TEXTUREFLAGS_PROCEDURAL))
		{
			int ImageSize = w * h * PixSize;
			if (m_lspTextureImages[_TextureID])
			{
				if (m_lspTextureImages[_TextureID]->m_Buffer.Len() != ImageSize)
				{
					m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
					memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
				}
			}
			else
			{
				m_lspTextureImages[_TextureID] = MNew(CBuffer_uint8);
				m_lspTextureImages[_TextureID]->m_Buffer.SetLen(ImageSize);
				memset(m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr(), 0, ImageSize);
			}

			pBuildTxt = m_lspTextureImages[_TextureID]->m_Buffer.GetBasePtr();
		}

		BuildTxt.CreateReference(w, h, Fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, w*PixSize, pBuildTxt, NULL);

		if (!(Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER))
		{
			MSCOPE(BuildInto, RENDER_GL);
			CImage* pImg = &BuildTxt;
			m_pTC->BuildInto(_TextureID, &pImg, 1, RequestVersion, ConvType, BuildMip);
		}
		else
		{
			MSCOPE(BuildInto, RENDER_GL);
			m_pTC->BuildInto(_TextureID, this);
		}

		GLTexLog("(CRenderContextPS3::GL_BuildTexture2D) Format " + TxtDesc.IDString());
		GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) Bind. %d", _RCID));
		glnBindTexture(_GLObjectTarget, _RCID);
		GLErr(CStrF("GL_BuildTexture2D (glnBindTexture, %s)", (char*) TxtDesc.IDString()) );

		GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) nMip %d, IntFmt %d", nMip, InternalFormat));

// URGENTFIXME: Remove this
//InternalFormat = GL_RGBA8;	// Force RGBA32

		CImage* pImg1 = &BuildTxt;
		CImage* pImg2 = &BuildTxt2;

		// Shrink texture until the driver can handle it, or a predefined number of mipmap steps.
		if (!(Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER))
		{
			while(PicMip > 0 || pImg1->GetWidth() > CRCGLES_MAX_TEXTURESIZE || pImg1->GetHeight() > CRCGLES_MAX_TEXTURESIZE)
			{
				DebugBreak();

//				int w = pImg1->GetWidth() >> 1;
//				int h = pImg1->GetHeight() >> 1;
//				if (w < 1) w = 1;
//				if (h < 1) h = 1;
//				if (w == 1 && h == 1) break;

//				GLTexLog(CStrF("(CRenderContextPS3::GL_BuildTexture2D) Mip %d, %d x %d", Mip, w, h));

//				GL_InitTextureBuildBuffers(Max(w, h), _TextureID);
//				pImg2->CreateReference(w, h, Fmt, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, w*PixSize, pBuildTxt, NULL);

//				if (w == pImg1->GetWidth())
//					CImage::StretchHalfY(pImg1, pImg2);
//				else if(h == pImg1->GetHeight())
//					CImage::StretchHalfX(pImg1, pImg2);
//				else
//					CImage::StretchHalf(pImg1, pImg2);

//				Swap(pImg1, pImg2);
//				PicMip--;
			}
		}

		if (nMip > 1) nMip = GLGetMipMapLevels(pImg1->GetWidth(), pImg1->GetHeight());

		{
			int w = pImg1->GetWidth();
			int h = pImg1->GetHeight();

			if (Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER)
			{
				DebugBreak();
				nMip = 1;
			}
			else
			{
				glnTexImage2D(_GLImageTarget, 0, InternalFormat, w, h, 0, GLFmt, FeedDataType, pBuildTxt);
				GLErr(CStrF("GL_BuildTexture2D (glnTexImage2D %dx%d)", w, h));
			}
			if(nMip > 1)
			{
				glGenerateMipmapOES(_GLObjectTarget);
				GLErr(CStrF("GL_BuildTexture2D (glGenerateMipmapOES(%d))", _GLObjectTarget));
			}
		}
	}

	GL_SetTextureParameters(_RCID, _TextureID, _GLObjectTarget, _Properties);
	GLErr("GL_BuildTexture2D (PostBuild)");
};
*/
void CRenderContextPS3::GL_SetTextureParameters(int _RCID, int _TextureID, int _GLObjectTarget, const CTC_TextureProperties& _Properties)
{
	// -------------------------------------------------------------------
	// Set the usual texture-parameters
	GLTexLog("(CRenderContextPS3::GL_BuildTexture2D) TexParams.");

	GLErr("GL_BuildTexture2D (Pre wrap)");

/*	if (Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_S)
		LogFile("U-CLAMP ON TEXTURE " + m_pTC->GetName(_TextureID));
	if (Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_T)
		LogFile("V-CLAMP ON TEXTURE " + m_pTC->GetName(_TextureID));*/

	if (_GLObjectTarget == GL_TEXTURE_CUBE_MAP)
	{
//		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
//		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
//		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);

		// Fixme: Requires EXT_TEXTURE_EDGE_CLAMP (always present for cubemap enabled HW?)
		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		GLErr("GL_BuildTexture2D (Wrap STR)");
	}
	else
	{
		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_S, (_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_U) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glnTexParameteri(_GLObjectTarget, GL_TEXTURE_WRAP_T, (_Properties.m_Flags & CTC_TEXTUREFLAGS_CLAMP_V) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		GLErr("GL_BuildTexture2D (Wrap ST)");
	}

//	glnTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glnTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	int MagFilter = GL_LINEAR;
	int MinFilter = GL_LINEAR_MIPMAP_NEAREST;

	switch(_Properties.m_MagFilter)
	{
	case CTC_MAGFILTER_DEFAULT : MagFilter = (m_Mode.m_Filter == CRC_GLOBALFILTER_POINT) ? GL_NEAREST : GL_LINEAR; break;
	case CTC_MAGFILTER_NEAREST : MagFilter = GL_NEAREST; break;
	case CTC_MAGFILTER_LINEAR : MagFilter = GL_LINEAR; break;
	case CTC_MAGFILTER_ANISOTROPIC : MagFilter = GL_LINEAR; break;
	}

	if (/*(nMip > 1) && */
		!(_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP))
	{
		// MIP-Mapping
		switch(_Properties.m_MinFilter)
		{
		case CTC_MINFILTER_DEFAULT : 
			{
				if (_Properties.m_MIPFilter == CTC_MIPFILTER_LINEAR)
					MinFilter = (m_Mode.m_Filter == CRC_GLOBALFILTER_POINT) ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
				else
				{
					switch(m_Mode.m_Filter)
					{
					case CRC_GLOBALFILTER_POINT : MinFilter = GL_NEAREST_MIPMAP_NEAREST; break;
					case CRC_GLOBALFILTER_BILINEAR : MinFilter = GL_LINEAR_MIPMAP_NEAREST; break;
					case CRC_GLOBALFILTER_TRILINEAR : MinFilter = GL_LINEAR_MIPMAP_LINEAR; break;
					}

//					MinFilter = (m_Mode.m_Filter == CRC_GLOBALFILTER_POINT) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
				}
				break;
			}
		case CTC_MINFILTER_NEAREST : MinFilter = (_Properties.m_MIPFilter == CTC_MIPFILTER_LINEAR) ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST; break;
		case CTC_MINFILTER_ANISOTROPIC :
		case CTC_MINFILTER_LINEAR : MinFilter = (_Properties.m_MIPFilter == CTC_MIPFILTER_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST; break;
		}
	}
	else
	{
		MinFilter = GL_LINEAR;
		// No MIPmaps.
		switch(_Properties.m_MinFilter)
		{
		case CTC_MINFILTER_DEFAULT : MinFilter = (m_Mode.m_Filter == CRC_GLOBALFILTER_POINT) ? GL_NEAREST : GL_LINEAR; break;
		case CTC_MINFILTER_NEAREST : MinFilter = GL_NEAREST; break;
		case CTC_MINFILTER_ANISOTROPIC :
		case CTC_MINFILTER_LINEAR : MinFilter = GL_LINEAR;  break;
		}
	}

	glnTexParameteri(_GLObjectTarget, GL_TEXTURE_MAG_FILTER, MagFilter);
	glnTexParameteri(_GLObjectTarget, GL_TEXTURE_MIN_FILTER, MinFilter);
	GLErr("GL_BuildTexture2D (Filter)");

	{
 		fp4 Anisotropy = Max(1.0f, Min(m_MaxAnisotropy, LERP(1.0f, m_MaxAnisotropy, m_Anisotropy)));
		if (_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP)
			Anisotropy = 1;
		if (_Properties.m_Flags & (CTC_TEXTUREFLAGS_CLAMP_U | CTC_TEXTUREFLAGS_CLAMP_V))
			Anisotropy = 1;


 		glnTexParameterf(_GLObjectTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, Anisotropy);
		GLErr(CStrF("GL_BuildTexture2D (Anisotropic %f, Target %d)", Anisotropy, _GLObjectTarget));
	}

	GLErr("GL_BuildTexture2D (Exit)");

//	T_Stop(Time);
//	if(m_bLog) LogFile(T_String("CRenderContextPS3::GL_BuildTexture2D ",Time));
	GLTexLog(CStr("(CRenderContextPS3::GL_BuildTexture2D) Done."));
};

void CRenderContextPS3::GL_SetTextureParameters(int _TextureID)
{
	CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	if (!(pIDInfo->m_Fresh & 1))
		return;

	int Target = (pIDInfo->m_Fresh & IDINFO_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);

	glBindTexture(Target, _TextureID);
	GLErr("GL_SetTextureParameters");

	GL_SetTextureParameters(_TextureID, _TextureID, Target, Properties);
}

void CRenderContextPS3::GL_UpdateAllTextureParameters()
{
	int nTxt = m_pTC->GetIDCapacity();
	for(int i = 1; i < nTxt; i++)
		GL_SetTextureParameters(i);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	CRenderContextPS3::ms_This.m_CurrentAttrib.m_TextureID[0] = 0;
	GLErr("GL_UpdateAllTextureParameters");
}

void CRenderContextPS3::GL_BuildTexture(int _RCID, int _TextureID, const CTC_TextureProperties& _Properties)
{
	MSCOPESHORT(CRenderContextPS3::GL_BuildTexture);
	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);

	CRC_IDInfo* pIDInfoCurrent = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];

	if ((((pIDInfoCurrent->m_Fresh & IDINFO_CUBEMAP) != 0) !=
		((Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAP) != 0)) ||
		!(_Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER))
		)
	{
		GLuint TexID = _TextureID;
		glnDeleteTextures(1, &TexID);
		GLErr("GL_BuildTexture (glnDeleteTextures)");
		pIDInfoCurrent->m_Fresh &= ~(1 | IDINFO_BUILT);
	}

	if (Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAP)
	{
		pIDInfoCurrent->m_Fresh |= IDINFO_CUBEMAP;
		// -------------------------------------------------------------------
		// Cubemap (All cubemap faces get the same texture)
		{
		// ConOutL("(CRenderContextPS3::GL_BuildTexture) CUBEMAP");
		//	GLErr("GL_BuildTexture");
			glBindTexture(GL_TEXTURE_CUBE_MAP, _TextureID);
			GLErr("GL_BuildTexture");

			for(int i = 0; i < 6; i++)
			{
		// ConOutL(CStrF("(CRenderContextPS3::GL_BuildTexture) CUBEMAP %d", i));
				GL_BuildTexture2D(_RCID, _TextureID, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Properties);
		// ConOutL(CStrF("(CRenderContextPS3::GL_BuildTexture) CUBEMAP %d done", i));
			}


			GLErr("GL_BuildTexture (Post cube)");
		}
	}
	else if (Properties.m_Flags & CTC_TEXTUREFLAGS_CUBEMAPCHAIN)
	{
		pIDInfoCurrent->m_Fresh |= IDINFO_CUBEMAP;
		// -------------------------------------------------------------------
		// Cubemap chain
		{
		// ConOutL(CStrF("(CRenderContextPS3::GL_BuildTexture) CUBEMAPCHAIN %d", _TextureID));
		//	GLErr("GL_BuildTexture");
			glBindTexture(GL_TEXTURE_CUBE_MAP, _TextureID);
			GLErr("GL_BuildTexture");

			CImage DescRef;
			int nMipMaps;
			m_pTC->GetTextureDesc(_TextureID, &DescRef, nMipMaps);

			CTextureContainer* pTC = m_pTC->GetTextureContainer(_TextureID);
			int iLocal = m_pTC->GetLocal(_TextureID);
			uint8 aVersions[CTC_TEXTUREVERSION_MAX];
			int nVersions = m_pTC->EnumTextureVersions(_TextureID, aVersions, CTC_TEXTUREVERSION_MAX);

			for(int i = 0; i < 6; i++)
			{

				int TextureID = pTC->GetTextureID(iLocal + (i * nVersions));

				CImage Desc;
				m_pTC->GetTextureDesc(TextureID, &Desc, nMipMaps);
				if (Desc.GetWidth() != DescRef.GetWidth() ||
					Desc.GetHeight() != DescRef.GetHeight() ||
					Desc.GetFormat() != DescRef.GetFormat())
					Error("GL_BuildTexture", CStrF("Inconsistent cubemap faces (%s != %s): %s", Desc.IDString().Str(), DescRef.IDString().Str(), m_pTC->GetName(_TextureID)));

			// ConOutL(CStrF("        Face %d, %s, nMIPmaps %d, ", i, Desc.IDString().Str(), nMipMaps) + m_pTC->GetName(_TextureID + i) );
				GL_BuildTexture2D(_RCID, TextureID, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Properties);
			}

		// ConOutL(CStrF("(CRenderContextPS3::GL_BuildTexture) CUBEMAPCHAIN %d done", _TextureID));

			GLErr("GL_BuildTexture (Post cubechain)");
		}
	}
	else
	{
		pIDInfoCurrent->m_Fresh &= ~IDINFO_CUBEMAP;
		// -------------------------------------------------------------------
		// Regular 2D-texture
		glBindTexture(GL_TEXTURE_2D, _TextureID);

		GL_BuildTexture2D(_RCID, _TextureID, GL_TEXTURE_2D, GL_TEXTURE_2D, Properties);


		GLErr("GL_BuildTexture (Post 2D)");
	}

	// Flag texture as built and fresh
	pIDInfoCurrent->m_Fresh |= (IDINFO_BUILT | 1);

	if (m_bLog && !(Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER)))
	{
		CImage TxtDesc;
		CTC_TextureProperties Properties;
		int Ret_nMipMaps = 0;
		m_pTC->GetTextureDesc(_TextureID, &TxtDesc, Ret_nMipMaps);
		int TxtFlags = m_pTC->GetTextureFlags(_TextureID);

		int PicMip = (Properties.m_Flags & CTC_TEXTUREFLAGS_NOPICMIP) ? 0 : MaxMT(0, m_lPicMips[Properties.m_iPicMipGroup] + Properties.GetPicMipOffset());
		
		/*M_TRACEALWAYS("Texture Loaded: %s, ID %i, Picmip %d(%d), Size %d,%d, Format %.8x, %.8x, (%s Precache %i)\n", 
			m_pTC->GetName(_TextureID).Str(), _TextureID,
			PicMip, 
			Properties.m_iPicMipGroup, 
			TxtDesc.GetWidth() >> PicMip, TxtDesc.GetHeight() >> PicMip,
			TxtDesc.GetFormat(),
			Properties.m_Flags,
			Properties.GetFlagsString().Str(), TxtFlags & 1);*/
	}

	GLErr("GL_BuildTexture (Exit)");
}

void CRenderContextPS3::GL_UnloadTexture(int _RCID)
{
	CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_RCID];
	if(pIDInfo->m_Fresh & IDINFO_BUILT)
	{
		GLuint Tex = _RCID;
		glnDeleteTextures(1, &Tex);
		GLErr("GL_UnloadTexture (glnDeleteTextures)");
	}

	lTextures[_RCID].Clear();
	pIDInfo->m_Fresh &= ~(1 | IDINFO_BUILT);
}

void CRenderContextPS3::GL_SetTexture(int _TextureID, int _iTexChannel, bool _bPrecache)
{
	MSCOPESHORT(CRenderContextPS3::GL_SetTexture);

	if (_iTexChannel > 0)
	{
		glActiveTexture(GL_TEXTURE0 + _iTexChannel);
		GLErr("GL_SetTexture (glSelectTexture)");
	}

	CRC_IDInfo* pIDInfoCurrent = &m_spTCIDInfo->m_pTCIDInfo[CRenderContextPS3::ms_This.m_CurrentAttrib.m_TextureID[_iTexChannel]];
	CRC_IDInfo* pIDInfoNew = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	int OldTarget = (pIDInfoCurrent->m_Fresh & IDINFO_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	int NewTarget = (pIDInfoNew->m_Fresh & IDINFO_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

	//if (OldTarget != NewTarget)
	{
		glBindTexture(OldTarget, 0);
		GLErr("GL_SetTexture");
	}

	if (_TextureID)
	{
		CTC_TextureProperties Properties;
		m_pTC->GetTextureProperties(_TextureID, Properties);

		// Debug stuff
		CImage TxtDesc;
		int nMipMaps;
		m_pTC->GetTextureDesc(_TextureID, &TxtDesc, nMipMaps);

		m_nBindTexture++;
		CRC_IDInfo* pIDInfo = &m_spTCIDInfo->m_pTCIDInfo[_TextureID];
		if (!(pIDInfo->m_Fresh & 1))
		{
			if (!m_bAllowTextureLoad && !(Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER)))
			{
				M_TRACEALWAYS("GL: Unprecached texture %s (%d)\n", m_pTC->GetName(_TextureID).Str(), _TextureID );
				
				glBindTexture(NewTarget, 0);
				GLErr("GL_SetTexture");
			}
			else
			{
				M_TRACEALWAYS("GL: Unbuilt texture %s (%d)\n", m_pTC->GetName(_TextureID).Str(), _TextureID );
				CRenderContextPS3::ms_This.m_CurrentAttrib.m_TextureID[_iTexChannel] = _TextureID;

				GL_BuildTexture(_TextureID, _TextureID, Properties);

				// BuildTexture fucks with bind, redo it
				if (_iTexChannel > 0)
					glActiveTexture(GL_TEXTURE0);
				glBindTexture(NewTarget, _TextureID);
				GLErr("GL_SetTexture");
			}
		}
		else
		{
			glBindTexture(NewTarget, _TextureID);
			GLErr("GL_SetTexture");
		}

		{
			fp4 Bias = fp4(int8(Properties.m_MIPMapLODBias)) / 64.0f;
			glnTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, Bias);
			GLErr("GL_SetTexture (LodBias)");
		}

	}
	else
	{
		glBindTexture(NewTarget, 0);
		GLErr("GL_SetTexture");
		m_nBindTexture++;
	}

	GLErr("GL_SetTexture (0)");
	if (_iTexChannel > 0)
		glActiveTexture(GL_TEXTURE0);

	GLErr("GL_SetTexture (1)");
}
