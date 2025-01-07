#include "PCH.h"

//--------------------------------------------------------------------------------
#ifndef M_RTM
#include "../Server/WServer.h"
static bool bDebug = false;
#endif
//--------------------------------------------------------------------------------

#include "WAGI.h"
#include "WAGI_Token.h"
#include "WAGI_StateInst.h"
#include "WAG_ClientData.h"

//--------------------------------------------------------------------------------

void CWAGI_Token::Clear()
{
	m_pAGI = NULL;
	m_ID = AG_TOKENID_NULL;
	//m_bFirstRefresh = true;
	m_RefreshGameTime.Reset();
	CWAGI_SIQ::Clear();
}

//--------------------------------------------------------------------------------

void CWAGI_Token::CopyFrom(const CWAGI_Token* _pToken)
{
	m_pAGI = _pToken->m_pAGI;
	m_ID = _pToken->m_ID;
//	m_bFirstRefresh = _pToken->m_bFirstRefresh;
	m_RefreshGameTime = _pToken->m_RefreshGameTime;
	CWAGI_SIQ::CopyFrom(_pToken);
}

//--------------------------------------------------------------------------------

int16 CWAGI_Token::GetStateIndex() const
{
	//MSCOPESHORT(CWAGI_Token::GetStateIndex);

	const CWAGI_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return AG_STATEINDEX_NULL;

	return pStateInstance->GetStateIndex();
}

//--------------------------------------------------------------------------------

CMTime CWAGI_Token::GetEnterStateTime() const
{
	MSCOPESHORT(CWAGI_Token::GetEnterStateTime);

	const CWAGI_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return AGI_UNDEFINEDTIME;

	return pStateInstance->GetEnqueueTime();
}

//--------------------------------------------------------------------------------
#ifndef M_RTM
static bool s_bDebugServer = false;
static bool s_bDebugClient = false;
static bool s_bDebugNonPlayer = false;
static bool s_bDebugPlayer = true;
#endif

void CWAGI_Token::EnterState(const CWAGI_Context* _pContext, int16 _iAction, fp32 _ForceOffset)
{
	MSCOPESHORT(CWAGI_Token::EnterState);

#ifndef M_RTM
	bool bIsServer = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsServer() : 0;
	bool bIsClient = _pContext->m_pWPhysState ? _pContext->m_pWPhysState->IsClient() : 0;
	int ObjectFlags = _pContext->m_pObj ? _pContext->m_pObj->GetPhysState().m_ObjectFlags : 0;
	int iObject = _pContext->m_pObj ? _pContext->m_pObj->m_iObject : 0;

	{
		uint32 AGIDebugFlags = 0;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
		if (pReg != NULL)
			AGIDebugFlags = pReg->GetValuei("AGI_DEBUG_FLAGS");

		CWorld_Server* pWorld = TDynamicCast<CWorld_Server>(_pContext->m_pWPhysState);
		if (pWorld)
			AGIDebugFlags = pWorld->Registry_GetServer()->GetValuei("AGI_DEBUG_FLAGS");


		bool bDebugServer = s_bDebugServer || (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
		bool bDebugClient = s_bDebugClient || (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
		bool bDebugPlayer = s_bDebugPlayer || (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
		bool bDebugNonPlayer = s_bDebugNonPlayer || (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0;
/*		bDebugServer = true;
		bDebugClient = true;
		bDebugPlayer = true;
		bDebugNonPlayer = true;
		bIsServer = true;*/

		bool bIsPlayer = ((ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
		if (((bIsServer && bDebugServer) ||
			(bIsClient && bDebugClient)) &&
			((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
		{
			CStr Msg;
			
			if (bIsServer)
				Msg += CStrF("S iO:%d ", iObject);
			else
				Msg += CStrF("C iO:%d ", iObject);

			int iExportedActionName = _iAction; // FIXME: This 1/1 mapping is NOT garanteed.
//			CStr ExportedActionName = (_iAction != -1) ? (m_pAGI->GetExportedActionName(iExportedActionName)) : (CStr("ACTION == -1"));
			CStr ExportedActionName;
			if( _iAction != -1 ) ExportedActionName = m_pAGI->GetExportedActionName(iExportedActionName);
			else ExportedActionName = "ACTION == -1";
			Msg += CStrF("T:%d GT:%3.2f iA:%d '%s'", m_ID, _pContext->m_GameTime.GetTime(), _iAction, ExportedActionName.Str());
			const CXRAG_Action* pAction = m_pAGI->GetAction(_iAction);
			if (pAction != NULL)
			{
				int16 iState = pAction->GetTargetStateIndex();

				if (iState != AG_STATEINDEX_NULL)
				{
					if (iState != AG_STATEINDEX_TERMINATE)
					{
						iState &= AG_TARGETSTATE_INDEXMASK;
						Msg += CStrF(" -> State%d", iState);
						CStr StateName = m_pAGI->GetStateName(iState);
						if (StateName != "")
							Msg += CStrF(" '%s'", StateName.Str());
					}
					else
					{
						Msg += CStr(" -> TERMINATE");
					}
				}

				//Msg += CStrF(", ABlendD %3.3f", pAction->GetAnimBlendDuration());

				const CXRAG_State* pState = m_pAGI->GetState(iState);
				if (pState != NULL && pState->GetNumAnimLayers())
				{
					int32 Numlayers = pState->GetNumAnimLayers();
					for (int32 i = 0; i < Numlayers; i++)
					{
						const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(pState->GetBaseAnimLayerIndex()+i);
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
				/*Msg += "\n";
				M_TRACEALWAYS(Msg.Str());*/
			}
		}
	}
#endif
	{
		const CXRAG_Action* pAction = m_pAGI->GetAction(_iAction);
		if (pAction != NULL)
		{
			int16 iState = pAction->GetTargetStateIndex();
			if (iState != AG_STATEINDEX_NULL)
			{
				m_pAGI->m_pEvaluator->AG_OnEnterState(_pContext, GetID(), iState, _iAction);
				m_pAGI->AddEnterStateEntry(CWAGI_EnterStateEntry(iState, _iAction, GetID()));
			}
		}
	}

	m_RefreshGameTime = _pContext->m_GameTime;
	OnEnterState(_pContext, _iAction, _ForceOffset);
}

//--------------------------------------------------------------------------------

void CWAGI_Token::LeaveState(const CWAGI_Context* _pContext, int16 _iAction)
{
	MSCOPESHORT(CWAGI_Token::LeaveState);

#ifdef	AG_DEBUG
	if (bDebug)
	{
		CStr Msg;
		
		if (_pContext->m_pWPhysState->IsServer())
			Msg += CStrF("(Server %X, pObj %X) ", _pContext->m_pWPhysState, _pContext->m_pObj);
		else
			Msg += CStrF("(Client %X, pObj %X) ", _pContext->m_pWPhysState, _pContext->m_pObj);

		Msg += CStrF("LeaveState: TokenID %d, GameTime %3.2f, iAction %d", m_ID, _pContext->m_GameTime, _iAction);
	
		ConOutL(Msg);
	}
#endif
	OnLeaveState(_pContext, _iAction);
}

//--------------------------------------------------------------------------------

void CWAGI_Token::EvalNodeInit()
{
	const CWAGI_StateInstance* pCachedStateInstance = GetTokenStateInstance();
	const CXRAG_State* pCachedState = m_pAGI->GetState( pCachedStateInstance->GetStateIndex() );
	m_CachedBaseNodeIndex = pCachedState->GetBaseNodeIndex();
}

uint16 CWAGI_Token::EvalNode(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, const CXRAG_ConditionNode* _pNode, fp32& _TimeFraction) const
{
	fp32 NewTimeFraction = 0;
	bool ConditionResult = m_pAGI->EvaluateCondition(_pContext, _iStatePropertyParamBase, _pNode, NewTimeFraction);

/*
	if ((_pContext->m_pObj->m_iObject == 10) && (_pContext->m_pWPhysState->IsServer()))
	{
		if (ConditionResult)
			ConOutL(CStrF("ServerPlayer: <P%d O%d %3.3f> = TRUE", _pNode->GetProperty(), _pNode->GetOperator(), _pNode->GetConstant()));
		else
			ConOutL(CStrF("ServerPlayer: <P%d O%d %3.3f> = FALSE", _pNode->GetProperty(), _pNode->GetOperator(), _pNode->GetConstant()));
	}
*/

	fp32 TrueTimeFraction;
	fp32 FalseTimeFraction = _TimeFraction;
	int TrueAction = AG_NODEACTION_FAILPARSE;
	int FalseAction = AG_NODEACTION_FAILPARSE;

	if (ConditionResult)
	{
		TrueTimeFraction = Max(_TimeFraction, NewTimeFraction);
//		TrueAction = EvalNodeAction(_pContext, _iStatePropertyParamBase, _pNode->GetTrueAction(), TrueTimeFraction);

		TrueAction = _pNode->GetTrueAction();

		if (TrueAction != AG_NODEACTION_FAILPARSE)
		{
			if ((TrueAction & AG_NODEACTION_TYPEMASK) != AG_NODEACTION_ENDPARSE)
			{
				int iNode = m_CachedBaseNodeIndex + (TrueAction & AG_NODEACTION_NODEMASK);

				TrueAction = EvalNode(_pContext, _iStatePropertyParamBase, m_pAGI->GetNode(iNode), TrueTimeFraction);
			}
		}
	}

	FalseAction = _pNode->GetFalseAction();

	if (FalseAction != AG_NODEACTION_FAILPARSE)
	{
		if ((FalseAction & AG_NODEACTION_TYPEMASK) != AG_NODEACTION_ENDPARSE)
		{
			int iNode = m_CachedBaseNodeIndex + (FalseAction & AG_NODEACTION_NODEMASK);

			FalseAction = EvalNode(_pContext, _iStatePropertyParamBase, m_pAGI->GetNode(iNode), FalseTimeFraction);
		}
	}

//	FalseAction = EvalNodeAction(_pContext, _iStatePropertyParamBase, _pNode->GetFalseAction(), FalseTimeFraction);

	if (TrueAction != AG_NODEACTION_FAILPARSE)
	{
		if (FalseAction != AG_NODEACTION_FAILPARSE)
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
	else if (FalseAction != AG_NODEACTION_FAILPARSE)
	{
		_TimeFraction = FalseTimeFraction;
		return FalseAction;
	}

	_TimeFraction = 0;
	return AG_NODEACTION_FAILPARSE;
}

//--------------------------------------------------------------------------------

uint16 CWAGI_Token::EvalNodeAction(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, uint16 _NodeAction, fp32& _TimeFraction)
{
//	MSCOPESHORT( CWAGI_Token::EvalNodeAction );

	if (_NodeAction == AG_NODEACTION_FAILPARSE)
		return _NodeAction;

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_ENDPARSE)
		return _NodeAction;

	const CXRAG_State* pState = m_pAGI->GetState(GetStateIndex());
	int iNode = pState->GetBaseNodeIndex() + (_NodeAction & AG_NODEACTION_NODEMASK);
	const CXRAG_ConditionNode* pNode = m_pAGI->GetNode(iNode);

	return EvalNode(_pContext, _iStatePropertyParamBase, pNode, _TimeFraction);
}

//--------------------------------------------------------------------------------

uint32 CWAGI_Token::Refresh(CWAGI_Context* _pContext)
{
	MSCOPESHORT( CWAGI_Token::Refresh );

	CWAGI_StateInstance* pStateInstance = GetTokenStateInstance();
	if (pStateInstance != NULL)
	{
		if (m_RefreshGameTime.IsReset())
			m_RefreshGameTime = GetEnterStateTime();

		if (m_RefreshGameTime.Compare(_pContext->m_GameTime) < 0)
		{
/*
			if ((_pContext->m_GameTime - m_RefreshGameTime) > 0.05f)
				ConOutL(CStrF("CWAGI_Token::Refresh() - Token %d, Forcing Context GameTime from %3.3f to %3.3f", m_ID, _pContext->m_GameTime, m_RefreshGameTime));
*/
		//	if (_pContext->m_pWPhysState->IsServer()) -- enable locally, don't check in!
			{
				// If the animgraph has been disabled for a while, don't worry about last refresh
				if (!((_pContext->m_GameTime - m_RefreshGameTime).GetTime() > 0.15f))
					_pContext->StartAtTime(m_RefreshGameTime, _pContext);
			}
		}
	}

	// FIXME: Should this be called here or in the end of this function?!
	// It might be good to outdate old stateinstances before GetTokenStateInstance is called.
	RefreshQueue(_pContext);

	pStateInstance = GetTokenStateInstance();
	if (pStateInstance == NULL)
		return AGI_TOKENREFRESHFLAGS_FAILED;

	int16 iState = pStateInstance->GetStateIndex();
	if (iState == AG_STATEINDEX_NULL)
		return AGI_TOKENREFRESHFLAGS_FAILED;

	const CXRAG_State* pState = m_pAGI->GetState(iState);
	if (pState == NULL)
		return AGI_TOKENREFRESHFLAGS_FAILED;

/*
	if (CWAGI::DebugEnabled(_pContext))
	{
		CStr StateName;
		if (iState != AG_STATEINDEX_NULL)
		{
			if (iState != AG_STATEINDEX_TERMINATE)
			{
				iState &= AG_TARGETSTATE_INDEXMASK;
				StateName = m_pAGI->GetStateName(iState);
				if (StateName != "")
					StateName = CStrF("State%d (%s)", iState, StateName.Str());
				else
					StateName = CStrF("State%d (N/A)", iState);
			}
			else
			{
				StateName = CStr("TERMINATE");
			}
		}
		if (_pContext->m_pWPhysState->IsServer())
			ConOutL(CStrF("(SERVER, GameTime %3.3f) Token%d is in %s", _pContext->m_GameTime, GetID(), StateName.Str()));
		else
			ConOutL(CStrF("(CLIENT, GameTime %3.3f) Token%d is in %s", _pContext->m_GameTime, GetID(), StateName.Str()));
	}
*/

	const CXRAG_ConditionNode* pNode = m_pAGI->GetNode(pState->GetBaseNodeIndex());
	if (pNode == NULL)
		return AGI_TOKENREFRESHFLAGS_FAILED;

	M_ASSERT(m_pAGI->m_pEvaluator, "!");
	m_pAGI->m_pEvaluator->AG_RefreshStateInstanceProperties(_pContext, pStateInstance);

	int16 iStatePropertyParamBase = pState->GetBasePropertyParamIndex();

	CMTime StartGameTime = _pContext->m_GameTime;
	CMTime EndGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);
	m_RefreshGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);

	fp32 TimeFraction = 0;
	uint16 NodeAction;
	{
		MSCOPESHORT( CWAGI_Token::EvalNode );
		EvalNodeInit();
		NodeAction = EvalNode(_pContext, iStatePropertyParamBase, pNode, TimeFraction);
	}
	if (NodeAction != AG_NODEACTION_FAILPARSE)
	{
		int iAction = pState->GetBaseActionIndex() + (NodeAction & AG_NODEACTION_ACTIONMASK);
		const CXRAG_Action* pAction = m_pAGI->GetAction(iAction);
		if (pAction == NULL)
			return AGI_TOKENREFRESHFLAGS_FAILED;
/*
		int iExportedActionName = iAction; // FIXME: This 1/1 mapping is NOT garanteed.
		CStr ExportedActionName = m_pAGI->GetExportedActionName(iExportedActionName);
		if (CWAGI::DebugEnabled(_pContext))
		{
			if (_pContext->m_pWPhysState->IsServer())
				ConOutL(CStrF("(SERVER) Token::Refresh() - Action%d (%s)", iAction, ExportedActionName.Str()));
			else
				ConOutL(CStrF("(CLIENT) Token::Refresh() - Action%d (%s)", iAction, ExportedActionName.Str()));
		}
*/
		int8 ActionTokenID = pAction->GetTokenID();
		if (ActionTokenID == AG_TOKENID_NULL)
			ActionTokenID = GetID();

		int16 iTargetState = pAction->GetTargetStateIndex();
		bool bMoveAction = (iTargetState != AG_STATEINDEX_NULL);
		bool bOtherToken = (ActionTokenID != GetID());
		bool bSameState = !bMoveAction || bOtherToken;
		bool bIncludeStartTime = !bSameState;

		_pContext->SplitAtFraction(TimeFraction, bIncludeStartTime, _pContext);
		m_RefreshGameTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(_pContext->m_TimeSpan);

		// Hmm, ok so the token might have been moved in this
		CWAGI* pAGI = m_pAGI;
		int32 iTokenID = GetID();
		pAGI->InvokeEffects(_pContext, pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances());

		// If the token moved is earlier in the queue, force refresh
		if (ActionTokenID < iTokenID)
			pAGI->ForceRefresh();

		if (bMoveAction)
			pAGI->MoveAction(_pContext, ActionTokenID, iAction);

		// WARNING: Don't make ANY references to this token anymore in this function, since it might have been reallocated.

		bool bRefresh = ((bMoveAction || (TimeFraction > 0.01f)) && (_pContext->m_GameTime.Compare((EndGameTime - CMTime::CreateFromSeconds(0.01f))) < 0));
		if (bRefresh)
			return (AGI_TOKENREFRESHFLAGS_PERFORMEDACTION | AGI_TOKENREFRESHFLAGS_REFRESH);
		else
			return AGI_TOKENREFRESHFLAGS_PERFORMEDACTION;
	}

	return AGI_TOKENREFRESHFLAGS_FINISHED;
}

//--------------------------------------------------------------------------------

#if	defined(M_RTM) && defined( AG_DEBUG )
#warning "M_RTM and AG_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
