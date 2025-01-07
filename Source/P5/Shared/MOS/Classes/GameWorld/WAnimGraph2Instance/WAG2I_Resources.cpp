//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WAG2I.h"
#include "../WDataRes_AnimGraph2.h"
#include "../../../../../Projects/Main/GameWorld/WServerMod.h" // Forbidden! Move func to WServer_Core?

//--------------------------------------------------------------------------------

void CWAG2I::AddResourceIndex_AnimGraph2(int32 _iAnimGraph2Res, const CStr& _AGName)
{
	m_lAnimGraph2Res.Add(CAG2Res(_iAnimGraph2Res,StringToHash(_AGName.UpperCase())));
}

void CWAG2I::SetResourceIndex_AnimGraph2(int32 _iAnimGraph2Res, const CStr& _AGName,int32 _iSlot)
{
	if (m_lAnimGraph2Res.Len() <= _iSlot)
		m_lAnimGraph2Res.SetLen(_iSlot + 1);

	m_lAnimGraph2Res[_iSlot] = CAG2Res(_iAnimGraph2Res,StringToHash(_AGName.UpperCase()));
}

void CWAG2I::ClearSessionInfo(int32 _iSlot)
{
	if (m_lAnimGraph2Res.Len() > _iSlot)
		m_lAnimGraph2Res[_iSlot] = CAG2Res();
	m_lspAnimGraph2.Clear();
	m_lTokens.Clear();
	m_OverlayAnim.Clear();
	m_OverlayAnimLipSync.Clear();
	m_OverLayAnimLipSyncBaseJoint = 0;
	m_OverlayAnim_StartTime = AG2I_UNDEFINEDTIME;
	m_LastPackMode = -1;
}

//--------------------------------------------------------------------------------

void CWAG2I::SetOverlayAnim(int32 _iAnimContainerResource, int16 _iAnimSeq, CMTime _StartTime, uint8 _Flags)
{
	m_OverlayAnim = CXRAG2_Animation(_iAnimContainerResource, _iAnimSeq);
	m_OverlayAnim.m_Flags = _Flags;
	m_OverlayAnim_StartTime = _StartTime;
	m_DirtyFlag |= AG2I_DIRTYFLAG_OVERLAY;
}

void CWAG2I::SetOverlayAnimLipSync(int32 _iAnimContainerResource, int16 _iAnimSeq, int8 _BaseJoint)
{
	m_OverlayAnimLipSync = CXRAG2_Animation(_iAnimContainerResource, _iAnimSeq);
	m_OverLayAnimLipSyncBaseJoint = _BaseJoint;
	m_DirtyFlag |= AG2I_DIRTYFLAG_OVERLAY_LIPSYNC;
}

//--------------------------------------------------------------------------------

void CWAG2I::ClearOverlayAnim()
{
	m_OverlayAnim.Clear();
	m_OverlayAnimLipSync.Clear();
	m_OverLayAnimLipSyncBaseJoint = 0;
	m_OverlayAnim_StartTime = AG2I_UNDEFINEDTIME;
}

//--------------------------------------------------------------------------------

bool CWAG2I::AcquireAllResources(const CWAG2I_Context *_pContext)
{
	MSCOPESHORT(CWAG2I::AcquireAllResources);
	int32 Len = m_lAnimGraph2Res.Len();
	if (Len > 0 && Len == m_lspAnimGraph2.Len())
		return true;
	else if (!_pContext->m_pWPhysState || Len == 0)
		return false;

	m_lspAnimGraph2.SetLen(Len);
	CAG2Res* lAGRes = m_lAnimGraph2Res.GetBasePtr();
	spCXRAG2* lspAG = m_lspAnimGraph2.GetBasePtr();

	for (int32 i = 0; i < Len; i++)
	{
		CWRes_AnimGraph2* pAnimGraph2Res = _pContext->m_pWPhysState->GetMapData()->GetResource_AnimGraph2(lAGRes[i].m_iResource);

		if (!pAnimGraph2Res)
		{
			m_lspAnimGraph2.Clear();
			return false;
		}
		lspAG[i] = pAnimGraph2Res->GetAnimGraph();	
	}

	return true;
}

bool CWAG2I::AcquireAllResourcesToken(const CWAG2I_Context* _pContext)
{
	MSCOPESHORT(CWAG2I::AcquireAllResources);
	int32 Len = m_lAnimGraph2Res.Len();
	if (Len > 0 && Len == m_lspAnimGraph2.Len())
		return true;
	else if (!_pContext->m_pWPhysState || Len == 0)
		return false;

	m_lspAnimGraph2.SetLen(Len);
	CAG2Res* lAGRes = m_lAnimGraph2Res.GetBasePtr();
	spCXRAG2* lspAG = m_lspAnimGraph2.GetBasePtr();
	bool bAllCrap = true;
	for (int32 i = 0; i < Len; i++)
	{
		CWRes_AnimGraph2* pAnimGraph2Res = _pContext->m_pWPhysState->GetMapData()->GetResource_AnimGraph2(lAGRes[i].m_iResource);

		if (!pAnimGraph2Res)
		{
			lspAG[i] = NULL;
			continue;
			//m_lspAnimGraph2.Clear();
			//return false;
		}
		bAllCrap = false;
		lspAG[i] = pAnimGraph2Res->GetAnimGraph();	
	}
	if (bAllCrap)
		m_lspAnimGraph2.Clear();

	return !bAllCrap;
}

bool CWAG2I::AcquireAllResourcesFromMapData(CMapData* _pMapData)
{
	MSCOPESHORT(CWAG2I::AcquireAllResourcesFromMapData);
	if (!_pMapData || !m_lAnimGraph2Res.Len())
		return false;

	int32 Len = m_lAnimGraph2Res.Len();
	if (Len == m_lspAnimGraph2.Len())
		return true;

	m_lspAnimGraph2.SetLen(Len);
	CAG2Res* lAGRes = m_lAnimGraph2Res.GetBasePtr();
	spCXRAG2* lspAG = m_lspAnimGraph2.GetBasePtr();

	for (int32 i = 0; i < Len; i++)
	{
		CWRes_AnimGraph2* pAnimGraph2Res = _pMapData->GetResource_AnimGraph2(lAGRes[i].m_iResource);

		if (!pAnimGraph2Res)
		{
			m_lspAnimGraph2.Clear();
			return false;
		}
		lspAG[i] = pAnimGraph2Res->GetAnimGraph();	
	}

	return true;
}

//--------------------------------------------------------------------------------

void CWAG2I::ClearAnimListCache()
{
	// Go through all anim names and clear cache
	int32 Len = m_lspAnimGraph2.Len();
	for (int32 i = 0; i < Len; i++)
		if (m_lspAnimGraph2[i])
			m_lspAnimGraph2[i]->ClearAnimSequenceCache();
}

void CWAG2I::TagAnimSetFromImpulses(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<CXRAG2_Impulse>& _lImpulses)
{
	MSCOPESHORT(CWAG2I::TagAnimSetFromImpulses);
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Hmm, so basically, find matching graphblocks from given impulses, go to first and last 
	// state, check animlayer indexes and tag all animations in those animlayers.
	for (int32 i = 0; i < m_lspAnimGraph2.Len(); i++)
	{
		const CXRAG2* pAnimGraph = GetAnimGraph(i);
		int32 ILen = _lImpulses.Len();
		for (int32 i = 0; i < ILen; i++)
		{
			CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_lImpulses[i]);
			if (iBlock != -1)
			{
				// Find first and last animlayer
				const CXRAG2_GraphBlock* pBlock = pAnimGraph->GetGraphBlock(iBlock);
				M_ASSERT(pBlock,"CWAG2I::TagAnimSetFromImpulses Invalid graphblock");
				// No states, no animations
				if (pBlock->GetNumStates() <= 0)
					continue;

				const CXRAG2_State* pFirstState = pAnimGraph->GetState(pBlock->GetBaseStateIndex());
				const CXRAG2_State* pLastState = pAnimGraph->GetState(pBlock->GetBaseStateIndex() + pBlock->GetNumStates() - 1);
				int32 iBaseAnimLayer = pFirstState->GetBaseAnimLayerIndex();
				int32 iLastAnimLayer = pLastState->GetBaseAnimLayerIndex() + pLastState->GetNumAnimLayers();
				int32 NumLayers = iLastAnimLayer - iBaseAnimLayer;
				for (int32 i = 0; i < NumLayers; i++)
				{
					const CXRAG2_AnimLayer* pLayer = pAnimGraph->GetAnimLayer(iBaseAnimLayer + i);
					M_ASSERT(pLayer, "CWAG2I::TagAnimSetFromImpulses Invalid AnimLayer");
					// If no precache, continue
					if (pLayer->m_AnimFlags & CXR_ANIMLAYER_NOPRECACHE)
						continue;
					const CXRAG2_AnimNames* pAnimName = pAnimGraph->GetAnimName(pLayer->GetAnimIndex());
					M_ASSERT(pAnimName, "CWAG2I::TagAnimSetFromImpulses Invalid AnimName");

					CStr ContainerName = pAnimGraph->GetAnimContainerName(pAnimName->m_iContainerName);
					//ConOutL(CStrF("Adding animation: %s:%d",ContainerName.Str(),pAnimName->m_iAnimSeq));
					pServerMod->AddAnimContainerEntry(ContainerName, pAnimName->m_iAnimSeq);
				}
			}
		}
	}
}

void CWAG2I::TagAnimSetFromBlockReaction(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const CXRAG2_Impulse& _BlockImpulse, const TArray<CXRAG2_Impulse>& _lImpulses, bool _bEvenNoPreCache)
{
	// Ok, this will find an animation block from the blockimpulse and tag any animations that 
	// the blocks reactions point to in a state

	MSCOPESHORT(CWAG2I::TagAnimSetFromImpulses);
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Hmm, so basically, find matching graphblocks from given impulses, go to first and last 
	// state, check animlayer indexes and tag all animations in those animlayers.
	int32 ImpulseLen = _lImpulses.Len();
	for (int32 i = 0; i < m_lspAnimGraph2.Len(); i++)
	{
		const CXRAG2* pAnimGraph = GetAnimGraph(i);

		// Get matching block
		CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_BlockImpulse);
		if (iBlock == -1)
			continue;
		for (int32 iImpulse = 0; iImpulse < ImpulseLen; iImpulse++)
		{
			CAG2ReactionIndex iReaction = pAnimGraph->GetMatchingReaction(iBlock,_lImpulses[iImpulse]);
			if (iReaction == -1)
				continue;
			
			const CXRAG2_Reaction* pReaction = pAnimGraph->GetReaction(iReaction);
			// Get targetstate if it has any movetokens within the graphblock
			int32 nMoveTokens = pReaction->GetNumMoveTokens();
			for (int32 iMoveToken = pReaction->GetBaseMoveTokenIndex(); iMoveToken < (pReaction->GetBaseMoveTokenIndex() + nMoveTokens); iMoveToken++)
			{
				const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(iMoveToken);
				if (pMoveToken->m_iTargetGraphBlock != -1 && 
					pMoveToken->m_iTargetGraphBlock != iBlock)
					continue;

				if (pMoveToken->m_iTargetState != -1 && !pMoveToken->m_TargetStateType)
				{
					const CXRAG2_State* pState = pAnimGraph->GetState(pMoveToken->m_iTargetState);
					int32 nAnimLayers = pState->GetNumAnimLayers();
					for (int32 iAnimLayer = pState->GetBaseAnimLayerIndex(); iAnimLayer < (pState->GetBaseAnimLayerIndex() + nAnimLayers); iAnimLayer++)
					{
						const CXRAG2_AnimLayer* pLayer = pAnimGraph->GetAnimLayer(iAnimLayer);
						M_ASSERT(pLayer, "CWAG2I::TagAnimSetFromBlockReaction Invalid AnimLayer");
						// If no precache, continue
						if (!_bEvenNoPreCache && (pLayer->m_AnimFlags & CXR_ANIMLAYER_NOPRECACHE))
							continue;
						const CXRAG2_AnimNames* pAnimName = pAnimGraph->GetAnimName(pLayer->GetAnimIndex());
						M_ASSERT(pAnimName, "CWAG2I::TagAnimSetFromBlockReaction Invalid AnimName");

						CStr ContainerName = pAnimGraph->GetAnimContainerName(pAnimName->m_iContainerName);
						//ConOutL(CStrF("Adding animation: %s:%d",ContainerName.Str(),pAnimName->m_iAnimSeq));
						pServerMod->AddAnimContainerEntry(ContainerName, pAnimName->m_iAnimSeq);
					}
				}
			}
		}
	}
}

void CWAG2I::TagAnimSetFromBlockReactionSwitchState(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<CXRAG2_Impulse>& _lBlockImpulses, const TArray<CXRAG2_Impulse>& _lReactionImpulses,const TArray<int32>& _lActionVals)
{
	MSCOPESHORT(CWAG2I::TagAnimSetFromImpulses);
	if (!AcquireAllResources(_pContext) || _pContext->m_pWPhysState->IsClient())
		return;

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pContext->m_pWPhysState);
	if (!pServerMod)
		return;

	// Hmm, so basically, find matching graphblocks from given impulses, go to first and last 
	// state, check animlayer indexes and tag all animations in those animlayers.
	const int32 BlockImpulseLen = _lBlockImpulses.Len();
	const int32 ReactionImpulseLen = _lReactionImpulses.Len();
	const int32 ActionValLen = _lActionVals.Len();
	for (int32 i = 0; i < m_lspAnimGraph2.Len(); i++)
	{
		const CXRAG2* pAnimGraph = GetAnimGraph(i);

		for (int32 iBlockImp = 0; iBlockImp < BlockImpulseLen; iBlockImp++)
		{
			// Get matching block
			CAG2GraphBlockIndex iBlock = pAnimGraph->GetMatchingGraphBlock(_lBlockImpulses[iBlockImp]);
			if (iBlock == -1)
				continue;
			for (int32 iReactionImp = 0; iReactionImp < ReactionImpulseLen; iReactionImp++)
			{
				CAG2ReactionIndex iReaction = pAnimGraph->GetMatchingReaction(iBlock,_lReactionImpulses[iReactionImp]);
				if (iReaction == -1)
					continue;

				const CXRAG2_Reaction* pReaction = pAnimGraph->GetReaction(iReaction);
				// Get targetstate if it has any movetokens within the graphblock
				int32 nMoveTokens = pReaction->GetNumMoveTokens();
				for (int32 iMoveToken = pReaction->GetBaseMoveTokenIndex(); iMoveToken < (pReaction->GetBaseMoveTokenIndex() + nMoveTokens); iMoveToken++)
				{
					const CXRAG2_MoveToken* pMoveToken = pAnimGraph->GetMoveToken(iMoveToken);
					if (pMoveToken->m_iTargetGraphBlock != -1 && 
						pMoveToken->m_iTargetGraphBlock != iBlock)
						continue;

					// Make sure we've found a switchstate?
					if (pMoveToken->m_iTargetState != -1 && pMoveToken->m_TargetStateType)
					{
						// Ok, got a switchstate, evalutate it and see what state we end up in (with the property vals given)
						for (int32 iActionVal = 0; iActionVal < ActionValLen; iActionVal++)
						{
							CAG2MoveTokenIndex iMT = pAnimGraph->GetMatchingMoveTokenInt(pMoveToken->m_iTargetState,_lActionVals[iActionVal]);
							const CXRAG2_MoveToken* pMoveTokenAV = pAnimGraph->GetMoveToken(iMT);
							if (!pMoveTokenAV || pMoveTokenAV->m_TargetStateType)
								continue;

							const CXRAG2_State* pState = pAnimGraph->GetState(pMoveTokenAV->m_iTargetState);
							M_ASSERT(pState, "CWAG2I::TagAnimSetFromBlockReaction Invalid State");
							int32 nAnimLayers = pState->GetNumAnimLayers();
							for (int32 iAnimLayer = pState->GetBaseAnimLayerIndex(); iAnimLayer < (pState->GetBaseAnimLayerIndex() + nAnimLayers); iAnimLayer++)
							{
								const CXRAG2_AnimLayer* pLayer = pAnimGraph->GetAnimLayer(iAnimLayer);
								M_ASSERT(pLayer, "CWAG2I::TagAnimSetFromBlockReaction Invalid AnimLayer");
								const CXRAG2_AnimNames* pAnimName = pAnimGraph->GetAnimName(pLayer->GetAnimIndex());
								M_ASSERT(pAnimName, "CWAG2I::TagAnimSetFromBlockReactionSwitchState Invalid AnimName");

								CStr ContainerName = pAnimGraph->GetAnimContainerName(pAnimName->m_iContainerName);
								//ConOutL(CStrF("Adding animation: %s:%d",ContainerName.Str(),pAnimName->m_iAnimSeq));
								pServerMod->AddAnimContainerEntry(ContainerName, pAnimName->m_iAnimSeq);
							}
						}
					}
				}
			}
		}
	}
}
