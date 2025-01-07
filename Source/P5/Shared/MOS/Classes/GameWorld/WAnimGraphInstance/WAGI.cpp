#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAGI.h"
#include "WAGI_StateInstPacked.h"
#include "WAG_ClientData.h"
#include "../../../XR/XRAnimGraph/AnimGraph.h"
#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"
#include "../WDataRes_AnimGraph.h"
#include "../WPhysState.h"
#include "../WObjects/WObj_Game.h"

//#ifndef M_RTM
//#define WAGI_DEBUG_ENABLE
//#endif

MRTC_IMPLEMENT(CWAGI_Token_Packed, CReferenceCount);

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CWAGI::Clear()
{
	MSCOPE(CWAGI::Clear, WAGI);

	//ConOutL(CStrF("  CWAGI::Clear() - pAGI %X", this));

	m_iRandseed = 0;

	m_iAnimGraphRes = -1;
	m_iAnimListRes = -2;

	m_spAnimGraph = NULL;
	m_spAnimList = NULL;

	m_nAnimGraphRefs = 0;
	m_nAnimListRefs = 0;

	m_lTokens.Clear();

	m_OverlayAnim.Clear();
	m_OverlayAnimLipSync.Clear();
	m_OverLayAnimLipSyncBaseJoint = 0;
	m_OverlayAnim_StartTime = AGI_UNDEFINEDTIME;
	m_bOverlayAnimFirstVelocityRequest = false;
	m_bDisableAll = false;
	m_bForceRefresh = false;

	//ConOutL(CStrF("CWAGI::Clear() - pAGI %X, iRand %d, iAnimGraphRes %d, iAnimListRes %d", this, m_iRand, m_iAnimGraphRes, m_iAnimListRes));
}

//--------------------------------------------------------------------------------

void CWAGI::CopyFrom(const CWAGI* _pAGI)
{
	MSCOPE(CWAGI::CopyFrom, WAGI);

	//ConOutL(CStrF("  CWAGI::CopyFrom(%X) - pAGI %X", _pAGI, this));

	m_iRandseed = _pAGI->m_iRandseed;

	m_iAnimGraphRes = _pAGI->m_iAnimGraphRes;
	m_iAnimListRes = _pAGI->m_iAnimListRes;

	m_spAnimGraph = NULL;
	m_spAnimList = NULL;

	m_nAnimGraphRefs = 0;
	m_nAnimListRefs = 0;

	m_lTokens.SetLen(_pAGI->m_lTokens.Len());
	for (int iToken = 0; iToken < _pAGI->m_lTokens.Len(); iToken++)
	{
		CWAGI_Token* pToken = &(m_lTokens[iToken]);
		pToken->CopyFrom(&(_pAGI->m_lTokens[iToken]));
		pToken->SetAGI(this);
	}

	m_lEnterStateEntries.SetLen(_pAGI->m_lEnterStateEntries.Len());
	for (int iEntry = 0; iEntry < _pAGI->m_lEnterStateEntries.Len(); iEntry++)
	{
		m_lEnterStateEntries[iEntry].CopyFrom(_pAGI->m_lEnterStateEntries[iEntry]);
	}

	m_OverlayAnim = _pAGI->m_OverlayAnim;
	m_OverlayAnimLipSync = _pAGI->m_OverlayAnimLipSync;
	m_OverLayAnimLipSyncBaseJoint = _pAGI->m_OverLayAnimLipSyncBaseJoint;
	m_OverlayAnim_StartTime = _pAGI->m_OverlayAnim_StartTime;
	m_bOverlayAnimFirstVelocityRequest = _pAGI->m_bOverlayAnimFirstVelocityRequest;
	m_bDisableAll = _pAGI->m_bDisableAll;

	//ConOutL(CStrF("CWAGI::CopyFrom(%X) - pAGI %X, iRand %d, iAnimGraphRes %d, iAnimListRes %d", _pAGI, this, m_iRand, m_iAnimGraphRes, m_iAnimListRes));
}

//--------------------------------------------------------------------------------

uint32 CWAGI::GetTokenStateFlags(const CWAGI_Context *_pContext, int8 _TokenID, uint8 _iFlags) const
{
	MSCOPE(CWAGI::GetTokenStateFlags, WAGI);

	const CWAGI_Token* pToken = GetTokenFromID(_TokenID);
	if (pToken == NULL)
		return 0;

	AGIResScope(this, _pContext, return 0);

	const CXRAG_State* pState = GetState(pToken->GetStateIndex());
	if (pState == NULL)
		return 0;

	return pState->GetFlags(_iFlags);
}

//--------------------------------------------------------------------------------

uint32 CWAGI::GetStateFlags(const CXRAG_State* _pState, uint8 _iFlags) const
{
	MSCOPE(CWAGI::GetTokenStateFlags, WAGI);

	if (_pState == NULL)
		return 0;

	return _pState->GetFlags(_iFlags);
}

//--------------------------------------------------------------------------------

bool CWAGI::GetTokenStateConstantValue(const CWAGI_Context *_pContext, int8 _TokenID, uint16 _StateConstantID, fp32& _Value) const
{
	MSCOPE(CWAGI::GetTokenStateConstantValue, WAGI);

	const CWAGI_Token* pToken = GetTokenFromID(_TokenID);
	if (pToken == NULL)
		return false;

	AGIResScope(this, _pContext, return false);

	return GetStateConstantValue(pToken->GetStateIndex(), _StateConstantID, _Value);
}

//--------------------------------------------------------------------------------

fp32 CWAGI::GetTokenStateConstantValueDef(const CWAGI_Context *_pContext, int8 _TokenID, uint16 _StateConstantID, fp32 _DefaultValue) const
{
	MSCOPE(CWAGI::GetTokenStateConstantValueDef, WAGI);

	fp32 Result;
	if (GetTokenStateConstantValue(_pContext, _TokenID, _StateConstantID, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

bool CWAGI::GetStateConstantValue(CAGStateIndex _iState, uint16 _StateConstantID, fp32& _Value) const
{
	MSCOPE(CWAGI::GetStateConstantValue, WAGI);

	const CXRAG_State* pState = GetState(_iState);
	if (pState == NULL)
		return false;


	for (int jStateConstant = 0; jStateConstant < pState->GetNumConstants(); jStateConstant++)
	{
		int16 iStateConstant = pState->GetBaseConstantIndex() + jStateConstant;
		const CXRAG_StateConstant* pStateConstant = m_spAnimGraph->GetStateConstant(iStateConstant);
		if (pStateConstant->m_ID == _StateConstantID)
		{
			_Value = pStateConstant->m_Value;
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

fp32 CWAGI::GetStateConstantValueDef(CAGStateIndex _iState, uint16 _StateConstantID, fp32 _DefaultValue) const
{
	MSCOPE(CWAGI::GetStateConstantValueDef, WAGI);

	fp32 Result;
	if (GetStateConstantValue(_iState, _StateConstantID, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

bool CWAGI::GetStateConstantValue(const CXRAG_State* _pState, uint16 _StateConstantID, fp32& _Value) const
{
	MSCOPE(CWAGI::GetStateConstantValue, WAGI);

	if (_pState == NULL)
		return false;


	int32 NumConstants = _pState->GetNumConstants();
	for (int jStateConstant = 0; jStateConstant < NumConstants; jStateConstant++)
	{
		int16 iStateConstant = _pState->GetBaseConstantIndex() + jStateConstant;
		const CXRAG_StateConstant* pStateConstant = m_spAnimGraph->GetStateConstant(iStateConstant);
		if (pStateConstant->m_ID == _StateConstantID)
		{
			_Value = pStateConstant->m_Value;
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

fp32 CWAGI::GetStateConstantValueDef(const CXRAG_State* _pState, uint16 _StateConstantID, fp32 _DefaultValue) const
{
	MSCOPE(CWAGI::GetStateConstantValueDef, WAGI);

	fp32 Result;
	if (GetStateConstantValue(_pState, _StateConstantID, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

void CWAGI::GetAnimLayers(const CWAGI_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAGI::GetAnimLayers, WAGI);

/*
	CWAGI_Context AGIContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AGIContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AGIContext);

	if (_nLayers <= 0)
	{
		_nLayers = 0;
		return;
	}
	
	{
		MSCOPESHORT(OverlayAnim);
		// OverlayAnim
		if (m_OverlayAnim.IsValid() && m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME) == false)
		{
			CMTime ContinousTime = _pContext->m_GameTime - m_OverlayAnim_StartTime;
			spCXR_Anim_SequenceData pOverlayAnimSeq = m_OverlayAnim.GetAnimSequenceData_MapData(_pContext->m_pWPhysState->GetMapData());
			if (pOverlayAnimSeq != NULL)
			{
				CMTime LoopedTime = pOverlayAnimSeq->GetLoopedTime(ContinousTime);

				fp32 LayerTimeScale = 1.0f;
				fp32 LayerBlend = 1.0f;
				uint8 iLayerBaseJoint = 0;
				uint32 LayerFlags = 0;

				_pLayers[0].Create3(pOverlayAnimSeq, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
				_pLayers[0].m_ContinousTime = ContinousTime;
	#ifdef WAGI_DEBUG_ENABLE
				_pLayers[0].m_DebugMessage = CStrF("(Overlay) iRes %d, iAnim %d, AnimStartTime %3.3f, GameTime %3.3f, LoopedAnimTime %3.3f (ContinuousAnimTime %3.3f)",
												  m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq, 
												m_OverlayAnim_StartTime, _pContext->m_GameTime, LoopedTime, ContinousTime);
	#endif
				_nLayers = 1;

			}
			else
			{
#ifdef	AG_DEBUG
				ConOut(CStrF("Invalid OverlayAnim (iAnimContainerResource %d, iAnimSeq %d).", m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq));
#endif
				_nLayers = 0;
			}

			spCXR_Anim_SequenceData pOverlayAnimSeqLipSync = m_OverlayAnimLipSync.GetAnimSequenceData_MapData(_pContext->m_pWPhysState->GetMapData());
			if (pOverlayAnimSeqLipSync)
			{
				CMTime LoopedTime = pOverlayAnimSeqLipSync->GetLoopedTime(ContinousTime);
				fp32 LayerTimeScale = 1.0f;
				fp32 LayerBlend = 1.0f;
				uint8 iLayerBaseJoint = m_OverLayAnimLipSyncBaseJoint;
				uint32 LayerFlags = CXR_ANIMLAYER_ADDITIVEBLEND;

				_pLayers[_nLayers].Create3(pOverlayAnimSeqLipSync, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
				_pLayers[_nLayers].m_ContinousTime = ContinousTime;
#ifdef WAGI_DEBUG_ENABLE
				_pLayers[_nLayers].m_DebugMessage = CStrF("(Overlay) iRes %d, iAnim %d, AnimStartTime %3.3f, GameTime %3.3f, LoopedAnimTime %3.3f (ContinuousAnimTime %3.3f)",
					m_OverlayAnimLipSync.m_iAnimContainerResource, m_OverlayAnimLipSync.m_iAnimSeq, 
					m_OverlayAnim_StartTime, _pContext->m_GameTime, LoopedTime, ContinousTime);
#endif
				_nLayers++;
			}

			return;
		}
	}

	AGIResScope(this, _pContext, {_nLayers = 0; return;});
/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
	{
		//ConOutL(CStrF("CWAGI::GetAnimLayers() - pAGI = %X, iRand %d - FAILED!", this, m_iRand));
		_nLayers = 0;
		return;
	}
*/
	CXR_AnimLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;
	
	{
		MSCOPESHORT(TokenLayers);
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			if (_nLayers >= nMaxLayers)
				break;

			CWAGI_Token* pToken = &(m_lTokens[iToken]);

			pLayers = &(_pLayers[_nLayers]);

			bool bAllowBlend = _nLayers > 0;
			int nTokenLayers = nMaxLayers - _nLayers;
			pToken->GetAnimLayers(_pContext, pLayers, nTokenLayers, _iDisableStateInstanceAnimsCallbackMsg, bAllowBlend);
			_nLayers += nTokenLayers;
		}
	}
	// Make sure we have full blended layer
	bool bGotFullBlend = false;
	int32 iLastFull = 999;
	bool bForceSub = false;
	bool bHasBBN10 = false;
	TArray<CXR_AnimLayer> lBBN10;
	TArray<CXR_AnimLayer> lNotBBN10;
	int32 iBBN10Start = -1;
	for (int32 i = 0; i < _nLayers; i++)
	{
		// If we're supposed to force subsequent to blendbase 8, do that
		if (bForceSub && !_pLayers[i].m_iBlendBaseNode)
			_pLayers[i].m_iBlendBaseNode = 8;

		if (_pLayers[i].m_iBlendBaseNode == 10)
		{
			lBBN10.Add(_pLayers[i]);
			bHasBBN10 = true;
			if (iBBN10Start == -1)
				iBBN10Start = i;
		}
		else if (bHasBBN10)
		{
			lNotBBN10.Add(_pLayers[i]);
		}

		if (!bGotFullBlend && !_pLayers[i].m_iBlendBaseNode)
		{
			iLastFull = Min(i,iLastFull);
			if (_pLayers[i].m_Blend = 1.0f)
				bGotFullBlend = true;
		}
		if (/*_pLayers[i].m_Blend >= 1.0f &&*/ (_pLayers[i].m_Flags & CXR_ANIMLAYER_FORCESUBSEQUENTTO8))
		{
			//ConOutL(CStrF("Blend: %f BlendIn: %f BlendOut: %f", _pLayers[i].m_Blend, _pLayers[i].m_BlendIn, _pLayers[i].m_BlendOut));
			bForceSub = true;
		}
	}
	if (iBBN10Start != -1 && (lNotBBN10.Len() > 0))
	{
		int32 Len = lNotBBN10.Len();
		for (int32 i = 0; i < Len; i++)
			_pLayers[iBBN10Start + i] = lNotBBN10[i];
		int32 LenBBN10 = lBBN10.Len();
		for (int32 i = 0; i < LenBBN10; i++)
			_pLayers[iBBN10Start + i + Len] = lBBN10[i];
	}


	if (!bGotFullBlend && (iLastFull < _nLayers))
	{
		_pLayers[iLastFull].m_Blend = 1.0f;
	}

#ifdef WAGI_DEBUG_ENABLE
	if (CWAGI::DebugEnabled(_pContext))
	{
		//ConOutL(CStrF("GetAnimLayers: nLayers = %d", _nLayers));
		for (int iLayer = 0; iLayer < _nLayers; iLayer++)
		{
			if (_pContext->m_pWPhysState->IsServer())
				ConOutL(CStrF("(Server, iObj %d (%X)) Layer%d (b%3.3f): %s", _pContext->m_pObj->m_iObject, _pContext->m_pObj, iLayer, _pLayers[iLayer].m_Blend, _pLayers[iLayer].m_DebugMessage.Str()));
			else
				ConOutL(CStrF("(Client, iObj %d (%X)) Layer%d (b%3.3f): %s", _pContext->m_pObj->m_iObject, _pContext->m_pObj, iLayer, _pLayers[iLayer].m_Blend, _pLayers[iLayer].m_DebugMessage.Str()));
		}
	}
#endif

#ifdef WAGI_DEBUG_ENABLE
	if ((_nLayers == 0) && (m_lTokens.Len() > 0))
	{
		if (CWAGI::DebugEnabled(_pContext))
		{
			if (_pContext->m_pWPhysState->IsServer())
				ConOutL(CStrF("(Server) iObj %d (%X) CWAGI::GetAnimLayers() - nLayers = %d", _pContext->m_pObj->m_iObject, _pContext->m_pObj, _nLayers));
			else
				ConOutL(CStrF("(Client) iObj %d (%X) CWAGI::GetAnimLayers() - nLayers = %d", _pContext->m_pObj->m_iObject, _pContext->m_pObj, _nLayers));
		}
	}
#endif
//	UnacquireAllResources();
}

bool CWAGI::GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iToken, int32 _iAnim, int32 _StartTick) const
{
	MSCOPE(CWAGI::GetSpecificAnimLayer, WAGI);
	// Find specific animlayer in given token
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAGI_Token* pToken = &(m_lTokens[iToken]);

		if (pToken->GetID() == _iToken)
			return pToken->GetSpecificAnimLayer(_pContext, _Layer, _iAnim, _StartTick);
	}

	return false;
}

//--------------------------------------------------------------------------------

bool CWAGI::GetAnimVelocity(const CWAGI_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAGI::GetAnimVelocity, WAGI);

	AGIResScope(this, _pContext, return false);

/*
	CWAGI_Context AGIContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AGIContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AGIContext);

/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
		return false;
*/

	CMat43fp32 ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
	CVec3Dfp32(0, 0, 1).SetMatrixRow(ObjMatrix, 2); ObjMatrix.RecreateMatrix(2, 0);

	_MoveVelocity = 0;
	_RotVelocity.Unit();

	CWAGI_Context OverlayContext;
	if(m_OverlayAnim.IsValid() && m_bOverlayAnimFirstVelocityRequest)
	{
		_pContext = _pContext->Extend(m_OverlayAnim_StartTime, &OverlayContext);
		m_bOverlayAnimFirstVelocityRequest = false;
	}

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);
	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
#ifdef	AG_DEBUG
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;
#endif

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if ((Layer.m_iBlendBaseNode > 1) || (Layer.m_Flags & CXR_ANIMLAYER_LAYERNOVELOCITY))
			continue;

		// Calculate absolute positions.
		CVec3Dfp32 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
		fp32 TimeSpan = _pContext->m_TimeSpan;
		CMTime Diff = TimeA - CMTime::CreateFromSeconds(TimeSpan) - CMTime::CreateFromSeconds(Layer.m_TimeOffset);
		// Hack to compensate for missed velocity
		if (Diff.Compare(CMTime::CreateFromSeconds(0.01f)) < 0)
		{
			// Must catch up to our own timeframe
			TimeSpan += TimeA.GetTime() - Layer.m_TimeOffset;
			TimeA = CMTime::CreateFromSeconds(Layer.m_TimeOffset);
		}
		Layer.m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;	
		CMTime TimeB = Layer.m_spSequence->GetLoopedTime(TimeA + CMTime::CreateFromSeconds(TimeSpan * Layer.m_TimeScale));
		Layer.m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		CVec3Dfp32 dMove;
		CQuatfp32 dRot;
		dMove = (MoveB - MoveA);
		dRot = RotA;
		dRot.Inverse();
		dRot = RotB * dRot;

		// Compensate for moving while rotating.
		CQuatfp32 InvRotA;
		CMat43fp32 MatA;
		MatA.Unit();
		RotA.CreateMatrix3x3(MatA);
		CVec3Dfp32(0,0,1).SetMatrixRow(MatA,2);
		MatA.RecreateMatrix(2,0);
		CMat43fp32 InvMatA;
		MatA.InverseOrthogonal(InvMatA);
//		InvRotA = RotA; InvRotA.Inverse();
	//	InvRotA.CreateMatrix3x3(InvMatA);
		dMove.MultiplyMatrix3x3(InvMatA);

		// FIXME: Might be removed...
		if (RotA.DotProd(RotB) < 0.0f)
		{
			dRot.k[0] = -dRot.k[0];
			dRot.k[1] = -dRot.k[1];
			dRot.k[2] = -dRot.k[2];
			dRot.k[3] = -dRot.k[3];
		}

/*
		ConOutL(CStrF("AnimPosA <%3.3f, %3.3f, %3.3f>, AnimPosB <%3.3f, %3.3f, %3.3f>, dAnimPos <%3.3f, %3.3f, %3.3f>",
					 MoveA[0], MoveA[1], MoveA[2], MoveB[0], MoveB[1], MoveB[2], dMove[0], dMove[1], dMove[2]));
*/
		// Add loopseam compensation.
		// ARGH this is too delicate so I have to add some delta value to compensate
		TimeB += CMTime::CreateFromSeconds(0.00001f);
		bool bLooping = TimeB.Compare(TimeA) < 0;
		if (bLooping)
		{
			CVec3Dfp32 LoopMove;
			CQuatfp32 LoopRot;
			Layer.m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dMove += LoopMove;
			dRot *= LoopRot;
		}

		fp32 BlendFactor = Layer.m_Blend;

		// Convert into Object direction.
		dMove.MultiplyMatrix3x3(ObjMatrix);

		// Just use this layers velocity
		if (Layer.m_Flags & CXR_ANIMLAYER_FORCELAYERVELOCITY)
		{
			_MoveVelocity = dMove;
			_RotVelocity = dRot;
			break;
		}

		// Weight Move using layer blendfactor.
		CVec3Dfp32 TempVelo;
		CVec3Dfp32::Lerp(_MoveVelocity, dMove, TempVelo, BlendFactor);
		_MoveVelocity = TempVelo;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		CQuatfp32::Lerp(_RotVelocity, dRot, TempRot, BlendFactor);
		_RotVelocity = TempRot;
	}

//	UnacquireAllResources();
	return true;
}
//--------------------------------------------------------------------------------

bool CWAGI::GetAnimOffset(const CWAGI_Context* _pContext, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset)
{
	MSCOPE(CWAGI::GetAnimVelocity, WAGI);

	CMat43fp32 ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
	CVec3Dfp32(0, 0, 1).SetMatrixRow(ObjMatrix, 2); ObjMatrix.RecreateMatrix(2, 0);

	_MoveOffset = 0;
	_RotOffset.Unit();

	CWAGI_Context OverlayContext;
	if (m_bOverlayAnimFirstVelocityRequest)
	{
		_pContext = _pContext->Extend(m_OverlayAnim_StartTime, &OverlayContext);
		m_bOverlayAnimFirstVelocityRequest = false;
	}

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers,0);

	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if (Layer.m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;
		CMTime TimeB = CMTime::CreateFromSeconds(Layer.m_Time);
		Layer.m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		fp32 BlendFactor = Layer.m_Blend;

		// Convert into Object direction.
		MoveB.MultiplyMatrix3x3(ObjMatrix);

		// Weight Move using layer blendfactor.
		CVec3Dfp32 TempMove;
		CVec3Dfp32::Lerp(_MoveOffset, MoveB, TempMove, BlendFactor);
		_MoveOffset = TempMove;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		CQuatfp32::Lerp(_RotOffset, RotB, TempRot, BlendFactor);
		_RotOffset = TempRot;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CWAGI::GetAnimRelOffset(const CWAGI_Context* _pContext, CVec3Dfp32 _MoveOffset, CQuatfp32 _RotOffset, CVec3Dfp32& _MoveRelOffset, CQuatfp32& _RotRelOffset)
{
	MSCOPE(CWAGI::GetAnimVelocity, WAGI);

	CMat43fp32 ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
	CVec3Dfp32(0, 0, 1).SetMatrixRow(ObjMatrix, 2); ObjMatrix.RecreateMatrix(2, 0);

	_MoveRelOffset = 0;
	_RotRelOffset.Unit();

	CWAGI_Context OverlayContext;
	if (m_bOverlayAnimFirstVelocityRequest)
	{
		_pContext = _pContext->Extend(m_OverlayAnim_StartTime, &OverlayContext);
		m_bOverlayAnimFirstVelocityRequest = false;
	}

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers,0);

	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if (Layer.m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;
		CMTime TimeB = CMTime::CreateFromSeconds(Layer.m_Time);
		Layer.m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		CVec3Dfp32 dMove;
		CQuatfp32 dRot;
		dMove = MoveB - _MoveOffset;
		dRot = _RotOffset;
		dRot.Inverse();
		dRot = RotB * dRot;

		// Compensate for moving while rotating.
		CQuatfp32 InvRotA;
		InvRotA = _RotOffset; InvRotA.Inverse();
		CMat43fp32 InvMatA;
		InvRotA.CreateMatrix3x3(InvMatA);
		dMove.MultiplyMatrix3x3(InvMatA);

		// FIXME: Might be removed...
		CAxisRotfp32 dRotAR(dRot);

		if ((_RotOffset.DotProd(RotB) < 0.0f) || ((dRotAR.m_Angle != 0.0f) && (dRotAR.m_Axis[2] < 0.9f)))
		{
			if ((dRotAR.m_Angle != 0.0f) && (dRotAR.m_Axis[2] < 0.9f))
			{
				dRot.k[0] = -dRot.k[0];
				dRot.k[1] = -dRot.k[1];
				dRot.k[2] = -dRot.k[2];
				dRot.k[3] = -dRot.k[3];
			}

			CAxisRotfp32 dRotAR_Corrected(dRot);
			int x = 0;
		}

		fp32 BlendFactor = Layer.m_Blend;

		// Convert into Object direction.
		dMove.MultiplyMatrix3x3(ObjMatrix);

		// Weight Move using layer blendfactor.
		CVec3Dfp32 TempMove;
		CVec3Dfp32::Lerp(_MoveRelOffset, dMove, TempMove, BlendFactor);
		_MoveRelOffset = TempMove;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		CQuatfp32::Lerp(_RotRelOffset, dRot, TempRot, BlendFactor);
		_RotRelOffset = TempRot;
	}

	return true;
}

// Get only the rotational part?
bool CWAGI::GetAnimRotVelocity(const CWAGI_Context* _pContext, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAGI::GetAnimRotVelocity, WAGI);

	AGIResScope(this, _pContext, return false);

	_RotVelocity.Unit();

	CWAGI_Context OverlayContext;
	if(m_OverlayAnim.IsValid() && m_bOverlayAnimFirstVelocityRequest)
	{
		_pContext = _pContext->Extend(m_OverlayAnim_StartTime, &OverlayContext);
		m_bOverlayAnimFirstVelocityRequest = false;
	}

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);
	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
#ifdef	AG_DEBUG
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;
#endif

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if (Layer.m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		CVec3Dfp32 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
		Layer.m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;
		CMTime TimeB = Layer.m_spSequence->GetLoopedTime(TimeA + CMTime::CreateFromSeconds(_pContext->m_TimeSpan * Layer.m_TimeScale));
		Layer.m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		CQuatfp32 dRot;
		dRot = RotA;
		dRot.Inverse();
		dRot = RotB * dRot;

		// FIXME: Might be removed...
		if (RotA.DotProd(RotB) < 0.0f)
		{
			dRot.k[0] = -dRot.k[0];
			dRot.k[1] = -dRot.k[1];
			dRot.k[2] = -dRot.k[2];
			dRot.k[3] = -dRot.k[3];
		}

		// Add loopseam compensation.
		// ARGH this is too delicate so I have to add some delta value to compensate
		TimeB += CMTime::CreateFromSeconds(0.00001f);
		bool bLooping = TimeB.Compare(TimeA) < 0;
		if (bLooping)
		{
			CVec3Dfp32 LoopMove;
			CQuatfp32 LoopRot;
			Layer.m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dRot *= LoopRot;
		}

		fp32 BlendFactor = Layer.m_Blend;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		CQuatfp32::Lerp(_RotVelocity, dRot, TempRot, BlendFactor);
		_RotVelocity = TempRot;
	}

//	UnacquireAllResources();
	return true;
}

bool CWAGI::HasAnimVelocity(const CWAGI_Context* _pContext, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAGI::HasAnimVelocity, WAGI);
	if (!AcquireAllResources(_pContext))
		return false;

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);

	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
		if (pLayers[iLayer].m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		CVec3Dfp32 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(pLayers[iLayer].m_Time);
		pLayers[iLayer].m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;
		CMTime TimeB = pLayers[iLayer].m_spSequence->GetLoopedTime(TimeA + CMTime::CreateFromSeconds(_pContext->m_TimeSpan * pLayers[iLayer].m_TimeScale));
		pLayers[iLayer].m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		CVec3Dfp32 dMove;
		CQuatfp32 dRot;
		dMove = MoveB - MoveA;
		dRot = RotA;
		dRot.Inverse();
		dRot = RotB * dRot;

		// Add loopseam compensation.
		bool bLooping = TimeB.Compare(TimeA) < 0;
		if (bLooping)
		{
			CVec3Dfp32 LoopMove;
			CQuatfp32 LoopRot;
			pLayers[iLayer].m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dMove += LoopMove;
			dRot *= LoopRot;
		}

		if(dMove != 0)
		{
			UnacquireAllResources();
			return true;
		}
		if(dRot.k[0] != 0 || dRot.k[1] != 0 || dRot.k[2] != 0)
		{
			UnacquireAllResources();
			return true;
		}
	}

	UnacquireAllResources();
	return false;
}

//--------------------------------------------------------------------------------

void CWAGI::CheckAnimEvents(const CWAGI_Context* _pContext, int _iCallbackMessage)
{
	MSCOPE(CWAGI::CheckAnimEvents_2, WAGI);
	AGIResScope(this, _pContext, return);
/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
		return;
*/

	CXR_AnimLayer pLayers[AGI_MAXANIMLAYERS];
	int nLayers = AGI_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, 0);
	if (nLayers == 0)
	{
//		UnacquireAllResources();
		return;
	}

	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		fp32 StartScanTime = pLayers[iLayer].m_Time;

		// Check for freshly started animations and make sure that the first keyframes are scanned
		if(pLayers[iLayer].m_ContinousTime.Compare(_pContext->m_TimeSpan - 0.0001f) < 0)
		{
			continue;
		}
		if( pLayers[iLayer].m_ContinousTime.Compare(_pContext->m_TimeSpan * 2 - 0.0001f) < 0)
		{
//			ConOut("No loop");
			StartScanTime = 0.0f;
		}
//		if(_pContext->m_pWPhysState->IsClient())
//			ConOut(CStrF("CheckAnimEvents GT: %f   CT: %f", _pContext->m_GameTime, pLayers[iLayer].m_ContinousTime));

		CMTime EndScanTime = pLayers[iLayer].m_spSequence->GetLoopedTime(CMTime::CreateFromSeconds(StartScanTime + _pContext->m_TimeSpan));
		if (EndScanTime.Compare(StartScanTime) < 0)
		{
			CMTime MidScanTime = CMTime::CreateFromSeconds(pLayers[iLayer].m_spSequence->GetDuration());
			CheckAnimEvents(_pContext, pLayers[iLayer].m_spSequence, CMTime::CreateFromSeconds(StartScanTime), MidScanTime, _iCallbackMessage);
			CheckAnimEvents(_pContext, pLayers[iLayer].m_spSequence, CMTime(), EndScanTime, _iCallbackMessage);
		}
		else
		{
			CheckAnimEvents(_pContext, pLayers[iLayer].m_spSequence, CMTime::CreateFromSeconds(StartScanTime), EndScanTime, _iCallbackMessage);
		}

	}

//	UnacquireAllResources();
}


// Update breakout points
/*if (pKey->m_EventParams[0] != 0)
{
	// Ok, what range should the breakout points have?
	// Fixed param types, ie "from here we can run"
	// Or have codes/flags for certain types?
	ConOut(CStrF("Got PARAM: %d", pKey->m_EventParams[0]));
}*/

void CWAGI::FindBreakoutSequence(const CWAGI_Context* _pContext, int16 _iState, fp32 _Offset, int16& _iSequence, CMTime& _AnimTime)
{
	const CXRAG_State* pState = GetState(_iState);
	// 0x00100000 == CHAR_STATEFLAG_DISABLEBREAKOUT
	if (!pState || (pState->GetFlags(0) & 0x00100000))
		return;

	CXRAG_AnimList* pAnimList = GetAnimList();
	const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if (pAnimLayer && pAnimList && (pState->GetNumAnimLayers() > 0))
	{
		int16 iAnim = pAnimLayer->GetAnimIndex();

		CWorldData* pWData;
		if(_pContext->m_pWPhysState)
			pWData = _pContext->m_pWPhysState->GetWorldData();
		else
		{
			M_ASSERT(_pContext->m_pMapData, "!");
			pWData = _pContext->m_pMapData->m_spWData;
		}

		CXR_Anim_SequenceData* pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, iAnim);
		if (pAnimLayerSeq && (pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
		{				
			int16 iTimeControlProperty = pAnimLayer->GetTimeControlProperty();
			CMTime ContinousTime = m_pEvaluator->AG_EvaluateProperty(_pContext, iTimeControlProperty, NULL).GetTime();
			ContinousTime = (ContinousTime + CMTime::CreateFromSeconds(pAnimLayer->GetTimeOffset() + _Offset)).Scale(pAnimLayer->GetTimeScale());
			// Set previous properties
			_iSequence = iAnim;
			_AnimTime = pAnimLayerSeq->GetLoopedTime(ContinousTime);
		}
		else
		{
			_iSequence = -1;
			_AnimTime.Reset();
		}
	}
}
bool CWAGI::FindEntryPoint(const CWAGI_Context* _pContext, int16 _iState, int16 _iPrevSequence, CMTime _PrevTime, fp32& _Offset)
{
	CXRAG_AnimList* pAnimList = GetAnimList();
	const CXRAG_State* pState = GetState(_iState);
	
	if (!pState || (_iPrevSequence == -1))
		return false;

	int16 iNewSeq = pState->GetBaseAnimLayerIndex();
	const CXRAG_AnimLayer* pAnimLayerNew = GetAnimLayer(iNewSeq);
	if (pAnimLayerNew && pAnimList && (pState->GetNumAnimLayers() > 0))
	{
		int16 iAnimNew = pAnimLayerNew->GetAnimIndex();

		CWorldData* pWData;
		if(_pContext->m_pWPhysState)
			pWData = _pContext->m_pWPhysState->GetWorldData();
		else
		{
			M_ASSERT(_pContext->m_pMapData, "!");
			pWData = _pContext->m_pMapData->m_spWData;
		}
		CXR_Anim_SequenceData* pAnimLayerSeqPrev = pAnimList->GetAnimSequenceData(pWData, _iPrevSequence);
		CXR_Anim_SequenceData* pAnimLayerSeqNew = pAnimList->GetAnimSequenceData(pWData, iAnimNew);
		if (pAnimLayerSeqPrev && pAnimLayerSeqNew && 
			(pAnimLayerSeqNew->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
		{
			_Offset = pAnimLayerSeqNew->FindEntryTime(pAnimLayerSeqPrev, _PrevTime);
			return true;
		}
	}

	return false;
}

// Find breakoutpoints from given time and forward
void CWAGI::FindBreakoutPoints(const CWAGI_Context* _pContext, int16 _iState, CXR_Anim_BreakoutPoints& _Points, fp32 _Offset)
{
	MSCOPE(CWAGI::FindBreakoutPoints, WAGI);
	AGIResScope(this, _pContext, return);
	/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
	return;
	*/

	// Reset breakoutpoint data
	_Points.m_lPoints.Clear();

	// Mmmkay, find stateinstance for given state, 
	CXRAG_AnimList* pAnimList = GetAnimList();
	if (pAnimList == NULL)
		return;

	// First create the points in this array, then copy to real data
	CBreakoutPoint lPoints[255];
	uint MaxPoints = 255;
	int32 PointLen = 0;

	const CXRAG_State* pState = GetState(_iState);
	if (!pState || !pState->GetNumAnimLayers())
		return;

	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		//		ConOutL(CStrF("iAnim %d", iAnim));

		CWorldData* pWData;
		if(_pContext->m_pWPhysState)
			pWData = _pContext->m_pWPhysState->GetWorldData();
		else
		{
			M_ASSERT(_pContext->m_pMapData, "!");
			pWData = _pContext->m_pMapData->m_spWData;
		}
		CXR_Anim_SequenceData* pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, iAnim);
		if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
			continue;

		int16 iTimeControlProperty = pAnimLayer->GetTimeControlProperty();
		CMTime ContinousTime = m_pEvaluator->AG_EvaluateProperty(_pContext, iTimeControlProperty, NULL).GetTime();
		ContinousTime = (ContinousTime + CMTime::CreateFromSeconds(pAnimLayer->GetTimeOffset() + _Offset)).Scale(pAnimLayer->GetTimeScale());
		CMTime LoopedTime = pAnimLayerSeq->GetLoopedTime(ContinousTime);

		//ConOut(CStrF("Breakout Looped time: %f",LoopedTime));

		PointLen += pAnimLayerSeq->FindBreakoutPoints(lPoints, MaxPoints, LoopedTime);
	}

	// Create breakoutpoints
	_Points.Create(lPoints,PointLen);
}

void CWAGI::FindEntryPoints(const CWAGI_Context* _pContext, int16 _iState, CXR_Anim_EntryPoints& _Points)
{
	MSCOPE(CWAGI::FindEntryPoints, WAGI);
	AGIResScope(this, _pContext, return);

	// Reset Entrypoint data
	_Points.m_lPoints.Clear();

	// Mmmkay, find stateinstance for given state, 
	CXRAG_AnimList* pAnimList = GetAnimList();
	if (pAnimList == NULL)
		return;

	// First create the points in this array, then copy to real data
	CEntryPoint lPoints[255];
	uint MaxPoints = 255;
	int32 PointLen = 0;

	const CXRAG_State* pState = GetState(_iState);
	if (!pState || !pState->GetNumAnimLayers())
		return;

	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		//		ConOutL(CStrF("iAnim %d", iAnim));

		CWorldData* pWData;
		if(_pContext->m_pWPhysState)
			pWData = _pContext->m_pWPhysState->GetWorldData();
		else
		{
			M_ASSERT(_pContext->m_pMapData, "!");
			pWData = _pContext->m_pMapData->m_spWData;
		}
		CXR_Anim_SequenceData* pAnimLayerSeq = pAnimList->GetAnimSequenceData(pWData, iAnim);
		if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
			continue;
		/*if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASENTRYPOINTS))
			continue;*/

		PointLen += pAnimLayerSeq->FindEntryPoints(lPoints,MaxPoints);
	}

	// Create entrypoints
	_Points.Create(lPoints,PointLen);
}

bool CWAGI::GetAbsoluteAnimOffset(const CWAGI_Context* _pContext, int32 _iAnim, fp32 _TimeOffset, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset)
{
	MSCOPE(CWAGI::GetAbsoluteAnimOffset, WAGI);

	_MoveOffset = 0;
	_RotOffset.Unit();

	// Find sequence data from given animation index
	const spCXR_Anim_SequenceData pSeq = m_spAnimList->GetAnimSequenceData(_pContext->m_pWPhysState->m_spWData,_iAnim);
	if (!pSeq)
		return false;

	// Calculate absolute positions.
	CMTime LoopedTime = CMTime::CreateFromSeconds(_TimeOffset);
	LoopedTime = pSeq->GetLoopedTime(LoopedTime);
	pSeq->EvalTrack0(LoopedTime, _MoveOffset, _RotOffset);

	return true;
}

//--------------------------------------------------------------------------------

CAGAnimIndex CWAGI::GetAnimFromAction(const CWAGI_Context* _pContext, CStr& _MoveAction, fp32& _TimeOffset) const
{
	MSCOPE(CWAGI::GetAnimFromAction, WAGI);
	// Start by finding action and seeing to which state it leads, and then find animation from 
	// that state, only care about first anim layer for now

	uint32 ActionHashKey = StringToHash(_MoveAction);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	const CXRAG_State* pState = (pAction ? GetState(pAction->GetTargetStateIndex()) : NULL);
	if (!pState || !pState->GetNumAnimLayers())
		return false;
	const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if (!pAnimLayer)
		return false;
	_TimeOffset = pAction->GetAnimTimeOffset();

	return pAnimLayer->GetAnimIndex();
}

bool CWAGI::GetAnimFromFirstActionInState(const CWAGI_Context* _pContext, CAGAnimIndex _iAnim, int32 _iToken, CAGAnimIndex& _iTargetAnim, CAGActionIndex& _iTargetAction) const
{
	MSCOPE(CWAGI::GetAnimFromFirstActionInAnimState, WAGI);
	// Find state that contains given animation, find the first action in that state that has a 
	// movetoken to a state with an animation
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAGI_Token* pToken = &(m_lTokens[iToken]);

		if (pToken->GetID() != _iToken)
			continue;
		int32 iState = pToken->GetSpecificState(_pContext, _iAnim);
		const CXRAG_State* pState = (iState >= 0 ? GetState(iState) : NULL);
		const CXRAG_State* pNextState = (iState >= 0 ? GetState(iState+1) : NULL);
		if (pState && pNextState)
		{
			int32 iBaseIndex = pState->GetBaseActionIndex();
			int32 NumActions = pNextState->GetBaseActionIndex() - iBaseIndex;
			for (int32 i = 0; i < NumActions; i++)
			{
				// Find an action that leads to a state with an animation
				const CXRAG_Action* pAction = GetAction(iBaseIndex + i);
				if (!pAction)
					continue;
				
				const CXRAG_State* pTargetState = GetState(pAction->GetTargetStateIndex());
				if (pTargetState && pTargetState->GetNumAnimLayers() > 0)
				{
					// Yay, found a target state with animation
					const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(pTargetState->GetBaseAnimLayerIndex());
					if (!pAnimLayer)
						return false;

					_iTargetAnim = pAnimLayer->GetAnimIndex();
					_iTargetAction = iBaseIndex + i;
					return true;
				}
			}
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

void CWAGI::CheckAnimEvents(const CWAGI_Context* _pContext, CXR_Anim_SequenceData* _pAnimSeqData, CMTime _StartTime, CMTime _EndTime, int _iCallbackMessage) const
{
	MSCOPE(CWAGI::CheckAnimEvents, WAGI);

//	uint32 ScanMask = ANIM_MASK_DIALOGUE | ANIM_MASK_EVENT1 | ANIM_MASK_SOUNDNAME;
	uint32 ScanMask = -1;
	const CXR_Anim_DataKey *pKey = _pAnimSeqData->GetEvents(_StartTime, _EndTime, ScanMask);
	while (pKey != NULL)
	{
		CWObject_Message Msg(_iCallbackMessage, aint(pKey));
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);

		pKey = _pAnimSeqData->GetEvents(_StartTime, _EndTime, ScanMask);
	}
}

//--------------------------------------------------------------------------------

void CWAGI::PredictionMiss_AddToken(CWAGI_Context* _pContext, int8 _TokenID)
{
	MSCOPE(CWAGI::PredictionMiss_AddToken, WAGI);

	CWAGI_Token* pToken = GetTokenFromID(_TokenID, true);
}

//--------------------------------------------------------------------------------

void CWAGI::PredictionMiss_RemoveToken(CWAGI_Context* _pContext, int8 _TokenID)
{
	MSCOPE(CWAGI::PredictionMiss_RemoveToken, WAGI);

	CWAGI_Token* pToken = GetTokenFromID(_TokenID, false);
	if (pToken != NULL)
		pToken->PredictionMiss_Remove(_pContext);
}

//--------------------------------------------------------------------------------

const CWAGI_Token* CWAGI::GetTokenFromID(int8 _TokenID) const
{
	MSCOPE(CWAGI::GetTokenFromID, WAGI);

	for (int jToken = 0; jToken < m_lTokens.Len(); jToken++)
		if (_TokenID == m_lTokens[jToken].GetID())
			return (&m_lTokens[jToken]);

	return NULL;
}

//--------------------------------------------------------------------------------

CWAGI_Token* CWAGI::GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent)
{
	MSCOPE(CWAGI::GetTokenFromID_2, WAGI);

	for (int jToken = 0; jToken < m_lTokens.Len(); jToken++)
		if (_TokenID == m_lTokens[jToken].GetID())
			return (&m_lTokens[jToken]);

	if (_bCreateNonExistent && (_TokenID != AG_TOKENID_NULL))
	{
		uint8 iToken = m_lTokens.Add(CWAGI_Token(_TokenID, this));
		return (&m_lTokens[iToken]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------
/*
void CWAGI::PerformAction(const CWAGI_Context* _pContext, int8 _ActionTokenID, int16 _iAction)
{
}
*/
//--------------------------------------------------------------------------------
void CWAGI::DoActionEffects(const CWAGI_Context* _pContext, int16 _iAction)
{
	const CXRAG_Action* pAction = GetAction(_iAction);
	if (pAction)
		InvokeEffects(_pContext, pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances());
}

void CWAGI::MoveAction(const CWAGI_Context* _pContext, int8 _ActionTokenID, int16 _iAction, fp32 _ForceOffset,int32 _MaxQueued)
{
	MSCOPE(CWAGI::MoveAction, WAGI);

	// FIXME: Think about error checking OnLeaveState/OnEnterState, etc...

	CWAGI_Token* pActionToken = GetTokenFromID(_ActionTokenID, false);

	if (pActionToken != NULL)
	{
		// Check if we're loaded with other stateinstances that wants to get through, if so
		// delete the first one in the queue
		pActionToken->ForceMaxStateInstances(_MaxQueued);
		pActionToken->LeaveState(_pContext, _iAction);
		pActionToken->EnterState(_pContext, _iAction, _ForceOffset);
	}
	else
	{
		pActionToken = GetTokenFromID(_ActionTokenID, true);
		if (pActionToken != NULL)
			pActionToken->EnterState(_pContext, _iAction, _ForceOffset);
		else
			ConOutL(CStrF("ERROR: Can't create AGI Token %d.", _ActionTokenID));
	}
/*
	// FIXME: Rewrite this dubble loop selection sort into something nicer...
	for (uint8 iToken = 0; iToken < (m_lTokens.Len() - 1); iToken++)
	{
		CWAGI_Token* pTokenI = &(m_lTokens[iToken]);
		const CXRAG_State* pStateI = m_spAnimGraph->GetState(pTokenI->GetStateIndex());
		for (uint8 jToken = iToken + 1; jToken < m_lTokens.Len(); jToken++)
		{
			CWAGI_Token* pTokenJ = &(m_lTokens[jToken]);
			const CXRAG_State* pStateJ = m_spAnimGraph->GetState(pTokenJ->GetStateIndex());

			if (pStateI->GetPriority() > pStateJ->GetPriority())
			{
				CWAGI_Token TempToken;
				TempToken.CopyFrom(pTokenI);
				pTokenI->CopyFrom(pTokenJ);
				pTokenJ->CopyFrom(&TempToken);
				break;
			}
		}
	}
*/
}

//--------------------------------------------------------------------------------
#define AG_TOKEN_EFFECT	(2)
void CWAGI::Refresh(const CWAGI_Context* _pContext)
{
	MSCOPE(CWAGI::Refresh, WAGI);

/*
	CStr Location;
	if (_pContext->m_pWPhysState->IsServer())
		Location = CStrF("(Server %X) ", _pContext->m_pWPhysState);
	else
		Location = CStrF("(Client %X) ", _pContext->m_pWPhysState);
	ConOutL(Location + CStrF("CWAGI::Refresh() - GameTime %3.3f, iRandseed %d", _pContext->m_GameTime, m_iRandseed));
*/
	m_iRandseed++;


	CWAGI_Context AGIContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AGIContext);

	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AGIContext);

	UnacquireAllResources();
	if (!AcquireAllResources(_pContext))
		return;

	// If no token exists, create a starting default token...
	if (m_lTokens.Len() == 0)
	{
		_pContext = _pContext->Extend(CMTime(), &AGIContext);
		MoveAction(_pContext, AG_TOKENID_DEFAULT, AG_ACTIONINDEX_START);
	}

	M_ASSERT(m_pEvaluator, "!");
	m_pEvaluator->AG_RefreshGlobalProperties(_pContext);

	{
	MSCOPESHORT(Tokens);
	// FIXME: Make sure iToken is not discontinuous by adding new tokens, etc.
	int32 TotNumStateInstances = 0;
	bool bHasEffectToken = false;
	int32 NumForceRefresh = 0;
WAGI_FORCEREFRESH:
	m_bForceRefresh = false;
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		// Create copy of context so that the original is kept safe from splits and extends.
		CWAGI_Context TokenContext;
		_pContext->Duplicate(&TokenContext);

		int nRefreshs = 0;
		CWAGI_Token* pToken = &(m_lTokens[iToken]);
		if (pToken->GetID() == AG_TOKEN_EFFECT)
			bHasEffectToken = true;
		uint32 RefreshFlags = AGI_TOKENREFRESHFLAGS_REFRESH;
		while ((RefreshFlags & AGI_TOKENREFRESHFLAGS_REFRESH) != 0)
		{
			RefreshFlags = pToken->Refresh(&TokenContext);
			pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.

			if ((RefreshFlags & AGI_TOKENREFRESHFLAGS_PERFORMEDACTION) != 0)
			{
				CWAGI_StateInstance* pTokenStateInstance = pToken->GetTokenStateInstance();
				if ((pTokenStateInstance != NULL) && (!pTokenStateInstance->HasAnimation()))
				{
#ifdef	AG_DEBUG
					if ((RefreshFlags & AGI_TOKENREFRESHFLAGS_REFRESH) == 0)
					{
						if (CWAGI::DebugEnabled(_pContext))
						{
/*
							if (_pContext->m_pWPhysState->IsServer())
								ConOutL(CStrF("(Server), iObj %d (%X)) Token%d was forced to leave/refresh State%d.", _pContext->m_pObj->m_iObject, _pContext->m_pObj, pToken->GetID(), pToken->GetStateIndex()));
							else
								ConOutL(CStrF("(Client), iObj %d (%X)) Token%d was forced to leave/refresh State%d.", _pContext->m_pObj->m_iObject, _pContext->m_pObj, pToken->GetID(), pToken->GetStateIndex()));
*/						}
					}
#endif
					RefreshFlags = AGI_TOKENREFRESHFLAGS_REFRESH;
				}
			}

			nRefreshs++;
			if (nRefreshs > 100)
			{
#ifdef	AG_DEBUG
				if (_pContext->m_pWPhysState->IsServer())
					ConOutL(CStrF("(Server) ERROR: AGI::Refresh() - Possible infinite refreshs for Token %d in State %d. Breaking out...", pToken->GetID(), pToken->GetStateIndex()));
				else
					ConOutL(CStrF("(CLIENT) ERROR: AGI::Refresh() - Possible infinite refreshs for Token %d in State %d. Breaking out...", pToken->GetID(), pToken->GetStateIndex()));
#endif
				break;
			}
		}
		pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.

		const CWAGI_StateInstance* pInstance = pToken->GetTokenStateInstance();
		if ((pToken->GetID() == 0) && pInstance && (!pInstance->HasAnimation()))
		{
			if (CWAGI::DebugEnabled(_pContext))
			{
				if (_pContext->m_pWPhysState->IsServer())
					ConOutL(CStrF("(Server) WARNING: Token%d in State%d has no animation.", pToken->GetID(), pToken->GetStateIndex()));
				else
					ConOutL(CStrF("(Client) WARNING: Token%d in State%d has no animation.", pToken->GetID(), pToken->GetStateIndex()));

			}
		}
		int32 NumStateInstances = pToken->GetNumStateInstances();
		TotNumStateInstances += NumStateInstances;
		if (NumStateInstances == 0)
		{
			//ConOutL(CStrF("CWAGI::Refresh() - Terminating Token %d", pToken->GetID()));
			m_lTokens.Del(iToken--);
		}
	}
		if (m_bForceRefresh && (NumForceRefresh < 3))
		{
			// Don't wanne be stuck here forever if there's a mistake
			NumForceRefresh++;
			goto WAGI_FORCEREFRESH;
		}
		// Increment importantag ticker if we don't have too many state instances
		if ((TotNumStateInstances <= m_lTokens.Len()) && !bHasEffectToken)
		{
			m_pEvaluator->IncrementLastAGEvent();
		}
	}
}

//--------------------------------------------------------------------------------

void CWAGI::RefreshPredictionMisses(const CWAGI_Context* _pContext)
{
	MSCOPE(CWAGI::RefreshPredictionMisses, WAGI);

/*
	CWAGI_Context AGIContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AGIContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AGIContext);

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		CWAGI_Token* pToken = &(m_lTokens[iToken]);
		pToken->RefreshPredictionMisses(_pContext);
	}
}

void CWAGI::AddEnterStateEntry(const CWAGI_EnterStateEntry& _OnEnterStateEntry)
{
	MSCOPE(CWAGI::AddEnterStateEntry, WAGI);
	if (_OnEnterStateEntry.m_TokenID < 0)
		return;
	// Only need the latest state entry from each token, (tokenid starts from zero)
	if (m_lEnterStateEntries.Len() < (_OnEnterStateEntry.m_TokenID+1))
		m_lEnterStateEntries.SetLen(_OnEnterStateEntry.m_TokenID+1);

	m_lEnterStateEntries[_OnEnterStateEntry.m_TokenID] = _OnEnterStateEntry;
}

void CWAGI::ClientConsumeEnterStateEntries(CWAGI_Context* _pContext)
{
	MSCOPE(CWAGI::ClientConsumeEnterStateEntries, WAGI);
	
	int32 Len = m_lEnterStateEntries.Len();
	for (int32 i = 0; i < Len; i++)
	{
		CWAGI_EnterStateEntry& Entry = m_lEnterStateEntries[i];
		if (Entry.m_TokenID != -1)
			m_pEvaluator->AG_OnEnterState(_pContext, Entry.m_TokenID, Entry.m_iState, Entry.m_iEnterStateAction);
	}
}

void CWAGI::ServerClearEnterStateEntries()
{
	m_lEnterStateEntries.SetLen(0);
}

#ifndef M_RTM
	//Get debug info about current state for all tokens
CStr CWAGI::DebugStateInfo(const CWAGI_Context* _pContext)
{
  	CStr Res = CStrF("(iObj: %d, GameTime: %3.2f)", _pContext->m_pObj->m_iObject, _pContext->m_GameTime.GetTime());
 	if (m_lTokens.Len())
	{
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			Res += CStrF("|    Token %d: State ", iToken);
			CWAGI_Token* pToken = &(m_lTokens[iToken]);
			const CXRAG_State* pState = GetState(pToken->GetStateIndex());
			if (pState != NULL)
			{
				int16 iState = pToken->GetStateIndex();
				if (iState != AG_STATEINDEX_NULL)
				{
					if (iState != AG_STATEINDEX_TERMINATE)
					{
						iState &= AG_TARGETSTATE_INDEXMASK;
						CStr StateName = GetStateName(iState);
						if (StateName != "")
							Res += CStrF("'%s' ", StateName.Str());
						Res += CStrF("(iState: %d)", iState);
					}
					else
					{
						Res += CStr("<TERMINATE>");
					}
				}
				else
				{
					Res += CStr("<NULL>");
				}

				const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(pState->GetBaseAnimLayerIndex());
				if (pAnimLayer != NULL)
				{
					int16 iAnim = pAnimLayer->GetAnimIndex();
					Res += CStrF(", iAnim %d", iAnim);
				}
			}
			else
			{
				Res += CStr("<NULL>");
			}
		}
	}
	else
	{
		Res += "|    No Tokens!";
	}
	return Res;
};
#endif


#if	defined(M_RTM) && defined( AG_DEBUG )
#warning "M_RTM and AG_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
