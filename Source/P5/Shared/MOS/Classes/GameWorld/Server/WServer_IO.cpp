#include "PCH.h"
#include "WServer_Core.h"

CRegistryDirectory *CWorld_ServerCore::World_LoadObjects(CStr _FileName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_LoadObjects, TArray<spCRegistry>());
	MSCOPE(CWorld_ServerCore::World_LoadObjects, WORLD_SERVER);

/*
	_FileName = m_spWData->ResolveFileName("WORLDS\\" + _FileName);
	//ConOutL("(CWorld_ServerCore::World_LoadObjects) Spawning objects from " + _FileName);

	CDataFile DFile;
	DFile.Open(_FileName);

	// Read Object-Attributes
	if (!DFile.GetNext("OBJECTATTRIBS"))
		Error("ReadWorld", "No OBJECTATTRIBS entry.");

	int nObjects = DFile.GetUserData();
	int Version = DFile.GetUserData2();
	TArray<spCRegistry> lspObjects;
	lspObjects.SetLen(nObjects);
	for(int i = 0; i < nObjects; i++)
	{
		spCRegistry spObj = REGISTRY_CREATE;
		//		spCWObjectAttributes spObj = DNew(CWObjectAttributes) CWObjectAttributes;
		if (spObj == NULL) MemError("Read");
		spObj->Read(DFile.GetFile(), Version);
		lspObjects[i] = spObj;
	}

	DFile.Close();

	return lspObjects;
*/
	int iRc = m_spMapData->GetResourceIndex_ObjectAttribs(_FileName);
	return m_spMapData->GetResource_ObjectAttribs(iRc);
}
/*
void CWorld_ServerCore::World_SpawnObjectsFromFile(CCFile* _pF, int _nObjects, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnObjectsFromFile, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SpawnObjectsFromFile, WORLD_SERVER);

	//	ConOutL("(CWorld_ServerCore::World_SpawnObjectsFromMap) Spawning objects from file.");
	for(int32 i = 0; i < _nObjects; i++)
	{
		int32 Pos = _pF->Pos();
		int32 FPos; _pF->ReadLE(FPos);
		if (Pos != FPos) Error("World_Load", CStrF("Object serialization out of sync on object %d/%d, Offset %d != %d", i, _nObjects, FPos, Pos));
		LogFile(CStrF("Loading object %d", i));
		CStr Class;
		Class.Read(_pF);
		LogFile(CStr("Class ") + Class);

		int iObj = m_spObjectHeap->AllocID();
		if (iObj == -1) Error("World_Load", "Unable to allocate object.");

		CWObject* pObj = m_spMapData->CreateObject(Class);
		if (!pObj)
		{
			m_spObjectHeap->FreeID(iObj);
			ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::World_Load) Could not create object of class %s", (char*) Class));
			continue;
		}
		m_lspObjects[iObj] = pObj;
		pObj->m_pWServer = this;
		pObj->m_iObject = iObj;
		//		pObj->OnCreate();
		pObj->OnLoad(_pF);
		if (!Object_SetPosition(iObj, pObj->m_Pos))
		{
			ConOutL(CStrF("§cf80WARNING: Failed to set object-position (%d, %s, %s)", iObj, (char*)Class, (char*) pObj->GetPositionMatrix().GetString()));
		}

		// It is questionable if this call should be made.
		pObj->OnFinishEvalKeys();

		if (_pTransform)
		{
			CMat4Dfp32 NewPos;
			pObj->m_Pos.Multiply(*_pTransform, NewPos);
			pObj->m_Pos = NewPos;
			pObj->OnTransform(*_pTransform);
		}
	}
}

void CWorld_ServerCore::World_SpawnObjectsFromXSV(CStr _FileName, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnObjectsFromXSV, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SpawnObjectsFromXSV, WORLD_SERVER);

	CDataFile DFile;
	DFile.Open(_FileName);

	if (!DFile.GetNext("OBJECTS")) Error("World_SpawnObjectsFromXSV", "OBJECTS entry not found.");
	int32 nObjects = 0;
	DFile.GetFile()->ReadLE(nObjects);
	ConOutL(CStrF("(CWorld_ServerCore::World_SpawnObjectsFromXSV) %d objects.", DFile.GetUserData()));
	World_SpawnObjectsFromFile(DFile.GetFile(), nObjects, _pTransform);
	DFile.Close();
}
*/
/*
int CWorld_ServerCore::World_SaveObjects(CCFile* _pFile, CSelection& _Selection, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SaveObjects, 0);
	MSCOPE(CWorld_ServerCore::World_SaveObjects, WORLD_SERVER);

	const int16* piObj;
	int32 nObj = Selection_Get(Selection, &piObj);
	_pFile->WriteLE(nObj);

	CMat4Dfp32 TransformInv;
	if (_pTransform) _pTransform->InverseOrthogonal(TransformInv);

	// Save objects
	int nSave = 0;
	for(int i = 0; i < nObj; i++)
	{
		int iObj = piObj[i];
		CWObject* pObj = m_lspObjects[iObj];
		if (pObj)
		{
			// Write file position for error checking.
			int32 Pos = _pFile->Pos();
			_pFile->WriteLE(Pos);

			// Write classname
			m_spMapData->GetResourceName(pObj->m_iClass).Del(0, 4).Write(_pFile);

			CMat4Dfp32 ObjPos = pObj->m_Pos;
			if (_pTransform)
			{
				CMat4Dfp32 NewPos;
				pObj->m_Pos.Multiply(*_pTransform, NewPos);
				pObj->m_Pos = NewPos;;
				pObj->OnTransform(*_pTransform);
			}
			pObj->OnSave(_pFile);

			if (_pTransform)
			{
				pObj->OnTransform(TransformInv);
				pObj->m_Pos = ObjPos;
			}
			nSave++;
		}
	}

	return nSave;
}

void CWorld_ServerCore::World_Save(CStr _SaveName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Save, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_Save, WORLD_SERVER);
}


void CWorld_ServerCore::World_SpawnTransitZone(CStr _Name, const CMat4Dfp32& _Transform)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnTransitZone, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SpawnTransitZone, WORLD_SERVER);
	
	CStr Path = m_SavePath + "Current" + "\\";
	if (CDiskUtil::FileExists(Path + _Name + ".XTZ"))
		World_SpawnObjectsFromXSV(Path + _Name + ".XTZ", &_Transform);
	else
	{
		if (CDiskUtil::FileExists(m_spWData->ResolveFileName("WORLDS\\" + _Name + ".XTZ")))
			World_SpawnObjectsFromWorld(_Name + ".XTZ", &_Transform);
		else
			ConOutL("§cf80WARNING: Could not find transitzone " + _Name);
	}
}
*/

/*
void CWorld_ServerCore::World_LoadPlayers(CStr _FileName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_LoadPlayers, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_LoadPlayers, WORLD_SERVER);
	
	CDataFile DFile;
	DFile.Open(_FileName);
	CCFile* pF = DFile.GetFile();

	if (!DFile.GetNext("PLAYERS")) Error("World_LoadPlayers", "PLAYERS entry not found.");
	m_lspPlayers.Clear();
ConOutL(CStrF("(CWorld_ServerCore::World_LoadPlayers) %d players.", DFile.GetUserData()));
	for(int32 i = 0; i < DFile.GetUserData(); i++)
	{
		int32 iP;
		pF->ReadLE(iP);

	Error("World_LoadPlayers", "Out of order.");
	// FIXME: All elements in lspPlayers must be allocated.

		if (m_lspPlayers.Len() <= iP) m_lspPlayers.SetLen(iP+1);
		m_lspPlayers[iP]->m_Name.Read(pF);
		pF->ReadLE(m_lspPlayers[iP]->m_Flags);
//		pF->ReadLE(m_lPlayers[i].m_Frags);
//		m_lPlayers[i].m_Team.Read(pF);
	}
	DFile.Close();
}

void CWorld_ServerCore::World_Load(CStr _SaveName, CStr _WorldName, bool _bLoadPlayers, TArray<uint8> _Data)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Load, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_Load, WORLD_SERVER);
	
	ConOutL("(CWorld_ServerCore::World_Load) Loading save-game " + _SaveName);
	if (_WorldName != "") ConOutL("(CWorld_ServerCore::World_Load) World " + _WorldName);

	CHECKMEMORY("World_Load (1)");
	World_Close();
	World_CopySave(_SaveName, "Current");

	// Get world-name if none was specifed.
	if (_WorldName == "")
	{
		CStr Path = m_SavePath + _SaveName + "\\";
		if (CDiskUtil::FileExists(Path + "Saveinfo.XSI"))
		{
			CDataFile DFile;
			DFile.Open(Path + "SaveInfo" + ".XSI");
			if (!Registry_GetWorld()->Read(&DFile, "WORLDREGISTRY"))
				Error("World_Load", "Corrupt save-info file.");
			DFile.Close();

			Registry_OnReadWorldKeys();
			_WorldName = m_WorldName;
		}
		else
		{
			Error("World_Load", "No world-name was specified and save-folder did not contain a SaveInfo.XSI file.");
		}
	}
	

	CHECKMEMORY("World_Load (2)");
	try
	{
		CStr Path = m_SavePath + _SaveName + "\\";

		World_Init(_WorldName);
		m_ServerMode |= SERVER_MODE_SPAWNWORLD;

		CHECKMEMORY("World_Load (2b)");

		// Load players
		if (_bLoadPlayers) World_LoadPlayers(Path + _WorldName + ".XSV");

		CHECKMEMORY("World_Load (2c)");

		if (CDiskUtil::FileExists(Path + _WorldName + ".XSV"))
			World_SpawnObjectsFromXSV(Path + _WorldName + ".XSV");
		else
			World_SpawnObjectsFromWorld(_WorldName);

		World_DoOnSpawnWorld();

		CHECKMEMORY("World_Load (2d)");

		LogFile("Assigning players.");
		// Assign objects for all players.
		for(int i = 0; i < m_lspPlayers.Len(); i++)
		{
			if (m_lspPlayers[i]->m_Flags & PLAYER_FLAGS_PLAYER)
			{
				int iObj = Selection_GetSingleTarget(selection("PLAYER%d", i));
				Game_GetObject()->Player_SetObject(i, iObj);
				if (iObj < 0)
				{
					ConOutL(CStrF("§cf80WARNING: Could not find object for player %d", i));
//					Player_Respawn(i);
				}
			}
		}

		SetUpdate_All(SERVER_CLIENTUPDATE_WORLD);
		m_ServerMode &= ~SERVER_MODE_SPAWNWORLD;
	}
	catch(CCException)
	{
		ConOutL("(CWorld_ServerCore::CWorld_Load) Failed to load game.");
		World_Close();
		throw;
	}
	CHECKMEMORY("World_Load (3)");
}*/
/*
TArray<uint8> CWorld_ServerCore::World_CreateFullSave()
{
	TArray<uint8> lSaveGame;
	spCCFile spFileMem = CDiskUtil::CreateCCFile(lSaveGame, CFILE_READ | CFILE_BINARY);
	CDataFile DataFile;
	DataFile.Create(spFileMem);

	// Save players
	CCFile *pF = DataFile.GetFile();
	{
		DataFile.BeginEntry("PLAYERS");
		int nSaved = 0;
		for(int32 i = 0; i < m_lspPlayers.Len(); i++)
		{
			if (m_lspPlayers[i]->m_Flags & PLAYER_FLAGS_INUSE)
			{
				pF->WriteLE(i);
				m_lspPlayers[i]->m_Name.Write(pF);
				pF->WriteLE(m_lspPlayers[i]->m_Flags);
				if ((m_lspPlayers[i]->m_Flags & PLAYER_FLAGS_PLAYER) && Object_Get(m_lspPlayers[i]->m_iObject))
					Object_SetName(m_lspPlayers[i]->m_iObject, CStrF("PLAYER%d", i));
				nSaved++;
			}
		}
		DataFile.EndEntry(nSaved);
	}

	// -------------------------------------------------------------------
	// Assign names to all player-objects.
	{
		for(int32 i = 0; i < m_lspPlayers.Len(); i++)
		{
			if ((m_lspPlayers[i]->m_Flags & (PLAYER_FLAGS_INUSE | PLAYER_FLAGS_PLAYER)) == (PLAYER_FLAGS_INUSE | PLAYER_FLAGS_PLAYER))
			{
				if (Object_Get(m_lspPlayers[i]->m_iObject))
					Object_SetName(m_lspPlayers[i]->m_iObject, CStrF("PLAYER%d", i));
			}
		}
	}

	// -------------------------------------------------------------------
	// Save objects

	// Clear TZ index.
//	for(int i = 0; i < m_lspObjects.Len(); i++)
//	{
//		CWObject* pObj = Object_Get(i);
//		if (pObj) pObj->m_iTZObject = 0;
//	}

	// Get all transitzones.
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		{
			Selection_AddClass(iSel, "Trigger_TransitZone");

			// Go through all TZs.

			// An important mechanic with this two-pass approach is that no object can be stored twice even 
			// if it should be a member of two TZs. We also get to know what objects are not a member of any TZ 
			// and should go into the level's savegame.

			const int16* piObj = NULL;
			int nObj = Selection_Get(Selection, &piObj);
			ConOutL(CStrF("CWorld_ServerCore::World_Save) %d transitzones.", nObj));
			{
				for(int i = 0; i < nObj; i++)
				{
					// Get all objects intersecting the TZ and set the iTZObject member.
					int iTZObj = piObj[i];
					CWObject* pObj = Object_Get(iTZObj);
					if (!pObj) continue;
					TSelection<CSelection::LARGE_BUFFER> Selection;
					{
						Selection_AddOriginInside(iSel, pObj->GetPositionMatrix(), pObj->GetPhysState());
						Selection_RemoveOnClass(iSel, "Trigger_TransitZone");
						Selection_RemoveOnClass(iSel, "Worldspawn");
						Selection_RemoveOnClass(iSel, "Trigger_ChangeWorld");

						const int16* piObj = NULL;
						int nObj = Selection_Get(Selection, &piObj);
						ConOutL(CStrF("CWorld_ServerCore::World_Save) Tagging %d objects for transitzone %d.", nObj, iTZObj));
						for(int i = 0; i < nObj; i++)
							Object_Get(piObj[i])->m_iTZObject = iTZObj;
					}
				}
			}

			// Go through all TZs.
			{
				for(int i = 0; i < nObj; i++)
				{
					int iTZObj = piObj[i];
					CStr Name = Object_GetName(iTZObj);
					if (Name == "")
					{
						ConOutL("§cf80WARNING: (CWorld_ServerCore::World_Save) Trigger_TransitZone with no targetname.");
						continue;
					}

					// Get TZ transform.
					CMat4Dfp32 TZPos = Object_GetPositionMatrix(iTZObj);
					CMat4Dfp32 TZTransform;
					TZPos.InverseOrthogonal(TZTransform);

					// Create selection with matching iTZObject and save it.
					TSelection<CSelection::LARGE_BUFFER> Selection;
					{
						Selection_AddTransitZone(iSel, iTZObj);

						Error("World_SaveFull", "Transitzones are broken");
						ConOutL(CStrF("(CWorld_ServerCore::World_Save) Saving %s, %d objects.", (char*) (Path + Name + ".XTZ"), Selection_Get(Selection, NULL)));
						CDataFile DataFile;
						DataFile.Create(Path + Name + ".XTZ");
						DataFile.BeginEntry("OBJECTS");
						int nSave = World_SaveObjects(DataFile.GetFile(), iSel, &TZTransform);
						DataFile.EndEntry(nSave);
						DataFile.Close();
					}
				}
			}

		}
	}


	DataFile.BeginEntry("OBJECTS");
	int nSave = 0;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	{
		Selection_AddTransitZone(iSel, 0);		// Select all objects that was not saved in an XTZ.
//		Selection_AddAll(iSel);
		nSave = World_SaveObjects(pF, iSel);
	}

	DataFile.EndEntry(nSave);
	DataFile.Close();
	spFileMem->Close();

	return lSaveGame;
}

// --------------------------------------------------------------------
// Func: World_Load()
// Desc: Corresponds to World_Save() and loads the data save by that 
//       function, and thereby loads a game.
// --------------------------------------------------------------------
void CWorld_ServerCore::World_Load(CStr _SaveName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Load, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_Load, WSERVERMOD);
	if(m_GameType.CompareNoCase("campaign") == -1)
		return;

	//	World_Close();

	try
	{
		int32 nLoadDataLength;
		TArray<uint8> lLoadData;
		CStr Path;

		m_NextGameType = "Campaign";
		World_Change("Campaign", 0);

		if(_Data.Len() > 0)
		{
			nLoadDataLength = _Data.Len();
			lLoadData = _Data;
		}
		else
		{
#if defined(PLATFORM_XBOX)
			Path = XTL_OpenSaveDir(m_SavePath, _SaveName);
			if(Path.Len() <= 0) 
				Error("World_Load", "Save-folder did not exist.");
			Path += "\\";
#else
			Path = m_spWData->ResolvePath("Save\\") + _SaveName + "\\";
#endif
			if(!CDiskUtil::FileExists(Path + "gamestate.xrg"))
				Error("World_Load", "Save-folder did not contain a default.xrg file.");

			CCFile File;
			File.Open(Path + "gamestate.xrg", CFILE_READ | CFILE_BINARY, RLE);
			nLoadDataLength = File.Length();
			lLoadData.SetLen(nLoadDataLength);
			File.ReadLE(lLoadData.GetBasePtr(), nLoadDataLength);
			File.Close();
		}

#if defined(PLATFORM_XBOX)
		HANDLE hCalcSig = XCalculateSignatureBegin(NULL);
		DWORD res = XCalculateSignatureUpdate(hCalcSig, lLoadData.GetBasePtr(), nLoadDataLength);
		XCALCSIG_SIGNATURE Signature;
		res =  XCalculateSignatureEnd(hCalcSig, &Signature);

		CCFile FileSig;
		FileSig.Open(Path + "savesig", CFILE_READ | CFILE_BINARY);
		if (FileSig.Length() < XCALCSIG_SIGNATURE_SIZE)
			Error("World_Load", "Corrupt signature file.");

		uint8 val;
		for(int k=0; k<XCALCSIG_SIGNATURE_SIZE ;k++)
		{
			FileSig.Read(val);
			if(val != Signature.Signature[k])
			{
				// Signature missmatch
				FileSig.Close();
				ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::World_Load) Signature %d missmatch. (%.8x != %.8x)", k, val, Signature.Signature[k]));
				Error("World_Load", "Signature missmatch.");
			}
		}

		uint32 PadSize = (1 - (nLoadDataLength / 16384)) * 16384;

		if (FileSig.Length() != XCALCSIG_SIGNATURE_SIZE + PadSize)
			Error("World_Load", "Corrupt signature file.");

		for(int i = 0; i < PadSize; i++)
		{
			uint8 Byte;
			FileSig.ReadLE(Byte);
			if (Byte != 0)
				Error("World_Load", "Corrupt signature file.");
		}

		FileSig.Close();

#elif defined(PLATFORM_WIN_PC)
		// Decrypt using Linux's memfrob() algorithm, cheesy but keeps the normal users from tampering with it. 
		// Also calculate the CRC-16 checksum for the savegame (used for error checking)
		CCrcCheck crc16;
		uint8 checksum_calc[CHECKSUM_BYTELENGTH];
		int i;

		for(i=0; i<nLoadDataLength - CHECKSUM_BYTELENGTH;i++)
		{
			lLoadData.GetBasePtr()[i] ^= 42;			// Decrypt
			crc16.UpdateCrc(lLoadData.GetBasePtr()[i]); // Calculate checksum
		}	

		// Compare the calculated checksum with the one written last in the savegame
		crc16.GetChecksumList(checksum_calc);
		int checksum_start = i;
		for(i=0; i<CHECKSUM_BYTELENGTH;i++)
		{
			if(checksum_calc[i] != lLoadData.GetBasePtr()[checksum_start + i]) 
			{
				ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::World_Load) Signature %d missmatch. (%.8x != %.8x)", i, checksum_calc[i], lLoadData.GetBasePtr()[checksum_start + i]));
				Error("World_Load", "Corrupt signature for save game.");
			}
		}

		// Delete the checksum from the 'stream'
		lLoadData.Delx(nLoadDataLength - CHECKSUM_BYTELENGTH, CHECKSUM_BYTELENGTH);
#endif

		CCFile FileMem;
		FileMem.Open(lLoadData, CFILE_READ | CFILE_BINARY);
		int iGame = Selection_GetSingleTarget(Selection);
		if(iGame)
		{
			CWObject *pGameObj = m_lspObjects[iGame];
			pGameObj->OnLoad(&FileMem);
		}

		FileMem.Close();

		//		SetUpdate_All(SERVER_CLIENTUPDATE_WORLD);
	}
	catch(CCException _Ex)
	{
		//  We need to inform the player abt it!!
		ConOutL("(CWorld_Server::CWorld_Load) Failed to load game.");
		World_Close();
		throw;
	}
}
*/
