#include "PCH.h"

#ifdef M_Profile

#include "WObj_System.h"
#include "WObj_Primitives.h"
//#include "WObj_Messages.h"

#define PRIMITIVES_TIME (20 * 15)

// -------------------------------------------------------------------
//  CWObject_CoordinateSystem
// -------------------------------------------------------------------
void CWObject_CoordinateSystem::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnIncludeClass, MAUTOSTRIP_VOID);

	_pWData->GetResourceIndex_Model("CoordSys");
}

void CWObject_CoordinateSystem::OnCreate()
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Model::OnCreate();
	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("CoordSys");
	m_Time = PRIMITIVES_TIME;
}

void CWObject_CoordinateSystem::OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnClientCreate, MAUTOSTRIP_VOID);

	CWObject_Model::OnClientCreate(_pObj, _pWClient);
	_pObj->m_iModel[0] = _pWClient->GetMapData()->GetResourceIndex_Model("CoordSys");
	_pObj->m_Data[0] = PRIMITIVES_TIME;
}

void CWObject_CoordinateSystem::OnRefresh()
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnRefresh, MAUTOSTRIP_VOID);

	if (m_Time) m_Time--;
	if (!m_Time)
	{
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}
	CWObject_Model::OnRefresh();
}

void CWObject_CoordinateSystem::OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnClientExecute, MAUTOSTRIP_VOID);

	if (_pObj->m_Data[0]) _pObj->m_Data[0]--;
	if (!_pObj->m_Data[0])
	{
		_pWClient->Object_Destroy(_pObj->m_iObject);
		return;
	}
	CWObject_Model::OnClientExecute(_pObj, _pWClient);
}

void CWObject_CoordinateSystem::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnLoad, MAUTOSTRIP_VOID);

	CWObject_Model::OnLoad(_pFile);
	_pFile->ReadLE(m_Time);
}

void CWObject_CoordinateSystem::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_CoordinateSystem_OnSave, MAUTOSTRIP_VOID);

	CWObject_Model::OnSave(_pFile);
	_pFile->WriteLE(m_Time);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CoordinateSystem, CWObject_Model, 0x0100);


// -------------------------------------------------------------------
//  CWObject_Line
// -------------------------------------------------------------------
void CWObject_Line::OnCreate()
{
	MAUTOSTRIP(CWObject_Line_OnCreate, MAUTOSTRIP_VOID);

	CWObject_Model::OnCreate();
	
	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("Line");
}

void CWObject_Line::OnRefresh()
{
	MAUTOSTRIP(CWObject_Line_OnRefresh, MAUTOSTRIP_VOID);

	if (GetAnimTick(m_pWServer) >= PRIMITIVES_TIME)
	{
		Destroy();
		return;
	}
	
	CWObject_Model::OnRefresh();
}

aint CWObject_Line::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Line_OnMessage, 0);

	if(_Msg.m_Msg == OBJMSG_PRIMITIVES_LINE)
	{
		CMat4Dfp32 Mat;
		Mat.UnitNot3x3();
		CVec3Dfp32 fwd = (_Msg.m_VecParam1 - _Msg.m_VecParam0);
		_Msg.m_VecParam0.SetMatrixRow(Mat, 3);
		fwd.SetMatrixRow(Mat, 0);
		if(M_Fabs(CVec3Dfp32(0, 0, 1) * fwd) < 0.99f)
			CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
		else
			CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 2);
		Mat.RecreateMatrix(0, 2);
		SetPosition(Mat);
		float LengthSqr = (_Msg.m_VecParam0 - _Msg.m_VecParam1).LengthSqr();
		if(LengthSqr >= Sqr(2048))
			ConOut("Line is too long");
		else
			m_iAnim0 = int16(M_Sqrt(LengthSqr) * 16);
		m_iAnim1 = _Msg.m_Param0 >> 16;
		m_iAnim2 = _Msg.m_Param0;
		return 0;
	}
	
	return CWObject_Model::OnMessage(_Msg);
}

void CWObject_Line::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_Line_OnIncludeClass, MAUTOSTRIP_VOID);

	_pWData->GetResourceIndex_Model("Line");
}

void CWObject_Line::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Line_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Line::OnClientRefresh);

	_pObj->m_CreationGameTick++;
	CWObject_Model::OnClientRefresh(_pObj, _pWClient);
}

void CWObject_Line::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Line_OnClientRender, MAUTOSTRIP_VOID);

	if(_pObj->m_iAnim0 != 0)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if(pModel)
		{
			CXR_AnimState AnimState(CMTime::CreateFromSeconds(fp32(_pObj->m_iAnim0) / 16), CMTime(), 0, 0, _pObj->m_lModelInstances[0], _pObj->m_iObject);
			CPixel32 c = (_pObj->m_iAnim1 << 16) | _pObj->m_iAnim2;
			int Fade = (PRIMITIVES_TIME - _pObj->GetAnimTick(_pWClient));
			if(Fade > 0)
			{
				if(Fade < 16)
					c = CPixel32(c.GetR(), c.GetG(), c.GetB(), (c.GetA() * Fade) / 16);
				AnimState.m_Data[0] = c;
				
				_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState);
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Line, CWObject_Model, 0x0100);

#endif
