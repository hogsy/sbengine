
#include "PCH.h"

#include "WServer_Core.h"
#include "../WPackets.h"
#include "../WObjects/WObj_Game.h"



// URGENTFIXME: THIS SHOULD BE ENABLED!
#define SERVER_LAZYUPDATE

bool CWorld_ServerCore::CreateClientUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer, int _bAllObjects)
{
	MAUTOSTRIP(CWorld_ServerCore_CreateClientUpdate, false);
	MSCOPESHORT(CWorld_ServerCore::CreateClientUpdate);
	M_NAMEDEVENT("CreateClientUpdate", 0xff000000);

	World_CommitDeferredSceneGraphLinkage();

	// true => update is complete
	// false => additional updates is required.

	CWorld_Client* pWClient = m_lspClients[_iClient];
	CWServer_ClientInfo* pWClientInfo = m_lspClientInfo[_iClient];
#ifdef WSERVER_NOLOCALMIRROR
	if (pWClientInfo->m_spLocalClient != NULL)
		pWClient = pWClientInfo->m_spLocalClient;
#endif


#ifdef ENABLE_CLIENTUPDATE_SIZECHECK
	static int SIZECHECK_BYTES = -1;
	if (SIZECHECK_BYTES < 0)
	{
		MACRO_GetSystemEnvironment(pEnv);
		bint bNetDebugEnabled = pEnv->GetValuei("NET_DEBUG", 1);
		SIZECHECK_BYTES = bNetDebugEnabled ? 2 : 0;
	}
#else
# define SIZECHECK_BYTES 0
#endif

	if (!_pDeltaBuffer)
		Error("CreateClientUpdate", "_pDeltaBuffer == NULL");
	if (_pDeltaBuffer) _pDeltaBuffer->Clear();

	TArray<spCWObject_Client> lspObjectsMirror = pWClient->Object_GetArray();

	int nObj = m_lspObjects.Len();
	lspObjectsMirror.SetLen(nObj);
	TPtr<CWObject_Client>* lspClientObj = lspObjectsMirror.GetBasePtr();

	CWS_ClientObjInfo* pObjInfo = pWClientInfo->m_lObjInfo.GetBasePtr();
	if (pWClientInfo->m_lObjInfo.Len() != nObj)
		Error("CreateClientUpdate", "Client object-info not initialized.");

	uint8 DeltaBuffer[4096];

//	CWObject NullObj;
	CWObject_Client NullObjClient;

	TProfileDef(TPVS);
	TProfileDef(TPVSEnum);
	TProfileDef(TCompare);

	const int MaxObj = 4096;
	uint16 liObj[MaxObj];
	static uint16 liObjClient[MaxObj];
	if (m_lspObjects.Len() > MaxObj) Error("CreateClientUpdate", "Too many objects.");
	int niObj = 0;
	int iPlayer = pWClientInfo->m_iPlayer;
	CWObject_Game *pGame = Game_GetObject();
	CWObject* pPlayerObj = pGame ? pGame->Player_GetObject(iPlayer) : NULL;

//	const uint8* pPVS = NULL;
	const uint8* pPVS = NULL;
	if (pPlayerObj && m_pSceneGraph && m_spSceneGraphInstance != NULL && !_bAllObjects)
	{
		TMeasureProfile(TPVS);
		pPVS = m_pSceneGraph->SceneGraph_PVSLock(0, Client_GetPVSPosition(pWClientInfo));
//		pPVS = m_pSceneGraph->SceneGraph_PVSLock(0, pPlayerObj->GetPosition() + CVec3Dfp32(0,0,1));

//		ConOut("Before " + (pPlayerObj->GetPosition() + CVec3Dfp32(0,0,1)).GetString());
//		ConOut("After " + Client_GetPVSPosition(pWClientInfo).GetString());
	}

	if (!pPVS)
	{
		for(int i = 0; i < nObj; i++)
			liObj[i] = i;
		niObj = nObj;
	}
	else
	{
		TMeasureProfile(TPVSEnum);

		int nObjClient = pWClient->World_EnumerateVisible(Client_GetPVSPosition(pWClientInfo), liObjClient, MaxObj);
		niObj = m_spSceneGraphInstance->SceneGraph_EnumeratePVS(pPVS, liObj, MaxObj, liObjClient, nObjClient);

		int iPlayerObj = pPlayerObj->m_iObject;
		int i = 0;
		for(; i < niObj; i++)
			if (liObj[i] == iPlayerObj) break;
		if (i == niObj) 
		{
//			ConOut("(CWorld_ServerCore::CreateClientUpdate) Player was not in PVS.");
			liObj[niObj++] = iPlayerObj;
		}
	}

	
	bool bComplete = true;
	{
		TMeasureProfile(TCompare);

		int TotSize = 0;
		int nUpdatedObj = 0;
		for(int i = 0; i < niObj; i++)
		{
			int iObj = liObj[i];

			if (!m_lspObjects.ValidPos(iObj))
			{
				ConOut(CStrF("Object out of range %d/%d", iObj, m_lspObjects.Len()));
				continue;
			}

			CWObject* pObj = m_lspObjects[iObj];
			if(pObj && pObj->m_DirtyMask != 0)
			{
				Object_SetDirty(iObj, pObj->m_DirtyMask);
				pObj->m_DirtyMask = 0;
			}

	#ifdef SERVER_LAZYUPDATE
			if (!_bAllObjects && !pObjInfo[iObj].m_DirtyMask)
			{
				if (pObj)
					pObj->m_LastClientUpdate = m_SimulationTick;
				continue;
			}
	//		int bHierarchy = pObjInfo[iObj].m_DirtyMask & CWO_DIRTYMASK_HIERARCHY;
	#endif

			if (pObj && /*!bHierarchy && */pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOUPDATE) continue;

			if (pObj && pObj->IsDestroyed())
				Error("CreateClientUpdate", CStrF("Object not destroyed. %d", iObj));

			CWObject_Client* pClObj = lspClientObj[iObj];

			if( pClObj )
			{
				if (!pClObj->m_iClass)
				{
					pClObj = &NullObjClient;
				}
				else if (pObj && (pClObj->m_iClass != pObj->m_iClass))
				{
					pClObj = &NullObjClient;
				}
			}
			else pClObj = &NullObjClient;

//			NullObj.m_iObject = iObj;
			int Size = 0;
			if (pObj && pObj->m_iClass)
				Size = pObj->OnCreateClientUpdate(_iClient, &pObjInfo[iObj], pClObj, &DeltaBuffer[SIZECHECK_BYTES], 0);
			else
			{
				if( pClObj->m_iClass )
				{
					uint8* pD = &DeltaBuffer[SIZECHECK_BYTES];
					PTR_PUTINT16(pD, iObj & 0xfff);
					Size = 2;
				}
			}

			if (SIZECHECK_BYTES > 0)
			{
				DeltaBuffer[0] = Size & 0xff;
				DeltaBuffer[1] = Size >> 8;
				Size += SIZECHECK_BYTES;
			}

//				NullObj.OnCreateClientUpdate(_iClient, &pObjInfo[iObj], pClObj, &DeltaBuffer[0], 0);

			if (Size > sizeof(DeltaBuffer))
			{
#ifndef M_RTM
				Error("CreateClientUpdate", CStrF("Temporary buffer overrun. Possible stack-trashing. (%d, %s, %s)", Size, pObj->GetName(), pObj->MRTC_GetRuntimeClass()->m_ClassName));
#else
				Error("CreateClientUpdate", CStrF("Temporary buffer overrun. Possible stack-trashing. (%d, %s, %s)", Size, "", ""));
#endif
			}
			else if(Size > sizeof(DeltaBuffer) / 3)
				ConOutL(CStrF("Extremely large packet (%i bytes) from Object %i (%s %s)", Size, pObj->m_iObject, pObj->GetName(), pObj->GetTemplateName()));

	#ifndef M_RTM
			if (pObj && Size > SIZECHECK_BYTES)
			{
	//			MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(pObj->m_iClass);
	//			if (pRTC && _bAllObjects && m_bNetShowMsg) ConOutL(CStrF("    iObj %d, Size %d, %s", iObj, Size, (char*)pRTC->m_ClassName ));

				CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(pObj->m_iClass);
				if (pCS)
				{
					pCS->m_NetUpdate += Size;
					pCS->m_nNetUpdates++;
				}
				nUpdatedObj++;
			}
	#endif

			TotSize += Size;
			if (Size > SIZECHECK_BYTES)
			{
				if (!_pDeltaBuffer->AddDeltaUpdate(&DeltaBuffer[0], Size))
				{
					if (!_bAllObjects)
					{
						ConOut(CStrF("(CWorld_ServerCore::CreateClientUpdate) Max frame-size exceeded. %d ents -> %d+%d", nUpdatedObj, _pDeltaBuffer->GetSize(), Size));
						M_TRACEALWAYS("(CWorld_ServerCore::CreateClientUpdate) Max frame-size exceeded. %d ents -> %d+%d\n", nUpdatedObj, _pDeltaBuffer->GetSize(), Size);
					}
					bComplete = false;
					break;
				}
				else
				{
					pObjInfo[iObj].m_DirtyMask = 0;
					if (pObj)
						pObj->m_LastClientUpdate = m_SimulationTick;
				}
			}
			else
			{
				if (pObj)
					pObj->m_LastClientUpdate = m_SimulationTick;
			}
		}
	}

	if (pPVS && m_pSceneGraph)
		m_pSceneGraph->SceneGraph_PVSRelease(pPVS);

	_pDeltaBuffer->m_nSimTicks = pWClientInfo->m_nClientSimTicks;
	_pDeltaBuffer->m_SimulationTick = m_SimulationTick;
	pWClientInfo->m_nClientSimTicks = 0;

#if defined(M_Profile)
	CRegistry* pReg = Registry_GetClientVar(_iClient);
	if (pReg && pReg->GetValuei("SHOWUPDATETIME", 0, 0))
		Net_ConOut(TString("Client Update, PVS", TPVS) + TString(", Enum PVS", TPVSEnum) + TString(", Compare", TCompare), _iClient);
#endif

	return bComplete;
//	ConOutL(CStrF("Frame out: %d", TotSize));
}

int PackDeltaRegistry_r(const CRegistry* _pReg, const CRegistry* _pOldReg, uint8* _pData, int _RecommendLen, int _MaxLen, int _RecurseDepth);

bool CWorld_ServerCore::CreateClientRegUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer)
{
	MAUTOSTRIP(CWorld_ServerCore_CreateClientRegUpdate, false);
	MSCOPESHORT(CWorld_ServerCore::CreateClientRegUpdate);
	// 'true' means that nothing was added to the _pDeltaBuffer

	CWorld_Client* pWClient = m_lspClients[_iClient];
	CWServer_ClientInfo* pWClientInfo = m_lspClientInfo[_iClient];
#ifdef WSERVER_NOLOCALMIRROR
	if (pWClientInfo->m_spLocalClient != NULL)
		pWClient = pWClientInfo->m_spLocalClient;
#endif

	if (!_pDeltaBuffer)
		Error("CreateClientRegUpdate", "_pDeltaBuffer == NULL");
	_pDeltaBuffer->Clear();

	if (!pWClientInfo->m_spNetReg)
		Error("CreateClientRegUpdate", "No client registry.");

	if (!pWClient->Registry_GetRoot())
		Error("CreateClientRegUpdate", "Mirror registry not created.");

	FillChar(_pDeltaBuffer->GetBufferPos(), _pDeltaBuffer->BufferLeft(), 0xdd);

	bool bDone = true;
	int BufferLeft = _pDeltaBuffer->BufferLeft();
	int Size = PackDeltaRegistry_r(pWClientInfo->m_spNetReg, pWClient->Registry_GetRoot(), _pDeltaBuffer->GetBufferPos(), 150, BufferLeft, 0);
	if (Size)
	{
		if (Size > BufferLeft)
			M_ASSERT(0, "?");
		_pDeltaBuffer->IncSize(Size);
		bDone = false;

	}


	return bDone;
}

void CWorld_ServerCore::UnpackServerRegUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer)
{
	MAUTOSTRIP(CWorld_ServerCore_UnpackServerRegUpdate, MAUTOSTRIP_VOID);
/*	CWServer_ClientInfo* pWClientInfo = m_lspClientInfo[_iClient];

	uint8* pDB = _pDeltaBuffer->m_Data;
	int Pos = 0;
	while(Pos < _pDeltaBuffer->m_Size)
	{
		int iReg;
		PTR_PUTINT8(pDB, iReg);

		int nUnpacked = UnpackDeltaRegistry(m_lspNetReg[iReg], pDB, _pDeltaBuffer->m_Size-Pos);
		pDB += nUnpacked;
		Pos += nUnpacked;
	}*/
}

// -------------------------------------------------------------------
void CWorld_ServerCore::Client_Init(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_Init, MAUTOSTRIP_VOID);
	m_lspClientInfo[_iClient]->m_spClientReg = m_spServerReg->CreateDir(CStrF("CLIENTMIRROR%d", _iClient));

/*	if(m_lspClientInfo[_iClient]->m_iPlayer == -1)
	{
		CWObject_Game *pGame = Game_GetObject();
		spCWO_Player spPlayer = pGame->CreatePlayerObject();
		int iPlayer = pGame->Player_Add(spPlayer, _iClient);
	if (iPlayer == -1) Error("Net_CheckPendingConnections", "Unable to create player.");
	m_lspClientInfo[_iClient]->m_iPlayer = iPlayer;
	}*/
	m_lspClientInfo[_iClient]->m_ServerUpdateMask = SERVER_CLIENTUPDATE_WORLD;
}

void CWorld_ServerCore::Client_SetNumRegDirectories(int _iClient, int _nReg)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_SetNumRegDirectories, MAUTOSTRIP_VOID);
	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];

	if (!pCI->m_spNetReg)
		pCI->m_spNetReg = REGISTRY_CREATE;

	int OldLen = pCI->m_spNetReg->GetNumChildren();
	pCI->m_spNetReg->SetNumChildren(_nReg);
//	pCI->m_lspNetReg.SetLen(_nReg);
//	m_lspClients[_iClient]->m_lspNetReg.SetLen(_nReg);
	for(int i = OldLen; i < _nReg; i++)
	{
		pCI->m_spNetReg->SetValue(i, CFStrF("NETREG%d", i).Str());
//		pCI->m_lspNetReg[i] = pCI->m_spClientReg->CreateDir(CStrF("NETREG%d", i));
//		m_lspClients[_iClient]->m_lspNetReg[i] = pCI->m_spClientReg->CreateDir(CStrF("NETREG%d", i));
	}
}

CVec3Dfp32 CWorld_ServerCore::Client_GetPVSPosition(CWServer_ClientInfo* _pCInfo)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetPVSPosition, CVec3Dfp32());
	CMat4Dfp32 Mat;
	Mat.Unit();

	CWObject_Game *pGame = Game_GetObject();
	CWObject* pObj = pGame ?  pGame->Player_GetObject(_pCInfo->m_iPlayer) : NULL;
	if (pObj)
	{
		CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
		Msg.m_pData = &Mat;
		Msg.m_DataSize = sizeof(Mat);
		Message_SendToObject(Msg, pObj->m_iObject);
	}
	return CVec3Dfp32::GetMatrixRow(Mat, 3);
}

int CWorld_ServerCore::Client_GetNumLagFrames(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetNumLagFrames, 0);
	if (!m_lspClientInfo.ValidPos(_iClient)) return 0;
	if (!m_lspClientInfo[_iClient]) return 0;
	return m_lspClientInfo[_iClient]->m_nClientSimTicks;
}

int CWorld_ServerCore::Client_GetCount()
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetCount, 0);
	int nC = 0;
	for(int i = 0; i < m_lspClients.Len(); i++)
		if (m_lspClients[i]) nC++;
	return nC;
}

uint32 CWorld_ServerCore::Client_ClientsToWaitFor(int _iClient, int _iNotClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_ClientsToWaitFor, 0);
	uint32 ClientsToWaitFor = 0; // (wait for max 32 clients...)
	TAP<spCWServer_ClientInfo> pClients = m_lspClientInfo;

	for (int i = 0; i < pClients.Len(); i++)
	{
		if (i == _iNotClient)
			continue;

		if ((i == _iClient) || (_iClient == -1 && pClients[i] != NULL && pClients[i]->m_State == WCLIENT_STATE_INGAME))
			ClientsToWaitFor |= M_BitD(i);
	}

	return ClientsToWaitFor;
}

CWorld_Client* CWorld_ServerCore::Client_Get(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_Get, NULL);
	if (!m_lspClients.ValidPos(_iClient)) return NULL;
	return m_lspClients.GetBasePtr()[_iClient];
}

CWServer_ClientInfo* CWorld_ServerCore::Client_GetInfo(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetInfo, NULL);
	if (!m_lspClientInfo.ValidPos(_iClient)) return NULL;
	return m_lspClientInfo.GetBasePtr()[_iClient];
}

int CWorld_ServerCore::Client_Connect(int _hCon)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_Connect, 0);
	// Create client-mirror
//LogFile("	-Creating new client mirror");
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ClientClass);
	spCWorld_Client spC = safe_cast<CWorld_Client> ((CReferenceCount*)spObj);
	if (!spC) Error("Net_CheckPendingConnections", "Unable to create client-object.");

	CWC_CreateInfo CreateInfo;
	CreateInfo.m_Mode = WCLIENT_MODE_MIRROR;
	CreateInfo.m_spGameReg = m_spGameReg;
	CreateInfo.m_spWData = NULL;
	CreateInfo.m_spMapData = m_spMapData;

	spC->Create(CreateInfo);
	spCWServer_ClientInfo spCI = MNew(CWServer_ClientInfo);
	if (!spCI) MemError("Client_Connect");

	int iC = 0;
	for(iC = 0; iC < m_lspClients.Len(); iC++)
		if (!m_lspClients[iC]) break;

	if (iC == m_lspClients.Len())
	{
		iC = m_lspClients.Add(spC);
		int iCI = m_lspClientInfo.Add(spCI);
		if (iC != iCI) Error("Net_CheckPendingConnections", "Internal error.");
	}
	else
	{
		m_lspClients[iC] = spC;
		m_lspClientInfo[iC] = spCI;
	}

	spCI->m_hConnection = _hCon;
	spCI->m_lObjInfo.Clear();
	spCI->m_lObjInfo.SetLen(m_lspObjects.Len());

	Client_SetNumRegDirectories(iC, WCLIENT_REG_NUM);

	// Update cheats variable
	// Removed because the server registry is now read-only.
/*	if (!Registry_GetServer()->GetValuei("ALWAYSCHEATS", 0, 0))
	{
		int nClients = 0;
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL) nClients++;

		Registry_GetServer()->SetValuei("CHEATS", (nClients > 1) ? 0 : 1);
	}
*/

	Client_Init(iC);
	Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_ADDCLIENT, iC), WSERVER_GAMEOBJNAME);

	return iC;
}

int CWorld_ServerCore::Client_ConnectLocal(spCWorld_Client _spLocal)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_ConnectLocal, 0);
	int iC = Client_Connect(-1);
	m_lspClientInfo[iC]->m_spLocalClient = _spLocal;
	return iC;
}

int CWorld_ServerCore::Client_GetPlayer(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetPlayer, 0);
	if (!m_lspClientInfo.ValidPos(_iClient)) return -1;
	if (m_lspClientInfo[_iClient] == NULL) return -1;
	return m_lspClientInfo[_iClient]->m_iPlayer;	
}

void CWorld_ServerCore::Client_SetPlayer(int _iClient, int _iPlayer)
{
	if (!m_lspClientInfo.ValidPos(_iClient)) return;
	if (m_lspClientInfo[_iClient] == NULL) return;

	m_lspClientInfo[_iClient]->m_iPlayer = _iPlayer;
}

int CWorld_ServerCore::Client_GetPing(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Client_GetPing, 0);
	if (!m_lspClientInfo.ValidPos(_iClient)) return 0;
	if (m_lspClientInfo[_iClient] == NULL) return 0;
	return m_lspClientInfo[_iClient]->m_LastPing;	
}

CStr GetPacketInfo(int _Size, CNetMsg& _Packet);
CStr GetPacketInfo(int _Size, CNetMsg& _Packet)
{
	MAUTOSTRIP(GetPacketInfo, CStr());
	CStr s;
	switch(_Packet.m_MsgType)
	{
	case WPACKET_CONTROLS : s = "CONTROLS"; break;
	case WPACKET_FILEREQUEST : s = "FILEREQ"; break;
	case WPACKET_VOID : s = "VOID"; break;
	default : s = CStrF("UNKNOWN(%d)", _Packet.m_MsgType);
	}

	s += CStrF(", %d", _Size);
	return s;
}

