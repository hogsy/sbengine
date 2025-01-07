/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:		WObj_Model_Ladder.cpp	
					
	Author:			Olle Rosenquist
					
	Copyright:	Copyright O3 Games AB 2002	
					
	Contents:		CWObject_PhysLadder implementation
					
	Comments:		
					
	History:		
		020905:		Olle Rosenquist, Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WOBJ_Model_Ladder.h"
#include "../WObj_Game/WObj_GameMessages.h"
//#include "../WRPG/WRPGFist.h"
#include "../WObj_CharMsg.h"
#include "../WObj_Char.h"
#include "../../GameWorld/WClientMod_Defines.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Constructor, doesn't do much
						
	Comments:		
\*____________________________________________________________________*/
CWObject_Phys_Ladder::CWObject_Phys_Ladder()
{
	// Ain't doing nothing right now...
	return;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Called on creation
						
	Comments:		Doesn't do much
\*____________________________________________________________________*/
void CWObject_Phys_Ladder::OnCreate()
{
	m_Position1 = 0;
	m_Position2 = 0;
	m_Normal = CVec3Dfp32(0,0,1.0f);
	m_StepOffAnimType = 0;
	// Not much to do here yet, so just call base class OnCreate
	CWObject_Phys_Ladder_Parent::OnCreate();
	CACSClientData* pCD = GetACSClientData(this);
	pCD->SetCutsceneFlags(ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS); //Don't pause ai as default
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Reads a CVec3Dfp32 from a string
						
	Parameters:			
		_Str:			String on the form "0.4,16.1,-3.4"
						
	Returns:			Vector translated from the string
						
	Comments:			
\*____________________________________________________________________*/
CVec3Dfp32 CWObject_Phys_Ladder::ReadVector(CStr _Str)
{
	CVec3Dfp32 Vector;
	// Vectors, on the form "0.4,16.1,-3.4"
	Vector.k[0] = _Str.GetStrSep(",").Val_fp64();
	Vector.k[1] = _Str.GetStrSep(",").Val_fp64();
	Vector.k[2] = _Str.GetStrSep(",").Val_fp64();

	return Vector;
}

bool CWObject_Phys_Ladder::GetRelativeLadderPosition(CWorld_PhysState* _pWPhys,
													CWO_Character_ClientData* _pCD,
													const CVec3Dfp32& _CharPos, CMTime& _Value)
{
	if (!_pCD)
		return false;

	int iLadder = (int)_pCD->m_ControlMode_Param0;
	if (iLadder == 0)
		iLadder = _pCD->m_3PI_FocusObject;

	bool bResult = true;
	CVec3Dfp32 Pos1, Pos2;

	bResult = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
		0,0,0,0,0,0,&Pos1, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	bResult = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
		0,0,0,0,0,0,&Pos2, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	int HangrailMode = _pWPhys->Phys_Message_SendToObject(
		CWObject_Message(OBJMSG_LADDER_GETHANGRAILMODE), iLadder);

	// Control mode not correct yet
	if (!bResult)
		return false;

	// Find relative position
	CVec3Dfp32 LadderDir,PointOnLadder;
	fp32 LadderLength,LadderDot;
	
	if (HangrailMode & LADDER_HANDRAIL_MODEFLIPPED)
	{
		LadderDir = Pos1 - Pos2;
		LadderLength = LadderDir.Length();
		LadderDir = LadderDir / LadderLength;
		
		PointOnLadder = Pos2 + LadderDir * ((_CharPos - Pos2) * LadderDir);
		// Check if point is outside bounds (pos2<->pos1)
		LadderDot = ((PointOnLadder - Pos2) * LadderDir)/LadderLength;
	}
	else
	{
		LadderDir = Pos2 - Pos1;
		LadderLength = LadderDir.Length();
		LadderDir = LadderDir / LadderLength;
		
		PointOnLadder = Pos1 + LadderDir * ((_CharPos - Pos1) * LadderDir);
		// Check if point is outside bounds (pos2<->pos1)
		LadderDot = ((PointOnLadder - Pos1) * LadderDir)/LadderLength;
	}
	
	_Value = CMTime::CreateFromSeconds((LadderDot < 0.0f ? 0.0f : (LadderDot > 1.0f ? 1.0f : LadderDot)));
	// There is some [0,1] bound but I don't remember the name at this time
	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Handles runtime class initialization parameters
						
	Parameters:		
		_pKey:			Registry key
			
	Comments:		
\*____________________________________________________________________*/
/*void CWObject_Phys_Ladder::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().Find("POS1") != -1)
	{
		m_Position1 = ReadVector(_pKey->GetThisValue());
	}
	else if (_pKey->GetThisName().Find("POS2") != -1)
	{
		m_Position2 = ReadVector(_pKey->GetThisValue());
	}
	else if (_pKey->GetThisName().Find("NORMAL") != -1)
	{
		// Set the normal at position LADDERNORMAL
		CStr Value = _pKey->GetThisValue();
		// Remove any paranthesis in the beginning
		Value.GetStrSep("(");
		m_Normal = ReadVector(Value);
		// Pack the normal
		SetUserDataNormal32(this, m_Normal, LADDERNORMAL);
	}
	// Do base class
	CWObject_Phys_Ladder_Parent::OnEvalKey(_pKey);
}*/

#define LADDER_STANDARD_WIDTH 20
#define HANGRAIL_STANDARD_HEIGHT 10
void CWObject_Phys_Ladder::OnFinishEvalKeys()
{
	// Moved to onfinishevalkeys
	// Set game physics flags otherwise the ladder won't collide with player
	m_Flags |= FLAGS_GAMEPHYSICS;

	CWObject_Phys_Ladder_Parent::OnFinishEvalKeys();

	// Always third person on ladder
	CACSClientData* pCD = GetACSClientData(this);
	pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_USETHIRDPERSON);

	// Read ladder information from enginepath data (first position is bottom, second is top
	// direction is normal)
	CWO_PosHistory* KeyFrames = GetClientData(this);
	if(m_iAnim0 != -1 && !KeyFrames->IsValid())
		LoadPath(m_pWServer, this, m_iAnim0);

	int NumKeys = (KeyFrames ? (KeyFrames->m_lSequences.Len() > 0 ? 
		KeyFrames->m_lSequences[0].GetNumKeyframes() : 0) : 0);

	// Need atleast 2 positions to make a ladder
	if (NumKeys > 1)
	{
		// Read the two positions needed for a ladder
		CMat4Dfp32 Position;
		KeyFrames->m_lSequences[0].GetMatrix(0,Position);
		m_Position1 = CVec3Dfp32::GetMatrixRow(Position,3);
		m_Normal = CVec3Dfp32::GetMatrixRow(Position,0);
		// Use this position as position 2 for next ledge (don't increase i...)
		KeyFrames->m_lSequences[0].GetMatrix(1,Position);
		m_Position2 = CVec3Dfp32::GetMatrixRow(Position,3);

		SetUserDataVector32MA(this, m_Position1, LADDERPOSITION1);
		SetUserDataVector32MA(this, m_Position2, LADDERPOSITION2);
		SetUserDataNormal32(this, m_Normal, LADDERNORMAL);
	}

	// Ok Check angle between Z axis and ladder normal

	CVec3Dfp32 LadderDir = m_Position2 - m_Position1;
	fp32 LadderLength = LadderDir.Length();
	LadderDir = LadderDir / LadderLength;

	fp32 Height;
	fp32 WidthX;
	fp32 WidthY;
	CVec3Dfp32 Offset = LadderDir * LadderLength / 2.0f;

	bool bLadder = M_Fabs(LadderDir[2]) > 0.5f;

	if (bLadder)
	{
		Height = ClampRange(LadderLength/2.0f,255);
		WidthX = LADDER_STANDARD_WIDTH*0.5f;
		WidthY = LADDER_STANDARD_WIDTH*0.5f;
	}
	else
	{
		Height = HANGRAIL_STANDARD_HEIGHT;
		//WidthX = ClampRange(LadderLength*0.5f,255);
		//WidthY = ClampRange(LadderLength*0.5f,255);
		CVec3Dfp32 Right;
		LadderDir.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Right);
		Right *= LADDER_STANDARD_WIDTH;

		CVec3Dfp32 EndPoint = (LadderDir * LadderLength*0.5f);
		WidthX  = Max(Abs((EndPoint + Right).k[0]), Abs((EndPoint - Right).k[0]));
		WidthY  = Max(Abs((EndPoint + Right).k[1]), Abs((EndPoint - Right).k[1]));
	}

	if ((WidthX > CWO_PHYSICSPRIM_MAXDIM_XY) || (WidthY > CWO_PHYSICSPRIM_MAXDIM_XY))
	{
		// Well this sucks, but lets make the bbox as large as we can then
		Offset = LadderDir * CWO_PHYSICSPRIM_MAXDIM_XY;
		WidthX = Min(WidthX, fp32(CWO_PHYSICSPRIM_MAXDIM_XY));
		WidthY = Min(WidthY, fp32(CWO_PHYSICSPRIM_MAXDIM_XY));
	}

	// Set physics as pickup, so it can be selected
	m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_PICKUP);
//	m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
	// Set some physics so we can select it
	CWO_PhysicsState Phys;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
		CVec3Dfp32(WidthX, WidthY, Height),Offset);
	Phys.m_nPrim = 1;
	Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
	Phys.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;//OBJECT_FLAGS_WORLD;//| OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	Phys.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
	// Well then, this doesn't work for the moment
	//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	// Find stepofftype
	if (bLadder)
		m_StepOffAnimType = FindStepOffAnimType(m_pWServer, m_Position1,m_Position2,m_Normal);

	if (m_UseName.Len() == 0)
		m_UseName = "§LACS_LADDER";
	pCD->SetChoiceString(m_UseName);
}

//
void CWObject_Phys_Ladder::OnSpawnWorld()
{
	CWObject_Phys_Ladder_Parent::OnSpawnWorld();
	// Moved to OnFinishEvalKeys
	/*// Read ladder information from enginepath data (first position is bottom, second is top
	// direction is normal)
	CWObject_Phys_Ladder_Parent::OnSpawnWorld();
	CWO_PosHistory* KeyFrames = GetClientData(this);
	if(m_iAnim0 != -1 && !KeyFrames->IsValid())
		LoadPath(m_pWServer, this, m_iAnim0);

	int NumKeys = (KeyFrames ? (KeyFrames->m_lSequences.Len() > 0 ? 
		KeyFrames->m_lSequences[0].GetNumKeyframes() : 0) : 0);
	
	// Need atleast 2 positions to make a ladder
	if (NumKeys > 1)
	{
		// Read the two positions needed for a ladder
		CMat4Dfp32 Position;
		KeyFrames->m_lSequences[0].GetMatrix(0,Position);
		m_Position1 = CVec3Dfp32::GetMatrixRow(Position,3);
		m_Normal = CVec3Dfp32::GetMatrixRow(Position,0);
		// Use this position as position 2 for next ledge (don't increase i...)
		KeyFrames->m_lSequences[0].GetMatrix(1,Position);
		m_Position2 = CVec3Dfp32::GetMatrixRow(Position,3);
		
		SetUserDataVector32MA(this, m_Position1, LADDERPOSITION1);
		SetUserDataVector32MA(this, m_Position2, LADDERPOSITION2);
		SetUserDataNormal32(this, m_Normal, LADDERNORMAL);
	}

	// Ok Check angle between Z axis and ladder normal

	CVec3Dfp32 LadderDir = m_Position2 - m_Position1;
	fp32 LadderLength = LadderDir.Length();
	LadderDir = LadderDir / LadderLength;

	fp32 Height;
	fp32 WidthX;
	fp32 WidthY;
	CVec3Dfp32 Offset = LadderDir * LadderLength / 2.0f;

	bool bLadder = M_Fabs(LadderDir[2]) > 0.5f;

	if (bLadder)
	{
		Height = ClampRange(LadderLength/2.0f,255);
		WidthX = LADDER_STANDARD_WIDTH*0.5f;
		WidthY = LADDER_STANDARD_WIDTH*0.5f;
	}
	else
	{
		Height = HANGRAIL_STANDARD_HEIGHT;
		//WidthX = ClampRange(LadderLength*0.5f,255);
		//WidthY = ClampRange(LadderLength*0.5f,255);
		CVec3Dfp32 Right;
		LadderDir.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Right);
		Right *= LADDER_STANDARD_WIDTH;

		CVec3Dfp32 EndPoint = (LadderDir * LadderLength*0.5f);
		WidthX  = Max(Abs((EndPoint + Right).k[0]), Abs((EndPoint - Right).k[0]));
		WidthY  = Max(Abs((EndPoint + Right).k[1]), Abs((EndPoint - Right).k[1]));
	}

	if ((WidthX > CWO_PHYSICSPRIM_MAXDIM) || (WidthY > CWO_PHYSICSPRIM_MAXDIM))
	{
		// Well this sucks, but lets make the bbox as large as we can then
		CVec3Dfp32 Offset = LadderDir * CWO_PHYSICSPRIM_MAXDIM;
		WidthX = (WidthX > CWO_PHYSICSPRIM_MAXDIM ? CWO_PHYSICSPRIM_MAXDIM : WidthX);
		WidthY = (WidthY > CWO_PHYSICSPRIM_MAXDIM ? CWO_PHYSICSPRIM_MAXDIM : WidthY);
	}
	
	// Set physics as pickup, so it can be selected
	m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
	// Set some physics so we can select it
	CWO_PhysicsState Phys;
	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(WidthX, WidthY, Height),Offset);
	Phys.m_nPrim = 1;
	Phys.m_ObjectFlags = OBJECT_FLAGS_PICKUP;
	Phys.m_ObjectIntersectFlags = 0;//OBJECT_FLAGS_WORLD;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET;//OBJECT_FLAGS_WORLD;//| OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	Phys.m_iExclude = -1;//_iExclude == -1 ? m_iOwner : _iExclude;
	// Well then, this doesn't work for the moment
	//m_pWServer->Object_SetPosition(m_iObject,GetPosition() + Offset);
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	// Find stepofftype
	if (bLadder)
		m_StepOffAnimType = FindStepOffAnimType(m_pWServer, m_Position1,m_Position2,m_Normal);*/
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Server side messagehandler
						
	Parameters:		
		_Msg:			Message to handle
			
	Returns:		1 if the message was handled or ladder type if 
					the message was OBJMSG_LADDER_GETTYPE, otherwise 0 
					or whatever the base class message handler returns
						
	Comments:		
\*____________________________________________________________________*/
aint CWObject_Phys_Ladder::OnMessage(const CWObject_Message& _Msg)
{
	// Take care of assorted messages, 
	switch (_Msg.m_Msg)
	{
	case OBJMSG_LADDER_GETPOSITION1:
		{
			// Return the vector in the message m_pData
			CVec3Dfp32 Position1;
			GetUserDataVector32MA(this, Position1, LADDERPOSITION1);
	
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = Position1;
				
				return 1;
			}

			return 0;

			// Put the vector in the message data pointer, if there is space enough
			/*if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = m_Position1;
				
				return 1;
			}

			return 0;*/
		}
	case OBJMSG_LADDER_GETPOSITION2:
		{
			// Return the vector in the message m_pData
			CVec3Dfp32 Position2;
			GetUserDataVector32MA(this, Position2, LADDERPOSITION2);
			
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = Position2;

				return 1;
			}
			
			return 0;
			// Put the vector in the message data pointer, if there is space enough
			/*if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = m_Position2;
				
				return 1;
			}

			return 0;*/
		}
	case OBJMSG_LADDER_GETNORMAL:
		{
			// Return the vector in the message m_pData
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = m_Normal;
				
				return 1;
			}

			return 0;
		}
	case OBJMSG_LADDER_SETPOSITION1:
		{
			m_Position1 = _Msg.m_VecParam0;
			SetUserDataVector32MA(this, m_Position1, LADDERPOSITION1);
			return 1;
		}
	case OBJMSG_LADDER_SETPOSITION2:
		{
			m_Position2 = _Msg.m_VecParam0;
			SetUserDataVector32MA(this, m_Position2, LADDERPOSITION2);
			return 1;
		}
	case OBJMSG_LADDER_SETNORMAL:
		{
			m_Normal = _Msg.m_VecParam0;
			SetUserDataNormal32(this, m_Normal, LADDERNORMAL);
			return 1;
		}
	case OBJMSG_LADDER_GETTYPE:
		{
			// Ok Check angle between Z axis and ladder normal
			CVec3Dfp32 Pos1, Pos2, LadderDir;

			LadderDir = m_Position1 - m_Position2;
			LadderDir.Normalize();

			if (M_Fabs(LadderDir[2]) > 0.5f)
				return CWOBJECT_LADDER_TYPE_LADDER;
			else
				return CWOBJECT_LADDER_TYPE_HANGRAIL;
		}
	case OBJMSG_LADDER_INACTIVATELADDERCAM:
		{
			// Inactivate the ladder cam
			CWObject* pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			
			if (pCD)
			{
				int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
				CameraMode &= ~CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE;
				pCD->m_ActionCutSceneCameraMode = CameraMode;
			
				return true;
			}

			if (m_spActiveCutsceneCamera)
				m_spActiveCutsceneCamera = NULL;

			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_ISACTIONCUTSCENE:
		{
			return false;
		}
	case OBJMSG_LADDER_ISLADDER:
		{
			return true;
		}
	case OBJMSG_LADDER_GETCURRENTENDPOINT:
		{
			int Endpoint;
			GetLadderEndpoint(this,Endpoint);
			return Endpoint;
		}
	case OBJMSG_LADDER_FINDENDPOINT:
		{
			// Param0 = height, vecparam0 = charpos
			// Find endpoint from given character position
			CVec3Dfp32 LadderDir = m_Position2 - m_Position1;
			fp32 Length = LadderDir.Length();
			LadderDir = LadderDir / Length;
			fp32 LadderDot;
			CVec3Dfp32 PointOnLadder;
			int Result = FindLadderPoint(LadderDir, _Msg.m_VecParam0,
				m_Position1, PointOnLadder, Length, (fp32)_Msg.m_Param0,(fp32)_Msg.m_Param1,
				LadderDot);
			
			// Set the result on the ladder enpoint as well
			SetLadderEndpoint(this,Result);

			return Result;
		}
	case OBJMSG_LADDER_GETCHARLADDERPOS:
		{
			if (!_Msg.m_pData || (_Msg.m_DataSize < sizeof(CVec3Dfp32)))
				return false;
			CVec3Dfp32 LadderDir = m_Position2 - m_Position1;
			LadderDir.Normalize();
			*((CVec3Dfp32*)_Msg.m_pData) = m_Position1 + LadderDir * ((_Msg.m_VecParam0 - m_Position1) * LadderDir);
			return true;
		}
	case OBJMSG_LADDER_FINDENDPOINTOFFSET:
		{
			// Param0 = height, vecparam0 = charpos
			// Find endpoint from given character position
			CVec3Dfp32 LadderDir = m_Position2 - m_Position1;
			fp32 Length = LadderDir.Length();
			LadderDir = LadderDir / Length;
			CVec3Dfp32 PointOnLadder;
			fp32 Result = FindLadderPointOffset(LadderDir, _Msg.m_VecParam0,
				m_Position1, PointOnLadder, Length, (fp32)_Msg.m_Param0,(fp32)_Msg.m_Param1);

			return *(int32*)&Result;
		}
	case OBJMSG_ACTIONCUTSCENE_GETTYPE:
		{
			// Ok Check angle between Z axis and ladder normal
			CVec3Dfp32 Pos1, Pos2, LadderDir;

			LadderDir = m_Position1 - m_Position2;
			LadderDir.Normalize();

			if (M_Fabs(LadderDir * CVec3Dfp32(0,0,1)) > 0.5f)
				return ACTIONCUTSCENE_TYPE_LADDER;
			else
				return ACTIONCUTSCENE_TYPE_HANGRAIL;
		}
	case OBJMSG_LADDER_CONFIGUREHANGRAIL:
		{
			// Switches position of pos1/2 if necessary (so we'll face forward on hangrail always)
			CVec3Dfp32 LadderDir = m_Position2 - m_Position1;

			if ((_Msg.m_VecParam0 * LadderDir) < 0.0f)
			{
				// Switch places
				CVec3Dfp32 Temp = m_Position1;
				m_Position1 = m_Position2;
				m_Position2 = Temp;
				// Save in m_Data
				SetUserDataVector32MA(this, m_Position1, LADDERPOSITION1);
				SetUserDataVector32MA(this, m_Position2, LADDERPOSITION2);
				int Mode;
				GetHangrailMode(this,Mode);
				Mode = (Mode & LADDER_HANDRAIL_MODEFLIPPED ? 0 : LADDER_HANDRAIL_MODEFLIPPED);
				SetHangrailMode(this,Mode);
			}

			return true;
		}
	case OBJMSG_LADDER_GETHANGRAILMODE:
		{
			int Mode;
			GetHangrailMode(this,Mode);
			return Mode;
		}
	case OBJMSG_LADDER_FINDSTEPOFFANIMTYPE:
		{
			return m_StepOffAnimType;
		}
	case OBJMSG_IMPULSE:
		{
			if (_Msg.m_Param0 == LADDER_IMPULSE_FORCEGRABLADDER)
			{
				// Hmm, ok we want to grab this ladder at any cost it seems
				// First we need to find the player
				CWObject_Game *pGame = m_pWServer->Game_GetObject();
				CWObject* pPlayer = (pGame ?pGame->Player_GetObject(0) : NULL);
				CWO_Character_ClientData* pCD = (pPlayer ? CWObject_Character::GetClientData(pPlayer) : NULL);
				if (!pCD)
					return false;

				// Make the player grab the ladder
				GrabLadder(m_iObject, SELECTION_HANGRAIL, m_pWServer,pPlayer, pCD,true);

				return true;
			}
			break;
		}
	case OBJMSG_ACTIONCUTSCENE_GETFOCUSPOS:
		{
			if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CVec3Dfp32))
			{
				CVec3Dfp32 LadderDir;

				LadderDir = m_Position2 - m_Position1;
				fp32 Len = LadderDir.Length();
				LadderDir *= (1.0f / Len); // .Normalize();

				fp32 t = Clamp((_Msg.m_VecParam0 - m_Position1) * LadderDir, -16.0f, Len + 16.0f);
				CVec3Dfp32 PointOnLadder = m_Position1 + LadderDir * t;
				*(CVec3Dfp32*)_Msg.m_pData = PointOnLadder;
				return (t < Len*0.5f) ? 1 : -1;
			}
			return 1;
		}
	case OBJMSG_CHAR_USE:
		{
			if(m_iAnim2 != 0)
				return m_pWServer->Message_SendToObject(_Msg, m_iAnim2);

			// Do some activating stuff.....
			CWObject_CoreData* pChar = m_pWServer->Object_GetCD(_Msg.m_iSender);
			CWO_Character_ClientData* pCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
			GrabLadder(m_iObject, SELECTION_LADDER, m_pWServer,pChar, pCD);

			// Assuming sender is the character id
			/*if (_Msg.m_iSender != -1)
			return DoAction(_Msg.m_iSender);*/

			return true;
		}
		// Run OBJMSG_ACTIONCUTSCENE_ACTIVATE if not

	default:
		break;
	}

	return CWObject_Phys_Ladder_Parent::OnMessage(_Msg);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Handles client messages
						
	Parameters:		
		_pObj:			Ladder object
		_pWClient:		World client
		_Msg:			Message to handle
			
	Returns:		1 if the message was handled, otherwise 0 or 
					whatever the base class message handler returns
						
	Comments:		
\*____________________________________________________________________*/
aint CWObject_Phys_Ladder::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
/*	if (_pObj == NULL)
	{
		ConOutL("CWObject_Phys_Ladder::OnClientMessage() - Invalid object pointer (_pObj == NULL).");
		return 0;
	}*/

	switch (_Msg.m_Msg)
	{
	case OBJMSG_LADDER_GETPOSITION1:
		{
			// Return the vector in the message m_pData
			CVec3Dfp32 Position1;
			GetUserDataVector32MA(_pObj, Position1, LADDERPOSITION1);
	
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = Position1;
				
				return 1;
			}

			return 0;
		}
	case OBJMSG_LADDER_GETPOSITION2:
		{
			// Return the vector in the message m_pData
			CVec3Dfp32 Position2;
			GetUserDataVector32MA(_pObj, Position2, LADDERPOSITION2);
			
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*(CVec3Dfp32*)(_Msg.m_pData) = Position2;

				return 1;
			}
			
			return 0;
		}
	case OBJMSG_LADDER_GETNORMAL:
		{
			// Return the vector in the message m_pData
			// Put the vector in the message data pointer, if there is space enough
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				CVec3Dfp32 Normal;
				GetUserDataNormal32(_pObj, Normal, LADDERNORMAL);
				Normal.Normalize();

				*(CVec3Dfp32*)(_Msg.m_pData) = Normal;
				
				return 1;
			}

			return 0;
		}
	case OBJMSG_LADDER_GETCHARLADDERPOS:
		{
			if (!_Msg.m_pData || (_Msg.m_DataSize < sizeof(CVec3Dfp32)))
				return false;
			CVec3Dfp32 Position1;
			GetUserDataVector32MA(_pObj, Position1, LADDERPOSITION1);
			CVec3Dfp32 Position2;
			GetUserDataVector32MA(_pObj, Position2, LADDERPOSITION2);
			CVec3Dfp32 LadderDir = Position2 - Position1;
			LadderDir.Normalize();
			*((CVec3Dfp32*)_Msg.m_pData) = Position1 + LadderDir * ((_Msg.m_VecParam0 - Position1) * LadderDir);
			return true;
		}
	case OBJMSG_LADDER_GETTYPE:
		{
			// Ok Check angle between Z axis and ladder normal
			CVec3Dfp32 Pos1, Pos2, LadderDir;
			GetUserDataVector32MA(_pObj, Pos1, LADDERPOSITION1);
			GetUserDataVector32MA(_pObj, Pos2, LADDERPOSITION2);

			LadderDir = Pos2 - Pos1;
			LadderDir.Normalize();

			if (M_Fabs(LadderDir * CVec3Dfp32(0,0,1)) > 0.5f)
				return CWOBJECT_LADDER_TYPE_LADDER;
			else
				return CWOBJECT_LADDER_TYPE_HANGRAIL;
		}
	case OBJMSG_LADDER_ISLADDER:
		{
			return true;
		}
	case OBJMSG_LADDER_GETCURRENTENDPOINT:
		{
			int Endpoint;
			GetLadderEndpoint(_pObj,Endpoint);
			return Endpoint;
		}
	case OBJMSG_LADDER_GETHANGRAILMODE:
		{
			int Mode;
			GetHangrailMode(_pObj,Mode);
			return Mode;
		}
	default:
		break;
	}

	return CWObject_Phys_Ladder_Parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_Phys_Ladder::TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS)
{
	TArray<CXRAG2_Impulse> lImpulses;
	// Add standard animations
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_CLIMBUP));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_SLIDEDOWN));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_SLIDESTART));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_SLIDESTOP));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_SLIDESTOPEND));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_CLIMBONDOWN));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_CLIMBONUP));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,AG2_IMPULSEVALUE_LADDERMOVE_IDLE));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LADDERMOVE,m_StepOffAnimType + (AG2_IMPULSEVALUE_LADDERMOVE_CLIMBOFFUPL4PLUS - LADDER_ENDPOINTTYPE_LEFT4PLUS)));
	//_pAGI->TagAnimSetFromLadderStepOffType(_pContext,m_pWServer->GetMapData(),m_pWServer->m_spWData,m_StepOffAnimType);
	_pAGI->TagAnimSetFromBlockReaction(_pContext,m_pWServer->GetMapData(),m_pWServer->m_spWData,CXRAG2_Impulse(AG2_IMPULSETYPE_CONTROLMODE,AG2_IMPULSEVALUE_CONTROLMODE_LADDER),lImpulses);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Disables the cutscene camera at the end of the 
					cutscene, also disables updates of this object
						
	Parameters:		
		_iCharacter:	Character index of the character who activated
						this object
			
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CWObject_Phys_Ladder::OnEndACS(int _iCharacter)
{
	// Do baseclass
	CWObject_ActionCutscene::OnEndACS(_iCharacter);

	// Disable further updates of this object
	m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;

	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (pCD)
	{
		pCD->m_Phys_Width = pCD->m_LedgePhysWidthCached;
		// Resetphys?
		CWObject_Character::Char_SetPhysics(pObj, m_pWServer, m_pWServer, 
			((pCD->m_Control_Press & CONTROLBITS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND),false,true);

		// Invalidata ladder data
		// Set ledge id and flags to control mode parameter 0
		pCD->m_ControlMode_Param0 = 0.0f;
		pCD->m_ControlMode_Param1 = 0.0f;
		pCD->m_ControlMode_Param2 = 0.0f;
		pCD->m_ControlMode_Param3 = 0.0f;
		pCD->m_ControlMode_Param4 = 0;
	}

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Checks if a thirdperson camera should be used, and 
					if so initializes it
						
	Parameters:		
		_iCharacter:	Character index of the character who activated
						this object
			
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CWObject_Phys_Ladder::DoActionSuccess(int _iCharacter)
{
	CACSClientData* pCD = GetACSClientData(this);
	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
	{
		CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

		if (pCD)
		{
			// Add the camera to the action
			spCActionCutsceneCamera spCamera = GetActionCutsceneCamera(m_iObject, _iCharacter);
			spCamera->SetActive();
			spCamera->OnRefresh();
			spCamera->SetValid(pCD);
			m_spActiveCutsceneCamera = spCamera;
			// Set thirdperson flag so the head will be shown
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
			 
			// Make sure the ledge camera will be updated when it's active
			m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;

			return true;
		}
	}
	
	return false;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Creates a ranom action cutscene camera, or if given 
					a specific camera index it will be created instead
						
	Parameters:		
		_iObject:		Object to focus on if thats what the camera 
						should do
		_iCharacter:	Character to focus on, if thats what the camera 
						should do
		_Specific:		Specific camera to use (if wanted)
			
	Returns:		Tries to create a nice cutscene camera for 
					ladder/ledge, or the specific one if if exists
						
	Comments:		
\*____________________________________________________________________*/
spCActionCutsceneCamera CWObject_Phys_Ladder::GetActionCutsceneCamera(int _iObject, 
		int _iCharacter, int _Specific)
{
	int iIndex = -1;

	if ((_Specific != -1) && (_Specific < m_CameraCount))
		iIndex = _Specific;
	else if (m_CameraCount > 0)
	{
		iIndex = MRTC_RAND() % m_CameraCount;
	}

	spCActionCutsceneCamera spCamera = MNew(CActionCutsceneCamera);
	
	if (spCamera != NULL)
	{
		spCamera->SetServer(m_pWServer);
		spCamera->OnCreate();
		if (iIndex != -1)
		{
			spCamera->ConfigureCamera(m_lCameraConfigs[iIndex]);
		}
		else
		{
			// Ok, no camera specifies, try to create a nice one automatically
			// should also be a bit random, eg left/rightside/close/far
			// Form:    "Mode:ViewXY:ViewXY:ViewZ:DistanceOffset:HeightOffset"
			// Example: "CHARACTER:BEHIND:NONE:LEVELED:100.0:60.0"
			const int nCams = 4;
			static char* pCameraConfigs[nCams] = 
			{
				"LADDER:BEHIND:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
				"LADDER:BEHIND:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"LADDER:BEHIND:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"LADDER:BEHIND:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
			};
			iIndex = MRTC_RAND() % nCams;

			if (!spCamera->ConfigureFromString(CStr(pCameraConfigs[iIndex])))
				spCamera->MakeDefaultCamera();
		}

		spCamera->SetCharacterAndObject(_iCharacter, _iObject);
	}

	return spCamera;
}

#define LADDER_STEPSIZE (20.0f)
#define LADDER_STEPSIZE (20.0f)
#define LADDER_MAXVELOCITY (3.0f)
#define LADDER_HANGRAILOFFSETZ (67.0f)
#define LADDER_LADDEROFFSETZ (64.0f)
#define LADDER_TOPENDPOINTOFFSET (64.0f)
int32 CWObject_Phys_Ladder::FindStepOffAnimType(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _LadderPos1, 
												const CVec3Dfp32& _LadderPos2,const CVec3Dfp32& _Normal)
{
	// Ok then, first find how many steps we have, then find which step we're on, and then
	// return correct stepoffanimation type that should be used
	fp32 LadderLen = (_LadderPos2 - _LadderPos1).Length();
	// Should round off to closest whole
	int32 NumSteps = RoundToInt(LadderLen / LADDER_STEPSIZE) + 1;
	bool bEven = (NumSteps % 2 == 0 ? true : false);

	// Ok, now we have number of steps, find what type of stepoff we're dealing with
	int32 StepOffType = FindStepOffType(_pWPhys, _LadderPos2, _Normal);
	switch (StepOffType)
	{
	case LADDER_STEPOFFTYPE_4PLUS:
		{
			/*if (!bEven)
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_LEFT4PLUS: POS: %s",_LadderPos1.GetString().GetStr()));
			else
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_RIGHT4PLUS: POS: %s",_LadderPos1.GetString().GetStr()));*/
			return (!bEven ? LADDER_ENDPOINTTYPE_LEFT4PLUS : LADDER_ENDPOINTTYPE_RIGHT4PLUS);
		}
	case LADDER_STEPOFFTYPE_0:
		{
			/*if (!bEven)
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_LEFT0: POS: %s",_LadderPos1.GetString().GetStr()));
			else
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_RIGHT0: POS: %s",_LadderPos1.GetString().GetStr()));*/
			return (!bEven ? LADDER_ENDPOINTTYPE_LEFT0 : LADDER_ENDPOINTTYPE_RIGHT0);
		}
	case LADDER_STEPOFFTYPE_4MINUS:
		{
			/*if (bEven)
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_LEFT4MINUS: POS: %s",_LadderPos1.GetString().GetStr()));
			else
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_RIGHT4MINUS: POS: %s",_LadderPos1.GetString().GetStr()));*/
			return (bEven ? LADDER_ENDPOINTTYPE_LEFT4MINUS : LADDER_ENDPOINTTYPE_RIGHT4MINUS);
		}
	case LADDER_STEPOFFTYPE_8MINUS:
		{
			/*if (bEven)
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_LEFT8MINUS: POS: %s",_LadderPos1.GetString().GetStr()));
			else
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_RIGHT8MINUS: POS: %s",_LadderPos1.GetString().GetStr()));*/
			return (bEven ? LADDER_ENDPOINTTYPE_LEFT8MINUS : LADDER_ENDPOINTTYPE_RIGHT8MINUS);
		}
	case LADDER_STEPOFFTYPE_12MINUS:
		{
			/*if (bEven)
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_LEFT12MINUS: POS: %s",_LadderPos1.GetString().GetStr()));
			else
				ConOut(CStrF("GOT: LADDER_STEPOFFTYPE_RIGHT12MINUS: POS: %s",_LadderPos1.GetString().GetStr()));*/
			return (bEven ? LADDER_ENDPOINTTYPE_LEFT12MINUS : LADDER_ENDPOINTTYPE_RIGHT12MINUS);
		}
	default:
		{
			ConOut("THIS SHOULD NOT HAPPEN!!");
			break;
		}
	}

	return 0;
}

#define TESTWIDTH (10.0f)
#define HALFTESTWIDTH (TESTWIDTH*0.5f)
#define SMALLOFFSET (0.1f)
int CWObject_Phys_Ladder::FindStepOffType(CWorld_PhysState* _pPhysState, 
										  const CVec3Dfp32& _LadderEnd, const CVec3Dfp32& _Normal)
{
	// Ok, we have 5 stepoffheights. 
	// First test middle (8) (deciding point)
	// If 8 collided -> either 12 or 16
	//		So test 12, if collide -> 16, if not -> 12
	// else (8 not collided -> either 0 or 4 or 8
	//		So test 4
	//		if collide -> 8
	//		else test 0, if collide -> 4 else 0
	
	CWO_PhysicsState PhysState;
	CWO_PhysicsPrim Prim;
	PhysState.m_nPrim = 1;
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
	PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
	//PhysState.m_PhysFlags = 0;
	// Intersect doors and doors/platforms
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
	// Use a bounding box test above ladder end and determine ladder type from there
	CVec3Dfp32 TestPoint;
	int Type = 0;

	// First test stepoff 8, if that doesn't collide it's either 0 or 4 or 8, 
	// if that collides it's either 12 or 16
	TestPoint = _LadderEnd + CVec3Dfp32(0,0,LADDER_STEPOFFHEIGHT_8 + HALFTESTWIDTH + SMALLOFFSET) - 
		_Normal * TESTWIDTH;
	Prim.Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH),0);
	
	// Set primitive
	TSelection<CSelection::LARGE_BUFFER> Selection;
	PhysState.m_Prim[0] = Prim;
	_pPhysState->Selection_AddIntersection(Selection, TestPoint, PhysState);
	const int16* pSel;
	int nSel = _pPhysState->Selection_Get(Selection, &pSel);

	// Debug draw the bitching testbox!!!
	/*{
		int32 Color = (nSel > 0 ? 0xffff0000 : 0xff00ff00);
		CBox3Dfp32 Box;
		Box.m_Min = TestPoint - CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH);
		Box.m_Max = TestPoint + CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH);
		_pPhysState->Debug_RenderAABB(Box,Color,10.0f);
	}*/

	// If we don't collide with anything check for 0 or 4 or 8
	if (nSel == 0)
	{

		// Test 4 if that doesn't collide it's either 0 or 4 otherwise 8, 
		TestPoint = _LadderEnd + CVec3Dfp32(0,0,LADDER_STEPOFFHEIGHT_4 + HALFTESTWIDTH + SMALLOFFSET) - 
			_Normal * TESTWIDTH;
		Prim.Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH),0);

		// Set primitive
		TSelection<CSelection::LARGE_BUFFER> Selection;
		PhysState.m_Prim[0] = Prim;
		_pPhysState->Selection_AddIntersection(Selection, TestPoint, PhysState);
		nSel = _pPhysState->Selection_Get(Selection, &pSel);
		
		// If selection is zero -> either 4 or zero, otherwise it's a 8
		if (nSel == 0)
		{
			TestPoint = _LadderEnd + CVec3Dfp32(0,0,LADDER_STEPOFFHEIGHT_0 + HALFTESTWIDTH + SMALLOFFSET) - 
				_Normal * TESTWIDTH;
			Prim.Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH),0);

			// Set primitive
			TSelection<CSelection::LARGE_BUFFER> Selection;
			PhysState.m_Prim[0] = Prim;
			_pPhysState->Selection_AddIntersection(Selection, TestPoint, PhysState);
			nSel = _pPhysState->Selection_Get(Selection, &pSel);

			// If selection is zero -> type 0, otherwise type 4
			if (nSel == 0)
			{
				// Offset 0 -> animtype 0
				Type = LADDER_STEPOFFTYPE_0;
			}
			else
			{
				// Offset 4 -> animtype +4
				Type = LADDER_STEPOFFTYPE_4PLUS;
			}
		}
		else
		{
			// Offset 8 -> animtype -12
			Type = LADDER_STEPOFFTYPE_12MINUS;
		}
	}
	else
	{
		// If 8 collided -> either 12 or 16
		//		So test 12, 
		TestPoint = _LadderEnd + CVec3Dfp32(0,0,LADDER_STEPOFFHEIGHT_12 + HALFTESTWIDTH + SMALLOFFSET) - 
			_Normal * TESTWIDTH;
		Prim.Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(HALFTESTWIDTH, HALFTESTWIDTH, HALFTESTWIDTH),0);

		// Set primitive
		TSelection<CSelection::LARGE_BUFFER> Selection;
		PhysState.m_Prim[0] = Prim;
		_pPhysState->Selection_AddIntersection(Selection, TestPoint, PhysState);
		nSel = _pPhysState->Selection_Get(Selection, &pSel);
		
		// collide -> 16, otherwise -> 12
		if (nSel == 0)
		{
			// Offset 12 -> animtype -8
			Type = LADDER_STEPOFFTYPE_8MINUS;
		}
		else
		{
			// Offset 16 -> animtype -4
			Type = LADDER_STEPOFFTYPE_4MINUS;
		}
	}

	return Type;
}

fp32 CWObject_Phys_Ladder::FindLadderPointOffset(const CVec3Dfp32& _LadderDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLadder, fp32 _LadderLength, fp32 _Offset1,
		fp32 _Offset2)
{
	// Check if we are outside the ladder, in that case set appropriate mode
	_PointOnLadder = _Pos1 + _LadderDir * ((_ObjPos - _Pos1) * _LadderDir);
	// Check if point is outside bounds (pos2<->pos1)
	CVec3Dfp32 PointOnLadder = _PointOnLadder - _LadderDir * _Offset1;
	fp32 LadderDot = ((PointOnLadder -_LadderDir) - _Pos1) * _LadderDir;
	if (LadderDot < 0.0f)
	{
		//ConOut("LADDERPOINT_BELOW");
		return LadderDot;
	}
	
	PointOnLadder = _PointOnLadder + _LadderDir * _Offset2;
	LadderDot = ((PointOnLadder +_LadderDir) - _Pos1) * _LadderDir;
	if (LadderDot > _LadderLength)
	{
		//ConOut("LADDERPOINT_ABOVE");
		return LadderDot - _LadderLength;
	}

	//ConOut("LADDERPOINT_NONE");
	return 0.0f;
}

/*LADDER_ENDPOINT_NOENDPOINT = 0,
	LADDER_ENDPOINT_ABOVE = 1,
	LADDER_ENDPOINT_BELOW = 2,*/
// Finds where we are on the ladder (middle/bottom/top)
int CWObject_Phys_Ladder::FindLadderPoint(const CVec3Dfp32& _LadderDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLadder, const fp32& _LadderLength, 
		const fp32& _Offset1, const fp32& _Offset2, fp32& _LadderDot)
{
	// Check if we are outside the ladder, in that case set appropriate mode
	_PointOnLadder = _Pos1 + _LadderDir * ((_ObjPos - _Pos1) * _LadderDir);
	// Check if point is outside bounds (pos2<->pos1)
	_LadderDot = (_PointOnLadder - _Pos1) * _LadderDir;
	CVec3Dfp32 PointOnLadder = _PointOnLadder - _LadderDir * _Offset1;
	fp32 LadderDot = (PointOnLadder - _Pos1) * _LadderDir;
	if (LadderDot < 0.0f)
	{
		//ConOut("LADDERPOINT_BELOW");
		return LADDER_ENDPOINT_BELOW;
	}
	
	PointOnLadder = _PointOnLadder + _LadderDir * _Offset2;
	LadderDot = (PointOnLadder - _Pos1) * _LadderDir;
	if (LadderDot > _LadderLength)
	{
		//ConOut("LADDERPOINT_ABOVE");
		return LADDER_ENDPOINT_ABOVE;
	}

	//ConOut("LADDERPOINT_NONE");
	return LADDER_ENDPOINT_NOENDPOINT;
}

fp32 CWObject_Phys_Ladder::FindStepOffTestHeight(int32 _Type)
{
	switch (_Type)
	{
	case LADDER_ENDPOINTTYPE_LEFT4PLUS:
	case LADDER_ENDPOINTTYPE_LEFT0:
	case LADDER_ENDPOINTTYPE_RIGHT4PLUS:
	case LADDER_ENDPOINTTYPE_RIGHT0:
		{
			return 80.1f;
		}
	default:
	/*case LADDER_ENDPOINTTYPE_LEFT4MINUS:
	case LADDER_ENDPOINTTYPE_LEFT8MINUS:
	case LADDER_ENDPOINTTYPE_LEFT12MINUS:
	case LADDER_ENDPOINTTYPE_RIGHT4MINUS:
	case LADDER_ENDPOINTTYPE_RIGHT8MINUS:
	case LADDER_ENDPOINTTYPE_RIGHT12MINUS:*/
		{
			return 60.1f;
		}
	}
}

#define LADDER_NORMALOFFSET (11.0f)
#define LADDER_ENDPOINTTESTZ (60.1f)
bool CWObject_Phys_Ladder::GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
										  CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, 
										   const CVec3Dfp32& _Move, const CMat4Dfp32& _MatLook, 
										   const uint32& _Press, uint32& _Released, 
										   CWO_Character_ClientData* _pCD, int16& _Flags)
{
	_Flags |= 3;	// Turn off gravity in PhysUtil_Move (and indicate "inAir")
	_VRet = 0;
	CVec3Dfp32 Pos1, Pos2, LadderNormal, ObjPos, LadderDir, PointOnLadder;
	fp32 LadderLength;
	ObjPos = _pObj->GetPosition();

	int iLadder = (int)_pCD->m_ControlMode_Param0;

	bool bResult = true;

	bResult = (_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
		0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	bResult = (_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
		0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	bResult = (_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL,
		0,0,_pObj->m_iObject, 0,0,0,&LadderNormal, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);

	// Control mode not correct yet
	if (!bResult)
		return false;

	LadderDir = Pos2 - Pos1;
	LadderLength = LadderDir.Length();
	LadderDir = LadderDir / LadderLength;
	//Height *= 2.0f;
	fp32 Width = (fp32)_pCD->m_Phys_Width;

	// Save ladder endpoint in m_Data
	int LadderType = (M_Fabs(LadderDir[2]) > 0.5f ? CWOBJECT_LADDER_TYPE_LADDER :
		CWOBJECT_LADDER_TYPE_HANGRAIL);
	
	fp32 LadderDot;
	int LadderPoint = (LadderType == CWOBJECT_LADDER_TYPE_LADDER ? 
		FindLadderPoint(LadderDir, ObjPos, Pos1, PointOnLadder, LadderLength, 0.0f,
			FindStepOffTestHeight(_pCD->m_ControlMode_Param4),LadderDot) :
		FindLadderPoint(LadderDir, ObjPos, Pos1, PointOnLadder, LadderLength, Width,Width,LadderDot));
	
	CWObject_CoreData* pLadder = _pPhysState->Object_GetCD(iLadder);
	if (pLadder)
		SetLadderEndpoint(pLadder,LadderPoint);
	PointOnLadder += LadderNormal;

	_Flags |= 2;

	CVec3Dfp32 MoveVelocity;
	CQuatfp32 RotVelocity;

	CWAG2I_Context AG2Context(_pObj, _pPhysState, _pCD->m_GameTime);
	_pCD->m_RelAnimPos.GetVelocityLadder(&AG2Context, _pCD->m_AnimGraph2.GetAG2I(), MoveVelocity, RotVelocity);

	RotVelocity.Normalize();
	CAxisRotfp32 RotVelAR;
	RotVelAR.Create(RotVelocity);

	_pPhysState->Object_SetVelocity(_pObj->m_iObject, MoveVelocity);
	_pPhysState->Object_SetRotVelocity(_pObj->m_iObject, RotVelAR);

	return true;

	/*if (LadderPoint != 0)
	{
		ConOut(CStrF("LadderPoint: %d Distance To Top: %f",LadderPoint,(PointOnLadder - Pos2).Length()));
	}*/

	// Normal ladder operation, adjust to middle
	// Only way to get off ladder in the middle is to jump off...
	/*{
		// -------------------------------------------------------------------
		//  PC:ish ladder control: Project movement velocity onto ladder-plane
		// -------------------------------------------------------------------
		
		// Find adjustment towards middle of ladder
		CVec3Dfp32 Side, Point, ObjPos;
		ObjPos = _pObj->GetPosition();

		CVec3Dfp32 AnimVelocity;
		// Get velocity from animation if its a hangrail
		CXR_MediumDesc MediumDesc;
		CWObject_Character::Char_ControlMode_Anim(_iSel,_pObj, _pPhysState, _Move, CVec3Dfp32::GetMatrixRow(_MatLook,0), _Press, _Released, 
			MediumDesc, _Flags);
		AnimVelocity = _pObj->GetVelocity().m_Move;
		
		LadderNormal.CrossProd(LadderDir, Side);
		CVec3Dfp32 CharPointOnLadder,Adjustment;
		Adjustment = 0.0f;
		if ((_pCD->m_AnimGraph.GetStateFlagsLo() & CHAR_STATEFLAG_MOVADVADJUST) && 
			(LadderType == CWOBJECT_LADDER_TYPE_LADDER))
		{
			CharPointOnLadder = PointOnLadder + LadderNormal * (LADDER_NORMALOFFSET + 0.01f);
			Adjustment = -Side * ((ObjPos - CharPointOnLadder) * Side);
			Adjustment -= (LadderNormal * ((ObjPos - CharPointOnLadder) * LadderNormal));

			// Remove side and normal from the velocity
			AnimVelocity -= Side * (Side * AnimVelocity);
			AnimVelocity -= LadderNormal * (LadderNormal * AnimVelocity);
			
			if (_pCD->m_AnimGraph.GetStateFlagsLo() & CHAR_STATEFLAG_LADDERSTEPADJUST)
			{
				// Zero height velocity
				AnimVelocity.k[2] = 0;

				// Adjust towards nearest step
				fp32 Point = LadderDot / LADDER_STEPSIZE;
				fp32 MaxFrac = (LadderLength/LADDER_STEPSIZE) - 3.0f;

				// Adjust to closest "whole"
				fp32 Diff;
				if (Point < 0.0f)
				{
					Diff = -Point;
				}
				else if (Point > MaxFrac)
				{
					Diff = MaxFrac - Point;
				}
				else
				{
					Diff = M_Floor(Point) - Point;
					if (M_Fabs(Diff) > 0.5f)
						Diff = M_Ceil(Point) - Point;
				}

				Adjustment.k[2] += Diff * LADDER_STEPSIZE;
			}

			// Don't adjust too much
			Adjustment *= 0.5f;
		}
		else if (LadderType == CWOBJECT_LADDER_TYPE_HANGRAIL)
		{
			if (_pCD->m_AnimGraph.GetStateFlagsLo() & CHAR_STATEFLAG_MOVADVADJUST)
			{
				// Hangrail adjust
				CharPointOnLadder = PointOnLadder + LadderNormal * LADDER_HANGRAILOFFSETZ;//_pCD->m_Phys_Height * 2.0f;
				Adjustment = -Side * ((ObjPos - CharPointOnLadder) * Side);
				Adjustment -= (LadderNormal * ((ObjPos - CharPointOnLadder) * LadderNormal));
				// Don't adjust too much
				Adjustment = Adjustment * 0.5f;

				// Remove side and height from the velocity
				AnimVelocity -= Side * (Side * AnimVelocity);
				AnimVelocity -= LadderNormal * (LadderNormal * AnimVelocity);
			}

			// Safety, don't go over past endpoints
			if (LadderDot < 0.0f)
			{
				AnimVelocity -= LadderDir * (LadderDir * AnimVelocity);
				Adjustment += LadderDir;
			}
			else if (LadderDot > LadderLength)
			{
				AnimVelocity -= LadderDir * (LadderDir * AnimVelocity);
				Adjustment -= LadderDir;
			}
		}

		fp32 LengthSqr = Adjustment.LengthSqr();

		if (LengthSqr > (LADDER_MAXVELOCITY * LADDER_MAXVELOCITY))
				Adjustment *= LADDER_MAXVELOCITY / M_Sqrt(LengthSqr);
		
		// Adjust position towards middle of ladder
		AnimVelocity += Adjustment;

		_pPhysState->Object_SetVelocity(_pObj->m_iObject, AnimVelocity);

		// Adjust look to closest ladder direction
		if (!(_pCD->m_AnimGraph.GetStateFlagsLo() & CHAR_STATEFLAG_FORCEROTATE))
		{
			CMat4Dfp32 Mat;
			CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);

			if (LadderType == CWOBJECT_LADDER_TYPE_HANGRAIL)
			{
				if (CharDir * LadderDir < 0.0f)
					LadderDir = -LadderDir;
			}
			else
			{
				LadderDir = -LadderNormal;
			}

			// Get direction
			_pCD->m_Control_Look_Wanted = CWObject_Character::GetLook(LadderDir);
			_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
			_pCD->m_Control_Look_Wanted.CreateMatrixFromAngles(0, Mat);
			_pPhysState->Object_SetRotation(_pObj->m_iObject, Mat);
		}
	}

	return true;*/
}

#define LADDER_PHYS_WIDTH (6)
#define LADDER_ENTERDISTANCEBELOW (12.0f)
#define LADDER_ENTERDISTANCEABOVE (12.0f)
void CWObject_Phys_Ladder::GrabLadder(int32 _iLadder, int32 _Type, CWorld_PhysState* _pWPhys,
									  CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD,
									  bool bForceInAir)
{
	if (!_pWPhys || !_pObj || !_pCD)
		return;

	if ((_Type & SELECTION_MASK_TYPE) == SELECTION_LADDER)
	{
		// Set ladder control mode
		CWObject_Character::Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_LADDER);
		((CWObject_Character *)_pObj)->UpdateVisibilityFlag();
		CWAG2I_Context AGContext(_pObj,_pWPhys,_pCD->m_GameTime);
		CXRAG2_Impulse StartLadder(AG2_IMPULSETYPE_CONTROLMODE,AG2_IMPULSEVALUE_CONTROLMODE_LADDER);
		_pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext,StartLadder,0);
		//ConOut("Setting Ladder controlmode");
		//CWObject_Character::Char_SetAnimSequence(pObj, pWPhys, 0);

		// Make the character use the ladder camera
		_pWPhys->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
			_pObj->m_iObject), _iLadder);

		// Set ladder object id at param0
		_pCD->m_ControlMode_Param0 = (fp32)_iLadder;
		// Make sure the character is "close" and facing the ladder before making him climb
		CVec3Dfp32 LadderNormal,Pos1,Pos2;
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL, 0,0, _pObj->m_iObject, 0, 0, 
			0, &LadderNormal, sizeof(CVec3Dfp32)), _iLadder);
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
			0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), _iLadder);
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
			0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), _iLadder);
		
		// Set stepoff type to param4 (an int)
		_pCD->m_ControlMode_Param4 = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_FINDSTEPOFFANIMTYPE), _iLadder);

		// Controls character body direction when on ladder
		_pCD->m_ControlMode_Param1 = LadderNormal.k[0];
		_pCD->m_ControlMode_Param2 = LadderNormal.k[1];
		_pCD->m_ControlMode_Param3 = LadderNormal.k[2];

		// Set cached phys width
		_pCD->m_LedgePhysWidthCached = _pCD->m_Phys_Width;
		// Set physbox width to the defined width (problems with animations in rotated positions otherwise)
		_pCD->m_Phys_Width = LADDER_PHYS_WIDTH;

		// Set the new physics
		CWObject_Character::Char_SetPhysics(_pObj, _pWPhys, NULL, CWObject_Character::Char_GetPhysType(_pObj), false, true);

		// Set startposition and direction of character when we grab the hangrail
		CVec3Dfp32 PointOnLadder;
		CVec3Dfp32 LadderDir = Pos2 - Pos1;
		fp32 LadderDot;
		fp32 Len = LadderDir.Length();
		LadderDir = LadderDir / Len;
		int EndPos = FindLadderPoint(LadderDir, _pObj->GetPosition(), Pos1, PointOnLadder, Len, 0.0f, 65.0f,LadderDot);
		CMat4Dfp32 PosMat = _pObj->GetPositionMatrix();
		CVec3Dfp32& PerfectPos = CVec3Dfp32::GetMatrixRow(PosMat,3);
		if (EndPos == LADDER_ENDPOINT_ABOVE)
		{
			PointOnLadder = Pos1 - LadderNormal * LADDER_ENTERDISTANCEABOVE;

			// Set z position as well
			int32 StepOffType = _pCD->m_ControlMode_Param4;
			fp32 EndPosZ = 0.0f;
			switch (StepOffType)
			{
			case LADDER_ENDPOINTTYPE_LEFT4PLUS:
			case LADDER_ENDPOINTTYPE_RIGHT4PLUS:
				{
					EndPosZ = -4.0f;
					break;
				}
			case LADDER_ENDPOINTTYPE_LEFT0:
			case LADDER_ENDPOINTTYPE_RIGHT0:
				{
					EndPosZ = 0.0f;
					break;
				}
			case LADDER_ENDPOINTTYPE_LEFT4MINUS:
			case LADDER_ENDPOINTTYPE_RIGHT4MINUS:
				{
					EndPosZ = 16.0f;
					break;
				}
			case LADDER_ENDPOINTTYPE_LEFT8MINUS:
			case LADDER_ENDPOINTTYPE_RIGHT8MINUS:
				{
					EndPosZ = 12.0f;
					break;
				}
			case LADDER_ENDPOINTTYPE_LEFT12MINUS:
			case LADDER_ENDPOINTTYPE_RIGHT12MINUS:
				{
					EndPosZ = 8.0f;
					break;
				}
			default:
				EndPosZ = 0.0f;
				break;
			}
			CVec3Dfp32 EndPos = Pos1 + LadderDir * Len;
			PerfectPos.k[2] = EndPos.k[2] + EndPosZ + SMALLOFFSET;
			//ConOut("DOING ABOVE!!!!!");
		}
		else if (EndPos == LADDER_ENDPOINT_BELOW)
		{
			PointOnLadder = Pos1 + LadderNormal * LADDER_ENTERDISTANCEBELOW;
		}
		else
		{
			// We're on the middle of the ladder somewhere
			PointOnLadder = Pos1 + LadderNormal * ((fp32)_pCD->m_LedgePhysWidthCached);
		}

		PerfectPos.k[0] = PointOnLadder[0];
		PerfectPos.k[1] = PointOnLadder[1];
		LadderNormal = -LadderNormal;
		LadderNormal.SetMatrixRow(PosMat,0);
		PosMat.RecreateMatrix(0,2);
		
		// Set "perfect" position and zero velocity
		//ConOut(CStrF("Perfect: %s Posmat: %s",PerfectPos.GetString().GetStr(),CVec3Dfp32::GetMatrixRow(PosMat,3).GetString().GetStr()));
		_pWPhys->Object_SetPosition(_pObj->m_iObject, PosMat);
		//ConOut("WARNING FAILED LADDER ENTRY");
		_pWPhys->Object_SetVelocity(_pObj->m_iObject,0);
		
		//_pWPhys->Debug_RenderAABB(*_pObj->GetAbsBoundBox(),0xffff00ff,10.0f);
	}
	else if ((_Type & SELECTION_MASK_TYPE) == SELECTION_HANGRAIL)
	{
		// Set ladder control mode
		CWObject_Character::Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_HANGRAIL);
		((CWObject_Character *)_pObj)->UpdateVisibilityFlag();
		//ConOut("Setting Hangrail controlmode");
		//CWObject_Character::Char_SetAnimSequence(pObj, pWPhys, 0);

		// Make the character use the ladder camera
		_pWPhys->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
			_pObj->m_iObject), _iLadder);

		// Set ladder object id at param0
		_pCD->m_ControlMode_Param0 = (fp32)_iLadder;
		
		// Set cached phys width
		_pCD->m_LedgePhysWidthCached = _pCD->m_Phys_Width;

		// Configure direction of hangrail
		// Check which direction we are heading
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 0);
		/*_pWPhys->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_LADDER_CONFIGUREHANGRAIL,0,0,0,0,Direction), _iLadder);*/
		// Make sure the character is "close" and facing the ladder before making him climb
		CVec3Dfp32 /*HangRailNormal, */ Pos1, Pos2;
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
			0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), _iLadder);
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
			0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), _iLadder);

		CVec3Dfp32 HangRailDir = Pos2 - Pos1;
		fp32 HohLength = HangRailDir.Length();
		HangRailDir = HangRailDir / HohLength;
		CVec3Dfp32 HangRailDirCached = HangRailDir;
		if (Direction * HangRailDir < 0.0f)
		{
			HangRailDir = -HangRailDir;
		}

		// Set enter type to param4 (an int)
		_pCD->m_ControlMode_Param4 = (_pCD->m_Phys_bInAir||bForceInAir ? HANGRAIL_ENTERTYPE_FROMAIR : HANGRAIL_ENTERTYPE_FROMGROUND);

		// Controls character body direction when on ladder
		_pCD->m_ControlMode_Param1 = HangRailDir.k[0];
		_pCD->m_ControlMode_Param2 = HangRailDir.k[1];
		_pCD->m_ControlMode_Param3 = HangRailDir.k[2];

		// Set startposition and direction of character when we grab the hangrail
		CMat4Dfp32 PosMat = _pObj->GetPositionMatrix();
		CVec3Dfp32& PerfectPos = CVec3Dfp32::GetMatrixRow(PosMat,3);
		CVec3Dfp32 PointOnHangRail = Pos1 + HangRailDirCached * ((PerfectPos - Pos1) * HangRailDirCached);
		fp32 LadderDot = HangRailDirCached * (PointOnHangRail - Pos1);
		// Check so that we're not outside the range
		if (LadderDot < 0.0f)
		{
			Pos1 += HangRailDirCached;
			PerfectPos.k[0] = Pos1[0];
			PerfectPos.k[1] = Pos1[1];
		}
		else if (LadderDot > HohLength)
		{
			Pos2 -= HangRailDirCached;
			PerfectPos.k[0] = Pos2[0];
			PerfectPos.k[1] = Pos2[1];
		}
		else
		{
			PerfectPos.k[0] = PointOnHangRail[0];
			PerfectPos.k[1] = PointOnHangRail[1];
		}

		if (_pCD->m_ControlMode_Param4 == HANGRAIL_ENTERTYPE_FROMAIR)
		{
			PerfectPos.k[2] = PointOnHangRail[2] - LADDER_HANGRAILOFFSETZ;
		}
		HangRailDir.SetMatrixRow(PosMat,0);
		PosMat.RecreateMatrix(0,2);
		// Set "perfect" position and zero velocity
		_pWPhys->Object_SetPosition(_pObj->m_iObject, PosMat);
		_pWPhys->Object_SetVelocity(_pObj->m_iObject,0);
	}
}

enum
{
	LADDERMOVE_CLIMBUP = 0,
	LADDERMOVE_SLIDEDOWN,
	LADDERMOVE_SLIDESTART,
	LADDERMOVE_SLIDESTOP,
	LADDERMOVE_SLIDESTOPEND,
	LADDERMOVE_CLIMBONDOWN,
	LADDERMOVE_CLIMBONUP,
	LADDERMOVE_IDLE,
	LADDERMOVE_CLIMBOFFUPL4PLUS,
	LADDERMOVE_CLIMBOFFUPL0,
	LADDERMOVE_CLIMBOFFUPL4MINUS,
	LADDERMOVE_CLIMBOFFUPL8MINUS,
	LADDERMOVE_CLIMBOFFUPL12MINUS,
	LADDERMOVE_CLIMBOFFUPR4PLUS,
	LADDERMOVE_CLIMBOFFUPR0,
	LADDERMOVE_CLIMBOFFUPR4MINUS,
	LADDERMOVE_CLIMBOFFUPR8MINUS,
	LADDERMOVE_CLIMBOFFUPR12MINUS
};

static uint8 slClimbOffTable[] = 
{
	LADDERMOVE_CLIMBOFFUPL4PLUS,
	LADDERMOVE_CLIMBOFFUPL0,
	LADDERMOVE_CLIMBOFFUPL4MINUS,
	LADDERMOVE_CLIMBOFFUPL8MINUS,
	LADDERMOVE_CLIMBOFFUPL12MINUS,
	LADDERMOVE_CLIMBOFFUPR4PLUS,
	LADDERMOVE_CLIMBOFFUPR0,
	LADDERMOVE_CLIMBOFFUPR4MINUS,
	LADDERMOVE_CLIMBOFFUPR8MINUS,
	LADDERMOVE_CLIMBOFFUPR12MINUS
};

bool CWObject_Phys_Ladder::MakeLadderMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _Move)
{
	// Ok then, we're making a ledge move
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD)
		return false;
	int iLadder = (int)pCD->m_ControlMode_Param0;
	CWObject_CoreData* pObjLadder = _pWPhys->Object_GetCD(iLadder);
	if (!pObjLadder)
		return false;

	CVec3Dfp32 Bottom,Top,Normal;
	bool bResult = true;
	bResult = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
		0,0,iLadder, 0,0,0,&Bottom, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	bResult = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
		0,0,iLadder, 0,0,0,&Top, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
	bResult = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL,
		0,0,iLadder, 0,0,0,&Normal, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);

	CVec3Dfp32 LadderDir = Top - Bottom;
	fp32 LadderLength = LadderDir.Length();
	LadderDir = LadderDir / LadderLength;
	CVec3Dfp32 CharPos = _pObj->GetPosition();
	CVec3Dfp32 PointOnLadder = Bottom + LadderDir * ((CharPos - Bottom) * LadderDir);
	// Check if point is outside bounds (pos2<->pos1)
	fp32 CurrentPos = (PointOnLadder - Bottom) * LadderDir;

	if (_Move == LADDERMOVE_CLIMBUP)
	{
		// Check so if we should use one of the climboff moves instead
		// First check how close we are to the endpoint
		int32 StepOffType = pCD->m_ControlMode_Param4;
		if (!(StepOffType >= LADDER_ENDPOINTTYPE_LEFT4PLUS && StepOffType <= LADDER_ENDPOINTTYPE_RIGHT12MINUS))
			return false;

		fp32 TestHeight = FindStepOffTestHeight(StepOffType);
		// check for closeness?
		fp32 LadderDot;
		if (FindLadderPoint(LadderDir, CharPos, Bottom, PointOnLadder, LadderLength, 0.0f,TestHeight,LadderDot) == LADDER_ENDPOINT_ABOVE)
			_Move = slClimbOffTable[StepOffType - 1];
	}

	if (_Move == LADDERMOVE_CLIMBONUP)
	{
		if ((CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0) * Normal < 0) &&  (CharPos - PointOnLadder) * Normal > 0)
		{
			_Move = LADDERMOVE_IDLE;
		}
		else
		{
			// CHeck what kind off stepoff height we have
//			int32 StepOffType = pCD->m_ControlMode_Param4;
			//CurrentPos = LadderLength - FindStepOffTestHeight(StepOffType);
			CurrentPos -= 58.0f;
		}
	}

	pCD->m_RelAnimPos.MakeLadderMove(_pWPhys,_pObj,pCD,_Move,Bottom,Top,Normal,CurrentPos);
	pCD->m_RelAnimPos.MakeDirty();

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets a vector in the user data
						
	Parameters:		
		_pObj:			Object to set vector in
		_Data:			Vector to set
		_Position:		What position in the user data array to set
							the vector in, remember, it takes up 3
							"slots" in the array
			
	Comments:		Position 0 = 0, Position 1 = 3 and so on in the user
					data array, so a maximum of two vector fits there
\*____________________________________________________________________*/
void CWObject_Phys_Ladder::SetUserData(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position)
{
	int Pos = _Position * sizeof(CVec3Dfp32) / sizeof(int32);

	if ((Pos + sizeof(CVec3Dfp32)/sizeof(int32)) < CWO_NUMDATA)
	{
		memcpy(&_pObj->m_Data[Pos], &_Data, sizeof(CVec3Dfp32));
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets a vector to user data, packed to 32 bits
					which means it will only take up one 
					"User data slot"
						
	Parameters:		
		_pObj:			Object to set vector in
		_Data:			Vector to set
		_Position:		What position in user data array to set the 
							packed vector in
			
	Comments:		
\*____________________________________________________________________*/
void CWObject_Phys_Ladder::SetUserDataNormal32(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position)
{
	CVec3Dfp32 Data = _Data;
	uint32 Packed = Data.Pack32(1.01f);

	*(uint32*)&(_pObj->m_Data[_Position]) = Packed;
}

/*void CWObject_Phys_Ladder::SetUserDataVector32(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position)
{
	CVec3Dfp32 Data = _Data;
	uint32 Final32 = 0;
	int32 MaxAxis = 511;
	// Make position relative to object position
	Data -= _pObj->GetPosition();
	for (int i = 0; i < 3; i++)
	{
		int32 Axis = (int)M_Ceil(Data.k[i]);
		// Sign bit at position 9, total 10 bits / axis (maybe Z should have one more or something?)
		int32 Sign = (Axis < 0 ? 1 << 9 : 0);
		// Max 9 bits == 511
		Axis = Min(MaxAxis, Abs(Axis));
		Final32 |= (Axis | Sign) << (10 * i);
	}

	*(uint32*)&(_pObj->m_Data[_Position]) = Final32;
}*/
// Major axis 
enum
{
	MAJORAXIS_X	= 0,
	MAJORAXIS_Y	= 1,
	MAJORAXIS_Z	= 2,
	AXISMASK	= 3 << 30,
	SIGN		= 1 << 29,
	DATAMASK = ~(AXISMASK | SIGN),
};
void CWObject_Phys_Ladder::SetUserDataVector32MA(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, 
												 int _Position)
{
	CVec3Dfp32 Data = _Data;
	uint32 Final32;
	// Make position relative to object position
	Data -= _pObj->GetPosition();
	// Determine major axis
	int32 MajorAxis;
	CVec3Dfp32 AbsData;
	AbsData.k[0] = M_Fabs(Data[0]);
	AbsData.k[1] = M_Fabs(Data[1]);
	AbsData.k[2] = M_Fabs(Data[2]);
	if (AbsData[0] > AbsData[1])
	{
		if (AbsData[0] > AbsData[2])
			// Major axis X
			MajorAxis = MAJORAXIS_X;
		else
			// Major axis Z
			MajorAxis = MAJORAXIS_Z;
	}
	else
	{
		if (AbsData[1] > AbsData[2])
			// Major axis Y
			MajorAxis = MAJORAXIS_Y;
		else
			// Major axis Z
			MajorAxis = MAJORAXIS_Z;
	}

	// Store major axis information at bits 30-31
	int32 Len = (int32)Data[MajorAxis];
	// If it's over  big you're screwed anyways
	if (Len < 0)
	{
		Len = -Len;
		Len |= SIGN;
	}

	Final32 = MajorAxis << 30;
	Final32 |= Len;
	*(uint32*)&(_pObj->m_Data[_Position]) = Final32;
}

void CWObject_Phys_Ladder::GetUserDataVector32MA(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, 
												 int _Position)
{
	CVec3Dfp32 Data = 0;
	uint32 Final32 = *(uint32*)&(_pObj->m_Data[_Position]);
	int32 MajorAxis = (Final32 & AXISMASK) >> 30;
	fp32 Value = (fp32)(Final32 & DATAMASK);
	if (Final32 & SIGN)
		Value = -Value;
	Data[MajorAxis] = Value;
	Data += _pObj->GetPosition();

	_Data = Data;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets a vector from user data
						
	Parameters:		
		_pObj:			Object to get vector from
		_Data:			Sets the vector in the userdata to this one
		_Position:		What position in the user data array to get
							the vector from, remember, it takes up 3
							"slots" in the array
			
	Comments:		Position 0 = 0, Position 1 = 3 and so on in the user
					data array, so a maximum of two vector fits there
\*____________________________________________________________________*/
void CWObject_Phys_Ladder::GetUserData(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position)
{
	int Pos = _Position * sizeof(CVec3Dfp32) / sizeof(int32);
	
	if ((Pos + sizeof(CVec3Dfp32)/sizeof(int32)) < CWO_NUMDATA)
	{
		memcpy(&_Data, &_pObj->m_Data[Pos], sizeof(CVec3Dfp32));
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets a packed vector from user data
						
	Parameters:		
		_pObj:			Object to get vector from
		_Data:			Sets the vector in the userdata to this one
		_Position:		What position in the user data array to get
							the vector from, since the vector is 
							packed it only takes up 1 user data slot
			
	Comments:		
\*____________________________________________________________________*/
void CWObject_Phys_Ladder::GetUserDataNormal32(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position)
{
	uint32 Packed;

	Packed = *(uint32*)&(_pObj->m_Data[_Position]);
	_Data.Unpack32(Packed,1.01f);
}

/*void CWObject_Phys_Ladder::GetUserDataVector32(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position)
{
	CVec3Dfp32 Data;
	uint32 Final32 = *(uint32*)&(_pObj->m_Data[_Position]);
	int32 MaxAxis = 511;
	// Make position relative to object position
	for (int i = 0; i < 3; i++)
	{
		int32 Axis = (Final32 >> (10 * i)) & 1023;
		int Sign = Axis & (1 << 9);
		if (Sign)
			Axis = -(Axis & ~(1 << 9));
		Data[i] = (fp32) Axis;
	}

	Data += _pObj->GetPosition();

	_Data = Data;
}*/

void CWObject_Phys_Ladder::GetLadderEndpoint(CWObject_CoreData* _pObj, int& _EndPoint)
{
	_EndPoint = _pObj->m_Data[LADDERENDPOINT] & LADDER_ENDPOINT_MASK;
}

void CWObject_Phys_Ladder::SetLadderEndpoint(CWObject_CoreData* _pObj, const int& _EndPoint)
{
	int Temp = _pObj->m_Data[LADDERENDPOINT] & ~LADDER_ENDPOINT_MASK;
	_pObj->m_Data[LADDERENDPOINT] = _EndPoint | Temp;
}

void CWObject_Phys_Ladder::GetHangrailMode(CWObject_CoreData* _pObj, int& _Mode)
{
	_Mode = _pObj->m_Data[LADDERENDPOINT] & ~LADDER_ENDPOINT_MASK;
}

void CWObject_Phys_Ladder::SetHangrailMode(CWObject_CoreData* _pObj, const int& _Mode)
{
	int Temp = _pObj->m_Data[LADDERENDPOINT] & LADDER_ENDPOINT_MASK;
	_pObj->m_Data[LADDERENDPOINT] = _Mode | Temp;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Phys_Ladder, CWObject_Phys_Ladder_Parent, 0x0100);

