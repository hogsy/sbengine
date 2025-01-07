#ifndef WAGI_StateInstQueue_h
#define WAGI_StateInstQueue_h

//--------------------------------------------------------------------------------
/*
#include "WAGI.h"
#include "WAGI_Context.h"
*/
#include "WAGI_StateInst.h"
#include "WAGI_StateInstPacked.h"
#include "WAGI_SIID.h"

class CWAGI;
class CWAGI_Context;
//class CWAGI_StateInstance;

//--------------------------------------------------------------------------------

#define AGI_STATEINSTANCEINDEX_NULL ((uint8)(-1))

//--------------------------------------------------------------------------------

#define SIQFLAGS_PREDMISS_ADD		0x01
#define SIQFLAGS_PREDMISS_REMOVE	0x02

//--------------------------------------------------------------------------------

class CWAGI_SIQ
{
	private:

		TArray<CWAGI_StateInstance>		m_lStateInstances;
		CWAGI_SIID						m_PlayingSIID;
		CMTime							m_PreviousPlayingSequenceTime;
		int16							m_iPreviousPlayingSequence;
		uint8							m_Flags;

		

		/*CXR_Anim_BreakoutPoints			m_BreakoutPoints;
		CXR_Anim_EntryPoints			m_EntryPoints;*/

	public:

		CWAGI_SIQ() { Clear(); }
		void Clear();

		void CopyFrom(const CWAGI_SIQ* _pSIQ);

		void SetQueueAGI(CWAGI* _pAGI)
		{
			for (int i = 0; i < m_lStateInstances.Len(); i++)
				m_lStateInstances[i].SetAGI(_pAGI);
		}

		void SetQueueSIQ()
		{
			for (int i = 0; i < m_lStateInstances.Len(); i++)
				m_lStateInstances[i].SetSIQ(this);
		}

virtual	CWAGI* GetAGI() pure;

		uint8 GetNumStateInstances() const { return m_lStateInstances.Len(); }
		const CWAGI_StateInstance* GetStateInstance(uint8 _iSI) const { return &(m_lStateInstances[_iSI]); }

		const CWAGI_StateInstance* GetTokenStateInstance() const;
		CWAGI_StateInstance* GetTokenStateInstance();

		//fp32 GetPrevStateInstanceAnimTimeOffset(CWAGI_Context* _pContext, CWAGI_SIID* _pSIID, int8 _iTimeOffsetType);

		void RefreshQueue(const CWAGI_Context* _pContext);
		void RefreshPredictionMisses(const CWAGI_Context* _pContext);
		bool OnEnterState(const CWAGI_Context* _pContext, int16 _iEnterAction, fp32 _ForceOffset = 0.0f);
		bool OnLeaveState(const CWAGI_Context* _pContext, int16 _iLeaveAction);

		bool DisableStateInstanceAnims(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance, int _iDisableStateInstanceAnimsCallbackMsg) const;
		void GetAnimLayers(const CWAGI_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg, bool _bAllowBlend);
		bool GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iAnim, int32 _StartTick) const;
		CAGStateIndex GetSpecificState(const CWAGI_Context* _pContext, int32 _iAnim) const;

		void PredictionMiss_AddStateInstance(CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP);
		void PredictionMiss_RemoveStateInstance(CWAGI_Context* _pContext, const CWAGI_SIID* _pSIID);
		void PredictionMiss_Remove(CWAGI_Context* _pContext);

		void ForceMaxStateInstances(int32 _MaxQueued);

		void ResetPMFlags();
		bool IsPMAdded() { return ((m_Flags & SIQFLAGS_PREDMISS_ADD) != 0); }
		bool IsPMRemoved() { return ((m_Flags & SIQFLAGS_PREDMISS_REMOVE) != 0); }
		
//		void RemoveMatchingStateInstance(const CWAGI_SIID* _pRemSIID);
		CWAGI_StateInstance* CreateAndEnqueueStateInstance(const CWAGI_Context* _pContext, CWAGI_SIID* _pSIID);
		CWAGI_StateInstance* GetMatchingStateInstance(const CWAGI_SIID* _pSIID, bool _bCreateNonExistent);

		CWAGI_SIID GetPlayingSIID() const { return m_PlayingSIID; }
		void SetPlayingSIID(const CWAGI_SIID& _PlayingSIID) { m_PlayingSIID = _PlayingSIID; }
		uint8 ResolvePlayingSIID();

};

//--------------------------------------------------------------------------------

#endif /* WAGI_StateInstQueue_h */
