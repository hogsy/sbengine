/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Turret.h

Author:			Olle Rosenquist

Copyright:		Copyright O3 Games AB 2005

Contents:		

Comments:		

History:		
051207:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_TURRET
#define _INC_WOBJ_TURRET


#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "../WObj_Char.h"

#define CWObject_TurretParent CWObject_Model

class CWObject_Turret : public CWObject_TurretParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		TURRET_AG2_DIRTYMASK = CAutoVarContainer::DIRTYMASK_2_1,
		TURRET_TYPE_MOUNTEDGUN = 0,
		TURRET_TYPE_FLOODLIGHT = 1,
		TURRET_TYPE_HELI = 2,
		TURRET_TYPE_OWTANK = 3,
		OBJMSG_TURRET_GETMINMAXYZ = OBJMSGBASE_PLAYER + 9456,
		OBJMSG_TURRET_HANDLEINPUT,
		OBJMSG_TURRET_GETPOSITION,
		OBJMSG_TURRET_GETRELATIVEANGLES,
		OBJMSG_TURRET_GETATTACHPOSITIONS,
		OBJMSG_TURRET_GETWEAPON,
		OBJMSG_TURRET_GETTRACKINGTIME,
		OBJMSG_TURRET_GETTURRETFLAGS,

		TURRET_FLAG_FLASHLIGHTENABLED = M_Bit(0),
		TURRET_FLAG_MUZZLEFLASHDISABLED = M_Bit(1),
		TURRET_FLAG_MOUNTLIGHT = M_Bit(2),
		TURRET_FLAG_WAITSPAWNED = M_Bit(3),
		TURRET_FLAG_BLACKHOLEENABLED = M_Bit(4),
		TURRET_FLAG_LIGHTSCRIPTED = M_Bit(5),
		TURRET_FLAG_NOLIGHTSHADOW = M_Bit(6),
		TURRET_FLAG_NOCAMERA = M_Bit(7),
	};

	class CAutoVar_TurretEffect
	{
	public:
		// Extramodel (effect)
		CMTime m_ExtraModelStartTime;
		TPtr<CXR_ModelInstance> m_spExtraModelInstance;
		fp32    m_ExtraModelDuration;
		uint16 m_iExtraModel;
		uint8 m_iExtraModelAttachPoint;
		uint8 m_iExtraModelRotTrack;
		int16 m_ExtraModelSeed;

		CAutoVar_TurretEffect();
		void Clear();

		CMTime GetTime(const CMTime& _CurrentTime);
		
		void SetExtraModel(CMapData* _pMapData, CMTime _EffectTime, fp32 _Duration, uint16 _iExtraModel, uint8 _iAttach, uint8 _iRotTrack, int16 _Seed = 0, bool _bCreateInstance = false);

		void RenderExtraModel(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_Model* _pBaseModel,
			CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject);

		void CopyFrom(const CWObject_Turret::CAutoVar_TurretEffect& _From);
		void Pack(uint8 *&_pD, CMapData* _pMapData) const;
		void Unpack(const uint8 *&_pD, CMapData* _pMapData);
	};

	class CWO_Turret_ClientData : public CReferenceCount, public CAutoVarContainer
	{
	public:
		friend class CWObject_Turret;

		CWObject_CoreData* m_pObj;
		CWorld_PhysState* m_pWPhysState;
		class CTurretAnimGraph : public CWO_ClientData_AnimGraph2Interface
		{
		public:
			spCWAG2I        m_spAGI;
			spCWAG2I_Mirror m_spMirror;

			int32 m_StateFlagsLo;
			int32 m_StateFlagsHi;
			bool m_bNeedUpdate;

			CTurretAnimGraph();
			virtual const CWAG2I* GetAG2I() const { return m_spAGI; }
			virtual       CWAG2I* GetAG2I()       { return m_spAGI; }

			virtual void SetInitialProperties(const CWAG2I_Context* _pContext);
			virtual void UpdateImpulseState(const CWAG2I_Context* _pContext);
			virtual void AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2ActionIndex _iEnterAction);

			void Copy(const CTurretAnimGraph& _CD)
			{
				CWO_ClientData_AnimGraph2Interface::Copy(_CD);
				m_StateFlagsLo = _CD.m_StateFlagsLo;
				m_StateFlagsHi = _CD.m_StateFlagsHi;
				m_bNeedUpdate = _CD.m_bNeedUpdate;
			}
			void Clear()
			{
				CWO_ClientData_AnimGraph2Interface::Clear();
				m_StateFlagsLo = 0;
				m_StateFlagsHi = 0;
				m_bNeedUpdate = false;
			}
		};
		CTurretAnimGraph m_AnimGraph2;
		CVec3Dfp32 m_PivotPoint;
		CVec3Dfp32 m_CharOffset;
		spCXR_SkeletonInstance m_spSkelInstance;
		fp32 m_TurrentAngleZ;
		fp32 m_TurrentAngleY;
		fp32 m_LastSkelInstanceFrac;
		int32 m_LastSkelInstanceTick;

		AUTOVAR_SETCLASS(CWO_Turret_ClientData, CAutoVarContainer);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32,m_CameraOffset,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32,m_AttachLeftHand,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32,m_AttachRightHand,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32,m_CharacterOffset,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_uint8,m_iCameraRotTrack,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8,m_iRenderAttachBone,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8,m_iRenderAttachPoint,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8,m_iTurretRotTrack,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8,m_iShootBone0,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8,m_iShootBone1,DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int16,m_iRenderAttached,DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_int16,m_iMountedChar,DIRTYMASK_0_2);
		CAUTOVAR_OP(CAutoVar_int32,m_MuzzleFlashTick,DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_int32,m_MuzzleLightColor,DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_uint16,m_MuzzleLightRange,DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_uint16,m_FlashLightRange,DIRTYMASK_0_4);
		CAUTOVAR_OP(CAutoVar_int32,m_FlashLightColor,DIRTYMASK_0_4);
		CAUTOVAR_OP(CAutoVar_fp32,m_MaxTurrentAngleZ,DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_fp32,m_MaxTurrentAngleY,DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_fp32,m_AimDamper,DIRTYMASK_0_5);
		CAUTOVAR(CAutoVar_TurretEffect,m_TurretEffect0,DIRTYMASK_0_6);
		CAUTOVAR(CAutoVar_TurretEffect,m_TurretEffect1,DIRTYMASK_1_0);
		CAUTOVAR_OP(CAutoVar_uint8,m_ItemFlags,DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint8,m_TurretType,DIRTYMASK_1_2);
		CAUTOVAR_OP(CAutoVar_uint8,m_TurretFlags,DIRTYMASK_1_3);
		CAUTOVAR_OP(CAutoVar_uint16,m_VisibilityMask,DIRTYMASK_1_4);
		CAUTOVAR_OP(CAutoVar_CMat4Dfp32,m_TurretLocalPos,DIRTYMASK_2_0);

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_CameraOffset)
		AUTOVAR_PACK_VAR(m_AttachLeftHand)
		AUTOVAR_PACK_VAR(m_AttachRightHand)
		AUTOVAR_PACK_VAR(m_CharacterOffset)
		AUTOVAR_PACK_VAR(m_iCameraRotTrack)
		AUTOVAR_PACK_VAR(m_iRenderAttachBone)
		AUTOVAR_PACK_VAR(m_iRenderAttachPoint)
		AUTOVAR_PACK_VAR(m_iTurretRotTrack)
		AUTOVAR_PACK_VAR(m_iShootBone0)
		AUTOVAR_PACK_VAR(m_iShootBone1)
		AUTOVAR_PACK_VAR(m_iRenderAttached)
		AUTOVAR_PACK_VAR(m_iMountedChar)
		AUTOVAR_PACK_VAR(m_MuzzleFlashTick)
		AUTOVAR_PACK_VAR(m_MuzzleLightColor)
		AUTOVAR_PACK_VAR(m_MuzzleLightRange)
		AUTOVAR_PACK_VAR(m_FlashLightRange)
		AUTOVAR_PACK_VAR(m_FlashLightColor)
		AUTOVAR_PACK_VAR(m_MaxTurrentAngleZ)
		AUTOVAR_PACK_VAR(m_MaxTurrentAngleY)
		AUTOVAR_PACK_VAR(m_AimDamper)
		AUTOVAR_PACK_VAR(m_TurretEffect0)
		AUTOVAR_PACK_VAR(m_TurretEffect1)
		AUTOVAR_PACK_VAR(m_ItemFlags)
		AUTOVAR_PACK_VAR(m_TurretType)
		AUTOVAR_PACK_VAR(m_TurretFlags)
		AUTOVAR_PACK_VAR(m_VisibilityMask)
		AUTOVAR_PACK_VAR(m_TurretLocalPos)
		AUTOVAR_PACK_END
	public:
		CWO_Turret_ClientData();

		void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
		void OnRefresh();
		void AddAimModifier(CXR_SkeletonInstance* _pSkel, CWorld_PhysState* _pWPhys);

		const CMat4Dfp32& GetLocalPos() const { return m_TurretLocalPos.m_Value; }
	};
	TThinArray<CWO_SimpleMessage> m_lMsgOnMount;
	TThinArray<CWO_SimpleMessage> m_lMsgOnDismount;
	// "Fake" weapon, that we use to spawn projectiles and such (no model)
	TPtr<CRPG_Object_Item> m_spWeapon;
	CStr m_RenderAttachedObj;
	int32 m_LastActivation;
	int32 m_TempGUID;
	//int32 m_TurretFlags;
	int16 m_TrackingTime; //Hack! This is used by only AI, but should instead be a physical limitation of how fast we can track with turret.
	bool m_bDismountAvailable;

	void Spawn(bool _bSpawn);
	//void CheckAttached();
	static void GetRenderAttachPos(CWorld_PhysState* _pWPhys, CWO_Turret_ClientData& CD, CMat4Dfp32& _Mat, fp32 _IPTime);

	void UpdateVisibilityFlag();
	static bool OnRefresh_WeaponLights(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMat4Dfp32& _Pos, fp32 _IPTime, CMat4Dfp32* _pMat);
	static bool OnRefresh_WeaponLights_Model(CWObject_CoreData* _pObj, CWO_Turret_ClientData& CD, const int& _iLight, CWorld_PhysState* _pWPhysState,
		CXR_SceneGraphInstance* _pSGI, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel, const CMTime& _GameTime,
		CXR_AnimState& _AnimState, CMat4Dfp32* _pMat, const fp32& _IPTime, const bool& _bMuzzleFlash, const bool& _bFlashlight);
	void GetShootDirection0(CMat4Dfp32& _Cam);
	void GetShootDirection1(CMat4Dfp32& _Cam);
	static void OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos = NULL, bool _bIgnoreCache = false);
	static bool GetEvaluatedPhysSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime, const CMat4Dfp32* _pPos);
	static const CWO_Turret_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_Turret_ClientData& GetClientData(      CWObject_CoreData* _pObj);

	virtual void OnCreate();								// Called immediately after object has been created and has recieved object-index and server-pointer. It is called EVERY time an object is created, even if it's from a savegame.
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);			// Called (after OnCreate) once for every key when spawning from an XW file for the first time.
	virtual void OnFinishEvalKeys();						// Called when all keys of the entity has been processed (which is after OnCreate()) with OnEvalKey.
	virtual aint OnMessage(const CWObject_Message& _Msg);	// Called at any time when object receives a message.
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;	// Called whenever a delta-update should be compiled.
	virtual void OnRefresh();								// Called once every game-tick, unless CWO_CLIENTFLAG

	//static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);							// Called when resource-manager has received notification that the class will be used, and thus gives the class opportunity to include other classes.
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);				// Called once every game-tick if the object is potentially visible, unless CWO_CLIENTFLAGS_NOREFRESH is specified
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);// Called in addition to OnClientRender on objects with CWO_CLIENTFLAGS_VISIBILITY. OnClientRenderVis is called before OnClientRender
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);	// Called zero, one or several times for every frame rendered. Should not modify the state of the object in ways that make assumptions of the call-rate. (i.e, it's perfectly legal for the client to call the object 20 times for one rendered frame)
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnFinishDeltaLoad();						// Called after loading delta savegame (also applied when spawning level without savegame)
};

#endif
