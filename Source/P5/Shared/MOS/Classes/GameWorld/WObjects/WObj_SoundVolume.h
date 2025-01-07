#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Hook.h"


// -------------------------------------------------------------------
// 
//	SoundVolume
//
//	Handels sound volumes, see the document about Sound System Version 2
//	for a complete overview of the system.
//
//	m_iAnim0 = Falloff (max 512)
//	{ 
//		m_Data[n*2+0] = Sound Index
//		m_Data[n*2+1] = Direction Vector
//	} n*
//
// -------------------------------------------------------------------
class CWObject_SoundVolume : public CWObject
{
		MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_SoundVolume();

	enum
	{
		CLIENTOBJ_CLIENTDATA = 0,
	};

	struct CClientData : public CReferenceCount, public CAutoVarContainer
	{
		AUTOVAR_SETCLASS(CClientData, CAutoVarContainer);

		CAUTOVAR_OP(CAutoVar_int8, m_ApplyEffects, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_Room, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_RoomHF, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_Reflections, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_Reverb, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_RoomRolloffFactor, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_DecayTime, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_DecayHFRatio, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_ReflectionsDelay, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_ReverbDelay, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_Diffusion, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_Density, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_HFReference, DIRTYMASK_1_5);

		CAUTOVAR_OP(CAutoVar_fp32, m_EnvironmentSize, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_DecayLFRatio, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_EchoTime, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_EchoDepth, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_ModulationTime, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_ModulationDepth, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_AirAbsorptionHF, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_LFReference, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_RoomLF, DIRTYMASK_1_5);

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_ApplyEffects)
		AUTOVAR_PACK_VAR(m_Room)
		AUTOVAR_PACK_VAR(m_RoomHF)
		AUTOVAR_PACK_VAR(m_RoomRolloffFactor)
		AUTOVAR_PACK_VAR(m_DecayTime)
		AUTOVAR_PACK_VAR(m_DecayHFRatio)
		AUTOVAR_PACK_VAR(m_Reflections)
		AUTOVAR_PACK_VAR(m_ReflectionsDelay)
		AUTOVAR_PACK_VAR(m_Reverb)
		AUTOVAR_PACK_VAR(m_ReverbDelay)
		AUTOVAR_PACK_VAR(m_Diffusion)
		AUTOVAR_PACK_VAR(m_Density)
		AUTOVAR_PACK_VAR(m_HFReference)
		AUTOVAR_PACK_VAR(m_EnvironmentSize)
		AUTOVAR_PACK_VAR(m_DecayLFRatio)
		AUTOVAR_PACK_VAR(m_EchoTime)
		AUTOVAR_PACK_VAR(m_EchoDepth)
		AUTOVAR_PACK_VAR(m_ModulationTime)
		AUTOVAR_PACK_VAR(m_ModulationDepth)
		AUTOVAR_PACK_VAR(m_AirAbsorptionHF)
		AUTOVAR_PACK_VAR(m_LFReference)
		AUTOVAR_PACK_VAR(m_RoomLF)
		AUTOVAR_PACK_END

		void Apply(const CSoundContext::CFilter *_pFilter);
		void GetFilterSettings(CSoundContext::CFilter &_Dest);
	};

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnRefresh();

	static aint OnClientMessage(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CWObject_Message &_Msg);

	static const CClientData *GetClientData(const CWObject_CoreData* _pObj);
	static CClientData *GetClientData(CWObject_CoreData* _pObj);
	CClientData *GetClientData();
};
