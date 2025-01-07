#ifndef _INC_WOBJ_AI_INTERFACE
#define _INC_WOBJ_AI_INTERFACE

#include "WObj_RPG.h"
#include "WObj_AI/AI_ResourceHandler.h"

class CAI_Core;
class CRPG_Object_Item;
class CRPG_Object_Inventory;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Interface_AI
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Interface_AI : public CWObject_RPG
{
public:
	virtual CWObject_Interface_AI* GetInterface_AI() { return this; }

	//Get AI interface for other object 
	CWObject_Interface_AI * GetInterface_AI(int _iObj);

	virtual CAI_Core* AI_GetAI() { return NULL; }									//Set the CAI_Core pointer that data points at to the agent's AI pointer.  

	virtual bool AI_IsSpawned() { return false; }									// Check if the agent spawned
	virtual bool AI_IsAlive() { return false; }										//Check if the agent is considered to be "alive" i.e. functional, not no-clipping etc.
	virtual bool AI_IsBot() { return false; }
	virtual bool AI_IsWeightLess() { return false; }
	virtual int AI_GetActivationState() { return 0; };
	virtual void AI_SetActivationState(int _State) {};

	virtual fp32  AI_GetMaxSpeed(int _Speed) { return 0.0f; }						//Set the fp32 that data points at to maximum speed in the direction specified as param0, see enum below

	virtual CRPG_Object_Item* AI_GetRPGItem(int _EquipPlace) { return NULL; }		//Set the CRPG_Object_Item pointer that data points at to currently wielded item of type given in param0 (RPG_CHAR_INVENTORY_WEAPONS etc)
	virtual CRPG_Object_Inventory* AI_GetRPGInventory(int _iSlot) { return NULL; }	//Set the CRPG_Object_Inventory pointer that data points at to inventory of the type given in param0 (RPG_CHAR_INVENTORY_WEAPONS etc)

	virtual uint16 AI_GetPhysFlags() { return 0; } //Get physics influencing flags (immune, immobile etc)

	//Get the number of teams the agent belongs to and add the team indices to the given list
	virtual uint AI_GetTeams(uint16* _piResult, uint _MaxElem) const { return 0; }

	//Return value of rpg attribute specified in the param0, see enum in ai_def.h.
	virtual int  AI_GetRPGAttribute(int _Attribute, int _Param1 = 0) { return 0; }			

	//Return position [matrix] of camera last frame
	virtual CMat4Dfp32 AI_GetLastCamera() { CMat4Dfp32 Res; Res.Unit(); Res.k[3][0] = _FP32_MAX; Res.k[3][1] = _FP32_MAX; Res.k[3][2] = _FP32_MAX; return Res; };			
	virtual CVec3Dfp32 AI_GetLastCameraPos() { return CVec3Dfp32(_FP32_MAX); };			

	//Get control mode (see PLAYER_CONTROLMODE_XXX, WObj_Char.h)
	virtual int AI_GetControlMode(){ return 0; };

	//Set animation physics control on/off (true/false)
	virtual void AI_SetAnimControlMode(bool _bOn){};

	// Returns a randomly chosen object id based on the supplied params
	virtual CWObject* AI_GetSingleTarget(int _iObject, CStr _pName, int _iSender)
	{
		CWObject* pObj = NULL;
		if (m_pWServer)
		{
			pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(_pName));

		}
		return(pObj);
	};

	virtual void AI_GetBaseMat(CMat4Dfp32& _RetValue) {}							// Get character position matrix, but without the look-angles merged into it..
	virtual void AI_GetWeaponMat(CMat4Dfp32& _RetValue) {}						// Position + direction of weapon muzzle 
	virtual void AI_GetHeadMat(CMat4Dfp32& _RetValue) {}							// Get position of eyes + direction of look
	virtual void AI_GetActivateMat(CMat4Dfp32& _RetValue) {}						// Get activate position of agent

	virtual CVec3Dfp32 AI_GetBasePos() { return 0; }
	virtual CVec3Dfp32 AI_GetWeaponPos() { return 0; }
	virtual CVec3Dfp32 AI_GetHeadPos() { return 0; }
	virtual CVec3Dfp32 AI_GetActivatePos() { return 0; }

	virtual bool AI_IsForceRotated() { return false; }
	virtual bool AI_IsJumping() { return false; }								// Returns true during jump
	virtual bool AI_CanJump(const bool _bFromAir = false) { return false; }									// Use this to determine if we can jump
	virtual bool AI_Jump(const CMat4Dfp32* _pDestMat, int32 _Flags);				// Returns false if we cannot jump
	virtual void AI_EnableWallClimbing(bool _bActivate) {}						// Activate/deactivate wallclimbing
	virtual bool AI_IsWallClimbingEnabled() { return false; }					// Returns true if wall climbing is enabled
	virtual bool AI_IsOnWall() { return false; }								// Returns true when not on ground
	virtual void AI_SetEyeLookDir(const CVec3Dfp32& _EyeLook = CVec3Dfp32(_FP32_MAX,0,0), const int16 _iObj = 0) {}

	/*
	virtual bool AI_IsIncapacitated() { return true; }								//Check if the agent is capable of action/perception
	virtual void AI_GetVulnerablePos(CVec3Dfp32& _RetValue) {}						//Set the CVec3Dfp32 that the data points at to the vulnerable position of the agent (e.g. head for characters)
	virtual fp32  AI_GetNoise() { return 0.0f; }
	virtual fp32  AI_GetVisibility() { return 0.0f; }
	virtual int  AI_GetControlMode() { return 0; }

	//Notifies object of preferred movement mode.
	virtual void AI_SetAnimGraphMode(uint8 _Mode){};		

	//Get AI rank
	virtual int AI_GetRank(){ return 0;};
	*/


	//Make all bots pause/resume behaviour according to given flags. Script will still work as normal.
	void PauseAllAI(int _PauseFlags = CAI_ResourceHandler::PAUSE_GENERAL);	
	void UnpauseAllAI(int _PauseFlags = CAI_ResourceHandler::PAUSE_GENERAL);
};

/*	old system:									new system:

	OBJMSG_AIQUERY_GETAI,						AI_GetAI
	OBJMSG_AIQUERY_GETACTIVATEPOSITION,		    AI_GetActivatePosition
	OBJMSG_AIQUERY_ISALIVE,						AI_IsAlive
	OBJMSG_AIQUERY_GETMAXSPEED,					AI_GetMaxSpeed
	OBJMSG_AIQUERY_GETAIMPOSITION,				AI_GetAimPosition
	OBJMSG_AIQUERY_RPGATTRIBUTE,				AI_GetRPGAttribute
	OBJMSG_AIQUERY_GETRPGITEM,					AI_GetRPGItem
	OBJMSG_AIQUERY_GETRPGINVENTORY,				AI_GetRPGInventory
	OBJMSG_AIQUERY_GETVULNERABLEPOS,			AI_GetVulnerablePos
	OBJMSG_AIQUERY_HASBOW,						AI_HasBow
	OBJMSG_AIQUERY_BOWLOADED,					AI_IsBowLoaded
	OBJMSG_AIQUERY_USINGWEAPON,					AI_IsUsingWeapon

	OBJMSG_CHAR_NOISE							AI_GetNoise
	OBJMSG_CHAR_VISIBILITY						AI_GetVisibility
	OBJMSG_CHAR_GETCONTROLMODE					AI_GetControlMode
	OBJMSG_CHAR_ISWEIGHTLESS					AI_IsWeightLess
	OBJMSG_CHAR_ISINBOWMODE						AI_IsInBowMode
*/	

 
#endif // _INC_WOBJ_AI_INTERFACE

