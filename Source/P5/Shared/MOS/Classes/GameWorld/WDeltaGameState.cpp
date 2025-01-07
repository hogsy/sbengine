#include "PCH.h"
#include "WDeltaGameState.h"
#include "Server/WServer_Core.h"
#include "WObjects/WObj_Game.h"
#include "WObjects/WObj_System.h"
#include "../GameContext/WGameContext.h"

//#define VERBOSE

#define VERBOSE_OUT(_Msg)
#if defined(VERBOSE) && (defined(_DEBUG) || defined(M_MIXEDDEBUG))
	#undef VERBOSE_OUT
	#define VERBOSE_OUT(_Msg) M_TRACE(_Msg + "\n")
#endif


CWorld_DeltaGameState::CWorld_DeltaGameState()
{
	m_bChangeLevel = false;
	m_bHasInsertedGUID = false;
}


CWorld_DeltaGameState::~CWorld_DeltaGameState()
{
  	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return;

	pGame->m_spAsyncSaveContext = NULL;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Called when we need to change level
\*____________________________________________________________________*/
void CWorld_DeltaGameState::ChangeLevel(CWorld_ServerCore* _pCore, const char* _pNewLevel, int _Flags)
{
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return;

	pGame->m_spAsyncSaveContext = NULL;
	RefreshBlockedWorldMessages();

	m_bChangeLevel = true;
	CaptureLevel(_pCore);
	CapturePendingPlayers(_pCore);
	LoadLevel(_pCore, _pNewLevel, _Flags);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Captures the current Player and Global state
\*____________________________________________________________________*/
void CWorld_DeltaGameState::CaptureSaveInfo(CWorld_ServerCore* _pCore, TArray<uint8>& _lData)
{
	TArray<uint8> lTempData;
	spCCFile spFileMem = CDiskUtil::CreateCCFile(lTempData, CFILE_READ | CFILE_BINARY);
	CDataFile DataFile;
	DataFile.Create(spFileMem);
	CCFile *pF = DataFile.GetFile();

	VERBOSE_OUT(CStrF("[WDeltaGameState] Writing 'SAVEINFO':\n- WorldName: %s\n- GameTick: %d\n- NextGUID: 0x%08X\n- NextConstraintID: %d", 
		_pCore->m_WorldName.Str(), _pCore->GetGameTick(), _pCore->m_NextGUID, _pCore->m_NextConstraintID));

	DataFile.BeginEntry("SAVEINFO");
	pF->Writeln(_pCore->m_WorldName);
	int32 SimulationTick = _pCore->GetGameTick();
	pF->WriteLE(SimulationTick);
	pF->WriteLE(_pCore->m_NextGUID);
	pF->WriteLE(uint32(_pCore->m_NextConstraintID));
	DataFile.EndEntry(0);

	CapturePendingPlayers(_pCore);
	DataFile.BeginEntry("PLAYERS");
	for(int i = 0; i < m_lPendingPlayers.Len(); i++)
	{
		m_lPendingPlayers[i].m_Position.Write(pF);
		m_lPendingPlayers[i].m_SpecialClass.Write(pF);
		pF->WriteLE(m_lPendingPlayers[i].m_Flags);
		uint16 Len = m_lPendingPlayers[i].m_lData.Len();
		pF->WriteLE(Len);
		pF->Write(m_lPendingPlayers[i].m_lData.GetBasePtr(), Len);
	}
	DataFile.EndEntry(m_lPendingPlayers.Len());

	DataFile.BeginEntry("PRECACHEINFO");
	int nPrecacheInfo = m_lPrecacheInfoNames.Len();
	for(int i = 0; i < nPrecacheInfo; i++)
	{
		pF->WriteLE(m_lPrecacheInfoTypes[i]);
		m_lPrecacheInfoNames[i].Write(pF);
	}
	DataFile.EndEntry(nPrecacheInfo);

	VERBOSE_OUT(CStrF("[WDeltaGameState] Writing 'SPAWNFLAGS': 0x%08X", _pCore->m_CurSpawnMask));
	DataFile.BeginEntry("SPAWNFLAGS");			// Store global spawn flags
	DataFile.EndEntry(_pCore->m_CurSpawnMask);

	ClearPendingPlayers();

	DataFile.Close();

//	_lData = CDiskUtil::Compress( lTempData, LZSS_HUFFMAN );
	_lData = lTempData;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Checks with all players so we can save
\*____________________________________________________________________*/
bool CWorld_DeltaGameState::CanSave(CWorld_ServerCore *_pCore)
{
	// Checks with players so we can save
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pCore->Selection_AddClass(Selection, "CharPlayer");
	const int16 *lpSel;
	int nSel = _pCore->Selection_Get(Selection, &lpSel);
//	uint8 nPlayers = 0;
	bool bStatus = true;
	bool bHasPlayer = false;
	CWObject_Message Msg(OBJSYSMSG_PLAYER_CANSAVE);
	for(int32 i = 0; i < nSel; i++)
	{
		CWObject *pObj = _pCore->Object_Get(lpSel[i]);
		if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
		{
			bHasPlayer = true;
			if (!_pCore->Message_SendToObject(Msg,pObj->m_iObject))
			{
				bStatus = false;
				break;
			}
		}
	}

	return bStatus && bHasPlayer;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Captures the current Level state
\*____________________________________________________________________*/
void CWorld_DeltaGameState::CaptureLevel(CWorld_ServerCore *_pCore)
{
	CStr Level = _pCore->m_WorldName.GetFilenameNoExt();
	int i = 0;
	for(i = 0; i < m_lDeltaWorldData.Len(); i++)
		if(m_lDeltaWorldData[i].m_Level.CompareNoCase(Level) == 0)
			break;

	if(i == m_lDeltaWorldData.Len())
	{
		m_lDeltaWorldData.Add(CDeltaWorldData());
		m_lDeltaWorldData[i].m_Level = Level;
	}

	m_lDeltaWorldData[i].m_lData = CreateDeltaSave(_pCore);
	m_lDeltaWorldData[i].m_DataSize = m_lDeltaWorldData[i].m_lData.Len();
	m_lDeltaWorldData[i].m_LocalSpawnFlags = (_pCore->m_CurSpawnMask & SERVER_SPAWNFLAGS_LOCAL);
	_pCore->GetMapData()->GetIndexMap( m_lDeltaWorldData[i].m_lResourceMappings );
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Called when a savegame is to be saved to medium
\*____________________________________________________________________*/
bool CWorld_DeltaGameState::SaveGame(CWorld_ServerCore *_pCore, CStr _SaveName, bool _bDotest)
{
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return false;

	if(_bDotest && !CanSave(_pCore))
	{
		pGame->m_SaveInfoString = "§LMENU_SAVEDISABLED";
		pGame->m_LastWriteTime = CMTime::GetCPU() + CMTime::CreateFromSeconds(2.0f);
		return false;
	}

	pGame->m_spAsyncSaveContext = NULL;
	RefreshBlockedWorldMessages();

	pGame->m_spAsyncSaveContext = MNew1(CGameContext::CSaveContext, pGame->m_spWData);

	m_lLastSaveInfo.Clear();
	CaptureSaveInfo(_pCore, m_lLastSaveInfo);
	CaptureLevel(_pCore);

	for(int nSaves = 0; nSaves < 2; nSaves++)
	{
		if(nSaves == 1)
		{
			if(_SaveName == "Checkpoint" || _SaveName == "Quick")
				break;
			else
				_SaveName = "Checkpoint";
		}
	
		if(!pGame->BeginWriteSaveFile("", _SaveName))
			return false;

		pGame->m_SaveInfoString = "§LMENU_SAVINGCHECKPOINT";

		m_SaveInfoSize = m_lLastSaveInfo.Len();
		pGame->WriteSaveFileBlock((uint8 *)&m_SaveInfoSize, sizeof(int32));
		int AlignedLen = (m_SaveInfoSize+3)/4*4; // this will pad to align to 4
		m_lLastSaveInfo.SetLen(AlignedLen);
		pGame->WriteSaveFileBlock(m_lLastSaveInfo.GetBasePtr(), AlignedLen);
		m_lLastSaveInfo.QuickSetLen(m_SaveInfoSize); // Makes sure the array is not realocated, and contains the aligned bytes needed

		m_nDeltaWorldData = m_lDeltaWorldData.Len();
		pGame->WriteSaveFileBlock((uint8 *)&m_nDeltaWorldData, sizeof(int32));

		for(int i = 0; i < m_lDeltaWorldData.Len(); i++)
		{
			CDeltaWorldData& DeltaWorldData = m_lDeltaWorldData[i];

			int Len = DeltaWorldData.m_lData.Len();
			DeltaWorldData.m_DataSize = Len;
			pGame->WriteSaveFileBlock((const uint8 *)DeltaWorldData.m_Level.Str(), ((DeltaWorldData.m_Level.Len() + 1) + 3)/4*4); // null termination, this will pad to align to 4
			pGame->WriteSaveFileBlock((uint8 *)&DeltaWorldData.m_DataSize, sizeof(int32));

			int AlignedLen = (Len+3)/4*4;
			DeltaWorldData.m_lData.SetLen(AlignedLen);
			pGame->WriteSaveFileBlock(DeltaWorldData.m_lData.GetBasePtr(), AlignedLen);
			DeltaWorldData.m_lData.QuickSetLen(Len); // Makes sure the array is not realocated, and contains the aligned bytes needed

			pGame->WriteSaveFileBlock((uint8 *)&DeltaWorldData.m_LocalSpawnFlags, sizeof(uint32));
		}

		pGame->EndWriteSaveFile(false);
	}

	pGame->FillProfile("");


	//pGame->WriteSaveFile("", _SaveName, SaveInfo);
	/*
	pGame->WriteTranscript(_pCore->m_spWData, CompleteName, lFiles);

#ifdef PLATFORM_XBOX
	// copy the dashboard icon
	CopyFile("d:\\xbox\\saveimage.xbx", pGame->GetSavePath(_pCore->m_spWData, CompleteName)+"\\saveimage.xbx", FALSE);
#endif*/

	/*
	try
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CStr Profile = pSys->GetOptions()->GetValue("GAME_PROFILE");
		if(Profile == "")
			Profile = "debug";
		CStr CompleteName = _SaveName + " - " + Profile;
		TArray<CStr> lFiles;

	#if defined(PLATFORM_XBOX)
	#if defined(M_DEMO_XBOX)
		const char *pSaveDrv = "Z:\\Saves\\";
	#else
		const char *pSaveDrv = "U:\\";
	#endif
		XDeleteSaveGame(pSaveDrv, CompleteName.Unicode().StrW());
	#elif defined( PLATFORM_PS2 )
		#warning "Implement CWorld_DeltaGameState::SaveGame"
		return;
	#elif defined( PLATFORM_DOLPHIN )
	#else
		CStr Path = _pCore->m_spWData->ResolvePath("Save\\") + CompleteName + "\\";
		CDiskUtil::DelTree(Path);
		CDiskUtil::CreatePath(Path);
	#endif
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		if(!pGame)
			return;

		{
			m_lLastSaveInfo.Clear();
			CaptureSaveInfo(_pCore, m_lLastSaveInfo);
	#ifndef M_DEMO
			pGame->WriteSaveFile(_pCore->m_spWData, CompleteName, "Save.xsg", m_lLastSaveInfo, true);
			lFiles.Add("Save.xsg");
	#endif
		}

	

		CaptureLevel(_pCore);
	#ifndef M_DEMO
		for(int i = 0; i < m_lDeltaWorldData.Len(); i++)
		{
			TArray<uint8> LevelInfo;
			m_lDeltaWorldData[i].m_lData.Duplicate(&LevelInfo);
			pGame->WriteSaveFile(_pCore->m_spWData, CompleteName, m_lDeltaWorldData[i].m_Level + ".xws", LevelInfo, true);
			lFiles.Add(m_lDeltaWorldData[i].m_Level + ".xws");
		}
		pGame->WriteTranscript(_pCore->m_spWData, CompleteName, lFiles);

		#ifdef PLATFORM_XBOX
			// copy the dashboard icon
			CopyFile("d:\\xbox\\saveimage.xbx", pGame->GetSavePath(_pCore->m_spWData, CompleteName)+"\\saveimage.xbx", FALSE);
		#endif
	#endif
	}
	catch (CCException)
	{
	}
	*/

	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Called when a savegame is to be loaded from medium
\*____________________________________________________________________*/
void CWorld_DeltaGameState::LoadGame(CWorld_ServerCore *_pCore, CStr _SaveName)
{
	m_bChangeLevel = false;
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return;

	pGame->m_spAsyncSaveContext = NULL;
	RefreshBlockedWorldMessages();

	if(!pGame->BeginReadSaveFile("", _SaveName))
		return;

	int32 Len;

	
	// 
	CStr Level;
	uint32 NextGUID, NextConstraintID;
	{
		// read data
		m_lLastSaveInfo.Clear();
		pGame->m_CurrentSaveFile.Read(&Len, sizeof(int32));
		int AlignedLen = (Len+3)/4*4;
		m_lLastSaveInfo.SetLen(AlignedLen);
		pGame->m_CurrentSaveFile.Read(m_lLastSaveInfo.GetBasePtr(), AlignedLen);
		m_lLastSaveInfo.QuickSetLen(Len);

//		TArray<uint8> lTempData = CDiskUtil::Decompress( m_lLastSaveInfo );
		TArray<uint8> lTempData = m_lLastSaveInfo;
		spCCFile spFileMem = CDiskUtil::CreateCCFile(lTempData, CFILE_READ | CFILE_BINARY);
		CDataFile DataFile;
		DataFile.Open(spFileMem, 0);
		CCFile *pF = DataFile.GetFile();
		if(!DataFile.GetNext("SAVEINFO"))
		{
			pGame->EndReadSaveFile();
			return;
		}

		VERBOSE_OUT(CStrF("[WDeltaGameState] Reading 'SAVEINFO':"));
		Level = pF->Readln();
		int32 SimulationTick;
		pF->ReadLE(SimulationTick);
		_pCore->SetGameTick(SimulationTick);
		pF->ReadLE(NextGUID); // set after LoadLevel is completed
		pF->ReadLE(NextConstraintID);
		_pCore->m_NextConstraintID = NextConstraintID; // set before loading level, so that new constraints won't collide with saved constraints
		VERBOSE_OUT(CStrF("- WorldName: %s\n- GameTick: %d\n- NextGUID: 0x%08X\n- NextConstraintID: %d\n", Level.Str(), SimulationTick, NextGUID, NextConstraintID));

		if(DataFile.GetNext("PLAYERS"))
		{
			int nPlayers = DataFile.GetUserData();
			m_lPendingPlayers.SetLen(nPlayers);
			for(int i = 0; i < nPlayers; i++)
			{
				m_lPendingPlayers[i].m_Position.Read(pF);
				m_lPendingPlayers[i].m_SpecialClass.Read(pF);
				pF->ReadLE(m_lPendingPlayers[i].m_Flags);
				m_lPendingPlayers[i].m_PendingPlayerObj = -1;
				m_lPendingPlayers[i].m_PendingPlayerGUID = -1;
				uint16 Len;
				pF->ReadLE(Len);
				m_lPendingPlayers[i].m_lData.SetLen(Len);
				pF->Read(m_lPendingPlayers[i].m_lData.GetBasePtr(), Len);
			}
		}

		if(DataFile.GetNext("PRECACHEINFO"))
		{
			int32 nInfo = DataFile.GetUserData();
			m_lPrecacheInfoNames.SetLen(nInfo);
			m_lPrecacheInfoTypes.SetLen(nInfo);
			for(int i = 0; i < nInfo; i++)
			{
				pF->ReadLE(m_lPrecacheInfoTypes[i]);
				m_lPrecacheInfoNames[i].Read(pF);
			}
		}

		// Read global spawnflags
		if (DataFile.GetNext("SPAWNFLAGS"))
		{
			uint32 SpawnMask = DataFile.GetUserData();
			VERBOSE_OUT(CStrF("[WDeltaGameState] Reading 'SPAWNFLAGS': 0x%08X", SpawnMask));
			_pCore->m_PrevSpawnMask = SpawnMask;

			_pCore->m_CurSpawnMask &= SERVER_SPAWNFLAGS_LOCAL;					// keep local
			_pCore->m_CurSpawnMask |= (SpawnMask & SERVER_SPAWNFLAGS_GLOBAL);	// update global
		}

		DataFile.Close();
	}

	/*
	CDeltaWorldData Data;
	pGame->m_CurrentSaveFile.Read(Len, sizeof(int));
	Data.m_lData.SetLen(Len);
	pGame->m_CurrentSaveFile.Read(Data.m_lData.GetBasePtr(), Len);
		WriteSaveFileBlock((uint8 *)&Len, sizeof(int32));
		*/

	m_lDeltaWorldData.Clear();
	int32 NumDelta;
	pGame->m_CurrentSaveFile.Read(&NumDelta, sizeof(int32));
	for(int i = 0; i < NumDelta; i++)
	{
		CDeltaWorldData Data;

		char aLevelName[256];
		int32 c = 0;
		while(1)
		{
			pGame->m_CurrentSaveFile.Read(&aLevelName[c], sizeof(char));
			if(aLevelName[c] == 0)
				break;
			c++;
		}
		Data.m_Level = aLevelName;

		c+=1;
		c = (c+3)/4*4 - c; // align fix
		uint8 extra;
		while(c--)
			pGame->m_CurrentSaveFile.Read(extra);

		pGame->m_CurrentSaveFile.Read(&Len, sizeof(int32));
		int AlignedLen = (Len +3)/4*4;
		Data.m_lData.SetLen(AlignedLen);
		pGame->m_CurrentSaveFile.Read(Data.m_lData.GetBasePtr(), AlignedLen);
		Data.m_lData.QuickSetLen(Len); // Makes sure the array is not realocated, and contains the aligned bytes needed

		pGame->m_CurrentSaveFile.Read(&Data.m_LocalSpawnFlags, sizeof(Data.m_LocalSpawnFlags));

		m_lDeltaWorldData.Add(Data);
	}

	pGame->EndReadSaveFile();

#ifdef PLATFORM_CONSOLE
	LoadLevel(_pCore, Level, 0);
#else
	LoadLevel(_pCore, Level, SERVER_CHANGEWORLD_KEEPRESOURCES);
#endif

	NextGUID = Max(NextGUID, (uint32)_pCore->m_NextGUID);
	VERBOSE_OUT(CStrF("[WDeltaGameState] Setting NextGUID to 0x%08X", NextGUID));
	_pCore->m_NextGUID = NextGUID;

	/*
	m_bChangeLevel = false;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CStr Profile = pSys->GetOptions()->GetValue("GAME_PROFILE");
	if(Profile == "")
		Profile = "debug";
	CStr CompleteName = _SaveName + " - " + Profile;

	CFStr Path;
#if defined(PLATFORM_XBOX)
	#if defined(M_DEMO_XBOX)
		Path = "Z:\\Save\\" + CompleteName + "\\";
	#else
		const char *pSaveDrv = "U:\\";
		if(XCreateSaveGame(pSaveDrv, CompleteName.Unicode().StrW(), OPEN_EXISTING, 0, Path, Path.GetMax()) != ERROR_SUCCESS)
			return;

		if(Path == "")
			return;
		Path += "\\";
	#endif
#elif defined( PLATFORM_PS2 )
	#warning "Implement CWorld_DeltaGameState::LoadGame"
	return;
#else
	Path = _pCore->m_spWData->ResolvePath("Save\\") + CompleteName + "\\";
#endif
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return; // WE SHOULD WARN

#ifndef M_DEMO
	m_lDeltaWorldData.Clear();
	CDirectoryNode Dir;
	Dir.ReadDirectory(Path + "*.xws");
	int nf = Dir.GetFileCount();
	for(int i = 0; i < nf; i++)
	{
		CDir_FileRec* pF = Dir.GetFileRec(i);
		if(!Dir.IsDirectory(i))
		{
			CDeltaWorldData Data;
			if(!pGame->ReadSaveFile(_pCore->m_spWData, CompleteName, pF->m_Name.Str(), Data.m_lData, true))
				continue;

			Data.m_Level = pF->m_Name.GetFilenameNoExt();
			m_lDeltaWorldData.Add(Data);
		}
	}
#endif

	CStr Level;
	{
		// Load SaveInfo
#ifndef M_DEMO
		m_lLastSaveInfo.Clear();
		if(!pGame->ReadSaveFile(_pCore->m_spWData, CompleteName, "Save.xsg", m_lLastSaveInfo, true))
			return;
#endif

		spCCFile spFileMem = CDiskUtil::CreateCCFile(m_lLastSaveInfo, CFILE_READ | CFILE_BINARY);
		CDataFile DataFile;
		DataFile.Open(spFileMem, 0);
		CCFile *pF = DataFile.GetFile();
		if(!DataFile.GetNext("SAVEINFO"))
			return;

		Level = pF->Readln();
		pF->ReadLE(_pCore->m_SimulationTick);
		pF->ReadLE(_pCore->m_NextGUID);

		if(DataFile.GetNext("PLAYERS"))
		{
			int nPlayers = DataFile.GetUserData();
			m_lPendingPlayers.SetLen(nPlayers);
			for(int i = 0; i < nPlayers; i++)
			{
				m_lPendingPlayers[i].m_Position.Read(pF);
				m_lPendingPlayers[i].m_SpecialClass.Read(pF);
				uint16 Len;
				pF->ReadLE(Len);
				m_lPendingPlayers[i].m_lData.SetLen(Len);
				pF->Read(m_lPendingPlayers[i].m_lData.GetBasePtr(), Len);
			}
		}

		if(DataFile.GetNext("PRECACHEINFO"))
		{
			int32 nInfo = DataFile.GetUserData();
			m_lPrecacheInfoNames.SetLen(nInfo);
			m_lPrecacheInfoTypes.SetLen(nInfo);
			for(int i = 0; i < nInfo; i++)
			{
				pF->ReadLE(m_lPrecacheInfoTypes[i]);
				m_lPrecacheInfoNames[i].Read(pF);
			}
		}

		DataFile.Close();
	}

#ifdef PLATFORM_CONSOLE
	LoadLevel(_pCore, Level, 0);
#else
	LoadLevel(_pCore, Level, SERVER_CHANGEWORLD_KEEPRESOURCES);
#endif
	*/
}


void CWorld_DeltaGameState::PrecacheInfo(const char *_pResource, int _Type)
{
	m_lPrecacheInfoNames.Add(_pResource);
	m_lPrecacheInfoTypes.Add(_Type);
}


void CWorld_DeltaGameState::LoadLevel(CWorld_ServerCore *_pCore, const char *_pNewLevel, int _Flags)
{
	CFStr Level = CStr(_pNewLevel).GetFilenameNoExt();

	// Find matching save-game
	uint32 LocalSpawnFlags = M_Bit(SERVER_SPAWNFLAGS_LAYERS_BASE) | SERVER_SPAWNFLAGS_ONCE; // Default: set "layer1+once"
	int iDeltaWorldData = -1;
	for (int i = 0; i < m_lDeltaWorldData.Len(); i++)
		if (m_lDeltaWorldData[i].m_Level.CompareNoCase(Level) == 0)
		{
			LocalSpawnFlags = m_lDeltaWorldData[i].m_LocalSpawnFlags;
			iDeltaWorldData = i;
			break;
		}

	// Apply local spawnflags
	_pCore->m_CurSpawnMask &= SERVER_SPAWNFLAGS_GLOBAL;						// keep global
	_pCore->m_CurSpawnMask |= (LocalSpawnFlags & SERVER_SPAWNFLAGS_LOCAL);	// update local
	VERBOSE_OUT(CStrF("[WDeltaGameState] Loading level '%s', SpawnMask: %08X", Level.Str(), _pCore->m_CurSpawnMask));

	if (iDeltaWorldData >= 0)
	{
		M_ASSERT(_pCore->m_lResourceNameHashes.Len() == 0, "IndexMap internal error");
		_pCore->m_lResourceNameHashes.Clear();
		_pCore->m_lResourceNameHashes = m_lDeltaWorldData[iDeltaWorldData].m_lResourceMappings;
	}

	// Clear 'inactive' lists
	m_lInactiveOrg.Clear();
	m_lInactiveNew.Clear();

	// Start new level
	_pCore->World_Change(_pNewLevel, _Flags);

	// Before we mess with the objects, store a list of orig-objects
	RegisterOrgObjects(_pCore);

	// Apply savegame
	if (iDeltaWorldData >= 0)
		ApplyDeltaSave(_pCore, m_lDeltaWorldData[iDeltaWorldData].m_lData, _Flags);

	// Allow us to precache, even though World_Change is complete
	int iOldState = _pCore->GetMapData()->GetState();
	_pCore->GetMapData()->SetState(iOldState & (~WMAPDATA_STATE_NOCREATE));

	for(int i = 0; i < m_lPrecacheInfoNames.Len(); i++)
	{
		CWObject_Message Msg(OBJMSG_GAME_LOADPRECACHEINFO, m_lPrecacheInfoTypes[i]);
		Msg.m_pData = (void *)m_lPrecacheInfoNames[i].Str();
		_pCore->Message_SendToObject(Msg, _pCore->Game_GetObjectIndex());
	}
	CWObject_Message Msg(OBJMSG_GAME_LOADANIMATIONS);
	_pCore->Message_SendToObject(Msg, _pCore->Game_GetObjectIndex());
	_pCore->GetMapData()->SetState(iOldState);

	m_lPrecacheInfoNames.Clear();
	m_lPrecacheInfoTypes.Clear();
}


bool CWorld_DeltaGameState::SendMessageToLevel(const char *_pTarget, const CWObject_Message &_Msg)
{
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return false;

	CFStr Target = _pTarget;
	int Len = Target.Len();
	if(Len == 0)
		return false;
	if(Target[Len - 1] == '_')
		return false;

	if(pGame->m_spAsyncSaveContext != NULL)
	{
		int i = m_lBlockedWorldMessages.Len();
		m_lBlockedWorldMessages.SetLen(i + 1);
		CBlockedWorldMessage *pMsg = &m_lBlockedWorldMessages[i];
		pMsg->m_Target = _pTarget;
		pMsg->m_Msg = _Msg.m_Msg;
		pMsg->m_Param0 = _Msg.m_Param0;
		if(_Msg.m_DataSize > 0)
		{
			pMsg->m_lData.SetLen(_Msg.m_DataSize);
			memcpy(pMsg->m_lData.GetBasePtr(), _Msg.m_pData, _Msg.m_DataSize);
		}
		return true;
	}

	// Find level-entry
	CFStr Level = Target.GetStrSep(":");
	int i;
	for(i = 0; i < m_lDeltaWorldData.Len(); i++)
		if(m_lDeltaWorldData[i].m_Level.CompareNoCase(Level.Str()) == 0)
			break;

	TArray<uint8> lOrgObjects, lNewObjects, lMessageData;
	int OrgObjectsUD = 0;
	int NewObjectsUD = 0;
	int nMessages = 0;

	if(m_lDeltaWorldData.Len() == i)
	{
		// Not found, create level-entry
		m_lDeltaWorldData.Add(CDeltaWorldData());
		m_lDeltaWorldData[i].m_Level = Level;
		m_lDeltaWorldData[i].m_LocalSpawnFlags = M_Bit(SERVER_SPAWNFLAGS_LAYERS_BASE) | SERVER_SPAWNFLAGS_ONCE; // Default: set "layer1+once"
	}
	else if (m_lDeltaWorldData[i].m_lData.Len())
	{
		// Open data stream
//		TArray<uint8> lBuf = CDiskUtil::Decompress(m_lDeltaWorldData[i].m_lData);
		TArray<uint8> lBuf = m_lDeltaWorldData[i].m_lData;
		spCCFile spFileMem = CDiskUtil::CreateCCFile(lBuf, CFILE_READ | CFILE_WRITE | CFILE_BINARY);
		CDataFile DataFile;
		DataFile.Open(spFileMem, 0);
		CCFile *pF = DataFile.GetFile();

		// Read previous data-entries
		while(true)
		{
			CFStr Block = DataFile.GetNext();
			if(Block == "")
				break;

			if (Block == "ORGOBJECTS")
			{
				lOrgObjects.SetLen(DataFile.GetEntrySize());
				OrgObjectsUD = DataFile.GetUserData();
				pF->Read(lOrgObjects.GetBasePtr(), DataFile.GetEntrySize());
			}
			else if (Block == "NEWOBJECTS")
			{
				lNewObjects.SetLen(DataFile.GetEntrySize());
				NewObjectsUD = DataFile.GetUserData();
				pF->Read(lNewObjects.GetBasePtr(), DataFile.GetEntrySize());
			}

			/*	TArray<uint8> lPrecacheInfo;
			int nPrecacheInfo = 0;
			if(DataFile.GetNext("PRECACHEINFO"))
			{
			lPrecacheInfo.SetLen(DataFile.GetEntrySize());
			nPrecacheInfo = DataFile.GetUserData();
			pF->Read(lPrecacheInfo.GetBasePtr(), DataFile.GetEntrySize());
			}*/

			else if(Block == "MESSAGES")
			{
				// Read previous Message data-entry
				TArray<uint8> Data;
				int Size = DataFile.GetEntrySize();
				nMessages = DataFile.GetUserData();
				lMessageData.SetLen(Size);
				pF->Read(lMessageData.GetBasePtr(), Size);
			}
		}

		DataFile.Close();
	}

	// Reset file, and open for write
	TArray<uint8> lBuf;
	spCCFile spFileMem = CDiskUtil::CreateCCFile(lBuf, CFILE_READ | CFILE_BINARY);
	CDataFile DataFile;
	DataFile.Create(spFileMem);
	CCFile *pF = DataFile.GetFile();


	// Write back original data
	if (lOrgObjects.Len())
	{
		DataFile.BeginEntry("ORGOBJECTS");
		pF->Write(lOrgObjects.GetBasePtr(), lOrgObjects.Len());
		DataFile.EndEntry(OrgObjectsUD);
	}
	if (lNewObjects.Len())
	{
		DataFile.BeginEntry("NEWOBJECTS");
		pF->Write(lNewObjects.GetBasePtr(), lNewObjects.Len());
		DataFile.EndEntry(NewObjectsUD);
	}

	// Write old and append new message data
	{
		DataFile.BeginEntry("MESSAGES");
		pF->Write(lMessageData.GetBasePtr(), lMessageData.Len());
		{
			nMessages++;
			Target.Write(pF);
			pF->WriteLE(_Msg.m_Msg);
			pF->WriteLE(int32(_Msg.m_Param0));
			if (_Msg.m_pData)
				pF->WriteLE(_Msg.m_DataSize);
			else
				pF->WriteLE(int32(0));

			if (_Msg.m_DataSize && _Msg.m_pData)
				pF->Write(_Msg.m_pData, _Msg.m_DataSize);
		}
		DataFile.EndEntry(nMessages);
	}

	DataFile.Close();
	spFileMem->Close();

//	m_lDeltaWorldData[i].m_lData = CDiskUtil::Compress(lBuf, LZSS_HUFFMAN);
	m_lDeltaWorldData[i].m_lData = lBuf;
	m_lDeltaWorldData[i].m_DataSize = m_lDeltaWorldData[i].m_lData.Len();
	return true;
}


void CWorld_DeltaGameState::CapturePendingPlayers(CWorld_ServerCore *_pCore)
{
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pCore->Selection_AddClass(Selection, "CharPlayer");
	const int16 *lpSel;
	int nSel = _pCore->Selection_Get(Selection, &lpSel);
	uint8 nPlayers = 0;
	int i;
	for(i = 0; i < nSel; i++)
	{
		CWObject *pObj = _pCore->Object_Get(lpSel[i]);
		if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
			nPlayers++;
	}

	m_lPendingPlayers.SetLen(nPlayers);

	m_lPrecacheInfoNames.Clear();
	m_lPrecacheInfoTypes.Clear();

	nPlayers = 0;
	for(i = 0; i < nSel; i++)
	{
		CWObject *pObj = _pCore->Object_Get(lpSel[i]);
		if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
		{
			m_lPendingPlayers[nPlayers].m_Position = pObj->GetPositionMatrix();
			m_lPendingPlayers[nPlayers].m_SpecialClass = (const char *)_pCore->Message_SendToObject(CWObject_Message(OBJMSG_PLAYER_GETSPECIALCLASS), lpSel[i]);
			int PhysType = _pCore->Message_SendToObject(CWObject_Message(OBJMSG_PLAYER_GETPHYSTYPE), lpSel[i]);
			if(PhysType == 2)
				m_lPendingPlayers[nPlayers].m_Flags = INFOPLAYERSTART_FLAGS_STARTCROUCHED;
			else
				m_lPendingPlayers[nPlayers].m_Flags = 0;

			m_lPendingPlayers[nPlayers].m_PendingPlayerObj = -1;
			m_lPendingPlayers[nPlayers].m_PendingPlayerGUID = -1;

			spCCFile spFileMem = CDiskUtil::CreateCCFile(m_lPendingPlayers[nPlayers].m_lData, CFILE_READ | CFILE_BINARY);
			pObj->OnDeltaSave(spFileMem);
			spFileMem->Close();
			m_lPendingPlayers[nPlayers].m_lData.OptimizeMemory();
			nPlayers++;
		}
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Called to apply delta-save to newly spawned player
\*____________________________________________________________________*/
bool CWorld_DeltaGameState::ApplyNextPendingPlayer(CWorld_ServerCore *_pCore, int _iObject, int _Flags)
{
	if(m_lPendingPlayers.Len() == 0)
		return false;

	CWObject *pObj = _pCore->Object_Get(_iObject);
	if(!pObj)
		return false;

	spCCFile spFileMem = CDiskUtil::CreateCCFile(m_lPendingPlayers[0].m_lData, CFILE_READ | CFILE_BINARY);
	pObj->OnDeltaLoad(spFileMem, _Flags);
	spFileMem->Close();

	m_lPendingPlayers.Del(0);
	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Called to check if there exist save info for
					another player
\*____________________________________________________________________*/
bool CWorld_DeltaGameState::GetNextPendingPlayerPos(CMat4Dfp32 &_Mat)
{
	if(m_bChangeLevel)
		return false;

	if(m_lPendingPlayers.Len() == 0)
		return false;

	_Mat = m_lPendingPlayers[0].m_Position;
	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Return next playerclass to be spawned
\*____________________________________________________________________*/
CStr CWorld_DeltaGameState::GetNextPendingPlayerClass(int *_piObj, int *_pGUID, int16 *_pFlags)
{
	if(m_lPendingPlayers.Len() == 0)
		return "";

	if(_piObj)
		*_piObj = m_lPendingPlayers[0].m_PendingPlayerObj;
	if(_pGUID)
		*_pGUID = m_lPendingPlayers[0].m_PendingPlayerGUID;
	if(_pFlags)
		*_pFlags |= m_lPendingPlayers[0].m_Flags;

	return m_lPendingPlayers[0].m_SpecialClass;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   	Checks if the player has visited the current before
			If no level input is provided, current level will be
			checked
\*____________________________________________________________________*/
bool CWorld_DeltaGameState::IsFirstLevelVisit(CWorld_ServerCore *_pCore, CStr _Level)
{
	CStr Level = _Level.IsEmpty() ? CStr(_pCore->m_WorldName).GetFilenameNoExt() : _Level;
	for (int i = 0; i < m_lDeltaWorldData.Len(); i++)
		if (m_lDeltaWorldData[i].m_Level.CompareNoCase(Level) == 0)
		{
//			TArray<uint8> lBuf = CDiskUtil::Decompress(m_lDeltaWorldData[i].m_lData);
			TArray<uint8> lBuf = m_lDeltaWorldData[i].m_lData;
			if (lBuf.Len() == 0)
				return true;

			spCCFile spFileMem = CDiskUtil::CreateCCFile(lBuf, CFILE_READ | CFILE_WRITE | CFILE_BINARY);
			CDataFile DataFile;
			DataFile.Open(spFileMem, 0);

			// Read previous data-entries
			while (true)
			{
				CFStr Block = DataFile.GetNext();
				if (Block == "")
					break;
				if (Block == "ORGOBJECTS")
					return false;
				if (Block == "NEWOBJECTS")
					return false;
			}
		}
	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:   This removes all delta save info for a specified level 
				(but will not alter local spawnflags nor pending messages)
\*______________________________________________________________________________*/
void CWorld_DeltaGameState::ResetSaveInfo(const char* _pLevel)
{
	// Find matching save-game
	TAP<CDeltaWorldData> pLevelSaves = m_lDeltaWorldData;
	for (int i = 0; i < pLevelSaves.Len(); i++)
		if (pLevelSaves[i].m_Level.CompareNoCase(_pLevel) == 0)
		{
			pLevelSaves[i].m_lData.Clear();
			pLevelSaves[i].m_DataSize = 0; // Only updated during save
			ConOutL(CStrF("*LEVEL RESET* - cleared all save data for level '%s'...", _pLevel));
			return;
		}
}


void CWorld_DeltaGameState::ModifyLocalSpawnFlags(const char* _pLevel, bool _bSet, uint32 _Spawnflags)
{
	// Find existing entry
	TAP<CDeltaWorldData> pLevelSaves = m_lDeltaWorldData;
	int i;
	for (i = 0; i < pLevelSaves.Len(); i++)
		if (pLevelSaves[i].m_Level.CompareNoCase(_pLevel) == 0)
			break;

	// Need to create new one?
	if (i == pLevelSaves.Len())
	{
		m_lDeltaWorldData.Add(CDeltaWorldData());
		pLevelSaves = m_lDeltaWorldData;
		pLevelSaves[i].m_Level = _pLevel;
		pLevelSaves[i].m_LocalSpawnFlags = M_Bit(SERVER_SPAWNFLAGS_LAYERS_BASE) | SERVER_SPAWNFLAGS_ONCE; // Default: set "layer1+once"
		pLevelSaves[i].m_DataSize = 0;
	}

	if (_bSet)
		pLevelSaves[i].m_LocalSpawnFlags |= _Spawnflags;
	else
		pLevelSaves[i].m_LocalSpawnFlags &= ~_Spawnflags;
}


void CWorld_DeltaGameState::ClearPendingPlayers()
{
	m_lPendingPlayers.Clear();
}


void CWorld_DeltaGameState::ClearSaves()
{
	m_lDeltaWorldData.Clear();
}


bool CWorld_DeltaGameState::RefreshBlockedWorldMessages()
{
	if(!m_lBlockedWorldMessages.Len())
		return true;

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return false;

	if(!pGame->m_spAsyncSaveContext == NULL)
		return false;

	for(int i = 0; i < m_lBlockedWorldMessages.Len(); i++)
	{
		CBlockedWorldMessage *pMsg = &m_lBlockedWorldMessages[i];
		CWObject_Message Msg(pMsg->m_Msg, pMsg->m_Param0);
		Msg.m_pData = pMsg->m_lData.GetBasePtr();
		Msg.m_DataSize = pMsg->m_lData.Len();
		SendMessageToLevel(pMsg->m_Target, Msg);
	}
	m_lBlockedWorldMessages.Clear();
	return true;
}


/*
bool CWorld_DeltaGameState::ReadSaveFile(spCWorldData _spWData, CStr _SaveName, CStr _FileName, TArray<uint8> &_lData, bool _bCryptate)
{
	int32 nLoadDataLength;
	CFStr Path;

#if defined(PLATFORM_XBOX)
	// Create (or open an existing) save game with the specified name.
#if defined(M_DEMO_XBOX)
	const char *pSaveDrv = "Z:\\Saves\\";
#else
	const char *pSaveDrv = "U:\\";
#endif
	if(XCreateSaveGame(pSaveDrv, _SaveName.Unicode().StrW(), OPEN_EXISTING, 0, Path, Path.GetMax()) != ERROR_SUCCESS)
		return false;

	//if(Path == "")
	//	return false;

	Path += "\\";
#else
	Path = _spWData->ResolvePath("Save\\") + _SaveName + "\\";
#endif

	CStr FilePath = Path.Str() + _FileName;
	if(!CDiskUtil::FileExists(FilePath))
		return false;

	CCFile File;
	File.Open(FilePath, CFILE_READ | CFILE_BINARY);
	nLoadDataLength = File.Length();
	_lData.SetLen(nLoadDataLength);
	File.ReadLE(_lData.GetBasePtr(), nLoadDataLength);
	File.Close();

	// Decryptate data and check checksum
	if(_bCryptate)
	{
		uint8 *pData = _lData.GetBasePtr();

		// Extract checksum
		uint8 FileCrcCalc[CHECKSUM_BYTELENGTH];
		for(int j = 0; j < CHECKSUM_BYTELENGTH; j++)
			FileCrcCalc[j] = pData[j + _lData.Len() - CHECKSUM_BYTELENGTH];
		_lData.SetLen(_lData.Len() - CHECKSUM_BYTELENGTH);

		CCrcCheck Crc;
		for(int i = 0; i < _lData.Len(); i++)
		{
			pData[i] ^= 42;
			Crc.UpdateCrc(pData[i]);
		}

		// Compare checksum
		uint8 CrcCalc[CHECKSUM_BYTELENGTH];
		Crc.GetChecksumList(CrcCalc);
		for(int k = 0; k < CHECKSUM_BYTELENGTH; k++)
			if(CrcCalc[k] != FileCrcCalc[k])
				return false;
	}

	return true;
}
*/


#ifdef M_RTM
#  define MACRO_WRITEVERIFYTYPE(_pObj)
#  define MACRO_READVERIFYTYPE(_pObj)
#  define MACRO_READIGNORETYPE
#  define MACRO_WRITEVERIFY
#  define MACRO_READVERIFY
#else // M_RTM
#  ifdef COMPILER_RTTI
#    define MACRO_WRITEVERIFYTYPE(_pObj) {CStr Fetto(typeid(*_pObj).name()); Fetto.Write(pF);}
#    define MACRO_READVERIFYTYPE(_pObj) {CStr Fetto(typeid(*_pObj).name()); CStr FettoFromFile; FettoFromFile.Read(pF); M_ASSERT(Fetto == FettoFromFile, CStrF("Load/save _TYPE_ mismatch in file '%s' on line %i", __FILE__, __LINE__));}
#  else	// COMPILER_RTTI
#    define MACRO_WRITEVERIFYTYPE(_pObj) {CStr Fetto(_pObj->MRTC_ClassName()); Fetto.Write(pF);}
#    define MACRO_READVERIFYTYPE(_pObj) {CStr Fetto(_pObj->MRTC_ClassName()); CStr FettoFromFile; FettoFromFile.Read(pF); M_ASSERT(Fetto == FettoFromFile, CStrF("Load/save _TYPE_ mismatch in file '%s' on line %i", __FILE__, __LINE__));}
#  endif // COMPILER_RTTI
#    define MACRO_READIGNORETYPE {CStr().Read(pF);}
#    define MACRO_WRITEVERIFY {uint32 Apa = 0xDEADC0DE; pF->WriteLE(Apa);}
#    define MACRO_READVERIFY {uint32 Apa; pF->ReadLE(Apa); M_ASSERT(Apa == 0xDEADC0DE, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };
#endif // M_RTM


bool CWorld_DeltaGameState::ApplyDeltaSave(CWorld_ServerCore* _pCore, TArray<uint8> _lData, int _Flags)
{
	if (_lData.Len() == 0)
		return false;

	TArray<uint8> lBuf;
//	lBuf = CDiskUtil::Decompress(_lData);
	lBuf = _lData;

	spCCFile spFileMem = CDiskUtil::CreateCCFile(lBuf, CFILE_READ | CFILE_BINARY);
	CDataFile DataFile;
	DataFile.Open(spFileMem, 0);
	CCFile *pF = DataFile.GetFile();

	while (true)
	{
		CFStr Block = DataFile.GetNext();
		if (Block == "")
			break;

		if (Block == "ORGOBJECTS")
		{
			uint nObj = DataFile.GetUserData();
			for (uint i = 0; i < nObj; i++)
			{
				uint32 GUID, nDataSize;
				pF->ReadLE(GUID);
				pF->ReadLE(nDataSize);
				M_ASSERT(nDataSize < 10000, "savegame data size overflow!");
				if (nDataSize > 10000)
					return false;

				CWObject* pObj = _pCore->Object_GetWithGUID(GUID);
				if (pObj)
				{
					VERBOSE_OUT(CStrF("[WDeltaGameState] Loading ORGOBJECT, GUID: %08X, Index: %d, Type: %s, Name: '%s'", GUID, pObj->m_iObject, typeid(*pObj).name(), pObj->GetName()));
					MACRO_READVERIFYTYPE(pObj);
					pObj->OnDeltaLoad(pF, _Flags);
				}
				else
				{
					VERBOSE_OUT(CStrF("[WDeltaGameState] Storing ORGOBJECT as inactive, GUID: %08X", GUID));
					m_lInactiveOrg.Add( SInactiveObj(GUID, pF, nDataSize) );
				}
				MACRO_READVERIFY;
			}
		}
		else if (Block == "NEWOBJECTS")
		{
			//pF->ReadLE(_pCore->m_NextGUID);

			uint nObj = DataFile.GetUserData();
			for (uint i = 0; i < nObj; i++)
			{
				uint32 GUID, SpawnMask, nDataSize;
				pF->ReadLE(GUID);
				pF->ReadLE(SpawnMask);
				pF->ReadLE(nDataSize);
				M_ASSERT(nDataSize < 10000, "savegame data size overflow!");
				if (nDataSize > 10000)
					return false;

				if (_pCore->World_TestSpawnFlags(SpawnMask))
				{
					CFStr TemplateName;
					TemplateName.Read(pF);

					CMat4Dfp32 Mat;
					Mat.Read(pF);

					int iObj = _pCore->Object_Create(TemplateName, Mat, -1, NULL, 0, -1, GUID);
					CWObject *pObj = _pCore->Object_Get(iObj);
					if (!pObj)
						return false;

		//			uint32 OldGUID = _pCore->Object_GetGUID(iObj);
		//			bool bOK = _pCore->Object_ChangeGUID(iObj, GUID);
		//			M_ASSERT(bOK, "Couldn't set GUID!");
					VERBOSE_OUT(CStrF("[WDeltaGameState] Loading NEWOBJECT, GUID: %08X, Index: %d, Type: %s", GUID, iObj, typeid(*pObj).name()));
					pObj->OnDeltaLoad(pF, _Flags);
				}
				else
				{
					VERBOSE_OUT(CStrF("[WDeltaGameState] Storing NEWOBJECT as inactive, GUID: %08X, SpawnMask: %08X", GUID, SpawnMask));
					m_lInactiveNew.Add( SInactiveObj(GUID, pF, nDataSize, SpawnMask) );
				}
				MACRO_READVERIFY;
			}
			int32 PendingPlayerObj = -1;
			int32 PendingPlayerGUID = -1;
			pF->ReadLE(PendingPlayerObj);
			pF->ReadLE(PendingPlayerGUID);
			if(m_lPendingPlayers.Len() > 0)
			{
				m_lPendingPlayers[0].m_PendingPlayerObj = PendingPlayerObj;
				m_lPendingPlayers[0].m_PendingPlayerGUID = PendingPlayerGUID;
			}
			// Fake insert player guid into hash until it's created, so object that needs it can access it
			_pCore->m_spGUIDHash->Insert(PendingPlayerObj, PendingPlayerGUID);
			m_bHasInsertedGUID = true;
		}
		else if (Block == "DELETED")
		{
			uint nDeleted = DataFile.GetUserData();
			for (uint i = 0; i < nDeleted; i++)
			{
				uint32 GUID;
				pF->ReadLE(GUID);
				CWObject* pObj = _pCore->Object_GetWithGUID(GUID);
				if (pObj)
				{ // object exists, delete!
					_pCore->Object_DestroyInstant(pObj->m_iObject);
				}
				else
				{ // object wasn't spawned. manually add it to m_lSpawnedOrg so that it's kept marked as DELETED
					m_lSpawnedOrg.Add(GUID);
				}
			}
		}
		else if (Block == "MESSAGES")
		{
			uint nMessages = DataFile.GetUserData();
			for (uint i = 0; i < nMessages; i++)
			{
				CWObject_Message Msg;
				CStr Target;
				Target.Read(pF);
				pF->ReadLE(Msg.m_Msg);
				int32 Param0;
				pF->ReadLE(Param0);
				Msg.m_Param0 = Param0;
				pF->ReadLE(Msg.m_DataSize);
				TArray<uint8> Data;
				if(Msg.m_DataSize > 0)
				{
					Data.SetLen(Msg.m_DataSize);
					Msg.m_pData = Data.GetBasePtr();
					pF->Read(Msg.m_pData, Msg.m_DataSize);
				}
				_pCore->Message_SendToTarget(Msg, Target);
			}
		}
		else if (Block == "PHYS_CONSTRAINTS")
		{
			uint nConstraints = DataFile.GetUserData();
			for (uint i = 0; i < nConstraints; i++)
			{
				CWD_ConstraintDescriptor Tmp;
				Tmp.Read(pF, _pCore);
				uint32 iObjA, iObjB;
				Tmp.GetConnectedObjects(iObjA, iObjB);
				CWObject* pObj1 = _pCore->Object_Get(iObjA);
				CWObject* pObj2 = _pCore->Object_Get(iObjB);
				_pCore->Phys_DoAddConstraint(pObj1, pObj2, Tmp);
			}
			VERBOSE_OUT(CFStrF("Loaded %d constraints (NextID: %d)", nConstraints, _pCore->m_NextConstraintID));
		}
	}
	DataFile.Close();

	// Reset constraint "NextID" to the maximum ID (optimization - constraints may be deleted by OnDeltaLoad and/or the "DELETED"-block)
	int MaxConstraintID = 0;
	TAP<const CWD_ConstraintDescriptor> pConstraints = _pCore->m_lConstraints;
	for (uint i = 0; i < pConstraints.Len(); i++)
		MaxConstraintID = Max(MaxConstraintID, pConstraints[i].GetID());
	_pCore->m_NextConstraintID = MaxConstraintID + 1;
	VERBOSE_OUT(CFStrF("NOTE: Server NextConstraintID was set to: %d", _pCore->m_NextConstraintID));

	return true;
}


void CWorld_DeltaGameState::RegisterOrgObjects(CWorld_ServerCore *_pCore)
{
	m_lSpawnedOrg.Clear();
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pCore->Selection_AddAll(Selection);
	const int16* lpObj;
	int nObj = _pCore->Selection_Get(Selection, &lpObj);
	for (uint i = 0; i < nObj; i++)
	{
		CWObject* pObj = _pCore->Object_Get(lpObj[i]);
		if (pObj && !pObj->IsDestroyed() && !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) && pObj->m_bOriginallySpawned)
		{
			uint32 GUID = _pCore->Object_GetGUID(pObj->m_iObject);
			m_lSpawnedOrg.Add(GUID);
		}
	}
}


/*
void CWorld_DeltaGameState::WriteSaveFile(spCWorldData _pWData, CStr _SaveName, CStr _FileName, TArray<uint8> _lData, bool _bCryptate)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Save, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_DeltaGameState::World_Save, WSERVERMOD);

	// Cryptate data and add checksum
	if(_bCryptate)
	{
		CCrcCheck Crc;
		uint8 *pData = _lData.GetBasePtr();
		for(int i = 0; i < _lData.Len(); i++)
		{
			Crc.UpdateCrc(pData[i]);
			pData[i] ^= 42;
		}

		// Add the checksum last in the file
		uint8 CrcCalc[CHECKSUM_BYTELENGTH];
		Crc.GetChecksumList(CrcCalc);
		for(int j = 0; j < CHECKSUM_BYTELENGTH; j++)
			_lData.Add(CrcCalc[j]);
	}

	try
	{
		CStr Path; // holds the savegame path

#if defined (PLATFORM_XBOX)
		CFStr TempPath;

#if defined(M_DEMO_XBOX)
	const char *pSaveDrv = "Z:\\Saves\\";
#else
	const char *pSaveDrv = "U:\\";
#endif
		if(XCreateSaveGame(pSaveDrv, _SaveName.Unicode().StrW(), OPEN_ALWAYS, 0, TempPath, TempPath.GetMax()) != ERROR_SUCCESS)
			return;

		//if(TempPath == "")
		//	return;

		Path = TempPath;
#else
		Path = _pWData->ResolvePath("Save\\") + _SaveName + "\\";
		CDiskUtil::CreatePath(Path);
#endif
		// Nuke previous files before saving game.
		if(CDiskUtil::FileExists(Path + _FileName))
			CDiskUtil::DelFile(Path + _FileName);

		// Write save game to disk
		{
			CCFile File;
			File.OpenExt(Path + "\\" + _FileName, CFILE_WRITE | CFILE_BINARY | CFILE_NODEFERCLOSE);
			File.Write(_lData.GetBasePtr(), _lData.Len());
			File.Close();
		}

	}
	catch(CCException _Ex)
	{
		// Corrupt Save
		//  We need to inform the player abt it!!
		ConOutL("§cf80WARNING: (CWorld_DeltaGameState::World_Save) Exception during save."); 	
		throw;
		//Error("CWorld_DeltaGameState::World_Save", _Ex.GetExceptionInfo().GetString());
	}
}
*/
	// ----------------------------------------------------------------------------
	// Provide the metadata image file.
	// Copy the image from the '\Xbox' catalog.
/*	if(CDiskUtil::FileExists(_SaveName + "saveimage.xbx"))
		CDiskUtil::DelFile(_SaveName + "saveimage.xbx");
	if(CDiskUtil::FileExists(_SaveName + "Padding.dat"))
		CDiskUtil::DelFile(_SaveName + "Padding.dat");

	CStr ImageSrc = CStr("D:\\XBox\\saveimage.xbx";
	CStr ImageDest = Path + "saveimage.xbx";
	CopyFile(ImageSrc, ImageDest, FALSE);

	// ----------------------------------------------------------------------------
	// Calculate the amount of blocks needed per file etc
	HANDLE hFile;
	DWORD dwSize;
	DWORD bytes1 = 0, bytes2 = 0, bytes3 = 0;

	hFile = CreateFile(Path + "saveimage.xbx", 0, GENERIC_ALL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		dwSize = GetFileSize (hFile, NULL); 
		CloseHandle(hFile);
		if(dwSize > 0)
			bytes1 = dwSize;
	}

	hFile = CreateFile(Path + "savemeta.xbx", 0, GENERIC_ALL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		dwSize = GetFileSize (hFile, NULL); 
		CloseHandle(hFile);
		if(dwSize > 0)
			bytes2 = dwSize;
	}

	hFile = CreateFile(Path + _Name, 0, GENERIC_ALL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		dwSize = GetFileSize (hFile, NULL); 
		CloseHandle(hFile);
		if(dwSize > 0)
			bytes3 = dwSize;
	}

	DWORD dwClusterSize = XGetDiskClusterSize("U:\\");
	DWORD dwApprCluster1 = ( 5120 + (dwClusterSize-1) ) / dwClusterSize;	// Image
	DWORD dwApprCluster2 = ( 1024 + (dwClusterSize-1) ) / dwClusterSize;	// Metafile
	DWORD dwApprCluster3 = ( 32768 + (dwClusterSize-1) ) / dwClusterSize;	// Savegame

	DWORD dwRealCluster1 = ( bytes1 + (dwClusterSize-1) ) / dwClusterSize;	// Image
	DWORD dwRealCluster2 = ( bytes2 + (dwClusterSize-1) ) / dwClusterSize;	// Metafile
	DWORD dwRealCluster3 = ( bytes3 + (dwClusterSize-1) ) / dwClusterSize;	// Savegame

	DWORD diffCluster = (dwApprCluster1 - dwRealCluster1) + (dwApprCluster2 - dwRealCluster2) + (dwApprCluster3 - dwRealCluster3);

	// ----------------------------------------------------------------------------
	// Write padding file to keep size constant
	{
		CCFile File;
		File.OpenExt(Path + "Padding.dat", CFILE_WRITE | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0, -1, ClusterSize);

		uint32 PadSize = (diffCluster * dwClusterSize);
		TArray<uint8> lBuf;
		lBuf.SetLen(PadSize);

		File.Write(Buf);
		File.Close();
	}

	// 1 cluster is currently 16 kb
	// The title require 4 clusters(blocks):
	//		1 cluster for the options		(500b)
	//		1 cluster for the titlemeta		(50b)
	//		1 cluster for the titleimage	(10kb)
	//		1 cluster for the directory	

	// Each save require 6 clusters(blocks):
	//		1 cluster for the signature	(20b)
	//		1 cluster for the directory
	//		1 cluster for the image		(4kb)
	//		1 cluster for the metafile	(20b)
	//		2 cluster for the savegame	(32kb)
*/


// Helper class to fill in correct size of a saved object
struct SizeWriter
{
	CCFile* m_pF;
	fint m_iPos;

	SizeWriter(CCFile* _pF)
	{
		m_pF = _pF;
		m_iPos = _pF->Pos();
		_pF->WriteLE(uint32(0));
	}

	~SizeWriter()
	{
		fint iCurr = m_pF->Pos();
		uint32 nSize = uint32(iCurr - m_iPos) - sizeof(uint32);
		M_ASSERT(nSize < 10000, "Save data size overflow!");
		m_pF->Seek(m_iPos);
		m_pF->WriteLE(nSize);
		m_pF->Seek(iCurr);
	}
};


TArray<uint8> CWorld_DeltaGameState::CreateDeltaSave(CWorld_ServerCore* _pCore)
{
	TArray<uint8> lSaveGame;
	spCCFile spFileMem = CDiskUtil::CreateCCFile(lSaveGame, CFILE_WRITE | CFILE_BINARY);
	CDataFile DataFile;
	DataFile.Create(spFileMem, CDATAFILE_VERSION, EDataFileFlag_NoHidePos);

	CCFile *pF = DataFile.GetFile();
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pCore->Selection_AddAll(Selection);
	const int16 *lpObj;
	int nObj = _pCore->Selection_Get(Selection, &lpObj);

//	m_lPrecacheInfoNames.Clear();
//	m_lPrecacheInfoTypes.Clear();

	// All orig-objects that no longer exists are stored in the 'DELETED' chunk
	{
		DataFile.BeginEntry("DELETED");
		uint nDeleted = 0;
		uint nOrgSpawned = m_lSpawnedOrg.Len();
		for (uint i = 0; i < nOrgSpawned; i++)
		{
			uint32 GUID = m_lSpawnedOrg[i];
			CWObject* pObj = _pCore->Object_GetWithGUID(GUID);
			if (!pObj || pObj->IsDestroyed())
			{
				VERBOSE_OUT(CStrF("[WDeltaGameState] Writing DELETED, GUID: %08X", GUID));
				pF->WriteLE(GUID);
				nDeleted++;
			}
		}
		DataFile.EndEntry(nDeleted);
	}

	{
		// Save originally spawned objects
		// These objects are remnants from world-spawn and their object-ID's should 
		// match exactly when we are loading a delta-save after another world-spawn.
		DataFile.BeginEntry("ORGOBJECTS");
		int nSaved = 0;
		for (int i = 0; i < nObj; i++)
		{
			int16 iObject = lpObj[i];
			CWObject* pObj = _pCore->Object_Get(iObject);
			if (pObj && !pObj->IsDestroyed() && !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) && pObj->m_bOriginallySpawned && !pObj->m_bNoSave)
			{
				uint32 GUID = _pCore->Object_GetGUID(iObject);
				nSaved++;
				VERBOSE_OUT(CStrF("[WDeltaGameState] Saving ORGOBJECT, GUID: %08X, Index: %i, Type: %s, Name: '%s'", GUID, iObject, typeid(*pObj).name(), pObj->GetName()));
				{
					pF->WriteLE(GUID); //TODO: Remove (not needed, used for debug)
					SizeWriter x(pF);  // will fill in the final size of this block (excluding GUID and size itself)
					MACRO_WRITEVERIFYTYPE(pObj);
					pObj->OnDeltaSave(pF);
				}
				MACRO_WRITEVERIFY;
			}
		}

		// Store all inactive objects as well    (objects that belong to a different spawn mask)
		TAP<SInactiveObj> pInactive = m_lInactiveOrg;
		for (uint i = 0; i < pInactive.Len(); i++)
		{
			uint32 GUID = pInactive[i].m_GUID;
			uint32 Size = pInactive[i].m_Data.Len();
			VERBOSE_OUT(CStrF("[WDeltaGameState] Saving inactive ORGOBJECT, GUID: %08X, Size: %d", GUID, Size));
			pF->WriteLE(GUID);
			pF->WriteLE(Size);
			pF->Write(pInactive[i].m_Data.GetBasePtr(), Size);
			nSaved++;
			MACRO_WRITEVERIFY;
		}
		DataFile.EndEntry(nSaved);
	}

	{
		// Save objects created after spawn
		// We assume that these are created from a class/template, and
		// not from a registry (too slow to spawn runtime), and therefore
		// we save the templatename that will be needed fo re-creation.
		DataFile.BeginEntry("NEWOBJECTS");
		//pF->WriteLE(_pCore->m_NextGUID);

		int nSaved = 0;
		int32 iPlayerObj = -1;
		int32 iPlayerGUID = -1;
		for (int i = 0; i < nObj; i++)
		{
			int iObject = lpObj[i];
			CWObject *pObj = _pCore->Object_Get(iObject);
			if (pObj)
			{
				if (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
				{
					iPlayerObj = iObject;
					iPlayerGUID = _pCore->Object_GetGUID(iPlayerObj);
				}
				else if (!pObj->m_bOriginallySpawned && !pObj->m_bNoSave && !pObj->IsDestroyed())
				{
					uint32 GUID = _pCore->Object_GetGUID(iObject);
					uint32 SpawnMask = pObj->OnMessage(CWObject_Message(OBJMSG_GETSPAWNMASK));
					nSaved++;
					VERBOSE_OUT(CStrF("[WDeltaGameState] Saving NEWOBJECT, GUID: %08X, Type: %s, Name: '%s'", GUID, typeid(*pObj).name(), pObj->GetName()));
					{
						pF->WriteLE(GUID);
						pF->WriteLE(SpawnMask);
						SizeWriter x(pF);  // will fill in the final size of this block (excluding GUID, SpawnMask and size itself)

						if (pObj->GetTemplateName() != NULL)
							CFStr(pObj->GetTemplateName()).Write(pF);
						else
							_pCore->GetMapData()->GetResourceName(pObj->m_iClass).Del(0, 4).Write(pF);
						CMat4Dfp32 Mat = pObj->GetPositionMatrix();
						Mat.Write(pF);

						pObj->OnDeltaSave(pF);
					}
					MACRO_WRITEVERIFY;
				}
			}
		}

		// Store all inactive objects as well    (objects that belong to a different spawn mask)
		TAP<SInactiveObj> pInactive = m_lInactiveNew;
		for (uint i = 0; i < pInactive.Len(); i++)
		{
			uint32 GUID = pInactive[i].m_GUID;
			uint32 SpawnMask = pInactive[i].m_SpawnMask;
			uint32 Size = pInactive[i].m_Data.Len();
			VERBOSE_OUT(CStrF("[WDeltaGameState] Saving inactive NEWOBJECT, GUID: %08X, SpawnMask: %08X, Size: %d", GUID, SpawnMask, Size));
			pF->WriteLE(GUID);
			pF->WriteLE(SpawnMask);
			pF->WriteLE(Size);
			pF->Write(pInactive[i].m_Data.GetBasePtr(), Size);
			nSaved++;
			MACRO_WRITEVERIFY;
		}

		pF->WriteLE(iPlayerObj);
		pF->WriteLE(iPlayerGUID);
		
		DataFile.EndEntry(nSaved);
	}

/*	{
		DataFile.BeginEntry("PRECACHEINFO");
		int nPrecacheInfo = m_lPrecacheInfoNames.Len();
		for(int i = 0; i < nPrecacheInfo; i++)
		{
			pF->WriteLE(m_lPrecacheInfoTypes[i]);
			m_lPrecacheInfoNames[i].Write(pF);
		}
		DataFile.EndEntry(nPrecacheInfo);
	}*/

	{
		// Save physics engine state  (not byteorder safe)
		DataFile.BeginEntry("PHYS_CONSTRAINTS");
		TAP<const CWD_ConstraintDescriptor> pConstraints = _pCore->m_lConstraints;
		uint nConstraints = 0;
		for (uint i = 0; i < pConstraints.Len(); i++)
		{
			if (pConstraints[i].GetID() < 0)
				continue;

			uint32 SubA, SubB;
			pConstraints[i].GetConnectedSubObjects(SubA, SubB);
			if ((SubA != 0xFFFF) || (SubB != 0xFFFF))  // Constraints with subobjects need to be recreated
				continue;

			pConstraints[i].Write(pF, _pCore);
			nConstraints++;
		}
		DataFile.EndEntry(nConstraints);
		VERBOSE_OUT(CFStrF("Saved %d constraints (%d skipped)", nConstraints, pConstraints.Len() - nConstraints ));
	}

	DataFile.Close();
	int Size = spFileMem->Pos();
	spFileMem->Close();
	lSaveGame.SetLen(Size);

//	TArray<uint8> Array = CDiskUtil::Compress(lSaveGame, LZSS_HUFFMAN);
	TArray<uint8> Array = lSaveGame;
	Array.OptimizeMemory();
	return Array;
//	return lSaveGame;

//	m_lPrecacheInfoNames.Clear();
//	m_lPrecacheInfoTypes.Clear();
}

