#include "PCH.h"
#include "../../../MCC/MCC.h"
#include "../MSystem.h"
#include "MInputEnvelope.h"


//
// TODO: Fix so the client's GetGameTime is used instead of GetCpuClock/GetCpuFrequency
//
CInputEnvelopePoint::CInputEnvelopePoint() : m_fValue( 0.0f ), m_fTime( 0.0 )
{
}


//
//
//
CInputEnvelopePoint::CInputEnvelopePoint(fp32 _fTime, fp32 _fValue) : m_fValue( _fValue ), m_fTime( _fTime )
{
}


// -------------------------------------------------------------------


//
//
//
fp32 CInputEnvelopeChannel::GetFeedbackForce(fp32 _fTime)
{
	if(!m_lPoints.Len())
		return 0.0f;

	if(m_lPoints.Len() == 1)
		return m_lPoints[0]->m_fValue;

	int32 i;
	for(i = 0; i < m_lPoints.Len(); i++)
	{
		if(m_lPoints[i]->m_fTime > _fTime)
			break;
	}

	// Check if this envelope shouldn't be here
	if(i == m_lPoints.Len())
		return 0;

	// Special case
	if(i == 0)
		return m_lPoints[0]->m_fValue;

	// Interpolate
	fp32 fTimeSlice = m_lPoints[i]->m_fTime - m_lPoints[i-1]->m_fTime;
	fp32 Amount = (_fTime - m_lPoints[i-1]->m_fTime) / fTimeSlice;
	fp32 fForce = m_lPoints[i-1]->m_fValue + (m_lPoints[i]->m_fValue - m_lPoints[i-1]->m_fValue) * Amount;

	return fForce;
}


// Called from CInputEnvelope, never call directly becouse CInputEnvelope need to process the point too
void CInputEnvelopeChannel::AddPoint(const fp32 _fTime, const fp32 _fValue)
{
	spCInputEnvelopePoint spPoint = MNew2(CInputEnvelopePoint, _fTime, _fValue );
	m_lPoints.Add(spPoint);
}

// -------------------------------------------------------------------



//
//
//
CInputEnvelope::CInputEnvelope() : m_fEndTime( 0.0 )
{
}


//
//
//
void CInputEnvelope::Create( const CStr &_name )
{
	m_Name	= _name;
}


//
//
//
spCInputEnvelopeChannel CInputEnvelope::GetChannel(int32 _Channel)
{
	for(int32 i = 0; i < m_lChannels.Len(); i++)
	{
		if(m_lChannels[i]->m_ID == _Channel)
			return m_lChannels[i];
	}

	return NULL;
}


//
// (JK?) No support for sustain or loop yet
//
fp32 CInputEnvelope::GetFeedbackForce(fp32 _fTime, int32 _Channel)
{
	if( _fTime < 0)
		return 0.0f;

	//
	// Find channel
	//
	spCInputEnvelopeChannel spChannel = GetChannel(_Channel);
	if(spChannel == NULL)
		return 0.0f;

	return spChannel->GetFeedbackForce(_fTime);
}


//
//
//
void CInputEnvelope::AddPoint( const fp32 _fTime, const fp32 _fValue, int32 _Channel)
{
	if(_fTime < 0)
		return;

	spCInputEnvelopeChannel spChannel = GetChannel(_Channel);
	if(spChannel == NULL)
		return;

	if (m_fEndTime < _fTime)
		m_fEndTime = _fTime;

	spChannel->AddPoint(_fTime, _fValue);
}

// -------------------------------------------------------------------


//
//
//
CInputEnvelopeInstance::CInputEnvelopeInstance(CInputEnvelope *_pEnvelope ) : m_fCeiling( 1.0f )
{
	m_pEnvelope	= _pEnvelope;
	m_fStartTime = CMTime::GetCPU();
}


//
//
//
fp32 CInputEnvelopeInstance::GetFeedbackForce(CMTime _fTime, int32 _Channel)
{
	spCInputEnvelopeChannel spChannel = m_pEnvelope->GetChannel(_Channel);
	
	if(spChannel == NULL)
		return 0;

	return spChannel->GetFeedbackForce((_fTime - m_fStartTime).GetTime());
}


//
//
//
bool CInputEnvelopeInstance::IsActive( CMTime _fTime )
{
	return (m_pEnvelope->m_fEndTime > (_fTime - m_fStartTime).GetTime());
}

// -------------------------------------------------------------------


//
//
//

CInputEnvelopeInstanceList::CInputEnvelopeInstanceList()
{
}


//
//
//

spCInputEnvelopeInstance CInputEnvelopeInstanceList::AppendEnvelope(CInputEnvelope *_pEnvelope)
{
	if (!_pEnvelope)
		return NULL;
	
	spCInputEnvelopeInstance spInstance = MNew1(CInputEnvelopeInstance, _pEnvelope );
	if (!spInstance)
		return NULL;

	m_Instances.Add( spInstance );
	return spInstance;
}


//
//
//

void CInputEnvelopeInstanceList::FlushEnvelopes()
{
	m_Instances.Clear();
}


//
//
//

spCInputEnvelopeInstance CInputEnvelopeInstanceList::SetEnvelope(CInputEnvelope *_pEnvelope)
{
	FlushEnvelopes();
	return AppendEnvelope( _pEnvelope );
}


//
//
//

void CInputEnvelopeInstanceList::RemoveEnvelope(CInputEnvelopeInstance *_pEnvelope)
{
	for(int32 i = 0; i < m_Instances.Len(); i++)
	{
		if (m_Instances[i] == _pEnvelope)
		{
			m_Instances[i] = NULL;
			m_Instances.Del( i );
			return;
		}
	}
}


//
//
//

fp32 CInputEnvelopeInstanceList::GetFeedbackForce(CMTime _fTime, int32 _Channel)
{
	fp32 fValue = 0.0f;

	for (int32 i = 0; i < m_Instances.Len(); i++)
		fValue += m_Instances[i]->GetFeedbackForce(_fTime, _Channel);

	fValue = (fValue < 0.0f) ? 0.0f : (fValue > 1.0f) ? 1.0f : fValue;

	return fValue;
}


//
//
//

void CInputEnvelopeInstanceList::Update()
{
	CMTime fCurrentEnvelopeTime = CMTime::GetCPU();

	for (int32 i = 0; i < m_Instances.Len();)
	{
		if (!m_Instances[i]->IsActive(fCurrentEnvelopeTime))
			m_Instances.Del( i );
		else
			i++;
	}
}

// -------------------------------------------------------------------


//
//
//

CPlayerInputEnvelopeInstanceList::CPlayerInputEnvelopeInstanceList()
{
}


//
//
//

void CPlayerInputEnvelopeInstanceList::Create()
{
	//
	// HACK: This is just for debugging purposes.. find a good way to do this later
	//
	for(int32 i = 0; i < INPUT_MAXGAMEPADS; i++)
	{
		spCInputEnvelopeInstanceList spPlayer = MNew(CInputEnvelopeInstanceList);
		if (!spPlayer)
			Error_static("CPlayerInputEnvelopeInstanceList::Create", "Create");
		m_PlayerLists.Add( spPlayer );
	}
}


//
//
//

spCInputEnvelopeInstance CPlayerInputEnvelopeInstanceList::AppendEnvelope(const int _index, CInputEnvelope *_pEnvelope)
{
	if(!m_PlayerLists.ValidPos(_index))
		return NULL;

	return m_PlayerLists[_index]->AppendEnvelope( _pEnvelope );
}


//
//
//

spCInputEnvelopeInstance CPlayerInputEnvelopeInstanceList::SetEnvelope(const int _index, CInputEnvelope *_pEnvelope)
{
	if(!m_PlayerLists.ValidPos(_index))
		return NULL;

	return m_PlayerLists[_index]->SetEnvelope( _pEnvelope );
}


//
//
//

void CPlayerInputEnvelopeInstanceList::FlushEnvelopes(const int _index)
{
	if (!m_PlayerLists.ValidPos(_index))
		return;
	m_PlayerLists[_index]->FlushEnvelopes();
}


//
//
//

void CPlayerInputEnvelopeInstanceList::FlushEnvelopes( )
{
	for(int32 i = 0; i < m_PlayerLists.Len(); i++)
		m_PlayerLists[i]->FlushEnvelopes();
}


//
//
//

void CPlayerInputEnvelopeInstanceList::RemoveEnvelope( const int _index, CInputEnvelopeInstance *_pEnvelope )
{
	if(!m_PlayerLists.ValidPos(_index))
		return;

	m_PlayerLists[_index]->RemoveEnvelope(_pEnvelope);
}


//
//
//

fp32 CPlayerInputEnvelopeInstanceList::GetFeedbackForce( int _index, CMTime _fTime, int32 _Channel)
{
	if (!m_PlayerLists.ValidPos(_index))
		return 0.0f;

	return m_PlayerLists[_index]->GetFeedbackForce(_fTime, _Channel);
}


//
//
//

void CPlayerInputEnvelopeInstanceList::Update()
{
	for(int32 i = 0; i < m_PlayerLists.Len(); i++)
		m_PlayerLists[i]->Update();
}

// -------------------------------------------------------------------


//
//
//

CInputEnvelopeList::CInputEnvelopeList()
{
}


//
//
//

void CInputEnvelopeList::Create()
{
#ifdef PLATFORM_CONSOLE
	spCRegistry	spRegistry = REGISTRY_CREATE;
	
	MACRO_GetRegisterObject( CSystem, pSys, "SYSTEM" );

	CStr Path = pSys->GetEnvironment()->GetValue("DEFAULTGAMEPATH", "Content\\");
	CStr File = pSys->m_ExePath + Path.GetStrSep(";") + "Feedback\\Feedback.xrg";
	if(CDiskUtil::FileExists(File))
		spRegistry->XRG_Read(File);
	else
		ConOutL(CStrF("Could not find ForceFeedback Envelopes (%s)", Path.Str()));

	m_Envelopes.Clear();	// Make sure list is cleared

	// Find envelops
	for(int32 i = 0; i < spRegistry->GetNumChildren(); i++)
	{
		CRegistry *pChild = spRegistry->GetChild(i);

		if(pChild->GetThisName().Compare("ENVELOPE") == 0)
		{
			CInputEnvelope *pEnvelope = MNew(CInputEnvelope);
			pEnvelope->Create(pChild->GetThisValue());

			// Find the channels
			for(int j = 0; j < pChild->GetNumChildren(); j++)
			{
				CRegistry *pChannelReg = pChild->GetChild(j);
			
				if(pChannelReg->GetThisName().Compare("CHANNEL") == 0)
				{
					CInputEnvelopeChannel *pChannel = MNew(CInputEnvelopeChannel);
					pChannel->m_ID = pChannelReg->GetThisValuei();
					pEnvelope->m_lChannels.Add(pChannel);

					// Finaly add the points to the channel
					for(int j = 0; j < pChannelReg->GetNumChildren(); j++)
					{
						CRegistry *pPoint = pChannelReg->GetChild(j);

						if(pPoint->GetThisName().Compare("POINT") == 0)
						{
							fp32 fTime = pPoint->GetValuef("TIME");
							fp32 fForce = pPoint->GetValuef("FORCE");

							// We must add the points thru the Envelope so it can update it's endtime
							pEnvelope->AddPoint(fTime, fForce, pChannel->m_ID);
						}
						else
						{
							// Unknown thingie in the registry.. complain and move on
							LogFile(CStrF("Unknown ENVELOPE registry entry '%s', skipping.", pPoint->GetThisName().Str()));
						}
					}
				}
			}

			m_Envelopes.Add(pEnvelope);
		}
		else
		{
			// Unknown thingie in the registry.. complain and move on
			LogFile(CStrF("Unknown ENVELOPE registry entry '%s', skipping.", pChild->GetThisName().Str()));
		}
	}
#endif	
	//*/
}


//
//
//

spCInputEnvelope CInputEnvelopeList::FindEnvelope(const CStr &_name)
{
	for(int32 i = 0; i < m_Envelopes.Len(); i++ )
	{
		if(_name.Compare( m_Envelopes[i]->m_Name) == 0)
			return m_Envelopes[i];
	}

	return NULL;
}
