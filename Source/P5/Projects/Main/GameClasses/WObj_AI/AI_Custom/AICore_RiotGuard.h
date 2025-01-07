#ifndef _INC_AICORE_RIOTGUARD
#define _INC_AICORE_RIOTGUARD

#include "../AICore.h"

class CAI_Core_RiotGuard : public CAI_Core
{
public:
	//Constants
	
	//Messages

	//Activity mode

	//Animgraph modes

protected:
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

	//Riotguard should never look, only turn
	virtual void AddFacePositionCmd(const CVec3Dfp32& _Pos);
	virtual void AddAimAtPositionCmd(const CVec3Dfp32& _Pos);
	virtual void OnLook(fp32 _Heading = _FP32_MAX, fp32 _Pitch = _FP32_MAX, int _Time = 1);
	virtual void OnTrack(const CVec3Dfp32& _Pos, int _Time, bool bHeadingOnly = false);

	//Adds commands to move towards the given position, at the given speed or full speed by default
	virtual void AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed = -1.0f, bool _bPerfectPlacement = false);

	//Checks if we've gotten stuck, given the destination
	virtual bool IsStuck(const CVec3Dfp32& _Dest);

	//Make moves to escape from a "stuck" position
	virtual void OnEscapeSequenceMove(fp32 _Speed);

	//Move to position. Free mode device if it's been used for looking
	virtual bool OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial = false, bool _bPerfectPlacement = false);

public:
	//Interface

	//Copy constructor
	CAI_Core_RiotGuard(CAI_Core * _pAI_Core);

	//Initializer
	virtual void Init(CWObject * _pCharacter, CWorld_Server * _pServer);

	//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
	//but information is gathered.
	virtual void OnRefresh(bool _bPassive = false);

	//Handles incoming messages
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Handles any registry keys of a character that are AI-related
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	// As we can take some serious damage and just shrug it off we need special code here
	virtual void OnTakeDamage(int _iAttacker, int _Damage, int _iDamageType);
};


#endif
