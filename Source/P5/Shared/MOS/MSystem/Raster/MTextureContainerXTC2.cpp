
#include "PCH.h"
#include "MSystem.h"
#include "MTextureContainerXTC2.h"

// WORLDDATA_TEXTURECONTAINERS, Memused:    1 404 684 in 6674 allocations, Activity: 84974 Allocations, 78300 Deletions
MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_VirtualXTC2, CTextureContainer);

IMPLEMENT_OPERATOR_NEW(CTextureContainer_VirtualXTC2);

static TArray<uint8> g_lLoadScratch;

CTextureContainer_VirtualXTC2::CTextureContainer_VirtualXTC2()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_ctor, MAUTOSTRIP_VOID);
	m_bIsCached = false;
	m_bHasXT0 = false;
}

CTextureContainer_VirtualXTC2::~CTextureContainer_VirtualXTC2()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_dtor, MAUTOSTRIP_VOID);
	M_TRY
	{
		if (m_pTC)
		{
			for(int i = 0; i < m_lTextureDesc.Len(); i++)
			{
				if (m_lTextureDesc[i].m_TextureID)
					m_pTC->FreeID(m_lTextureDesc[i].m_TextureID);
			}
		}
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
}


void CTextureContainer_VirtualXTC2::ReadImageDirectory(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_ReadImageDirectory, MAUTOSTRIP_VOID);
	// Check for Image-directory.
	_pDFile->PushPosition();
	if (_pDFile->GetNext("IMAGEDIRECTORY5"))
	{
		int nTxt = _pDFile->GetUserData();
		m_lTextureDesc.SetLen(nTxt);
#ifndef USE_HASHED_TEXTURENAME
		m_lTextureNameHeap.SetLen(32*nTxt);
#endif

		mint TextureNameHeapPos = 0;

		CCFile* pFile = _pDFile->GetFile();

		int iTxt;
		for(iTxt = 0; iTxt < nTxt; iTxt++)
		{
			CTextureDesc& Desc = m_lTextureDesc[iTxt];

			int32 iPalette;
			pFile->ReadLE(iPalette);
			Desc.m_iPalette = iPalette;

		#ifdef USE_PACKED_TEXTUREPROPERTIES
			uint32 PaletteFilePos;
			pFile->ReadLE(PaletteFilePos); // stored in struct further down
		#else
			pFile->ReadLE(Desc.m_PaletteFilePos);
		#endif

			{
				CFStr TmpStr;
				TmpStr.Read(pFile);
				int NameLen = TmpStr.Len();
				if (NameLen > 31) Error("Read", "Too long texture-name.");
				
#ifdef USE_HASHED_TEXTURENAME				
				// Convert the name to ID
				Desc.m_NameID = StringToHash(TmpStr);
				Desc.m_TextureID = m_pTC->AllocID(m_iTextureClass, iTxt, Desc.m_NameID);

#else
				// Set name heap offset, not absolute address
				Desc.m_pName = (char*)TextureNameHeapPos;
				CStrBase::mfsncpy(&m_lTextureNameHeap[TextureNameHeapPos], CSTR_FMT_ANSI, (char*)TmpStr, CSTR_FMT_ANSI, NameLen+1);
				
				TextureNameHeapPos += NameLen+1;
#endif				
			}

			int32 PicMip;
			pFile->ReadLE(PicMip);
			Desc.m_iPicMip = PicMip;

			int32 nMipMaps;
			pFile->ReadLE(nMipMaps);
			Desc.m_nMipMaps = nMipMaps;

			CImage Header;
			Header.ReadHeader(pFile);

			Desc.m_Format = Header.GetFormat();
			Desc.m_MemModel = Header.GetMemModel();
			Desc.m_Width	= Header.GetWidth();
			Desc.m_Height	= Header.GetHeight();
			if (Desc.GetWidth() != Header.GetWidth() ||
				Desc.GetHeight() != Header.GetHeight())
#ifdef USE_HASHED_TEXTURENAME
				Error("ReadImageDirectory", CStrF("Invalid texture dimensions, ID:%d, %s", Desc.m_TextureID, Header.IDString().Str()));
#else
				Error("ReadImageDirectory", CStrF("Invalid texture dimensions, %s, %s", (m_lTextureNameHeap.GetBasePtr() + (mint)Desc.m_pName), Header.IDString().Str()));
#endif				

			pFile->ReadLE(Desc.m_TextureDataFilePos);

		#ifdef USE_PACKED_TEXTUREPROPERTIES
			Desc.m_PaletteFileOffset = 0;
			if (PaletteFilePos)
			{
				M_ASSERT(Abs(int(PaletteFilePos - Desc.m_TextureDataFilePos)) < (1<<24), "PaletteFileOffset doesn't fit in 24 bits!");
				Desc.m_PaletteFileOffset = PaletteFilePos - Desc.m_TextureDataFilePos;
			}
		#endif

			//			for(int i = 0; i < Desc.m_nMipMaps; i++)
			//				pFile->ReadLE(Desc.m_lMipMapFilePos[i]);

			Desc.m_Properties.Read(pFile);

			if (Desc.m_Width == 2048 && Desc.m_Height == 2048 && Desc.m_Properties.GetPicMipOffset() == -1)
			{
				// screw you!(E3 hack!)
				M_TRACEALWAYS("ignoring picmip offset -2 on big fucking texture (2048 x 2048)\n");
				Desc.m_Properties.m_PicMipOffset = 0;
			}
		}

#ifndef USE_HASHED_TEXTURENAME		
		// Realloc name heap
		m_lTextureNameHeap.SetLen(TextureNameHeapPos);

		// Offset all name pointers to the new heap base address
		// Allocate texture IDs.
		for(iTxt = 0; iTxt < nTxt; iTxt++)
		{
			CTextureDesc& Desc = m_lTextureDesc[iTxt];

			Desc.m_pName += (mint)m_lTextureNameHeap.GetBasePtr();
			Desc.m_TextureID = m_pTC->AllocID(m_iTextureClass, iTxt, Desc.m_pName);
		}
#endif
	}
	else
	{
		LogFile(CStrF("ERROR: No IMAGEDIRECTORY5 entry in %s", _pDFile->GetFile()->GetFileName().Str()));
		Error("ReadImageDirectory", "No IMAGEDIRECTORY5 entry.");
	}

/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		CRegistry* pEnv = pSys->GetEnvironment();
		
		if (pEnv && pEnv->GetValuei("RESOURCES_LOG", 0) != 0)
		{
			MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
			if (pTC)
			{
				CCFile TulFile;
				TulFile.Open(pSys->m_ExePath + "\\Textures.tul", CFILE_READ);
				int32 Len;
				TulFile.ReadLE(Len);
				for (int i = 0; i < Len; ++i)
				{
					CStr Temp;
					
					Temp.Read(&TulFile);

					int TexID = pTC->GetTextureID(Temp);
					if (TexID >= 0)
						pTC->SetTextureParam(TexID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_USED);
				}
			}
		}
	}*/

	
	_pDFile->PopPosition();
}



void CTextureContainer_VirtualXTC2::PostCreate()
{
	if (CDiskUtil::FileExists(m_FileName + ".xt0"))
	{
		m_bHasXT0 = true;

		CCFile File;
		File.Open(m_FileName + ".xt0", CFILE_BINARY|CFILE_READ);

		uint32 nTextures;
		File.ReadLE(nTextures);

		while (nTextures)
		{
			uint32 iLocal;
			uint32 FileOffset;
			File.ReadLE(iLocal);
			File.ReadLE(FileOffset);
			m_lTextureDesc[iLocal].m_TextureXT0FilePos = FileOffset;
			--nTextures;
		}
	}
}

void CTextureContainer_VirtualXTC2::ClearCache()
{
	// Close XT0 file
    m_XT0File.Close();    
}
void CTextureContainer_VirtualXTC2::ReadTexture(int _iLocal, CTextureImages* _pTexture, int _iMipMapStart, int _iMipMapEnd, int _nVirtual)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_ReadTexture, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_VirtualXTC2::ReadTexture); //AR-SCOPE
	
	M_TRY
	{
		const CTextureDesc& Desc = m_lTextureDesc[_iLocal];
		M_ASSERT(_pTexture, "!");
		
		_pTexture->m_iLocal = _iLocal;

		if (m_bHasXT0 && _iMipMapStart == Desc.m_iPicMip && Desc.m_TextureXT0FilePos && !_nVirtual && !CByteStream::XDF_GetRecord() && !CByteStream::XDF_GetUse())
		{
			M_ASSERT(Desc.m_PaletteFilePos == 0 && Desc.m_iPalette < 0, "Palette not supported for xt0");
			if (!m_XT0File.IsOpen())
			{
				m_XT0File.Open(m_FileName + ".xt0", CFILE_BINARY|CFILE_READ);
			}
			m_XT0File.Seek(Desc.m_TextureXT0FilePos);
			_pTexture->m_lMipMaps[_iMipMapStart].Read(&m_XT0File, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM, _pTexture->m_spPalette);

			if (_iMipMapEnd == _iMipMapStart) // Nothing more to do
				return;
		}

		CCFile File;
		File.Open(GetFileName(), CFILE_BINARY | CFILE_READ);
		
		// Read palette
		if (!_pTexture->m_spPalette)
		{
			// Check palette index
			spCImagePalette spPal;
			if (Desc.m_iPalette >= 0)
				spPal = m_lspPalettes[Desc.m_iPalette];
			
			// Read palette if we're supposed to do so..
		#ifdef USE_PACKED_TEXTUREPROPERTIES
			if (Desc.m_PaletteFileOffset && (spPal == NULL))
		#else
			if (Desc.m_PaletteFilePos && (spPal == NULL))
		#endif
			{
				spPal = MNew(CImagePalette);
				if (spPal == NULL) MemError("Read");
				File.Seek(Desc.GetPaletteFilePos());

				uint8 Tmp[256*4];
				File.Read(&Tmp, sizeof(Tmp));

				CPixel32 Pal[256];
				for (int i=0; i<256; i++)
					Pal[i] = CPixel32(Tmp[i*4+2], Tmp[i*4+1], Tmp[i*4+0], Tmp[i*4+3]);

				spPal->SetPalette(Pal, 0, 256);
			}

			_pTexture->m_spPalette = spPal;
		}
		
		File.Seek(Desc.m_TextureDataFilePos);
		File.ReadLE(&_pTexture->m_lMipMapFilePos[Desc.m_iPicMip], (Desc.m_nMipMaps - Desc.m_iPicMip));
		
		// Read mipmap range
		for(int iMipMap = _iMipMapStart; iMipMap <= _iMipMapEnd; iMipMap++)
		{
			// Skip if the mipmap is already loaded
			if (_pTexture->m_MipMapLoadMask & (1 << iMipMap))
				continue;
			
			File.Seek(_pTexture->m_lMipMapFilePos[iMipMap]);
			_pTexture->m_lMipMaps[iMipMap].Read(&File, IMAGE_MEM_TEXTURE | IMAGE_MEM_SYSTEM | (_nVirtual ? IMAGE_MEM_VIRTUAL : 0), _pTexture->m_spPalette);
			if (_nVirtual)
            	--_nVirtual;
			_pTexture->m_MipMapLoadMask |= 1 << iMipMap;
		}
		
		File.Close();
	}
	M_CATCH(
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		throw;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		throw;
	}
	)
#endif
}


CImage* CTextureContainer_VirtualXTC2::GetTextureMipMap(int _iLocal, int _iMipMap, int _nMips, int _nVirtual)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTextureMipMap, NULL);
	MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTextureMipMap);
	const CTextureDesc& Desc = m_lTextureDesc[_iLocal];
	if (_iMipMap < Desc.m_iPicMip || _iMipMap >= Desc.m_nMipMaps)
	{
#ifdef USE_HASHED_TEXTURENAME	
		M_TRACEALWAYS("(CTextureContainer_VirtualXTC2::GetTextureMipMap) Invalid mipmap requested: %d/%d/%d, texture '%08X'", _iMipMap, Desc.m_iPicMip, Desc.m_nMipMaps, Desc.m_NameID);
		Error("GetMipMap", CStrF("Invalid mipmap requested: %d/%d/%d, texture '%08X'", _iMipMap, Desc.m_iPicMip, Desc.m_nMipMaps, Desc.m_NameID));
#else
		M_TRACEALWAYS("(CTextureContainer_VirtualXTC2::GetTextureMipMap) Invalid mipmap requested: %d/%d/%d, texture %s", _iMipMap, Desc.m_iPicMip, Desc.m_nMipMaps, Desc.m_pName);
		Error("GetMipMap", CStrF("Invalid mipmap requested: %d/%d/%d, texture %s", _iMipMap, Desc.m_iPicMip, Desc.m_nMipMaps, Desc.m_pName));
#endif		
	}

	DLock(m_Lock);
	if (m_spTempTexture != NULL && m_spTempTexture->m_iLocal != _iLocal)
		m_spTempTexture = NULL;

	int iEnd = _nMips;
	if (Desc.m_Properties.m_Flags & CTC_TEXTUREFLAGS_NOMIPMAP)
		iEnd = _iMipMap;
	else if (iEnd < 1)
		iEnd = Desc.m_nMipMaps-1;
	else
		iEnd = _iMipMap + iEnd - 1;

	if (!m_spTempTexture)
	{
		MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTextureMipMap_FirstLoad);
		m_spTempTexture = MNew(CTextureImages);

		if (!m_spTempTexture)
			MemError("GetMipMap");

		// We assume we access the largest mipmap first and that all subsequent mipmaps will also be accessed later.
		ReadTexture(_iLocal, m_spTempTexture, _iMipMap, iEnd, _nVirtual);
		return &m_spTempTexture->m_lMipMaps[_iMipMap];
	}
	else
	{
		MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTextureMipMap_AdditionalLoad);
		// Check if this mipmap is already loaded.
		if (m_spTempTexture->m_MipMapLoadMask & (1 << _iMipMap))
			return &m_spTempTexture->m_lMipMaps[_iMipMap];
		else
		{
			// We should not end up here, but in case we do it must work anyway.
			ConOutLD("§cf80WARNING: (CTextureContainer_VirtualXTC2::GetMipMap) Unexpected texture access pattern.");
			ReadTexture(_iLocal, m_spTempTexture, _iMipMap, iEnd, _nVirtual);

			return &m_spTempTexture->m_lMipMaps[_iMipMap];
		}
	}
}

CImage* CTextureContainer_VirtualXTC2::GetTextureNumMip(int _iLocal, int _iMipMap, int _nMips, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTexture, NULL);
	MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTexture);

	// Find correct iLocal for _iLocal/_TextureVersion pair
	int iLocal = _iLocal;
	uint16 iGlobalID = m_lTextureDesc[iLocal].m_TextureID;
	int nNumLocal = m_lTextureDesc.Len();
	while(m_lTextureDesc[iLocal].m_Properties.m_TextureVersion != _TextureVersion)
	{
		iLocal++;
		if((iLocal >= nNumLocal) || m_lTextureDesc[iLocal].m_TextureID != iGlobalID)
		{
			// Could not find correct version
			iLocal = _iLocal;
			break;
		}
	}

	DLock(m_Lock);
	if (m_spTempTexture != NULL && m_spTempTexture->m_iLocal != iLocal)
	{
		if (m_spTempTexture->m_MipMapAccessMask)
			Error("GetTexture", "Access mask not cleared.");
		m_spTempTexture = NULL;
	}

	CImage* pRet = GetTextureMipMap(iLocal, _iMipMap, _nMips, false);
	if (m_spTempTexture != NULL)
		m_spTempTexture->m_MipMapAccessMask |= (1 << _iMipMap);
	return pRet;
}

CImage* CTextureContainer_VirtualXTC2::GetTexture(int _iLocal, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTexture, NULL);
	MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTexture);

	// Find correct iLocal for _iLocal/_TextureVersion pair
	int iLocal = _iLocal;
	uint16 iGlobalID = m_lTextureDesc[iLocal].m_TextureID;
	int nNumLocal = m_lTextureDesc.Len();
	while(m_lTextureDesc[iLocal].m_Properties.m_TextureVersion != _TextureVersion)
	{
		iLocal++;
		if((iLocal >= nNumLocal) || m_lTextureDesc[iLocal].m_TextureID != iGlobalID)
		{
			// Could not find correct version
			iLocal = _iLocal;
			break;
		}
	}

	DLock(m_Lock);
	if (m_spTempTexture != NULL && m_spTempTexture->m_iLocal != iLocal)
	{
		if (m_spTempTexture->m_MipMapAccessMask)
			Error("GetTexture", "Access mask not cleared.");
		m_spTempTexture = NULL;
	}

	CImage* pRet = GetTextureMipMap(iLocal, _iMipMap, -1, false);
	if (m_spTempTexture != NULL)
		m_spTempTexture->m_MipMapAccessMask |= (1 << _iMipMap);
	return pRet;
}

CImage* CTextureContainer_VirtualXTC2::GetVirtualTexture(int _iLocal, int _iMipMap, int _nVirtual, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTexture, NULL);
	MSCOPESHORT(CTextureContainer_VirtualXTC2::GetTexture);

	// Find correct iLocal for _iLocal/_TextureVersion pair
	int iLocal = _iLocal;
	uint16 iGlobalID = m_lTextureDesc[iLocal].m_TextureID;
	int nNumLocal = m_lTextureDesc.Len();
	while(m_lTextureDesc[iLocal].m_Properties.m_TextureVersion != _TextureVersion)
	{
		iLocal++;
		if((iLocal >= nNumLocal) || m_lTextureDesc[iLocal].m_TextureID != iGlobalID)
		{
			// Could not find correct version
			iLocal = _iLocal;
			break;
		}
	}

	DLock(m_Lock);
	if (m_spTempTexture != NULL && m_spTempTexture->m_iLocal != iLocal)
	{
		if (m_spTempTexture->m_MipMapAccessMask)
			Error("GetTexture", "Access mask not cleared.");
		m_spTempTexture = NULL;
	}

	CImage* pRet = GetTextureMipMap(iLocal, _iMipMap, -1, _nVirtual);
	if (m_spTempTexture != NULL)
		m_spTempTexture->m_MipMapAccessMask |= (1 << _iMipMap);
	return pRet;
}

void CTextureContainer_VirtualXTC2::ReleaseTexture(int _iLocal, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_ReleaseTexture, MAUTOSTRIP_VOID);
	// Find correct iLocal for _iLocal/_TextureVersion pair
	int iLocal = _iLocal;
	uint16 iGlobalID = m_lTextureDesc[iLocal].m_TextureID;
	int nNumLocal = m_lTextureDesc.Len();
	while(m_lTextureDesc[iLocal].m_Properties.m_TextureVersion != _TextureVersion)
	{
		iLocal++;
		if((iLocal >= nNumLocal) || m_lTextureDesc[iLocal].m_TextureID != iGlobalID)
		{
			iLocal = _iLocal;
			break;
		}
	}

	DLock(m_Lock);
	if (m_spTempTexture && m_spTempTexture->m_iLocal == iLocal)
	{
		m_spTempTexture->m_MipMapAccessMask &= ~(1 << _iMipMap);
		m_spTempTexture->m_MipMapLoadMask &= ~(1 << _iMipMap);
		m_spTempTexture->m_lMipMaps[_iMipMap].Destroy();

		if (!m_spTempTexture->m_MipMapLoadMask)
			m_spTempTexture = NULL;
	}
}

void CTextureContainer_VirtualXTC2::ReleaseTextureAllMipmaps(int _iLocal, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_ReleaseTexture, MAUTOSTRIP_VOID);

	// Find correct iLocal for _iLocal/_TextureVersion pair
	int iLocal = _iLocal;
	uint16 iGlobalID = m_lTextureDesc[iLocal].m_TextureID;
	int nNumLocal = m_lTextureDesc.Len();
	while(m_lTextureDesc[iLocal].m_Properties.m_TextureVersion != _TextureVersion)
	{
		iLocal++;
		if((iLocal >= nNumLocal) || m_lTextureDesc[iLocal].m_TextureID != iGlobalID)
			return;
	}

	DLock(m_Lock);
	if (m_spTempTexture && m_spTempTexture->m_iLocal == iLocal)
	{
		m_spTempTexture = NULL;
	}
}


void CTextureContainer_VirtualXTC2::Create(CDataFile* _pDFile, int _NumCache)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_Create, MAUTOSTRIP_VOID);
	m_FileName = _pDFile->GetFile()->GetFileName();
	m_ContainerName = m_FileName.GetFilename();

//	if (!_pDFile->GetNext("IMAGELIST")) Error("-", "Invalid XTC.");
//	if (!_pDFile->GetSubDir()) Error("-", "Invalid XTC.");
	ReadImageDirectory(_pDFile);
}

void CTextureContainer_VirtualXTC2::Create(CStr _FileName, int _NumCache)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_Create_2, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_FileName);
	Create(&DFile, _NumCache);
	DFile.Close();
	PostCreate();
}

#ifndef PLATFORM_CONSOLE
void CTextureContainer_VirtualXTC2::WriteXTCStripData(CStr _Source, CStr _Dest)
{
	CDataFile DFile;
	DFile.Open(_Source);
	if (DFile.GetNext("IMAGEDIRECTORY5"))
	{
		int32 UserData1 = DFile.GetUserData();
		int32 UserData2 = DFile.GetUserData2();
		int32 nSize = DFile.GetEntrySize();
		TArray<uint8> lData;
		lData.SetLen(nSize);
		DFile.GetFile()->Read(lData.GetBasePtr(), nSize);
		CDataFile DFile2;
		DFile2.Create(_Dest, 2);
		DFile2.BeginEntry("IMAGEDIRECTORY5");
		DFile2.GetFile()->Write(lData.GetBasePtr(), nSize);
		DFile2.EndEntry(UserData1, UserData2);
		DFile2.Close();

	}
	DFile.Close();
}
#endif


void CTextureContainer_VirtualXTC2::SetCacheFile(CStr _CacheFileName)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_SetCacheFile, MAUTOSTRIP_VOID);
	m_CacheFileName = _CacheFileName;

	if (CDiskUtil::FileExists(_CacheFileName))
		m_bIsCached = true;
}

CStr CTextureContainer_VirtualXTC2::GetFileName()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetFileName, CStr());
	if (m_CacheFileName == "")
		return m_FileName;
	else
	{
		if (!m_bIsCached)
		{
			CStr CachePath = m_CacheFileName.GetPath();

			ConOutL(CStrF("(CTextureContainer_VirtualXTC2) Caching %s to %s", (char*) m_FileName, (char*)m_CacheFileName));

			if (CachePath != "")
				CDiskUtil::CreatePath(CachePath);
			CDiskUtil::CpyFile(m_FileName, m_CacheFileName, 1024 * 512);
			m_bIsCached = true;
		}
		return m_CacheFileName;
	}
}

int CTextureContainer_VirtualXTC2::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetNumLocal, 0);
	return m_lTextureDesc.Len();
}

CStr CTextureContainer_VirtualXTC2::GetName(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetName, CStr());
#ifdef USE_HASHED_TEXTURENAME
	M_TRACEALWAYS("CTextureContainer_VirtualXTC2::GetName, You must use hash instead of name!\n");
	Error_static("CTextureContainer_VirtualXTC2::GetName", "Name is not stored in texture. You must use ID instead."); 
#else
	return m_lTextureDesc[_iLocal].m_pName;
#endif	
}

int CTextureContainer_VirtualXTC2::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetLocal, 0);
	int nTxt = m_lTextureDesc.Len();
#ifdef USE_HASHED_TEXTURENAME
	uint32 NameID = StringToHash(_pName);
	for (int iTxt=0; iTxt<nTxt; iTxt++)
	{
		if (m_lspTextures[iTxt]->m_NameID == NameID)
			return iTxt;
	}
#else
	for(int iTxt = 0; iTxt < nTxt; iTxt++)
	{
		if (0 == CStrBase::CompareNoCase(m_lTextureDesc[iTxt].m_pName, _pName))
			return iTxt;
	}
#endif		
	return -1;
}

int CTextureContainer_VirtualXTC2::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTextureID, 0);
	return m_lTextureDesc[_iLocal].m_TextureID;
}

int CTextureContainer_VirtualXTC2::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTextureDesc, 0);
	const CTextureDesc& Desc = m_lTextureDesc[_iLocal];
	if (_pTargetImg)
		_pTargetImg->CreateVirtual_NoDelete(Desc.GetWidth(), Desc.GetHeight(), Desc.m_Format, Desc.m_MemModel, 0);
//		_pTargetImg->CreateVirtual(Desc.GetWidth(), Desc.GetHeight(), Desc.m_Format, Desc.m_MemModel);
	_Ret_nMipmaps = Desc.m_nMipMaps;
	return CTC_TEXTURE_ACCESS;
}

void CTextureContainer_VirtualXTC2::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetTextureProperties, MAUTOSTRIP_VOID);
	const CTextureDesc& Desc = m_lTextureDesc[_iLocal];
	_Properties = Desc.m_Properties;
}

int CTextureContainer_VirtualXTC2::GetFirstMipmapLevel(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_GetFirstMipmapLevel, MAUTOSTRIP_VOID);
	const CTextureDesc& Desc = m_lTextureDesc[_iLocal];
	return Desc.m_iPicMip;
}

void CTextureContainer_VirtualXTC2::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
{
	MAUTOSTRIP(CTextureContainer_VirtualXTC2_BuildInto, MAUTOSTRIP_VOID);
	for(int i = 0; i < _nMipmaps; i++)
	{
		int iMip = i + _iStartMip;
		CImage* pImg = GetTexture(_iLocal, iMip, _TextureVersion);
		if (!pImg)
			Error("BuildInto", "GetTexture failed.");

		CImage Tmp;
		if (pImg->IsCompressed())
		{
			pImg->Decompress(&Tmp);
			pImg = &Tmp;
		}

		CImage::Convert(pImg, _ppImg[i], _ConvertType);

		ReleaseTexture(_iLocal, iMip, _TextureVersion);
	}
}

int CTextureContainer_VirtualXTC2::EnumTextureVersions(int _iLocal, uint8* _pDest, int _nMaxVersions)
{
	int nVersions = 0;
	int TexID = m_lTextureDesc[_iLocal].m_TextureID;
	int iLocal = _iLocal;
	int nLocal = m_lTextureDesc.Len();
	while((iLocal < nLocal) && (nVersions < _nMaxVersions) && (m_lTextureDesc[iLocal].m_TextureID == TexID))
	{
		if(_pDest) _pDest[nVersions]	= m_lTextureDesc[iLocal].m_Properties.m_TextureVersion;

		nVersions++;
		iLocal++;
	}

	return nVersions;
}
