#include "PCH.h"

#include "XRSurfaceContext.h"
#include "XRSurf.h"
#include "../MOS.h"
#include "XREngineVar.h"
#include "XREngine.h"

#ifndef PLATFORM_CONSOLE
# define ENABLE_SURFACE_CACHE
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_VBOperator::OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_OnTestHWAccelerated, false);
	return false;
}

void CXR_VBOperator::OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_OnInitSurface, MAUTOSTRIP_VOID);
}

int CXR_VBOperator::OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const
{
	MAUTOSTRIP(CXR_VBOperator_OnGetTextureCount, 0);
	return 1;
}

int CXR_VBOperator::OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const
{
	MAUTOSTRIP(CXR_VBOperator_OnEnumTextureID, 0);
	return _pSurfLayer->m_TextureID;
}


CStr CXR_VBOperator::OnDebugDump(class CXR_Engine* _pEngine, const class CXW_LayerOperation& _Oper)
{
	MAUTOSTRIP(CXR_VBOperator_OnDebugDump, CStr());
	return "CXR_VBOperator";
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SurfaceContext
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_SurfaceContext, CReferenceCount);

CXR_SurfaceContext::CXR_SurfaceContext()
{
	MAUTOSTRIP(CXR_SurfaceContext_ctor, MAUTOSTRIP_VOID);
}

CXR_SurfaceContext::~CXR_SurfaceContext()
{
	MAUTOSTRIP(CXR_SurfaceContext_dtor, MAUTOSTRIP_VOID);
}


void CXR_SurfaceContext::CreateHashTableSafe()
{
	MAUTOSTRIP(CXR_SurfaceContext_CreateHashTableSafe, MAUTOSTRIP_VOID);
	M_LOCK(m_Lock);
	CreateHashTable();
}

void CXR_SurfaceContext::CreateHashTable()
{
	MSCOPESHORT(CXR_SurfaceContext::CreateHashTable);
	MAUTOSTRIP(CXR_SurfaceContext_CreateHashTable, MAUTOSTRIP_VOID);
	if (m_spHash != NULL) return;

	m_spHash = MNew(CMap32);
	if (!m_spHash) 
		MemError("CreateHashTable");

	m_HashCapacity = Max(m_HashCapacity, m_lspSurfaces.Len());
	int HashBits = Max(4, Log2(GetGEPow2(m_HashCapacity)) - 1);
	m_spHash->Create(m_HashCapacity, HashBits);

	for(int i = 0; i < m_lspSurfaces.Len(); i++)
		m_spHash->Insert(i, StringToHash(m_lspSurfaces[i]->m_Name));
}

void CXR_SurfaceContext::Create()
{
	MAUTOSTRIP(CXR_SurfaceContext_Create, MAUTOSTRIP_VOID);
	M_LOCK(m_Lock);

	m_spHash = NULL;
	m_HashCapacity = 0;
	m_lspSurfaces.Clear();

	// Create a default-surface
	spCXW_Surface spSurf = MNew(CXW_Surface);
	if (!spSurf) MemError("Create");

	spSurf->m_Name = "$$$DEFAULT$$$";
	spSurf->SetNumSequences(1);
	CXW_SurfaceSequence* pSeq = spSurf->GetSequence(0);
	pSeq->SetNumKeyFrames(1);

	CXW_SurfaceKeyFrame* pKey = pSeq->GetKey(0);
	pKey->m_lTextures.SetLen(1);

	CXW_SurfaceLayer& Layer = pKey->m_lTextures[0];
	Layer.SetTextureName("SPECIAL_DEFAULTSURF");

	m_lspSurfaces.Add(spSurf);

	// Reserve ID 0 for 'invalid'
	m_lspVBOperators.Clear();
	m_lspVBOperators.Add(spCXR_VBOperator(NULL));
	m_lspVBOperatorNames.Clear();
	m_lspVBOperatorNames.Add("$$$$void$$$$");
}

int CXR_SurfaceContext::GetNumSurfaces()
{
	MAUTOSTRIP(CXR_SurfaceContext_GetNumSurfaces, 0);
	M_LOCK(m_Lock);
	return m_lspSurfaces.Len();
}

void CXR_SurfaceContext::UpdateTextureIDs(int _SurfaceID)
{
	MSCOPESHORT(CXR_SurfaceContext::UpdateTextureIDs);
	MAUTOSTRIP(CXR_SurfaceContext_UpdateTextureIDs, MAUTOSTRIP_VOID);
	M_LOCK(m_Lock);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("UpdateTextureIDs", "No texture-context.");

	if (_SurfaceID == -1)
	{
		for(int i = 0; i < m_lspSurfaces.Len(); i++)
		{
			m_lspSurfaces[i]->Init();
			m_lspSurfaces[i]->InitTextures(false);
		}
	}
	else
	{
		if (!m_lspSurfaces.ValidPos(_SurfaceID))
			Error("UpdateTextureIDs", CStrF("Invalid surface ID %d", _SurfaceID));
		m_lspSurfaces[_SurfaceID]->Init();
		m_lspSurfaces[_SurfaceID]->InitTextures(false);
	}
}

spCKeyContainerNode CXR_SurfaceContext::LoadSurfaceScript(const char* _pFileName, bool _bIgnoreParseOption)
{
	MAUTOSTRIP(CXR_SurfaceContext_LoadSurfaceScript, NULL);
	MSCOPESHORT(CXR_SurfaceContext::LoadSurfaceScript);
	CStr Name(_pFileName);
	CStr Ext = Name.GetFilenameExtenstion();

	if (/*Ext.CompareNoCase("TXT") == 0 ||*/
		Ext.CompareNoCase("XTX") == 0)
	{
		M_LOCK(m_Lock);
		M_TRY
		{ 
			spCKeyContainerNode spRoot = MNew(CKeyContainerNode);
			if (!spRoot) return NULL;
			spRoot->ReadFromScript(Name);

			int iKey = spRoot->GetKeys()->GetKeyIndex("CLASSNAME");
			if (iKey >= 0)
			{
				CStr ClassName = spRoot->GetKeys()->GetKeyValue(iKey).UpperCase();
				if (ClassName == "NODE" || ClassName == "SURFACE" || ClassName == "SURFACES")
				{
					if (!_bIgnoreParseOption)
					{
						int iKeyParse = spRoot->GetKeys()->GetKeyIndex("XR_PARSE");
						if (iKeyParse >= 0)
							if (!spRoot->GetKeys()->GetKeyValue(iKeyParse).Val_int())
							{
								LogFile("        Skipping: " + Name);
								return NULL;
							}
					}
//							ConOutL("        " + _Path + pRec->m_Name);
					LogFile("        Loading: " + Name);
					return spRoot;
				}
			}
		}
		M_CATCH(
		catch(CCException) 
		{
			ConOutL("§cf80WARNING: Failure reading surfaces from: " + Name);
			return NULL;
		}
		)
	}
	return NULL;
}

TArray<spCXW_Surface> CXR_SurfaceContext::CreateSurfaces(CKeyContainerNode* _pNode)
{
	MAUTOSTRIP(CXR_SurfaceContext_CreateSurfaces, TArray<spCXW_Surface>());
	MSCOPESHORT(CXR_SurfaceContext::CreateSurfaces);
	TArray<spCXW_Surface> lspSurfaces;
	lspSurfaces.SetGrow(128);

	CXW_Surface Surf;
	CXW_Surface::ParseNode_r(_pNode, Surf, lspSurfaces);

	TArray<spCXW_Surface> lspCombined = CXW_Surface::CombineSurfaces(lspSurfaces);
	TArray<spCXW_Surface> lspExpanded = CXW_Surface::ExpandLODSurfaces(lspCombined);
	return lspExpanded;
}

void CXR_SurfaceContext::AddSurfaces(TArray<spCXW_Surface> _lspSurfaces)
{
	MAUTOSTRIP(CXR_SurfaceContext_AddSurfaces, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_SurfaceContext::AddSurfaces);
	M_LOCK(m_Lock);
	m_HashCapacity = Max(m_HashCapacity, m_lspSurfaces.Len() + _lspSurfaces.Len());
	m_spHash = NULL;
	CreateHashTable();

	int iStart = m_lspSurfaces.Len();
	m_lspSurfaces.Add(&_lspSurfaces);

	if (m_spHash != NULL)
	{
		for(int i = iStart; i < m_lspSurfaces.Len(); i++)
		{
			int Hash = StringToHash(m_lspSurfaces[i]->m_Name);
			if (m_spHash->GetIndex(Hash) >= 0)
			{

				ConOutL(CStrF("§cf80        WARNING: Duplicate surface: %s", m_lspSurfaces[i]->m_Name.Str()));
			}
	//LogFile(CStrF("(AddSurfaces) %.8x, %s", Hash, m_lspSurfaces[i]->m_Name.Str()));
			m_spHash->Insert(i, Hash);
		}
	}
	UpdateTextureIDs(-1);
}

void CXR_SurfaceContext::AddSurfaces(CCFile* _pFile)
{
	MAUTOSTRIP(CXR_SurfaceContext_AddSurfaces_2, MAUTOSTRIP_VOID);
	Error("AddSurfaces", "Not implemented.");
//	TArray<spCXW_Surface> lspSurf = CXW_Surface::ReadScript(Name.GetStrSep(";"));
//	AddSurfaces(lspSurf);
}

void CXR_SurfaceContext::AddSurfaces(CStr _FileName)
{
	MAUTOSTRIP(CXR_SurfaceContext_AddSurfaces_3, MAUTOSTRIP_VOID);
	while(_FileName != "")
	{
		TArray<spCXW_Surface> lspSurf = CXW_Surface::ReadScript(_FileName.GetStrSep(";"));
		AddSurfaces(lspSurf);
	}
}

void CXR_SurfaceContext::AddSurfaces(CKeyContainerNode* _pRoot)
{
	MAUTOSTRIP(CXR_SurfaceContext_AddSurfaces_4, MAUTOSTRIP_VOID);
	AddSurfaces(CreateSurfaces(_pRoot));
}


TArray<spCXW_Surface> CXR_SurfaceContext::ScanDirectory(CStr _Path, bool _bIgnoreParseOption)
{
	TArray<spCXW_Surface> lResult;
	lResult.SetGrow(4096);

	bool bReadXSU = (_bIgnoreParseOption == false);	// _bIgnoreParseOption is used by XWC
	bool bLoadNonCompiledSurfaces = true;
	if (bReadXSU)
	{
		CDirectoryNode Dir;
		Dir.ReadDirectory(_Path + "*.XSU");
		int nFiles = Dir.GetFileCount();
		for (int i = 0; i < nFiles; i++)
		{
			bLoadNonCompiledSurfaces = false;
			CDir_FileRec* pRec = Dir.GetFileRec(i);
			CDataFile DFile;
			DFile.Open(_Path + pRec->m_Name);
			if (DFile.GetNext("SURFACES"))
			{
				TArray<spCXW_Surface> lspSurf;
				CXW_Surface::Read(DFile.GetFile(), lspSurf, DFile.GetUserData());
				lResult.Add(&lspSurf);
			}
			DFile.Close();
		}
	}

	if (bLoadNonCompiledSurfaces)
	{
		bool bUseCache = false;
#ifdef ENABLE_SURFACE_CACHE
		// Create checksum from modify date of all .xtx files in current directory
		uint64 CheckSum = 5381;
		{
			CDirectoryNode Dir;
			Dir.ReadDirectory(_Path + "*.XTX");
			uint nFiles = Dir.GetFileCount();
			for (uint i = 0; i < nFiles; i++)
			{
				const CDir_FileRec* pRec = Dir.GetFileRec(i);
				CheckSum = CheckSum*33 + uint64(pRec->m_WriteTime);
			}
		}

		// Look for cache file and compare the checksum
		CFStr CacheFile = CFStrF("%s_%sSurfaceCache.XSC", _Path.Str(), _bIgnoreParseOption ? "XWC" : "");
		if (CDiskUtil::FileExists(CacheFile))
		{
			CDataFile DFile;
			DFile.Open(CacheFile);
			if (DFile.GetNext("CHECKSUM"))
			{
				uint64 x;
				DFile.GetFile()->ReadLE(x);
				if ((x == CheckSum) && DFile.GetNext("SURFACES"))
				{
					LogFile(CStrF("Reading cached surfaces: %s", CacheFile.Str()));
					bUseCache = true;
					TArray<spCXW_Surface> lspSurf;
					CXW_Surface::Read(DFile.GetFile(), lspSurf, DFile.GetUserData());
					lResult.Add(&lspSurf);
				}
			}
		}
#endif

		TArray<spCXW_Surface> lspSurfaceCache;
		CDirectoryNode Dir;
		Dir.ReadDirectory(_Path + "*");
		uint nFiles = Dir.GetFileCount();
		for (uint i = 0; i < nFiles; i++)
		{
			CDir_FileRec* pRec  = Dir.GetFileRec(i);
			if (pRec->IsDirectory())
			{
				if (pRec->m_Name.Copy(0,1) != ".")
				{
					TArray<spCXW_Surface> lspSurf;
					lspSurf = ScanDirectory(_Path + pRec->m_Name + "\\", _bIgnoreParseOption);
					lResult.Add(&lspSurf);
				}
			}
			else if (!bUseCache)		// If checksum didn't match, read surfaces
			{
				if (bReadXSU && (pRec->m_Ext.CompareNoCase("XSU") == 0))
				{
					CDataFile DFile;
					DFile.Open(_Path + pRec->m_Name);
					if (DFile.GetNext("SURFACES"))
					{
						TArray<spCXW_Surface> lspSurf;
						CXW_Surface::Read(DFile.GetFile(), lspSurf, DFile.GetUserData());
						lResult.Add(&lspSurf);
					}
					DFile.Close();
				}
				else
				{
					spCKeyContainerNode spRoot = LoadSurfaceScript(_Path + pRec->m_Name, _bIgnoreParseOption);
					if (spRoot != NULL)
					{
						TArray<spCXW_Surface> lspSurf = CreateSurfaces(spRoot);
						lResult.Add(&lspSurf);
					}
				}
			}
		}

#ifdef ENABLE_SURFACE_CACHE
		// Create cache file
		if (!bUseCache && lResult.Len())
		{
			LogFile(CStrF("Creating surface cache: %s", CacheFile.Str()));
			CDataFile DFile;
			DFile.Create(CacheFile);
			{
				DFile.BeginEntry("CHECKSUM");
				DFile.GetFile()->WriteLE(CheckSum);
				DFile.EndEntry(0);
			}
			{
				DFile.BeginEntry("SURFACES");
				CXW_Surface::Write(DFile.GetFile(), lResult);
				DFile.EndEntry(lResult.Len());
			}
			DFile.Close();
		}
#endif
	}
	return lResult;
}


void CXR_SurfaceContext::AddDirectory(CStr _Path)
{
	MAUTOSTRIP(CXR_SurfaceContext_AddDirectory, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_SurfaceContext::AddDirectory);
	M_LOCK(m_Lock);

	TArray<spCXW_Surface> lspSurf = ScanDirectory(_Path, false);
	AddSurfaces(lspSurf);
}


void CXR_SurfaceContext::UpdateSurfaces(CStr _Path)
{
	MAUTOSTRIP(CXR_SurfaceContext_UpdateSurfaces, MAUTOSTRIP_VOID);
	M_LOCK(m_Lock);
	CDirectoryNode Dir;
	Dir.ReadDirectory(_Path + "*");

	int nFiles = Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		CDir_FileRec* pRec  = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
				UpdateSurfaces(_Path + pRec->m_Name + "\\");
		}
		else
		{
			spCKeyContainerNode spRoot = LoadSurfaceScript(_Path + pRec->m_Name);
			if (spRoot != NULL)
			{
				TArray<spCXW_Surface> lspSurfaces = CreateSurfaces(spRoot);
				LogFile(CStrF("        %d surfaces in ", lspSurfaces.Len()) + _Path + pRec->m_Name);

				for(int i = 0; i < lspSurfaces.Len(); i++)
				{
					int SurfID = GetSurfaceID(lspSurfaces[i]->m_Name);
					if (SurfID)
					{
						m_lspSurfaces[SurfID] = lspSurfaces[i];
						UpdateTextureIDs(SurfID);
					}
				}
			}
		}
	}
}

CStr CXR_SurfaceContext::GetSurfaceName(int _SurfaceID)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceName, CStr());
	M_LOCK(m_Lock);
	if (!m_lspSurfaces.ValidPos(_SurfaceID))
		Error("GetSurfaceName", CStrF("Invalid surface ID %d", _SurfaceID));
	return m_lspSurfaces[_SurfaceID]->m_Name;
}

int CXR_SurfaceContext::GetSurfaceID(const char* _pName)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceID, 0);
	M_LOCK(m_Lock);
	CreateHashTable();
	int Hash = StringToHash(_pName);
	int SurfaceID = m_spHash->GetIndex(Hash);

//	int SurfaceID = m_spHash->GetIndex(_pName);
	if (SurfaceID < 0)
	{
//		LogFile(CStrF("(GetSurfaceID) %.8x, %s not found", Hash, _pName));
		SurfaceID = 0;
	}
	return SurfaceID;
//	return m_spHash->GetIndex(_pName);
}

int CXR_SurfaceContext::GetSurfaceID(uint32 _NameHash)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceID, 0);
	M_LOCK(m_Lock);
	CreateHashTable();
	int SurfaceID = m_spHash->GetIndex(_NameHash);

//	int SurfaceID = m_spHash->GetIndex(_pName);
	if (SurfaceID < 0)
		SurfaceID = 0;
	return SurfaceID;
//	return m_spHash->GetIndex(_pName);
}

CXW_Surface* CXR_SurfaceContext::GetSurface(int _SurfaceID)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurface, NULL);
	M_LOCK(m_Lock);
	if (!m_lspSurfaces.ValidPos(_SurfaceID))
		Error("GetSurface", CStrF("Invalid surface ID %d", _SurfaceID));

	return m_lspSurfaces[_SurfaceID];
}

CXW_Surface* CXR_SurfaceContext::GetSurface(const char* _pName)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurface_2, NULL);
	M_LOCK(m_Lock);
	int SurfaceID = GetSurfaceID(_pName);
	if (SurfaceID < 0) SurfaceID = 0;

	return m_lspSurfaces[SurfaceID];
}

CXW_Surface* CXR_SurfaceContext::GetSurfaceVersion(int _SurfaceID, CXR_Engine *_pEngine)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceVersion, NULL);
	return GetSurfaceVersion(_SurfaceID, _pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
}

CXW_Surface* CXR_SurfaceContext::GetSurfaceVersion(int _SurfaceID, int _SurfOptions, int _SurfCaps)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceVersion_2, NULL);
	M_LOCK(m_Lock);
	CXW_Surface *pOrgSurf = GetSurface(_SurfaceID);
	return pOrgSurf->GetSurface(_SurfOptions, _SurfCaps);
}

CXW_SurfaceKeyFrame* CXR_SurfaceContext::GetSurfaceKey(int _SurfaceID, int _SurfOptions, int _SurfCaps, CMTime _Time, int _iSeq)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceKey, NULL);
	M_LOCK(m_Lock);
	CXW_Surface* pSurf = GetSurfaceVersion(_SurfaceID, _SurfOptions, _SurfCaps);
	CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(_iSeq, _Time, m_TempKeyFrame.Get());
	return pSurfKey;
}

CXW_SurfaceKeyFrame* CXR_SurfaceContext::GetSurfaceKey(int _SurfaceID, CXR_Engine *_pEngine, CMTime _Time, int _iSeq)
{
	MAUTOSTRIP(CXR_SurfaceContext_GetSurfaceKey_2, NULL);
	return GetSurfaceKey(_SurfaceID, _pEngine->m_SurfOptions, _pEngine->m_SurfCaps, _Time, _iSeq);
}

int CXR_SurfaceContext::VBOperator_GetOperatorID(const char* _pClassName)
{
	MAUTOSTRIP(CXR_SurfaceContext_VBOperator_GetOperatorID, 0);
	M_LOCK(m_Lock);
	for(int i = 0; i < m_lspVBOperatorNames.Len(); i++)
		if (m_lspVBOperatorNames[i].CompareNoCase(_pClassName) == 0) return i;

	MSCOPESHORT(CXR_SurfaceContext::VBOperator_GetOperatorID);
	CStr ClassName = CStrF("CXR_VBOperator_%s", _pClassName);

	TPtr<CReferenceCount> spObj = MRTC_GOM()->GetClassRegistry()->CreateObject(ClassName);
	if (spObj == NULL)
	{
		// Ugly:
		static TArray<CStr> lFailedOpers;

		// Check if we've failed this operator already, and in that case we don't log anything.
		for(int i = 0; i < lFailedOpers.Len(); i++)
			if (lFailedOpers[i].CompareNoCase(_pClassName) == 0)
				return 0;

		lFailedOpers.Add(CStr(_pClassName));
	
ConOutL("§cf80        WARNING: Invalid VB layer operator " + ClassName);
		return 0;
	}

	// ConOutL("(CXR_SurfaceContext::VBOperator_GetOperatorID) Created operator " + ClassName);

	spCXR_VBOperator spVBOper = safe_cast<CXR_VBOperator>((CReferenceCount*)spObj);
	m_lspVBOperators.Add(spVBOper);
	return m_lspVBOperatorNames.Add(CStr(_pClassName));
}

CXR_VBOperator* CXR_SurfaceContext::VBOperator_Get(int _VBClassID)
{
	MAUTOSTRIP(CXR_SurfaceContext_VBOperator_Get, NULL);
	M_LOCK(m_Lock);
	return (m_lspVBOperators.ValidPos(_VBClassID)) ? 
		(CXR_VBOperator*)m_lspVBOperators[_VBClassID] :
		(CXR_VBOperator*)NULL;
}


CXW_SurfaceKeyFrame& CXR_SurfaceContext::GetTempSurfaceKeyFrame()
{
	return *m_TempKeyFrame.Get();
}
