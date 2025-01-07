//#include "WObj_System.h"
#include "WObj_Hook.h"
#include "WObj_RailSystem.h"
#include "WObj_Packer.h"

// -------------------------------------------------------------------
// 
//	Rail
//
// -------------------------------------------------------------------
namespace NRail
{
	//
	//
	//
	class CWObject_RailHandler;
	class CWObject_RailWagon;
	class CWObject_Rail;

	// types
	typedef uint16 NodeIndex;
	typedef uint16 DirNodeIndex;
	typedef uint8 WagonIndex;
	typedef uint16 PriorityIndex;

	class CTeamHandler
	{
	public:
		TArray<uint16> m_liTeams;

		int IsMember(uint16 _iTeam);

		void AddTeam(uint16 _iTeam);
		void AddTeam(CWorld_Server *_pWServer, const char *_pName);
		int IsTeamMessage(const CWObject_Message& _Msg);
		aint OnMessage(const CWObject_Message& _Msg);
	};

	//
	//
	//
	class CWObject_RailMessagePoint : public CWObject
	{
	MRTC_DECLARE_SERIAL_WOBJECT;
	public:
		enum
		{
			TYPE_SCAN=0,
			TYPE_LOST,
			TYPE_DETECT,
			TYPE_DESTROY,
			TYPE_ENEMY,
			TYPE_CORPSE,
			TYPE_MELEEFIGHT,
			NUMTYPES,
		};

		TArray<CWO_SimpleMessage> m_lMessages[NUMTYPES];
		TArray<CStr> m_lUsers;
		virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	};

	//
	//
	//
	class CWObject_RailWagon : public CWObject_Interface_AI //CWObject_Model
	{
		MRTC_DECLARE_SERIAL_WOBJECT;
	public:
		enum
		{
			MODELMODE_RAIL,
			MODELMODE_STATIC,
			MODELMODE_STATIC_DESTROYED,

			FLAG_REMOVE=1,

			TYPE_TRANSPORT=0,
			TYPE_CAMERA=1,
			TYPE_TURRET=2,

			GOAL_ROAM=0,		// randomly explore the network
			GOAL_DISMISS=1,		// find an exit point, don't shoot or anything
		};

		static CDataPacker<0,0xFFFFFFFF,0> m_Data_UpVector0;
		static CDataPacker<1,0xFFFFFFFF,0> m_Data_UpVector1;
		static CDataPacker<2,0xFFFFFFFF,0> m_Data_Direction0;
		static CDataPacker<3,0xFFFFFFFF,0> m_Data_Direction1;

		//
		static CDataPacker<4,0xFFFF,16> m_Data_iLaserBeam;
		static CDataPacker<4,0xF,12> m_Data_iAttach_Laser;
		static CDataPacker<4,0xF,8> m_Data_iAttach_Look;
		static CDataPacker<4,0xF,4> m_Data_Scan;
		static CDataPacker<4,0xF,0> m_Data_FireFlash;

		//
		static CDataPacker<5,0x3,0> m_Data_FlashLight;
		static CDataPacker<5,0x3,2> m_Data_ModelMode;

		//static CDataPacker<0,0xFF,16> m_ColorG;
		//static CDataPacker<0,0xFF,8> m_ColorB;


		//
		uint8 m_Flags;
		uint8 m_Type;
		uint8 m_Static;
		uint8 m_CanUseLight;

		PriorityIndex m_Priority;

		int16 m_iHandler;
		CWObject_RailHandler &GetHandler();

		//
		int32 m_Goal;
		int32 m_Connected;
		CVec3Dfp32 m_Velocity;

		// position
		fp32 m_TravelTime; // 0 < x < 1
		NodeIndex m_iPosition;
		NodeIndex m_iNextPosition;

		//
		fp32 m_Speed_Normal;
		fp32 m_Speed_Search;
		fp32 m_Speed_Detected;
		fp32 m_Brake;

		//
		fp32 m_CurrentSpeed;
		fp32 m_MaxSpeed;
		fp32 m_TimeScale;

		//
		int16 m_Health;

		//
		int16 m_iSound_Engine;
		int16 m_iSound_Destroy;
		int16 m_iSound_Fire;

		enum
		{
			RAIL_SOUND_SCAN = 0,
			RAIL_SOUND_LOST,
			RAIL_SOUND_DETECT,
			RAIL_SOUND_CORPSE,
			RAIL_SOUND_MELEE,
			RAIL_SOUND_ENEMY,
		};

		int16 m_iSound_Scan;
		int16 m_iSound_Lost;
		int16 m_iSound_Detect;
		int16 m_iSound_Corpse;
		int16 m_iSound_MeleeFight;
		int16 m_iSound_Enemy;

		int16 m_iSound_Turn;
		int16 m_iSound_TurnStop;
		int16 m_iSound_TurnStart;

		// movement
		CStr m_EnginePathName;
		int32 m_iEnginePath;

		fp32 m_Pitch_Min;
		fp32 m_Pitch_Max;
		fp32 m_Heading_Min;
		fp32 m_Heading_Max;
		fp32 m_PauseTime; // user
		fp32 m_SweepTime; // user
		fp32 m_MovementTime; // 0 < x < Max(m_PauseTime, m_SweepTime)
		int32 m_MovementPhase;

		// Target object to automatically shoot at regardless of enemies etc
		int32 m_iScriptedTarget;

		bool m_Drops;

		//
		uint16 m_ScanTime;				// in ms	(AB: Why not tick?)
		uint16 m_ScanToDetectTime;		// in ms (AB: Why not tick?)
		int m_LastEnemyMsgTick;
		CVec3Dfp32 m_LastScanPos;

		//
		CStr m_DestroyEffect;

		//
		CVec3Dfp32 m_UpVector;
		CVec3Dfp32 m_AimDirection;

		//
		TArray<NodeIndex> m_lPlannedPath;

		// turret
		CStr m_Projectile;
		CStr m_MuzzleFlash;
		uint32 m_SpawnedObjectGUID;
		fp32 m_ReloadCounter; // 0 < x < Max(m_ReloadTime+m_ReloadTimeRandom, m_ReloadTimeMagazine)
		
		int32 m_AmmoLeft;
		int32 m_MagazineSize;

		fp32 m_ReloadTimeMagazine; // user
		fp32 m_ReloadTime; // user
		fp32 m_ReloadTimeRandom; // user
		fp32 m_AimRandom;

#ifndef M_RTM
		int8 m_DestructStatus;
#endif
		//
		CWObject_RailWagon();
#ifndef M_RTM
		~CWObject_RailWagon();
#endif


		void UpdatePosition(fp32 _TravelTime);
		void ReachedNode();

		void Disconnect();
		void Fire(const CMat4Dfp32 &_Mat);

		// Ai Stuff
		TPtr<class CAI_Core_Turret> m_spAI;
		CTeamHandler m_TeamHandler;

		void Destroy();

		virtual uint AI_GetTeams(uint16* _piResult, uint _MaxElem) const;
		virtual CAI_Core* AI_GetAI();
		virtual bool AI_IsAlive();
		virtual void AI_GetAimPosition(CVec3Dfp32& _RetValue);
		virtual void AI_GetLookMatrix(CMat4Dfp32& _RetValue);
		void ClearKnowledge();
		static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*);

		virtual aint OnMessage(const CWObject_Message& _Msg);

		virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
		virtual void OnCreate();
		virtual void OnDestroy();
		virtual void OnRefresh();

		virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
		virtual void OnDeltaSave(CCFile* _pFile);

		virtual void OnSpawnWorld();

		static void GetPointMatrix(CWObject_CoreData *_pObj, CWorld_PhysState *_pWorld, CXR_AnimState &State, CMat4Dfp32 &_Mat, int32 _iBone, int32 _iAttach=-1);
		static void GetPointMatrix(CWObject_CoreData *_pObj, CWorld_PhysState* _pWorld, CMat4Dfp32 &_Mat, int32 _iBone, int32 _iAttach=-1);

		static CXR_Skeleton *GetSkeleton(CWObject_CoreData* _pObj, CWorld_PhysState* _pWorld);
		static bool GetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWClient, CXR_AnimState& _AnimState, const CMat4Dfp32& _Matrix, fp32 _AnimTime, fp32 _TickFrac, int _iModel);
		static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);
		static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
		static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat);
	};

	//
	//
	//
	class CWObject_RailHandler : public CWObject
	{
	MRTC_DECLARE_SERIAL_WOBJECT;
	public:
		// enums
		enum 
		{
			RAIL_MAXNODES=1024,
			RAIL_MAXCONNECTION=8,

			RAIL_INVALIDNODE=0xFFFF,
		};

		//
		//
		//
		class CNode
		{
		public:
			enum
			{
				FLAG_ENTRY=1
			};

			CVec3Dfp32 m_Position;
			CVec3Dfp32 m_Direction;
			CVec3Dfp32 m_UpVector;
			
			uint8 m_Flags;

			PriorityIndex m_Priority;

			DirNodeIndex m_iDirNodeStart;
			uint8 m_NumDirNodes;
		};

		//
		//
		//
		class CDirNode
		{
		public:
			//NodeIndex m_iThis;
			//uint32 m_Bend:2;
			NodeIndex m_iNext;
		};

		//
		// general properties
		//l
		bool m_GotPower;
		fp32 m_PowerLevel;


		//
		// Wagon Handling
		//
		TArray<int32> m_lMobileWagons;	// those that are on the rail
		TArray<int32> m_lWagons;		// all wagons
		bool AllScanLost(const CWObject_RailWagon* _pCallWagon);
		void AddUniqueProcessedDead(int _iDead);
		void AddUniqueEnemy(int _iEnemy);
		void SetUnfriendly(int _iUnfriendly);
		// List of corpses detected and processed
		TArray<int32> m_lProcessedDead;
		// List of new enemies from spotted perps
		TArray<int32> m_lEnemies;
		// Object that all wagons should attack when nonzero
		int32 m_iScriptedTarget;
		void SetPower(bool _State);
		bool GotPower();

		//WagonIndex AddWagon(CWObject_RailWagon &_rWagon);
		CWObject_RailWagon &GetWagon(const WagonIndex _Index);
		CWObject_RailWagon &GetWagonFromObj(const int32 _iObj);
		int32 NumWagons();

		CWObject_RailWagon *CreateWagon(const char *_pName, const NodeIndex _iStart, const NodeIndex _iDirection);
		
		void DeleteWagon(const WagonIndex _Index);

		NodeIndex GetClosestNode(CVec3Dfp32 &_Point);
		fp32 GetTimeScale(const NodeIndex _iStart, const NodeIndex _iNext);
		CMat4Dfp32 GetMatrix(const NodeIndex _iStart, const NodeIndex _iDirection, fp32 _Time = 0.0f);
		CWObject_RailMessagePoint *GetMessagePoint(CVec3Dfp32 _Point, const char *_pName);
		void SendMessages(CWObject_RailWagon *_pWagon, CVec3Dfp32 _Point, int32 _Type,int32 _iActivator = 0);

		//
		// Nodes
		//
		CNode *m_pNodes;
		int32 m_NumNodes;

		CDirNode *m_pDirNodes;
		int32 m_NumDirNodes;

		NodeIndex *m_pEntryNodes;
		int32 m_NumEntryNodes;

		// The last sound type played by the a wagon in the system
		int m_iLastPlaySound;
		// Tick when a sound was last played by the system
		int m_LastPlaySoundTick;
		// Minimum duration betweeb sounds of equal priority in system
		int m_SoundPeriod;
		int m_CanFireTick;

		bool CanPlaySound(int _iSound);
		bool CanFire();

		inline CNode &GetNode(NodeIndex _Index) { AssertNode(_Index); return m_pNodes[_Index]; }
		CDirNode &GetDirNode(DirNodeIndex _Index) { AssertDirNode(_Index); return m_pDirNodes[_Index]; }

		int32 GetPotentialNodes(const NodeIndex _iThis, const NodeIndex _iPrev, NodeIndex _aiNodes[], int32 _Max);

		void Prioritize(const NodeIndex _iNode);

	// some extra checks in debug
	#ifdef M_RELEASE
		inline const bool IsValidNode(const NodeIndex _iNode) { return (_iNode!=RAIL_INVALIDNODE); }
		inline void AssertNode(const NodeIndex _iNode) { }
		inline void AssertDirNode(const DirNodeIndex _iNode) { }
		inline void AssertInit() {}
	#else
		inline const bool IsValidNode(const NodeIndex _iNode) { return (_iNode!=RAIL_INVALIDNODE && _iNode<m_NumNodes); }
		inline void AssertNode(const NodeIndex _iNode) { M_ASSERT(IsValidNode(_iNode), "Invalid node"); }
		inline void AssertDirNode(const DirNodeIndex _iNode) { M_ASSERT(_iNode!=RAIL_INVALIDNODE && _iNode<m_NumDirNodes, "Invalid dir node"); }
		inline void AssertWagon(const WagonIndex _iWagon) { M_ASSERT(_iWagon < NumWagons(), "Invalid wagon index"); }

		inline void AssertInit()
		{
			M_ASSERT(m_pNodes, "Nodes are missing");
			M_ASSERT(m_pDirNodes, "Nodes are missing");
			M_ASSERT(m_pEntryNodes, "Nodes are missing");
		}

		inline bool ShouldUpdate()
		{
			if(!m_pNodes || !m_pDirNodes || !m_pEntryNodes)
				return false;
			return true;
		}

	#endif

		// returns true if nodes are connected
		const bool CheckConnectivity(const uint16 _iNode1, const uint16 _iNode2);

		//
		CWObject_RailHandler();
		~CWObject_RailHandler();
		virtual void OnCreate();
		virtual void OnRefresh();
		
		// Load and store processed dead and changed relations
		virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
		virtual void OnDeltaSave(CCFile* _pFile);
		
		bool GetTargetsByName(int _iSender,int _iObject,CStr _Names,TArray<int16> _liTargets);
		virtual aint OnMessage(const CWObject_Message &_Msg);
	};

	//
	//
	//
	class CWObject_Rail : public CWObject_Engine_Path
	{
		MRTC_DECLARE_SERIAL_WOBJECT;
	public:
		virtual void OnSpawnWorld();
	};
}
