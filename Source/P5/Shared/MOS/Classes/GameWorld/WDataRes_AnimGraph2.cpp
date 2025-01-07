#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../../XR/XRAnimGraph2/AnimGraph2Defs.h"
#include "WDataRes_AnimGraph2.h"

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

CXR_AnimGraph2* CWRes_AnimGraph2::GetAnimGraph()
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

void CWRes_AnimGraph2::OnPrecache(class CXR_Engine* _pEngine)
{
	if (m_spAnimGraph)
		m_spAnimGraph->ClearAnimSequenceCache();
}

//--------------------------------------------------------------------------------

bool CWRes_AnimGraph2::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	m_pWData = _pWData;

	CStr FileName = ResolveFilename(_pWData, _pName, "XAH");
	if (!CDiskUtil::FileExists(FileName))
	{
		ConOutL(CStrF("§cf80WARNING: (CWRes_AnimGraph2::Create) Could not find %s", FileName.Str()));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

int CWRes_AnimGraph2::IsLoaded()
{
	return (m_spAnimGraph != NULL);
}

//--------------------------------------------------------------------------------

void CWRes_AnimGraph2::OnLoad()
{
	MSCOPESHORT(CWRes_AnimGraph2::OnLoad);
	if (IsLoaded()) return;

	CStr FileName = ResolveFilename(m_pWData, m_Name, "XAH");

	m_spAnimGraph = MNew(CXR_AnimGraph2);
	m_spAnimGraph->Read(FileName);
}

//--------------------------------------------------------------------------------

void CWRes_AnimGraph2::OnUnload()
{
	m_spAnimGraph = NULL;
}

//--------------------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CWRes_AnimGraph2, CWResource);
