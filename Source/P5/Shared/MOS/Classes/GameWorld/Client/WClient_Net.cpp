
#include "PCH.h"

#include "WClient_Core.h"
#include "../WPhysState_Hash.h"
#include "../WPackets.h"


#ifdef ENABLE_CLIENTUPDATE_SIZECHECK
# define CLIENTUPDATE_CHECKSIZE(pObj, UnpackSize, PackSize)												\
	if (s_bNetDebugEnabled && UnpackSize != PackSize)																			\
	{																									\
		M_TRACEALWAYS("SEVERE ERROR: unpack size mismatch! (Class: %s)\n", pObj->m_pRTC->m_ClassName);	\
		M_BREAKPOINT;																					\
		UnpackSize = PackSize;																			\
	}
#else
# define CLIENTUPDATE_CHECKSIZE(pObj, UnpackSize, PackSize)
#endif


// -------------------------------------------------------------------
//  Network I/O for local client.
// -------------------------------------------------------------------
void CWorld_ClientCore::Local_InitQueues()
{
	MAUTOSTRIP(CWorld_ClientCore_Local_InitQueues, MAUTOSTRIP_VOID);
	int Buffer = m_pEnv->GetValuei("CL_LOCALMSGBUFFER", 16);
//LogFile("(CWorld_ClientCore::Local_InitQueues) " + GetClientInfo() + CStrF(", CL_LOCALMSGBUFFER=%d", Buffer));
	typedef TQueue<CNetMsgPack> TQueue_CNetMsgPack;
	m_spLocalInQueue = MNew1(TQueue_CNetMsgPack, Buffer);
	m_spLocalOutQueue = MNew1(TQueue_CNetMsgPack, Buffer);
}

bool CWorld_ClientCore::Local_OutQueueEmpty()
{
	MAUTOSTRIP(CWorld_ClientCore_Local_OutQueueEmpty, false);
	if (!m_spLocalOutQueue) return true;
	return m_spLocalOutQueue->Empty();
}

const CNetMsgPack* CWorld_ClientCore::Local_GetOutMsg()
{
	MAUTOSTRIP(CWorld_ClientCore_Local_GetOutMsg, NULL);
	if (!m_spLocalOutQueue) Error("Local_GetOutMsg", "No buffers.");
	return &m_spLocalOutQueue->Get();
}

bool CWorld_ClientCore::Local_PutInMsg(const CNetMsgPack& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Local_PutInMsg, false);
	if (!m_spLocalInQueue) Local_InitQueues();
//ConOutL(CStrF("CL %.8x, Local_PutInMsg %d", this, _Msg.m_Size));
	bool bOk = m_spLocalInQueue->Put(_Msg);
//	if (!bOk) 
//		ConOut("(Local_PutInMsg) Failed to add local message.");
	return bOk;
}

CCmdQueue* CWorld_ClientCore::Local_GetCmdQueue()
{
	return m_LocalPlayer.m_spCmdQueue;
}

void CWorld_ClientCore::Local_ClearCmdQueue()
{
	CCmdQueue* pQ = m_LocalPlayer.m_spCmdQueue;

	while(!pQ->Empty())	// This is retarded
		pQ->Get();

	m_LocalPlayer.m_spCmdQueue->m_LastAccumTime = 0;
	m_LocalPlayer.m_spCmdQueue->m_LastTmpAccumTime = 0;
}

// -------------------------------------------------------------------
//  Network game state update
// -------------------------------------------------------------------
void CWorld_ClientCore::Net_InitUpdateBuffers()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_InitUpdateBuffers, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Net_InitUpdateBuffers);
	if (!m_lspFrameIn.Len())
	{
		MSCOPE(CWorld_ClientCore::Net_InitUpdateBuffers, WORLD_CLIENT);
		m_lspFrameIn.SetLen(CWCLIENT_NFRAMEBUFFERS);
		for(int i = 0; i < m_lspFrameIn.Len(); i++)
		{
			m_lspFrameIn[i] = MNew(CWObjectUpdateBuffer);
			if(!m_lspFrameIn[i])
				MemError("Net_InitUpdateBuffers");
		}

		m_lFrameInTime.SetLen(m_lspFrameIn.Len());
		m_iFrameInHead = 0;
		m_iFrameInTail = 0;
//		m_LastFrameTime = CMTime::GetCPU();
		m_LastFrameTime.Snapshot();
		m_LastUnpackTime = m_LastFrameTime;
	}
}

void CWorld_ClientCore::Net_UnpackClientUpdate(CWObjectUpdateBuffer* _pDeltaBuffer, int _Flags)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_UnpackClientUpdate, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Net_UnpackClientUpdate);
	M_NAMEDEVENT("Net_UnpackClientUpdate", 0xff000000);

//	m_LastUnpackTime = CMTime::GetCPU();
	m_LastUnpackTime.Snapshot();

	// If this is a mirror we cannot interpolate or simulate.
	if (GetClientMode() == WCLIENT_MODE_MIRROR) 
		_Flags = 0;

	int nObj = m_lspObjects.Len();
	spCWObject_Client* lpClientObj = m_lspObjects.GetBasePtr();
	CWObject_Client NullObj;

	m_LocalPlayer.m_ServerControlSerial = _pDeltaBuffer->m_ControlSerial;

	// Remove old control-frames
	if (m_RecMode != WCLIENT_RECMODE_PLAYBACK)
	{
		MSCOPESHORT( OnClientPredict_1 );
		// OnClientPredict 1, Predict all cmds until m_ServerControlSerial
		if (m_ClientMode & WCLIENT_MODE_PRIMARY &&
			m_LocalPlayer.m_ServerControlSerial != 0x7fff)
		{
//			CWC_Player* pP = Player_GetLocal();
			int iObj = Player_GetLocalObject();
			CWObject_Client* pObj = Object_Get(iObj);
			if (pObj)
			{
				pObj->m_pRTC->m_pfnOnClientPredict(pObj, this, 1, _pDeltaBuffer->m_ControlSerial, 0);
			}
		}

		if (m_LocalPlayer.m_ServerControlSerial != 0x7fff)
		{
			m_LocalPlayer.m_spCmdQueue->DelTailCmd(_pDeltaBuffer->m_ControlSerial, 0);
			m_LocalPlayer.m_spCmdQueue->m_LastAccumTime = fp32(int16(_pDeltaBuffer->m_ControlAccumTime)) / 1000.0f;

//		ConOutL(CStrF("CLIENT UPDATE ID %d, ACCUM %f", _pDeltaBuffer->m_ControlSerial, m_LocalPlayer.m_spCmdQueue->m_LastAccumTime));
		}
		else
		{
//		ConOutL(CStrF("CLIENT UPDATE ID %d, NO PROGRESS", _pDeltaBuffer->m_ControlSerial, m_LocalPlayer.m_spCmdQueue->m_LastAccumTime, m_LocalPlayer.m_spCmdQueue->m_LastAccumTime - SERVER_TIMEPERFRAME));
		}
	}

//	if (m_bNetShowMsg && _pDeltaBuffer->m_SimulationTick - m_SimulationTick != 1)
//		ConOutL(CStrF("Tick %d, Delta %d", _pDeltaBuffer->m_SimulationTick, _pDeltaBuffer->m_SimulationTick - m_SimulationTick));

	if( _Flags & WCLIENT_UNPACKUPD_SIMULATE )
	{
		MSCOPESHORT( Simulate );
		int nSim = _pDeltaBuffer->m_nSimTicks;

		for (int i = 0; i < nSim; i++) 
			Simulate(true);
		
	//ConOut(CStrF("nCSim %d", nSim));

		if (nSim > 1 && m_bNetShowMsg) ConOutL(CStrF("nCSim %d", nSim));
	}

	m_SimulationTick = _pDeltaBuffer->m_SimulationTick;
	m_SimulationTime = CMTime::CreateFromTicks(m_SimulationTick, m_TickTime);

//CHECKMEMORY("Net_Refresh");
	int Pos = 0;
	int nChunks = 0; 
	uint8* pData = _pDeltaBuffer->GetObjectBuffer();
	while((Pos < _pDeltaBuffer->GetDataSize()) && (nChunks < _pDeltaBuffer->m_nObjects))
	{
#ifdef ENABLE_CLIENTUPDATE_SIZECHECK
		static bool s_bNetDebugEnabled = m_pEnv->GetValuei("NET_DEBUG", 1) != 0;
		uint PackSize = 0;
		if (s_bNetDebugEnabled)
		{
			PackSize = pData[Pos+0] + (pData[Pos+1] << 8);
			Pos += 2;
		}
#endif

		int iObj = CWObject_Client::GetObjectNr(&pData[Pos]);
//		uint8* pByte = NULL;
		if ((iObj < 0) || (iObj >= nObj))
		{
			ConOutL(CStrF("(CWorld_ClientCore::UnpackClientUpdate) %s, Invalid object-index, aborting update. (%d/%d)", (char*)GetClientInfo(), iObj, nObj));
			M_TRACEALWAYS("(CWorld_ClientCore::UnpackClientUpdate) %s, Invalid object-index, aborting update. (%d/%d)\n", (char*)GetClientInfo(), iObj, nObj);
			break;
		}
//LogFile(CStrF("(UCU) iObj %d, %d, %d/%d", iObj, Pos, nChunks, _pDeltaBuffer->m_nChunks));
		CWObject_Client* pObj = lpClientObj[iObj];

		int OldClass = (pObj) ? pObj->m_iClass : 0;
		int NewClass = CWObject_CoreData::GetUpdatedClass(OldClass, &pData[Pos]);

		if (NewClass)
		{
			int OnClientUpdateFlags = 0;

			if (!pObj)
			{
				MSCOPESHORT( new_CWObject_Client );
				pObj =  MNew(CWObject_Client);
				if (!pObj)
					MemError("Net_UnpackClientUpdate");
				m_lspObjects[iObj] = pObj;
				OnClientUpdateFlags |= CWO_ONCLIENTUPDATEFLAGS_DOLINK;
			}

			if(OldClass != NewClass)
			{
				pObj->m_lspClientObj[0] = NULL;
				pObj->m_pRTC = m_spMapData->GetResource_Class(NewClass);
			}
			
			pObj->m_iObject = iObj;
			pObj->m_iClass = NewClass;

			if (pObj->m_pRTC)
			{
				uint UnpackSize = pObj->m_pRTC->m_pfnOnClientUpdate(pObj, this, &pData[Pos], OnClientUpdateFlags);
				CLIENTUPDATE_CHECKSIZE(pObj, UnpackSize, PackSize);
				Pos += UnpackSize;
			}
			else
			{
	//			pClientObj[iObj].AddDeltaUpdate(&_pDeltaBuffer->m_Data[Pos]);
				M_TRACEALWAYS("SEVERE ERROR: Invalid object class, game-state cannot be unpacked. (Obj %d, iPrevClass %d, iNewClass %d)\n", iObj, OldClass, NewClass);
				ConOutL(CStrF("SEVERE ERROR: Invalid object class, game-state cannot be unpacked. (Obj %d, iPrevClass %d, iNewClass %d)", iObj, OldClass, NewClass));

				if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
				{
					m_iObjVisPool.Remove(iObj);
					if (m_spSpaceEnum) m_spSpaceEnum->Remove(iObj);
				}
				if (m_spSceneGraphInstance != NULL)
					m_spSceneGraphInstance->SceneGraph_UnlinkElement(iObj);
				pObj->Clear();
				Sound_UpdateObject(iObj);
				m_lspObjects[iObj] = NULL;

	//			pObj->Dump(m_spMapData, -1);
				return;
			}

			pObj->m_iObject = iObj;
			if (OnClientUpdateFlags & CWO_ONCLIENTUPDATEFLAGS_DOLINK)
				Object_Link(iObj);

			if (pObj->m_iClass != OldClass)
				pObj->m_LastPos = pObj->m_Pos;

			Sound_UpdateObject(iObj);
		}
		else
		{
			int Flags = 0;
			Pos += CWObject::OnClientUpdate(&NullObj, this, &pData[Pos], Flags);
			Object_Destroy(iObj);
		}

//		Pos += pClientObj[iObj].AddDeltaUpdate(iObj, &_pDeltaBuffer->m_Data[Pos]);
		nChunks++;
	}
//CHECKMEMORY("Net_Refresh");

	{
		MSCOPESHORT( OnClientPredict_2 );
		// OnClientPredict 2, Compare update with prediction
		if (m_ClientMode & WCLIENT_MODE_PRIMARY &&
			(m_RecMode != WCLIENT_RECMODE_PLAYBACK) &&
			m_LocalPlayer.m_ServerControlSerial != 0x7fff)
		{
//			CWC_Player* pP = Player_GetLocal();
			int iObj = Player_GetLocalObject();
			CWObject_Client* pObj = Object_Get(iObj);
			if (pObj)
			{
				M_NAMEDEVENT("OnClientPredict", 0xff000000);
				pObj->m_pRTC->m_pfnOnClientPredict(pObj, this, 2, _pDeltaBuffer->m_ControlSerial, 0);
			}
		}
	}


	const CRegistry* pReg = Registry_Get(0);
	if (pReg) 
	{
		if (m_RecMode != WCLIENT_RECMODE_PLAYBACK)
		{
			m_TimeScale = Max(0.01f, pReg->GetValuef("TIMESCALE", 1.0f));	// TimeScale can't be zero.
			m_TickRealTime = m_TickTime / m_TimeScale;
		}
	}
}

void CWorld_ClientCore::Net_UnpackGameFrame()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_UnpackGameFrame, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Net_UnpackGameFrame);
	if (m_iFrameInTail == m_iFrameInHead) return;

	// Unpacks tail and increments tail

	Net_UnpackClientUpdate(m_lspFrameIn[m_iFrameInTail]);

	m_lspFrameIn[m_iFrameInTail]->Clear();
	m_iFrameInTail = (m_iFrameInTail + 1) % m_lspFrameIn.Len();
}

void CWorld_ClientCore::Net_FlushGameFrames()
{
	while(m_iFrameInTail != m_iFrameInHead)
		Net_UnpackGameFrame();
}

void CWorld_ClientCore::Net_UnpackGameFrames()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_UnpackGameFrames, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Net_UnpackGameFrames);

	if (m_lspFrameIn.Len())
	{
		if (GetRecMode() == WCLIENT_RECMODE_PLAYBACK || (GetClientMode() & WCLIENT_MODE_LOCAL))
		{
			while(m_iFrameInTail != m_iFrameInHead)
			{
				Net_UnpackGameFrame();
			}
		}
		else
		{
//			CMTime Time = CMTime::GetCPU();
			CMTime Time;
			Time.Snapshot();
			fp32 dTime = Max(fp32(0.0f), Min(1.0f, ((Time - m_LastFrameTime).GetTime() * m_TimeScale)));
			fp32 LastLag = 0.0f;
			int bSlowdown = 0;

			fp32 TickTime = GetGameTickTime();
			while (dTime >= TickTime)
			{
				m_ClientUpdateMask |= WCLIENT_SERVERUPDATE_CONTROLS;

	//			if (m_hConnection >= 0)
				{
					if (m_iFrameInTail != m_iFrameInHead)
					{
						LastLag = (Time - m_lFrameInTime[m_iFrameInTail]).GetTime();

						/*
						CPixel32 Color = 0xff404040;
						if (dTime > 0.051f)
							Color = 0xff505000;
						else if (dTime < 0.049f)
							Color = 0xff004050;

						AddGraphPlot(WCLIENT_GRAPH_SOUNDTIME, dTime, Color);
*/

						Net_UnpackGameFrame();
					}
					else
					{
						bSlowdown = 1;

		//				LogFile(CStrF("Force slowdown. %f", m_LastFrameTime));
		//				m_LastFrameTime += fp64(SERVER_TIMEPERFRAME * 0.5f);
//						m_LastFrameTime = CMTime::GetCPU() - CMTime::CreateFromSeconds(SERVER_TIMEPERFRAME);
						m_LastFrameTime.Snapshot();
						m_LastFrameTime -= CMTime::CreateFromSeconds(TickTime);

		//				m_LastFrameTime = Time;
		//				m_LastFrameTime += fp64(SERVER_TIMEPERFRAME);
						break;
					}
				}

				Time = Time + CMTime::CreateFromSeconds(m_FramePeriod);
				dTime -= m_FramePeriod;
				m_LastFrameTime = m_LastFrameTime + CMTime::CreateFromSeconds(m_FramePeriod / m_TimeScale);
			}

			if (!bSlowdown)
			{
				if (m_iFrameInTail != m_iFrameInHead)
					m_FramePeriod = TickTime - 0.002f;
				else
					m_FramePeriod = TickTime - Min(0.002f, LastLag - 0.001f);
			}
			else
				m_FramePeriod = TickTime + 0.002f;

		}
//LogFile(CStrF("Lag %fms, Period %f", LastLag * 1000.0, m_FramePeriod));
//		m_LastFrameTime -= 0.001f;
	}
}


bool CWorld_ClientCore::Net_CreateServerRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_CreateServerRegUpdate, false);
/*	CWorld_Client* pWClient = m_lspClients[_iClient];
	CWServer_ClientInfo* pWClientInfo = m_lspClientInfo[_iClient];

	if (!_pDeltaBuffer)
		Error("CreateClientRegUpdate", "_pDeltaBuffer == NULL");
	_pDeltaBuffer->Clear();

	for(int i = 0; i < pWClientInfo->m_spNetReg.Len(); i++)
	{
		if (!PackDeltaRegistry(i, pWClientInfo->m_spNetReg[i], pWClient->m_spNetReg[i], _pDeltaBuffer)) return false;
	}
*/
	return true;
}

int UnpackDeltaRegistry_r(CRegistry* _pReg, const uint8* _pData, int _Len, int _RecurseDepth);

bool CWorld_ClientCore::Net_UnpackClientRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_UnpackClientRegUpdate, false);
	MSCOPESHORT(CWorld_ClientCore::Net_UnpackClientRegUpdate);
	M_NAMEDEVENT("Net_UnpackClientRegUpdate", 0xffff0000);

	if (_pDeltaBuffer->GetDataSize())
	{
		UnpackDeltaRegistry_r(m_spNetReg, _pDeltaBuffer->GetObjectBuffer(), _pDeltaBuffer->GetDataSize(), 0);
	}


	return true;
}

// -------------------------------------------------------------------
//  Network messaging.
// -------------------------------------------------------------------
bool CWorld_ClientCore::Net_PutMsg(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_PutMsg, false);
//	int hCon = m_hConnection;
//	if (hCon == -1) Error("Net_PutMsg", "Not a network-client.");

	if (!m_OutMsg.AddMsg(_Msg))
	{
		if (!Net_FlushMessages()) return false;
		if (!m_OutMsg.AddMsg(_Msg)) Error("Net_PutMsg", "Internal error.");
	}
	return true;
}

const CNetMsg* CWorld_ClientCore::Net_GetMsg()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_GetMsg, NULL);
//	int hCon = m_hConnection;
//	if (hCon == -1) Error("Net_GetMsg", "Not a network-client.");

	const CNetMsg* pMsg = m_InMsg.GetMsg();
	if (!pMsg)
	{
		m_InMsg.Clear();
		if (m_hConnection == -1)
		{
			if (!m_spLocalInQueue) return NULL;
			if (m_spLocalInQueue->Empty()) return NULL;
			m_InMsg = m_spLocalInQueue->Get();
		}
		else
		{
			if (m_spNetwork != NULL)
			{
				while (1)
				{
					CNetwork_Packet *pPacket = m_spNetwork->Connection_PacketReceive(m_hConnection);
					if (pPacket)
					{
						if (pPacket->m_Size > NET_MAXPACKETSIZE)
						{
							m_spNetwork->Connection_PacketRelease(m_hConnection, pPacket);
							ConOut("(CWorld_ClientCore::Net_GetMsg) Threw away packet that was bigger than NET_MAXPACKETSIZE.");
							continue;
						}
						/*
						int InPackets = m_spNetwork->Connection_GetQueuedInSize(m_hConnection);
						int OutPackets = m_spNetwork->Connection_GetQueuedInSize(m_hConnection);
						static CMTime NextUpdate = CMTime::GetCPU();
						CMTime Now = CMTime::GetCPU();

						if (Now.Compare(NextUpdate) > 0)
						{
							NextUpdate = Now + CMTime::CreateFromSeconds(2.0);
							M_TRACEALWAYS("0x%08x: In %d , Out %d \n", MRTC_SystemInfo::OS_GetProcessID(), InPackets, OutPackets);
						}*/

						m_InMsg.m_Size = pPacket->m_Size;
						memcpy(m_InMsg.m_MsgBuf, pPacket->m_pData, m_InMsg.m_Size);
						m_spNetwork->Connection_PacketRelease(m_hConnection, pPacket);
						break;
					}
					else
						break;
				}
			}
			else
				Error("Net_GetMsg", "Internal error: " + GetClientInfo());

		}
		pMsg = m_InMsg.GetMsg();
	}

	return pMsg;
}

bool CWorld_ClientCore::Net_FlushMessages()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_FlushMessages, false);
	if (!m_OutMsg.m_Size) return true;
	MSCOPE(CWorld_ClientCore::Net_FlushMessages, WORLD_CLIENT);

	if (m_hConnection == -1)
	{
		if (!m_spLocalOutQueue) Local_InitQueues();

		if (m_spLocalOutQueue->Put(m_OutMsg))
		{
			m_OutMsg.Clear();
			return true;
		}
		else
			return false;
	}
	else
	{
		CNetwork_Packet Packet;
		Packet.m_pData = m_OutMsg.m_MsgBuf;
		Packet.m_Size = m_OutMsg.m_Size;
		if (!m_spNetwork->Connection_PacketSend(m_hConnection, &Packet)) 
			return false;
		m_OutMsg.Clear();
	}
	return true;
}


bool CWorld_ClientCore::Net_FlushConnection()
{
	if (m_hConnection == -1)
	{
		return true;
	}
	else
	{
		m_spNetwork->Connection_Flush(m_hConnection);
		return true;
	}
}

static const char* Msg2Str(int _Msg)
{
	MAUTOSTRIP(Msg2Str, NULL);
	switch(_Msg)
	{
		case WPACKET_VOID : return "Void";
		case WPACKET_CONTROLS : return "Controls";
		case WPACKET_PLAYERINFO : return "PlayerInfo";
		case WPACKET_DELTAFRAME : return "DeltaFrame";
		case WPACKET_STATUSBARINFO : return "StatusBarInfo";
		case WPACKET_SOUND : return "Sound";

		case WPACKET_LARGESIZE : return "LargeSize";

		case WPACKET_WORLD : return "World";
		case WPACKET_FILEREQUEST : return "FileRequest";
		case WPACKET_FILEFRAGMENT : return "FileFragment";
		case WPACKET_COMMAND : return "Command";
		case WPACKET_RESOURCEID : return "ResourceID";
		case WPACKET_ENTERGAME : return "EnterGame";
		case WPACKET_DELTAREGISTRY : return "DeltaRegistry";
		default : return "Unknown";
	}
}


void CWorld_ClientCore::Net_OnMessage_Sound(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_Sound, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_Sound);
	// Parse msg: (WPACKET_SOUND)
	//		iSound		int16
	//		iObject		int16
	//		iMaterial	uint8
	//		AttnType	uint8
	//	(if AttnType != "off"):
	//			Volume		uint16
	//			Pos			VecInt16_Max32768
	//			(Vel)		VecInt16_Max32768   (only if attntype==2)
	int Pos = 0;
	int iSound = _Msg.GetInt16(Pos);
	int iObject = _Msg.GetInt16(Pos);
	uint8 iMaterial = (uint8)_Msg.GetInt8(Pos);
	int AttnType = _Msg.GetInt8(Pos);

	if (AttnType == WCLIENT_ATTENUATION_OFF)
	{
		Sound_Off(iObject, iSound, iMaterial);
		return;
	}

	fp32 Volume = fp32(_Msg.GetInt16(Pos)) * (1.0f / 2047.0f);
	CVec3Dfp32 SndPos = _Msg.GetVecInt16_Max32768(Pos);
	CVec3Dfp32 SndVec(0);
	fp32 Delay = 0.0f;

	int iChannel = 0;
	if (AttnType == WCLIENT_ATTENUATION_LRP)
	{
		SndVec = _Msg.GetVecInt16_Max32768(Pos);
		if(SndVec == CVec3Dfp32(0))
		{
			// ConOutL("(CWorld_ClientCore::Net_OnMessage_Sound) LRP sound with vector 0");
			return;
		}
	}
	else if (AttnType == WCLIENT_ATTENUATION_LOOP)
	{
		// Looping sound
		iChannel = WCLIENT_CHANNEL_SFXLOOPING;
		AttnType = WCLIENT_ATTENUATION_3D;
	}
	else if (AttnType == WCLIENT_ATTENUATION_2D_CUTSCENE)
	{
		iChannel = WCLIENT_CHANNEL_CUTSCENE;
		AttnType = WCLIENT_ATTENUATION_2D;
		Delay = _Msg.Getfp32(Pos);
	}
	else if (AttnType == WCLIENT_ATTENUATION_3D_OVERRIDE)
	{
		SndVec = _Msg.GetVecInt16_Max32768(Pos);
	}
	else if (AttnType == WCLIENT_ATTENUATION_FORCE_3D_LOOP)
		iChannel = WCLIENT_CHANNEL_SFXLOOPING;

	uint32 GroupID = _Msg.GetInt32(Pos);

	bool bLoop = (iChannel == WCLIENT_CHANNEL_SFXLOOPING);
	Sound_Play(iChannel, iObject, SndPos, iSound, AttnType, iMaterial, Volume, SndVec, Delay, bLoop, GroupID);
}


void CWorld_ClientCore::Net_OnMessage_World(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_World, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_World);
//	ConOutL("(CWorld_ClientCore::Net_OnMessage) " + GetClientInfo() + ", World."/*,*/ );

	int Pos = 0;
	int nObj = _Msg.GetInt16(Pos);
	int nRc = _Msg.GetInt16(Pos);
	int SpawnMask = _Msg.GetInt32(Pos);

	CStr WName;
	WName.Capture( (char*)&_Msg.m_Data[Pos], 32);

	int bIsMirror = m_ClientMode & WCLIENT_MODE_MIRROR;
	if (!bIsMirror)
	{
		m_spWData->Resource_AsyncCacheDisable(4);
		m_spWData->Resource_WorldOpenClient(WName);
	}

	// Init object-heap
	World_Close();
	World_InitClientExecuteObjects();

	World_Init(nObj, nRc, WName, SpawnMask);
	m_ClientState = WCLIENT_STATE_CHANGELEVEL;
	m_ClientUpdateMask |= WCLIENT_SERVERUPDATE_PLAYERINFO;
	m_Precache = 0;
	m_LastRcvRc = 0;

	// Zap any uncommited updates.
	m_lspFrameIn.Clear();
	m_lFrameInTime.Clear();
}

void CWorld_ClientCore::Net_OnMessage_ResourceID(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_ResourceID, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_ResourceID);
	if ((m_ClientState == WCLIENT_STATE_CHANGELEVEL) && (m_spMapData!=NULL))
	{
		int Pos = 0;
		int RcID = _Msg.GetInt16(Pos);
		int  RcClass = _Msg.GetInt8(Pos);
		CStr RcName = _Msg.GetStr(Pos);
		m_LastRcvRc = RcID;
		if (!m_spMapData->SetResource(RcID, RcName, RcClass))
		{
			if(RcID != 0)
				ConOutL(CStrF("§cf80WARNING: (CWorld_ClientCore::Net_OnMessage_ResourceID) Failed to set resource %d, Name %s, Class %d", RcID, (char*)RcName, RcClass));
		}
	}
}

void CWorld_ClientCore::Net_OnMessage_DoPrecache(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_DoPrecache, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_DoPrecache);
	if (m_ClientState != WCLIENT_STATE_CHANGELEVEL) return;

	if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys->GetEnvironment()->GetValue("rs_preload_textures", "1").Val_int() == 1)
			Precache_Init();
	}
	
	m_ClientState = WCLIENT_STATE_PRECACHE;
}

void CWorld_ClientCore::Net_OnMessage_EnterGame(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_EnterGame, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_EnterGame);
	if (m_ClientState != WCLIENT_STATE_CHANGELEVEL &&
	    m_ClientState != WCLIENT_STATE_PRECACHE) return;

/*	if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys->GetEnvironment()->GetValue("rs_preload_textures", "1").Val_int() == 1)
			Precache_Init();
	}*/
	
	m_ClientState = WCLIENT_STATE_INGAME;

//	m_LastFrameTime = CMTime::GetCPU();
	m_LastFrameTime.Snapshot();
	m_LastUnpackTime = m_LastFrameTime;
}

void CWorld_ClientCore::Net_OnMessage_DeltaFrame(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_DeltaFrame, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_DeltaFrame);
	Net_InitUpdateBuffers();

	//ConOutL(CStrF("(CWorld_ClientCore::Net_OnMessage) dFrame...  %d", _Msg.m_MsgSize));

	if (m_lspFrameIn[m_iFrameInHead]->AddPacket(_Msg))
	{
		if (m_bNetShowMsg) ConOutL(CStrF("(CWorld_ClientCore::Net_OnMessage) Frame complete, size %d", m_lspFrameIn[m_iFrameInHead]->GetSize() ));

		// ConOutL(CStrF("%10.3f, Recieved complete frame %d", GetCPUClock() / GetCPUFrequency(), m_lspFrameIn[m_iFrameInHead]->m_SimulationTick));


		// Force update if queue is full.
		int iFrameInHeadMod = (m_iFrameInHead + 1) % m_lspFrameIn.Len();
		if (iFrameInHeadMod == m_iFrameInTail)
			Net_UnpackGameFrame();

		//LogFile(CStrF("Upd %d", m_lspFrameIn[m_iFrameInHead]->m_Size));

//		m_lFrameInTime[m_iFrameInHead] = CMTime::GetCPU();
		m_lFrameInTime[m_iFrameInHead].Snapshot();
		m_iFrameInHead = iFrameInHeadMod;

		Net_UnpackGameFrames();

#ifdef NEVER
		//LogFile("	-Frame complete!");
		{
			m_iFrameInHead = iFrameInHeadMod;
			while(m_iFrameInTail != m_iFrameInHead)
			{
				if (m_bNetShowMsg) ConOutL(CStr("(CWorld_ClientCore::Net_OnMessage) Unpacking frame."));
				Net_UnpackClientUpdate(m_lspFrameIn[m_iFrameInTail]);

				fp64 Time = GetTime();

				if (!(GetClientMode() & WCLIENT_MODE_MIRROR))
				{
					fp64 Period = Time - m_LastFrameTime;
					Moderated(m_FramePeriod, Period, m_FramePeriodPrim, 384);

//					ConOut(CStrF("Period %.2f", m_FramePeriod));
				}

				m_LastFrameTime = Time;

				m_lspFrameIn[m_iFrameInTail]->Clear();
				m_iFrameInTail = (m_iFrameInTail + 1) % m_lspFrameIn.Len();
			}
		}
#endif
	}

//			ConOutL(CStrF("	-FrameSize %d, %d, %d", m_FrameOut.m_Size, m_FrameOut.m_nChunks, m_FrameOut.m_IOPos));

//			int Size = _Msg.m_Data[0] + (_Msg.m_Data[1] << 8);
//			int nChunks = _Msg.m_Data[2] + (_Msg.m_Data[3] << 8);
//			LogFile(CStrF("	-First package.  FrameSize %d, %d", Size, nChunks));
}

void CWorld_ClientCore::Net_OnMessage_DeltaRegistry(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_DeltaRegistry, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_DeltaRegistry);
	//LogFile(CStrF("(CWorld_ClientCore::Net_OnMessage) DeltaReg %d", _Msg.m_MsgSize));
	if (m_NetRegIn.AddPacket(_Msg))
	{
		// ConOut(CStrF("(CWorld_ClientCore::OnMessage) Registry update, size %d", m_NetRegIn.m_Size));
		//LogFile("	-Frame complete!");
		Net_UnpackClientRegUpdate(&m_NetRegIn);
		m_NetRegIn.Clear();
	}

	OnRegistryChange();
	//LogFile(CStrF("	-DeltaReg %d, %d, %d", m_NetRegIn.m_Size, m_NetRegIn.m_nChunks, m_NetRegIn.m_IOPos));

}

void CWorld_ClientCore::Net_OnMessage_Command(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_Command, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_Command);
	// This is probably too unsafe to go into the final.
	// Anyone with RCON have the power to unleach pure evil on the clients.
	CStr Cmd;
	Cmd.Capture((const char*)_Msg.m_Data, _Msg.m_MsgSize);

	if (m_bNetShowMsg) ConOutL(CStrF("%s, Command '%s'", (char*)GetClientInfo(), (char*)Cmd));

	M_TRY
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (pCon) pCon->ExecuteString(Cmd);
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
}

void CWorld_ClientCore::Net_OnMessage_ObjNetMsg(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_ObjNetMsg, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_ObjNetMsg);
	if (m_ClientState == WCLIENT_STATE_INGAME)
	{
		int Pos = 0;
		int MsgType = _Msg.GetInt8(Pos);
		int iObj = _Msg.GetInt16(Pos);
		int iClass = _Msg.GetInt16(Pos);

		CWObject_Client* pObj = Object_Get(iObj);
		if (pObj)
		{
			if (pObj->m_iClass == iClass)
			{
				CNetMsg Msg(MsgType);
				Msg.AddData((void*)&_Msg.m_Data[Pos], _Msg.m_MsgSize - Pos);
				pObj->m_pRTC->m_pfnOnClientNetMsg(pObj, this, Msg);
			}
			else
			{
				M_TRACE("(WARNING: NetMsg sent to %d, but class does not match. (%d != %d)\n", iObj, iClass, pObj->m_iClass);
			}
		}
	}
}

void CWorld_ClientCore::Net_OnMessage_NullObjNetMsg(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_NullObjNetMsg, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_NullObjNetMsg);
	if (m_ClientState == WCLIENT_STATE_INGAME && m_spMapData != NULL)
	{
		int Pos = 0;
		int MsgType = _Msg.GetInt8(Pos);
		int iClass = _Msg.GetInt16(Pos);
		CNetMsg Msg(MsgType);
		Msg.AddData((void*)&_Msg.m_Data[Pos], _Msg.m_MsgSize - Pos);

		MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);
		if (pRTC) pRTC->m_pfnOnClientNetMsg(NULL, this, Msg);
	}
}

void CWorld_ClientCore::Net_OnMessage_Pause(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_Pause, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_Pause);
	if (_Msg.m_MsgSize < 1)
		return;

	int Pos = 0;
	int Pause = _Msg.GetInt8(Pos);
	World_Pause(Pause != 0);
}

void CWorld_ClientCore::Net_OnMessage_SingleStep(const CNetMsg& _Msg)
{
	MSCOPESHORT(Net_OnMessage_SingleStep);
	
	if (m_ClientMode & WCLIENT_MODE_PAUSE)
	{
		CCmd Cmd; // force idle command
		Cmd.Create_Cmd0(255, int(GetGameTickTime() * 1000)); 
		AddCmd_Forced(Cmd);
		Net_SendControls(true);
	}
}

void CWorld_ClientCore::Net_OnMessage_LocalFrameFraction(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage_LocalFrameFraction, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage_LocalFrameFraction);
	if (_Msg.m_MsgSize != 4)
		return;

	int Pos = 0;
	fp32 Fraction = _Msg.Getfp32(Pos);
	m_LocalFrameFraction = Fraction;
}

void CWorld_ClientCore::Net_OnMessage_SoundSync(const CNetMsg& _Msg)
{
	// Got sync command from server - Unpause all voices in specified sync group and then delete the group.
	int Pos = 0;
	uint32 GroupID = _Msg.GetInt32(Pos);
	if (m_SoundSyncGroups.Find(GroupID) >= 0)
	{
		CWC_SoundSyncGroups::SGroup& Group = m_SoundSyncGroups.Get(GroupID);
		if (m_spSound)
		{
			TAP<int> pVoices = Group.m_lVoices;
			for (int i = 0; i < pVoices.Len(); i++)
				m_spSound->Voice_Unpause(pVoices[i]);
		}
		m_SoundSyncGroups.Remove(GroupID);
	}
}


int CWorld_ClientCore::Net_OnProcessMessage(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnProcessMessage, 0);
//	MSCOPE(CWorld_ClientCore::Net_OnProcessMessage, WORLD_CLIENT)
	switch(_Msg.m_MsgType)
	{
	// -------------------------------------------------------------------
	case WPACKET_SOUND :
		Net_OnMessage_Sound(_Msg);
		break;

	//case WPACKET_SOUND2 :
	//	Net_OnMessage_Sound2(_Msg);
	//	break;

	// -------------------------------------------------------------------
	case WPACKET_WORLD :
		Net_OnMessage_World(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_RESOURCEID :
		Net_OnMessage_ResourceID(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_DOPRECACHE :
		Net_OnMessage_DoPrecache(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_ENTERGAME :
		Net_OnMessage_EnterGame(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_STATUSBARINFO :
		{
			// Just ignore to get rid of error msg in demos.
		}
		break;

	// -------------------------------------------------------------------
	case WPACKET_DELTAFRAME :
		Net_OnMessage_DeltaFrame(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_DELTAREGISTRY :
		Net_OnMessage_DeltaRegistry(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_COMMAND :
		Net_OnMessage_Command(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_OBJNETMSG :
		Net_OnMessage_ObjNetMsg(_Msg);
		break;

	// -------------------------------------------------------------------
	case WPACKET_NULLOBJNETMSG :
		Net_OnMessage_NullObjNetMsg(_Msg);
		break;

	case WPACKET_PAUSE :
		Net_OnMessage_Pause(_Msg);
		break;

	case WPACKET_SINGLESTEP:
		Net_OnMessage_SingleStep(_Msg);
		break;

	case WPACKET_LOCALFRAMEFRACTION :
		Net_OnMessage_LocalFrameFraction(_Msg);
		break;

	case WPACKET_SOUNDSYNC: 	
		Net_OnMessage_SoundSync(_Msg);
		break;

	default :
		{
			return 0;
		}
		break;
	}
	return 1;
}

void CWorld_ClientCore::Net_OnMessage(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_OnMessage, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Net_OnMessage, WORLD_CLIENT);
	if (m_RecMode == WCLIENT_RECMODE_RECORDING)
		Demo_StoreMessage(_Msg, 0);

	m_NetLoadSecondTotal += _Msg.GetSize();
	m_NetLoadFrameTotal += _Msg.GetSize();

	if (m_bNetShowMsg)
		ConOutL(CStrF("%s, Msg %s, Msg %d", (char*)GetClientInfo(), Msg2Str(_Msg.m_MsgType), _Msg.m_MsgSize));

	if (!Net_OnProcessMessage(_Msg))
		ConOutL(CStrF("§cf80WARNING: Unprocessed network-msg. (Type %d, Size %d)", _Msg.m_MsgType, _Msg.m_MsgSize));
}

bool CWorld_ClientCore::Net_SetClientVar(CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_SetClientVar, false);
	CNetMsg Msg(WPACKET_SETVAR);
	Msg.AddInt16(0);
	Msg.AddStr(_Key.Ansi());
	Msg.AddStr(_Value.Ansi());

	if (Net_PutMsg(Msg))
	{
		m_spUserReg->SetValue(_Key, _Value);
		m_ClientUpdateMask |= WCLIENT_SERVERUPDATE_PLAYERINFO;
		OnRegistryChange();
		return true;
	}
	return false;
}

void CWorld_ClientCore::Net_AddIdleCommand()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_AddIdleCommand, MAUTOSTRIP_VOID);
	// This function is used to force a command to the player command queue so that
	// there's a full game-frame worth of input to send to the server.

	Con_Cmd(255);	// Adds a dummy cmd 255.
}


void CWorld_ClientCore::Net_SendClientVars()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_SendClientVars, MAUTOSTRIP_VOID);
	// Send playerinfo (client-vars nowdays) if anything have been changed.
	// This code kinda sucks and should be rewritten.

	if (m_ClientUpdateMask & WCLIENT_SERVERUPDATE_PLAYERINFO)
	{
		MSCOPE(CWorld_ClientCore::Net_SendClientVars, WORLD_CLIENT);

		CNetMsg Msg(WPACKET_PLAYERINFO);

		CRegistry* pUser = m_spUserReg;
		if (pUser)
		{
			for(int i = 0; i < pUser->GetNumChildren(); i++)
			{
				if (Msg.AddStr(pUser->GetName(i).Ansi()))
				{
					if (!Msg.AddStr(pUser->GetValue(i).Ansi()))
						ConOutL("§cf80WARNING: Too many client user variables.");
				}
				else
					ConOutL("§cf80WARNING: Too many client user variables.");
			}
		}

		if (Net_PutMsg(Msg))
			m_ClientUpdateMask &= ~WCLIENT_SERVERUPDATE_PLAYERINFO;
	}
}

void CWorld_ClientCore::Net_SendControls(bool _bForced)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_SendControls, MAUTOSTRIP_VOID);
	if ((m_ClientMode & WCLIENT_MODE_PAUSE) && !_bForced)
		return;

	MSCOPE(CWorld_ClientCore::Net_SendControls, WORLD_CLIENT);
	
	// Send controls...
	{
//		CMTime T = CMTime::GetCPU();
		CMTime T;
		T.Snapshot();
		CControlFrame Ctrl;
		if (!m_LocalPlayer.m_spCmdQueue->GetSendFrame(Ctrl, GetGameTickTime()))
		{
			if (((T - m_LocalPlayer.m_spCmdQueue->GetLastSendTime()).GetTime() > GetGameTickTime()*0.5f))
			{
				Net_AddIdleCommand();
				m_LocalPlayer.m_spCmdQueue->GetSendFrame(Ctrl, GetGameTickTime());
			}
		}

		if (Ctrl.m_MsgSize)
		{
			if (!Net_PutMsg(Ctrl))
			{
//						ConOut("§cf80WARNING: Could not send controls.");
			}
			else
				m_LocalPlayer.m_spCmdQueue->SetLastSendTime(T);
		}
		else
		{

		}
	}
}


void CWorld_ClientCore::Net_SetConnection(spCNetwork _spNetwork, int _hConnection)
{
	MAUTOSTRIP(CWorld_ClientCore_Net_SetConnection, MAUTOSTRIP_VOID);
	m_spNetwork = _spNetwork;
	m_hConnection = _hConnection;

	if (m_hConnection < 0 && m_spNetwork == NULL)
		Error("Net_SetConnection", "Internal error.");
}

int CWorld_ClientCore::Net_GetConnection()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_GetConnection, 0);
	return m_hConnection;
}

uint32 CWorld_ClientCore::Net_GetConnectionStatus()
{
	MAUTOSTRIP(CWorld_ClientCore_Net_GetConnectionStatus, 0);
	if (!m_spNetwork) return 0;
	return m_spNetwork->Connection_Status(m_hConnection);
}

