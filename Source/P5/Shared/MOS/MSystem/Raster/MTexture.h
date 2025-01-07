
#ifndef _INC_MTexture
#define _INC_MTexture

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Texture management
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios 1997, 2001
					
	Contents:		CTextureContainer, virtual base class.
					CTextureContext
					
	Comments:		The texture context keeps track of texture names (IDs) within an 
					application and their 'dirty' status for each render context.
					
	History:		
		970328:		Created File
					ID/Index allocator
					Texture manager
					Abstract texture class

		000610		Added PicMip nr and compress option.

		010523		ReleaseTexture function to use in conjunction with GetTexture to signal
					end of image usage.


\*____________________________________________________________________________________________*/

#include "MRTC.h"
#include "MImage.h"

class CTextureContext;
class CRegistry;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContext/CTextureContainer enumerators
|__________________________________________________________________________________________________
\*************************************************************************************************/

//----------------------------------------------------------------
enum
{
	CTC_TEXTURE_NOWRAP				= 1,	// Texture doesn't need to be wrappable when used.
	CTC_TEXTURE_ACCESS				= 2,	// CImage* GetTexture(...) supported.
	CTC_MAXTEXTURESIZE				= 2048,
	CTC_MAXTEXTURESIZESHIFT			= 12,

	//----------------------------------------------------------------
	CTC_TXTIDINFO_CLASSMASK			= 0x0fff,
	CTC_TXTIDINFO_FLAGSMASK			= 0xf000,
	CTC_TXTIDINFO_FLAGSSHIFT		= 12,

	CTC_TXTIDFLAGS_PRECACHE			= 1,
	CTC_TXTIDFLAGS_ALLOCATED		= 2,
	CTC_TXTIDFLAGS_RESIDENT			= 4,
#ifdef PLATFORM_CONSOLE
	CTC_TXTIDFLAGS_WASDELETED		= 8,
#else
	CTC_TXTIDFLAGS_USED				= 8,
	CTC_TXTIDFLAGS_WASDELETED		= 0,
#endif

	//----------------------------------------------------------------
	CTC_TEXTUREPARAM_PICMIPINDEX	= 0,
	CTC_TEXTUREPARAM_FLAGS			= 1,	// Flags |= Value;
	CTC_TEXTUREPARAM_CLEARFLAGS		= 2,	// Flags &= ~Value;
};

//----------------------------------------------------------------
class SYSTEMDLLEXPORT CTC_TxtIDInfo
{
	int16 m_Stuff;				// Reg. texture constructor class.

public:
	uint16 m_iLocal;			// TextureClass local texture index

	CTC_TxtIDInfo();

	int GetTxtClass();
	void SetTxtClass(int _iClass);
	int GetFlags();
	void SetFlags(int _Flags);
};

//----------------------------------------------------------------
enum
{
	// Current version
	CTC_PROPERTIES_VERSION = 0x0103,

	// Flags
	CTC_TEXTUREFLAGS_NOMIPMAP = DBit(0),
	CTC_TEXTUREFLAGS_NOPICMIP = DBit(1),
	CTC_TEXTUREFLAGS_CLAMP_U = DBit(2),
	CTC_TEXTUREFLAGS_CLAMP_V = DBit(3),
	CTC_TEXTUREFLAGS_HIGHQUALITY = DBit(4),
	CTC_TEXTUREFLAGS_NOCOMPRESS = DBit(5),
	CTC_TEXTUREFLAGS_RENDER = DBit(6),
	CTC_TEXTUREFLAGS_CUBEMAP = DBit(7),			// This texture is used on all 6 faces of the cube

	CTC_TEXTUREFLAGS_CUBEMAPCHAIN = DBit(8),	// This and the 5 following textures make up a cubemap
	CTC_TEXTUREFLAGS_PROCEDURAL = DBit(9),		// 
	CTC_TEXTUREFLAGS_NORMALMAP = DBit(10),
	CTC_TEXTUREFLAGS_BACKBUFFER = DBit(11),
	CTC_TEXTUREFLAGS_BACKBUFFERDISCARDOLD = DBit(12),
	CTC_TEXTUREFLAGS_PALETTE = DBit(13),
	CTC_TEXTUREFLAGS_DISCARDABLE = DBit(14),
	CTC_TEXTUREFLAGS_NOSHARPEN = DBit(15),
	CTC_TEXTUREFLAGS_BORDERCOLOR_U = DBit(16),
	CTC_TEXTUREFLAGS_BORDERCOLOR_V = DBit(17),

	CTC_TEXTUREFLAGS_DOUBLEBUFFER = DBit(18),
	CTC_TEXTUREFLAGS_RENDEROLDBUFFER = DBit(19),
	CTC_TEXTUREFLAGS_CONTINUETILING = DBit(20),
	CTC_TEXTUREFLAGS_RENDERDISABLEZBUFFER = DBit(21),
	CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT = DBit(22),
	CTC_TEXTUREFLAGS_CLEARWHENCONTINUETILING = DBit(23),

	// Magnification filter
	CTC_MAGFILTER_DEFAULT = 0,
	CTC_MAGFILTER_NEAREST = 1,
	CTC_MAGFILTER_LINEAR = 2,
	CTC_MAGFILTER_ANISOTROPIC = 3,			// Unsupported

	// Minification filter
	CTC_MINFILTER_DEFAULT = 0,
	CTC_MINFILTER_NEAREST = 1,
	CTC_MINFILTER_LINEAR = 2,
	CTC_MINFILTER_ANISOTROPIC = 3,			// Unsupported

	// MIP-Filter
	CTC_MIPFILTER_DEFAULT = 0,
	CTC_MIPFILTER_NEAREST = 1,
	CTC_MIPFILTER_LINEAR = 2,

	CTC_TEXTUREVERSION_ANY		= 0xff,
	CTC_TEXTUREVERSION_RAW		= 0,
	CTC_TEXTUREVERSION_S3TC		= 1,
	CTC_TEXTUREVERSION_3DC		= 2,
	CTC_TEXTUREVERSION_HILO		= 3,
	CTC_TEXTUREVERSION_FLOAT	= 4,
	CTC_TEXTUREVERSION_CTX		= 5,
	CTC_TEXTUREVERSION_MAX		= 6
};

#ifdef PLATFORM_DOLPHIN
# define USE_PACKED_TEXTUREPROPERTIES
#endif

//----------------------------------------------------------------
class SYSTEMDLLEXPORT CTC_TextureProperties
{
public:
 #ifdef USE_PACKED_TEXTUREPROPERTIES
	uint32 m_Flags        : 16;
	uint32 m_MagFilter    : 2;
	uint32 m_MinFilter    : 2;
	uint32 m_MIPFilter    : 2;
	uint32 m_iPicMipGroup : 4;
	uint32 m_Padd__       : 6;
 #else
	uint32 m_Flags;
	uint8 m_MagFilter;
	uint8 m_MinFilter;
	uint8 m_MIPFilter;
	uint8 m_MIPMapLODBias;
	uint8 m_Anisotropy;					// No render context support this atm.
	uint8 m_iPicMipGroup;
	uint8 m_PicMipOffset;
	uint8 m_TextureVersion;
	uint32 m_Padd2__;
 #endif	

	static const char* ms_TxtPropFlagsTranslate[];
	static const char* ms_TxtPropFilterTranslate[];
	static const char* ms_TxtPropMIPTranslate[];
	static const char* ms_TxtConvertTranslate[];

	CTC_TextureProperties()
	{
		m_Flags = 0;
		m_MagFilter = CTC_MAGFILTER_DEFAULT;
		m_MinFilter = CTC_MINFILTER_DEFAULT;
		m_MIPFilter = CTC_MIPFILTER_DEFAULT;
		m_iPicMipGroup = 0;
	#ifndef USE_PACKED_TEXTUREPROPERTIES
		m_MIPMapLODBias = 0;
		m_Anisotropy = 0;
		m_PicMipOffset = 0;
		m_TextureVersion = CTC_TEXTUREVERSION_RAW;
		m_Padd2__ = 0;
	#endif
	}

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;

	bool operator == (const CTC_TextureProperties& _Other) const
	{
		return (m_Flags == _Other.m_Flags)
			&& (m_MagFilter == _Other.m_MagFilter)
			&& (m_MinFilter == _Other.m_MinFilter)
			&& (m_MIPFilter == _Other.m_MIPFilter)
			&& (m_MIPMapLODBias == _Other.m_MIPMapLODBias)
			&& (m_Anisotropy == _Other.m_Anisotropy)
			&& (m_iPicMipGroup == _Other.m_iPicMipGroup)
			&& (m_PicMipOffset == _Other.m_PicMipOffset)
			&& (m_TextureVersion == _Other.m_TextureVersion);
	}
	bool Parse_XRG(const CRegistry &_Reg);
	void Get_XRG(CRegistry &_Reg, int _DefaultPicmip = 0);

	int GetPicMipOffset() const { return (int8)m_PicMipOffset; };
	
	CStr GetFlagsString();
	CStr GetMagFilterString();
	CStr GetMinFilterString();
	CStr GetMIPFilterString();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum 
{
	ETCBuildFlags_NewTexture = DBit(0),
};
class SYSTEMDLLEXPORT CTextureContainer : public CReferenceCount
{
protected:
	int m_iTextureClass;
	CTextureContext* m_pTC;
	CTextureContainer* m_pRefreshNext;
	CTextureContainer* m_pRefreshPrev;
	NThread::CMutual m_Lock;

public:
	MACRO_OPERATOR_TPTR(CTextureContainer)
	void MRTC_Delete();


	CTextureContainer();
	~CTextureContainer();

	CTextureContext* GetTextureContext() { return m_pTC; };

	virtual void Create(void* _pContext) {};
	virtual void OnRefresh();

	virtual void ClearCache() {}

	virtual CStr GetContainerName() { return CStrF("TxtClass %d", m_iTextureClass); };
	virtual const char *GetContainerSortName() {return "";}; // Must return an unique name if any file IO is done
	virtual int GetNumLocal() pure;
	virtual CStr GetName(int _iLocal) { return CStrF("TxtClass %d, Local %d", m_iTextureClass, _iLocal); };
	virtual int GetLocal(const char* _pName) { return -1; };
	virtual int GetTextureID(int _iLocal) pure;
	virtual void SetTextureParam(int _iLocal, int _Param, int _Value);
	virtual int GetTextureParam(int _iLocal, int _Param);
	virtual void SetTextureParamfv(int _iLocal, int _Param, const fp4* _pValues);
	virtual void GetTextureParamfv(int _iLocal, int _Param, fp4* _pRetValues);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps) pure;
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
	virtual int GetFirstMipmapLevel(int _iLocal) { return 0; }
	virtual int EnumTextureVersions(int _iLocal, uint8* _pDestVersion, int _nMaxVersions);

	virtual CImage* GetTexture(int _iLocal, int _iMipMap, int _TextureVersion) { return NULL; };
	virtual CImage* GetTextureNumMip(int _iLocal, int _iMipMap, int _nMips, int _TextureVersion) { return GetTexture(_iLocal, _iMipMap, _TextureVersion); };
	virtual CImage* GetVirtualTexture(int _iLocal, int _iMipMap, int _nVirtual, int _TextureVersion) { return GetTexture(_iLocal, _iMipMap, _TextureVersion); };
	virtual void ReleaseTexture(int _iLocal, int _iMipMap, int _TextureVersion) {};
	virtual void ReleaseTextureAllMipmaps(int _iLocal, int _TextureVersion) {};
	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0) pure;
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC) {};

	M_INLINE MRTC_CriticalSection& GetLock() { return m_Lock; }

	friend class CTextureContext;
};

typedef TPtr<CTextureContainer> spCTextureContainer;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CRenderContext;

class SYSTEMDLLEXPORT CTextureContext : public CReferenceCount
{
	MRTC_DECLARE;

protected:
	TList_Vector<CTextureContainer*> m_lpTC;
	TList_Vector<CTC_TxtIDInfo> m_lTxtIDInfo;
	int m_IDCapacity;
	CIDHeap m_TIHeap;

	TList_Vector<CRenderContext*> m_lpRC;

	int AllocRCID(int _iRC, int _tnr);
	void FreeRCID(int _iRC, int _ID);

	CTextureContainer* m_pRefreshFirst;

#ifdef USE_HASHED_TEXTURENAME
	class CIDHash* m_pHash;
#else
	CStringHashConst m_Hash;
#endif	
	
public:

	NThread::CMutual m_DeleteContainerLock;

	DECLARE_OPERATOR_NEW


	CTextureContext();
	~CTextureContext();

	void LogUsed(CStr _FileName);
	void ClearUsage();

	virtual void Create(int _IDCapacity, int _BufferSize);

	virtual int GetIDCapacity() { return m_IDCapacity; };
	virtual int AllocID(int _iTC, int _iLocal, const char* _pName);		// Caller assert that _pName will remain valid until FreeID() has been called for the same ID.
	virtual int AllocID(int _iTC, int _iLocal, uint32 _NameID);			// _NameID should be created from the texture name (with StringToHash())
	virtual void FreeID(int _ID);
	virtual bool IsValidID(int _ID);
	virtual int GetTextureFlags(int _ID);
	virtual void SetTextureParam(int _ID, int _Param, int _Value);
	virtual int GetTextureParam(int _ID, int _Param);
	virtual void SetTextureParamfv(int _ID, int _Param, const fp4* _pValues);
	virtual void GetTextureParamfv(int _ID, int _Param, fp4* _pRetValues);
	virtual int GetTextureDesc(int _ID, CImage* _pTextureDesc, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _ID, CTC_TextureProperties&);
	virtual int GetTextureID(const char* _pTxtName);
	virtual int GetTextureID(uint32 _TxtNameID);
	virtual CStr GetName(int _ID);

	virtual int GetLocal(int _ID);
	virtual CTextureContainer* GetTextureContainer(int _ID);

	virtual CImage* GetTexture(int _ID, int _iMipMap, int _TextureVersion);		// Gain direct access to texture mipmap. Only supported when a texture has the CTC_TEXTURE_ACCESS flag.
	virtual void ReleaseTexture(int _ID, int _iMipMap, int _TextureVersion);		// Signal that a direct access texture is no longer used so that the system may unload the texture.
	virtual void ReleaseTextureAllMipmaps(int _ID, int _TextureVersion);		// Signal that a direct access texture is no longer used so that the system may unload the texture.

	virtual int EnumTextureVersions(int _ID, uint8* _pDest, int _nMax);

	virtual void BuildInto(int _ID, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
	virtual void BuildInto(int _ID, class CRenderContext* _pRC);
	virtual void MakeDirty(int _ID);
	virtual void MakeDirtyIndirectInterleaved(uint16* _piID, int32* _pID, int _nIDs, int _IDOffset);
	virtual void MakeDirty(uint16* _pID, int _nIDs);
	virtual void MakeDirty(uint32* _pID, int _nIDs);

	virtual void Refresh();

	virtual void ClearCache();

	virtual int AddRenderContext(CRenderContext* _pRC);
	virtual void RemoveRenderContext(int _iRC);

	virtual int AddTextureClass(CTextureContainer* _pTClass);
	virtual void RemoveTextureClass(int _iTClass);
	virtual void EnableTextureClassRefresh(int _iTClass);
	virtual void DisableTextureClassRefresh(int _iTClass);
	
	virtual int GetNumTC();
};



class CPrecacheTextureCompare
{
public:
	static int Compare(CTextureContext* _pContext, uint16 _i0, uint16 _i1)
	{
		MAUTOSTRIP(RegistryCompare, 0);

		CTextureContainer *pTCFirst = _pContext->GetTextureContainer(_i0);
		CTextureContainer *pTCSecond = _pContext->GetTextureContainer(_i1);
		if (pTCFirst != pTCSecond)
		{
			const char *pFirstStr = pTCFirst->GetContainerSortName();
			const char *pSecondStr = pTCSecond->GetContainerSortName();
			if (pFirstStr != pSecondStr)
			{
				int iCmp = CStrBase::stricmp(pFirstStr, pSecondStr);
				if (iCmp != 0)
					return iCmp;
			}
		}

		int iLocalFirst = _pContext->GetLocal(_i0);
		int iLocalSecond = _pContext->GetLocal(_i1);
		if (iLocalFirst > iLocalSecond)
			return 1;
		else if (iLocalFirst < iLocalSecond)
			return -1;

		return 0;
	}
};


#define MACRO_GetTextureContext MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT"); if (!pTC) M_BREAKPOINT

typedef TPtr<CTextureContext> spCTextureContext;

// -------------------------------------------------------------------
#endif // _INC_MTexture
