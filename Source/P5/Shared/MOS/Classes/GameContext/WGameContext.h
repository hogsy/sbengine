#ifndef __WGAMECONTEXT_H
#define __WGAMECONTEXT_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			GameContext base class

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CGameContext
\*____________________________________________________________________________________________*/

#include "../../MOS.h"
#include "../GameWorld/WMapData.h"
#include "../GameWorld/Client/WClient.h"
//#include "../GameWorld/FrontEnd/WFrontEnd.h"
#include "../Video/MVideo.h"
#include "../../MSystem/Misc/MRegistry_Compiled.h"
#include "../../Shared/mos/Classes/GameWorld/Client/WClient_Sound.h"

#define WMODE_SERVER		1
#define WMODE_LOCALCLIENT	2
#define WMODE_REMOTECLIENT	4
#define WMODE_DEMO			8
#define WMODE_LOADING		16
#define WMODE_GAMELOADED	32
#define WMODE_TIMEDEMO		64

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CGameContext

	Comments:		GameContext base class	
\*____________________________________________________________________*/
class CGameContext : public CConsoleClient
{
public:
	enum
	{
		CGC_COMMAND_DISCONNECT = 0,
		CGC_COMMAND_MAP,
		CGC_COMMAND_CONNECT,
		CGC_COMMAND_SETGAME,
		CGC_COMMAND_LOADGAME,
		CGC_COMMAND_FREERESOURCES,
		CGC_COMMAND_SCRIPT,
		CGC_COMMAND_CHANGEMAP,
		CGC_COMMAND_SCRIPTGRABSCREEN,
		CGC_COMMAND_PLAYCUTSCENESOUND,
		CGC_COMMAND_TIMELEAP,

		CGC_COMMAND_LAST,
	};

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class:			CGC_Command

		Comments:		GameContext command class for deferring commands
	\*____________________________________________________________________*/
	class CGC_Command
	{
	public:
		int m_Command;
		CStr m_lParams[4];
		CFStr m_lFParam[2];
		TArray<uint8> m_Param;
		
		CGC_Command() { };

		CGC_Command(const CGC_Command& ToCopy)
		{
			m_Command = ToCopy.m_Command;
			m_lParams[0] = ToCopy.m_lParams[0];
			m_lParams[1] = ToCopy.m_lParams[1];
			m_lParams[2] = ToCopy.m_lParams[2];
			m_lParams[3] = ToCopy.m_lParams[3];
			m_lFParam[0] = ToCopy.m_lFParam[0];
			m_lFParam[1] = ToCopy.m_lFParam[1];
			m_Param = ToCopy.m_Param;
		}

		CGC_Command(int _Command)
		{
			m_Command  = _Command;
		}

		CGC_Command(int _Command, CStr _Param0)
		{
			m_Command  = _Command;
			m_lParams[0] = _Param0;
		}

		CGC_Command(int _Command, CStr _Param0, CStr _Param1)
		{
			m_Command  = _Command;
			m_lParams[0] = _Param0;
			m_lParams[1] = _Param1;
		}

		CGC_Command(int _Command, CStr _Param0, CStr _Param1, CStr _Param2)
		{
			m_Command  = _Command;
			m_lParams[0] = _Param0;
			m_lParams[1] = _Param1;
			m_lParams[2] = _Param2;
		}

		CGC_Command(int _Command, CStr _Param0, CStr _Param1, CStr _Param2, CStr _Param3)
		{
			m_Command  = _Command;
			m_lParams[0] = _Param0;
			m_lParams[1] = _Param1;
			m_lParams[2] = _Param2;
			m_lParams[3] = _Param3;
		}

		CGC_Command(int _Command, CStr _Param0, TArray<uint8> &_Param1)
		{
			m_lParams[0] = _Param0;
			m_Command = _Command;
			m_Param = _Param1;
		}
	};

	CSystem* m_pSystem;
	CStr m_WorldPathesAbs;
	CStr m_WorldPathesRel;
	TArray<CStr> m_lWorldPathes;
	CStr m_DefaultGameName;
	CStr m_GameName;
	spCWorldData m_spWData;
	spCRegistry m_spGameReg;
	spCXR_Engine m_spEngine;
#ifdef M_ENABLE_REGISTRYCOMPILED
	TArray<TPtr<CRegistryCompiled> > m_lspRegCompiledStringTables;
#endif
	
	MRTC_CriticalSection m_PendingCommandLock;
	TArray<CGC_Command> m_lPendingCommands;

	fp32 m_Viewport3DScaleX;
	fp32 m_Viewport3DScaleY;

	CMTime m_SimulateTime;			// Time in s.
	CMTime m_LastSimulateTime;		// Time in s.

	fp32 m_SimulateCPUTimeCurrent;
	fp32 m_SimulateCPUTimeAverage;
	fp32 m_SimulateCPUTimeAveragePrim;
	int m_nSplitScreens;
	int m_hListenConnection;
	int m_hServerSocket;
	int m_Mode;
	
	spCSoundContext m_spSoundContext;
	fp32 m_VolumeAmbient;
	fp32 m_VolumeSfx;
	fp32 m_VolumeVoice;
	fp32 m_VolumeFrontEnd;

	TArray<CStr> m_lDemos;
	int m_iDemo;

	CMTime m_TimeDemoFrameTimer;
	int m_nTimeDemoFramesCaptured;
	TArray<fp32> m_TimeDemoFrameTimes;

	fp32 m_TimeDemoRate;
	CStr m_TimeDemoCapturePath;
	spCVideoFile m_spTimeDemoFile;
	spCImage m_spTimeDemoCapture;
	spCImage m_spTimeDemoConv;

	int m_nTimeDemoFramesPlayed;
	int m_nTimeDemoMaxFrames;

	uint m_bTimeDemoGraph : 1;
	uint m_bStartCallGraphPending : 1;
	uint m_bStopCallGraphPending : 1;
	uint m_bSimulateFractional : 4;
	uint m_bLoadedGameDll : 1;
	uint m_bBooting : 1;
	uint m_bSplitAxis : 1;
	uint m_bVirtualWideScreen : 1;
	uint m_bTimeLeap : 1;
	uint m_bBenchmark : 1;
	uint m_bSoakMode : 1;

	uint m_nSoakCount;

	fp32 m_WideScreen;

	CMTime m_TimeDemoStart;

	CStr m_DLLName;
	CStr m_ServerClass;
	CStr m_ClientClass;
	CStr m_ClientGameClass;

	spCNetwork_Device m_spNetDevice;
	CStr m_NetDeviceSettings;

	CStr m_CurrentProfile;
	CStr m_CurrentProfilePath;
	bool m_bValidProfileLoaded;

	CStr m_PendingSetGameWorldPathes;
	CStr m_PendingSetGameGameName;

	// FPS counter and stats settings
/*	int8 m_bShowFPS : 1;
	int8 m_bSimpleFPS : 1;
	int8 m_bShowStats : 1;*/

public:
	int GetMode() { return m_Mode; }
	TPtr<class CWorld_Server> m_spWServer;
	TArray<spCWorld_Client> m_lspWClients;
	TPtr<class CWFrontEnd> m_spFrontEnd;
	spCNetwork m_spNetwork;
	MRTC_CriticalSection m_ServerDeleteLock;

	CGameContext();
	~CGameContext();

	CStr ResolveFileName(CStr _FileName);
	CStr ResolvePath(CStr _FileName);

	virtual void ExecuteCommand(CGC_Command& _Cmd);
	virtual void AddPendingCommand(const CGC_Command& _Cmd);
	void FlushPendingCommands();

	virtual void Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine);

	virtual void CreateResourceManager();
	virtual void CreateGameRegistry(CStr _GameName);
	virtual void CreateStringTables(CStr _StringTables);

	virtual uint32 GetViewFlags() {return 0;}
	virtual fp32 DetermineWidescreen();

	bool SetGame(CStr _WorldPathes, CStr _GameName);
	bool SafeSetGame(CStr &_WorldPathes, CStr &_Game);

	bool CreateClientGame();
	void ChangeWorld(CStr _FileName, int _Flags = 0);

	void InitServer(bint _bInitNetwork = 0, CStr _Settings = "");

	void DestroyClients();
	virtual void OnDisconnect(){}
	virtual void InitLocalGame();
	void Disconnect(bool _bDestroyNet = true);
	void Connect(CStr _Address);
	virtual CWorld_Client* GetCurrentClient();

	virtual void UpdateControllerStatus() {};
	virtual void SetDefaultProfileSettings(CRegistry* _pOptions) {};

protected:
	CMTime m_PauseTime;
	int m_CutsceneStatus;

public:
	int m_PauseCount;
	bool m_bSingleStep;

	void StartSimulation();
	void StopSimulation();
	void Simulate();
	virtual void Simulate_Pause();
	void Simulate_Resume();

	void SetSoundContext(spCSoundContext _spSoundContext);
	void SetVolume(fp32 _VolumeAmbient, fp32 _VolumeSfx, fp32 _VolumeVoice, fp32 _VolumeFrontEnd);
	

	// Passing "" as profile to any of these functions will result in the usage of the options registry
	//
	dllvirtual void FixProfile(CStr &_rProfile); // replaces "" to the value of GAME_PROFILE

	dllvirtual bool CreateProfile(CStr _Profile);
	dllvirtual bool DeleteProfile(CStr _Profile);
	dllvirtual int32 EnumProfiles(TArray<CStr> &_rProfiles);
	dllvirtual bool ValidateProfile(CStr _Profile);
	dllvirtual CStr GetProfilePath(CStr _Profile);
	dllvirtual bool LoadLastValidProfile(void);
	
	dllvirtual bool FillProfile(CStr _Profile);
	dllvirtual bool RemoveProfileFiller(CStr _Profile);

	dllvirtual int32 EnumSaveFiles(CStr _Profile, TArray<CStr> &_rSaveFiles, bool _bAll = false);
	dllvirtual bool ValidateSaveFile(CStr _Profile, CStr _Name);
	dllvirtual bool DeleteSaveFile(CStr _Profile, CStr _Name);

	CMTime m_LastWriteTime;

	CCFile m_CurrentSaveFile;
	CStr m_SaveInfoString;
	CStr m_CurrentSaveProfile;
	CStr m_CurrentSaveFilename;
	TArray<CStr> m_lSaveFiles;

	class CSaveContext : public CReferenceCount
	{
	public:
		class CAsyncOptions : public CFileAsyncWriteFillDirOptions
		{
		public:
			void InitFile(mint _Size)
			{
				*((uint32 *)m_pFillData) = _Size;
			}
		};

		CAsyncOptions m_AsyncOptions;
		char m_Data[16*1024];
		CFStr m_Directory;
		CFStr m_CalcSizeSearchString;
		spCWorldData m_spWData;

		CSaveContext(CWorldData *_pWData)
		{
			m_spWData = _pWData;
			m_spWData->Resource_AsyncCacheDisable(M_Bit(16));
			memset(m_Data, 0x80, 16*1024);
			m_AsyncOptions.m_pFillData = m_Data;
			m_AsyncOptions.m_pDirectory = m_Directory;
			m_AsyncOptions.m_pCalcSizeSearchString = m_CalcSizeSearchString;
			m_AsyncOptions.m_pFillFileName = "_filler";
#ifdef PLATFORM_PS3
			//JK-FIX: This is temporary since filewriting is horrible slow
			m_AsyncOptions.m_FillDataSize = 16*1024;
			m_AsyncOptions.m_FillSize = 0;
#else
			m_AsyncOptions.m_FillDataSize = 16*1024;
			m_AsyncOptions.m_FillSize = 100*(16*1024); // SAVEGAMEMAXSIZE
#endif
			m_AsyncOptions.m_PerFileSize = m_AsyncOptions.m_FillSize;
		}		

		~CSaveContext()
		{
			m_spWData->m_AsyncWriter.BlockUntilDone();
			m_spWData->Resource_AsyncCacheEnable(M_Bit(16));
		}
	};


	TPtr<CSaveContext> m_spAsyncSaveContext;

	TArray<TArray<CFileAsyncWriteChunk> > m_llChunks;
	TArray<CFileAsyncWriteChunk> m_lPendingChunks;
	TArray<uint8*> m_lCRCData;

	CStr m_SaveProfile;
	TArray<uint8> m_lProfileInfo;
	virtual bool WriteProfileInfo(CStr _Profile);


	dllvirtual bool BeginWriteSaveFile(CStr _Profile, CStr _Name);
	dllvirtual bool WriteSaveFileBlock(const uint8 *_pData, int32 _Size, bool _UpdateCrc = true);
	dllvirtual bool EndWriteSaveFile(bool _WriteFiller = true);

	dllvirtual bool BeginReadSaveFile(CStr _Profile, CStr _Name, bool _Validate=true);
	dllvirtual bool EndReadSaveFile();
	dllvirtual bool ReadWholeSaveFile(CStr _Profile, CStr _Name, TArray<uint8> &_rData);


	dllvirtual bool ReadSaveFileToReg(CStr _Profile, CStr _Name, CRegistry *_pReg);

	/*
	dllvirtual bool ValidateSave(spCWorldData _pWData, CStr _Savename);
	dllvirtual void DeleteSave(CStr _Savename);
	dllvirtual bool ReadTranscript(spCWorldData _pWData, CStr _SaveName, TArray<CStr> &_lFiles);
	dllvirtual void WriteTranscript(spCWorldData _pWData, CStr _SaveName, TArray<CStr> &_lFiles);
	dllvirtual void WriteSaveFile(spCWorldData _pWData, CStr _SaveName, CStr _FileName, TArray<uint8> _lData, bool _bCryptate);
	dllvirtual CStr GetSavePath(spCWorldData _pWData, CStr _SaveName);
	dllvirtual bool ReadSaveFile(spCWorldData _pWData, CStr _SaveName, CStr _FileName, TArray<uint8> &_lData, bool _bCryptate);
	*/

	virtual void DoLoadingStream(){};
	bool CanRenderGame();
	bool ProcessKey(const CScanKey& _Key);
	
	static void GetViewport(CSystem* _pSystem, CRC_Viewport* _vp, int _type, CVec2Dfp32 _Scale = 1.0f, bool _bVirtualWideScreen = false, fp32 _WideScreen = 1.0);
	void GetViewport(CRC_Viewport* _vp, int _type);

	virtual void RenderClient(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _GUIVP,  CWorld_Client* _pC, int _Context);
	virtual void RenderAllClients(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void RenderSplitScreenGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, bool _bVSplit, bool _bHSplit);
//	virtual void RenderFPSCounter(CRenderContext* _pRC, CRC_Util2D* _Util2D, const CClipRect& _ClipVP, fp32 _FPS, fp32 _AvgFPS, const char* _Win32MemoryStatusStr); 
	virtual void Render_Corrupt(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);

	virtual void Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void RenderClients(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void RenderGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, int _Context);
	virtual void Refresh(class CXR_VBManager* _pVBM);
	virtual void PrecachePerform(CRenderContext* _pRC);

	bool GameRunning();
protected:
	void PlayDemo(CStr _Name, bool _bTimeDemo);
	void Capture(CStr _Name, int _FPS);

public:
	// These are run via FlushPendingCommands/ExecuteCommand, these cannot be registered as console commands.
	virtual void Command_Map(CStr _Name);
	virtual void Command_Disconnect();
	virtual void Command_Connect(CStr _Addr);
	virtual void Command_SetGame(CStr _WorldPathes, CStr _Game);
	virtual void Command_LoadGame(CStr _GameName);
	virtual void Command_FreeResources();
	virtual void Command_Script(CStr _Script);
	virtual void Command_ChangeMap(CStr _Name, CStr _Flags);
	virtual void Command_PlayCutsceneSound(CStr _Name, CStr _SyncSound);

	// Console add-ins
	void Con_BSPModelEnable(int _Mask);
	void Con_BSPModelSwitch(int _Mask);

	void Con_SplitScreen(int _nSplitScreens);
	void Con_SplitAxis(int _Axis);
	void Con_AddLocalClient(int _bSplit);
	void Con_Pause(int _Enable);
	void Con_SingleStep();
	void Con_Dump(int _DumpFlags);
	void Con_Map(CStr _Name);										// Deferred
	void Con_SMap(CStr _Name);										// Deferred
	void Con_ChangeMap(CStr _Name, int _Flags);
	void Con_Record(CStr _DemoName);
	void Con_Stop();
	void Con_Demo(CStr _Name);
	void Con_CaptureDemo(CStr _Name, CStr _Dir, int _Rate);
	void Con_TimeDemoGraph(CStr _Name);
	void Con_TimeDemo(CStr _Name);
	void Con_Benchmark(CStr _Name);
	void Con_Connect(CStr _Addr);									// Deferred
	void Con_Disconnect();											// Deferred
	void Con_Listen();
	void Con_Dir(CStr _Path);
	void Con_SaveGame(CStr _GameName);
	void Con_LoadGame(CStr _GameName);								// Deferred
	void Con_SetGame(CStr _WorldPathes, CStr _Game);				// Deferred
	void Con_SetGameKey(CStr _Key, CStr _Value);
	void Con_FreeResources();										// Deferred
	void Con_DeferredScript(CStr _Script);
	void Con_DeferredScriptGrabScreen(CStr _Script);
	void Con_SetWidescreen(int _ws);
	void Con_SimulateFractional(int _v);
	void Con_ViewScale(fp32 _vX, fp32 _vY);
	void Con_AddViewScale(fp32 _vX, fp32 _vY);
	void Con_SetGameClass(CStr _s);

	void Con_ListOptions(void);
	void Con_ForceOption(CStr _Option, CStr _NewSetting);
	void Con_SetOption(CStr _Option, CStr _NewSetting);
	void Con_AddOption(CStr _Option, CStr _Setting);
	void Con_DelOption(CStr _Option);
	void Con_TimeLeap(int _v);

	void Con_SoakMode();

	void Register(CScriptRegisterContext &_RegContext);
};

typedef TPtr<CGameContext> spCGameContext;

#endif //_WGAMECONTEXT_H

