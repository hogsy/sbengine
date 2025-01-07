
#ifndef _INC_WOBJ_RPG
#define _INC_WOBJ_RPG

#include "WObj_Messages.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "WObj_Misc/WObj_Burnable.h"
#include "WObj_Misc/WObj_Object.h"
#include "WObj_Sys/WObj_Physical.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../Shared/mos/Classes/GameWorld/Client/WClient_Core.h"

enum
{
	NETMSG_PLAYDIALOGUE = 0x10,
	NETMSG_STOPDIALOGUE,
	NETMSG_FORWARDDIALOGUE,

	OBJMSG_RPG_SPEAK_OLD = 0x100c, // Moved from OBJMSG_CHAR_SPEAK

	OBJMSG_RPG_AVAILABLEPICKUP = OBJMSGBASE_RPGOBJ,
	OBJMSG_RPG_REPLACEPICKUP,
	OBJMSG_RPG_SETRIGIDBODY,

	OBJMSG_RPG_ISPICKUP,
	OBJMSG_RPG_GETPICKUPANIM,
	OBJMSG_RPG_SETTEMPPICKUPSURFACE,

//	OBJMSG_RPG_GETITEMMATRIX,
	OBJMSG_RPG_GETITEMMODEL,
	OBJMSG_RPG_GETRPGOBJECT,

	OBJMSG_RPG_ACTIVATEDIALOGUEITEM,

	OBJMSG_RPG_SETNEWVELOCITY,
	OBJMSG_RPG_SPEAK = 0x200a,				// ARGH! This is *not* how you do it! Script messages (static enums) should not be mixed with code messages (dynamic enums)

	RPG_DATAINDEX_DIALOGUEID = 0,
	RPG_DATAINDEX_CURDIALOGUEINDEX = 1,
	RPG_DATAINDEX_CURDIALOGUEITEMTICK = 2,
	RPG_DATAINDEX_LAST,

	
};

class CWObject_RPG : public CWObject_Physical
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	TPtr<class CRPG_Object> m_spRPGObject;

	CWObject_RPG();

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	static int IncludeDialogue(CStr _Dialogue, CMapData *_pMapData);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	virtual bool PlayDialogue_Hash(uint32 _DialogueItemHash, uint _Flags = 0, int _Material = 0, int _StartOffset = 0, uint32 _SyncGroupID = 0, int8 _AttnType = WCLIENT_ATTENUATION_3D);
	static bool PlayDialogue_Hash(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, uint32 _DialogueItemHash, uint _Flags, uint8 _Material, int _AttnType, int _StartOffset = 0, uint32 _SyncGroupID = 0);
	static CWRes_Dialogue *GetDialogueResource(CWObject_CoreData* _pObj, CWorld_PhysState* _pWClient);
	
	static int GetDialogueID(CWObject_CoreData *_pObj);
	int GetDialogueID();
	
	static void Voice_Play(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CVec3Dfp32& _Pos, const CVec3Dfp32 &_Orientation, int _iSound, uint32 _CustomVocapAnim, uint16 _DialogueAnimFlags, int _iVocapMovingHold, fp32 _MaxAttDist, fp32 _MinAttDist, bool _b2D, int _StartOffset, uint32 _SyncGroupID, uint8 _Channel);
	static void Voice_Stop(CWObject_Client *_pObj, CWorld_Client *_pWClient);
	static void Voice_Refresh(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CVec3Dfp32 &_Pos, const CVec3Dfp32 &_Orientation, fp32 _Occlusion = 0.0f);

	static int Voice_GetHandle(CWObject_CoreData *_pObj);
};

class CWObject_Item : public CWObject_Object
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	enum
	{
		CLIENTFLAGS_ATTACHED = (CWO_CLIENTFLAGS_USERBASE << 0),
		CLIENTFLAGS_ROTATE = (CWO_CLIENTFLAGS_USERBASE << 1),
		
		FLAGS_ROTATE = 1,
		FLAGS_FALL = 2,
		FLAGS_WAITSPAWN = 4,
		FLAGS_PHYSSET = 8,
		FLAGS_NODYNAMIC = 16,

		ITEM_PHYS_IDLETICKS = 4,
	};

	CFStr m_ItemTemplate;
	TPtr<class CRPG_Object_Item> m_spDummyObject;
	int m_iItemRepickCountdown;
	int m_nPhysIdleTicks : 8;
	int	m_iPickupAnim;
	int8 m_Flags;
	CWO_SimpleMessage m_Msg_OnPickup;

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	//virtual void OnRefresh();
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	aint OnMessage(const CWObject_Message& _Msg);
	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine);

	virtual void TagAnimationsForPrecache(class CWAG2I_Context* _pContext, class CWAG2I* _pAgi);

	bool SetItem(const char *_pName);
	bool SetItem(CRPG_Object_Item *_pObj);

	void AttachCharacter(CWObject *_pChar, int _iItemSlot);
	void DetachCharacter(CWObject *_pChar, class CRPG_Object_Item *_pItem, const CMat4Dfp32 &_Pos);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	//static bool OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo = NULL);

	void CreatePhys();
};

#endif
