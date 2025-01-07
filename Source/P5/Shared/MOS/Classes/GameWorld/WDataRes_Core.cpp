#include "PCH.h"

#include "WDataRes_Core.h"
#include "WEvilTemplate.h"

//#pragma optimize("",off)

MRTC_IMPLEMENT_DYNAMIC(CWRes_DLL, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Registry, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Model, CWResource);
#ifndef MDISABLE_CLIENT_SERVER_RES
MRTC_IMPLEMENT_DYNAMIC(CWRes_Class, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Template, CWResource);
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_Model
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWRes_Model::OnPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWRes_Model_OnPrecache, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model::OnPrecache, RES_XMD);

	CXR_Model* pModel = GetModel();
	if (pModel)
		pModel->OnPrecache(_pEngine, 0);
}

void CWRes_Model::OnPostPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWRes_Model_OnPostPrecache, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Model::OnPostPrecache, RES_XMD);

	CXR_Model* pModel = GetModel();
	if (pModel)
		pModel->OnPostPrecache(_pEngine);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| DLL, STATIC LIBRARY
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifdef WCLIENT_STATICLIBRARY

// The DLL resource is just a dummy when not using dynamic-libraries.

bool CWRes_DLL::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_DLL_Create, false);
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	return true;
};

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| DLL, DYNAMIC LIBRARY
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef WCLIENT_STATICLIBRARY

bool CWRes_DLL::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_DLL_Create_2, false);
	MSCOPE(CWRes_DLL::Create, RES_DLL);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

//		ConOut("        " + _Name);
	MACRO_GetSystem;
	CStr FileName = pSys->GetModuleName(_pWData->ResolveFileName(CStr(_pName).CopyFrom(4)), true);

	if (!CDiskUtil::FileExists(FileName))
	{
		ConOutL(CStrF("(CWRes_DLL::Create) DLL doesn't exist: %s", _pName));
		return false;
	}

	if (MRTC_GetObjectManager()->GetDllLoading())
	{
		M_TRY
		{
			MRTC_GetObjectManager()->GetClassRegistry()->LoadClassLibrary(FileName);
		}
		M_CATCH(
		catch(CCException)
		{
			return false;
		}
		)
	}

	m_FileName = FileName;
	return true;
}

CWRes_DLL::~CWRes_DLL()
{
	MAUTOSTRIP(CWRes_DLL_dtor, MAUTOSTRIP_VOID);
	if (m_FileName.Len())
	{
		if (MRTC_GetObjectManager()->GetDllLoading())
		{
			M_TRY
			{
				MRTC_GetObjectManager()->GetClassRegistry()->UnloadClassLibrary(m_FileName);
			}
			M_CATCH(
			catch(CCException)
			{
			}
			)
		}
	}
}

#endif

#ifndef MDISABLE_CLIENT_SERVER_RES

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CLASS
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWRes_Class::CWRes_Class()
{
	MAUTOSTRIP(CWRes_Class_ctor, MAUTOSTRIP_VOID);
	m_pRTC = NULL;
}

MRTC_CRuntimeClass_WObject* CWRes_Class::GetRuntimeClass() const
{
	MAUTOSTRIP(CWRes_Class_GetRuntimeClass, NULL);
	return m_pRTC;
}

void CWRes_Class::ClearStats()
{
	MAUTOSTRIP(CWRes_Class_ClearStats, MAUTOSTRIP_VOID);
	m_Stats.Clear();
}

CWorldData::CWD_ClassStatistics* CWRes_Class::GetStats()
{
	MAUTOSTRIP(CWRes_Class_GetStats, NULL);
	return &m_Stats;
}

bool CWRes_Class::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Class_Create, false);
	MSCOPE(CWRes_Class::Create, RES_CLASS);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	// DebugLog("(CWRes_Class::Create)" + _Name);
	int Len = CStrBase::StrLen(_pName);
	if (Len < 5) return false;

	CFStr ClassName = CFStrF("CWObject_%s", &_pName[4]);

	MRTC_CRuntimeClass* pRTC = MRTC_GOM()->GetClassRegistry()->GetRuntimeClass(ClassName);
	if (!pRTC) return false;
	m_pRTC = (MRTC_CRuntimeClass_WObject*) pRTC;
	m_Name = _pName;

	// DebugLog("(CWRes_Class::Create) Done.");
	return true;
}


// -------------------------------------------------------------------
//  TEMPLATE
// -------------------------------------------------------------------
CWRes_Template::CWRes_Template()
{
	m_pWData = NULL;
}

CWRes_Template::~CWRes_Template()
{
	m_lspObjectReg.Clear();
#ifdef M_ENABLE_REGISTRYCOMPILED
	m_lspCompiledObjectReg.Clear();
#endif
}

bool CWRes_Template::Load()
{
	OnUnload();

	CFStr Name(m_Name);
	CFStr TplName(Name.CopyFrom(4));
	
	if(m_pWData->GetGameReg())
	{
		CRegistry *pTemplates = m_pWData->GetGameReg()->Find("SERVER\\TEMPLATES_EVAL");
		if(pTemplates)
		{
			CRegistry *pReg = pTemplates->FindChild(TplName);
			if (pReg)
			{
				CRegistry *pObjs = pReg->FindChild("TEMPLATES");
				if (pObjs)
				{
					int nObj = pObjs->GetNumChildren();
					m_lspObjectReg.SetLen(nObj);
					m_lObjectMat.SetLen(nObj);
					for (int i = 0; i < nObj; ++i)
					{
						CRegistry *pChild = pObjs->GetChild(i);
						
						int iOrder = pChild->GetValuei("ORDER", i);
						m_lspObjectReg[iOrder] = pChild;
						m_lObjectMat[i].Unit();
					}
					CRegistry *pMat = pObjs->FindChild("MAT");
					if (pMat)
					{
						M_ASSERT(pMat->Anim_ThisGetNumSeq() == 1, "Must be");
						M_ASSERT(pMat->Anim_ThisGetNumKF() == nObj, "Must be");

						for (int i = 0; i < nObj; ++i)
						{
							pMat->Anim_ThisGetKFValueaf(0, i, 16, (fp32 *)m_lObjectMat[i].k);
						}
					}
				}
				else
				{
					m_lspCompiledObjectReg.SetLen(0);
					m_lspObjectReg.SetLen(0);
				}
			}
			else
			{
				m_lspCompiledObjectReg.SetLen(0);
				m_lspObjectReg.SetLen(0);
			}
			m_Flags |= WRESOURCE_LOADED;
			return true;
		}

		pTemplates = m_pWData->GetGameReg()->Find("SERVER\\TEMPLATES");
		if(pTemplates)
		{
			spCRegistry spReg = REGISTRY_CREATE;
			if (!spReg)
				return false;

			spReg->AddKey("CLASSNAME", TplName.Str());
			TArray<spCRegistry> lspObjectReg;
			Entity_EvalTemplate_r(pTemplates, spReg, lspObjectReg);
			m_lObjectMat.SetLen(lspObjectReg.Len());

			// Simple recursion check. Delete all objects that have the same class as the original object.
			for(int i = lspObjectReg.Len() - 1; i >= 0; i--)
			{
				spCRegistry spClassName = lspObjectReg[i]->FindChild("CLASSNAME");
				if(!spClassName || spClassName->GetThisValue().CompareNoCase(TplName) == 0) 
				{
					ConOutL(CStrF("§cf80WARNING: (CWRes_Template::Create) Unknown object %i in template '%s'.", i, TplName.Str() ));
					lspObjectReg[i] = NULL;
//					m_lspObjectReg.Del(i);
//					m_lspObjectMat.Del(i);
				}
				else
				{
//						ConOutL(CStr("CWRes_Template::Create, ") + spClassName->GetThisValue() + ": " + (char*)TplName);
					m_lObjectMat[i] = Entity_GetPosition(lspObjectReg[i]);
					lspObjectReg[i]->DeleteKey("ORIGIN");
					lspObjectReg[i]->DeleteKey("ANGLE");
					lspObjectReg[i]->DeleteKey("ANGLES");
				}
			}
			
			// Delete
			int nObj = 0;
			{
				for(int i = 0; i < lspObjectReg.Len(); i++)
					if (lspObjectReg[i] != NULL)
					{
						if (nObj != i)
						{
							lspObjectReg[nObj] = lspObjectReg[i];
							m_lObjectMat[nObj] = m_lObjectMat[i];
						}
						nObj++;
					}

				lspObjectReg.SetLen(nObj);
				m_lObjectMat.SetLen(nObj);
		/*		for (int i = 0; i < lspObjectReg.Len(); ++i)
				{
					lspObjectReg[i]->XRG_Write(CStrF("D:\\RegSave%03d.xrg", i));
				}*/

			}

			// Compile regs
			{
#ifdef M_ENABLE_REGISTRYCOMPILED
				m_lspCompiledObjectReg.SetLen(nObj);
				m_lspObjectReg.SetLen(nObj);
				
				for(int i = 0; i < nObj; i++)
				{
					m_lspCompiledObjectReg[i] = MNew(CRegistryCompiled);
					if (!m_lspCompiledObjectReg[i])
						MemError("Create");
					m_lspCompiledObjectReg[i]->Compile(lspObjectReg[i], true);
					m_lspObjectReg[i] = m_lspCompiledObjectReg[i]->GetRoot();
//					m_lspObjectReg[i] = lspObjectReg[i];

				}
#else
				m_lspObjectReg.SetLen(nObj);
				
				for(int i = 0; i < nObj; i++)
				{
					m_lspObjectReg[i] = lspObjectReg[i];
				}
				
#endif
			}
			lspObjectReg.Destroy();
			m_Flags |= WRESOURCE_LOADED;
			return true;
		}
	}

	ConOutL(CStrF("§cf80WARNING: (CWRes_Template::Create) Template '%s' could not be evaluated.", m_Name.Str() ));
	m_Flags |= WRESOURCE_LOADED;
	return true;
}

bool CWRes_Template::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Template_Create, false);
	MSCOPE(CWRes_Template::Create, RES_TEMPLATE);

	m_pWData = _pWData;
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;
	m_Name = _pName;

	return Load();
}

void CWRes_Template::OnLoad()
{
	if (!(m_Flags & WRESOURCE_LOADED))
		if (!Load())
			Error("OnLoad", "Failed.");
}

void CWRes_Template::OnUnload()
{
	m_lObjectMat.Destroy();
	m_lspObjectReg.Destroy();;
#ifdef M_ENABLE_REGISTRYCOMPILED
	m_lspCompiledObjectReg.Destroy();;
#endif
	m_Flags &= ~WRESOURCE_LOADED;
}


#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| REGISTRY
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWRes_Registry::CWRes_Registry()
{
	m_pWData = NULL;
}

CWRes_Registry::~CWRes_Registry()
{
	MAUTOSTRIP(CWRes_Registry_dtor, MAUTOSTRIP_VOID);
#ifdef M_ENABLE_REGISTRYCOMPILED
	if (m_spRegCompiled != NULL && 
		m_spReg != NULL && 
		m_spReg->MRTC_ReferenceCount() != 1)
	{
		M_TRACE("(~CWRes_Registry) ERROR: Invalid reference count %d, %s", m_spReg->MRTC_ReferenceCount(), m_Name.Str());
		M_ASSERT(0, "!");
	}
#endif
	m_spReg = NULL;
#ifdef M_ENABLE_REGISTRYCOMPILED
	m_spRegCompiled = NULL;
#endif

}

CRegistry* CWRes_Registry::GetRegistry()
{
	MAUTOSTRIP(CWRes_Registry_GetRegistry, NULL);
	if (!m_spReg)
		OnLoad();
	return m_spReg;
}

bool CWRes_Registry::Load()
{
#ifdef M_ENABLE_REGISTRYCOMPILED
	CStr FileName = m_pWData->ResolveFileName(m_Name.CopyFrom(4) + ".XCR", false);	// Must resolve with bAllowCache == FALSE !!!
	if (CDiskUtil::FileExists(FileName))
	{
		m_spRegCompiled = MNew(CRegistryCompiled);
		if (!m_spRegCompiled)
			MemError("Create");
		{
//			D_NOXDF;
			m_spRegCompiled->Read_XCR(FileName);
		}
		m_spReg = m_spRegCompiled->GetRoot();
	}
	else
#endif
	{
		TArray<CStr> lDefines;
		MACRO_GetSystemRegistry(pSysReg);
		CRegistry* pRegDef = (pSysReg) ? pSysReg->GetDir("GAMECONTEXT\\REGISTRYDEFINES") : NULL;
		if (pRegDef)
		{
			for(int i = 0; i < pRegDef->GetNumChildren(); i++)
				lDefines.Add(pRegDef->GetName(i));
		}

		m_spReg = REGISTRY_CREATE;
		if (!m_spReg) MemError("Create");
		CStr FileName = m_pWData->ResolveFileName(m_Name.Copy(4, 100000) + ".XRG", false);	// Must resolve with bAllowCache == FALSE !!!
		if (!CDiskUtil::FileExists(FileName)) 
			return false;
		m_spReg->XRG_Read(FileName, lDefines);
		m_spReg->SimulateRegistryCompiled(true);
	}
	m_Flags |= WRESOURCE_LOADED;
	return true;
}

bool CWRes_Registry::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Registry_Create, false);
	MSCOPE(CWRes_Registry::Create, RES_REGISTRY);

	m_pWData = _pWData;

	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	m_Name = _pName;
	return Load();
}

void CWRes_Registry::OnLoad()
{
	if (!(m_Flags & WRESOURCE_LOADED))
		if (!Load())
			Error("OnLoad", "Failed.");
}

void CWRes_Registry::OnUnload()
{
	m_spReg = NULL;
	m_spRegCompiled = NULL;
	m_Flags &= ~WRESOURCE_LOADED;
}

