#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../../XR/XRAnimGraph/AnimGraphDefs.h"
#include "WDataRes_AnimGraph.h"

//--------------------------------------------------------------------------------

static CStr ResolveFilename(CWorldData* _pWData, CStr _Filename, CStr _Extension)
{
	if (_Filename.SubStr(3, 1) == ":")
		_Filename = _Filename.RightFrom(4);

	_Filename = "ANIM\\" + _Filename;

	_Extension = "." + _Extension.UpperCase();
	if (_Filename.Right(4).UpperCase() != _Extension)
		_Filename = _Filename + _Extension;

	_Filename = _pWData->ResolveFileName(_Filename);

	return _Filename;
}

//--------------------------------------------------------------------------------

CXR_AnimGraph* CWRes_AnimGraph::GetAnimGraph()
{
	if (IsLoaded())
		return m_spAnimGraph;
	else
	{
		OnLoad();
		if (IsLoaded())
			return m_spAnimGraph;
		else
			return NULL;
	}
}

//--------------------------------------------------------------------------------

bool CWRes_AnimGraph::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	m_pWData = _pWData;

	CStr FileName = ResolveFilename(_pWData, _pName, "XAG");
	if (!CDiskUtil::FileExists(FileName))
	{
		ConOutL(CStrF("§cf80WARNING: (CWRes_AnimGraph::Create) Could not find %s", FileName.Str()));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

int CWRes_AnimGraph::IsLoaded()
{
	return (m_spAnimGraph != NULL);
}

//--------------------------------------------------------------------------------

void CWRes_AnimGraph::OnLoad()
{
	MSCOPESHORT(CWRes_AnimGraph::OnLoad);
	if (IsLoaded()) return;

	CStr FileName = ResolveFilename(m_pWData, m_Name, "XAG");

	m_spAnimGraph = MNew(CXR_AnimGraph);
	m_spAnimGraph->Read(FileName);
}

//--------------------------------------------------------------------------------

void CWRes_AnimGraph::OnUnload()
{
	m_spAnimGraph = NULL;
}

//--------------------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CWRes_AnimGraph, CWResource);

//--------------------------------------------------------------------------------

CXRAG_AnimList* CWRes_AGAnimList::GetAnimList()
{
	return m_spAnimList;
}

//--------------------------------------------------------------------------------

bool CWRes_AGAnimList::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	m_pWData = _pWData;
	m_pMapData = _pMapData;

	ParseReg();

	return true;
}
	
//--------------------------------------------------------------------------------

int CWRes_AGAnimList::IsLoaded()
{
	return (m_spAnimList != NULL);
}

void CWRes_AGAnimList::OnPrecache(class CXR_Engine* _pEngine)
{
	m_spAnimList->ClearCache();
}
//--------------------------------------------------------------------------------

void CWRes_AGAnimList::ParseReg()
{
	if (IsLoaded()) return;

	m_spAnimList = MNew(CXRAG_AnimList);

	int Len = m_Name.Len();
	if(Len < 5)
		return;

	spCRegistry spReg;

	CStr Name = &(((char*)m_Name)[4]);
	CStr Name2 = Name.GetStrSep("+");

	/*#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		// Create load log for animlist
		CStr World = m_pMapData->GetWorld();
		World = World.GetStrSep(".XW");
		CStr Path = m_pMapData->ResolvePath(CStr("MemUsage\\"));
		Path += CStrF("AnimList_%s_",World.GetStr()) + Name2;
		m_spAnimList->m_LogFile.Create(Path);
	#endif*/

	while(Name2 != "")
	{
		CStr FileName = ResolveFilename(m_pWData, Name2, "XRG");
		if (!CDiskUtil::FileExists(FileName))
			break;

		spCRegistry spReg2 = REGISTRY_CREATE;
		spReg2->XRG_Read(FileName);
		if(spReg2->GetNumChildren() < 1 || spReg2->GetName(0) != "ANIMLIST")
			Error("Create", CStrF("File %s is an invalid AnimList", (char *)FileName));

		Name2 = spReg2->GetValue(0);

		if(spReg)
			spReg2->CopyDir(spReg);
		spReg = spReg2;
	}

	if(!spReg || spReg->GetNumChildren() < 1)
		return;

	while(Name != "")
	{
		CStr Name2 = Name.GetStrSep("+");
		CStr FileName = ResolveFilename(m_pWData, Name2, "XRG");
		if (!CDiskUtil::FileExists(FileName))
			break;

		spCRegistry spReg2 = REGISTRY_CREATE;
		spReg2->XRG_Read(FileName);
		if(spReg2->GetNumChildren() < 1 || spReg2->GetName(0) != "ANIMLIST")
			Error("Create", CStrF("File %s is an invalid AnimList", (char *)FileName));

		spReg->CopyDir(spReg2);
	}

	if(!spReg || spReg->GetNumChildren() < 1)
		return;

	CRegistry *pList = spReg->GetChild(0);
	///if(pList->GetNumChildren() > 256)
	//	Error("Create", CStrF("AnimList %s contains more than 255 animations (%s=%s)", (char*)Name, (char *)pList->GetName(256), (char *)pList->GetValue(256)));

	for(int iAnim = 0; iAnim < pList->GetNumChildren(); iAnim++)
	{
		spCWResource spAnimContainerResource;
		int16 iAnimSeq;

		CFStr AnimSeq = pList->GetValue(iAnim);
		CFStr AnimContainerResource = AnimSeq.GetStrSep(":");
		if(AnimContainerResource != "")
		{
			iAnimSeq = AnimSeq.Val_int();
/*
			if(iAnimSeq >= AG_MAXANIMLISTSIZE)
				Error("Create", CStrF("AnimList %s contains a sequence index greater than %d (anim %i)", (AG_MAXANIMLISTSIZE - 1), (char*)Name, iAnim));
*/
			int iAnimContainerResource = 0;

			#ifdef WAGI_USEAGRESOURCEMANAGEMENT
				m_spAnimList->SetAnim(iAnim, iAnimContainerResource, iAnimSeq, NULL/*m_pWData->GetResourceRef(iAnimContainerResource)*/,AnimContainerResource);
			#endif
			#ifndef WAGI_USEAGRESOURCEMANAGEMENT_ONLY
			{
				MSCOPE(GetResourceIndex, RES_AGANIMELIST);
				iAnimContainerResource = m_pWData->GetResourceIndex("ANM:" + AnimContainerResource, WRESOURCE_CLASS_XSA, m_pMapData);
				m_MemUsed -= MSCOPE_MEMDELTA;
			}
			m_spAnimList->SetAnim(iAnim, iAnimContainerResource, iAnimSeq, m_pWData->GetResourceRef(iAnimContainerResource));
			#endif
			
/*
			if(iAnimContainerResource == 0)
				ConOutL(CStrF("§cf80WARNING: (CWRes_AnimList::Create) AnimList %s had an invalid sequence at slot %i (%s = '%s').", (char*)Name, iAnim, pList->GetName(iAnim).Str(), pList->GetValue(iAnim).Str()));
			else
				spAnimContainerResource = m_pWData->GetResourceRef(iAnimContainerResource);
*/
		}
		else
		{
			m_spAnimList->SetAnim(iAnim, 0, -1, 0);
		}
	}
}

//--------------------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CWRes_AGAnimList, CWResource);

//--------------------------------------------------------------------------------
