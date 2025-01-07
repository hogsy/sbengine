#include "PCH.h"
#include "MRender.h"
#include "MRCCore.h"
#include "../MSystem.h"
#ifdef  PLATFORM_SHINOBI
#include "CW_Breakpoint.h"
#endif
#include "MTextureContainers.h"
#include "MTextureContainerXTC2.h"

#include "MFloat.h"

const char* CTC_TextureProperties::ms_TxtPropFlagsTranslate[] = 
{
	"nomipmap", "nopicmip", "clampu", "clampv", "hq", "nocompress", "render", "cubemap", "cubemapchain", "procedural", "normalmap", "backbuffer", "backbufferdiscardold", "palette", "discardable", "nosharpen", "borderu", "borderv", (char*)NULL
};

const char* CTC_TextureProperties::ms_TxtPropFilterTranslate[] = 
{
	"default", "nearest", "linear", "anisotropic", (char*)NULL
};

const char* CTC_TextureProperties::ms_TxtPropMIPTranslate[] = 
{
	"default", "nearest", "linear", (char*)NULL
};

const char* CTC_TextureProperties::ms_TxtConvertTranslate[] = 
{
	"none", "afromrgb", "bump", "bumpenv", "alpha", "$$$", "normalmap2", "anisotropicdirection", "cubefromcylinder", (char*)NULL
};

// ----------------------------------------------------------------
// CTC_TextureProperties
// ----------------------------------------------------------------
CStr CTC_TextureProperties::GetFlagsString()
{
	MAUTOSTRIP(CTC_TextureProperties_GetFlagsString, CStr());
	CStr Flags;
	Flags.CreateFromFlags(m_Flags, ms_TxtPropFlagsTranslate);
	return Flags;
}

CStr CTC_TextureProperties::GetMagFilterString()
{
	MAUTOSTRIP(CTC_TextureProperties_GetMagFilterString, CStr());
	CStr Filter;
	Filter.CreateFromInt(m_MagFilter, ms_TxtPropFilterTranslate);
	return Filter;
}

CStr CTC_TextureProperties::GetMinFilterString()
{
	MAUTOSTRIP(CTC_TextureProperties_GetMinFilterString, CStr());
	CStr Filter;
	Filter.CreateFromInt(m_MinFilter, ms_TxtPropFilterTranslate);
	return Filter;
}

CStr CTC_TextureProperties::GetMIPFilterString()
{
	MAUTOSTRIP(CTC_TextureProperties_GetMIPFilterString, CStr());
	CStr Filter;
	Filter.CreateFromInt(m_MIPFilter, ms_TxtPropMIPTranslate);
	return Filter;
}

bool CTC_TextureProperties::Parse_XRG(const CRegistry &_Reg)
{
	MAUTOSTRIP(CTC_TextureProperties_Parse_XRG, false);
	if(_Reg.GetThisName() == "FLAGS")
		m_Flags = CStr::TranslateFlags(_Reg.GetThisValue(), ms_TxtPropFlagsTranslate);
	
	else if(_Reg.GetThisName() == "MAGFILTER")
		m_MagFilter = CStr::TranslateInt(_Reg.GetThisValue(), ms_TxtPropFilterTranslate);

	else if(_Reg.GetThisName() == "MINFILTER")
		m_MinFilter = CStr::TranslateInt(_Reg.GetThisValue(), ms_TxtPropFilterTranslate);
	
	else if(_Reg.GetThisName() == "MIPFILTER")
		m_MIPFilter = CStr::TranslateInt(_Reg.GetThisValue(), ms_TxtPropMIPTranslate);

#ifndef USE_PACKED_TEXTUREPROPERTIES
	else if(_Reg.GetThisName() == "MIPMAPLODBIAS")
		m_MIPMapLODBias = _Reg.GetThisValuei();

	else if(_Reg.GetThisName() == "ANISOTROPY")
		m_Anisotropy = _Reg.GetThisValuei();
#endif

	else if(_Reg.GetThisName() == "PICMIP")
		m_iPicMipGroup = _Reg.GetThisValuei();

	else if(_Reg.GetThisName() == "PICMIPOFFSET" || _Reg.GetThisName() == "PICMIPOFS")
		m_PicMipOffset = _Reg.GetThisValuei();

	else
		return false;

	return true;
}

void CTC_TextureProperties::Get_XRG(CRegistry &_Reg, int _DefaultPicmip)
{
	MAUTOSTRIP(CTC_TextureProperties_Get_XRG, MAUTOSTRIP_VOID);
	if(m_Flags != 0)
		_Reg.AddKey("flags", GetFlagsString());
	
	if(m_MagFilter != CTC_MAGFILTER_DEFAULT)
		_Reg.AddKey("magfilter", GetMagFilterString());
	
	if(m_MinFilter != CTC_MINFILTER_DEFAULT)
		_Reg.AddKey("minfilter", GetMinFilterString());
	
	if(m_MIPFilter != CTC_MIPFILTER_DEFAULT)
		_Reg.AddKey("mipfilter", GetMIPFilterString());
	
#ifndef USE_PACKED_TEXTUREPROPERTIES
	if(m_MIPMapLODBias != 0)
		_Reg.AddKey("mipmaplodbias", CStrF("%i", m_MIPMapLODBias));
	
	if(m_Anisotropy != 0)
		_Reg.AddKey("anisotropy", CStrF("%i", m_Anisotropy));
#endif
	
	if(m_iPicMipGroup != _DefaultPicmip)
		_Reg.AddKey("picmip", CStrF("%i", m_iPicMipGroup));

	if(m_PicMipOffset != 0)
		_Reg.AddKeyi("picmipoffset", GetPicMipOffset());
}

// ----------------------------------------------------------------
// CTC_TxtIDInfo
// ----------------------------------------------------------------
CTC_TxtIDInfo::CTC_TxtIDInfo()
{
	MAUTOSTRIP(CTC_TxtIDInfo_ctor, MAUTOSTRIP_VOID);
	m_Stuff = 0;
	m_iLocal = 0;
}

int CTC_TxtIDInfo::GetTxtClass()
{ 
	MAUTOSTRIP(CTC_TxtIDInfo_GetTxtClass, 0);
	return m_Stuff & CTC_TXTIDINFO_CLASSMASK; 
}

void CTC_TxtIDInfo::SetTxtClass(int _iClass) 
{ 
	MAUTOSTRIP(CTC_TxtIDInfo_SetTxtClass, MAUTOSTRIP_VOID);
	m_Stuff = 
		(m_Stuff & CTC_TXTIDINFO_FLAGSMASK) + 
		(_iClass & CTC_TXTIDINFO_CLASSMASK); 
}

int CTC_TxtIDInfo::GetFlags()
{ 
	MAUTOSTRIP(CTC_TxtIDInfo_GetFlags, 0);
	return (m_Stuff & CTC_TXTIDINFO_FLAGSMASK) >> CTC_TXTIDINFO_FLAGSSHIFT; 
}

void CTC_TxtIDInfo::SetFlags(int _Flags) 
{ 
	MAUTOSTRIP(CTC_TxtIDInfo_SetFlags, MAUTOSTRIP_VOID);
	m_Stuff = 
		((_Flags << CTC_TXTIDINFO_FLAGSSHIFT) & CTC_TXTIDINFO_FLAGSMASK) + 
		(m_Stuff & CTC_TXTIDINFO_CLASSMASK); 
}

// ----------------------------------------------------------------
//  CTC_TextureProperties
// ----------------------------------------------------------------
void CTC_TextureProperties::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CTC_TextureProperties_Read, MAUTOSTRIP_VOID);
	uint16 Ver = 0;
	_pFile->ReadLE(Ver);

	switch(Ver)
	{
#ifdef USE_PACKED_TEXTUREPROPERTIES	
	case CTC_PROPERTIES_VERSION:
		{
			uint32 Tmp32;
			uint8 Tmp8;

			_pFile->ReadLE(Tmp32); M_ASSERT(Tmp32 < 65536, "Flags doesn't fit in 16 bits!");
			m_Flags = Tmp32;
			_pFile->ReadLE(Tmp8);  M_ASSERT(Tmp8 < 4, "MagFilter doesn't fit in 2 bits!");
			m_MagFilter = Tmp8;
			_pFile->ReadLE(Tmp8);  M_ASSERT(Tmp8 < 4, "MinFilter doesn't fit in 2 bits!");
			m_MinFilter = Tmp8;
			_pFile->ReadLE(Tmp8);  M_ASSERT(Tmp8 < 4, "MIPFilter doesn't fit in 2 bits!");
			m_MIPFilter = Tmp8;
			_pFile->ReadLE(Tmp8); // m_MIPMapLODBias
			_pFile->ReadLE(Tmp8); // Anisotropy
			_pFile->ReadLE(Tmp8);  M_ASSERT(Tmp8 < 16, "PicMipGroup doesn't fit in 4 bits!");
			m_iPicMipGroup = Tmp8;
			_pFile->ReadLE(Tmp8);  // m_Padd0__
			_pFile->ReadLE(Tmp8);  // m_Padd1__
			_pFile->ReadLE(Tmp32); // m_Padd2__
		}
		break;

#else
	case 0x0101 :
		{
			uint8 Flags;
			_pFile->ReadLE(Flags);
			m_Flags = Flags;
			_pFile->ReadLE(m_MagFilter);
			_pFile->ReadLE(m_MinFilter);
			_pFile->ReadLE(m_MIPFilter);
			_pFile->ReadLE(m_MIPMapLODBias);
			_pFile->ReadLE(m_Anisotropy);
			_pFile->ReadLE(m_iPicMipGroup);
			_pFile->ReadLE(m_PicMipOffset);
		}
		break;

	case 0x0102 :
		{
			uint8 Padd1;
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_MagFilter);
			_pFile->ReadLE(m_MinFilter);
			_pFile->ReadLE(m_MIPFilter);
			_pFile->ReadLE(m_MIPMapLODBias);
			_pFile->ReadLE(m_Anisotropy);
			_pFile->ReadLE(m_iPicMipGroup);
			_pFile->ReadLE(m_PicMipOffset);
			_pFile->ReadLE(Padd1);
			_pFile->ReadLE(m_Padd2__);

			if( m_Flags & CTC_TEXTUREFLAGS_NOCOMPRESS )
				m_TextureVersion	= CTC_TEXTUREVERSION_RAW;
			else
				m_TextureVersion	= CTC_TEXTUREVERSION_S3TC;
		}
		break;

	case 0x0103 :
		{
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_MagFilter);
			_pFile->ReadLE(m_MinFilter);
			_pFile->ReadLE(m_MIPFilter);
			_pFile->ReadLE(m_MIPMapLODBias);
			_pFile->ReadLE(m_Anisotropy);
			_pFile->ReadLE(m_iPicMipGroup);
			_pFile->ReadLE(m_PicMipOffset);
			_pFile->ReadLE(m_TextureVersion);
			_pFile->ReadLE(m_Padd2__);
		}
		break;
#endif

	default :
		Error_static("CTC_TextureProperties::Read", CStrF("Unsupported version %.4x", Ver));
	}
}

void CTC_TextureProperties::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CTC_TextureProperties_Write, MAUTOSTRIP_VOID);

#ifdef USE_PACKED_TEXTUREPROPERTIES
	Error_static("CTC_TextureProperties::Write", "Not supported!");
#else

	uint16 Ver = CTC_PROPERTIES_VERSION;
	_pFile->WriteLE(Ver);

	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_MagFilter);
	_pFile->WriteLE(m_MinFilter);
	_pFile->WriteLE(m_MIPFilter);
	_pFile->WriteLE(m_MIPMapLODBias);
	_pFile->WriteLE(m_Anisotropy);
	_pFile->WriteLE(m_iPicMipGroup);
	_pFile->WriteLE(m_PicMipOffset);
	_pFile->WriteLE(m_TextureVersion);
	_pFile->WriteLE(m_Padd2__);
#endif
}

/*
void CTC_TextureProperties::XRG_Read(CRegistry *_pReg)
{
	MAUTOSTRIP(CTC_TextureProperties_XRG_Read, MAUTOSTRIP_VOID);
	for(int i = 0; i < _pReg->GetNumChildren(); i++)
	{
		if(_pReg->GetName(i) == "FLAGS")
			m_Flags = CStr::TranslateFlags(_pReg->GetValue(i), g_TxtPropFlagsTranslate);
		
		else if(_pReg->GetName(i) == "MAGFILTER")
			m_MagFilter = CStr::TranslateFlags(_pReg->GetValue(i), g_TxtPropFilterTranslate);

		else if(_pReg->GetName(i) == "MINFILTER")
			m_MinFilter = CStr::TranslateFlags(_pReg->GetValue(i), g_TxtPropFilterTranslate);
		
		else if(_pReg->GetName(i) == "MIPFILTER")
			m_MIPFilter = CStr::TranslateFlags(_pReg->GetValue(i), g_TxtPropMIPTranslate);

		else if(_pReg->GetName(i) == "MIPMAPLODBIAS")
			m_MIPMapLODBias = _pReg->GetValuei(i);

		else if(_pReg->GetName(i) == "ANISOTROPY")
			m_Anisotropy = _pReg->GetValuei(i);

		else if(_pReg->GetName(i) == "PICMIP")
			m_iPicMip = _pReg->GetValuei(i);
	}
}

spCRegistry CTC_TextureProperties::XRG_Write()
{
	MAUTOSTRIP(CTC_TextureProperties_XRG_Write, NULL);
	spCRegistry spReg = REGISTRY_CREATE;
	if(m_Flags != 0)
	{
		CStr Flags;
		Flags.CreateFromFlags(m_Flags, g_TxtPropFlagsTranslate);
		spReg->AddKey("FLAGS", Flags);
	}
	
	if(m_MagFilter != CTC_MAGFILTER_DEFAULT)
	{
		CStr Filter;
		Filter.CreateFromInt(m_MagFilter, g_TxtPropFilterTranslate);
		spReg->AddKey("MAGFILTER", Filter);
	}
	
	if(m_MinFilter != CTC_MINFILTER_DEFAULT)
	{
		CStr Filter;
		Filter.CreateFromInt(m_MinFilter, g_TxtPropFilterTranslate);
		spReg->AddKey("MINFILTER", Filter);
	}
	
	if(m_MIPFilter != CTC_MIPFILTER_DEFAULT)
	{
		CStr Filter;
		Filter.CreateFromInt(m_MIPFilter, g_TxtPropMIPTranslate);
		spReg->AddKey("MIPFILTER", Filter);
	}
	
	if(m_MIPMapLODBias != 0)
		spReg->AddKeyi("MIPMAPLODBIAS", m_MIPMapLODBias);
	
	if(m_Anisotropy != 0)
		spReg->AddKeyi("ANISOTROPY", m_Anisotropy);
	
	if(m_iPicMip != 0)
		spReg->AddKeyi("PICMIP", m_iPicMip);
	
	return spReg;
}
*/

// ----------------------------------------------------------------
//  CTextureClass
// ----------------------------------------------------------------
CTextureContainer::CTextureContainer()
{
	MAUTOSTRIP(CTextureContainer_ctor, MAUTOSTRIP_VOID);
	m_iTextureClass = -1;
	m_pRefreshNext = NULL;
	m_pRefreshPrev = NULL;

	// Get TextureContext
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("-", "No texture-context available.");
	m_pTC = pTC;

	m_iTextureClass = m_pTC->AddTextureClass(this);
}

CTextureContainer::~CTextureContainer()
{
	MAUTOSTRIP(CTextureContainer_dtor, MAUTOSTRIP_VOID);
//LogFile("Destroying TC.");
	if (m_iTextureClass >= 0)
		m_pTC->RemoveTextureClass(m_iTextureClass);

//LogFile("Destroying TC..");
	m_pTC = NULL;
	m_iTextureClass = -1;
//LogFile("Destroying TC...");
}

void CTextureContainer::MRTC_Delete()
{
	// Make sure we take the delete lock
	M_LOCK(m_pTC->m_DeleteContainerLock);
	CReferenceCount::MRTC_Delete();
}

void CTextureContainer::OnRefresh()
{
	MAUTOSTRIP(CTextureContainer_OnRefresh, MAUTOSTRIP_VOID);
}

void CTextureContainer::SetTextureParam(int _iLocal, int _Param, int _Value)
{
	MAUTOSTRIP(CTextureContainer_SetTextureParam, MAUTOSTRIP_VOID);
}

int CTextureContainer::GetTextureParam(int _iLocal, int _Param)
{
	MAUTOSTRIP(CTextureContainer_GetTextureParam, 0);
	return 0;
}

void CTextureContainer::SetTextureParamfv(int _iLocal, int _Param, const fp4* _pValues)
{
	MAUTOSTRIP(CTextureContainer_SetTextureParamfv, MAUTOSTRIP_VOID);
}

void CTextureContainer::GetTextureParamfv(int _iLocal, int _Param, fp4* _pRetValues)
{
	MAUTOSTRIP(CTextureContainer_GetTextureParamfv, MAUTOSTRIP_VOID);
}

void CTextureContainer::GetTextureProperties(int _iLocal, CTC_TextureProperties& _TxtProperties)
{
	MAUTOSTRIP(CTextureContainer_GetTextureProperties, MAUTOSTRIP_VOID);
	CTC_TextureProperties TxtProperties;
	_TxtProperties = TxtProperties;
}

int CTextureContainer::EnumTextureVersions(int _iLocal, uint8* _pDestVersion, int _nMaxVersions)
{
	_pDestVersion[0] = CTC_TEXTUREVERSION_RAW;
	return 1;
}


// ----------------------------------------------------------------
//  CTextureContext
// ----------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CTextureContext, CReferenceCount);

IMPLEMENT_OPERATOR_NEW(CTextureContext);



#ifdef USE_HASHED_TEXTURENAME
	struct CIDHashElement { uint32 m_Value; };

	class CIDHash : public THash<int16, CIDHashElement>
	{
		int GetHashIndex(uint32 _Value) const { return _Value & 1023; }

	public:
		void Create(int _nIndices)
		{
			THash<int16, CIDHashElement>::Create(_nIndices, 1024, false);
		}

		void Insert(int _Index, uint32 _Value)
		{
			THash<int16, CIDHashElement>::Remove(_Index);
			THash<int16, CIDHashElement>::Insert(_Index, GetHashIndex(_Value));
			CHashIDInfo& IDInfo = m_pIDInfo[_Index];
			IDInfo.m_Value = _Value;
		}

		int GetIndex(uint32 _Value) const
		{
			int ID = m_pHash[GetHashIndex(_Value)];
			while (ID != -1)
			{
				if (m_pIDInfo[ID].m_Value == _Value)
					return ID;
				ID = m_pIDInfo[ID].m_iNext;
			}
			return -1;
		}
	};
#endif



CTextureContext::CTextureContext()
{
	MAUTOSTRIP(CTextureContext_ctor, MAUTOSTRIP_VOID);
	m_IDCapacity = 0;
	m_pRefreshFirst = NULL;
#ifdef USE_HASHED_TEXTURENAME
	m_pHash = DNew(CIDHash) CIDHash;
#endif
}

/*
CTextureContext::CTextureContext(int _IDCapacity, int _BufferSize) : 
	m_TIHeap(_IDCapacity)
{
	MAUTOSTRIP(CTextureContext_ctor, MAUTOSTRIP_VOID);
	// Alloc an ID and check that it's zero, Index 0 is not to be used for allocations 
	// since it represents 'no texture'.
	if (m_TIHeap.AllocID() != 0)
		Error("-", "Internal error.");

	m_IDCapacity = _IDCapacity;

	// Alloc textureinfo
	m_lTxtIDInfo.SetLen(m_IDCapacity);
};
*/

CTextureContext::~CTextureContext()
{
	MAUTOSTRIP(CTextureContext_dtor, MAUTOSTRIP_VOID);
	m_lpRC.Clear();
	m_lTxtIDInfo.Clear();
#ifdef USE_HASHED_TEXTURENAME
	delete m_pHash;
#endif
}


void CTextureContext::ClearCache()
{
	for (int i = 0; i < m_lpTC.Len(); ++i)
	{
		if (m_lpTC[i])
		{
			m_lpTC[i]->ClearCache();
		}
	}
}

void CTextureContext::ClearUsage()
{
#ifndef PLATFORM_CONSOLE
	for (int i = 0; i < m_lTxtIDInfo.Len(); ++i)
	{
		CTC_TxtIDInfo& TxtID = m_lTxtIDInfo[i];
		TxtID.SetFlags(TxtID.GetFlags() & (~CTC_TXTIDFLAGS_USED));
	}
#endif
}

void CTextureContext::LogUsed(CStr _FileName)
{
#ifndef PLATFORM_CONSOLE
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		// Write resourse log
		CCFile ResourceLog;
		
		if (!_FileName.Len())
			_FileName = pSys->m_ExePath + "\\Textures.tul";

		ResourceLog.Open(_FileName, CFILE_WRITE);
		uint32 NumFound = 0;
		ResourceLog.WriteLE(NumFound);

		for (int i = 0; i < m_lTxtIDInfo.Len(); ++i)
		{
			if (GetTextureFlags(i) & CTC_TXTIDFLAGS_USED)
			{
				CTextureContainer* pTexContainter = GetTextureContainer(i);
				if (pTexContainter)
				{
					int Local = GetLocal(i);
					
					if (CTextureContainer_Plain *pTexC = TDynamicCast<CTextureContainer_Plain >(pTexContainter))
					{
						++NumFound;
						CStr Str = pTexC->GetName(Local);
						Str.Write(&ResourceLog);
					}
					else if (CTextureContainer_VirtualXTC2 *pTexC = TDynamicCast<CTextureContainer_VirtualXTC2 >(pTexContainter))
					{
						++NumFound;
						CStr Str = pTexC->GetName(Local);
						Str.Write(&ResourceLog);
					}
				}
			}
		}
		// Rewrite num found
		ResourceLog.Seek(0);
		ResourceLog.WriteLE(NumFound);
	}
#endif
}

void CTextureContext::Create(int _IDCapacity, int _BufferSize)
{
	MAUTOSTRIP(CTextureContext_Create, MAUTOSTRIP_VOID);
	m_TIHeap.Create(_IDCapacity);

	// Alloc an ID and check that it's zero, Index 0 is not to be used for allocations 
	// since it represents 'no texture'.
	if (m_TIHeap.AllocID() != 0)
		Error("-", "Internal error.");

	m_IDCapacity = _IDCapacity;

	// Alloc textureinfo
	m_lTxtIDInfo.SetLen(m_IDCapacity);
	
#ifdef USE_HASHED_TEXTURENAME
	m_pHash->Create(m_IDCapacity);
#else	
	m_Hash.Create(m_IDCapacity, false, 10);
#endif	
}


int CTextureContext::AllocID(int _iTC, int _iLocal, const char* _pName)
{
#ifdef USE_HASHED_TEXTURENAME
	return AllocID(_iTC, _iLocal, StringToHash(_pName));
#else
	MAUTOSTRIP(CTextureContext_AllocID, 0);
	int ID = m_TIHeap.AllocID();
	if (ID < 0) Error("AllocID", "Out of texture-IDs.");
	CTC_TxtIDInfo* pID = &m_lTxtIDInfo[ID];
	pID->SetFlags(CTC_TXTIDFLAGS_ALLOCATED | CTC_TXTIDFLAGS_WASDELETED);
	pID->SetTxtClass(_iTC);
	pID->m_iLocal = _iLocal;

	if (_pName)
		m_Hash.Insert(ID, _pName);
//LogFile(CStrF("AllocID (ID %d, iTC %d, iL %d, Flg %d)", ID, pID->GetTxtClass(), pID->m_iLocal, pID->GetFlags()));
	return ID;
#endif	
}


int CTextureContext::AllocID(int _iTC, int _iLocal, uint32 _NameID)
{
#ifndef USE_HASHED_TEXTURENAME
	Error("CTextureContext::AllocID", "Can't use NameID, use Name instead!");
	return -1;
#else
	MAUTOSTRIP(CTextureContext_AllocID, 0);
	int ID = m_TIHeap.AllocID();
	if (ID < 0) Error("AllocID", "Out of texture-IDs.");
	CTC_TxtIDInfo* pID = &m_lTxtIDInfo[ID];
	pID->SetFlags(CTC_TXTIDFLAGS_ALLOCATED | CTC_TXTIDFLAGS_WASDELETED);
	pID->SetTxtClass(_iTC);
	pID->m_iLocal = _iLocal;
	
	if (_NameID)
		m_pHash->Insert(ID, _NameID);
	return ID;
#endif
}


void CTextureContext::FreeID(int _ID)
{
	MAUTOSTRIP(CTextureContext_FreeID, MAUTOSTRIP_VOID);
	if ((_ID < 0) || (_ID >= m_IDCapacity)) return;
	MakeDirty(_ID);
	CTC_TxtIDInfo* pID = &m_lTxtIDInfo[_ID];
//LogFile(CStrF("FreeID (ID %d, iTC %d, iL %d)", _ID, pID->GetTxtClass(), pID->m_iLocal));
	pID->SetTxtClass(0);
	pID->SetFlags(0);
	pID->m_iLocal = 0;
	m_TIHeap.FreeID(_ID);
	
#ifdef USE_HASHED_TEXTURENAME
	m_pHash->Remove(_ID);
#else
	m_Hash.Remove(_ID);
#endif	
}


bool CTextureContext::IsValidID(int _ID)
{
	MAUTOSTRIP(CTextureContext_IsValidID, false);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC.ValidPos(iTC)) return false;
	if (!m_lpTC[iTC]) return false;
	int nLocal = m_lpTC[iTC]->GetNumLocal();
	if (iL < 0 || iL >= nLocal) return false;
	return (m_lTxtIDInfo[_ID].GetFlags() & CTC_TXTIDFLAGS_ALLOCATED) != 0;
}


int CTextureContext::GetTextureFlags(int _ID)
{
	MAUTOSTRIP(CTextureContext_GetTextureFlags, 0);
	if (_ID > m_lTxtIDInfo.Len())
	{
		LogFile( CStrF("GetTextureFlags(): Id %d is less than %d", _ID, m_lTxtIDInfo.Len()) );
		return 0;
	}
	return m_lTxtIDInfo[_ID].GetFlags();
}


void CTextureContext::SetTextureParam(int _ID, int _Param, int _Value)
{
	MAUTOSTRIP(CTextureContext_SetTextureParam, MAUTOSTRIP_VOID);
	if (!_ID)
		return;

	CTC_TxtIDInfo& TxtID = m_lTxtIDInfo[_ID];
	if (!(TxtID.GetFlags() & CTC_TXTIDFLAGS_ALLOCATED))
	{
#ifdef USE_HASHED_TEXTURENAME
		M_TRACE("Texture not allocated %d\n", _ID);
#else	
		CStr Name = GetName(_ID);
		M_TRACE("Texture not allocated %d, %s\n", _ID, Name.Str());
#endif		
		M_ASSERT(0, "!");
		return;
//		Error("SetTextureParam", CStr("Invalid texture: %d, _ID));
	}

	switch(_Param)
	{
	case CTC_TEXTUREPARAM_FLAGS :
		if (_Value & CTC_TXTIDFLAGS_ALLOCATED)
			Error("SetTextureParam", "Invalid parameter to CTC_TEXTUREPARAM_FLAGS.");

		TxtID.SetFlags(TxtID.GetFlags() | _Value);
		break;

	case CTC_TEXTUREPARAM_CLEARFLAGS :
		if (_Value & CTC_TXTIDFLAGS_ALLOCATED)
			Error("SetTextureParam", "Invalid parameter to CTC_TEXTUREPARAM_CLEARFLAGS.");

		TxtID.SetFlags(TxtID.GetFlags() & ~_Value);
		break;

	default :
		{
			int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
			int iL = m_lTxtIDInfo[_ID].m_iLocal;
			if (!m_lpTC[iTC]) Error("SetTextureParam", CStrF("Invalid ID %d", _ID));
			m_lpTC[iTC]->SetTextureParam(iL, _Param, _Value);
		}
	}
}


int CTextureContext::GetTextureParam(int _ID, int _Param)
{
	MAUTOSTRIP(CTextureContext_GetTextureParam, 0);
	CTC_TxtIDInfo& TxtID = m_lTxtIDInfo[_ID];

	switch(_Param)
	{
	case CTC_TEXTUREPARAM_FLAGS :
		return TxtID.GetFlags();

	default :
		{
			int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
			int iL = m_lTxtIDInfo[_ID].m_iLocal;
			if (!m_lpTC[iTC]) Error("GetTextureParam", CStrF("Invalid ID %d", _ID));
			return m_lpTC[iTC]->GetTextureParam(iL, _Param);
		}
	}
}


void CTextureContext::SetTextureParamfv(int _ID, int _Param, const fp4* _pValues)
{
	MAUTOSTRIP(CTextureContext_SetTextureParamfv, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("SetTextureParamfv", CStrF("Invalid ID %d", _ID));
	m_lpTC[iTC]->SetTextureParamfv(iL, _Param, _pValues);
}


void CTextureContext::GetTextureParamfv(int _ID, int _Param, fp4* _pRetValues)
{
	MAUTOSTRIP(CTextureContext_GetTextureParamfv, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("GetTextureParamfv", CStrF("Invalid ID %d", _ID));
	m_lpTC[iTC]->GetTextureParamfv(iL, _Param, _pRetValues);
}



int CTextureContext::GetTextureDesc(int _ID, CImage* _pTextureDesc, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContext_GetTextureDesc, 0);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;

	#ifdef _DEBUG
		if (!m_lpTC[iTC]) 
			M_ASSERT(0, "?");
	#else
		if (!m_lpTC[iTC]) Error("GetTextureDesc", CStrF("Invalid ID %d", _ID));
	#endif

//	if (!m_lpTC[iTC]) Error("GetTextureDesc", CStrF("Invalid TextureID (ID %d, iTC %d, iL %d)", _ID, iTC, iL));
	return m_lpTC[iTC]->GetTextureDesc(iL, _pTextureDesc, _Ret_nMipmaps);
}


void CTextureContext::GetTextureProperties(int _ID, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContext_GetTextureProperties, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("GetTextureProperties", CStrF("Invalid ID %d", _ID));
	m_lpTC[iTC]->GetTextureProperties(iL, _Properties);
}


int CTextureContext::GetNumTC()
{
	MAUTOSTRIP(CTextureContext_GetNumTC, 0);
	return m_lpTC.Len();
}


int CTextureContext::GetTextureID(const char* _pTxtName)
{
	MAUTOSTRIP(CTextureContext_GetTextureID, 0);
#ifdef USE_HASHED_TEXTURENAME
	return GetTextureID(StringToHash(_pTxtName));
#else	
	int TextureID = m_Hash.GetIndex(_pTxtName);
	if (TextureID == -1)
		return 0;

	return TextureID;
#endif		

/*	int nTC = m_lpTC.Len();
	int iLocal = 0;
	CTextureContainer** ppTC = m_lpTC.GetBasePtr();
	for(int i = 0; i < nTC; i++)
		if (ppTC[i]) if ((iLocal = ppTC[i]->GetLocal(_pTxtName)) >= 0)
		{
			int TxtID = ppTC[i]->GetTextureID(iLocal);
			return TxtID;
		}

	return 0;*/
}

int CTextureContext::GetTextureID(uint32 _TxtNameID)
{
#ifdef USE_HASHED_TEXTURENAME
	int TextureID = m_pHash->GetIndex(_TxtNameID);
	if (TextureID != -1)
		return TextureID;
#else
	Error("CTextureContext::GetTextureID", "Can't use TxtNameID. Use TxtName instead!");
#endif
	return 0;
}


CStr CTextureContext::GetName(int _ID)
{
	MAUTOSTRIP(CTextureContext_GetName, CStr());
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("GetName", CStrF("Invalid ID %d", _ID));
	return m_lpTC[iTC]->GetName(iL);
}


int CTextureContext::GetLocal(int _ID)
{
	MAUTOSTRIP(CTextureContext_GetLocal, 0);
	return m_lTxtIDInfo[_ID].m_iLocal;
}


CTextureContainer* CTextureContext::GetTextureContainer(int _ID)
{
	MAUTOSTRIP(CTextureContext_GetTextureContainer, NULL);
	return m_lpTC[m_lTxtIDInfo[_ID].GetTxtClass()];
}


CImage* CTextureContext::GetTexture(int _ID, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContext_GetTexture, NULL);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("GetTexture", CStrF("Invalid TextureID (ID %d, iTC %d, iL %d)", _ID, iTC, iL));
	return m_lpTC[iTC]->GetTexture(iL, _iMipMap, _TextureVersion);
}

void CTextureContext::ReleaseTexture(int _ID, int _iMipMap, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContext_ReleaseTexture, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("ReleaseTexture", CStrF("Invalid TextureID (ID %d, iTC %d, iL %d)", _ID, iTC, iL));
	m_lpTC[iTC]->ReleaseTexture(iL, _iMipMap, _TextureVersion);
}

void CTextureContext::ReleaseTextureAllMipmaps(int _ID, int _TextureVersion)
{
	MAUTOSTRIP(CTextureContext_ReleaseTextureAllMipmaps, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("ReleaseTexture", CStrF("Invalid TextureID (ID %d, iTC %d, iL %d)", _ID, iTC, iL));
	m_lpTC[iTC]->ReleaseTextureAllMipmaps(iL, _TextureVersion);
}

void CTextureContext::BuildInto(int _ID, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
{
	MAUTOSTRIP(CTextureContext_BuildInto, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("BuildInto", "Invalid TextureID");
	m_lpTC[iTC]->BuildInto(iL, _ppImg, _nMipmaps, _TextureVersion, _ConvertType, _iStartMip);
}



void CTextureContext::BuildInto(int _ID, class CRenderContext* _pRC)
{
	MAUTOSTRIP(CTextureContext_BuildInto_2, MAUTOSTRIP_VOID);
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("BuildInto", "Invalid TextureID");
	m_lpTC[iTC]->BuildInto(iL, _pRC);
}

void CTextureContext::MakeDirty(int _ID)
{
	MAUTOSTRIP(CTextureContext_MakeDirty, MAUTOSTRIP_VOID);
	int nRC = m_lpRC.Len();
	for(int i = 0; i < nRC; i++)
		if (m_lpRC[i]) m_lpRC[i]->Texture_GetTCIDInfo()->m_pTCIDInfo[_ID].m_Fresh &= ~1;
}

void CTextureContext::MakeDirtyIndirectInterleaved(uint16* _piID, int32* _pID, int _nIDs, int _IDOffset)
{
	MAUTOSTRIP(CTextureContext_MakeDirtyIndirectInterleaved, MAUTOSTRIP_VOID);
	Error("MakeDirtyIndirectInterleaved", "JUNK!");
}

void CTextureContext::MakeDirty(uint16* _pID, int _nIDs)
{
	MAUTOSTRIP(CTextureContext_MakeDirty_2, MAUTOSTRIP_VOID);
	int nRC = m_lpRC.Len();
	if (nRC > 32) Error("MakeDirty", "Too many RenderContexts.");

	for(int iRC = 0; iRC < nRC; iRC++) 
	if (m_lpRC[iRC])
	{
		CRC_IDInfo* pTCIDInfo = m_lpRC[iRC]->Texture_GetTCIDInfo()->m_pTCIDInfo;
		for(int i = 0; i < _nIDs; i++) pTCIDInfo[_pID[i]].m_Fresh &= ~1;
	}
}

void CTextureContext::MakeDirty(uint32* _pID, int _nIDs)
{
	MAUTOSTRIP(CTextureContext_MakeDirty_3, MAUTOSTRIP_VOID);
	int nRC = m_lpRC.Len();
	if (nRC > 32) Error("MakeDirty", "Too many RenderContexts.");

	for(int iRC = 0; iRC < nRC; iRC++) 
	if (m_lpRC[iRC])
	{
		CRC_IDInfo* pTCIDInfo = m_lpRC[iRC]->Texture_GetTCIDInfo()->m_pTCIDInfo;
		for(int i = 0; i < _nIDs; i++) pTCIDInfo[_pID[i]].m_Fresh &= ~1;
	}
}

void CTextureContext::Refresh()
{
	MAUTOSTRIP(CTextureContext_Refresh, MAUTOSTRIP_VOID);
	CTextureContainer* pTC = m_pRefreshFirst;
	while(pTC)
	{
		pTC->OnRefresh();
		pTC = pTC->m_pRefreshNext;
	}
}

int CTextureContext::AddRenderContext(CRenderContext* _pRC)
{
	MAUTOSTRIP(CTextureContext_AddRenderContext, 0);
	int iRC = -1;
	int nRC = m_lpRC.Len();
	for (int i = 0; ((i < nRC) && (iRC < 0)); i++)
		if (m_lpRC[i] == NULL) iRC = i;

	if (iRC < 0) 
		iRC = m_lpRC.Add(_pRC);
	else
		m_lpRC[iRC] = _pRC;

	return iRC;
};

void CTextureContext::RemoveRenderContext(int _iRC)
{
	MAUTOSTRIP(CTextureContext_RemoveRenderContext, MAUTOSTRIP_VOID);
	m_lpRC[_iRC] = NULL;
};

int CTextureContext::AddTextureClass(CTextureContainer* _pTClass)
{
	MAUTOSTRIP(CTextureContext_AddTextureClass, 0);
	int iTClass = -1;
	int nTClass = m_lpTC.Len();
	for (int i = 0; ((i < nTClass) && (iTClass < 0)); i++) 
		if (m_lpTC[i] == NULL) iTClass = i;

	if (iTClass < 0) 
		iTClass = m_lpTC.Add(_pTClass);
	else
		m_lpTC[iTClass] = _pTClass;

	return iTClass;
};

void CTextureContext::RemoveTextureClass(int _iTClass)
{
#ifdef PLATFORM_XENON
	if (!m_DeleteContainerLock.OwnsLock())
		M_BREAKPOINT; // The delete container lock have to be locked here
#endif
	MAUTOSTRIP(CTextureContext_RemoveTextureClass, MAUTOSTRIP_VOID);
	DisableTextureClassRefresh(_iTClass);
	m_lpTC[_iTClass] = NULL;
};

void CTextureContext::EnableTextureClassRefresh(int _iTClass)
{
	MAUTOSTRIP(CTextureContext_EnableTextureClassRefresh, MAUTOSTRIP_VOID);
	CTextureContainer* pTC = m_lpTC[_iTClass];
	if(!pTC) Error("EnableTextureClassRefresh", "Invalid TC.");

	if (m_pRefreshFirst == pTC || pTC->m_pRefreshNext || pTC->m_pRefreshPrev)
		return;

	pTC->m_pRefreshNext = m_pRefreshFirst;
	if (m_pRefreshFirst) m_pRefreshFirst->m_pRefreshPrev = pTC;
	m_pRefreshFirst = pTC;
}

void CTextureContext::DisableTextureClassRefresh(int _iTClass)
{
	MAUTOSTRIP(CTextureContext_DisableTextureClassRefresh, MAUTOSTRIP_VOID);
	CTextureContainer* pTC = m_lpTC[_iTClass];
	if(!pTC) Error("EnableTextureClassRefresh", "Invalid TC.");

	if (m_pRefreshFirst != pTC && !pTC->m_pRefreshNext && !pTC->m_pRefreshPrev)
		return;

	if (m_pRefreshFirst == pTC)
	{
		m_pRefreshFirst = pTC->m_pRefreshNext;
		if (pTC->m_pRefreshNext != NULL)
			pTC->m_pRefreshNext->m_pRefreshPrev = NULL;
	}
	else
	{
		if (pTC->m_pRefreshPrev != NULL)
			pTC->m_pRefreshPrev->m_pRefreshNext = pTC->m_pRefreshNext;
		if (pTC->m_pRefreshNext != NULL)
			pTC->m_pRefreshNext->m_pRefreshPrev = pTC->m_pRefreshPrev;
	}

	pTC->m_pRefreshNext = NULL;
	pTC->m_pRefreshPrev = NULL;
}

int CTextureContext::EnumTextureVersions(int _ID, uint8* _pDest, int _nMax)
{
	int iTC = m_lTxtIDInfo[_ID].GetTxtClass();
	int iL = m_lTxtIDInfo[_ID].m_iLocal;
	if (!m_lpTC[iTC]) Error("GetTexture", CStrF("Invalid TextureID (ID %d, iTC %d, iL %d)", _ID, iTC, iL));

	int nVersions = m_lpTC[iTC]->EnumTextureVersions(iL, _pDest, _nMax);
	return nVersions;

#if 0
	CTextureContainer_Plain* pTC = TDynamicCast<CTextureContainer_Plain, CTextureContainer>( m_lpTC[iTC] );
	CTextureContainer_VirtualXTC2* pXTC2 = TDynamicCast<CTextureContainer_VirtualXTC2>(m_lpTC[iTC]);
	if( pTC )
	{
		nVersions = pTC->EnumTextureVersions(iL, _pDest, _nMax);
/*
		int nLocal = pTC->GetNumLocal();
		while((iL < nLocal) && (nVersions < _nMax))
		{
			spCTexture spTex = pTC->GetTexture(iL, CTC_TEXTUREVERSION_ANY, false);
			if(spTex->m_TextureID != _ID)
				break;

			CTC_TextureProperties Properties;
			pTC->GetTextureProperties(iL, Properties);
			_pDest[nVersions++]	= Properties.m_TextureVersion;
			iL++;
		}
*/
	}
	else if(pXTC2)
	{
		nVersions	= pXTC2->EnumTextureVersions(iL, _pDest, _nMax);
	}
	else
		_pDest[nVersions++]	= CTC_TEXTUREVERSION_RAW;

	return nVersions;
#endif
}
