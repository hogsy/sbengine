
#include "PCH.h"

#include "WClient_Core.h"
#include "../WPackets.h"

// -------------------------------------------------------------------
// Demo recording/playback
// -------------------------------------------------------------------
#define WCLIENT_DEMOVERSION	0x0101

void CWorld_ClientCore::Demo_Start(CStr _FileName)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_Start, MAUTOSTRIP_VOID);
	if (!(m_ClientState & WCLIENT_STATE_INGAME))
		Error("Demo_Start", "No game is running.");

	CStr Path = _FileName.GetPath();
	if (Path != "")
		if (!CDiskUtil::CreatePath(Path))
		{
			ConOut("Unable to create path "+ Path);
			return;
		}

	m_RecordingName = _FileName;
	m_RecFile.Create(m_RecordingName);
	m_RecMode = WCLIENT_RECMODE_RECORDING;
	m_RecNumMsg = 0;

	M_TRY
	{
		m_RecFile.BeginEntry("DEMO");
		m_RecFile.EndEntry(WCLIENT_DEMOVERSION);
		m_RecFile.BeginSubDir();

		m_spMapData->Write(&m_RecFile);
		Demo_WriteInitialState(&m_RecFile);
	}
	M_CATCH(
	catch(CCException)
	{
		m_RecMode = WCLIENT_RECMODE_NORMAL;
		try { m_RecFile.Close(); } catch(...) {};
		CDiskUtil::DelFile(m_RecordingName);
	}
	)
	ConOut("Recording started. (" + _FileName + ")");

	m_RecFile.BeginEntry("MESSAGESTREAM");

	// Add player-info to begining of message-stream
	CWC_Player Dummy;
	{
		CNetMsg Msg(WPACKET_PLAYERINFO);
		Msg.AddInt8(0);
		Msg.AddInt8((int8)255);
		int16 iLocalPlayer = -1;
		Msg.AddInt16(iLocalPlayer);
		m_LocalPlayer.DeltaAddToPacket(Msg, &Dummy);
		Demo_StoreMessage(Msg, 0);
	}


}

void CWorld_ClientCore::Demo_Stop()
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_Stop, MAUTOSTRIP_VOID);
	if (m_RecMode == WCLIENT_RECMODE_RECORDING)
	{
		m_RecMode = WCLIENT_RECMODE_NORMAL;
		m_RecFile.EndEntry(m_RecNumMsg);
		m_RecFile.EndSubDir();
		m_RecFile.Close();
		ConOut("Recording ended.");
	}
}

#define INIT_PAD(n) uint8 Pad[n]; FillChar(&Pad, n, 0xaa); const int nPad = n;

#define CHECK_PAD(Name)	\
	LogFile( CStrF("CHECK_PAD: %s", (char*) #Name) );	\
	{ for(int i = 0; i < nPad; i++) if (Pad[i] != 0xaa) LogFile( CStrF("Pad failure at pos %.8x, address %.8x, Value %.2x", i, &Pad[i], Pad[i]) );	} \
	LogFile( "CHECK_PAD: done." );

void CWorld_ClientCore::Demo_Get(CStr _FileName)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_Get, MAUTOSTRIP_VOID);
	ConOutL("(CWorld_ClientCore::Demo_Get) " + _FileName);

	CDataFile DFile;
	DFile.Open(_FileName);
	if (!DFile.GetNext("DEMO")) Error("Demo_Get", "Not a valid demo-file.");
	if (DFile.GetUserData() != WCLIENT_DEMOVERSION) Error("Demo_Get", CStrF("Wrong demo-file version (%.4x should be %.4x)", DFile.GetUserData(), WCLIENT_DEMOVERSION));
	if (!DFile.GetSubDir()) Error("Demo_Get", "Not a valid demo-file.");

	World_Close();

	uint32 SimulationTick;
	// -------------------------------------------------------------------
	//  CLIENT
	DFile.PushPosition();
	if (!DFile.GetNext("CLIENT")) Error("Demo_ReadInitialState", "Corrupt demo-file.");
	CCFile* pF = DFile.GetFile();
	pF->ReadLE(SimulationTick);
	CStr WorldName;
	WorldName.Read(pF);
	int32 nObj; pF->ReadLE(nObj);
	int32 nClObj; pF->ReadLE(nClObj);

	m_spWData->Resource_AsyncCacheDisable(2);
	m_spWData->Resource_WorldOpenServer(WorldName.GetFilenameNoExt());

	DFile.PopPosition();

	spCMapData spMap = MNew(CMapData);
	if (!spMap) MemError("Demo_Get");
	spMap->Create(m_spWData);
	spMap->Read(&DFile);
	m_spMapData = spMap;
	Demo_ReadInitialState(&DFile);
	Demo_ReadMsgStream(&DFile);

	m_RecMode = WCLIENT_RECMODE_PLAYBACK;
	m_ClientState = WCLIENT_STATE_INGAME;
	DFile.Close();

	m_spWData->Resource_AsyncCacheDisable(4);
	m_spWData->Resource_AsyncCacheEnable(2);
	m_spWData->Resource_WorldOpenClient(m_WorldName);
}

int CWorld_ClientCore::Demo_PlayTick()
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_PlayTick, 0);
	int nPolled = 0;
	if (m_RecMode == WCLIENT_RECMODE_PLAYBACK)
	{
		m_iFrameInHead = 0;
		m_iFrameInTail = 0;
		while(m_RecStream.MsgAvail())
		{
			nPolled++;
			const CNetMsg* pMsg = m_RecStream.GetNextMsg();
			Net_OnMessage(*pMsg);
			if (m_iFrameInHead) break;
		}
	}

	m_FramePeriod = GetGameTickTime();

	return (nPolled != 0);
}

// -------------------------------------------------------------------
void CWorld_ClientCore::Demo_ReadInitialState(CDataFile* _pDFile)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_ReadInitialState, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	uint32 SimulationTick;
	// -------------------------------------------------------------------
	//  CLIENT
	CStr WorldName;
	{
		if (!_pDFile->GetNext("CLIENT")) Error("Demo_ReadInitialState", "Corrupt demo-file.");
		pF->ReadLE(SimulationTick);
		WorldName.Read(pF);
		int32 nObj; pF->ReadLE(nObj);
		int32 nClObj; pF->ReadLE(nClObj);
		World_Init(nObj, -1, WorldName, 0); // TODO: fix this spawnmask -kma
		World_InitClientExecuteObjects();
	}

	// -------------------------------------------------------------------
	//  REGISTRY
	{
		_pDFile->PushPosition();

			if (!_pDFile->GetNext("REGISTRY")) Error("Demo_ReadInitialState", "Corrupt demo-file.");
			m_spNetReg = REGISTRY_CREATE;
			m_spNetReg->Read(_pDFile->GetFile(), _pDFile->GetUserData2());

/*			m_lspNetReg.Clear();
			m_lspNetReg.SetLen(_pDFile->GetUserData());
			for(int i = 0; i < m_lspNetReg.Len(); i++)
			{
				m_lspNetReg[i] = REGISTRY_CREATE;
				if (!m_lspNetReg[i]) MemError("Demo_ReadInitialState");
				m_lspNetReg[i]->Read(_pDFile->GetFile());
			}*/

		_pDFile->PopPosition();
	}

	// -------------------------------------------------------------------
	//  OBJECTS
	{
		_pDFile->PushPosition();
			if (!_pDFile->GetNext("OBJECTS")) Error("Demo_ReadInitialState", "Corrupt demo-file.");

		
			for(int i = 0; i < _pDFile->GetUserData(); i++)
			{
				int32 iObj; pF->ReadLE(iObj);
				int32 iClass; pF->ReadLE(iClass);

				if (iClass)
				{
					m_lspObjects[iObj] = MNew(CWObject_Client);
					if (!m_lspObjects[iObj])
						MemError("Demo_ReadInitialState");

					MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);
					if (!pRTC) Error("Demo_ReadInitialState", "Invalid class found.");

					m_lspObjects[iObj]->m_pRTC = pRTC;
					pRTC->m_pfnOnInitClientObjects(m_lspObjects[iObj], this);

					int32 dPos = -pF->Pos();
					pRTC->m_pfnOnClientLoad(m_lspObjects[iObj], this, pF, m_spMapData, 0);
					dPos += pF->Pos();
					int32 Size; pF->ReadLE(Size);

					if (dPos != Size)
						Error("Demo_ReadInitialState", CStrF("Corrupt object. (Obj %d, Class %d, ClassName %s, Size %d != %d)", iObj, iClass, pRTC->m_ClassName, dPos, Size));
				}
			}
	ConOutL(CStrF("(CWorld_ClientCore::Demo_ReadInitialState) nObjects %dd", _pDFile->GetUserData()));
		_pDFile->PopPosition();
	}

	// -------------------------------------------------------------------
	//  CLIENT-EXECUTE OBJECTS
	{
		World_InitClientExecuteObjects();
		_pDFile->PushPosition();

			if (!_pDFile->GetNext("CLIENTOBJECTS")) Error("Demo_ReadInitialState", "Corrupt demo-file.");

			int i;
			for(i = 0; i < _pDFile->GetUserData(); i++)
			{
				int32 iObj; pF->ReadLE(iObj);
				int32 iClass; pF->ReadLE(iClass);

				if (iClass)
				{
					m_lspClientObjects[iObj] = MNew(CWObject_ClientExecute);
					if (!m_lspClientObjects[iObj])
						MemError("Demo_ReadInitialState");

					m_lspClientObjects[iObj]->Read(pF);
					
					MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);
					if (!pRTC) Error("Demo_ReadInitialState", "Invalid class found.");

					m_lspClientObjects[iObj]->m_pRTC = pRTC;
				}
			}

			if(i == _pDFile->GetUserData())
			{
				m_spClientObjectHeap = MNew(CIDHeap);
				m_spClientObjectHeap->Read(pF);
			}
			else
				m_spClientObjectHeap = MNew1(CIDHeap, 512);

	ConOutL(CStrF("(CWorld_ClientCore::Demo_ReadInitialState) nClObjects %d", _pDFile->GetUserData()));
		_pDFile->PopPosition();
	}

	m_SimulationTick = SimulationTick;
	m_SimulationTime = CMTime::CreateFromTicks(m_SimulationTick, GetGameTickTime());
	World_Init(-1, -1, WorldName, 0); // TODO: fix this spawnmask -kma
}

void CWorld_ClientCore::Demo_WriteInitialState(CDataFile* _pDFile)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_WriteInitialState, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	// -------------------------------------------------------------------
	//  CLIENT
	{
		_pDFile->BeginEntry("CLIENT");
		pF->WriteLE(uint32(m_SimulationTick));
		m_WorldName.Write(pF);
		pF->WriteLE(int32(m_lspObjects.Len()));
		pF->WriteLE(int32(m_lspClientObjects.Len()));
		_pDFile->EndEntry(0);
	}

	// -------------------------------------------------------------------
	//  REGISTRY
	{
		_pDFile->BeginEntry("REGISTRY");
		m_spNetReg->Write(_pDFile->GetFile());

//		for(int i = 0; i < m_lspNetReg.Len(); i++)
//			m_lspNetReg[i]->Write(_pDFile->GetFile());
		_pDFile->EndEntry(m_spNetReg->GetNumChildren(), REGISTRY_BINARY_VERSION);
	}

	// -------------------------------------------------------------------
	//  OBJECTS
	{
		_pDFile->BeginEntry("OBJECTS");
		int nStored = 0;
		for(int i = 0; i < m_lspObjects.Len(); i++)
		{
			CWObject_Client* pObj = m_lspObjects[i];
			if (pObj && pObj->m_iClass)
			{
				pF->WriteLE(int32(i));
				pF->WriteLE(int32(pObj->m_iClass));

				MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(pObj->m_iClass);
				if (!pRTC) Error("Write_ReadInitialState", "Invalid class found.");

				int32 dPos = -_pDFile->GetEntryPos();
				pRTC->m_pfnOnClientSave(pObj, this, pF, m_spMapData, 0);
				dPos += _pDFile->GetEntryPos();
				pF->WriteLE(dPos);

				nStored++;
			}
		}
		_pDFile->EndEntry(nStored);
	}

	// -------------------------------------------------------------------
	//  CLIENT-EXECUTE OBJECTS
	{
		_pDFile->BeginEntry("CLIENTOBJECTS");
		int nStored = 0;
		for(int i = 0; i < m_lspClientObjects.Len(); i++) 
		{
			CWObject_ClientExecute* pObj = m_lspClientObjects[i];
			if (pObj && pObj->m_iClass)
			{
				pF->WriteLE(int32(i));
				pF->WriteLE(int32(pObj->m_iClass));

				pObj->Write(pF);
				nStored++;
			}
		}
		m_spClientObjectHeap->Write(pF);
		_pDFile->EndEntry(nStored);
	}
}

void CWorld_ClientCore::Demo_ReadMsgStream(CDataFile* _pDFile)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_ReadMsgStream, MAUTOSTRIP_VOID);
	m_RecStream.Read(_pDFile);
}

void CWorld_ClientCore::Demo_StoreMessage(const CNetMsg& _Msg, fp32 _TimeStamp)
{
	MAUTOSTRIP(CWorld_ClientCore_Demo_StoreMessage, MAUTOSTRIP_VOID);
	m_RecFile.GetFile()->WriteLE(_TimeStamp);
	_Msg.Write(m_RecFile.GetFile());
	m_RecNumMsg++;
}

