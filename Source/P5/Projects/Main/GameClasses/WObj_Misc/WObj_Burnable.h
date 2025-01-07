/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Burnable.h

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CWO_Burnable

Comments:		OnClientUpdate_Burning
					- Add in 'OnClientUpdate' to take care of model instance data
				
				OnCreateClientUpdate_Burning
					- Add in 'OnCreateClientUpdate' to make sure dirty flags is cleared out

				OnClientRender_Burning
					- Add in 'OnClientRender' for default rendering

				OnEvalKey_Burning
					- Add in 'OnEvalKey' to implement user defined models from templates

History:
	060523:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_BURNABLE
#define _INC_WOBJ_BURNABLE


#include "../../../../Shared/MOS/Classes/GameWorld/WObjCore.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "../WObj_Sys/WObj_Physical.h"


#define BURNABLE_FLAGS_SKELETON(_x)		M_Bit((BURNABLE_FLAGS_SKELETON0 + _x))


#define CWO_BURNABLEACCESS_RW(_Type, _Name) \
public: \
	const _Type& _Name() const	{ return m_##_Name; } \
		  _Type& _Name()		{ m_bDirty = true; return m_##_Name; } \
private: \
	_Type m_##_Name


#define CWO_BURNABLEACCESS_RW_ARRAY(_Type, _Name, _Size) \
public: \
	const _Type& _Name(uint _i) const	{ return m_##_Name[_i]; } \
		  _Type& _Name(uint _i)			{ m_bDirty = true; return m_##_Name[_i]; } \
private: \
	_Type m_##_Name[_Size]


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Burnable
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Burnable
{
private:
	enum
	{
		BURNABLE_NUM_MODELS			= 3,
		BURNABLE_FLAGS_ISBURNING	= M_Bit(0),	// Indicates burning state
		BURNABLE_FLAGS_SAVETIME		= M_Bit(1),

		BURNABLE_FLAGS_SAVEMASK		= BURNABLE_FLAGS_SAVETIME,
	};

	// Server
	bool m_bDirty;
//	int16			m_Flashpoint;		// Flashpoint for object
//	int16			m_Temperature;		// Temperature of object
//	fp64			m_Wetness;
	

	// Client
	TPtr<CXR_ModelInstance>	m_lModelInstance[BURNABLE_NUM_MODELS];

	// Server & Client
	CWO_BURNABLEACCESS_RW_ARRAY(int32, iBurnModel, BURNABLE_NUM_MODELS);
	CWO_BURNABLEACCESS_RW_ARRAY(uint8, iBurnSkelType, BURNABLE_NUM_MODELS);
	CWO_BURNABLEACCESS_RW(int32, BurnStartTick);
	CWO_BURNABLEACCESS_RW(int32, BurnEndTick);
	CWO_BURNABLEACCESS_RW(uint8, BurnFlags);
	CWO_BURNABLEACCESS_RW(int32, BurnTime);

	// Server helpers
	static void IncludeModelFromKey(const char* _pKey, CRegistry* _pReg, CMapData* _pMapData);

	// Client helpers
	void UpdateModelInstance(uint32 _iModelInstance, CWObject_CoreData* _pObj, CWorld_Client* _pWClient);

public:
	CWO_Burnable();

	bool IsBurning() const;
	bool IsBurned() const;
	bool SetBurning(bool _bBurning, CWorld_Server* _pWServer);

	// Server
	bool OnEvalKey(CWorld_Server* _pWServer, uint32 _KeyHash, const CRegistry* _pReg);
	bool SetValidModel(CWorld_Server* _pWServer);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer);
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	void CopyBurnable(const CWO_Burnable& _Burnable);
	bool OnRefresh(CWObject_CoreData* _pObj, CWorld_Server* _pWServer);

	// Client
	void OnClientRefresh(CWObject_CoreData* _pObj, CWorld_Client* _pWClient);
	void OnClientRender(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _WMat, CXR_Engine* _pEngine, CXR_Skeleton* _pSkeleton = NULL, CXR_SkeletonInstance* _pSkeletonInstance = NULL, const CBox3Dfp32* _pBBox = NULL, const uint32* _pnBBox = NULL) const;
	void OnClientUpdate(CWObject_CoreData* _pObj, CWorld_Client* _pWClient);
	CMTime GetBurnTime(CWorld_Client* _pWClient) const;

	// Replication
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);
};


M_INLINE bool CWO_Burnable::IsBurning() const
{
	return (m_BurnFlags & BURNABLE_FLAGS_ISBURNING) != 0;
}

M_INLINE bool CWO_Burnable::IsBurned() const
{
	return ((m_BurnFlags & BURNABLE_FLAGS_ISBURNING) || m_BurnTime > 0);
}

M_INLINE CMTime CWO_Burnable::GetBurnTime(CWorld_Client* _pWClient) const
{
	if (m_BurnFlags & BURNABLE_FLAGS_ISBURNING)
		return CMTime::CreateFromTicks((_pWClient->GetGameTick() - m_BurnStartTick) + m_BurnTime, _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());

	return CMTime::CreateFromTicks(m_BurnTime, _pWClient->GetGameTickTime(), 0.0f);
}


typedef CWO_Burnable CWO_Burnable_ClientData; //TEMP! --remove when renamed everywhere...
typedef TAutoVar<CWO_Burnable> CAutoVar_Burnable;


#endif

