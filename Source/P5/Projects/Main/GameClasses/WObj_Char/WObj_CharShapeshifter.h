/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Character class for shapeshifter(multiplayer) characters.

Author:			Roger Mattsson

Copyright:		2005 Starbreeze Studios AB

Contents:		CWObject_CharShapeshifter

Comments:

History:		
051117:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharShapeshifter_h__
#define __WObj_CharShapeshifter_h__

#include "WObj_CharDarkling.h"
#include "WObj_CharPlayer.h"
#include "WObj_CharShapeshifter_ClientData.h"

enum
{
	SHAPESHIFTER_REMOVE_FLAGS_REMOVE =	M_Bit(0),
	SHAPESHIFTER_REMOVE_FLAGS_CHECK =	M_Bit(1),
};

class CWObject_CharShapeshifter : public CWObject_CharDarkling
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_CharDarkling parent_darkling;
	typedef CWObject_Character parent_human;

public:
	// Client data - server only interface
	const CWO_CharShapeshifter_ClientData& GetClientData() const { return GetClientData(this); }
	CWO_CharShapeshifter_ClientData& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharShapeshifter_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharShapeshifter_ClientData& GetClientData(CWObject_CoreData* _pObj);

	CWObject_CharShapeshifter();

	static void		OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void		OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static aint		OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static int		OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	virtual void	OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void	OnFinishEvalKeys(void);
	static void		OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void	OnInitInstance(const aint* _pParam, int _nParam);
	virtual aint	OnMessage(const CWObject_Message& _Msg);
	static int		OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);
	virtual void	OnRefresh();
	virtual void	OnPress();
	virtual void	SpawnCharacter(int _PhysMode = PLAYER_PHYS_STAND, int _SpawnBehavior = 0);
		
	bool			ToggleDarklingHuman(bool _bForce = false, bool _bSpawnShape = true);
	void			RemoveAllDarklingsWeapons(void);
	void			EquipSomething(void);
	void			ResetSpeeds(void);

private:
	int				m_Fov[2];
	int				m_PhysWidth[2];
	int				m_PhysHeight[2];
	int				m_PhysHeightCrouched[2];
	int				m_DeadRadius[2];
	int				m_Dialogues[2];
	fp32			m_PhysStepsize[2];
	fp32			m_PhysFriction[2];

	fp32			m_SpeedForward[2];
	fp32			m_SpeedSidestep[2];
	fp32			m_SpeedUp[2];
	fp32			m_SpeedJump[2];
	int32			m_SelfRemoveTick;
	uint8			m_SelfRemoveFlags;
	int32			m_WeaponAtShapeshift;
};



#endif // __WObj_CharShapeshifter_h__
