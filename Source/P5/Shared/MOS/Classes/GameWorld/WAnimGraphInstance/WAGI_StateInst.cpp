#include "PCH.h"

//--------------------------------------------------------------------------------

#ifdef	AG_DEBUG
static bool bDebug = false;
#endif
//--------------------------------------------------------------------------------

#define DEFAULT_INVBLENDINDURATION	(0.0f)
#define DEFAULT_INVBLENDOUTDURATION	(0.0f)

#define CHAR_STATEFLAG_ATTACHNOREL	0x00000020
#define CHAR_STATEFLAG_SYNCHACK		0x00001000
#define CHAR_STATEFLAG_WALLCOLLISION 0x00010000
#define CHAR_STATEFLAG_FORCELAYERVELOCITY 0x00020000
#define CHAR_STATEFLAG_LAYERNOVELOCITY 0x00080000
#define CHAR_STATEFLAG_LAYERADDITIVEBLEND 0x00800000
#define CHAR_STATEFLAG_LAYERADJUSTFOROFFSET	0x01000000
#define CHAR_STATEFLAG_TAGUSEANIMCAMERA 0x04000000
#define CHAR_STATEFLAG_SKIPFORCEKEEP 0x00004000

//--------------------------------------------------------------------------------

#include "WAGI_StateInst.h"
#include "WAGI.h"
#include "WAGI_Context.h"
#include "WAGI_StateInstPacked.h"
#include "WAG_ClientData.h"

//--------------------------------------------------------------------------------

CWAGI_StateInstance::CWAGI_StateInstance(const CWAGI_SIID* _pSIID, CWAGI* _pAGI, CWAGI_SIQ* _pSIQ)
{
	// Misc
	m_pAGI = _pAGI;
	m_pSIQ = _pSIQ;
	m_Flags = 0;
	m_EnqueueTime = _pSIID->GetEnqueueTime();
	m_iState = AG_STATEINDEX_NULL;
	m_Priority = 0;

	// Enter
	m_iEnterAction = _pSIID->GetEnterActionIndex();
	m_EnterTime = AGI_UNDEFINEDTIME;

	m_Enter_AnimBlendDuration = 0;
	m_Enter_AnimBlendDelay = 0;
	m_Enter_AnimTimeOffset = 0;
	m_Enter_iAnimTimeOffsetType = AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT;

	m_iLoopControlAnimLayer = 0;
	m_AnimLoopDuration = 0;
	m_TimeScale = 1.0f;
	m_bHasAnimation = false;
	m_bSkipForceKeep = 0;

	m_BlendInStartTime = AGI_UNDEFINEDTIME;
	m_BlendInEndTime = AGI_UNDEFINEDTIME;
	m_InvBlendInDuration = DEFAULT_INVBLENDINDURATION;

	// Leave
	m_iLeaveAction = AG_ACTIONINDEX_NULL;
	m_LeaveTime = AGI_UNDEFINEDTIME;

	m_Leave_AnimBlendDuration = 0;
	m_Leave_AnimBlendDelay = 0;

	m_BlendOutStartTime = AGI_UNDEFINEDTIME;
	m_BlendOutEndTime = AGI_UNDEFINEDTIME;
	m_InvBlendOutDuration = DEFAULT_INVBLENDOUTDURATION;

	// Proceed Condition
	m_ProceedCondition_bDefault = true;
	m_ProceedCondition_iProperty = 0;
	m_ProceedCondition_iOperator = 0;
	m_ProceedCondition_Constant = 0;
}

//--------------------------------------------------------------------------------

fp32 CWAGI_StateInstance::GetAnimLoopDuration(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_StateInstance::GetAnimLoopDuration);

	CXRAG_AnimList* pAnimList = m_pAGI->GetAnimList();
	if (pAnimList == NULL)
		return 0.0f;

	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL || !pState->GetNumAnimLayers())
		return 0.0f;

	const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(pState->GetBaseAnimLayerIndex() + m_iLoopControlAnimLayer);
	if (pAnimLayer == NULL)
		return 0.0f;

	CWorldData* pWData;
	if(_pContext->m_pWPhysState)
		pWData = _pContext->m_pWPhysState->GetWorldData();
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pWData = _pContext->m_pMapData->m_spWData;
	}

	spCXR_Anim_SequenceData pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, pAnimLayer->GetAnimIndex());
	if (pAnimLayerSeq == NULL)
		return 0;

	return pAnimLayerSeq->GetDuration() / pAnimLayer->GetTimeScale();
}

fp32 CWAGI_StateInstance::GetTimeScale(const CWAGI_Context* _pContext) const
{
	MSCOPESHORT(CWAGI_StateInstance::GetTimeScale);

	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL || !pState->GetNumAnimLayers())
		return 0.0f;

	const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(pState->GetBaseAnimLayerIndex() + m_iLoopControlAnimLayer);
	if (pAnimLayer == NULL)
		return 0.0f;


	return(pAnimLayer->GetTimeScale());
}

//--------------------------------------------------------------------------------
/*
fp32 CWAGI_StateInstance::GetAnimTime(CWAGI_Context* _pContext, int8 _iAnimLayer) const
{
	const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(_iAnimLayer);
	if (pAnimLayer == NULL)
		return 0;

	spCXR_Anim_SequenceData pAnimLayerSeq = pAnimList->GetAnimSequenceData(pAnimLayer->GetAnimIndex());
	if (pAnimLayerSeq == NULL)
		return 0;

	m_pAGI->RefreshStateInstanceProperties(_pContext, this);

	fp32 AnimTime;
	AnimTime = m_pAGI->EvaluateProperty(_pContext, pAnimLayer->GetTimeControlProperty());
	AnimTime = (AnimTime + pAnimLayer->GetTimeOffset() + m_Enter_AnimTimeOffset) * pAnimLayer->GetTimeScale();
	AnimTime = pAnimLayerSeq->GetLoopedTime(AnimTime);

	return AnimTime;
}
*/
//--------------------------------------------------------------------------------
/*
fp32 CWAGI_StateInstance::GetAnimTimeOffset(CWAGI_Context* _pContext, int8 _iAnimTimeOffsetType)
{
	if (_iAnimTimeOffsetType == AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT)
		return 0;

	fp32 AnimTime = GetAnimTime(_pContext);

	if (_iAnimTimeOffsetType == AG_ANIMLAYER_TIMEOFFSETTYPE_INHERIT)
		return AnimTime;

	if (_iAnimTimeOffsetType == AG_ANIMLAYER_TIMEOFFSETTYPE_INHERITEQ)
		return (AnimTime / m_AnimLoopDuration);

	return 0;
}
*/
//--------------------------------------------------------------------------------
/*
void CWAGI_StateInstance::AdjustAnimTimeOffset(CWAGI_Context* _pContext)
{
	if ((m_EnterTime == AGI_UNDEFINEDTIME) || (m_Enter_iAnimTimeOffsetType == AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT))
		return;

	CWAGI_SIID SIID(GetEnqueueTime(), GetEnterActionIndex());
	fp32 InheritedAnimTimeOffset = m_pSIQ->GetPrevStateInstanceAnimTimeOffset(_pContext, &SIID, m_Enter_iAnimTimeOffsetType);

	switch (m_Enter_iAnimTimeOffsetType)
	{
		case AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT: InheritedAnimTimeOffset = 0; break;
		case AG_ANIMLAYER_TIMEOFFSETTYPE_INHERIT: break;
		case AG_ANIMLAYER_TIMEOFFSETTYPE_INHERITEQ: InheritedAnimTimeOffset *= m_AnimLoopDuration; break;
	}

	m_Enter_AnimTimeOffset += InheritedAnimTimeOffset;
}
*/
//--------------------------------------------------------------------------------

bool CWAGI_StateInstance::ProceedQuery(const CWAGI_Context* _pContext, fp32& _TimeFraction) const
{
	MSCOPESHORT(CWAGI_StateInstance::ProceedQuery);
	
	// If no custom condition is specified, use default condition (i.e. have token left this state already?).
	if (m_ProceedCondition_bDefault)
	{
		_TimeFraction = 0;
		return (m_iLeaveAction != AG_ACTIONINDEX_NULL);
	}

	M_ASSERT(m_pAGI->m_pEvaluator, "!");
	return m_pAGI->m_pEvaluator->AG_EvaluateCondition(_pContext, 
													m_ProceedCondition_iProperty, NULL,
													m_ProceedCondition_iOperator,
													CAGVal::From(m_ProceedCondition_Constant),
													_TimeFraction);
}

//--------------------------------------------------------------------------------

bool CWAGI_StateInstance::EnterState_Setup(const CWAGI_Context* _pContext, int16 _iEnterAction)
{
	MSCOPESHORT(CWAGI_StateInstance::EnterState_Setup);

	m_EnqueueTime = _pContext->m_GameTime;	

	m_iEnterAction = _iEnterAction;
	const CXRAG_Action* pAction = m_pAGI->GetAction(m_iEnterAction);
	if (pAction == NULL)
		return false;

	m_Enter_AnimBlendDuration = pAction->GetAnimBlendDuration();
	m_Enter_AnimBlendDelay = pAction->GetAnimBlendDelay();
	m_Enter_AnimTimeOffset = pAction->GetAnimTimeOffset();
	m_Enter_iAnimTimeOffsetType = pAction->GetAnimTimeOffsetType();
	m_iState = pAction->GetTargetStateIndex() & AG_TARGETSTATE_INDEXMASK;
	
	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL)
		return false;

	m_iLoopControlAnimLayer = pState->GetLoopControlAnimLayerIndex();
	m_AnimLoopDuration = GetAnimLoopDuration(_pContext);
	m_TimeScale = GetTimeScale(_pContext);

	m_bHasAnimation = false;
	m_bSkipForceKeep = pState->GetFlags(1) & CHAR_STATEFLAG_SKIPFORCEKEEP ? 1 : 0;
	/*if (m_iState == 1726)
		ConOut(CStrF("ForceKeep: %d",m_bSkipForceKeep));*/
	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		if (iAnim != AG_ANIMINDEX_NULL)
		{
			m_bHasAnimation = true;
			break;
		}
	}

	m_Priority = pState->GetPriority();

	m_ProceedCondition_iProperty = pState->GetAQPCProperty();
	m_ProceedCondition_iOperator = pState->GetAQPCOperator();
	m_ProceedCondition_Constant = pState->GetAQPCConstant();

	m_EnterTime = AGI_UNDEFINEDTIME;
	m_BlendInStartTime = AGI_UNDEFINEDTIME;
	m_BlendInEndTime = AGI_UNDEFINEDTIME;
	m_InvBlendInDuration = DEFAULT_INVBLENDINDURATION;

	return true;
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::EnterState_Initiate(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_StateInstance::EnterState_Initiate);

	m_EnterTime = _pContext->m_GameTime;
	m_BlendInStartTime = m_EnterTime + CMTime::CreateFromSeconds(m_Enter_AnimBlendDelay);

	if (m_Enter_AnimBlendDuration < 0.0f)
	{
		m_BlendInEndTime = AGI_LINKEDTIME;
		m_InvBlendInDuration = m_Enter_AnimBlendDuration;
	}
	else
	{
		m_BlendInEndTime = m_EnterTime + CMTime::CreateFromSeconds(m_Enter_AnimBlendDelay +  m_Enter_AnimBlendDuration);
		m_InvBlendInDuration = (m_Enter_AnimBlendDuration > 0) ? (1.0f / m_Enter_AnimBlendDuration) : DEFAULT_INVBLENDINDURATION;
	}
	//AdjustAnimTimeOffset(_pContext);
}

//--------------------------------------------------------------------------------

bool CWAGI_StateInstance::LeaveState_Setup(const CWAGI_Context* _pContext, int16 _iLeaveAction)
{
	MSCOPESHORT(CWAGI_StateInstance::LeaveState_Setup);

	m_iLeaveAction = _iLeaveAction;
	const CXRAG_Action* pAction = m_pAGI->GetAction(m_iLeaveAction);
	if (pAction == NULL)
		return false;

	m_Leave_AnimBlendDuration = pAction->GetAnimBlendDuration();
	m_Leave_AnimBlendDelay = pAction->GetAnimBlendDelay();

	m_LeaveTime = AGI_UNDEFINEDTIME;
	m_BlendOutStartTime = AGI_UNDEFINEDTIME;
	m_BlendOutEndTime = AGI_UNDEFINEDTIME;
	m_InvBlendOutDuration = DEFAULT_INVBLENDOUTDURATION;

	// FIXME: Add support for only blending this stateinstance out if no succeeding stateinstance exists.
	// This is only the case when the token is terminated and no succeeding state will be reached.
	// I think setting m_InvBlendOutDuration = 0.0f will do...
	// Mayby it can be signaled by an AnimBlendDuration = -1.0f?

	return true;
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::LeaveState_Initiate(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_StateInstance::LeaveState_Initiate);

	m_LeaveTime = _pContext->m_GameTime;
	m_BlendOutStartTime = m_LeaveTime + CMTime::CreateFromSeconds(m_Leave_AnimBlendDelay);

	/*if (m_iState == 1726)
		ConOutL(CStrF("State: %d %s LeaveTime: %f GT: %f SkipForceKeep: %d", m_iState, _pContext->m_pWPhysState->IsServer() ? "S:":"C:",m_BlendOutStartTime.GetTime(),_pContext->m_pWPhysState->GetGameTime().GetTime(),m_bSkipForceKeep));*/

	if (m_Leave_AnimBlendDuration < 0.0f)
	{
		m_BlendOutEndTime = AGI_LINKEDTIME;
		m_InvBlendOutDuration = m_Leave_AnimBlendDuration;
	}
	else
	{
		m_BlendOutEndTime = m_LeaveTime +  CMTime::CreateFromSeconds(m_Leave_AnimBlendDelay +  m_Leave_AnimBlendDuration);
		m_InvBlendOutDuration = (m_Leave_AnimBlendDuration > 0) ? (1.0f / m_Leave_AnimBlendDuration) : DEFAULT_INVBLENDOUTDURATION;
	}
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::PredictionMiss_Add(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP)
{
	MSCOPESHORT(CWAGI_StateInstance::PredictionMiss_Add);

	m_Flags |= STATEINSTANCEFLAGS_PREDMISS_ADD;

	// FIXME: Implement smooth prediction miss fading...
	UnpackSIP(_pContext, _pSIP);
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::PredictionMiss_Remove(const CWAGI_Context* _pContext)
{
	MSCOPESHORT(CWAGI_StateInstance::PredictionMiss_Remove);

	m_Flags |= STATEINSTANCEFLAGS_PREDMISS_REMOVE;

	fp32 FadePredictionMiss_AnimBlendDuration = 0.0f;

	if (m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME)) // See if stateinstance has been reached at all.
	{ // StateInstance not yet reached. Set BlendOutEndTime to current gametime (so it will be removed).
		m_EnterTime = _pContext->m_GameTime;
		m_BlendInStartTime = m_EnterTime;
		m_BlendInEndTime = m_EnterTime;
		m_InvBlendInDuration = DEFAULT_INVBLENDINDURATION;
		m_LeaveTime = _pContext->m_GameTime;
		m_BlendOutStartTime = m_LeaveTime;
		m_BlendOutEndTime = m_LeaveTime;
		m_InvBlendOutDuration = DEFAULT_INVBLENDOUTDURATION;
		return;
	}
	else if (m_LeaveTime.AlmostEqual(AGI_UNDEFINEDTIME)) // See if there already is a current blendout initiated.
	{ // No blendout initiated.
		m_LeaveTime = _pContext->m_GameTime;
		m_BlendOutStartTime = m_LeaveTime;
		m_BlendOutEndTime = m_LeaveTime + CMTime::CreateFromSeconds(FadePredictionMiss_AnimBlendDuration);
		m_InvBlendOutDuration = (FadePredictionMiss_AnimBlendDuration > 0) ? (1.0f / FadePredictionMiss_AnimBlendDuration) : DEFAULT_INVBLENDOUTDURATION;
		return;
	}
	else
	{ // Blendout already initiated.
		// See if current blendout is slower than a faked one would be.
		if ((_pContext->m_GameTime + CMTime::CreateFromSeconds(FadePredictionMiss_AnimBlendDuration)).Compare(m_BlendOutEndTime) < 0)
		{
			// Find how much headstart current blendout has (i.e. how far current blendout has gone).
			fp32 CurrentBlend = Clamp01((_pContext->m_GameTime - m_BlendOutStartTime).GetTime() * m_InvBlendOutDuration);

			// Find when new blendout duration would end, given the current headstart.
			m_LeaveTime = _pContext->m_GameTime - CMTime::CreateFromSeconds(FadePredictionMiss_AnimBlendDuration * (1.0f - CurrentBlend));

			m_BlendOutStartTime = m_LeaveTime;

			m_BlendOutEndTime = m_LeaveTime + CMTime::CreateFromSeconds(FadePredictionMiss_AnimBlendDuration);
			m_InvBlendOutDuration = (FadePredictionMiss_AnimBlendDuration > 0) ? (1.0f / FadePredictionMiss_AnimBlendDuration) : DEFAULT_INVBLENDOUTDURATION;
		}
	}
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::SyncWithSIP(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP)
{
	MSCOPESHORT(CWAGI_StateInstance::SyncWithSIP);

	m_Flags |= STATEINSTANCEFLAGS_PREDMISS_UPDATE;
/*
	if (m_EnqueueTime != _pSIP->GetEnqueueTime())
		ConOutL(CStrF("CWAGI_StateInstance::SyncWithSIP() - EnqueueTime %3.3f differs from SIP %3.3f", m_EnqueueTime, _pSIP->GetEnqueueTime()));

//	m_iEnterAction = _pSIP->GetEnterActionIndex();

	if (m_EnterTime != _pSIP->GetEnterTime())
		ConOutL(CStrF("CWAGI_StateInstance::SyncWithSIP() - EnterTime %3.3f differs from SIP %3.3f", m_EnterTime, _pSIP->GetEnterTime()));

//	m_iLeaveAction = _pSIP->GetLeaveActionIndex();

	if (m_LeaveTime != _pSIP->GetLeaveTime())
		ConOutL(CStrF("CWAGI_StateInstance::SyncWithSIP() - LeaveTime %3.3f differs from SIP %3.3f", m_LeaveTime, _pSIP->GetLeaveTime()));

//	m_BlendOutEndTime = _pSIP->GetBlendOutEndTime();
*/
	// FIXME: Implement smooth prediction miss fading...
	UnpackSIP(_pContext, _pSIP);
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::PackSIP(CWAGI_SIP* _pSIP) const
{
	MSCOPESHORT(CWAGI_StateInstance::PackSIP);

	_pSIP->SetEnqueueTime(m_EnqueueTime);
	_pSIP->SetEnterActionIndex(m_iEnterAction);
	_pSIP->SetEnterTime(m_EnterTime);
	_pSIP->SetLeaveActionIndex(m_iLeaveAction);
	_pSIP->SetLeaveTime(m_LeaveTime);
	_pSIP->SetBlendOutEndTime(m_BlendOutEndTime);
	_pSIP->SetAnimTimeOffset(m_Enter_AnimTimeOffset);
}

//--------------------------------------------------------------------------------

// Brute Force overwrite/replace everything...
void CWAGI_StateInstance::UnpackSIP(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP)
{
	MSCOPESHORT(CWAGI_StateInstance::UnpackSIP);

	m_EnqueueTime = _pSIP->GetEnqueueTime();
	m_iEnterAction = _pSIP->GetEnterActionIndex();
	m_EnterTime = _pSIP->GetEnterTime();
	m_iLeaveAction = _pSIP->GetLeaveActionIndex();
	m_LeaveTime = _pSIP->GetLeaveTime();
	m_BlendOutEndTime = _pSIP->GetBlendOutEndTime();
	m_Enter_AnimTimeOffset = _pSIP->GetAnimTimeOffset();

	const CXRAG_Action* pEnterAction = m_pAGI->GetAction(m_iEnterAction);
	if (pEnterAction != NULL)
	{
		m_Enter_AnimBlendDuration = pEnterAction->GetAnimBlendDuration();
		m_Enter_AnimBlendDelay = pEnterAction->GetAnimBlendDelay();
		m_Enter_iAnimTimeOffsetType = pEnterAction->GetAnimTimeOffsetType();
		m_iState = pEnterAction->GetTargetStateIndex() & AG_TARGETSTATE_INDEXMASK;
			
		const CXRAG_State* pState = m_pAGI->GetState(m_iState);
		if (pState != NULL)
		{
			m_iLoopControlAnimLayer = pState->GetLoopControlAnimLayerIndex();
			m_AnimLoopDuration = GetAnimLoopDuration(_pContext);

			m_bHasAnimation = false;
			m_bSkipForceKeep = pState->GetFlags(1) & CHAR_STATEFLAG_SKIPFORCEKEEP ? 1 : 0;
			/*if (m_iState == 1726)
				ConOut(CStrF("ForceKeep: %d",m_bSkipForceKeep));*/
			for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
			{
				int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

				const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(iAnimLayer);
				if (pAnimLayer == NULL)
					continue;

				int16 iAnim = pAnimLayer->GetAnimIndex();
				if (iAnim != AG_ANIMINDEX_NULL)
				{
					m_bHasAnimation = true;
					break;
				}
			}

			m_Priority = pState->GetPriority();
			m_ProceedCondition_iProperty = pState->GetAQPCProperty();
			m_ProceedCondition_iOperator = pState->GetAQPCOperator();
			m_ProceedCondition_Constant = pState->GetAQPCConstant();
		}

		if (m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME) == false)
		{
			m_BlendInStartTime = m_EnterTime + CMTime::CreateFromSeconds(m_Enter_AnimBlendDelay);

			if (m_Enter_AnimBlendDuration < 0.0f)
			{
				m_BlendInEndTime = AGI_LINKEDTIME;
				m_InvBlendInDuration = m_Enter_AnimBlendDuration;
			}
			else
			{
				m_BlendInEndTime = m_EnterTime + CMTime::CreateFromSeconds(m_Enter_AnimBlendDelay +  m_Enter_AnimBlendDuration);
				m_InvBlendInDuration = (m_Enter_AnimBlendDuration > 0) ? (1.0f / m_Enter_AnimBlendDuration) : DEFAULT_INVBLENDINDURATION;
			}
			//AdjustAnimTimeOffset(_pContext);
		}
		else
		{
			m_BlendInStartTime = AGI_UNDEFINEDTIME;
			m_BlendInEndTime = AGI_UNDEFINEDTIME;
			m_InvBlendInDuration = DEFAULT_INVBLENDINDURATION;
		}
	}

	const CXRAG_Action* pLeaveAction = m_pAGI->GetAction(m_iLeaveAction);
	if (pLeaveAction != NULL)
	{
		m_Leave_AnimBlendDuration = pLeaveAction->GetAnimBlendDuration();
		m_Leave_AnimBlendDelay = pLeaveAction->GetAnimBlendDelay();

		if (m_LeaveTime.AlmostEqual(AGI_UNDEFINEDTIME) == false)
		{
			m_BlendOutStartTime = m_LeaveTime + CMTime::CreateFromSeconds(m_Leave_AnimBlendDelay);

			if (m_Leave_AnimBlendDuration < 0.0f)
			{
				m_BlendOutEndTime = AGI_LINKEDTIME;
				m_InvBlendOutDuration = m_Leave_AnimBlendDuration;
			}
			else
			{
				m_BlendOutEndTime = m_LeaveTime + CMTime::CreateFromSeconds(m_Leave_AnimBlendDelay +  m_Leave_AnimBlendDuration);
				m_InvBlendOutDuration = (m_Leave_AnimBlendDuration > 0) ? (1.0f / m_Leave_AnimBlendDuration) : DEFAULT_INVBLENDOUTDURATION;
			}
		}
		else
		{
			m_BlendOutStartTime = AGI_UNDEFINEDTIME;
			m_BlendOutEndTime = AGI_UNDEFINEDTIME;
			m_InvBlendOutDuration = DEFAULT_INVBLENDOUTDURATION;
		}
	}
}

//--------------------------------------------------------------------------------

void CWAGI_StateInstance::GetAnimLayers(const CWAGI_Context* _pContext, bool _bBlendOut, CXR_AnimLayer* _pLayers, int& _nLayers, int _iSI)
{
	MSCOPE(CWAGI_StateInstance::GetAnimLayers, CWAGI);

	if (_nLayers < 1)
		return;

	int nMaxLayers = _nLayers;
	_nLayers = 0;

	if (!m_bHasAnimation)
		return;

	CXRAG_AnimList* pAnimList = m_pAGI->GetAnimList();
	if (pAnimList == NULL)
		return;

	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL)
		return;

	if (m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME))
		return;

	fp32 Blend, BlendIn, BlendOut;

	if (!_bBlendOut || m_LeaveTime.AlmostEqual(AGI_UNDEFINEDTIME))
	{
			if (m_InvBlendInDuration < 0.0f)
				BlendIn = m_InvBlendInDuration;
			else if (m_InvBlendInDuration > 0)
				BlendIn = Clamp01((_pContext->m_GameTime - m_BlendInStartTime).GetTime() * m_InvBlendInDuration);
			else
				BlendIn = 1.0f;

			BlendOut = 1.0f;

		//ConOutL(CStrF("BlendIn = %3.3f", Blend));
	}
	else
	{
		if (m_InvBlendInDuration < 0.0f)
			BlendIn = m_InvBlendInDuration;
		else if (m_InvBlendInDuration > 0)
			BlendIn = Clamp01((_pContext->m_GameTime - m_BlendInStartTime).GetTime() * m_InvBlendInDuration);
		else
			BlendIn = 1.0f;

		if (m_InvBlendOutDuration < 0.0f)
			BlendOut = m_InvBlendOutDuration;
		else
			BlendOut = 1.0f - Clamp01((_pContext->m_GameTime - m_BlendOutStartTime).GetTime() * m_InvBlendOutDuration);

		//ConOutL(CStrF("BlendIn = %3.3f, BlendOut = %3.3f, Blend = %3.3f", BlendIn, BlendOut, Blend));
	}

	if ((BlendIn < 0.0f) && (BlendOut < 0.0f))
		Blend = -3.0f;
	else
		Blend = BlendIn * BlendOut; // Smoother than Min(BlendIn, BlendOut)

	if (Blend == 0)
		return;

	CWorldData* pWData;
	if(_pContext->m_pWPhysState)
		pWData = _pContext->m_pWPhysState->GetWorldData();
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pWData = _pContext->m_pMapData->m_spWData;
	}

	int nLayers = 0;
	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		if (nLayers >= nMaxLayers)
			break;

		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		fp32 LayerBlend = Blend * pAnimLayer->GetOpacity();

		if (LayerBlend == 0.0f)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
//		ConOutL(CStrF("iAnim %d", iAnim));

		spCXR_Anim_SequenceData pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, iAnim);
		if (pAnimLayerSeq == NULL)
			continue;

		M_ASSERT(m_pAGI->m_pEvaluator, "!");
		m_pAGI->m_pEvaluator->AG_RefreshStateInstanceProperties(_pContext, this);

		int16 iTimeControlProperty = pAnimLayer->GetTimeControlProperty();
		CMTime ContinousTime = m_pAGI->m_pEvaluator->AG_EvaluateProperty(_pContext, iTimeControlProperty, NULL).GetTime();
		ContinousTime = (ContinousTime + CMTime::CreateFromSeconds(pAnimLayer->GetTimeOffset() + m_Enter_AnimTimeOffset)).Scale(pAnimLayer->GetTimeScale());
		CMTime LoopedTime = pAnimLayerSeq->GetLoopedTime(ContinousTime);

#ifdef	AG_DEBUG
		bool bLocalDebug = false;
		if (bLocalDebug)
			ConOutL(CStrF("AnimLayer: pObj %X, StateInstance %d, GameTime %3.3f, iAnim %d, AnimAnchorTime %3.3f, LoopedTime %3.3f, Blend %3.2f, BI %3.3f - %3.3f, BO %3.3f - %3.3f",
						_pContext->m_pObj, _iSI, _pContext->m_GameTime, pAnimLayer->GetAnimIndex(), m_EnterTime.GetTime(), LoopedTime.GetTime(), LayerBlend,
						m_BlendInStartTime, m_BlendInEndTime.GetTime(), m_BlendOutStartTime.GetTime(), m_BlendOutEndTime.GetTime()));
#endif
		uint32 Flags = 0;
		uint32 StateFlags = pState->GetFlags(0);
		if (StateFlags & CHAR_STATEFLAG_ATTACHNOREL)
			Flags |= CXR_ANIMLAYER_ATTACHNOREL;
		if (StateFlags & CHAR_STATEFLAG_SYNCHACK)
			Flags |= CXR_ANIMLAYER_TAGSYNC;
		if (StateFlags & CHAR_STATEFLAG_WALLCOLLISION)
			Flags |= CXR_ANIMLAYER_TAGWALLCOLLISION;
		if (StateFlags & CHAR_STATEFLAG_FORCELAYERVELOCITY)
			Flags |= CXR_ANIMLAYER_FORCELAYERVELOCITY;
		if (StateFlags & CHAR_STATEFLAG_LAYERNOVELOCITY)
			Flags |= CXR_ANIMLAYER_LAYERNOVELOCITY;
		if (StateFlags & CHAR_STATEFLAG_LAYERADDITIVEBLEND)
			Flags |= CXR_ANIMLAYER_ADDITIVEBLEND;
		if (StateFlags & CHAR_STATEFLAG_TAGUSEANIMCAMERA)
			Flags |= CXR_ANIMLAYER_TAGUSEANIMCAMERA;

		Flags |= pState->GetFlags(1) & CXR_ANIMLAYER_FORCESUBSEQUENTTO8;

		_pLayers[nLayers].Create3(pAnimLayerSeq, LoopedTime, pAnimLayer->GetTimeScale(), LayerBlend, pAnimLayer->GetBaseJointIndex(), Flags);
		_pLayers[nLayers].m_BlendIn = BlendIn;
		_pLayers[nLayers].m_BlendOut = BlendOut;

		if (StateFlags & CHAR_STATEFLAG_LAYERADJUSTFOROFFSET)
			_pLayers[nLayers].m_TimeOffset = m_Enter_AnimTimeOffset;

#ifdef AG_DEBUG
		_pLayers[nLayers].m_DebugMessage = CStrF("iState %d, iAnim %d, iBaseJoint %d, iStateInstance %d, GameTime %3.3f, AnimAnchorTime %3.3f, LoopedTime %3.3f, Blend %3.2f, BI %3.3f - %3.3f, BO %3.3f - %3.3f",
			m_iState, pAnimLayer->GetAnimIndex(), pAnimLayer->GetBaseJointIndex(), _iSI, _pContext->m_GameTime, m_EnterTime, LoopedTime, LayerBlend,
			m_BlendInStartTime, m_BlendInEndTime, m_BlendOutStartTime, m_BlendOutEndTime);
#endif
		_pLayers[nLayers++].m_ContinousTime = ContinousTime;

//		_pLayers[nLayers++].Create3(pAnimLayerSeq, AnimTime, pAnimLayer->GetTimeScale(), pAnimLayer->GetMergeOperator(), LayerBlend, pAnimLayer->GetBaseJointIndex(), Flags);
	}

	_nLayers = nLayers;
}

bool CWAGI_StateInstance::GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _Layer, int _iSI, int32 _iAnim, int32 _StartTick) const
{
	MSCOPE(CWAGI_StateInstance::GetAnimLayers, CWAGI);

	CMTime StartTick = PHYSSTATE_TICKS_TO_TIME(_StartTick-1, _pContext->m_pWPhysState);
	if (!m_bHasAnimation || m_EnterTime.Compare(StartTick) < 0)
		return false;

	CXRAG_AnimList* pAnimList = m_pAGI->GetAnimList();
	if (pAnimList == NULL)
		return false;

	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL)
		return false;

	if (m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME))
		return false;

	fp32 Blend, BlendIn, BlendOut;

	{
		if (m_LeaveTime.AlmostEqual(AGI_UNDEFINEDTIME))
		{
			if (m_InvBlendInDuration < 0.0f)
				BlendIn = m_InvBlendInDuration;
			else if (m_InvBlendInDuration > 0)
				BlendIn = Clamp01((_pContext->m_GameTime - m_BlendInStartTime).GetTime() * m_InvBlendInDuration);
			else
				BlendIn = 1.0f;

			BlendOut = 1.0f;

			//ConOutL(CStrF("BlendIn = %3.3f", Blend));
		}
		else
		{
			if (m_InvBlendInDuration < 0.0f)
				BlendIn = m_InvBlendInDuration;
			else if (m_InvBlendInDuration > 0)
				BlendIn = Clamp01((_pContext->m_GameTime - m_BlendInStartTime).GetTime() * m_InvBlendInDuration);
			else
				BlendIn = 1.0f;

			if (m_InvBlendOutDuration < 0.0f)
				BlendOut = m_InvBlendOutDuration;
			else
				BlendOut = 1.0f - Clamp01((_pContext->m_GameTime - m_BlendOutStartTime).GetTime() * m_InvBlendOutDuration);

			//ConOutL(CStrF("BlendIn = %3.3f, BlendOut = %3.3f, Blend = %3.3f", BlendIn, BlendOut, Blend));
		}
	}

	if ((BlendIn < 0.0f) && (BlendOut < 0.0f))
		Blend = -3.0f;
	else
		Blend = BlendIn * BlendOut; // Smoother than Min(BlendIn, BlendOut)

	if (Blend == 0)
		return false;

	CWorldData* pWData;
	if(_pContext->m_pWPhysState)
		pWData = _pContext->m_pWPhysState->GetWorldData();
	else
	{
		M_ASSERT(_pContext->m_pMapData, "!");
		pWData = _pContext->m_pMapData->m_spWData;
	}

	int nLayers = 0;
	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		if (iAnim != _iAnim)
			continue;

		spCXR_Anim_SequenceData pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, iAnim);
		if (pAnimLayerSeq == NULL)
			continue;

		fp32 LayerBlend = Blend * pAnimLayer->GetOpacity();

		if (LayerBlend == 0.0f)
			continue;

		M_ASSERT(m_pAGI->m_pEvaluator, "!");
		m_pAGI->m_pEvaluator->AG_RefreshStateInstanceProperties(_pContext, this);

		int16 iTimeControlProperty = pAnimLayer->GetTimeControlProperty();
		CMTime ContinousTime = m_pAGI->m_pEvaluator->AG_EvaluateProperty(_pContext, iTimeControlProperty, NULL).GetTime();
		ContinousTime = (ContinousTime + CMTime::CreateFromSeconds(pAnimLayer->GetTimeOffset() + m_Enter_AnimTimeOffset)).Scale(pAnimLayer->GetTimeScale());
		CMTime LoopedTime = pAnimLayerSeq->GetLoopedTime(ContinousTime);

		uint32 Flags = 0;
		uint32 StateFlags = pState->GetFlags(0);
		if (StateFlags & CHAR_STATEFLAG_ATTACHNOREL)
			Flags |= CXR_ANIMLAYER_ATTACHNOREL;
		if (StateFlags & CHAR_STATEFLAG_SYNCHACK)
			Flags |= CXR_ANIMLAYER_TAGSYNC;
		if (StateFlags & CHAR_STATEFLAG_WALLCOLLISION)
			Flags |= CXR_ANIMLAYER_TAGWALLCOLLISION;
		if (StateFlags & CHAR_STATEFLAG_FORCELAYERVELOCITY)
			Flags |= CXR_ANIMLAYER_FORCELAYERVELOCITY;
		if (StateFlags & CHAR_STATEFLAG_LAYERNOVELOCITY)
			Flags |= CXR_ANIMLAYER_LAYERNOVELOCITY;
		if (StateFlags & CHAR_STATEFLAG_LAYERADDITIVEBLEND)
			Flags |= CXR_ANIMLAYER_ADDITIVEBLEND;

		_Layer.Create3(pAnimLayerSeq, LoopedTime, pAnimLayer->GetTimeScale(), LayerBlend, pAnimLayer->GetBaseJointIndex(), Flags);
		_Layer.m_BlendIn = BlendIn;
		_Layer.m_BlendOut = BlendOut;

		if (StateFlags & CHAR_STATEFLAG_LAYERADJUSTFOROFFSET)
			_Layer.m_TimeOffset = m_Enter_AnimTimeOffset;

		_Layer.m_ContinousTime = ContinousTime;

		return true;
	}

	return false;
}

bool CWAGI_StateInstance::HasSpecificAnimation(int32 _iAnim) const
{
	MSCOPESHORT(CWAGI_StateInstance::HasSpecificAnimation);

	if (!m_bHasAnimation)
		return false;

	const CXRAG_AnimList* pAnimList = m_pAGI->GetAnimList();
	if (pAnimList == NULL)
		return false;

	const CXRAG_State* pState = m_pAGI->GetState(m_iState);
	if (pState == NULL)
		return false;

	if (m_EnterTime.AlmostEqual(AGI_UNDEFINEDTIME))
		return false;

	int nLayers = 0;
	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int32 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = m_pAGI->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		if (pAnimLayer->GetAnimIndex() == _iAnim)
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------

#if	defined(M_RTM) && defined( AG_DEBUG )
#warning "M_RTM and AG_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
