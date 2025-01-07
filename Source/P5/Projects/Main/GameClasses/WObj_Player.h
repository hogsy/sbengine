#ifndef _INC_WOBJ_PLAYER
#define _INC_WOBJ_PLAYER

#include "WObj_RPG.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Interface_AI.h"

bool IntersectModel(int _iModel, const CMat4Dfp32& _WMat, const CXR_AnimState& _AnimState, CWorld_PhysState* _pWPhysState, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

// -------------------------------------------------------------------

//#define LOG_PREDICTION

// #define LOG_AUTHORING
// #define LOG_CLIENTAUTHORING

// #define LOG_SERVERAUTHORING_CMD
// #define LOG_CLIENTAUTHORING_CMD

#define PLAYER_CLIENTOBJ_CLIENTDATA		0
#define PLAYER_CLIENTOBJ_SKELINSTANCE	1
#define PLAYER_CLIENTOBJ_AGISTATE		2

#define PLAYER_SELFSPLASH_SCALE 0.4f

enum
{
	PLAYER_CLIENTFLAGS_SERVERCONTROL = 		(1 << CWO_CLIENTFLAGS_USERSHIFT),

	PLAYER_CLIENTFLAGS_USERSHIFT = (CWO_CLIENTFLAGS_USERSHIFT+1)
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Player_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Player_ClientData : public CReferenceCount, public CAutoVarContainer
{
public:
	class CWObject_CoreData* m_pObj;

	int m_bIsPredicting : 1;			// True during a predicted client side refresh (ClientPredictFrame), m_bIsClientRefresh is also set if this flag is set.
	int m_bIsClientRefresh : 1;			// True during a client side OnClientRefresh()
	int m_bIsServerRefresh : 1;			// True during a server side OnRefresh()
	int m_bDoOneTimeEffects : 1;		// True if one-time client-side effects should be performed. (playing sounds, etc..)

	// These are used by prediction logic to keep track of client side copies of the player.
	int m_bIsValidPrediction : 1;
	int m_bIsFullFramePrediction : 1;
	int m_LastUpdateNumTicks : 8;
	int m_PredictLastCmdID;

	uint32 m_OneTimeEffectsGameTick;

	int32 m_GameTick;
	fp32 m_PredictFrameFrac;				// The end time of a partially predicted refresh, typically used for interpolation of non-contingeous values during rendering.

	CMTime m_GameTime;

	virtual void Copy(const CWO_Player_ClientData& _CD);

	virtual CWObject_Client* CreateClientObject(CWObject_Client* _pObj, CWorld_Client* _pWClient) = 0;
	virtual void CopyClientObject(CWObject_Client* _pObj, CWObject_Client* _pDest) = 0;

	virtual int OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog = false) = 0;
	virtual int OnClientCheckPredictionMisses(CWorld_Client* _pWClient, CWObject_Client* _pObj, CWObject_Client* _pPredict, bool _bLog = false) = 0;

//	void operator=(const CWO_Player_ClientData& _CD);
	void Clear(CWObject_CoreData* _pObj);
	CWO_Player_ClientData();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Player
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Player : public CWObject_Interface_AI

{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static CWO_Player_ClientData* GetClientData(CWObject_CoreData* _pObj);
	static const CWO_Player_ClientData* GetClientData(const CWObject_CoreData* _pObj);
	static CXR_SkeletonInstance* GetClientSkelInstance(CWObject_CoreData* _pObj);

	static CWO_Player_ClientData* GetCD(CWObject_CoreData* _pObj);					// Always != NULL
	static const CWO_Player_ClientData* GetCD(const CWObject_CoreData* _pObj);		// Always != NULL

	static bool IsLocalPlayer(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static bool IsPredicted(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static bool IsMirror(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	static CWObject_Client* Player_GetLastValidPrediction(CWObject_Client* _pObj);
	static int Player_GetCopyNumber(CWObject_Client* _pObj);
	static CWObject_Client* Player_FindCopyWithCmdID(CWObject_Client* _pObj, int _CmdID);
	static void Player_InvalidatePredictions(CWObject_Client* _pPredict);
	static void OnClientPredict(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _Cmd, int _Param0, int _Param1);
};


#endif // _INC_WOBJ_PLAYER
