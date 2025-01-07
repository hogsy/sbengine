#include "PCH.h"

#include "WObj_Player.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Player, CWObject_RPG, 0x0100);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Player_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWO_Player_ClientData::Copy(const CWO_Player_ClientData& _CD)
{
	MAUTOSTRIP(CWO_Player_ClientData_Copy, MAUTOSTRIP_VOID);
	m_GameTick = _CD.m_GameTick;
	m_GameTime = _CD.m_GameTime;
}

void CWO_Player_ClientData::Clear(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWO_Player_ClientData_Clear, MAUTOSTRIP_VOID);
	m_pObj = _pObj;
	
	m_bIsValidPrediction = false;
	m_bIsFullFramePrediction = false;
	m_bIsPredicting = false;
	m_bIsClientRefresh =  false;
	m_bIsServerRefresh = false;
	m_bDoOneTimeEffects = false;
	m_PredictLastCmdID = -1;
	m_PredictFrameFrac = 1.0f;
	m_OneTimeEffectsGameTick = 0;
	m_GameTick = 0;
	m_GameTime.Reset();
}

CWO_Player_ClientData::CWO_Player_ClientData()
{
	MAUTOSTRIP(CWO_Player_ClientData_ctor, MAUTOSTRIP_VOID);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Player
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CWObject_Player::IsLocalPlayer(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Player_IsLocalPlayer, false);
	return _pObj->m_iObject == _pWClient->Player_GetLocalObject();
}

bool CWObject_Player::IsPredicted(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Player_IsPredicted, false);
#ifdef WCLIENT_FIXEDRATE
	return false;

#else

	if (_pWClient->GetRecMode() == 2) return false;		// Not predicted if we're in demo playback mode
	if (!IsLocalPlayer(_pObj, _pWClient)) return false;	// Not predicted if it is not the local player
	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SERVERCONTROL) return false;

	// Not predicted if prediction is disabled.
	CRegistry* pUserReg = _pWClient->Registry_GetUser();
	int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
	return bNoPredict == 0;
#endif
}

bool CWObject_Player::IsMirror(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Player_IsMirror, false);
	return (_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR);
}

// -------------------------------------------------------------------
//#ifdef _DEBUG
	CWO_Player_ClientData* CWObject_Player::GetClientData(CWObject_CoreData* _pObj)
	{
		CWO_Player_ClientData* pPCD = safe_cast<CWO_Player_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		return pPCD;
	};
	const CWO_Player_ClientData* CWObject_Player::GetClientData(const CWObject_CoreData* _pObj)
	{
		const CWO_Player_ClientData* pPCD = safe_cast<const CWO_Player_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		return pPCD;
	};
	CXR_SkeletonInstance* CWObject_Player::GetClientSkelInstance(CWObject_CoreData* _pObj)
	{
		CXR_SkeletonInstance *pSI = safe_cast<CXR_SkeletonInstance>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE]);
		return pSI;
	};
/*#else
	CWO_Player_ClientData* CWObject_Player::GetClientData(CWObject_CoreData* _pObj) { return (CWO_Player_ClientData*)(CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]; };
	const CWO_Player_ClientData* CWObject_Player::GetClientData(const CWObject_CoreData* _pObj) { return (CWO_Player_ClientData*)(const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]; };
	CXR_SkeletonInstance* CWObject_Player::GetClientSkelInstance(CWObject_CoreData* _pObj) { return (CXR_SkeletonInstance*)(CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE]; };
#endif*/

// -------------------------------------------------------------------
CWO_Player_ClientData* CWObject_Player::GetCD(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWObject_Player_GetCD, NULL);
	CWO_Player_ClientData* pCD = safe_cast<CWO_Player_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);	

	if (!pCD)
		Error_static("CWObject_Player::GetCD", "NULL clientdata.");

	return pCD;
}

const CWO_Player_ClientData* CWObject_Player::GetCD(const CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWObject_Player_GetCD_2, NULL);
	const CWO_Player_ClientData* pCD = safe_cast<const CWO_Player_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);	

	if (!pCD)
		Error_static("CWObject_Player::GetCD", "NULL clientdata.");

	return pCD;
}

// -------------------------------------------------------------------
CWObject_Client* CWObject_Player::Player_GetLastValidPrediction(CWObject_Client* _pObj)
{
	MAUTOSTRIP(CWObject_Player_Player_GetLastValidPrediction, NULL);
	CWObject_Client* pPred = _pObj;
	while(pPred)
	{
		if (!pPred->GetNext()) break;
		if (!GetCD(pPred->GetNext())->m_bIsValidPrediction) break;
		pPred = pPred->GetNext();
	}

	return pPred;
}

int CWObject_Player::Player_GetCopyNumber(CWObject_Client* _pObj)
{
	MAUTOSTRIP(CWObject_Player_Player_GetCopyNumber, 0);
	int iCopy = 0;
	while(_pObj->GetPrev())
	{
		iCopy++;
		_pObj = _pObj->GetPrev();
	}
	return iCopy;
}

CWObject_Client* CWObject_Player::Player_FindCopyWithCmdID(CWObject_Client* _pObj, int _CmdID)
{
	MAUTOSTRIP(CWObject_Player_Player_FindCopyWithCmdID, NULL);
	while(_pObj)
	{
		CWO_Player_ClientData *pCD = GetCD(_pObj);
		if(pCD->m_PredictLastCmdID == _CmdID) return _pObj;
		_pObj = _pObj->GetNext();
	}
	return NULL;
}

void CWObject_Player::Player_InvalidatePredictions(CWObject_Client* _pPredict)
{
	MAUTOSTRIP(CWObject_Player_Player_InvalidatePredictions, MAUTOSTRIP_VOID);
	while(_pPredict)
	{
		CWO_Player_ClientData *pCDPred = GetCD(_pPredict);
		pCDPred->m_bIsValidPrediction = false;
		_pPredict = _pPredict->GetNext();
	}
}

//#define LOG_PREDICT2
//#define LOG_CLIENTAUTHORING

// -------------------------------------------------------------------
void CWObject_Player::OnClientPredict(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _Cmd, int _Param0, int _Param1)
{
	MAUTOSTRIP(CWObject_Player_OnClientPredict, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Player::OnClientPredict, CHARACTER);
	CWC_Player* pP = _pWClient->Player_GetLocal();
	if (!pP->m_spCmdQueue) return;
	CWO_Player_ClientData *pCD = GetClientData(_pObj);
	if (!pCD) return;

	if (pCD->m_OneTimeEffectsGameTick < pCD->m_GameTick)
		pCD->m_OneTimeEffectsGameTick = pCD->m_GameTick;

//M_TRACEALWAYS(CStrF("Prediction start %s, Vel %s, %s", (char*)pPredict->GetPosition().GetString(), (char*)pPredict->GetMoveVelocity().GetString(), (char*)_pObj->GetMoveVelocity().GetString()));

	if (_Cmd == 1)
	{
		// -------------------------------------------------------------------
		//  1 == Retire commands, _Param0 == Last Cmd ID
		// -------------------------------------------------------------------
		int LastID = _Param0;
		CCmd* pCmd = pP->m_spCmdQueue->GetTail();

		// Search for command LastID
		int bFound = false;
		while(pCmd)
		{
			if (pCmd->m_ID == LastID)
			{
				bFound = true;
				break;
			}
			pCmd = pP->m_spCmdQueue->GetNext(pCmd);
		}

		// Retire all commands previous to, and including, LastID.
		if (bFound)
		{
#ifdef LOG_CLIENTAUTHORING
M_TRACEALWAYS(CStrF("(Player::OnClientPredict) RETIRE: Found ID %d in cmd-queue\r\n", LastID));
#endif
			CWObject_Client* pMatchPredict = NULL;
			CWObject_Client* pPredict = _pObj;
			while(pPredict)
			{
				CWO_Player_ClientData *pCDPred = GetClientData(pPredict);
				if (!pCDPred) break;

				if (pCDPred->m_bIsValidPrediction)
				{
					if (pCDPred->m_PredictLastCmdID == LastID)
					{
//						ConOut("Found predicted frame.");
						pMatchPredict = pPredict;
						break;
					}
				}
				else
					break;

				pPredict = pPredict->GetNext();
			}

			if (pMatchPredict == _pObj)
			{
				M_TRACEALWAYS(CStrF("(Char::OnClientPredict) RETIRE: Server char hasn't moved. LastID == %d\r\n", LastID ));
				return;
			}

pMatchPredict = NULL;

			if (!pMatchPredict)
			{
				CWObject_Client* pPredict = Player_GetLastValidPrediction(_pObj);
				if (pPredict == _pObj)
					pPredict = GetCD(_pObj)->CreateClientObject(_pObj, _pWClient);

				{
					CWO_Player_ClientData* pCDPred = GetClientData(pPredict);
					if (pCDPred->m_bIsValidPrediction && pCDPred->m_bIsFullFramePrediction)
						pPredict = GetCD(_pObj)->CreateClientObject(pPredict, _pWClient);
				}

#ifdef LOG_PREDICT2
M_TRACEALWAYS(CStrF("(Player::OnClientPredict) RETIRE: Copying master (%.8x) to prediction (%.8x), (LastID %d)\r\n", _pObj, pPredict, LastID));
#endif
				GetCD(_pObj)->CopyClientObject(_pObj, pPredict);

				pP->m_spCmdQueue->ResetTempTail();

				CControlFrame Ctrl;
				fp32 Accum = pP->m_spCmdQueue->m_LastTmpAccumTime;
				bool bFrameFound = false;

				fp32 TQueue = pP->m_spCmdQueue->GetTotalTempCmdTime_IncAccum();
				fp32 TQueueInitial = TQueue;
//#ifdef LOG_PREDICT2
//M_TRACEALWAYS(CStrF("(Char::OnClientPredict) RETIRE: TQueue %f", TQueue));
//#endif
				pCD->m_LastUpdateNumTicks = 0;

				while(!bFrameFound && TQueue > 0.0f)
				{
					_pObj->m_iActiveClientCopy = CWObject_Player::Player_GetCopyNumber(pPredict);
					if (!_pWClient->Object_GetCD(_pObj->m_iObject))
					{
						M_TRACEALWAYS(CStrF("§cf80WARNING: Could not obtain client copy %d\r\n", _pObj->m_iActiveClientCopy));
						break;
					}

					CWO_Player_ClientData *pCDPred = GetClientData(pPredict);
					if (!pCDPred) Error_static("CWObject_Character::OnClientPredict", "NULL client-data.");
					pCDPred->m_bIsClientRefresh =  true;
					if (pCDPred->m_GameTick == pCD->m_OneTimeEffectsGameTick)
					{
						pCDPred->m_bDoOneTimeEffects = true;
						pCD->m_OneTimeEffectsGameTick++;
					}


					int EndID = -1;
					do 
					{
						if(bFrameFound)
						{
							// Already found the command we're looking for, make sure to execute any empty time which appears before the next command...
							if(!pP->m_spCmdQueue->GetEmptyTempFrame(Ctrl, _pWClient->GetGameTickTime()))
								break;
						}
						else
							pP->m_spCmdQueue->GetTempFrame(Ctrl, _pWClient->GetGameTickTime(), LastID);

#ifdef LOG_PREDICT2
M_TRACEALWAYS(CStrF("(Player::OnClientPredict) RETIRE: %d, Predicting frame with copy %.8x, LastID %d, TQueue %f -> %f\r\n", pCDPred->m_GameTick, pPredict, LastID, TQueue, pP->m_spCmdQueue->GetTotalTempCmdTime_IncAccum()));
#endif
#ifdef LOG_CLIENTAUTHORING
						EndID = GetCD(pPredict)->OnClientPredictControlFrame(_pWClient, pPredict, Ctrl, true, true);
M_TRACEALWAYS(CStrF("(Player::OnClientPredict) RETIRE: %d, CLIENT PREDICT ID ?->%d, ACCUM %f -> %f, Pred %.8x, base %.8x\r\n", 
		pCDPred->m_GameTick, EndID, Accum, pP->m_spCmdQueue->m_LastTmpAccumTime, pPredict, _pObj));
#else
						EndID = GetCD(pPredict)->OnClientPredictControlFrame(_pWClient, pPredict, Ctrl, true);
#endif
						pCD->m_LastUpdateNumTicks++;

						pCDPred->m_bDoOneTimeEffects = false;

						Accum = pP->m_spCmdQueue->m_LastTmpAccumTime;
						TQueue = pP->m_spCmdQueue->GetTotalTempCmdTime_IncAccum();

						if(EndID == LastID)
							bFrameFound = true;

					} while(TQueue > _pWClient->GetGameTickTime());

					pCDPred->m_bIsClientRefresh =  false;
					if (bFrameFound)
					{
//						ConOut(CStrF("Predicted to end-frame.  %.8x", pPredict));

#ifdef LOG_PREDICT2
M_TRACEALWAYS(CStrF("(UpdatePrediction) Copying client-data from predict copy %.8x to mastercopy %.8x, CD %.8x -> %.8x, EndID == LastID == %d\r\n", pPredict, _pObj, pCDPred, pCD, LastID));
#endif

						pCD->Copy(*pCDPred);
						pCDPred->m_bIsValidPrediction = 1;
						pCDPred->m_PredictLastCmdID = LastID;
						pCD->m_bIsValidPrediction = 1;
						pCD->m_PredictLastCmdID = LastID;
						break;

//						if (pPrediction->GetPosition().Distance(
					}
#ifdef LOG_PREDICT2
M_TRACEALWAYS(CStrF("(UpdatePrediction) Looping copy %.8x, LastID %d != EndID %d, TQueue %f\r\n", pPredict, LastID, EndID, TQueue));
#endif
				}

//#ifdef LOG_CLIENTAUTHORING
				if (!bFrameFound)
				{
					M_TRACEALWAYS("-------------------------------------------------------------------\r\n");
					M_TRACEALWAYS(CStrF("FRAME NOT FOUND with lastID == %d, TQueue %.3f\r\n", LastID, TQueueInitial ));
					M_TRACEALWAYS("-------------------------------------------------------------------\r\n");
				}
//#endif

				_pObj->m_iActiveClientCopy = 0;

			}
			else
			{
				M_TRACEALWAYS("Could not match prediction frame!\r\n");
			}

		}
		else
		{
#ifdef LOG_CLIENTAUTHORING
M_TRACEALWAYS(CStrF("(UpdatePrediction) Didn't find ID %d in cmd-queue\r\n", LastID));
#endif
		}

		// Invalidate all predicted object instances.
		Player_InvalidatePredictions(_pObj->GetNext());

	}
	else if (_Cmd == 2)
	{
		if (!IsPredicted(_pObj, _pWClient)) return;

		// -------------------------------------------------------------------
		//  2 == Compare update with prediction, _Param0 == Last Cmd ID
		// -------------------------------------------------------------------
		int ID = _Param0;

		CWObject_Client* pPredict = Player_FindCopyWithCmdID(_pObj->GetNext(), ID);
		if (pPredict)
		{
/*			if (_pObj == pPredict)
			{
				M_TRACEALWAYS(CStrF("(Predict 2) Server char hasn't moved. LastID == %d", ID ));
			}
			else*/
			{
				pCD->OnClientCheckPredictionMisses(_pWClient, _pObj, pPredict, false);
			}
		}
		else
		{
			if (pCD->m_PredictLastCmdID == ID || ID == 255)
			{
			}
			else
			{
	#ifdef LOG_CLIENTAUTHORING
				M_TRACEALWAYS(CStrF("Didn't find a prediction copy with lastID == %d, Current ID %d, _pObj = %.8x\r\n", ID, pCD->m_PredictLastCmdID, _pObj ));
	#endif
				CWObject_Client* pPred = _pObj;
				while(pPred)
				{
	#ifdef LOG_CLIENTAUTHORING
					CWO_Player_ClientData* pCD = GetClientData(pPred);
					M_TRACEALWAYS(CStrF("     %.8x, Valid %d, Full %d, LastID %d, Frac %f\r\n", pPred , pCD->m_bIsValidPrediction, pCD->m_bIsFullFramePrediction, pCD->m_PredictLastCmdID, pCD->m_PredictFrameFrac));
	#endif
					pPred = pPred->GetNext();
				}

				Player_InvalidatePredictions(_pObj->GetNext());
			}
		}
	}
	else
	{
		// -------------------------------------------------------------------
		//  0 == Predict command queue
		// _Param1 == 1: m_iActiveClientCopy is not restored
		// -------------------------------------------------------------------

		TProfileDef(TPredict);
		{
			TMeasureProfile(TPredict);
			// Alive
			pP->m_spCmdQueue->ResetTempTail();

	#ifdef LOG_PREDICTION
	M_TRACEALWAYS(CStrF("Client AccumTime %f\r\n", pP->m_spCmdQueue->m_LastAccumTime));
	#endif

			CWObject_Client* pBase = _pObj;

			int LastID = -1;
			int EndID = -1;
			int ActiveCopy = 0;
			CControlFrame Ctrl;

			CWO_Player_ClientData *pCDPrev = NULL;
//			fp64 TQueue = pP->m_spCmdQueue->GetTotalTempCmdTime_IncAccum();
			while(pP->m_spCmdQueue->GetTempFrame(Ctrl, _pWClient->GetGameTickTime()))
			{
//				pP->m_spCmdQueue->GetTempFrame(Ctrl, SERVER_TIMEPERFRAME)

				ActiveCopy++;
				if (ActiveCopy > 20) break;

				if (!pBase->GetNext())
				{
					GetCD(pBase)->CreateClientObject(pBase, _pWClient);

/*					TPtr<CWObject_Client> spObj = DNew(CWObject_Client) CWObject_Client;
					if (!spObj) break;
					spObj->LinkAfter(pBase);
					InitClientObjects(spObj, _pWClient);*/
				}

				CWObject_Client* pPredict = pBase->GetNext();
				bool FullFrame = pP->m_spCmdQueue->TempFrameAvail();

				CWO_Player_ClientData *pCDBase = GetClientData(pBase);
				CWO_Player_ClientData *pCDPred = GetClientData(pPredict);
				if (!pCDBase || !pCDPred) break;

				if (FullFrame && pCDPred->m_bIsValidPrediction && pCDPred->m_bIsFullFramePrediction)
				{
					pBase = pPredict;
					continue;
				}

				GetCD(pPredict)->CopyClientObject(pBase, pPredict);
				pCDPred->m_bIsValidPrediction = true;

				_pObj->m_iActiveClientCopy = ActiveCopy;
				if (!_pWClient->Object_GetCD(_pObj->m_iObject))
				{
					M_TRACEALWAYS(CStrF("§cf80WARNING: Could not obtain client copy %d\r\n", _pObj->m_iActiveClientCopy));
					break;
				}

				pCDPred->m_bIsPredicting = true;
				pCDPred->m_bIsClientRefresh =  true;
				if (FullFrame && (pCDPred->m_GameTick == pCD->m_OneTimeEffectsGameTick))
				{
					pCDPred->m_bDoOneTimeEffects = true;
					pCD->m_OneTimeEffectsGameTick++;
				}

#ifdef LOG_PREDICT2
				EndID = GetCD(pPredict)->OnClientPredictControlFrame(_pWClient, pPredict, Ctrl, FullFrame, true);
#else
				EndID = GetCD(pPredict)->OnClientPredictControlFrame(_pWClient, pPredict, Ctrl, FullFrame);
#endif
				if(EndID != -1)
				{
					LastID = EndID;
				}
				else
				{
					// Frame without any cmd's, use last frame's ID (still might be -1 but this is better)
					pCDPred->m_PredictLastCmdID = LastID;

					// Invalidate previous prediction since these 2 are sharing CmdID
					if(pCDPrev) {pCDPrev->m_PredictLastCmdID = -1;}
				}

				pCDPrev = pCDPred;
				pCDPred->m_bIsPredicting = false;
				pCDPred->m_bIsClientRefresh =  false;
				pCDPred->m_bDoOneTimeEffects = false;

#ifdef LOG_PREDICT2
M_TRACEALWAYS(CStrF("(Prediction) Predicted (%.8x) -> (%.8x), CD %.8x -> %.8x\r\n", pBase, pPredict, pCDBase, pCDPred));
#endif

#ifdef LOG_PREDICTION
M_TRACEALWAYS(CStrF("Predict %d, %d, Obj %.8x -> %.8x\r\n", ActiveCopy, FullFrame, pBase, pPredict));
#endif
				pBase = pPredict;
//				TQueue = pP->m_spCmdQueue->GetTotalCmdTime_IncAccum();
			}

			if (pBase->GetNext())
				GetClientData(pBase->GetNext())->m_bIsValidPrediction = false;
				
#ifdef LOG_PREDICTION
			M_TRACEALWAYS(CStrF("PREDICTED ID %d->%d from %s to %s\r\n", /*CmdTotalIDStart*/-1, EndID, (char*)_pObj->GetPosition().GetString(), (char*)pBase->GetPosition().GetString()));
#endif
		}

		if (_Param1 != 1)
			_pObj->m_iActiveClientCopy = 0;

#ifdef M_Profile
		_pWClient->AddGraphPlot(WCLIENT_GRAPH_PREDICTTIME, TPredict.GetTime(), 0xff606090);
#endif
	}
}

