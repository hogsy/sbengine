#ifndef _INC_AI_SCRIPTHANDLER
#define _INC_AI_SCRIPTHANDLER

//Helper class that handles all script control of an AI
class CAI_ScriptHandler
{
private:
	//Pointer to owning AI-object
	CAI_Core* m_pAI;

	//Sequence index to animation
	int m_iAnim;
	//Animation code for current animation
	bool m_bTorsoAnim;
	//Has the animation been started?
	bool m_bAnimStarted;
	//Animation timer
	int m_AnimTimer;

	// Holds all members needed for engine path following
	// ***
	CAI_FollowPathData* m_pFD;
	// ***

	//Server index to engine path to follow.
	int m_iPath;
	//Following mode, see enum in public section below
	int m_iFollowMode;
	//Keeps track of when we need to navigate our way back to the path
	bool m_bFallenBehind;
	//Keeps track of the position to go to when following path
	CVec3Dfp32 m_PathPosition;
	//Are we flying?
	bool m_bFlying;
	//Should we turn on gravity when we change path following or clear script?
	bool m_bTurnOnGravity;
	//Should we turn on animation control when we change path following or clear script?
	bool m_bTurnOnAnimControl;
	//The position to go to when following relative
	CVec3Dfp32 m_MovePosRel;
	//The last tick we propelled a path
	int m_LastPropelTick;
	//Are we at a stop in path?
	bool m_bStop;
	//Look vector when following path
	CVec3Dfp32 m_LookVector;

	//The action devices to perform, one possible per each device 
	enum {
		MIN_ACTION = 0, 
		LOOK = MIN_ACTION,
		MOVE,
		JUMP,
		WEAPON,
		ITEM,
		STANCE,
		DARKNESS,
		MAX_ACTION,	
	};
	int m_liAction[MAX_ACTION];

	//Parameters for the actions
	CVec3Dfp32 m_MoveParam;
	CVec3Dfp32 m_LookParam;
	int m_iWeaponParam;
	int m_iItemParam;
public:
	// *** FIXME: Unify target params between CAI_Core, CAI_ScriptHandler and CAI_Action and its children ***
	int m_iTargetParam;
	int m_iSecondaryTargetParam;
private:
	int m_LookTimeout;					// Time to stop looking after a LOOK_AT, SNAP_LOOK_AT command
	CWO_ScenePoint* m_pScenePoint;
	CWO_ScenePoint* m_pNextScenePoint;	// Set m_pScenePoint to m_pNextScenePoint AFTER script has finished
	// Copy the scenepoint so that it won't get overwritten by a movetoscenepoint message in ActivateScenePoint()
	// (The fiendish scripters are known to do such evilness)
	CWO_ScenePoint* m_pDoorScenePoint;

	int m_ScenePointStayTimeout;	// timer tick when we no longer need to stay at the scenepoint
	int m_ScenePointBlendTicks;
#define MOVETO_SP_PP_BLENDTICKS	8

	// *** Ugly haxxor for delayed behaviour response ***
	// Remove as soon as Darkness ships goddammit!
	int m_iDelayedBehaviour;
	int m_iDelayedBehaviourTarget;
	CMat4Dfp32 m_DelayedbehaviourMat;
	int m_BehaviourStartTimer;

	//The melee manouvre to perform
	int m_iMeleeManoeuvre;

	//Parameters for the melee manoeuvres
	int m_iMeleeTarget;

	//Power is one of the CAI_Device_Darkness::POWER_XXX enum values
	int8 m_DarknessPower;
	enum {
		DARKNESSSTATE_INIT,

		DARKNESSSTATE_DEMONARM_PREGRAB,
		DARKNESSSTATE_DEMONARM_GRABBING,
		DARKNESSSTATE_DEMONARM_GRABBED,
		DARKNESSSTATE_DEMONARM_THROWING,
	};
	int8 m_DarknessState;
	int m_DarknessUseTimer;
	//The target object for our darkness usage.
	int m_DarknessTarget;
	int m_DarknessSecondaryTarget;
	int8 m_DarknessUsage;
	
	//Any waiting actions are put on this queue (not in use...)
	static const int MAX_NO_WAITING_ACTIONS;
	//TQueue<int> m_qWaitingActions;

	//Maps the actions in the enum above to the devices
	static int ms_iDeviceMap[MAX_ACTION];

	//Chooses the actions the bot should take to perform the current animation if there are available devices
	void OnPerformAnimation();

	//Creates commands that perform the current actions 
	void OnPerformActions();
	
	//Move along a path on the ground using pathfinding, stopping the path when it gets too far away, and looking in the paths 
	//direction if look device is available. If we should force follow, we don't stop path when
	//falling behind and always just move bliondly towards the path.
	void OnGroundFollowPath();

	//Move to the path object along the ground, but when the object has been reached, jump and then fly
	//along the path. If the we should force follow, we always fly off immediately and fly towards the path object.
	void OnFlyFollowPath();

	//Teleport to paths position every frame
	void OnTeleportFollowMe();

	//Chooses the actions the bot should take to follow the path, if there are available devices
	void OnFollowPath();

	//Act aggressively
	void OnAggressive();

	// Handle haxxor delayed behaviour
	void OnDelayedBehaviour();

	// Use darkness power
	void OnUseDarkness();
	void OnUseDemonArm(int _GrabTarget = 0, int _WhackTarget = 0);
	void SetDarknessState(int _State);
	void UseDarkness(int _Power, const CVec3Dfp32& _Move = CVec3Dfp32(_FP32_MAX));
	void StopUsingDarkness();
	void ClearUseDarkness();
	CVec3Dfp32 GetDarknessPosition();

	// Clear all scripting parameters
	void ClearParameters();

public:

	//Actions
	enum {
		INVALID_ACTION = 0,
		CONTINUOUS_MOVE = 1,
		CONTINUOUS_LOOK,
		
		TOGGLE_CROUCH,	// Deprecated
		
		SINGLE_JUMP,
		SINGLE_SWITCH_WEAPON, 
		SINGLE_SWITCH_ITEM, 
		SINGLE_USE_WEAPON,
		SINGLE_USE_ITEM,

		MOVE_TO,
		MOVE_TO_SCENEPOINT,
		LOOK_AT,
		SNAP_LOOK_AT,
		TURRET_MOUNT,

		ATTACK,
		AIM,

		FORCE_USE_WEAPON,
		FORCE_USE_ITEM,
		FORCE_SHOOT,

		AGGRESSIVE,
		USE_DARKNESS,
	};
	
	//Constructor. Script is initially invalid.
	CAI_ScriptHandler();
	void SetAI(CAI_Core* _pAI);

	//Set path to follow, specifying following mode
	enum {
		FOLLOW_NORMAL	= 0,
		FOLLOW_FORCE	= 0x1,
		FOLLOW_FLY		= 0x2,
		FOLLOW_RELATIVE = 0x4,
		FOLLOW_TELEPORT = 0x8,
		FOLLOW_TRUE		= 0x10,
	};
	void SetPath(int _iPath, int _iFollowMode = FOLLOW_NORMAL);

	//Set animation to execute and also starts animation. Animations therefore starts asynchronously.
	//If animsequence is -1 then any currnet animation is stopped.
	void SetAnimation(int _iAnimSequence, bool _bTorsoAnim = false);

	//Set action to take
	void SetAction(int _iAction, CWO_ScenePoint* _pSP);
	void SetAction(int _iAction, int _iParam = -1, const CVec3Dfp32& _VecParam = CVec3Dfp32(_FP32_MAX));
	void SetAction(int _iAction, const CVec3Dfp32& _VecParam);
	void SetAction(int _iAction,CStr _Str);

	void SetDelayedBehaviour(int _iBehaviour, int _iTarget, fp32 _Delay, CMat4Dfp32& _Mat);

	//Hold/release attacks
	void SetHoldAttacks(bool _bHold);

	//Removes any current script parameters, effectively releasing script control
	void Clear();

	//Chooses the actions the bot should take at the current time.
	void OnTakeAction();

	//Script controller is valid when it wants to control the AI
	bool IsValid();
	bool IsPaused();
	void SetPause(bool _bPause);

	// Returns the mounted object ID if any
	bool SetMountScenepoint(CWO_ScenePoint* _pScenePoint);
	// Returbs m_pScenePoint if MOVE_TO_SCENEPOINT is on
	CWO_ScenePoint* GetMoveToScenepoint();
	M_FORCEINLINE CWO_ScenePoint* GetDoorScenepoint() { return(m_pDoorScenePoint); };
	M_FORCEINLINE void ClearDoorScenepoint()
	{
		if (m_pDoorScenePoint)
		{
			if (!m_pScenePoint)
			{
				m_liAction[MOVE] = 0;
			}
			m_ScenePointStayTimeout = -1;
			m_ScenePointBlendTicks = 0;
			m_pDoorScenePoint = NULL;
		}
	};

	//The given action is performed immediately (and asynchronously), regardless of any normal restrictions
	void ForcedAction(int _iAction, int _iParam = 0);

	//Change AI user
	void ReInit(CAI_Core* _pAI);

	//Load/Save
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);

#ifndef M_RTM
	//Get some debug info about the bot
	virtual CStr GetDebugString();
#endif
};

#endif
