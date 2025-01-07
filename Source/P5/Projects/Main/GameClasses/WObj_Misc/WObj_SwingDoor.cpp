#include "PCH.h"
#include "WObj_SwingDoor.h"
#include "WObj_ScenePoint.h"
#include "../WObj_CharMsg.h"
#include "../WObj_Sys/WObj_Trigger.h"
#include "../../../../Shared/Mos/Classes/Gameworld/WBlockNavGrid.h"

//#define DOOR_DEBUG

enum 
{ 
	class_CreepingDark = MHASH6('CWOb','ject','_','Cree','ping','Dark'),
};

void CWObject_SwingDoor::OnCreate()
{
	CWObject_SwingDoorParent::OnCreate();
	CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	m_SwingDoorFlags = SWINGDOORFLAGS_HANDLE_RIGHT_SIDE;
	m_iUserPortal = -1;
	m_QueuedAction = 0;
	m_LastOpenRequest = 0;
	pCD->m_Normal = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_Tangent = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	m_DemonArmAttach = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	m_MaxSwingAngle = 0.25f;
	m_MaxSwingTime = 1.0f;
	m_SwingAngle = 0.25f;
	m_SwingTime = 1.0f;
	m_Axis = CVec3Dfp32(0.0f,0.0f,1.0f);
	m_Offset = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_CurrentState = SWINGDOOR_STATE_CLOSED;
	m_StartTick = 0;
	m_iSoundOpenStart = 0;
	m_iSoundOpenStop = 0;
	m_iSoundMove = 0;
	m_iSoundMoveLoop = 0;
	m_iSoundShutStart = 0;
	m_iSoundShutStop = 0;
	m_iActivator = -1;
	m_HandleType = SWINGDOOR_HANDLE_TYPE_HANDLE;
	m_iHandle1 = -1;
	m_iHandle2 = -1;
	m_iHandleModel = -1;
	m_HandleSideOffset = 17.0f;
	m_DoorHeightOffset = 0.0f;
	m_HandleState = SWINGDOOR_HANDLE_STATE_IDLE;
	m_HandleStartTick = 0;
	m_HandleSwingTime = 0.3f;
	m_iBehaviour = 0;
	m_liDoorSPs[0] = -1;
	m_liDoorSPs[1] = -1;
	m_CloseRadius = 256.0f;
	m_FailedResetStartTick = -1;
	m_iLastAI = -1;
	m_TangentStart = 0.0f;
	m_LastAngle = 1.0f;
	m_ConstraintID = -1;
	m_LastOpenTick = 0;
	m_LastDynamicCollisionTick = 0;
	m_AIRequestedSPSTick = 0;

	SetRefresh(false);
}

void CWObject_SwingDoor::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	//ConOut(CStrF("Key: %s Value: %s",_pKey->GetThisName().GetStr(),_pKey->GetThisValue().GetStr()));
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	int32 KeyValuei = _pKey->GetThisValuei();
	fp32 KeyValuef = _pKey->GetThisValuef();
	switch (_KeyHash)
	{
	case MHASH2('FLAG','S'): // "FLAGS"
		{
			static const char *FlagsTranslate[] =
			{
				"gamephysics", "blocknav","noplayer","noai","nodarkling","neverclose","nodefaulttrigger","msgonce","locked","nodefaultsp","nodefaulthandle",NULL
			};

			m_SwingDoorFlags = KeyValue.TranslateFlags(FlagsTranslate);
			break;
		}
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
			Model_Set(0, m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName), false);
			break;
		}
	case MHASH4('MAXS','WING','ANGL','E'): // "MAXSWINGANGLE"
		{
			m_MaxSwingAngle = KeyValue.Val_fp64();
			break;
		}
	case MHASH3('SWIN','GTIM','E'): // "SWINGTIME"
		{
			m_MaxSwingTime = KeyValue.Val_fp64();
			break;
		}
	case MHASH1('AXIS'): // "AXIS"
		{
			m_Axis.ParseString(KeyValue);
			m_Axis.Normalize();
			break;
		}
	case MHASH2('ATTA','CH'): // "ATTACH"
		{
			m_Offset.ParseString(KeyValue);
			break;
		}
	case MHASH3('USER','PORT','AL'): // "USERPORTAL"
		{
			m_Userportal = KeyValue;
			break;
		}
	case MHASH2('NORM','AL'): // "NORMAL"
		{
			CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
			pCD->m_Normal.ParseString(KeyValue);
			pCD->m_Normal = pCD->m_Normal.GetSnapped(0.1f, 0.05f);
			pCD->m_Normal.Normalize();
			break;
		}
	case MHASH4('SOUN','D_OP','ENST','ART'): // "SOUND_OPENSTART"
		{
			m_iSoundOpenStart = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_OP','ENST','OP'): // "SOUND_OPENSTOP"
		{
			m_iSoundOpenStop = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_MO','VE'): // "SOUND_MOVE"
		{
			m_iSoundMove = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_MO','VELO','OP'): // "SOUND_MOVELOOP"
		{
			m_iSoundMoveLoop = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_SH','UTST','ART'): // "SOUND_SHUTSTART"
		{
			m_iSoundShutStart = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_SH','UTST','OP'): // "SOUND_SHUTSTOP"
		{
			m_iSoundShutStop = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('BEHA','VIOU','R'): // "BEHAVIOUR"
		{
			m_iBehaviour = KeyValuei;
			break;
		}
	case MHASH3('HAND','LE_T','YPE'):
		{
			m_HandleType = KeyValuei;
		}
		break;
	case MHASH3('HAND','LE_M','ODEL'):
		{
			m_iHandleModel = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
		}
		break;
	case MHASH3('HAND','LE_S','IDE'):
		{
			if(KeyValuei)
				m_SwingDoorFlags &= ~SWINGDOORFLAGS_HANDLE_RIGHT_SIDE;
		}
		break;
	case MHASH5('HAND','LE_S','IDE_','OFFS','ET'):
		{
			m_HandleSideOffset = KeyValuef;
		}
		break;
	case MHASH5('DOOR','_HEI','GHT_','OFFS','ET'):
		{
			m_DoorHeightOffset = KeyValuef;
		}
		break;
	case MHASH2('RADI','US'):
		{
			m_CloseRadius = KeyValuef;
		}
		break;
	case MHASH2('MAXA','NGLE'):
		{
			m_MaxSwingAngle = KeyValuef / 360.0f;
		}
		break;
	case MHASH2('OPEN','TIME'):
		{
			m_MaxSwingTime = KeyValuef;
		}
		break;
	default:
		{
			if (KeyName.Find("CHILD") != -1)
			{
				// Add child
				int32 Len = m_lChildrenNames.Len();
				for (int32 i = 0; i < Len; i++)
				{
					if (m_lChildrenNames[i] == KeyValue)
						return;
				}
				m_lChildrenNames.Add(KeyValue);
			}
			else if (KeyName.Find("MSG_OPEN") != -1)
			{
				m_OpenMessage.Parse(KeyValue, m_pWServer);
			}
			else if (KeyName.Find("MSG_CLOSE") != -1)
			{
				m_CloseMessage.Parse(KeyValue, m_pWServer);
			}
			else
				CWObject_SwingDoorParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_SwingDoor::OnFinishEvalKeys()
{
	CWObject_SwingDoorParent::OnFinishEvalKeys();
	// If we don't have a name create one
	if (CStr(GetName()).Len() == 0)
	{
		// Create name
		static int sIdent = 0;
		CStr Name = CStrF("SWINGDR_%d",sIdent);
		while (m_pWServer->Selection_GetSingleTarget(Name) > 0)
		{
			sIdent++;
			Name = CStrF("SWINGDR_%d",sIdent);
		}
		m_pWServer->Object_SetName(m_iObject,Name);
	}

	CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(pCD->m_Normal, m_Tangent);

	//Turn on physics on the door and set up constraint
	int iFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	if (m_SwingDoorFlags & SWINGDOORFLAGS_BLOCKNAVIGATION)
		iFlags |= OBJECT_FLAGS_NAVIGATION;
	if(m_iModel[0] > 0)
		Model_SetPhys(m_iModel[0], false, iFlags, OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_PUSHABLE | OBJECT_PHYSFLAGS_ROTATION | OBJECT_PHYSFLAGS_PHYSICSCONTROLLED | OBJECT_PHYSFLAGS_PHYSMOVEMENT);

	SetMass(49.0f);
	m_pWServer->Object_InitRigidBody(m_iObject, false);


	//Try adding a box
/*	CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	if (pM)
	{
		CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
		if (pPhysModel)
		{
			CMat4Dfp32 PosMat = GetPositionMatrix();
			CBox3Dfp32 BBox;
			pPhysModel->Phys_GetBound_Box(PosMat, BBox);
			CWO_PhysicsState PhysState(GetPhysState());
			PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, (BBox.m_Max - BBox.m_Min) * 0.5f, 0));
			PhysState.m_ObjectFlags = iFlags | OBJECT_FLAGS_PLAYERPHYSMODEL;
			PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION;
			m_pWServer->Object_SetPhysics(m_iObject, PhysState);
		}
	}*/
	
	//ClientFlags() |= CLIENTFLAGS_PHYSICS;
	//m_Flags |= FLAGS_GAMEPHYSICS;

	m_TangentStart = m_Tangent * GetPositionMatrix().GetRow(1);
}


bool CWObject_SwingDoor::GetSetupValues(CVec3Dfp32& _XDir, fp32& _Thickness, CBox3Dfp32& _InBox)
{
	CSwingDoorClientData* pCD = GetSwingDoorClientData(this);

	CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	if (pM)
	{
		CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
		if (pPhysModel)
		{
			CBox3Dfp32 OutBox;
			CMat4Dfp32 PosMat = GetPositionMatrix();
			pPhysModel->Phys_GetBound_Box(PosMat, _InBox);
			m_DoorBottomPos = PosMat.GetRow(3);
			m_DoorBottomPos.k[2] = _InBox.m_Min.k[2];
			OutBox = _InBox;
			// Ok got the bounding box, extrude along x-axis
			_XDir = pCD->m_Normal; //(CVec3Dfp32::GetMatrixRow(PosMat,0));

			fp32 BoxSize = M_Fabs((_InBox.m_Max - _InBox.m_Min) * m_Tangent);
			if (m_Offset == CVec3Dfp32(0.0f))
			{
				// No offset, use calculated offset
				m_Offset = CVec3Dfp32::GetMatrixRow(PosMat,3) - m_Tangent * (BoxSize * 0.5f);
			}
	
			// Check out our offset and which direction x should be :)
			if (m_Offset != CVec3Dfp32(0.0f))
			{
				CVec3Dfp32 Center;
				_InBox.GetCenter(Center);
				fp32 Dot = (m_Offset - Center) * m_Tangent;
				if (Dot > 0)
					_XDir = -_XDir;
			}

			_Thickness = M_Fabs((_InBox.m_Max - _InBox.m_Min) * _XDir);
			return true;
		}
	}
	return false;
}


void CWObject_SwingDoor::CalcHandlePositions()
{
	CVec3Dfp32 XDir;
	fp32 Thickness;
	CBox3Dfp32 InBox;
	if (!GetSetupValues(XDir, Thickness, InBox))
		return;

	// Handle 1
	CMat4Dfp32 HandlePos = GetPositionMatrix();
	HandlePos.GetRow(0) = XDir;
	if(m_HandleType == 1)
	{
		if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
		{
			HandlePos.GetRow(1) = m_Tangent;
			HandlePos.GetRow(2) = -GetPositionMatrix().GetRow(2);
			HandlePos.GetRow(3) += m_Tangent * m_HandleSideOffset;
		}
		else
		{
			HandlePos.GetRow(1) = -m_Tangent;
			HandlePos.GetRow(3) -= m_Tangent * m_HandleSideOffset;
		}
	}
	else
	{
		if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
		{
			HandlePos.GetRow(1) = -m_Tangent;
			HandlePos.GetRow(3) += m_Tangent * m_HandleSideOffset;
		}
		else
		{
			HandlePos.GetRow(1) = -m_Tangent;
			HandlePos.GetRow(3) -= m_Tangent * m_HandleSideOffset;
		}
	}
	HandlePos.GetRow(3) += XDir * (Thickness / 2.0f);
	HandlePos.k[3][2] -= (InBox.m_Max.k[2] - InBox.m_Min.k[2]) / 2.0f;
	HandlePos.k[3][2] += 32.0f - m_DoorHeightOffset;
	m_Handle1OrgMat = HandlePos;

	// Handle 2 
	HandlePos = GetPositionMatrix();
	HandlePos.GetRow(0) = -XDir;
	if(m_HandleType == 1)
	{
		if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
		{
			HandlePos.GetRow(1) = m_Tangent;
			HandlePos.GetRow(3) += m_Tangent * m_HandleSideOffset;
		}
		else
		{
			HandlePos.GetRow(1) = -m_Tangent;
			HandlePos.GetRow(2) = -GetPositionMatrix().GetRow(2);
			HandlePos.GetRow(3) -= m_Tangent * m_HandleSideOffset;
		}
	}
	else
	{
		if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
		{
			HandlePos.GetRow(1) = m_Tangent;
			HandlePos.GetRow(3) += m_Tangent * m_HandleSideOffset;
		}
		else
		{
			HandlePos.GetRow(1) = m_Tangent;
			HandlePos.GetRow(3) -= m_Tangent * m_HandleSideOffset;
		}
	}
	HandlePos.GetRow(3) -= XDir * (Thickness / 2.0f);
	HandlePos.k[3][2] -= (InBox.m_Max.k[2] - InBox.m_Min.k[2]) / 2.0f;
	HandlePos.k[3][2] += 32.0f - m_DoorHeightOffset;

	m_Handle2OrgMat = HandlePos;
}


void CWObject_SwingDoor::CreateScenepoints()
{
	if(!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTHANDLE))
	{
		CVec3Dfp32 XDir;
		fp32 Thickness;
		CBox3Dfp32 InBox;
		GetSetupValues(XDir, Thickness, InBox);

		for(int i = 0; i < 2; i++)
		{
			CWO_ScenePoint sp;
			sp.m_Position = (i == 0) ? m_Handle1OrgMat : m_Handle2OrgMat;
			fp32 d = (i == 0) ? 1.0f : -1.0f;
			sp.m_Position.GetRow(3) -= XDir * 19.0f * d;
			sp.m_Position.GetRow(3) += m_Tangent * 4.0f;
			sp.m_Position.k[3][2] -= 32.0f + m_DoorHeightOffset;
			sp.m_Type = CWO_ScenePoint::DOOR;
			sp.m_SqrRadius = 0.0f;
			sp.m_iBehaviour = m_iBehaviour;
			sp.m_Flags |= CWO_ScenePoint::FLAGS_PLAYONCE;
			CWO_SimpleMessage Msg;
			Msg.m_Msg = OBJMSG_SWINGDOOR_OPEN;
			Msg.m_StrParam = "1"; //This is so to avoid asking the AI about opening the door, since it's the AI that will activate this msg
			Msg.m_Target = GetName();
			if(i)
				Msg.m_Param0 = 2;
			else
				Msg.m_Param0 = 0;
			sp.m_lMessages_Animation.Add(Msg);

			// Everyone ($all) can use the scenepoint (but only one at a time?)
			CWO_ScenePointInitBlock sp_init;
			CNameHash name_hash("$all");
			sp_init.m_lUsers.Add(name_hash);
			// All users must get the door sp, but only one at a time can really use it
			sp.m_Resource.Init(1,CWO_ScenePoint::RESOURCE_MINRESERVATIONTIME, CWO_ScenePoint::RESOURCE_POLLINTERVAL);
			CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());
			m_liDoorSPs[i] = pSPM->AddScenePoint(sp, sp_init);
		}
	}
	else
		ConOutL(CStrF("§cf00ERROR: Door %s has scenepoints but no handles, can't place scene points correctly, skipping", GetName()));
}


void CWObject_SwingDoor::CreateHandles()
{
	if (!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTHANDLE) && m_iHandleModel != -1)
	{
		if (m_iHandle1 <= 0)
		{
			m_iHandle1 = m_pWServer->Object_Create("Model", m_Handle1OrgMat, m_iObject);
			if(m_iHandle1 > 0)
			{
				m_pWServer->Object_AddChild(m_iObject, m_iHandle1);
				CWObject_Model* pObj = safe_cast<CWObject_Model>(m_pWServer->Object_Get(m_iHandle1));

				pObj->Model_Set(0, m_iHandleModel);
				pObj->UpdateVisibility();
			}
		}

		if (m_iHandle2 <= 0)
		{
			m_iHandle2 = m_pWServer->Object_Create("Model", m_Handle2OrgMat, m_iObject);
			if(m_iHandle2 > 0)
			{
				m_pWServer->Object_AddChild(m_iObject, m_iHandle2);
				CWObject_Model* pObj = safe_cast<CWObject_Model>(m_pWServer->Object_Get(m_iHandle2));

				pObj->Model_Set(0, m_iHandleModel);
				pObj->UpdateVisibility();
			}
		}
	}
}


void CWObject_SwingDoor::CreateConstraint()
{
	CVec3Dfp32 XDir;
	fp32 Thickness;
	CBox3Dfp32 InBox;
	if (!GetSetupValues(XDir, Thickness, InBox))
		return;

	fp32 BoxSize = M_Fabs((InBox.m_Max - InBox.m_Min) * m_Tangent);
	fp32 BoxHeight = M_Fabs((InBox.m_Max - InBox.m_Min) * CVec3Dfp32(0.0f, 0.0f, 1.0f)) / 2.0f;

	CMat4Dfp32 WorldMat = GetPositionMatrix();
	WorldMat.GetRow(3) += (m_Tangent * BoxSize / 2.0f);
	m_WallPos = WorldMat.GetRow(3);

	// don't recreate constraint if already created
	if (m_ConstraintID < 0)
	{
		m_ConstraintID = m_pWServer->Phys_AddAxisConstraint2(m_iObject, WorldMat, BoxHeight, m_MaxSwingAngle, m_MaxSwingAngle, true);
		M_TRACE("Door %d, created constraint: %d\n", m_iObject, m_ConstraintID);
	}
}


void CWObject_SwingDoor::CreateTriggers()
{
	CVec3Dfp32 XDir;
	fp32 Thickness;
	CBox3Dfp32 InBox;
	GetSetupValues(XDir, Thickness, InBox);

	CVec3Dfp32 Pos = GetPosition();
	CMat4Dfp32 PosMat = GetPositionMatrix();
	fp32 BoxSize = M_Fabs((InBox.m_Max - InBox.m_Min) * m_Tangent);
	CBox3Dfp32 OutBox = InBox;

	// Thickness == amount we have to move it...
	// Check which lies closest
	fp32 MaxDist = (InBox.m_Max - Pos) * XDir;
	//fp32 MinDist = (InBox.m_Min - Pos) * XDir;
	BoxSize -= Thickness;
	if (MaxDist > 0.0f)
	{
		InBox.m_Max -= XDir * Thickness;
		InBox.m_Min -= XDir * (Thickness + BoxSize);
		OutBox.m_Max += XDir * (Thickness + BoxSize);
		OutBox.m_Min += XDir * Thickness;
	}
	else
	{
		InBox.m_Max -= XDir * (Thickness + BoxSize);
		InBox.m_Min -= XDir * Thickness;
		OutBox.m_Max += XDir * Thickness;
		OutBox.m_Min += XDir * (Thickness + BoxSize);
	}

	//Make the trigger a bit wider
	MaxDist = (InBox.m_Max - Pos) * m_Tangent;
	if (MaxDist > 0.0f)
	{
		InBox.m_Max += m_Tangent * 20.0f;
		InBox.m_Min -= m_Tangent * 20.0f;
		OutBox.m_Max += m_Tangent * 20.0f;
		OutBox.m_Min -= m_Tangent * 20.0f;
	}
	else
	{
		InBox.m_Max -= m_Tangent * 20.0f;
		InBox.m_Min += m_Tangent * 20.0f;
		OutBox.m_Max -= m_Tangent * 20.0f;
		OutBox.m_Min += m_Tangent * 20.0f;
	}

	/*m_pWServer->Debug_RenderAABB(InBox,0xff0000ff,100,false);
	m_pWServer->Debug_RenderAABB(OutBox,0xff00ff00,100,false);*/

	// Ok then, created the two boxes needed for the triggers, now to create the 
	// triggers
	OutBox.GetCenter( CVec3Dfp32::GetMatrixRow(PosMat,3));
	int32 iOutBox = m_pWServer->Object_Create("trigger_ext",PosMat);
	InBox.GetCenter(CVec3Dfp32::GetMatrixRow(PosMat,3));
	int32 iInBox = m_pWServer->Object_Create("trigger_ext",PosMat);

	CWO_SimpleMessage Messages[2];

	Messages[0].m_Target = Messages[1].m_Target = GetName();
	Messages[0].m_Msg = Messages[1].m_Msg = OBJMSG_SWINGDOOR_OPEN;
	// 1 = close, 0 = openin, 2 = openout
	Messages[0].m_Param0 = 0;
	Messages[1].m_Param0 = 1;
	Messages[0].SendPrecache(m_iObject,m_pWServer);
	Messages[1].SendPrecache(m_iObject,m_pWServer);

	CWObject_Message MsgIn(OBJMSG_TRIGGER_CONFIGURE);
	// Param0 == flags, param1 number of normal messages
	// Physbox size from vecparam0/1 in the message
	// delaytime the m_isender
	// m_pData is array of messages
	MsgIn.m_Param0 = 0;
	MsgIn.m_Param1 = 1;
	MsgIn.m_iSender = 0;
	MsgIn.m_pData = Messages;
	MsgIn.m_DataSize = sizeof(CWO_SimpleMessage) * 2;
	MsgIn.m_VecParam1 = InBox.m_Max;
	MsgIn.m_VecParam0 = InBox.m_Min;
	m_pWServer->Message_SendToObject(MsgIn,iInBox);

	// Change type to openout
	Messages[0].m_Param0 = 2;
	Messages[0].SendPrecache(m_iObject,m_pWServer);

	MsgIn.m_VecParam1 = OutBox.m_Max;
	MsgIn.m_VecParam0 = OutBox.m_Min;
	m_pWServer->Message_SendToObject(MsgIn,iOutBox);

	if (!(m_SwingDoorFlags & SWINGDOORFLAGS_NODARKLING))
	{
		uint16 Notifyflags = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_GET_INTERSECT_NOTIFYFLAGS), iOutBox);
		Notifyflags |= OBJECT_FLAGS_WORLDTELEPORT; //This is so we can collide with darklings

		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_SET_INTERSECT_NOTIFYFLAGS, Notifyflags), iOutBox); 
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_TRIGGER_SET_INTERSECT_NOTIFYFLAGS, Notifyflags), iInBox);
	}
}


void CWObject_SwingDoor::OnSpawnWorld()
{
	CWObject_SwingDoorParent::OnSpawnWorld();
	CSwingDoorClientData* pCD = GetSwingDoorClientData(this);

	//CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	// Create two triggers from the model of the door (hopefully this works)
	// One for each case (openin/openout)
	// Also setup scenepoints for the AI and put on the handles on the door


	// Offset is in world coordinates atm
	if (m_Offset != CVec3Dfp32(0.0f))
		m_Offset = m_Offset - GetPosition();

	m_OriginalPosition = GetPositionMatrix(); 


	CalcHandlePositions();
	if (!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTSP))
		CreateScenepoints();

	//The triggers
	if (!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTTRIGGER))
		CreateTriggers();

	if(m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED)
	{	// Locking
		Lock(true);
	}

	if(m_Userportal.Len() > 0)
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddTarget(Selection, m_Userportal);
		m_pWServer->Selection_RemoveOnClass(Selection, m_iClass); // Remove swingdoors, in case the door and userportal have the same name..
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		if(nSel > 1)
		{
			ConOutL(CStrF("[SwingDoor] - ERROR: Multiple instances of userportal '%s', only one allowed!", m_Userportal.Str()));
			m_pWServer->Object_Destroy(m_iObject);
			return;
		}
		if(nSel > 0)
		{
			m_iUserPortal = pSel[0];
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETCONTROLLER, 0, 0, m_iObject), m_iUserPortal);
		}

		m_Userportal.Clear();
	}

	SetRefresh(false);

	// Add children
	CWObject_Message SetParentMsg(OBJMSG_SWINGDOORATTACH_SETPARENT,m_iObject);
	int Len = m_lChildrenNames.Len();

	for (int i = 0; i < Len; i++)
	{
		int iChild = m_pWServer->Selection_GetSingleTarget(m_lChildrenNames[i]);
		if (iChild > 0)
		{
			CMat4Dfp32 OrgPos = m_pWServer->Object_GetPositionMatrix(iChild);
			// Set us as parent for this child
			m_lChildren.Add(CChild(iChild,OrgPos));
			if(!m_pWServer->Message_SendToObject(SetParentMsg, iChild))
				m_pWServer->Object_AddChild(m_iObject, iChild);
		}
	}

	if(Len == 0 && (m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTSP))
		m_SwingDoorFlags |= SWINGDOORFLAGS_NOSP;

	// Clear string array
	m_lChildrenNames.Clear();

	// Send precache messages
	m_OpenMessage.SendPrecache(m_iObject, m_pWServer);
	m_CloseMessage.SendPrecache(m_iObject, m_pWServer);

	m_DemonArmAttach -= m_Tangent * m_HandleSideOffset;

	// Make sure the door start inactive and not moving, otherwise some doors on big levels seems to slowly open up without this
	CAxisRotfp32 Rot;
	Rot.Create(0.0f, 0.0f, 0.0f);
	m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
	m_pWServer->Phys_SetStationary(m_iObject, true);
}


void CWObject_SwingDoor::OnFinishDeltaLoad()
{
	CWObject_SwingDoorParent::OnFinishDeltaLoad();

	CreateHandles();
	CreateConstraint();
}


void CWObject_SwingDoor::SetRefresh(bool _bOn)
{
	int32 Len = m_lChildren.Len();
	if (_bOn)
	{
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		for (int32 i = 0; i < Len; i++)
		{
			CWObject* pObj = m_pWServer->Object_Get(m_lChildren[i].m_iChild);
			if (pObj)
				pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		}
	}
	else
	{
		if(m_CurrentState & SWINGDOOR_STATE_CLOSED)
		{
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
			for (int32 i = 0; i < Len; i++)
			{
				CWObject* pObj = m_pWServer->Object_Get(m_lChildren[i].m_iChild);
				if (pObj)
					pObj->ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
			}
		}
	}
}

void CWObject_SwingDoor::OpenDoorIn(bool _bForceOpen)
{
	if(m_FailedResetStartTick + 5 > m_pWServer->GetGameTick())
		return;

	//CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	m_LastOpenRequest = m_pWServer->GetGameTick();
	if (m_CurrentState & SWINGDOOR_STATE_CLOSED)
	{
		if(m_LastOpenTick + m_pWServer->GetGameTicksPerSecond() > m_pWServer->GetGameTick())
			return;

		// restore constraint
		if(m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED)
		{
			m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, m_MaxSwingAngle);
			m_SwingDoorFlags &= ~SWINGDOORFLAGS_CONSTRAINT_MODDED;
		}

		HandleOpen();
		SetRefresh(true);
		// Set starttime
		m_StartTick = m_pWServer->GetGameTick();
		m_CurrentState = (SWINGDOOR_STATE_OPENING_IN | SWINGDOOR_STATE_OPENING);
		// Open userportal
		if (m_iUserPortal != -1)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, m_iObject), m_iUserPortal);

		// Play open sound
		CVec3Dfp32 Pos = GetPosition();
		m_pWServer->Sound_At(Pos, m_iSoundOpenStart, 0);
		m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
		// Play looping sound
		m_iSound[0] = m_iSoundMoveLoop;
		m_pWServer->Sound_On(m_iObject, m_iSound[0],2);
		m_DirtyMask |= CWO_DIRTYMASK_SOUND;
		m_LastOpenTick = m_pWServer->GetGameTick();

		// Send open message
		if (!((m_SwingDoorFlags & SWINGDOORFLAGS_MESSAGEMASK_ONCE_OPEN) == SWINGDOORFLAGS_MESSAGEMASK_ONCE_OPEN))
		{
			m_OpenMessage.SendMessage(m_iObject,m_iObject,m_pWServer);
			m_SwingDoorFlags |= SWINGDOORFLAGS_MESSAGE_SENTOPEN;
		}
	}
	else
	{
		if(m_SwingAngle != m_MaxSwingAngle || _bForceOpen)
		{
			if(m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
			{
				//Calcuate new start tick so it matches where the door is atm
				fp32 p = m_SwingAngle / m_MaxSwingAngle;
				fp32 elapsed_time = (m_pWServer->GetGameTick() - m_StartTick) * m_pWServer->GetGameTickTime();
				if(elapsed_time < m_SwingTime)
					p = (m_SwingAngle * (elapsed_time / m_SwingTime)) / m_MaxSwingAngle;

				int StartTick = RoundToInt((m_MaxSwingTime * m_pWServer->GetGameTicksPerSecond()) * p);
				m_StartTick = m_pWServer->GetGameTick() - StartTick;	

				SetRefresh(true);
				m_CurrentState = (SWINGDOOR_STATE_OPENING_IN | SWINGDOOR_STATE_OPENING);

				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundOpenStart, 0);
				m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
				// Play looping sound
				m_iSound[0] = m_iSoundMoveLoop;
				m_pWServer->Sound_On(m_iObject, m_iSound[0],2);
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;

				m_SwingAngle = m_MaxSwingAngle;
				m_SwingTime = m_MaxSwingTime;
			}
			else if(m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
				OpenDoorOut();
		}
	}
}

void CWObject_SwingDoor::OpenDoorOut(bool _bForceOpen)
{
	//CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	if(m_FailedResetStartTick + 5 > m_pWServer->GetGameTick())
		return;

	m_LastOpenRequest = m_pWServer->GetGameTick();
	if (m_CurrentState & SWINGDOOR_STATE_CLOSED)
	{
		if(m_LastOpenTick + m_pWServer->GetGameTicksPerSecond() > m_pWServer->GetGameTick())
			return;

		// restore constraint
		if(m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED)
		{
			m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, m_MaxSwingAngle);
			m_SwingDoorFlags &= ~SWINGDOORFLAGS_CONSTRAINT_MODDED;
		}

		HandleOpen();
		// Set starttime
		SetRefresh(true);
		m_StartTick = m_pWServer->GetGameTick();
		m_CurrentState = (SWINGDOOR_STATE_OPENING_OUT | SWINGDOOR_STATE_OPENING);
		// Open userportal
		if (m_iUserPortal != -1)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, m_iObject), m_iUserPortal);

		// Play open sound
		CVec3Dfp32 Pos = GetPosition();
		m_pWServer->Sound_At(Pos, m_iSoundOpenStart, 0);
		m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
		// Play looping sound
		m_iSound[0] = m_iSoundMoveLoop;
		m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
		m_DirtyMask |= CWO_DIRTYMASK_SOUND;
		m_LastOpenTick = m_pWServer->GetGameTick();

		// Send open message
		if (!((m_SwingDoorFlags & SWINGDOORFLAGS_MESSAGEMASK_ONCE_OPEN) == SWINGDOORFLAGS_MESSAGEMASK_ONCE_OPEN))
		{
			m_OpenMessage.SendMessage(m_iObject,m_iObject,m_pWServer);
			m_SwingDoorFlags |= SWINGDOORFLAGS_MESSAGE_SENTOPEN;
		}
	}
	else
	{
		if(m_SwingAngle != m_MaxSwingAngle || _bForceOpen)
		{
			if(m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
			{
				//Calcuate new start tick so it matches where the door is atm
				fp32 p = m_SwingAngle / m_MaxSwingAngle;
				fp32 elapsed_time = (m_pWServer->GetGameTick() - m_StartTick) * m_pWServer->GetGameTickTime();
				if(elapsed_time < m_SwingTime)
					p = (m_SwingAngle * (elapsed_time / m_SwingTime)) / m_MaxSwingAngle;

				int StartTick = RoundToInt((m_MaxSwingTime * m_pWServer->GetGameTicksPerSecond()) * p);
				m_StartTick = m_pWServer->GetGameTick() - StartTick;	

				SetRefresh(true);
				m_CurrentState = (SWINGDOOR_STATE_OPENING_OUT | SWINGDOOR_STATE_OPENING);

				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundOpenStart, 0);
				m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
				// Play looping sound
				m_iSound[0] = m_iSoundMoveLoop;
				m_pWServer->Sound_On(m_iObject, m_iSound[0],2);
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;

				m_SwingAngle = m_MaxSwingAngle;
				m_SwingTime = m_MaxSwingTime;
			}
			else if(m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
				OpenDoorIn();
		}
	}
}

void CWObject_SwingDoor::HandleOpen(void)
{
	m_HandleStartTick = m_pWServer->GetGameTick();
	m_HandleState = SWINGDOOR_HANDLE_STATE_OPEN;
	SetRefresh(true);
}

void CWObject_SwingDoor::HandleClose(void)
{
	m_HandleStartTick = m_pWServer->GetGameTick();
	m_HandleState = SWINGDOOR_HANDLE_STATE_CLOSE;
}

void CWObject_SwingDoor::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_SwingDoorParent::OnIncludeClass(_pWData,_pWServer);
	_pWData->GetResourceIndex_Class("trigger_ext");
}

void CWObject_SwingDoor::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_SwingDoorParent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
	
	IncludeSoundFromKey("SOUND_OPENSTART", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_OPENSTOP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_MOVE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_MOVELOOP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_SHUTSTART", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_SHUTSTOP", _pReg, _pMapData);
}

void CWObject_SwingDoor::CloseDoor(bool _bForceClose)
{
	if(m_CurrentState & SWINGDOOR_STATE_CLOSED || m_SwingDoorFlags & SWINGDOORFLAGS_CD_OPENED_DOOR || ((m_SwingDoorFlags & SWINGDOORFLAGS_OPENED_BY_SCRIPT) && !_bForceClose))
		return;
	//CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
	if (((m_LastOpenRequest+1) < m_pWServer->GetGameTick() && 
		!(m_SwingDoorFlags & SWINGDOORFLAGS_NEVERCLOSE)) || _bForceClose)
	{
		//Only close door if player or activator isn't close to it
		fp32 l = 10000000.0f;
		CWObject *pObj = m_pWServer->Object_Get(m_iActivator);
		if(pObj)
		{
			CVec3Dfp32 Pos = pObj->GetPosition() - GetPosition();
			l = Pos.LengthSqr();
		}

		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		for(int i = 0; i < pGame->Player_GetNum(); i++)
		{
			int iPlayer = pGame->Player_GetObjectIndex(i);
			pObj = m_pWServer->Object_Get(iPlayer);
			CVec3Dfp32 Vec = pObj->GetPosition() - GetPosition();
			fp32 tmp = Vec.LengthSqr();
			if(tmp < l)
				l = tmp;

			//Only close door if we got our back towards it
			Vec.Normalize();
			if(Vec * pObj->GetPositionMatrix().GetRow(0) < 0.0f)
				l = 0.0f;
		}
		if(l > (m_CloseRadius * m_CloseRadius) || _bForceClose)
		{
			if ((!(m_CurrentState & (SWINGDOOR_STATE_OPENING | SWINGDOOR_STATE_CLOSING | SWINGDOOR_STATE_CLOSED)) && (m_LastOpenRequest+20) < m_pWServer->GetGameTick()))
			{
				// Set starttime
				SetRefresh(true);
				m_StartTick = m_pWServer->GetGameTick();
				m_CurrentState = (m_CurrentState | SWINGDOOR_STATE_CLOSING);
				m_QueuedAction = 0;

				// Play close sound
				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundShutStart, 0);
				m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
				// Play looping sound
				m_iSound[0] = m_iSoundMoveLoop;
				m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;

				SetDrawToNavgrid(false);
			}
			else if(!(m_CurrentState & SWINGDOOR_STATE_CLOSING) && !_bForceClose)
			{
				// Queue closing of the door
				SetRefresh(true);
				//ConOut("QUEUEING CLOSED DOOR");
				m_QueuedAction = SWINGDOOR_ACTION_CLOSEDOOR;
			}
			else if(!(m_CurrentState & SWINGDOOR_STATE_CLOSING) && _bForceClose)
			{
				m_FailedResetStartTick = m_pWServer->GetGameTick();

				//Fake lock the door
				Lock(true, true);

				SetRefresh(true);

				//Calcuate new start tick so it matches where the door is atm
				fp32 elapsed_time = (m_pWServer->GetGameTick() - m_StartTick) * m_pWServer->GetGameTickTime();
				fp32 p = 0.0f;
				if(elapsed_time < m_SwingTime)
					p = (elapsed_time / m_SwingTime);

				int StartTick = RoundToInt((m_MaxSwingTime * m_pWServer->GetGameTicksPerSecond()) * (1.0f - p));
				m_StartTick = m_pWServer->GetGameTick() - StartTick;
				m_CurrentState &= ~SWINGDOOR_STATE_OPENING;
				m_CurrentState = (m_CurrentState | SWINGDOOR_STATE_CLOSING);

				m_QueuedAction = 0;

				// Play close sound
				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundShutStart, 0);
				m_pWServer->Sound_At(Pos, m_iSoundMove, 0);
				// Play looping sound
				m_iSound[0] = m_iSoundMoveLoop;
				m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;

				SetDrawToNavgrid(false);
			}
		}
	}
}

CWObject_SwingDoor::CSwingDoorClientData* CWObject_SwingDoor::GetSwingDoorClientData(CWObject_CoreData* _pObj)
{
	if(_pObj->m_lspClientObj[0] == NULL)
	{
		_pObj->m_lspClientObj[0] = MNew(CSwingDoorClientData);
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_CreepingDark::GetCreepingDarkClientData", "Could not allocate ClientData.")
			CSwingDoorClientData *pData = (CSwingDoorClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		return pData;
	}
	else
		return (CSwingDoorClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

const CWObject_SwingDoor::CSwingDoorClientData* CWObject_SwingDoor::GetSwingDoorClientData(const CWObject_CoreData* _pObj)
{
	return (const CSwingDoorClientData *)(const CReferenceCount *)_pObj->m_lspClientObj[0];
}

int CWObject_SwingDoor::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	CSwingDoorClientData* pCD = const_cast<CSwingDoorClientData*>(GetSwingDoorClientData(this));
	if(!pCD)
		Error_static("CWObject_SwingDoor::OnCreateClientUpdate", "Unable to pack client update.");

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject_SwingDoorParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;

	pD += pCD->OnCreateClientUpdate(pD);

	return (uint8*)pD - _pData;
}


int CWObject_SwingDoor::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MAUTOSTRIP(CWObject_SwingDoor_OnClientUpdate, 0);
	MSCOPESHORT(CWObject_SwingDoor::OnClientUpdate);

	const uint8* pD = &_pData[CWObject_SwingDoorParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

	CSwingDoorClientData *pCD = GetSwingDoorClientData(_pObj);

	pD += pCD->OnClientUpdate(pD);

	return (uint8*)pD - _pData;
}

void CWObject_SwingDoor::OnRefresh()
{
	if(m_CurrentState & SWINGDOOR_STATE_DEMONARM)
		return;

	if(m_SwingDoorFlags & SWINGDOORFLAGS_CD_OPENED_DOOR)
	{	//Check if player is still using CD, if not we can close door
		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		CWObject* pObj = pGame->Player_GetObject(0);
		if(pObj)
		{
			CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pChar);
			if(!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK))
				m_SwingDoorFlags &= ~SWINGDOORFLAGS_CD_OPENED_DOOR;
		}
	}

	CWObject_SwingDoorParent::OnRefresh();
	// Update position of door (it it's swinging, otherwise turn refresh off)
	UpdateDoorPosition();

	// Auto-close door
	if (!(m_SwingDoorFlags & SWINGDOORFLAGS_NEVERCLOSE) && !(m_CurrentState & SWINGDOOR_STATE_OPENING) && m_CurrentState & (SWINGDOOR_STATE_OPENING_IN|SWINGDOOR_STATE_OPENING_OUT))
	{	
		fp32 OpenTime = (m_pWServer->GetGameTick() - m_StartTick) * m_pWServer->GetGameTickTime();
		if (OpenTime > 5.0f)
			CloseDoor();
	}

	if(m_FailedResetStartTick != -1)
	{
		fp32 OpenTime = (m_pWServer->GetGameTick() - m_FailedResetStartTick) * m_pWServer->GetGameTickTime();
		if (OpenTime > 5.0f)
		{
			Lock(false, true);
			m_FailedResetStartTick = -1;
		}
	}

#ifdef DOOR_DEBUG
	CXR_Model* pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	if (pM)
	{
		CXR_PhysicsModel* pPhysModel = pM->Phys_GetInterface();
		if (pPhysModel)
		{
			CBox3Dfp32 InBox,OutBox;
			CMat4Dfp32 PosMat = GetPositionMatrix();
			pPhysModel->Phys_GetBound_Box(PosMat, InBox);
			m_DoorBottomPos = PosMat.GetRow(3);
			m_DoorBottomPos.k[2] = InBox.m_Min.k[2];
			OutBox = InBox;
			// Ok got the bounding box, extrude along x-axis
			CVec3Dfp32 XDir;//(CVec3Dfp32::GetMatrixRow(PosMat,0));
			XDir = m_Normal;
			CVec3Dfp32 YDir;//(CVec3Dfp32::GetMatrixRow(PosMat,1));
			CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(m_Normal,YDir);

			fp32 BoxSize = M_Fabs((InBox.m_Max - InBox.m_Min) * YDir);
			if (m_Offset == CVec3Dfp32(0.0f))
			{
				// No offset, use calculated offset
				m_Offset = CVec3Dfp32::GetMatrixRow(PosMat,3) - YDir * (BoxSize * 0.5f);
			}

			// Check out our offset and which direction x should be :)
			if (m_Offset != CVec3Dfp32(0.0f))
			{
				CVec3Dfp32 Center;
				InBox.GetCenter(Center);
				fp32 Dot = (m_Offset - Center) * YDir;
				if (Dot > 0)
					XDir = -XDir;
			}
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
			fp32 ThickNess = M_Fabs((InBox.m_Max - InBox.m_Min) * XDir);
			fp32 MaxDist = (InBox.m_Max - Pos) * XDir;
			//fp32 MinDist = (InBox.m_Min - Pos) * XDir;
			BoxSize -= ThickNess;
			if (MaxDist > 0.0f)
			{
				InBox.m_Max -= XDir * ThickNess;
				InBox.m_Min -= XDir * (ThickNess + BoxSize);
				OutBox.m_Max += XDir * (ThickNess + BoxSize);
				OutBox.m_Min += XDir * ThickNess;
			}
			else
			{
				InBox.m_Max -= XDir * (ThickNess + BoxSize);
				InBox.m_Min -= XDir * ThickNess;
				OutBox.m_Max += XDir * ThickNess;
				OutBox.m_Min += XDir * (ThickNess + BoxSize);
			}

			//Make the trigger a bit wider
			MaxDist = (InBox.m_Max - Pos) * YDir;
			if (MaxDist > 0.0f)
			{
				InBox.m_Max += YDir * 20.0f;
				InBox.m_Min -= YDir * 20.0f;
				OutBox.m_Max += YDir * 20.0f;
				OutBox.m_Min -= YDir * 20.0f;
			}
			else
			{
				InBox.m_Max -= YDir * 20.0f;
				InBox.m_Min += YDir * 20.0f;
				OutBox.m_Max -= YDir * 20.0f;
				OutBox.m_Min += YDir * 20.0f;
			}
			m_pWServer->Debug_RenderAABB(InBox, 0xffff0000);
			m_pWServer->Debug_RenderAABB(OutBox, 0xff00ff00);
		}
	}
#endif
}

void CWObject_SwingDoor::UpdateDoorPosition()
{
	m_pWServer->Debug_RenderMatrix(GetPositionMatrix(), 1.0f);

	if (m_CurrentState & (SWINGDOOR_STATE_OPENING|SWINGDOOR_STATE_CLOSING|SWINGDOOR_STATE_PHYSICS))
	{
		bool bMoveDoor = false;
		if(m_CurrentState & (SWINGDOOR_STATE_OPENING|SWINGDOOR_STATE_CLOSING))
			bMoveDoor = true;
		if(m_CurrentState & SWINGDOOR_STATE_PHYSICS)
		{
			bMoveDoor = false;
			CAxisRotfp32 Rot = m_pWServer->Object_GetRotVelocity(m_iObject);
			//If door is moving enough from the physics we don'w do anything, if it's not we will take over and move the door ourself so it stops in a good position
			if(Rot.m_Angle > m_SwingAngle / (m_pWServer->GetGameTicksPerSecond() * m_SwingTime))
				bMoveDoor = false;
			else if(m_LastDynamicCollisionTick + 5 < m_pWServer->GetGameTick())
				bMoveDoor = true;
		}
		
		if(bMoveDoor)
		{
			if(m_CurrentState & SWINGDOOR_STATE_PHYSICS)
			{	//We need to figure out where we are moving
/*				CAxisRotfp32 Rot = m_pWServer->Object_GetRotVelocity(m_iObject);

				CVec3Dfp32 Right = GetPositionMatrix().GetRow(1);
				Right.k[2] = 0.0f;
				Right.Normalize();
				fp32 InOrOut = m_Tangent * Right;
				InOrOut = InOrOut - m_TangentStart;
				InOrOut = Clamp(InOrOut, -1.0f, 1.0f);
				fp32 Angle = M_ASin(InOrOut) / (_PI * 2.0f);*/
				
				//We need to find out if door is on out, or in-side and which direction it's swinging
				CVec3Dfp32 OrgRight(0.0f, 1.0f, 0.0f);
				fp32 Dot = OrgRight * GetPositionMatrix().GetRow(0);
				fp32 LastDot = OrgRight * GetLastPositionMatrix().GetRow(0);
				if(Dot > 0.0f)
				{	//Door is on 'out'-side
					if(LastDot > Dot && Dot < 0.98f)
					{	//Door is moving towards close position
						m_CurrentState = SWINGDOOR_STATE_CLOSING | SWINGDOOR_STATE_OPENING_OUT;
					}
					else
					{	//Door is moving towards OPEN_OUT position
						m_CurrentState = SWINGDOOR_STATE_OPENING | SWINGDOOR_STATE_OPENING_OUT;
					}
				}
				else
				{	//Door is on 'in'-side
					if(LastDot < Dot && Dot > -0.98f)
					{	//Door is moving towards close position
						m_CurrentState = SWINGDOOR_STATE_CLOSING | SWINGDOOR_STATE_OPENING_IN;
					}
					else
					{	//Door is moving towards OPEN_IN position
						m_CurrentState = SWINGDOOR_STATE_OPENING | SWINGDOOR_STATE_OPENING_IN;
					}
				}
			}

			CAxisRotfp32 Rot;
			Rot.m_Axis = CVec3Dfp32(0.0f, 0.0f, 1.0f);
			Rot.m_Angle = m_SwingAngle / (m_pWServer->GetGameTicksPerSecond() * m_SwingTime);	

			if(m_CurrentState & SWINGDOOR_STATE_CLOSING)
			{
				CVec3Dfp32 Right = GetPositionMatrix().GetRow(1);
				Right.k[2] = 0.0f;
				Right.Normalize();
				fp32 d = m_Tangent * Right;
				d = d - m_TangentStart;
				d = Clamp(d, -1.0f, 1.0f);
				fp32 Angle = M_Fabs(M_ASin(d) / (_PI * 2.0f));

				if(Angle - Rot.m_Angle < 0.0f)
				{
					if(!(m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED))
					{
						//Modify the constraint so we can stop the door exactly where we want to
						if(m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
							m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, 0.0f);
						else if(m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
							m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, 0.0f, m_MaxSwingAngle);
						m_SwingDoorFlags |= SWINGDOORFLAGS_CONSTRAINT_MODDED;
					}
				}
			}

			//Change rotation axis depending on which way we are opening the door
			if((m_CurrentState & SWINGDOOR_STATE_OPENING_OUT && !(m_CurrentState & SWINGDOOR_STATE_CLOSING)) ||
				(m_CurrentState & SWINGDOOR_STATE_OPENING_IN && m_CurrentState & SWINGDOOR_STATE_CLOSING))
			{
				//Rot.m_Angle *= -1.0f;
				Rot.m_Axis = CVec3Dfp32(0.0f, 0.0f, -1.0f);
			}

			m_pWServer->Phys_SetStationary(m_iObject, false);
			m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
		}
	}

	// Check if door has finished updating
	if(!(m_CurrentState & SWINGDOOR_STATE_PHYSICS) && m_CurrentState & (SWINGDOOR_STATE_OPENING|SWINGDOOR_STATE_CLOSING))
	{
		CVec3Dfp32 Right = GetPositionMatrix().GetRow(1);
		Right.k[2] = 0.0f;	//Constraint ain't perfect, sometimes the door can lean a bit when the player fucks with door, we must ignore that. 
		Right.Normalize();
		fp32 d = m_Tangent * Right;
		d = d - m_TangentStart;
		d = Clamp(d, -1.0f, 1.0f);
		fp32 Angle = M_Fabs(M_ASin(d) / (_PI * 2.0f));

		//find out if door is completly open
		bool bDone = false;
		if(Angle < 0.0002 && m_CurrentState & SWINGDOOR_STATE_CLOSING)
			bDone = true;	//Door is closed
		if(Angle > (m_SwingAngle - 0.001f) && m_CurrentState & SWINGDOOR_STATE_OPENING)
			bDone = true;

		if(m_pWServer->GetGameTick() - m_LastOpenRequest < 1.0f * m_pWServer->GetGameTicksPerSecond())
			bDone = false;

		if(m_CurrentState & SWINGDOOR_STATE_OPENING && Angle > (m_SwingAngle - 0.03f) && m_LastAngle > Angle && TruncToInt(m_StartTick + m_pWServer->GetGameTicksPerSecond() * 2.0f) < m_pWServer->GetGameTick())
			bDone = true;	//For some reason some doors can refuse to open all the way, this is a hack that forces the door to be finished if it's been trying for longer then 2 seconds

		m_LastAngle = Angle;

		if(bDone)
		{	//it's open, set the new states, update navgrid, user portals and start some sounds
			m_SwingTime = m_MaxSwingTime;
			m_iActivator = -1;
			if (m_CurrentState & SWINGDOOR_STATE_OPENING)
			{
				SetDrawToNavgrid(true);
				if (m_SwingDoorFlags & SWINGDOORFLAGS_NEVERCLOSE)
					SetRefresh(false);

				m_CurrentState &= ~SWINGDOOR_STATE_OPENING;
				// Play open stop sound
				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundOpenStop, 0);
				// Stop looping sound
				m_iSound[0] = 0;
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;

				CAxisRotfp32 Rot;
				Rot.Create(0.0f, 0.0f, 0.0f);
				m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
				m_pWServer->Phys_SetStationary(m_iObject, true);
			}
			else if (m_CurrentState & SWINGDOOR_STATE_CLOSING)
			{
				SetDrawToNavgrid(false);
				// Close userportal
				if (m_iUserPortal)
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 0, 0, m_iObject), m_iUserPortal);
				m_CurrentState = SWINGDOOR_STATE_CLOSED;

				//Door is to be locked
				if(m_SwingDoorFlags & SWINGDOORFLAGS_LOCK_WHEN_CLOSED)
				{
					Lock(true);
					m_SwingDoorFlags &= ~SWINGDOORFLAGS_LOCK_WHEN_CLOSED;
				}
				// Stop refreshing
				SetRefresh(false);
				// Play close stop sound
				CVec3Dfp32 Pos = GetPosition();
				m_pWServer->Sound_At(Pos, m_iSoundShutStop, 0);
				// Stop looping sound
				m_iSound[0] = 0;
				m_DirtyMask |= CWO_DIRTYMASK_SOUND;
				m_SwingDoorFlags &= ~SWINGDOORFLAGS_OPENED_BY_SCRIPT;

				// Send close message
				if (!((m_SwingDoorFlags & SWINGDOORFLAGS_MESSAGEMASK_ONCE_CLOSE) == SWINGDOORFLAGS_MESSAGEMASK_ONCE_CLOSE))
				{
					m_CloseMessage.SendMessage(m_iObject,m_iObject,m_pWServer);
					m_SwingDoorFlags |= SWINGDOORFLAGS_MESSAGE_SENTCLOSE;
				}

				CAxisRotfp32 Rot;
				Rot.Create(0.0f, 0.0f, 0.0f);
				m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
				m_pWServer->Phys_SetStationary(m_iObject, true);
			}
			if (m_QueuedAction == SWINGDOOR_ACTION_CLOSEDOOR)
			{
				//ConOut("CLOSING DOOR FROM QUEUE");
				CloseDoor();
			}
			m_iLastAI = -1;
		}
	}

	//Animate the handles
	if(m_iHandle1 != -1 && m_iHandle2 != -1)
	{
		CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
		switch(m_HandleState)
		{
		case SWINGDOOR_HANDLE_STATE_CLOSE:
			{
				uint32 compare = m_HandleStartTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * m_HandleSwingTime);
				if(compare >= m_pWServer->GetGameTick())
				{
					uint32 Tick = m_pWServer->GetGameTick() - m_HandleStartTick;
					fp32 time = Tick / (m_pWServer->GetGameTicksPerSecond() * m_HandleSwingTime);

					fp32 CurrentAngle = 0.0f;
					if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
						CurrentAngle = -(0.125f * (1.0f - time));	
					else
						CurrentAngle = 0.125f * (1.0f - time);	
					CWObject *pObj = m_pWServer->Object_Get(m_iHandle1);
					CMat4Dfp32 Mat;
					pCD->m_Normal.CreateAxisRotateMatrix(CurrentAngle, Mat);

					CMat4Dfp32 PosMat = m_Handle1OrgMat;
					CMat4Dfp32 NewPos;
					PosMat.Multiply3x3(Mat, NewPos);
					m_pWServer->Object_SetRotation(m_iHandle1, NewPos);

					pObj = m_pWServer->Object_Get(m_iHandle2);
					(-pCD->m_Normal).CreateAxisRotateMatrix(-CurrentAngle, Mat);

					PosMat = m_Handle2OrgMat;
					PosMat.Multiply3x3(Mat, NewPos);
					m_pWServer->Object_SetRotation(m_iHandle2, NewPos);
				}
				else
				{
					m_HandleStartTick = m_pWServer->GetGameTick();
					m_HandleState = SWINGDOOR_HANDLE_STATE_IDLE;
				}
			}
			break;
		case SWINGDOOR_HANDLE_STATE_OPEN:
			{
				uint32 compare = m_HandleStartTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * m_HandleSwingTime);
				if(compare > m_pWServer->GetGameTick())
				{
					uint32 Tick = m_pWServer->GetGameTick() - m_HandleStartTick;
					fp32 time = Tick / (m_pWServer->GetGameTicksPerSecond() * m_HandleSwingTime);
					
					fp32 CurrentAngle = 0.0f;
					if(m_SwingDoorFlags & SWINGDOORFLAGS_HANDLE_RIGHT_SIDE)
						CurrentAngle = -(0.125f * time);	
					else
						CurrentAngle = 0.125f * time;	
					
					CWObject *pObj = m_pWServer->Object_Get(m_iHandle1);
					CMat4Dfp32 Mat;
					pCD->m_Normal.CreateAxisRotateMatrix(CurrentAngle, Mat);

					CMat4Dfp32 PosMat = m_Handle1OrgMat;
					CMat4Dfp32 NewPos;
					PosMat.Multiply3x3(Mat, NewPos);
					m_pWServer->Object_SetRotation(m_iHandle1, NewPos);

					pObj = m_pWServer->Object_Get(m_iHandle2);
					(-pCD->m_Normal).CreateAxisRotateMatrix(-CurrentAngle, Mat);

					PosMat = m_Handle2OrgMat;
					PosMat.Multiply3x3(Mat, NewPos);
					m_pWServer->Object_SetRotation(m_iHandle2, NewPos);
				}
				else
				{
					m_HandleStartTick = m_pWServer->GetGameTick();
					m_HandleState = SWINGDOOR_HANDLE_STATE_CLOSE;
				}
			}
			break;
		case SWINGDOOR_HANDLE_STATE_IDLE:
			{
			}
			break;
		}
	}
}

//Door failed to open or close, something collided with it
//Find out if the collision will hinder us, if so change the direction on the door, otherwise carry on
void CWObject_SwingDoor::FailedToOpenCloseDoor(CVec3Dfp32 _CollisionPoint, CVec3Dfp32 _ObjectPos)
{
	CVec3Dfp32 At = m_WallPos - GetPosition();
	CVec3Dfp32 CollAt = m_WallPos - _CollisionPoint;
	CVec3Dfp32 DoorNormal;
	At.k[2] = 0.0f;
	At.Normalize();
	At.CrossProd(CVec3Dfp32(0.0f, 0.0f, 1.0f), DoorNormal);
	CollAt.k[2] = 0.0f;
	CollAt.Normalize();

	bool bChange = false;
	fp32 d = CollAt * DoorNormal;
	if(d < 0.0f)
	{	//The collision is on the 'in'-side
		if(m_CurrentState & SWINGDOOR_STATE_CLOSING && m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
			bChange = true;
		if(m_CurrentState & SWINGDOOR_STATE_OPENING && m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
			bChange = true;
	}
	else
	{	//The collision is on the 'out'-side
		if(m_CurrentState & SWINGDOOR_STATE_CLOSING && m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
			bChange = true;
		if(m_CurrentState & SWINGDOOR_STATE_OPENING && m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
			bChange = true;
	}

	if(!bChange)
	{	//Fallback, sometime the collision point isn't good enough, might be a precision problem or something, 
		//but the collision point is inside the door, and just enough to be on the other side, so we try once 
		//with the objects position as a fallback, this can cause incorrect results with complex objects
		CollAt = m_WallPos - _ObjectPos;
		CollAt.k[2] = 0.0f;
		CollAt.Normalize();
		fp32 d = CollAt * DoorNormal;
		if(d < 0.0f)
		{	//The collision is on the 'in'-side
			if(m_CurrentState & SWINGDOOR_STATE_CLOSING && m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
				bChange = true;
			if(m_CurrentState & SWINGDOOR_STATE_OPENING && m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
				bChange = true;
		}
		else
		{	//The collision is on the 'out'-side
			if(m_CurrentState & SWINGDOOR_STATE_CLOSING && m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
				bChange = true;
			if(m_CurrentState & SWINGDOOR_STATE_OPENING && m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
				bChange = true;
		}
	}
	
	if(bChange)
	{	//Something is in the way, abort
		if(m_iLastAI != -1)	//Tell the AI to abort
		{
			CWObject_Message Msg(OBJMSG_CHAR_AICANOPENDOOR, 0, 0, m_iObject);
			Msg.m_Param0 = -1;
			m_pWServer->Message_SendToObject(Msg, m_iLastAI);
		}

		if(m_CurrentState & SWINGDOOR_STATE_OPENING)
			CloseDoor(true);
		else if(m_CurrentState & SWINGDOOR_STATE_CLOSING)
		{
			if(m_CurrentState & SWINGDOOR_STATE_OPENING_OUT)
				OpenDoorOut(true);
			else if(m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
				OpenDoorIn(true);
		}
	}
}

int CWObject_SwingDoor::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_OnPhysicsEvent, 0);
	switch(_Event)
	{
	case CWO_PHYSEVENT_DYNAMICS_COLLISION :
		{
			//We got a collision, fix constraint and user portals, set up the correct states
			if(_pPhysState->IsServer())
			{
				CWorld_Server *pServer = (CWorld_Server *)_pPhysState;
				CWObject *pObj = pServer->Object_Get(_pObj->m_iObject);
				CWObject_SwingDoor *pSwingDoor = TDynamicCast<CWObject_SwingDoor>(pObj);
 				if(pSwingDoor)
				{
					if(!(pSwingDoor->m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED))
					{
						if(pSwingDoor->m_CurrentState & SWINGDOOR_STATE_CLOSED && pSwingDoor->m_iUserPortal != -1)
							pServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, pSwingDoor->m_iObject), pSwingDoor->m_iUserPortal);
						if(_pObjOther && pSwingDoor->m_CurrentState & (SWINGDOOR_STATE_CLOSING | SWINGDOOR_STATE_OPENING))
							pSwingDoor->FailedToOpenCloseDoor(_pCollisionInfo->m_Pos, _pObjOther->GetPosition());
						pSwingDoor->m_CurrentState |= SWINGDOOR_STATE_PHYSICS;
						pSwingDoor->SetRefresh(true);
						pSwingDoor->m_LastAngle = 1.0f;
						pSwingDoor->m_LastDynamicCollisionTick = pServer->GetGameTick();
						pSwingDoor->m_SwingDoorFlags &= ~SWINGDOORFLAGS_OPENED_BY_SCRIPT;
						//Restore the constraint
						if(pSwingDoor->m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED)
						{
							pServer->Phys_UpdateAxisConstraintAngles(pSwingDoor->m_ConstraintID, pSwingDoor->m_MaxSwingAngle, pSwingDoor->m_MaxSwingAngle);
							pSwingDoor->m_SwingDoorFlags &= ~SWINGDOORFLAGS_CONSTRAINT_MODDED;
						}
					}
				}
			}
			return CWObject_SwingDoorParent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
		}
		break;

	default :
		return CWObject_SwingDoorParent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	}
	return CWObject_SwingDoorParent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}

aint CWObject_SwingDoor::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	//Open msg from the triggers or script
	case OBJMSG_SWINGDOOR_OPEN:
		{
			// Characters can't open locked doors..
			if (m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED)
				return false;

			//Door resently failed, let it be for a while
			if(m_FailedResetStartTick + m_pWServer->GetGameTicksPerSecond() > m_pWServer->GetGameTick())
				return false;

			// If's an AI has triggered this, ask it if it want's to 
			// do the opening closing
			// Check if it's a player
			int bIsPlayer = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISPLAYER),_Msg.m_Param1);
			CWObject_CoreData *pCD = m_pWServer->Object_GetCD(_Msg.m_Param1);
			CWObject *pObj = m_pWServer->Object_Get(_Msg.m_Param1);
			if(pObj && pObj->IsClass(class_CreepingDark))
				return false;
			CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pObj);
			if(pChar)
			{
				CWO_Character_ClientData *pCharCD = pChar->GetClientData(pChar);

				if(pCharCD && pCharCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL)
					return false;
			}

			if(pCD)
			{
				CVec3Dfp32 Vel = pCD->GetMoveVelocity();

				fp32 Speed = Vel.Length();
				if(Speed > 4.0f)
					m_SwingTime = 0.3f;
			}

			bool bAskAI = true;
			if (!(m_CurrentState & SWINGDOOR_STATE_CLOSED))
			{	// Don't ask AI when door is not closed
				bAskAI = false;
			}
			else if(_Msg.m_DataSize)
			{
				CFStr Val = (const char *)_Msg.m_pData;
				if (Val.Compare("1") == 0)
				{
					bAskAI = false;
					m_iLastAI = m_iActivator;
				}
			}

			if (!bIsPlayer)
			{	// Don't bother with player
				CWObject_Message Msg(OBJMSG_CHAR_AICANOPENDOOR, 0, 0, m_iObject);

				//Check if this AI can use the door or if someone else is already using it
				if(m_iActivator != _Msg.m_Param1 && m_AIRequestedSPSTick + 5 > m_pWServer->GetGameTick())
				{
					Msg.m_Param0 = -1;
					Msg.m_VecParam0 = m_DoorBottomPos;
					m_pWServer->Message_SendToObject(Msg,_Msg.m_Param1);
					return false;
				}

				if (bAskAI)
				{
					if(m_FailedResetStartTick + 5 > m_pWServer->GetGameTick())
						return false;

					bool bDoorHasSP = true;
					if(!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTSP) && (m_SwingDoorFlags & SWINGDOORFLAGS_NOSP))
						bDoorHasSP = false;

					if(_Msg.m_Param0 == 0)
						Msg.m_Param0 = m_liDoorSPs[0];
					else
						Msg.m_Param0 = m_liDoorSPs[1];

					Msg.m_VecParam0 = m_DoorBottomPos;

					if((m_SwingDoorFlags & SWINGDOORFLAGS_NOAI) || (m_pWServer->Message_SendToObject(Msg,_Msg.m_Param1)))
					{
						//The AI will open the door with the SP or AIs can't open door (but will the AI now that) //TODO: Fix no use value in this msg for AIs
						m_AIRequestedSPSTick = m_pWServer->GetGameTick();
						m_iActivator = _Msg.m_Param1;
						return false;
					}
					m_iLastAI = m_iActivator;
				}
				else
				{
					if(!(m_SwingDoorFlags & SWINGDOORFLAGS_NOAI))
					{	// We cannot use 0 as false because the SP manager treat index 0 as legit
						Msg.m_Param0 = -1;
						Msg.m_VecParam0 = m_DoorBottomPos;
						m_pWServer->Message_SendToObject(Msg,_Msg.m_Param1);
					}
				}
			}
			m_iActivator = _Msg.m_Param1;

			//Reset timers
			if(m_CurrentState & SWINGDOOR_STATE_CLOSED)
			{
				m_SwingTime = m_MaxSwingTime;
				m_SwingAngle = m_MaxSwingAngle;
			}

			switch(_Msg.m_Param0) 
			{
			case 1:
			    {
					CloseDoor();
			    }
				break;
			case 0:
			    {
					OpenDoorIn();
			    }
				break;
			case 2:
			    {
					OpenDoorOut();
			    }
				break;
			}

			return 1;
		}
	case OBJMSG_SWINGDOOR_HANDLE_OPEN:
		{
			m_HandleSwingTime = _Msg.m_VecParam0.k[0];
			HandleOpen();
		}
		break;
	case OBJMSG_SWINGDOOR_GETRENDERMATRIX:
		{
			if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CMat4Dfp32))
			{
				GetRenderMatrix(this,m_pWServer, _Msg.m_VecParam0.k[0], (CMat4Dfp32*)_Msg.m_pData);
				return true;
			}
			return false;
		}
	case OBJMSG_SWINGDOOR_ADDCHILD:
		{
			// Add a child
			if (_Msg.m_Param0 != 0)
			{
				CMat4Dfp32 OrgPos = m_pWServer->Object_GetPositionMatrix(_Msg.m_Param0);
				// Set us as parent for this child
				m_lChildren.Add(CChild(_Msg.m_Param0,OrgPos));
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_SWINGDOORATTACH_SETPARENT,m_iObject), _Msg.m_Param0);
			}
			return true;
		}

	case OBJMSG_SWINGDOOR:
		{
			CStr str = (char*)_Msg.m_pData;
			switch(_Msg.m_Param0)
			{
			case 0:	//open
				{
					int val = str.GetStrSep(",").Val_int();
					fp32 Time = str.GetStrSep(",").Val_fp64();
					if(Time)
						m_SwingTime = Time;
					fp32 Angle = str.GetStrSep(",").Val_fp64();
					if(Angle)
						m_SwingAngle = Angle / 360.0f;
					switch(val) 
					{
					case 1:
						CloseDoor(true);
						break;
					case 0:
						if(m_CurrentState & SWINGDOOR_STATE_CLOSED)
						{
							OpenDoorIn(true);
							m_SwingDoorFlags |= SWINGDOORFLAGS_OPENED_BY_SCRIPT;
						}
						break;
					case 2:
						if(m_CurrentState & SWINGDOOR_STATE_CLOSED)
						{
							OpenDoorOut(true);
							m_SwingDoorFlags |= SWINGDOORFLAGS_OPENED_BY_SCRIPT;
						}
						break;
					}
				}
				break;
			case 1:	//lock
				{
					bool bLock = false;
					if(str.Val_int())
						bLock = true;
					Lock(bLock);
				}
				break;
			case 2:	//change maxangle
				{
					m_MaxSwingAngle = str.Val_fp64() / 360.0f;
				}
				break;
			case 3:	//change swing time
				{
					m_MaxSwingTime = str.Val_fp64();
				}
				break;
			case 4:	//Pushed (Creeping Dark, almost same as OnPhysEvent 
				{
					if(!(m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED))
					{
						if(m_CurrentState & SWINGDOOR_STATE_CLOSED && m_iUserPortal != -1)
							m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, m_iObject), m_iUserPortal);
						m_CurrentState |= SWINGDOOR_STATE_PHYSICS;
						SetRefresh(true);
						m_LastAngle = 1.0f;
						m_LastDynamicCollisionTick = m_pWServer->GetGameTick();
						if(m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED)
						{
							m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, m_MaxSwingAngle);
							m_SwingDoorFlags &= ~SWINGDOORFLAGS_CONSTRAINT_MODDED;
						}
						if(_Msg.m_Param1 == 1) //it's Creeping dark that did this, the can't close until player stop using CD
							m_SwingDoorFlags |= SWINGDOORFLAGS_CD_OPENED_DOOR;
					}
				}
				break;
			}
			
			return true;
		}
	
	case OBJMSG_SWINGDOOR_LOCK:
		{
			bool bLock = (_Msg.m_Param0 != 0);
			Lock(bLock);
	
			return true;
		}

	case OBJMSG_OBJECT_FIND_DEMONARMATTACHPOINT:
		{
			return (aint)&m_DemonArmAttach;
		}

	case OBJMSG_OBJECT_DEMONARM_GRABBED:
		{
			if(!(m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED))
			{
				m_CurrentState |= SWINGDOOR_STATE_DEMONARM;
				SetRefresh(true);
				m_LastAngle = 1.0f;
				m_SwingDoorFlags &= ~SWINGDOORFLAGS_OPENED_BY_SCRIPT;
				//restore the constraint
				if(m_SwingDoorFlags & SWINGDOORFLAGS_CONSTRAINT_MODDED)
				{
					m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, m_MaxSwingAngle);
					m_SwingDoorFlags &= ~SWINGDOORFLAGS_CONSTRAINT_MODDED;
				}

				//open the user portal
				if(m_CurrentState & SWINGDOOR_STATE_CLOSED && m_iUserPortal != -1)
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_FUNCPORTAL_SETSTATE, 1, 0, m_iObject), m_iUserPortal);
			}
		}
		return true;

	case OBJMSG_OBJECT_DEMONARM_RELEASED:
		{
			if(!(m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED))
			{
				m_CurrentState &= ~SWINGDOOR_STATE_DEMONARM;
				m_CurrentState |= SWINGDOOR_STATE_PHYSICS;
			}
		}
		return true;

	case OBJMSG_SWINGDOOR_CANDARKLINGOPEN:
		{
			if(m_SwingDoorFlags & (SWINGDOORFLAGS_NODARKLING | SWINGDOORFLAGS_LOCKED))
				return 0;
			return 1;
		}

	//For the IK, tells if the constraint are on the left or right side on the door relative to the sender
	case OBJMSG_SWINGDOOR_GETOPENDIR:
		{
			CSwingDoorClientData* pCD = GetSwingDoorClientData(this);
			CWObject *pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			CVec3Dfp32 Dir = (pObj->GetPosition() - GetPosition()).Normalize();
			fp32 dot = Dir * pCD->m_Normal;
			if(dot < 0.0f)
				return 2;	//Left side
			else 
				return 1;	//Right side
		}
		return 1;

/*
	case OBJMSG_DAMAGE:
		{
			bool bIsPlayer = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISPLAYER), _Msg.m_iSender) != 0;
			bool bNoPlayer = (m_SwingDoorFlags & SWINGDOORFLAGS_NOPLAYER) != 0;
			bool bLocked = (m_SwingDoorFlags & SWINGDOORFLAGS_LOCKED) != 0;
			if (bIsPlayer && !bNoPlayer && !bLocked)
			{
				const CWO_DamageMsg* pMsg = CWO_DamageMsg::GetSafe(_Msg);
				if (pMsg)
				{
                    CVec3Dfp32 Dir = (pMsg->m_Position - GetPosition());
					fp32 Side = Dir * m_Normal;
					if (Side < 0.0f)
						OpenDoorIn();
					else
						OpenDoorOut();

					m_SwingDoorFlags |= SWINGDOORFLAGS_AUTOCLOSE;
				}
			}
			break; // let parent handle other damage stuff...
		}
*/
	}

	return CWObject_SwingDoorParent::OnMessage(_Msg);
}

aint CWObject_SwingDoor::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	if (_Msg.m_Msg == OBJMSG_SWINGDOOR_GETRENDERMATRIX)
	{
		if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CMat4Dfp32))
		{
			GetRenderMatrix(_pObj, _pWClient, _Msg.m_VecParam0.k[0], (CMat4Dfp32*)_Msg.m_pData);
			return true;
		}
		return false;
	}
	//For the IK, tells if the constraint are on the left or right side on the door relative to the sender
	else if(_Msg.m_Msg == OBJMSG_SWINGDOOR_GETOPENDIR)
	{
		CSwingDoorClientData* pCD = GetSwingDoorClientData(_pObj);
		CWObject_Client *pObj = _pWClient->Object_Get(_Msg.m_iSender);
		CVec3Dfp32 Dir = (pObj->GetPosition() - _pObj->GetPosition()).Normalize();
		fp32 dot = Dir * pCD->m_Normal;
		if(dot < 0.0f)
			return 2;	//Left side
		else 
			return 1;	//Right side
	}
	else
	{
		return CWObject_SwingDoorParent::OnClientMessage(_pObj, _pWClient,_Msg);
	}
}

//Lock/unlock the door, turn off physics on it, draw it to the navgrid, unspawn the SPs
//However we could be fake locking it also, in t hat case we will leave the physics on it
//We just want the AI to look on the door as if it's locked
void CWObject_SwingDoor::Lock(bool _bLock, bool _bFakeIt)
{
	if(_bLock)
	{	// Locking
		if(!(m_CurrentState & SWINGDOOR_STATE_CLOSED) && !_bFakeIt)
		{
			m_SwingDoorFlags |= SWINGDOORFLAGS_LOCK_WHEN_CLOSED;
			CloseDoor(true);
			return;
		}

		SetDrawToNavgrid(true);

		if(!_bFakeIt)
		{
			CWO_PhysicsState PhysState(GetPhysState());
			PhysState.m_PhysFlags &= ~OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			m_pWServer->Object_SetPhysics(m_iObject, PhysState);
			m_pWServer->Object_InitRigidBody(m_iObject, false);

			m_SwingDoorFlags |= SWINGDOORFLAGS_LOCKED;
			m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, 0.0f, 0.0f);
		}

		if(!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTSP))
		{
			CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());
		
			CWO_ScenePoint* pSp = pSPM->GetScenePointFromIndex(m_liDoorSPs[0]);
			if(pSp) pSp->UnSpawn(pSPM);
			pSp = pSPM->GetScenePointFromIndex(m_liDoorSPs[1]);
			if(pSp) pSp->UnSpawn(pSPM);
		}
	}
	else
	{	// Unlocking
		if (!(m_SwingDoorFlags & SWINGDOORFLAGS_BLOCKNAVIGATION))
		{
			//if(m_iModel[0] > 0)
			//	Model_SetPhys(m_iModel[0], false, iFlags, OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_ROTATION, false);

			SetDrawToNavgrid(false);
		}
		if(!_bFakeIt)
		{
			CWO_PhysicsState PhysState(GetPhysState());
			PhysState.m_PhysFlags |= OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			m_pWServer->Object_SetPhysics(m_iObject, PhysState);
			m_pWServer->Object_InitRigidBody(m_iObject, false);
			m_pWServer->Phys_SetStationary(m_iObject, false);

			m_SwingDoorFlags &= ~SWINGDOORFLAGS_LOCKED;
			m_pWServer->Phys_UpdateAxisConstraintAngles(m_ConstraintID, m_MaxSwingAngle, m_MaxSwingAngle);
		}

		if(!(m_SwingDoorFlags & SWINGDOORFLAGS_NODEFAULTSP))
		{
			CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());

			CWO_ScenePoint* pSp = pSPM->GetScenePointFromIndex(m_liDoorSPs[0]);
			if(pSp) pSp->Spawn(pSPM);
			pSp = pSPM->GetScenePointFromIndex(m_liDoorSPs[1]);
			if(pSp) pSp->Spawn(pSPM);
		}
	}
}

//Start/stop drawing the door to the nawgrid
void CWObject_SwingDoor::SetDrawToNavgrid(bool _bDrawTo)
{
	if(_bDrawTo)
	{
		m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_NAVIGATION);
	}
	else
	{
		m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags & ~OBJECT_FLAGS_NAVIGATION);
	}
}

void CWObject_SwingDoor::GetSwing(/*CWObject_CoreData* _pObj, */CMat4Dfp32& _Mat, const CMTime& _CurrentTime)
{
	_Mat.Unit();
	//CSwingDoorClientData* pCD = GetSwingDoorClientData(_pObj);
	// Depending on time
	CMTime StartTime = PHYSSTATE_TICKS_TO_TIME(m_StartTick, m_pWServer);
	fp32 Time = Clamp01((_CurrentTime - StartTime).GetTime() / m_SwingTime);
	// Do something interesting with time? (so it isn't linear)
	if (m_CurrentState & SWINGDOOR_STATE_CLOSING)
		Time = 1.0f - Time;

	fp32 CurrentAngle = 0.0f;
	if (m_CurrentState & SWINGDOOR_STATE_OPENING_IN)
		CurrentAngle = m_SwingAngle * Time;
	else
		CurrentAngle = -m_SwingAngle * Time;

	m_Axis.CreateAxisRotateMatrix(CurrentAngle,_Mat);
	CMat4Dfp32 Move,Temp;
	Move.Unit();
	(-m_Offset).SetMatrixRow(Move,3);
	Move.Multiply(_Mat,Temp);
	m_Offset.SetMatrixRow(Move,3);
	Temp.Multiply(Move,_Mat);
}

void CWObject_SwingDoor::GetRenderMatrix(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys, fp32 _IPTime, CMat4Dfp32* _pMat)
{
	Interpolate2(_pObj->GetLastPositionMatrix(),_pObj->GetPositionMatrix(),*_pMat,_IPTime);
}

void CWObject_SwingDoor::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_SwingDoor_OnClientRender, MAUTOSTRIP_VOID);

	CMat4Dfp32 Mat;
	GetRenderMatrix(_pObj, _pWClient, _pWClient->GetRenderTickFrac(),&Mat);

	CMTime Time;
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);

			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}
	}
}

void CWObject_SwingDoor::OnDeltaSave(CCFile* _pFile)
{
	CWObject_SwingDoorParent::OnDeltaSave(_pFile);

	_pFile->WriteLE(m_StartTick);
	_pFile->WriteLE(m_QueuedAction);
	_pFile->WriteLE(m_LastOpenRequest);
	_pFile->WriteLE(m_CurrentState);
	_pFile->WriteLE(m_ClientFlags);
	_pFile->WriteLE(m_SwingDoorFlags);
	_pFile->WriteLE(m_ConstraintID);
	_pFile->WriteLE(m_MaxSwingAngle);	
	_pFile->WriteLE(m_MaxSwingTime);	
	_pFile->WriteLE(m_SwingAngle);		
	_pFile->WriteLE(m_SwingTime);
	// Save position!!?!
	CMat4Dfp32 Pos = GetPositionMatrix();
	_pFile->WriteLE((int8*)&Pos,sizeof(Pos));
}

void CWObject_SwingDoor::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_SwingDoorParent::OnDeltaLoad(_pFile, _Flags);
	_pFile->ReadLE(m_StartTick);
	_pFile->ReadLE(m_QueuedAction);
	_pFile->ReadLE(m_LastOpenRequest);
	_pFile->ReadLE(m_CurrentState);
	_pFile->ReadLE(ClientFlags());
	_pFile->ReadLE(m_SwingDoorFlags);
	_pFile->ReadLE(m_ConstraintID);
	_pFile->ReadLE(m_MaxSwingAngle);	
	_pFile->ReadLE(m_MaxSwingTime);	
	_pFile->ReadLE(m_SwingAngle);		
	_pFile->ReadLE(m_SwingTime);
	// Load position!!?!
	CMat4Dfp32 Pos;
	_pFile->ReadLE((int8*)&Pos,sizeof(Pos));
	m_pWServer->Object_SetPosition(m_iObject,Pos);
}





MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SwingDoor, CWObject_SwingDoorParent, 0x0100);


int CWObject_SwingDoor::CSwingDoorClientData::OnCreateClientUpdate(uint8* _pData)
{
	// This is only done when something is changed anyway so just save the info
	uint8* pD = _pData;

	PTR_PUTUINT8(pD,m_DirtyMask);

	if (m_DirtyMask & SWINGDOOR_CLIENTDATA_DIRTYMASK_NORMAL)
	{
		PTR_PUTFP32(pD, m_Normal[0]);
		PTR_PUTFP32(pD, m_Normal[1]);
		PTR_PUTFP32(pD, m_Normal[2]);
		m_DirtyMask &= ~SWINGDOOR_CLIENTDATA_DIRTYMASK_NORMAL;
	}
	
	return (pD - _pData);
}

int CWObject_SwingDoor::CSwingDoorClientData::OnClientUpdate(const uint8* _pData)
{
	const uint8* pD = _pData;

	uint8 Mask;
	PTR_GETUINT8(pD,Mask);

	if (Mask & SWINGDOOR_CLIENTDATA_DIRTYMASK_NORMAL)
	{
		PTR_GETFP32(pD, m_Normal[0]);
		PTR_GETFP32(pD, m_Normal[1]);
		PTR_GETFP32(pD, m_Normal[2]);
	}
	
	return (pD - _pData);
}

void CWObject_SwingDoorAttach::OnCreate()
{
	CWObject_SwingDoorAttachParent::OnCreate();
	m_iParent = -1;
	m_Offset = 0.0f;
}

void CWObject_SwingDoorAttach::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH2('PARE','NT'): // "PARENT"
	case MHASH2('ATTA','CH'): // "ATTACH"
		{
			// Find parent
			m_Parent = _pKey->GetThisValue();
			break;
		}
	default:
		{
			CWObject_SwingDoorAttachParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_SwingDoorAttach::OnSpawnWorld()
{
	CWObject_SwingDoorAttachParent::OnSpawnWorld();
	
	// Set this as child with our parent
	if (m_Parent.Len() > 0)
	{
		int iParent = m_pWServer->Selection_GetSingleTarget(m_Parent);
		m_pWServer->Message_SendToObject(CWObject_Message(CWObject_SwingDoor::OBJMSG_SWINGDOOR_ADDCHILD,m_iObject),iParent);
		m_Parent.Clear();
	}
}

aint CWObject_SwingDoorAttach::OnMessage(const CWObject_Message& _Msg)
{
	if (_Msg.m_Msg == CWObject_SwingDoor::OBJMSG_SWINGDOORATTACH_SETPARENT)
	{
		// Set parent
		SetParent(_Msg.m_Param0);
		// Set offset
		SetOffset(GetPosition() - m_pWServer->Object_GetPosition(m_iParent));

		return true;
	}
	else
	{
		return CWObject_SwingDoorAttachParent::OnMessage(_Msg);
	}
}

void CWObject_SwingDoorAttach::SetParent(int32 _iParent)
{
	m_iParent = _iParent;
	Data(0) = _iParent;
}

void CWObject_SwingDoorAttach::SetOffset(const CVec3Dfp32& _Offset)
{
	m_Offset = _Offset;
	Data(1) = *(int32*)&m_Offset[0];
	Data(2) = *(int32*)&m_Offset[1];
	Data(3) = *(int32*)&m_Offset[2];
}

int32 CWObject_SwingDoorAttach::GetParent(CWObject_CoreData* _pObj)
{
	return _pObj->m_Data[0];
}

void CWObject_SwingDoorAttach::GetOffset(CWObject_CoreData* _pObj, CVec3Dfp32& _Offset)
{
	_Offset.k[0] = *(fp32*)&_pObj->m_Data[1];
	_Offset.k[1] = *(fp32*)&_pObj->m_Data[2];
	_Offset.k[2] = *(fp32*)&_pObj->m_Data[3];
}

void CWObject_SwingDoorAttach::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CMat4Dfp32 Mat;
	CWObject_SwingDoor::GetRenderMatrix(_pObj, _pWClient, _pWClient->GetRenderTickFrac(),&Mat);

	CMTime Time;
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);

			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}
	}

	/*CMat4Dfp32 ParentMat,CurrentMat,Final;
	ParentMat.Unit();
	CWObject_Message MatMsg(CWObject_SwingDoor::OBJMSG_SWINGDOOR_GETRENDERMATRIX);
	MatMsg.m_pData = &ParentMat;
	MatMsg.m_DataSize = sizeof(CMat4Dfp32);
	MatMsg.m_VecParam0.k[0] = _pWClient->GetRenderTickFrac();
	if (!_pWClient->Phys_Message_SendToObject(MatMsg,GetParent(_pObj)))
		return;

	CurrentMat = _pObj->GetPositionMatrix();
	CVec3Dfp32 Offset;
	GetOffset(_pObj,Offset);
	Offset.SetMatrixRow(CurrentMat,3);
	CurrentMat.Multiply(ParentMat,Final);



	CMTime Time;
	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);

			_pEngine->Render_AddModel(pModel, Final, AnimState);
		}
	}*/
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SwingDoorAttach, CWObject_SwingDoorAttachParent, 0x0100);
