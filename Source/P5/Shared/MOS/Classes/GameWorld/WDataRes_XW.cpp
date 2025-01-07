
#include "PCH.h"

#include "WMapData.h"
#include "WDataRes_XW.h"
#include "../../XR/XRBlockNavGrid.h"
#include "../../XR/XRNavGraph.h"
#include "WBlockNavGrid.h"
#include "../../XRModels/Model_BSP/WBSPModel.h"
#include "../../XRModels/Model_BSP2/WBSP2Model.h"
#include "../../XRModels/Model_BSP3/WBSP3Model.h"
#include "../../XRModels/Model_BSP4/WBSP4Model.h"
#include "../../XRModels/Model_BSP4Glass/WBSP4Glass.h"

MRTC_IMPLEMENT_DYNAMIC(CWRes_XWIndex, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_XW, CWRes_Model);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_XW2, CWRes_Model_XW);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_XW3, CWRes_Model_XW);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_XW4, CWRes_Model_XW);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_Glass, CWRes_Model_XW);
MRTC_IMPLEMENT_DYNAMIC(CWRes_XWResource, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_XWNavGrid, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_XWNavGraph, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_XW, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_ObjectAttribs, CWResource);

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Index
|__________________________________________________________________________________________________
\*************************************************************************************************/

int CWRes_XWIndex::GetModelFileOffset(int _iModel)
{
	MAUTOSTRIP(CWRes_XWIndex_GetModelFileOffset, 0);
	if (!m_liModelOffsets.ValidPos(_iModel))
		Error("GetModelFileOffset", CStrF("Invalid model index. %d/%d", _iModel, m_liModelOffsets.Len()));
	return m_liModelOffsets[_iModel];
}

bool CWRes_XWIndex::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XWIndex_Create, false);
	MSCOPE(CWRes_XWIndex::Create, RES_XW_INDEX);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);

	if (!CDiskUtil::FileExists(FileName)) return false;

	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
//	CCFile* pFile = spDFile->GetFile();

	// Read BSP-Models
	if (!spDFile->GetNext("BSPMODELS"))
		Error("Create", "No BSP-Models in file.");

	int nModels = spDFile->GetUserData();

	if (!spDFile->GetSubDir())
		Error("Create", "No BSP-Models in file.");

	m_liModelOffsets.SetLen(nModels);
	int iModel = 0;
	while (spDFile->GetNext("BSPMODEL"))
	{
		m_liModelOffsets[iModel] = spDFile->GetEntryDataFileOffset();
//		LogFile(CStrF("Found one BSPMODEL. Entry offset %d", m_liModelOffsets[iModel]));

		iModel++;
		if (iModel > nModels)
			Error("Create", CStrF("Invalid number of BSPMODEL entries. %d > %d", iModel, nModels));
	}

	if (iModel != nModels)
		Error("Create", CStrF("Invalid number of BSPMODEL entries. %d != %d", iModel, nModels));

	spDFile->Close();
	spDFile = NULL;

	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_Model_XW
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model* CWRes_Model_XW::GetModel()
{
	MAUTOSTRIP(CWRes_Model_XW_GetModel, NULL);
	return m_spModel; 
};

#if 1

bool CWRes_Model_XW::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass, spCXR_Model _spModel)
{
	MAUTOSTRIP(CWRes_Model_XW_Create, false);
	MSCOPE(CWRes_Model_XW::Create, RES_XW_MODEL);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	
	m_pWData = _pWData;
	m_spModel = _spModel;
	m_Flags |= WRESOURCE_LOADED;

	return true;
}

bool CWRes_Model_XW::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_XW_Create_2, false);
	MSCOPE(CWRes_Model_XW::Create_2, RES_XW_MODEL);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	
	m_pWData = _pWData;
	return true;
}

void CWRes_Model_XW::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_XW_OnLoad, MAUTOSTRIP_VOID);
	if (IsLoaded())
		return;

	ReadAllModels(m_pWData, "CXR_Model_BSP");
	m_Flags |= WRESOURCE_LOADED;
}

void CWRes_Model_XW::CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const CBSP_CreateInfo& _CreateInfo)
{
	((CXR_Model_BSP*)_pModel)->Create(NULL, _pDFile, NULL, _CreateInfo);
}

bool CWRes_Model_XW::ReadAllModels(CWorldData* _pWData, const char* _pClassName)
{
	MAUTOSTRIP(CWRes_Model_XW_ReadAllModels, false);
	MSCOPESHORT(CWRes_Model_XW::ReadAllModels); //AR-SCOPE

	CStr Name = m_Name.Copy(4, 100000);
	int ModelNr = 0;
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		ModelNr = Name.Val_int();
		Name = NewName;
	}
	if (Name.GetFilenameExtenstion() == "") 
		Name += ".XW";

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);

	if (!CDiskUtil::FileExists(FileName))
		return false;
//		Error("ReadAllModels", "Invalid world file name: " + FileName);

	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
//	CCFile* pFile = spDFile->GetFile();

	// Read object attributes and determine glass existence in the map or not
	int iGlass = -1, iPlayerPhys = -1;
	{
		int iIndex = _pWData->GetResourceIndex(CStrF("ATR:%s", Name.GetFilenameNoExt().Str()), NULL);
		if (iIndex > 0)
		{
			CWResource *pRes = _pWData->GetResource(iIndex);
			if (pRes)
			{
				CWRes_ObjectAttribs *pObjAttr = TDynamicCast<CWRes_ObjectAttribs>(pRes);
				if (pObjAttr)
				{
					CRegistryDirectory *pRegDir = pObjAttr->GetRegDir();
					int nObjAttribsObjects = pRegDir->NumReg();
					for (int i = 0; i < nObjAttribsObjects; i++)
					{
						CRegistry *pReg = pRegDir->GetReg(i);
						if (pReg->GetValuei("EVALED", 0))
						{
							pReg = pReg->FindChild("TEMPLATES");
							if (pReg && pReg->GetNumChildren())
								pReg = pReg->GetChild(0);
						}
						if (!pReg)
							continue;

						uint32 ClassNameHash = pReg->GetValue("CLASSNAME").StrHash();
						if (ClassNameHash == MHASH4('WORL','DPLA','YERP','HYS'))
							iPlayerPhys = pReg->GetValuei("BSPMODELINDEX", -1);
						else if (ClassNameHash == MHASH4('WORL','DGLA','SSSP','AWN'))
							iGlass = pReg->GetValuei("BSPMODELINDEX", -1);

						if (iPlayerPhys >= 0 && iGlass >= 0)
							break;
					}
				}
			}
		}
	}
	spDFile->Open(FileName);
	{
#if 0

		spDFile->PushPosition();
		
		if (spDFile->GetNext("OBJECTATTRIBS")) 
		{
			int nObjAttribsObjects = spDFile->GetUserData();
			int ObjAttribsVersion = spDFile->GetUserData2();
			for (int i = 0; i < nObjAttribsObjects; i++)
			{
				spCRegistry spObj = REGISTRY_CREATE;
				if (spObj == NULL) MemError("Read");
				spObj->Read(spDFile->GetFile(), ObjAttribsVersion);
				
				if(spObj->FindIndex("HASGLASSMODEL") && spObj->GetValue("HASGLASSMODEL").Val_int())
				{
					bGlass = true;
					spObj = 0;
					break;
				}

				spObj = 0;
			}
		}

		spDFile->PopPosition();
#endif
	}

	// Read BSP-Models
	if (!spDFile->GetNext("BSPMODELS"))
		Error("Create", "No BSP-Models in file.");

	int nModels = spDFile->GetUserData();

	if (!spDFile->GetSubDir())
		Error("Create", "No BSP-Models in file.");

	spCXR_Model spMaster;

	int iModel = 0;
	while (spDFile->GetNext("BSPMODEL"))
	{
		if (!spDFile->GetSubDir())
		{
			M_TRACE("NOTE: Skipped empty BSP model entry\n");
			LogFile("NOTE: Skipped empty BSP model entry");
			iModel++;
			continue; // Error("Create", "Invalid BSP-Model entry.");
		}

/*		M_TRACEALWAYS("-----------------------------------------------------------------------\n");
		M_TRACEALWAYS("(CWRes_Model_XW::ReadAllModels) %s, %d\n", m_Name.Str(), iModel);
		M_TRACEALWAYS("-----------------------------------------------------------------------\n");
		MRTC_GetMemoryManager()->MemTracking_Start();
*/
		bool bDoPhysModel = (iPlayerPhys == iModel);
		bool bGlassModel = (iGlass == iModel);

		// Check if this is a glass model or not.
		spDFile->PushPosition();
		if (spDFile->GetNext("GLASS_VERSION"))
			bGlassModel |= (spDFile->GetUserData() >= GLASS_VERSION_120);
		spDFile->PopPosition();

		TPtr<CXR_Model> spModel;

		const char* pRcPrefix = m_Name.Str();
		const char* pClassName = _pClassName;
		int iRcClass = m_iRcClass;

		if (iModel == iPlayerPhys)
		{ // Force bsp4
			pRcPrefix = "XW4:";
			pClassName = "CXR_Model_BSP4";
			iRcClass = WRESOURCE_CLASS_MODEL_XW4;
		}
		else if (bGlassModel)
		{ // Force bsp4glass
			pRcPrefix = "XGL:";
			pClassName = "CXR_Model_BSP4Glass";
			iRcClass = WRESOURCE_CLASS_MODEL_GLASS;
		}
		else if (iModel != ModelNr)
		{ // Force bsp2
			pRcPrefix = "XW2:";
			pClassName = "CXR_Model_BSP2";
			iRcClass = WRESOURCE_CLASS_MODEL_XW2;
		}


		CFStr RcName = CFStrF("%.4s%s:%d", pRcPrefix, Name.Str(), iModel);
		int iRc = _pWData->ResourceExists(RcName.Str());
		if ((iRc > 0) && (iModel != ModelNr))
		{
			//M_TRACE("Skipping existing: %s\n", RcName.Str());
			spModel = safe_cast<CWRes_Model>((CWResource*)_pWData->GetResource(iRc))->GetModel();
		}
		else
		{
			//M_TRACE("Creating: %s\n", RcName.Str());
			int dMemUsed = -MRTC_MemUsed();

			spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(pClassName);
			spModel = safe_cast<CXR_Model>((CReferenceCount*)spObj);
			if (!spModel) 
				Error("ReadAllModels", "Unable to create BSP-model.");

			CBSP_CreateInfo CI;
			if (!bDoPhysModel && !bGlassModel)
			{
				CI.m_spMaster = spMaster;
				CI.m_iSubModel = iModel;
				CI.m_nSubModels = nModels;
			}

			// Fill in master model for glass bsp
			if(bGlassModel)
			{
				M_TRACEALWAYS(CStrF("Reading glass model (%d)\n", iModel));
				CI.m_spMaster = spMaster;
			}

			CWRes_Model_XW* pRC = this;
			TPtr<CWRes_Model_XW> spRc = NULL;
			if (iModel != ModelNr)
			{
				if (bDoPhysModel)
					spRc = MNew(CWRes_Model_XW4);
				else if (bGlassModel)
					spRc = MNew(CWRes_Model_Glass);
				else
					spRc = (CWRes_Model_XW*)(CReferenceCount*) MRTC_GOM()->CreateObject(MRTC_ClassName());

				if (!spRc)
					Error("ReadAllModels", "Unable to create resource object.");

				pRC = spRc;
			}

			pRC->CreateModel(spModel, spDFile, CI);

			dMemUsed += MRTC_MemUsed();

			if (iModel == ModelNr)
			{
				m_spModel = spModel;
			}
			else
			{
				spRc->Create(_pWData, RcName.Str(), NULL, iRcClass, (CXR_Model*)spModel);
				_pWData->AddResource((CWResource*)spRc);
				m_MemUsed += dMemUsed;
				pRC->m_MemUsed -= dMemUsed;

				// FIXME:
	//			m_MemUsed += dMemUsed;
	//			spRc->m_MemUsed -= dMemUsed;

			}

	//		MRTC_GetMemoryManager()->MemTracking_Report(true);
	//		MRTC_GetMemoryManager()->MemTracking_Stop();
		}

		if (iModel == 0)
			spMaster = spModel;

		iModel++;
		if (iModel > nModels)
			Error("Create", CStrF("Invalid number of BSPMODEL entries. %d > %d", iModel, nModels));

		spDFile->GetParent();
	}

	if (iModel != nModels)
		Error("Create", CStrF("Invalid number of BSPMODEL entries. %d != %d", iModel, nModels));

	spDFile->Close();
	spDFile = NULL;
	return true;
}


#else

bool CWRes_Model_XW::Create(CWorldData* _pWData, CMapData* _pMapData, const char* _pClassName)
{
	MAUTOSTRIP(CWRes_Model_XW_Create_3, false);
	MSCOPE(CWRes_Model_XW::Create, RES_XW_MODEL);
	
//		ConOut("        " + _Name);
DebugLog("(CWRes_Model_XW::Create) " + m_Name);

	int ModelNr = 0;
	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		ModelNr = Name.Val_int();
		Name = NewName;
	}

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);
	if (!CDiskUtil::FileExists(FileName)) return false;

	int iRc = 0;
	{
		RESOURCE_MEMSCOPE;
		iRc = _pMapData->GetResourceIndex("XWI:" + Name);
		m_MemUsed -= RESOURCE_MEMDELTA;
	}

	CWResource* pRc = _pMapData->GetResource(iRc);
	if (!pRc) Error("Create", "Could not create XW-index resource.");

	CWRes_XWIndex* pXWIndex = safe_cast<CWRes_XWIndex> (pRc);
	if (!pXWIndex) Error("Create", "Resource was not an object of class CWRes_XWIndex.");

	uint32 FileOffset = pXWIndex->GetModelFileOffset(ModelNr);

//LogFile(CStrF("(CWRes_Model_XW::Create) Opening model %d from offset %d", ModelNr, FileOffset));

	spCCFile spFile = DNew(CCFile) CCFile;
	if (spFile == NULL) MemError("Create");
//LogFile(CStr("(CWRes_Model_XW::Create) Opening file."));
	spFile->Open(FileName, CFILE_BINARY | CFILE_READ);

	TPtr<CDataFile> spDFile = DNew(CDataFile) CDataFile;
	if (spDFile == NULL) MemError("Read");
//LogFile(CStr("(CWRes_Model_XW::Create) Opening datafile."));
	spDFile->Open(spFile, FileOffset);
//LogFile(CStr("(CWRes_Model_XW::Create) Datafile opened."));
	CCFile* pFile = spDFile->GetFile();

	if (!spDFile->GetNext("BSPMODEL"))
		Error("Create", "Corrupt XW file.");

	if (!spDFile->GetSubDir())
		Error("Create", "Invalid BSP-Model entry.");

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(_pClassName);
	TPtr<CXR_Model> spModel = safe_cast<CXR_Model>((CReferenceCount*)spObj);
	if (!spModel) Error("ReadWorld", "Unable to create BSP-model.");
	spModel->Create(NULL, spDFile, NULL);

//		spModel->Read(spDFile);
	m_spModel = spModel;

DebugLog("(TWRes_Model_XW::Create) Done.");
	return true;
}

bool CWRes_Model_XW::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_XW_Create_4, false);
	MSCOPE(CWRes_Model_XW::Create, RES_XW_MODEL);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	return Create(_pWData, _pMapData, "CXR_Model_BSP");
}

#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW2-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*
bool CWRes_Model_XW2::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_XW2_Create, false);
	MSCOPE(CWRes_Model_XW2::Create, RES_XW_MODEL);
	
	if (!CWRes_Model_XW::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	return CWRes_Model_XW::Create(_pWData, _pMapData, "CXR_Model_BSP2");
}
*/

void CWRes_Model_XW2::CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const CBSP_CreateInfo& _CreateInfo)
{
	((CXR_Model_BSP2*)_pModel)->Create(NULL, _pDFile, NULL, _CreateInfo);
}

void CWRes_Model_XW2::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_XW2_OnLoad, MAUTOSTRIP_VOID);
	if (IsLoaded())
		return;

	ReadAllModels(m_pWData, "CXR_Model_BSP2");
	m_Flags |= WRESOURCE_LOADED;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW3-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWRes_Model_XW3::CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const CBSP_CreateInfo& _CreateInfo)
{
	((CXR_Model_BSP3*)_pModel)->Create(NULL, _pDFile, NULL, _CreateInfo);
}

void CWRes_Model_XW3::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_XW3_OnLoad, MAUTOSTRIP_VOID);
	if (IsLoaded())
		return;

	ReadAllModels(m_pWData, "CXR_Model_BSP3");
	m_Flags |= WRESOURCE_LOADED;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW4-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWRes_Model_XW4::CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const CBSP_CreateInfo& _CreateInfo)
{
	((CXR_Model_BSP4*)_pModel)->Create(NULL, _pDFile, NULL, _CreateInfo);
}

void CWRes_Model_XW4::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_XW4_OnLoad, MAUTOSTRIP_VOID);
	if (IsLoaded())
		return;

	ReadAllModels(m_pWData, "CXR_Model_BSP4");
	m_Flags |= WRESOURCE_LOADED;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Glass-Model
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWRes_Model_Glass::CreateModel(CXR_Model* _pModel, CDataFile* _pDFile, const CBSP_CreateInfo& _CreateInfo)
{
	safe_cast<CXR_Model_BSP4Glass>(_pModel)->Create(NULL, _pDFile, NULL, _CreateInfo);
}

void CWRes_Model_Glass::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_Glass_OnLoad, MAUTOSTRIP_VOID);
	if (IsLoaded())
		return;

	ReadAllModels(m_pWData, "CXR_Model_BSP4Glass");
	m_Flags |= WRESOURCE_LOADED;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-Resource
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWRes_XWResource::CWRes_XWResource()
{
	MAUTOSTRIP(CWRes_XWResource_ctor, MAUTOSTRIP_VOID);
	m_pData = NULL;
	m_Size = 0;
	m_bExists = true;
}

CWRes_XWResource::~CWRes_XWResource()
{
	MAUTOSTRIP(CWRes_XWResource_dtor, MAUTOSTRIP_VOID);
	if (m_pData)
	{
		delete[] m_pData;
	}
}

void* CWRes_XWResource::GetData()
{
	MAUTOSTRIP(CWRes_XWResource_GetData, NULL);
	return m_pData;
}

int CWRes_XWResource::GetDataSize()
{
	MAUTOSTRIP(CWRes_XWResource_GetDataSize, 0);
	return m_Size;
}

bool CWRes_XWResource::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XWResource_Create, false);
	MSCOPE(CWRes_XWResource::Create, RES_XW_RES);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	int RcNr = 0;
	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		RcNr = Name.Val_int();
		Name = NewName;
	}

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);
	if (!CDiskUtil::FileExists(FileName)) return false;

	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
//	CCFile* pFile = spDFile->GetFile();

	// Read BSP-Models
	if (!spDFile->GetNext("RESOURCES"))
	{
		ConOutL("§cf80WARNING: No resources in file: " + FileName);
		return true;
//			Error("Create", "No resources in file: " + FileName);
	}

	int nRC = spDFile->GetUserData();
	if (RcNr < 0 || RcNr >= nRC)
	{
		ConOutL("§cf80WARNING: No resources in file: " + FileName);
		return true;
//			Error("Create", CStrF("Invalid BSP-model index. (%d/%d)", RcNr, nRC));
	}

	if (!spDFile->GetSubDir())
	{
		ConOutL("§cf80WARNING: No resources in file: " + FileName);
		return true;
//			Error("Create", "No resources in file: " + FileName);
	}

	int iRc = 0;
	while (spDFile->GetNext("RESOURCE"))
	{
		if (iRc == RcNr)
		{
			int Size = spDFile->GetEntrySize();
			if (Size <= 0) Error("Create", "Invalid resource size.");
			m_pData = DNew(uint8) uint8[Size];
			if (!m_pData) MemError("Create");

			spDFile->GetFile()->Read(m_pData, Size);
			m_Size = Size;
			break;
		}
		iRc++;
	}

	if (!m_pData)
		Error("Create", CStrF("Resource %d not found in %s.", RcNr, (char*) m_Name));

	if (!spDFile->GetParent()) Error("Create", "Internal error");

	return true;
}


TAP<const uint8> CWRes_XWResource::GetResourceData(uint _iResData)
{
	MAUTOSTRIP(GetResourceData, NULL);

	TAP<const uint8> pRet;

	uint32* piBuffer = (uint32 *)m_pData;
	if (!piBuffer)
	{
		ConOutL("§cf00ERROR: Resource requested, but there are no resources in the map!");
		return pRet;
	}

	uint32 Version = piBuffer[0];
	SwapLE(Version);
	if (Version != 1000)
		return pRet; // Invalid version

	uint32 nResources = piBuffer[1];
	SwapLE(nResources);
	if (_iResData >= nResources)
		return pRet; // Index out of range

	uint32 iPos = piBuffer[_iResData + 2];
	SwapLE(iPos);

	uint32* pData = (uint32*)(m_pData + iPos);
	uint32 nLen = pData[0];
	SwapLE(nLen);

	pRet.Set((const uint8*)(pData+1), nLen);
	return pRet;
}


// Tool stuff (for XWC & Ogier)
#ifndef PLATFORM_CONSOLE
const char* CWRes_XWResource::ReadResources(CCFile& _File, TArray<CResourceData>& _lRet)
{
	uint32 Version;
	_File.ReadLE(Version);
	if (Version != 1000)
		return "Unknown version of resources";

	uint32 nResources;
	_File.ReadLE(nResources);

	TArray<uint32> IndexList;
	IndexList.SetLen(nResources);
	_File.ReadLE(IndexList.GetBasePtr(), nResources);

	for (uint i = 0; i < nResources; i++)
	{
		CResourceData Data;
		uint32 Len;
		_File.ReadLE(Len);
		Data.SetLen(Len);
		_File.ReadLE(Data.GetBasePtr(), Len);
		_lRet.Add(Data);
	}
	return NULL; // all ok

/*
  -- this stuff is part of Jesper's file format changes that was reverted...

		char str[12];
		File.Read(str,12);
		if (strncmp(str,"MOS DATAFILE",12)==0)
		{
			IsDataFile=true;
		}
		// Old non browsable format
		else
		{
			File.Seek(0);
...

	if (IsDataFile)
	{
		CDataFile DataFile;
		DataFile.Open(_Path);
		spCCFile spFile = DataFile.GetFile();
		if (!DataFile.GetNext("VERSION"))
			MSGRETURN("No version entry of resources");
		const int Version = DataFile.GetUserData();
		if(Version != 1000)
			MSGRETURN("Unknown version of resources");
		if (!DataFile.GetNext("RESOURCES"))
			MSGRETURN("No resources in resourcesfile");
		const int nResources= DataFile.GetUserData();
		DataFile.GetSubDir();
		for(int i = 0; i < nResources; i++)
		{
			TArray<unsigned char> Data;
			if (!DataFile.GetNext("RESOURCE"))
				MSGRETURN("Missing resource in resourcesfile");
			const int32 Len=DataFile.GetEntrySize();
			Data.SetLen(Len);
			spFile->Read(Data.GetBasePtr(), Len);
			lResources.Add(Data);
		}
		DataFile.GetParent();
	}
*/
}

/* -- this is part of Jesper's file format change that was reverted. Moved from Ogier
void COgierClassHandler::WriteResources(CDataFile &_DataFile)
{
	int32 iVer = 1000;
	_DataFile.BeginEntry("VERSION");
	_DataFile.EndEntry(iVer);

	int32 nResources = m_lResources.Len();
	
	_DataFile.BeginEntry("RESOURCES");
	_DataFile.EndEntry(nResources);

	int32 iPos = nResources * 4 + 8;
	_DataFile.BeginSubDir();


	for(int i = 0; i < nResources; i++)
	{
		_DataFile.BeginEntry("RESOURCE");
		int32 Len = m_lResources[i].Len();
		_DataFile.GetFile()->Write(m_lResources[i].GetBasePtr(), Len);
		_DataFile.EndEntry(0);
	}
	_DataFile.EndSubDir();
}
*/



void CWRes_XWResource::WriteResources(const TArray<CResourceData>& _lResources, CCFile& _File)
{
	uint32 Version = 1000;
	_File.WriteLE(Version);

	uint32 nResources = _lResources.Len();
	_File.WriteLE(nResources);

	uint32 iPos = nResources * 4 + 8;
	for (uint i = 0; i < nResources; i++)
	{
		_File.WriteLE(iPos);
		iPos += _lResources[i].Len() + 4;
	}

	for(int i = 0; i < nResources; i++)
	{
		int32 Len = _lResources[i].Len();
		_File.WriteLE(Len);
		_File.WriteLE(_lResources[i].GetBasePtr(), Len);
	}
}
#endif // PLATFORM_CONSOLE


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-NavGrid
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_BlockNav_Grid_GameWorld* CWRes_XWNavGrid::GetNavGrid()
{
	MAUTOSTRIP(CWRes_XWNavGrid_GetNavGrid, NULL);
	return m_spNavGrid;
}

CWRes_XWNavGrid::~CWRes_XWNavGrid()
{
}

bool CWRes_XWNavGrid::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XWNavGrid_Create, false);
	MSCOPE(CWRes_XWNavGrid::Create, RES_XW_NAVGRID);

	m_pWData = _pWData;
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);

	if (!CDiskUtil::FileExists(FileName)) return false;

	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
//	CCFile* pFile = spDFile->GetFile();

	// Read BSP-Models
	if (!spDFile->GetNext("NAVIGATIONGRID"))
		return false;
//			Error("Create", "No NAVIGATIONGRID in file.");

	if (!spDFile->GetSubDir())
		return false;
//			Error("Create", "Invalid NAVIGATIONGRID entry.");

	spDFile->Close();

	return true;
}

void CWRes_XWNavGrid::OnLoad()
{
	MAUTOSTRIP(CWRes_XWNavGrid_OnLoad, MAUTOSTRIP_VOID);
	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}
	
	CStr FileName = m_pWData->ResolveFileName("WORLDS\\" + Name);
	
	if (!CDiskUtil::FileExists(FileName))
		Error("OnLoad", "!");
	
	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
//	CCFile* pFile = spDFile->GetFile();
	
	// Read BSP-Models
	if (!spDFile->GetNext("NAVIGATIONGRID"))
		Error("OnLoad", "!");
	
	if (!spDFile->GetSubDir())
		Error("OnLoad", "!");
	
	MRTC_SAFECREATEOBJECT_NOEX(spNavGrid, "CXR_BlockNav_Grid_GameWorld", CXR_BlockNav_Grid_GameWorld);
	
	if (!spNavGrid) Error("Create", "Unable to instance CXR_BlockNav_Grid object.");
	m_spNavGrid = spNavGrid;
//	m_spNavGrid->Create(64);		// CWorld_Server::World_Init() run Create() nowdays...
	m_spNavGrid->Read(spDFile);
	spDFile->GetParent();
	
	spDFile->Close();
	spDFile = NULL;
}

void CWRes_XWNavGrid::OnUnload()
{
	MAUTOSTRIP(CWRes_XWNavGrid_OnUnload, MAUTOSTRIP_VOID);
	m_spNavGrid = NULL;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-NavGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_NavGraph* CWRes_XWNavGraph::GetNavGraph()
{
	MAUTOSTRIP(CWRes_XWNavGraph_GetNavGraph, NULL);
	return m_spNavGraph;
}

bool CWRes_XWNavGraph::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XWNavGraph_Create, false);
	MSCOPE(CWRes_XWNavGraph::Create, RES_XW_NAVGRAPH);

	m_pWData = _pWData;

	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}
	
	CStr FileName = m_pWData->ResolveFileName("WORLDS\\" + Name);
	
	if (!CDiskUtil::FileExists(FileName)) return false;
	
	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
	
	if (!spDFile->GetNext("NAVIGATIONGRAPH"))
		return false;
	
	if (!spDFile->GetSubDir())
		return false;
	
	return true;
}

void CWRes_XWNavGraph::OnLoad()
{
	MAUTOSTRIP(CWRes_XWNavGraph_OnLoad, MAUTOSTRIP_VOID);
	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}
	
	CStr FileName = m_pWData->ResolveFileName("WORLDS\\" + Name);
	
	if (!CDiskUtil::FileExists(FileName))
		Error("OnLoad", "!");
	
	TPtr<CDataFile> spDFile = MNew(CDataFile);
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);
	
	if (!spDFile->GetNext("NAVIGATIONGRAPH"))
		Error("OnLoad", "!");
	
	if (!spDFile->GetSubDir())
		Error("OnLoad", "!");
	
	MRTC_SAFECREATEOBJECT_NOEX(spNavGraph, "CXR_NavGraph", CXR_NavGraph);
	
	if (!spNavGraph) 
		Error("Create", "Unable to instance CXR_NavGraph object.");
	m_spNavGraph = spNavGraph;
	m_spNavGraph->Read(spDFile);
	spDFile->GetParent();
	
	spDFile->Close();
	spDFile = NULL;
	
	//Hash must be built for navgraph as well, but this requries knowledge of navgrid, 
	//so it must be done in CWorld_Server::World_Init	
}

void CWRes_XWNavGraph::OnUnload()
{
	MAUTOSTRIP(CWRes_XWNavGraph_OnUnload, MAUTOSTRIP_VOID);
	m_spNavGraph = NULL;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| XW-File
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for a XW-file
\*____________________________________________________________________*/

bool CWRes_XW::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XW_Create, false);
/*	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		Name = NewName;
	}

	CStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);

	if (!CDiskUtil::FileExists(FileName)) return false;

	TPtr<CDataFile> spDFile = DNew(CDataFile) CDataFile;
	if (spDFile == NULL) MemError("Read");
	spDFile->Open(FileName);

	CStr Entry;
	while(Entry = spDFile->GetNext() != 


*/
	return false;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource for the "OBJECTATTRIBS" chunk in a XW-file
\*_____________________________________________________________________________________*/
bool CWRes_ObjectAttribs::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_ObjectAttribs_Create, false);
	MSCOPE(CWRes_ObjectAttribs::Create, WORLD_SERVER);

	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) 
		return false;

	CStr Name = m_Name.Copy(4, 100000);
	if (Name.Find(":") >= 0)
		Name = Name.GetStrSep(":");

	if (Name.GetFilenameExtenstion() == "") 
		Name += ".XW";

	CFStr FileName = _pWData->ResolveFileName("WORLDS\\" + Name);
	if (!CDiskUtil::FileExists(FileName)) 
		return false;

	// Read Object-Attributes
	CDataFile DFile;
	DFile.Open(FileName);
	DFile.PushPosition();
	if (DFile.GetNext("OBJECTATTRIBS")) 
	{
		int nObjects = DFile.GetUserData();
		int Version = DFile.GetUserData2();
		m_lspObjectRegs.SetLen(nObjects);
		for (int i = 0; i < nObjects; i++)
		{
			spCRegistry spObj = REGISTRY_CREATE;
			if (spObj == NULL) MemError("Read");
			spObj->Read(DFile.GetFile(), Version);
			m_lspObjectRegs[i] = spObj;
		}
		DFile.Close();
		if (m_pRegDir)
			delete m_pRegDir;
		m_pRegDir = DNew(CRegDirNormal) CRegDirNormal(this);
		return true;
	}
	DFile.PopPosition();

	if (DFile.GetNext("OBJATTRCONST"))
	{
		DFile.GetSubDir();
		m_CompiledReg.Read(&DFile);
		m_spCompiledRegRoot = m_CompiledReg.GetRoot();

		if (m_pRegDir)
			delete m_pRegDir;
		m_pRegDir = DNew(CRegDirConst) CRegDirConst(this);
		return true;
	}
	return false;
}


