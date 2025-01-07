#include "PCH.h"

//--------------------------------------------------------------------------------

#ifdef	AG_DEBUG
static bool bDebug = false;
#endif
//--------------------------------------------------------------------------------

#include "WAGI_StateInstQueue.h"
#include "WAGI.h"
#include "WAGI_Context.h"
#include "WAGI_StateInst.h"
#include "WAGI_StateInstPacked.h"

//--------------------------------------------------------------------------------
/*
fp32 CWAGI_SIQ::GetPrevStateInstanceAnimTimeOffset(CWAGI_Context* _pContext, CWAGI_SIID* _pSIID, int8 _iTimeOffsetType)
{
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime() == pStateInstance->m_EnqueueTime) &&
			(_pSIID->GetEnterActionIndex() == pStateInstance->m_iEnterAction))
		{
			if (iStateInstance == 0)
				return 0;

			CWAGI_StateInstance* pPrevStateInstance = &(m_lStateInstances[iStateInstance - 1]);
			return pPrevStateInstance->GetAnimTimeOffset(_pContext, _iTimeOffsetType);
		}
	}

	return 0;
}
*/
//--------------------------------------------------------------------------------

void CWAGI_SIQ::Clear()
{
	MSCOPESHORT(CWAGI_SIQ::Clear);
	m_Flags = 0;
	m_iPreviousPlayingSequence = -1;
	m_PreviousPlayingSequenceTime.Reset();
	m_PlayingSIID.Clear();
	m_lStateInstances.Clear();
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::CopyFrom(const CWAGI_SIQ* _pSIQ)
{
	MSCOPESHORT(CWAGI_SIQ::CopyFrom);
	m_Flags = _pSIQ->m_Flags;
	m_iPreviousPlayingSequence = _pSIQ->m_iPreviousPlayingSequence;
	m_PreviousPlayingSequenceTime = _pSIQ->m_PreviousPlayingSequenceTime;
	m_PlayingSIID = _pSIQ->m_PlayingSIID;
	_pSIQ->m_lStateInstances.Duplicate(&m_lStateInstances);
	SetQueueSIQ();
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::PredictionMiss_AddStateInstance(CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP)
{
	MSCOPESHORT(CWAGI_SIQ::PredictionMiss_AddStateInstance);

	int iSI;
	CWAGI_SIID AddSIID(_pSIP->GetEnqueueTime(), _pSIP->GetEnterActionIndex());
	for (iSI = 0; iSI < m_lStateInstances.Len(); iSI++)
	{
		CWAGI_StateInstance* pSI = &(m_lStateInstances[iSI]);
		CWAGI_SIID SIID(pSI->GetEnqueueTime(), pSI->GetEnterActionIndex());
		if (AddSIID.GetEnqueueTime().Compare(SIID.GetEnqueueTime()) < 0)
		{
			m_lStateInstances.SetLen(iSI); // Set length to one less than before, i.e. deleting this and all later entries.
			iSI = m_lStateInstances.Add(CWAGI_StateInstance(&AddSIID, GetAGI(), this));
			CWAGI_StateInstance* pSI = &(m_lStateInstances[iSI]);
			pSI->PredictionMiss_Add(_pContext, _pSIP);
			ResolvePlayingSIID(); // iPlayingStateInstance may be after the inserted one.
			return;
		}
	}

	iSI = m_lStateInstances.Add(CWAGI_StateInstance(&AddSIID, GetAGI(), this));
	CWAGI_StateInstance* pSI = &(m_lStateInstances[iSI]);
	pSI->PredictionMiss_Add(_pContext, _pSIP);
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::PredictionMiss_RemoveStateInstance(CWAGI_Context* _pContext, const CWAGI_SIID* _pSIID)
{
	MSCOPESHORT(CWAGI_SIQ::PredictionMiss_RemoveStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().AlmostEqual(pStateInstance->m_EnqueueTime)) &&
			(_pSIID->GetEnterActionIndex() == pStateInstance->m_iEnterAction))
		{
			pStateInstance->PredictionMiss_Remove(_pContext);
		}
	}
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::PredictionMiss_Remove(CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_SIQ::PredictionMiss_Remove);

	m_Flags |= SIQFLAGS_PREDMISS_REMOVE;

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		pStateInstance->PredictionMiss_Remove(_pContext);
	}
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::ResetPMFlags()
{
	MSCOPESHORT(CWAGI_SIQ::ResetPMFlags);

	m_Flags = 0;
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		pStateInstance->ResetPMFlags();
	}
}

//--------------------------------------------------------------------------------

CWAGI_StateInstance* CWAGI_SIQ::GetTokenStateInstance()
{
	MSCOPESHORT(CWAGI_SIQ::GetTokenStateInstance_1);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	uint8 iTokenStateInstance = (m_lStateInstances.Len() - 1);
	CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iTokenStateInstance]);

	// Make sure last stateinstance hasn't left it's state. In that case, the token is terminated.
	if (pStateInstance->GetLeaveTime().AlmostEqual(AGI_UNDEFINEDTIME) == false)
		return NULL;

	return pStateInstance;
}

const CWAGI_StateInstance* CWAGI_SIQ::GetTokenStateInstance() const
{
	//MSCOPESHORT(CWAGI_SIQ::GetTokenStateInstance_2);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	uint8 iTokenStateInstance = (m_lStateInstances.Len() - 1);
	const CWAGI_StateInstance* pStateInstance = (&(m_lStateInstances[iTokenStateInstance]));

	// Make sure last stateinstance hasn't left it's state. In that case, the token is terminated.
	if (pStateInstance->GetLeaveTime().AlmostEqual(AGI_UNDEFINEDTIME) == false)
		return NULL;

	return pStateInstance;
}


//--------------------------------------------------------------------------------

uint8 CWAGI_SIQ::ResolvePlayingSIID()
{
	MSCOPESHORT(CWAGI_SIQ::ResolvePlayingSIID);

	if ((m_PlayingSIID.GetEnqueueTime().AlmostEqual(AGI_UNDEFINEDTIME)) && 
		(m_PlayingSIID.GetEnterActionIndex() == AG_ACTIONINDEX_NULL))
	{
		return AGI_STATEINSTANCEINDEX_NULL;
	}

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		const CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((m_PlayingSIID.GetEnqueueTime().AlmostEqual(pStateInstance->GetEnqueueTime())) &&
			(m_PlayingSIID.GetEnterActionIndex() == pStateInstance->GetEnterActionIndex()) &&
			(!pStateInstance->IsPMRemoved()))
		{
			for (int jStateInstance = iStateInstance+1; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
				if ((m_PlayingSIID.GetEnqueueTime().AlmostEqual(m_lStateInstances[jStateInstance].GetEnqueueTime())) &&
					(m_PlayingSIID.GetEnterActionIndex() == m_lStateInstances[jStateInstance].GetEnterActionIndex()))
				{
#ifdef	AG_DEBUG
					if (bDebug)
						ConOutL(CStrF("ERROR: SIQ::ResolvePlayingSIID() - Inconsistent SIQ. Non-unique PSIID <QT %3.3f, iEA %d>", m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex()));
#endif
				}
			}

			return iStateInstance;
		}
	}
	
	return AGI_STATEINSTANCEINDEX_NULL;
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::RefreshQueue(const CWAGI_Context* _pContext)
{
	MSCOPESHORT( CWAGI_SIQ::RefreshQueue );
#ifdef	AG_DEBUG
	CStr Location;
	if( bDebug )
	{
		if (_pContext->m_pWPhysState->IsServer())
			Location = CStrF("(Server %X) SIQ::Refresh()", _pContext->m_pWPhysState);
		else
			Location = CStrF("(Client %X) SIQ::Refresh()", _pContext->m_pWPhysState);

		if (bDebug)
			ConOutL(Location + CStrF(" [Begin] - PSIID <QT %3.3f, iEA %d>, nSIs %d", 
								m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex(), 
								m_lStateInstances.Len()));
	}
#endif

	int iStateInstance;
	for (iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		// This is not really proper behavior?
		pStateInstance->ResetPMFlags();
	}

	CWAGI_Context SplitContext;
	int iPlayingStateInstance = ResolvePlayingSIID();
	if (iPlayingStateInstance != AGI_STATEINSTANCEINDEX_NULL)
	{
		CWAGI_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
		// Advance through StateInstance queue.
		fp32 TimeFraction;
		while ((pPlayingStateInstance != NULL) && pPlayingStateInstance->ProceedQuery(_pContext, TimeFraction))
		{
			CWAGI_SIID OldPSIID = m_PlayingSIID;

			_pContext = _pContext->SplitAtFraction(TimeFraction, false, &SplitContext);
			pPlayingStateInstance->LeaveState_Initiate(_pContext);

			iPlayingStateInstance++;
			if (m_lStateInstances.ValidPos(iPlayingStateInstance))
			{
				pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
				m_PlayingSIID = CWAGI_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterAction);
#ifdef	AG_DEBUG
				if (bDebug)
					ConOutL(Location + CStrF(" [PlayingSIID] <QT %3.3f, iEA %d> -> <QT %3.3f, iEA %d> (%d -> %d)", 
											OldPSIID.GetEnqueueTime(), OldPSIID.GetEnterActionIndex(),
											m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex(),
											iPlayingStateInstance-1, iPlayingStateInstance));
#endif
				pPlayingStateInstance->EnterState_Initiate(_pContext);
			}
			else
			{
				// Token is terminated and has no more stateinstances.
				pPlayingStateInstance = NULL;
				m_PlayingSIID.Clear();
			}
		}
	}
	else
	{
		for (iPlayingStateInstance = 0; iPlayingStateInstance < m_lStateInstances.Len(); iPlayingStateInstance++)
		{
			CWAGI_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
			if (!pPlayingStateInstance->IsPMRemoved() && (pPlayingStateInstance->m_LeaveTime.AlmostEqual(AGI_UNDEFINEDTIME)))
			{
				m_PlayingSIID = CWAGI_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterAction);

				if (pPlayingStateInstance->m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME))
				{
#ifdef AG_DEBUG
					if (bDebug)
						ConOutL(Location + CStrF(" [PlayingSIID] <None> -> <QT %3.3f, iEA %d> (None -> %d)", 
						m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex(),
						iPlayingStateInstance));
#endif
					pPlayingStateInstance->EnterState_Initiate(_pContext);
				}
			}
		}
	}

	RefreshPredictionMisses(_pContext);

#ifdef	AG_DEBUG
	if (bDebug)
		ConOutL(Location + CStrF(" [End]   - PSIID <QT %3.3f, iEA %d>, nSIs %d", 
								m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex(), 
								m_lStateInstances.Len()));
#endif
}


void CWAGI_SIQ::ForceMaxStateInstances(int32 _MaxQueued)
{
	if (_MaxQueued <= 0)
		return;
	// Delete first stateinstance in queue
	int32 Len = m_lStateInstances.Len();
	if ((Len > 0) && (Len  > _MaxQueued))
	{
		m_lStateInstances.Delx(0,Len - _MaxQueued);
		//ConOut(CStrF("Removing stateinstances in queue: Removed: %d Length: %d",Len - _MaxQueued,m_lStateInstances.Len()));
	}
}
//--------------------------------------------------------------------------------

void CWAGI_SIQ::RefreshPredictionMisses(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_SIQ::RefreshPredictionMisses);

#ifdef	AG_DEBUG
	CStr Location;
	if (_pContext->m_pWPhysState->IsServer())
		Location = CStrF("(Server %X) SIQ::Refresh()", _pContext->m_pWPhysState);
	else
		Location = CStrF("(Client %X) SIQ::Refresh()", _pContext->m_pWPhysState);
#endif
	// Remove all out blended StateInstances.
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len();)
	{
		const CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);

		if (pStateInstance->IsPMRemoved())
		{
			iStateInstance++;
			continue;
		}

		bool bForceKeep = false;
		if (pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) &&
			(pStateInstance->m_BlendOutStartTime.Compare(_pContext->m_GameTime) <= 0))
		{
			bForceKeep = (m_lStateInstances.Len() > 1);
			bool bSucceedingAnimation = false;
			// Search for fully blended in succeeding animation.
			for (int jStateInstance = iStateInstance + 1; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
			/*for (int jStateInstance = 0; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
				if (jStateInstance == iStateInstance)
					continue;*/

				const CWAGI_StateInstance* pPostStateInstance = &(m_lStateInstances[jStateInstance]);
				/*if (pPostStateInstance->m_bHasAnimation || (pPostState->GetFlags(1) & AG_STATEFLAG_FORCEREMOVE))
					bSucceedingAnimation = true;*/

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) &&
					  (pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					  ((pPostStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_UNDEFINEDTIME)) || 
					   ((pPostStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					    (pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					  (pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					 ((pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_LINKEDTIME)) &&
					  (pStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					  (pStateInstance->m_BlendOutStartTime.Compare(_pContext->m_GameTime) <= 0))))
				{
					bForceKeep = false;
					break;
				}
			}
/*
			if (!bSucceedingAnimation)
				bForceKeep = false;
*/
		}

		if ((pStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) && 
			(pStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0) && 
			!bForceKeep)
		{
/*
			if (CWAGI::DebugEnabled(_pContext))
				ConOutL(Location + CStrF(" [Depricating] - (BOET %3.3f < GT %3.3f), iSI %d <QT %3.3f, iEA %d>, nSIs %d", 
				BlendOutEndTime, _pContext->m_GameTime,
				iStateInstance,
				pStateInstance->GetEnqueueTime(), pStateInstance->GetEnterActionIndex(), 
				m_lStateInstances.Len()));
*/
			m_lStateInstances.Del(iStateInstance);
		}
		else
		{
/*
			if (bForceKeep && CWAGI::DebugEnabled(_pContext))
				ConOutL(CStrF("ForceKeep State%d", pStateInstance->GetStateIndex()));
*/
			iStateInstance++;
		}
	}
}

//--------------------------------------------------------------------------------

CWAGI_StateInstance* CWAGI_SIQ::CreateAndEnqueueStateInstance(const CWAGI_Context* _pContext, CWAGI_SIID* _pSIID)
{
	MSCOPESHORT(CWAGI_SIQ::CreateAndEnqueueStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pSI = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().AlmostEqual(pSI->GetEnqueueTime())) &&
			(_pSIID->GetEnterActionIndex()== pSI->GetEnterActionIndex()) &&
			(!pSI->IsPMRemoved()))
		{
#ifdef	AG_DEBUG
			if (bDebug)
				ConOutL(CStrF("ERROR: SIQ::CreateAndEnqueue() - Inconsistent SIQ. SIID <QT %3.3f, iEA %d> exist as <QT %3.3f, iEA %d, ET %3.3f, iLA %d, LT %3.3f>", 
							_pSIID->GetEnqueueTime(), _pSIID->GetEnterActionIndex(),
							pSI->GetEnqueueTime(), pSI->GetEnterActionIndex(), pSI->GetEnterTime(), 
							pSI->GetLeaveActionIndex(), pSI->GetLeaveTime()));
#endif
			pSI->PredictionMiss_Remove(_pContext);
//			m_lStateInstances.Del(iStateInstance);
//			return pSI;
		}

	}

	// Scan for inconsistent SIs, that already exists but occurs later in time.
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAGI_StateInstance* pSI = &(m_lStateInstances[iStateInstance]);
		// New SI occurs earlier than an already existing SI, so flush the existing SI, since it has never happend.
		if (_pSIID->GetEnqueueTime().Compare(pSI->GetEnqueueTime()) < 0)
			pSI->PredictionMiss_Remove(_pContext);
	}

	int iSI = m_lStateInstances.Add(CWAGI_StateInstance(_pSIID, GetAGI(), this));
	return &(m_lStateInstances[iSI]);
}

//--------------------------------------------------------------------------------

bool CWAGI_SIQ::OnEnterState(const CWAGI_Context* _pContext, int16 _iEnterAction, fp32 _ForceOffset)
{
	MSCOPESHORT(CWAGI_SIQ::OnEnterState);

	// Has been PMRemoved, but has been readded, clear PMRemove flag.
	if ((m_Flags & SIQFLAGS_PREDMISS_REMOVE) != 0)
		m_Flags &= ~SIQFLAGS_PREDMISS_REMOVE;

	const CXRAG_Action* pAction = GetAGI()->GetAction(_iEnterAction);
	if (pAction == NULL)
		return false;

	int16 iTargetState = pAction->GetTargetStateIndex();
	bool bTerminate = (iTargetState == AG_STATEINDEX_TERMINATE);
	if (bTerminate)
		return true;

	// FIXME: Add support to flush queue from all uninitiated stateinstances that does not have a "NoFlush" flag.

	CWAGI_SIID SIID(_pContext->m_GameTime, _iEnterAction);
	CWAGI_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &SIID);

	CWAGI_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
		return false;

	if (pAddedStateInstance != pTokenStateInstance)
		ConOutL("ERROR: TokenStateInstance != AddedStateInstance");

	if (!pTokenStateInstance->EnterState_Setup(_pContext, _iEnterAction))
		return false;

/////////
	// Breakout/entry point test
	{
		fp32 Offset = 0.0f;
		if (pTokenStateInstance->m_pAGI->FindEntryPoint(_pContext, pTokenStateInstance->m_iState, m_iPreviousPlayingSequence, 
			m_PreviousPlayingSequenceTime, Offset))
		{
			pTokenStateInstance->m_Enter_AnimTimeOffset = Offset;
		}
	}

	if (_ForceOffset != 0.0f)
		pTokenStateInstance->m_Enter_AnimTimeOffset = _ForceOffset;
/////////

	// Make sure we have a playing stateinstance.
	// Cases where PSIID should be reinitialized to entered state is when no PSIID exists or PSIID match created stateinstance.
	if (!m_PlayingSIID.IsValid())
	{
		m_PlayingSIID = CWAGI_SIID(pTokenStateInstance->GetEnqueueTime(), pTokenStateInstance->GetEnterActionIndex());
		pTokenStateInstance->EnterState_Initiate(_pContext);
/*
		ConOutL(CStrF("SIQ::OnEnterState() - PSIID initialized to <QT %3.3f, iEA %d>", 
					 m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex()));
*/
	}
	else // Another case is when the PlayingSIID has no animation and a succeeding SI has, but no refresh has yet been called to make it the playing one.
	{
		CWAGI_Context SplitContext;
		int32 iPlayingStateInstance = ResolvePlayingSIID();
		if (iPlayingStateInstance != AGI_STATEINSTANCEINDEX_NULL)
		{
			CWAGI_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
			// Advance through StateInstance queue.
			fp32 TimeFraction;
			while ((pPlayingStateInstance != NULL) && pPlayingStateInstance->ProceedQuery(_pContext, TimeFraction))
			{
				CWAGI_SIID OldPSIID = m_PlayingSIID;

				_pContext = _pContext->SplitAtFraction(TimeFraction, false, &SplitContext);
				pPlayingStateInstance->LeaveState_Initiate(_pContext);

				iPlayingStateInstance++;
				if (m_lStateInstances.ValidPos(iPlayingStateInstance))
				{
					pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
					m_PlayingSIID = CWAGI_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterAction);
					pPlayingStateInstance->EnterState_Initiate(_pContext);
				}
				else
				{
					// Token is terminated and has no more stateinstances.
					pPlayingStateInstance = NULL;
					m_PlayingSIID.Clear();
				}
			}
		}
	}

/*
	if ((m_PlayingSIID.GetEnqueueTime() == pTokenStateInstance->GetEnqueueTime()) && 
		(m_PlayingSIID.GetEnterActionIndex() == pTokenStateInstance->GetEnterActionIndex()))
	{
		pTokenStateInstance->EnterState_Initiate(_pContext);
	}
*/
	return true;
}

//--------------------------------------------------------------------------------

bool CWAGI_SIQ::OnLeaveState(const CWAGI_Context* _pContext, int16 _iLeaveAction)
{
	MSCOPESHORT(CWAGI_SIQ::OnLeaveState);

	// Has been PMRemoved, so don't destroy PM fadeouts, act as if not existant.
	if ((m_Flags & SIQFLAGS_PREDMISS_REMOVE) != 0)
		return true;

	CWAGI_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
		return false;

/////// Breakout/entry point test
	if (pTokenStateInstance->HasAnimation())
	{
		pTokenStateInstance->m_pAGI->FindBreakoutSequence(_pContext, pTokenStateInstance->m_iState, 
			pTokenStateInstance->GetEnterAnimTimeOffset(), m_iPreviousPlayingSequence, 
			m_PreviousPlayingSequenceTime);
	}
///////

	if (!pTokenStateInstance->LeaveState_Setup(_pContext, _iLeaveAction))
		return false;

	return true;
}

//--------------------------------------------------------------------------------

bool CWAGI_SIQ::DisableStateInstanceAnims(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance, int _iDisableSIAnimsCallbackMsg) const
{
	MSCOPESHORT(CWAGI_SIQ::DisableStateInstanceAnims);

	CWObject_Message Msg(_iDisableSIAnimsCallbackMsg, (aint)_pStateInstance);
	return (_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject) != 0);
}

//--------------------------------------------------------------------------------

void CWAGI_SIQ::GetAnimLayers(const CWAGI_Context* _pContext, CXR_AnimLayer* _pLayers, 
							  int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg,
							  bool _bAllowBlend)
{
	MSCOPESHORT(CWAGI_SIQ::GetAnimLayers);

	if (_nLayers < 1)
		return;

	int nTotalLayers = 0;
	bool bSkipForceKeep = false;
	
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		if ((_iDisableStateInstanceAnimsCallbackMsg != 0) &&
			DisableStateInstanceAnims(_pContext, pStateInstance, _iDisableStateInstanceAnimsCallbackMsg))
			continue;

		int nStateInstanceLayers = _nLayers - nTotalLayers;
		CXR_AnimLayer* pStateInstanceLayers = &(_pLayers[nTotalLayers]);

		bool bForceKeep = false;
		if (!nTotalLayers && pStateInstance->m_bSkipForceKeep)
			bSkipForceKeep = true;

		if (!pStateInstance->m_bSkipForceKeep && pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) &&
			(pStateInstance->m_BlendOutStartTime.Compare(_pContext->m_GameTime) <= 0) &&
			(!_bAllowBlend))
		{
			bForceKeep = (m_lStateInstances.Len() > 1);
			bool bSucceedingAnimation = false;
			// Search for fully blended in succeeding animation.
			for (int jStateInstance = iStateInstance + 1; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
			/*for (int jStateInstance = 0; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
				if (jStateInstance == iStateInstance)
					continue;*/

				const CWAGI_StateInstance* pPostStateInstance = &(m_lStateInstances[jStateInstance]);
				if (pPostStateInstance->m_bHasAnimation)
					bSucceedingAnimation = true;

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_UNDEFINEDTIME) == false) &&
					  (pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					  ((pPostStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_UNDEFINEDTIME)) || 
					   ((pPostStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					    (pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					  (pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					 ((pPostStateInstance->m_BlendInEndTime.AlmostEqual(AGI_LINKEDTIME)) &&
					  (pStateInstance->m_BlendOutEndTime.AlmostEqual(AGI_LINKEDTIME) == false) &&
					  (pStateInstance->m_BlendOutStartTime.Compare(_pContext->m_GameTime) <= 0))))
				{
					bForceKeep = false;
					break;
				}
			}
/*
			if (!bSucceedingAnimation)
				bForceKeep = false;
*/
		}

		/*if (pStateInstance->m_iState == 1726)
			ConOutL(CStrF("%s ForceKeep: %d SkipForceKeep: %d",_pContext->m_pWPhysState->IsServer() ? "S:":"C:",bForceKeep,pStateInstance->m_bSkipForceKeep));*/
		pStateInstance->GetAnimLayers(_pContext, !bForceKeep, pStateInstanceLayers, nStateInstanceLayers, iStateInstance);
		nTotalLayers += nStateInstanceLayers;
	}

	_nLayers = nTotalLayers;

	for (int iLayer = 0; iLayer < _nLayers; iLayer++)
	{
		if (_pLayers[iLayer].m_Blend == -1.0f)
		{
			if (iLayer > 0)
				_pLayers[iLayer].m_Blend = (1.0f - _pLayers[iLayer - 1].m_BlendOut);
			else
				_pLayers[iLayer].m_Blend = 1.0f;
		}
		else if (_pLayers[iLayer].m_Blend == -2.0f)
		{
			if ((iLayer + 1) < _nLayers)
				_pLayers[iLayer].m_Blend = (1.0f - _pLayers[iLayer + 1].m_BlendIn);
			else
				_pLayers[iLayer].m_Blend = 0.0f;
	}
		else if (_pLayers[iLayer].m_Blend == -3.0f)
	{
			if (iLayer > 0)
				_pLayers[iLayer].m_Blend = (1.0f - _pLayers[iLayer - 1].m_BlendOut);
			else
				_pLayers[iLayer].m_Blend = 1.0f;

			if ((iLayer + 1) < _nLayers)
				_pLayers[iLayer].m_Blend *= (1.0f - _pLayers[iLayer + 1].m_BlendIn);
		}
		else if (_pLayers[iLayer].m_Blend < 0.0f)
			_pLayers[iLayer].m_Blend = 0.0f;
	}

	if ((_nLayers > 1) && !_bAllowBlend && !bSkipForceKeep)
	{
		_pLayers[0].m_Blend = 1.0f;
	}
}

bool CWAGI_SIQ::GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iAnim, int32 _StartTick) const
{
	MSCOPESHORT(CWAGI_SIQ::GetSpecificAnimLayer);

	int32 Len = m_lStateInstances.Len();
	for (int iStateInstance = Len - 1; iStateInstance >= 0 ; iStateInstance--)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		if (pStateInstance->GetSpecificAnimLayer(_pContext, _Layer, iStateInstance, _iAnim,_StartTick))
			return true;
	}

	return false;
}

CAGStateIndex CWAGI_SIQ::GetSpecificState(const CWAGI_Context* _pContext, int32 _iAnim) const
{
	MSCOPESHORT(CWAGI_SIQ::GetSpecificState);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved() || !pStateInstance->HasSpecificAnimation(_iAnim))
			continue;

		return pStateInstance->GetStateIndex();
	}

	return -1;
}

//--------------------------------------------------------------------------------

CWAGI_StateInstance* CWAGI_SIQ::GetMatchingStateInstance(const CWAGI_SIID* _pSIID, bool _bCreateNonExistent)
{
	MSCOPESHORT(CWAGI_SIQ::GetMatchingStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		// FIXME: Should this ignore PMRemoved SIs?! (then this function could be used by CreateAndEnqueue...
		CWAGI_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().AlmostEqual(pStateInstance->m_EnqueueTime)) &&
			(_pSIID->GetEnterActionIndex() == pStateInstance->m_iEnterAction))
			return pStateInstance;
	}

	if (_bCreateNonExistent)
	{
		int iStateInstance = m_lStateInstances.Add(CWAGI_StateInstance(_pSIID, GetAGI(), this));
		return &(m_lStateInstances[iStateInstance]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------

#if	defined(M_RTM) && defined( AG_DEBUG )
#warning "M_RTM and AG_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
