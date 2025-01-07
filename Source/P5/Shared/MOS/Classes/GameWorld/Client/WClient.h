#ifndef __WCLIENT_H
#define __WCLIENT_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client base class

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWorld_Client
\*____________________________________________________________________________________________*/

#include "../WPhysState.h"
#include "../../../Classes/Win/MWinGrph.h"
#include "../../../MOS.h"
#include "../../../XR/XR.h"

#include "../WClassCmd.h"
#include "WClientClass.h"

// -------------------------------------------------------------------
enum
{
	CWCLIENT_NFRAMEBUFFERS				= 4,	// Length of queue for incoming game-updates. (m_lspFrameIn)

	CWCLIENT_DEFAULTCONTROLQUEUE		= 240,	// Length of outgoing CCmd queue. (m_LocalPlayer.m_spCmdQueue)

	// -------------------------------------------------------------------
	WCLIENT_REG_GAMESTATE				= 0,	// Registry root child nr for game-state relevant stuff, such as the LOCALPLAYER key.

	WCLIENT_REG_NUM						= 10,

	// -------------------------------------------------------------------
	//  Flags used in the Dump() functions. (console: wdump())
	WDUMP_SERVER						= 0x10,
	WDUMP_CLIENTS						= 0x20,
	WDUMP_MIRRORS						= 0x40,

	WDUMP_OBJECTS						= 1,
	WDUMP_WDATA							= 2,
	WDUMP_NETREG						= 4,

	// -------------------------------------------------------------------
	//  Flags used with m_ClientMode, GetClientMode()
	WCLIENT_MODE_PRIMARY				= 1,
	WCLIENT_MODE_MIRROR					= 2,
	WCLIENT_MODE_REFERENCE				= 4,
	WCLIENT_MODE_NOREPLICATION			= 8,
	WCLIENT_MODE_SPLITSCREEN			= 16,
	WCLIENT_MODE_PAUSE					= 32,
	WCLIENT_MODE_GUI					= 64,
	WCLIENT_MODE_LOCAL					= 128,

	// -------------------------------------------------------------------
	//  Flags used with m_ClientState
	WCLIENT_STATE_VOID					= 0,
	WCLIENT_STATE_CHANGELEVEL			= 1,
	WCLIENT_STATE_PRECACHE				= 2,
	WCLIENT_STATE_INGAME				= 3,

	// -------------------------------------------------------------------
	WCLIENT_SPLITSCREEN_REQUIRED		= 0,	// Split-screen is required
	WCLIENT_SPLITSCREEN_SHARE			= 1,	// Screen can be shared for all players, (loading, character select, etc.)

	// -------------------------------------------------------------------
	//  Graph index allocation
	WCLIENT_GRAPH_LAGTIME				= 0,
	WCLIENT_GRAPH_RENDERTIME,
	WCLIENT_GRAPH_NETBANDWIDTH,
	WCLIENT_GRAPH_SERVERTIME,
	WCLIENT_GRAPH_CLIENTTIME,
	WCLIENT_GRAPH_NETTIME,
	WCLIENT_GRAPH_PREDICTTIME,
	WCLIENT_GRAPH_SOUNDTIME,

	WCLIENT_GRAPH_MAX,
};

// -------------------------------------------------------------------
//  CWorld_Client
// -------------------------------------------------------------------
class CMapData;
typedef TPtr<CMapData> spCMapData;

class CWorldData;
typedef TPtr<CWorldData> spCWorldData;

// -------------------------------------------------------------------
class CWC_CreateInfo
{
public:
	int m_Mode;
	spCRegistry m_spGameReg;
	spCWorldData m_spWData;
	spCMapData m_spMapData;
	class CWorld_Client* m_pClientRef;
	int m_PlayerNr;
	spCXR_Engine m_spEngine;

	CWC_CreateInfo()
	{
		m_Mode = 0;
		m_pClientRef = NULL;
		m_PlayerNr = 0;
	}
};


// -------------------------------------------------------------------
class CWorld_Client : public CWorld_PhysState, public CXR_EngineClient, public CSubSystem
{

	MRTC_DECLARE;
protected:
	CVec4Dfp32 m_CurrentRenderTickFrac;

public:
	spCMapData m_spGUIData;

	uint m_Render_CharacterShadows : 1;

	virtual void Create(CWC_CreateInfo& _CreateInfo) pure;

	virtual void Precache_Init() pure;
	virtual void Precache_Perform(CRenderContext* _pRC, fp32 _dTime) pure;
	virtual CFStr Precache_GetItemName() pure;
	virtual fp32 Precache_GetProgress() pure;
	virtual int Precache_Status() pure;

	virtual void SetViewFlags(uint32 _Flags, fp32 _AspectChange) pure;
	virtual uint32 GetViewFlags() pure;

	// -------------------------------------------------------------------
	//  Various info
	virtual int GetClientMode() pure;							// Used to check if the client is a mirror or a 'primary' (ie, 'real' client.)
	virtual int GetClientState() pure;
	virtual int GetRecMode() pure;
	virtual int GetSplitScreenMode() pure;
	virtual CStr GetClientInfo() pure;						// Used to get debuginfo about the client
	virtual bool CanRenderGame() pure;						// 'true' indicates the client has completed connection sequence and is now capable of rendering the game.
	virtual CMTime GetLastSnapshotTime() pure;
	virtual CMTime GetLastCmdTime() pure;
	virtual int GetInteractiveState() pure;

	virtual bool HideHud() pure;

	// -------------------------------------------------------------------
	//  Debug graph
#ifdef M_Profile
	virtual class CPerfGraph* GetGraph(int _iGraph) pure;
	virtual void AddGraphPlot(int _iGraph, fp32 _Value, uint32 _Color = 0xffffffff) pure;
#endif

	// -------------------------------------------------------------------
	//  Time stuff
	virtual CMTime GetTime() pure;						// Unscaled system time
//	virtual fp32 GetRenderTickFrac() pure;				// Game tick fraction. [0..1], only valid when called from OnClientRender
	M_FORCEINLINE const fp32& GetRenderTickFrac() { return m_CurrentRenderTickFrac.k[0]; }
	M_FORCEINLINE vec128 GetRenderTickFracv() { return m_CurrentRenderTickFrac.v; }
	virtual CMTime GetRenderTime() pure;				// GetGameTime + GetRenderTickFrac*GetGameTickTime, only valid when called from OnClientRender
	virtual fp32 GetInterpolateTime() pure;
	virtual fp32 GetModeratedFramePeriod() pure;

	// -------------------------------------------------------------------
	// Player
	virtual CWC_Player* Player_GetLocal() pure;
	virtual int Player_GetLocalObject() pure;

	// -------------------------------------------------------------------
	// Registry
	virtual void OnRegistryChange() pure;						// This function will be called upon registry changes.

	virtual int Registry_GetNum() pure;							// Returns number of registry directories
	virtual const CRegistry* Registry_Get(int _iDirectory) pure;	// Returns a registry directory.
	virtual const CRegistry* Registry_GetRoot() pure;			// Returns the entire client registry
	virtual CRegistry* Registry_GetUser() pure;
	virtual CRegistry* Registry_GetGame() pure;			// Added by Talbot
	virtual CRegistry* Registry_GetEnvironment() pure; // Added by Talbot
protected:
	virtual CRegistry* Registry_GetNonConst(int _iDirectory) pure;// Returns a non-const registry directory.
	virtual CRegistry* Registry_GetRootNonConst() pure;			// Returns the entire client registry

	// -------------------------------------------------------------------
	// Client object-management
public:
	virtual int IsClientObject(int _iObj) pure;

	virtual CWObject_CoreData* Object_GetCD(int _iObj) pure;
	virtual class CWObject_Client* Object_Get(int _iObj) pure;
	virtual class CWObject_Client* Object_GetFirstCopy(int _iObj) pure;
	virtual class CWObject_ClientExecute* ClientObject_Get(int _iObj) pure;

protected:
	virtual TArray<TPtr<CWObject_Client> > Object_GetArray() pure;	// Only accessible by CWorld_Server

public:
	virtual int ClientObject_Create(const char* _pClassName, const CMat4Dfp32& _Pos) pure;
	virtual int ClientObject_Create(const char* _pClassName, const CVec3Dfp32& _Pos) pure;
	virtual void Object_Destroy(int _iObj) pure;

	virtual void Object_ForcePosition(int _iObj, const CMat4Dfp32& _Pos) pure;	// Set position without checking for collision
	virtual void Object_ForcePosition(int _iObj, const CVec3Dfp32& _Pos) pure;	// Set position without checking for collision
	virtual void Object_ForcePosition_World(int _iObj, const CMat4Dfp32& _Pos) pure;	// Set position without checking for collision
	
	virtual void Object_SetBox(int _iObj, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max) pure;			// N/A
	virtual int Object_SetDirty(int _iObj, int _Mask) pure;

	// Client-objects only:
	virtual bool Object_Move(int _iObj) pure;					// Move object without any collision. (moderately useful..)

	virtual aint ClientMessage_SendToObject(const CWObject_Message& _Msg, int _iObj) pure;

	// -------------------------------------------------------------------
	// Sound services
	virtual void Sound_SetSoundContext(TPtr<class CSoundContext> _spSC) pure;
	virtual void Sound_SetVolume(fp32 _VolumeAmbient, fp32 _VolumeSfx, fp32 _VolumeVoice) pure;
	virtual void Sound_SetGameMasterVolume(fp32 _Volume) pure;
	virtual void Sound_Reset() pure;
	virtual void Sound_Mute(bool _bMuted) pure;
	virtual void Sound_KillVoices() pure;
	virtual int Sound_Global(int _iChannel, int _iSound, fp32 _Volume = 1.0f, bool _bLoop = false, fp32 _Delay = 0.0f) pure;
	virtual int Sound_At(int _iChannel, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, uint32 _GroupID = 0) pure;
	virtual int Sound_At(int _iChannel, const CVec3Dfp32& _Pos, class CSC_SFXDesc *_pSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, uint32 _GroupID = 0) pure;
	virtual int Sound_On(int _iChannel, int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f) pure;
	virtual void Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial=0) pure;
	virtual void Sound_Kill(int _iObj) pure;
	virtual void Sound_SetVolume(int _hVoice, fp32 _Volume) pure;
	
	virtual bool Sound_MultiStream_Play(int *_pMusic, int _nMusic) pure;
	virtual void Sound_MultiStream_Volumes(fp32 *_pVolume, int _nVolumes, fp32 *_pFadeSpeeds) pure;

	virtual void Sound_UpdateSoundVolumes(const CVec3Dfp32 &_Pos) pure;
	virtual void Sound_SetSoundVolumesVolume(fp32 _Volume) pure;
	virtual void Sound_UpdateTrackSounds() pure;
	virtual void Sound_Clean() pure;

	virtual void Sound_SetSoundChannelVolume(int _Channel, fp32 _Volume) pure;
	virtual fp32  Sound_GetSoundChannelVolume(int _Channel) pure;

	// -------------------------------------------------------------------
	// Rendering
	virtual void Render_GetCamera(CMat4Dfp32& _Camera) pure;				// NOTE: This function invokes client-prediction and is therefore VERY costly.
	virtual void Render_GetLastRenderCamera(CMat4Dfp32& _Camera) pure;	// Use this to get the last predicted camera position.
	virtual void Render_ModifyViewport(CRC_Viewport& _Viewport) pure;
	virtual void Render_GetLastRenderViewport(CRC_Viewport& _Viewport) pure;	// Use this to get the viewport last returned by Render_GetViewport
	virtual CVec2Dfp32 Render_GetViewScaleMultiplier() pure;
	virtual void Render_World(CXR_VBManager* _pVBM, CRenderContext* _pRender, const CMat4Dfp32& _Camera, const CMat4Dfp32& _CameraVelocity, fp32 _InterpolationTime = 0.0f) pure;
	virtual void Render(CXR_VBManager* _pVBM, CRenderContext* _pRender, CRC_Viewport& _GUIVP, fp32 _InterpolationTime, int _Context) pure;	// Called from CGameContext
	virtual CXR_Engine* Render_GetEngine() pure;

	virtual void Precache_Perform(CRenderContext* _pRC) pure;



	// -------------------------------------------------------------------
	// Wallmark
	virtual int Wallmark_Create(const CMat4Dfp32 &_Pos, fp32 _Size, fp32 _Tolerance, CMTime _Time, const char* _pSurfName, int _Flags) pure;
	virtual int Wallmark_Create(const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags) pure;
	virtual int Wallmark_Create(CXR_Model* _pModel, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags) pure;
	virtual bool Wallmark_Destroy(int _GUID) pure;

	// -------------------------------------------------------------------
	// World
	virtual void World_Close() pure;
	virtual int World_EnumerateVisible(const CVec3Dfp32& _Pos, uint16* _piRetObj, int _MaxObj) pure;	// Returns a list of objects that are potentialy visible from _Pos
	virtual void World_SetModel(int _iModel, CXR_WallmarkContextCreateInfo* _pWCCI = NULL) pure;	// Model to use for scenegraph and navigation.
	virtual CXR_Model* World_GetModel() pure;

	// -------------------------------------------------------------------
	// Network I/O for local client.
	virtual bool Local_OutQueueEmpty() pure;
	virtual const CNetMsgPack* Local_GetOutMsg() pure;
	virtual bool Local_PutInMsg(const CNetMsgPack& _MsgPack) pure;

	virtual CCmdQueue* Local_GetCmdQueue() pure;
	virtual void Local_ClearCmdQueue() pure;

	// -------------------------------------------------------------------
	// Network stuff.
	virtual void Net_UnpackClientUpdate(CWObjectUpdateBuffer* _pDeltaBuffer, int _Flags) pure;
	virtual void Net_FlushGameFrames() pure;
	virtual bool Net_CreateServerRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer) pure;
	virtual bool Net_UnpackClientRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer) pure;

	virtual bool Net_PutMsg(const CNetMsg& _Packet) pure;				// Send netmsg to server, false if queue is full.
	virtual bool Net_FlushMessages() pure;								// Flush outgoing queue.
	virtual void Net_OnMessage(const CNetMsg& _Packet) pure;				// Only accessible by server. Called for each incomming netmsg, calls Net_OnProcessMessage
	virtual bool Net_SetClientVar(CStr _Key, CStr _Value) pure;

	// Stuff needed by CGameContext
	virtual void Net_SetConnection(spCNetwork _spNetwork, int _hConnection) pure;
	virtual int Net_GetConnection() pure;
	virtual uint32 Net_GetConnectionStatus() pure;

	virtual	bool ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey) pure;
	virtual void Refresh() pure;
	virtual void PreRefresh() pure;
	virtual void Simulate(int _bCalcInterpolation = 1) pure;

	// -------------------------------------------------------------------
	// Demo-recording/playback
	virtual void Demo_Start(CStr _FileName) pure;
	virtual void Demo_Stop() pure;
	virtual void Demo_Get(CStr _FileName) pure;
	virtual int Demo_PlayTick() pure;

	// Dump
	virtual void Dump(int _DumpFlags) pure;

	friend class CWorld_Server;
	friend class CWorld_ServerCore;
};

class CWorld_Client;
typedef TPtr<CWorld_Client> spCWorld_Client;

 
// -------------------------------------------------------------------
static M_INLINE void Interpolate(const CMat4Dfp32& _Pos, const CMat4Dfp32& _dPos, CMat4Dfp32& _Dest, fp32 t)
{
	MAUTOSTRIP(Interpolate, MAUTOSTRIP_VOID);
	if (t < 0.0f) 
		t = 0.0f;
	else if (t > 1.5f) 
		t = 1.5f;

	CMat4Dfp32 Pos2;
	_Pos.Multiply(_dPos, Pos2);

	_Dest.UnitNot3x3();
	CVec3Dfp32 v3 = CVec3Dfp32::GetMatrixRow(Pos2, 3) - CVec3Dfp32::GetMatrixRow(_Pos, 3);
	CVec3Dfp32::GetMatrixRow(_Dest, 3) = CVec3Dfp32::GetMatrixRow(_Pos, 3) + v3*t;

	CVec3Dfp32 v0 = CVec3Dfp32::GetMatrixRow(Pos2, 0) - CVec3Dfp32::GetMatrixRow(_Pos, 0);
	CVec3Dfp32::GetMatrixRow(_Dest, 0) = (CVec3Dfp32::GetMatrixRow(_Pos, 0) + v0*t).Normalize();
	CVec3Dfp32 v1 = CVec3Dfp32::GetMatrixRow(Pos2, 1) - CVec3Dfp32::GetMatrixRow(_Pos, 1);
	CVec3Dfp32::GetMatrixRow(_Dest, 2) = -((CVec3Dfp32::GetMatrixRow(_Pos, 1) + v1*t) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
	CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
}

// -------------------------------------------------------------------
/*static M_INLINE void Interpolate2(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, fp32 t)
{
	MAUTOSTRIP(Interpolate2, MAUTOSTRIP_VOID);
	if (CVec3Dfp32::GetMatrixRow(_Pos, 3).DistanceSqr(CVec3Dfp32::GetMatrixRow(_Pos2, 3)) > Sqr(256.0f))
	{
		_Dest = _Pos;
	}
	else if(!memcmp(&_Pos, &_Pos2, sizeof(_Pos)))
	{
		_Dest = _Pos;
	}
	else
	{
		fp32 tpos = ClampRange(t, 2.5f);
		t = ClampRange(t, 1.5f);
		_Dest.UnitNot3x3();
		CVec3Dfp32 v3 = CVec3Dfp32::GetMatrixRow(_Pos2, 3) - CVec3Dfp32::GetMatrixRow(_Pos, 3);
		CVec3Dfp32::GetMatrixRow(_Dest, 3) = CVec3Dfp32::GetMatrixRow(_Pos, 3) + v3*tpos;

		CVec3Dfp32 v0 = CVec3Dfp32::GetMatrixRow(_Pos2, 0) - CVec3Dfp32::GetMatrixRow(_Pos, 0);
		CVec3Dfp32::GetMatrixRow(_Dest, 0) = (CVec3Dfp32::GetMatrixRow(_Pos, 0) + v0*t).Normalize();
		CVec3Dfp32 v1 = CVec3Dfp32::GetMatrixRow(_Pos2, 1) - CVec3Dfp32::GetMatrixRow(_Pos, 1);
		CVec3Dfp32::GetMatrixRow(_Dest, 2) = -((CVec3Dfp32::GetMatrixRow(_Pos, 1) + v1*t) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
		CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
	}
}*/

M_FORCEINLINE void Interpolate2(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];
	vec128 p0y = _Pos.r[1];
	vec128 p0z = _Pos.r[2];
	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];
	vec128 p1y = _Pos2.r[1];
	vec128 p1z = _Pos2.r[2];
	vec128 p1w = _Pos2.r[3];
	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x4(dstx, dsty, dstz, z);

	vec128 dmove = M_VSub(p0w, p1w);
	vec128 teleportmask = M_VCmpGTMsk(M_VDp3(dmove, dmove), M_VScalar(256.0f*256.0f));

	dstx = M_VSelMsk(teleportmask, p1x, dstx);
	dsty = M_VSelMsk(teleportmask, p1y, dsty);
	dstz = M_VSelMsk(teleportmask, p1z, dstz);
	dstw = M_VSelMsk(teleportmask, p1w, dstw);

	_Dest.r[0] = dstx;
	_Dest.r[1] = dsty;
	_Dest.r[2] = dstz;
	_Dest.r[3] = dstw;
}

#endif //_INC_WCLIENT


