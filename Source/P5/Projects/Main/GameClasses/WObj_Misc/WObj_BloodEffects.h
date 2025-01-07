/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_BloodEffects.h

Author:			

Copyright:		2006 Starbreeze Studios AB

Contents:		CWObject_BloodEffect
				CWObject_BloodSystem

Comments:

History:
\*____________________________________________________________________________________________*/
#ifndef __INC__WObj_BloodEffects_h__
#define __INC__WObj_BloodEffects_h__


#include "../WObj_Weapons/WObj_Spells.h"
#include "../Models/WModel_BloodEffect.h"
#include "../Models/WModel_EffectSystem.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Packer.h"


#ifndef M_RTM
	#define BLOODSYSTEM_DEBUG
#endif


#define CDATAPACKER(_Name, _Index, _Mask, _Shift) \
		const int32 _Name(CWObject_CoreData* _pObj) const { return m_##_Name.Get(_pObj); } \
		void _Name(CWObject_CoreData* _pObj, int32 _Param) { m_##_Name.Set(_pObj, _Param); m_DirtyMask |= CWO_DIRTYMASK_DATA; } \
		static CDataPacker<_Index, _Mask, _Shift>	m_##_Name


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Enums
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	// Max
	BLOODSYSTEM_WALLMARK_MAX	= 16,			// 0...63
	BLOODSYSTEM_FLOORMARK_MAX	= 16,			// 0...63

	BLOODSYSTEM_CLIENTDATA		= 0,			// Client data spot

	// Dirty flags
	OBJ_BLOODSYS_DIRTY_CREATE	= M_Bit(2),		// System created, need to send initialization data
	OBJ_BLOODSYS_DIRTY_MASK		= 0xFFFC,		// Ignore (CWO_CLIENTUPDATE_AUTOVAR | CWO_CLIENTUPDATE_EXTRADATA)

	// Flags
	OBJ_BLOODSYS_FLAGS_SPURT	= M_Bit(0),		// Handle spurt effect
	OBJ_BLOODSYS_FLAGS_CLOUD	= M_Bit(1),		// Handle cloud effect
	OBJ_BLOODSYS_FLAGS_ENTRANCE	= M_Bit(2),		// Handle entrance hole effect
	OBJ_BLOODSYS_FLAGS_EXIT		= M_Bit(3),		// Handle exit hole
	OBJ_BLOODSYS_FLAGS_MODELS	= M_Bit(4),		

	// Runtime flags
	OBJ_BLOODSYS_FLAGS_0x0020	= M_Bit(5),
	OBJ_BLOODSYS_FLAGS_0x0040	= M_Bit(6),
	OBJ_BLOODSYS_FLAGS_0x0080	= M_Bit(7),
	OBJ_BLOODSYS_FLAGS_0x0100	= M_Bit(8),
	OBJ_BLOODSYS_FLAGS_0x0200	= M_Bit(9),
	OBJ_BLOODSYS_FLAGS_0x0400	= M_Bit(10),
	OBJ_BLOODSYS_FLAGS_0x0800	= M_Bit(11),
	OBJ_BLOODSYS_FLAGS_0x1000	= M_Bit(12),
	OBJ_BLOODSYS_FLAGS_0x2000	= M_Bit(13),
	OBJ_BLOODSYS_FLAGS_0x4000	= M_Bit(14),
	OBJ_BLOODSYS_FLAGS_CINFO	= M_Bit(15),		// Handle collision info

	// Masks
	OBJ_BLOODSYS_SPURT_MASK		= (OBJ_BLOODSYS_FLAGS_ENTRANCE | OBJ_BLOODSYS_FLAGS_EXIT),	// Spurt flags
	OBJ_BLOODSYS_CLOUD_MASK		= (OBJ_BLOODSYS_FLAGS_ENTRANCE | OBJ_BLOODSYS_FLAGS_EXIT),	// Cloud flags
	OBJ_BLOODSYS_SYSTEM_MASK	= (OBJ_BLOODSYS_FLAGS_SPURT | OBJ_BLOODSYS_FLAGS_CLOUD | OBJ_BLOODSYS_FLAGS_MODELS),	// System flags

	// Data
//	OBJ_BLOODSYS_DATA_OWNER		= 0,			// Owner data,
//	OBJ_BLOODSYS_DATA_SEED		= 1,			// Seed value (Creation game tick)
//	OBJ_BLOODSYS_DATA_FLAGS0	= 2,			// System flags / Spurt flags
//	OBJ_BLOODSYS_DATA_3_RSRVD	= 3,			// Reserved.
//	OBJ_BLOODSYS_DATA_FLAGS1	= 4,			// Cloud flags
//	OBJ_BLOODSYS_DATA_5			= 5,			// ...
//	OBJ_BLOODSYS_DATA_6			= 6,			// ...
//	OBJ_BLOODSYS_DATA_7			= 7,			// ...

	FXBLOOD_NETMSG_OLDDECAL		= 0,
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_BloodEffect_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_BloodEffect_ClientData : public CReferenceCount
{
public:
	struct SSpurtPoint
	{
		enum
		{
			FLAGS_MOVEMENT	= M_Bit(0),
			FLAGS_COLLISION	= M_Bit(1),
		};

		CVec3Dfp32	m_Pos;
		CVec3Dfp32	m_Vel;
		uint8		m_Flags;
	};

	struct SDecalData
	{
		int16 m_WallSurfaceID;
		int16 m_FloorSurfaceID;
		uint8 m_WallSize;
		uint8 m_FloorSize;
	};

	struct SSpurtPointsParam
	{
		uint8	m_nDecalPointsEntrance;
		uint8	m_nDecalPointsExit;
		uint8	m_nHelpPointsEntrance;
		uint8	m_nHelpPointsExit;

		void operator = (const SSpurtPointsParam& _Param)
		{
			m_nDecalPointsEntrance	= _Param.m_nDecalPointsEntrance;
			m_nDecalPointsExit		= _Param.m_nDecalPointsExit;
			m_nHelpPointsEntrance	= _Param.m_nHelpPointsEntrance;
			m_nHelpPointsExit		= _Param.m_nHelpPointsExit;
		}
	};

	CWorld_Client*		m_pWClient;
	CWObject_CoreData*	m_pObj;
	CXR_Model*			m_pBloodModel;
	
	int32				m_CreationTick;
	uint32				m_nActivePoints;

	uint8 				m_nDecalPointsEntrance;
	uint8 				m_nDecalPointsExit;
	uint8 				m_nHelpPointsEntrance;
	uint8 				m_nHelpPointsExit;

	CVec3Dfp32			m_ExitHole;

	fp32				m_Strength1;
	fp32				m_Strength2;
	fp32				m_Strength3;
	fp32				m_Strength4;

	TThinArray<SSpurtPoint>	m_lDecalPoints;
	TThinArray<SSpurtPoint>	m_lHelpPoints;
	TThinArray<SDecalData>	m_lDecalData;

	CWO_BloodEffect_ClientData();
	~CWO_BloodEffect_ClientData();

	void Clear(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, const int16 _iBloodModel);
	void AllocSpurtPoints(const CMat4Dfp32& _PosMat, const CVec3Dfp32& _ExitPos, CRand_MersenneTwister* pRand);
	void OnClientUpdate(const uint8*& _pD, const uint16 _SystemFlags, const uint16 _SpurtFlags, const uint16 _CloudFlags);
	void RefreshSpurtPoints();

	CBloodEffectModelParam* CreateModelData(CXR_VBManager* _pVBM);

	#ifdef BLOODSYSTEM_DEBUG
		void Debug_RenderSpurtPoints();
	#endif
};


typedef CWO_BloodEffect_ClientData::SSpurtPointsParam SBloodSpurtPointsParam;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_BloodEffect : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

private:
	typedef CWObject_Ext_Model parent;

	struct SBloodDecal
	{
		int16	m_SurfaceID;
		uint8	m_Size;
	};

	struct SMinMax
	{
		uint8		m_MinEntrance;
		uint8		m_MaxEntrance;
		uint8		m_MinExit;
		uint8		m_MaxExit;
	};

	// Server:
	SBloodDecal		m_lWallmark[BLOODSYSTEM_WALLMARK_MAX];
	SBloodDecal		m_lFloormark[BLOODSYSTEM_FLOORMARK_MAX];
	uint8			m_nWallmarks;
	uint8			m_nFloormarks;
	fp32			m_Duration;
	SMinMax			m_DecalPoints;
	SMinMax			m_HelpPoints;
	CVec3Dfp32		m_ExitHole;

	fp32			m_Strength1;
	fp32			m_Strength2;
	fp32			m_Strength3;
	fp32			m_Strength4;

	CDATAPACKER(SystemType,			0, 0x0000FFFF,	0);		// System flags
	CDATAPACKER(SpurtType,			0, 0x0000FFFF,	16);	// Spurt flags
	CDATAPACKER(CloudType,			1, 0x0000FFFF,	0);		// Cloud flags
	CDATAPACKER(iOwnerData,			1, 0x00003FFF,	16);	// Owner
	//CDATAPACKER(iFree,			1, 0x00000003,	30);	// 2 bits free
	CDATAPACKER(Seed,				2, 0xFFFFFFFF,	0);		// Seed value
	CDATAPACKER(iBloodModel,		3, 0x0000FFFF,	0);		// Blood model
	//CDATAPCKAER(iFree,			3, 0x0000FFFF,	16);	// 16 bits free
	CDATAPACKER(DecalScaleEntrance,	4, 0xFFFFFFFF,	0);		// Scale floormark decals

	static const char* m_slBloodSystemFlagsTranslate[];

	// Helpers for setting up effect data
	void Create_BloodSpurt(const CVec3Dfp32& _HitDir, const bool _bEntrance, const bool _bExit);
	void Create_BloodCloud(const bool _bEntrance, const bool _bExit);
	void Create_BloodData(const CVec3Dfp32& _HitPos, const CVec3Dfp32& _HitDir, const uint16 _SystemFlags, const bool _bEntrance, const bool _bExit);

	// Client Data
	static CWO_BloodEffect_ClientData*	AllocClientData(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, const int16 _iBloodModel);
	static CWO_BloodEffect_ClientData*	GetClientData(CWObject_CoreData* _pObj);

	// Old system
	#define SimpleEffect_F(_t) (Splash_Origin + Splash_Dir * Splash_Velocity * _t + Splash_Gravity * 0.5f * _t * _t)
	static int  SimpleEffect_RandExclude(int _nRand, int _Exclude);
	       void SimpleEffect_OnInitInstance();
	       void SimpleEffect_TraceWallmark();

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnRefresh();
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	
	static void OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
class CWObject_BloodEffect : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

private:
	enum
	{
		MAXWALLMARKS = 10,
		MAXFLOORMARKS = 10,

		NETMSG_CREATEWALLMARK = 0x40,
	};

	fp32			m_Splash_Velocity;
	CVec3Dfp32	m_Splash_Gravity;
	fp32			m_Wallmark_MaxDistanceSize;
	fp32			m_Wallmark_MinDistanceSize;
	fp32			m_Wallmark_ProjectionDepth;

	int			m_Wallmark_iSurface[MAXWALLMARKS];
	uint8		m_Wallmark_Sizes[MAXWALLMARKS];
	int			m_Wallmark_nSurfaces;
	int			m_Floormark_iSurface[MAXFLOORMARKS];
	uint8		m_Floormark_Sizes[MAXFLOORMARKS];
	int			m_Floormark_nSurfaces;

	int			m_Splash_nMaxIterations;
	fp32			m_Splash_WantedApproximation;
	fp32			m_Splash_MaxApproximationError;
	fp32			m_Splash_TimeStepErrorCorrectionScale;

	fp32			m_Splash_SafeTracedTime;
	fp32			m_Splash_TimeStep;

	fp32			m_Wallmark_SpawnTime;

	CVec3Dfp32	m_Wallmark_Pos;
	CVec3Dfp32	m_Wallmark_Normal;

	CVec3Dfp32	m_Splash_Origin;
	CVec3Dfp32	m_Splash_Dir;

private:
	CVec3Dfp32 F(fp32 _t) { return (m_Splash_Origin + m_Splash_Dir * m_Splash_Velocity * _t + m_Splash_Gravity * 0.5f * _t * _t); }

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	void TraceWallmark();
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
};
*/

#endif

