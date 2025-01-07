
#include "PCH.h"

#include "WDataRes_Models.h"
#include "../../XRModels/Model_TriMesh/WTriMesh.h"

//#define LOG_CTM_INCLUDE

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)

MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_XMD, CWRes_Model);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_Custom, CWRes_Model);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model_Custom_File, CWRes_Model_Custom);

// -------------------------------------------------------------------
//  XMD-MODEL
// -------------------------------------------------------------------
CWRes_Model_XMD::CWRes_Model_XMD()
{
	MAUTOSTRIP(CWRes_Model_XMD_ctor, MAUTOSTRIP_VOID);
	m_bExists = true;
	m_pWData = NULL;
}

CXR_Model* CWRes_Model_XMD::GetModel()
{
	MAUTOSTRIP(CWRes_Model_XMD_GetModel, NULL);
	if (m_spModelProxy != NULL)
		return (CXR_Model_VariationProxy*)m_spModelProxy;
	else
		return (CXR_Model_TriangleMesh*)m_spModel;

/*	if (!m_spModel && m_bExists)
	{
		if (!IsLoaded())
			ReadModel();

		if (!m_spModel) m_bExists = false;
	}
	return (CXR_Model_TriangleMesh*)m_spModel;*/
}

void CWRes_Model_XMD::ReadModel()
{
	MAUTOSTRIP(CWRes_Model_XMD_ReadModel, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::ReadModel, RES_XMD);
	
	TProfileDef(T);
	{
		TMeasureProfile(T);

	//		ConOut("        " + m_Name);

		CFStr Params = m_Name.Copy(4, 100000);
		CFStr FileN = Params.GetStrSep(":");
		CFStr Variation = Params.GetStrSep(";");

		if (Variation.Len())
		{
			MSCOPESHORT(GetOriginalModel);
			RESOURCE_MEMSCOPE;
			CFStr OrigName = "XMD:" + FileN;
			if (Params.Len())
				OrigName += ":;" + Params;
			int iResOrig = m_pWData->GetResourceIndex(OrigName, NULL);
			m_MemUsed -= RESOURCE_MEMDELTA;

			if (iResOrig > 0)
			{
				CWResource* pResOrig = m_pWData->GetResource(iResOrig);

				CWRes_Model* pResModel = safe_cast<CWRes_Model>(pResOrig);
				CXR_Model_TriangleMesh* pTriMesh = safe_cast<CXR_Model_TriangleMesh>(pResModel->GetModel());
				m_spModel = pTriMesh;
			}
		}

		if (m_spModel == NULL)
		{
			CFStr FileName = m_pWData->ResolveFileName("MODELS\\" + FileN + ".XMD");
			if (!CDiskUtil::FileExists(FileName))
			{
				ConOutL(CStrF("§cf80WARNING: %s doesn't exist.", (const char*) FileName));

				FileName = m_pWData->ResolveFileName("MODELS\\COORDSYS.XMD");
				if (!CDiskUtil::FileExists(FileName))
				{
					ConOutL(CStrF("§cf80WARNING: %s doesn't exist.", (const char*) FileName));
					return;
				}
			}

			if (!CDiskUtil::FileExists(FileName))
			{
				m_bExists = false;
				return;
			}

			spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_Model_TriangleMesh");
			m_spModel = safe_cast<CXR_Model_TriangleMesh>((CReferenceCount*)spObj);
			if (!m_spModel) Error("ReadModel", "Unable to instance CXR_Model_TriangleMesh object.");

			m_spModel->Create(Params);
			m_spModel->Read(FileName);
		}

		int iVariation = m_spModel->GetVariationIndex(Variation.Str());
		if (iVariation > 0)
		{
			m_spModelProxy = MNew(CXR_Model_VariationProxy);
			if (!m_spModelProxy) Error("ReadModel", "Unable to create CXR_Model_VariationProxy object");

			m_spModelProxy->Create((CXR_Model_TriangleMesh*)m_spModel, iVariation);
		}

	}

// ConOutL(CStrF("(CWRes_Model_XMD::ReadModel) %s, %f", m_Name.Str(), T / GetCPUFrequency() ));
}

bool CWRes_Model_XMD::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_XMD_Create, false);
	MSCOPE(CWRes_Model_XMD::Create, RES_XMD);
	
	if (!CWRes_Model::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	m_pWData = _pWData;
//		ConOut("        " + _Name);

	m_Name = _pName;

	return true;
}

void CWRes_Model_XMD::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_XMD_OnLoad, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnLoad, RES_XMD);
	
	if (IsLoaded()) return;
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys->GetEnvironment()->GetValue("rs_preload_xmd", "1").Val_int() == 0) return;

	ReadModel();
//	GetModel();
}

void CWRes_Model_XMD::OnUnload()
{
	MAUTOSTRIP(CWRes_Model_XMD_OnUnload, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnUnload, RES_XMD);

	m_spModel = NULL;
	m_spModelProxy = NULL;
}

void CWRes_Model_XMD::OnPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWRes_Model_XMD_OnPrecache, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnPrecache, RES_XMD);

	OnLoad();

	CWRes_Model::OnPrecache(_pEngine);
}

void CWRes_Model_XMD::OnPostPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWRes_Model_XMD_OnPostPrecache, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnPostPrecache, RES_XMD);

	CWRes_Model::OnPostPrecache(_pEngine);
}

void CWRes_Model_XMD::OnRefresh()
{
	MAUTOSTRIP(CWRes_Model_XMD_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnRefresh, RES_XMD);

//--Add if needed:		if (m_spModelProxy != NULL)
//--Add if needed:			m_spModelProxy->OnResourceRefresh();  else
	if (m_spModel != NULL)
		m_spModel->OnResourceRefresh();
}

void CWRes_Model_XMD::OnHibernate()
{
	MAUTOSTRIP(CWRes_Model_XMD_OnHibernate, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_XMD::OnHibernate, RES_XMD);

//--Add if needed:		if (m_spModelProxy != NULL)
//--Add if needed:	 		m_spModelProxy->OnHibernate();  else 
	if (m_spModel != NULL)
	{
		m_spModel->OnHibernate();
//		ConOutL(CStrF("(CWRes_Model_XMD::OnHibernate) Destroying trimesh, %s", m_Name.Str()));
//		m_spModel = NULL;
	}
}

// -------------------------------------------------------------------
//  CUSTOM-MODEL
// -------------------------------------------------------------------
CXR_Model* CWRes_Model_Custom::GetModel()
{
	MAUTOSTRIP(CWRes_Model_Custom_GetModel, NULL);
	return m_spModel;
}

bool CWRes_Model_Custom::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_Custom_Create, false);
	MSCOPE(CWRes_Model_Custom::Create, RES_XMD);
	
	if (!CWRes_Model::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
//	m_Name = _pName;		// JK: ok who added this shit? it breaks the resource manager since there is no length test here

#ifdef LOG_CTM_INCLUDE
	ConOut("        " + _Name);
#endif

	DebugLog("(CWRes_Model_Custom::Create) " + m_Name);

	CStr Name = m_Name.Copy(4, 100000);
	CStr ClassName = Name.GetStrSep(":");
	CStr Params = Name;

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_Model_" + ClassName);
	spCXR_Model spModel = safe_cast<CXR_Model>((CReferenceCount*)spObj);
	if (!spModel) return false;

	// Do we need a registry in the model when creating it?
	if (ClassName.CompareNoCase("EFFECTSYSTEM") == 0 && Params.Len() > 0)
	{
		CStr Reg = Params;
		int iRegRc = _pMapData->GetResourceIndex_Registry(CFStrF("Registry\\%s", Reg.GetStrSep(",").GetStr()));
		CRegistry* pRegistry = _pMapData->GetResource_Registry(iRegRc);
		if (pRegistry)
		{
			aint RegPtr = (aint)pRegistry;
			spModel->SetParam(CXR_MODEL_PARAM_REGISTRY, (aint)RegPtr);
		}
		else
		{
			ConOutL("cf00ERROR (CWRes_Model_Custom::Create): Failed to set registry parameter!");
		}
	}

	spModel->Create((char*) Params);
	m_spModel = spModel;

	DebugLog("(CWRes_Model_Custom::Create) Done.");
	return true;
}

// -------------------------------------------------------------------
//  CUSTOM-MODEL FILE
// -------------------------------------------------------------------
bool CWRes_Model_Custom_File::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Model_Custom_File_Create, false);
	MSCOPE(CWRes_Model_Custom_File::Create, RES_XMD);
	
	if (!CWRes_Model::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

#ifdef LOG_CTM_INCLUDE
	ConOut(CStrF("        %s", _pName);
#endif

	m_pWData = _pWData;
	OnLoad();

	return true;
}

void CWRes_Model_Custom_File::OnLoad()
{
	MAUTOSTRIP(CWRes_Model_Custom_File_OnLoad, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_Custom_File::OnLoad, RES_XMD);
	
	if (m_spModel != NULL)
		return;

	CStr Name = m_Name.Copy(4, 100000);
	CStr ClassName = Name.GetStrSep(":");
	CStr File = Name.GetStrSep(",");
	CStr FileName = m_pWData->ResolveFileName(File);
	CStr Params = Name;

//ConOutL("(CWRes_Model_Custom_File::Create) ClassName: " + ClassName);
//ConOutL("(CWRes_Model_Custom_File::Create) Params: " + Params);

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_Model_" + ClassName);
	spCXR_Model spModel = safe_cast<CXR_Model>((CReferenceCount*)spObj);
	if (!spModel) return;

	spModel->Create(FileName + "," + Params);
	m_spModel = spModel;

	DebugLog("(CWRes_Model_Custom_File::Create) Done.");
}

void CWRes_Model_Custom_File::OnUnload()
{
	MAUTOSTRIP(CWRes_Model_Custom_File_OnUnload, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model_Custom_File::OnUnload, RES_XMD);
	
	m_spModel = NULL;
}

