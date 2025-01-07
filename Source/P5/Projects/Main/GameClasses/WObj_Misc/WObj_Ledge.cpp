/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:		WObj_Ledge.cpp	
					
	Author:			Olle Rosenquist
					
	Copyright:	Copyright Starbreeze AB 2002	
					
	Contents:		CWObject_Ledge implementation
					
	Comments:		
					
	History:		
		021107:		Olle Rosenquist, Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_Ledge.h"
#include "../WObj_Char.h"
#include "../../GameWorld/WClientMod_Defines.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"

#define LEDGETYPE_HEIGHT_HIGH 90.0f
#define LEDGETYPE_HEIGHT_MEDIUM 60.0f
#define LEDGETYPE_HEIGHT_LOW 44.0f
//#define LEDGE_OPTIMALDISTANCE 8.4852813742385702928101323452582
#define LEDGE_OPTIMALDISTANCE (12.0f)
#define LEDGE_OPTIMALHEIGHT (62.0f)
#define LEDGE_CORNERADJUSTDISTANCE (5.0f)

void CWObject_Ledge::OnCreate()
{
	CWObject_Ledge_Parent::OnCreate();
	CACSClientData* pCD = GetACSClientData(this);
	pCD->SetCutsceneFlags(ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS); //Don't pause all ai as default
	m_GrabDistance = WOBJ_LEDGE_DEFAULTGRABDISTANCE;
	m_LedgeFlags = 0;
//	m_GrabTimer = 0;
	//m_iLinkLeft = CWOBJECT_LEDGE_LINK_UNDEFINED;
	//m_iLinkRight = CWOBJECT_LEDGE_LINK_UNDEFINED;
	
	// New...
	m_LedgeCount = 0;
	m_iDynamicRoot = -1;
	m_LastSwitchTest = 0;
}

void CWObject_Ledge::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Ledge_OnEvalKey, MAUTOSTRIP_VOID);
	if (_pKey->GetThisName().Find("LEDGEFLAGS") != -1)
   	{
		static const char *FlagsTranslate[] =
		{
			"dynamic","","","","","","","","","","","","","","usealtclimbup",NULL
		};
  		
		m_LedgeFlags |= _pKey->GetThisValue().TranslateFlags(FlagsTranslate);
	}
	else
		CWObject_Ledge_Parent::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_Ledge::OnFinishEvalKeys()
{
	MAUTOSTRIP( CWObject_Ledge_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWObject_Ledge_Parent::OnFinishEvalKeys();

	// Always third person on ledge
	CACSClientData* pCD = GetACSClientData(this);
	pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_USETHIRDPERSON);
	// Set so that the actioncutscene won't be updated when it's not activated
	if(!(m_Flags & FLAGS_UNRELIABLE))
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	/*if (!(m_LedgeFlags & CWOBJECT_LEDGE_TYPEMASK))
		ConOut(CStrF("LEDGE<%d>: WARNING THIS LEDGE HASN'T GOT A TYPE SET!!!!",m_iObject));*/
}

bool CWObject_Ledge::DoActionSuccess(int _iCharacter)
{
	CACSClientData* pCD = GetACSClientData(this);
	MAUTOSTRIP( CWObject_Ledge_DoActionSuccess, false);
	if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
	{
		CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

		if (pCD)
		{
			// Add the camera to the action
			spCActionCutsceneCamera spCamera = GetActionCutsceneCamera(m_iObject,_iCharacter);
			// Refresh inital camera
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
spCActionCutsceneCamera CWObject_Ledge::GetActionCutsceneCamera(int _iObject, 
		int _iCharacter, int _Specific)
{
	MAUTOSTRIP( CWObject_Ledge_GetActionCutsceneCamera, spCActionCutsceneCamera());
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
				"LEDGE:BEHIND:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
				"LEDGE:BEHIND:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"LEDGE:BEHIND:CAMERAVIEW_RIGHTSIDE:CAMERAVIEW_ABOVE:70.0:70.0",
				"LEDGE:BEHIND:CAMERAVIEW_LEFTSIDE:CAMERAVIEW_ABOVE:70.0:70.0", 
			};
			iIndex = MRTC_RAND() % nCams;

			if (!spCamera->ConfigureFromString(CStr(pCameraConfigs[iIndex])))
				spCamera->MakeDefaultCamera();
		}

		spCamera->SetCharacterAndObject(_iCharacter, _iObject);
	}

	return spCamera;
}

MACRO_PHOBOSDEBUG(void CWObject_Ledge::OnRefresh()
{
	CWObject_Ledge_Parent::OnRefresh();
//	DebugDraw();
})


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Serverside message handling
						
	Parameters:			
		_Msg:			Message to handle
						
	Returns:			Depends on the message
\*____________________________________________________________________*/
aint  CWObject_Ledge::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP( CWObject_Ledge_OnMessage, CWObject_Ledge_Parent::OnMessage(_Msg));
	// Take care of assorted messages, 
	switch (_Msg.m_Msg)
	{
	case OBJMSG_LADDER_GETTYPE:
		{
			return CWOBJECT_LADDER_TYPE_LEDGE;
		}
	case OBJMSG_LEDGE_GETFLAGS:
		{
			// Return the flags
			return m_LedgeFlags;
		}
	case OBJMSG_LEDGE_ISLEDGE:
		{
			return true;
		}
	case OBJMSG_LEDGE_CANGRAB:
		{
			// Check if we can grab the ledge
			//return CanGrab(this, (CWObject_CoreData*)_Msg.m_pData, _Msg.m_Param0 != 0);
			return false;
		}
	case OBJMSG_LEDGE_GRABLEDGE:
		{
			// Check if we can grab the ledge, relative position in "x" on the first vec param
			return GrabLedge((CWObject_CoreData*)_Msg.m_pData, _Msg.m_Param0, _Msg.m_Param1, 
				_Msg.m_VecParam0.k[0]);
		}
	case OBJMSG_LADDER_ISLADDER:
		{
			return false;
		}
	case OBJMSG_LEDGE_SWITCHLEDGE:
		{
			// Switch to left or right ledge, send a grabledge message 
			CWObject_CoreData* pChar = (CWObject_CoreData*)_Msg.m_pData;
			CWO_Character_ClientData* pCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
			if (!pCD || !SwitchLedge((_Msg.m_Param0 == 0),pCD, pChar->m_iObject))
			{
				Data(LADDERENDPOINT) = 0;
				return false;
			}

			return true;

			/*int iLedge = (_Msg.m_Param0 != 0 ? m_iLinkRight : m_iLinkLeft);
			int bResult = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GRABLEDGE,
					CWOBJECT_LEDGE_ENTERTYPELEVELED,CWOBJECT_LEDGE_ENTERTYPEBELOWLEDGE,
					0,0,0,0,_Msg.m_pData), iLedge);
			
			if (bResult)
			{
				CActionCutsceneCamera Cam;
				bool bCopyCam = false;
				if (m_spActiveCutsceneCamera)
				{
					Cam.CopyFrom(*m_spActiveCutsceneCamera);
					bCopyCam = true;
				}

				// DoActionSuccess on the new ledge
				CWObject_CoreData* pChar = (CWObject_CoreData*)_Msg.m_pData;
				// End Actioncutscene on this end
				OnEndACS(pChar->m_iObject);

				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,
						0,0,pChar->m_iObject,0,0,0,_Msg.m_pData), iLedge);

				// Get camera of new ledge
				CActionCutsceneCamera* pCam = NULL;
				m_pWServer->Message_SendToObject(CWObject_Message(
					OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA,0,0,0,0,0,0,&pCam),iLedge);
				if (pCam && bCopyCam)
					pCam->CopyFrom(Cam);
			}

			return bResult;*/
		}
	case OBJMSG_ACTIONCUTSCENE_GETTYPE:
		{
			return ACTIONCUTSCENE_TYPE_LEDGE;
		}
	case OBJMSG_LEDGE_GETACTIVELEDGEINFO:
		{
			// Put pos1/2 and normal to the given data
			if ((_Msg.m_DataSize >= (sizeof(CVec3Dfp32) * 3)) && LedgeDataValid(this))
			{
				CVec3Dfp32 Pos1,Pos2,Normal;
				fp32 Length;
				int LedgeFlags;
				UnPackLedgeData(this,Pos1,Pos2,Normal,Length,LedgeFlags);
				if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
					GetDynamicLedgePos(m_pWServer,this,Pos1,Pos2,Normal);
				((CVec3Dfp32*)_Msg.m_pData)[0] = Pos1;
				((CVec3Dfp32*)_Msg.m_pData)[1] = Pos2;
				((CVec3Dfp32*)_Msg.m_pData)[2] = Normal;
				return true;
			}

			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_ONENDACS:
		OnEndACS(_Msg.m_iSender);
		// inactivate cam as well
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
	case OBJMSG_LADDER_GETCURRENTENDPOINT:
		{
			return Data(LADDERENDPOINT);
		}
	case OBJMSG_LADDER_SETCURRENTENDPOINT:
		{
			Data(LADDERENDPOINT) = _Msg.m_Param0;
			return true;
		}
	case OBJMSG_LEDGE_CANCLIMBUP:
		{
			// Test if we can climb up (physbox test above and a bit inward the ledge)
			CVec3Dfp32 LeftPoint,RightPoint,Normal,LedgeDirection;
			fp32 Length;
			int LedgeFlags;
			UnPackLedgeData(this,LeftPoint,RightPoint,Normal,Length,LedgeFlags);
			if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				GetDynamicLedgePos(m_pWServer,this,LeftPoint,RightPoint,Normal);
			LedgeDirection = (RightPoint - LeftPoint) / Length;

			CVec3Dfp32 Point = LeftPoint + LedgeDirection * ((_Msg.m_VecParam0 - LeftPoint) * LedgeDirection);

			return CanClimbUp(m_pWServer,Point,LedgeDirection,Normal,_Msg.m_Param0,_Msg.m_Param1);
			/*{
				if (LedgeFlags & CWOBJECT_LEDGE_USEALTCLIMBUP)
					return 2;
				else
					return 1;
			}
			return 0;*/
		}
	case OBJMSG_LEDGE_GETCHARLEDGEPOS:
		{
			if (!_Msg.m_pData || _Msg.m_DataSize < sizeof(CVec3Dfp32))
				return false;
			CVec3Dfp32 LeftPoint,RightPoint,Normal,LedgeDirection;
			fp32 Length;
			int LedgeFlags;
			UnPackLedgeData(this,LeftPoint,RightPoint,Normal,Length,LedgeFlags);
			if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				GetDynamicLedgePos(m_pWServer,this,LeftPoint,RightPoint,Normal);
			LedgeDirection = (RightPoint - LeftPoint) / Length;

			*((CVec3Dfp32*)_Msg.m_pData) = LeftPoint + LedgeDirection * ((_Msg.m_VecParam0 - LeftPoint) * LedgeDirection);
			return true;
		}
	// Find endpoint
	case OBJMSG_LADDER_FINDENDPOINT:
		{
			// Param0 = height, vecparam0 = charpos
			// Find endpoint from given character position
			CVec3Dfp32 LeftPoint,RightPoint,Normal,LedgeDirection,PointOnLedge;
			fp32 LedgeLength,DotLeft,DotRight,CharWidth;
			int LedgeFlags;
			UnPackLedgeData(this,LeftPoint,RightPoint,Normal,LedgeLength,LedgeFlags);
			if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				GetDynamicLedgePos(m_pWServer,this,LeftPoint,RightPoint,Normal);
			LedgeDirection = (RightPoint - LeftPoint) / LedgeLength;

			int LinkMode = _Msg.m_Param1 & LEDGE_LINKMODE_MASK;
			CharWidth = *(fp32*)&_Msg.m_Param0;

			return FindLedgePoint(LedgeDirection, _Msg.m_VecParam0,
				LeftPoint, PointOnLedge, LedgeLength, CharWidth, LinkMode, DotLeft, DotRight);
		}
	default:
		{
			return CWObject_Ledge_Parent::OnMessage(_Msg);
		}
	}
}

bool CWObject_Ledge::SwitchLedge(bool _bLeft, CWO_Character_ClientData* _pCD, int32 _iChar)
{
	// Switch to left or right ledge, send a grabledge message 
	int iLedge = (int32)_pCD->m_ControlMode_Param2;
	int iTargetLedge = (_bLeft ? m_lLedges[iLedge].GetLedgeLeft() : m_lLedges[iLedge].GetLedgeRight());
	fp32 RightLength = 0.0f;
	if (CanSwitchLedge(_bLeft,_pCD,_iChar, RightLength))
	{
		const fp32 Phys_Width = _pCD->m_Phys_Width + 2.0f;
		fp32 LedgePos = (_bLeft ? 
			m_lLedges[iTargetLedge].GetRadiusLocal() * 2.0f - (Phys_Width) :
		(Phys_Width));
		// Find ledge position and set to control mode param 1
		_pCD->m_ControlMode_Param1 = LedgePos;

		// Set ledge id to parameter 2 (when using non-dynamic ledges)
		_pCD->m_ControlMode_Param2 = (fp32)iTargetLedge;

		// Set ledge mode to control mode parameter 4
		_pCD->m_ControlMode_Param4 = CWOBJECT_LEDGE_ENTERTYPELEVELED | m_lLedges[iTargetLedge].GetLedgeFlags();

		// Put pos1/2 and normal into m_Data
		PackLedgeData(m_lLedges[iTargetLedge].GetMidpoint(), m_lLedges[iTargetLedge].GetNormal(), 
			m_lLedges[iTargetLedge].GetRadiusLocal());
		return true;
	}
	return false;
	/*MAUTOSTRIP( CWObject_Ledge_SwitchLedge, false );

	bool bResult = false;

	// Check so there is place to move around the box
	if ((iTargetLedge != -1) && (iTargetLedge < m_lLedges.Len()))
	{
		CVec3Dfp32 LedgePoint = m_lLedges[iTargetLedge].GetLeftPoint();
		CVec3Dfp32 Normal = m_lLedges[iTargetLedge].GetNormal();
		const fp32 Phys_Width = _pCD->m_Phys_Width + 2.0f;
		fp32 LedgePos = (_bLeft ? 
			m_lLedges[iTargetLedge].GetRadiusLocal() * 2.0f - (Phys_Width) :
			(Phys_Width));

		if (m_lLedges[iTargetLedge].GetLedgeFlags() & CWOBJECT_LEDGE_DYNAMIC)
		{
			CVec3Dfp32 Temp;
			GetDynamicLedgePos(m_pWServer,this,LedgePoint,Temp,Normal);
		}

		const CVec3Dfp32 LedgeDir = m_lLedges[iTargetLedge].GetLedgeDir();
		LedgePoint +=  LedgeDir * LedgePos;

		// Use a bounding box test below the wanted ledgepoint and determine ledgetype from there...
		fp32 HalfWidth = (fp32)_pCD->m_Phys_Width;
		fp32 WidthOffset = (12.0f + 0.01f);
		WidthOffset *= WidthOffset * 2;
		WidthOffset = M_Sqrt(WidthOffset);

		LedgePoint = LedgePoint - CVec3Dfp32(0,0,LEDGETYPE_HEIGHT_MEDIUM*0.5f) + 
			Normal * WidthOffset;

		// First test medium ledge, if that collides it's a low ledge, if that doesn't collide but high does
		// it's a medium ledge, if that doesn't collide it's a high ledge..
		TSelection<CSelection::LARGE_BUFFER> Selection;

		CWO_PhysicsState PhysState;
		PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
				CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f),0);
		PhysState.m_nPrim = 1;
		PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
		PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
		//PhysState.m_PhysFlags = 0;
		// Intersect doors and doors/platforms
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
		PhysState.m_iExclude = _iChar;

		m_pWServer->Selection_AddIntersection(Selection, LedgePoint, PhysState);

		CBox3Dfp32 SomeBox;
		SomeBox.m_Min = LedgePoint - CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f);
		SomeBox.m_Max = LedgePoint + CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f);

#ifndef M_RTM
		m_pWServer->Debug_RenderAABB(SomeBox,0xffff0000,10.0f,false);
		m_pWServer->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
		m_pWServer->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
#endif

		const int16* pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);

		// If we are colliding with something it's a low ledge, otherwise check if it's a medium or a
		// high ledge
		if (nSel == 0)
		{
			
		}

	}

	return bResult;*/
}

bool CWObject_Ledge::CanSwitchLedge(bool _bLeft, CWO_Character_ClientData* _pCD, int32 _iChar, fp32& _RightLength)
{
	MAUTOSTRIP( CWObject_Ledge_SwitchLedge, false );
	if (_pCD->m_GameTick <= m_LastSwitchTest)
		return false;
	// Switch to left or right ledge, send a grabledge message 
	int iLedge = (int32)_pCD->m_ControlMode_Param2;
	int iTargetLedge = (_bLeft ? m_lLedges[iLedge].GetLedgeLeft() : m_lLedges[iLedge].GetLedgeRight());

	bool bResult = false;

	// Check so there is place to move around the box
	if ((iTargetLedge >= 0) && (iTargetLedge < m_lLedges.Len()))
	{
		CVec3Dfp32 LedgePoint = m_lLedges[iTargetLedge].GetLeftPoint();
		CVec3Dfp32 Normal = m_lLedges[iTargetLedge].GetNormal();
		const fp32 Phys_Width = _pCD->m_Phys_Width + 2.0f;
		fp32 LedgePos = (_bLeft ? 
			m_lLedges[iTargetLedge].GetRadiusLocal() * 2.0f - (Phys_Width) :
		(Phys_Width));

		if (m_lLedges[iTargetLedge].GetLedgeFlags() & CWOBJECT_LEDGE_DYNAMIC)
		{
			CVec3Dfp32 Temp;
			GetDynamicLedgePos(m_pWServer,this,LedgePoint,Temp,Normal);
		}

		const CVec3Dfp32 LedgeDir = m_lLedges[iTargetLedge].GetLedgeDir();
		LedgePoint +=  LedgeDir * LedgePos;

		if (!_bLeft)
			_RightLength = m_lLedges[iTargetLedge].GetRadiusLocal() * 2.0f;

		// Use a bounding box test below the wanted ledgepoint and determine ledgetype from there...
		fp32 HalfWidth = (fp32)_pCD->m_Phys_Width;
		fp32 WidthOffset = (12.0f + 0.01f);
		WidthOffset *= WidthOffset * 2;
		WidthOffset = M_Sqrt(WidthOffset);

		LedgePoint = LedgePoint - CVec3Dfp32(0,0,LEDGETYPE_HEIGHT_MEDIUM*0.5f) + 
			Normal * WidthOffset;

		// First test medium ledge, if that collides it's a low ledge, if that doesn't collide but high does
		// it's a medium ledge, if that doesn't collide it's a high ledge..
		TSelection<CSelection::LARGE_BUFFER> Selection;

		CWO_PhysicsState PhysState;
		/*	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, 
		CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f), 0));*/
		PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f),0);
		PhysState.m_nPrim = 1;
		PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
		PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
		//PhysState.m_PhysFlags = 0;
		// Intersect doors and doors/platforms
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL |OBJECT_FLAGS_PLAYERPHYSMODEL;
		PhysState.m_iExclude = _iChar;

		m_pWServer->Selection_AddIntersection(Selection, LedgePoint, PhysState);

		CBox3Dfp32 SomeBox;
		SomeBox.m_Min = LedgePoint - CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f);
		SomeBox.m_Max = LedgePoint + CVec3Dfp32(HalfWidth, HalfWidth, LEDGETYPE_HEIGHT_MEDIUM*0.5f);

#ifndef M_RTM
		m_pWServer->Debug_RenderAABB(SomeBox,0xffff0000,10.0f,false);
		m_pWServer->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
		m_pWServer->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
#endif

		const int16* pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);

		// If we are colliding with something it's a low ledge, otherwise check if it's a medium or a
		// high ledge
		if (nSel == 0)
			bResult = true;

	}

	return bResult;
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
bool CWObject_Ledge::OnEndACS(int _iCharacter)
{
	MAUTOSTRIP( CWObject_Ledge_OnEndACS, true );
	// Do baseclass
	CWObject_ActionCutscene::OnEndACS(_iCharacter);

	// Disable further updates of this object since it won't be updated anymore
	if(!(m_Flags & FLAGS_UNRELIABLE))
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	// Invalidate ledge data so GetUserAcc will be correct next time
	InvalidateLedgeData();

	// Put back cached phys width
	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (pCD)
	{
		pCD->m_Phys_Width = pCD->m_LedgePhysWidthCached;
		// Resetphys?
		int32 PhysType = CWObject_Character::Char_GetPhysType(pObj);
		CWObject_Character::Char_SetPhysics(pObj, m_pWServer, m_pWServer, 
			(PhysType != PLAYER_PHYS_DEAD ? (pCD->m_Control_Press & CONTROLBITS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND : PLAYER_PHYS_DEAD),false,true);
		
		// Invalidata ledge data
		// Set ledge id and flags to control mode parameter 0
		pCD->m_ControlMode_Param0 = 0;
		// Set ledge relative position in parameter 1
		pCD->m_ControlMode_Param1 = 0.0f;
		// Set ledge id to parameter 2 (when using non-dynamic ledges)
		pCD->m_ControlMode_Param2 = 0.0f;
		// Set ledge mode to control mode parameter 4
		pCD->m_ControlMode_Param4 = 0;
	}

	return true;
}
enum
{
	LEDGEMOVE_IDLE = 0,
	LEDGEMOVE_LEFT,
	LEDGEMOVE_RIGHT,
	LEDGEMOVE_UPLOW,
	LEDGEMOVE_UPMEDIUM,
	LEDGEMOVE_JUMPUPANDGRAB,
	LEDGEMOVE_CLIMBUPFROMHANGING,
	LEDGEMOVE_ALTCLIMBUPFROMHANGING,
	LEDGEMOVE_CLIMBDOWNTOLEDGE,
	LEDGEMOVE_ALTCLIMBDOWNTOLEDGE,
	LEDGEMOVE_DROPDOWN,
	LEDGEMOVE_CLIMBCORNERINLEFT,
	LEDGEMOVE_CLIMBCORNEROUTLEFT,
	LEDGEMOVE_CLIMBCORNERINRIGHT,
	LEDGEMOVE_CLIMBCORNEROUTRIGHT,
};
// Set ladder object id at param3
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Initializes the character for ledge movement
						
	Parameters:			
		_pChar:			Character object
		_LedgeMode:		Which ledge mode to use
						
	Returns:			Whether ledge mode could be entered or not
						(needs an anim manager that supports GOTOLEDGE
						 signal)
\*____________________________________________________________________*/
bool CWObject_Ledge::GrabLedge(CWObject_CoreData* _pChar, int _LedgeMode, int _LedgeID, 
							   fp32 _LedgePos)
{
	MAUTOSTRIP( CWObject_Ledge_GrabLedge, false );
	CWO_Character_ClientData* pCD = (_pChar ? CWObject_Character::GetClientData(_pChar) : NULL);
	if (!pCD || (_LedgeID == -1) || _LedgeID >= m_lLedges.Len())
		return false;

	//ConOut("Grabbing ledge");
//	m_GrabTimer = CWOBJECT_LEDGE_GRABTIMER;
	
	// Set ledge2 control mode, goes to ledge control mode when we're on the correct ledge 
	// position
	CWObject_Character::Char_SetControlMode(_pChar, PLAYER_CONTROLMODE_LEDGE2);
	//CWObject_Character::Char_SetControlMode(_pChar, PLAYER_CONTROLMODE_ANIMATION);
	((CWObject_Character *)_pChar)->UpdateVisibilityFlag();

	// Set cached phys width
	pCD->m_LedgePhysWidthCached = pCD->m_Phys_Width;
	// Set physbox width to the defined width (problems with animations in corners otherwise)
	pCD->m_Phys_Width = LEDGE_PHYS_WIDTH;
	// Needed if we're going to set position later down 
	CWObject_Character::Char_SetPhysics(_pChar, m_pWServer, m_pWServer, CWObject_Character::Char_GetPhysType(_pChar),false,true);

	// Set start position
	if (_LedgeMode & (CWOBJECT_LEDGE_ENTERTYPEBELOWLEDGE | CWOBJECT_LEDGE_ENTERTYPEABOVELEDGE | 
		CWOBJECT_LEDGE_ENTERTYPELEVELED))
	{
		/*Uppifrån:            9.70121
		Nedifrån hög:        29.253791
		Nedifrån medium:     17.888356
		Nedifrån låg:        18.000003*/
		// Must transform normals and stuff according to current dynamic ledgepos thingy!!!
		// FIXME
		CVec3Dfp32 LedgeNormal = m_lLedges[_LedgeID].GetNormal();
		CVec3Dfp32 LedgePos1 = m_lLedges[_LedgeID].GetLeftPoint();
		CVec3Dfp32 LedgeDir = m_lLedges[_LedgeID].GetLedgeDir();		
		if (m_lLedges[_LedgeID].GetLedgeFlags() & CWOBJECT_LEDGE_DYNAMIC)
		{
			CVec3Dfp32 Temp;
			GetDynamicLedgePos(m_pWServer,this,LedgePos1,Temp,LedgeNormal);
			/*CMat4Dfp32 Mat;
			int iParent = m_pWServer->Object_GetParent(m_iObject);
			if (iParent != 0)
			{
				CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,int(&Mat));
				m_pWServer->Phys_Message_SendToObject(RenderMat,iParent);
				// Parent predicted position + ledge local position -> real position
				CVec3Dfp32 LedgePos = GetLocalPosition() + CVec3Dfp32::GetMatrixRow(Mat,3);

				// If the ledge is dynamic make the positions "unrelative"
				LedgePos1 +=  LedgePos;
			}*/
		}

		CMat4Dfp32 CharMat = _pChar->GetLocalPositionMatrix();
		CVec3Dfp32 CharDir = -LedgeNormal;
		
		CVec3Dfp32 TargetPos = LedgePos1 + LedgeDir * _LedgePos;
		int16 MoveType = 0;
		if (_LedgeMode & CWOBJECT_LEDGE_ENTERTYPEABOVELEDGE)
		{
			TargetPos -= LedgeNormal * 9.70121f;
			CharDir = -LedgeNormal;
			MoveType = LEDGEMOVE_CLIMBDOWNTOLEDGE;
		}
		else if (_LedgeMode & CWOBJECT_LEDGE_TYPEHIGH)
		{
			TargetPos += LedgeNormal * 12.0f;//29.253791f;
			TargetPos.k[2] -= 62.0f;
			MoveType = LEDGEMOVE_JUMPUPANDGRAB;
		}
		else if (_LedgeMode & CWOBJECT_LEDGE_TYPEMEDIUM)
		{
			TargetPos += LedgeNormal * 12.0f;// 17.888356f;
			TargetPos.k[2] -= 64.0f;
			MoveType = LEDGEMOVE_UPMEDIUM;
		}
		else if (_LedgeMode & CWOBJECT_LEDGE_TYPELOW)
		{
			TargetPos += LedgeNormal * 12.0f;//18.000003f;
			//CharPos.k[2] += 16.0f;
			TargetPos.k[2] -= 48.0f;
			MoveType = LEDGEMOVE_UPLOW;
		}

		CVec3Dfp32 Left;
		CharDir.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Left);
		Left = -Left;
		
		//ConOut(CStrF("SETTING POS: %s", CharPos.GetString().GetStr()));
		CharDir.SetMatrixRow(CharMat,0);
		Left.SetMatrixRow(CharMat,1);
		CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(CharMat,2);
		TargetPos.SetMatrixRow(CharMat,3);
		CharMat.RecreateMatrix(0,2);
		//m_pWServer->Object_SetPosition(_pChar->m_iObject,CharMat);
		pCD->m_AnimGraph2.SetDestination(CharMat);
		CACSClientData* pACSCD = GetACSClientData(this);
		pACSCD->SetCachedMat(CharMat);
		m_pWServer->Debug_RenderMatrix(CharMat,20.0f,false);
		CWAG2I_Context AGContext(_pChar,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()));
		CWAG2I_Token* pToken = pCD->m_AnimGraph2.GetAG2I()->GetTokenFromID(0);
		if (pToken)
		{
			
			CAG2GraphBlockIndex iGB = pToken->GetGraphBlock();
			CXRAG2* pAG = pCD->m_AnimGraph2.GetAG2I()->GetAnimGraph(pToken->GetAnimGraphIndex());
			if (!pAG)
				return false;
			const CXRAG2_GraphBlock* pBlock = pAG->GetGraphBlock(iGB);
			if (!pBlock)
				return false;
			if (!(pBlock->m_Condition.m_ImpulseType == AG2_IMPULSETYPE_LEDGE))
			{
				// Force ourselves to the ledge
				CAG2GraphBlockIndex iBlock = pAG->GetMatchingGraphBlock(CXRAG2_Impulse(AG2_IMPULSETYPE_LEDGE,0));
				if (iBlock != -1)
				{
					CAG2ReactionIndex iReaction = pAG->GetMatchingReaction(iBlock,CXRAG2_Impulse(AG2_IMPULSETYPE_LEDGE,MoveType));
					const CXRAG2_Reaction* pReaction = pAG->GetReaction(iReaction);
					if (!pReaction)
						return false;
					pCD->m_AnimGraph2.GetAG2I()->InvokeEffects(&AGContext,pToken->GetAnimGraphIndex(), pReaction->GetBaseEffectInstanceIndex(), pReaction->GetNumEffectInstances());
					pCD->m_AnimGraph2.GetAG2I()->MoveGraphBlock(&AGContext,0,0,pReaction->GetBaseMoveTokenIndex());
				}
				else
				{
					// Just send move impulse
					pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext,CXRAG2_Impulse(AG2_IMPULSETYPE_LEDGE,MoveType),0);
				}
			}
		}

		// Only set rotation
		m_pWServer->Object_SetRotation(_pChar->m_iObject,CharMat);
	}

	// Zero velocity, which apparently doesn't help at all...
	CVelocityfp32 Vel;
	Vel.m_Move = 0.0f;
	Vel.m_Rot.Unit();
	m_pWServer->Object_SetVelocity(_pChar->m_iObject, Vel);

	// Set ledge id and flags to control mode parameter 0
	pCD->m_ControlMode_Param0 = (fp32)m_iObject;
	// Set ledge relative position in parameter 1
	pCD->m_ControlMode_Param1 = _LedgePos;
	// Set ledge id to parameter 2 (when using non-dynamic ledges)
	pCD->m_ControlMode_Param2 = (fp32)_LedgeID;
	// Set ledge mode to control mode parameter 4
	pCD->m_ControlMode_Param4 = _LedgeMode | m_lLedges[_LedgeID].GetLedgeFlags();

	// Put pos1/2 and normal into m_Data
	PackLedgeData(m_lLedges[_LedgeID].GetMidpoint(), m_lLedges[_LedgeID].GetNormal(), 
		m_lLedges[_LedgeID].GetRadiusLocal());

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Initializes object physics and finds linked 
						ledge id's
						
\*____________________________________________________________________*/
void CWObject_Ledge::OnSpawnWorld()
{
	MAUTOSTRIP( CWObject_Ledge_OnSpawnWorld, MAUTOSTRIP_VOID );
	// Bypass ledge, (should inherit from acs anyway...)
	CWObject_Ledge_Parent::OnSpawnWorld();
	// Set physics so it won't be selected
	CWO_PhysicsState Phys;
	Phys.m_ObjectFlags = OBJECT_FLAGS_WORLD;
	Phys.m_PhysFlags = OBJECT_FLAGS_WORLD;//| OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	Phys.m_iExclude = ~0;//_iExclude == -1 ? m_iOwner : _iExclude;
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	CWO_PosHistory* pKeyFrames = GetClientData(this);
	if(m_iAnim0 != -1 && pKeyFrames && !pKeyFrames->IsValid())
		LoadPath(m_pWServer, this, m_iAnim0);

	int NumKeys = pKeyFrames->m_lSequences.Len() ? pKeyFrames->m_lSequences[0].GetNumKeyframes() : 0;

	// Need atleast 2 positions to make a ledge
	if (NumKeys > 1)
	{
		// New version for testing
		CWObject_Ledge* pLedge = NULL;
		if (m_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
		{
			pLedge = this;
			m_lLedges.SetLen(NumKeys - 1);
		}
		else
		{
			pLedge = FindMainLedge(m_pWServer);
			// If no main ledge was found, make this ledge the main ledge
			if (!pLedge)
			{
				pLedge = this;
				m_LedgeFlags |= CWOBJECT_LEDGE_ISMAINLEDGE;
				// Add linkinfinite flag, otherwise the ledge might not get updated to client
				ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
				int ArrayLen = CalculateLedgeArrayLength();
				if (ArrayLen > 0)
					m_lLedges.SetLen(ArrayLen);
				
				// Create default bintree
				m_lLedgeBinTree.SetLen(1);
				m_lLedgeBinTree[0].Reset();
			}
		}
	
		TThinArray<CLedge>& lLedges = pLedge->m_lLedges;
		int16& LedgeCount = pLedge->m_LedgeCount;
		int iThisLedgeStart = LedgeCount;
		int i = 0;
		// Length calculated when main ledge is created
		//		lLedges.SetLen(LedgeCount + NumKeys - 1);
		
		while (i < (NumKeys - 1))
		{
			// Read the next two positions
			CMat4Dfp32 Position;
			CVec3Dfp32 LeftPoint, RightPoint, LedgeDir;
			fp32 LedgeLength;
			pKeyFrames->m_lSequences[0].GetMatrix(i++,Position);
			RightPoint = CVec3Dfp32::GetMatrixRow(Position,3);
			// Use this position as position 2 for next ledge (don't increase i...)
			pKeyFrames->m_lSequences[0].GetMatrix(i,Position);
			LeftPoint = CVec3Dfp32::GetMatrixRow(Position,3);
			LedgeDir = RightPoint - LeftPoint;
			LedgeLength = LedgeDir.Length();
			LedgeDir = LedgeDir / LedgeLength;

			lLedges[LedgeCount].GetMidpoint() = (RightPoint + LeftPoint) * 0.5f;
			// If dynamic ledge make positions relative
			if (m_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				lLedges[LedgeCount].GetMidpoint() -= GetPosition();

			lLedges[LedgeCount].GetLedgeDir() = LedgeDir;
			lLedges[LedgeCount].GetRadiusLocal() = LedgeLength * 0.5f;
			
			// Find normal
			/*lLedges[LedgeCount].GetNormal() = FindNormal(lLedges[LedgeCount].GetLeftPoint(),
				lLedges[LedgeCount].GetRightPoint());*/

			// Set flags
			lLedges[LedgeCount].GetLedgeFlags() = m_LedgeFlags;

			// Link with previous
			if (LedgeCount > iThisLedgeStart)
			{
				lLedges[LedgeCount - 1].GetLedgeLeft() = LedgeCount;
				lLedges[LedgeCount].GetLedgeRight() = LedgeCount - 1;

				// Find linkmodes...
				lLedges[LedgeCount - 1].GetLedgeFlags() |= FindLinkMode((lLedges[LedgeCount-1].GetRightPoint() - lLedges[LedgeCount-1].GetLeftPoint()).Normalize(),
					lLedges[LedgeCount].GetNormal(), true);
				lLedges[LedgeCount].GetLedgeFlags() |= FindLinkMode((lLedges[LedgeCount].GetRightPoint() - lLedges[LedgeCount].GetLeftPoint()).Normalize(),
					lLedges[LedgeCount - 1].GetNormal(), false);
			}
			LedgeCount++;
		}

		// Create first<->last connection ledge if they share a position
		if (((LedgeCount - 1 - iThisLedgeStart) > 1) && 
			(lLedges[iThisLedgeStart].GetRightPoint() == lLedges[LedgeCount - 1].GetLeftPoint()))//&& (m_LedgeFlags & CWOBJECT_LEDGE_LOOP))
		{
			int LedgeCountLoop = LedgeCount - 1;
			
			// Link ledges
			lLedges[iThisLedgeStart].GetLedgeRight() = LedgeCountLoop;
			lLedges[LedgeCountLoop - 1].GetLedgeLeft() = LedgeCountLoop;
			lLedges[LedgeCountLoop].GetLedgeRight() = LedgeCountLoop - 1;
			lLedges[LedgeCountLoop].GetLedgeLeft() = iThisLedgeStart;

			// Find linkmodes...
			lLedges[iThisLedgeStart].GetLedgeFlags() |= FindLinkMode((lLedges[iThisLedgeStart].GetRightPoint() - lLedges[iThisLedgeStart].GetLeftPoint()).Normalize(),
				lLedges[LedgeCountLoop].GetNormal(), false);
			lLedges[LedgeCountLoop - 1].GetLedgeFlags() |= FindLinkMode((lLedges[LedgeCountLoop - 1].GetRightPoint() - lLedges[LedgeCountLoop - 1].GetLeftPoint()).Normalize(),
				lLedges[LedgeCountLoop].GetNormal(), true);
			lLedges[LedgeCountLoop].GetLedgeFlags() |= FindLinkMode((lLedges[LedgeCountLoop].GetRightPoint() - lLedges[LedgeCountLoop].GetLeftPoint()).Normalize(),
				lLedges[LedgeCountLoop - 1].GetNormal(), false);
			lLedges[LedgeCountLoop].GetLedgeFlags() |= FindLinkMode((lLedges[LedgeCountLoop].GetRightPoint() - lLedges[LedgeCountLoop].GetLeftPoint()).Normalize(),
				lLedges[iThisLedgeStart].GetNormal(), true);
		}

		// Ok, build the bintree for these ledges..
		int iRoot = BuildBinCollTree(lLedges,iThisLedgeStart, LedgeCount - 1);

		// Ah crap, now we must insert the tree into the root of the main tree
		if (m_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
			m_iDynamicRoot = iRoot;
		else
			InsertNode(lLedges, pLedge->m_lLedgeBinTree,0, iRoot);
	}

	// Debug test
	//m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;

	// Invalidate ledge data so GetUserAcc will be correct next time
	InvalidateLedgeData();
	if (!(m_LedgeFlags & (CWOBJECT_LEDGE_DYNAMIC | CWOBJECT_LEDGE_ISMAINLEDGE)))
	{
		// Delete this ledge since it's not the main one or dynamic
		m_pWServer->Object_DestroyInstant(m_iObject);
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Handles client messages
						
	Parameters:		
		_pObj:			Ledge object
		_pWClient:		World client
		_Msg:			Message to handle
			
	Returns:		1 if the message was handled, otherwise 0 or 
					whatever the base class message handler returns
						
	Comments:		
\*____________________________________________________________________*/
aint CWObject_Ledge::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MAUTOSTRIP( CWObject_Ledge_OnClientMessage, CWObject_Ledge_Parent::OnClientMessage(_pObj, _pWClient, _Msg) );
/*	if (_pObj == NULL)
	{
		ConOutL("CWObject_Ledge::OnClientMessage() - Invalid object pointer (_pObj == NULL).");
		return 0;
	}*/

	switch (_Msg.m_Msg)
	{
	case OBJMSG_LEDGE_CANGRAB:
		{
			// Check if we can grab the ledge
			ConOut("Error: CAN'T TEST 'CANGRAB' ON THE CLIENT!!!");
			break;
		}
	case OBJMSG_LADDER_GETTYPE:
		{
			return CWOBJECT_LADDER_TYPE_LEDGE;
		}
	case OBJMSG_LADDER_ISLADDER:
		{
			return false;
		}
	case OBJMSG_LEDGE_GETACTIVELEDGEINFO:
		{
			// Put pos1/2 and normal to the given data
			if ((_Msg.m_DataSize >= (sizeof(CVec3Dfp32) * 3)) && LedgeDataValid(_pObj))
			{
				CVec3Dfp32 Pos1,Pos2,Normal;
				fp32 Length;
				int LedgeFlags;
				UnPackLedgeData(_pObj,Pos1,Pos2,Normal,Length,LedgeFlags);
				if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
					GetDynamicLedgePos(_pWClient,_pObj,Pos1,Pos2,Normal);
				((CVec3Dfp32*)_Msg.m_pData)[0] = Pos1;
				((CVec3Dfp32*)_Msg.m_pData)[1] = Pos2;
				((CVec3Dfp32*)_Msg.m_pData)[2] = Normal;
				return true;
			}

			return false;
		}
	case OBJMSG_LADDER_GETCURRENTENDPOINT:
		{
			return _pObj->m_Data[LADDERENDPOINT];
		}
	case OBJMSG_LADDER_FINDENDPOINT:
		{
			// Param0 = height, vecparam0 = charpos
			// Find endpoint from given character position
			CVec3Dfp32 LeftPoint,RightPoint,Normal,LedgeDirection,PointOnLedge;
			fp32 LedgeLength,DotLeft,DotRight,CharWidth;
			int LedgeFlags;
			UnPackLedgeData(_pObj,LeftPoint,RightPoint,Normal,LedgeLength,LedgeFlags);
			if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				GetDynamicLedgePos(_pWClient,_pObj,LeftPoint,RightPoint,Normal);
			LedgeDirection = (RightPoint - LeftPoint) / LedgeLength;

			int LinkMode = _Msg.m_Param1 & LEDGE_LINKMODE_MASK;
			CharWidth = *(fp32*)&_Msg.m_Param0;

			return FindLedgePoint(LedgeDirection, _Msg.m_VecParam0,
				LeftPoint, PointOnLedge, LedgeLength, CharWidth, LinkMode, DotLeft, DotRight);
		}
	case OBJMSG_LEDGE_GETCHARLEDGEPOS:
		{
			if (!_Msg.m_pData || _Msg.m_DataSize < sizeof(CVec3Dfp32))
				return false;
			CVec3Dfp32 LeftPoint,RightPoint,Normal,LedgeDirection;
			fp32 Length;
			int LedgeFlags;
			UnPackLedgeData(_pObj,LeftPoint,RightPoint,Normal,Length,LedgeFlags);
			if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				GetDynamicLedgePos(_pWClient,_pObj,LeftPoint,RightPoint,Normal);
			LedgeDirection = (RightPoint - LeftPoint) / Length;

			*((CVec3Dfp32*)_Msg.m_pData) = LeftPoint + LedgeDirection * ((_Msg.m_VecParam0 - LeftPoint) * LedgeDirection);
			return true;
		}
	default:
		break;
	}

	return CWObject_Ledge_Parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

#define LEDGE_MAXGRABDISTANCEGROUND (100.0f)

/*LEDGE_ENDPOINT_NOENDPOINT = 0,
	LEDGE_ENDPOINT_INLEFT = 3,
	LEDGE_ENDPOINT_OUTLEFT = 4,
	LEDGE_ENDPOINT_INRIGHT = 5,
	LEDGE_ENDPOINT_OUTRIGHT = 6,*/
// Finds where we are on the ladder (middle/bottom/top)
#define UGLYLEDGEADJUST 2.0f

int CWObject_Ledge::FindLedgePoint(const CVec3Dfp32& _LedgeDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLedge, const fp32& _LedgeLength, 
		const fp32& _CharWidth, const int& _LinkMode, fp32& _LedgeDotLeft, fp32& _LedgeDotRight)
{
	MAUTOSTRIP( CWObject_Ledge_FindLedgePoint, LEDGE_ENDPOINT_NOENDPOINT );
	// Check if we are outside the ledge, in that case set appropriate mode
	_PointOnLedge = _Pos1 + _LedgeDir * ((_ObjPos - _Pos1) * _LedgeDir);
	CVec3Dfp32 PointOnLedge = _PointOnLedge - _LedgeDir * (_CharWidth + 0.1f);
	// Check if point is outside bounds (pos2<->pos1)
	_LedgeDotLeft = (PointOnLedge - _Pos1) * _LedgeDir;
	if (_LedgeDotLeft <= 0.0f)
	{
		//ConOut("LEDGEPOINT_LEFT");
		if (_LinkMode & LEDGE_LINKMODE_LEFTMASK)
			return (_LinkMode & LEDGE_LINKMODE_LEFTIN ? LEDGE_ENDPOINT_INLEFT : LEDGE_ENDPOINT_OUTLEFT);
		else
		{
			//ConOut("Endpoint left");
			return LEDGE_ENDPOINT_LEFT;
		}
	}
	
	PointOnLedge = _PointOnLedge + _LedgeDir * (_CharWidth + 0.1f);
	_LedgeDotRight = (PointOnLedge - _Pos1) * _LedgeDir;
	if (_LedgeDotRight >= _LedgeLength)
	{
		//ConOut("LEDGEPOINT_RIGHT");
		if (_LinkMode & LEDGE_LINKMODE_RIGHTMASK)
			return (_LinkMode & LEDGE_LINKMODE_RIGHTIN ? LEDGE_ENDPOINT_INRIGHT : LEDGE_ENDPOINT_OUTRIGHT);
		else
		{
			//ConOut("Endpoint right");
			return LEDGE_ENDPOINT_RIGHT;
		}
	}

	//ConOut("LADDERPOINT_NONE");
	return LEDGE_ENDPOINT_NOENDPOINT;
}

bool CWObject_Ledge::GetDynamicLedgePos(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pLedge, 
										CVec3Dfp32& _Pos1, CVec3Dfp32& _Pos2, CVec3Dfp32& _Normal)
{
	MAUTOSTRIP( CWObject_Ledge_GetDynamicLedgePos, false );

	CMat4Dfp32 Mat;// = _pLedge->GetPositionMatrix();
	Mat.Unit();
	CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&Mat));
	_pWPhys->Phys_Message_SendToObject(RenderMat,_pLedge->m_iObject);
	// Parent predicted position + ledge local position -> real position
	//CVec3Dfp32 LedgePos = _pLedge->GetLocalPosition() + CVec3Dfp32::GetMatrixRow(Mat,3);
	//CVec3Dfp32 LocalPos(64.0f,68.0f,16.0f);
	//CVec3Dfp32 LocalPos;
	/*CVec3Dfp32 ParentPos = _pWPhys->Object_GetPosition(iParent);
	if (_pWPhys->IsClient())
		LocalPos = _pLedge->GetPosition() - ParentPos;//_pWPhys->Object_GetPosition(iParent);
	else*/
	//LocalPos = _pLedge->GetLocalPosition();

//	CVec3Dfp32 LedgePos = /*LocalPos */ CVec3Dfp32::GetMatrixRow(Mat,3);
	//ConOut(CStrF("%s: Mat: %s Pos: %s", (_pPhysState->IsServer() ? "S":"C"),Mat.GetString().GetStr(), pObjLedge->GetLocalPosition().GetString().GetStr()));
	//MatNorm.InverseOrthogonal(InvMat);
	CVec3Dfp32 Offset = CVec3Dfp32::GetMatrixRow(Mat,3);

	// If the ledge is dynamic make the positions "unrelative"
	//_Pos1 = _Pos1 * MatNorm;
	//_Pos2 = _Pos2 * MatNorm;
	_Pos1 += Offset;
	_Pos2 += Offset;
	// Skip this for now
	//_Normal = _Normal * MatNorm;
	return true;
}

void CWObject_Ledge::OnPress(const CWAG2I_Context* _pContext, CWO_Character_ClientData* _pCD, int _ControlPress, int _ControlLastPress)
{
	// No move if already moving
	if (_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BLOCKACTIVE)
		return;

	bool bIsPressLeft = ((_ControlPress & CONTROLBITS_DPAD_LEFT) && !(_ControlLastPress & CONTROLBITS_DPAD_LEFT));
	bool bIsPressRight = ((_ControlPress & CONTROLBITS_DPAD_RIGHT) && !(_ControlLastPress & CONTROLBITS_DPAD_RIGHT));
	bool bIsPressDown = ((_ControlPress & CONTROLBITS_DPAD_DOWN) && !(_ControlLastPress & CONTROLBITS_DPAD_DOWN));
	bool bIsPressUp = ((_ControlPress & CONTROLBITS_DPAD_UP) && !(_ControlLastPress & CONTROLBITS_DPAD_UP));
	int Val = (_pCD->m_AnimGraph2.GetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL) > 0.5f) ? _pCD->m_AnimGraph2.Property_FixedMoveAngleControl4(_pContext).GetInt() : 0;
	if (bIsPressUp || (Val == 1))
	{
		MakeLedgeMove(_pContext,LEDGEMOVE_CLIMBUPFROMHANGING);
		return;
	}
	if (bIsPressDown || (Val == 5))
	{
		MakeLedgeMove(_pContext,LEDGEMOVE_DROPDOWN);
		return;
	}
	if (bIsPressLeft || (Val == 3))
	{

		MakeLedgeMove(_pContext,LEDGEMOVE_LEFT);
		return;
	}
	if (bIsPressRight || (Val == 7))
	{
		MakeLedgeMove(_pContext,LEDGEMOVE_RIGHT);
		return;
	}
}

#define LEDGE_CORNER_DISTANCE (13.0f)
bool CWObject_Ledge::MakeLedgeMove(const CWAG2I_Context* _pContext, int32 _Move)
{
	// Ok then, we're making a ledge move
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (!pCD)
		return false;
	int iLedge = (int)pCD->m_ControlMode_Param0;
	CWObject_CoreData* pObjLedge = _pContext->m_pWPhysState->Object_GetCD(iLedge);
	if (!pObjLedge)
		return false;

	CVec3Dfp32 LeftPoint,RightPoint,Normal,CharPos,LedgeDir;
	fp32 CurrentPos,LedgeLength;
	int LedgeFlags;

	UnPackLedgeData(pObjLedge, LeftPoint, RightPoint, Normal,LedgeLength,LedgeFlags);
	if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
		GetDynamicLedgePos(_pContext->m_pWPhysState,pObjLedge,LeftPoint,RightPoint,Normal);

	LedgeDir = (LeftPoint - RightPoint) / LedgeLength;
	CharPos = _pContext->m_pObj->GetPosition();
	CVec3Dfp32 PointOnLedge = RightPoint + LedgeDir * ((CharPos - RightPoint) * LedgeDir);
	// Check if point is outside bounds (pos2<->pos1)
	CurrentPos = (PointOnLedge - RightPoint) * LedgeDir;

	int LedgeMode = pCD->m_ControlMode_Param4;

	int LinkMode = LedgeMode & LEDGE_LINKMODE_MASK;
	fp32 RightLength = 0.0f;

	// If we're close to left point or right point and we have link, make riddick go around corner
	// instead
	if (_Move == LEDGEMOVE_LEFT && ((LedgeLength - CurrentPos) <= LEDGE_CORNER_DISTANCE))
	{
		if (_pContext->m_pWPhysState->IsClient())
			return false;
		CWObject_Ledge* pRealLedge = (CWObject_Ledge*)pObjLedge;
		if (!pRealLedge->CanSwitchLedge(true,pCD,_pContext->m_pObj->m_iObject,RightLength))
		{
			pRealLedge->m_LastSwitchTest = pCD->m_GameTick+1;
			return false;
		}
		if (LinkMode & LEDGE_LINKMODE_LEFTIN)
		{
			_Move = LEDGEMOVE_CLIMBCORNERINLEFT;
		}
		else if (LinkMode & LEDGE_LINKMODE_LEFTOUT)
		{
			_Move = LEDGEMOVE_CLIMBCORNEROUTLEFT;
		}
		else
		{
			// Can't move more to the left..?
			return false;
		}
	}
	else if (_Move == LEDGEMOVE_RIGHT && ((CurrentPos - 0.5f) <= LEDGE_CORNER_DISTANCE))
	{
		if (_pContext->m_pWPhysState->IsClient())
			return false;

		CWObject_Ledge* pRealLedge = (CWObject_Ledge*)pObjLedge;
		if (!pRealLedge->CanSwitchLedge(false,pCD,_pContext->m_pObj->m_iObject,RightLength))
		{
			pRealLedge->m_LastSwitchTest = pCD->m_GameTick+1;
			return false;
		}
		if (LinkMode & LEDGE_LINKMODE_RIGHTIN)
		{
			_Move = LEDGEMOVE_CLIMBCORNERINRIGHT;
		}
		else if (LinkMode & LEDGE_LINKMODE_RIGHTOUT)
		{
			_Move = LEDGEMOVE_CLIMBCORNEROUTRIGHT;
		}
		else
		{
			// Can't move more to the left..?
			return false;
		}
	}
	else if (_Move == LEDGEMOVE_CLIMBUPFROMHANGING)
	{
		if (LedgeMode & CWOBJECT_LEDGE_USEALTCLIMBUP)
			_Move = LEDGEMOVE_ALTCLIMBUPFROMHANGING;
		// Check if we can climb up the ledge
		if (!CanClimbUp(_pContext->m_pWPhysState,PointOnLedge, LedgeDir,Normal, pCD->m_Phys_Width * 2.0f, pCD->m_Phys_Height * 2.0f))
			return false;
	}
	else if ((_Move == LEDGEMOVE_CLIMBDOWNTOLEDGE) && (LedgeMode & CWOBJECT_LEDGE_USEALTCLIMBUP))
	{
		_Move = LEDGEMOVE_ALTCLIMBDOWNTOLEDGE;
	}

	// Make the move
	MakeLedgeMove(_pContext, pObjLedge, pCD,_Move,LeftPoint,RightPoint,Normal,CurrentPos,RightLength);
	//pCD->m_RelAnimPos.MakeLedgeMove(_pContext->m_pWPhysState,_pContext->m_pObj,pCD,_Move,LeftPoint,RightPoint,Normal,CurrentPos,RightLength);
	//pCD->m_RelAnimPos.MakeDirty();

	return true;
}

void CWObject_Ledge::MakeLedgeMove(const CWAG2I_Context* _pContext, CWObject_CoreData* _pObjLedge, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength)
{
	// Find destination and set it
	CMat4Dfp32 Mat;
	//ConOutL(CStrF("%s: Making Move: Pos: %f",_pContext->m_pWPhysState->IsServer() ? "S":"C",_CurrentLedgePos));
	GetLedgePoint(Mat, _Move, _LeftPoint, _RightPoint, _Normal, _CurrentLedgePos, _RightLength);
	_pCD->m_AnimGraph2.SetDestination(Mat);
	_pContext->m_pWPhysState->Object_SetDirty(_pObjLedge->m_iObject,1 << CWO_DIRTYMASK_USERSHIFT);
	CACSClientData* pACSCD = GetACSClientData(_pObjLedge);
	pACSCD->SetCachedMat(Mat);
	
	// Send the impulse
	if (_pCD->m_AnimGraph2.GetAG2I()->SendImpulse(_pContext,CXRAG2_Impulse(AG2_IMPULSETYPE_LEDGE,_Move),0) && _pContext->m_pWPhysState->IsServer())
	{
		switch (_Move)
		{
		case LEDGEMOVE_CLIMBCORNERINLEFT:
		case LEDGEMOVE_CLIMBCORNEROUTLEFT:
			{
				CWObject_Message Msg(OBJMSG_LEDGE_SWITCHLEDGE);
				Msg.m_pData = _pContext->m_pObj;
				_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pObjLedge->m_iObject);
				break;
			}
		case LEDGEMOVE_CLIMBCORNERINRIGHT:
		case LEDGEMOVE_CLIMBCORNEROUTRIGHT:
			{
				CWObject_Message Msg(OBJMSG_LEDGE_SWITCHLEDGE,1);
				Msg.m_pData = _pContext->m_pObj;
				_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pObjLedge->m_iObject);
				break;
			}
		default:
			break;
		}
	}
}

bool CWObject_Ledge::GetLedgePoint(CMat4Dfp32& _Mat, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength)
{
	// Most of these just involve setting the anchor point as far away as the animation says, the 
	// corner animations need to be set at the far side of the ledges (whatever side we're at)
	CVec3Dfp32 LedgeDir = _LeftPoint - _RightPoint;
	fp32 Length = LedgeDir.Length();
	LedgeDir = LedgeDir / Length;

	// FIXED FOR DYNAMIC LEDGES

	// Preprocess ledgeposition
	if (_CurrentLedgePos > (Length - 14.0f))
		_CurrentLedgePos = Length - 12.0f;
	else if (_CurrentLedgePos < 14.0f)
		_CurrentLedgePos = 12.0f;

	//CMat4Dfp32 InvMat;
	_Mat.Unit();
	_Normal.SetMatrixRow(_Mat,0);
	CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(_Mat,2);
	_Mat.RecreateMatrix(0,2);
	//_Mat.InverseOrthogonal(InvMat);

	//LedgeDir = LedgeDir * InvMat;
	//CVec3Dfp32 Normal = _Normal * InvMat;
	//CVec3Dfp32 RightPoint = _RightPoint * InvMat;
	//CVec3Dfp32 LeftPoint = RightPoint + LedgeDir * Length;
	CVec3Dfp32& MidPoint = _Mat.GetRow(3);
	CVec3Dfp32& Dir = _Mat.GetRow(0);

	CVec3Dfp32 LedgePos = _RightPoint + LedgeDir * _CurrentLedgePos;

	switch (_Move)
	{
	case LEDGEMOVE_IDLE:
	case LEDGEMOVE_LEFT:
	case LEDGEMOVE_RIGHT:
	case LEDGEMOVE_ALTCLIMBUPFROMHANGING:
	case LEDGEMOVE_DROPDOWN:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_CLIMBUPFROMHANGING:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 61.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_UPLOW:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 48.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_UPMEDIUM:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 64.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_JUMPUPANDGRAB:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 96.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_CLIMBDOWNTOLEDGE:
		{
			Dir = _Normal;
			MidPoint = LedgePos - _Normal * 9.7f;// + CVec3Dfp32(0,0,1.0f);
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_ALTCLIMBDOWNTOLEDGE:
		{
			Dir = -_Normal;
			MidPoint = LedgePos + _Normal * 12.0f;// + CVec3Dfp32(0,0,1.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}
	case LEDGEMOVE_CLIMBCORNEROUTLEFT:
	case LEDGEMOVE_CLIMBCORNERINLEFT:
		{
			Dir = -_Normal;
			MidPoint = _LeftPoint + _Normal * 12.0f - LedgeDir * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;

			// Calculate offset from next ledge... :/
			/*CVec3Dfp32 NormalRight;
			CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			MidPoint =  -NormalRight * 12.0f + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;*/
		}
/*	case LEDGEMOVE_CLIMBCORNEROUTLEFT:
		{
			CVec3Dfp32 NormalRight;
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			MidPoint =  -NormalRight * 12.0f - Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			// Set point 12 units from corner
			break;
		}*/
	case LEDGEMOVE_CLIMBCORNEROUTRIGHT:
	case LEDGEMOVE_CLIMBCORNERINRIGHT:
		{
			Dir = -_Normal;
			MidPoint = _RightPoint + _Normal * 12.0f + LedgeDir * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);

			//WTF
			/*// Calculate offset from next ledge... :/
			CVec3Dfp32 NormalRight;
			//CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			MidPoint = LeftPoint - NormalRight * (_RightLength - 12.0f) + Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;*/
		}
	/*case LEDGEMOVE_CLIMBCORNEROUTRIGHT:
		{
			CVec3Dfp32 NormalRight;
			_Normal.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),NormalRight);
			//CVec3Dfp32(0.0f,0.0f,1.0f).CrossProd(_Normal,NormalRight);
			Dir = -_Normal;
			CMat4Dfp32 Mat,InvMat;
			Mat.Unit();
			_Normal.SetMatrixRow(Mat,0);
			CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(Mat,2);
			Mat.RecreateMatrix(0,2);
			Mat.InverseOrthogonal(InvMat);
			NormalRight = NormalRight * InvMat;
			MidPoint = LeftPoint - NormalRight * (_RightLength + 12.0f) - Normal * 12.0f;// - CVec3Dfp32(0,0,62.0f);
			MidPoint.k[2] -= 62.0f;
			_Mat.RecreateMatrix(0,2);
			break;
		}*/
	default:
		return false;
	}
	//m_Angle = CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);
	return true;
}

bool CWObject_Ledge::GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
										  CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, 
										  const CVec3Dfp32& _Move, const CMat4Dfp32& _MatLook,
										  CWO_Character_ClientData* _pCD, int16& _Flags, fp32 _dTime)
{
	MAUTOSTRIP( CWObject_Ledge_GetUserAccelleration, false );
	//_Flags |= 1;	// Turn off gravity in PhysUtil_Move
	_VRet = 0;
	int iLedge = (int)_pCD->m_ControlMode_Param0;
//	fp32 LedgePos = _pCD->m_ControlMode_Param1;
//	int iLedgeID = (int)_pCD->m_ControlMode_Param2;
	int LedgeMode = _pCD->m_ControlMode_Param4;

	int LinkMode = LedgeMode & LEDGE_LINKMODE_MASK;

	CWObject_CoreData* pObjLedge = _pPhysState->Object_GetCD(iLedge);
	if (!pObjLedge || !LedgeDataValid(pObjLedge))
		return true;

	CVec3Dfp32 Position1, Position2, Normal, LedgeDir, CharPos, PointOnLedge;
	fp32 LedgeLength;
	int LedgeFlags;

	// Get ledge data
	UnPackLedgeData(pObjLedge, Position1, Position2, Normal,LedgeLength,LedgeFlags);

	/*if (_pPhysState->IsClient())
		return false;*/
	
	// Check if the dynamic ledge is attached to anything..
	if (LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
	{
		GetDynamicLedgePos(_pPhysState,pObjLedge,Position1,Position2,Normal);
		/*CMat4Dfp32 Mat;
		int iParent = _pPhysState->Object_GetParent(pObjLedge->m_iObject);
		if (iParent != 0)
		{
			CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,int(&Mat));
			_pPhysState->Phys_Message_SendToObject(RenderMat,iParent);
			// Parent predicted position + ledge local position -> real position
			CVec3Dfp32 LedgePos = pObjLedge->GetLocalPosition() + CVec3Dfp32::GetMatrixRow(Mat,3);
			//ConOut(CStrF("%s: Mat: %s Pos: %s", (_pPhysState->IsServer() ? "S":"C"),Mat.GetString().GetStr(), pObjLedge->GetLocalPosition().GetString().GetStr()));

			// If the ledge is dynamic make the positions "unrelative"
			Position1 += LedgePos;
			Position2 += LedgePos;
		}*/
	}
	//ConOut(CStrF("%s: %s",(_pPhysState->IsServer() ? "S" :"C"),pObjLedge->GetLocalPosition().GetString().GetStr()));

	CharPos = _pObj->GetPosition();

	LedgeDir = (Position2 - Position1) / LedgeLength;

	fp32 LedgeDotLeft, LedgeDotRight;
	int LedgePoint = FindLedgePoint(LedgeDir, CharPos, Position1, PointOnLedge, LedgeLength, 
		LEDGE_OPTIMALDISTANCE, LinkMode, LedgeDotLeft, LedgeDotRight);
	// Set endpoint
	if (_pPhysState->IsServer())
		_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_SETCURRENTENDPOINT,LedgePoint),pObjLedge->m_iObject);
	else
		pObjLedge->m_Data[LADDERENDPOINT] = LedgePoint;

	CWAG2I_Context AGContext(_pObj,_pPhysState,CMTime::CreateFromTicks(_pCD->m_GameTick,_pPhysState->GetGameTickTime(),_dTime));
	int MoveType = _pCD->m_AnimGraph2.GetAnimPhysMoveType();
	int32 StateFlagsLo = _pCD->m_AnimGraph2.GetStateFlagsLo();
	switch (LedgePoint)
	{
	case LEDGE_ENDPOINT_LEFT:
	case LEDGE_ENDPOINT_OUTLEFT:
	case LEDGE_ENDPOINT_INLEFT:
		{
			if ((MoveType == ANIMPHYSMOVETYPE_LEFT) && (StateFlagsLo & AG2_STATEFLAG_BLOCKACTIVE) &&
				!(StateFlagsLo & AG2_STATEFLAG_RELOADING))
				MakeLedgeMove(&AGContext,LEDGEMOVE_IDLE);
			break;
		}
	case LEDGE_ENDPOINT_RIGHT:
	case LEDGE_ENDPOINT_OUTRIGHT:
	case LEDGE_ENDPOINT_INRIGHT:
	
		{
			// Stop for now
			if ((MoveType == ANIMPHYSMOVETYPE_RIGHT) && (StateFlagsLo & AG2_STATEFLAG_BLOCKACTIVE) &&
				!(StateFlagsLo & AG2_STATEFLAG_RELOADING))
				MakeLedgeMove(&AGContext,LEDGEMOVE_IDLE);
			break;
		}
	case LEDGE_ENDPOINT_NOENDPOINT:
	default:
		{
			// Do nothing
			break;
		}
	};

	CACSClientData* pACSCD = GetACSClientData(pObjLedge);
	// Set destination again, might not be set on client!
	_pCD->m_AnimGraph2.SetDestination(pACSCD->m_CachedSeqMat);
	CWObject_Character::Char_ControlMode_Anim2(_Selection, _pObj, _pPhysState, _Flags);
	return true;
	// Using animsync for now
	/*_Flags |= 2;
	CWAG2I_Context AGContext(_pObj, _pPhysState, _pCD->m_GameTime);
	CVec3Dfp32 MoveVelocity;
	CQuatfp32 RotVelocity;
	//_pCD->m_RelAnimPos.GetVelocityLedge(&AGContext, _pCD->m_AnimGraph2.GetAG2I(), Position2, Normal, MoveVelocity, RotVelocity);

	RotVelocity.Normalize();
	CAxisRotfp32 RotVelAR;
	RotVelAR.Create(RotVelocity);

	_pPhysState->Object_SetVelocity(_pObj->m_iObject, MoveVelocity);
	_pPhysState->Object_SetRotVelocity(_pObj->m_iObject, RotVelAR);

	return true;*/
}

// New ledge tests
CWObject_Ledge* CWObject_Ledge::FindMainLedge(CWorld_PhysState* _pWPhys)
{
	MAUTOSTRIP( CWObject_Ledge_FindMainLedge, NULL );
	// Do a selection on the "usable" type which should all be ledges
	TSelection<CSelection::LARGE_BUFFER> Selection;
	// Select the ledge class (don't know if this should be used FIXME)
	_pWPhys->Selection_AddClass(Selection, "Ledge");
	
	const int16* pSel;
	int nSel = _pWPhys->Selection_Get(Selection, &pSel);
	
	// Check if there were any ledges
	CWObject_Ledge* pLedge = NULL;
	if (nSel > 0)
	{
		for (int i = 0; i < nSel; i++)
		{
			// Got ledge lets see if we can use it
			int LedgeFlags = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LEDGE_GETFLAGS),pSel[i]);
			if (LedgeFlags & CWOBJECT_LEDGE_ISMAINLEDGE)
				pLedge = safe_cast<CWObject_Ledge>(_pWPhys->Object_GetCD(pSel[i]));
		}
	}

	return pLedge;
}

int	CWObject_Ledge::CalculateLedgeArrayLength()
{
	MAUTOSTRIP( CWObject_Ledge_CalculateLedgeArrayLength, 0 );
	// Do a selection on the "usable" type which should all be ledges
	TSelection<CSelection::LARGE_BUFFER> Selection;
	// Select the ledge class (don't know if this should be used FIXME)
	m_pWServer->Selection_AddClass(Selection, "Ledge");
	
	const int16* pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	
	// Check if there were any ledges
	int ArrayLen = 0;
	if (nSel > 0)
	{
		for (int i = 0; i < nSel; i++)
		{
			CWObject_CoreData* pObj = m_pWServer->Object_Get(pSel[i]);
			int LedgeFlags = m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_LEDGE_GETFLAGS),pSel[i]);
			// Don't calculate dynamic ledges
			if (pObj && !(LedgeFlags & CWOBJECT_LEDGE_DYNAMIC))
			{
				CWO_PosHistory* KeyFrames = GetClientData(pObj);
				if(pObj->m_iAnim0 != -1 && !KeyFrames->IsValid())
					LoadPath(m_pWServer, pObj, pObj->m_iAnim0);

				int NumKeys = (KeyFrames ? (KeyFrames->m_lSequences.Len() > 0 ? 
					KeyFrames->m_lSequences[0].GetNumKeyframes() : 0) : 0);
				
				// Need atleast 2 positions to make a ledge
				if (NumKeys > 1)
					ArrayLen += NumKeys - 1;
			}
		}
	}

	return ArrayLen;
}

int CWObject_Ledge::FindLinkMode(const CVec3Dfp32& _LedgeDir, const CVec3Dfp32 _LinkNormal, 
								 bool _bLinkLeft)
{
	MAUTOSTRIP( CWObject_Ledge_FindLinkMode, 0 );
	if (_bLinkLeft)
	{
		fp32 Dot = _LinkNormal * _LedgeDir;
		if (Dot < 0.0f)
		{
			// Out corner
			return LEDGE_LINKMODE_LEFTOUT;
		}
		else if (Dot > 0.0f)
		{
			return LEDGE_LINKMODE_LEFTIN;
		}
		else
		{
			ConOut("CWObject_Ledge::FindLinkMode: ERROR LEFTLINKMODE ERROR");
		}
	}
	else
	{
		fp32 Dot = _LinkNormal * _LedgeDir;
		if (Dot < 0.0f)
		{
			// Out corner
			return LEDGE_LINKMODE_RIGHTIN;
		}
		else if (Dot > 0.0f)
		{
			return LEDGE_LINKMODE_RIGHTOUT;
		}
		else
		{
			ConOut("CWObject_Ledge::FindLinkMode: ERROR RIGHTLINKMODE ERROR");
		}
	}

	// No link mode found
	return 0;
}

// Find maximum distance from midpoint
fp32 CWObject_Ledge::FindRange(const TThinArray<CLedge>& _lLedges, const CVec3Dfp32& _MidPoint, 
							  int _iIndex)
{
	MAUTOSTRIP( CWObject_Ledge_FindRange, 0.0f );
	fp32 MaxDist = 0;
	fp32 DistLeft = (_lLedges[_iIndex].GetLeftPoint() - _MidPoint).LengthSqr();
	fp32 DistRight = (_lLedges[_iIndex].GetRightPoint() - _MidPoint).LengthSqr();

	MaxDist = Max(DistLeft,DistRight);
	MaxDist = M_Sqrt(MaxDist);
	if (_lLedges[_iIndex].GetBinLeft() != -1)
	{
		fp32 DistLinkLeft = FindRange(_lLedges,_MidPoint, _lLedges[_iIndex].GetBinLeft());
		MaxDist = Max(MaxDist,DistLinkLeft);
	}
	if (_lLedges[_iIndex].GetBinRight() != -1)
	{
		fp32 DistLinkRight = FindRange(_lLedges,_MidPoint, _lLedges[_iIndex].GetBinRight());
		MaxDist = Max(MaxDist,DistLinkRight);
	}

	return MaxDist;
}

int CWObject_Ledge::BuildBinCollTree(TThinArray<CLedge>& _lLedges, int _Left, int _Right, bool _bMethod1)
{
	MAUTOSTRIP( CWObject_Ledge_BuildBinCollTree, -1 );
	int Len = _Right - _Left;
	// Terminated recursive call
	if (Len < 0)
		return -1;
	
	if (Len == 0)
	{
		// Root node, no children, radius is local radius
		_lLedges[_Right].GetBinLeft() = -1;
		_lLedges[_Right].GetBinRight() = -1;
		_lLedges[_Right].GetRadius() = (uint16)M_Ceil(_lLedges[_Right].GetRadiusLocal());

		return _Right;
	}
	else
	{
		// The ledge in the middle of the span as root and the rest as left/right nodes
		int iMiddle = (_Right + _Left)/2;
		CVec3Dfp32 PositionLeft,PositionRight;
		PositionLeft = _lLedges[iMiddle].GetLeftPoint();
		PositionRight = _lLedges[iMiddle].GetRightPoint();

		int iLeft = BuildBinCollTree(_lLedges, _Left, iMiddle - 1);
		int iRight = BuildBinCollTree(_lLedges, iMiddle + 1, _Right);
		_lLedges[iMiddle].GetBinLeft() = iLeft;
		_lLedges[iMiddle].GetBinRight() = iRight;
		
		// Find a sphere big enough to encompass all ledges below and including this node
		if (_bMethod1)
		{
			fp32 LengthLeft,LengthRight;
			fp32 MaxRad = _lLedges[iMiddle].GetRadiusLocal();
			LengthLeft = LengthRight = 0.0f;
			
			if (iLeft != -1)
				LengthLeft = FindRange(_lLedges,_lLedges[iMiddle].GetMidpoint(),iLeft);
			if (iRight != -1)
				LengthRight = FindRange(_lLedges,_lLedges[iMiddle].GetMidpoint(),iRight);
			MaxRad = Max(MaxRad,Max(LengthLeft,LengthRight));
			
			_lLedges[iMiddle].GetRadius() = (uint16)M_Ceil(MaxRad);
		}
		else
		{
			fp32 LengthLeft,LengthRight;
			LengthLeft = (iLeft < 0 ? 0 : 
				(_lLedges[iLeft].GetMidpoint() - _lLedges[iMiddle].GetMidpoint()).Length());
			LengthRight = (iRight < 0 ? 0 : 
				(_lLedges[iRight].GetMidpoint() - _lLedges[iMiddle].GetMidpoint()).Length());

			// Find max radius for this node
			fp32 MaxRad = _lLedges[iMiddle].GetRadiusLocal();
			LengthLeft += (iLeft < 0 ? 0 : _lLedges[iLeft].GetRadius());
			LengthRight += (iRight < 0 ? 0 : _lLedges[iRight].GetRadius());
			MaxRad = Max(MaxRad, Max(LengthLeft,LengthRight));

			_lLedges[iMiddle].GetRadius() = (uint16)M_Ceil(MaxRad);
		}

		return iMiddle;
	}
}

// Return new radius...
uint16 CWObject_Ledge::InsertNode(TThinArray<CLedge>& _lLedges, int _Start, int _NewNode)
{
	MAUTOSTRIP( CWObject_Ledge_InsertNode, 0 );
	if (_NewNode < 0)
		return 0;
	// Insert the node at the closest free ledge in the tree...
	// Check which node is closest
	int iLeft = _lLedges[_Start].GetBinLeft();
	int iRight = _lLedges[_Start].GetBinRight();
	
	if ((iLeft < 0) && iRight < 0)
	{
		// Insert left
		_lLedges[_Start].GetBinLeft() = _NewNode;
		
		// Find new radius for this node, should update parent to this node as well
		uint16 LengthLeft = (uint16)M_Ceil((_lLedges[_NewNode].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
		LengthLeft += _lLedges[_NewNode].GetRadius();
		_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthLeft);
	}
	else if (iLeft < 0)
	{
		// Check if the right node is closer than this node
		fp32 DistanceRight = (_lLedges[iRight].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		fp32 DistanceMiddle = (_lLedges[_Start].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		if (DistanceMiddle < DistanceRight)
		{
			// Place the node in the left link ("this" node is closer than the right node
			// Insert left
			_lLedges[_Start].GetBinLeft() = _NewNode;

			// Find new radius for this node
			uint16 LengthLeft = (uint16)M_Ceil((_lLedges[_NewNode].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
			LengthLeft += _lLedges[_NewNode].GetRadius();
			_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthLeft);
			
		}
		else
		{
			// Ok, recurse to the right and update the radius we got from that one
			InsertNode(_lLedges, iRight, _NewNode);
			// Find new radius for this node
			uint16 LengthRight = (uint16)M_Ceil((_lLedges[iRight].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
			LengthRight += _lLedges[iRight].GetRadius();
			_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthRight);
		}
	}
	else if (iRight < 0)
	{
		// Check if the right node is closer than this node
		fp32 DistanceLeft = (_lLedges[iLeft].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		fp32 DistanceMiddle = (_lLedges[_Start].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		if (DistanceMiddle < DistanceLeft)
		{
			// Place the node in the left link ("this" node is closer than the right node
			// Insert left
			_lLedges[_Start].GetBinRight() = _NewNode;

			// Find new radius for this node
			uint16 LengthRight = (uint16)M_Ceil((_lLedges[_NewNode].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
			LengthRight += _lLedges[_NewNode].GetRadius();
			_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthRight);
			
		}
		else
		{
			// Ok, recurse to the right and update the radius we got from that one
			InsertNode(_lLedges, iLeft, _NewNode);
			// Find new radius for this node
			uint16 LengthLeft = (uint16)M_Ceil((_lLedges[iLeft].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
			LengthLeft += _lLedges[iLeft].GetRadius();
			_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthLeft);
		}
	}
	else
	{
		// Find which is closest, left or right
		fp32 DistanceLeft = (_lLedges[iLeft].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		fp32 DistanceRight = (_lLedges[iRight].GetMidpoint() - _lLedges[_NewNode].GetMidpoint()).LengthSqr();

		int iFinal = -1;
		if (DistanceLeft < DistanceRight)
		{
			InsertNode(_lLedges, iLeft, _NewNode);
			iFinal = iLeft;
		}
		else
		{
			InsertNode(_lLedges, iRight, _NewNode);
			iFinal = iRight;
		}
		
		// Find new radius for this node
		uint16 LengthFinal = (uint16)M_Ceil((_lLedges[iFinal].GetMidpoint() - _lLedges[_Start].GetMidpoint()).Length());
		LengthFinal += _lLedges[iFinal].GetRadius();
		_lLedges[_Start].GetRadius() = Max(_lLedges[_Start].GetRadius(), LengthFinal);
	}

	return 0;
}

uint16 CWObject_Ledge::InsertNode(TThinArray<CLedge>& _lLedges, 
								  TThinArray<CBinNode>& _lLedgeBinTree, int _Start, int _NewNode)
{
	MAUTOSTRIP( CWObject_Ledge_InsertNode_2, 0 );
	// Insert the given node into the branch that has the closest midpoint...
//	int Len = _lLedgeBinTree.Len();
	
	// Check which branch to descend into
	// Insert the node at the closest free ledge in the tree...
	// Check which node is closest
	int iLeft = _lLedgeBinTree[_Start].GetBinLeft();
	int iRight = _lLedgeBinTree[_Start].GetBinRight();

	if (iLeft < 0)
	{
		// Insert left
		_lLedgeBinTree[_Start].GetBinLeft() = _NewNode;
		_lLedgeBinTree[_Start].GetFlags() |= CWOBJECT_LEDGE_PSEUDOLEFTISLEAF;
		
		// Find New midpoint
		CVec3Dfp32 MidPoint;
		MidPoint = _lLedgeBinTree[_Start].GetMidpoint();
		if (iRight >= 0)
		{
			MidPoint += _lLedges[_NewNode].GetMidpoint();
			MidPoint = MidPoint * 0.5f;
			_lLedgeBinTree[_Start].GetMidpoint() = MidPoint;
			int bIsLeaf = _lLedgeBinTree[_Start].RightIsLeaf();
			// Find new radius for the midpoint
			CVec3Dfp32 RightMidpoint = (bIsLeaf ? 
				 _lLedges[iRight].GetMidpoint() : _lLedgeBinTree[iRight].GetMidpoint());
			uint16 LengthRight = (uint16)M_Ceil((RightMidpoint - MidPoint).Length());
			LengthRight += (bIsLeaf ? _lLedges[iRight].GetRadius() : 
				_lLedgeBinTree[iRight].GetRadius());
			_lLedgeBinTree[_Start].GetRadius() = LengthRight;
		}

		// Find new radius for this node, should update parent to this node as well
		uint16 LengthLeft = (uint16)M_Ceil((_lLedges[_NewNode].GetMidpoint() - 
			MidPoint).Length());
		LengthLeft += _lLedges[_NewNode].GetRadius();
		// Check which radius is bigger, right or left..
		_lLedgeBinTree[_Start].GetRadius() = Max(_lLedgeBinTree[_Start].GetRadius(),LengthLeft);
	}
	else if (iRight < 0)
	{
		// Insert Right
		_lLedgeBinTree[_Start].GetBinRight() = _NewNode;
		_lLedgeBinTree[_Start].GetFlags() |= CWOBJECT_LEDGE_PSEUDORIGHTISLEAF;
		
		// Find New midpoint
		CVec3Dfp32 MidPoint;
		MidPoint = _lLedgeBinTree[_Start].GetMidpoint();
		if (iLeft >= 0)
		{
			MidPoint += _lLedges[_NewNode].GetMidpoint();
			MidPoint = MidPoint * 0.5f;
			_lLedgeBinTree[_Start].GetMidpoint() = MidPoint;
			int bIsLeaf = _lLedgeBinTree[_Start].LeftIsLeaf();
			// Find new radius for the midpoint
			CVec3Dfp32 RightMidpoint = (bIsLeaf ? 
				 _lLedges[iLeft].GetMidpoint() : _lLedgeBinTree[iLeft].GetMidpoint());
			uint16 LengthLeft = (uint16)M_Ceil((RightMidpoint - MidPoint).Length());
			LengthLeft += (bIsLeaf ? _lLedges[iLeft].GetRadius() : 
				_lLedgeBinTree[iLeft].GetRadius());
			_lLedgeBinTree[_Start].GetRadius() = LengthLeft;
		}

		// Find new radius for this node, should update parent to this node as well
		uint16 LengthRight = (uint16)M_Ceil((_lLedges[_NewNode].GetMidpoint() - 
			MidPoint).Length());
		LengthRight += _lLedges[_NewNode].GetRadius();
		// Check which radius is bigger, right or left..
		_lLedgeBinTree[_Start].GetRadius() = Max(_lLedgeBinTree[_Start].GetRadius(),LengthRight);
	}
	else
	{
		// Check which branch to insert into, or if we have to insert a new node

		// Begin by finding the closest node
		CVec3Dfp32 MidpointLeft = (_lLedgeBinTree[_Start].LeftIsLeaf() ? 
			_lLedges[iLeft].GetMidpoint() : _lLedgeBinTree[iLeft].GetMidpoint());
		CVec3Dfp32 MidpointRight = (_lLedgeBinTree[_Start].RightIsLeaf() ? 
			_lLedges[iRight].GetMidpoint() : _lLedgeBinTree[iRight].GetMidpoint());

		fp32 DistanceLeft = (MidpointLeft - _lLedges[_NewNode].GetMidpoint()).LengthSqr();
		fp32 DistanceRight = (MidpointRight - _lLedges[_NewNode].GetMidpoint()).LengthSqr();

		if (DistanceLeft < DistanceRight)
		{
			// Insert into left branch, if it's a leaf node create a new node and insert
			if (_lLedgeBinTree[_Start].LeftIsLeaf())
			{
				int16 iNewNode = _lLedgeBinTree.Len();
				_lLedgeBinTree.SetLen(iNewNode + 1);

				// Link to the new node and remove leaf flag on that node
				_lLedgeBinTree[_Start].GetBinLeft() = iNewNode;
				_lLedgeBinTree[_Start].GetFlags() &= ~CWOBJECT_LEDGE_PSEUDOLEFTISLEAF;

				CVec3Dfp32 Midpoint = MidpointLeft + _lLedges[_NewNode].GetMidpoint();
				Midpoint = Midpoint * 0.5f;
				_lLedgeBinTree[iNewNode].GetMidpoint() = Midpoint;
				_lLedgeBinTree[iNewNode].GetBinLeft() = iLeft;
				_lLedgeBinTree[iNewNode].GetBinRight() = _NewNode;
				_lLedgeBinTree[iNewNode].GetFlags() = CWOBJECT_LEDGE_PSEUDOLEFTISLEAF | 
					CWOBJECT_LEDGE_PSEUDORIGHTISLEAF;
			
				// Find new radius
				fp32 LengthLeft = (_lLedges[iLeft].GetMidpoint() - Midpoint).Length();
				LengthLeft += _lLedges[iLeft].GetRadius();
				fp32 LengthRight = (_lLedges[_NewNode].GetMidpoint() - Midpoint).Length();
				LengthRight += _lLedges[_NewNode].GetRadius();
				
				_lLedgeBinTree[iNewNode].GetRadius() = (uint16)M_Ceil(Max(LengthLeft,LengthRight));
   			}
			else
			{
				// Left is not leaf node->Recursive call
				InsertNode(_lLedges, _lLedgeBinTree, iLeft, _NewNode);
			}
		}
		else
		{
			// Insert into right branch, if it's a leaf node create a new node and insert
			if (_lLedgeBinTree[_Start].RightIsLeaf())
			{
				int16 iNewNode = _lLedgeBinTree.Len();
				_lLedgeBinTree.SetLen(iNewNode + 1);

				// Link to the new node and remove leaf flag on that node
				_lLedgeBinTree[_Start].GetBinRight() = iNewNode;
				_lLedgeBinTree[_Start].GetFlags() &= ~CWOBJECT_LEDGE_PSEUDORIGHTISLEAF;

				CVec3Dfp32 Midpoint = MidpointRight + _lLedges[_NewNode].GetMidpoint();
				Midpoint = Midpoint * 0.5f;
				_lLedgeBinTree[iNewNode].GetMidpoint() = Midpoint;
				_lLedgeBinTree[iNewNode].GetBinLeft() = _NewNode;
				_lLedgeBinTree[iNewNode].GetBinRight() = iRight;
				_lLedgeBinTree[iNewNode].GetFlags() = CWOBJECT_LEDGE_PSEUDOLEFTISLEAF | 
					CWOBJECT_LEDGE_PSEUDORIGHTISLEAF;
			
				// Find new radius
				fp32 LengthLeft = (_lLedges[_NewNode].GetMidpoint() - Midpoint).Length();
				LengthLeft += _lLedges[_NewNode].GetRadius();
				fp32 LengthRight = (_lLedges[iRight].GetMidpoint() - Midpoint).Length();
				LengthRight += _lLedges[iRight].GetRadius();
				
				_lLedgeBinTree[iNewNode].GetRadius() = (uint16)M_Ceil(Max(LengthLeft,LengthRight));
   			}
			else
			{
				// Left is not leaf node->Recursive call
				InsertNode(_lLedges, _lLedgeBinTree, iRight, _NewNode);
			}
		}
		// Update radius on parent node
		iLeft = _lLedgeBinTree[_Start].GetBinLeft();
		iRight = _lLedgeBinTree[_Start].GetBinRight();
		CVec3Dfp32 Midpoint = _lLedgeBinTree[_Start].GetMidpoint();
		int bIsLeafLeft = _lLedgeBinTree[_Start].LeftIsLeaf();
		int bIsLeafRight = _lLedgeBinTree[_Start].RightIsLeaf();
		fp32 LengthLeft = ((bIsLeafLeft ? _lLedges[iLeft].GetMidpoint() : _lLedgeBinTree[iLeft].GetMidpoint()) - Midpoint).Length();;
		LengthLeft += (bIsLeafLeft ? _lLedges[iLeft].GetRadius() : _lLedgeBinTree[iLeft].GetRadius());
		fp32 LengthRight = ((bIsLeafRight ? _lLedges[iRight].GetMidpoint() : _lLedgeBinTree[iRight].GetMidpoint()) - Midpoint).Length();
		LengthRight += (bIsLeafRight ? _lLedges[iRight].GetRadius() : _lLedgeBinTree[iRight].GetRadius());
		_lLedgeBinTree[_Start].GetRadius() = (uint16)Max(LengthLeft,LengthRight);
	}

	return 0;
}



// Find best ledge from the characters position (public)
int CWObject_Ledge::FindBestLedge(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _CharPos, int16 _iChar,
								  const CVec3Dfp32& _CharDir, const fp32& _Range, 
								  const bool& _bAirBorne, const fp32& _CharWidth,
								  int& _iBestLedge, int& _BestLedgeType, fp32& _LedgePos)
{
	MAUTOSTRIP( CWObject_Ledge_FindBestLedge, -1 );
	// Find all ledges and check each one
	// Do a selection on the "usable" type which should all be ledges
	TSelection<CSelection::LARGE_BUFFER> Selection;
	// Select the ledge class (don't know if this should be used FIXME)
	_pWPhys->Selection_AddClass(Selection, "Ledge");
	
	const int16* pSel;
	int nSel = _pWPhys->Selection_Get(Selection, &pSel);
	
	// Check if there were any ledges
	CWObject_Ledge* pLedge = NULL;
	int iBestLedgeObject = -1;
	if (nSel > 0)
	{
		fp32 BestLedgeSqr = _FP32_MAX;
		CVec3Dfp32 CharPos = _CharPos;
		CharPos.k[2] +=40.0f;
		_iBestLedge = -1;
		_BestLedgeType = -1;
		_LedgePos = 0.0f;
		CVec2Dfp32 CharDir2D(_CharDir.k[0], _CharDir.k[1]);
		CharDir2D.Normalize();
		const fp32 ZDir = _CharDir.k[2];

		fp32 Width = _CharWidth;//M_Sqrt(_CharWidth * _CharWidth * 2);
		
		struct TestVars Vars;
		Vars.m_CharPos = CharPos;
		Vars.m_CharDir2D = CharDir2D;
		Vars.m_ZDir = ZDir;
		//Vars.m_iRoot;
		Vars.m_RangeSqr = _Range * _Range;
		Vars.m_bAirborne = _bAirBorne;
		Vars.m_Width = Width;
		//Vars.m_DynPos = DynPos;
		Vars.m_iExclude = _iChar;

		for (int i = 0; i < nSel; i++)
		{
			// Got a pickup/fight/ladder/ledge/whatever, lets see it we can use it
			pLedge = safe_cast<CWObject_Ledge>(_pWPhys->Object_GetCD(pSel[i]));

			if (pLedge && (pLedge->m_lLedges.Len() > 0))
			{
				CACSClientData* pCD = GetACSClientData(pLedge);
				CVec3Dfp32 DynPos = 0.0f;

				fp32 TempBestLedgeSqr = BestLedgeSqr;
				fp32 TempLedgePos = _LedgePos;
				int iTempBestLedge = _iBestLedge;
				int TempBestLedgeType = _BestLedgeType;
				
				if (pLedge->m_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
				{
					if (pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN)
						continue;
					CMat4Dfp32 Mat;
					CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&Mat));
					_pWPhys->Phys_Message_SendToObject(RenderMat,pLedge->m_iObject);
					// Parent predicted position + ledge local position -> real position
					DynPos = /*pLedge->GetLocalPosition() +*/ CVec3Dfp32::GetMatrixRow(Mat,3);
					Vars.m_DynPos = DynPos;
					pLedge->FindBestLedge(Vars,pLedge->m_iDynamicRoot,/*CharPos, CharDir2D, ZDir, pLedge->m_iDynamicRoot, _Range * _Range, _bAirBorne,
						Width,DynPos,*/TempBestLedgeSqr,iTempBestLedge, TempBestLedgeType, TempLedgePos);
				}
				else
				{
					// Main ledge, with "pseudo ledge" tree
					Vars.m_DynPos = DynPos;
					pLedge->FindBestLedgePseudo(Vars,0,/*CharPos, CharDir2D, ZDir, 0, _Range * _Range, _bAirBorne,
						Width,DynPos,*/TempBestLedgeSqr,iTempBestLedge, TempBestLedgeType, TempLedgePos);
				}
				if ((iTempBestLedge != -1) && (TempBestLedgeSqr < BestLedgeSqr))
				{
					_iBestLedge = iTempBestLedge;
					_BestLedgeType = TempBestLedgeType;
					iBestLedgeObject = pLedge->m_iObject;
					_LedgePos = TempLedgePos;
				}
			}
		}
	}

	return (_iBestLedge != -1 ? iBestLedgeObject : -1);
}

// Find best ledge from the characters position (protected)
#define LEDGE_HEIGHTTHRESHOLD 5.0f
#define LEDGE_MAXHEIGHTTHRESHOLD 70.0f
#define LEDGE_GRABRANGETHRESHOLD 60.0f
void CWObject_Ledge::FindBestLedge(const struct TestVars& _Vars,const int32& _iRoot,/*const CVec3Dfp32& _CharPos, const CVec2Dfp32& _CharDir2D, const fp32& _ZDir,
								   const int& _iRoot, const fp32& _RangeSqr, 
								   const bool& _bAirborne, const fp32& _Width, const CVec3Dfp32& _DynPos,*/
								   fp32& _BestLedgeSqr, int& _iBestLedge, int& _BestLedgeType, fp32& _LedgePos)
{
	MAUTOSTRIP( CWObject_Ledge_FindBestLedge_2, -1 );
	// Ok, start at the root and find best ledge on the way
	fp32 NodeRadiusSqr = (fp32)m_lLedges[_iRoot].GetRadius();
	NodeRadiusSqr *= NodeRadiusSqr;
	// If the ledge is dynamic the midpoint is relative
	const CVec3Dfp32 MidPoint = (m_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC ? 
		m_lLedges[_iRoot].GetMidpoint() + _Vars.m_DynPos : m_lLedges[_iRoot].GetMidpoint());

	if (DoesIntersect(MidPoint,
		NodeRadiusSqr,_Vars.m_CharPos, _Vars.m_RangeSqr) != -1)
	{
		int iLeft = m_lLedges[_iRoot].GetBinLeft();
		int iRight = m_lLedges[_iRoot].GetBinRight();
		// Check left/right trees
		if (iLeft != -1)
		{
			FindBestLedge(_Vars,iLeft,/*_CharPos, _CharDir2D, _ZDir, iLeft, _RangeSqr, _bAirborne, _Width, _DynPos,*/_BestLedgeSqr, 
				_iBestLedge, _BestLedgeType, _LedgePos);
		}
		if (iRight != -1)
		{
			FindBestLedge(_Vars,iRight,/*_CharPos, _CharDir2D, _ZDir, iRight, _RangeSqr, _bAirborne, _Width, _DynPos,*/ _BestLedgeSqr, 
				_iBestLedge, _BestLedgeType, _LedgePos);
		}

		// Check this node
		NodeRadiusSqr = m_lLedges[_iRoot].GetRadiusLocal();
		NodeRadiusSqr *= NodeRadiusSqr;
		fp32 MiddleSqr = DoesIntersect(MidPoint,	NodeRadiusSqr,_Vars.m_CharPos, _Vars.m_RangeSqr);
		
		if ((MiddleSqr != -1) && (MiddleSqr < _BestLedgeSqr))
		{
			// Ok, the ledge is within range, now make sure the closest point on the ledge is 
			// within range
			// Find closest point on ledge
			CVec3Dfp32 LedgeDirection = m_lLedges[_iRoot].GetLedgeDir();
			CVec3Dfp32 LeftPoint = MidPoint - LedgeDirection * m_lLedges[_iRoot].GetRadiusLocal();			
			fp32 LedgeLength = m_lLedges[_iRoot].GetRadiusLocal() * 2.0f;
			CVec3Dfp32 Point = LeftPoint + LedgeDirection * ((_Vars.m_CharPos - LeftPoint) * LedgeDirection);
			CVec3Dfp32 DirToLedge = Point - _Vars.m_CharPos;
			CVec3Dfp32 DirToLedgeFlat = DirToLedge;
			DirToLedgeFlat.k[2] = 0.0f;
			
			// Check if point is outside bounds (pos2<->pos1)
			fp32 LedgeDot = (Point - LeftPoint) * LedgeDirection;
			if (!((LedgeDot > LedgeLength) || (LedgeDot < 0.0f)) && (M_Fabs(DirToLedge.k[2]) < 70.0f) && 
				(DirToLedgeFlat.LengthSqr() < LEDGE_GRABRANGETHRESHOLD * LEDGE_GRABRANGETHRESHOLD))
			{
				CVec3Dfp32 Normal = m_lLedges[_iRoot].GetNormal();
				CVec2Dfp32 Normal2D(Normal.k[0],Normal.k[1]);
				// Do some height checks and stuff to make sure we can use this ledge
				fp32 Dot = _Vars.m_CharDir2D * Normal2D;
				fp32 DirToLedgeDot = _Vars.m_CharDir2D * CVec2Dfp32(DirToLedge.k[0],DirToLedge.k[1]);

				// Check which type of ledge we are dealing with
				fp32 HeightDiff = MidPoint.k[2] - (_Vars.m_CharPos.k[2] - 40.0f);//CVec3Dfp32(0,0,40)
				int LedgeFlags = m_lLedges[_iRoot].GetLedgeFlags();

				// Check so that the character isn't looking at the other direction from 
				// where the ledge is
  				if ((HeightDiff < 10.0f) && (_Vars.m_ZDir > 0.0f))
					return;

				int LedgeType = FindLedgeType(Point, LedgeDirection, Normal, _Vars.m_Width, _Vars.m_iExclude);
				// If the character is on the ground -> either above or leveled
				if (!_Vars.m_bAirborne)
				{
					// When the character is standing on the ledge there shouldn't be much height difference
					if (LedgeType == CWOBJECT_LEDGE_TYPEHIGH)
					{
						// Enter from above ledge
						if ((HeightDiff < 1.0f) && (HeightDiff > -1.0f))
						{
							// Make sure the character is atleast facing the way the ledge is facing
							if (Dot > WOBJ_LEDGE_DIRECTIONSTRICTNESS)
							{
								_BestLedgeType = LedgeType | CWOBJECT_LEDGE_ENTERTYPEABOVELEDGE;
								_BestLedgeSqr = MiddleSqr;
								_iBestLedge = _iRoot;
								_LedgePos = LedgeDot;
							}
						}
						else
						{
							if ((HeightDiff < LEDGE_MAXGRABDISTANCEGROUND) &&
								(Dot < -WOBJ_LEDGE_DIRECTIONSTRICTNESS) &&
								(DirToLedgeDot > 0.0f))
							{
								_BestLedgeType = LedgeType | CWOBJECT_LEDGE_ENTERTYPEBELOWLEDGE | (LedgeFlags & CWOBJECT_LEDGE_USEALTCLIMBUP);
								_BestLedgeSqr = MiddleSqr;
								_iBestLedge = _iRoot;
								_LedgePos = LedgeDot;
							}
						}
					}
					else
					{
						if ((HeightDiff > LEDGE_HEIGHTTHRESHOLD) && 
							(HeightDiff < LEDGE_MAXHEIGHTTHRESHOLD) &&
							(DirToLedgeDot > 0.0f) &&
							((Dot < -WOBJ_LEDGE_DIRECTIONSTRICTNESS)/*||(Dot > WOBJ_LEDGE_DIRECTIONSTRICTNESS)*/) &&
							((LedgeType == CWOBJECT_LEDGE_TYPELOW) ||
							(LedgeType == CWOBJECT_LEDGE_TYPEMEDIUM)))
						{
							// Since these are low/medium ledges, check if we can climb up here
							if (CanClimbUp(m_pWServer,Point,LedgeDirection,Normal,26,64))
							{
								_BestLedgeType = LedgeType | CWOBJECT_LEDGE_ENTERTYPELEVELED;
								_BestLedgeSqr = MiddleSqr;
								_iBestLedge = _iRoot;
								_LedgePos = LedgeDot;
							}
						}
						else if ((HeightDiff > LEDGE_HEIGHTTHRESHOLD) && 
							(HeightDiff < 90.0f) &&
							(DirToLedgeDot > 0.0f) &&
							((Dot < -WOBJ_LEDGE_DIRECTIONSTRICTNESS) /*|| (Dot > WOBJ_LEDGE_DIRECTIONSTRICTNESS)*/) &&
							(LedgeType == CWOBJECT_LEDGE_TYPEMEDIUM))
						{
							// Hmm, ok we got a semi high ledge, ie we want to enter from air...
							_BestLedgeType = CWOBJECT_LEDGE_TYPEHIGH | CWOBJECT_LEDGE_ENTERTYPELEVELED;
							_BestLedgeSqr = MiddleSqr;
							_iBestLedge = _iRoot;
							_LedgePos = LedgeDot;
						}
					}
				}
				else
				{
					// Enter ledge from air, only look in front of us so we don't catch any 
					// ledges behind us when we're jumping
					// Check so that the heights aren't too different
					if ((LedgeType == CWOBJECT_LEDGE_TYPEHIGH) && 
						(HeightDiff > LEDGE_HEIGHTTHRESHOLD) &&
						(HeightDiff < 64) && 
						(DirToLedgeDot > 0.0f) && 
						(Dot < -WOBJ_LEDGE_DIRECTIONSTRICTNESS))
					{
						_BestLedgeType = LedgeType | CWOBJECT_LEDGE_ENTERTYPEBELOWLEDGE;
						_BestLedgeSqr = MiddleSqr;
						_iBestLedge = _iRoot;
						_LedgePos = LedgeDot;
					}
					/*else if ((HeightDiff > LEDGE_HEIGHTTHRESHOLD) && 
						(HeightDiff < 90.0f) &&
						((Dot < -WOBJ_LEDGE_DIRECTIONSTRICTNESS) || (Dot > WOBJ_LEDGE_DIRECTIONSTRICTNESS)) &&
						(LedgeType == CWOBJECT_LEDGE_TYPEMEDIUM))
					{
						// Hmm, ok we got a semi high ledge, ie we want to enter from air...
						_BestLedgeType = CWOBJECT_LEDGE_TYPEHIGH | CWOBJECT_LEDGE_ENTERTYPELEVELED;
						_BestLedgeSqr = MiddleSqr;
						_iBestLedge = _iRoot;
						_LedgePos = LedgeDot;
					}*/
				}
			}
		}
	}
}

// Find best "real" ledge to begin testing with, no dynamic ledges should get in here...
void CWObject_Ledge::FindBestLedgePseudo(const struct TestVars& _Vars,const int32& _iRoot,/*const CVec3Dfp32& _CharPos, const CVec2Dfp32& _CharDir2D, 
									const fp32& _ZDir, const int& _iRoot, const fp32& _RangeSqr, 
								   const bool& _bAirborne, const fp32& _Width, const CVec3Dfp32& _DynPos,*/
								   fp32& _BestLedgeSqr, int& _iBestLedge, int& _BestLedgeType, fp32& _LedgePos)
{
	MAUTOSTRIP( CWObject_Ledge_FindBestLedgePseudo, MAUTOSTRIP_VOID );
	// Ok, start at the root and find best ledge on the way
	fp32 NodeRadiusSqr = (fp32)m_lLedgeBinTree[_iRoot].GetRadius();
	NodeRadiusSqr *= NodeRadiusSqr;
	if (DoesIntersect(m_lLedgeBinTree[_iRoot].GetMidpoint(),
		NodeRadiusSqr,_Vars.m_CharPos, _Vars.m_RangeSqr) != -1)
	{
		int iLeft = m_lLedgeBinTree[_iRoot].GetBinLeft();
		int iRight = m_lLedgeBinTree[_iRoot].GetBinRight();
		// Check left/right trees
		if (iLeft != -1)
		{
			if (m_lLedgeBinTree[_iRoot].LeftIsLeaf())
			{
				FindBestLedge(_Vars,iLeft,/*_CharPos, _CharDir2D, _ZDir, iLeft, _RangeSqr, _bAirborne, _Width, _DynPos,*/_BestLedgeSqr, 
					_iBestLedge, _BestLedgeType, _LedgePos);
			}
			else
			{
				FindBestLedgePseudo(_Vars,iLeft,/*_CharPos, _CharDir2D, _ZDir, iLeft, _RangeSqr, _bAirborne, _Width, _DynPos,*/_BestLedgeSqr, 
					_iBestLedge, _BestLedgeType, _LedgePos);
			}
		}
		if (iRight != -1)
		{
			if (m_lLedgeBinTree[_iRoot].RightIsLeaf())
			{	
				FindBestLedge(_Vars,iRight,/*_CharPos, _CharDir2D, _ZDir, iRight, _RangeSqr, _bAirborne, _Width, _DynPos,*/_BestLedgeSqr, 
					_iBestLedge, _BestLedgeType, _LedgePos);
			}
			else
			{
				FindBestLedgePseudo(_Vars,iRight,/*_CharPos, _CharDir2D, _ZDir, iRight, _RangeSqr, _bAirborne, _Width, _DynPos,*/_BestLedgeSqr, 
					_iBestLedge, _BestLedgeType, _LedgePos);
			}
		}
	}
}

// Find what type of ledge we are dealing with (low/high)
#define SOMEVALUE 1.4142135623730950488016887242097f
int CWObject_Ledge::FindLedgeType(const CVec3Dfp32& _LedgePoint, const CVec3Dfp32& _LedgeDir,
								  const CVec3Dfp32& _Normal, const fp32& _Width, int16 _iExclude)
{
	MAUTOSTRIP( CWObject_Ledge_FindLedgeType, CWOBJECT_LEDGE_TYPELOW );
	// Use a bounding box test below the wanted ledgepoint and determine ledgetype from there...
	CVec3Dfp32 LedgePoint;
	int Type = 0;

	fp32 WidthOffset;
	{
		// Find offset from ledge
		WidthOffset = (_Width * 0.5f + 1.0f);
		
		fp32 DotX = _Normal.k[0];// * CVec3Dfp32(1.0f,0.0f,0.0f);
		fp32 DotY = _Normal.k[1];// * CVec3Dfp32(0.0f,1.0f,0.0f);
		fp32 DotB = Min(M_Fabs(DotX),M_Fabs(DotY));
		fp32 B = DotB * (_Width * 0.5f + 1.0f);
		fp32 C2 = M_Sqrt(Sqr(B * SOMEVALUE) + Sqr(WidthOffset));

		WidthOffset = C2 + B * (WidthOffset - B) / C2;
	}

	LedgePoint = _LedgePoint - CVec3Dfp32(0,0,LEDGETYPE_HEIGHT_MEDIUM*0.5f) + 
		_Normal * WidthOffset;
	
	// First test medium ledge, if that collides it's a low ledge, if that doesn't collide but high does
	// it's a medium ledge, if that doesn't collide it's a high ledge..
	TSelection<CSelection::LARGE_BUFFER> Selection;

	CWO_PhysicsState PhysState;
	PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f),0);
	PhysState.m_nPrim = 1;
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
	PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
	//PhysState.m_PhysFlags = 0;
	// Intersect doors and doors/platforms
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_PHYSOBJECT;
	PhysState.m_iExclude = _iExclude;
	
	m_pWServer->Selection_AddIntersection(Selection,LedgePoint,PhysState);
	const int16* pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);

	// Remove any dead chars from selection
	int NumSel = nSel;
	for (int32 i = 0; i < NumSel; i++)
	{
		CWObject_CoreData* pCollObj = m_pWServer->Object_GetCD(pSel[i]);
		if (pCollObj && (pCollObj->m_iObject == _iExclude || (CWObject_Character::Char_GetPhysType(pCollObj) == PLAYER_PHYS_DEAD)))
			nSel--;
	}

	// Debug draw the bitching testbox!!!
 	{
 		int32 Color = (nSel > 0 ? 0xffff0000 : 0xff00ff00);
		CBox3Dfp32 Box;
		Box.m_Min = LedgePoint - CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f);
		Box.m_Max = LedgePoint + CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f);
		m_pWServer->Debug_RenderAABB(Box,Color,10000.0f);
	}

	// If we are colliding with something it's a low ledge, otherwise check if it's a medium or a
	// high ledge
	if (nSel > 0)
	{
		Type = CWOBJECT_LEDGE_TYPELOW;
	}
	else
	{
		// Check type high
		LedgePoint = _LedgePoint - CVec3Dfp32(0,0,LEDGETYPE_HEIGHT_HIGH*0.5f) + 
			_Normal * WidthOffset;
		PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_HIGH*0.5f),0);
		m_pWServer->Selection_AddIntersection(Selection, LedgePoint, PhysState);
		//m_pWServer->Selection_AddBoundBox(Selection,OBJECT_FLAGS_PROJECTILE,Box.m_Min,Box.m_Max);

		// If we collide with something now it's a medium ledge, otherwise a high ledge
		nSel = m_pWServer->Selection_Get(Selection, &pSel);
		// Remove any dead chars from selection
		NumSel = nSel;
		for (int32 i = 0; i < nSel; i++)
		{
			CWObject_CoreData* pCollObj = m_pWServer->Object_GetCD(pSel[i]);
			if (pCollObj && (pCollObj->m_iObject == _iExclude || (CWObject_Character::Char_GetPhysType(pCollObj) == PLAYER_PHYS_DEAD)))
				nSel--;
		}
		Type = (nSel > 0 ? CWOBJECT_LEDGE_TYPEMEDIUM : CWOBJECT_LEDGE_TYPEHIGH);
	}

	return Type;
}

// Similar to FindLedgeType but with only one test..
bool CWObject_Ledge::CanClimbUp(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _LedgePoint, const CVec3Dfp32& _LedgeDir,
								  const CVec3Dfp32& _Normal, const fp32& _Width, const fp32& _Height)
{
	MAUTOSTRIP( CWObject_Ledge_CanClimbUp, false );
	// Use a bounding box test above the wanted ledgepoint and determine if we can climb up there...
	CVec3Dfp32 LedgePoint;
	fp32 QuarterWidth = _Width*0.25f;
	fp32 QuarterHeight = _Height * 0.25f + 1.0f;

	LedgePoint = _LedgePoint + CVec3Dfp32(0,0,(QuarterHeight + 1.0f)) - 
		_Normal * (QuarterWidth + 1.0f);

	TSelection<CSelection::LARGE_BUFFER> Selection;

	CWO_PhysicsState PhysState;
	PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, 
			CVec3Dfp32(QuarterWidth, QuarterWidth, QuarterHeight),0);
	PhysState.m_nPrim = 1;
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
	PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
	//PhysState.m_PhysFlags = 0;
	// Intersect doors and doors/platforms
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_PLAYERPHYSMODEL;

	CBox3Dfp32 SomeBox;
	SomeBox.m_Min = LedgePoint - CVec3Dfp32(QuarterWidth, QuarterWidth, QuarterHeight);
	SomeBox.m_Max = LedgePoint + CVec3Dfp32(QuarterWidth, QuarterWidth, QuarterHeight);

/*#ifndef M_RTM
	_pWPhys->Debug_RenderAABB(SomeBox,0xffff0000,10.0f,false);
	_pWPhys->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
	_pWPhys->Debug_RenderAABB(SomeBox,0xff000000,10.0f,false);
#endif*/
	
	_pWPhys->Selection_AddIntersection(Selection, LedgePoint, PhysState);

	const int16* pSel;
	int nSel = _pWPhys->Selection_Get(Selection, &pSel);

	// Remove any dead chars from selection
	for (int32 i = 0; i < nSel; i++)
	{
		CWObject_CoreData* pCollObj = _pWPhys->Object_GetCD(pSel[i]);
		if (pCollObj && (CWObject_Character::Char_GetPhysType(pCollObj) == PLAYER_PHYS_DEAD))
			nSel--;
	}
	
	return (nSel == 0);
}

void CWObject_Ledge::PackLedgeData(const CVec3Dfp32& _MidPoint, const CVec3Dfp32& _Normal, 
								   fp32 _Radius)
{
	MAUTOSTRIP( CWObject_Ledge_PackLedgeData, MAUTOSTRIP_VOID );
	// No pack of midpoint, need all info...
	Data(1) = *(uint32*)&_MidPoint[0];
	Data(2) = *(uint32*)&_MidPoint[1];
	Data(3) = *(uint32*)&_MidPoint[2];
	Data(4) = *(uint32*)&_Radius;
	uint32 Packed = _Normal.Pack32(1.01f);
	Data(5) = Packed;
	Data(6) = m_LedgeFlags;
}

void CWObject_Ledge::UnPackLedgeData(CWObject_CoreData* _pObj, CVec3Dfp32& _Position1, 
									 CVec3Dfp32& _Position2, CVec3Dfp32& _Normal, fp32& _Length,
									 int& _LedgeFlags)
{
	MAUTOSTRIP( CWObject_Ledge_UnPackLedgeData, MAUTOSTRIP_VOID );
	// No pack of midpoint, need all info...
	CVec3Dfp32 Midpoint, LedgeDir;
	fp32 Radius;
	Midpoint[0] = *(fp32*)&_pObj->m_Data[1];
	Midpoint[1] = *(fp32*)&_pObj->m_Data[2];
	Midpoint[2] = *(fp32*)&_pObj->m_Data[3];
	Radius = *(fp32*)&_pObj->m_Data[4];
	_Length = Radius * 2.0f;
	uint32 Packed;
	Packed = *(uint32*)&(_pObj->m_Data[5]);
	_Normal.Unpack32(Packed,1.01f);
	_Normal.Normalize();
	CVec3Dfp32(0,0,1).CrossProd(_Normal,LedgeDir);
	_Position1 = Midpoint - LedgeDir * Radius;
	_Position2 = Midpoint + LedgeDir * Radius;
	
	// Get ledgeflags
	_LedgeFlags = _pObj->m_Data[6];

	// If the ledge is dynamic make the positions "unrelative"
	/*if (_LedgeFlags & CWOBJECT_LEDGE_DYNAMIC)
	{
		_Position1 += _pObj->GetPosition();
		_Position2 += _pObj->GetPosition();
	}*/
}

void CWObject_Ledge::InvalidateLedgeData()
{
	MAUTOSTRIP( CWObject_Ledge_InvalidateLedgeData, MAUTOSTRIP_VOID );
	// Invalidate the data that is used when a ledge is grabbed
	Data(1) = -1;
	Data(2) = -1;
	Data(3) = -1;
	Data(4) = -1;
	Data(5) = -1;
	// ?
	Data(6) = 0;
}

#ifdef _DEBUG
enum
{
	CIRCLEDIR_XY,
	CIRCLEDIR_XZ,
	CIRCLEDIR_YZ,
};
void DrawCircle(CWorld_PhysState* _pWPhys, CVec3Dfp32 _StartPos, int _Res, fp32 _Size, int _Dir,
			   int _Color = 0xffffffff)
{
	MAUTOSTRIP( DrawCircle, MAUTOSTRIP_VOID );
	int Dir1,Dir2;
	CVec3Dfp32 MainAxis;
	CWireContainer* pWire = _pWPhys->Debug_GetWireContainer();
	if (!pWire)
		return;
	switch (_Dir)
	{
	case CIRCLEDIR_XY:
		{
			Dir1 = 0;
			Dir2 = 1;
			MainAxis = CVec3Dfp32(1.0f,0.0f,0.0f);
			break;
		}
	case CIRCLEDIR_XZ:
		{
			Dir1 = 0;
			Dir2 = 2;
			MainAxis = CVec3Dfp32(1.0f,0.0f,0.0f);
			break;
		}
	case CIRCLEDIR_YZ:
		{
			Dir1 = 1;
			Dir2 = 2;
			MainAxis = CVec3Dfp32(0.0f,1.0f,0.0f);
			break;
		}
	default:
		{
			Dir1 = 0;
			Dir2 = 1;
			MainAxis = CVec3Dfp32(1.0f,0.0f,0.0f);
			break;
		}
	}

	CVec3Dfp32 Dir(0,0,0);
	
	fp32 Frag = _PI2 / _Res;

	CVec3Dfp32 PrevPos = _StartPos + MainAxis * _Size;

	for (int i = 0; i < _Res; i++)
	{
		fp32 Angle = Frag * i;
		Dir.k[Dir1] = M_Cos(Angle);
		Dir.k[Dir2] = M_Sin(Angle);
		
		CVec3Dfp32 Pos = Dir * _Size + _StartPos;
		pWire->RenderWire(PrevPos, Pos, _Color, 0.1f, false);
		PrevPos = Pos;
	}

	pWire->RenderWire(PrevPos, _StartPos + MainAxis * _Size, _Color, 0.1f, false);
}
#endif

#ifdef PHOBOS_DEBUG
void CWObject_Ledge::DebugDraw()
{
	MAUTOSTRIP( CWObject_Ledge_DebugDraw, MAUTOSTRIP_VOID );
	int ColorLedge = 0xff00ff00;
	int ColorNormal = 0xff0000ff;
	int ColorLinkLeft = 0xffff0000;
	int ColorLinkRight = 0xffffff00;
	//const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	int Len = m_lLedges.Len();
	for (int i = 0; i < Len; i++)
	{
		CVec3Dfp32 Normal,LeftPoint,RightPoint,MidPoint;
		Normal = m_lLedges[i].GetNormal();
		LeftPoint = m_lLedges[i].GetLeftPoint();
		RightPoint = m_lLedges[i].GetRightPoint();
		if (m_lLedges[i].GetLedgeFlags() & CWOBJECT_LEDGE_DYNAMIC)
			GetDynamicLedgePos(m_pWServer,this,LeftPoint,RightPoint,Normal);

		CVec3Dfp32 Adjust = (m_lLedges[i].GetNormal() + CVec3Dfp32(0,0,1)) * 0.5f;
		m_pWServer->Debug_RenderWire(Adjust + LeftPoint,Adjust + RightPoint,ColorLedge);
		MidPoint = (Adjust * 2 + LeftPoint + RightPoint) * 0.5f;
		m_pWServer->Debug_RenderWire(MidPoint, MidPoint + Normal * 20,ColorNormal);
		
		// Draw coll circle
		DrawCircle(m_pWServer, MidPoint, 30, m_lLedges[i].GetRadius(), CIRCLEDIR_XY);
		DrawCircle(m_pWServer, MidPoint, 30, m_lLedges[i].GetRadiusLocal(), CIRCLEDIR_XY);

		// Draw lines to children...
		/*int LinkLeft = m_lLedges[i].GetBinLeft();
		int LinkRight = m_lLedges[i].GetBinRight();
		if (LinkLeft != -1)
		{
			CVec3Dfp32 MidLeft = m_lLedges[LinkLeft].GetMidpoint();
			m_pWServer->Debug_RenderWire(MidPoint, MidLeft,ColorLinkLeft);
		}
		if (LinkRight != -1)
		{
			CVec3Dfp32 MidRight = m_lLedges[LinkRight].GetMidpoint();
			m_pWServer->Debug_RenderWire(MidPoint, MidRight,ColorLinkRight);
		}*/
	}

	// Draw pseudotree
	Len = m_lLedgeBinTree.Len();
	for (int i = 0; i < Len; i++)
	{
		// Draw midpoint
		CVec3Dfp32 Midpoint = m_lLedgeBinTree[i].GetMidpoint();
		m_pWServer->Debug_RenderVertex(Midpoint, ColorLedge);
		
		// Draw coll circle
		DrawCircle(m_pWServer, m_lLedgeBinTree[i].GetMidpoint(), 30, m_lLedgeBinTree[i].GetRadius(), CIRCLEDIR_XY);

		// Draw lines to children...
		int LinkLeft = m_lLedgeBinTree[i].GetBinLeft();
		int LinkRight = m_lLedgeBinTree[i].GetBinRight();
		if (LinkLeft != -1)
		{
			CVec3Dfp32 MidLeft = (m_lLedgeBinTree[i].LeftIsLeaf() ? 
				m_lLedges[LinkLeft].GetMidpoint() : m_lLedgeBinTree[LinkLeft].GetMidpoint());
			m_pWServer->Debug_RenderWire(Midpoint, MidLeft,ColorLinkLeft);
		}
		if (LinkRight != -1)
		{
			CVec3Dfp32 MidRight = (m_lLedgeBinTree[i].RightIsLeaf() ? 
				m_lLedges[LinkRight].GetMidpoint() : m_lLedgeBinTree[LinkRight].GetMidpoint());
			m_pWServer->Debug_RenderWire(Midpoint, MidRight,ColorLinkRight);
		}
	}
}
#endif


CVec3Dfp32 CWObject_Ledge::GetOptCharPos(const CVec3Dfp32& _Point, const CVec3Dfp32& _Normal)
{
	MAUTOSTRIP( CWObject_Ledge_GetOptCharPos, CVec3Dfp32(0) );
	// Find height from ground FIXME
	/*fp32 BBOffset = _Width;
	BBOffset = M_Sqrt(BBOffset * BBOffset * 2);*/
	CVec3Dfp32 OptPos = _Point + _Normal * LEDGE_OPTIMALDISTANCE - CVec3Dfp32(0,0,LEDGE_OPTIMALHEIGHT);
	
	return OptPos;
}

/*CVec3Dfp32 CWObject_Ledge::GetOptCharPos(const CVec3Dfp32& _Point, const CVec3Dfp32& _Normal, 
	   const CVec3Dfp32& _Position1, const CVec3Dfp32& _LedgeDir, fp32 _LedgeLength, int32 _EndPoint)
{
	CVec3Dfp32 Point = _Point;

	if (_EndPoint != LEDGE_ENDPOINT_NOENDPOINT)
	{
		// Ok, must adjust to some endpoint
		switch (_EndPoint)
		{
			case LEDGE_ENDPOINT_INLEFT:
			case LEDGE_ENDPOINT_OUTLEFT:
			case LEDGE_ENDPOINT_LEFT:
				{
					Point = _Position1 + _LedgeDir * LEDGE_OPTIMALDISTANCE;
					break;
				}
			case LEDGE_ENDPOINT_INRIGHT:
			case LEDGE_ENDPOINT_OUTRIGHT:
			case LEDGE_ENDPOINT_RIGHT:
				{
					Point = _Position1 + _LedgeDir * (_LedgeLength - LEDGE_OPTIMALDISTANCE);
					break;
				}
			default:
				break;
		}
	}
	
	return (Point + _Normal * LEDGE_OPTIMALDISTANCE - CVec3Dfp32(0,0,LEDGE_OPTIMALHEIGHT));
}*/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Ledge, CWObject_Ledge_Parent, 0x0100);

const CVec3Dfp32 CWObject_Ledge::CLedge::GetLeftPoint() const
{
	// |---*---
	return m_Midpoint - m_LedgeDirection * m_RadiusLocal;
}

const CVec3Dfp32 CWObject_Ledge::CLedge::GetRightPoint() const
{
	// ---*---|
	return m_Midpoint + m_LedgeDirection * m_RadiusLocal;
}

const CVec3Dfp32 CWObject_Ledge::CLedge::GetNormal() const
{
	CVec3Dfp32 Normal;
	m_LedgeDirection.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Normal);
	
	return Normal;
}
