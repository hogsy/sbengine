/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Ledge.h
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright Starbreeze AB 2002
					
	Contents:		
					
	Comments:		
					
	History:		
		021107:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_LEDGE
#define _INC_WOBJ_LEDGE

//#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
//#include "../WObj_Messages.h"
#include "WObj_Model_Ladder.h"
#include "../WFixedCharMovement.h"

#define WOBJ_LEDGE_DEFAULTGRABDISTANCE (20.0f)
#define WOBJ_LEDGE_HEIGHTTOLERANCE (10.0f)
#define WOBJ_LEDGE_DIRECTIONSTRICTNESS (0.5f)
#define LEDGE_GRABDISTANCE (70.0f)
#define LEDGE_DEFAULTACTIVATIONRANGE (70.0f)
#define LEDGE_MAXVELOCITY (5.0f)
#define LEDGE_PHYS_WIDTH (8)

class CWObject_Character;
enum 
{
	OBJMSG_LEDGE_GETFLAGS = OBJMSGBASE_MISC_LEDGE,
	OBJMSG_LEDGE_ISLEDGE,
	OBJMSG_LEDGE_CANGRAB,
	OBJMSG_LEDGE_GRABLEDGE,
	OBJMSG_LEDGE_SWITCHLEDGE,
	OBJMSG_LEDGE_FINDBESTLEDGE,
	OBJMSG_LEDGE_GETACTIVELEDGEINFO,
	OBJMSG_LEDGE_CANCLIMBUP,
	OBJMSG_LEDGE_GETCHARLEDGEPOS,
	
	/*CWOBJECT_LEDGE_CANMOVEUP		= 1 << 0,
	CWOBJECT_LEDGE_CANMOVEDOWN		= 1 << 1,
	CWOBJECT_LEDGE_CANMOVELEFT		= 1 << 2,
	CWOBJECT_LEDGE_CANMOVERIGHT		= 1 << 3,*/
	CWOBJECT_LEDGE_DYNAMIC			= 1 << 0,
	CWOBJECT_LEDGE_TYPEHIGH			= 1 << 1,
	CWOBJECT_LEDGE_TYPEMEDIUM		= 1 << 2,
	CWOBJECT_LEDGE_TYPELOW			= 1 << 3,
	CWOBJECT_LEDGE_LOOP				= 1 << 4, // When creating from enginepath last + first is a ledge

	LEDGE_LINKMODE_LEFTIN			= 1 << 5,
	LEDGE_LINKMODE_LEFTOUT			= 1 << 6,
	LEDGE_LINKMODE_RIGHTIN			= 1 << 7,
	LEDGE_LINKMODE_RIGHTOUT			= 1 << 8,

	LEDGE_LINKMODE_MASK				= (LEDGE_LINKMODE_LEFTIN | LEDGE_LINKMODE_LEFTOUT | 
										LEDGE_LINKMODE_RIGHTIN | LEDGE_LINKMODE_RIGHTOUT),

	CWOBJECT_LEDGE_TYPEMASK			= (CWOBJECT_LEDGE_TYPEHIGH | CWOBJECT_LEDGE_TYPELOW | CWOBJECT_LEDGE_TYPEMEDIUM),

	CWOBJECT_LEDGE_ENTERTYPEABOVELEDGE	= 1 << 9,
	CWOBJECT_LEDGE_ENTERTYPEBELOWLEDGE	= 1 << 10,
	CWOBJECT_LEDGE_ENTERTYPELEVELED		= 1 << 11,

	CWOBJECT_LEDGE_ISMAINLEDGE			= 1 << 12,
	
	// Node inserted into collision tree for a more effective view
	CWOBJECT_LEDGE_ISPSEUDONODE			= 1 << 13,

	// Use alternative animation for climbing up
	CWOBJECT_LEDGE_USEALTCLIMBUP		= 1 << 14,

	CWOBJECT_LEDGE_PSEUDOLEFTISLEAF		= 1 << 0,
	CWOBJECT_LEDGE_PSEUDORIGHTISLEAF	= 1 << 1,

	CWOBJECT_LEDGE_LINK_UNDEFINED = -1,

	LEDGE_LINKMODE_NONE					= 0,

	LEDGE_LINKMODE_LEFTMASK				= (LEDGE_LINKMODE_LEFTIN | LEDGE_LINKMODE_LEFTOUT),
	LEDGE_LINKMODE_RIGHTMASK			= (LEDGE_LINKMODE_RIGHTIN | LEDGE_LINKMODE_RIGHTOUT),
};

#define CWOBJECT_LEDGE_GRABTIMER (15)

// Should inherit from actioncutscene instead...
#define CWObject_Ledge_Parent CWObject_ActionCutscene
class CWObject_Ledge : public CWObject_Ledge_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	fp32			m_GrabDistance;
	// Global ledge flags...
	int			m_LedgeFlags;
	int32		m_LastSwitchTest;

	// Ledge instance
	class CLedge
	{
	protected:
		CVec3Dfp32	m_LedgeDirection;
		CVec3Dfp32	m_Midpoint;
		fp32			m_RadiusLocal;
		// Which ledges to link with (-1 if not linked)
		int16		m_iLedgeLeft;
		int16		m_iLedgeRight;
		// Linkmode/etc...
		uint16		m_LedgeFlags;

		// Hierarchical bintree collision stuffs (to find ledge faster)
		uint16		m_Radius;
		int16		m_iBinLeft;
		int16		m_iBinRight;
	public:
		CLedge()
		{
			m_iLedgeLeft = -1;
			m_iLedgeRight = -1;
			m_iBinLeft = -1;
			m_iBinRight = -1;
		}
		const CVec3Dfp32 GetLeftPoint() const;
		const CVec3Dfp32 GetRightPoint() const;
		const CVec3Dfp32 GetNormal() const;
		
		MACRO_INLINEACCESS_RWEXT(BinLeft,m_iBinLeft,int16)
		MACRO_INLINEACCESS_RWEXT(BinRight,m_iBinRight,int16)
		MACRO_INLINEACCESS_RWEXT(LedgeLeft,m_iLedgeLeft,int16)
		MACRO_INLINEACCESS_RWEXT(LedgeRight,m_iLedgeRight,int16)
		MACRO_INLINEACCESS_RW(LedgeFlags,uint16)
		MACRO_INLINEACCESS_RW(RadiusLocal,fp32)
		MACRO_INLINEACCESS_RW(Radius,uint16)
		MACRO_INLINEACCESS_RW(Midpoint,CVec3Dfp32)
		MACRO_INLINEACCESS_RWEXT(LedgeDir,m_LedgeDirection,CVec3Dfp32)
	};

	class CBinNode
	{
	protected:
		CVec3Dfp32	m_Midpoint;
		uint16		m_Radius;
		uint16		m_Flags;
		int16		m_iBinLeft;
		int16		m_iBinRight;
	public:
		void Reset()
		{
			m_Midpoint = 0;
			m_Radius = 0;
			m_Flags = 0;
			m_iBinLeft = -1;
			m_iBinRight = -1;
		}
		M_INLINE bool LeftIsLeaf() { return (m_Flags & CWOBJECT_LEDGE_PSEUDOLEFTISLEAF) != 0; }
		M_INLINE bool RightIsLeaf() { return (m_Flags & CWOBJECT_LEDGE_PSEUDORIGHTISLEAF) != 0; }
		MACRO_INLINEACCESS_RWEXT(BinLeft,m_iBinLeft,int16)
		MACRO_INLINEACCESS_RWEXT(BinRight,m_iBinRight,int16)
		MACRO_INLINEACCESS_RW(Flags,uint16)
		MACRO_INLINEACCESS_RW(Radius,uint16)
		MACRO_INLINEACCESS_RW(Midpoint,CVec3Dfp32)
	};

	// In the main ledge we put all the ledge instances in this array
	TThinArray<CLedge>		m_lLedges;
	// Separate tree for connecting the ledges together into a big tree (so local ledges
	// won't be too big..->less intersect tests) the root of this tree is 0
	TThinArray<CBinNode>	m_lLedgeBinTree;
	int16				m_LedgeCount;
	int16				m_iDynamicRoot;

	// Calculate number of slots needed in ledge array
	int	CalculateLedgeArrayLength();

	// Grabs the ledge and enters a new control mode for the character
	bool GrabLedge(CWObject_CoreData* _pChar, int _LedgeMode, int _LedgeID, fp32 _LedgePos);

	static void MakeLedgeMove(const CWAG2I_Context* _pContext, CWObject_CoreData* _pObjLedge, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength);
	static bool GetLedgePoint(CMat4Dfp32& _Mat, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength);

	//static void OnDrop(int _iObject, CWO_Character_ClientData* _pCD);
	static int FindLedgePoint(const CVec3Dfp32& _LedgeDir, const CVec3Dfp32& _ObjPos,
		const CVec3Dfp32& _Pos1, CVec3Dfp32& _PointOnLedge, const fp32& _LedgeLength, 
		const fp32& _CharWidth, const int& _LinkMode, fp32& _LedgeDotLeft, fp32& _LedgeDotRight);

	// Find normal from the two ledge positions
	int FindLinkMode(const CVec3Dfp32& _LedgeDir, const CVec3Dfp32 _LinkNormal, bool _bLinkLeft);

	int BuildBinCollTree(TThinArray<CLedge>& _lLedges, int _Left, int _Right, bool _bMethod1 = false);
	uint16 InsertNode(TThinArray<CLedge>& _lLedges, int _Start, int _NewNode);
	uint16 InsertNode(TThinArray<CLedge>& _lLedges, TThinArray<CBinNode>& _lLedgeBinTree, int _Start, int _NewNode);
	fp32 FindRange(const TThinArray<CLedge>& _lLedges, const CVec3Dfp32& _MidPoint, int _iIndex);

	// Runtime find possible node(ledge) to grab, set nodeid to controlmodeparam
	// Finds closest ledge on the way
	struct TestVars
	{
		CVec3Dfp32 m_CharPos;
		CVec2Dfp32 m_CharDir2D;
		fp32 m_ZDir;
		fp32 m_RangeSqr;
		bool m_bAirborne;
		fp32 m_Width;
		CVec3Dfp32 m_DynPos;
		int16 m_iExclude;
	};

	void FindBestLedge(const struct TestVars& _Vars, const int32& _iRoot,/*const CVec3Dfp32& _CharPos, const CVec2Dfp32& _CharDir2D, const fp32& _ZDir,
			const int& _iRoot, const fp32& _RangeSqr, const bool& _bAirborne, 
			const fp32& _Width, const CVec3Dfp32& _DynPos,*/ fp32& _BestLedgeSqr, int& _iBestLedge, int& _BestLedgeType, 
			fp32& _LedgePos);
	void FindBestLedgePseudo(const struct TestVars& _Vars, const int32& _iRoot,/*const CVec3Dfp32& _CharPos, const CVec2Dfp32& _CharDir2D, const fp32& _ZDir,
			const int& _iRoot, const fp32& _RangeSqr, const bool& _bAirborne, 
			const fp32& _Width, const CVec3Dfp32& _DynPos,*/ fp32& _BestLedgeSqr, int& _iBestLedge, int& _BestLedgeType, 
			fp32& _LedgePos);
	// Check if two spheres intersect, returns the squared distance if they intersect, or -1 if
	// they don't
	M_INLINE fp32 DoesIntersect(const CVec3Dfp32& _Pos1, fp32 _RangeSqr1, 
		const CVec3Dfp32& _Pos2, fp32 _RangeSqr2)
	{
		fp32 DistSqr = (_Pos1 - _Pos2).LengthSqr();
		return (DistSqr < (_RangeSqr1 + _RangeSqr2) ? DistSqr : -1);
	}

	// Switch between ledges
	bool SwitchLedge(bool _bLeft, CWO_Character_ClientData* _pCD, int32 _iChar);
	bool CanSwitchLedge(bool _bLeft, CWO_Character_ClientData* _pCD, int32 _iChar, fp32& _RightLength);

	// Find what type of ledge we are dealing with (low/high)
	int FindLedgeType(const CVec3Dfp32& _LedgePoint, const CVec3Dfp32& _LedgeDir,
			const CVec3Dfp32& _Normal, const fp32& _Width, int16 _iExclude);

	// Put the neccessary information in m_Data when grabbing a ledge, this means that only one
	// person at a time can use a ledge, but that shouldn't be a problem in PB atleast
	void PackLedgeData(const CVec3Dfp32& _MidPoint, const CVec3Dfp32& _Normal, fp32 _Radius);
	static void UnPackLedgeData(CWObject_CoreData* _pObj,CVec3Dfp32& _Position1, CVec3Dfp32& _Position2, 
		CVec3Dfp32& _Normal, fp32& _Length, int& _LedgeFlags);
	void InvalidateLedgeData();
	static bool LedgeDataValid(CWObject_CoreData* _pObj) { return (_pObj->m_Data[3] != -1); }

	virtual bool OnEndACS(int _iCharacter);

	// Draws the ledges with "renderwire"
	void DebugDraw();

	virtual bool DoActionSuccess(int _iCharacter);
	virtual spCActionCutsceneCamera GetActionCutsceneCamera(int _iObject, 
		int _iCharacter, int _Specific = -1);

	// If we can climb up the ledge
	static bool CanClimbUp(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _LedgePoint, const CVec3Dfp32& _LedgeDir,
		const CVec3Dfp32& _Normal, const fp32& _Width, const fp32& _Height);

	static bool GetDynamicLedgePos(CWorld_PhysState* _pWPhys,CWObject_CoreData* _pLedge, CVec3Dfp32& _Pos1, CVec3Dfp32& _Pos2, CVec3Dfp32& _Normal);
	
	//virtual bool IsACS() { return false;	}
public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	MACRO_PHOBOSDEBUG(virtual void OnRefresh();)
	//static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient) {}
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, 
	const CWObject_Message& _Msg);

	static void OnPress(const CWAG2I_Context* _pContext, CWO_Character_ClientData* _pCD,int _ControlPress, int _ControlLastPress);

	//virtual void TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAGI, TArray<int32>& _liACS);

	/*static bool GetUserAccelleration(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, 
										  CWO_Character_ClientData* _pCD, fp32 _dTime);*/
	static bool GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, 
		const CVec3Dfp32& _Move, const CMat4Dfp32& _MatLook, CWO_Character_ClientData* _pCD, 
		int16& _Flags, fp32 _dTime);
	
	static int ResolveLedgeMode(CStr _Str);
	//int CanGrab(CWObject_CoreData* _pObj, CWObject_CoreData* _pChar, bool _bAirborne);
	// Don't render ledge (duh)
	static void OnClientRender(CWObject_Client*, CWorld_Client*, CXR_Engine*, const CMat4Dfp32&) { return; }

	static int FindBestLedge(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _CharPos, int16 _iChar,
		const CVec3Dfp32& _CharDir, const fp32& _Range, const bool& _bAirBorne, const fp32& _CharWidth,
		int& _iBestLedge, int& _iBestLedgeType, fp32& _LedgePos);

	// Find the ledge that contain all the other ledges (except the dynamic ones)
	static CWObject_Ledge* FindMainLedge(CWorld_PhysState* _pWPhys);

	static bool MakeLedgeMove(const CWAG2I_Context* _pContext, int32 _Move);

	static CVec3Dfp32 GetOptCharPos(const CVec3Dfp32& _Point, const CVec3Dfp32& _Normal);
	/*static CVec3Dfp32 GetOptCharPos(const CVec3Dfp32& _Point, const CVec3Dfp32& _Normal, 
		const CVec3Dfp32& _Position1, const CVec3Dfp32& _LedgeDir, fp32 _LedgeLength, int32 _EndPoint);*/
};
#endif
