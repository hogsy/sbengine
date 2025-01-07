#include "PCH.h"
#include "WModel_Multi.h"

#include "../Model_BSP/WBSPModel.h"
#include "../Model_BSP2/WBSP2Model.h"
#include "../Model_BSP3/WBSP3Model.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Multi, CReferenceCount);
IMPLEMENT_OPERATOR_NEW(CXR_Model_Multi);


#define Traverse(cmd)								\
{													\
for(int c = 0; c < GetNumModels(); c++)				\
	GetModel(c)->cmd;								\
}

CXR_Model_Multi::CXR_Model_Multi()
{
	SetThreadSafe(true);
}

int CXR_Model_Multi::GetNumModels()
{
	MAUTOSTRIP(CXR_Model_Multi_GetNumModels, 0);
	return Min(m_lPositions.Len(), m_lspModels.Len());
}

CXR_Model *CXR_Model_Multi::GetModel(int _iModel)
{
	MAUTOSTRIP(CXR_Model_Multi_GetModel, 0);
	return m_lspModels[_iModel];
}

CMat4Dfp32 CXR_Model_Multi::GetPosition(int _iModel)
{
	MAUTOSTRIP(CXR_Model_Multi_GetPosition, CMat4Dfp32());
	return m_lPositions[_iModel];
}

aint CXR_Model_Multi::GetParam(int _Param)
{
	MAUTOSTRIP(CXR_Model_Multi_GetParam, 0);
	if(GetNumModels() == 0)
		return 0;

	return m_lspModels[0]->GetParam(_Param);
}

void CXR_Model_Multi::SetParam(int _Param, aint _Value)
{
	MAUTOSTRIP(CXR_Model_Multi_SetParam, MAUTOSTRIP_VOID);
	Traverse(SetParam(_Param, _Value));
}

int CXR_Model_Multi::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_Multi_GetParamfv, 0);
	if(GetNumModels() == 0)
		return 0;

	return m_lspModels[0]->GetParamfv(_Param, _pRetValues);
}

void CXR_Model_Multi::SetParamfv(int _Param, const fp32* _pValues)
{
	MAUTOSTRIP(CXR_Model_Multi_SetParamfv, MAUTOSTRIP_VOID);
	Traverse(SetParamfv(_Param, _pValues));
}

fp32 CXR_Model_Multi::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Multi_GetBound_Sphere, 0.0f);
	// TODO
	if(GetNumModels() == 0)
		return 0;

	return m_lspModels[0]->GetBound_Sphere(_pAnimState);
}

void CXR_Model_Multi::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Multi_GetBound_Box, MAUTOSTRIP_VOID);
	// TODO
	_Box.m_Min = 0;
	_Box.m_Max = 0;

	if(GetNumModels() == 0)
		return;

	m_lspModels[0]->GetBound_Box(_Box, _pAnimState);
}

CXR_PhysicsModel* CXR_Model_Multi::Phys_GetInterface()
{
	if(GetNumModels() == 0)
		return NULL;

	return m_lspModels[0]->Phys_GetInterface();
}

void CXR_Model_Multi::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Multi_PreRender, MAUTOSTRIP_VOID);
	int nModels = GetNumModels();
	for(int i = 0; i < nModels; i++)
	{
		CMat4Dfp32 Pos;
		m_lPositions[i].Multiply(_WMat, Pos);
		m_lspModels[i]->PreRender(_pEngine, _pViewClip, _pAnimState, Pos, _VMat, _Flags);
	}
}

void CXR_Model_Multi::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Multi_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Multi::OnRender);
	int nModels = GetNumModels();
	for(int i = 0; i < nModels; i++)
	{
		CMat4Dfp32 Pos;
		m_lPositions[i].Multiply(_WMat, Pos);
		m_lspModels[i]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, Pos, _VMat, _Flags);
	}
}

void CXR_Model_Multi::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_Multi_OnPrecache, MAUTOSTRIP_VOID);
	int nModels = GetNumModels();
	for(int i = 0; i < nModels; i++)
		m_lspModels[i]->OnPrecache(_pEngine, _iVariation);
}

void CXR_Model_Multi::OnRefreshSurfaces()
{
	MAUTOSTRIP(CXR_Model_Multi_OnRefreshSurfaces, MAUTOSTRIP_VOID);
	int nModels = GetNumModels();
	for(int i = 0; i < nModels; i++)
		m_lspModels[i]->OnRefreshSurfaces();
}

void CXR_Model_Multi::AddToEngine(CXR_Engine *_pEngine, const CMat4Dfp32 &_Mat, CXR_AnimState &_AnimState, int _OnRenderFlags, bool _bSkipFirst)
{
	MAUTOSTRIP(CXR_Model_Multi_AddToEngine, MAUTOSTRIP_VOID);
	int nModels = GetNumModels();
	for(int i = (_bSkipFirst != false); i < nModels; i++)
	{
		if(m_lspModels[i]->View_GetInterface())
		{
			CMat4Dfp32 Pos;
			m_lPositions[i].Multiply(_Mat, Pos);
			_pEngine->Render_AddModel(m_lspModels[i], Pos, _AnimState, XR_MODEL_STANDARD, _OnRenderFlags);
		}
	}
}

CXR_Model *CXR_Model_Multi::GetViewClipModel(CMat4Dfp32 &_Mat)
{
	MAUTOSTRIP(CXR_Model_Multi_GetViewClipModel, MAUTOSTRIP_VOID);
	int nModels = GetNumModels();
	for(int i = 0; i < nModels; i++)
	{
		if(m_lspModels[i]->View_GetInterface())
		{
			_Mat = m_lPositions[i];
			return m_lspModels[i];
		}
	}
	return NULL;
}

bool CXR_Model_Multi::CreateFromXW(const char *_pFileName, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Multi_CreateFromXW, false);
	if(!CDiskUtil::FileExists(_pFileName))
		return false;

	CDataFile DFile;
	DFile.Open(_pFileName);

	int PreferedXRMode = _Flags & MASK_XRMODE;
	int XRMode = 0;
	DFile.PushPosition();
	if(!DFile.GetNext("OBJECTATTRIBS"))
		Error("CreateFromXW", "No OBJECTATTRIBS entry.");

	int nObjects = DFile.GetUserData();
	int Version = DFile.GetUserData2();
	for(int i = 0; i < nObjects; i++)
	{
		spCRegistry spReg = REGISTRY_CREATE;
		if (!spReg)
			MemError("CreateFromXW");
		spReg->Read(DFile.GetFile(), Version);

		CRegistry *pBSP = spReg->FindChild("BSPMODELINDEX");
		if(pBSP)
		{
			CMat4Dfp32 Pos;
			Pos.Unit();
			CRegistry *pMode = spReg->FindChild("XR_MODE");
			if(pMode)
				XRMode = pMode->GetThisValuei();
			CRegistry *pAngles = spReg->FindChild("ANGLES");
			if(pAngles)
			{
				CVec3Dfp32 v;
				v.ParseString(pAngles->GetThisValue());
				v *= 1.0f/360.0f;
				v.CreateMatrixFromAngles(0, Pos);
			}
			CRegistry *pOrigin = spReg->FindChild("ORIGIN");
			if(pOrigin)
			{
				CVec3Dfp32 V;
				pOrigin->GetThisValueaf(3, V.k);
				V.SetRow(Pos, 3);
			}

			int iBSP = pBSP->GetThisValuei();
			m_lPositions.SetLen(Max(iBSP + 1, m_lPositions.Len()));
			m_lPositions[iBSP] = Pos;
		}
	}

	DFile.PopPosition();
	DFile.PushPosition();

	if (!DFile.GetNext("BSPMODELS"))
		Error("CreateFromXW", "No BSP-Models in file.");

	int nModels = DFile.GetUserData();
	if(!DFile.GetSubDir())
		Error("CreateFromXW", "BSPMODELS was not a subdir.");
	
	spCXR_Model spFirst;
	while(DFile.GetNext("BSPMODEL"))
	{
		DFile.PushPosition();
		
		if(!DFile.GetSubDir())
			Error("CreateFromXW", "BSPMODEL was not a subdir.");

		CBSP_CreateInfo CI;
		CI.m_spMaster = spFirst;
		CI.m_iSubModel = m_lspModels.Len();
		CI.m_nSubModels = nModels;
		if(!(_Flags & FLAGS_MAKEPHYSSURFACEINVISIBLE))
			CI.m_Flags = CBSP_CreateInfo::FLAGS_MAKEPHYSSURFACESVISIBLE;

		spCXR_Model spModel;
		if(XRMode == 0)
		{
			spCXR_Model_BSP spBSP = MNew(CXR_Model_BSP);
			spBSP->Create(NULL, &DFile, NULL, CI);
			spModel = spBSP;
		}
		else if(XRMode == 1)
		{
			spCXR_Model_BSP2 spBSP = MNew(CXR_Model_BSP2);
			spBSP->Create(NULL, &DFile, NULL, CI);
			spModel = spBSP;
		}
		else if(XRMode == 2)
		{
			spCXR_Model_BSP3 spBSP = MNew(CXR_Model_BSP3);
			spBSP->Create(NULL, &DFile, NULL, CI);
			spModel = spBSP;
		}
		else if(XRMode == 3)
		{
			if( PreferedXRMode == 2 )
			{
				spCXR_Model_BSP3 spBSP = MNew(CXR_Model_BSP3);
				spBSP->Create(NULL, &DFile, NULL, CI);
				spModel = spBSP;
			}
			else
			{
				spCXR_Model_BSP2 spBSP = MNew(CXR_Model_BSP2);
				spBSP->Create(NULL, &DFile, NULL, CI);
				spModel = spBSP;
			}
		}
		else
		{
			Error("CreateFromXW", "Model has unsupported XR_MODE type");
		}
		
		if(m_lspModels.Len() == 0)
			spFirst = spModel;
		m_lspModels.Add(spModel);
		
		DFile.PopPosition();
	}
	DFile.PopPosition();
	
	DFile.Close();

	if(GetNumModels() > 0)
		return true;

	return false;
}

