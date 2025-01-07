#ifndef _INC_AICORE_BIGALIEN
#define _INC_AICORE_BIGALIEN

#include "../AICore.h"

class CAI_Core_BigAlien : public CAI_Core
{
public:
	//Constants
	
	//Messages
	enum {
		OBJMSG_GETACTIVITYMODE = OBJMSGBASE_BIGALIEN,
	};

	//Activity mode
	enum {
		MODE_DEFAULT = 0,
		MODE_SLEEPING,
	};

protected:
	//Activity mode
	int8 m_ActivityMode;

	//Big alien should be immune until this tick
	int32 m_ImmuneTick;	

	//Are we using move device to look?
	bool m_bLooking;

public: //for now
	//Methods

	//Get type of AI
	virtual int GetType();

	//Methods for getting some of the bots physical values (actually not; these values are approximations used for pathfinding etc)
	/*
	virtual fp32 GetBaseSize();
	virtual fp32 GetHeight();
	virtual fp32 GetCrouchHeight();
	virtual fp32 GetStepHeight();
	virtual fp32 GetJumpHeight();
	*/
	virtual fp32 GetWalkStepLength();
	virtual fp32 GetRunStepLength();

	//Alien should never look, only turn
	virtual void AddFacePositionCmd(const CVec3Dfp32& _Pos);
	virtual void AddAimAtPositionCmd(const CVec3Dfp32& _Pos);
	virtual void OnLook(fp32 _Heading = _FP32_MAX, fp32 _Pitch = _FP32_MAX, int _Time = 1, bool _bLookSoft = false, bool _bMaintainLook = false);
	virtual void OnTrack(const CVec3Dfp32& _Pos, int _Time, bool bHeadingOnly = false, bool _bLookSoft = false);

	//Adds commands to move towards the given position, at the given speed or full speed by default
	virtual void AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed = -1.0f, bool _bPerfectPlacement = false);

	//Checks if we've gotten stuck, given the destination
	virtual bool IsStuck(const CVec3Dfp32& _Dest);

	//Make moves to escape from a "stuck" position
	virtual void OnEscapeSequenceMove(fp32 _Speed);

	//Move to position. Free mode device if it's been used for looking
	virtual bool OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial = false, bool _bPerfectPlacement = false);

	//Act frustrated
	virtual void OnFrustration();

	//Handles impulses.
	virtual bool OnImpulse(int _iImpulse, int _iSender = 0, int _iParam = 0, const CVec3Dfp32& _VecParam = 0, void * _pData = NULL);

public:
	//Interface

	//Copy constructor
	CAI_Core_BigAlien(CAI_Core * _pAI_Core);

	//Initializer
	virtual void Init(CWObject * _pCharacter, CWorld_Server * _pServer);

	//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
	//but information is gathered.
	virtual void OnRefresh(bool _bPassive = false);

	//Handles incoming messages
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Handles any registry keys of a character that are AI-related
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Load/save
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};


#endif