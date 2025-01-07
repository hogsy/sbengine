
#include "PCH.h"

#include "WClient_Core.h"
#include "../WPackets.h"
#include "../../Shared/mos/Classes/GameWorld/Client/WClient_Sound.h"


// -------------------------------------------------------------------
//  Sound services
// -------------------------------------------------------------------
void CWorld_ClientCore::Sound_UpdateVolume()
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_UpdateVolume, MAUTOSTRIP_VOID);
	if (m_spSound != NULL)
	{
		m_spSound->Chn_SetVolume(m_hChannels[0], m_Sound_VolumeAmbient * m_Sound_VolumeGameMaster);
		m_spSound->Chn_SetVolume(m_hChannels[1], m_Sound_VolumeSfx * m_Sound_VolumeGameMaster);
		m_spSound->Chn_SetVolume(m_hChannels[2], m_Sound_VolumeSfx * m_Sound_VolumeGameMaster);
		m_spSound->Chn_SetVolume(m_hChannels[3], m_Sound_VolumeVoice * m_Sound_VolumeGameMaster);
		m_spSound->Chn_SetVolume(m_hChannels[4], m_Sound_VolumeSfx * m_Sound_VolumeGameMaster);
	}
}

//
//
void CWorld_ClientCore::Sound_SetSoundContext(spCSoundContext _spSC)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_SetSoundContext, MAUTOSTRIP_VOID);
	MSCOPESHORT(Sound_SetSoundContext);
	if (m_spSound != NULL)
	{
		Sound_KillVoices();

		for(int i = 0; i < WCLIENT_NUMCHANNELS; i++)
		{
			m_spSound->Chn_Free(m_hChannels[i]);
			m_hChannels[i] = -1;
		}
	}

	m_spSound = _spSC;

	if(m_spSound != NULL)
	{
		m_hChannels[0] = m_spSound->Chn_Alloc(WCLIENT_NVOICES_AMBIENT);
		m_hChannels[1] = m_spSound->Chn_Alloc(WCLIENT_NVOICES_SFX);
		m_hChannels[2] = m_spSound->Chn_Alloc(WCLIENT_NVOICES_SFXLOOPING);
		m_hChannels[3] = m_spSound->Chn_Alloc(WCLIENT_NVOICES_VOICE);
		m_hChannels[4] = m_spSound->Chn_Alloc(WCLIENT_NVOICES_CUTSCENE);
	}
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetOptions())
	{
		fp32 VolumeSfx = pSys->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f, 1);
		// Force refresh of volume setting by invalidating cached values
		m_Sound_VolumeAmbient = -1.0f;
		m_Sound_VolumeSfx = -1.0f;
		m_Sound_VolumeVoice = -1.0f;
		Sound_SetVolume(VolumeSfx, VolumeSfx, VolumeSfx);
	}
}

//
//
void CWorld_ClientCore::Sound_SetVolume(fp32 _VolumeAmbient, fp32 _VolumeSfx, fp32 _VolumeVoice)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_SetVolume, MAUTOSTRIP_VOID);
	if(_VolumeAmbient != m_Sound_VolumeAmbient || _VolumeSfx != m_Sound_VolumeSfx ||
	   _VolumeVoice != m_Sound_VolumeVoice)
	{
		m_Sound_VolumeAmbient = _VolumeAmbient;
		m_Sound_VolumeSfx = _VolumeSfx;
		m_Sound_VolumeVoice = _VolumeVoice;
		Sound_UpdateVolume();
	}
}

//
//
void CWorld_ClientCore::Sound_SetGameMasterVolume(fp32 _Volume)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_SetGameMasterVolume, MAUTOSTRIP_VOID);
	if(_Volume != m_Sound_VolumeGameMaster)
	{
		m_Sound_VolumeGameMaster = _Volume;
		Sound_UpdateVolume();
	}
}

//
//
void CWorld_ClientCore::Sound_Reset()
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_Reset, MAUTOSTRIP_VOID);
	Sound_KillVoices();
	if(m_spSound) m_spSound->MultiStream_Stop(false);
}

bool CWorld_ClientCore::Sound_IsActive()
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_IsActive, false);
	if(m_ClientState != WCLIENT_STATE_INGAME || Precache_Status() != PRECACHE_DONE)
		return false;
	
	if(!m_bSound_PosSet)
	{
		// Fix for trying to play sounds before first rendered frame
		// (this doesn't usually happen) - JA
		int iPlayer = Player_GetLocalObject();
		if(iPlayer == -1)
			return false;

		CMat4Dfp32 Camera;
		CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
		Msg.m_pData = &Camera;
		Msg.m_DataSize = sizeof(Camera);
		Camera.Unit();

		if (!ClientMessage_SendToObject(Msg, iPlayer))
			return false;

		m_spSound->Listen3D_SetCoordinateScale(1.0f/32.0f);
		Camera.RotY_x_M(-0.25f);
		Camera.RotX_x_M(0.25f);
		m_spSound->Listen3D_SetOrigin(Camera);
		m_bSound_PosSet = true;
	}

	return true;
}

void CWorld_ClientCore::Sound_Mute(bool _bMuted)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_Mute, MAUTOSTRIP_VOID);
	if(m_spSound == NULL)
		return;

	// Not supported any longer, should be muted by setting channel volume to 0.0
	/*
	for(int i = 0; i < WCLIENT_NUMCHANNELS; i++)
		m_spSound->Chn_Mute(m_hChannels[i], _bMuted);
		*/
}


void CWorld_ClientCore::Sound_KillVoices()
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_KillVoices, MAUTOSTRIP_VOID);

	if(m_spSound == NULL)
		return;

	for(int i = 0; i < m_lspObjects.Len() + m_lspClientObjects.Len(); i++)
		Sound_Kill(i);

	// Free all directional sounds
	// Remove all sounds that have the deletion flag set
	for(int i = 0; i < m_lDirectionalSounds.Len(); i++)
	{
		// Destroy voice
		if( m_lDirectionalSounds[i].m_iVoice != -1 )
		{
			m_spSound->Voice_Destroy(m_lDirectionalSounds[i].m_iVoice);
			m_lDirectionalSounds[i].m_iVoice	= -1;
		}
	}

	for( int i = 0; i < m_lTrackingSounds.Len(); i++)
	{
		if( m_lTrackingSounds[i].m_iVoice != -1 )
		{
			m_spSound->Voice_Destroy(m_lTrackingSounds[i].m_iVoice);
			m_lTrackingSounds[i].m_iVoice = -1;
		}
	}
	m_spSound->Chn_DestroyVoices(m_hChannels[WCLIENT_CHANNEL_CUTSCENE]);
	m_spSound->Chn_DestroyVoices(m_hChannels[WCLIENT_CHANNEL_AMBIENT]);
	m_spSound->Chn_DestroyVoices(m_hChannels[WCLIENT_CHANNEL_SFX]);
	m_spSound->Chn_DestroyVoices(m_hChannels[WCLIENT_CHANNEL_SFXLOOPING]);
	m_spSound->Chn_DestroyVoices(m_hChannels[WCLIENT_CHANNEL_VOICE]);
}

void CWorld_ClientCore::Sound_SetVolume(int _hVoice, fp32 _Volume)
{
	if(m_spSound == NULL)
		return;

	m_spSound->Voice_SetVolume(_hVoice, _Volume);
}

int CWorld_ClientCore::Sound_Global(int _iChannel, int _iSound, fp32 _Volume, bool _bLoop, fp32 _Delay)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_Global, -1);

	// Check so we can play the sounds
	if (!m_spSound || !Sound_IsActive())
		return -1;

	// Get sound description
	CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(_iSound);
	if (!pDesc)
		return -1;

	// Get what waveid to play
	int32 iWave = pDesc->GetPlayWaveId();
	if(iWave < 0)
		return -1;

	CSC_VoiceCreateParams CreateParams;
	m_spSound->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
	CreateParams.m_Volume *= _Volume;
	CreateParams.m_hChannel = m_hChannels[_iChannel];
	CreateParams.m_WaveID = iWave;
	CreateParams.m_bLoop = _bLoop;
	CreateParams.m_MinDelay = _Delay;

	// start the sound
	return Sound_CreateVoice(pDesc, iWave, CreateParams);
}

//
//
int CWorld_ClientCore::Sound_At(int _iChannel, const CVec3Dfp32& _Pos, CSC_SFXDesc *_pDesc, int _AttnType,
								uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, fp32 _Delay, uint32 _SyncGroupID)
{
	return Sound_Play(_iChannel, -1, _Pos, _pDesc, _AttnType, _iMaterial, _Volume, _V0, _Delay, false, _SyncGroupID);
}


int CWorld_ClientCore::Sound_At(int _iChannel, const CVec3Dfp32& _Pos, int _iSound, int _AttnType,
								 uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, fp32 _Delay, uint32 _SyncGroupID)
{
	if (m_spMapData != NULL)
		return Sound_Play(_iChannel, -1, _Pos, m_spMapData->GetResource_SoundDesc(_iSound), _AttnType, _iMaterial, _Volume, _V0, _Delay, false, _SyncGroupID);
	else
		return -1;
}

//
//
int CWorld_ClientCore::Sound_On(int _iChannel, int16 _iObject, int _iSound, int _AttnType,
								uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, fp32 _Delay)
{
	CVec3Dfp32 Dummy = 0.0f;
	return Sound_Play(_iChannel, _iObject, Dummy, _iSound, _AttnType, _iMaterial, _Volume, _V0, _Delay);
}

//
//
int CWorld_ClientCore::Sound_CreateVoice(CSC_SFXDesc *_pDesc, int16 _Wave, CSC_VoiceCreateParams &_Params)
//int CWorld_ClientCore::Sound_CreateVoice3D(CSC_SFXDesc *_pDesc, int16 _Wave, int _hChn, const CSC_3DProperties& _Properties, fp32 _Volume, fp32 _Delay, bool _bLoop)
{
	int WaveID = _pDesc->GetWaveId(_Wave);
	if (WaveID == -1)
		return 0;

	_Params.m_WaveID = WaveID;
	_Params.m_Volume *= (_pDesc->GetVolume() + Random*_pDesc->GetVolumeRandAmp()); // *pMix->m_3DGameVolume;
	_Params.m_Pitch *= _pDesc->GetPitch() + Random*_pDesc->GetPitchRandAmp();
	uint32 Prio = _pDesc->GetPriority();
	if (Prio)
		_Params.m_Priority = Prio;

	return m_spSound->Voice_Create(_Params);
}
/*
//
//
int CWorld_ClientCore::Sound_CreateVoice(CSC_SFXDesc *_pDesc, int16 _Wave, int _hChn, fp32 _Volume, bool _bLoop, fp32 _Delay)
{
	int WaveID = _pDesc->GetWaveId(_Wave);
	if (WaveID == -1)
		return 0;
	CWC_SoundMixSettings *pMix = Sound_GetMix();
	return m_spSound->Voice_Create(m_hChannels[_hChn], 
			WaveID, _pDesc->GetPriority(), 
			_pDesc->GetPitch() + Random*_pDesc->GetPitchRandAmp(), 
			(_pDesc->GetVolume() + Random*_pDesc->GetVolumeRandAmp())*_Volume * pMix->m_2DGameVolume * pMix->m_MixCategories[_pDesc->GetVolumeCategory()], 
			_bLoop, false, _Delay);
}
*/
//
//
int CWorld_ClientCore::Sound_Play(int _iChannel, int16 _iObject, const CVec3Dfp32& _Pos, int _iSound,
								  int _AttnType, uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, fp32 _Delay, bool _bLoop, uint32 _GroupID)
{
	if (m_spMapData != NULL)
		return Sound_Play(_iChannel, _iObject, _Pos, m_spMapData->GetResource_SoundDesc(_iSound), _AttnType, _iMaterial, _Volume, _V0, _Delay, _bLoop, _GroupID);
	else
		return -1;
}

void CWorld_ClientCore::Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial)
{
	CSC_SFXDesc* pSound = m_spMapData->GetResource_SoundDesc(_iSound);
	if (!pSound)
		return;

	int16 iWave = pSound->GetPlayWaveId(_iMaterial);
	
	// Go through tracking sounds
	TAP_RCD<CTrackingSound> lTracking = m_lTrackingSounds;
	bool bFirst = true;
	for (uint i = 0; i < lTracking.Len(); i++)
	{
		if (lTracking[i].m_iObject == _iObject && lTracking[i].m_iWave == iWave)
		{
			if (!bFirst)
				M_TRACEALWAYS("[ClientCore] NOTE: instance of sound %d is still active! (nuking that one aswell...)\n", _iSound);

			m_spSound->Voice_Destroy(lTracking[i].m_iVoice);
			m_lTrackingSounds.Del(i);
			lTracking = m_lTrackingSounds;
			i--;
			bFirst = false;
		}
	}
}


int CWorld_ClientCore::Sound_Play(int _iChannel, int16 _iObject, const CVec3Dfp32& _Pos, CSC_SFXDesc* _pSound,
								   int _AttnType, uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, fp32 _Delay, bool _bLoop, uint32 _SyncGroupID)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_Play, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Sound_Play);

	// get wave id 
	CSC_SFXDesc* pDesc = _pSound;
	int iWave = pDesc ? pDesc->GetPlayWaveId(_iMaterial) : -1;

	if (!pDesc || (iWave < 0) || !m_spSound || !m_spMapData ||
		((_AttnType != WCLIENT_ATTENUATION_2D && !m_bSound_PosSet) || 
			(m_ClientState != WCLIENT_STATE_INGAME) || (Precache_Status() != PRECACHE_DONE)))
		// Allow Global sounds to be played before first rendered frame
	{
		if (_SyncGroupID)
			Sound_AddVoiceToSyncGroup(-1, _SyncGroupID); // Makes sure group is created

		return -1;
	}

	// Make a stereo sound play as 2D
	uint SoundCategory = pDesc->GetCategory();
	bint bPause = _SyncGroupID != 0;

	int hVoice = -1;
	if (_AttnType == WCLIENT_ATTENUATION_2D || _AttnType == WCLIENT_ATTENUATION_2D_POS)
	{
		CSC_VoiceCreateParams CreateParams;
		m_spSound->Voice_InitCreateParams(CreateParams, SoundCategory);
		CreateParams.m_hChannel = m_hChannels[_iChannel];
		CreateParams.m_Volume = _Volume;
		CreateParams.m_bLoop = _bLoop;
		CreateParams.m_bStartPaused = bPause;
		CreateParams.m_MinDelay = _Delay;

		if (_AttnType == WCLIENT_ATTENUATION_2D_POS)
		{
			CreateParams.m_bFalloff = true;
			CreateParams.m_3DProperties.m_MinDist += pDesc->GetAttnMinDist();
			CreateParams.m_3DProperties.m_MaxDist += pDesc->GetAttnMaxDist();
		}
		else
		{
			CreateParams.m_3DProperties.m_MinDist += _Pos.k[0];
			CreateParams.m_3DProperties.m_MaxDist += _Pos.k[1];
		}

		CWObject_Client *pObj = NULL;
		if (_iObject > 0)
			pObj = Object_Get(_iObject);
		CVec3Dfp32 Offset = _Pos;

		if (pObj)
		{
			if (_AttnType != WCLIENT_ATTENUATION_2D_POS)
				CreateParams.m_3DProperties.m_Position = pObj->GetPosition() + _Pos;
			else
			{
				CreateParams.m_3DProperties.m_Position = pObj->GetPosition();
				Offset = 0.0f;
			}
			CreateParams.m_3DProperties.m_Velocity = pObj->GetMoveVelocity();
			CreateParams.m_3DProperties.m_Orientation = CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0);
		}
		else
		{
			Offset = _Pos;
			CreateParams.m_3DProperties.m_Position = _Pos;
		}

		hVoice = Sound_CreateVoice(pDesc, iWave, CreateParams);

		if (pObj)
		{
			CTrackingSound TrackSound;
			TrackSound.m_iWave = iWave;
			TrackSound.m_iObject = _iObject;
			TrackSound.m_iVoice = hVoice;
			TrackSound.m_Offset = Offset;
			m_lTrackingSounds.Add(TrackSound);
		}
	}
	else
	{
		CSC_VoiceCreateParams3D CreateParams;
		m_spSound->Voice_InitCreateParams(CreateParams, SoundCategory);

		CreateParams.m_hChannel = m_hChannels[_iChannel];
		CreateParams.m_Volume = _Volume;
		CreateParams.m_bLoop = _bLoop;
		CreateParams.m_bStartPaused = bPause;
		CreateParams.m_MinDelay = _Delay;

		// LRP sound
		if (_AttnType == WCLIENT_ATTENUATION_LRP)
		{
			CMat4Dfp32 Cam;
			Render_GetLastRenderCamera(Cam);
			CVec3Dfp32 PosListener = CVec3Dfp32::GetRow(Cam, 3);
			CVec3Dfp32 VecL = PosListener - _Pos;

			fp32 Proj = Clamp01((VecL * _V0) / (_V0 * _V0));
			CVec3Dfp32 PosSound;
			_Pos.Combine(_V0, Proj, PosSound);

			CreateParams.m_3DProperties.m_Position = PosSound;
			CreateParams.m_3DProperties.m_MinDist += pDesc->GetAttnMinDist();
			CreateParams.m_3DProperties.m_MaxDist += pDesc->GetAttnMaxDist();
			hVoice = Sound_CreateVoice(pDesc, iWave, CreateParams);
		}
		else
		{
			fp32 MaxDistance = pDesc->GetAttnMaxDist();
			fp32 MinDistance = pDesc->GetAttnMinDist();
			if (_AttnType == WCLIENT_ATTENUATION_3D_OVERRIDE)
			{
				MinDistance = _V0.k[0];
				MaxDistance = _V0.k[1];
			}
			CreateParams.m_3DProperties.m_MinDist += MinDistance;
			CreateParams.m_3DProperties.m_MaxDist += MaxDistance;

			CVec3Dfp32 Pos;
			Pos = _Pos;

			CWObject_Client *pObj = NULL;
			if (_iObject > 0)
				pObj = Object_Get(_iObject);

			if (pObj)
			{
				Pos += pObj->GetPosition();
			}

			CVec3Dfp32& CameraPos = CVec3Dfp32::GetRow(m_LastRenderCamera, 3);
			if (CameraPos == CVec3Dfp32(0) || (Pos - CameraPos).LengthSqr() < Sqr(MaxDistance) * 1.5f)
			{
				if (pObj)
				{
					CreateParams.m_3DProperties.m_Position = Pos;
					CreateParams.m_3DProperties.m_Orientation = CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0);
					CreateParams.m_3DProperties.m_Velocity = pObj->GetMoveVelocity();
					hVoice = Sound_CreateVoice(pDesc, iWave, CreateParams);

					CTrackingSound TrackSound;
					TrackSound.m_iWave = iWave;
					TrackSound.m_iObject = _iObject;
					TrackSound.m_iVoice = hVoice;
					TrackSound.m_Offset = _Pos;

					m_lTrackingSounds.Add(TrackSound);
				}
				else
				{
					CreateParams.m_3DProperties.m_Position = Pos;
					hVoice = Sound_CreateVoice(pDesc, iWave, CreateParams);
				}
			}
		}
	}

	if (_SyncGroupID)
		Sound_AddVoiceToSyncGroup(hVoice, _SyncGroupID);

	return hVoice;
}

//
//
void CWorld_ClientCore::Sound_Kill(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_Kill, MAUTOSTRIP_VOID);
	if (!m_spSound)
		return;

	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj)
		return;

	for(int i = 0; i < CWO_NUMSOUNDINDICES; i++)
	{
		if (pObj->m_hVoice[i] != -1) 
		{
			m_spSound->Voice_Destroy(pObj->m_hVoice[i]);
			pObj->m_hVoice[i] = -1;
		}
	}

	// Kill dialogoue
	if(pObj->m_ClientData[0] && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
	{
		m_spSound->Voice_Destroy(pObj->m_ClientData[0]);
		pObj->m_ClientData[0] = 0;
	}
}

//
//
//
int CWorld_ClientCore::Sound_UpdateObject(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Sound_UpdateObject, 0);
	MSCOPESHORT(Sound_UpdateObject);
	if (!m_spSound || !Sound_IsActive())
		return 0;
	
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj)
		return 0;

	int nVoices = 0;

	CMat4Dfp32 RenderCam;
	bool bRenderCamValid = false;

	for(int i = 0; i < CWO_NUMSOUNDINDICES; i++)
	{
		int iSound = pObj->m_iSound[i] & 0x7fff;
		bool bInfinite = (pObj->m_iSound[i] & 0x8000) != 0;

		if (iSound)
		{
			CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(iSound);
			if (!pDesc)
				iSound = 0;
			else if (!bInfinite)
			{
				if(!bRenderCamValid)
				{
					Render_GetLastRenderCamera(RenderCam);
					bRenderCamValid = true;
				}
				fp32 DistSqr = pObj->GetPosition().DistanceSqr(CVec3Dfp32::GetRow(RenderCam, 3));
				if (DistSqr > Sqr(pDesc->GetAttnMaxDist()))
					iSound = 0;
			}
		}

		if (iSound != pObj->m_iSoundPlaying[i] && pObj->m_iSoundPlaying[i] >= 0)
		{
			if( pObj->m_hVoice[i] != -1 )
			{
				m_spSound->Voice_Destroy(pObj->m_hVoice[i]);
				pObj->m_hVoice[i] = -1;
			}
			pObj->m_iSoundPlaying[i] = iSound;
			if (!iSound)
				continue;

			// get description
			CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(iSound);
			if(!pDesc)
				continue;

			// Get wave id to play
			int32 iW = pDesc->GetPlayWaveId();

			if(pDesc->GetWaveId(iW) >= 0)
			{
				uint32 SoundCategory = pDesc->GetCategory();

				CSC_VoiceCreateParams3D CreateParams;
				m_spSound->Voice_InitCreateParams(CreateParams, SoundCategory);

				CreateParams.m_bLoop = true;
				CreateParams.m_hChannel = m_hChannels[WCLIENT_CHANNEL_SFXLOOPING];
				CreateParams.m_3DProperties.m_MinDist = pDesc->GetAttnMinDist();
				CreateParams.m_3DProperties.m_MaxDist = pDesc->GetAttnMaxDist();
				CreateParams.m_3DProperties.m_Position = pObj->GetPosition();
				CreateParams.m_3DProperties.m_Orientation = CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0);
				CreateParams.m_3DProperties.m_Velocity = pObj->GetMoveVelocity();

				pObj->m_hVoice[i] = Sound_CreateVoice(pDesc, iW, CreateParams);

				nVoices++;
			}
		}
		else
		{
			if (pObj->m_hVoice[i] >= 0)
			{	
				CVec3Dfp32 Pos = pObj->GetPosition();
				CVec3Dfp32 Orientation = CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0);
				CVec3Dfp32 OldPos = pObj->GetLastPosition();
				CVec3Dfp32 Velocity = pObj->GetMoveVelocity();
				
				m_spSound->Voice3D_SetOrigin(pObj->m_hVoice[i], Pos);
				m_spSound->Voice3D_SetOrientation(pObj->m_hVoice[i], Orientation);
				m_spSound->Voice3D_SetVelocity(pObj->m_hVoice[i], Velocity);
				nVoices++;
			}
		}
	}

	return nVoices;
}

//
//
//
CWorld_ClientCore::CTrackingSound::CTrackingSound()
{
	m_Offset = CVec3Dfp32(0.0f,0.0f,0.0f);
	m_iObject = -1;
	m_iVoice = -1;
}

//
//
//
bool CWorld_ClientCore::UpdateTrackingSound(CTrackingSound &_TrackSound)
{
	CWObject_Client *pObj = Object_Get(_TrackSound.m_iObject);

	if(!pObj || _TrackSound.m_iVoice < 0)
		return false;

	if(!m_spSound->Voice_IsPlaying(_TrackSound.m_iVoice))
		return false;

	m_spSound->Voice3D_SetOrigin(_TrackSound.m_iVoice, pObj->GetPosition() + _TrackSound.m_Offset);
	m_spSound->Voice3D_SetVelocity(_TrackSound.m_iVoice, pObj->GetMoveVelocity());
	m_spSound->Voice3D_SetOrientation(_TrackSound.m_iVoice, CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0));
	
	return true;
}

//
//
//
void CWorld_ClientCore::Sound_UpdateTrackSounds()
{
	for(int32 i = 0; i < m_lTrackingSounds.Len(); i++)
	{
		if(!UpdateTrackingSound(m_lTrackingSounds[i]))
		{
			m_spSound->Voice_Destroy(m_lTrackingSounds[i].m_iVoice);
			m_lTrackingSounds.Del(i);
			i--;
			continue;
		}
	}
}

//
// Just clears out the variables
//
CWorld_ClientCore::CDirectionalSound::CDirectionalSound()
{
	m_Flags.m_Delete = 0;
	m_ObjectSoundNumber = 0;
	m_iObject = 0;
	m_iVoice = 0;
	m_Direction = CVec3Dfp32(0,0,0);
}

bool CWorld_ClientCore::Sound_MultiStream_Play(int *_pMusic, int _nMusic)
{
	if (!m_spSound || !Sound_IsActive() || !m_spMapData)
		return false;
	
	const int MaxStreams = 8;
	CFStr Music[MaxStreams];
	const char *lpMusic[MaxStreams];
	if(_nMusic > MaxStreams)
	{
		_nMusic = MaxStreams;
		ConOut("Too many multistreams");
	}

	for(int i = 0; i < _nMusic; i++)
	{
		if(_pMusic[i] == 0)
			lpMusic[i] = "";
		else
		{
			Music[i] = m_spMapData->GetResourceName(_pMusic[i]);
			if(Music[i].Len() > 4)
				lpMusic[i] = Music[i].Str() + 4;
			else
				lpMusic[i] = "";
		}
	}

	m_spSound->MultiStream_Play(lpMusic, _nMusic, true, true);
	return true;
}

void CWorld_ClientCore::Sound_MultiStream_Volumes(fp32 *_pVolume, int _nVolumes, fp32 *_pFadeSpeeds)
{
	if (m_spSound && Sound_IsActive())
		m_spSound->MultiStream_Volumes(_pVolume, _nVolumes, true, _pFadeSpeeds);
}

void CWorld_ClientCore::Sound_SetSoundChannelVolume(int _Channel, fp32 _Volume)
{
	if(m_spSound)
	{
		m_spSound->Chn_SetVolume( _Channel, _Volume );
		m_spSound->Chn_SetVolume( _Channel, _Volume );
	}
}

fp32 CWorld_ClientCore::Sound_GetSoundChannelVolume(int _Channel)
{
	fp32 ChannelVolume = 1.0f;
	if(m_spSound)
		ChannelVolume = m_spSound->Chn_GetVolume(_Channel);

	return ChannelVolume;
}

static CSoundContext::CFilter::CMinMax gs_MinMax;

void ReverbInterpolateStart(CSoundContext::CFilter &_Filter)
{
    memset(&_Filter, 0, sizeof(_Filter));
}

void ReverbInterpolate(CSoundContext::CFilter &_FilterDest, CSoundContext::CFilter &_Filter, fp32 _Contribution)
{
	_FilterDest.m_EnvironmentSize += _Filter.m_EnvironmentSize * _Contribution;
	_FilterDest.m_Diffusion += _Filter.m_Diffusion * _Contribution;
	_FilterDest.m_Room += _Filter.m_Room * _Contribution;
	_FilterDest.m_RoomHF += _Filter.m_RoomHF * _Contribution;
	_FilterDest.m_RoomLF += _Filter.m_RoomLF * _Contribution;
	_FilterDest.m_DecayTime += _Filter.m_DecayTime * _Contribution;
	_FilterDest.m_DecayHFRatio += _Filter.m_DecayHFRatio * _Contribution;
	_FilterDest.m_DecayLFRatio += _Filter.m_DecayLFRatio * _Contribution;
	_FilterDest.m_Reflections += _Filter.m_Reflections * _Contribution;
	_FilterDest.m_ReflectionsDelay += _Filter.m_ReflectionsDelay * _Contribution;
	_FilterDest.m_Reverb += _Filter.m_Reverb * _Contribution;
	_FilterDest.m_ReverbDelay += _Filter.m_ReverbDelay * _Contribution;
	_FilterDest.m_EchoTime += _Filter.m_EchoTime * _Contribution;
	_FilterDest.m_EchoDepth += _Filter.m_EchoDepth * _Contribution;
	_FilterDest.m_ModulationTime += _Filter.m_ModulationTime * _Contribution;
	_FilterDest.m_ModulationDepth += _Filter.m_ModulationDepth * _Contribution;
	_FilterDest.m_AirAbsorptionHF += _Filter.m_AirAbsorptionHF * _Contribution;
	_FilterDest.m_HFReference += _Filter.m_HFReference * _Contribution;
	_FilterDest.m_LFReference += _Filter.m_LFReference * _Contribution;
	_FilterDest.m_RoomRolloffFactor += _Filter.m_RoomRolloffFactor * _Contribution;
	
}

void CheckReverb(CSoundContext::CFilter &_FilterDest)
{

	if (M_Fabs(_FilterDest.m_EnvironmentSize - gs_MinMax.m_EnvironmentSize.Clamp(_FilterDest.m_EnvironmentSize)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_Diffusion - gs_MinMax.m_Diffusion.Clamp(_FilterDest.m_Diffusion)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_DecayTime - gs_MinMax.m_DecayTime.Clamp(_FilterDest.m_DecayTime)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_DecayHFRatio - gs_MinMax.m_DecayHFRatio.Clamp(_FilterDest.m_DecayHFRatio)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_DecayLFRatio - gs_MinMax.m_DecayLFRatio.Clamp(_FilterDest.m_DecayLFRatio)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_ReflectionsDelay - gs_MinMax.m_ReflectionsDelay.Clamp(_FilterDest.m_ReflectionsDelay)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_ReverbDelay - gs_MinMax.m_ReverbDelay.Clamp(_FilterDest.m_ReverbDelay)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_EchoTime - gs_MinMax.m_EchoTime.Clamp(_FilterDest.m_EchoTime)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_EchoDepth - gs_MinMax.m_EchoDepth.Clamp(_FilterDest.m_EchoDepth)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_ModulationTime - gs_MinMax.m_ModulationTime.Clamp(_FilterDest.m_ModulationTime)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_ModulationDepth - gs_MinMax.m_ModulationDepth.Clamp(_FilterDest.m_ModulationDepth)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_AirAbsorptionHF - gs_MinMax.m_AirAbsorptionHF.Clamp(_FilterDest.m_AirAbsorptionHF)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_HFReference - gs_MinMax.m_HFReference.Clamp(_FilterDest.m_HFReference)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_LFReference - gs_MinMax.m_LFReference.Clamp(_FilterDest.m_LFReference)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs(_FilterDest.m_RoomRolloffFactor - gs_MinMax.m_RoomRolloffFactor.Clamp(_FilterDest.m_RoomRolloffFactor)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs((fp32)_FilterDest.m_Room - gs_MinMax.m_Room.Clamp(_FilterDest.m_Room)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs((fp32)_FilterDest.m_RoomHF - gs_MinMax.m_RoomHF.Clamp(_FilterDest.m_RoomHF)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs((fp32)_FilterDest.m_RoomLF - gs_MinMax.m_RoomLF.Clamp(_FilterDest.m_RoomLF)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs((fp32)_FilterDest.m_Reflections - gs_MinMax.m_Reflections.Clamp(_FilterDest.m_Reflections)) > 0.001f)
		M_ASSERT(0, "");
	if (M_Fabs((fp32)_FilterDest.m_Reverb - gs_MinMax.m_Reverb.Clamp(_FilterDest.m_Reverb)) > 0.001f)
		M_ASSERT(0, "");

}

void ReverbInterpolateFinish(CSoundContext::CFilter &_FilterDest, fp32 _TotalContribution)
{
	fp32 Mult = 1.0 / _TotalContribution;
	// Take the average of all contributions
	_FilterDest.m_EnvironmentSize *= Mult;
	_FilterDest.m_Diffusion *= Mult;
	_FilterDest.m_DecayTime *= Mult;
	_FilterDest.m_DecayHFRatio *= Mult;
	_FilterDest.m_DecayLFRatio *= Mult;
	_FilterDest.m_ReflectionsDelay *= Mult;
	_FilterDest.m_ReverbDelay *= Mult;
	_FilterDest.m_EchoTime *= Mult;
	_FilterDest.m_EchoDepth *= Mult;
	_FilterDest.m_ModulationTime *= Mult;
	_FilterDest.m_ModulationDepth *= Mult;
	_FilterDest.m_AirAbsorptionHF *= Mult;
	_FilterDest.m_HFReference *= Mult;
	_FilterDest.m_LFReference *= Mult;
	_FilterDest.m_RoomRolloffFactor *= Mult;
	_FilterDest.m_RoomHF *= Mult;
	_FilterDest.m_RoomLF *= Mult;

	if (_TotalContribution > 1.0)
	{
		// Take the average of all volume parameters
		_FilterDest.m_Room *= Mult;
		_FilterDest.m_Reflections *= Mult;
		_FilterDest.m_Reverb *= Mult;
	}

	_FilterDest.m_DecayTime = exp(_FilterDest.m_DecayTime);
	_FilterDest.m_DecayHFRatio = exp(_FilterDest.m_DecayHFRatio);
	_FilterDest.m_DecayLFRatio = exp(_FilterDest.m_DecayLFRatio);
	_FilterDest.m_ReflectionsDelay = exp(_FilterDest.m_ReflectionsDelay);
	_FilterDest.m_ReverbDelay = exp(_FilterDest.m_ReverbDelay);
	_FilterDest.m_HFReference = exp(_FilterDest.m_HFReference);
	_FilterDest.m_EnvironmentSize = exp(_FilterDest.m_EnvironmentSize);
	_FilterDest.m_EchoTime = exp(_FilterDest.m_EchoTime);
	_FilterDest.m_ModulationTime = exp(_FilterDest.m_ModulationTime);
	_FilterDest.m_LFReference = exp(_FilterDest.m_LFReference);

	_FilterDest.m_Room = log10(_FilterDest.m_Room) * 2000.0;
	_FilterDest.m_RoomHF = log10(_FilterDest.m_RoomHF) * 2000.0;
	_FilterDest.m_RoomLF = log10(_FilterDest.m_RoomLF) * 2000.0;
	_FilterDest.m_Reflections = log10(_FilterDest.m_Reflections) * 2000.0;
	_FilterDest.m_Reverb = log10(_FilterDest.m_Reverb) * 2000.0;

	// Clamp Delays
	_FilterDest.m_ReflectionsDelay = gs_MinMax.m_ReflectionsDelay.Clamp(_FilterDest.m_ReflectionsDelay);
	_FilterDest.m_ReverbDelay = gs_MinMax.m_ReverbDelay.Clamp(_FilterDest.m_ReverbDelay);
#ifdef _DEBUG
	CheckReverb(_FilterDest);
#endif
}


//
// Sets the appropriate sounds for the position
//
void CWorld_ClientCore::Sound_UpdateSoundVolumes(const CVec3Dfp32 &_Pos)
{
	// Check if we can play sounds and that we got a world
	if (!m_spSound || !Sound_IsActive() || !m_spMapData)
		return;

	// Get objects
	const int32 MaxObjects = 256;
	uint16 liObjects[MaxObjects];
//	TThinArray<uint16> liObjects;
//	liObjects.SetLen(MaxObjects);
	int32 NumObjects = m_iObjSoundPool.EnumAll(liObjects, MaxObjects);
	
	// Set the deletion flag
	for(int32 i = 0; i < m_lDirectionalSounds.Len(); i++)
		m_lDirectionalSounds[i].m_Flags.m_Delete = 1;

	CSoundContext::CFilter InterpolatedFilter;
	ReverbInterpolateStart(InterpolatedFilter);
	fp32 TotalContribution = 0;
	int32 FilterHit = 0;
	//static int32 LastestFilterID = -1;

	for(int32 i = 0; i < NumObjects; i++)
	{
		// Do the object digging thingie
		CWObject_Client *pObj;
		CXR_Model *pModel;
		CXR_PhysicsModel *pPhysModel;
		CCollisionInfo CollisionInfo;
		CollisionInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_PENETRATIONDEPTH);
		
		if(!(pObj = Object_Get(liObjects[i])))
			continue;

		if(!(pModel = m_spMapData->GetResource_Model(pObj->m_iModel[0])))
			continue;

		if(!(pPhysModel = safe_cast<CXR_PhysicsModel>(pModel)))
			continue;

		int FallOff    = pObj->m_iAnim0;
		int MinFallOff = pObj->m_iAnim1;

		CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix());
		pPhysModel->Phys_Init(&PhysContext);
		if(pPhysModel->Phys_IntersectSphere(&PhysContext, _Pos, _Pos, FallOff, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_PLAYERSOLID, &CollisionInfo))
		{
			// Filter effects
			CWObject_Message Msg(666);
			CSoundContext::CFilter Filter;
			Msg.m_pData = &Filter;
			Msg.m_DataSize = sizeof(CSoundContext::CFilter);

			//Phys_Message_SendToObject();
			//CWObject_SoundVolume::CClientData *pCD = CWObject_SoundVolume::GetClientData(pObj);
		
			int iHit = Phys_Message_SendToObject(Msg, pObj->m_iObject);
			if (iHit)
			{
				fp32 Amount = 1;
				if (CollisionInfo.m_bIsValid)
					Amount = (-CollisionInfo.m_Distance)/FallOff;

				ReverbInterpolate(InterpolatedFilter, Filter, Amount);

				TotalContribution += Amount;
				FilterHit++;
			}

			// Calculate player direction from centerpoint of physmodel
			CVec3Dfp32 Center = pObj->GetPosition();
			CVec3Dfp32 CenterPlayerDirection = (_Pos-Center).Normalize();

			//
			for(int32 s = 0; s < 4; s++)
			{
				// Set volume, if the CollisionInfo isn't valid, then the lis
				fp32 Volume = 1;

				// Get and check sound
				int32 iSound = pObj->m_Data[s*2];
				if(!iSound)
					continue;

				// Unpack the  direction
				CVec3Dfp32 Dir;
				Dir.Unpack24(pObj->m_Data[s*2+1]&0xFFFFFF, 1.01f);
				uint8 FallOffModifyer = (pObj->m_Data[s*2+1]>>24)&0xFF;

				// Calculate Falloff
				fp32 CurrentFalloff = FallOff;

				if(FallOffModifyer != 100)
					CurrentFalloff = CurrentFalloff*(FallOffModifyer/100.0f);

				if(MinFallOff)	// Directional falloff
				{
					fp32 Amount = (-Dir-CenterPlayerDirection).Length()/2.0f;
					CurrentFalloff = MinFallOff + (CurrentFalloff-MinFallOff)*Amount;
				}

				// Calc volume
				int32 LeftOver = int32(FallOff-CurrentFalloff);
				if(CollisionInfo.m_bIsValid)
					Volume = ((-CollisionInfo.m_Distance)-LeftOver)/CurrentFalloff;

				if(Volume > 1)
					Volume = 1;

				Volume *= m_Sound_VolumeSoundVolume;

				// Find if the sound is currently playing
				int32 SoundID = -1;

				for(int32 c = 0; c < m_lDirectionalSounds.Len(); c++)
				{
					if(m_lDirectionalSounds[c].m_iObject != pObj->m_iObject)
						continue;

					if(m_lDirectionalSounds[c].m_ObjectSoundNumber != s)
						continue;

					SoundID = c;
					break;
				}

				// Create new sound if we didn't find one
				if(SoundID == -1)
				{
					CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(iSound);

					if(!pDesc)		// Should not happen because it's check during init.
						continue;

					int32 iWave = pDesc->GetPlayWaveId();
					

					if(pDesc->GetWaveId(iWave) >= 0)
					{

						CSC_VoiceCreateParams3D CreateParams;
						m_spSound->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());

						CreateParams.m_bLoop = true;
						CreateParams.m_hChannel = m_hChannels[WCLIENT_CHANNEL_SFXLOOPING];
						CreateParams.m_3DProperties.m_MinDist = 128;
						CreateParams.m_3DProperties.m_MaxDist = 256-16;
						CreateParams.m_3DProperties.m_Position = _Pos-Dir*256;
						CreateParams.m_bDoppler = false;
						CreateParams.m_bLoop = true;

						int32 iVoice = Sound_CreateVoice(pDesc, iWave, CreateParams);

						if(iVoice < 0)
							continue;

						// Add sound to list
						SoundID = m_lDirectionalSounds.Add(CDirectionalSound());

						// Set data so we can identify the sound later on
						m_lDirectionalSounds[SoundID].m_iObject = pObj->m_iObject;
						m_lDirectionalSounds[SoundID].m_ObjectSoundNumber = s;
						m_lDirectionalSounds[SoundID].m_iVoice = iVoice;
					}
				}

				// Update Sound
				if(SoundID != -1)
				{
					// Remove the deletion flag
					m_lDirectionalSounds[SoundID].m_Flags.m_Delete = 0;
					m_spSound->Voice3D_SetOrigin(m_lDirectionalSounds[SoundID].m_iVoice, _Pos-Dir*128-Dir*128*(1-Volume));
				}
			}
		}
	}

	if (FilterHit)
	{
		ReverbInterpolateFinish(InterpolatedFilter, TotalContribution);
//		m_spSound->Listen3D_SetFilter(&InterpolatedFilter);
		m_Sound_LastestNumHit = FilterHit;
	}
	else if (m_Sound_LastestNumHit)
	{
		CSoundContext::CFilter Filter;
		Filter.SetDefault();
//		m_spSound->Listen3D_SetFilter(&Filter);
		m_Sound_LastestNumHit = FilterHit;
	}

	// Remove all sounds that have the deletion flag set
	for(int32 i = 0; i < m_lDirectionalSounds.Len(); i++)
	{
		// Skip all that doesn't have the flag
		if(!m_lDirectionalSounds[i].m_Flags.m_Delete)
			continue;

		// Destroy voice and remove from list
		m_spSound->Voice_Destroy(m_lDirectionalSounds[i].m_iVoice);
		m_lDirectionalSounds.Del(i);
		i--;
	}

}

void CWorld_ClientCore::Sound_SetSoundVolumesVolume(fp32 _Volume)
{
	m_Sound_VolumeSoundVolume = _Volume;
}

void CWorld_ClientCore::Sound_Clean()
{
	Sound_KillVoices();

	// Destroy list
	m_lTrackingSounds.Destroy();

	// Destroy list
	m_lDirectionalSounds.Destroy();
}



// Add voice to sync group
void CWorld_ClientCore::Sound_AddVoiceToSyncGroup(int _hVoice, uint32 _GroupID)
{
	M_ASSERTHANDLER(_GroupID != 0, "Invalid input!", return);

	// Get (and if needed, create) sync group.
	// (this is needed even if the voice create failed, since otherwise UpdateSyncGroups will never send a sync command back to server)
	CWC_SoundSyncGroups::SGroup& g = m_SoundSyncGroups.Get(_GroupID);

	if (m_spSound && _hVoice > 0)
	{
		g.m_lVoices.Add(_hVoice);
//		m_spSound->Voice_Pause(_hVoice); This is now handled when starting the sound
	}
}


// Checks to see if any sound sync groups are ready to be released
void CWorld_ClientCore::Sound_UpdateSyncGroups()
{
	MSCOPESHORT(CWorld_ClientCore::Sound_UpdateSyncGroups);

	TAP<CWC_SoundSyncGroups::SGroup> pGroups = m_SoundSyncGroups.GetAll();
	for (int i = 0; i < pGroups.Len(); i++)
	{
		if (pGroups[i].m_bReadyToGo)
			continue;

		bool bAllDone = true;
		TAP<int> pVoices = pGroups[i].m_lVoices;
		for (int j = 0; j < pVoices.Len(); j++)
			if (!m_spSound->Voice_ReadyForPlay(pVoices[j]))
			{
				bAllDone = false;
				break;
			}

		if (bAllDone)
		{
			pGroups[i].m_bReadyToGo = true;			// mark as ready to prevent spamming the server while waiting for reply
			CNetMsg Msg(WPACKET_SOUNDSYNC);
			Msg.AddInt32(pGroups[i].m_GroupID);
			Net_PutMsg(Msg);
		}
	}
}

