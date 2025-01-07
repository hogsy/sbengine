#ifndef _INC_AICORE_TURRET
#define _INC_AICORE_TURRET

#include "../AICore.h"

//AI for handling turrets
class CAI_Core_Turret : public CAI_Core
{

public:
	//Sets behaviour from given behaviour type, parameter target name, distance and max distance parameters(if any)
	virtual bool SetAction(int _iAction, CStr _sParamName = "", fp32 _DistParam = -1.0f, fp32 _MaxDistParam = -1.0f);

	//Sets behaviour from keys
	virtual void SetActionFromKeys();

	//Turrets can only move along rails
	virtual int32 OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial = false, bool _bPerfectPlacement = false);

public:
	//Copy constructor
	CAI_Core_Turret();
	CAI_Core_Turret(CAI_Core* _pAI_Core);

	//Get type of AI
	virtual int GetType();

	//Create and return default behaviour
	spCAI_Behaviour DefaultBehaviour();

	//
	virtual int Perception_Sight(int _iObj,bool _bObjectOnly);

	//
	virtual int GetMoveMode() { return MOVEMODE_STAND; }

	//Handles any registry keys of a character that are AI-related
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual CVec3Dfp32 GetHeadPosition();

	virtual fp32 GetBaseSize() { return 24; };
	virtual fp32 GetHeight() { return 64; };
	virtual fp32 GetCrouchHeight()	{ return 32; };
	virtual fp32 GetStepHeight() { return 12; };
	virtual fp32 GetJumpHeight() { return 48; };
	virtual fp32 GetWalkStepLength() { return 24; };
	virtual fp32 GetRunStepLength() { return 48; };


	// Overrides the CAI_Core as we don't do anything here
	virtual void OnTakeAction();
	virtual void OnRefresh(bool _bPassive);
};

#endif
