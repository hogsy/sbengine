/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:		WObj_Model_Ladder.h	
					
	Author:			Olle Rosenquist
					
	Copyright:	Copyright O3 Games AB 2002	
					
	Contents:	WObj_Model_Ladder class definition
					
	Comments:		
					
	History:		
		020905:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_PHYS_LADDER
#define _INC_WOBJ_PHYS_LADDER

#include "WObj_ActionCutscene.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
#include "../WObj_Messages.h"

enum 
{
	OBJMSG_LADDER_GETPOSITION1 = OBJMSGBASE_MISC_LADDER,
	OBJMSG_LADDER_GETPOSITION2,
	OBJMSG_LADDER_GETNORMAL,
	OBJMSG_LADDER_SETPOSITION1,
	OBJMSG_LADDER_SETPOSITION2,
	OBJMSG_LADDER_SETNORMAL,
	OBJMSG_LADDER_GETTYPE,
	OBJMSG_LADDER_INACTIVATELADDERCAM,
	OBJMSG_LADDER_ISLADDER,
	OBJMSG_LADDER_ISHANGRAIL,
	OBJMSG_LADDER_GETCURRENTENDPOINT,
	OBJMSG_LADDER_CONFIGUREHANGRAIL,
	OBJMSG_LADDER_GETHANGRAILMODE,
	CWOBJECT_LADDER_TYPE_HANGRAIL,
	CWOBJECT_LADDER_TYPE_LADDER,
	CWOBJECT_LADDER_TYPE_LEDGE,
	OBJMSG_LADDER_FINDENDPOINT,
	OBJMSG_LADDER_FINDENDPOINTOFFSET,
	OBJMSG_LADDER_SETCURRENTENDPOINT,
	OBJMSG_LADDER_FINDSTEPOFFANIMTYPE,
	OBJMSG_LADDER_GETCHARLADDERPOS,

	LADDERPOSITION1 = 4,
	LADDERPOSITION2 = 5,
	LADDERNORMAL = 6,
	LADDERENDPOINT = 7,

	LADDER_ENDPOINT_NOENDPOINT = 0,
	LADDER_ENDPOINT_BELOW = 1,
	LADDER_ENDPOINT_ABOVE = 2,

	LADDER_STEPOFFTYPE_4PLUS	= 3,
	LADDER_STEPOFFTYPE_0		= 4, 
	LADDER_STEPOFFTYPE_4MINUS	= 5,
	LADDER_STEPOFFTYPE_8MINUS	= 6,
	LADDER_STEPOFFTYPE_12MINUS	= 7,

	LADDER_ENDPOINTTYPE_LEFT4PLUS	= 1,
	LADDER_ENDPOINTTYPE_LEFT0,
	LADDER_ENDPOINTTYPE_LEFT4MINUS,
	LADDER_ENDPOINTTYPE_LEFT8MINUS,
	LADDER_ENDPOINTTYPE_LEFT12MINUS,
	LADDER_ENDPOINTTYPE_RIGHT4PLUS,
	LADDER_ENDPOINTTYPE_RIGHT0,
	LADDER_ENDPOINTTYPE_RIGHT4MINUS,
	LADDER_ENDPOINTTYPE_RIGHT8MINUS,
	LADDER_ENDPOINTTYPE_RIGHT12MINUS,

	HANGRAIL_ENTERTYPE_FROMGROUND = 0,
	HANGRAIL_ENTERTYPE_FROMAIR = 1,

	LEDGE_ENDPOINT_NOENDPOINT = 0,
	LEDGE_ENDPOINT_INLEFT = 3,
	LEDGE_ENDPOINT_OUTLEFT = 4,
	LEDGE_ENDPOINT_INRIGHT = 5,
	LEDGE_ENDPOINT_OUTRIGHT = 6,
	LEDGE_ENDPOINT_LEFT = 7,
	LEDGE_ENDPOINT_RIGHT = 8,

	LADDER_ENDPOINT_MASK = 0x0000ffff,
	LADDER_HANDRAIL_MODEFLIPPED = 0x00010000,

	LADDER_IMPULSE_FORCEGRABLADDER = 10,
};

#define LADDER_STEPOFFHEIGHT_0 (0.0f)
#define LADDER_STEPOFFHEIGHT_4 (4.0f)
#define LADDER_STEPOFFHEIGHT_8 (8.0f)
#define LADDER_STEPOFFHEIGHT_12 (12.0f)
#define LADDER_STEPOFFHEIGHT_16 (16.0f)

#define CWObject_Phys_Ladder_Parent CWObject_ActionCutscene
class CWObject_Phys_Ladder : public CWObject_Phys_Ladder_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	CVec3Dfp32	m_Position1;
	CVec3Dfp32	m_Position2;
	CVec3Dfp32	m_Normal;

	int32		m_StepOffAnimType;

	virtual bool OnEndACS(int _iCharacter);

	static void SetUserData(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position);
	static void SetUserDataNormal32(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position);
	//static void SetUserDataVector32(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position);
	static void SetUserDataVector32MA(CWObject_CoreData* _pObj, const CVec3Dfp32& _Data, int _Position);
	static void GetUserData(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position);
	static void GetUserDataNormal32(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position);
	//static void GetUserDataVector32(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position);
	static void GetUserDataVector32MA(CWObject_CoreData* _pObj, CVec3Dfp32& _Data, int _Position);

	static void GetLadderEndpoint(CWObject_CoreData* _pObj, int& _EndPoint);
	static void SetLadderEndpoint(CWObject_CoreData* _pObj, const int& _EndPoint);
	static void GetHangrailMode(CWObject_CoreData* _pObj, int& _Mode);
	static void SetHangrailMode(CWObject_CoreData* _pObj, const int& _Mode);

	virtual bool DoActionSuccess(int _iCharacter);
	virtual spCActionCutsceneCamera GetActionCutsceneCamera(int _iObject, int _iCharacter, 
		int _Specific = -1);

	static int FindLadderPoint(const CVec3Dfp32& _LadderDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLadder, const fp32& _LadderLength, 
		const fp32& _Offset1, const fp32& _Offset2, fp32& _LadderDot);
	static fp32 FindLadderPointOffset(const CVec3Dfp32& _LadderDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLadder, fp32 _LadderLength, fp32 _Offset1, 
		fp32 _Offset2);

	static int FindStepOffType(CWorld_PhysState* _pPhysState, const CVec3Dfp32& _LadderEnd, 
		const CVec3Dfp32& _Normal);
	static int32 FindStepOffAnimType(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _LadderPos1, 
		const CVec3Dfp32& _LadderPos2,const CVec3Dfp32& _Normal);

	static fp32 FindStepOffTestHeight(int32 _Type);

	virtual bool IsACS() { return false;	}
public:
	CWObject_Phys_Ladder();
	virtual void OnCreate();
	//virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, 
		const CWObject_Message& _Msg);

	virtual void TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS);

	static bool GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, const CVec3Dfp32& _Move, 
		const CMat4Dfp32& _MatLook, const uint32& _Press, uint32& _Released, 
		CWO_Character_ClientData* _pCD, int16& _Flags);

	// Grab the ladder/hangrail
	static void GrabLadder(int32 _iLadder, int32 _Type, CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, bool bForceInAir = false);

	static bool MakeLadderMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _Move);

	// Get ladder positon[0,1], 0 is bottom 1 is top
	static bool GetRelativeLadderPosition(CWorld_PhysState* _pWPhys,CWO_Character_ClientData* _pCD, const CVec3Dfp32& _CharPos, CMTime& _Value);
	static CVec3Dfp32 ReadVector(CStr _Str);
};
#endif
