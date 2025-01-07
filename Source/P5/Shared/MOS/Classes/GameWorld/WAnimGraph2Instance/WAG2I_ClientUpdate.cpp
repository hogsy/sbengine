#include "PCH.h"

//--------------------------------------------------------------------------------
#ifndef M_RTM
extern bool bDebug;// = true;
extern bool bDebugSync;// = false;
#endif
//--------------------------------------------------------------------------------

#include "WAG2I.h"
#include "WAG2I_StateInstPacked.h"
#include "WAG2_ClientData.h"
//#include "../../../XR/XRAnimGraph2/AnimGraph2.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"
//#include "../WDataRes_AnimGraph2.h"
//#include "../WPhysState.h"

//--------------------------------------------------------------------------------

#ifdef	AG2_DEBUG
bool bDebug = true;
bool bDebugSync = false;
#endif

//#define AG2_NODIFF

//--------------------------------------------------------------------------------

int CWAG2I::OnCreateClientUpdate2(uint8* _pData, const CWAG2I* _pMirror, int8 _AGPackType) const
{
	MSCOPESHORT(CWAG2I::OnCreateClientUpdate2);

	static const CWAG2I Dummy;

	if (!_pData)
		return 0;

	uint8* pBase = _pData;

	// Okidoki then, this function will put anything
	
	// Update Flag (8 bit), (agres, Tokens, overlay)
	uint8& CopyFlag = _pData[0];
	_pData++;
	CopyFlag = 0;//m_DirtyFlag;
	if (!_pMirror)
		return (_pData - pBase);

	if (_AGPackType != _pMirror->m_LastPackMode)
	{
		// Copy over new packmode (sends a complete ag state over)
		PTR_PUTINT8(_pData,_AGPackType);
		CopyFlag |= AG2I_DIRTYFLAG_PACKTYPE;
		_pMirror = &Dummy;
	}

	// AGRes, just put it all, should only be copied once anyway, num entries (8bit), entry (32 + 32 bit)

	{
		uint8* pDataBefore = _pData;
		int8 ResLen = m_lAnimGraph2Res.Len();
		bool bOk = m_lAnimGraph2Res.Len() == _pMirror->m_lAnimGraph2Res.Len();
		PTR_PUTINT8(_pData,ResLen);
		for (int32 i = 0; i < ResLen; i++)//if (_pMirror->m_lAnimGraph2Res.Len()!= m_lAnimGraph2Res.Len())
		{
			if (bOk && _pMirror->m_lAnimGraph2Res[i] != m_lAnimGraph2Res[i])
				bOk = false;
			m_lAnimGraph2Res[i].OnCreateClientUpdate(_pData);
		}
		if (bOk)
			_pData = pDataBefore;
		else
			CopyFlag |= AG2I_DIRTYFLAG_RESOURCE;
	}
	CMTime Temp = _pMirror->m_OverlayAnim_StartTime;
	if ((_pMirror->m_OverlayAnim.m_iAnimContainerResource != m_OverlayAnim.m_iAnimContainerResource) ||
		(_pMirror->m_OverlayAnim.m_iAnimSeq != m_OverlayAnim.m_iAnimSeq) ||
		(Temp != m_OverlayAnim_StartTime))
	{
		CopyFlag |= AG2I_DIRTYFLAG_OVERLAY;
		PTR_PUTINT16(_pData, m_OverlayAnim.m_iAnimContainerResource);
		PTR_PUTINT16(_pData, m_OverlayAnim.m_iAnimSeq);
		PTR_PUTCMTIME(_pData, m_OverlayAnim_StartTime);
	}
	if ((_pMirror->m_OverlayAnimLipSync.m_iAnimContainerResource != m_OverlayAnimLipSync.m_iAnimContainerResource) ||
		(_pMirror->m_OverlayAnimLipSync.m_iAnimSeq != m_OverlayAnimLipSync.m_iAnimSeq))
	{
		CopyFlag |= AG2I_DIRTYFLAG_OVERLAY_LIPSYNC;
		PTR_PUTINT16(_pData, m_OverlayAnimLipSync.m_iAnimContainerResource);
		PTR_PUTINT16(_pData, m_OverlayAnimLipSync.m_iAnimSeq);
		PTR_PUTINT8(_pData, m_OverLayAnimLipSyncBaseJoint);
	}

	// Check if any tokens have been removed, and add them if any
	//if (m_lTokensRemoved.Len() > 0)
	{
		uint8* pDataBefore = _pData;
		int8& Len = ((int8*)_pData)[0];
		_pData++;
		Len = 0;
		int32 LenMirror = _pMirror->GetNumTokens();
		int32 LenLocal = m_lTokens.Len();
		for (int32 i = 0; i < LenMirror; i++)
		{
			bool bOk = false;
			for (int32 j = 0; j < LenLocal; j++)
			{
				if (_pMirror->m_lTokens[i].GetID() == m_lTokens[j].GetID())
				{
					bOk = true;
					break;
				}
			}
			if (!bOk)
			{
				PTR_PUTINT8(_pData,_pMirror->m_lTokens[i].GetID());
				Len++;
			}
		}
		if (Len > 0)
			CopyFlag |= AG2I_DIRTYFLAG_TOKENS_REMOVED;
		else
			_pData = pDataBefore;
	}

	// Tokens, num tokens, tokens write themselves
	uint8* pBeforeTokens = _pData;
	uint8& NumTokens = _pData[0];
	_pData++;
	NumTokens = 0;
	int32 Len = m_lTokens.Len();
	for (int32 i = 0; i < Len; i++)
	{
		uint8* pCurToken = _pData;
		// Write which token id we're updating
		int8 TokenID = m_lTokens[i].GetID();
		PTR_PUTINT8(_pData,TokenID);
		static const CWAG2I_Token DummyToken;
		const CWAG2I_Token* pTokenMirror = _pMirror->GetTokenFromID(TokenID);
		if (!pTokenMirror)
			pTokenMirror = &DummyToken;
		if (m_lTokens[i].OnCreateClientUpdate(_pData,pTokenMirror))
			NumTokens++;
		else
			_pData = pCurToken;
	}

	// If any tokens changed, add the token copy flag
	if (NumTokens > 0)
		CopyFlag |= AG2I_DIRTYFLAG_TOKENS;
	else
		_pData = pBeforeTokens;

	return (_pData - pBase);
}

int CWAG2I::OnClientUpdate2(CWAG2I_Context* _pContext, CWAG2I* _pAG2IMirror, CWAG2I* _pAG2I, const uint8* _pData)
{
	MSCOPESHORT(CWAG2I::OnCreateClientUpdate);

	if (!_pData || !_pAG2IMirror)
		return 0;

	const uint8* pBase = _pData;

	// Okidoki then, this function will put anything

	// Update Flag (8 bit), (agres, Tokens, overlay)
	PTR_GETINT8(_pData, _pAG2IMirror->m_DirtyFlag);

	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_PACKTYPE)
	{
		// Copy over new packmode (sends a complete ag state over)
		int8 Dirty = _pAG2IMirror->m_DirtyFlag;
		_pAG2IMirror->Clear();
		_pAG2IMirror->m_DirtyFlag = Dirty;
		PTR_GETINT8(_pData,_pAG2IMirror->m_LastPackMode);
	}

	// AGRes, just put it all, should only be copied once anyway, num entries (8bit), entry (32 + 32 bit)
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_RESOURCE)
	{
		int8 Len;
		PTR_GETINT8(_pData,Len);
		_pAG2IMirror->m_lAnimGraph2Res.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			_pAG2IMirror->m_lAnimGraph2Res[i].OnClientUpdate(_pData);
		// Clear resource pointers
		_pAG2IMirror->m_lspAnimGraph2.Clear();
		_pAG2IMirror->AcquireAllResources(_pContext);
	}
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_OVERLAY)
	{
		PTR_GETINT16(_pData, _pAG2IMirror->m_OverlayAnim.m_iAnimContainerResource);
		PTR_GETINT16(_pData, _pAG2IMirror->m_OverlayAnim.m_iAnimSeq);
		PTR_GETCMTIME(_pData, _pAG2IMirror->m_OverlayAnim_StartTime);
	}
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_OVERLAY_LIPSYNC)
	{
		PTR_GETINT16(_pData, _pAG2IMirror->m_OverlayAnimLipSync.m_iAnimContainerResource);
		PTR_GETINT16(_pData, _pAG2IMirror->m_OverlayAnimLipSync.m_iAnimSeq);
		PTR_GETINT8(_pData, _pAG2IMirror->m_OverLayAnimLipSyncBaseJoint);
	}

	// Check if any tokens have been removed, and add them if any
	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_TOKENS_REMOVED)
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
					if (_pAG2I)
						_pAG2I->m_pEvaluator->AG2_OnEnterState(_pContext, TokenID, AG2_STATEINDEX_TERMINATE, 0,0);
					_pAG2IMirror->m_lTokens.Del(j);
					break;
				}
			}
		}
	}

	if (_pAG2IMirror->m_DirtyFlag & AG2I_DIRTYFLAG_TOKENS)
	{
		// Tokens, num tokens, tokens write themselves
		int8 NumTokens;
		PTR_GETINT8(_pData, NumTokens);
		for (int32 i = 0; i < NumTokens; i++)
		{
			// Write which token id we're updating
			int8 TokenID;
			PTR_GETINT8(_pData,TokenID);

			// Find token and update it
			CWAG2I_Token* pToken = _pAG2IMirror->GetTokenFromID(TokenID,true);
			pToken->OnClientUpdate(_pContext, _pData);
		}
	}

	if (_pAG2I)
	{
		_pAG2I->AcquireAllResourcesToken(_pContext);
		_pAG2I->UpdateFromMirror(_pContext, _pAG2IMirror);
	}

	return (_pData - pBase);
}

void CWAG2I::RemoveToken(int32 _iToken)
{
	m_lTokens.Del(_iToken);
}

void CWAG2I::RemoveTokenByID(CAG2TokenID _iToken)
{
	for (int32 i = 0; i < m_lTokens.Len(); i++)
	{
		if (m_lTokens[i].GetID() == _iToken)
		{
			m_lTokens.Del(i);
			return;
		}
	}
}

// Read/write for savefile
void CWAG2I::Write(CCFile* _pFile) const
{
	// Skip animgraph resources, might change...
	int32 Len = m_lTokens.Len();
	// Write num tokens
	_pFile->WriteLE(Len);
	for (int32 i = 0; i < Len; i++)
		m_lTokens[i].Write(_pFile);

	// Overlays...?
	_pFile->WriteLE(m_OverlayAnim.m_iAnimContainerResource);
	_pFile->WriteLE(m_OverlayAnim.m_iAnimSeq);
	m_OverlayAnim_StartTime.Write(_pFile);
	_pFile->WriteLE(m_OverlayAnimLipSync.m_iAnimContainerResource);
	_pFile->WriteLE(m_OverlayAnimLipSync.m_iAnimSeq);
	_pFile->WriteLE(m_OverLayAnimLipSyncBaseJoint);
	// Write evaluator
	m_pEvaluator->Write(_pFile);
}

void CWAG2I::Read(CWAG2I_Context* _pContext, CCFile* _pFile)
{
	int32 Len;
	_pFile->ReadLE(Len);
	m_lTokens.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	{
		m_lTokens[i].SetAG2I(this);
		m_lTokens[i].Read(_pContext, _pFile);
	}

	// Overlays...?
	_pFile->ReadLE(m_OverlayAnim.m_iAnimContainerResource);
	_pFile->ReadLE(m_OverlayAnim.m_iAnimSeq);
	m_OverlayAnim_StartTime.Read(_pFile);
	_pFile->ReadLE(m_OverlayAnimLipSync.m_iAnimContainerResource);
	_pFile->ReadLE(m_OverlayAnimLipSync.m_iAnimSeq);
	_pFile->ReadLE(m_OverLayAnimLipSyncBaseJoint);
	
	// Read evaluator
	m_pEvaluator->Read(_pFile);
}

//--------------------------------------------------------------------------------

void CWAG2I::Log(CStr _Prefix, CStr _Name) const
{
	MSCOPESHORT(CWAG2I::Log);

#ifdef	AG2_DEBUG
	ConOutL(_Prefix + CStrF("%s (%X): iAGRes %d, iALRes %d, nTokens %d", (char*)_Name, this, m_iAnimGraph2Res, m_iAnimListRes, m_lTokens.Len()));

	if (m_OverlayAnim.IsValid() || (m_OverlayAnim_StartTime != AG2I_UNDEFINEDTIME))
		ConOutL(_Prefix + CStrF("  OverlayAnim: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f", m_OverlayAnim.m_iAnimContainerResource, m_OverlayAnim.m_iAnimSeq, m_OverlayAnim_StartTime));
	if (m_OverlayAnimLipSync.IsValid() || (m_OverlayAnim_StartTime != AG2I_UNDEFINEDTIME))
		ConOutL(_Prefix + CStrF("  OverlayAnimLipSync: iAnimContainerResource %d, iAnimSeq %d, StartTime %3.3f BaseJoint: %d", m_OverlayAnimLipSync.m_iAnimContainerResource, m_OverlayAnimLipSync.m_iAnimSeq, m_OverlayAnim_StartTime,m_OverLayAnimLipSyncBaseJoint));

	for (int iToken = 0; iToken < m_lTokens.Len(); iToken++)
	{
		const CWAG2I_Token* pToken = &(m_lTokens[iToken]);
		ConOutL(_Prefix + CStrF("  Token %d: nSIs %d, PSI <QT %3.3f, iEA %d>", 
							   pToken->GetID(), pToken->GetNumStateInstances(),
							   pToken->GetPlayingSIID().GetEnqueueTime(),
							   pToken->GetPlayingSIID().GetEnterActionIndex()));

		for (int iSI = 0; iSI < pToken->GetNumStateInstances(); iSI++)
		{
			const CWAG2I_StateInstance* pSI = pToken->GetStateInstance(iSI);
			ConOutL(_Prefix + CStrF("    SI %d: Flags [%s%s%s] <QT %3.3f, iEA %d, ET %3.3f, iLA %d, LT %3.3f>",
									iSI,
									(pSI->IsPMAdded() ? "A" : "a"),
									(pSI->IsPMRemoved() ? "R" : "r"),
									(pSI->IsPMUpdated() ? "U" : "u"),
									pSI->GetEnqueueTime(),
									pSI->GetEnterActionIndex(), pSI->GetEnterTime(),
									pSI->GetLeaveActionIndex(), pSI->GetLeaveTime()));
		}
	}
#endif
}
