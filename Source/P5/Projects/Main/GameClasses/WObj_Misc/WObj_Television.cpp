/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Television.cpp

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_Television implementation

	Comments:

	History:		
		050811:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Television.h"

#define DBG_OUT DO_IF(0) M_TRACEALWAYS


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_FlushTextureCallback
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_FlushTextureCallback
{
public:
	uint16 m_TextureID;

	static void DoWork(CRenderContext* _pRC, CXR_VBManager* , CXR_VertexBuffer* , void* _pContext, CXR_VBMScope* , int )
	{
		uint16 TextureID = ((CXR_FlushTextureCallback*)_pContext)->m_TextureID;
		_pRC->Texture_Flush(TextureID);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Television
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Television, CWObject_Television::parent, 0x0100);

void CWObject_Television::OnCreate()
{
	parent::OnCreate();

	m_iChannel = 0;
	m_AttnMin = 32.0f;
	m_AttnMax = 128.0f;
	m_ViewMax = 256.0f;
	m_Volume = 1.0f;

	m_ChannelChangeTicks = -1;
}

void CWObject_Television::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	switch (_KeyHash)
	{
	case MHASH2('CHAN','NEL'): // "CHANNEL"
		{
			m_iChannel = KeyValue.Val_int();
			break;
		}
	case MHASH2('ATTN','MIN'): // "ATTNMIN"
		{
			m_AttnMin = _pKey->GetThisValuef();;
			break;
		}
	case MHASH2('ATTN','MAX'): // "ATTNMAX"
		{
			m_AttnMax = _pKey->GetThisValuef();;
			break;
		}
	case MHASH2('VOLU','ME'): // "VOLUME"
		{
			m_Volume = _pKey->GetThisValuef();;
			break;
		}
	case MHASH3('VIEW','RANG','E'): // "VIEWRANGE"
		{
			m_ViewMax = _pKey->GetThisValuef();;
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("CHANNELSURFACE") == 0)
			{
#if 1
				uint8 iSlot = KeyName.RightFrom(14).Val_int();
				if (m_liSurfaces.Len() < (iSlot+1))
				{
					m_liSurfaces.SetLen(iSlot+1);
					m_liSounds.SetLen(iSlot+1);
				}

				int iSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
				m_liSurfaces[iSlot] = iSurface;

				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
				m_liSounds[iSlot] = iSound;

				DBG_OUT("[Television %d, %s], added channel '%s' (slot: %d, surface: %d, sound: %d)\n", 
					m_iObject, GetName(), KeyValue.Str(), iSlot, iSurface, iSound);
#else
				m_liSurfaces.SetLen(2);
				m_liSounds.SetLen(2);
				m_liSurfaces[0] = m_pWServer->GetMapData()->GetResourceIndex_Surface("VIDEO_Unforgiven");
				m_liSounds[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound("VIDEO_Unforgiven");
				m_liSurfaces[1] = m_pWServer->GetMapData()->GetResourceIndex_Surface("VIDEO_Simpsons");
				m_liSounds[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound("VIDEO_Simpsons");
#endif
			}
			else
				parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_Television::ObjectSpawn(bool _bSpawn)
{
	parent::ObjectSpawn(_bSpawn);

	CWO_PhysicsState Phys = GetPhysState();
	Phys.m_ObjectFlags |= OBJECT_FLAGS_PICKUP;
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	UpdateClient();
}


void CWObject_Television::UpdateClient()
{
	Data(DATA_TV_SURFACEID) = (m_liSurfaces.ValidPos(m_iChannel)) ? m_liSurfaces[m_iChannel] : 0;
	Data(DATA_TV_SOUNDID) = (m_liSounds.ValidPos(m_iChannel)) ? m_liSounds[m_iChannel] : 0;
	Data(DATA_TV_ATTENUATION) = Min(uint32(m_AttnMin), uint32(0xFFff)) | Min(uint32(m_AttnMax),uint32(0xFFff)) << 16;
	Data(DATA_TV_RANGE_VOLUME) = Min(uint32(m_ViewMax), uint32(0xFFff)) | Min(uint32(m_Volume * 0xFFff),uint32(0xFFff)) << 16;

	DBG_OUT("[Television %d, %s], UpdateClient: channel: %d\n",
		m_iObject, GetName(), m_iChannel);
}


void CWObject_Television::OnFinishEvalKeys()
{
	parent::OnFinishEvalKeys();
}


aint CWObject_Television::OnMessage(const CWObject_Message& _Msg)
{
	bool bWasOff = (m_iChannel == 0);

	switch (_Msg.m_Msg)
	{
	case OBJMSG_TELEVISION_SETCHANNEL:
		{
			int iNewChannel = _Msg.m_Param0;

			if (bWasOff)
			{
				m_iChannel = iNewChannel;
				UpdateClient();
			}
			else
			{
				// set "off"
				m_iChannel = 0;
				UpdateClient();

				// set correct channel after a few ticks
				m_iChannel = iNewChannel;
				m_ChannelChangeTicks = 3;
			}
			return 0;
		}

	case OBJMSG_USE:
		{
			int iNewChannel = m_liSurfaces.Len() ? (m_iChannel + 1) % m_liSurfaces.Len() : 0;

			if (bWasOff)
			{
				m_iChannel = iNewChannel;
				UpdateClient();
			}
			else
			{
				// set "off"
				m_iChannel = 0;
				UpdateClient();

				// set correct channel after a few ticks
				m_iChannel = iNewChannel;
				m_ChannelChangeTicks = 3;
			}
			return 0;
		}
	}
	return parent::OnMessage(_Msg);
}


void CWObject_Television::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	parent::OnDeltaLoad(_pFile, _Flags);

	// Set correct channel
	m_iChannel = 0;
	for (uint i = 0; i < m_liSurfaces.Len(); i++)
		if (m_liSurfaces[i] == Data(DATA_TV_SURFACEID))
		{
			m_iChannel = i;
			break;
		}

	UpdateClient();
}

void CWObject_Television::OnRefresh()
{
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	if (m_ChannelChangeTicks >= 0)
	{
		if (m_ChannelChangeTicks == 0)
			UpdateClient();

		m_ChannelChangeTicks--;
	}
}


int CWObject_Television::GetTextureFromSurface(CWorld_Client* _pWClient, int _iSurface)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
		CFStr SurfName = _pWClient->GetMapData()->GetResourceName(_iSurface);
		SurfName = CFStr("*") + SurfName.CopyFrom(4);
		return pTC->GetTextureID(SurfName);
	}
	return 0;
}


int CWObject_Television::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	int32& Data2 = _pObj->m_Data[DATA_TV_SURFACEID];

	int OldSurface = Data2 & 0xffff;
	int OldSound = _pObj->m_Data[DATA_TV_SOUNDID];
	int ret = parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	int NewSurface = Data2 & 0xffff;

	if ((OldSound > 0) && (NewSurface != OldSurface))
	{
		int iTex = GetTextureFromSurface(_pWClient, OldSurface);
		if (iTex > 0)
		{
			Data2 &= 0xffff;
			Data2 |= (iTex << 16);  // Set FlushTextureID
		}
	}
	return ret;
}


void CWObject_Television::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Television::OnClientRefresh);

	int iPlayerObj = _pWClient->Player_GetLocalObject();
	CWObject_CoreData* pPlayerObj = _pWClient->Object_GetCD(iPlayerObj);
	if (pPlayerObj)
	{
		CSoundContext* pSoundContext = safe_cast<CWorld_ClientCore>(_pWClient)->m_spSound;
		if (!pSoundContext)
			return;

		int iSound = _pObj->m_Data[DATA_TV_SOUNDID];
		if (iSound != -_pObj->m_iSoundPlaying[0])
		{
			// Kill old sound
			if (_pObj->m_hVoice[0] > 0)
				pSoundContext->Voice_Destroy(_pObj->m_hVoice[0]);

			// Play new sound
			fp32 Volume =  (uint32(_pObj->m_Data[DATA_TV_RANGE_VOLUME]) >> 16) / 65536.0f;
			fp32 AttnMin = (uint32(_pObj->m_Data[DATA_TV_ATTENUATION]) & 0xFFff);
			fp32 AttnMax = (uint32(_pObj->m_Data[DATA_TV_ATTENUATION]) >> 16);
			_pObj->m_hVoice[0] = _pWClient->Sound_On(WCLIENT_CHANNEL_CUTSCENE, _pObj->m_iObject, 
						iSound, WCLIENT_ATTENUATION_3D_OVERRIDE, 0, Volume, CVec3Dfp32(AttnMin, AttnMax, 0.0f));

			if (_pObj->m_hVoice[0] > 0)
			{
				DBG_OUT("[Television %d], started new sound: %d (hVoice: %d)\n", _pObj->m_iObject, iSound, _pObj->m_hVoice[0]);

				_pObj->m_iSoundPlaying[0] = -iSound;

				int iSurface = _pObj->m_Data[DATA_TV_SURFACEID] & 0xffff;
				DBG_OUT(" - surface: %d\n", iSurface);
				int iTex = GetTextureFromSurface(_pWClient, iSurface);
				if (iTex > 0)
				{
					MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
					CTextureContainer_Video* pVideoTC = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(iTex));
					int iLocal = pTC->GetLocal(iTex);
					if (pVideoTC && iLocal >= 0)
					{
						pVideoTC->SetSoundHandle(iLocal, _pObj->m_hVoice[0]);
					}
				}
			}
		}

		if (_pObj->m_hVoice[0] > 0)
		{
			fp32 ViewMax = (uint32(_pObj->m_Data[DATA_TV_RANGE_VOLUME]) & 0xffFF);
//			CSC_SFXDesc* pDesc = _pWClient->GetMapData()->GetResource_SoundDesc(iSound);
//			if (pDesc)
			{
				const CVec3Dfp32& Pos = _pObj->GetPosition();
				const CVec3Dfp32& PlayerPos = pPlayerObj->GetPosition();
				fp32 DistSqr = Pos.DistanceSqr(PlayerPos);
				if (DistSqr > Sqr(ViewMax))
				{
					// Pause sound
					pSoundContext->Voice_Pause(_pObj->m_hVoice[0]);
				}
				else
				{
					pSoundContext->Voice_Unpause(_pObj->m_hVoice[0]);
				}
			}
		}
	}
}


void CWObject_Television::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Television_OnClientRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Television::OnClientRender);

	// Flush old video texture?
	int16 FlushTextureID = (_pObj->m_Data[DATA_TV_SURFACEID] >> 16);
	if (FlushTextureID)
	{
		CXR_VBManager* pVBM = _pEngine->GetVBM();
		if (pVBM)
		{
			CXR_FlushTextureCallback* pCallback = (CXR_FlushTextureCallback*)pVBM->Alloc(sizeof(CXR_FlushTextureCallback));
			if (pCallback)
			{
				pCallback->m_TextureID = FlushTextureID;
				pVBM->AddCallback(CXR_FlushTextureCallback::DoWork, pCallback, 0.0f);

				// Clear FlushTextureID
				_pObj->m_Data[DATA_TV_SURFACEID] &= 0xffff;
			}
		}
	}

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);

	// Override surface group 1
	int16 iChannelSurface = _pObj->m_Data[DATA_TV_SURFACEID] & 0xffff;
	uint32 OnRenderFlags = CXR_MODEL_ONRENDERFLAGS_SURF1_ADD;
	AnimState.m_lpSurfaces[1] = _pWClient->GetMapData()->GetResource_Surface(iChannelSurface);

	DoRender(_pObj, _pWClient, _pEngine, AnimState, OnRenderFlags);
}

