#include "PCH.h"

//--------------------------------------------------------------------------------

#ifdef	AG2_DEBUG
static bool bDebug = false;
#endif
//--------------------------------------------------------------------------------

#include "WAG2I.h"
#include "WAG2I_Context.h"
#include "WAG2I_StateInst.h"
#include "WAG2I_StateInstPacked.h"
#include "WAG2_ClientData.h"
#include "../server/wserver.h"

//--------------------------------------------------------------------------------

MRTC_IMPLEMENT(CWAG2I_Token, CObj);

int16 CWAG2I_Token::GetStateIndex() const
{
	//MSCOPESHORT(CWAG2I_Token::GetStateIndex);

	const CWAG2I_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return AG2_STATEINDEX_NULL;

	return pStateInstance->GetStateIndex();
}

int8 CWAG2I_Token::GetAnimGraphIndex() const
{
	//MSCOPESHORT(CWAG2I_Token::GetStateIndex);

	const CWAG2I_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return -1;

	return pStateInstance->GetAnimGraphIndex();
}

void CWAG2I_Token::Clear()
{
	MSCOPESHORT(CWAG2I_Token::Clear);
	m_pAG2I = NULL;
	m_ID = AG2_TOKENID_NULL;
	m_RefreshGameTime.Reset();
	m_Flags = 0;
	m_iPreviousPlayingSequence = -1;
	m_iGraphBlock = AG2_GRAPHBLOCKINDEX_NULL;
	m_PreviousPlayingSequenceTime.Reset();
	m_PlayingSIID.Clear();
	m_lStateInstances.Clear();
	m_DirtyFlag = 0;
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::CopyFrom(const CWAG2I_Token& _Token, CWAG2I* _pOurAG2I)
{
	MSCOPESHORT(CWAG2I_Token::CopyFrom);
	m_pAG2I = _pOurAG2I;
	m_ID = _Token.m_ID;
	m_RefreshGameTime = _Token.m_RefreshGameTime;
	m_Flags = _Token.m_Flags;
	m_iPreviousPlayingSequence = _Token.m_iPreviousPlayingSequence;
	m_iGraphBlock = _Token.m_iGraphBlock;
	m_PreviousPlayingSequenceTime = _Token.m_PreviousPlayingSequenceTime;
	m_PlayingSIID = _Token.m_PlayingSIID;
	TAP_RCD<const CWAG2I_StateInstance> lOtherStateInstance = _Token.m_lStateInstances;
	m_lStateInstances.SetLen(lOtherStateInstance.Len());
	TAP_RCD<CWAG2I_StateInstance> lStateInstance = m_lStateInstances;
	for (int32 i = 0; i < lOtherStateInstance.Len(); i++)
	{
		lStateInstance[i] = lOtherStateInstance[i];
		lStateInstance[i].m_pAG2I = _pOurAG2I;
	}
}

//--------------------------------------------------------------------------------

uint32 CWAG2I_Token::Refresh(CWAG2I_Context* _pContext)
{
	MSCOPESHORT( CWAG2I_Token::Refresh );

	CWAG2I_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance != NULL)
	{
		if (pStateInstance->m_bShouldDisableRefresh)
		{
			m_Flags |= TOKENFLAGS_NOREFRESH;
		}
		else
		{
			m_Flags &= ~TOKENFLAGS_NOREFRESH;
		}

		if (m_RefreshGameTime.IsReset())
			m_RefreshGameTime = GetEnterStateTime();

		CMTime Diff = m_RefreshGameTime - _pContext->m_GameTime;
		if (Diff.GetTime() < -0.001f)
		{
			/*
			if ((_pContext->m_GameTime - m_RefreshGameTime) > 0.05f)
			ConOutL(CStrF("CWAG2I_Token::Refresh() - Token %d, Forcing Context GameTime from %3.3f to %3.3f", m_ID, _pContext->m_GameTime, m_RefreshGameTime));
			*/
			// If the AnimGraph2 has been disabled for a while, don't worry about last refresh
			if (((_pContext->m_GameTime - m_RefreshGameTime).GetTime() <= 0.15f))
				_pContext->StartAtTime(m_RefreshGameTime, _pContext);
		}
	}

	// FIXME: Should this be called here or in the end of this function?!
	// It might be good to outdate old stateinstances before GetTokenStateInstance is called.
	RefreshQueue(_pContext);

	pStateInstance = GetTokenStateInstance();
	// Make sure we don't refresh terminated states
	if (pStateInstance == NULL || pStateInstance->m_bGotTerminate)
		return AG2I_TOKENREFRESHFLAGS_FAILED;

	// Refresh sync anim (if any)
	if (pStateInstance->m_bHasSyncAnim)
		pStateInstance->UpdateSyncAnimScale(_pContext,m_pAG2I->GetEvaluator()->GetSyncAnimTimeScale());
	if (pStateInstance->m_bHasAdaptiveTimeScale)
		pStateInstance->UpdateAdaptiveTimeScale(_pContext,m_pAG2I->GetEvaluator()->GetAdaptiveTimeScale());

	int16 iState = pStateInstance->GetStateIndex();
	if (iState == AG2_STATEINDEX_NULL)
		return AG2I_TOKENREFRESHFLAGS_FAILED;

	CAG2AnimGraphID iAnimGraph = pStateInstance->GetAnimGraphIndex();

	const CXRAG2_State* pState = m_pAG2I->GetState(iState,pStateInstance->GetAnimGraphIndex());
	if (pState == NULL)
		return AG2I_TOKENREFRESHFLAGS_FAILED;

	const CXRAG2_ConditionNodeV2* pNode = m_pAG2I->GetNode(pState->GetBaseNodeIndex(),iAnimGraph);
	if (pNode == NULL)
		return AG2I_TOKENREFRESHFLAGS_FAILED;

	M_ASSERT(m_pAG2I->m_pEvaluator, "!");
	m_pAG2I->m_pEvaluator->AG2_RefreshStateInstanceProperties(_pContext, pStateInstance);

	//	int16 iStatePropertyParamBase = pState->GetBasePropertyParamIndex();

	CMTime StartGameTime = _pContext->m_GameTime;
	CMTime EndGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);
	m_RefreshGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);

	fp32 TimeFraction = 0;
	uint16 NodeAction;
	{
		MSCOPESHORT( CWAG2I_Token::EvalNode );
		EvalNodeInit();
		NodeAction = EvalNode(_pContext, pNode, TimeFraction);
	}

	// FIXME CHANGE THIS!!!!
	if (NodeAction != AG2_NODEACTION_FAILPARSE)
	{
		int iAction = pState->GetBaseActionIndex() + (NodeAction & AG2_NODEACTION_ACTIONMASK);
		const CXRAG2_Action* pAction = m_pAG2I->GetAction(iAction,iAnimGraph);
		if (pAction == NULL)
			return AG2I_TOKENREFRESHFLAGS_FAILED;

		CWAG2I* pAG2I = m_pAG2I;
		pAG2I->InvokeEffects(_pContext,iAnimGraph, pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances());
		CWAG2I_StateInstance* pCurrentStateInstance = GetTokenStateInstance();
		// If stateinstance queue is changed when effects have been invoked, abort current
		// evaluation
		if (pCurrentStateInstance != pStateInstance || iState != pStateInstance->m_iState)
			return AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION;

		// Hmm, ok so the token might have been moved in this
		int NumMoveTokens = pAction->GetNumMoveTokens();

		bool NumMovesOnThisToken = 0;
		int32 iTokenID = GetID();
		for (int32 i = 0; i < NumMoveTokens; i++)
		{
			int iMoveToken = pAction->GetBaseMoveTokenIndex() + i;
			const CXRAG2_MoveToken* pMoveToken = pAG2I->GetMoveToken(iMoveToken,iAnimGraph);
			M_ASSERT(pMoveToken,"CWAG2I_Token::Refresh Invalide movetoken");
			int8 ActionTokenID = pMoveToken->GetTokenID();
			if (ActionTokenID == AG2_TOKENID_NULL)
				ActionTokenID = iTokenID;

			//			int16 iTargetState = pMoveToken->GetTargetStateIndex();
			int16 iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();
			bool bOtherToken;
			if (ActionTokenID != iTokenID)
			{
				bOtherToken = true;
			}
			else
			{
				NumMovesOnThisToken++;
				bOtherToken = false;
			}

			_pContext->SplitAtFraction(TimeFraction, !bOtherToken, _pContext);
			//m_RefreshGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);

			// If the token moved is earlier in the queue, force refresh
			if (ActionTokenID < iTokenID)
				pAG2I->ForceRefresh();

			if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
				pAG2I->MoveGraphBlock(_pContext, iAnimGraph,ActionTokenID, iMoveToken);
			else
				pAG2I->MoveAction(_pContext, iAnimGraph,ActionTokenID, iMoveToken);
		}

		// WARNING: Don't make ANY references to this token anymore in this function, since it might have been reallocated.
		if (!NumMoveTokens)
			_pContext->SplitAtFraction(TimeFraction, true, _pContext);

		bool bRefresh = ((NumMovesOnThisToken || (TimeFraction > 0.01f)) && (_pContext->m_GameTime.Compare((EndGameTime - CMTime::CreateFromSeconds(0.01f))) < 0));
		if (bRefresh)
			return (AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION | AG2I_TOKENREFRESHFLAGS_REFRESH);
		else
			return AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION;
	}

	return AG2I_TOKENREFRESHFLAGS_FINISHED;
}

void CWAG2I_Token::EndTimeLeap(CWAG2I_Context* _pContext)
{
	// Remove all stateinstances but the last one and modify entertime of the last one
	// inject the starttimeleap layers if any with modified times
	if (m_lStateInstances.Len() > 1)
		m_lStateInstances.Delx(0,m_lStateInstances.Len()-1);
	if (m_lStateInstances.Len())
	{
		// Modify entertime
		CMTime GameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_pWPhysState->GetGameTickTime());
		CMTime BlendDiff = m_lStateInstances[0].m_BlendInStartTime - m_lStateInstances[0].m_EnterTime;
		m_lStateInstances[0].m_EnterTime = GameTime;
		m_lStateInstances[0].m_BlendInStartTime = GameTime + BlendDiff;
	}
}
//--------------------------------------------------------------------------------

CMTime CWAG2I_Token::GetEnterStateTime() const
{
	MSCOPESHORT(CWAG2I_Token::GetEnterStateTime);

	const CWAG2I_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return AG2I_UNDEFINEDTIME;

	return pStateInstance->GetEnqueueTime();
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::PredictionMiss_AddStateInstance(CWAG2I_Context* _pContext, const CWAG2I_SIP* _pSIP)
{
	MSCOPESHORT(CWAG2I_Token::PredictionMiss_AddStateInstance);

	int iSI;
	CWAG2I_SIID AddSIID(_pSIP->GetEnqueueTime(), _pSIP->GetEnterMoveTokenIndex(),_pSIP->GetAnimGraphIndex());
	for (iSI = 0; iSI < m_lStateInstances.Len(); iSI++)
	{
		CWAG2I_StateInstance* pSI = &(m_lStateInstances[iSI]);
		CWAG2I_SIID SIID(pSI->GetEnqueueTime(), pSI->GetEnterMoveTokenIndex(),pSI->GetAnimGraphIndex());
		if (AddSIID.GetEnqueueTime().Compare(SIID.GetEnqueueTime()) < 0)
		{
			m_lStateInstances.SetLen(iSI); // Set length to one less than before, i.e. deleting this and all later entries.
			iSI = m_lStateInstances.Add(CWAG2I_StateInstance(&AddSIID, GetAG2I(), this));
			CWAG2I_StateInstance* pSI = &(m_lStateInstances[iSI]);
			pSI->PredictionMiss_Add(_pContext, _pSIP);
			ResolvePlayingSIID(); // iPlayingStateInstance may be after the inserted one.
			return;
		}
	}

	iSI = m_lStateInstances.Add(CWAG2I_StateInstance(&AddSIID, GetAG2I(), this));
	CWAG2I_StateInstance* pSI = &(m_lStateInstances[iSI]);
	pSI->PredictionMiss_Add(_pContext, _pSIP);
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::PredictionMiss_RemoveStateInstance(CWAG2I_Context* _pContext, const CWAG2I_SIID* _pSIID)
{
	MSCOPESHORT(CWAG2I_Token::PredictionMiss_RemoveStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().Compare(pStateInstance->m_EnqueueTime) == 0) &&
			(_pSIID->GetEnterMoveTokenIndex() == pStateInstance->m_iEnterMoveToken))
		{
			pStateInstance->PredictionMiss_Remove(_pContext);
		}
	}
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::PredictionMiss_Remove(CWAG2I_Context* _pContext)
{
	MSCOPESHORT(CWAG2I_Token::PredictionMiss_Remove);

	m_Flags |= TOKENFLAGS_PREDMISS_REMOVE;

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		pStateInstance->PredictionMiss_Remove(_pContext);
	}
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::ResetPMFlags()
{
	MSCOPESHORT(CWAG2I_Token::ResetPMFlags);

	m_Flags = 0;
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		pStateInstance->ResetPMFlags();
	}
}

//--------------------------------------------------------------------------------

CWAG2I_StateInstance* CWAG2I_Token::GetTokenStateInstance()
{
	MSCOPESHORT(CWAG2I_Token::GetTokenStateInstance_1);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	int32 iTokenStateInstance = (m_lStateInstances.Len() - 1);
	CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iTokenStateInstance]);

	// Make sure last stateinstance hasn't left it's state. In that case, the token is terminated.
	if (!pStateInstance->GetLeaveTime().AlmostEqual(AG2I_UNDEFINEDTIME))
		return NULL;

	return pStateInstance;
}

const CWAG2I_StateInstance* CWAG2I_Token::GetTokenStateInstance() const
{
	//MSCOPESHORT(CWAG2I_Token::GetTokenStateInstance_2);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	int32 iTokenStateInstance = (m_lStateInstances.Len() - 1);
	const CWAG2I_StateInstance* pStateInstance = (&(m_lStateInstances[iTokenStateInstance]));

	// Make sure last stateinstance hasn't left it's state. In that case, the token is terminated.
	if (!pStateInstance->GetLeaveTime().AlmostEqual(AG2I_UNDEFINEDTIME))
		return NULL;

	return pStateInstance;
}

CWAG2I_StateInstance* CWAG2I_Token::GetTokenStateInstanceUpdate()
{
	MSCOPESHORT(CWAG2I_Token::GetTokenStateInstanceUpdate_1);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	return &(m_lStateInstances[m_lStateInstances.Len() - 1]);
}

const CWAG2I_StateInstance* CWAG2I_Token::GetTokenStateInstanceUpdate() const
{
	MSCOPESHORT(CWAG2I_Token::GetTokenStateInstanceUpdate_2);

	if (m_lStateInstances.Len() == 0)
		return NULL;

	return &(m_lStateInstances[m_lStateInstances.Len() - 1]);
}


//--------------------------------------------------------------------------------

uint8 CWAG2I_Token::ResolvePlayingSIID()
{
	MSCOPESHORT(CWAG2I_Token::ResolvePlayingSIID);

	if ((m_PlayingSIID.GetEnqueueTime().Compare(AG2I_UNDEFINEDTIME) == 0) && 
		(m_PlayingSIID.GetEnterMoveTokenIndex() == AG2_ACTIONINDEX_NULL))
	{
		return AG2I_STATEINSTANCEINDEX_NULL;
	}

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		const CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((m_PlayingSIID.GetEnqueueTime().Compare(pStateInstance->GetEnqueueTime()) == 0) &&
			(m_PlayingSIID.GetEnterMoveTokenIndex() == pStateInstance->GetEnterMoveTokenIndex()) &&
			(!pStateInstance->IsPMRemoved()))
		{
			for (int jStateInstance = iStateInstance+1; jStateInstance < m_lStateInstances.Len(); jStateInstance++)
			{
				if ((m_PlayingSIID.GetEnqueueTime().Compare(m_lStateInstances[jStateInstance].GetEnqueueTime()) == 0) &&
					(m_PlayingSIID.GetEnterMoveTokenIndex() == m_lStateInstances[jStateInstance].GetEnterMoveTokenIndex()))
				{
#ifdef	AG2_DEBUG
					if (bDebug)
						ConOutL(CStrF("ERROR: Token::ResolvePlayingSIID() - Inconsistent Token. Non-unique PSIID. <QT %3.3f, iEA %d>", m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex()));
#endif
				}
			}

			return iStateInstance;
		}
	}

	return AG2I_STATEINSTANCEINDEX_NULL;
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::RefreshQueue(const CWAG2I_Context* _pContext)
{
	MSCOPESHORT( CWAG2I_Token::RefreshQueue );
#ifdef	AG2_DEBUG
	CStr Location;
	if( bDebug )
	{
		if (_pContext->m_pWPhysState->IsServer())
			Location = CStrF("(Server %X) Token::Refresh()", _pContext->m_pWPhysState);
		else
			Location = CStrF("(Client %X) Token::Refresh()", _pContext->m_pWPhysState);

		if (bDebug)
			ConOutL(Location + CStrF(" [Begin] - PSIID <QT %3.3f, iEMT %d>, nSIs %d", 
			m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterMoveTokenIndex(), 
			m_lStateInstances.Len()));
	}
#endif

	int iStateInstance;
	for (iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		// This is not really proper behavior?
		pStateInstance->ResetPMFlags();
	}

	CWAG2I_Context SplitContext;
	int iPlayingStateInstance = ResolvePlayingSIID();
	if (iPlayingStateInstance != AG2I_STATEINSTANCEINDEX_NULL)
	{
		CWAG2I_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
		// Advance through StateInstance queue.
		fp32 TimeFraction;
		while ((pPlayingStateInstance != NULL) && pPlayingStateInstance->ProceedQuery(_pContext, TimeFraction))
		{
			CWAG2I_SIID OldPSIID = m_PlayingSIID;

			_pContext = _pContext->SplitAtFraction(TimeFraction, false, &SplitContext);
			pPlayingStateInstance->LeaveState_Initiate(_pContext);

			iPlayingStateInstance++;
			if (m_lStateInstances.ValidPos(iPlayingStateInstance))
			{
				pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
				m_PlayingSIID = CWAG2I_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterMoveToken,pPlayingStateInstance->m_iAnimGraph);
#ifdef	AG2_DEBUG
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
			CWAG2I_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
			if (!pPlayingStateInstance->IsPMRemoved() && (pPlayingStateInstance->m_LeaveTime.Compare(AG2I_UNDEFINEDTIME) == 0))
			{
				m_PlayingSIID = CWAG2I_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterMoveToken,pPlayingStateInstance->m_iAnimGraph);

				if (pPlayingStateInstance->m_EnterTime.Compare(AG2I_UNDEFINEDTIME) == 0)
				{
#ifdef AG2_DEBUG
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

#ifdef	AG2_DEBUG
	if (bDebug)
		ConOutL(Location + CStrF(" [End]   - PSIID <QT %3.3f, iEA %d>, nSIs %d", 
		m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex(), 
		m_lStateInstances.Len()));
#endif
}


void CWAG2I_Token::ForceMaxStateInstances(int32 _MaxQueued)
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

void CWAG2I_Token::RefreshPredictionMisses(const CWAG2I_Context* _pContext)
{
	MSCOPESHORT(CWAG2I_Token::RefreshPredictionMisses);

#ifdef	AG2_DEBUG
	CStr Location;
	if (_pContext->m_pWPhysState->IsServer())
		Location = CStrF("(Server %X) Token::Refresh()", _pContext->m_pWPhysState);
	else
		Location = CStrF("(Client %X) Token::Refresh()", _pContext->m_pWPhysState);
#endif

	// Emergency delete
#ifndef M_RTM
	// Just delete everything below 100
	if (m_lStateInstances.Len() > 105)
	{
		CStr Msg("<");
		for (int32 i = 0; i < 5; i++)
		{
			int32 iRandom = (int32)(Random * 100.0f);
			const CWAG2I_StateInstance& StateInstance = m_lStateInstances[iRandom];
			const CXRAG2_MoveToken* pMoveToken = GetAG2I()->GetMoveToken(StateInstance.m_iEnterMoveToken,StateInstance.m_iAnimGraph);
			if (pMoveToken)
			{
				Msg += CStrF("(iState: %d, %s",StateInstance.m_iState,(pMoveToken->m_TargetStateType ? GetAG2I()->GetSwitchStateName(StateInstance.m_iState,StateInstance.m_iAnimGraph) : GetAG2I()->GetStateName(StateInstance.m_iState,StateInstance.m_iAnimGraph)).Str());
			}
			if (i < 4)
				Msg += ", ";
		}
		Msg += ">";
		m_lStateInstances.Delx(0,100);
		// Go through some states and get their names
		ConOutL(CStrF("CWAG2I_Token::RefreshPredictionMisses: UBERMANY STATE INSTANCES, SAMPLE: %s",Msg.Str()));
	}
#endif

	int32 Len = m_lStateInstances.Len();
	int32 FullBlendStateIndex = 0;
	bool bHasAnimation = false;
	CWAG2I_StateInstance* pInstances = m_lStateInstances.GetBasePtr();
	// Search backwards and find the first fully blended in state instance
	for (int iStateInstance = Len-1; iStateInstance >= 0; iStateInstance--)
	{
		const CWAG2I_StateInstance& PostStateInstance = pInstances[iStateInstance];
		bHasAnimation |= PostStateInstance.m_bHasAnimation;
		if ((PostStateInstance.m_bHasAnimation || PostStateInstance.m_bGotTerminate) &&
			(((PostStateInstance.m_BlendInEndTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
			(PostStateInstance.m_BlendInEndTime.Compare(AG2I_LINKEDTIME) != 0) &&

			((PostStateInstance.m_BlendOutEndTime.Compare(AG2I_UNDEFINEDTIME) == 0) || 
			((PostStateInstance.m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
			(PostStateInstance.m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&

			(PostStateInstance.m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
			((PostStateInstance.m_BlendInEndTime.Compare(AG2I_LINKEDTIME) == 0))))
		{
			// Earliest fully blended state, assume everthing below it is ok to remove below
			FullBlendStateIndex = iStateInstance;
			// Break on earliest state we have a fully blended anim
			break;
		}
	}

	CMTime ClientOffsetTerm = (_pContext->m_pWPhysState && _pContext->m_pWPhysState->IsClient()) ? CMTime::CreateFromTicks(1,_pContext->m_pWPhysState->GetGameTickTime()) : CMTime::CreateFromSeconds(0.0f);
	CMTime ClientOffsetZero;
	ClientOffsetZero.Reset();

	// Remove all out blended StateInstances.
	for (int iStateInstance = 0; iStateInstance < Len;)
	{
		CWAG2I_StateInstance& StateInstance = pInstances[iStateInstance];

		if (StateInstance.IsPMRemoved())
		{
			iStateInstance++;
			continue;
		}

		// If we got a state after us and that has entered state, leave this state
		if (iStateInstance < (Len-1) && StateInstance.m_bGotLeaveState &&
			!StateInstance.m_bGotLeaveStateInit)
			StateInstance.LeaveState_Initiate(_pContext);

		// Force keep everything above last blended state
		bool bForceKeep = Len > 1 ? bHasAnimation && (iStateInstance >= FullBlendStateIndex) : !StateInstance.m_bGotTerminate;

		// Only force keep the last fully blended state
		if (!StateInstance.m_BlendOutEndTime.AlmostEqual(AG2I_UNDEFINEDTIME) && 
			(StateInstance.m_BlendOutEndTime.Compare(_pContext->m_GameTime - (StateInstance.m_bGotTerminate ? ClientOffsetTerm : ClientOffsetZero)) <= 0) && 
			!bForceKeep)
		{
			// Change the full blended state
			if (iStateInstance < FullBlendStateIndex)
				FullBlendStateIndex--;

			m_lStateInstances.Del(iStateInstance);
			// Get new len and baseptr
			Len = m_lStateInstances.Len();
			pInstances = m_lStateInstances.GetBasePtr();
		}
		else
		{
			iStateInstance++;
		}
	}
}

//--------------------------------------------------------------------------------

CWAG2I_StateInstance* CWAG2I_Token::CreateAndEnqueueStateInstance(const CWAG2I_Context* _pContext, CWAG2I_SIID* _pSIID)
{
	MSCOPESHORT(CWAG2I_Token::CreateAndEnqueueStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pSI = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().Compare(pSI->GetEnqueueTime()) == 0) &&
			(_pSIID->GetEnterMoveTokenIndex()== pSI->GetEnterMoveTokenIndex()) &&
			(!pSI->IsPMRemoved()))
		{
#ifdef	AG2_DEBUG
			if (bDebug)
				ConOutL(CStrF("ERROR: Token::CreateAndEnqueue() - Inconsistent Token. SIID <QT %3.3f, iEMT %d> exist as <QT %3.3f, iEMT %d, ET %3.3f, iLMT %d, LT %3.3f>", 
				_pSIID->GetEnqueueTime(), _pSIID->GetEnterMoveTokenIndex(),
				pSI->GetEnqueueTime(), pSI->GetEnterMoveTokenIndex(), pSI->GetEnterTime(), 
				pSI->GetLeaveMoveTokenIndex(), pSI->GetLeaveTime()));
#endif
			pSI->PredictionMiss_Remove(_pContext);
			//			m_lStateInstances.Del(iStateInstance);
			//			return pSI;
		}

	}

	// Scan for inconsistent SIs, that already exists but occurs later in time.
	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		CWAG2I_StateInstance* pSI = &(m_lStateInstances[iStateInstance]);
		// New SI occurs earlier than an already existing SI, so flush the existing SI, since it has never happend.
		if (_pSIID->GetEnqueueTime().Compare(pSI->GetEnqueueTime()) < 0)
			pSI->PredictionMiss_Remove(_pContext);
	}

	int iSI = m_lStateInstances.Add(CWAG2I_StateInstance(_pSIID, GetAG2I(), this));
	return &(m_lStateInstances[iSI]);
}

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
#ifndef M_RTM
static bool s_bDebugServer = false;
static bool s_bDebugClient = false;
static bool s_bDebugNonPlayer = true;
static bool s_bDebugPlayer = true;

void CWAG2I_Token::SetDebugOut(bool _bVal)
{
	s_bDebugServer = _bVal;
}
#endif

void CWAG2I_Token::EnterState(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset)
{
	MSCOPESHORT(CWAG2I_Token::EnterState);

#ifndef M_RTM
	bool bIsServer = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsServer() : 0;
	bool bIsClient = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsClient() : 0;
	int ObjectFlags = _pContext->m_pObj ? _pContext->m_pObj->GetPhysState().m_ObjectFlags : 0;
	int iObject = _pContext->m_pObj ? _pContext->m_pObj->m_iObject : 0;

	if (!m_pAG2I->m_bDisableDebug)
	{
		uint32 AG2IDebugFlags = 0;
		int32 iAGDbgObj = 0;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
		if (pReg != NULL)
			AG2IDebugFlags = pReg->GetValuei("AG2I_DEBUG_FLAGS");

		CWorld_Server* pWorld = TDynamicCast<CWorld_Server>(_pContext->m_pWPhysState);
		if (pWorld)
		{
			AG2IDebugFlags = pWorld->Registry_GetServer()->GetValuei("AG2I_DEBUG_FLAGS");
			iAGDbgObj = pWorld->Registry_GetServer()->GetValuei("agdbgobj", 0);
		}
		if (!s_bDebugServer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0)
			s_bDebugServer = true;
		if (!s_bDebugClient && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0)
			s_bDebugClient = true;
		if (!s_bDebugPlayer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0)
			s_bDebugPlayer = true;
		if (!s_bDebugNonPlayer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0)
			s_bDebugNonPlayer = true;

		bool bDebugServer = s_bDebugServer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
		bool bDebugClient = s_bDebugClient || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
		bool bDebugPlayer = s_bDebugPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
		bool bDebugNonPlayer = s_bDebugNonPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0; 

		if (iAGDbgObj && (iAGDbgObj != _pContext->m_pObj->m_iObject))
		{
			// Only output for specific ag object
			bDebugServer = false;
			bDebugClient = false;
		}

		bool bIsPlayer = ((ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
		if (((bIsServer && bDebugServer) ||
			(bIsClient && bDebugClient)) &&
			((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
		{
			CFStr Msg;

			if (bIsServer)
			{
				CWObject* pObj = (CWObject*)_pContext->m_pObj;
				CFStr Name = pObj->GetName();
				if (Name.Len())
					Name = "(" + Name + ")";
				Msg += CFStrF("S iO:%d%s ", iObject,Name.Str());
			}
			else
				Msg += CFStrF("C iO:%d ", iObject);

			//			CStr ExportedActionName = (_iAction != -1) ? (m_pAG2I->GetExportedActionName(iExportedActionName)) : (CStr("ACTION == -1"));
			Msg += CFStrF("T:%d GT:%3.4f iMT:%d", m_ID, _pContext->m_GameTime.GetTime(), _iMoveToken);
			const CXRAG2_MoveToken* pMoveToken = m_pAG2I->GetMoveToken(_iMoveToken,_iAnimGraph);
			if (pMoveToken)
			{
				int16 iState = pMoveToken->GetTargetStateIndex();
			
				if (iState != AG2_STATEINDEX_NULL)
				{
					if (iState != AG2_STATEINDEX_TERMINATE)
					{
						iState &= AG2_TARGETSTATE_INDEXMASK;
						Msg += CStrF(" -> State%d", iState);
						CFStr StateName = pMoveToken->m_TargetStateType ? m_pAG2I->GetSwitchStateName(iState,_iAnimGraph) : m_pAG2I->GetStateName(iState,_iAnimGraph);
						if (StateName != "")
							Msg += CStrF(" '%s'", StateName.Str());
					}
					else
					{
						Msg += " -> TERMINATE";
					}
				}

				//Msg += CStrF(", ABlendD %3.3f", pAction->GetAnimBlendDuration());

				const CXRAG2_State* pState = m_pAG2I->GetState(iState, _iAnimGraph);
				if (pState != NULL && pState->GetNumAnimLayers())
				{
					int32 Numlayers = pState->GetNumAnimLayers();
					for (int32 i = 0; i < Numlayers; i++)
					{
						const CXRAG2_AnimLayer* pAnimLayer = m_pAG2I->GetAnimLayer(pState->GetBaseAnimLayerIndex()+i, _iAnimGraph);
						if (pState->GetNumAnimLayers() && pAnimLayer != NULL)
						{
							int16 iAnim = pAnimLayer->GetAnimIndex();
							Msg += CStrF(", iAnim %d", iAnim);
						}
					}
				}
				else
				{
					Msg += ", iAnim -1";
				}

				//CStr Msg2 = Msg.GetStrSep("->");
				//Msg = Msg.Del(0,Msg.Len()/2);
				//ConOutL(Msg2);
				ConOutL(Msg);
				//if (AG2IDebugFlags & 16)
				{
					Msg += "\n";
					M_TRACEALWAYS(Msg.Str());
				}
			}
		}
	}
#endif
	{
		const CXRAG2_MoveToken* pMoveToken = m_pAG2I->GetMoveToken(_iMoveToken,_iAnimGraph);
		if (pMoveToken)
		{
			int16 iState = pMoveToken->GetTargetStateIndex();
			if (iState != AG2_STATEINDEX_NULL)
				m_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext, GetID(), iState, _iAnimGraph,_iMoveToken);
		}
	}

	m_RefreshGameTime = _pContext->m_GameTime;
	OnEnterState(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
}

#ifndef M_RTM
void CWAG2I_Token::DebugPrintMoveToken(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	bool bIsServer = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsServer() : 0;
	bool bIsClient = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsClient() : 0;
	int ObjectFlags = _pContext->m_pObj ? _pContext->m_pObj->GetPhysState().m_ObjectFlags : 0;
	int iObject = _pContext->m_pObj ? _pContext->m_pObj->m_iObject : 0;

	if (!m_pAG2I->m_bDisableDebug)
	{
		uint32 AG2IDebugFlags = 0;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
		if (pReg != NULL)
			AG2IDebugFlags = pReg->GetValuei("AG2I_DEBUG_FLAGS");

		CWorld_Server* pWorld = TDynamicCast<CWorld_Server>(_pContext->m_pWPhysState);
		if (pWorld)
			AG2IDebugFlags = pWorld->Registry_GetServer()->GetValuei("AG2I_DEBUG_FLAGS");


		bool bDebugServer = s_bDebugServer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
		bool bDebugClient = s_bDebugClient || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
		bool bDebugPlayer = s_bDebugPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
		bool bDebugNonPlayer = s_bDebugNonPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0;

		bool bIsPlayer = ((ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
		if (((bIsServer && bDebugServer) ||
			(bIsClient && bDebugClient)) &&
			((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
		{
			CStr Msg;

			if (bIsServer)
			{
				CWObject* pObj = (CWObject*)_pContext->m_pObj;
				CStr Name = pObj->GetName();
				if (Name.Len())
					Name = "(" + Name + ")";
				Msg += CStrF("S-GB iO:%d%s ", iObject,Name.Str());
			}
			else
				Msg += CStrF("C-GB iO:%d ", iObject);

			//			CStr ExportedActionName = (_iAction != -1) ? (m_pAG2I->GetExportedActionName(iExportedActionName)) : (CStr("ACTION == -1"));
			Msg += CStrF("T:%d GT:%3.2f iMT:%d", m_ID, _pContext->m_GameTime.GetTime(), _iMoveToken);
			const CXRAG2_MoveToken* pMoveToken = m_pAG2I->GetMoveToken(_iMoveToken,_iAnimGraph);
			if (pMoveToken)
			{
				int16 iState = pMoveToken->GetTargetStateIndex();
				int16 iGraphBlock = pMoveToken->GetTargetGraphBlockIndex();

				if (iState != AG2_STATEINDEX_NULL)
				{
					if (iState != AG2_STATEINDEX_TERMINATE)
					{
						iState &= AG2_TARGETSTATE_INDEXMASK;
						Msg += CStrF(" -> State%d", iState);
						CStr StateName = pMoveToken->m_TargetStateType ? m_pAG2I->GetSwitchStateName(iState,_iAnimGraph) : m_pAG2I->GetStateName(iState,_iAnimGraph);
						if (StateName != "")
							Msg += CStrF(" '%s'", StateName.Str());
					}
					else
					{
						Msg += CStr(" -> TERMINATE");
					}
				}

				if (iGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
				{
					CStr GraphBlockName = GetAG2I()->GetExportedGraphBlockName(pMoveToken->m_iTargetGraphBlock,_iAnimGraph);
					Msg += CStrF(" -> GraphBlock %d ('%s')", iGraphBlock,GraphBlockName.Str());
				}

				//Msg += CStrF(", ABlendD %3.3f", pAction->GetAnimBlendDuration());

				const CXRAG2_State* pState = m_pAG2I->GetState(iState,_iAnimGraph);
				if (pState != NULL && pState->GetNumAnimLayers())
				{
					int32 Numlayers = pState->GetNumAnimLayers();
					for (int32 i = 0; i < Numlayers; i++)
					{
						const CXRAG2_AnimLayer* pAnimLayer = m_pAG2I->GetAnimLayer(pState->GetBaseAnimLayerIndex()+i,_iAnimGraph);
						if (pState->GetNumAnimLayers() && pAnimLayer != NULL)
						{
							int16 iAnim = pAnimLayer->GetAnimIndex();
							Msg += CStrF(", iAnim %d", iAnim);
						}
					}
				}
				else
				{
					Msg += CStr(", iAnim -1");
				}

				//CStr Msg2 = Msg.GetStrSep("->");
				//Msg = Msg.Del(0,Msg.Len()/2);
				//ConOutL(Msg2);
				ConOutL(Msg);
				//if (AG2IDebugFlags & 16)
				{
					Msg += "\n";
					M_TRACEALWAYS(Msg.Str());
				}
			}
		}
	}
}
#endif

void CWAG2I_Token::EnterGraphBlock(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset)
{
#ifndef M_RTM
	MSCOPESHORT(CWAG2I_Token::EnterState);
	bool bIsServer = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsServer() : 0;
	bool bIsClient = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsClient() : 0;
	int ObjectFlags = _pContext->m_pObj ? _pContext->m_pObj->GetPhysState().m_ObjectFlags : 0;
	int iObject = _pContext->m_pObj ? _pContext->m_pObj->m_iObject : 0;

	if (!m_pAG2I->m_bDisableDebug)
	{
		uint32 AG2IDebugFlags = 0;
		int32 iAGDbgObj = 0;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
		if (pReg != NULL)
			AG2IDebugFlags = pReg->GetValuei("AG2I_DEBUG_FLAGS");

		CWorld_Server* pWorld = TDynamicCast<CWorld_Server>(_pContext->m_pWPhysState);
		if (pWorld)
		{
			AG2IDebugFlags = pWorld->Registry_GetServer()->GetValuei("AG2I_DEBUG_FLAGS");
			iAGDbgObj = pWorld->Registry_GetServer()->GetValuei("agdbgobj", 0);
		}
		if (!s_bDebugServer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0)
			s_bDebugServer = true;
		if (!s_bDebugClient && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0)
			s_bDebugClient = true;
		if (!s_bDebugPlayer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0)
			s_bDebugPlayer = true;
		if (!s_bDebugNonPlayer && (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0)
			s_bDebugNonPlayer = true;

		bool bDebugServer = s_bDebugServer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
		bool bDebugClient = s_bDebugClient || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
		bool bDebugPlayer = s_bDebugPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
		bool bDebugNonPlayer = s_bDebugNonPlayer || (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0;

		if (iAGDbgObj && (iAGDbgObj != _pContext->m_pObj->m_iObject))
		{
			// Only output for specific ag object
			bDebugServer = false;
			bDebugClient = false;
		}

		bool bIsPlayer = ((ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
		if (((bIsServer && bDebugServer) ||
			(bIsClient && bDebugClient)) &&
			((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
		{
			CStr Msg;

			if (bIsServer)
			{
				CWObject* pObj = (CWObject*)_pContext->m_pObj;
				CStr Name = pObj->GetName();
				if (Name.Len())
					Name = "(" + Name + ")";
				Msg += CStrF("S-GB iO:%d%s ", iObject,Name.Str());
			}
			else
				Msg += CStrF("C-GB iO:%d ", iObject);

			//			CStr ExportedActionName = (_iAction != -1) ? (m_pAG2I->GetExportedActionName(iExportedActionName)) : (CStr("ACTION == -1"));
			Msg += CStrF("T:%d GT:%3.2f iMT:%d", m_ID, _pContext->m_GameTime.GetTime(), _iMoveToken);
			const CXRAG2_MoveToken* pMoveToken = m_pAG2I->GetMoveToken(_iMoveToken,_iAnimGraph);
			if (pMoveToken)
			{
				int16 iState = pMoveToken->GetTargetStateIndex();
				int16 iGraphBlock = pMoveToken->GetTargetGraphBlockIndex();

				if (iState != AG2_STATEINDEX_NULL)
				{
					if (iState != AG2_STATEINDEX_TERMINATE)
					{
						iState &= AG2_TARGETSTATE_INDEXMASK;
						Msg += CStrF(" -> State%d", iState);
						CStr StateName = pMoveToken->m_TargetStateType ? m_pAG2I->GetSwitchStateName(iState,_iAnimGraph) : m_pAG2I->GetStateName(iState,_iAnimGraph);
						if (StateName != "")
							Msg += CStrF(" '%s'", StateName.Str());
					}
					else
					{
						Msg += CStr(" -> TERMINATE");
					}
				}

				if (iGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
				{
					CStr GraphBlockName = GetAG2I()->GetExportedGraphBlockName(pMoveToken->m_iTargetGraphBlock,_iAnimGraph);
					Msg += CStrF(" -> GraphBlock %d ('%s')", iGraphBlock,GraphBlockName.Str());
				}

				//Msg += CStrF(", ABlendD %3.3f", pAction->GetAnimBlendDuration());

				const CXRAG2_State* pState = m_pAG2I->GetState(iState,_iAnimGraph);
				if (pState != NULL && pState->GetNumAnimLayers())
				{
					int32 Numlayers = pState->GetNumAnimLayers();
					for (int32 i = 0; i < Numlayers; i++)
					{
						const CXRAG2_AnimLayer* pAnimLayer = m_pAG2I->GetAnimLayer(pState->GetBaseAnimLayerIndex()+i,_iAnimGraph);
						if (pState->GetNumAnimLayers() && pAnimLayer != NULL)
						{
							int16 iAnim = pAnimLayer->GetAnimIndex();
							Msg += CStrF(", iAnim %d", iAnim);
						}
					}
				}
				else
				{
					Msg += CStr(", iAnim -1");
				}

				//CStr Msg2 = Msg.GetStrSep("->");
				//Msg = Msg.Del(0,Msg.Len()/2);
				//ConOutL(Msg2);
				ConOutL(Msg);
				//if (AG2IDebugFlags & 16)
				{
					Msg += "\n";
					M_TRACEALWAYS(Msg.Str());
				}
			}
		}
	}
#endif

	m_RefreshGameTime = _pContext->m_GameTime;
	OnEnterGraphBlock(_pContext, _iMoveToken, _iAnimGraph);//, _ForceOffset);
}

bool CWAG2I_Token::OnEnterState(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset)
{
	MSCOPESHORT(CWAG2I_Token::OnEnterState);

	// Has been PMRemoved, but has been readded, clear PMRemove flag.
	if ((m_Flags & TOKENFLAGS_PREDMISS_REMOVE) != 0)
		m_Flags &= ~TOKENFLAGS_PREDMISS_REMOVE;

	const CXRAG2_MoveToken* pMoveToken = GetAG2I()->GetMoveToken(_iEnterMoveToken, _iAnimGraph);
	if (!pMoveToken)
		return false;

	// This should never happen (getting to a switchstate)
	M_ASSERT(!pMoveToken->GetTargetStateType(),"INVALID TARGET STATE TYPE");

	int16 iTargetState = pMoveToken->GetTargetStateIndex();
	bool bTerminate = (iTargetState == AG2_STATEINDEX_TERMINATE);
	if (bTerminate)
	{
		CWAG2I_StateInstance* pTokenStateInstance = GetTokenStateInstance();
		if (pTokenStateInstance)
		{
			if (!pTokenStateInstance->m_bGotLeaveState)
				pTokenStateInstance->LeaveState_Setup(_pContext,_iEnterMoveToken);
			if (!pTokenStateInstance->m_bGotLeaveStateInit)
				pTokenStateInstance->LeaveState_Initiate(_pContext);
		}
		return true;
	}

	if (iTargetState == AG2_STATEINDEX_STARTAG)
		ConOut("FIXME STARTAG!!");

	// FIXME: Add support to flush queue from all uninitiated stateinstances that does not have a "NoFlush" flag.

	CWAG2I_SIID SIID(_pContext->m_GameTime, _iEnterMoveToken,_iAnimGraph);
	CWAG2I_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &SIID);

	CWAG2I_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
		return false;

#ifndef M_RTM
	if (pAddedStateInstance != pTokenStateInstance)
		ConOutL("ERROR: TokenStateInstance != AddedStateInstance");
#endif

	if (!pTokenStateInstance->EnterState_Setup(_pContext, _iEnterMoveToken))
		return false;

	/////////
	// Breakout/entry point test
	{
		fp32 Offset = 0.0f;
		if (pTokenStateInstance->m_pAG2I->FindEntryPoint(_pContext, pTokenStateInstance->m_iState, pTokenStateInstance->m_iAnimGraph, m_iPreviousPlayingSequence, 
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
		m_PlayingSIID = CWAG2I_SIID(pTokenStateInstance->GetEnqueueTime(), pTokenStateInstance->GetEnterMoveTokenIndex(),pTokenStateInstance->GetAnimGraphIndex());
		pTokenStateInstance->EnterState_Initiate(_pContext);
		/*
		ConOutL(CStrF("Token::OnEnterState() - PSIID initialized to <QT %3.3f, iEA %d>", 
		m_PlayingSIID.GetEnqueueTime(), m_PlayingSIID.GetEnterActionIndex()));
		*/
	}
	else // Another case is when the PlayingSIID has no animation and a succeeding SI has, but no refresh has yet been called to make it the playing one.
	{
		CWAG2I_Context SplitContext;
		int32 iPlayingStateInstance = ResolvePlayingSIID();
		if (iPlayingStateInstance != AG2I_STATEINSTANCEINDEX_NULL)
		{
			CWAG2I_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
			// Advance through StateInstance queue.
			fp32 TimeFraction;
			while ((pPlayingStateInstance != NULL) && pPlayingStateInstance->ProceedQuery(_pContext, TimeFraction))
			{
				CWAG2I_SIID OldPSIID = m_PlayingSIID;

				_pContext = _pContext->SplitAtFraction(TimeFraction, false, &SplitContext);
				pPlayingStateInstance->LeaveState_Initiate(_pContext);

				iPlayingStateInstance++;
				if (m_lStateInstances.ValidPos(iPlayingStateInstance))
				{
					pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
					m_PlayingSIID = CWAG2I_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterMoveToken,pPlayingStateInstance->m_iAnimGraph);
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

bool CWAG2I_Token::OnEnterGraphBlock(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	MSCOPESHORT(CWAG2I_Token::OnEnterGraphBlock);

	// Has been PMRemoved, but has been readded, clear PMRemove flag.
	if ((m_Flags & TOKENFLAGS_PREDMISS_REMOVE) != 0)
		m_Flags &= ~TOKENFLAGS_PREDMISS_REMOVE;

	// When entering graphblock, do movetoken to correct given state, if no state is specified
	// do the default state, register block reactions with agi (just give block id I guess..)

	int16 iBlock = AG2_GRAPHBLOCKINDEX_NULL;
	int16 iTargetState = AG2_STATEINDEX_NULL;
	const CXRAG2_MoveToken* pMoveToken = GetAG2I()->GetMoveToken(_iEnterMoveToken, _iAnimGraph);
	if (!pMoveToken)
		return false;
	iBlock = pMoveToken->GetTargetGraphBlockIndex();

	// Check target state
	int32 NumLoops = 0;
	while (pMoveToken->GetTargetStateType())
	{
		// Get New movetoken index
		GetAG2I()->EvaluateSwitchState(_pContext,_iEnterMoveToken,_iAnimGraph);
		pMoveToken = GetAG2I()->GetMoveToken(_iEnterMoveToken,_iAnimGraph);
		M_ASSERT(pMoveToken,"INVALID MOVETOKEN");
		NumLoops++;
		M_ASSERT(NumLoops < 10,"STUCK IN LOOP");
		#ifndef M_RTM
		// Debug print the switch state changes
		DebugPrintMoveToken(_pContext,_iEnterMoveToken,_iAnimGraph);
		#endif
	}
	// If the target is not the same block anymore go to the correct one
	if (pMoveToken->GetTargetGraphBlockIndex() != -1 && pMoveToken->GetTargetGraphBlockIndex() != iBlock)
	{
		CWAG2I_Token* pToken = safe_cast<CWAG2I_Token>(this);
		pToken->EnterGraphBlock(_pContext,_iEnterMoveToken,_iAnimGraph);
		return true;
	}

	iTargetState = pMoveToken->m_iTargetState;

	const CXRAG2_GraphBlock* pBlock = GetAG2I()->GetGraphBlock(iBlock, _iAnimGraph);
	if (!pBlock)
		return false;

	if (iTargetState == AG2_STATEINDEX_NULL)
	{
		// Do the start action in this graphblock
		//iTargetState = pBlock->GetStartStateIndex();
		// First movetoken in the block is the start movetoken
		const CXRAG2_MoveToken* pMoveToken = GetAG2I()->GetMoveToken(pBlock->GetStartMoveTokenIndex(), _iAnimGraph);
		M_ASSERT(pMoveToken,"CWAG2I_Token::OnEnterGraphBlock MOVETOKEN DOESN'T EXIST");
		iTargetState = pMoveToken->GetTargetStateIndex();
	}

	// Move token to target state
	bool bTerminate = (iTargetState == AG2_STATEINDEX_TERMINATE);
	if (bTerminate)
		return true;

	if (iTargetState == AG2_STATEINDEX_STARTAG)
		ConOut("FIXME STARTAG!!");

	// FIXME: Add support to flush queue from all uninitiated stateinstances that does not have a "NoFlush" flag.

	CWAG2I_SIID SIID(_pContext->m_GameTime, _iEnterMoveToken,_iAnimGraph);
	CWAG2I_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &SIID);

	CWAG2I_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
		return false;

	if (pAddedStateInstance != pTokenStateInstance)
		ConOutL("ERROR: TokenStateInstance != AddedStateInstance");

	if (!pTokenStateInstance->EnterState_Setup(_pContext, _iEnterMoveToken))
		return false;

	/////////
	// Breakout/entry point test
	{
		fp32 Offset = 0.0f;
		if (pTokenStateInstance->m_pAG2I->FindEntryPoint(_pContext, pTokenStateInstance->m_iState, pTokenStateInstance->m_iAnimGraph, m_iPreviousPlayingSequence, 
			m_PreviousPlayingSequenceTime, Offset))
		{
			pTokenStateInstance->m_Enter_AnimTimeOffset = Offset;
		}
	}
	/////////

	// Make sure we have a playing stateinstance.
	// Cases where PSIID should be reinitialized to entered state is when no PSIID exists or PSIID match created stateinstance.
	if (!m_PlayingSIID.IsValid())
	{
		m_PlayingSIID = CWAG2I_SIID(pTokenStateInstance->GetEnqueueTime(), pTokenStateInstance->GetEnterMoveTokenIndex(),pTokenStateInstance->GetAnimGraphIndex());
		pTokenStateInstance->EnterState_Initiate(_pContext);
	}
	else // Another case is when the PlayingSIID has no animation and a succeeding SI has, but no refresh has yet been called to make it the playing one.
	{
		CWAG2I_Context SplitContext;
		uint8 iPlayingStateInstance = ResolvePlayingSIID();
		if (iPlayingStateInstance != AG2I_STATEINSTANCEINDEX_NULL)
		{
			CWAG2I_StateInstance* pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
			// Advance through StateInstance queue.
			fp32 TimeFraction;
			while ((pPlayingStateInstance != NULL) && pPlayingStateInstance->ProceedQuery(_pContext, TimeFraction))
			{
				CWAG2I_SIID OldPSIID = m_PlayingSIID;

				_pContext = _pContext->SplitAtFraction(TimeFraction, false, &SplitContext);
				pPlayingStateInstance->LeaveState_Initiate(_pContext);

				iPlayingStateInstance++;
				if (m_lStateInstances.ValidPos(iPlayingStateInstance))
				{
					pPlayingStateInstance = &(m_lStateInstances[iPlayingStateInstance]);
					m_PlayingSIID = CWAG2I_SIID(pPlayingStateInstance->m_EnqueueTime, pPlayingStateInstance->m_iEnterMoveToken,pPlayingStateInstance->m_iAnimGraph);
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

	// Set new graphblock
	m_iGraphBlock = iBlock;

	// Enter state on evalutator
	if (pMoveToken)
	{
		int16 iState = pMoveToken->GetTargetStateIndex();
		if (iState != AG2_STATEINDEX_NULL)
			m_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext, GetID(), iState, _iAnimGraph,_iEnterMoveToken);
	}

	return true;
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::LeaveState(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	MSCOPESHORT(CWAG2I_Token::LeaveState);

#ifdef	AG2_DEBUG
	if (bDebug)
	{
		CStr Msg;

		if (_pContext->m_pWPhysState->IsServer())
			Msg += CStrF("(Server %X, pObj %X) ", _pContext->m_pWPhysState, _pContext->m_pObj);
		else
			Msg += CStrF("(Client %X, pObj %X) ", _pContext->m_pWPhysState, _pContext->m_pObj);

		Msg += CStrF("LeaveState: TokenID %d, GameTime %3.2f, iMoveToken %d", m_ID, _pContext->m_GameTime, _iMoveToken);

		ConOutL(Msg);
	}
#endif
	OnLeaveState(_pContext, _iMoveToken, _iAnimGraph);
}

void CWAG2I_Token::LeaveGraphBlock(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	MSCOPESHORT(CWAG2I_Token::LeaveState);

	OnLeaveGraphBlock(_pContext, _iMoveToken, _iAnimGraph);
}

bool CWAG2I_Token::OnLeaveGraphBlock(const CWAG2I_Context* _pContext, int16 _iLeaveMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	MSCOPESHORT(CWAG2I_Token::OnLeaveGraphBlock);

	// Set previous graphblock impulsetype and value
	const CXRAG2_GraphBlock* pGraphBlock = GetAG2I()->GetGraphBlock(m_iGraphBlock, _iAnimGraph);
	if (pGraphBlock)
	{
		GetAG2I()->m_pEvaluator->SetPropertyInt(PROPERTY_INT_PREVIOUSGRAPHBLOCKTYPE,pGraphBlock->GetCondition().m_ImpulseType);
		GetAG2I()->m_pEvaluator->SetPropertyInt(PROPERTY_INT_PREVIOUSGRAPHBLOCKVALUE,pGraphBlock->GetCondition().m_ImpulseValue);
	}

	// Leave state
	OnLeaveState(_pContext, _iLeaveMoveToken, _iAnimGraph);

	// Has been PMRemoved, so don't destroy PM fadeouts, act as if not existant.
	/*if ((m_Flags & TOKENFLAGS_PREDMISS_REMOVE) != 0)
	return true;

	CWAG2I_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
	return false;

	/////// Breakout/entry point test
	if (pTokenStateInstance->HasAnimation())
	{
	pTokenStateInstance->m_pAG2I->FindBreakoutSequence(_pContext, pTokenStateInstance->m_iState, 
	pTokenStateInstance->GetEnterAnimTimeOffset(), m_iPreviousPlayingSequence, 
	m_PreviousPlayingSequenceTime);
	}
	///////

	if (!pTokenStateInstance->LeaveState_Setup(_pContext, _iLeaveAction, _iLeaveMoveToken))
	return false;*/

	return true;
}

//--------------------------------------------------------------------------------

bool CWAG2I_Token::OnLeaveState(const CWAG2I_Context* _pContext, int16 _iLeaveMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	MSCOPESHORT(CWAG2I_Token::OnLeaveState);

	// Has been PMRemoved, so don't destroy PM fadeouts, act as if not existant.
	if ((m_Flags & TOKENFLAGS_PREDMISS_REMOVE) != 0)
		return true;

	CWAG2I_StateInstance* pTokenStateInstance = GetTokenStateInstance();
	if (pTokenStateInstance == NULL)
		return false;

	/////// Breakout/entry point test
	if (pTokenStateInstance->HasAnimation())
	{
		pTokenStateInstance->FindBreakoutSequence(_pContext, m_iPreviousPlayingSequence, 
			m_PreviousPlayingSequenceTime);
	}
	///////

	pTokenStateInstance->m_LeaveStateFrom = 1;
	if (!pTokenStateInstance->LeaveState_Setup(_pContext, _iLeaveMoveToken))
		return false;

	return true;
}

//--------------------------------------------------------------------------------

bool CWAG2I_Token::DisableStateInstanceAnims(const CWAG2I_Context* _pContext, const CWAG2I_StateInstance* _pStateInstance, int _iDisableSIAnimsCallbackMsg) const
{
	MSCOPESHORT(CWAG2I_Token::DisableStateInstanceAnims);

	CWObject_Message Msg(_iDisableSIAnimsCallbackMsg, (aint)_pStateInstance);
	return (_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject) != 0);
}

//--------------------------------------------------------------------------------

CAG2StateIndex CWAG2I_Token::GetAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, 
										   int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg,
										   bool _bAllowBlend) const
{
	MSCOPESHORT(CWAG2I_Token::GetAnimLayers);

	if (_nLayers < 1)
		return -1;

	int nTotalLayers = 0;
	bool bSkipForceKeep = false;
	CAG2StateIndex iStatePerfect = -1;
	bool bPerfectDisabled = false;
	int32 NumStateInst = m_lStateInstances.Len();

	for (int iStateInstance = 0; iStateInstance < NumStateInst; iStateInstance++)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		bPerfectDisabled |= pStateInstance->m_bNoPrevExactMove;

		if ((_iDisableStateInstanceAnimsCallbackMsg != 0) &&
			DisableStateInstanceAnims(_pContext, pStateInstance, _iDisableStateInstanceAnimsCallbackMsg))
			continue;

		int nStateInstanceLayers = _nLayers - nTotalLayers;
		CXR_AnimLayer* pStateInstanceLayers = &(_pLayers[nTotalLayers]);

		bool bForceKeep = false;
		if (!nTotalLayers && pStateInstance->m_bSkipForceKeep)
			bSkipForceKeep = true;

		if (!pStateInstance->m_bSkipForceKeep && pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
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

				const CWAG2I_StateInstance* pPostStateInstance = &(m_lStateInstances[jStateInstance]);
				if (pPostStateInstance->m_bHasAnimation)
					bSucceedingAnimation = true;

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_UNDEFINEDTIME) == 0) || 
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					(pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) == 0) &&
					(pStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
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
		pStateInstance->GetAnimLayers(_pContext, !bForceKeep, pStateInstanceLayers, nStateInstanceLayers, (iStateInstance == (NumStateInst - 1)) && (nTotalLayers == 0));
		if (nStateInstanceLayers && pStateInstance->m_bHasPerfectMovement)
			iStatePerfect = pStateInstance->m_iState;
		nTotalLayers += nStateInstanceLayers;
	}

	_nLayers = nTotalLayers;

	/*for (int iLayer = 0; iLayer < _nLayers; iLayer++)
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
	}*/

	if ((_nLayers > 1) && !_bAllowBlend && !bSkipForceKeep)
	{
		_pLayers[0].m_Blend = 1.0f;
	}
	return bPerfectDisabled ? -1 : iStatePerfect;
}

void CWAG2I_Token::GetValueCompareLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int32 _Value) const
{
	MSCOPESHORT(CWAG2I_Token::GetValueCompareLayers);
	if (_nLayers < 1)
		return;

	int nTotalLayers = 0;
	bool bSkipForceKeep = false;

	// Only get top anim layer..
	TAP_RCD<const CWAG2I_StateInstance> lStateInstances = m_lStateInstances;
	for (int iStateInstance = 0; iStateInstance < lStateInstances.Len(); iStateInstance++)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAG2I_StateInstance* pStateInstance = &(lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		int nStateInstanceLayers = _nLayers - nTotalLayers;
		CXR_AnimLayer* pStateInstanceLayers = &(_pLayers[nTotalLayers]);

		bool bForceKeep = false;
		if (!nTotalLayers && pStateInstance->m_bSkipForceKeep)
			bSkipForceKeep = true;

		if (!pStateInstance->m_bSkipForceKeep && pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
			(pStateInstance->m_BlendOutStartTime.Compare(_pContext->m_GameTime) <= 0))
		{
			bForceKeep = (m_lStateInstances.Len() > 1);
			bool bSucceedingAnimation = false;
			// Search for fully blended in succeeding animation.
			for (int jStateInstance = iStateInstance + 1; jStateInstance < lStateInstances.Len(); jStateInstance++)
			{
				const CWAG2I_StateInstance* pPostStateInstance = &(lStateInstances[jStateInstance]);
				if (pPostStateInstance->m_bHasAnimation)
					bSucceedingAnimation = true;

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_UNDEFINEDTIME) == 0) || 
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					(pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) == 0) &&
					(pStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
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

		// Only care about the value compare layers
		pStateInstance->GetValueCompareLayers(_pContext, pStateInstanceLayers, nStateInstanceLayers, _Value);
		nTotalLayers += nStateInstanceLayers;
	}

	_nLayers = nTotalLayers;
}

void CWAG2I_Token::GetTopEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, int& _nLayers, bool _bAllowBlend)
{
	MSCOPESHORT(CWAG2I_Token::GetAnimLayers);

	if (_nLayers < 1)
		return;

	int nTotalLayers = 0;
	bool bSkipForceKeep = false;

	// Only get top anim layer..
	for (int iStateInstance = m_lStateInstances.Len()-1; iStateInstance >= 0; iStateInstance--)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		int nStateInstanceLayers = _nLayers - nTotalLayers;
		CEventLayer* pStateInstanceLayers = &(_pLayers[nTotalLayers]);

		bool bForceKeep = false;
		if (!nTotalLayers && pStateInstance->m_bSkipForceKeep)
			bSkipForceKeep = true;

		if (!pStateInstance->m_bSkipForceKeep && pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
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

				const CWAG2I_StateInstance* pPostStateInstance = &(m_lStateInstances[jStateInstance]);
				if (pPostStateInstance->m_bHasAnimation)
					bSucceedingAnimation = true;

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_UNDEFINEDTIME) == 0) || 
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					(pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) == 0) &&
					(pStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
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
		// Only care about the sequences not their blend and such...
		pStateInstance->GetEventLayers(_pContext, pStateInstanceLayers, nStateInstanceLayers);
		nTotalLayers += nStateInstanceLayers;

		// Break if this state instace had an anim layer
		if (nStateInstanceLayers > 0)
			break;
	}

	_nLayers = nTotalLayers;
}

void CWAG2I_Token::GetEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, int& _nLayers, bool _bAllowBlend)
{
	MSCOPESHORT(CWAG2I_Token::GetAnimLayers);

	if (_nLayers < 1)
		return;

	int nTotalLayers = 0;
	bool bSkipForceKeep = false;

	// Only get top anim layer..
	for (int iStateInstance = m_lStateInstances.Len()-1; iStateInstance >= 0; iStateInstance--)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		int nStateInstanceLayers = _nLayers - nTotalLayers;
		CEventLayer* pStateInstanceLayers = &(_pLayers[nTotalLayers]);

		bool bForceKeep = false;
		if (!nTotalLayers && pStateInstance->m_bSkipForceKeep)
			bSkipForceKeep = true;

		if (!pStateInstance->m_bSkipForceKeep && pStateInstance->m_bHasAnimation && 
			(pStateInstance->m_BlendOutStartTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
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

				const CWAG2I_StateInstance* pPostStateInstance = &(m_lStateInstances[jStateInstance]);
				if (pPostStateInstance->m_bHasAnimation)
					bSucceedingAnimation = true;

				if (pPostStateInstance->m_bHasAnimation &&
					(((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_UNDEFINEDTIME) != 0) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_UNDEFINEDTIME) == 0) || 
					((pPostStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
					(pPostStateInstance->m_BlendOutEndTime.Compare(_pContext->m_GameTime) <= 0))) &&
					(pPostStateInstance->m_BlendInEndTime.Compare(_pContext->m_GameTime) <= 0)) ||
					((pPostStateInstance->m_BlendInEndTime.Compare(AG2I_LINKEDTIME) == 0) &&
					(pStateInstance->m_BlendOutEndTime.Compare(AG2I_LINKEDTIME) != 0) &&
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
		// Only care about the sequences not their blend and such...
		pStateInstance->GetEventLayers(_pContext, pStateInstanceLayers, nStateInstanceLayers);
		nTotalLayers += nStateInstanceLayers;
	}

	_nLayers = nTotalLayers;
}

bool CWAG2I_Token::GetSpecificAnimLayer(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iAnim, int32 _StartTick) const
{
	MSCOPESHORT(CWAG2I_Token::GetSpecificAnimLayer);

	int32 Len = m_lStateInstances.Len();
	for (int iStateInstance = Len - 1; iStateInstance >= 0 ; iStateInstance--)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved())
			continue;

		if (pStateInstance->GetSpecificAnimLayer(_pContext, _Layer, iStateInstance, _iAnim,_StartTick))
			return true;
	}

	return false;
}

bool CWAG2I_Token::GetAnimLayerFromState(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, CAG2StateIndex _iState) const
{
	MSCOPESHORT(CWAG2I_Token::GetAnimLayerFromState);

	int32 Len = m_lStateInstances.Len();
	int nLayers = 1;
	for (int iStateInstance = Len - 1; iStateInstance >= 0 ; iStateInstance--)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved() || pStateInstance->m_iState != _iState)
			continue;

		pStateInstance->GetAnimLayerSeqs(_pContext, &_Layer, nLayers);
		return nLayers > 0;
	}

	return false;
}

const CWAG2I_StateInstance* CWAG2I_Token::GetSpecificState(const CWAG2I_Context* _pContext, int32 _iAnim) const
{
	MSCOPESHORT(CWAG2I_Token::GetSpecificState);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		//ConOutL(CStrF("iSI %d", iStateInstance));

		const CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if (pStateInstance->IsPMRemoved() || !pStateInstance->HasSpecificAnimation(_iAnim))
			continue;

		return pStateInstance;
	}

	return NULL;
}

//--------------------------------------------------------------------------------

CWAG2I_StateInstance* CWAG2I_Token::GetMatchingStateInstance(const CWAG2I_SIID* _pSIID, bool _bCreateNonExistent)
{
	MSCOPESHORT(CWAG2I_Token::GetMatchingStateInstance);

	for (int iStateInstance = 0; iStateInstance < m_lStateInstances.Len(); iStateInstance++)
	{
		// FIXME: Should this ignore PMRemoved SIs?! (then this function could be used by CreateAndEnqueue...
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_pSIID->GetEnqueueTime().Compare(pStateInstance->m_EnqueueTime) == 0) &&
			(_pSIID->GetEnterMoveTokenIndex() == pStateInstance->m_iEnterMoveToken) &&
			(_pSIID->GetAnimGraphIndex() == pStateInstance->m_iAnimGraph))
			return pStateInstance;
	}

	if (_bCreateNonExistent)
	{
		int iStateInstance = m_lStateInstances.Add(CWAG2I_StateInstance(_pSIID, GetAG2I(), this));
		return &(m_lStateInstances[iStateInstance]);
	}

	return NULL;
}

void CWAG2I_Token::RemoveStateInstance(int32 _iStateInstance)
{
	m_lStateInstances.Del(_iStateInstance);
}

bool CWAG2I_Token::RemoveStateInstance(const CWAG2I_SIID& _SIID)
{
	int32 Len = m_lStateInstances.Len();
	for (int iStateInstance = 0; iStateInstance < Len; iStateInstance++)
	{
		// FIXME: Should this ignore PMRemoved SIs?! (then this function could be used by CreateAndEnqueue...
		CWAG2I_StateInstance* pStateInstance = &(m_lStateInstances[iStateInstance]);
		if ((_SIID.GetEnqueueTime().Compare(pStateInstance->m_EnqueueTime) == 0) &&
			(_SIID.GetEnterMoveTokenIndex() == pStateInstance->m_iEnterMoveToken))
		{
			m_lStateInstances.Del(iStateInstance);
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

void CWAG2I_Token::EvalNodeInit()
{
	const CWAG2I_StateInstance* pCachedStateInstance = GetTokenStateInstance();
	m_CachedAnimGraphIndex = pCachedStateInstance->GetAnimGraphIndex();
	const CXRAG2_State* pCachedState = m_pAG2I->GetState(pCachedStateInstance->GetStateIndex(), m_CachedAnimGraphIndex);
	m_CachedBaseNodeIndex = pCachedState->GetBaseNodeIndex();
}

uint16 CWAG2I_Token::EvalNode(const CWAG2I_Context* _pContext, const CXRAG2_ConditionNodeV2* _pNode, fp32& _TimeFraction) const
{
	fp32 NewTimeFraction = 0;
	bool ConditionResult = m_pAG2I->EvaluateCondition(_pContext, _pNode, NewTimeFraction);

	fp32 TrueTimeFraction;
	fp32 FalseTimeFraction = _TimeFraction;
	int TrueAction = AG2_NODEACTION_FAILPARSE;
	int FalseAction = AG2_NODEACTION_FAILPARSE;
	const CXRAG2* pAnimGraph = m_pAG2I->GetAnimGraph(m_CachedAnimGraphIndex);
	M_ASSERT(pAnimGraph,"Invalid AnimGraph");

	if (ConditionResult)
	{
		TrueTimeFraction = Max(_TimeFraction, NewTimeFraction);
		//		TrueAction = EvalNodeAction(_pContext, _iStatePropertyParamBase, _pNode->GetTrueAction(), TrueTimeFraction);

		TrueAction = _pNode->GetTrueAction();

		if (TrueAction != AG2_NODEACTION_FAILPARSE)
		{
			if ((TrueAction & AG2_NODEACTION_TYPEMASK) != AG2_NODEACTION_ENDPARSE)
			{
				int iNode = m_CachedBaseNodeIndex + (TrueAction & AG2_NODEACTION_NODEMASK);

				TrueAction = EvalNode(_pContext, pAnimGraph->GetNode(iNode), TrueTimeFraction);
			}
		}
	}

	FalseAction = _pNode->GetFalseAction();

	if (FalseAction != AG2_NODEACTION_FAILPARSE)
	{
		if ((FalseAction & AG2_NODEACTION_TYPEMASK) != AG2_NODEACTION_ENDPARSE)
		{
			int iNode = m_CachedBaseNodeIndex + (FalseAction & AG2_NODEACTION_NODEMASK);

			FalseAction = EvalNode(_pContext, pAnimGraph->GetNode(iNode), FalseTimeFraction);
		}
	}

	//	FalseAction = EvalNodeAction(_pContext, _iStatePropertyParamBase, _pNode->GetFalseAction(), FalseTimeFraction);

	if (TrueAction != AG2_NODEACTION_FAILPARSE)
	{
		if (FalseAction != AG2_NODEACTION_FAILPARSE)
		{
			if (FalseTimeFraction < TrueTimeFraction)
			{
				_TimeFraction = FalseTimeFraction;
				return FalseAction;
			}
		}

		_TimeFraction = TrueTimeFraction;
		return TrueAction;
	}
	else if (FalseAction != AG2_NODEACTION_FAILPARSE)
	{
		_TimeFraction = FalseTimeFraction;
		return FalseAction;
	}

	_TimeFraction = 0;
	return AG2_NODEACTION_FAILPARSE;
}

//--------------------------------------------------------------------------------

uint16 CWAG2I_Token::EvalNodeAction(const CWAG2I_Context* _pContext, uint16 _NodeAction, fp32& _TimeFraction)
{
	//	MSCOPESHORT( CWAG2I_Token::EvalNodeAction );

	if (_NodeAction == AG2_NODEACTION_FAILPARSE)
		return _NodeAction;

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_ENDPARSE)
		return _NodeAction;

	const CXRAG2_State* pState = m_pAG2I->GetState(GetStateIndex(),m_CachedAnimGraphIndex);
	int iNode = pState->GetBaseNodeIndex() + (_NodeAction & AG2_NODEACTION_NODEMASK);
	const CXRAG2_ConditionNodeV2* pNode = m_pAG2I->GetNode(iNode,m_CachedAnimGraphIndex);

	return EvalNode(_pContext, pNode, _TimeFraction);
}

bool CWAG2I_Token::OnCreateClientUpdate(uint8*& _pData, const CWAG2I_Token* _pMirror) const
{
	// ID should already be sorted (from agi update), go ahead with the other stuff
	uint8& CopyFlags = _pData[0];
	_pData++;
	CopyFlags = 0;//m_DirtyFlag;

	if (_pMirror->m_iGraphBlock != m_iGraphBlock)
	{
		CopyFlags |= AG2I_TOKEN_DIRTYFLAG_GRAPHBLOCK;
		PTR_PUTINT16(_pData, m_iGraphBlock);
	}

	// Only copy playing stateinstance (for now...)
	const CWAG2I_StateInstance* pStateInst = GetTokenStateInstanceUpdate();
	if (pStateInst)
	{
		uint8* pDataBefore = _pData;
		// Put siid (entermovetoken and enqueue time)
		CWAG2I_SIID Siid;
		Siid.m_EnqueueTime = pStateInst->GetEnqueueTime();
		Siid.m_iEnterMoveToken = pStateInst->GetEnterMoveTokenIndex();
		Siid.m_iAnimGraph = pStateInst->GetAnimGraphIndex();

		// Should be the only stateinstance in the mirror
		const CWAG2I_StateInstance* pStateInstMirror = _pMirror->GetNumStateInstances() > 0 ? _pMirror->GetStateInstance(0) : NULL;
		static const CWAG2I_StateInstance DummyInst;
		if (!pStateInstMirror)
		{
			pStateInstMirror = &DummyInst;
		}
		else if (!(Siid.GetEnqueueTime().AlmostEqual(pStateInstMirror->m_EnqueueTime)) ||
			(Siid.GetEnterMoveTokenIndex() != pStateInstMirror->m_iEnterMoveToken) ||
			(Siid.GetAnimGraphIndex() != pStateInstMirror->m_iAnimGraph))
		{
			// Check so that siid match
			pStateInstMirror = &DummyInst;
		}

		Siid.OnCreateClientUpdate(_pData);
		if (pStateInst->OnCreateClientUpdate(_pData, pStateInstMirror))
			CopyFlags |= AG2I_TOKEN_DIRTYFLAG_STATEINSTANCE;
		else
			_pData = pDataBefore;
	}

	return (CopyFlags != 0);
}

bool CWAG2I_Token::OnClientUpdate(CWAG2I_Context* _pContext, const uint8*& _pData)
{
	//	const uint8* pBase = _pData;

	// Okidoki then, this function will put anything

	// Update Flag (8 bit), (agres, Tokens, overlay)
	PTR_GETINT8(_pData, m_DirtyFlag);

	if (m_DirtyFlag & AG2I_TOKEN_DIRTYFLAG_GRAPHBLOCK)
	{
		PTR_GETINT16(_pData, m_iGraphBlock);
	}

	// Only copy playing stateinstance (for now...)
	if (m_DirtyFlag & AG2I_TOKEN_DIRTYFLAG_STATEINSTANCE)
	{
		// Get siid
		CWAG2I_SIID Siid;
		Siid.OnClientUpdate(_pData);
		CWAG2I_StateInstance* pStateInst = GetMatchingStateInstance(&Siid,false);
		if (pStateInst)
		{
			pStateInst->OnClientUpdate(_pContext, Siid, _pData);
		}
		else
		{
			// Clear old states (only keep the recent state)
			m_lStateInstances.Clear();
			// Non existant state instance, create and enqueue it
			CWAG2I_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &Siid);
			pAddedStateInstance->EnterState_Initiate(_pContext);
			pAddedStateInstance->OnClientUpdate(_pContext, Siid, _pData);
		}

	}

	return false;
}

void CWAG2I_Token::UpdateFromMirror(CWAG2I_Context* _pContext, CWAG2I_Token* _pMirror)
{
	m_iGraphBlock = _pMirror->m_iGraphBlock;

	// Update stateinstances, if the stateinstance exists, make sure it's first in line
	// if it doesn't exist, put that stateinstance first in list
	CWAG2I_StateInstance* pMirrorStateInst = _pMirror->GetTokenStateInstanceUpdate();
	if (!pMirrorStateInst)
		return;

	CWAG2I_StateInstance* pStateInst = m_lStateInstances.Len() > 0 ? &(m_lStateInstances[m_lStateInstances.Len()-1]) : NULL;
	if (pStateInst)
	{
		CWAG2I_SIID SIID(pMirrorStateInst->GetEnqueueTime(),pMirrorStateInst->GetEnterMoveTokenIndex(),pMirrorStateInst->GetAnimGraphIndex());
		//		fp32 EnqueueDiff = (pStateInst->GetEnqueueTime() - pMirrorStateInst->GetEnqueueTime()).GetTime();
		//		int32 iMTDIFF = pStateInst->GetEnterMoveTokenIndex() - pMirrorStateInst->GetEnterMoveTokenIndex();
		//		int32 iAGDiff = pStateInst->GetAnimGraphIndex() - pMirrorStateInst->GetAnimGraphIndex();

		fp32 TickTime = _pContext->m_pWPhysState->GetGameTickTime();
		if ((Abs((pStateInst->GetEnqueueTime() - pMirrorStateInst->GetEnqueueTime()).GetTime()) < TickTime) &&
			(pStateInst->GetEnterMoveTokenIndex() == pMirrorStateInst->GetEnterMoveTokenIndex()) &&
			(pStateInst->GetAnimGraphIndex() == pMirrorStateInst->GetAnimGraphIndex()))
		{
			// Found compatible stateinst, update it (and possible change enqueue time)
			pStateInst->m_EnqueueTime = pMirrorStateInst->m_EnqueueTime;
			pStateInst->UpdateFromMirror(_pContext, pMirrorStateInst);
			pStateInst->EnterState_Initiate(_pContext);
		}
		else
		{
			// Leave current state.... (somehow FIXME)
			pStateInst->LeaveState_Initiate(_pContext);

			CWAG2I_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &SIID);
			CWAG2I_Context Context(*_pContext);
			Context.m_GameTime = pMirrorStateInst->m_EnqueueTime;
			pAddedStateInstance->EnterState_Setup(&Context,pMirrorStateInst->GetEnterMoveTokenIndex());

			fp32 Offset = 0.0f;
			if (pAddedStateInstance->GetAG2I()->FindEntryPoint(&Context, pAddedStateInstance->GetStateIndex(), pAddedStateInstance->GetAnimGraphIndex(), m_iPreviousPlayingSequence, 
				m_PreviousPlayingSequenceTime, Offset))
			{
				pAddedStateInstance->m_Enter_AnimTimeOffset = Offset;
			}

			Context.m_GameTime = pMirrorStateInst->m_EnterTime;
			pAddedStateInstance->EnterState_Initiate(&Context);
			m_PlayingSIID = CWAG2I_SIID(pAddedStateInstance->m_EnqueueTime, pAddedStateInstance->m_iEnterMoveToken,pAddedStateInstance->m_iAnimGraph);
			m_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext,m_ID,pAddedStateInstance->m_iState,SIID.m_iAnimGraph,SIID.m_iEnterMoveToken);
			//CStr StateName = m_pAG2I->GetStateName(pAddedStateInstance->GetStateIndex(), SIID.m_iAnimGraph);
			//ConOutL(CStrF("T: %f - Predictionmiss, entering new state: %d (%s) EQ: %f EN: %f  EQDiff: %f iMTDiff: %d iAGDiff: %d",_pContext->m_GameTime.GetTime(),pAddedStateInstance->GetStateIndex(),StateName.Str(),pAddedStateInstance->m_EnqueueTime.GetTime(),pAddedStateInstance->m_EnterTime.GetTime(), EnqueueDiff, iMTDIFF,iAGDiff));
		}
	}
	else
	{
		// If enqueue time diff is larger than 2 server ticks something is wrong, and skip to add it
		/*if ((pMirrorStateInst->GetEnqueueTime() - _pContext->m_GameTime).Compare(CMTime::CreateFromSeconds(SERVER_TIMEPERFRAME*2)) >= 0.0f)
		{
		ConOut(CStrF("Suspicious state probably removed, but : %f",));
		return;
		}*/
		CWAG2I_SIID SIID(pMirrorStateInst->GetEnqueueTime(),pMirrorStateInst->GetEnterMoveTokenIndex(),pMirrorStateInst->GetAnimGraphIndex());
		CWAG2I_StateInstance* pAddedStateInstance = CreateAndEnqueueStateInstance(_pContext, &SIID);
		CWAG2I_Context Context(*_pContext);
		Context.m_GameTime = pMirrorStateInst->m_EnqueueTime;
		pAddedStateInstance->EnterState_Setup(&Context,pMirrorStateInst->GetEnterMoveTokenIndex());
		fp32 Offset = 0.0f;
		if (pAddedStateInstance->GetAG2I()->FindEntryPoint(&Context, pAddedStateInstance->GetStateIndex(), pAddedStateInstance->GetAnimGraphIndex(), m_iPreviousPlayingSequence, 
			m_PreviousPlayingSequenceTime, Offset))
		{
			pAddedStateInstance->m_Enter_AnimTimeOffset = Offset;
		}

		Context.m_GameTime = pMirrorStateInst->m_EnterTime;
		pAddedStateInstance->EnterState_Initiate(&Context);
		m_PlayingSIID = CWAG2I_SIID(pAddedStateInstance->m_EnqueueTime, pAddedStateInstance->m_iEnterMoveToken,pAddedStateInstance->m_iAnimGraph);
		m_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext,m_ID,pAddedStateInstance->m_iState,SIID.m_iAnimGraph,SIID.m_iEnterMoveToken);

		//CStr StateName = m_pAG2I->GetStateName(pAddedStateInstance->GetStateIndex(), SIID.m_iAnimGraph);
		//ConOutL(CStrF("T: %f - Predictionmiss, ADDING new state: %d (%s) EQ: %f EN: %f",_pContext->m_GameTime.GetTime(),pMirrorStateInst->GetStateIndex(),StateName.Str(), pAddedStateInstance->m_EnqueueTime.GetTime(),pAddedStateInstance->m_EnterTime.GetTime()));
	}
}

void CWAG2I_Token::Write(CCFile* _pFile) const
{
	// Write infos
	m_RefreshGameTime.Write(_pFile);
	_pFile->WriteLE(m_ID);
	m_PlayingSIID.Write(_pFile);
	m_PreviousPlayingSequenceTime.Write(_pFile);
	_pFile->WriteLE(m_iPreviousPlayingSequence);
	_pFile->WriteLE(m_iGraphBlock);
	_pFile->WriteLE(m_Flags);
	
	// Save first namehash
	int32 NameHash = 0;
	if (m_pAG2I->m_lAnimGraph2Res.Len())
	{
		NameHash = m_pAG2I->m_lAnimGraph2Res[0].m_NameHash;
	}
	_pFile->WriteLE(NameHash);
	

	// Write stateinstances
	int32 Len = m_lStateInstances.Len();
	_pFile->WriteLE(Len);
	for (int32 i = 0; i < Len; i++)
		m_lStateInstances[i].Write(_pFile);
}

void CWAG2I_Token::Read(CWAG2I_Context* _pContext, CCFile* _pFile)
{
	// Read infos
	// Write infos
	
	m_RefreshGameTime.Read(_pFile);
	_pFile->ReadLE(m_ID);
	m_PlayingSIID.Read(_pFile);
	m_PreviousPlayingSequenceTime.Read(_pFile);
	_pFile->ReadLE(m_iPreviousPlayingSequence);
	_pFile->ReadLE(m_iGraphBlock);
	_pFile->ReadLE(m_Flags);

	// Check first agname and see if it's the same
	bool bDoOnenterState = false;
	if (m_pAG2I->m_lAnimGraph2Res.Len())
	{
		int32 NameHash;
		_pFile->ReadLE(NameHash);
		if (m_pAG2I->m_lAnimGraph2Res[0].m_NameHash == NameHash)
			bDoOnenterState = true;
	}
	// Write stateinstances
	int32 Len;
	_pFile->ReadLE(Len);
	m_lStateInstances.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	{
		m_lStateInstances[i].SetAG2I(m_pAG2I);
		//m_lStateInstances[i].SetToken(this);
		m_lStateInstances[i].Read(_pFile);
		// Do onenterstate so flags remains plz
		if (bDoOnenterState)
			m_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext,m_ID,m_lStateInstances[i].m_iState,m_lStateInstances[i].m_iAnimGraph,m_lStateInstances[i].m_iEnterMoveToken);
	}
}

//--------------------------------------------------------------------------------

#if	defined(M_RTM) && defined( AG2_DEBUG )
#warning "M_RTM and AG2_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
