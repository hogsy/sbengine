//
// annosoft lipsync analyser
//
#include "PCH.h"
#include "LipSync.h"

#ifdef LIPSYNC_ANNOSOFT
// includes
#include <liblipsync.h>

using namespace NLipSync;

//
// 
//
class CAnalysis_AnnoSoft : public CAnalysis
{
public:
	ISyncResultsCollection *m_pSyncResults; // sync marker results
	IObservationStream *m_pObservation; // audio data stream

	int32 m_NumFrames;
	int32 m_FrameRate;
	SUncompressedFrame *m_pFrames;

	//
	int32 GetVisualTarget(uint16 _Phoneme)
	{
		if(_Phoneme == 'm\0' || _Phoneme == 'b\0' || _Phoneme == 'p\0' || _Phoneme == 'x\0') return 0;
		if(_Phoneme == 'NG' || _Phoneme == 'g\0' || _Phoneme == 'k\0' || _Phoneme == 'h\0') return 1;
		if(_Phoneme == 'ZH' || _Phoneme == 'CH' || _Phoneme == 'SH' || _Phoneme == 'j\0') return 2;
		if(_Phoneme == 'AE' || _Phoneme == 'AY' || _Phoneme == 'AA' || _Phoneme == 'IH') return 3;
		if(_Phoneme == 'IY' || _Phoneme == 'y\0') return 4;
		if(_Phoneme == 'f\0'  || _Phoneme == 'v\0') return 5;
		if(_Phoneme == 'IY' || _Phoneme == 'EY' || _Phoneme == 'y\0') return 6;
		if(_Phoneme == 'NG' || _Phoneme == 'g\0' || _Phoneme == 'k\0' || _Phoneme == 'h\0') return 7;
		if(_Phoneme == 'OW' || _Phoneme == 'OY' || _Phoneme == 'AA') return 8;
		if(_Phoneme == 'AH' || _Phoneme == 'AO' || _Phoneme == 'EH' || _Phoneme == 'AY') return 9;
		if(_Phoneme == 'ER' || _Phoneme == 'r\0') return 10;
		if(_Phoneme == 's\0' || _Phoneme == 'z\0') return 11;
		if(_Phoneme == 'l\0') return 12;
		if(_Phoneme == 'DH' || _Phoneme == 'TH' || _Phoneme == 't\0' || _Phoneme == 'n\0'  || _Phoneme == 'd\0') return 13;
		if(_Phoneme == 'w\0' || _Phoneme == 'UW' || _Phoneme == 'UH' || _Phoneme == 'AW') return 14;

		return -1;
	}

	//
	//
	void Init()
	{
		CSyncMarker *pBegin = m_pSyncResults->begin();
		CSyncMarker *pEnd = m_pSyncResults->end();

		//
		int32 FrameRate = m_FrameRate;
		int32 EndTime = 0;

		//
		for(CSyncMarker *pCurrent = pBegin; pCurrent != pEnd; pCurrent++)
		{
			if(pCurrent->type == CSyncMarker::phoneme && pCurrent->milliEnd > EndTime)
				EndTime = pCurrent->milliEnd;
		}

		// create frames and nullify them
		m_NumFrames = (int32)((EndTime/1000.0f)*FrameRate);
		m_pFrames = DNew(SUncompressedFrame) SUncompressedFrame[m_NumFrames];
		
		for(int32 i = 0; i < m_NumFrames; i++)
			m_pFrames[i].m_Mask = 0;

		//
		for(CSyncMarker *pCurrent = pBegin; pCurrent != pEnd; pCurrent++)
		{
			if(pCurrent->type == CSyncMarker::phoneme)
			{
				uint16 Phonome = (((unsigned char*)pCurrent->szPhoneme)[0]<<8) | (((unsigned char*)pCurrent->szPhoneme)[1]);
				int32 Target = GetVisualTarget(Phonome);

				if(Target != -1)
				{
					uint16 Mask = (1<<Target);
					uint8 Value = (uint8)(pCurrent->intensity*255.0f);
					int32 StartFrame = (pCurrent->milliStart/1000.0f)*FrameRate;
					int32 EndFrame = (pCurrent->milliEnd/1000.0f)*FrameRate;
					
					for(int32 Frame = StartFrame; Frame < EndFrame; Frame++)
					{
						m_pFrames[Frame].m_Mask |= Mask;
						m_pFrames[Frame].m_Values[Target] = Value;
					}

					M_TRACEALWAYS("Phoneme %s(%d) %d->%d\n", pCurrent->szPhoneme, GetVisualTarget(Phonome), pCurrent->milliStart, pCurrent->milliEnd);
				}
			}

			if(pCurrent->type == CSyncMarker::energy)
			{
				M_TRACEALWAYS("Energy %f %d->%d\n", pCurrent->intensity, pCurrent->milliStart, pCurrent->milliEnd);
			}
		}

		M_TRACEALWAYS("\n\n");

	}

	//
	//
	virtual ~CAnalysis_AnnoSoft()
	{
		delete [] m_pFrames;
		DestroySyncResultsCollection(m_pSyncResults);
		DestroyObservationStream(m_pObservation);
	}

	//
	//
	virtual int32 GetFrameRate()
	{
		return m_FrameRate;
	}

	//
	//
	virtual int32 GetFirstFrame()
	{
		return 0;
	}
		
	//
	//
	virtual int32 GetLastFrame()
	{
		return m_NumFrames;
	}

	//
	//
	virtual fp32 GetSpeechTargetValueAtFrame(int32 _Track, int32 _Frame)
	{
		if(m_pFrames[_Frame].m_Mask&(1<<_Track))
			return m_pFrames[_Frame].m_Values[_Track]/256.0f;
		return 0;
	}
};

//
// 
//
class CAnalyser_AnnoSoft : public CAnalyser
{
public:
	CLipSyncAccousticHMM *m_pHmm; // hidden markov model
	ITextlessPhnRecognizer *m_pRecognizer; // recognizer object
	char *m_pModelData;

	CAnalyser_AnnoSoft()
	{
		// read file
		CCFile File;
		File.Open("S:\\PB\\AnnoLipSync\\ResModel40.hmm", CFILE_READ|CFILE_BINARY);

		int32 Len = File.Length();
		m_pModelData = DNew(char) char[Len];
		File.Read(m_pModelData, Len);

		//
		serror err = kNoError;
		char szError[320];
		CreateAccHMM(m_pModelData, Len, szError, &m_pHmm);

		err = CreateTextlessPhnRecognizer(&m_pRecognizer);


		// clean up
		File.Close();
	}

	//
	//
	virtual ~CAnalyser_AnnoSoft()
	{
		delete [] m_pModelData;
		DestroyAccHMM(m_pHmm);
		DestroyTextlessPhnRecognizer(m_pRecognizer);
	}

	//
	//
	virtual CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate)
	{
		serror err = kNoError;

		// observe file
		IObservationStream *pObservation = NULL; // audio data stream
		err = CreateObservationStreamFromAudioFile(_pFilename, m_pHmm, &pObservation);

		// get results
		ISyncResultsCollection *pSyncResults = NULL;
		err = CreateSyncResultsCollection(0, &pSyncResults);
		err = m_pRecognizer->RecognizePhonemes(pObservation, m_pHmm, NULL, pSyncResults);

		// crate new analysis
		CAnalysis_AnnoSoft *pAnalysis = DNew(CAnalysis_AnnoSoft) CAnalysis_AnnoSoft();
		pAnalysis->m_pSyncResults = pSyncResults;
		pAnalysis->m_pObservation = pObservation;
		pAnalysis->m_FrameRate = _FrameRate;
		pAnalysis->Init();

		return pAnalysis;
	}
};

CAnalyser_AnnoSoft *g_pAnalyser = NULL;

//
// 
//
class CLipSyncRemover
{
public:
	~CLipSyncRemover()
	{
		if(g_pAnalyser)
		{
			delete g_pAnalyser;
			g_pAnalyser = NULL;
		}
	}
};

CLipSyncRemover g_LipSyncRemover;

//
//
//
CAnalyser *NLipSync::GetAnalyser()
{
	if(!g_pAnalyser)
		g_pAnalyser = DNew(CAnalyser_AnnoSoft) CAnalyser_AnnoSoft();
	return g_pAnalyser;
}

#endif // LIPSYNC_NOANALYSER
