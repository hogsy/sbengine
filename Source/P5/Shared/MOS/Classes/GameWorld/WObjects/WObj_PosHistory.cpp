#include "PCH.h"
#include "WObj_PosHistory.h"
#include "MFloat.h"

#define KOTT_RESOURCETAG 'PATH'

CWO_PosHistory::CWO_PosHistory()
{
	MAUTOSTRIP(CWO_PosHistory_ctor, MAUTOSTRIP_VOID);

	m_bTransform = false;
}

bool CWO_PosHistory::IsTransformed()
{
	MAUTOSTRIP(CWO_PosHistory_IsTransformed, false);

	return m_bTransform;
}

CMat4Dfp32& CWO_PosHistory::GetTransformation()
{
	return m_Transform;
}


void CWO_PosHistory::LoadPath(const void* __pData, const CMat4Dfp32 &_Transform)
{
	MAUTOSTRIP(CWO_PosHistory_LoadPath, MAUTOSTRIP_VOID);
	const uint32* _pData = (const uint32*)__pData;

	uint32 IDTag = _pData[0];
	if (IDTag == KOTT_RESOURCETAG)
		_pData++; // Skip the tag. It's just there so XWC can recognise paths in resource data

	// Version check
	int32 nVersion = _pData[0];
	SwapLE(nVersion);
	int Version = nVersion & POSHISTORY_IDMASK;
	if(Version == POSHISTORY_RESOURCEID || Version == POSHISTORY_PACKED_RESOURCEID)
	{
		m_Transform = _Transform;
		CMat4Dfp32 Unit;
		Unit.Unit();
		if(!m_Transform.AlmostEqual(Unit, 0.001f))
			m_bTransform = true;

		int32 nSeq = _pData[1];
		SwapLE(nSeq);
		
		int iPos = 2;
		for(int s = 0; s < nSeq; s++)
		{
			CSequence Seq;
			Seq.m_pData = &_pData[iPos];
			Seq.m_Version = nVersion;
			m_lSequences.Add(Seq);

			int32 nPath = _pData[iPos++];
			SwapLE(nPath);

			if(Version == POSHISTORY_RESOURCEID)
				iPos += sizeof(CFileKeyframe) * nPath / 4;
			else
			{
				if(nPath > 0)
					iPos += sizeof(CFileKeyframe) / 4;
				if(nPath > 1)
					iPos += sizeof(CFileKeyframe) / 4;
				if(nPath > 2)
					iPos += sizeof(CFileKeyframe_Packed) * (nPath - 2) / 4;
			}

			if(nVersion & POSHISTORY_FLAGS)
				iPos += (nPath + 3) / 4;
		}
	}
}

bool CWO_PosHistory::IsValid()
{
	MAUTOSTRIP(CWO_PosHistory_IsValid, false);

	return m_lSequences.Len() > 0;
}


bool CWO_PosHistory::GetMatrix(int _iSeq, fp32 _Time, bool _bLoop, int _iInterpolateType, CMat4Dfp32 &_Result)
{
	if(!m_lSequences[_iSeq].GetMatrix(_Time, _bLoop, _iInterpolateType, _Result))
		return false;

	if(m_bTransform)
	{
		CMat4Dfp32 Mat;
		_Result.Multiply(m_Transform, Mat);
		_Result = Mat;
	}

	return true;
}

void CWO_PosHistory::GenerateCache(fp32 _NumSamples)
{
	MAUTOSTRIP(CWO_PosHistory_GenerateCache, MAUTOSTRIP_VOID);

	if (m_lCache.Len() > 0)
		return;
	
	int nSeq = m_lSequences.Len();
	m_lCache.SetLen(nSeq);
	
	CMat4Dfp32 pos;
	
	for (int iSeq = 0; iSeq < nSeq; iSeq++)
	{
		CSequence *pSeq = &m_lSequences[iSeq];
		if (pSeq->GetNumKeyframes() > 0)
		{
			fp32 duration = pSeq->GetDuration();
			
			m_lCache[iSeq].SetLen(mint(_NumSamples));
			
			for (int iSample = 0; iSample < _NumSamples; iSample++)
			{
				fp32 time = duration * (fp32)iSample / (fp32)_NumSamples;
				if (!pSeq->GetMatrix(time, 0, 1, pos))
				{
					pos.Unit();
				}
				m_lCache[iSeq][iSample] = pos;
			}
		}
	}
}

int CWO_PosHistory::CSequence::GetNumKeyframes() const
{
	int32 nKeys = m_pData[0];
	SwapLE(nKeys);
	return nKeys;
}

int CWO_PosHistory::CSequence::GetTimeIndex(fp32 _Time) const
{
	fp32 Time = MaxMT(_Time, 0);
	
	int nKeyframes = GetNumKeyframes();
	
	int i;
	for(i = 0; i < nKeyframes; i++)
		if(Time + 0.001f < GetTime(i))
			return i - 1;
		
	if(Time - 0.001f < GetTime(i - 1))
		return i - 1;
		
	return i;
}

fp32 CWO_PosHistory::CSequence::GetKeyframeLength(int _iKeyframe) const
{
	if(_iKeyframe < 0)
		return 0.1f;
	int nKeyframes = GetNumKeyframes();
	int iK1 = Min(_iKeyframe, nKeyframes - 2);
	int iK2 = Min(_iKeyframe + 1, nKeyframes - 1);
	
	return GetTime(iK2) - GetTime(iK1);
}

bool CWO_PosHistory::CSequence::GetMatrix(fp32 _Time, bool _bLoop, int _iInterpolateType, CMat4Dfp32 &_Res) const
{
	MAUTOSTRIP(CWO_PosHistory_GetMatrix, false);

	int nPath = GetNumKeyframes();
	if(nPath > 0)
	{
		CMat4Dfp32 Mat;

		if(nPath == 1)
		{
			GetMatrix(0, _Res);
			return true;
		}

		if(_bLoop)
		{
			fp32 TotLen = GetDuration();
			int iTime = int(_Time / TotLen);
			_Time -= iTime * TotLen;
		}

		int iTimeIndex = GetTimeIndex(_Time);
		if(iTimeIndex >= nPath)
			GetMatrix(nPath - 1, Mat);
		else
		{		
//			int iTime = int(_Time + 0.0001f);
			
			switch(_iInterpolateType)
			{
			case 0:	// Linear
				{
					int T0 = Min(Max(iTimeIndex + 0, 0), nPath - 1);
					int T1 = Min(Max(iTimeIndex + 1, 0), nPath - 1);
					if(_Time < 0)
						T1 = 0;

					bool bInterpolate = true;
					{
//						fp32 Duration = GetTime(T1) - GetTime(T0);
						/*
						// *** What is this check for? ***
						if(Duration > 0)
						{
							CVec3Dfp32 VSpeed = GetPos(T1) - GetPos(T0);
							VSpeed *= 1.0f / Duration;
							fp32 Speed = VSpeed.LengthSqr();
	//						static MaxSpeed = 0;
	//						if(Speed > MaxSpeed)
	//							MaxSpeed = Speed;
							
							// My guess is that this should be 'if(Speed > 500000 * 500000)'
							if(Speed > 500000)
								bInterpolate = false;
						}
						*/
					}
					
					fp32 L1 = GetKeyframeLength(iTimeIndex);
					fp32 Fraq = (_Time - GetTime(iTimeIndex)) / L1;
					
					// Fix for keyframes that lies on top of each other
					// Error occurs on GC
					if(L1 < 0.0001f)
						L1 = 0.0001f;

					if(bInterpolate)
					{
						CQuatfp32 Q;
						GetRot(T0).Lerp(GetRot(T1), Fraq, Q);
						Q.CreateMatrix(Mat);
						(GetPos(T0) + (GetPos(T1) - GetPos(T0)) * Fraq).SetRow(Mat, 3);
					}
					else
						GetMatrix(T0, Mat);
				}
				break;

			case 1:	// Spline
			case 3:	// Backman Spline
				{
					int T0 = iTimeIndex - 1;
					int T1 = iTimeIndex;
					int T2 = iTimeIndex + 1;
					int T3 = iTimeIndex + 2;
					if(_bLoop)
					{
						if(T0 < 0)
							T0 += nPath - 1;
						if(T2 > nPath - 1)
							T2 -= nPath - 1;
						if(T3 > nPath - 1)
						{
							T3 -= nPath - 1;
							if(T3 > nPath - 1)
								T3 -= nPath - 1;
						}
					}
					else
					{
						if(T0 < 0)
							T0 = 0;
						if(T2 > nPath - 1)
							T2 = nPath - 1;
						if(T3 > nPath - 1)
							T3 = nPath - 1;
					}
					
					fp32 L0 = GetKeyframeLength(T0);
					fp32 L1 = GetKeyframeLength(T1);
					fp32 L2 = GetKeyframeLength(T2);
					
					// Fix for keyframes that lies on top of each other
					// Error occurs on GC
					if(L0 < 0.0001f)
						L0 = 0.0001f;
					if(L1 < 0.0001f)
						L1 = 0.0001f;
					if(L2 < 0.0001f)
						L2 = 0.0001f;

					fp32 Fraq = (_Time - GetTime(iTimeIndex)) / L1;
					{
						CQuatfp32 QRes;
						CQuatfp32 QTmpR0 = GetRot(T0);
						CQuatfp32 QTmpR1 = GetRot(T1);
						CQuatfp32 QTmpR2 = GetRot(T2);
						CQuatfp32 QTmpR3 = GetRot(T3);
						CQuatfp32::Spline(&QTmpR0, &QTmpR1, &QTmpR2, 
							&QTmpR1, &QTmpR2, &QTmpR3,
							&QRes, Fraq, L0, L1, L1, L2, 1);
						QRes.CreateMatrix(Mat);
					}
					{
						CVec3Dfp32 Res;
						CVec3Dfp32 VTmpP0 = GetPos(T0);
						CVec3Dfp32 VTmpP1 = GetPos(T1);
						CVec3Dfp32 VTmpP2 = GetPos(T2);
						CVec3Dfp32 VTmpP3 = GetPos(T3);
						
						// If the two keys T1, T2 are on the same position (within a unit)
						// they should return the same position to avoid cubic cusps
						if (VTmpP1.DistanceSqr(VTmpP2) > 1.0f)
						{
							if (_iInterpolateType == 1)
							{
								CVec3Dfp32::Spline(&VTmpP0, &VTmpP1, &VTmpP2,
											&VTmpP1, &VTmpP2, &VTmpP3,
											&Res, Fraq, L0, L1, L1, L2, 1);
							}
							else
							{
								CVec3Dfp32::SplineCardinal(0.5f, &VTmpP0, &VTmpP1, &VTmpP2, &VTmpP3,
											&Res, Fraq, L0, L1, L2, 1);
							}
						}
						else
						{
							// Just LERP between the keys
							Res = LERP(VTmpP1,VTmpP2,Fraq);
						}
						Res.SetRow(Mat, 3);
					}
				}
				break;

			case 2:
				{	// Step
					GetMatrix(iTimeIndex, Mat);
				}
				break;

			default:
				break;
			}
		}

		_Res = Mat;

		return true;
	}
	return false;
}

int CWO_PosHistory::CSequence::GetFlags(int _iKeyframe) const
{
	if(!(m_Version & POSHISTORY_FLAGS))
		return 0;

	int nKeyframes = GetNumKeyframes();

	int Size = 0;
	if(GetVersion() == POSHISTORY_RESOURCEID)
		Size = sizeof(int)+nKeyframes * (sizeof(CFileKeyframe));
	else
	{
		Size = sizeof(CFileKeyframe);
		if(nKeyframes > 1)
			Size += sizeof(CFileKeyframe);
		if(nKeyframes > 2)
			Size += (nKeyframes - 2) * sizeof(CFileKeyframe_Packed);
	}

	char *pFlags = ((char *)m_pData) + Size;
	
	return pFlags[_iKeyframe];
}

bool CWO_PosHistory::GetCacheMatrix(int _iSeq, fp32 _Time, CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWO_PosHistory_GetCacheMatrix, false);
	
	if(m_lCache.Len() <= _iSeq)
		return false;
	
	if (m_lCache[_iSeq].Len() == 0)
		return false;
	
	fp32 Duration = m_lSequences[_iSeq].GetDuration();
	if ((_Time < 0.0f) || (_Time > Duration))
		return false;
	
	int NumSamples = m_lCache[_iSeq].Len();
	fp32 SampleDuration = Duration / (fp32)NumSamples;
	
	fp32 sample = (_Time / SampleDuration);
	int iSample1 = Min(TruncToInt(sample), NumSamples - 1);
	int iSample2 = Min(iSample1 + 1, NumSamples - 1);
	fp32 fraction = sample - (fp32)iSample1;
	
//	m_lCache[_iSeq][iSample1].Lerp(m_lCache[_iSeq][iSample2], fraction, pos);
	
	CVec3Dfp32 VRes;
	CVec3Dfp32 V1, V2;
	V1 = CVec3Dfp32::GetMatrixRow(m_lCache[_iSeq][iSample1], 3);
	V2 = CVec3Dfp32::GetMatrixRow(m_lCache[_iSeq][iSample2], 3);
	V1.Lerp(V2, fraction, VRes);
	
	CQuatfp32 q1, q2;
	q1.Create(m_lCache[_iSeq][iSample1]);
	q2.Create(m_lCache[_iSeq][iSample2]);
	
	CQuatfp32 QRes;
	q1.Lerp(q2, fraction, QRes);
	
	QRes.CreateMatrix(_Pos);
	VRes.SetRow(_Pos, 3);
	
	return true;
}

void CWO_PosHistory::CSequence::GetMatrix(int _iKeyframe, CMat4Dfp32 &_Result) const
{
	if(GetVersion() == POSHISTORY_RESOURCEID)
	{
		CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) * _iKeyframe / 4);
		pKeyframe->GetRot().CreateMatrix(_Result);
		pKeyframe->GetPos().SetRow(_Result, 3);
	}
	else
	{
		if(_iKeyframe == 0)
		{
			CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1);
			pKeyframe->GetRot().CreateMatrix(_Result);
			pKeyframe->GetPos().SetRow(_Result, 3);
		}
		else
		{
			int nKeys = GetNumKeyframes();
			if(_iKeyframe == nKeys - 1)
			{
				CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) / 4);
				pKeyframe->GetRot().CreateMatrix(_Result);
				pKeyframe->GetPos().SetRow(_Result, 3);
			}
			else
			{
				CFileKeyframe_Packed *pKeyframe = (CFileKeyframe_Packed *)(m_pData + 1 + sizeof(CFileKeyframe) * 2 / 4 + (_iKeyframe - 1) * sizeof(CFileKeyframe_Packed) / 4);
				pKeyframe->GetRot().CreateMatrix(_Result);
				pKeyframe->GetPos().SetRow(_Result, 3);
			}	
		}
	}
}

int CWO_PosHistory::CSequence::GetVersion() const
{
	return m_Version&POSHISTORY_IDMASK;
}

fp32 CWO_PosHistory::CSequence::GetTime(int _iKeyframe) const
{
	if(GetVersion() == POSHISTORY_RESOURCEID)
	{
		CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) * _iKeyframe / 4);
		return pKeyframe->GetTime();
	}
	else
	{
		if(_iKeyframe == 0)
		{
			CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1);
			return pKeyframe->GetTime();
		}
		else
		{
			int nKeys = GetNumKeyframes();
			if(_iKeyframe == nKeys - 1)
			{
				CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) / 4);
				return pKeyframe->GetTime();
			}
			else
			{
				CFileKeyframe_Packed *pKeyframe = (CFileKeyframe_Packed *)(m_pData + 1 + sizeof(CFileKeyframe) * 2 / 4 + (_iKeyframe - 1) * sizeof(CFileKeyframe_Packed) / 4);
				return pKeyframe->GetTime();
			}	
		}
	}
}

fp32 CWO_PosHistory::CSequence::GetDuration() const
{
	int nKeys = GetNumKeyframes();
	if(GetVersion() == POSHISTORY_RESOURCEID)
	{
		CFileKeyframe *pKey = (CFileKeyframe *)(m_pData + 1 + (nKeys - 1) * sizeof(CFileKeyframe) / 4);
		return pKey->GetTime();
	}
	else
	{
//		int nKeys = GetNumKeyframes();
		CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) / 4);
		return pKeyframe->GetTime();
	}
}

CVec3Dfp32 CWO_PosHistory::CSequence::GetPos(int _iKeyframe) const
{
	if(GetVersion() == POSHISTORY_RESOURCEID)
	{
		CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) * _iKeyframe / 4);
		return pKeyframe->GetPos();
	}
	else
	{
		if(_iKeyframe == 0)
		{
			CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1);
			return pKeyframe->GetPos();
		}
		else
		{
			int nKeys = GetNumKeyframes();
			if(_iKeyframe == nKeys - 1)
			{
				CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) / 4);
				return pKeyframe->GetPos();
			}
			else
			{
				CFileKeyframe_Packed *pKeyframe = (CFileKeyframe_Packed *)(m_pData + 1 + sizeof(CFileKeyframe) * 2 / 4 + (_iKeyframe - 1) * sizeof(CFileKeyframe_Packed) / 4);
				return pKeyframe->GetPos();
			}	
		}
	}
}

CQuatfp32 CWO_PosHistory::CSequence::GetRot(int _iKeyframe) const
{
	if(GetVersion() == POSHISTORY_RESOURCEID)
	{
		CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) * _iKeyframe / 4);
		return pKeyframe->GetRot();
	}
	else
	{
		if(_iKeyframe == 0)
		{
			CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1);
			return pKeyframe->GetRot();
		}
		else
		{
			int nKeys = GetNumKeyframes();
			if(_iKeyframe == nKeys - 1)
			{
				CFileKeyframe *pKeyframe = (CFileKeyframe *)(m_pData + 1 + sizeof(CFileKeyframe) / 4);
				return pKeyframe->GetRot();
			}
			else
			{
				CFileKeyframe_Packed *pKeyframe = (CFileKeyframe_Packed *)(m_pData + 1 + sizeof(CFileKeyframe) * 2 / 4 + (_iKeyframe - 1) * sizeof(CFileKeyframe_Packed) / 4);
				return pKeyframe->GetRot();
			}	
		}
	}
}



#ifndef PLATFORM_CONSOLE
int CPosHistory_EditData::Load(const void* __pData)
{
	const uint32* _piData = (const uint32*)__pData;

	uint32 IDTag = _piData[0];
	if (IDTag == KOTT_RESOURCETAG)
		_piData++; // Skip the tag. It's just there so XWC can recognise paths in resource data

	int Ver = _piData[0];
	int VerID = Ver & POSHISTORY_IDMASK;
	if (VerID == POSHISTORY_RESOURCEID || VerID == POSHISTORY_PACKED_RESOURCEID)
	{
		int nSeq = _piData[1];
		int iPos = 2;

		m_lPath.SetLen(nSeq);
		m_lTimes.SetLen(nSeq);
		m_lFlags.SetLen(nSeq);

		for(int i = 0; i < nSeq; i++)
		{
			int iSequence = i;
			int nPath = _piData[iPos++];
			if (VerID == POSHISTORY_RESOURCEID)
			{
				const CWO_PosHistory::CFileKeyframe *pData = (const CWO_PosHistory::CFileKeyframe *)&_piData[iPos];
				for(int j = 0; j < nPath; j++)
				{
					CMat4Dfp32 Mat;
					pData[j].GetRot().CreateMatrix(Mat);
					Mat.RecreateMatrix(0, 2);
					pData[j].GetPos().SetMatrixRow(Mat, 3);
					SetMatrix(iSequence, Mat, pData[j].GetTime());
				}
				iPos += (sizeof(CWO_PosHistory::CFileKeyframe) * nPath) / 4;
			}
			else
			{
				if(nPath > 0)
				{
					const CWO_PosHistory::CFileKeyframe *pData = (const CWO_PosHistory::CFileKeyframe *)&_piData[iPos];
					CMat4Dfp32 Mat;
					pData[0].GetRot().CreateMatrix(Mat);
					Mat.RecreateMatrix(0, 2);
					pData[0].GetPos().SetMatrixRow(Mat, 3);
					SetMatrix(iSequence, Mat, pData[0].GetTime());
					iPos += sizeof(CWO_PosHistory::CFileKeyframe) / 4;
				}
				if(nPath > 1)
				{
					const CWO_PosHistory::CFileKeyframe *pData = (const CWO_PosHistory::CFileKeyframe *)&_piData[iPos];
					CMat4Dfp32 Mat;
					pData[0].GetRot().CreateMatrix(Mat);
					Mat.RecreateMatrix(0, 2);
					pData[0].GetPos().SetMatrixRow(Mat, 3);
					SetMatrix(iSequence, Mat, pData[0].GetTime());
					iPos += sizeof(CWO_PosHistory::CFileKeyframe) / 4;
				}

				const CWO_PosHistory::CFileKeyframe_Packed *pData = (const CWO_PosHistory::CFileKeyframe_Packed *)&_piData[iPos];
				for(int i = 1; i < nPath - 1; i++)
				{
					CMat4Dfp32 Mat;
					pData[i - 1].GetRot().CreateMatrix(Mat);
					Mat.RecreateMatrix(0, 2);
					pData[i - 1].GetPos().SetMatrixRow(Mat, 3);
					SetMatrix(iSequence, Mat, pData[i - 1].GetTime());
					iPos += sizeof(CWO_PosHistory::CFileKeyframe_Packed) / 4;
				}
			}

			if(Ver & POSHISTORY_FLAGS)
			{
				const char *pFlags = (const char *)&_piData[iPos];
				for(int j = 0; j < m_lFlags[i].Len(); j++)
					m_lFlags[i][j] = pFlags[j];
				iPos += (m_lFlags[i].Len() + 3) / 4;
			}
		}
	}
	//	else
	//		Error_static("CPosHistory_EditData::Load", CStrF("Unknown version of engine-path data (%i)", Ver));

	return VerID;
}


bool CPosHistory_EditData::Save(TArray<uint8>& _Data, bool _bLodded) const
{
	int nSeq = m_lPath.Len();
	int nPath0 = m_lPath[0].Len();
	if (nPath0 <= 1 && nSeq <= 1)
		return false;

	int Len = 0;
	if (_bLodded)
	{
		Len = 4 + 4 + 4;	// IDTag + Version + nSequences
		int s;
		for(s = 0; s < m_lPath.Len(); s++)
		{
			Len += 4;
			int nKF = m_lPath[s].Len();
			if(nKF > 0)
				Len += sizeof(CWO_PosHistory::CFileKeyframe);
			if(nKF > 1)
				Len += sizeof(CWO_PosHistory::CFileKeyframe);
			if(nKF > 2)
				Len += sizeof(CWO_PosHistory::CFileKeyframe_Packed) * (nKF - 2);
		}

		_Data.SetLen(Len);

		int *piData = (int *)_Data.GetBasePtr();
		int Ver = POSHISTORY_PACKED_RESOURCEID;
		for(s = 0; s < m_lFlags.Len(); s++)
			for(int k = 0; k < m_lFlags[s].Len(); k++)
				if(m_lFlags[s][k] != 0)
				{
					Ver |= POSHISTORY_FLAGS;
					break;
				}

		piData[0] = Ver;
		piData[1] = m_lPath.Len();
		int iPos = 2;
		for(s = 0; s < m_lPath.Len(); s++)
		{
			int nPath = m_lPath[s].Len();
			piData[iPos++] = nPath;

			if(nPath > 0)
			{
				CWO_PosHistory::CFileKeyframe *pData = (CWO_PosHistory::CFileKeyframe *)&piData[iPos];
				CQuatfp32 Q;
				Q.Create(m_lPath[s][0]);
				pData->Create(m_lTimes[s][0], CVec3Dfp32::GetRow(m_lPath[s][0], 3), Q);
				iPos += sizeof(CWO_PosHistory::CFileKeyframe) / 4;
			}
			if(nPath > 1)
			{
				CWO_PosHistory::CFileKeyframe *pData = (CWO_PosHistory::CFileKeyframe *)&piData[iPos];
				CQuatfp32 Q;
				Q.Create(m_lPath[s][nPath - 1]);
				pData->Create(m_lTimes[s][nPath - 1], CVec3Dfp32::GetRow(m_lPath[s][nPath - 1], 3), Q);
				iPos += sizeof(CWO_PosHistory::CFileKeyframe) / 4;
			}

			CWO_PosHistory::CFileKeyframe_Packed *pData = (CWO_PosHistory::CFileKeyframe_Packed *)&piData[iPos];
			for(int i = 1; i < nPath - 1; i++)
			{
				CQuatfp32 Q;
				Q.Create(m_lPath[s][i]);
				pData[i - 1].Create(m_lTimes[s][i], CVec3Dfp32::GetMatrixRow(m_lPath[s][i], 3), Q);
				iPos += sizeof(CWO_PosHistory::CFileKeyframe_Packed) / 4;
			}

			if(Ver & POSHISTORY_FLAGS)
			{
				char *pFlags = (char *)&piData[iPos];
				for(int j = 0; j < m_lFlags[s].Len(); j++)
					pFlags[j] = m_lFlags[s][j];
				iPos += (m_lFlags[s].Len() + 3) / 4;
			}
		}
	}
	else
	{
		int nKF = 0;
		for(int s = 0; s < m_lPath.Len(); s++)
			nKF += m_lPath[s].Len();

		int Ver = POSHISTORY_RESOURCEID;
		for(int s = 0; s < m_lFlags.Len(); s++)
			for(int k = 0; k < m_lFlags[s].Len(); k++)
				if(m_lFlags[s][k] != 0)
				{
					Ver |= POSHISTORY_FLAGS;
					break;
				}

		Len = 4 + 4 + 4 + nSeq * 4 + nKF * sizeof(CWO_PosHistory::CFileKeyframe);
		if(Ver & POSHISTORY_FLAGS)
			for(int s = 0; s < m_lPath.Len(); s++)
			{
				int nInts = (m_lFlags[s].Len() + 3) / 4;
				Len += nInts * 4;
			}

		_Data.SetLen(Len);
		int *piData = (int *)_Data.GetBasePtr();
		
		piData[0] = Ver;
		piData[1] = m_lPath.Len();
		int32 iPos = 2;
		for(int s = 0; s < m_lPath.Len(); s++)
		{
			int nPath = m_lPath[s].Len();
			piData[iPos++] = nPath;
			CWO_PosHistory::CFileKeyframe *pData = (CWO_PosHistory::CFileKeyframe *)&piData[iPos];

			for(int i = 0; i < nPath; i++)
			{
				CQuatfp32 Q;
				Q.Create(m_lPath[s][i]);
				pData[i].Create(m_lTimes[s][i], CVec3Dfp32::GetMatrixRow(m_lPath[s][i], 3), Q);
			}
			iPos += (sizeof(CWO_PosHistory::CFileKeyframe) * nPath) / 4;

			if(Ver & POSHISTORY_FLAGS)
			{
				char *pFlags = (char *)&piData[iPos];
				for(int j = 0; j < m_lFlags[s].Len(); j++)
					pFlags[j] = m_lFlags[s][j];
				iPos += (m_lFlags[s].Len() + 3) / 4;
			}
		}
	}

	return true;
}


int CPosHistory_EditData::SetMatrix(int _iSequence, const CMat4Dfp32& _Mat, fp32 _Time, uint8 _Flags)
{
	CMat4Dfp32 Mat(_Mat);
	CVec3Dfp32::GetMatrixRow(Mat, 3).Snap(1.0f, 0.001f);

	int i;
	for (i = 0; i < m_lTimes[_iSequence].Len(); i++)
		if (_Time <= m_lTimes[_iSequence][i])
			break;

	if (i == m_lTimes[_iSequence].Len())
	{
		m_lPath[_iSequence].Add(Mat);
		m_lTimes[_iSequence].Add(_Time);
		m_lFlags[_iSequence].Add(_Flags);
	}
	else if (m_lTimes[_iSequence][i] != _Time)
	{
		m_lPath[_iSequence].Insert(i, Mat);
		m_lTimes[_iSequence].Insert(i, _Time);
		m_lFlags[_iSequence].Insert(i,_Flags);
	}
	else
	{
		m_lPath[_iSequence][i] = Mat;
	}

	return i;
}


void CPosHistory_EditData::LodDynamics(fp32 _Time)
{
	for (int s = 0; s < m_lTimes.Len(); s++)
	{
		fp32 LastDuration = _FP32_MAX;
		for (int i = 0; i < m_lTimes[s].Len() - 1; i++)
		{
			fp32 Duration = m_lTimes[s][i + 1] - m_lTimes[s][i];
			if(Duration < _Time + 0.0001f && LastDuration < _Time + 0.0001f)
			{
				m_lTimes[s].Del(i);
				m_lPath[s].Del(i);
				i--;
				Duration += LastDuration;
			}
			LastDuration = Duration;
		}
	}
}

#endif // PLATFORM_CONSOLE
