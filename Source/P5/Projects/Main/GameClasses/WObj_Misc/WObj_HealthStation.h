// ...men annars hade det inte varit så tokigt.
#ifndef __WObj_HealthStation_h
#define __WObj_HealthStation_h

#include "WOBj_ActionCutscene.h"

enum
{
	HEALTHSTATIONTYPE_SMALL = 1,		// Small healthstation, only give health
	HEALTHSTATIONTYPE_BIG,				// Big healthstation, give one additional plupp and full health
};

class CWObject_HealthStation : public CWObject_ActionCutscene
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	// Per healthstation instance
	CActionCutsceneCamera::ConfigBlock	m_RefillCamera; // Camera for refill acs
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerRefill;
	CStr m_CamConfigStr;
	int8 m_MaxLoad;				// Max load of this healthstation.
	int8 m_Load;				// Current load of the station. 0 when depledet. Should never be more than m_Max
	uint16 m_RefillMsgDelay;	// Delay in ticks from activation to Refill Health message is sent to player object
	int8 m_Type;				// Type of healthstation

	// Per activation
	int16 m_iActivator;			// Object that activates this healthstation
	int m_ActivationTick;		// Gametick of activation
	bool m_bHaveGivenHealth;	// Have we given health yet?

protected:
	virtual void OnActivateMessage(int16 _iActivator);	// Called when OBJMSG_ACTIONCUTSCENE_ACTIVATE has been sent to this object
	virtual void OnEndMessage();

	virtual bool DoActionSuccess(int _iCharacter);
	CRPG_Object_Inventory *GetActivatorInventory(int16 _iActivator);
	CRPG_Object_Item *GetActivatorCartItem(int16 _iActivator);
	int32 GetNumActivatorCarts(int16 _iActivator);
	void UseActivatorCart(int16 _iActivator);

	bool ActivatorGotCart();
	void RemoveActivatorCart();

	virtual spCActionCutsceneCamera GetActionCutsceneCamera(int _iObject, int _iCharacter, 
		int _Specific = -1);

	virtual void TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAGI, TArray<int32>& _liACS);

public:
	virtual void OnCreate();
	void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	void OnFinishEvalKeys();
	virtual void GiveHealth();
	virtual void GivePlupp();
	virtual void OnRefresh();
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

#endif // __WObj_HealthStation_h
