/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_VoCap.h

	Author:			Jens Andersson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CVoCap, CVoCap_AnimItem

	Comments:

	History:		
		051017:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WOBJ_VOCAP_H
#define __WOBJ_VOCAP_H

enum
{
	VOCAP_BLEND_LOWERBODY = 0,
	VOCAP_BLEND_UPPERBODY,
	VOCAP_BLEND_FACE,
	VOCAP_NUM_BLENDS,

	SETHOLD_BI_BLENDINTIME = 0,
	SETHOLD_BI_BEGIN = 1,
	SETHOLD_BI_END = 2,
	SETHOLD_BO_BLENDOUTTIME = 0,
	SETHOLD_BO_LOOPBLENDTIME = 1,

	VOCAP_BLEND_LOWERBODY_MASK = (1 << VOCAP_BLEND_LOWERBODY),
	VOCAP_BLEND_UPPERBODY_MASK = (1 << VOCAP_BLEND_UPPERBODY),
	VOCAP_BLEND_FACE_MASK = (1 << VOCAP_BLEND_FACE),
};

#define VOCAP_SETHOLDBLENDTIME 0.5f


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVoCap_AnimItem
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CVoCap_AnimItem : public CReferenceCount
{
public:
	enum
	{
		FLAGS_BLENDINGOUT = 1,
		FLAGS_ITEMISHOLDREPLACER = 2,
	};

	fp32 m_Time;
	fp32 m_EventTime;
	fp32 m_TimeScale; // Needed for badly recorded animations
	fp32 m_Blend[VOCAP_NUM_BLENDS];
	fp32 m_BlendInTime[VOCAP_NUM_BLENDS];
	fp32 m_BlendOutTime[VOCAP_NUM_BLENDS];
	spCXR_Anim_SequenceData m_spCurSequence;
	int16 m_iMovingHold;
	int16 m_iEventKey;
	int8 m_Flags;

	TPtr<class CVoCap_AnimItem> m_spNextItem;

	CVoCap_AnimItem &operator =(const CVoCap_AnimItem& _AnimItem);
	void SetTime(fp32 _Time);
	CVoCap_AnimItem *Refresh(fp32 _Duration, int _iMovingHold, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers, fp32 &_MaxBlend);
	void GetEventLayers(fp32 _Duration, int _iMovingHold, class CEventLayer* _pLayers, int32& _nLayers, int _MaxLayers);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CVoCap
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CVoCap
{
	enum
	{
		VOCAP_MAXANIMRESOURCES = 16,
	};

	CMTime m_LastReferenceTime;
	fp32 m_TimeCodeDiff;
	int m_hVoice;

public:
	uint16 m_lAnimResources[VOCAP_MAXANIMRESOURCES];
	uint16 m_DialogueAnimFlags;
	int m_nAnimResources;
	TPtr<CVoCap_AnimItem> m_spAnimQueue;

public:
	CVoCap();
	void Init(uint16* _piAnimations, int _nAnimations);
	void Init(CStr _Animations, CMapData* _pMapData);
	void ClearQueue();

	void SetVoice(CSoundContext* _pSound, CMapData* _pMapData, int _iWave, uint32 _CustomAnim, int _iMovingHold, int _hVoice, const CMTime& _ReferenceTime, fp32 _StartOffset, uint16 _DialogueAnimFlags);
	void SetHold(CMapData *_pMapData, uint32 _Hash, fp32 _RangeBegin, fp32 _RangeEnd);
	void StopVoice();

	static fp32 GetTimeCodeDiff(CXR_Anim_SequenceData* _pSeq, CSoundContext* _pSound, int _iWave);

	uint32 GetWaveNameHash(CSoundContext* _pSound, int _iWave) const;
	CXR_Anim_SequenceData* GetSequenceFromName(CMapData* _pMapData, uint32 _NameHash, int* _piAnimRc = NULL, int* _piSeq = NULL) const;
	void ClearReferenceTime();

	CVoCap& operator =(const CVoCap& _VC);

	void SetBlendTimes(fp32* _pBlends);
	fp32 Eval(CSoundContext* _pSound, int _hVoice, const CMTime& _ReferenceTime, int _iMovingHold, class CXR_AnimLayer* _pLayers, int& _nLayers, int _MaxLayers, CWorld_Client* _pWClientForDebugRender = NULL);
	bool IsActiveFullLayer(int32 _iMovingHold);
	bool GetEventLayers(CSoundContext* _pSound, int _hVoice, const CMTime& _ReferenceTime, int32 _iMovingHold, CEventLayer* _pLayers, int32& _nLayers);
};

#endif
