#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAG2I.h"
#include "WAG2I_StateInstPacked.h"
#include "WAG2_ClientData.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"
#include "../WDataRes_AnimGraph2.h"
#include "../WPhysState.h"
#include "../WObjects/WObj_Game.h"
// For debug purposes
//#include "../../../../../Projects/Main/GameClasses/WObj_Char.h"

//#ifndef M_RTM
//#define WAG2I_DEBUG_ENABLE
//#endif

#define	STATEFLAGHI_EXACTSTARTPOSITION	0x00000080
#define	STATEFLAGHI_APPLYTURNCORRECTION	0x00000004
#define ANIMLAYER_MOVELAYERTOEND 0x0040
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CWAG2I::Clear()
{
	MSCOPE(CWAG2I::Clear, WAG2I);

	//ConOutL(CStrF("  CWAG2I::Clear() - pAG2I %X", this));

	m_iRandseed = 0;
	m_DirtyFlag = 0;

	m_lAnimGraph2Res.Clear();
	m_lspAnimGraph2.Clear();

	m_lTokens.Clear();

	m_OverlayAnim.Clear();
	m_OverlayAnimLipSync.Clear();
	m_OverLayAnimLipSyncBaseJoint = 0;
	m_OverlayAnim_StartTime = AG2I_UNDEFINEDTIME;
	m_bDisableAll = false;
	m_bForceRefresh = false;
	m_pEvaluator = NULL;
	m_LastPackMode = -1;
	m_iOverlayKey = 0;
	m_bNeedUpdate = false;
#ifndef M_RTM
	m_bDisableDebug = false;
#endif

	//ConOutL(CStrF("CWAG2I::Clear() - pAG2I %X, iRand %d, iAnimGraph2Res %d, iAnimListRes %d", this, m_iRand, m_iAnimGraph2Res, m_iAnimListRes));
}

//--------------------------------------------------------------------------------

void CWAG2I::CopyFrom(const CWAG2I* _pAG2I)
{
	MSCOPE(CWAG2I::CopyFrom, WAG2I);

	//ConOutL(CStrF("  CWAG2I::CopyFrom(%X) - pAG2I %X", _pAG2I, this));

	m_iRandseed = _pAG2I->m_iRandseed;

	TAP_RCD<const CAG2Res> lOtherRes = _pAG2I->m_lAnimGraph2Res;
	m_lAnimGraph2Res.SetLen(lOtherRes.Len());
	TAP_RCD<CAG2Res> lRes = m_lAnimGraph2Res;
	for (int32 i = 0; i < lOtherRes.Len(); i++)
		lRes[i] = lOtherRes[i];

	m_lspAnimGraph2.QuickSetLen(0);
	
	TAP_RCD<const CWAG2I_Token> lOtherToken = _pAG2I->m_lTokens;
	m_lTokens.SetLen(lOtherToken.Len());
	TAP_RCD<CWAG2I_Token> lToken = m_lTokens;
	for (int iToken = 0; iToken < lOtherToken.Len(); iToken++)
		lToken[iToken].CopyFrom(lOtherToken[iToken],this);

	m_OverlayAnim = _pAG2I->m_OverlayAnim;
	m_OverlayAnimLipSync = _pAG2I->m_OverlayAnimLipSync;
	m_OverLayAnimLipSyncBaseJoint = _pAG2I->m_OverLayAnimLipSyncBaseJoint;
	m_OverlayAnim_StartTime = _pAG2I->m_OverlayAnim_StartTime;
	m_bDisableAll = _pAG2I->m_bDisableAll;
	m_bNeedUpdate = _pAG2I->m_bNeedUpdate;
#ifndef M_RTM
	m_bDisableDebug = _pAG2I->m_bDisableDebug;
#endif

	//ConOutL(CStrF("CWAG2I::CopyFrom(%X) - pAG2I %X, iRand %d, iAnimGraph2Res %d, iAnimListRes %d", _pAG2I, this, m_iRand, m_iAnimGraph2Res, m_iAnimListRes));
}

// Update the animgraph from a server mirror
void CWAG2I::UpdateFromMirror(CWAG2I_Context* _pContext, CWAG2I* _pAG2IMirror)
{
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_RESOURCE)
	{
		int32 Len = _pAG2IMirror->m_lAnimGraph2Res.Len();
		m_lAnimGraph2Res.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			m_lAnimGraph2Res[i] = _pAG2IMirror->m_lAnimGraph2Res[i];

		m_lspAnimGraph2.Clear();
		AcquireAllResourcesToken(_pContext);
	}

	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_OVERLAY)
	{
		m_OverlayAnim = _pAG2IMirror->m_OverlayAnim;
		m_OverlayAnim_StartTime = _pAG2IMirror->m_OverlayAnim_StartTime;
	}
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_OVERLAY_LIPSYNC)
	{
		m_OverlayAnimLipSync = _pAG2IMirror->m_OverlayAnimLipSync;
		m_OverLayAnimLipSyncBaseJoint = _pAG2IMirror->m_OverLayAnimLipSyncBaseJoint;
	}

	// Skip removed tokens for now, shouldn't be needed(?)
	/*if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_TOKENS_REMOVED)
	{
		int8 Len;
		PTR_GETINT8(_pData,Len);
		for (int32 i = 0; i < Len; i++)
		{
			int8 TokenID;
			PTR_GETINT8(_pData, TokenID);

			// Find and remove tokenid (if any)
			int32 nTokens = _pAG2IMirror->m_lTokens.Len();
			for (int32 j = 0; j < nTokens; j++)
			{
				if (_pAG2IMirror->m_lTokens[j].GetID() == TokenID)
				{
					_pAG2IMirror->m_lTokens.Del(j);
					break;
				}
			}
		}
	}*/
	// Remove tokens that shouldn't be here
	int32 Len = _pAG2IMirror->GetNumTokens();
	int32 LenLocal = m_lTokens.Len();
	for (int32 i = 0; i < LenLocal; i++)
	{
		bool bOk = false;
		for (int32 j = 0; j < Len; j++)
		{
			if (m_lTokens[i].GetID() == _pAG2IMirror->m_lTokens[j].GetID())
			{
				bOk = true;
				break;
			}
		}
		if (!bOk)
		{
			m_lTokens.Del(i);
			LenLocal = m_lTokens.Len();
			i--;
		}
	}

	// Go through all tokens, and make sure they are up to date

	for (int32 i = 0; i < Len; i++)
	{
		CWAG2I_Token* pToken = GetTokenFromID(_pAG2IMirror->m_lTokens[i].GetID(),true);
		pToken->UpdateFromMirror(_pContext,&_pAG2IMirror->m_lTokens[i]);
	}

	_pAG2IMirror->m_DirtyFlag = 0;
}

void CWAG2I::SetEvaluator(CWO_ClientData_AnimGraph2Interface* _pEvaluator)
{ 
	m_pEvaluator = _pEvaluator;
}

// Search for matching animgraph name
CAG2AnimGraphID CWAG2I::GetAnimGraphIDFromNameHash(int32 _Hash) const
{
	int32 Len = m_lAnimGraph2Res.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_lAnimGraph2Res[i].m_NameHash == _Hash)
			return i;
	}

	return -1;
}
//--------------------------------------------------------------------------------

uint32 CWAG2I::GetTokenStateFlags(const CWAG2I_Context *_pContext, int8 _TokenID, uint8 _iFlags) const
{
	MSCOPE(CWAG2I::GetTokenStateFlags, WAG2I);

	const CWAG2I_Token* pToken = GetTokenFromID(_TokenID);
	if (!pToken)
		return 0;

	const CWAG2I_StateInstance* pStateInstance = pToken->GetTokenStateInstance();
	if (pStateInstance)
		return GetState(pStateInstance->GetStateIndex(),pStateInstance->GetAnimGraphIndex())->GetFlags(_iFlags);

	return 0;
}

//--------------------------------------------------------------------------------

uint32 CWAG2I::GetStateFlags(const CXRAG2_State* _pState, uint8 _iFlags) const
{
	MSCOPE(CWAG2I::GetTokenStateFlags, WAG2I);

	if (_pState == NULL)
		return 0;

	return _pState->GetFlags(_iFlags);
}

//--------------------------------------------------------------------------------

bool CWAG2I::GetTokenStateConstantValue(const CWAG2I_Context *_pContext, int8 _TokenID, CAG2AnimGraphID _iAnimGraph, uint16 _StateConstantID, fp32& _Value) const
{
	MSCOPE(CWAG2I::GetTokenStateConstantValue, WAG2I);

	const CWAG2I_Token* pToken = GetTokenFromID(_TokenID);
	if (pToken == NULL)
		return false;

	return GetStateConstantValue(pToken->GetStateIndex(), _StateConstantID, _iAnimGraph, _Value);
}

//--------------------------------------------------------------------------------

fp32 CWAG2I::GetTokenStateConstantValueDef(const CWAG2I_Context *_pContext, int8 _TokenID, CAG2AnimGraphID _iAnimGraph, uint16 _StateConstantID, fp32 _DefaultValue) const
{
	MSCOPE(CWAG2I::GetTokenStateConstantValueDef, WAG2I);

	fp32 Result;
	if (GetTokenStateConstantValue(_pContext, _TokenID, _iAnimGraph, _StateConstantID, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

bool CWAG2I::GetStateConstantValue(CAG2StateIndex _iState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32& _Value) const
{
	MSCOPE(CWAG2I::GetStateConstantValue, WAG2I);

	const CXRAG2_State* pState = GetState(_iState,_iAnimGraph);
	if (pState == NULL)
		return false;

	const CXRAG2* pAG2 = GetAnimGraph(_iAnimGraph);
	for (int jStateConstant = 0; jStateConstant < pState->GetNumConstants(); jStateConstant++)
	{
		int16 iStateConstant = pState->GetBaseConstantIndex() + jStateConstant;
		const CXRAG2_StateConstant* pStateConstant = pAG2->GetStateConstant(iStateConstant);
		if (pStateConstant->m_ID == _StateConstantID)
		{
			_Value = pStateConstant->m_Value;
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

fp32 CWAG2I::GetStateConstantValueDef(CAG2StateIndex _iState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32 _DefaultValue) const
{
	MSCOPE(CWAG2I::GetStateConstantValueDef, WAG2I);

	fp32 Result = 0.0f;
	if (GetStateConstantValue(_iState, _StateConstantID, _iAnimGraph, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

bool CWAG2I::GetStateConstantValue(const CXRAG2_State* _pState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32& _Value) const
{
	MSCOPE(CWAG2I::GetStateConstantValue, WAG2I);

	if (_pState == NULL)
		return false;


	const CXRAG2* pAG = GetAnimGraph(_iAnimGraph);
	int32 NumConstants = _pState->GetNumConstants();
	for (int jStateConstant = 0; jStateConstant < NumConstants; jStateConstant++)
	{
		int16 iStateConstant = _pState->GetBaseConstantIndex() + jStateConstant;
		const CXRAG2_StateConstant* pStateConstant = pAG->GetStateConstant(iStateConstant);
		if (pStateConstant->m_ID == _StateConstantID)
		{
			_Value = pStateConstant->m_Value;
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

fp32 CWAG2I::GetStateConstantValueDef(const CXRAG2_State* _pState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32 _DefaultValue) const
{
	MSCOPE(CWAG2I::GetStateConstantValueDef, WAG2I);

	fp32 Result = 0.0f;
	if (GetStateConstantValue(_pState, _StateConstantID, _iAnimGraph, Result))
		return Result;

	return _DefaultValue;
}

//--------------------------------------------------------------------------------

CAG2StateIndex CWAG2I::GetAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAG2I::GetAnimLayers, WAG2I);

	AcquireAllResources(_pContext);
/*
	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AG2IContext);

	if (_nLayers <= 0)
	{
		_nLayers = 0;
		return -1;
	}
	
	int32 NumOverlay = 0;
	{
		MSCOPESHORT(OverlayAnim);
		// OverlayAnim
		if (m_OverlayAnim.IsValid() && (m_OverlayAnim_StartTime.Compare(AG2I_UNDEFINEDTIME) != 0))
		{
			CMTime ContinousTime = _pContext->m_GameTime - m_OverlayAnim_StartTime;
			spCXR_Anim_SequenceData pOverlayAnimSeq = m_OverlayAnim.GetAnimSequenceData(_pContext->m_pWorldData);
			if (pOverlayAnimSeq != NULL)
			{
				CMTime LoopedTime = pOverlayAnimSeq->GetLoopedTime(ContinousTime);

				fp32 LayerTimeScale = 1.0f;
				fp32 LayerBlend = 1.0f;
				uint8 iLayerBaseJoint = 0;
				uint32 LayerFlags = 0;

				_pLayers[0].Create3(pOverlayAnimSeq, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
				_pLayers[0].m_ContinousTime = ContinousTime;
	#ifdef WAG2I_DEBUG_ENABLE
				_pLayers[0].m_DebugMessage = CStrF("(Overlay) iRes %d, iAnim %d, AnimStartTime %3.3f, GameTime %3.3f, LoopedAnimTime %3.3f (ContinuousAnimTime %3.3f)",
												  m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq, 
												m_OverlayAnim_StartTime, _pContext->m_GameTime, LoopedTime, ContinousTime);
	#endif
				M_ASSERT(0x7fc00000 != (uint32&)_pLayers[0].m_Time, "!");
				NumOverlay = 1;
			}
			else
			{
#ifdef	AG2_DEBUG
				ConOut(CStrF("Invalid OverlayAnim (iAnimContainerResource %d, iAnimSeq %d).", m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq));
#endif
				_nLayers = 0;
			}

			spCXR_Anim_SequenceData pOverlayAnimSeqLipSync = m_OverlayAnimLipSync.GetAnimSequenceData(_pContext->m_pWorldData);
			if (pOverlayAnimSeqLipSync)
			{
				CMTime LoopedTime = pOverlayAnimSeqLipSync->GetLoopedTime(ContinousTime);
				fp32 LayerTimeScale = 1.0f;
				fp32 LayerBlend = 1.0f;
				uint8 iLayerBaseJoint = m_OverLayAnimLipSyncBaseJoint;
				uint32 LayerFlags = CXR_ANIMLAYER_ADDITIVEBLEND;

				_pLayers[_nLayers].Create3(pOverlayAnimSeqLipSync, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
				_pLayers[_nLayers].m_ContinousTime = ContinousTime;
#ifdef WAG2I_DEBUG_ENABLE
				_pLayers[_nLayers].m_DebugMessage = CStrF("(Overlay) iRes %d, iAnim %d, AnimStartTime %3.3f, GameTime %3.3f, LoopedAnimTime %3.3f (ContinuousAnimTime %3.3f)",
					m_OverlayAnimLipSync.m_iAnimContainerResource, m_OverlayAnimLipSync.m_iAnimSeq, 
					m_OverlayAnim_StartTime, _pContext->m_GameTime, LoopedTime, ContinousTime);
#endif
				M_ASSERT(0x7fc00000 != (uint32&)_pLayers[_nLayers].m_Time, "!");
				NumOverlay++;
			}
		}
	}
/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
	{
		//ConOutL(CStrF("CWAG2I::GetAnimLayers() - pAG2I = %X, iRand %d - FAILED!", this, m_iRand));
		_nLayers = 0;
		return;
	}
*/
	const int32 MaxLayers = 32;
	CXR_AnimLayer pTempLayers[MaxLayers];
	CXR_AnimLayer* pLayers;
	int nMaxLayers = _nLayers;
	CAG2StateIndex iPerfectState = -1;
	_nLayers = 0;
	
	{
		MSCOPESHORT(TokenLayers);
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			if (_nLayers >= MaxLayers)
				break;

			CWAG2I_Token* pToken = &(m_lTokens[iToken]);

			pLayers = &(pTempLayers[_nLayers]);

			bool bAllowBlend = _nLayers > 0;
			int nTokenLayers = MaxLayers - _nLayers;
			CAG2StateIndex iTokenPerfectState = pToken->GetAnimLayers(_pContext, pLayers, nTokenLayers, _iDisableStateInstanceAnimsCallbackMsg, bAllowBlend);
			if (iPerfectState == -1)
				iPerfectState = iTokenPerfectState;
			_nLayers += nTokenLayers;
		}
	}
	if (NumOverlay)
	{
		for (int32 i = 0; i < _nLayers; i++)
		{
			 if (pTempLayers[i].m_Flags & CXR_ANIMLAYER_FORCEAFTERVOCAP)
				 _pLayers[NumOverlay++] = pTempLayers[i];
		}
		_nLayers = NumOverlay;
		return -1;
	}
	// Make sure we have full blended layer
	CXR_AnimLayer TempEndLayers[8];
	int32 NumTempEndLayers = 0;
	bool bGotFullBlend = false;
	int32 iLastFull = 999;
	int32 i16 = -1;
	int32 i21 = -1;
	for (int32 i = _nLayers - 1; i >= 0; i--)
	{
		if (pTempLayers[i].m_Flags & ANIMLAYER_MOVELAYERTOEND)
		{
			// Remove "freeze" flag..
			pTempLayers[i].m_Flags &= ~ANIMLAYER_MOVELAYERTOEND;
			TempEndLayers[NumTempEndLayers++] = pTempLayers[i];
			// Don't add layer
			pTempLayers[i].m_Blend = 0.0f;
			continue;
		}
		if (pTempLayers[i].m_Flags & CXR_ANIMLAYER_REMOVEPREVNODELAYERS)
		{
			if (i16 == -1 && pTempLayers[i].m_iBlendBaseNode == 16)
			{
				i16 = i;
				pTempLayers[i].m_Blend = 1.0f;
			}
			if (i21 == -1 && pTempLayers[i].m_iBlendBaseNode == 21)
			{
				i21 = i;
				pTempLayers[i].m_Blend = 1.0f;
			}
		}
		if (!bGotFullBlend && !pTempLayers[i].m_iBlendBaseNode)
		{
			iLastFull = Min(i,iLastFull);
			if (pTempLayers[i].m_Blend > 0.999f)
			{
				bGotFullBlend = true;
				continue;
			}
		}
		if (bGotFullBlend)
		{
			// Remove this layer
			pTempLayers[i].m_Blend = 0.0f;
		}
	}
	
	if (!bGotFullBlend && (iLastFull < _nLayers))
		pTempLayers[iLastFull].m_Blend = 1.0f;

	int32 nTotalLayers = 0;
	for (int32 i = 0; i < _nLayers; i++)
	{
		if (pTempLayers[i].m_Blend > 0.0f)
		{
			if (pTempLayers[i].m_Flags & CXR_ANIMLAYER_REMOVEPREVNODELAYERS)
			{
				if (pTempLayers[i].m_iBlendBaseNode == 16 && i == i16)
				{
					_pLayers[nTotalLayers++] = pTempLayers[i];
				}
				else if (pTempLayers[i].m_iBlendBaseNode == 21 && i == i21)
				{
					_pLayers[nTotalLayers++] = pTempLayers[i];
				}
			}
			else
			{
				_pLayers[nTotalLayers++] = pTempLayers[i];
				if (nTotalLayers >= nMaxLayers)
					break;
			}
		}
	}
	for (int32 i = 0; i < NumTempEndLayers; i++)
	{
		if (nTotalLayers >= nMaxLayers)
			break;
		_pLayers[nTotalLayers++] = TempEndLayers[i];
	}
	_nLayers = nTotalLayers;

#ifdef WAG2I_DEBUG_ENABLE
	if (CWAG2I::DebugEnabled(_pContext))
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

#ifdef WAG2I_DEBUG_ENABLE
	if ((_nLayers == 0) && (m_lTokens.Len() > 0))
	{
		if (CWAG2I::DebugEnabled(_pContext))
		{
			if (_pContext->m_pWPhysState->IsServer())
				ConOutL(CStrF("(Server) iObj %d (%X) CWAG2I::GetAnimLayers() - nLayers = %d", _pContext->m_pObj->m_iObject, _pContext->m_pObj, _nLayers));
			else
				ConOutL(CStrF("(Client) iObj %d (%X) CWAG2I::GetAnimLayers() - nLayers = %d", _pContext->m_pObj->m_iObject, _pContext->m_pObj, _nLayers));
		}
	}
#endif
//	UnacquireAllResources();
	return iPerfectState;
}

void CWAG2I::GetValueCompareLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int32 _Value)
{
	MSCOPE(CWAG2I::GetAnimLayers, WAG2I);

	AcquireAllResources(_pContext);
	if (_nLayers <= 0)
	{
		_nLayers = 0;
		return;
	}

	const int32 MaxLayers = 32;
	CXR_AnimLayer pTempLayers[MaxLayers];
	CXR_AnimLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;

	{
		MSCOPESHORT(TokenLayers);
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			if (_nLayers >= MaxLayers)
				break;

			CWAG2I_Token* pToken = &(m_lTokens[iToken]);

			pLayers = &(pTempLayers[_nLayers]);

			int nTokenLayers = MaxLayers - _nLayers;
			pToken->GetValueCompareLayers(_pContext, pLayers, nTokenLayers, _Value);
			_nLayers += nTokenLayers;
		}
	}

	bool bGotFullBlend = false;
	int32 iLastFull = 999;
	for (int32 i = _nLayers - 1; i >= 0; i--)
	{
		if (!bGotFullBlend && !pTempLayers[i].m_iBlendBaseNode)
		{
			iLastFull = Min(i,iLastFull);
			if (pTempLayers[i].m_Blend > 0.999f)
			{
				bGotFullBlend = true;
				continue;
			}
		}
		if (bGotFullBlend)
		{
			// Remove this layer
			pTempLayers[i].m_Blend = 0.0f;
		}
	}

	if (!bGotFullBlend && (iLastFull < _nLayers))
		pTempLayers[iLastFull].m_Blend = 1.0f;
	int32 nTotalLayers = 0;
	for (int32 i = 0; i < _nLayers; i++)
	{
		if (pTempLayers[i].m_Blend > 0.0f)
		{
			_pLayers[nTotalLayers++] = pTempLayers[i];
			if (nTotalLayers >= nMaxLayers)
				break;
		}
	}
	_nLayers = nTotalLayers;
}

void CWAG2I::GetTopEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int& _nLayers)
{
	MSCOPE(CWAG2I::GetTopAnimLayers, WAG2I);

	AcquireAllResources(_pContext);

	if (_nLayers <= 0)
	{
		_nLayers = 0;
		return;
	}

	CEventLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;

	if (m_OverlayAnim.IsValid() && (m_OverlayAnim_StartTime.Compare(AG2I_UNDEFINEDTIME) != 0))
	{
		CMTime ContinousTime = _pContext->m_GameTime - m_OverlayAnim_StartTime;
		spCXR_Anim_SequenceData pOverlayAnimSeq = m_OverlayAnim.GetAnimSequenceData(_pContext->m_pWorldData);
		if (pOverlayAnimSeq != NULL)
		{
			CMTime LoopedTime = pOverlayAnimSeq->GetLoopedTime(ContinousTime);

			fp32 LayerTimeScale = 1.0f;
			fp32 LayerBlend = 1.0f;
			uint8 iLayerBaseJoint = 0;
			uint32 LayerFlags = 0;

			_pLayers[0].m_Layer.Create3(pOverlayAnimSeq, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
			_pLayers[0].m_Layer.m_ContinousTime = ContinousTime;
			_pLayers[0].m_pKey = &m_iOverlayKey;
			_nLayers = 1;
			return;
		}
	}

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		if (_nLayers >= nMaxLayers)
			break;

		CWAG2I_Token* pToken = &(m_lTokens[iToken]);

		pLayers = &(_pLayers[_nLayers]);

		bool bAllowBlend = _nLayers > 0;
		int nTokenLayers = nMaxLayers - _nLayers;
		pToken->GetTopEventLayers(_pContext, pLayers, nTokenLayers, bAllowBlend);
		for (int32 i = 0; i < nTokenLayers; i++)
			_pTokenIDs[_nLayers + i] = pToken->GetID();
		_nLayers += nTokenLayers;
	}
	//	UnacquireAllResources();
}

void CWAG2I::GetEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int& _nLayers)
{
	MSCOPE(CWAG2I::GetAllAnimLayers, WAG2I);

	if (_nLayers <= 0)
	{
		_nLayers = 0;
		return;
	}

	CEventLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;

	if (m_OverlayAnim.IsValid() && (m_OverlayAnim_StartTime.Compare(AG2I_UNDEFINEDTIME) != 0))
	{
		CMTime ContinousTime = _pContext->m_GameTime - m_OverlayAnim_StartTime;
		spCXR_Anim_SequenceData pOverlayAnimSeq = m_OverlayAnim.GetAnimSequenceData(_pContext->m_pWorldData);
		if (pOverlayAnimSeq != NULL)
		{
			CMTime LoopedTime = pOverlayAnimSeq->GetLoopedTime(ContinousTime);

			fp32 LayerTimeScale = 1.0f;
			fp32 LayerBlend = 1.0f;
			uint8 iLayerBaseJoint = 0;
			uint32 LayerFlags = 0;

			_pLayers[0].m_Layer.Create3(pOverlayAnimSeq, LoopedTime, LayerTimeScale, LayerBlend, iLayerBaseJoint, LayerFlags);
			_pLayers[0].m_Layer.m_ContinousTime = ContinousTime;
			_pLayers[0].m_pKey = &m_iOverlayKey;
			_nLayers = 1;
			return;
		}
	}

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		if (_nLayers >= nMaxLayers)
			break;

		CWAG2I_Token* pToken = &(m_lTokens[iToken]);

		pLayers = &(_pLayers[_nLayers]);

		bool bAllowBlend = _nLayers > 0;
		int nTokenLayers = nMaxLayers - _nLayers;
		pToken->GetEventLayers(_pContext, pLayers, nTokenLayers, bAllowBlend);
		for (int32 i = 0; i < nTokenLayers; i++)
			_pTokenIDs[_nLayers + i] = pToken->GetID();
		_nLayers += nTokenLayers;
	}
	//	UnacquireAllResources();
}

void CWAG2I::GetAnimLayersFromToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAG2I::GetAnimLayers, WAG2I);

	AcquireAllResourcesToken(_pContext);
	/*
	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);
	*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AG2IContext);

	const CWAG2I_Token* pToken = GetTokenFromID(_iToken);
	if (_nLayers <= 0 || !pToken)
	{
		_nLayers = 0;
		return;
	}

	const int32 MaxLayers = 16;
	CXR_AnimLayer pTempLayers[MaxLayers];
	CXR_AnimLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;

	{
		MSCOPESHORT(TokenLayers);
		pLayers = &(pTempLayers[_nLayers]);

		bool bAllowBlend = _nLayers > 0;
		int nTokenLayers = MaxLayers - _nLayers;
		pToken->GetAnimLayers(_pContext, pLayers, nTokenLayers, _iDisableStateInstanceAnimsCallbackMsg, bAllowBlend);
		_nLayers += nTokenLayers;
	}

	// Make sure we have full blended layer
	CXR_AnimLayer TempEndLayers[8];
	int32 NumTempEndLayers = 0;
	bool bGotFullBlend = false;
	int32 iLastFull = 999;
	for (int32 i = _nLayers - 1; i >= 0; i--)
	{
		if (!bGotFullBlend && !pTempLayers[i].m_iBlendBaseNode)
		{
			iLastFull = Min(i,iLastFull);
			if (pTempLayers[i].m_Blend > 0.999f)
			{
				bGotFullBlend = true;
				continue;
			}
		}
		if (bGotFullBlend)
		{
			// Remove this layer
			pTempLayers[i].m_Blend = 0.0f;
		}
	}

	if (!bGotFullBlend && (iLastFull < _nLayers))
		pTempLayers[iLastFull].m_Blend = 1.0f;

	int32 nTotalLayers = 0;
	for (int32 i = 0; i < _nLayers; i++)
	{
		if (pTempLayers[i].m_Blend > 0.0f)
		{
			_pLayers[nTotalLayers++] = pTempLayers[i];
			if (nTotalLayers >= nMaxLayers)
				break;
		}
	}
	_nLayers = nTotalLayers;
}

bool CWAG2I::GetAnimLayerFromState(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CXR_AnimLayer& _Layer, CAG2StateIndex _iState)
{
	MSCOPE(CWAG2I::GetAnimLayersFromState, WAG2I);

	AcquireAllResources(_pContext);

	CWAG2I_Token* pToken = GetTokenFromID(_iToken);
	if (!pToken)
		return false;

	{
		MSCOPESHORT(TokenLayers);
		// Go through stateinstances
		
		return pToken->GetAnimLayerFromState(_pContext, _Layer, _iState);
	}
}

void CWAG2I::GetTopEventLayersFromToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CEventLayer* _pLayers, int& _nLayers)
{
	MSCOPE(CWAG2I::GetAnimLayers, WAG2I);

	AcquireAllResources(_pContext);
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AG2IContext);

	CWAG2I_Token* pToken = GetTokenFromID(_iToken);
	if (_nLayers <= 0 || !pToken)
	{
		_nLayers = 0;
		return;
	}

	CEventLayer* pLayers;
	int nMaxLayers = _nLayers;
	_nLayers = 0;

	{
		MSCOPESHORT(TokenLayers);
		pLayers = &(_pLayers[_nLayers]);

		bool bAllowBlend = _nLayers > 0;
		int nTokenLayers = nMaxLayers - _nLayers;
		pToken->GetTopEventLayers(_pContext, pLayers, nTokenLayers, bAllowBlend);
		_nLayers += nTokenLayers;
	}
}

bool CWAG2I::GetSpecificAnimLayer(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iToken, int32 _iAnim, int32 _StartTick) const
{
	MSCOPE(CWAG2I::GetSpecificAnimLayer, WAG2I);
	// Find specific animlayer in given token
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAG2I_Token* pToken = &(m_lTokens[iToken]);

		if (pToken->GetID() == _iToken)
			return pToken->GetSpecificAnimLayer(_pContext, _Layer, _iAnim, _StartTick);
	}

	return false;
}

//--------------------------------------------------------------------------------

int32 CWAG2I::GetAnimVelocity(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAG2I::GetAnimVelocity, WAG2I);

/*
	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AG2IContext);

/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
		return false;
*/

	const CVec3Dfp32& UpVector = m_pEvaluator->GetUpVector();

	CMat4Dfp32 ObjMatrix;
	if(_pContext->m_pObj)
	{
		ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
		ObjMatrix.GetRow(2) = UpVector;
		ObjMatrix.RecreateMatrix(2, 0);
	}
	else
		ObjMatrix.Unit();

	_MoveVelocity = 0;
	_RotVelocity.Unit();

	CWAG2I_Context OverlayContext;
	if (m_OverlayAnim.IsValid())
	{
		CXR_Anim_SequenceData* pSeq = m_OverlayAnim.GetAnimSequenceData_MapData(_pContext->m_pWPhysState->GetMapData());
		// Ok then, similar to GetAnimVelocityFromDestination
		// Start position (in destination...)
		CMat4Dfp32 StartPos = m_pEvaluator->GetDestination();
		if (!pSeq || StartPos.GetRow(3) == CVec3Dfp32(_FP32_MAX))
			return 0;

		CMat4Dfp32 CurrentPos = _pContext->m_pObj->GetPositionMatrix();
		CurrentPos.GetRow(2) = m_pEvaluator->GetUpVector();
		CurrentPos.RecreateMatrix(2,0);
		CVec3Dfp32 dMoveCurrent = CurrentPos.GetRow(3) - StartPos.GetRow(3);
		CQuatfp32 CurrentRot, StartRot,dRotCurrent;
		CurrentRot.Create(CurrentPos);
		StartRot.Create(StartPos);
		dRotCurrent = StartRot;
		dRotCurrent.Inverse();
		dRotCurrent = CurrentRot * dRotCurrent;
		dRotCurrent.Inverse();
		
		// Find total movement from this time to endtime
		VecUnion AnimPosNext;
		CQuatfp32 AnimRotNext;
		//	fp32 LayerDuration = Layer.m_spSequence->GetDuration();
		CMTime TimeA = CMTime::CreateFromSeconds((_pContext->m_GameTime - m_OverlayAnim_StartTime).GetTime() + _pContext->m_TimeSpan);
		// Assume end time is correct for the state (no early outings..)
		pSeq->EvalTrack0(TimeA, AnimPosNext.v128, AnimRotNext);

		// Calculate relative movement/rotation to end of animation
		AnimPosNext.v128 = M_VMulMat3x3(AnimPosNext.v128, StartPos);

		_MoveVelocity = AnimPosNext.v3 - dMoveCurrent;
		_RotVelocity = AnimRotNext * dRotCurrent;
		return 2;
	}

	CXR_AnimLayer pLayers[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	CAG2StateIndex iPerfectState = GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);
	// Perfect state override
	if (iPerfectState != -1)
	{
		const CXRAG2_State* pState = GetState(iPerfectState,0);
		if (pState)
		{
			if (!(pState->GetFlags(0) & STATEFLAGHI_APPLYTURNCORRECTION))
			{
				if (pState->GetFlags(1) & STATEFLAGHI_EXACTSTARTPOSITION)
					GetAnimVelocityFromDestination(_pContext,_MoveVelocity,_RotVelocity,iPerfectState);
				else
					GetAnimVelocityToDestination(_pContext,_MoveVelocity,_RotVelocity,iPerfectState);
				return 2;
			}
		}
	}
	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
#ifdef	AG2_DEBUG
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;
#endif

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if ((Layer.m_iBlendBaseNode > 1) || (Layer.m_Flags & CXR_ANIMLAYER_LAYERNOVELOCITY))
			continue;

		// Calculate absolute positions.
		vec128 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
		fp32 TimeSpan = _pContext->m_TimeSpan;
		CMTime Diff = TimeA - CMTime::CreateFromSeconds(TimeSpan) - CMTime::CreateFromSeconds(Layer.m_TimeOffset);
		// Hack to compensate for missed velocity
		/*if (Diff.Compare(CMTime::CreateFromSeconds(0.01f)) < 0)
		{
			// Must catch up to our own timeframe
			TimeSpan += TimeA.GetTime() - Layer.m_TimeOffset;
			TimeA = CMTime::CreateFromSeconds(Layer.m_TimeOffset);
		}*/
		Layer.m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		vec128 MoveB;
		CQuatfp32 RotB;	
		CMTime TimeB = Layer.m_spSequence->GetLoopedTime(TimeA + CMTime::CreateFromSeconds(TimeSpan * Layer.m_TimeScale));
		Layer.m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		VecUnion dMove;
		CQuatfp32 dRot;
		dMove.v128 = M_VSub(MoveB, MoveA);
		dRot = RotA;
		dRot.Inverse();
		dRot = RotB * dRot;

		// Compensate for moving while rotating.
		CQuatfp32 InvRotA;
		CMat4Dfp32 MatA;
		MatA.Unit();
		RotA.CreateMatrix3x3(MatA);
		MatA.GetRow(2) = UpVector;
		MatA.RecreateMatrix(2,0);
		CMat4Dfp32 InvMatA;
		MatA.InverseOrthogonal(InvMatA);
//		InvRotA = RotA; InvRotA.Inverse();
	//	InvRotA.CreateMatrix3x3(InvMatA);
		dMove.v128 = M_VMulMat3x3(dMove.v128,InvMatA);

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
			vec128 LoopMove;
			CQuatfp32 LoopRot;
			Layer.m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dMove.v128 = M_VSetW1(M_VAdd(dMove.v128,LoopMove));
			dRot *= LoopRot;
		}

		fp32 BlendFactor = Layer.m_Blend;

		// Convert into Object direction.
		dMove.v128 = M_VMulMat3x3(dMove.v128, ObjMatrix);

		// Just use this layers velocity
		if (Layer.m_Flags & CXR_ANIMLAYER_FORCELAYERVELOCITY)
		{
			_MoveVelocity = dMove.v3;
			_RotVelocity = dRot;
			break;
		}

		// Weight Move using layer blendfactor.
		CVec3Dfp32 TempVelo;
		_MoveVelocity.Lerp(dMove.v3, BlendFactor, TempVelo);
		_MoveVelocity = TempVelo;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		_RotVelocity.Lerp(dRot, BlendFactor, TempRot);
		_RotVelocity = TempRot;
	}

//	UnacquireAllResources();
	return 1;
}

//--------------------------------------------------------------------------------
// TEMP REMOVE (ONLY FOR DEBUG OUTPUT!!!)
//#include "../../../../../Projects/Main/GameClasses/WObj_Char.h"

bool CWAG2I::GetAnimVelocityToDestination(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVel, CAG2StateIndex _iState)
{
	// Assumes animations start at 0,0,0
	CWAG2I_Token* pToken = GetTokenFromID(0);
	if (!pToken)
		return false;
	const CXRAG2_State* pState = GetState(_iState,pToken->GetAnimGraphIndex());
	if (!pState)
		return false;

	fp32 AdjustmentOffset = m_pEvaluator->GetAdjustmentOffset();
	fp32 AdjustmentCutOff = m_pEvaluator->GetAdjustmentCutOff();
	// Target position
	CMat4Dfp32 DestPos = m_pEvaluator->GetDestination();
	if (DestPos.GetRow(3) == CVec3Dfp32(_FP32_MAX))
		return false;

	// Take up from dest.. or start...?
	CVec3Dfp32 UpVector = DestPos.GetRow(2);
	CMat4Dfp32 StartPos = m_pEvaluator->GetStartPosition();

	CMat4Dfp32 CurrentPos = _pContext->m_pObj->GetPositionMatrix();
	vec128 DeltaPos = M_VSub(DestPos.r[3], CurrentPos.r[3]);
	CQuatfp32 CurrentRot, TargetRot,DeltaRot,qTemp;
	/*CurrentPos.GetRow(2) = UpVector;
	CurrentPos.RecreateMatrix(2, 0);*/
	/*StartPos.GetRow(2) = UpVector;
	StartPos.RecreateMatrix(2, 0);*/

	CurrentRot.Create(CurrentPos);
	DeltaRot = CurrentRot;
	DeltaRot.Inverse();
	TargetRot.Create(DestPos);
	DeltaRot = TargetRot * DeltaRot;

	/*{
		_pContext->m_pWPhysState->Debug_RenderMatrix(CurrentPos,15.0f);
		_pContext->m_pWPhysState->Debug_RenderMatrix(DestPos,15.0f);
		//_pContext->m_pWPhysState->Debug_RenderWire(CurrentPos.GetRow(3) + CVec3Dfp32(0,0,50),CurrentPos.GetRow(3) + CVec3Dfp32(0,0,50) + DestDir * 150.0f,0xff7f7f7f,15.0f);
		_pContext->m_pWPhysState->Debug_RenderWire(CVec3Dfp32(0,0,150),DestPos.GetRow(3),0xff7f7f7f,15.0f);
	}*/

	// Calculate from current time to the end how much we will be moving, correlate with current position
	// and blend in the difference...

 	_MoveVelocity = 0.0f;
	_RotVel.Unit();

	CXR_AnimLayer Layer;
	// Only get the layer that has the flag set...
	if (!pToken->GetAnimLayerFromState(_pContext, Layer, _iState) || Layer.m_iBlendBaseNode > 1)
		return false;

	// Find total movement from this time to endtime
	VecUnion AnimPosNextTick;
	vec128 AnimPosCurrent,AnimPosEnd;
	fp32 LayerDuration = Layer.m_spSequence->GetDuration();
	if (AdjustmentCutOff == 0.0f)
		AdjustmentCutOff = LayerDuration;


	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
	CMTime TimeB = CMTime::CreateFromSeconds(Min(Layer.m_Time + _pContext->m_TimeSpan* Layer.m_TimeScale,LayerDuration));
	CMTime TimeEnd = CMTime::CreateFromSeconds(LayerDuration);
	// Assume end time is correct for the state (no early outings..)
	CQuatfp32 AnimRotCurrent,AnimRotNextTick;
	AnimPosCurrent = AnimPosNextTick.v128 = AnimPosEnd = M_VConst(0,0,0,1.0f);
	AnimRotCurrent.Unit();
	AnimRotNextTick.Unit();
	// test remove anim movement, just compensation
	//AnimRotCurrent.Unit();
	//AnimRotNextTick.Unit();
	//AnimRotEnd.Unit();
	// Calculate relative movement/rotation to end of animation
	vec128 AnimMoveToEnd;
	//CQuatfp32 AnimRotToEnd;
	AnimMoveToEnd = M_VSub(AnimPosEnd, AnimPosCurrent);
	/*AnimRotToEnd = AnimRotCurrent;
	AnimRotToEnd.Inverse();
	AnimRotToEnd = AnimRotEnd * AnimRotToEnd;
	AnimRotToEnd.Normalize();*/


	//CQuatfp32 ExtraRotToEnd;
	//CQuatfp32 ExtraRotNextFrame;

	vec128 ExtraMove;
	vec128 ExtraMoveNextFrame;
	
	CMat4Dfp32 InvPrevRot;
	InvPrevRot.Unit();
    // Ok, new rule, all perfect dest anims should have "blend" rotation physics
	if (AdjustmentOffset <= Layer.m_Time && Layer.m_Time < AdjustmentCutOff)
	{
		if (!(pState->GetFlags(1) & 0x00000400))
		{
			CQuatfp32 AnimRotEnd;
			Layer.m_spSequence->EvalTrack0(TimeA, AnimPosCurrent, AnimRotCurrent);
			Layer.m_spSequence->EvalTrack0(TimeB, AnimPosNextTick.v128, AnimRotNextTick);
			Layer.m_spSequence->EvalTrack0(TimeEnd, AnimPosEnd, AnimRotEnd);
		}
		if(AdjustmentOffset != 0.0f)
		{
			vec128 OffsetPos;
			CQuatfp32 OffsetRot;
			Layer.m_spSequence->EvalTrack0(CMTime::CreateFromSeconds(AdjustmentOffset), OffsetPos, OffsetRot);
			CMat4Dfp32 Offset;
			Offset.Create(OffsetRot, OffsetPos);
			CMat4Dfp32 Res;
			Offset.Multiply(StartPos, Res);
			////_pContext->m_pWPhysState->Debug_RenderMatrix(StartPos, 0.05f, false, 0xffffffff, 0xffffffff, 0xffffffff);
			StartPos = Res;
			//_pContext->m_pWPhysState->Debug_RenderMatrix(StartPos, 0.05f, false, 0xffff0000, 0xffff0000, 0xffff0000);
			//_pContext->m_pWPhysState->Debug_RenderMatrix(DestPos, 0.05f, false, 0xff00ff00, 0xff00ff00, 0xff00ff00);

		}
		//_pContext->m_pWPhysState->Debug_RenderMatrix(StartPos,20.0f,false,0xffff0000,0xffff0000,0xffff0000);
		// Rotation from start to current (so we know how much to adjust previous movement from...)
		CQuatfp32 StartCurrent;
		StartCurrent.Create(StartPos);
		StartCurrent.Inverse();
		StartCurrent = CurrentRot * StartCurrent;
		// Amount of total extra rotation we have to blend in (end - start - animrot)
		// Compensate for moving while rotating
		qTemp = StartCurrent;
		qTemp.Normalize();
		qTemp.CreateMatrix3x3(InvPrevRot);
		CMat4Dfp32 InvMatA;
		InvPrevRot.InverseOrthogonal(InvMatA);
		// Rot from startpos to current...?
		InvMatA.Multiply3x3(CurrentPos,InvPrevRot);

		// Adjust from current to end with the time we have left...
		fp32 TimeLeft = (AdjustmentCutOff - Layer.m_Time) * Layer.m_TimeScale;
		fp32 dT = TimeLeft > 0.0f ? Min(1.0f, (_pContext->m_TimeSpan * Layer.m_TimeScale) / TimeLeft) : 1.0f;
		CQuatfp32 StartRot,DestRot,Meep;
		CMat4Dfp32 DeltaMat;
		StartRot.Create(StartPos);
		DestRot.Create(DestPos);
		fp32 RotTime = Max(0.0f,Min(1.0f,(Layer.m_Time - AdjustmentOffset + _pContext->m_TimeSpan * Layer.m_TimeScale /*+ 0.01f*/) / (AdjustmentCutOff - AdjustmentOffset)));
		//StartRot.Interpolate(DestRot,Meep,Min(1.0f,Max(0.0f,Layer.m_Time / (LayerDuration)));
		StartRot.Lerp(DestRot,RotTime,Meep);
		Meep.CreateMatrix(DeltaMat);
		_pContext->m_pWPhysState->Object_SetRotation(_pContext->m_pObj->m_iObject,DeltaMat);

		// Movement
		vec128 AnimMoveToEndRot= M_VMulMat3x3(AnimMoveToEnd, InvPrevRot);
		ExtraMove = M_VSub(DeltaPos, AnimMoveToEndRot);
		ExtraMoveNextFrame = M_VMul(ExtraMove, M_VLdScalar(dT));
	}
	else if (Layer.m_Time >= AdjustmentCutOff)
	{
		// Calculate absolute positions.
		return true;
		/*CVec3Dfp32 MoveA;
		CQuatfp32 RotA;
		Layer.m_spSequence->EvalTrack0(TimeA, MoveA, RotA);
		CVec3Dfp32 MoveB;
		CQuatfp32 RotB;	
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
		CMat4Dfp32 MatA;
		MatA.Unit();
		RotA.CreateMatrix3x3(MatA);
		MatA.GetRow(2) = UpVector;
		MatA.RecreateMatrix(2,0);
		CMat4Dfp32 InvMatA;
		MatA.InverseOrthogonal(InvMatA);
		//		InvRotA = RotA; InvRotA.Inverse();
		//	InvRotA.CreateMatrix3x3(InvMatA);
		dMove.MultiplyMatrix3x3(InvMatA);

		if (RotA.DotProd(RotB) < 0.0f)
		{
			dRot.k[0] = -dRot.k[0];
			dRot.k[1] = -dRot.k[1];
			dRot.k[2] = -dRot.k[2];
			dRot.k[3] = -dRot.k[3];
		}

		// Convert into Object direction.
		dMove.MultiplyMatrix3x3(CurrentPos);

		ConOut(CStrF("Move: %s",dMove.GetString().Str()));
		_MoveVelocity = dMove;
		_RotVel = dRot;
		return true;*/
	}
	else
	{	
		/*ExtraRotToEnd.Unit();
		ExtraRotNextFrame.Unit();*/
		if (pState->GetFlags(1) & 0x00000400)
		{
			Layer.m_spSequence->EvalTrack0(TimeB, AnimPosNextTick.v128, AnimRotNextTick);
		}
		//_pContext->m_pWPhysState->Debug_RenderMatrix(CurrentPos,20.0f,false,0xff00ff00,0xff00ff00,0xff00ff00);
		CVec3Dfp32 dMoveCurrent = CurrentPos.GetRow(3) - StartPos.GetRow(3);

		CQuatfp32 StartRot;
		StartRot.Create(StartPos);

		_MoveVelocity = AnimPosNextTick.v3 - dMoveCurrent;
		_RotVel.Unit();
		CQuatfp32 TargetRot = AnimRotNextTick * StartRot;
		CAxisRotfp32 RotNextTick;
		RotNextTick.Create(AnimRotNextTick);
		CMat4Dfp32 MatTargetRot;
		TargetRot.CreateMatrix(MatTargetRot);
		_pContext->m_pWPhysState->Object_SetRotation(_pContext->m_pObj->m_iObject,MatTargetRot);

		/*ExtraMove = ExtraMoveNextFrame = AnimMoveToEndRot = 0.0f;
		// Set rotation instead of setting velocity, should be better, add perfect velocity as well
		CQuatfp32 INext = AnimRotCurrent;
		INext.Inverse();
		AnimRotNextTick.Multiply(INext, _RotVel);*/
		return true;
	}
	
	// Calculate relative movement/rotation from end of animation to destination position
	// SHOULD BE 0.0 NOW!!!
	vec128 dMoveDest;
	dMoveDest = M_VSub(M_VSub(DeltaPos, AnimMoveToEnd), ExtraMove);
	
	// Find delta to next frame for animation
	VecUnion dMoveAnim;
	CQuatfp32 dRotAnim;
	dMoveAnim.v128 = M_VSub(AnimPosNextTick.v128, AnimPosCurrent);
	/*dRotAnim = AnimRotNextTick;
	qTemp = AnimRotCurrent;
	qTemp.Inverse();
	dRotAnim *= qTemp;*/

	// Compensate for moving while rotating.
	dMoveAnim.v128 = M_VMulMat3x3(dMoveAnim.v128, InvPrevRot);

	// Add relative movement to current anim delta
	dMoveAnim.v128 = M_VAdd(dMoveAnim.v128, ExtraMoveNextFrame);
	_MoveVelocity = dMoveAnim.v3;
	//_pContext->m_pWPhysState->Debug_RenderMatrix(CurrentPos,20.0f,false,0xff0000ff,0xff0000ff,0xff0000ff);

	// Add extrarot
	//_RotVel = dRotAnim * ExtraRotNextFrame;

	//if (_pContext->m_pObj->m_iObject == 50)
	/*CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	
	static bool bDebug = true;
	if (bDebug && _pContext->m_pWPhysState->IsServer())
	{
		_pContext->m_pWPhysState->Debug_RenderWire(CurrentPos.GetRow(3),CurrentPos.GetRow(3) + _MoveVelocity,0xff7f007f,15.0f);
		CAxisRotfp32 AnimRotCurrentAR(AnimRotCurrent);
		CAxisRotfp32 ARRotDiff(ExtraRotToEnd);
		CStr Str = CStrF("%d:%s:%f:%f - Velocity: %s DeltaPos: %s DeltaDiff: %s ExtraMove: %s CharPos: %s Targetpos: %s iState: %d", _iState,(_pContext->m_pWPhysState->IsServer() ? "S":"C"),pCD->m_GameTime.GetTime(),(AdjustmentCutOff - Layer.m_Time) * Layer.m_TimeScale,_MoveVelocity.GetString().GetStr(),DeltaPos.GetString().GetStr(),dMoveDest.GetString().Str(),ExtraMoveNextFrame.GetString().Str(),_pContext->m_pObj->GetPosition().GetString().GetStr(), DestPos.GetString().GetStr(), _iState);
		Str += CStrF("LayerT: %f DmoveEnd: %s DmoveFrame: %s StartPos: %s CurrentPos: %s AnimPosEnd: %s AnimPosCurrent: %s CurrentTime: %f Duration: %f",Layer.m_Time,DeltaPos.GetString().Str(),dMoveAnim.GetString().Str(),StartPos.GetString().Str(),CurrentPos.GetString().Str(),AnimPosEnd.GetString().Str(),AnimPosCurrent.GetString().Str(),Layer.m_Time,LayerDuration);
		//if (_pContext->m_pWPhysState->IsServer())
		ConOutL(Str);
	}*/
	return true;
}

//--------------------------------------------------------------------------------

// Misleading name I guess, but from destination go to exactly the orientation and destination in the animation
bool CWAG2I::GetAnimVelocityFromDestination(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVel, CAG2StateIndex _iState)
{
	// Assumes animations start at 0,0,0
	CWAG2I_Token* pToken = GetTokenFromID(0);
	if (!pToken)
		return false;

	// Start position (in destination...)
	CMat4Dfp32 StartPos = m_pEvaluator->GetDestination();
	if (StartPos.GetRow(3) == CVec3Dfp32(_FP32_MAX))
		return false;

	CMat4Dfp32 CurrentPos = _pContext->m_pObj->GetPositionMatrix();
	CurrentPos.GetRow(2) = m_pEvaluator->GetUpVector();
	CurrentPos.RecreateMatrix(2,0);
	CVec3Dfp32 dMoveCurrent = CurrentPos.GetRow(3) - StartPos.GetRow(3);
	CQuatfp32 CurrentRot, StartRot,dRotCurrent;
	CurrentRot.Create(CurrentPos);
	StartRot.Create(StartPos);
	dRotCurrent = StartRot;
	dRotCurrent.Inverse();
	dRotCurrent = CurrentRot * dRotCurrent;
	dRotCurrent.Inverse();

	/*{
		CVec3Dfp32 DestDir = dMoveCurrent;
		DestDir.Normalize();
		_pContext->m_pWPhysState->Debug_RenderMatrix(StartPos,15.0f);
		_pContext->m_pWPhysState->Debug_RenderWire(CurrentPos.GetRow(3) + CVec3Dfp32(0,0,50),CurrentPos.GetRow(3) + CVec3Dfp32(0,0,50) + DestDir * 150.0f,0xff7f7f7f,15.0f);
		//_pContext->m_pWPhysState->Debug_RenderWire(CVec3Dfp32(0,0,150),DestPos.GetRow(3),0xff7f7f7f,15.0f);
	}*/

	_MoveVelocity = 0.0f;
	_RotVel.Unit();

	CXR_AnimLayer Layer;
	// Only get the layer that has the flag set...
	if (!pToken->GetAnimLayerFromState(_pContext, Layer, _iState) || Layer.m_iBlendBaseNode > 1)
		return false;

	// Find total movement from this time to endtime
	VecUnion AnimPosNext;
	CQuatfp32 AnimRotNext;
//	fp32 LayerDuration = Layer.m_spSequence->GetDuration();
	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time + _pContext->m_TimeSpan* Layer.m_TimeScale);
	// Assume end time is correct for the state (no early outings..)
	Layer.m_spSequence->EvalTrack0(TimeA, AnimPosNext.v128, AnimRotNext);
	AnimPosNext.v3 *= m_pEvaluator->GetAnimMoveScale();

	// Calculate relative movement/rotation to end of animation
	AnimPosNext.v128 = M_VMulMat3x3(AnimPosNext.v128, StartPos);
	_MoveVelocity = AnimPosNext.v3 - dMoveCurrent;
	//_RotVel = AnimRotNext * dRotCurrent;
	CQuatfp32 TargetRot = AnimRotNext * StartRot;
	CMat4Dfp32 MatTargetRot;
	TargetRot.CreateMatrix(MatTargetRot);
	_pContext->m_pWPhysState->Object_SetRotation(_pContext->m_pObj->m_iObject,MatTargetRot);

	/*if (_pContext->m_pObj->m_iObject == 50)
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	static bool bDebug = true;
	if (bDebug)
	{
		CStr Str = CStrF("iObj: %d %s:%f - Velocity: %s DeltaPos: %s CharPos: %s Startpos: %s iState: %d", _pContext->m_pObj->m_iObject,(_pContext->m_pWPhysState->IsServer() ? "S":"C"),_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGAMETICK),_pContext->m_pObj->m_iObject) * _pContext->m_pWPhysState->GetGameTickTime(),_MoveVelocity.GetString().GetStr(),AnimPosTEMP.GetString().GetStr(),CurrentPos.GetRow(3).GetString().Str(),StartPos.GetRow(3).GetString().GetStr(), _iState);
		//if (_pContext->m_pWPhysState->IsServer())
		ConOutL(Str);
	}*/
	return true;
}

void CWAG2I::GetRotVelocityToDest(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CAxisRotfp32& _RotVel, CAG2StateIndex _iState, fp32 _RotAngleTotal)
{
	CWAG2I_Token* pToken = GetTokenFromID(0);
	if (!pToken)
		return;
	const CXRAG2_State* pState = GetState(_iState,pToken->GetAnimGraphIndex());
	if (!pState)
		return;
	CXR_AnimLayer Layer;
	// Only get the layer that has the flag set...
	if (!pToken->GetAnimLayerFromState(_pContext, Layer, _iState) || Layer.m_iBlendBaseNode > 1)
		return;

	// Find total movement from this time to endtime
	vec128 AnimPosNextTick,AnimPosCurrent,AnimPosEnd;
	CQuatfp32 AnimRotCurrent,AnimRotNextTick,AnimRotEnd,dRotToEnd,dRotCurrent;
	fp32 LayerDuration = Layer.m_spSequence->GetDuration();

	CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
	CMTime TimeB = CMTime::CreateFromSeconds(Min(Layer.m_Time + _pContext->m_TimeSpan* Layer.m_TimeScale,LayerDuration));
	CMTime TimeEnd = CMTime::CreateFromSeconds(LayerDuration);
	Layer.m_spSequence->EvalTrack0(TimeA, AnimPosCurrent, AnimRotCurrent);
	Layer.m_spSequence->EvalTrack0(TimeB, AnimPosNextTick, AnimRotNextTick);
	Layer.m_spSequence->EvalTrack0(TimeEnd, AnimPosEnd, AnimRotEnd);

	// Rotvel
	AnimRotCurrent.Inverse();
	dRotCurrent = AnimRotNextTick * AnimRotCurrent;
	dRotToEnd = AnimRotEnd * AnimRotCurrent;
	CAxisRotfp32 dAxisRotToEnd;
	_RotVel.Create(dRotCurrent);
	dAxisRotToEnd.Create(dRotToEnd);
	// ExtraRot
	if (dAxisRotToEnd.m_Axis * CVec3Dfp32(0.0f,0.0f,1.0f) < 0.0f)
		dAxisRotToEnd.m_Angle = -dAxisRotToEnd.m_Angle;

	if (_RotVel.m_Axis * CVec3Dfp32(0.0f,0.0f,1.0f) < 0.0f)
	{
		_RotVel.m_Axis = -_RotVel.m_Axis;
		_RotVel.m_Angle = -_RotVel.m_Angle;
	}
	
	fp32 ExtraRotToEnd = _RotAngleTotal + dAxisRotToEnd.m_Angle;
	fp32 TimeLeft = Max(_pContext->m_TimeSpan, (TimeEnd - TimeA).GetTime());
	//ConOut(CStrF("RotVel: %f  TotalLeft: %f ExtraRot: %f",_RotVel.m_Angle + _pContext->m_TimeSpan * (ExtraRotToEnd / TimeLeft),_RotAngleTotal,ExtraRotToEnd));
	_RotVel.m_Angle -= _pContext->m_TimeSpan * (ExtraRotToEnd / (TimeEnd - TimeA).GetTime());
	

	// MoveVel
	CMat4Dfp32 ObjMatrix;
	if(_pContext->m_pObj)
	{
		ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
		ObjMatrix.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
		ObjMatrix.RecreateMatrix(2, 0);
	}
	else
		ObjMatrix.Unit();

	CQuatfp32 InvRotA;
	CMat4Dfp32 MatA;
	MatA.Unit();
	AnimRotCurrent.CreateMatrix3x3(MatA);
	MatA.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
	MatA.RecreateMatrix(2,0);
	CMat4Dfp32 InvMatA;
	MatA.InverseOrthogonal(InvMatA);
	//		InvRotA = RotA; InvRotA.Inverse();
	//	InvRotA.CreateMatrix3x3(InvMatA);
	VecUnion dMove;
	dMove.v128 = M_VSub(AnimPosNextTick, AnimPosCurrent);
	dMove.v128 = M_VMulMat3x3(dMove.v128,InvMatA);
	// Convert into Object direction.
	dMove.v128 = M_VMulMat3x3(dMove.v128, ObjMatrix);
	_MoveVelocity = dMove.v3;
}

//--------------------------------------------------------------------------------

bool CWAG2I::GetTopLayerTotalAnimOffset(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset, CAG2TokenID _iToken)
{
	_MoveOffset = 0;
	_RotOffset.Unit();

	CEventLayer Layer;
	int nLayers = 1;
	GetTopEventLayersFromToken(_pContext,_iToken,&Layer,nLayers);
	if (nLayers > 0)
	{
		const CVec3Dfp32& UpVector = m_pEvaluator->GetUpVector();
		CMat4Dfp32 ObjMatrix = _pContext->m_pObj->GetPositionMatrix();
		ObjMatrix.GetRow(2) = UpVector;
		ObjMatrix.RecreateMatrix(2, 0);
		CMTime Time = CMTime::CreateFromSeconds(Layer.m_Layer.m_spSequence->GetDuration());
		VecUnion tmp;
		Layer.m_Layer.m_spSequence->EvalTrack0(Time, tmp.v128, _RotOffset);
		// Convert into Object direction.
		tmp.v128=M_VMulMat3x3(tmp.v128, ObjMatrix);
		_MoveOffset=tmp.v3;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------

// Get only the rotational part?
bool CWAG2I::GetAnimRotVelocity(const CWAG2I_Context* _pContext, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAG2I::GetAnimRotVelocity, WAG2I);

	_RotVelocity.Unit();

	CWAG2I_Context OverlayContext;
	if(m_OverlayAnim.IsValid())
	{
		// Ok then, similar to GetAnimVelocityFromDestination
		// Start position (in destination...)
		CMat4Dfp32 StartPos = m_pEvaluator->GetDestination();
		if (StartPos.GetRow(3) == CVec3Dfp32(_FP32_MAX))
			return 0;

		CMat4Dfp32 CurrentPos = _pContext->m_pObj->GetPositionMatrix();
		CurrentPos.GetRow(2) = m_pEvaluator->GetUpVector();
		CurrentPos.RecreateMatrix(2,0);
		CQuatfp32 CurrentRot, StartRot,dRotCurrent;
		CurrentRot.Create(CurrentPos);
		StartRot.Create(StartPos);
		dRotCurrent = StartRot;
		dRotCurrent.Inverse();
		dRotCurrent = CurrentRot * dRotCurrent;
		dRotCurrent.Inverse();

		// Find total movement from this time to endtime
		vec128 AnimPosNext;
		CQuatfp32 AnimRotNext;
		//	fp32 LayerDuration = Layer.m_spSequence->GetDuration();
		CMTime TimeA = CMTime::CreateFromSeconds((_pContext->m_GameTime - m_OverlayAnim_StartTime).GetTime() + _pContext->m_TimeSpan);
		// Assume end time is correct for the state (no early outings..)
		m_OverlayAnim.m_pSeq->EvalTrack0(TimeA, AnimPosNext, AnimRotNext);

		// Calculate relative movement/rotation to end of animation
		_RotVelocity = AnimRotNext * dRotCurrent;
		return true;
	}

	CXR_AnimLayer pLayers[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);
	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
#ifdef	AG2_DEBUG
		bool bApplyPhysics = true;
		if (!bApplyPhysics)
			continue;
#endif

		CXR_AnimLayer& Layer = pLayers[iLayer];

		if (Layer.m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		vec128 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(Layer.m_Time);
		Layer.m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		vec128 MoveB;
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
			vec128 LoopMove;
			CQuatfp32 LoopRot;
			Layer.m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dRot *= LoopRot;
		}

		fp32 BlendFactor = Layer.m_Blend;

		// Weight Rot using layer blendfactor.
		CQuatfp32 TempRot;
		_RotVelocity.Lerp(dRot, BlendFactor, TempRot);
		_RotVelocity = TempRot;
	}

//	UnacquireAllResources();
	return true;
}

bool CWAG2I::HasAnimVelocity(const CWAG2I_Context* _pContext, int _iDisableStateInstanceAnimsCallbackMsg)
{
	MSCOPE(CWAG2I::HasAnimVelocity, WAG2I);
	if (!AcquireAllResources(_pContext))
		return false;

	CXR_AnimLayer pLayers[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	GetAnimLayers(_pContext, pLayers, nLayers, _iDisableStateInstanceAnimsCallbackMsg);

	for (int iLayer = 0; iLayer < nLayers; iLayer++)
	{
		// FIXME: Query animation layer for this.
		if (pLayers[iLayer].m_iBlendBaseNode > 1)
			continue;

		// Calculate absolute positions.
		vec128 MoveA;
		CQuatfp32 RotA;
		CMTime TimeA = CMTime::CreateFromSeconds(pLayers[iLayer].m_Time);
		pLayers[iLayer].m_spSequence->EvalTrack0(TimeA, MoveA, RotA);

		vec128 MoveB;
		CQuatfp32 RotB;
		CMTime TimeB = pLayers[iLayer].m_spSequence->GetLoopedTime(TimeA + CMTime::CreateFromSeconds(_pContext->m_TimeSpan * pLayers[iLayer].m_TimeScale));
		pLayers[iLayer].m_spSequence->EvalTrack0(TimeB, MoveB, RotB);

		// Calculate relative deltas.
		vec128 dMove;
		CQuatfp32 dRot;
		dMove = M_VSub(MoveB, MoveA);
		dRot = RotA;
		dRot.Inverse();
		dRot = RotB * dRot;

		// Add loopseam compensation.
		bool bLooping = TimeB.Compare(TimeA) < 0;
		if (bLooping)
		{
			vec128 LoopMove;
			CQuatfp32 LoopRot;
			pLayers[iLayer].m_spSequence->GetTotalTrack0(LoopMove, LoopRot);
			dMove = M_VSetW1(M_VAdd(dMove, LoopMove));
			dRot *= LoopRot;
		}

		if (!M_VCmpAllEq(M_VSetW0(dMove),M_VZero()))
			return true;
		if(dRot.k[0] != 0.0f || dRot.k[1] != 0.0f || dRot.k[2] != 0.0f)
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------

void CWAG2I::CheckAnimEvents(const CWAG2I_Context* _pContext, int _iCallbackMessage, uint32 _ScanMask, bool _bOnlyTop)
{
	MSCOPE(CWAG2I::CheckAnimEvents, WAG2I);

	if (!AcquireAllResourcesToken(_pContext))
		return;


	CEventLayer pLayers[AG2I_MAXANIMLAYERS];
	CAG2TokenID pLayerTokenID[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	// We're not really interested in any other layers than the top one...?
	if (_bOnlyTop)
		GetTopEventLayers(_pContext, pLayers, pLayerTokenID, nLayers);
	else
		GetEventLayers(_pContext, pLayers, pLayerTokenID, nLayers);
	CheckAnimEvents(_pContext,pLayers,pLayerTokenID,nLayers,_iCallbackMessage,_ScanMask);
}


void CWAG2I::CheckTokenAnimEvents(const CWAG2I_Context* _pContext, int _iCallbackMessage, CAG2TokenID _iToken, uint32 _ScanMask)
{
	MSCOPE(CWAG2I::CheckTokenAnimEvents, WAG2I);

	if (!AcquireAllResourcesToken(_pContext))
		return;


	CEventLayer pLayers[AG2I_MAXANIMLAYERS];
	CAG2TokenID pLayerTokenID[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	// We're not really interested in any other layers than the top one...?
	GetTopEventLayersFromToken(_pContext, _iToken, pLayers, nLayers);
	for (int32 i = 0; i < nLayers; i++)
		pLayerTokenID[i] = _iToken;
	CheckAnimEvents(_pContext,pLayers,pLayerTokenID,nLayers,_iCallbackMessage, _ScanMask);
}

void CWAG2I::CheckAnimEvents(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int32 _nLayers, int _iCallbackMessage, uint32 _ScanMask) const
{
	MSCOPE(CWAG2I::CheckTokenAnimEvents_2, WAG2I);
	M_ASSERT(_pLayers && _pTokenIDs,"CWAG2I::CheckAnimEvents LAYERS INVALID");
	if (_nLayers == 0)
		return;

	for (int iLayer = 0; iLayer < _nLayers; iLayer++)
	{
		fp32 StartScanTime = _pLayers[iLayer].m_Layer.m_ContinousTime.GetTime();
		fp32 TimeSpan = _pContext->m_TimeSpan * _pLayers[iLayer].m_Layer.m_TimeScale;
		fp32 Offset = 0.0001f / _pLayers[iLayer].m_Layer.m_TimeScale;
		CMTime EndScanTime = _pLayers[iLayer].m_Layer.m_spSequence->GetLoopedTime(CMTime::CreateFromSeconds(_pLayers[iLayer].m_Layer.m_Time + TimeSpan * 0.9f));

		if ((_pLayers[iLayer].m_Layer.m_Flags & CXR_ANIMLAYER_CHECKALLANIMEVENTS) ||
			(_pLayers[iLayer].m_Layer.m_ContinousTime.Compare(TimeSpan * 2.0f + Offset) < 0))
		{
			StartScanTime = 0.0f;
		}

		/*if (bDebugOut && _pContext->m_pWPhysState->IsClient() && _pLayers[iLayer].m_Layer.m_TimeScale != 1.0f)
			ConOut(CStrF("GameTime: %f LayerTime: %f Cont: %f StartScan: %f EndScan: %f TimeSpan: %f key: %d",_pContext->m_GameTime.GetTime(),_pLayers[iLayer].m_Layer.m_Time,_pLayers[iLayer].m_Layer.m_ContinousTime.GetTime(),StartScanTime,EndScanTime.GetTime(),TimeSpan,*_pLayers[iLayer].m_pKey));*/

		//		if(_pContext->m_pWPhysState->IsClient())
		//ConOutL(CStrF("CheckAnimEvents GT: %f   CT: %f SCT: %f ECT: %f TS: %f Seq: %.8x", _pContext->m_GameTime.GetTime(), _pLayers[iLayer].m_ContinousTime.GetTime(),StartScanTime,EndScanTime.GetTime(),TimeScale,_pLayers[iLayer].m_spSequence));
		// Seems edges get missed sometimes, add a little safety
		//EndScanTime += CMTime::CreateFromSeconds(0.001f);
		if (EndScanTime.Compare(CMTime::CreateFromSeconds(_pLayers[iLayer].m_Layer.m_Time)) < 0)
		{
			EndScanTime = CMTime::CreateFromSeconds(_pLayers[iLayer].m_Layer.m_Time);
		}
	
		CheckAnimEvents(_pContext, _pLayers[iLayer].m_Layer.m_spSequence, CMTime::CreateFromSeconds(StartScanTime), EndScanTime, _pLayers[iLayer].m_Layer.m_Time, _pTokenIDs[iLayer], _iCallbackMessage, _ScanMask,*_pLayers[iLayer].m_pKey);
	}
}

//--------------------------------------------------------------------------------

void CWAG2I::CheckAnimEvents(const CWAG2I_Context* _pContext, CXR_Anim_SequenceData* _pAnimSeqData, CMTime _StartTime, CMTime _EndTime, fp32 _LayerTime, CAG2TokenID _iToken, int _iCallbackMessage, uint32 _ScanMask, int16& _iKey) const
{
	MSCOPE(CWAG2I::CheckAnimEvents_Layer, WAG2I);

	// Don't care about these events since they are taken care of elsewhere
	_ScanMask &= ~(ANIM_EVENT_MASK_BREAKOUT|ANIM_EVENT_MASK_ENTRY|ANIM_EVENT_MASK_SYNC);
	const CXR_Anim_DataKey *pKey = _pAnimSeqData->GetEvents(_StartTime, _EndTime, _ScanMask, _iKey);
	while (pKey != NULL)
	{
		// Put in the actual time of the event...
		CMTime EventTime = _pContext->m_GameTime + CMTime::CreateFromSeconds(pKey->m_AbsTime - _LayerTime);
		CWObject_Message Msg(_iCallbackMessage, aint(pKey));
		Msg.m_Reason = _iToken;
		Msg.m_pData = &EventTime;
		Msg.m_DataSize = sizeof(EventTime);
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);

		pKey = _pAnimSeqData->GetEvents(_StartTime, _EndTime, _ScanMask, _iKey);
	}
}

fp32 CWAG2I::GetCurrentLoopDuration(const CWAG2I_Context* _pContext, CAG2TokenID _iToken)
{
	const CWAG2I_Token* pToken = GetTokenFromID(_iToken);
	const CWAG2I_StateInstance* pInst = pToken ? pToken->GetTokenStateInstance() : NULL;
	if (!pInst)
		return 0.0f;

	return pInst->GetAnimLoopDurationE();
}

// Update breakout points
/*if (pKey->m_EventParams[0] != 0)
{
	// Ok, what range should the breakout points have?
	// Fixed param types, ie "from here we can run"
	// Or have codes/flags for certain types?
	ConOut(CStrF("Got PARAM: %d", pKey->m_EventParams[0]));
}*/

bool CWAG2I::FindEntryPoint(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, int16 _iPrevSequence, CMTime _PrevTime, fp32& _Offset) const
{
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	const CXRAG2_State* pState = pAnimGraph->GetState(_iState);
	M_ASSERT(pAnimGraph && pState, "Animgraph index error");
	
	if (_iPrevSequence == -1 || (pState->GetNumAnimLayers() <= 0))
		return false;

	int16 iNewSeq = pState->GetBaseAnimLayerIndex();
	const CXRAG2_AnimLayer* pAnimLayerNew = pAnimGraph->GetAnimLayer(iNewSeq);
	int16 iAnimNew = pAnimLayerNew->GetAnimIndex();

	const CXR_Anim_SequenceData* pAnimLayerSeqPrev = pAnimGraph->GetAnimSequenceData(_pContext->m_pWorldData, _iPrevSequence);
	const CXR_Anim_SequenceData* pAnimLayerSeqNew = pAnimGraph->GetAnimSequenceData(_pContext->m_pWorldData, iAnimNew);
	if (pAnimLayerSeqPrev && pAnimLayerSeqNew && 
		(pAnimLayerSeqNew->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
	{
		_Offset = pAnimLayerSeqNew->FindEntryTime(pAnimLayerSeqPrev, _PrevTime);
		//ConOutL(CStrF("Exit: %f Entry: %f,",_PrevTime.GetTime(),_Offset));
		return true;
	}

	return false;
}

// Find breakoutpoints from given time and forward
void CWAG2I::FindBreakoutPoints(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, CXR_Anim_BreakoutPoints& _Points, fp32 _Offset) const
{
	MSCOPE(CWAG2I::FindBreakoutPoints, WAG2I);
	/*
	if (!AcquireAllResources(_pContext->m_pWPhysState))
	return;
	*/

	// Reset breakoutpoint data
	_Points.m_lPoints.Clear();

	// Mmmkay, find stateinstance for given state, 
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAnimGraph,"Invalid animgraph");

	// First create the points in this array, then copy to real data
	CBreakoutPoint lPoints[255];
	uint MaxPoints = 255;
	int32 PointLen = 0;

	const CXRAG2_State* pState = pAnimGraph->GetState(_iState);
	if (!pState || !pState->GetNumAnimLayers())
		return;

	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG2_AnimLayer* pAnimLayer = pAnimGraph->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		//		ConOutL(CStrF("iAnim %d", iAnim));

		const CXR_Anim_SequenceData* pAnimLayerSeq = pAnimGraph->GetAnimSequenceData(_pContext->m_pWorldData, iAnim);
		if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
			continue;

		int16 iTimeControlProperty = pAnimLayer->GetTimeControlProperty();
		CMTime ContinousTime = m_pEvaluator->AG2_EvaluateProperty(_pContext, iTimeControlProperty, AG2_PROPERTYTYPE_FLOAT).GetTime();
		ContinousTime = (ContinousTime + CMTime::CreateFromSeconds(pAnimLayer->GetTimeOffset() + _Offset)).Scale(pAnimLayer->GetTimeScale());
		CMTime LoopedTime = pAnimLayerSeq->GetLoopedTime(ContinousTime);

		//ConOut(CStrF("Breakout Looped time: %f",LoopedTime));

		PointLen += pAnimLayerSeq->FindBreakoutPoints(lPoints, MaxPoints, LoopedTime);
	}

	// Create breakoutpoints
	_Points.Create(lPoints,PointLen);
}

void CWAG2I::FindEntryPoints(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, CXR_Anim_EntryPoints& _Points) const
{
	MSCOPE(CWAG2I::FindEntryPoints, WAG2I);

	// Reset Entrypoint data
	_Points.m_lPoints.Clear();

	// Mmmkay, find stateinstance for given state, 
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAnimGraph,"Invalid animgraph");

	// First create the points in this array, then copy to real data
	CEntryPoint lPoints[255];
	uint MaxPoints = 255;
	int32 PointLen = 0;

	const CXRAG2_State* pState = pAnimGraph->GetState(_iState);
	if (!pState || !pState->GetNumAnimLayers())
		return;

	for (int jAnimLayer = 0; jAnimLayer < pState->GetNumAnimLayers(); jAnimLayer++)
	{
		int16 iAnimLayer = pState->GetBaseAnimLayerIndex() + jAnimLayer;

		const CXRAG2_AnimLayer* pAnimLayer = pAnimGraph->GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			continue;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		//		ConOutL(CStrF("iAnim %d", iAnim));

		const CXR_Anim_SequenceData* pAnimLayerSeq = pAnimGraph->GetAnimSequenceData(_pContext->m_pWorldData, iAnim);
		if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
			continue;
		/*if (pAnimLayerSeq == NULL || !(pAnimLayerSeq->m_Flags & ANIM_SEQFLAGS_HASENTRYPOINTS))
			continue;*/

		PointLen += pAnimLayerSeq->FindEntryPoints(lPoints, MaxPoints);
	}

	// Create entrypoints
	_Points.Create(lPoints,PointLen);
}

bool CWAG2I::GetAbsoluteAnimOffset(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int32 _iAnim, fp32 _TimeOffset, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset) const
{
	MSCOPE(CWAG2I::GetAbsoluteAnimOffset, WAG2I);

	_MoveOffset = 0;
	_RotOffset.Unit();

	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAnimGraph,"Invalid animgraph");

	// Find sequence data from given animation index
	const CXR_Anim_SequenceData* pSeq = pAnimGraph->GetAnimSequenceData(_pContext->m_pWorldData,_iAnim);
	if (!pSeq)
		return false;

	// Calculate absolute positions.
	CMTime LoopedTime = CMTime::CreateFromSeconds(_TimeOffset);
	LoopedTime = pSeq->GetLoopedTime(LoopedTime);
	VecUnion tmp;
	pSeq->EvalTrack0(LoopedTime, tmp.v128, _RotOffset);
	_MoveOffset=tmp.v3;

	return true;
}

//--------------------------------------------------------------------------------

CAG2AnimIndex CWAG2I::GetAnimFromAction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, CStr& _MoveAction, fp32& _TimeOffset) const
{
	MSCOPE(CWAG2I::GetAnimFromAction, WAG2I);
	// Start by finding action and seeing to which state it leads, and then find animation from 
	// that state, only care about first anim layer and first move token for now
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAnimGraph,"Invalid animgraph");

	uint32 ActionHashKey = StringToHash(_MoveAction);
	int16 iAction = GetActionIndexFromHashKey(ActionHashKey, _iAnimGraph);
	const CXRAG2_Action* pAction = pAnimGraph->GetAction(iAction);
	const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(pAction->GetBaseMoveTokenIndex());
	const CXRAG2_State* pState = (pMoveToken ? pAnimGraph->GetState(pMoveToken->GetTargetStateIndex()) : NULL);
	if (!pState || !pState->GetNumAnimLayers())
		return false;
	const CXRAG2_AnimLayer* pAnimLayer = pAnimGraph->GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if (!pAnimLayer)
		return false;
	_TimeOffset = pMoveToken->GetAnimTimeOffset();

	return pAnimLayer->GetAnimIndex();
}

CAG2AnimIndex CWAG2I::GetAnimFromReaction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, CXRAG2_Impulse _Impulse, int8 _iToken, fp32& _TimeOffset) const
{
	// 
	const CXRAG2* pAnimGraph = GetAnimGraph(0);
	const CWAG2I_Token* pToken = GetTokenFromID(_iToken);
	if (!pAnimGraph || !pToken)
		return -1;

	// Meh, use animgraph0 for now
	CAG2ReactionIndex iReaction = pAnimGraph->GetMatchingReaction(pToken->GetGraphBlock(), _Impulse);
	if (iReaction == -1)
		return -1;
	
	// Assume first movetoken in the reaction
	const CXRAG2_Reaction* pReaction = pAnimGraph->GetReaction(iReaction);

	const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(pReaction->GetBaseMoveTokenIndex());
	const CXRAG2_State* pState = (pMoveToken ? pAnimGraph->GetState(pMoveToken->GetTargetStateIndex()) : NULL);
	if (!pState || !pState->GetNumAnimLayers())
		return false;
	const CXRAG2_AnimLayer* pAnimLayer = pAnimGraph->GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if (!pAnimLayer)
		return false;
	_TimeOffset = pMoveToken->GetAnimTimeOffset();

	return pAnimLayer->GetAnimIndex();
}

bool CWAG2I::GetAnimFromFirstActionInState(const CWAG2I_Context* _pContext, CAG2AnimIndex _iAnim, int32 _iToken, CAG2AnimIndex& _iTargetAnim, CAG2ActionIndex& _iTargetAction) const
{
	MSCOPE(CWAG2I::GetAnimFromFirstActionInAnimState, WAG2I);
	// Find state that contains given animation, find the first action in that state that has a 
	// movetoken to a state with an animation
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAG2I_Token* pToken = &(m_lTokens[iToken]);

		if (pToken->GetID() != _iToken)
			continue;

		const CWAG2I_StateInstance* pStateInstance = pToken->GetSpecificState(_pContext, _iAnim);
		const CXRAG2* pAnimGraph = pStateInstance ? GetAnimGraph(pStateInstance->GetAnimGraphIndex()) : NULL;
		if (!pAnimGraph)
			continue;

		int32 iState = pStateInstance->GetStateIndex();
		const CXRAG2_State* pState = (pStateInstance ? pAnimGraph->GetState(iState) : NULL);
		const CXRAG2_State* pNextState = (pStateInstance ? pAnimGraph->GetState(iState+1) : NULL);
		if (pState && pNextState)
		{
			int32 iBaseIndex = pState->GetBaseActionIndex();
			int32 NumActions = pNextState->GetBaseActionIndex() - iBaseIndex;
			for (int32 i = 0; i < NumActions; i++)
			{
				// Find an action that leads to a state with an animation
				const CXRAG2_Action* pAction = pAnimGraph->GetAction(iBaseIndex + i);
				const CXRAG2_MoveToken* pMoveToken = (pAction && pAction->GetNumMoveTokens() ? pAnimGraph->GetMoveToken(pAction->GetBaseMoveTokenIndex()) : NULL);
				if (!pMoveToken)
					continue;

				const CXRAG2_State* pTargetState = pAnimGraph->GetState(pMoveToken->GetTargetStateIndex());
				if (pTargetState && pTargetState->GetNumAnimLayers() > 0)
				{
					// Yay, found a target state with animation
					const CXRAG2_AnimLayer* pAnimLayer = pAnimGraph->GetAnimLayer(pTargetState->GetBaseAnimLayerIndex());
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

bool CWAG2I::GetConstantFromGraphBlock(CXRAG2_Impulse _Impulse, CAG2TokenID _TokenID, CAG2StateConstantID _iConstant, fp32& _Constant)
{
	MSCOPE(CWAG2I::GetConstantFromGraphBlock, WAG2I);

	CWAG2I_Token* pToken = GetTokenFromID(_TokenID);
	if (!pToken)
		return false;

	// Get a matching reaction from animgraph
	CAG2AnimGraphID iAnimGraph = pToken->GetAnimGraphIndex();
	if (iAnimGraph == -1)
		return false;

	const CXRAG2* pAnimGraph = GetAnimGraph(iAnimGraph);
	M_ASSERT(pAnimGraph,"CWAG2I::GetConstantFromGraphBlock - Invalid animgraph");
	CAG2ReactionIndex iReaction = pAnimGraph->GetMatchingReaction(pToken->GetGraphBlock(),_Impulse);
	if (iReaction == -1)
		return false;

	// Ok, found a reaction, check for target graphblock and / or state
	const CXRAG2_Reaction* pReaction = pAnimGraph->GetReaction(iReaction);
	M_ASSERT(pReaction,"CWAG2I::GetConstantFromGraphBlock - Reaction nonexistant");

	const CXRAG2_GraphBlock* pBlock = NULL;
	int32 nMoveTokens = pReaction->GetNumMoveTokens();
	int32 nMoveAnimGraphs = pReaction->GetNumMoveAnimGraphs();
	if (nMoveAnimGraphs != 0)
	{
		// Move over to another animgraph
		const CXRAG2_MoveAnimGraph* pMoveAG = pAnimGraph->GetMoveAnimGraph(pReaction->GetBaseMoveAnimgraphIndex());

		CAG2AnimGraphID iAnimGraphNew = GetAnimGraphIDFromNameHash(pMoveAG->GetAnimGraphName());
		if (iAnimGraphNew == -1)
			return false;

		const CXRAG2* pNewAG = GetAnimGraph(iAnimGraphNew);
		// Ok, found new animgraph, now find a matching graphblock over there...
		CAG2GraphBlockIndex iBlock = pNewAG->GetMatchingGraphBlock(_Impulse);
		if (iBlock == -1)
			return false;

		pBlock = pNewAG->GetGraphBlock(iBlock);
		M_ASSERT(pBlock,"CWAG2I::SendImpulse Invalid graphblock");
	}
	else if (nMoveTokens == 0)
	{
		// No movetokens found, try to find a matching graphblock instead (might be 
		// a behavior reaction for instance, that just says the graphblock can react to it)
		CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_Impulse);
		if (iBlock == -1)
			return false;
		pBlock = pAnimGraph->GetGraphBlock(iBlock);
		M_ASSERT(pBlock,"CWAG2I::SendImpulse Invalid graphblock");
	}
	else
	{
		CAG2MoveTokenIndex iBaseMoveToken = pReaction->GetBaseMoveTokenIndex();
		for (int32 i = 0; i < nMoveTokens; i++)
		{
			int iMoveToken = iBaseMoveToken + i;
			const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(iMoveToken);
			M_ASSERT(pMoveToken,"CWAG2I::GetConstantFromGraphBlock - Invalid movetoken");
			int16 iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();

			// If we got target graphblock, use that....
			if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
			{
				pBlock = pAnimGraph->GetGraphBlock(iTargetGraphBlock);
				break;
			}
		}
	}
	if (!pBlock)
		pBlock = pAnimGraph->GetGraphBlock(pToken->GetGraphBlock());

	M_ASSERT(pBlock,"CWAG2I::GetConstantFromGraphBlock - Invalid graphblock");

	// Ok then should have a graphblock now
	uint16 iConstantStart = pBlock->GetBaseStateConstantIndex();
	int32 NumConstants = pBlock->GetNumStateConstants();
	for (int32 i = 0; i < NumConstants; i++)
	{
		const CXRAG2_StateConstant* pConstant = pAnimGraph->GetStateConstant(iConstantStart + i);
		if (pConstant->m_ID == _iConstant)
		{
			_Constant = pConstant->m_Value;
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------------

void CWAG2I::PredictionMiss_AddToken(CWAG2I_Context* _pContext, int8 _TokenID)
{
	MSCOPE(CWAG2I::PredictionMiss_AddToken, WAG2I);

//	CWAG2I_Token* pToken = GetTokenFromID(_TokenID, true);
}

//--------------------------------------------------------------------------------

void CWAG2I::PredictionMiss_RemoveToken(CWAG2I_Context* _pContext, int8 _TokenID)
{
	MSCOPE(CWAG2I::PredictionMiss_RemoveToken, WAG2I);

	CWAG2I_Token* pToken = GetTokenFromID(_TokenID, false);
	if (pToken != NULL)
		pToken->PredictionMiss_Remove(_pContext);
}

//--------------------------------------------------------------------------------

const CWAG2I_Token* CWAG2I::GetTokenFromID(int8 _TokenID) const
{
	MSCOPE(CWAG2I::GetTokenFromID, WAG2I);

	for (int jToken = 0; jToken < m_lTokens.Len(); jToken++)
		if (_TokenID == m_lTokens[jToken].GetID())
			return (&m_lTokens[jToken]);

	return NULL;
}

int32 CWAG2I::GetTokenIndexFromID(int8 _TokenID)
{
	MSCOPE(CWAG2I::GetTokenFromID, WAG2I);

	int32 Len = m_lTokens.Len();
	for (int jToken = 0; jToken < Len; jToken++)
		if (_TokenID == m_lTokens[jToken].GetID())
			return jToken;

	return -1;
}

//--------------------------------------------------------------------------------

CWAG2I_Token* CWAG2I::GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent)
{
	MSCOPE(CWAG2I::GetTokenFromID_2, WAG2I);

	for (int jToken = 0; jToken < m_lTokens.Len(); jToken++)
		if (_TokenID == m_lTokens[jToken].GetID())
			return (&m_lTokens[jToken]);

	if (_bCreateNonExistent && (_TokenID != AG2_TOKENID_NULL))
	{
		uint8 iToken = m_lTokens.Add(CWAG2I_Token(_TokenID, this));
		return (&m_lTokens[iToken]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------
/*
void CWAG2I::PerformAction(const CWAG2I_Context* _pContext, int8 _ActionTokenID, int16 _iAction)
{
}
*/
//--------------------------------------------------------------------------------
void CWAG2I::DoActionEffects(const CWAG2I_Context* _pContext, int16 _iAction, CAG2AnimGraphID _iAnimGraph)
{
	const CXRAG2_Action* pAction = GetAction(_iAction, _iAnimGraph);
	if (pAction)
		InvokeEffects(_pContext, _iAnimGraph, pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances());
}

void CWAG2I::MoveAction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int8 _ActionTokenID, int16 _iMoveToken, fp32 _ForceOffset,int32 _MaxQueued)
{
	MSCOPE(CWAG2I::MoveAction, WAG2I);

	// FIXME: Think about error checking OnLeaveState/OnEnterState, etc...

	CWAG2I_Token* pActionToken = GetTokenFromID(_ActionTokenID, false);
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	if (!pAnimGraph)
	{
		ConOut(CStrF("CWAG2I::MoveAction - AnimGraph BROKEN Token: %d, iMT: %d iAG: %d",_ActionTokenID,_iMoveToken,_iAnimGraph));
		return;
	}
	M_ASSERT(pAnimGraph,"Invalid animgraph");
	const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(_iMoveToken);
	M_ASSERT(pMoveToken,"INVALID MOVETOKEN");

	// Evaluate new state if targetstatetype is a switchstate
	int32 NumLoops = 0;
	while (pMoveToken->GetTargetStateType())
	{
		// Get New movetoken index
		EvaluateSwitchState(_pContext,_iMoveToken,_iAnimGraph);
		pMoveToken = pAnimGraph->GetMoveToken(_iMoveToken);
		M_ASSERT(pMoveToken,"INVALID MOVETOKEN");
		NumLoops++;
		M_ASSERT(NumLoops < 10,"STUCK IN LOOP");
	}

	// Check if we should move graphblock instead
	if (pMoveToken->GetTargetGraphBlockIndex() != AG2_GRAPHBLOCKINDEX_NULL)
	{
		MoveGraphBlock(_pContext,_iAnimGraph,_ActionTokenID,_iMoveToken);
		return;
	}
	
	if (pMoveToken->m_iTargetState == AG2_STATEINDEX_STARTAG)
	{
		MoveGraphBlock(_pContext,0,0,0);
		return;
	}

	if (pActionToken != NULL)
	{
		// Check if we're loaded with other stateinstances that wants to get through, if so
		// delete the first one in the queue
		CAG2AnimGraphID iOldAG = pActionToken->GetAnimGraphIndex();
		pActionToken->ForceMaxStateInstances(_MaxQueued);
		pActionToken->LeaveState(_pContext, _iMoveToken, iOldAG);
		pActionToken->EnterState(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
	}
	else
	{
		// Don't create a token just to terminate it
		if (pMoveToken->m_iTargetState != AG2_STATEINDEX_TERMINATE)
		{
			pActionToken = GetTokenFromID(_ActionTokenID, true);
			if (pActionToken != NULL)
				pActionToken->EnterState(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
			else
				ConOutL(CStrF("ERROR: Can't create AG2I Token %d.", _ActionTokenID));
		}
	}
}

void CWAG2I::MoveGraphBlock(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int8 _ActionTokenID, int16 _iMoveToken, fp32 _ForceOffset, int32 _MaxQueued)
{
	MSCOPE(CWAG2I::MoveGraphBlock, WAG2I);

	// FIXME: Think about error checking OnLeaveState/OnEnterState, etc...

	CWAG2I_Token* pActionToken = GetTokenFromID(_ActionTokenID, false);
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	if (!pAnimGraph)
	{
		ConOut(CStrF("CWAG2I::MoveGraphBlock - AnimGraph BROKEN Token: %d, iMT: %d iAG: %d",_ActionTokenID,_iMoveToken,_iAnimGraph));
		return;
	}
	//M_ASSERT(pAnimGraph,"Invalid animgraph");
	const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(_iMoveToken);
	M_ASSERT(pMoveToken,"INVALID MOVETOKEN");
	if (pMoveToken->m_iTargetState == AG2_STATEINDEX_STARTAG)
	{
		MoveGraphBlock(_pContext,0,0,0);
		return;
	}

	if (pActionToken != NULL)
	{
		// Check if we're loaded with other stateinstances that wants to get through, if so
		// delete the first one in the queue
		pActionToken->ForceMaxStateInstances(_MaxQueued);
		// Check if we're already in the correct graphblock, if so just move state
		int8 iOldAG;
		if (pActionToken->GetNumStateInstances())
		{
			iOldAG = pActionToken->GetStateInstance(pActionToken->GetNumStateInstances() - 1)->GetAnimGraphIndex();
		}
		else
		{
			iOldAG = _iAnimGraph;
		}
		
		if (pActionToken->GetGraphBlock() == pMoveToken->m_iTargetGraphBlock &&
			iOldAG == _iAnimGraph)
		{
			// Just move state
			int32 NumLoops = 0;
			while (pMoveToken->GetTargetStateType())
			{
				// Get New movetoken index
				EvaluateSwitchState(_pContext,_iMoveToken,_iAnimGraph);
				pMoveToken = pAnimGraph->GetMoveToken(_iMoveToken);
				M_ASSERT(pMoveToken,"INVALID MOVETOKEN");
				NumLoops++;
				M_ASSERT(NumLoops < 10,"STUCK IN LOOP");
			}
			pActionToken->LeaveState(_pContext, _iMoveToken, iOldAG);
			pActionToken->EnterState(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
		}
		else
		{
			pActionToken->LeaveGraphBlock(_pContext, _iMoveToken, iOldAG);
			pActionToken->EnterGraphBlock(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
		}
	}
	else
	{
		// Don't move to terminate if this token isn't here at all
		if (pMoveToken->m_iTargetState != AG2_STATEINDEX_TERMINATE)
		{
			pActionToken = GetTokenFromID(_ActionTokenID, true);
			if (pActionToken != NULL)
				pActionToken->EnterGraphBlock(_pContext, _iMoveToken, _iAnimGraph, _ForceOffset);
			else
				ConOutL(CStrF("ERROR: Can't create AG2I Token %d.", _ActionTokenID));
		}
	}
}


void CWAG2I::UpdateImpulseState(const CWAG2I_Context* _pContext)
{
	m_pEvaluator->UpdateImpulseState(_pContext);
}

// This function might move token/graphblock
bool CWAG2I::SendImpulse(const CWAG2I_Context* _pContext, const CXRAG2_Impulse& _Impulse)
{
	// Send impulse to all tokens (or just until we get a match?)
	int32 Len = m_lTokens.Len();
	bool bFoundMatch = false;
	for (int32 i = 0; i < Len; i++)
	{
		if (SendImpulse(_pContext, _Impulse, m_lTokens[i].GetID()))
			bFoundMatch = true;
	}

	return bFoundMatch;
}

bool CWAG2I::SendImpulse(const CWAG2I_Context* _pContext, const CXRAG2_Impulse& _Impulse, int8 _iToken, bool _bForce)
{
	// First try to find a matching reaction to current block, if a reaction is found
	// then check if we should just move within the graphblock or move to another graphblock
	// another graphblock might or might not be defined

	if (!AcquireAllResources(_pContext))
		return false;
#ifdef AG2_RECORDPROPERTYCHANGES
	m_pEvaluator->m_PropertyRecorder.AddImpulse(_pContext, _Impulse, _iToken);
#endif

	// Find token and graphblock
	int32 iToken = -1;
	int32 Len = m_lTokens.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_lTokens[i].GetID() == _iToken)
		{
			iToken = i;
			break;
		}
	}

	// No token found...
	if (iToken == -1)
		return false;

	// Get a matching reaction from animgraph
	CAG2AnimGraphID iAnimGraph;
	if(_bForce)
	{
		CWAG2I_StateInstance *pStateInstance = m_lTokens[iToken].GetTokenStateInstanceUpdate();
		if(!pStateInstance)
			return false;

		iAnimGraph = pStateInstance->GetAnimGraphIndex();
	}
	else
		iAnimGraph = m_lTokens[iToken].GetAnimGraphIndex();

	{ // TEMP HACK!
		if (iAnimGraph == -1)
			return false;
	}

	const CXRAG2* pAnimGraph = GetAnimGraph(iAnimGraph);
	CAG2ReactionIndex iReaction = pAnimGraph->GetMatchingReaction(m_lTokens[iToken].GetGraphBlock(),_Impulse);
	if (iReaction == -1)
		return false;

	// Ok, found a reaction, check for target graphblock and / or state
	const CXRAG2_Reaction* pReaction = pAnimGraph->GetReaction(iReaction);
	M_ASSERT(pReaction,"CWAG2I::SendImpulse - Reaction nonexistant");
	
	// Do the movetokens and effects
	InvokeEffects(_pContext, iAnimGraph, pReaction->GetBaseEffectInstanceIndex(), pReaction->GetNumEffectInstances());
	
	int32 nMoveTokens = pReaction->GetNumMoveTokens();
	int32 nMoveAnimGraphs = pReaction->GetNumMoveAnimGraphs();
	if (nMoveAnimGraphs != 0)
	{
		// Move over to another animgraph
		const CXRAG2_MoveAnimGraph* pMoveAG = pAnimGraph->GetMoveAnimGraph(pReaction->GetBaseMoveAnimgraphIndex());

		CAG2AnimGraphID iAnimGraphNew = GetAnimGraphIDFromNameHash(pMoveAG->GetAnimGraphName());
		if (iAnimGraphNew == -1)
			return false;

		const CXRAG2* pNewAG = GetAnimGraph(iAnimGraphNew);
		// Ok, found new animgraph, now find a matching graphblock over there...
		CAG2GraphBlockIndex iBlock = pNewAG->GetMatchingGraphBlock(_Impulse);
		if (iBlock == -1)
			return false;

		const CXRAG2_GraphBlock* pBlock = pNewAG->GetGraphBlock(iBlock);
		M_ASSERT(pBlock,"CWAG2I::SendImpulse Invalid graphblock");
		// Movegraphblock to start state of that block
		MoveGraphBlock(_pContext, iAnimGraphNew, _iToken, pBlock->GetStartMoveTokenIndex());
	}
	else if (nMoveTokens == 0)
	{
		// No movetokens found, try to find a matching graphblock instead (might be 
		// a behavior reaction for instance, that just says the graphblock can react to it)
		CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_Impulse);
		if (iBlock == -1)
			return false;
		const CXRAG2_GraphBlock* pBlock = pAnimGraph->GetGraphBlock(iBlock);
		M_ASSERT(pBlock,"CWAG2I::SendImpulse Invalid graphblock");
		// Movegraphblock to start state of that block
		MoveGraphBlock(_pContext, iAnimGraph, _iToken, pBlock->GetStartMoveTokenIndex());
	}
	else
	{
		CAG2MoveTokenIndex iBaseMoveToken = pReaction->GetBaseMoveTokenIndex();
		for (int32 i = 0; i < nMoveTokens; i++)
		{
			int iMoveToken = iBaseMoveToken + i;
			const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(iMoveToken);
			M_ASSERT(pMoveToken,"CWAG2I::SendImpulse Invalid movetoken");
			int8 ActionTokenID = pMoveToken->GetTokenID();
			// if no token is defined, use the given one
			if (ActionTokenID == AG2_TOKENID_NULL)
				ActionTokenID = _iToken;

			int16 iTargetState = pMoveToken->GetTargetStateIndex();
			int16 iTargetGraphBlock = pMoveToken->GetTargetGraphBlockIndex();

			// If the token moved is earlier in the queue, force refresh
			if (ActionTokenID < _iToken)
				ForceRefresh();

			if (iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
				MoveGraphBlock(_pContext, iAnimGraph, ActionTokenID, iMoveToken);
			else if (iTargetState == AG2_STATEINDEX_NULL)
			{
				CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_Impulse);
				if (iBlock == -1)
					return false;
				const CXRAG2_GraphBlock* pBlock = pAnimGraph->GetGraphBlock(iBlock);
				M_ASSERT(pBlock,"CWAG2I::SendImpulse Invalid graphblock");
				// Movegraphblock to start state of that block
				MoveGraphBlock(_pContext, iAnimGraph, ActionTokenID, pBlock->GetStartMoveTokenIndex());
			}
			else
				MoveAction(_pContext, iAnimGraph, ActionTokenID, iMoveToken);
		}
	}


	return true;
}

bool CWAG2I::EvaluateSwitchState(const CWAG2I_Context* _pContext, CAG2MoveTokenIndex& _iMoveToken, CAG2AnimGraphID _iAnimGraph)
{
	if (!AcquireAllResources(_pContext))
		return false;

	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAnimGraph,"INVALID ANIMGRAPH INDEX");
	const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(_iMoveToken);
	M_ASSERT(pMoveToken && pMoveToken->GetTargetStateType(),"CWAG2I::EvaluateSwitchState INVALID MOVETOKENINDEX");
	const CXRAG2_SwitchState* pSwitchState = pAnimGraph->GetSwitchState(pMoveToken->GetTargetStateIndex());
	M_ASSERT(pSwitchState,"CWAG2I::EvaluateSwitchState INVALID SWITCHSTATEINDEX");

	// Evaluate switchstate property
	CAG2Val Val = m_pEvaluator->AG2_EvaluateProperty(_pContext,pSwitchState->GetPropertyID(),pSwitchState->GetPropertyType());
	// This should always be valid (switchstates have a default movetoken)
	_iMoveToken = pAnimGraph->GetMatchingMoveTokenInt(pMoveToken->GetTargetStateIndex(),Val.GetInt());

	return (_iMoveToken != -1);
}

CAG2GraphBlockIndex CWAG2I::GraphBlockExists(const CXRAG2_Impulse& _Impulse, CAG2AnimGraphID _iAnimGraph)
{
	// Check if a graphblock matches given impulse
	const CXRAG2* pAG = GetAnimGraph(_iAnimGraph);
	if (!pAG)
		return -1;

	// Ok, found new animgraph, now find a matching graphblock over there...
	return pAG->GetMatchingGraphBlock(_Impulse);
}

bool CWAG2I::RestartAG(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAG)
{
	// Restarts the animgraph with a new AG
	if (!AcquireAllResources(_pContext))
		return false;
	if (_iAG >= m_lAnimGraph2Res.Len())
		return false;

	// Clear all tokens
	m_lTokens.Clear();
	m_pEvaluator->Clear();
	m_pEvaluator->SetInitialProperties(_pContext);
	// First movetoken should be start movetoken
	MoveGraphBlock(_pContext,_iAG,0,0);
	Refresh(_pContext);
	
	return true;
}

//--------------------------------------------------------------------------------
#define AG2_TOKEN_EFFECT	(2)
uint32 CWAG2I::Refresh(const CWAG2I_Context* _pContext)
{
	MSCOPE(CWAG2I::Refresh, WAG2I);

	m_iRandseed++;

	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);


	if (!AcquireAllResources(_pContext))
		return 0;

	// If no token exists, create a starting default token...
	if (m_lTokens.Len() == 0)
	{
		//_pContext = _pContext->Extend(CMTime(), &AG2IContext);
		// First movetoken should be start movetoken
		MoveGraphBlock(_pContext,0,0,0);
		if (_pContext->m_pObj)
			m_iRandseed = _pContext->m_pObj->m_iObject;
		//MoveAction(_pContext, AG2_TOKENID_DEFAULT, AG2_ACTIONINDEX_START, 0);
	}

	M_ASSERT(m_pEvaluator, "!");
	m_pEvaluator->AG2_RefreshGlobalProperties(_pContext);

	// Check overlay anim...
	if ((m_OverlayAnim.m_Flags & CXRAG2_Animation::XRAG2_OVERLAYREMOVEWHENFINISHED) && m_OverlayAnim.IsValid() &&
		(m_OverlayAnim_StartTime.Compare(AG2I_UNDEFINEDTIME) != 0))
	{
		CMTime ContinousTime = _pContext->m_GameTime - m_OverlayAnim_StartTime;
		spCXR_Anim_SequenceData pOverlayAnimSeq = m_OverlayAnim.GetAnimSequenceData(_pContext->m_pWorldData);
		if (pOverlayAnimSeq && (pOverlayAnimSeq->GetDuration() < ContinousTime.GetTime()))
		{
			ClearOverlayAnim();
			m_pEvaluator->ClearDestinationLock();
		}
	}
	uint32 ReturnRefreshFlags = 0;
	{
	MSCOPESHORT(Tokens);
	// FIXME: Make sure iToken is not discontinuous by adding new tokens, etc.
	m_bNeedUpdate = false;
	int32 TotNumStateInstances = 0;
	bool bHasEffectToken = false;
	int32 NumForceRefresh = 0;
WAG2I_FORCEREFRESH:
	m_bForceRefresh = false;
	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		// Create copy of context so that the original is kept safe from splits and extends.
		CWAG2I_Context TokenContext;
		_pContext->Duplicate(&TokenContext);

		int nRefreshs = 0;
		CWAG2I_Token* pToken = &(m_lTokens[iToken]);
		if ((pToken->m_Flags & TOKENFLAGS_NOREFRESH) && pToken->GetNumStateInstances() == 1)
			continue;
		if (pToken->GetID() == AG2_TOKEN_EFFECT)
			bHasEffectToken = true;
		uint32 RefreshFlags = AG2I_TOKENREFRESHFLAGS_REFRESH;
		while ((RefreshFlags & AG2I_TOKENREFRESHFLAGS_REFRESH) != 0)
		{
			RefreshFlags = pToken->Refresh(&TokenContext);
			ReturnRefreshFlags |= RefreshFlags;
			if (m_lTokens.Len() <= iToken)
				break;
			pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.

			if ((RefreshFlags & AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION) != 0)
			{
				m_bNeedUpdate = true;
				CWAG2I_StateInstance* pTokenStateInstance = pToken->GetTokenStateInstance();
				if ((pTokenStateInstance != NULL) && (!pTokenStateInstance->HasAnimation()))
				{
#ifdef	AG2_DEBUG
					if ((RefreshFlags & AG2I_TOKENREFRESHFLAGS_REFRESH) == 0)
					{
						if (CWAG2I::DebugEnabled(_pContext))
						{
/*
							if (_pContext->m_pWPhysState->IsServer())
								ConOutL(CStrF("(Server), iObj %d (%X)) Token%d was forced to leave/refresh State%d.", _pContext->m_pObj->m_iObject, _pContext->m_pObj, pToken->GetID(), pToken->GetStateIndex()));
							else
								ConOutL(CStrF("(Client), iObj %d (%X)) Token%d was forced to leave/refresh State%d.", _pContext->m_pObj->m_iObject, _pContext->m_pObj, pToken->GetID(), pToken->GetStateIndex()));
*/						}
					}
#endif
					RefreshFlags = AG2I_TOKENREFRESHFLAGS_REFRESH;
				}
			}

			nRefreshs++;
			if (nRefreshs > 100)
			{
#ifdef	AG2_DEBUG
				if (_pContext->m_pWPhysState->IsServer())
					ConOutL(CStrF("(Server) ERROR: AG2I::Refresh() - Possible infinite refreshs for Token %d in State %d. Breaking out...", pToken->GetID(), pToken->GetStateIndex()));
				else
					ConOutL(CStrF("(CLIENT) ERROR: AG2I::Refresh() - Possible infinite refreshs for Token %d in State %d. Breaking out...", pToken->GetID(), pToken->GetStateIndex()));
#endif
				break;
			}
		}
		if (m_lTokens.Len() <= iToken)
			break;
		pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.
#ifndef M_RTM
		const CWAG2I_StateInstance* pInstance = pToken->GetTokenStateInstance();
		if ((pToken->GetID() == 0) && pInstance && (!pInstance->HasAnimation()))
		{
			if (CWAG2I::DebugEnabled(_pContext))
			{
				if (_pContext->m_pWPhysState->IsServer())
					ConOutL(CStrF("(Server) WARNING: Token%d in State%d has no animation.", pToken->GetID(), pToken->GetStateIndex()));
				else
					ConOutL(CStrF("(Client) WARNING: Token%d in State%d has no animation.", pToken->GetID(), pToken->GetStateIndex()));

			}
		}
#endif
		int32 NumStateInstances = pToken->GetNumStateInstances();
		TotNumStateInstances += NumStateInstances;
		if (NumStateInstances == 0)
		{
			//ConOutL(CStrF("CWAG2I::Refresh() - Terminating Token %d", pToken->GetID()));
			RemoveToken(iToken--);
			//m_lTokens.Del(iToken--);
		}
	}
		if (m_bForceRefresh && (NumForceRefresh < 3))
		{
			// Don't wanne be stuck here forever if there's a mistake
			NumForceRefresh++;
			goto WAG2I_FORCEREFRESH;
		}
		// Increment importantag ticker if we don't have too many state instances
		if ((TotNumStateInstances <= m_lTokens.Len()) && !bHasEffectToken)
		{
			m_pEvaluator->IncrementLastAGEvent();
		}
	}
	return ReturnRefreshFlags;
}

void CWAG2I::RefreshToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CAG2AnimGraphID _iAG)
{
	MSCOPE(CWAG2I::Refresh, WAG2I);

	m_iRandseed++;

	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);


	if (!AcquireAllResourcesToken(_pContext))
		return;
	
	if (!(m_lspAnimGraph2.Len() > _iAG) || !m_lspAnimGraph2[_iAG])
		return;

	// If no token exists, create a starting default token...
	if (m_lTokens.Len() == 0)
	{
		_pContext = _pContext->Extend(CMTime(), &AG2IContext);
		// First movetoken should be start movetoken
		MoveGraphBlock(_pContext,_iAG,_iToken,0);
		if (_pContext->m_pObj)
			m_iRandseed = _pContext->m_pObj->m_iObject;
		//MoveAction(_pContext, AG2_TOKENID_DEFAULT, AG2_ACTIONINDEX_START, 0);
	}

	M_ASSERT(m_pEvaluator, "!");
	m_pEvaluator->AG2_RefreshGlobalProperties(_pContext);

	{
		MSCOPESHORT(Tokens);
		// FIXME: Make sure iToken is not discontinuous by adding new tokens, etc.
//		int32 TotNumStateInstances = 0;
		int32 NumForceRefresh = 0;
WAG2I_FORCEREFRESH:
		m_bForceRefresh = false;
		// Find token
		int32 iToken = GetTokenIndexFromID(_iToken);
		if (iToken == -1)
		{
			_pContext = _pContext->Extend(CMTime(), &AG2IContext);
			// First movetoken should be start movetoken
			MoveGraphBlock(_pContext,_iAG,_iToken,0);

			iToken = GetTokenIndexFromID(_iToken);
			if (iToken == -1)
				return;
		}

		CWAG2I_Token* pToken = &(m_lTokens[iToken]);	

		// Create copy of context so that the original is kept safe from splits and extends.
		CWAG2I_Context TokenContext;
		_pContext->Duplicate(&TokenContext);

		int nRefreshs = 0;
		uint32 RefreshFlags = AG2I_TOKENREFRESHFLAGS_REFRESH;
		while ((RefreshFlags & AG2I_TOKENREFRESHFLAGS_REFRESH) != 0)
		{
			RefreshFlags = pToken->Refresh(&TokenContext);
			if (m_lTokens.Len() <= iToken)
				break;
			pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.

			if ((RefreshFlags & AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION) != 0)
			{
				CWAG2I_StateInstance* pTokenStateInstance = pToken->GetTokenStateInstance();
				if ((pTokenStateInstance != NULL) && (!pTokenStateInstance->HasAnimation()))
				{
					RefreshFlags = AG2I_TOKENREFRESHFLAGS_REFRESH;
				}
			}

			nRefreshs++;
			if (nRefreshs > 100)
				break;
		}
		pToken = &(m_lTokens[iToken]); // In case m_lTokens array have been reallocated.
		if (pToken->GetNumStateInstances() == 0)
		{
			//ConOutL(CStrF("CWAG2I::Refresh() - Terminating Token %d", pToken->GetID()));
			RemoveTokenByID(_iToken);
		}

		if (m_bForceRefresh && (NumForceRefresh < 3))
		{
			// Don't wanne be stuck here forever if there's a mistake
			NumForceRefresh++;
			goto WAG2I_FORCEREFRESH;
		}
	}
}

//--------------------------------------------------------------------------------

void CWAG2I::RefreshPredictionMisses(const CWAG2I_Context* _pContext)
{
	MSCOPE(CWAG2I::RefreshPredictionMisses, WAG2I);

/*
	CWAG2I_Context AG2IContext; // Create copy of context so that the original is kept safe from splits and extends.
	_pContext = _pContext->Duplicate(&AG2IContext);
*/
	//_pContext = _pContext->ApplyTimeScale(0.1f, 0.0f, &AG2IContext);
	TAP_RCD<CWAG2I_Token> lTokens = m_lTokens;
	for (int iToken = 0; iToken < lTokens.Len(); iToken++)
	{
		CWAG2I_Token* pToken = &(lTokens[iToken]);
		pToken->RefreshPredictionMisses(_pContext);
	}
}


void CWAG2I::EndTimeLeap(CWAG2I_Context* _pContext)
{
	// Save a copy of all tokens affecting stateinstances, to inject when the timeleap is ending

	// TimeLeap the individual tokens
	TAP_RCD<CWAG2I_Token> lTokens = m_lTokens;
	for (int i = 0; i < lTokens.Len(); i++)
	{
		lTokens[i].EndTimeLeap(_pContext);
	}
}

#ifndef M_RTM
	//Get debug info about current state for all tokens
CStr CWAG2I::DebugStateInfo(const CWAG2I_Context* _pContext)
{
  	CStr Res = CStrF("(iObj: %d, GameTime: %3.2f)", _pContext->m_pObj->m_iObject, _pContext->m_GameTime.GetTime());
 	if (m_lTokens.Len())
	{
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			Res += CStrF("|    Token %d: State ", iToken);
			CWAG2I_Token* pToken = &(m_lTokens[iToken]);
			CAG2AnimGraphID iAnimGraph = pToken->GetAnimGraphIndex();
			const CXRAG2_State* pState = GetState(pToken->GetStateIndex(), iAnimGraph);
			if (pState != NULL)
			{
				int16 iState = pToken->GetStateIndex();
				if (iState != AG2_STATEINDEX_NULL)
				{
					if (iState != AG2_STATEINDEX_TERMINATE)
					{
						iState &= AG2_TARGETSTATE_INDEXMASK;
						CStr StateName = GetStateName(iState,iAnimGraph);
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

				const CXRAG2_AnimLayer* pAnimLayer = GetAnimLayer(pState->GetBaseAnimLayerIndex(),iAnimGraph);
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


#if	defined(M_RTM) && defined( AG2_DEBUG )
#warning "M_RTM and AG2_DEBUG at the same time (slow code)"
#endif

//--------------------------------------------------------------------------------
