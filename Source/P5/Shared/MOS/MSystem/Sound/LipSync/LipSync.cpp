//
// talkback lipsync analyser
//
#include "PCH.h"

#include "LipSync.h"
#include "../../../XR/XRSkeleton.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2.h"

void CLipSync::Clear()
{
	m_WaveID = -1;
	m_CurrentWaveID = -1;
	m_hVoice = -1;
	for( int i = 0; i < 16; i++ )
		m_LastValue[i]	= 0;
}

void CLipSync::CopyFrom(const CLipSync &_LipSync)
{
	m_WaveID = _LipSync.m_WaveID;
	m_CurrentWaveID = _LipSync.m_CurrentWaveID;
	m_hVoice = _LipSync.m_hVoice;
	m_Data = _LipSync.m_Data;
}

void CLipSync::SetVoice(int _WaveID, int _hVoice)
{
	m_WaveID = _WaveID;
	m_hVoice = _hVoice;
}

static int aConv[] = { 4, 3, 6, 9, 8, 15, 12, 2, 5, 13, 14, 0,  7, 10, 1, 11 };

void CLipSync::Eval(CSoundContext* _pSound, int _iAnimResource, CMapData* _pMapData, class CXR_AnimLayer* _pLayers, int& _nLayers, const int* _pRottracks, int _nRottracks, fp32 _Scale)
{
	if (!_pSound)
		return;

	//
	// dig for lip data
	// 
	if(m_CurrentWaveID != m_WaveID)
	{
		// get raw lipdata and unpack
		void *pRawData = _pSound->LSD_Get(m_WaveID);
		if(pRawData)
		{
			m_Data.ReadData(pRawData);
			m_CurrentWaveID = m_WaveID;
		}
		else
		{
			// invalidate
			m_CurrentWaveID = -1;
			m_WaveID = -1;
		}
	}

	// return on invalid waveid or if m_hVoice isn't playing anymore (skipping)
	if(m_WaveID == -1 || !_pSound->Voice_IsPlaying(m_hVoice))
	{
		fp32 Time = 0.0f;
		CMTime ZeroTime = CMTime::CreateFromSeconds(Time);
		for( int t = 0; t < m_Data.m_SpeechTargets; t++ )
		{
			if( m_LastValue[t] > 0 )
			{
				int32 iSeq = aConv[t];
				CXRAG2_Animation Anim(_iAnimResource, iSeq);

				CXR_Anim_SequenceData *pSeq = Anim.GetAnimSequenceData_MapData(_pMapData);
				if(pSeq != NULL)
				{
					fp32 Blend = m_LastValue[t];
					//fp32 Time = 0, Blend = Value*0.8f; // + (NextValue - Value) * BlendTime;
					//fp32 Time = 0, Blend = powf(Value, 0.5f);
					//fp32 Time = 0, Blend = powf(Value, 0.5f)*Amp;

					// FIX ME!
					for(int i = 0; i < _nRottracks; i++)
						_pLayers[_nLayers++].Create3(pSeq, ZeroTime, 1.0f, Blend * _Scale, _pRottracks[i], CXR_ANIMLAYER_ADDITIVEBLEND);
				}

				m_LastValue[t]	= Max( 0.0f, m_LastValue[t] - 0.01f );
			}
		}
		return;
	}

	// calculate times
	int32 FrameRate = m_Data.m_FrameRate;
	fp32 CurrentTime = _pSound->Voice_GetPlayPos(m_hVoice).GetTime();
	int LipFrame = (int)(CurrentTime*(m_Data.m_FrameRate)) - m_Data.m_FirstFrame;
	int LipNextFrame = Min((int)m_Data.m_NumFrames-1, LipFrame+1);
	fp32 BlendTime = (CurrentTime-((int)(CurrentTime*FrameRate)/(fp32)FrameRate))*FrameRate;

	// aAdd layer
	//M_TRACEALWAYS("%d(%.2f) ", t, Value/256.0f);

	const bool Log = false;

	// check if we still are in lip animation
	if(LipFrame >= 0 && LipFrame < m_Data.m_NumFrames)
	{
		M_ASSERT(LipNextFrame >= 0, "LipNextFrame to small");
		M_ASSERT(LipFrame >= 0, "LipFrame to small");
		M_ASSERT(LipNextFrame < m_Data.m_NumFrames, "LipNextFrame to large");
		M_ASSERT(LipFrame < m_Data.m_NumFrames, "LipFrame to large");
		// get track data
		fp32 Time = 0.0f;
		CMTime ZeroTime = CMTime::CreateFromSeconds(Time);
		int32 Bit = 1;
		for(int32 t = 0; t < m_Data.m_SpeechTargets; t++)
		{
			if(m_Data.m_pFrames[LipFrame].m_Mask&Bit || m_Data.m_pFrames[LipNextFrame].m_Mask&Bit)
			{
				// Add layer
				fp32 Value = 0;
				fp32 NextValue = 0;

				if(m_Data.m_pFrames[LipFrame].m_Mask&Bit)
					Value = m_Data.GetTrackValue(LipFrame, t) * (1.0f/255.0f);

				if(m_Data.m_pFrames[LipNextFrame].m_Mask&Bit)
					NextValue = m_Data.GetTrackValue(LipNextFrame, t) * (1.0f/255.0f);

				if(Log)
					M_TRACEALWAYS("%d(%.2f-%.2f) ", t, Value, NextValue);

				int32 iSeq = aConv[t];
				CXRAG2_Animation Anim(_iAnimResource, iSeq);

				CXR_Anim_SequenceData *pSeq = Anim.GetAnimSequenceData_MapData(_pMapData);
				if(pSeq != NULL)
				{
					fp32 Blend = Value * (1.0f - BlendTime) + NextValue * BlendTime;
					//fp32 Time = 0, Blend = Value*0.8f; // + (NextValue - Value) * BlendTime;
					//fp32 Time = 0, Blend = powf(Value, 0.5f);
					//fp32 Time = 0, Blend = powf(Value, 0.5f)*Amp;

					// FIX ME!
					for(int i = 0; i < _nRottracks; i++)
						_pLayers[_nLayers++].Create3(pSeq, ZeroTime, 1.0f, Blend, _pRottracks[i], CXR_ANIMLAYER_ADDITIVEBLEND);

					m_LastValue[t]	= Blend;
				}
			}

			Bit<<=1;
		}
	}

	if(Log)
		M_TRACEALWAYS("\n");
}

fp32 CLipSync::GetBlendValues(CSoundContext* _pSound, int &_num)
{
	if(m_CurrentWaveID != m_WaveID)
	{
		// get raw lipdata and unpack
		void *pRawData = _pSound->LSD_Get(m_WaveID);
		if(pRawData)
		{
			m_Data.ReadData(pRawData);
			m_CurrentWaveID = m_WaveID;
		}
		else
		{
			// invalidate
			m_CurrentWaveID = -1;
			m_WaveID = -1;
		}
	}

	// return on invalid waveid or if m_hVoice isn't playing anymore (skipping)
	if(m_WaveID == -1 || !_pSound->Voice_IsPlaying(m_hVoice))
	{
		return 0.0f;
	}

	// calculate times
	int32 FrameRate = m_Data.m_FrameRate;
	fp32 CurrentTime = _pSound->Voice_GetPlayPos(m_hVoice).GetTime();
	int LipFrame = (int)(CurrentTime*(m_Data.m_FrameRate)) - m_Data.m_FirstFrame;
	int LipNextFrame = Min((int)m_Data.m_NumFrames-1, LipFrame+1);
	fp32 BlendTime = (CurrentTime-((int)(CurrentTime*FrameRate)/(fp32)FrameRate))*FrameRate;

	// check if we still are in lip animation
	fp32 BlendValues = 0.0f;
	if(LipFrame >= 0 && LipFrame < m_Data.m_NumFrames)
	{
		M_ASSERT(LipNextFrame >= 0, "LipNextFrame to small");
		M_ASSERT(LipFrame >= 0, "LipFrame to small");
		M_ASSERT(LipNextFrame < m_Data.m_NumFrames, "LipNextFrame to large");
		M_ASSERT(LipFrame < m_Data.m_NumFrames, "LipFrame to large");
		// get track data
		fp32 Time = 0.0f;
		CMTime ZeroTime = CMTime::CreateFromSeconds(Time);
		int32 Bit = 1;
		for(int32 t = 0; t < m_Data.m_SpeechTargets; t++)
		{
			if(m_Data.m_pFrames[LipFrame].m_Mask&Bit || m_Data.m_pFrames[LipNextFrame].m_Mask&Bit)
			{
				// Add layer
				fp32 Value = 0;
				fp32 NextValue = 0;

				if(m_Data.m_pFrames[LipFrame].m_Mask&Bit)
					Value = m_Data.GetTrackValue(LipFrame, t) * (1.0f/255.0f);

				if(m_Data.m_pFrames[LipNextFrame].m_Mask&Bit)
					NextValue = m_Data.GetTrackValue(LipNextFrame, t) * (1.0f/255.0f);

				fp32 Blend = Value * (1.0f - BlendTime) + NextValue * BlendTime;
				BlendValues += Blend;
				_num++;
			}

			Bit<<=1;
		}
	}

	return BlendValues;
}

using namespace NLipSync;

//
//
//
#ifdef LIPSYNC_NOANALYSER
CAnalyser *NLipSync::GetAnalyser() { return NULL; }
#endif // LIPSYNC_NOANALYSER

//
//
//
CData::CData()
{
	Clear();
}

//
//
//
CData::~CData()
{
	Clear();
}

//
//
//
int32 CData::GetNumTracks(int32 _FrameNumber)
{
	M_ASSERT(_FrameNumber < m_NumFrames, "CData::GetNumTracks #1");
	M_ASSERT(_FrameNumber >= 0, "CData::GetNumTracks #1");
	M_ASSERT(m_pFrames, "CData::GetTrackValue: m_pFrames");
	return m_pFrames[_FrameNumber].GetNumTracks();
}

uint8 CData::GetTrackValue(int32 _FrameNumber, int32 _TrackIndex)
{
	M_ASSERT(m_pFrames, "CData::GetTrackValue: m_pFrames");
	M_ASSERT(m_pNumbers, "CData::GetTrackValue: m_pNumbers");
	M_ASSERT(_FrameNumber < m_NumFrames && _FrameNumber >= 0, "CData::GetTrackValue: FrameNumber");

	uint16 IndexShift = (uint16)m_pFrames[_FrameNumber].GetIndexShift(_TrackIndex);
	uint16 NumberIndex = m_pFrames[_FrameNumber].m_StartIndex;

	M_ASSERT(NumberIndex+IndexShift < m_NumNumbers && NumberIndex+IndexShift >= 0, "CData::GetTrackValue Index");
	return m_pNumbers[NumberIndex+IndexShift];
}


//
//
//
void CData::Clear()
{
	m_spMemoryBlock = NULL;

	m_pNumbers = NULL;
	m_pFrames = NULL;
	m_NumFrames = 0;
	m_NumNumbers = 0;

	m_FrameRate = 0;
	m_FirstFrame = 0;
	m_SpeechTargets = 0;
}

template<typename T>
M_INLINE void CStream_ReadLE(CStream_Memory& _Stream, T& _Variable)
{
	_Stream.Read(&_Variable, sizeof(T));
	::SwapLE(_Variable);
}

//
//
//
void CData::ReadData(void *_pData)
{
	Clear();

	CStream_Memory Memory((uint8*)_pData, 1000000, 1000000);

	// Read header
	CStream_ReadLE(Memory, m_FirstFrame);
	CStream_ReadLE(Memory, m_NumFrames);
	CStream_ReadLE(Memory, m_FrameRate);
	CStream_ReadLE(Memory, m_SpeechTargets);
	CStream_ReadLE(Memory, m_NumNumbers);

	// Create Frames
	m_spMemoryBlock = MNew1(CMemoryBlock, sizeof(SFrame)*m_NumFrames+m_NumNumbers);
	//M_ASSERT(m_pMemoryBlock, "CData::ReadData: Memory Error");
	m_pFrames = (SFrame*)(m_spMemoryBlock->m_pMem);
	m_pNumbers = m_spMemoryBlock->m_pMem+sizeof(SFrame)*m_NumFrames;

	for(int32 i = 0; i < m_NumFrames; i++)
		m_pFrames[i].m_Mask = 0;

	// Read frames
	int32 CurrentFrame = 0;
	while(CurrentFrame < m_NumFrames)
	{
		uint8 Data1;
		Memory.Read(&Data1, sizeof(Data1));

		if(Data1&(1<<7))
		{
			// RLE
			Data1 &= ~(1<<7);

			while(Data1--)
				m_pFrames[CurrentFrame++].m_Mask = 0;
		}
		else // Mask
		{
			uint8 Data2;
			Memory.Read(&Data2, sizeof(Data2));
			m_pFrames[CurrentFrame++].m_Mask = Data2|(Data1<<8);
		}
	}
	M_ASSERT(CurrentFrame == m_NumFrames, "CData::ReadData: frame error");

	// read numbers
	Memory.Read(m_pNumbers, m_NumNumbers);

	// fix indexing
	uint16 CurrentIndex = 0;
	for(int32 f = 0; f < m_NumFrames; f++)
	{
		if(m_pFrames[f].m_Mask)
		{
			m_pFrames[f].m_StartIndex = CurrentIndex;
			CurrentIndex += (uint16)m_pFrames[f].GetNumTracks();
		}
	}
}

//
//
//
void NLipSync::CreateCompressedData(CAnalysis *_pAnalysis, void *&_rpData, int32 &_rSize)
{
	int8 FirstFrame = _pAnalysis->GetFirstFrame();
	int32 LastFrame = _pAnalysis->GetLastFrame();
	uint16 NumFrames = LastFrame-FirstFrame + 1;

	SUncompressedFrame *pUncompressedFrames = DNew(SUncompressedFrame) SUncompressedFrame[NumFrames];

	// Construct frames
	int32 NumValues = 0;
	for(int32 f = 0; f < NumFrames; f++)
	{
		pUncompressedFrames[f].m_Mask = 0;

		uint16 Bit = 1;
		for(int32 s = 0; s < NUMSPEECHTARGETS; s++)
		{
			fp32 Value = _pAnalysis->GetSpeechTargetValueAtFrame(s, f+FirstFrame);

			if (Value < 0.0f || Value > 1.0f)
				M_TRACEALWAYS("*** LipSync SpeechTarget value is out of range!\n");

			pUncompressedFrames[f].m_Values[s] = (uint8)(Value*255.0f);
			if(pUncompressedFrames[f].m_Values[s])
			{
				pUncompressedFrames[f].m_Mask |= Bit;
				NumValues++;
			}

			Bit <<= 1;
		}
	}

	// Create numbers
	uint8 pNumbers[1024*10];
	int32 Current = 0;

	for(int32 f = 0; f < NumFrames; f++)
	{
		for(int32 s = 0; s < NUMSPEECHTARGETS; s++)
		{
			if(pUncompressedFrames[f].m_Values[s])
				pNumbers[Current++] = pUncompressedFrames[f].m_Values[s];
		}
	}

	// Write masks (RLEd, RLEd)
	int32 FramesWritten = 0;
	uint8 aBuffer[1024*10];
	int Pos = 0;
	for(int f = 0; f < NumFrames; f++)
	{
		if(!pUncompressedFrames[f].m_Mask && f < NumFrames-1 && !pUncompressedFrames[f+1].m_Mask)
		{
			int r;
			for(r = f; r < NumFrames; r++)
				if(pUncompressedFrames[r].m_Mask)
					break;

			int Count = r-f;

			Count = Min( Count, 127 );
			r	= f + Count;	// Need to recalculate r if Count was clamped

			aBuffer[Pos++] = ((1<<7)|(uint8)Count);
			f = r-1;
			FramesWritten += Count;
		}
		else
		{
			aBuffer[Pos++] = (pUncompressedFrames[f].m_Mask>>8) & 0x007f;
			aBuffer[Pos++] = pUncompressedFrames[f].m_Mask&0x00FF;
		}
	}

	_rSize = Pos+NumValues+9; // 9 = header

	// create data
	_rpData = DNew(uint8) uint8[_rSize];
	uint8 *pData = (uint8 *)_rpData;

	// header
	CStream_Memory Memory(pData, 0, _rSize);
	Memory.Write(&FirstFrame, sizeof(FirstFrame));
	Memory.Write(&NumFrames, sizeof(NumFrames));

	uint8 FrameRate = _pAnalysis->GetFrameRate();
	Memory.Write(&FrameRate, sizeof(FrameRate));
	uint8 SpeechTargets = 15;
	Memory.Write(&SpeechTargets, sizeof(SpeechTargets));

	Memory.Write(&NumValues, sizeof(NumValues));

	// masks
	Memory.Write(aBuffer, Pos);

	// numbers
	Memory.Write(pNumbers, NumValues);
}
