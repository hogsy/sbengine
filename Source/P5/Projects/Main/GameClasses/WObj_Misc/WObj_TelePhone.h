/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Telephone.h

Author:			Olle Rosenquist

Copyright:		Copyright Starbreeze AB 2004

Contents:		

Comments:		

History:		
041110:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_TELEPHONE
#define _INC_WOBJ_TELEPHONE

#include "WObj_ActionCutscene.h"
#include "../WObj_Char.h"
#include "../WNameHash.h"

// Should only be one / map.. (character so we can talk to it.......?)
#define CWObject_TelephoneRegistryParent CWObject_Character
class CWObject_TelephoneRegistry : public CWObject_TelephoneRegistryParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	class CDialogLink
	{
	public:
		CStr m_PhoneNumber;
		CNameHash m_DialogLink;
		int8 m_Status;				// Status of this number (functional, busy..?)

		CDialogLink()
			: m_Status(0) { }

		CDialogLink(const CStr& _PhoneNumber, const CStr& _DialogLink, int8 _Status = 0)
		{
			m_PhoneNumber = _PhoneNumber;
			m_DialogLink = _DialogLink;
			m_Status = _Status;
		}

		CDialogLink(const CStr& _PhoneNumber, const CNameHash& _DialogLink, int8 _Status = 0)
		{
			m_PhoneNumber = _PhoneNumber;
			m_DialogLink = _DialogLink;
			m_Status = _Status;
		}
	};
	// Registry between onevalkey and spawn, needs everyone else to be spawned before looking
	// up object indexes to people...?
	//TPtr<CRegistry> m_spReg;
	TArray<CDialogLink> m_lWrongNumbers;

	bool AddPhoneNumber(const CStr& _PhoneNumber, const CStr& _DialogLink);

	// Change dialog of a number
	bool ChangeDialog(const CStr& _PhoneNumber, const CStr& _DialogLink);
public:
	TArray<CDialogLink> m_lPhoneBook;

	dllvirtual void AddPhoneNumber(const CStr& _PhoneNumber, const CNameHash _Hash);

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnCreate();
	virtual void OnSpawnWorld();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

	bool CallWrongNumber(const CStr& _Number, int32 _iChar, const CVec3Dfp32& _CallPos);
	bool CallNumber(const CStr& _Number, int32 _iChar, const CVec3Dfp32& _CallPos);
	bool CallNumber(int32 _iNumber, int32 _iChar, const CVec3Dfp32& _CallPos);
	// Returns index of phone number
	int FindPhoneNumber(const CStr& _PhoneNumber);
	int FindPhoneNum(const CStr& _PhoneNumber);
};


#define CWObject_TelephoneParent CWObject_ActionCutscene
class CWObject_Telephone : public CWObject_TelephoneParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	TPtr<CWObject_TelephoneRegistry> m_spGlobalRegistry;
	// Phone number currently ringing
	int32 m_DialingNumber;
	int32 m_LastDialTone;

	enum{
		PHONESOUND_DIALTONE_0,
		PHONESOUND_DIALTONE_1,
		PHONESOUND_DIALTONE_2,
		PHONESOUND_DIALTONE_3,
		PHONESOUND_DIALTONE_4,
		PHONESOUND_DIALTONE_5,
		PHONESOUND_DIALTONE_6,
		PHONESOUND_DIALTONE_7,
		PHONESOUND_DIALTONE_8,
		PHONESOUND_DIALTONE_9,
		PHONESOUND_BUSY,
		PHONESOUND_HANGUP,
		PHONESOUND_PREDIALTONE,
		PHONESOUND_PICKUPOTHEREND,
		PHONESOUND_RINGING,
		PHONESOUND_SIZE,
	};
	int m_lPhoneSounds[PHONESOUND_SIZE];
	bool m_bRunPreDial:1;
	int32 m_iActiveChar;

	int m_NextDialAtThisTick;
	int m_NumbersLeftToDial;
	void DialNumber();
		
	void MoveSelection(int32 _iChoice);

	virtual bool DoActionSuccess(int _iCharacter);
	virtual bool OnEndACS(int _iCharacter);

	TThinArray<CWO_SimpleMessage> m_lMsg_StartRing;
	TThinArray<CWO_SimpleMessage> m_lMsg_StopRing;
public:
	//CWObject_Object();
	virtual void OnCreate();

	//int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	//static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	static void GetCurrentNumber(CWorld_PhysState* _pWPhys, int32 _iObject, CStr& _Number);
	static void GetCurrentSelection(CWorld_PhysState* _pWPhys, int32 _iObject, CVec2Dint32& _Selection);

	virtual void OnSpawnWorld();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);

	static void RenderPad(CWorld_Client* _pWClient, CRC_Util2D *_pUtil2D, CWObject_CoreData* pObj, CWO_Character_ClientData* _pCD);

	virtual bool CanActivate(int _iCharacter);
	static bool CanActivateClient(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _iCharacter);
	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);
};

#endif
