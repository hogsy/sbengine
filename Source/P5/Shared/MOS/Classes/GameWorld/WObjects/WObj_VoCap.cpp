/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_VoCap.h

Author:			Jens Andersson

Copyright:		2005-2006 Starbreeze Studios AB

Contents:		CVoCap, CVoCap_AnimItem

Comments:

History:		
051017:		Created file
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_VoCap.h"
#include "../WAnimGraph2Instance/WAG2I_StateInst.h"
#include "WObj_AnimUtils.h"

// Errors in data received from Centroid. It's still not clear if this is due to error when recording sound or the animation
#define P5_TIMECODE_STARTSCALE (30.0f / 29.97f)		// Some mixup between 30Hz and NTCS standard 30.0f / 29.97f causes this mul to be needed on timecode.
#define P5_TIMECODE_OFFSET (-0.2f)					// This could be an engine thing, but good matching between sound and animation seem to need a 0.2sec offset
//#define P5_TIMECODE_TIMESCALE (30.0f / 29.85f)		// Even though the start now is correct, it seems like a scale of 99.5% is needed to keep them in sync.
#define P5_TIMECODE_TIMESCALE (30.0f / 29.82f)		// Even though the start now is correct, it seems like a scale of 99.5% is needed to keep them in sync.


#ifndef M_RTM
class CVoCapDebug
{
public:
	CMTime m_LastRefreshTime;
	uint m_nEntries;
	NThread::CMutual m_Lock;

	void Update(CWorld_Client& _WClient, fp32 _WaveTime, fp32 _AnimTime, fp32 _Diff)
	{
		DLock(m_Lock);
		CMTime t = _WClient.GetRenderTime();
		if (t != m_LastRefreshTime)
		{
			m_nEntries = 0;
			m_LastRefreshTime = t;
		}

		CXR_Engine* pEngine = _WClient.Render_GetEngine();
		CDebugRenderContainer* pDebugRender = _WClient.Debug_GetWireContainer();
		if (pEngine && pDebugRender)
		{
			const CMat4Dfp32& VMat = pEngine->GetVC()->m_CameraWMat;

			CVec3Dfp32 Pos;
			Pos.k[0] = 0.0f;
			Pos.k[1] = -3.0f + 0.4f * m_nEntries;
			Pos.k[2] = 10.0f;
			Pos *= VMat;

			CFStr Text = CFStrF("W: %.3f,  A: %.3f,  tcd: %.3f", _WaveTime, _AnimTime, _Diff);
			pDebugRender->RenderText(Pos, Text.Str(), 0xffff7f7f, 0.0f, false);
		}
		m_nEntries++;
	}
};
static CVoCapDebug s_VoCapDebug;
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVoCap_AnimItem
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CVoCap_AnimItem::SetTime(fp32 _Time)
{
	m_Time = _Time;
}

static fp32 Sinc(fp32 _x)
{
	return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;
}

CVoCap_AnimItem &CVoCap_AnimItem::operator =(const CVoCap_AnimItem& _AnimItem)
{
	m_Time = _AnimItem.m_Time;
	m_EventTime = _AnimItem.m_EventTime;
	for(int i = 0; i < VOCAP_NUM_BLENDS; i++)
	{
		m_Blend[i] = _AnimItem.m_Blend[i];
		m_BlendInTime[i] = _AnimItem.m_BlendInTime[i];
		m_BlendOutTime[i] = _AnimItem.m_BlendOutTime[i];
	}
	m_spCurSequence = _AnimItem.m_spCurSequence;
	m_Flags = _AnimItem.m_Flags;
	m_iMovingHold = _AnimItem.m_iMovingHold;
	m_iEventKey = _AnimItem.m_iEventKey;
	m_TimeScale = _AnimItem.m_TimeScale;

	if(!_AnimItem.m_spNextItem)
		m_spNextItem = NULL;
	else
	{
		if(!m_spNextItem)
			m_spNextItem = MNew(CVoCap_AnimItem);
		*m_spNextItem = *_AnimItem.m_spNextItem;
	}
	return *this;
}

void InsertLayer(CXR_AnimLayer &_Layer, int _Pos, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers)
{
	if(_nLayers >= _MaxLayers)
		return;

	for(int i = _nLayers - 1; i >= _Pos; i--)
		_pLayers[i + 1] = _pLayers[i];
	_pLayers[_Pos] = _Layer;
	_nLayers++;
}

CVoCap_AnimItem* CVoCap_AnimItem::Refresh(fp32 _Duration, int _iMovingHold, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers, fp32& _MaxBlend)
{
	if(m_spNextItem)
		m_spNextItem = m_spNextItem->Refresh(_Duration, _iMovingHold, _pLayers, _nLayers, _MaxLayers, _MaxBlend);

	if(m_Flags & FLAGS_ITEMISHOLDREPLACER)
	{
		m_Time += _Duration;
		if(m_Flags & FLAGS_BLENDINGOUT)
		{
			m_Blend[0] = Clamp01(m_Blend[0] - _Duration / m_BlendOutTime[SETHOLD_BO_BLENDOUTTIME]);
			if(m_Blend[0] == 0)
				return m_spNextItem;
		}

		bool bLayerFound = false;
		for(int i = 0; i < _nLayers; i++)
			if(_pLayers[i].m_Flags & CXR_ANIMLAYER_LAYERISIDLE)
			{
				if(m_Blend[0] < 1.0f)
				{
					if(!(m_Flags & FLAGS_BLENDINGOUT))
						m_Blend[0] = Clamp01(m_Blend[0] + _Duration / m_BlendInTime[SETHOLD_BI_BLENDINTIME]);
					if(m_Blend[0] < 1.0f)
					{
						// This Hold is not fully blended in, so we can't just replace the idle-layer since that would give a pop from normal idle to the new hold.
						// Instead we need to add an additional layer for the Hold animation
						CXR_AnimLayer Layer = _pLayers[i];
						Layer.m_Blend *= m_Blend[0];
						InsertLayer(Layer, ++i, _pLayers, _nLayers, _MaxLayers);
					}
				}
				_pLayers[i].m_spSequence = m_spCurSequence;
				fp32 Time = m_Time;
				fp32 LoopDuration = (m_BlendInTime[SETHOLD_BI_END] - m_BlendInTime[SETHOLD_BI_BEGIN]) - m_BlendOutTime[SETHOLD_BO_LOOPBLENDTIME];
				int nLoop = TruncToInt(Time / LoopDuration);
				Time -= nLoop * LoopDuration;
				_pLayers[i].m_Time = Time + m_BlendInTime[SETHOLD_BI_BEGIN] + m_BlendOutTime[SETHOLD_BO_LOOPBLENDTIME];

				if(Time > LoopDuration - m_BlendOutTime[SETHOLD_BO_LOOPBLENDTIME])
				{
					// Nearing the end of the loop. We need to blend in an extra layer with the hold restarted to make sure the loop is smooth
					CXR_AnimLayer Layer = _pLayers[i];
					Layer.m_Blend = m_Blend[0] * Sinc(((LoopDuration - Time) / m_BlendOutTime[SETHOLD_BO_LOOPBLENDTIME]));
					_pLayers[i].m_Time = _pLayers[i].m_Time - LoopDuration;
					InsertLayer(Layer, ++i, _pLayers, _nLayers, _MaxLayers);
				}
				bLayerFound = true;
			}

		if(!bLayerFound)
			m_Blend[0] = Clamp01(m_Blend[0] - _Duration / m_BlendOutTime[SETHOLD_BO_BLENDOUTTIME]);
		return this;
	}

	fp32 Duration = _Duration * m_TimeScale;
	fp32 AnimDuration = m_spCurSequence->GetDuration();
	fp32 NewTime = m_Time + Duration;
	fp32 BlendTrigger = AnimDuration - m_BlendOutTime[0];

	int ValidFlags;
	if((_iMovingHold == 1 && m_iMovingHold == 1) || _iMovingHold == -1)
		ValidFlags = VOCAP_BLEND_LOWERBODY_MASK | VOCAP_BLEND_UPPERBODY_MASK | VOCAP_BLEND_FACE_MASK;
	else
		ValidFlags = VOCAP_BLEND_FACE_MASK;

	int iFinished = 0;
	for(int i = 0; i < VOCAP_NUM_BLENDS; i++)
	{
		if(!(ValidFlags & (1 << i)))
		{
			if(Duration > 0)
				m_Blend[i] = Max(m_Blend[i] - Duration / m_BlendOutTime[i], 0.0f);
		}
		else if(m_Flags & FLAGS_BLENDINGOUT)
		{
			if(Duration > 0)
			{
				m_Blend[i] -= Duration / m_BlendOutTime[i];
				if(m_Blend[i] <= 0)
					iFinished++;
			}
		}
		else if(NewTime > BlendTrigger)
		{
			m_Blend[i] -= Min(NewTime - BlendTrigger, Duration) / m_BlendOutTime[i];
			if(m_Blend[i] <= 0)
				iFinished++;
		}
		else if(m_Blend[i] < 1.0f && m_Time < BlendTrigger && NewTime > 0)
		{
			if(m_BlendInTime[i] == 0)
				m_Blend[i] = 1.0f;
			else
			{
				fp32 DurationClamp = Min(NewTime, Duration);
				m_Blend[i] = Min(1.0f, m_Blend[i] + Min(BlendTrigger - NewTime, DurationClamp) / m_BlendInTime[i]);
			}
		}
	}
	if(iFinished == VOCAP_NUM_BLENDS)
		return m_spNextItem;

	m_Time += Duration;

	if(_nLayers < _MaxLayers)
	{
		if(m_Blend[0] == m_Blend[2] && m_Blend[0] > 0)
		{
			if(m_Blend[0] > 0.999f) // If layer is full, we can restart the layer-list
				_nLayers = 0;
			fp32 Blend = Sinc(m_Blend[0]);
			_pLayers[_nLayers++] = CXR_AnimLayer(m_spCurSequence, CMTime::CreateFromSeconds(m_Time), 1.0f, Blend, 0);
			_MaxBlend = Max(_MaxBlend, Blend);
		}
		else
		{
			if(m_Blend[0] > 0)
			{
				if(m_Blend[0] > 0.999f) // If layer is full, we can restart the layer-list
					_nLayers = 0;
				fp32 Blend = Sinc(m_Blend[0]);
				_pLayers[_nLayers++] = CXR_AnimLayer(m_spCurSequence, CMTime::CreateFromSeconds(m_Time), 1.0f, Blend, 0);
				_MaxBlend = Max(_MaxBlend, Blend);
			}
			if(_nLayers < _MaxLayers && m_Blend[2] > 0)
			{
				fp32 Blend = Sinc(m_Blend[2]);
				_pLayers[_nLayers++] = CXR_AnimLayer(m_spCurSequence, CMTime::CreateFromSeconds(m_Time), 1.0f, Blend, 11, CXR_ANIMLAYER_IGNOREBASENODE);
				_MaxBlend = Max(_MaxBlend, Blend);
			}
		}
	}

	return this;
}

void CVoCap_AnimItem::GetEventLayers(fp32 _Duration, int _iMovingHold, CEventLayer* _pLayers, int32 &_nLayers, int _MaxLayers)
{
	if(m_spNextItem)
	{
		m_spNextItem->GetEventLayers(_Duration, _iMovingHold, _pLayers, _nLayers, _MaxLayers);
		return;
	}

	if((_iMovingHold == 1 && m_iMovingHold == 1) || _iMovingHold == -1)
	{
		_pLayers[_nLayers].m_Layer = CXR_AnimLayer(m_spCurSequence, CMTime::CreateFromSeconds(m_EventTime), 1.0f, Sinc(m_Blend[0]), 0);
		_pLayers[_nLayers++].m_pKey = &m_iEventKey;
	}

	// Update eventtime
	m_EventTime = m_Time;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVoCap
|__________________________________________________________________________________________________
\*************************************************************************************************/
CVoCap::CVoCap()
{
	m_nAnimResources = 0;
}

void CVoCap::Init(uint16 *_piAnimations, int _nAnimations)
{
	memcpy(m_lAnimResources, _piAnimations, _nAnimations * sizeof(uint16));
	m_nAnimResources = _nAnimations;
}

void CVoCap::Init(CStr _Animations, CMapData* _pMapData)
{
	m_nAnimResources = 0;
	m_spAnimQueue = NULL;

	while(_Animations != "")
	{
		int iAnim = _pMapData->GetResourceIndex_Anim(_Animations.GetStrSep(","));
		if(iAnim > 0)
			m_lAnimResources[m_nAnimResources++] = iAnim;
	}
}


CVoCap& CVoCap::operator =(const CVoCap& _VC)
{
	m_LastReferenceTime = _VC.m_LastReferenceTime;
	m_TimeCodeDiff = _VC.m_TimeCodeDiff;
	m_DialogueAnimFlags = _VC.m_DialogueAnimFlags;
	m_hVoice = _VC.m_hVoice;
	m_nAnimResources = _VC.m_nAnimResources;
	for(int i = 0; i < m_nAnimResources; i++)
		m_lAnimResources[i] = _VC.m_lAnimResources[i];

	if(!_VC.m_spAnimQueue)
		m_spAnimQueue = NULL;
	else
	{
		if(!m_spAnimQueue)
			m_spAnimQueue = MNew(CVoCap_AnimItem);
		*m_spAnimQueue = *_VC.m_spAnimQueue;
	}

	return *this;
}


void CVoCap::ClearQueue()
{
	m_spAnimQueue = NULL;
}


uint32 CVoCap::GetWaveNameHash(CSoundContext* _pSound, int _iWave) const
{
	if (_pSound)
	{
		CWaveContext* pWC = _pSound->Wave_GetContext();
		return pWC->GetWaveNameID(_iWave);
	}
	return 0;
}


CXR_Anim_SequenceData* CVoCap::GetSequenceFromName(CMapData* _pMapData, uint32 _NameHash, int* _piAnimRc, int* _piSeq) const
{
	if (!_NameHash)
		return NULL;

	for (uint i = 0; i < m_nAnimResources; i++)
	{
		CXR_Anim_Base* pAnim = _pMapData->GetResource_Anim(m_lAnimResources[i]);
		if (pAnim)
		{
			uint nSeq = pAnim->GetNumSequences();
			for (uint iSeq = 0; iSeq < nSeq; iSeq++)
			{
				CXR_Anim_SequenceData* pSeq = pAnim->GetSequence(iSeq);
				uint32 AnimNameHash = pSeq->GetNameHash();
				if (AnimNameHash == _NameHash)
				{
					if (_piAnimRc) *_piAnimRc = m_lAnimResources[i];
					if (_piSeq)	*_piSeq = iSeq;
					return pSeq;
				}
			}
		}
	}
	return NULL;
}


fp32 CVoCap::GetTimeCodeDiff(CXR_Anim_SequenceData *_pSeq, CSoundContext *_pSound, int _iWave)
{
	fp32 AnimTimeCode = _pSeq->m_TimeCode.GetTime();
	fp32 SoundTimeCode = _pSound->Timecode_Get(_iWave);

	fp32 Res = AnimTimeCode - SoundTimeCode / P5_TIMECODE_STARTSCALE;

#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE)
	if (Abs(Res) > 60)
		ConOutL(CStrF("WARNING: TimeCode is incorrect on line %s (SoundTimeCode: %.2f  AnimTimeCode: %.2f", _pSeq->m_Name.Str(), SoundTimeCode, AnimTimeCode));
#endif

	return Res;
}


void CVoCap::SetVoice(CSoundContext* _pSound, CMapData* _pMapData, int _iWave, uint32 _CustomAnim, int _iMovingHold, int _hVoice, const CMTime &_ReferenceTime, fp32 _StartOffset, uint16 _DialogueAnimFlags)
{
 	StopVoice();

	m_DialogueAnimFlags = _DialogueAnimFlags;

	// if no custom anim is given, use the name of the wave
	uint32 NameHash = _CustomAnim ? _CustomAnim : GetWaveNameHash(_pSound, _iWave);
	CXR_Anim_SequenceData* pSeq = GetSequenceFromName(_pMapData, NameHash);
	if (!pSeq)
		return;

	m_TimeCodeDiff = GetTimeCodeDiff(pSeq, _pSound, _iWave);
	m_LastReferenceTime = _ReferenceTime;

	TPtr<CVoCap_AnimItem> spItem = MNew(CVoCap_AnimItem);
	if(!m_spAnimQueue)
		m_spAnimQueue = spItem;
	else
	{
		spItem->m_spNextItem = m_spAnimQueue;
		m_spAnimQueue = spItem;
	}

	spItem->m_EventTime = 0.0f;
	spItem->m_iEventKey = 0;
	spItem->m_spCurSequence = pSeq;
	spItem->m_Blend[VOCAP_BLEND_LOWERBODY] = 0;
	spItem->m_Blend[VOCAP_BLEND_UPPERBODY] = 0;
	spItem->m_Blend[VOCAP_BLEND_FACE] = 0;
	if(_iMovingHold == 0)
	{
		spItem->m_BlendInTime[VOCAP_BLEND_LOWERBODY] = 0.0f;
		spItem->m_BlendOutTime[VOCAP_BLEND_LOWERBODY] = 1.0f;
		spItem->m_BlendInTime[VOCAP_BLEND_UPPERBODY] = 0.0f;
		spItem->m_BlendOutTime[VOCAP_BLEND_UPPERBODY] = 1.0f;
		spItem->m_BlendInTime[VOCAP_BLEND_FACE] = 0.0f;
		spItem->m_BlendOutTime[VOCAP_BLEND_FACE] = 0.1f;
	}
	else
	{
		spItem->m_BlendInTime[VOCAP_BLEND_LOWERBODY] = 1.0f;
		spItem->m_BlendOutTime[VOCAP_BLEND_LOWERBODY] = 1.0f;
		spItem->m_BlendInTime[VOCAP_BLEND_UPPERBODY] = 1.0f;
		spItem->m_BlendOutTime[VOCAP_BLEND_UPPERBODY] = 1.0f;
		spItem->m_BlendInTime[VOCAP_BLEND_FACE] = 0.1f;
		spItem->m_BlendOutTime[VOCAP_BLEND_FACE] = 0.1f;
	}
	spItem->m_Time = _StartOffset - m_TimeCodeDiff;
	spItem->m_Flags = 0;
	spItem->m_iMovingHold = _iMovingHold;

	spItem->m_TimeScale = P5_TIMECODE_TIMESCALE;
	if(_DialogueAnimFlags & DIALOGUEANIMFLAGS_VOCAP_SPECIALSYNC)
	{
		m_TimeCodeDiff += P5_TIMECODE_OFFSET;
	}
	else
	{
		spItem->m_TimeScale = P5_TIMECODE_TIMESCALE;
	}

	m_hVoice = _hVoice;
}


void CVoCap::StopVoice()
{
	if(m_spAnimQueue && !(m_spAnimQueue->m_Flags & CVoCap_AnimItem::FLAGS_ITEMISHOLDREPLACER))
		m_spAnimQueue->m_Flags |= CVoCap_AnimItem::FLAGS_BLENDINGOUT;

	m_hVoice = 0;
}

void CVoCap::SetHold(CMapData *_pMapData, uint32 _NameHash, fp32 _RangeBegin, fp32 _RangeEnd)
{
	CVoCap_AnimItem *pItem = m_spAnimQueue;
	CVoCap_AnimItem *pLast = NULL;
	CXR_Anim_SequenceData* pSeq = GetSequenceFromName(_pMapData, _NameHash);
	while(pItem) // If other hold is already active, blend that one out
	{
		if(pItem->m_Flags & CVoCap_AnimItem::FLAGS_ITEMISHOLDREPLACER)
		{
			if(pItem->m_spCurSequence == pSeq && pItem->m_BlendInTime[SETHOLD_BI_BEGIN] == _RangeBegin && pItem->m_BlendInTime[SETHOLD_BI_END] == _RangeEnd)
			{
				pItem->m_Flags &= ~CVoCap_AnimItem::FLAGS_BLENDINGOUT;
				return;
			}

			pItem->m_Flags |= CVoCap_AnimItem::FLAGS_BLENDINGOUT;
		}
		pLast = pItem;
		pItem = pItem->m_spNextItem;
	}

	if(pSeq)
	{
		TPtr<CVoCap_AnimItem> spItem = MNew(CVoCap_AnimItem);
		spItem->m_spCurSequence = pSeq;
		spItem->m_Flags = CVoCap_AnimItem::FLAGS_ITEMISHOLDREPLACER;
		spItem->m_Time = 0;
		spItem->m_iMovingHold = 0;
		spItem->m_TimeScale = 1.0f;
		spItem->m_BlendInTime[SETHOLD_BI_BLENDINTIME] = VOCAP_SETHOLDBLENDTIME;
		spItem->m_BlendInTime[SETHOLD_BI_BEGIN] = _RangeBegin;
		spItem->m_BlendInTime[SETHOLD_BI_END] = _RangeEnd;
		spItem->m_BlendOutTime[SETHOLD_BO_BLENDOUTTIME] = VOCAP_SETHOLDBLENDTIME;
		spItem->m_BlendOutTime[SETHOLD_BO_LOOPBLENDTIME] = VOCAP_SETHOLDBLENDTIME;
		spItem->m_BlendOutTime[2] = 0.0f;
		spItem->m_Blend[VOCAP_BLEND_LOWERBODY] = 0;
		spItem->m_Blend[VOCAP_BLEND_UPPERBODY] = 0;
		spItem->m_Blend[VOCAP_BLEND_FACE] = 0;
		if(pLast)
			pLast->m_spNextItem = spItem;
		else
			m_spAnimQueue = spItem;
	}
}

fp32 CVoCap::Eval(CSoundContext* _pSound, int _hVoice, const CMTime& _ReferenceTime, int _iMovingHold, class CXR_AnimLayer* _pLayers, int& _nLayers, int _MaxLayers, CWorld_Client* _pWClientForDebugRender)
{
	if(!m_spAnimQueue)
		return 0.0f;

	fp32 Duration = (_ReferenceTime - m_LastReferenceTime).GetTime();
	if(m_hVoice && _pSound && _pSound->Voice_IsPlaying(_hVoice) && !(m_spAnimQueue->m_Flags & CVoCap_AnimItem::FLAGS_ITEMISHOLDREPLACER))
	{
		fp32 WaveTime = _pSound->Voice_GetPlayPos(_hVoice).GetTime();
		fp32 AnimTime = ((WaveTime - m_TimeCodeDiff) - Duration) * m_spAnimQueue->m_TimeScale;
		m_spAnimQueue->SetTime(AnimTime); // Duration needs to be subtracted because it added again before use later on

#ifndef M_RTM
		static bool s_bShowDebugRender = false;
		if (_pWClientForDebugRender && s_bShowDebugRender)
			s_VoCapDebug.Update(*_pWClientForDebugRender, WaveTime, AnimTime, m_TimeCodeDiff);
#endif
	}
	else
		m_hVoice = NULL;

	int iStartLayer;
	for(iStartLayer = 0; iStartLayer < _nLayers; iStartLayer++)
		if(_pLayers[iStartLayer].m_Flags & CXR_ANIMLAYER_FORCEAFTERVOCAP)
			break;

	fp32 MaxBlend = 0.0f;
	if(iStartLayer == _nLayers)
		m_spAnimQueue = m_spAnimQueue->Refresh(Duration, _iMovingHold, _pLayers, _nLayers, _MaxLayers, MaxBlend);
	else
	{
		// Refresh needs the original layers for potential Vocap Holds
		// Store away all layers after ForceAfterVocap flag
		int nMoveLayers = Max(64, _nLayers - iStartLayer);
		CXR_AnimLayer Layers[64];
		memcpy(&Layers, _pLayers + iStartLayer, sizeof(CXR_AnimLayer) * nMoveLayers);

		int iOldStartLayer = iStartLayer;
		m_spAnimQueue = m_spAnimQueue->Refresh(Duration, _iMovingHold, _pLayers, iStartLayer, _MaxLayers, MaxBlend);

		if(iOldStartLayer != iStartLayer)
			// Vocap overwrote layers. Append ForceAfterVocap layers
			memcpy(_pLayers + iStartLayer, Layers, sizeof(CXR_AnimLayer) * nMoveLayers);
		_nLayers = iStartLayer + nMoveLayers;
	}

	m_LastReferenceTime = _ReferenceTime;

	return MaxBlend;
}

bool CVoCap::IsActiveFullLayer(int32 _iMovingHold)
{
	if (!m_spAnimQueue)
		return false;

	return ((_iMovingHold == 1 && m_spAnimQueue->m_iMovingHold == 1) || _iMovingHold == -1);
}

bool CVoCap::GetEventLayers(CSoundContext *_pSound, int _hVoice, const CMTime &_ReferenceTime, int32 _iMovingHold, CEventLayer* _pLayers, int32& _nLayers)
{
	if(!m_spAnimQueue)
		return false;

	int32 MaxLayers = _nLayers;
	_nLayers = 0;
	m_spAnimQueue->GetEventLayers(0.0f, _iMovingHold, _pLayers, _nLayers,MaxLayers);
	return (_nLayers > 0);
}

void CVoCap::ClearReferenceTime()
{
	m_LastReferenceTime.Reset();
}
