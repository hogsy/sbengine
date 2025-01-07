/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Class that plays a 2D sound and fades it after distance.

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWObject_MusicFader
				CWObject_MusicTrackFader

Comments:

History:		
060727:			Created file
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_MusicFader.h"
#include "../WObj_Game/WObj_GameCore.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MusicFader, CWObject, 0x0100);

CWObject_MusicFader::CWObject_MusicFader()
{
	m_Range = 320.0f;
	m_InnerRange = 32.0f;
	m_iSound = 0;
	m_bPlaying = false;
}

void CWObject_MusicFader::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	switch(_KeyHash)
	{
	case MHASH2('MUSI','C'):
		{
			m_iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
		}
		break;
	case MHASH2('RANG','E'):
		{
			m_Range = _pKey->GetThisValuef();
		}
		break;
	case MHASH3('INNE','R_RA','NGE'):
		{
			m_InnerRange = _pKey->GetThisValuef();
		}
	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
		}
		break;
	};
}

void CWObject_MusicFader::OnCreate()
{
	CWObject::OnCreate();

	ClientFlags() |= (CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_INVISIBLE);
}

aint CWObject_MusicFader::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			if(!m_bPlaying)
			{
				CVec3Dfp32 Offset(0.0f);
				//We store the range information we need in offset, since this will be sent over anyway and isn't used for 2D sound
				Offset.k[0] = m_InnerRange;
				Offset.k[1] = m_Range;
				m_pWServer->Sound_On(m_iObject, m_iSound, WCLIENT_ATTENUATION_2D_POS, 0, 1.0f, -1, 0, Offset);
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				m_bPlaying = true;
			}
			return 1;
		}
		break;
	default:
		{
			return CWObject::OnMessage(_Msg);
		}
		break;
	};	
}

void CWObject_MusicFader::OnRefresh(void)
{
	CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Game_GetObject()->Player_GetObjectIndex(0));
	CVec3Dfp32 Vec = pObj->GetPosition() - GetPosition();
	if(Vec.LengthSqr() > ((m_Range * m_Range) + 25600.0f))	//+ 5 meters
	{
		m_pWServer->Sound_On(m_iObject, m_iSound, WCLIENT_ATTENUATION_OFF);
		m_bPlaying = false;
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_MusicTrackFader, CWObject_MusicFader, 0x0100);

CWObject_MusicTrackFader::CWObject_MusicTrackFader()
{
	m_iTrack = 0;
}

void CWObject_MusicTrackFader::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	switch(_KeyHash)
	{
	case MHASH2('TRAC','K'):
		{
			m_iTrack = _pKey->GetThisValuei();
		}
	default:
		{
			CWObject_MusicFader::OnEvalKey(_KeyHash, _pKey);
		}
		break;
	};
}

void CWObject_MusicTrackFader::OnCreate()
{
	CWObject_MusicFader::OnCreate();

	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

void CWObject_MusicTrackFader::OnRefresh(void)
{
	CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Game_GetObject()->Player_GetObjectIndex(0));
	CVec3Dfp32 Vec = pObj->GetPosition() - GetPosition();
	fp32 l = Vec.Length();
	fp32 p = 1.0f;
	if(l > m_Range)
		p = 0.0f;
	else if(l > m_InnerRange)
		p = 1.0f - ((l - m_InnerRange) / (m_Range - m_InnerRange));

	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_pWServer->Game_GetObjectIndex());
	if(pCD)
	{
		if(p)
		{
			if(pCD->m_liMusicVolumeModifier[m_iTrack] < p)
			{
				pCD->m_liMusicVolumeModifier[m_iTrack] = p;
				pCD->m_liMusicVolumeModifier.MakeDirty();
			}
		}
	}
}


