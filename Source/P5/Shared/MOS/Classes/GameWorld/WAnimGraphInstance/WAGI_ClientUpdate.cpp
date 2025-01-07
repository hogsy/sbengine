#include "PCH.h"

//--------------------------------------------------------------------------------

#include "WAGI.h"
#include "WAGI_StateInstPacked.h"
#include "WAG_ClientData.h"
//#include "../../../XR/XRAnimGraph/AnimGraph.h"
#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"
//#include "../WDataRes_AnimGraph.h"
//#include "../WPhysState.h"

//--------------------------------------------------------------------------------
#if defined(AG_DEBUG) || !defined(M_RTM)
static bool bDebug = true;
static bool bDebugSync = false;
//#define AG_DEBUG_LOG ConOutL
void AG_DEBUG_LOG(const CStr& _str) { M_TRACE("%s\n", _str.Str()); }
#endif
//--------------------------------------------------------------------------------

//#define AG_NODIFF

//--------------------------------------------------------------------------------

int CWAGI::OnCreateClientUpdate(uint8* _pData, const CWAGI_Packed* _pPAGI, int8 _Type) const
{
	MSCOPESHORT(CWAGI::OnCreateClientUpdate);

	if ((_pData == NULL) || (_pPAGI == NULL))
		return 0;

	uint8* pBase = _pData;
	// Put type of update
	PTR_PUTINT8(_pData,_Type);
	if (_Type == WAGI_CLIENTUPDATE_DIRECTACCESS)
	{

		// Game is singleplayer, so we can get away with just sending the 'this'-pointer as an uint32
		//((CWAGI*)this)->MRTC_AddRef();

		PTR_PUTMINT(_pData, mint(this));
	}
	else
	{
		CWAGI_Packed ServerPAGI;
		CreatePackedState(&ServerPAGI);
		const CWAGI_Packed& ClientPAGI = *_pPAGI;
		CWAGI_Packed DiffPAGI;
		ServerPAGI.CreateDiff(&ClientPAGI, &DiffPAGI);
		_pData += DiffPAGI.Write(_pData);

		#ifdef	AG_DEBUG
			bool bLocalDebug = false;
			if (DiffPAGI.m_Flags == 0)
				bLocalDebug = false;

			if (bLocalDebug)
			{
				AG_DEBUG_LOG("ServerSync BEGIN ----------");
				Log("  ", "ServerAGI");
				ServerPAGI.Log("  ", "ServerPAGI");
				ClientPAGI.Log("  ", "ClientMirrorPAGI");
				DiffPAGI.Log("  ", "ServerDiffPAGI");
				AG_DEBUG_LOG("ServerSync END ----------");
			}
		#endif
	}

	return (_pData - pBase);
}

//--------------------------------------------------------------------------------

int CWAGI::OnClientUpdate(CWAGI_Context* _pContext, const uint8* _pData, CWAGI_Packed* _pPAGI, CWAGI* _pAGI)
{
	MSCOPESHORT(CWAGI::OnClientUpdate);

	if ((_pData == NULL) || (_pPAGI == NULL))
		return 0;

	const uint8* pBase = _pData;
	int8 Type = 0;
	PTR_GETINT8(_pData,Type);

	if (Type == WAGI_CLIENTUPDATE_DIRECTACCESS)
	{
		M_ASSERT(sizeof(mint) == sizeof(CWAGI*), "Unsupported pointer size!");
		CWAGI* pServerAGI;
		PTR_GETMINT(_pData, (mint&)pServerAGI);
		M_ASSERT(pServerAGI, "Bad data!");
		if (_pAGI)
		{
			int x = _pAGI->m_nAnimListRefs;
			_pAGI->CopyFrom(pServerAGI);
			_pAGI->m_nAnimListRefs = x;
			_pAGI->ClientConsumeEnterStateEntries(_pContext);
			pServerAGI->ServerClearEnterStateEntries();
			_pAGI->m_pEvaluator->SetDummyAG((spCWAGI)pServerAGI);
		}
		//pServerAGI->MRTC_DelRef();
	}
	else
	{
		CWAGI_Packed DiffPAGI; 
		_pData += DiffPAGI.Read(_pData);
		CWAGI_Packed& ClientPAGI = *_pPAGI;

		#ifdef	AG_DEBUG
			bool bLocalDebug = (Type == 2); //DEBUG
			if (DiffPAGI.m_Flags == 0)
				bLocalDebug = false;

			if (bLocalDebug)
			{
				AG_DEBUG_LOG(CStrF("ClientSync BEGIN (pWClient %X, pWObj %X (iObj %d), GameTime %3.3f) ----------", _pContext->m_pWPhysState, _pContext->m_pObj, _pContext->m_pObj->m_iObject, _pContext->m_GameTime.GetTime()));
				DiffPAGI.Log("  ", "ClientDiffPAGI");
				ClientPAGI.Log("  ", "ClientPAGI(pre)");
			}
		#endif

			ClientPAGI.UpdateWithDiff(&DiffPAGI);

		#ifdef	AG_DEBUG
			if (bLocalDebug)
				ClientPAGI.Log("  ", "ClientPAGI(post)");
		#endif

			if (_pAGI != NULL)
			{
		#ifdef	AG_DEBUG
				if (bLocalDebug)
					_pAGI->Log("  ", "ClientAGI(pre)");
		#endif
				_pAGI->SyncWithPAGI(_pContext, &ClientPAGI);
				_pAGI->ClientConsumeEnterStateEntries(_pContext);

		#ifdef	AG_DEBUG
				if (bLocalDebug)
					_pAGI->Log("  ", "ClientAGI(post)");
		#endif
			}

		#ifdef	AG_DEBUG
			if (bLocalDebug)
				AG_DEBUG_LOG("ClientSync END ----------");
		#endif
	}

	return (_pData - pBase);
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

// Run on server
void CWAGI::CreatePackedState(CWAGI_Packed* _pPAGI) const
{
	MSCOPESHORT(CWAGI::CreatePackedState);

	_pPAGI->m_Flags = 0;

	_pPAGI->m_Flags |= AGI_PACKEDAGI_RANDSEED;
	_pPAGI->m_iRandseed = m_iRandseed;

	_pPAGI->m_Flags |= AGI_PACKEDAGI_RESOURCEINDICES;
	_pPAGI->m_iAnimGraphRes = m_iAnimGraphRes;
	_pPAGI->m_iAnimListRes = m_iAnimListRes;

	if (m_OverlayAnim.IsValid() && !m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME))
	{
		_pPAGI->m_Flags |= AGI_PACKEDAGI_OVERLAYANIM;
		_pPAGI->m_OverlayAnim_iAnimContainerResource = m_OverlayAnim.m_iAnimContainerResource;
		_pPAGI->m_OverlayAnim_iAnimSeq = m_OverlayAnim.m_iAnimSeq;
		_pPAGI->m_OverlayAnim_StartTime = m_OverlayAnim_StartTime;
	}
	if (m_OverlayAnimLipSync.IsValid() && !m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME))
	{
		_pPAGI->m_Flags |= AGI_PACKEDAGI_OVERLAYANIMLIPSYNC;
		_pPAGI->m_OverlayAnim_iAnimContainerResourceLipSync = m_OverlayAnimLipSync.m_iAnimContainerResource;
		_pPAGI->m_OverlayAnim_iAnimSeqLipSync = m_OverlayAnimLipSync.m_iAnimSeq;
		_pPAGI->m_OverlayAnim_LipSyncBaseJoint = m_OverLayAnimLipSyncBaseJoint;
	}

	if (m_lTokens.Len() > 0)
	{
		_pPAGI->m_Flags |= AGI_PACKEDAGI_TOKENS;

		_pPAGI->m_lTokens.SetLen(m_lTokens.Len());
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
		{
			const CWAGI_Token* pToken = &(m_lTokens[iToken]);
			CWAGI_Token_Packed* pPToken = &(_pPAGI->m_lTokens[iToken]);

			pPToken->m_ID = pToken->GetID();
			pPToken->m_Flags = 0;

/*
			if (m_lTokens[iToken].IsFirstRefresh())
			pPToken->m_Flags |= AGI_PACKEDTOKEN_FIRSTREFRESH;
*/

			pPToken->m_Flags |= AGI_PACKEDTOKEN_REFRESHGAMETIME;
			pPToken->m_RefreshGameTime = pToken->GetRefreshGameTime();

			pPToken->m_Flags |= AGI_PACKEDTOKEN_REFRESHGAMETIME;

			pPToken->m_Flags |= AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE;
			pPToken->m_PlayingSIID = pToken->GetPlayingSIID();

			uint8 nSIPs = pToken->GetNumStateInstances();
			if (nSIPs > 0)
			{
				pPToken->m_Flags |= AGI_PACKEDTOKEN_SIPS;

				pPToken->m_lSIPs.SetLen(nSIPs);
				for (int iSIP = 0; iSIP < nSIPs; iSIP++)
				{
					const CWAGI_StateInstance* pStateInstance = pToken->GetStateInstance(iSIP);
					pStateInstance->PackSIP(&(pPToken->m_lSIPs[iSIP]));
				}
			}
		}
	}

	if (m_bDisableAll)
		_pPAGI->m_Flags |= AGI_PACKEDAGI_ISDISABLED;

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	int8 Len = (int8)m_lEnterStateEntries.Len();
	if (Len > 0)
	{
		_pPAGI->m_Flags |= AGI_PACKEDAGI_HASENTERSTATEENTRIES;
		_pPAGI->m_lEnterStateEntries.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			_pPAGI->m_lEnterStateEntries[i].CopyFrom(m_lEnterStateEntries[i]);
	}
#endif
}

//--------------------------------------------------------------------------------

// Run on client
void CWAGI::SyncWithPAGI(CWAGI_Context* _pContext, const CWAGI_Packed* _pClientPAGI)
{
	MSCOPESHORT(CWAGI::SyncWithPAGI);

	if ((_pClientPAGI->m_Flags & AGI_PACKEDAGI_RANDSEED) == 0)
	{
		// FIXME: Look into this shit later sometime...
//		AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - ClientPAGI has no randseed.");
//		return;
	}

	m_iRandseed = _pClientPAGI->m_iRandseed;

	if ((_pClientPAGI->m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) == 0)
	{
#ifdef	AG_DEBUG
		AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - ClientPAGI has no resource indices.");
#endif
		return;
	}

#ifdef	AG_DEBUG
	if (_pClientPAGI->m_iAnimGraphRes == 0)
		AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - Invalid iAGRes");

	if (_pClientPAGI->m_iAnimListRes == 0)
		AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - Invalid iALRes");
#endif

	m_iAnimGraphRes = _pClientPAGI->m_iAnimGraphRes;
	m_iAnimListRes = _pClientPAGI->m_iAnimListRes;

	if ((_pClientPAGI->m_Flags & AGI_PACKEDAGI_OVERLAYANIM) != 0)
	{
		if (_pClientPAGI->m_OverlayAnim_iAnimContainerResource != m_OverlayAnim.m_iAnimContainerResource || 
			_pClientPAGI->m_OverlayAnim_iAnimSeq != m_OverlayAnim.m_iAnimSeq)
			m_OverlayAnim.ClearCache();
		m_OverlayAnim.m_iAnimContainerResource = _pClientPAGI->m_OverlayAnim_iAnimContainerResource;
		m_OverlayAnim.m_iAnimSeq = _pClientPAGI->m_OverlayAnim_iAnimSeq;
		m_OverlayAnim_StartTime = _pClientPAGI->m_OverlayAnim_StartTime;
		m_OverlayAnim.ClearCache();
	}
	if (_pClientPAGI->m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC)
	{
		m_OverlayAnimLipSync.m_iAnimContainerResource = _pClientPAGI->m_OverlayAnim_iAnimContainerResourceLipSync;
		m_OverlayAnimLipSync.m_iAnimSeq = _pClientPAGI->m_OverlayAnim_iAnimSeqLipSync;
		m_OverLayAnimLipSyncBaseJoint = _pClientPAGI->m_OverlayAnim_LipSyncBaseJoint;
	}

	if (!AcquireAllResources(_pContext))
	{
//		AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - Can't acquire all resources.");
		return;
	}

	if ((_pClientPAGI->m_Flags & AGI_PACKEDAGI_TOKENS) == 0)
	{
//		AG_DEBUG_LOG("WARNING: CWAGI::SyncWithPAGI() - ClientPAGI has no tokens.");
		return;
	}
	int iToken;
	for (iToken = 0; iToken < _pClientPAGI->m_lTokens.Len(); iToken++)
	{
		const CWAGI_Token_Packed* pPToken = &(_pClientPAGI->m_lTokens[iToken]);
		CWAGI_Token* pToken = GetTokenFromID(pPToken->m_ID, false);

		if (pToken == NULL)
		{
			PredictionMiss_AddToken(_pContext, pPToken->m_ID);
			pToken = GetTokenFromID(pPToken->m_ID, false);
#ifdef	AG_DEBUG
			if (bDebugSync)
				AG_DEBUG_LOG(CStrF("CWAGI::SyncWithPAGI() - PredictionMiss, Adding Token %d", 
							 pToken->GetID()));
#endif
		}

		if ((pPToken->m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) != 0)
		{
			pToken->SetRefreshGameTime(pPToken->m_RefreshGameTime);
		}

		if ((pPToken->m_Flags & AGI_PACKEDTOKEN_SIPS) == 0)
		{
#ifdef	AG_DEBUG
			AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() - ClientPAGI Token has no SIPs.");
#endif
			continue;
		}

		uint8 nSIPs = pPToken->m_lSIPs.Len();
		for (int iSIP = 0; iSIP < nSIPs; iSIP++)
		{
			const CWAGI_SIP* pSIP = &(pPToken->m_lSIPs[iSIP]);
			CWAGI_SIID SIID(pSIP->GetEnqueueTime(), pSIP->GetEnterActionIndex());
			CWAGI_StateInstance* pSI = pToken->GetMatchingStateInstance(&SIID, false);
			if (pSI == NULL)
			{
				pToken->PredictionMiss_AddStateInstance(_pContext, pSIP);
#ifdef	AG_DEBUG
				if (bDebugSync && DebugEnabled(_pContext))
					AG_DEBUG_LOG(CStrF("CWAGI::SyncWithPAGI() - PredictionMiss, Adding new SI <QT %3.3f, iEA %d>", 
								 SIID.GetEnqueueTime().GetTime(), SIID.GetEnterActionIndex()));
#endif
			}
			else
			{
				pSI->SyncWithSIP(_pContext, pSIP);
#ifdef	AG_DEBUG
				if (bDebugSync && DebugEnabled(_pContext))
					AG_DEBUG_LOG(CStrF("CWAGI::SyncWithPAGI() - Synching SI <ET %3.3f, iEA %d>", 
								 SIID.GetEnqueueTime().GetTime(), SIID.GetEnterActionIndex()));
#endif
			}
		}

		for (int iSI = 0; iSI < pToken->GetNumStateInstances(); iSI++)
		{
			const CWAGI_StateInstance* pSI = pToken->GetStateInstance(iSI);
			if (!pSI->IsPMRemoved())
			{
				CWAGI_SIID SIID(pSI->GetEnqueueTime(), pSI->GetEnterActionIndex());
				const CWAGI_SIP* pSIP = pPToken->GetMatchingSIP(&SIID);
				if (pSIP == NULL)
				{
					pToken->PredictionMiss_RemoveStateInstance(_pContext, &SIID);
#ifdef	AG_DEBUG
					if (bDebugSync && DebugEnabled(_pContext))
						AG_DEBUG_LOG(CStrF("CWAGI::SyncWithPAGI() - PredictionMiss, Removing SI <QT %3.3f, iEA %d>", 
									SIID.GetEnqueueTime().GetTime(), SIID.GetEnterActionIndex()));
#endif
				}
			}
		}

		if ((pPToken->m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) == 0)
		{
#ifdef	AG_DEBUG
			AG_DEBUG_LOG("ERROR: CWAGI::SyncWithPAGI() ClientPAGI Token has no playing state instance.");
#endif
			continue;
		}

		if (!pPToken->m_PlayingSIID.GetEnqueueTime().AlmostEqual(pToken->GetPlayingSIID().GetEnqueueTime()) ||
			(pPToken->m_PlayingSIID.GetEnterActionIndex() != pToken->GetPlayingSIID().GetEnterActionIndex()))
		{
			pToken->SetPlayingSIID(pPToken->m_PlayingSIID);
		}

		pToken->ResetPMFlags();
	}

	for (iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		CWAGI_Token* pToken = &(m_lTokens[iToken]);
		const CWAGI_Token_Packed* pPToken = _pClientPAGI->GetTokenFromID(pToken->GetID());
		if ((pPToken == NULL) && (!pToken->IsPMRemoved()))
		{
			// Token will not be removed from list before all stateinstances have PM faded.
			PredictionMiss_RemoveToken(_pContext, pToken->GetID());
#ifdef	AG_DEBUG
			if (bDebugSync && DebugEnabled(_pContext))
				AG_DEBUG_LOG(CStrF("CWAGI::SyncWithPAGI() - PredictionMiss, Removing Token %d", 
							 pToken->GetID()));
#endif
		}
	}

	// Check if we should be disabled
	if (_pClientPAGI->m_Flags & AGI_PACKEDAGI_ISDISABLED)
		m_bDisableAll = true;

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	int32 Len = _pClientPAGI->m_lEnterStateEntries.Len();
	m_lEnterStateEntries.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	{
		m_lEnterStateEntries[i].CopyFrom(_pClientPAGI->m_lEnterStateEntries[i]);
	}
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

// Run on server, using clientmirror.
void CWAGI_Packed::CreateDiff(const CWAGI_Packed* _pPAGIRef, CWAGI_Packed* _pPAGIDiff) const
{
	MSCOPESHORT(CWAGI_Packed::CreateDiff);

	// Assumes all flags on all levels are on in both 'this' and _pPAGIRef.

	_pPAGIDiff->m_Flags = 0;
	_pPAGIDiff->m_iRandseed = 0;
	_pPAGIDiff->m_iAnimGraphRes = 0;
	_pPAGIDiff->m_iAnimListRes = 0;

	//NOTE: This creates massive network traffic (randseed is changed at each refresh)
	//      It needs to be done differently
	if (m_iRandseed != _pPAGIRef->m_iRandseed)
	{
		_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_RANDSEED;
		_pPAGIDiff->m_iRandseed = m_iRandseed;
	}

	if ((m_iAnimGraphRes != _pPAGIRef->m_iAnimGraphRes) ||
		(m_iAnimListRes != _pPAGIRef->m_iAnimListRes))
	{
		_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_RESOURCEINDICES;
		_pPAGIDiff->m_iAnimGraphRes = m_iAnimGraphRes;
		_pPAGIDiff->m_iAnimListRes = m_iAnimListRes;
	}

	if ((_pPAGIRef->m_OverlayAnim_iAnimContainerResource != m_OverlayAnim_iAnimContainerResource) ||
		(_pPAGIRef->m_OverlayAnim_iAnimSeq != m_OverlayAnim_iAnimSeq) ||
		(_pPAGIRef->m_OverlayAnim_StartTime.AlmostEqual(m_OverlayAnim_StartTime) == false))
	{
		_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_OVERLAYANIM;
		_pPAGIDiff->m_OverlayAnim_iAnimContainerResource = m_OverlayAnim_iAnimContainerResource;
		_pPAGIDiff->m_OverlayAnim_iAnimSeq = m_OverlayAnim_iAnimSeq;
		_pPAGIDiff->m_OverlayAnim_StartTime = m_OverlayAnim_StartTime;
	}

	if ((_pPAGIRef->m_OverlayAnim_iAnimContainerResourceLipSync != m_OverlayAnim_iAnimContainerResourceLipSync) ||
		(_pPAGIRef->m_OverlayAnim_iAnimSeqLipSync != m_OverlayAnim_iAnimSeqLipSync) ||
		(_pPAGIRef->m_OverlayAnim_StartTime.AlmostEqual(m_OverlayAnim_StartTime) == false))
	{
		_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_OVERLAYANIMLIPSYNC;
		_pPAGIDiff->m_OverlayAnim_iAnimContainerResourceLipSync = m_OverlayAnim_iAnimContainerResourceLipSync;
		_pPAGIDiff->m_OverlayAnim_iAnimSeqLipSync = m_OverlayAnim_iAnimSeqLipSync;
		_pPAGIDiff->m_OverlayAnim_LipSyncBaseJoint = m_OverlayAnim_LipSyncBaseJoint;
	}

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAGI_Token_Packed* pPToken = &(m_lTokens[iToken]);
		const CWAGI_Token_Packed* pPTokenRef = _pPAGIRef->GetTokenFromID(pPToken->m_ID);
		CWAGI_Token_Packed* pPTokenDiff = NULL;

		if (pPTokenRef == NULL)
		{
			// Token does not exist in Ref. Add it to Diff.
			pPTokenDiff = _pPAGIDiff->GetTokenFromID(pPToken->m_ID, true);

			pPTokenDiff->m_Flags |= AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE;
			pPTokenDiff->m_PlayingSIID = pPToken->m_PlayingSIID;

			pPTokenDiff->m_Flags |= AGI_PACKEDTOKEN_REFRESHGAMETIME;
			pPTokenDiff->m_RefreshGameTime = pPToken->m_RefreshGameTime;

			for (int iSIP = 0; iSIP < pPToken->m_lSIPs.Len(); iSIP++)
			{
				const CWAGI_SIP* pSIP = &(pPToken->m_lSIPs[iSIP]);
				CWAGI_SIID SIID(pSIP->GetEnqueueTime(), pSIP->GetEnterActionIndex());
				CWAGI_SIP* pSIPDiff = pPTokenDiff->GetMatchingSIP(&SIID, true);
				*pSIPDiff = *pSIP;
			}
		}
		else
		{ // Token exist in Ref. Search if for differences.

			// Verify PlayingStateInstanceID.
			if ((pPToken->m_PlayingSIID.GetEnqueueTime().AlmostEqual(pPTokenRef->m_PlayingSIID.GetEnqueueTime()) == false) ||
				(pPToken->m_PlayingSIID.GetEnterActionIndex() != pPTokenRef->m_PlayingSIID.GetEnterActionIndex()))
			{
				if (pPTokenDiff == NULL)
					pPTokenDiff = _pPAGIDiff->GetTokenFromID(pPToken->m_ID, true);

				pPTokenDiff->m_Flags |= AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE;
				pPTokenDiff->m_PlayingSIID = pPToken->m_PlayingSIID;
			}

			// Verify RefreshGameTime
			// NOTE: This creates massive network traffic. It needs to be reworked!!
			if (pPToken->m_RefreshGameTime.AlmostEqual(pPTokenRef->m_RefreshGameTime) == false)
			{
				if (pPTokenDiff == NULL)
					pPTokenDiff = _pPAGIDiff->GetTokenFromID(pPToken->m_ID, true);

				pPTokenDiff->m_Flags |= AGI_PACKEDTOKEN_REFRESHGAMETIME;
				pPTokenDiff->m_RefreshGameTime = pPToken->m_RefreshGameTime;
			}

			// Check for stuff to add.
			int iSIP;
			for (iSIP = 0; iSIP < pPToken->m_lSIPs.Len(); iSIP++)
			{
				const CWAGI_SIP* pSIP = &(pPToken->m_lSIPs[iSIP]);
				CWAGI_SIID SIID(pSIP->GetEnqueueTime(), pSIP->GetEnterActionIndex());
				const CWAGI_SIP* pSIPRef = pPTokenRef->GetMatchingSIP(&SIID);

				// SIP does not exist or is different in Ref. Add it to Diff.
				if ((pSIPRef == NULL) || !CWAGI_SIP::Equal(pSIP, pSIPRef))
				{
					if (pPTokenDiff == NULL)
						pPTokenDiff = _pPAGIDiff->GetTokenFromID(pPToken->m_ID, true);

					CWAGI_SIID SIID(pSIP->GetEnqueueTime(), pSIP->GetEnterActionIndex());
					CWAGI_SIP* pSIPDiff = pPTokenDiff->GetMatchingSIP(&SIID, true);
					*pSIPDiff = *pSIP;
				}
			}

			// Check for stuff to remove.
			for (iSIP = 0; iSIP < pPTokenRef->m_lSIPs.Len(); iSIP++)
			{
				const CWAGI_SIP* pSIPRef = &(pPTokenRef->m_lSIPs[iSIP]);
				CWAGI_SIID SIIDRef(pSIPRef->GetEnqueueTime(), pSIPRef->GetEnterActionIndex());
				const CWAGI_SIP* pSIP = pPToken->GetMatchingSIP(&SIIDRef);

				// SIP does exist only in Ref. Add RemSIID to Diff.
				if (pSIP == NULL)
				{
					if (pPTokenDiff == NULL)
						pPTokenDiff = _pPAGIDiff->GetTokenFromID(pPToken->m_ID, true);

					pPTokenDiff->AddRemSIID(CWAGI_SIID(pSIPRef->GetEnqueueTime(), pSIPRef->GetEnterActionIndex()));
				}
			}
		}
	}

	for (int iTokenRef = 0; iTokenRef < _pPAGIRef->m_lTokens.Len(); iTokenRef++)
	{
		const CWAGI_Token_Packed* pPTokenRef = &(_pPAGIRef->m_lTokens[iTokenRef]);
		const CWAGI_Token_Packed* pPToken = GetTokenFromID(pPTokenRef->m_ID);
		CWAGI_Token_Packed* pPTokenDiff = NULL;

		if (pPToken == NULL)
		{
			// Token exists only in Ref. Add RemoveTokenID to Diff.
			_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_REMTOKENIDS;
			_pPAGIDiff->m_lRemTokenIDs.Add(pPTokenRef->m_ID);
		}
	}

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	int8 Len = (int8)m_lEnterStateEntries.Len();
	if (Len > 0)
	{
		_pPAGIDiff->m_Flags |= AGI_PACKEDAGI_HASENTERSTATEENTRIES;
		_pPAGIDiff->m_lEnterStateEntries.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			_pPAGIDiff->m_lEnterStateEntries[i].CopyFrom(m_lEnterStateEntries[i]);
	}
#endif
}

//--------------------------------------------------------------------------------

void CWAGI_Packed::UpdateWithDiff(const CWAGI_Packed* _pPAGIDiff)
{
	MSCOPESHORT(CWAGI_Packed::UpdateWithDiff);

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_RANDSEED)
	{
		m_Flags |= AGI_PACKEDAGI_RANDSEED;
		m_iRandseed = _pPAGIDiff->m_iRandseed;
	}

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_RESOURCEINDICES)
	{
		m_Flags |= AGI_PACKEDAGI_RESOURCEINDICES;
		m_iAnimGraphRes = _pPAGIDiff->m_iAnimGraphRes;
		m_iAnimListRes = _pPAGIDiff->m_iAnimListRes;
	}

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_TOKENS)
	{
		for (int iToken = 0; iToken < _pPAGIDiff->m_lTokens.Len(); iToken++)
		{
			const CWAGI_Token_Packed* pPTokenDiff = &(_pPAGIDiff->m_lTokens[iToken]);
			CWAGI_Token_Packed* pPToken = GetTokenFromID(pPTokenDiff->m_ID, true);

			if ((pPTokenDiff->m_Flags & AGI_PACKEDTOKEN_SIPS) != 0)
			{
				uint8 nSIPs = pPTokenDiff->m_lSIPs.Len();
				for (int iSIP = 0; iSIP < nSIPs; iSIP++)
				{
					const CWAGI_SIP* pSIPDiff = &(pPTokenDiff->m_lSIPs[iSIP]);
					CWAGI_SIID SIIDDiff(pSIPDiff->GetEnqueueTime(), pSIPDiff->GetEnterActionIndex());
					CWAGI_SIP* pSIP = pPToken->GetMatchingSIP(&SIIDDiff, true);
					*pSIP = *pSIPDiff;
				}
			}

			if ((pPTokenDiff->m_Flags & AGI_PACKEDTOKEN_REMSIIDS) != 0)
			{
				uint8 nRemSIIDs = pPTokenDiff->m_lRemSIIDs.Len();
				for (int iRemSIID = 0; iRemSIID < nRemSIIDs; iRemSIID++)
				{
					const CWAGI_SIID* pRemSIID = &(pPTokenDiff->m_lRemSIIDs[iRemSIID]);
					if (!pPToken->RemoveMatchingSIP(pRemSIID))
					{
#ifdef	AG_DEBUG
						AG_DEBUG_LOG(CStrF("WARNING: CWAGI_Packed::UpdateWithDiff() - RemSIID <%3.3f, %d> not found!!!", pRemSIID->GetEnqueueTime().GetTime(), pRemSIID->GetEnterActionIndex()));
#endif
					}
				}
			}

			if ((pPTokenDiff->m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) != 0)
			{
				// Verify already here that the new Playing StateInstance exists in the PAGI.
				pPToken->m_Flags |= AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE;
				pPToken->m_PlayingSIID = pPTokenDiff->m_PlayingSIID;
			}

			if ((pPTokenDiff->m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) != 0)
			{
				pPToken->m_Flags |= AGI_PACKEDTOKEN_REFRESHGAMETIME;
				pPToken->m_RefreshGameTime = pPTokenDiff->m_RefreshGameTime;
			}


		}
	}

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_REMTOKENIDS)
	{
		for (int iRemTokenDiff = 0; iRemTokenDiff < _pPAGIDiff->m_lRemTokenIDs.Len(); iRemTokenDiff++)
		{
			int8 RemTokenIDDiff = _pPAGIDiff->m_lRemTokenIDs[iRemTokenDiff];
			for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
			{
				CWAGI_Token_Packed* pPToken = &(m_lTokens[iToken]);
				if (pPToken->m_ID == RemTokenIDDiff)
				{
					m_lTokens.Del(iToken);
					break;
				}
			}
		}
	}

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_OVERLAYANIM)
	{
		m_Flags |= AGI_PACKEDAGI_OVERLAYANIM;
		m_OverlayAnim_iAnimContainerResource = _pPAGIDiff->m_OverlayAnim_iAnimContainerResource;
		m_OverlayAnim_iAnimSeq = _pPAGIDiff->m_OverlayAnim_iAnimSeq;
		m_OverlayAnim_StartTime = _pPAGIDiff->m_OverlayAnim_StartTime;
	}

	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC)
	{
		m_Flags |= AGI_PACKEDAGI_OVERLAYANIMLIPSYNC;
		m_OverlayAnim_iAnimContainerResourceLipSync = _pPAGIDiff->m_OverlayAnim_iAnimContainerResourceLipSync;
		m_OverlayAnim_iAnimSeqLipSync = _pPAGIDiff->m_OverlayAnim_iAnimSeqLipSync;
		m_OverlayAnim_LipSyncBaseJoint = _pPAGIDiff->m_OverlayAnim_LipSyncBaseJoint;
	}

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	if (_pPAGIDiff->m_Flags & AGI_PACKEDAGI_HASENTERSTATEENTRIES)
	{
		int8 Len = (int8)_pPAGIDiff->m_lEnterStateEntries.Len();
		if (Len > 0)
		{
			m_lEnterStateEntries.SetLen(Len);
			for (int32 i = 0; i < Len; i++)
				m_lEnterStateEntries[i].CopyFrom(_pPAGIDiff->m_lEnterStateEntries[i]);
		}
	}
#endif
}

const CWAGI_Token_Packed* CWAGI_Packed::GetTokenFromID(int8 _TokenID) const
{
	MSCOPESHORT(CWAGI_Packed::GetTokenFromID);

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		if (m_lTokens[iToken].m_ID == _TokenID)
			return &(m_lTokens[iToken]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------

CWAGI_Token_Packed* CWAGI_Packed::GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent)
{
	MSCOPESHORT(CWAGI_Packed::GetTokenFromID);

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		if (m_lTokens[iToken].m_ID == _TokenID)
			return &(m_lTokens[iToken]);
	}

	if (_bCreateNonExistent)
	{
		m_Flags |= AGI_PACKEDAGI_TOKENS;
		int8 iToken = m_lTokens.Add(CWAGI_Token_Packed(_TokenID));
		return &(m_lTokens[iToken]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------

const CWAGI_SIP* CWAGI_Token_Packed::GetMatchingSIP(const CWAGI_SIID* _pSIID) const
{
	MSCOPESHORT(CWAGI_Token_Packed::GetMatchingSIP);

	for (int iSIP = 0; iSIP < m_lSIPs.Len(); iSIP++)
	{
		if (m_lSIPs[iSIP].GetEnqueueTime().AlmostEqual(_pSIID->GetEnqueueTime()) &&
			(m_lSIPs[iSIP].GetEnterActionIndex() == _pSIID->GetEnterActionIndex()))
			return &(m_lSIPs[iSIP]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------

CWAGI_SIP* CWAGI_Token_Packed::GetMatchingSIP(const CWAGI_SIID* _pSIID, bool _bCreateNonExistent)
{
	MSCOPESHORT(CWAGI_Token_Packed::GetMatchingSIP);

	for (int iSIP = 0; iSIP < m_lSIPs.Len(); iSIP++)
	{
		if (m_lSIPs[iSIP].GetEnqueueTime().AlmostEqual(_pSIID->GetEnqueueTime()) &&
			(m_lSIPs[iSIP].GetEnterActionIndex() == _pSIID->GetEnterActionIndex()))
			return &(m_lSIPs[iSIP]);
	}

	if (_bCreateNonExistent)
	{
		m_Flags |= AGI_PACKEDTOKEN_SIPS;
		CWAGI_SIP SIP;
		SIP.SetEnqueueTime(_pSIID->GetEnqueueTime());
		SIP.SetEnterActionIndex(_pSIID->GetEnterActionIndex());
		int iSIP = m_lSIPs.Add(SIP);
		return &(m_lSIPs[iSIP]);
	}

	return NULL;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

int CWAGI_Packed::Write(uint8* _pData) const
{
	MSCOPESHORT(CWAGI_Packed::Write);

	uint8* pBase = _pData;

	PTR_PUTUINT8(_pData, m_Flags);

	if ((m_Flags & AGI_PACKEDAGI_RANDSEED) != 0)
	{
		PTR_PUTUINT32(_pData, m_iRandseed);
	}

	if ((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0)
	{
		PTR_PUTINT32(_pData, m_iAnimGraphRes);
		PTR_PUTINT32(_pData, m_iAnimListRes);
	}

	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
	{
		PTR_PUTUINT8(_pData, m_lTokens.Len());
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
			_pData += m_lTokens[iToken].Write(_pData);
	}

	if ((m_Flags & AGI_PACKEDAGI_REMTOKENIDS) != 0)
	{
		PTR_PUTUINT8(_pData, m_lRemTokenIDs.Len());
		for (int iRemTokenID = 0; iRemTokenID < m_lRemTokenIDs.Len(); iRemTokenID++)
			PTR_PUTINT8(_pData, m_lRemTokenIDs[iRemTokenID]);
	}

	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIM) != 0)
	{
		PTR_PUTINT16(_pData, m_OverlayAnim_iAnimContainerResource);
		PTR_PUTINT16(_pData, m_OverlayAnim_iAnimSeq);
		PTR_PUTCMTIME(_pData, m_OverlayAnim_StartTime);
	}

	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC) != 0)
	{
		PTR_PUTINT16(_pData, m_OverlayAnim_iAnimContainerResourceLipSync);
		PTR_PUTINT16(_pData, m_OverlayAnim_iAnimSeqLipSync);
		PTR_PUTINT8(_pData, m_OverlayAnim_LipSyncBaseJoint);
	}

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	if ((m_Flags & AGI_PACKEDAGI_HASENTERSTATEENTRIES) != 0)
	{
		PTR_PUTUINT8(_pData, m_lEnterStateEntries.Len());
		for (int iEntry = 0; iEntry< m_lEnterStateEntries.Len(); iEntry++)
			_pData += m_lEnterStateEntries[iEntry].Write(_pData);
	}
#endif

	return (_pData - pBase);
}

//--------------------------------------------------------------------------------

void CWAGI_Packed::Write(CCFile* _pFile) const
{
	_pFile->WriteLE(m_Flags);

//	if ((m_Flags & AGI_PACKEDAGI_RANDSEED) != 0)
	{
		_pFile->WriteLE(m_iRandseed);
	}

//	if ((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0)
	{
		_pFile->WriteLE(m_iAnimGraphRes);
		_pFile->WriteLE(m_iAnimListRes);
	}

//	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
	{
		uint8 Len = m_lTokens.Len();
		_pFile->WriteLE(Len);
		for (int iToken = 0; iToken < Len; iToken++)
			m_lTokens[iToken].Write(_pFile);
	}

//	if ((m_Flags & AGI_PACKEDAGI_REMTOKENIDS) != 0)
	{
		uint8 Len = m_lRemTokenIDs.Len();
		_pFile->WriteLE(Len);
		for (int iRemTokenID = 0; iRemTokenID < Len; iRemTokenID++)
			_pFile->WriteLE(m_lRemTokenIDs[iRemTokenID]);
	}
//	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIM) != 0)
	{
		_pFile->WriteLE(m_OverlayAnim_iAnimContainerResource);
		_pFile->WriteLE(m_OverlayAnim_iAnimSeq);
		m_OverlayAnim_StartTime.Write(_pFile);
	}
//	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC) != 0)
	{
		_pFile->WriteLE(m_OverlayAnim_iAnimContainerResourceLipSync);
		_pFile->WriteLE(m_OverlayAnim_iAnimSeqLipSync);
		_pFile->WriteLE(m_OverlayAnim_LipSyncBaseJoint);
	}


#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
//	if ((m_Flags & AGI_PACKEDAGI_HASENTERSTATEENTRIES) != 0)
	{
		uint8 Len = m_lEnterStateEntries.Len();
		_pFile->WriteLE(Len);
		for (int iEntry = 0; iEntry< m_lEnterStateEntries.Len(); iEntry++)
			m_lEnterStateEntries[iEntry].Write(_pFile);
	}
#endif 
}

//--------------------------------------------------------------------------------

int CWAGI_Token_Packed::Write(uint8* _pData) const
{
	MSCOPESHORT(CWAGI_Token_Packed::Write);

	uint8* pBase = _pData;
    
	PTR_PUTINT8(_pData, m_ID);
	PTR_PUTUINT8(_pData, m_Flags);

	if ((m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) != 0)
	{
		PTR_PUTCMTIME(_pData, m_PlayingSIID.m_EnqueueTime);
		PTR_PUTINT16(_pData, m_PlayingSIID.m_iEnterAction);
	}

	if ((m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) != 0)
	{
		PTR_PUTCMTIME(_pData, m_RefreshGameTime);
	}

	if ((m_Flags & AGI_PACKEDTOKEN_SIPS) != 0)
	{
		PTR_PUTUINT8(_pData, m_lSIPs.Len());
		PTR_PUTDATA(_pData, m_lSIPs.GetBasePtr(), m_lSIPs.Len() * sizeof(CWAGI_SIP));
	}

	if ((m_Flags & AGI_PACKEDTOKEN_REMSIIDS) != 0)
	{
		PTR_PUTUINT8(_pData, m_lRemSIIDs.Len());
		PTR_PUTDATA(_pData, m_lRemSIIDs.GetBasePtr(), m_lRemSIIDs.Len() * sizeof(CWAGI_SIID));
	}

    return (_pData - pBase);
}

//--------------------------------------------------------------------------------

void CWAGI_Token_Packed::Write(CCFile* _pFile) const
{
	_pFile->WriteLE(m_ID);
	_pFile->WriteLE(m_Flags);

//	if ((m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) != 0)
	{
		m_PlayingSIID.m_EnqueueTime.Write(_pFile);
		_pFile->WriteLE(m_PlayingSIID.m_iEnterAction);
	}

//  if ((m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) != 0)
	{
		m_RefreshGameTime.Write(_pFile);
	}

//	if ((m_Flags & AGI_PACKEDTOKEN_SIPS) != 0)
	{
		uint8 nSIPs = m_lSIPs.Len();
		_pFile->WriteLE(nSIPs);
		_pFile->Write(m_lSIPs.GetBasePtr(), m_lSIPs.Len() * sizeof(CWAGI_SIP));
	}

//	if ((m_Flags & AGI_PACKEDTOKEN_REMSIIDS) != 0)
	{
		uint8 nRemSIIDs = m_lRemSIIDs.Len();
		_pFile->WriteLE(nRemSIIDs);
		_pFile->Write(m_lRemSIIDs.GetBasePtr(), m_lRemSIIDs.Len() * sizeof(CWAGI_SIID));
	}
}

//--------------------------------------------------------------------------------

int CWAGI_Packed::Read(const uint8* _pData)
{
	MSCOPESHORT(CWAGI_Packed::Read);

	const uint8* pBase = _pData;

	PTR_GETUINT8(_pData, m_Flags);

	if ((m_Flags & AGI_PACKEDAGI_RANDSEED) != 0)
	{
		PTR_GETUINT32(_pData, m_iRandseed);
	}

	if ((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0)
	{
		PTR_GETINT32(_pData, m_iAnimGraphRes);
		PTR_GETINT32(_pData, m_iAnimListRes);
	}

	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
	{
		uint8 nTokens;
		PTR_GETUINT8(_pData, nTokens);
		m_lTokens.SetLen(nTokens);
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
			_pData += m_lTokens[iToken].Read(_pData);
	}

	if ((m_Flags & AGI_PACKEDAGI_REMTOKENIDS) != 0)
	{
		uint8 nRemTokenIDs;
		PTR_GETUINT8(_pData, nRemTokenIDs);
		m_lRemTokenIDs.SetLen(nRemTokenIDs);
		for (int iRemTokenID = 0; iRemTokenID < m_lRemTokenIDs.Len(); iRemTokenID++)
			PTR_GETINT8(_pData, m_lRemTokenIDs[iRemTokenID]);
	}

	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIM) != 0)
	{
		PTR_GETINT16(_pData, m_OverlayAnim_iAnimContainerResource);
		PTR_GETINT16(_pData, m_OverlayAnim_iAnimSeq);
		PTR_GETCMTIME(_pData, m_OverlayAnim_StartTime);
	}

	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC) != 0)
	{
		PTR_GETINT16(_pData, m_OverlayAnim_iAnimContainerResourceLipSync);
		PTR_GETINT16(_pData, m_OverlayAnim_iAnimSeqLipSync);
		PTR_GETINT8(_pData, m_OverlayAnim_LipSyncBaseJoint);
	}

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
	if ((m_Flags & AGI_PACKEDAGI_HASENTERSTATEENTRIES) != 0)
	{
		uint8 nEntries;
		PTR_GETUINT8(_pData, nEntries);
		m_lEnterStateEntries.SetLen(nEntries);
		for (int iEntry = 0; iEntry < nEntries; iEntry++)
			_pData += m_lEnterStateEntries[iEntry].Read(_pData);
	}
#endif

	return (_pData - pBase);
}

//--------------------------------------------------------------------------------

void CWAGI_Packed::Read(CCFile* _pFile)
{
	_pFile->ReadLE(m_Flags);

//	if ((m_Flags & AGI_PACKEDAGI_RANDSEED) != 0)
	{
		_pFile->ReadLE(m_iRandseed);
	}

//	if ((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0)
	{
		_pFile->ReadLE(m_iAnimGraphRes);
		_pFile->ReadLE(m_iAnimListRes);
	}

//	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
	{
		uint8 nTokens;
		_pFile->ReadLE(nTokens);
		m_lTokens.SetLen(nTokens);
		for (int iToken = 0; iToken < nTokens; iToken++)
			m_lTokens[iToken].Read(_pFile);
	}

//	if ((m_Flags & AGI_PACKEDAGI_REMTOKENIDS) != 0)
	{
		uint8 nRemTokenIDs;
		_pFile->ReadLE(nRemTokenIDs);
		m_lRemTokenIDs.SetLen(nRemTokenIDs);
		for (int iRemTokenID = 0; iRemTokenID < nRemTokenIDs; iRemTokenID++)
			_pFile->ReadLE(m_lRemTokenIDs[iRemTokenID]);
	}
	//	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIM) != 0)
	{
		_pFile->ReadLE(m_OverlayAnim_iAnimContainerResource);
		_pFile->ReadLE(m_OverlayAnim_iAnimSeq);
		m_OverlayAnim_StartTime.Read(_pFile);
	}

	//	if ((m_Flags & AGI_PACKEDAGI_OVERLAYANIMLIPSYNC) != 0)
	{
		_pFile->ReadLE(m_OverlayAnim_iAnimContainerResourceLipSync);
		_pFile->ReadLE(m_OverlayAnim_iAnimSeqLipSync);
		_pFile->ReadLE(m_OverlayAnim_LipSyncBaseJoint);
	}

#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
//	if ((m_Flags & AGI_PACKEDAGI_HASENTERSTATEENTRIES) != 0)
	{
		uint8 nEntries;
		_pFile->ReadLE(nEntries);
		m_lEnterStateEntries.SetLen(nEntries);
		for (int iEntry = 0; iEntry < nEntries; iEntry++)
			m_lEnterStateEntries[iEntry].Read(_pFile);
	}
#endif 
}

//--------------------------------------------------------------------------------

int CWAGI_Token_Packed::Read(const uint8* _pData)
{
	MSCOPESHORT(CWAGI_Token_Packed::Read);

	const uint8* pBase = _pData;
    
	PTR_GETINT8(_pData, m_ID);
	PTR_GETUINT8(_pData, m_Flags);

	if ((m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) != 0)
	{
		PTR_GETCMTIME(_pData, m_PlayingSIID.m_EnqueueTime);
		PTR_GETINT16(_pData, m_PlayingSIID.m_iEnterAction);
	}

	if ((m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) != 0)
	{
		PTR_GETCMTIME(_pData, m_RefreshGameTime);
	}

	if ((m_Flags & AGI_PACKEDTOKEN_SIPS) != 0)
	{
		uint8 nSIPs;
		PTR_GETUINT8(_pData, nSIPs);
		m_lSIPs.SetLen(nSIPs);
		PTR_GETDATA(_pData, m_lSIPs.GetBasePtr(), m_lSIPs.Len() * sizeof(CWAGI_SIP));
	}

	if ((m_Flags & AGI_PACKEDTOKEN_REMSIIDS) != 0)
	{
		uint8 nRemSIIDs;
		PTR_GETUINT8(_pData, nRemSIIDs);
		m_lRemSIIDs.SetLen(nRemSIIDs);
		PTR_GETDATA(_pData, m_lRemSIIDs.GetBasePtr(), m_lRemSIIDs.Len() * sizeof(CWAGI_SIID));
	}

    return (_pData - pBase);
}

//--------------------------------------------------------------------------------

void CWAGI_Token_Packed::Read(CCFile* _pFile)
{
	_pFile->ReadLE(m_ID);
	_pFile->ReadLE(m_Flags);

//	if ((m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) != 0)
	{
		m_PlayingSIID.m_EnqueueTime.Read(_pFile);
		_pFile->ReadLE(m_PlayingSIID.m_iEnterAction);
	}

	{
		m_RefreshGameTime.Read(_pFile);
	}

//	if ((m_Flags & AGI_PACKEDTOKEN_SIPS) != 0)
	{
		uint8 nSIPs;
		_pFile->ReadLE(nSIPs);
		m_lSIPs.SetLen(nSIPs);
		_pFile->Read(m_lSIPs.GetBasePtr(), m_lSIPs.Len() * sizeof(CWAGI_SIP));
	}

//	if ((m_Flags & AGI_PACKEDTOKEN_REMSIIDS) != 0)
	{
		uint8 nRemSIIDs;
		_pFile->ReadLE(nRemSIIDs);
		m_lRemSIIDs.SetLen(nRemSIIDs);
		_pFile->Read(m_lRemSIIDs.GetBasePtr(), m_lRemSIIDs.Len() * sizeof(CWAGI_SIID));
	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CWAGI_Packed::Log(CStr _Prefix, CStr _Name) const
{
	MSCOPESHORT(CWAGI_Packed::Log);

#ifdef	AG_DEBUG
	CStr Msg = _Prefix + _Name;

	Msg += CStrF(", Flags [%s%s]", 
				(((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0) ? "R" : "r"),
				(((m_Flags & AGI_PACKEDAGI_TOKENS) != 0) ? "T" : "t"));

	if ((m_Flags & AGI_PACKEDAGI_RESOURCEINDICES) != 0)
		Msg += CStrF(", iAGRes %d, iALRes %d", m_iAnimGraphRes, m_iAnimListRes);

	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
		Msg += CStrF(", nTokens %d", m_lTokens.Len());

	AG_DEBUG_LOG(Msg);

	if ((m_OverlayAnim_iAnimContainerResource != 0) ||
		(m_OverlayAnim_iAnimSeq != -1) ||
		(!m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME)))
		AG_DEBUG_LOG(_Prefix + CStrF("  OverlayAnim: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f", m_OverlayAnim_iAnimContainerResource, m_OverlayAnim_iAnimSeq, m_OverlayAnim_StartTime.GetTime()));
	if ((m_OverlayAnim_iAnimContainerResourceLipSync != 0) ||
		(m_OverlayAnim_iAnimSeqLipSync != -1) ||
		(!m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME)))
		AG_DEBUG_LOG(_Prefix + CStrF("  OverlayAnimLipSync: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f BaseJoint: %d", m_OverlayAnim_iAnimContainerResourceLipSync, m_OverlayAnim_iAnimSeqLipSync, m_OverlayAnim_StartTime.GetTime(), m_OverlayAnim_LipSyncBaseJoint));

	if ((m_Flags & AGI_PACKEDAGI_TOKENS) != 0)
	{
		for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
			m_lTokens[iToken].Log(_Prefix + "  ");
	}

	AG_DEBUG_LOG("");
#endif
}

//--------------------------------------------------------------------------------
	
void CWAGI_Token_Packed::Log(CStr _Prefix) const
{
	MSCOPESHORT(CWAGI_Token_Packed::Log);

#ifdef	AG_DEBUG
	CStr Msg;
	Msg += CStrF("%sToken: ID %d", (char*)_Prefix, m_ID);
	
	Msg += CStrF(", Flags [%s%s%s%s]", 
		((m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE) ? "P" : "p"),
		((m_Flags & AGI_PACKEDTOKEN_SIPS) ? "S" : "s"),
		((m_Flags & AGI_PACKEDTOKEN_REMSIIDS) ? "R" : "r"),
		((m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME) ? "G" : "g"));

	if (m_Flags & AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE)
		Msg += CStrF(", PSI <%3.2f, %d>", 
					m_PlayingSIID.m_EnqueueTime.GetTime(),
					m_PlayingSIID.m_iEnterAction);

	if (m_Flags & AGI_PACKEDTOKEN_SIPS)
		Msg += CStrF(", nSIPs %d", m_lSIPs.Len());

	if (m_Flags & AGI_PACKEDTOKEN_REMSIIDS)
		Msg += CStrF(", nRemSIIDs %d", m_lRemSIIDs.Len());

	if (m_Flags & AGI_PACKEDTOKEN_REFRESHGAMETIME)
		Msg += CStrF(", RGT %3.2f", m_RefreshGameTime.GetTime());

	AG_DEBUG_LOG(Msg);

	if (m_lSIPs.Len() > 0)
	{
		for (int iSIP = 0; iSIP < m_lSIPs.Len(); iSIP++)
			AG_DEBUG_LOG(_Prefix + CStrF("  SIP %d: <QT %3.3f, iEA %d, ET %3.3f, iLA %d, LT %3.3f>", 
						iSIP, m_lSIPs[iSIP].GetEnqueueTime().GetTime(),
						m_lSIPs[iSIP].GetEnterActionIndex(), m_lSIPs[iSIP].GetEnterTime().GetTime(),
						m_lSIPs[iSIP].GetLeaveActionIndex(), m_lSIPs[iSIP].GetLeaveTime().GetTime()));
	}

	if (m_lRemSIIDs.Len() > 0)
	{
		Msg = _Prefix + "  RemSIIDs: ";
		for (int iRemSIID = 0; iRemSIID < m_lRemSIIDs.Len(); iRemSIID++)
			Msg += CStrF("  <QT %3.3f, iEA %d>", m_lRemSIIDs[iRemSIID].GetEnqueueTime().GetTime(), m_lRemSIIDs[iRemSIID].GetEnterActionIndex());

		AG_DEBUG_LOG(Msg);
	}
#endif
}

//--------------------------------------------------------------------------------

void CWAGI::Log(CStr _Prefix, CStr _Name) const
{
	MSCOPESHORT(CWAGI::Log);

#ifdef	AG_DEBUG
	AG_DEBUG_LOG(_Prefix + CStrF("%s (%X): iAGRes %d, iALRes %d, nTokens %d", (char*)_Name, this, m_iAnimGraphRes, m_iAnimListRes, m_lTokens.Len()));

	if (m_OverlayAnim.IsValid() || (!m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME)))
		AG_DEBUG_LOG(_Prefix + CStrF("  OverlayAnim: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f", m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq, m_OverlayAnim_StartTime.GetTime()));
	if (m_OverlayAnimLipSync.IsValid() || (!m_OverlayAnim_StartTime.AlmostEqual(AGI_UNDEFINEDTIME)))
		AG_DEBUG_LOG(_Prefix + CStrF("  OverlayAnimLipSync: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f BaseJoint: %d", m_OverlayAnimLipSync.m_iAnimContainerResource, m_OverlayAnimLipSync.m_iAnimSeq, m_OverlayAnim_StartTime.GetTime(), m_OverLayAnimLipSyncBaseJoint));

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAGI_Token* pToken = &(m_lTokens[iToken]);
		AG_DEBUG_LOG(_Prefix + CStrF("  Token %d: nSIs %d, PSI <QT %3.3f, iEA %d>", 
							   pToken->GetID(), pToken->GetNumStateInstances(),
							   pToken->GetPlayingSIID().GetEnqueueTime().GetTime(),
							   pToken->GetPlayingSIID().GetEnterActionIndex()));

		for (int iSI = 0; iSI < pToken->GetNumStateInstances(); iSI++)
		{
			const CWAGI_StateInstance* pSI = pToken->GetStateInstance(iSI);
			AG_DEBUG_LOG(_Prefix + CStrF("    SI %d: Flags [%s%s%s] <QT %3.3f, iEA %d, ET %3.3f, iLA %d, LT %3.3f>",
									iSI,
									(pSI->IsPMAdded() ? "A" : "a"),
									(pSI->IsPMRemoved() ? "R" : "r"),
									(pSI->IsPMUpdated() ? "U" : "u"),
									pSI->GetEnqueueTime().GetTime(),
									pSI->GetEnterActionIndex(), pSI->GetEnterTime().GetTime(),
									pSI->GetLeaveActionIndex(), pSI->GetLeaveTime().GetTime()));
		}
	}
#endif
}
