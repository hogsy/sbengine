#ifndef _INC_MTextureContainers
#define _INC_MTextureContainers

#include "../SysInc.h"
#include "MTexture.h"

// #define MERGE_NORMAL_SPECULAR

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTexture;
typedef TPtr<CTexture> spCTexture;
typedef TArray<spCTexture> lspCTexture;

class SYSTEMDLLEXPORT CTexture : public CReferenceCount
{
	MRTC_DECLARE;

public:

	DECLARE_OPERATOR_NEW


	static int GetMipMapLevels(int _w, int _h, int _MinSize = 1);

	int32 m_TextureID;			// Usually needed =)
	int32 m_iPalette;			// Index in external palette, -1 if not used.
	uint32 m_PaletteFilePos;
	spCImagePalette m_spPal;
#ifdef USE_HASHED_TEXTURENAME
	uint32 m_NameID;
#else		
	char m_Name[32];
#endif	
	int32 m_nMipmaps;			// Includes the largest map which isn't in m_lspMaps
	spCImage m_lspMaps[CTC_MAXTEXTURESIZESHIFT];		// m_lspMaps[0] will always point to NULL (m_LargestMap)
	uint32 m_lMapFilePos[CTC_MAXTEXTURESIZESHIFT];
	uint32 m_MipMapFilePosPos;
	int16 m_IsLoaded;
	CTC_TextureProperties m_Properties;

	uint32 m_LargestMapFilePos;
	CImage m_LargestMap;

	CTexture();
	spCTexture Duplicate();

	dllvirtual void SetPalette(spCImagePalette _spPal, int _iPal = -1);
	dllvirtual bool IsPalettized() const;

#ifndef PLATFORM_CONSOLE
	dllvirtual void Create(spCImage _spImg, int _nMipmaps = 1, int _DestFormat = -1, int _ImageConvertType = -1);
	dllvirtual void Write(CDataFile* _pDFile);
	dllvirtual void Write2(CDataFile* _pDFile, int32 *_PicMip);
	dllvirtual void WriteIndexData(CCFile* _pFile);
	dllvirtual void WriteIndexData2(CCFile* _pFile, int32 *_PicMip);
	dllvirtual void Compress(int _Compression, fp32 _Quality);
	dllvirtual void Decompress(bool _DecompMipmap = true);

	dllvirtual void SerializeWrite(CDataFile* _pDFile);
	dllvirtual void SerializeWrite(CDataFile* _pDFile, int32 _PicMip );
	dllvirtual void SerializeRead(CDataFile* _pDFile);
#endif

	dllvirtual void Read(CDataFile* _pDFile, TArray<spCImagePalette>* _plspPalettes = NULL, int _iPalBase = 0);
	dllvirtual void ReadIndexData(CCFile* _pFile, TArray<spCImagePalette>* _plspPalettes = NULL, int _iPalBase = 0);
	dllvirtual bool IsCompressed();

	dllvirtual int Virtual_IsLoaded(int _iMipMap) { return ((m_IsLoaded>>_iMipMap) & 1); };
	dllvirtual void Virtual_Read(CDataFile* _pDFile, TArray<spCImagePalette>* _plspPalettes = NULL, int _iPalBase = 0);
	dllvirtual void Virtual_Load(CCFile* _pFile);
	dllvirtual void Virtual_Load(CCFile* _pFile, int _iMipmap);
	dllvirtual void Virtual_Unload();
	dllvirtual void Virtual_Unload(int _iMipmap);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Plain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_Plain;
typedef TPtr<CTextureContainer_Plain> spCTextureContainer_Plain;

// -------------------------------------------------------------------
enum
{
	XTX_COMPILE_QUALITY_LQ	= 1,
	XTX_COMPILE_QUALITY_HQ	= 2,
	XTX_COMPILE_QUALITY_HQNP= 3,
	XTX_COMPILE_QUALITY_MASK= 3,

	XTX_COMPILE_ORG			= 4,
	XTX_COMPILE_RENDER		= 8,
	XTX_COMPILE_THUMBNAILS	= 16,
};

class SYSTEMDLLEXPORT CTextureContainer_Plain : public CTextureContainer
{
	MRTC_DECLARE;

public:
	class CXTXCompileResult
	{
	public:
		spCTextureContainer_Plain m_spTCThumb;
	};

protected:
	TArray<spCImagePalette> m_lspPalettes;
	TArray<spCTexture> m_lspTextures;
	TArray<uint32>	m_lTextureCompileChecksums;

//	spCStringHash m_spHash;
//	CStringHash m_Hash;
	CStr m_Source;
	CStr m_ContainerName;

public:
	class CTC_CompileInfo
	{
	public:
		CTC_CompileInfo()
		{
			m_GenLod = 0;
		}

		CTC_CompileInfo(CStr _Path)
		{
			m_Path = _Path;
			m_GenLod = 0;
		}

		CStr m_Path;
		CTC_TextureProperties m_Properties;
		int m_GenLod;
	};

	
	class CTC_PostFilterParams
	{
	public:
		spCRegistry	m_spFilters;
	};

	class CTC_CubeFilterParams
	{
	public:
		int		m_Type;
		float	m_aParams[4];
	};


	DECLARE_OPERATOR_NEW

	CTextureContainer_Plain();
	~CTextureContainer_Plain();

//	virtual void DestroyHash();
//	virtual void InsertHash(int _Texture);

	dllvirtual CStr GetContainerName() { return m_ContainerName; };
	dllvirtual const char *GetContainerSortName() {return m_ContainerName;};

	// IO
	dllvirtual int AddTexture(spCTexture _spTxt);
#ifndef PLATFORM_CONSOLE
	dllvirtual int AddTexture(spCImage _spImg, const CTC_TextureProperties& _Properties, int _nMipmaps = 1, CStr _Name = (char*)NULL);
	dllvirtual int AddTexture(CStr _FileName, const CTC_TextureProperties& _Properties, CStr _Name = (char*)NULL);
	dllvirtual int AddTexture(spCImage _spImg, const CTC_TextureProperties& _Properties, CStr _Name, int _ConvertType = 0, void* _pConvertParam = NULL);
	dllvirtual int AddTexture(CStr _FileName, const CTC_TextureProperties& _Properties, CStr _Name, int _ConvertType = 0, void* _pConvertParam = NULL);
	dllvirtual void SetTexture(int _iLocal, spCImage _spImg, const CTC_TextureProperties& _Properties, int _ConvertType = 0, void* _pConvertParam = NULL);
	dllvirtual void CreateTextureImage(spCTexture _spTxt, spCImage _spImg, int _ConvertType = 0, void* _pConvertParam = NULL);
	dllvirtual void DeleteTexture(int _LocalID);
	dllvirtual void AddFiltered(CTextureContainer_Plain *_pSrcXTC, TArray<CStr> *_pAllowed);
	dllvirtual void Add(CTextureContainer_Plain* _pDestXTC);
	dllvirtual int AddFromScriptLine(CStr _Line, CStr _ScriptPath);
	dllvirtual int AddFromKeys(CKeyContainer* _pKeys, CStr _BasePath);
	dllvirtual int AddFromScript(CStr _FileName);
	dllvirtual void AddFromWAD(CStr _FileName);

	dllvirtual void WriteImageList(CDataFile* _pDFile);
	dllvirtual void WriteImageDirectory(CDataFile* _pDFile);

	dllvirtual void WriteImageList2(CDataFile* _pDFile, int32 *_PicMip);
	dllvirtual void WriteImageDirectory2(CDataFile* _pDFile, int32 *_PicMip);

	dllvirtual void WriteXTC(CDataFile* _pDFile, bool _bWriteList = true);
	dllvirtual void WriteXTC(const char* _pName);
	dllvirtual void WriteXTC2(CDataFile* _pDFile, int32 *_PicMip);
	dllvirtual void WriteXTC2(const char* _pName, int32 *_PicMip);
	dllvirtual void WriteXTC2_XT0(const char* _pName, int32 *_PicMip);

	dllvirtual void WriteWAD(CStr _FileName);
	dllvirtual void FilterTextures(TArray<CStr> *_pValidTextures);
	dllvirtual void ScaleToPow2();
	dllvirtual void StripVersions(const uint8* _pKeepVersions, int _nVersions);
	dllvirtual void RemoveTextureVersion(CStr _Name, uint8 _Version);

	dllvirtual int GetNumRealLocal()
	{
		return m_lspTextures.Len();
	}
	dllvirtual CTexture* GetRealLocal(int _iLocal)
	{
		return m_lspTextures[_iLocal];
	}
	

	dllvirtual	void createDualParaboloidMaps( void );
	dllvirtual	void swizzleTexturesForGC( void );
	dllvirtual	void swizzleTexturesForPS2( void );
	// -------------------------------------------------------------------
	// XTX compile

	dllvirtual void XTXCompile(int _Flags, const CRegistry &_Reg, CStr _SourcePath, CXTXCompileResult& _Result);
	dllvirtual void XTXUpdate(int _Flags, const CRegistry &_Reg, CStr _ContainerName, CStr _SourcePath, CXTXCompileResult& _Result);

	// Operations
	dllvirtual void Quantize(spCImagePalette _spPal = NULL);

	dllvirtual void Compress(int _Compression, fp32 _Quality);
	dllvirtual void Decompress();
	dllvirtual void Recompress( int _FromFormat, int _ToFormat, fp32 _Quality );	// Compress all textures with given format

	dllvirtual spCTextureContainer_Plain CreateSubContainer(uint8* _pTxtAddFlags, int _FlagListLen);
	dllvirtual spCTextureContainer_Plain CreateSubContainer(uint32* _piTxtAdd, int _nTextures);

	dllvirtual void FilterCubemap(int _iTexture, const CTC_CubeFilterParams& _Params);
	dllvirtual void FilterTexture(int _iTexture, const CTC_PostFilterParams& _Params);
	dllvirtual void SmoothCubemapEdges(int _iTexture);
	dllvirtual void swizzleTexturesForXBOX( void );

	dllvirtual void ConvertTextures( uint32 _SupportedFormats, uint32 _compression, float _quality);

#ifdef MERGE_NORMAL_SPECULAR
	dllvirtual spCImage MergeNormalSpecMaps(CImage* _pNMap, CImage* _pSMap); // Could be static
#endif

#endif

	dllvirtual int AddFromImageList(CDataFile* _pDFile);
	dllvirtual void AddFromXTC(const char* _pName);
	dllvirtual void AddFromXTC(CDataFile* _pDFile);

	dllvirtual void SetSource(CStr _Source);
	dllvirtual CStr GetSource();

	dllvirtual int GetNumTextures();
	virtual spCTexture GetTextureMap(int _iLocal, int _TextureVersion, bool _bForceLoaded = true);

	dllvirtual int EnumTextureVersions(int _iLocal, uint8* _pDestVersion, int _nMaxVersions);

	dllvirtual void AddCompileChecksum(uint32 _Checksum);
	dllvirtual uint32 GetCompileChecksum(int _iLocal);

	// CTextureContainer interface
	dllvirtual void Clear(bool _FreeIDs = true);
	dllvirtual int GetNumLocal();
	dllvirtual CStr GetName(int _iLocal);
	dllvirtual int GetLocal(const char* _pName);
	dllvirtual int GetTextureID(int _iLocal);
	dllvirtual void SetTextureParam(int _iLocal, int _Param, int _Value);
	dllvirtual int GetTextureParam(int _iLocal, int _Param);
	dllvirtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	dllvirtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
	dllvirtual CImage* GetTexture(int _iLocal, int _iMipMap, int _TextureVersion);
	dllvirtual void ReleaseTexture(int _iLocal, int _iMipMap);
	dllvirtual void ReleaseTextureAllMipmaps(int _iLocal);
	dllvirtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);

	dllvirtual int GetCorrectLocal(int _iLocal, int _TextureVersion);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_VirtualXTC
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if 0
class CTextureCache_Entry
{
	int m_Texture;
	int m_MipMap;
public:
	CTextureCache_Entry();
	CTextureCache_Entry(int _Texture, int _MipMap);

	bool operator==(CTextureCache_Entry _e);
	CTextureCache_Entry operator=(CTextureCache_Entry _e);

	int GetTexture(){ return m_Texture; };
	int GetMipMap(){ return m_MipMap; };
};
#endif

// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CTextureContainer_VirtualXTC : public CTextureContainer_Plain
{
	MRTC_DECLARE;
	friend class CTextureCache;

protected:
//	TArray<CTextureCache_Entry> m_liCached;
	CStr m_FileName;
	CStr m_CacheFileName;
	spCCFile m_spPrecacheFile;
//	bool m_bIsCached;

	dllvirtual void ScanImageList(CDataFile* _pDFile);
	dllvirtual void Unload(int _iLocal);
	dllvirtual void Unload(int _iLocal, int _iMipMap);
	dllvirtual void Load(int _iLocal);
	dllvirtual void Load(int _iLocal, int _iMipMap, int _nMips = 1);

	dllvirtual CStr GetFileName();	// If caching is enabled, GetFileName invokes cache copying if the file is not in the cache. Returns the cached filename.

public:
	DECLARE_OPERATOR_NEW

	dllvirtual void Create(CDataFile* _pDFile, CStr _FileName = CStr(), int _NumCache = 8);
	dllvirtual void Create(CStr _FileName, int _NumCache = 8);
//	dllvirtual void SetCacheFile(CStr _CacheFileName);
	dllvirtual void AddFromXTC(const char* _pName);
	CTextureContainer_VirtualXTC();
	~CTextureContainer_VirtualXTC();

	// Info
	dllvirtual int GetNumTextures();
	virtual spCTexture GetTextureMap(int _iLocal, int _TextureVersion, bool _bForceLoaded = true);

	// CTextureContainer interface
	dllvirtual void Clear();
	dllvirtual int GetLocal(const char* _pName);
	dllvirtual int GetTextureID(int _iLocal);
	dllvirtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	dllvirtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
	dllvirtual CImage* GetTexture(int _iLocal, int _iMipMap, int _TextureVersion);
	dllvirtual CImage* GetTextureNumMip(int _iLocal, int _iMipMap, int _nMips, int _TextureVersion);
	dllvirtual void ReleaseTexture(int _iLocal, int _iMipMap, int _TextureVersion);
	dllvirtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
	dllvirtual void OpenPrecache();
	dllvirtual void ClosePrecache();
};

typedef TPtr<CTextureContainer_VirtualXTC> spCTextureContainer_VirtualXTC;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Video
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_Video : public CTextureContainer
{
	MRTC_DECLARE;
public:
	virtual void CreateFromDirectory(CStr _Path) pure;
	virtual int AddVideo(CStr _FileName) pure;

	virtual void CloseVideo(int _iLocal) pure;
	virtual void SetVolume(int _iLocal, fp32 fpVol) pure;
	virtual void Pause(int _iLocal, bool _Paused = true) pure;
	virtual void AutoRestart(int _iLocal, bool _EnableAutoRestart = true) pure;
	virtual void Rewind(int _iLocal) pure;
	virtual bool IsOnLastFrame(int _iLocal) pure;
	virtual bool MoveToLastFrame(int _iLocal) pure;
	virtual int GetFrame(int _iLocal) pure;
	virtual int GetNumFrames(int _iLocal) pure;
	virtual int GetWidth(int _iLocal) pure;
	virtual int GetHeight(int _iLocal) pure;
	virtual void SetFrame(int _iLocal, int _Frame) {}
	virtual fp32 GetTime(int _iLocal) pure;
	virtual void SetSoundHandle(int _iLocal, int _hSound) pure;

	virtual void GlobalPause() {}
	virtual void GlobalResume() {}
};


#endif // _INC_MTextureContainers
