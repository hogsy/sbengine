#include "PCH.h"
#include "WObj_Button.h"

// -------------------------------------------------------------------
//  Func_FunBox
// -------------------------------------------------------------------
CWObject_Func_FunBox::CWObject_Func_FunBox()
{
	m_Pos = 0;
	m_Mode = 0;
	m_Speed = 4;
	m_MoveRange = 16;
}

void CWObject_Func_FunBox::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
 const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	const fp32 Valuef = _Value.Val_fp64();
	if (__KeyHash == MHASH2('SPEE','D'))
	{ 
		m_Speed = Valuef/256.0f; 
	}
	else if(__KeyHash == MHASH3('MOVE','RANG','E'))
	{
		m_MoveRange = Valuef; 
	}
	else
		CWObject_Model::OnEvalKey(_pKey);
}

void CWObject_Func_FunBox::OnRefresh()
{
	switch(m_Mode)
	{
	case 0:
		//Waiting;
		break;
		
	case 1:
		//Moving;
		m_Pos += m_Speed;
		if(m_Pos >= m_MoveRange)
		{
			m_Pos = m_MoveRange;
			m_Mode = 0;
		}
		
		CVec3Dfp32 NewPos = m_Startpos + m_Direction * m_Pos;
		m_pWServer->Object_SetVelocity(m_iObject, NewPos - GetPosition());
		m_pWServer->Object_MovePhysical(m_iObject);
		break;
	}
}

int CWObject_Func_FunBox::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			if(m_Mode == 0)
			{
				switch(_Msg.m_Param0)
				{
				case 0:
					m_Direction = CVec3Dfp32(1, 0, 0);
					break;
				case 1:
					m_Direction = CVec3Dfp32(0, 1, 0);
					break;
				case 2:
					m_Direction = CVec3Dfp32(-1, 0, 0);
					break;
				case 3:
					m_Direction = CVec3Dfp32(0, -1, 0);
					break;
				}
				m_Mode = 1;
				m_Direction.Normalize();
				m_Startpos = GetPosition();
				m_Pos = 0;
			}
			return 1;
		}
	default :
		return CWObject::OnMessage(_Msg);
	}
}

void CWObject_Func_FunBox::OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model::OnLoad(_pFile, _pWData, _Flags);
	_pFile->ReadLE(m_Speed);
	_pFile->ReadLE(m_Pos);
	_pFile->ReadLE(m_MoveRange);
	_pFile->ReadLE(m_Mode);
	m_Startpos.Read(_pFile);
	m_Direction.Read(_pFile);
}

void CWObject_Func_FunBox::OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model::OnSave(_pFile, _pWData, _Flags);
	_pFile->WriteLE(m_Speed);
	_pFile->WriteLE(m_Pos);
	_pFile->WriteLE(m_MoveRange);
	_pFile->WriteLE(m_Mode);
	m_Startpos.Write(_pFile);
	m_Direction.Write(_pFile);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_FunBox, CWObject_Model, 0x0100);

// -------------------------------------------------------------------
//  FUNC_BUTTON
// -------------------------------------------------------------------
CWObject_Func_Button::CWObject_Func_Button() : CWObject_Model()
{
	m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE;
	
	m_MoveDirection = CVec3Dfp32(0, 0, 1);
	m_MoveRange = 8;
	
	m_RotAngles = 0;
	m_Pos = 0;
	m_Speed = 2;
	m_Mode = 3;
	
	m_Param = 0;
	
	m_bRotate = false;
	
	m_Wait = 20*3;
	m_WaitCount = 0;
}

void CWObject_Func_Button::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
 const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	if (__KeyHash == MHASH4('BSPM','ODEL','INDE','X')) 
	{
		CStr ModelName = CStrF("$WORLD:%d", _Value.Val_int());
/*
		int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);
		int iModel;
		if( XRMode == 1 )
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
		else if( XRMode == 2 )
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
		else
			iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
*/
		iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
		if (!iModel) Error("OnEvalKey", "Failed to acquire world-model.");
		
		m_iModel[0] = iModel;
		
		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iModel[0], 0, 0, 1.0f);
		Phys.m_nPrim = 1;
		
		Phys.m_PhysFlags = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_TRIGGER;
		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			ConOutL("§cf80WARNING: Unable to set trigger physics state.");
		
		// Set bound-box.
		{
			CBox3Dfp32 Box;
			m_pWServer->GetMapData()->GetResource_Model(m_iModel[0])->GetBound_Box(Box);
			m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
		}
		return;
	}
	
	if (__KeyHash == MHASH1('WAIT'))
	{
		m_Wait = _Value.Val_int(); 
		return;
	}
	
	if (__KeyHash == MHASH2('ROTA','NGLE'))
	{
		if (_Value.Find(",") >= 0)
		{
			m_RotAngles.ParseString(_Value);
			m_RotAngles *= 1.0f/360.0f;
		}
		else
			m_RotAngles = CVec3Dfp32(0,0,_Value.Val_fp64() / 360.0f); 
		m_bRotate = true;
		return;
	}
	
	if (__KeyHash == MHASH2('SPEE','D'))
	{ 
		m_Speed = _Value.Val_fp64()/256.0; 
		return;
	}
	
	if(__KeyHash == MHASH3('MOVE','RANG','E'))
	{
		m_MoveRange = _Value.Val_fp64(); 
		return;
	}
	
	if(__KeyHash == MHASH4('MOVE','DIRE','CTIO','N'))
	{
		m_MoveDirection.ParseString(_Value);
		m_MoveDirection.Normalize();
		return;
	}
	
	if(__KeyHash == MHASH2('PARA','M'))
	{
		m_Param = _Value.Val_int();
	}
	
	if(__KeyHash == MHASH2('TARG','ETS'))
	{
		while(_Value != "")
			m_Targets.Add(_Value.GetStrSep(";"));
		return;
	}
	
	CWObject_Model::OnEvalKey(_pKey);
	
	if (__KeyHash == MHASH2('ORIG','IN'))
	{
		m_StartPos = GetPosition();
		return;
	}
	
}

void CWObject_Func_Button::SetPosition()
{
	if(m_bRotate)
	{
		CMat43fp32 NewMat, NewMat2;
		CVec3Dfp32 Angles(m_RotAngles);
		Angles *= (m_Pos / m_MoveRange);
		Angles.CreateMatrixFromAngles(0, NewMat);
		CVec3Dfp32::GetMatrixRow(NewMat, 3) = GetPosition();
		m_pWServer->Object_SetPosition(m_iObject, NewMat);
	}
	else
	{
		CVec3Dfp32 NewPos = m_StartPos + m_MoveDirection * m_Pos;
		m_pWServer->Object_SetVelocity(m_iObject, NewPos - GetPosition());
		m_pWServer->Object_MovePhysical(m_iObject);
	}
}

void CWObject_Func_Button::OnRefresh()
{
	switch(m_Mode)
	{
	case 0 : 
		// Pushing
		{
			m_Pos += m_Speed;
			if(m_Pos >= m_MoveRange)
			{
				m_Pos = m_MoveRange;
				m_WaitCount = m_Wait;
				m_Mode = 2;
			}
			
			SetPosition();
			
			break;
		}
	case 1 : 
		// Resetting
		{
			m_Pos -= m_Speed;
			if(m_Pos <= 0)
			{
				m_Pos = 0;
				m_Mode = 3;
			}
			
			SetPosition();
			
			break;
		}
	case 2 : 
		// Pushed, waiting for reset
		{
			if(m_Wait != -1)
			{
				if (m_WaitCount)
					m_WaitCount--;
				else
				{
					m_Mode = 1;
				}
			}
			break;
		}
	case 3 :
		// Ready
		{
			break;
		}
	}
}

void CWObject_Func_Button::TriggerButton()
{
	if(m_Mode == 3)
	{
		m_Mode = 0;
		if(GetTarget())
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_Param), GetTarget());
		int nr = m_Targets.Len();
		for(int i = 0; i < nr; i++)
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, m_Param), m_Targets[i]);
	}
}

int CWObject_Func_Button::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION:
		{
			TriggerButton();
			return 1;
		}
		
	case OBJMSG_IMPULSE:
		{
			if(m_Mode == 2)
				m_Mode = 1;
			return 1;
		}
	default :
		return CWObject::OnMessage(_Msg);
	}
}

void CWObject_Func_Button::OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model::OnLoad(_pFile, _pWData, _Flags);
	_pFile->ReadLE(m_Speed);
	_pFile->ReadLE(m_Pos);
	_pFile->ReadLE(m_MoveRange);
	_pFile->ReadLE(m_Mode);
	_pFile->ReadLE(m_Wait);
	_pFile->ReadLE(m_WaitCount);
	_pFile->ReadLE(m_Param);
	_pFile->ReadLE(m_bRotate);
	
	m_StartPos.Read(_pFile);
	m_RotAngles.Read(_pFile);
	m_MoveDirection.Read(_pFile);
	
	int16 nTargets;
	_pFile->ReadLE(nTargets);
	m_Targets.SetLen(nTargets);
	for(int t = 0;t < nTargets; t++)
		m_Targets[t].Read(_pFile);
}

void CWObject_Func_Button::OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Model::OnSave(_pFile, _pWData, _Flags);
	_pFile->WriteLE(m_Speed);
	_pFile->WriteLE(m_Pos);
	_pFile->WriteLE(m_MoveRange);
	_pFile->WriteLE(m_Mode);
	_pFile->WriteLE(m_Wait);
	_pFile->WriteLE(m_WaitCount);
	_pFile->WriteLE(m_Param);
	_pFile->WriteLE(m_bRotate);
	
	m_StartPos.Write(_pFile);
	m_RotAngles.Write(_pFile);
	m_MoveDirection.Write(_pFile);
	
	int16 nTargets = m_Targets.Len();
	_pFile->WriteLE(nTargets);
	for(int t = 0;t < nTargets; t++)
		m_Targets[t].Write(_pFile);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_Button, CWObject_Model, 0x0100);
