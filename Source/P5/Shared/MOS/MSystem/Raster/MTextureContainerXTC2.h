/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texturecontainer for contentcompiled textures.
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MTextureContainerXTC2
#define _INC_MTextureContainerXTC2

#include "MTexture.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_VirtualXTC2
|__________________________________________________________________________________________________
\*************************************************************************************************/
class SYSTEMDLLEXPORT CTextureContainer_VirtualXTC2 : public CTextureContainer
{
	MRTC_DECLARE;

public:

	DECLARE_OPERATOR_NEW


	class CTextureDesc
	{
	public:
#if defined(USE_PACKED_TEXTUREPROPERTIES)
		CTC_TextureProperties m_Properties; // 4
	#ifdef USE_HASHED_TEXTURENAME
		uint32 m_NameID;                    // 4
	#else
		char*  m_pName;
	#endif
		uint32 m_Format;                    // 4
		uint32 m_MemModel;                  // 4 
		uint32 m_TextureDataFilePos;        // 4
		int32  m_PaletteFileOffset : 24;    // 3 (offset from m_TextureDataFilePos)
		int32  m_iPalette          : 8;     // 1
		uint16 m_TextureID;					// 2
		uint16 m_Width;						// 2
		uint16 m_Height;					// 2
		uint8  m_nMipMaps : 4;              // 0.5
		uint8  m_iPicMip  : 4;              // 0.5
		uint8  m_Pad0;						// 1.0  (total: 32 bytes)

		int GetPaletteFilePos() const { return m_TextureDataFilePos + m_PaletteFileOffset; }
#else
		CTC_TextureProperties m_Properties;	// 16
		uint32 m_PaletteFilePos;			// 4
		int8 m_iPalette;					// 1
		uint8 m_nMipMaps:4;					// 0.5
		uint8 m_iPicMip:4;					// 0.5
		uint16 m_Width;						// 2
		uint16 m_Height;					// 2
//		uint8 m_iFormat;					// 1
//		uint8 m_iMemModel;					// 1
		uint32 m_Format;					// 4
		uint32 m_MemModel;					// 4
		uint32 m_TextureID;					// 4
	#ifdef USE_HASHED_TEXTURENAME
		uint32 m_NameID;
	#else		
		char* m_pName;						// 4
	#endif		
		uint32 m_TextureDataFilePos;		// 4  (total: 46 bytes)
		uint32 m_TextureXT0FilePos;			// 4  (total: 52 bytes)

		int GetPaletteFilePos() const { return m_PaletteFilePos; }
#endif

		int GetWidth() const { return m_Width; };
		int GetHeight() const { return m_Height; };

		CTextureDesc()
		{
			FillChar(this, sizeof(CTextureDesc), 0);
		}
	};

	class CTextureImages : public CReferenceCount
	{
	public:
		CTextureImages()
		{
			m_MipMapLoadMask = 0;
			m_MipMapAccessMask = 0;
		}

		uint16 m_iLocal;							// Max textures per container is hereby 65536.
		uint16 m_MipMapLoadMask;
		uint16 m_MipMapAccessMask;
		CTextureDesc m_Desc;
		spCImagePalette m_spPalette;
		uint32 m_lMipMapFilePos[CTC_MAXTEXTURESIZESHIFT];	// 48
		CImage m_lMipMaps[CTC_MAXTEXTURESIZESHIFT];
	};

	typedef TPtr<CTextureImages> spCTextureImages;

	CStr m_FileName;
	CStr m_CacheFileName;
	CStr m_ContainerName;
	uint32 m_bIsCached : 1;
	uint32 m_bHasXT0 : 1;

	CCFile m_XT0File;

	TThinArray<spCImagePalette> m_lspPalettes;
	TThinArray<CTextureDesc> m_lTextureDesc;
	TThinArray<char> m_lTextureNameHeap;

	spCTextureImages m_spTempTexture;

	MACRO_OPERATOR_TPTR(CTextureContainer_VirtualXTC2)

	CTextureContainer_VirtualXTC2();
	~CTextureContainer_VirtualXTC2();

	CStr GetContainerName() { return m_ContainerName; };
	const char *GetContainerSortName() {return m_ContainerName;};

	void ReadImageDirectory(CDataFile* _pDFile);
	void ReadTexture(int _iLocal, CTextureImages* _pTexture, int _iMipMapStart, int _iMipMapEnd, int _nVirtual);
	CImage* GetTextureMipMap(int _iLocal, int _iMipMap, int _nMips, int _nVirtual);

	int EnumTextureVersions(int _iLocal, uint8* _pDest, int _nMaxVersions);

	virtual CImage* GetTexture(int _iLocal, int _iMipMap, int _TextureVersion);
	virtual CImage* GetTextureNumMip(int _iLocal, int _iMipMap, int _nMips, int _TextureVersion);
	virtual CImage* GetVirtualTexture(int _iLocal, int _iMipMap, int _nVirtual, int _TextureVersion);
	virtual void ReleaseTexture(int _iLocal, int _iMipMap, int _TextureVersion);
	virtual void ReleaseTextureAllMipmaps( int _iLocal, int _TextureVersion );

	virtual void Create(CDataFile* _pDFile, int _NumCache = 4);
	virtual void Create(CStr _FileName, int _NumCache = 4);
	virtual void SetCacheFile(CStr _CacheFileName);	// Set and enable cachefile.
	virtual CStr GetFileName();						// If caching is enabled, GetFileName invokes cache copying if the file is not in the cache. Returns the cached filename.

	virtual void ClearCache();

	void PostCreate();

	virtual int GetNumLocal();
	virtual CStr GetName(int _iLocal);
	virtual int GetLocal(const char* _pName);
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
	virtual int GetFirstMipmapLevel(int _iLocal);

	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC) {};

#ifndef PLATFORM_CONSOLE
	static void WriteXTCStripData(CStr _Source, CStr _Dest);
#endif
};

typedef TPtr<CTextureContainer_VirtualXTC2> spCTextureContainer_VirtualXTC2;

#endif // _INC_MTextureContainerXTC2

