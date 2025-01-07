
#include "PCH.h"
#include "CModelHistory2.h"

MRTC_IMPLEMENT(CModelHistory2, CModelData);

void CModelHistory2::Clear()
{
	m_iEntry = -1;
	m_iOldestEntry = 0;
	m_nEntries = 0;

	m_NoChange_iLastEntry = -1;
	m_NoChange_EntryCount = 0;

	ClearBoundBox();
}


void CModelHistory2::operator=(const CXR_ModelInstance &_Instance)
{
	CModelData::operator=(_Instance);

	const CModelHistory2* pHistory = safe_cast<const CModelHistory2>(&_Instance);
	if (pHistory)
	{
		m_EntryDuration = pHistory->m_EntryDuration;
		m_MaxEntryDelay = pHistory->m_MaxEntryDelay;

		memcpy(m_Entry, pHistory->m_Entry, sizeof(m_Entry));
		m_iEntry = pHistory->m_iEntry;
		m_iOldestEntry = pHistory->m_iOldestEntry;
		m_nEntries = pHistory->m_nEntries;

		m_BoundBox = pHistory->m_BoundBox;
		m_BoundBox_bIsEmpty = pHistory->m_BoundBox_bIsEmpty;
		m_BoundBox_bStraightMovement = pHistory->m_BoundBox_bStraightMovement;
	}
}


int CModelHistory2::GetIndex1(fp32 _Time) const
{
#ifndef M_RTM
	if (false)
	{
		if (m_nEntries > 0)
			ConOutL(CStrF("GetIPM: Time = %f, NewestTime = %f, OldestTime = %f", _Time, m_Entry[m_iEntry].m_Time, m_Entry[m_iOldestEntry].m_Time));
		else
			ConOutL(CStrF("GetIPM: Time = %f", _Time));
	}

	if (m_nEntries == 0)
		return -1;

	if ((_Time > m_Entry[m_iEntry].m_Time) || (_Time < m_Entry[m_iOldestEntry].m_Time))
		return -1;
#endif


	int indexcount = 1;
	int index = ( m_iEntry - 1 ) & 31;

	while ((m_Entry[index].m_Time > _Time) && (indexcount < m_nEntries))
	{
		index	= ( index - 1 ) & 31;
		indexcount++;
	}

	return index;
}

//------------------------------------------------------------------


bool CModelHistory2::GetFlags(fp32 _Time, int& _Flags) const
{
#ifndef M_RTM
	if (false)
	{
		if (m_nEntries > 0)
			ConOutL(CStrF("GetIPM: Time = %f, NewestTime = %f, OldestTime = %f", _Time, m_Entry[m_iEntry].m_Time, m_Entry[m_iOldestEntry].m_Time));
		else
			ConOutL(CStrF("GetIPM: Time = %f", _Time));
	}

#endif

	int32 index1 = GetIndex1(_Time);

#ifndef M_RTM
	if (index1 == -1)
		return false;
#endif

	_Flags = m_Entry[index1].m_Flags;

	return true;
}


void CModelHistory2::AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero)
{
	if (false)
	{
		if (m_nEntries > 0)
			ConOutL(CStrF("AddEntry: Flags = %d, Time = %f, NewestTime = %f, OldestTime = %f", _Flags, _Time, m_Entry[m_iEntry].m_Time, m_Entry[m_iOldestEntry].m_Time));
		else
			ConOutL(CStrF("AddEntry: Flags = %d, Time = %f", _Flags, _Time));
	}

	if(m_nEntries > 0)
	{
		if (m_nEntries > 1)
		{
			if (_Time < (m_Entry[m_iOldestEntry].m_Time))
				Clear();
		}

		if (m_nEntries > 1)
		{
			if (m_Entry[m_iOldestEntry].m_Time > m_Entry[m_iEntry].m_Time)
				Clear();
		}

		if (m_nEntries > 0)
		{
			fp32 TimeJumpTolerance = 0.2f;			//was:	(4.0f * fp32(SERVER_TIMEPERFRAME));
			if (_Time < (m_Entry[m_iEntry].m_Time - TimeJumpTolerance))
				Clear();
		}
	}

	if ((m_nEntries == 0) && (_Time > 0) && _bAddZero)
		AddEntry(_Matrix, 0, _Flags, false);

	_Matrix.InverseOrthogonal(m_LastWorldToLocal);
	m_LastWorldToLocal_bIsValid = true;
	
	if (m_nEntries >= 2)
	{
		int iSecondEntry = ( m_iEntry - 1 ) & 31;

		const fp32 DeltaTime = _Time - m_Entry[iSecondEntry].m_Time;
		if (DeltaTime > m_EntryDuration)
		{
			int OldEntry = m_iEntry;
			m_Entry[m_iEntry].m_Time = _Time;
			m_iEntry = ( m_iEntry + 1 ) & 31;
			if (m_nEntries < Length)
			{
				m_nEntries++;
				ExpandBoundBox(m_Entry[OldEntry].m_Pos, m_LastWorldToLocal);
			}
			else
			{
				ClearBoundBox();
			}

			m_iOldestEntry = ( m_iEntry - m_nEntries + 1 ) & 31;
		}
		else
		{
			// Not enough time has passed since last entry was added.
			// Keep last iEntry and replace it with new entry data.
		}

		//m_Entry[m_iEntry].m_Matrix = _Matrix;
		CEntry& Entry = m_Entry[m_iEntry];
		Entry.m_Pos = CVec3Dfp32::GetMatrixRow(_Matrix, 3);
		Entry.m_Rot.Create(_Matrix);

		Entry.m_Time = _Time;
		Entry.m_Flags = _Flags;

		// Update speed of second entry (since we know a better speed now, using both neighbouring entries).
		if (m_nEntries > 1)
		{
			CVec3Dfp32 Speed01;
			if (DeltaTime == 0.0f)
				Speed01 = 0.0f;
			else
				Speed01 = (Entry.m_Pos - m_Entry[iSecondEntry].m_Pos) / (DeltaTime / m_EntryDuration);

			if (m_nEntries > 2)
			{
				int iSecondSecondEntry = ( m_iEntry - 2 ) & 31;
				CVec3Dfp32 Speed12 = (m_Entry[iSecondEntry].m_Pos - m_Entry[iSecondSecondEntry].m_Pos);
//				Entry.m_Speed = LERP(Speed12, Speed01, 1+0*0.5f * DeltaTime / m_EntryDuration);
				Entry.m_Speed = Speed01;
			}
			else
			{
				Entry.m_Speed = Speed01;
			}
		}

		if (m_BoundBox_bIsEmpty)
		{
			if (m_BoundBox_bStraightMovement)
				ExpandBoundBox(m_Entry[m_iOldestEntry].m_Pos, m_LastWorldToLocal);
			else
				CalculateBoundBox(m_LastWorldToLocal);
		}

		ExpandBoundBox(Entry.m_Pos, m_LastWorldToLocal);
	}
	else
	{
		m_iEntry = ( m_iEntry + 1 ) & 31;
		if (m_nEntries < Length) m_nEntries++;

		CEntry& Entry = m_Entry[m_iEntry];

		//m_Entry[m_iEntry].m_Matrix = _Matrix;
		Entry.m_Pos = CVec3Dfp32::GetMatrixRow(_Matrix, 3);
		Entry.m_Rot.Create(_Matrix);

		Entry.m_Time = _Time;
		Entry.m_Speed = 0;
		Entry.m_Flags = _Flags;

		ExpandBoundBox(Entry.m_Pos, m_LastWorldToLocal);

		m_iOldestEntry = ( m_iEntry - m_nEntries + 1 ) & 31;
	}

	if (m_iEntry != m_NoChange_iLastEntry)
	{
		if (m_NoChange_iLastEntry != -1)
		{
			const CEntry& Entry = m_Entry[m_iEntry];
//			bool bEqualPos = ((Entry.m_Pos.k[0] == m_Entry[m_NoChange_iLastEntry].m_Pos.k[0]) &&
//							  (Entry.m_Pos.k[1] == m_Entry[m_NoChange_iLastEntry].m_Pos.k[1]) &&
//							  (Entry.m_Pos.k[2] == m_Entry[m_NoChange_iLastEntry].m_Pos.k[2]));
//			bool bEqualRot = ((Entry.m_Rot.k[0] == m_Entry[m_NoChange_iLastEntry].m_Rot.k[0]) &&
//							  (Entry.m_Rot.k[1] == m_Entry[m_NoChange_iLastEntry].m_Rot.k[1]) &&
//							  (Entry.m_Rot.k[2] == m_Entry[m_NoChange_iLastEntry].m_Rot.k[2]) &&
//							  (Entry.m_Rot.k[3] == m_Entry[m_NoChange_iLastEntry].m_Rot.k[3]));
//			bool bEqual = bEqualPos && bEqualRot;

			const CEntry& NoChangeEntry = m_Entry[m_NoChange_iLastEntry];

			bool bEqual = (M_Fabs(Entry.m_Pos.k[0] - NoChangeEntry.m_Pos.k[0]) + M_Fabs(Entry.m_Pos.k[1] - NoChangeEntry.m_Pos.k[1]) + M_Fabs(Entry.m_Pos.k[2] - NoChangeEntry.m_Pos.k[2]) + M_Fabs(Entry.m_Rot.k[0] - NoChangeEntry.m_Rot.k[0]) + M_Fabs(Entry.m_Rot.k[1] - NoChangeEntry.m_Rot.k[1]) + M_Fabs(Entry.m_Rot.k[2] - NoChangeEntry.m_Rot.k[2])) == 0.0f;

			if (bEqual)
				m_NoChange_EntryCount++;
			else
				m_NoChange_EntryCount = 0;
		}

		m_NoChange_iLastEntry = m_iEntry;
	}
}


bool CModelHistory2::IsSingularity() const
{
	int c = 0;
	int i1 = m_iEntry;
	int i2 = ( i1 - 1 ) & 31;

	while (c < (m_nEntries - 1))
	{
//			if ((CVec3Dfp32::GetMatrixRow(m_Entry[i2].m_Matrix, 3) - CVec3Dfp32::GetMatrixRow(m_Entry[i1].m_Matrix, 3)) != 0)
		if ((m_Entry[i2].m_Pos - m_Entry[i1].m_Pos) != CVec3Dfp32(0))
			return false;

		c++;
		i1 = i2;
		i2 = ( i2 - 1 ) & 31;
	}

	return true;
}


bool CModelHistory2::GetInterpolatedSpeed(fp32 _Time, CVec3Dfp32& _Speed) const
{
	if (m_NoChange_EntryCount >= Length)
	{
		_Speed = m_Entry[m_iEntry].m_Speed;
		return true;
	}

	int32 index = GetIndex1(_Time);

#ifndef M_RTM
	if (index == -1)
		return false;
#endif
	
	uint index1 = index;
	uint index2 = ( index + 1 ) & 31;
	const CEntry& Entry1 = m_Entry[index1]; 
	const CEntry& Entry2 = m_Entry[index2]; 

	fp32 t1 = Entry1.m_Time;
	fp32 t2 = Entry2.m_Time;
	fp32 tf = (_Time - t1) / (t2 - t1);

	_Speed = LERP(Entry1.m_Speed, Entry2.m_Speed, tf);
	return true;
}

//------------------------------------------------------------------


bool CModelHistory2::GetInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, CVec3Dfp32& _Speed) const
{
	if (m_NoChange_EntryCount >= Length)
	{
		const CEntry& Entry = m_Entry[m_iEntry];
		Entry.m_Rot.CreateMatrix(_LocalToWorld);
		Entry.m_Pos.SetMatrixRow(_LocalToWorld, 3);
		_Speed = Entry.m_Speed;
		return true;
	}

	uint index = GetIndex1(_Time);

#ifndef M_RTM
	if (index == -1)
		return false;
#endif
	
	uint index1 = index;
	uint index2 = ( index + 1 ) & 31;
	const CEntry& Entry1 = m_Entry[index1];
	const CEntry& Entry2 = m_Entry[index2];

	fp32 t1 = Entry1.m_Time;
	fp32 t2 = Entry2.m_Time;
	fp32 tf = (_Time - t1) / (t2 - t1);

	CVec3Dfp32 Pos;
	CQuatfp32 Rot;
	Entry1.m_Pos.Lerp(Entry2.m_Pos, tf, Pos);
	Entry1.m_Rot.Lerp(Entry2.m_Rot, tf, Rot);
	Rot.CreateMatrix(_LocalToWorld);
	Pos.SetMatrixRow(_LocalToWorld, 3);

	_Speed = LERP(Entry1.m_Speed, Entry2.m_Speed, tf);
	return true;
}

//------------------------------------------------------------------


bool CModelHistory2::GetInterpolatedPos(fp32 _Time, CVec3Dfp32& _Pos, CVec3Dfp32& _Speed) const
{
	if (m_NoChange_EntryCount >= Length)
	{
		const CEntry& Entry = m_Entry[m_iEntry];
		_Pos = Entry.m_Pos;
		_Speed = Entry.m_Speed;
		return true;
	}

	uint index = GetIndex1(_Time);

#ifndef M_RTM
	if (index == -1)
		return false;
#endif
	
	uint index1 = index;
	uint index2 = ( index + 1 ) & 31;
	const CEntry& Entry1 = m_Entry[index1];
	const CEntry& Entry2 = m_Entry[index2];

	fp32 t1 = Entry1.m_Time;
	fp32 t2 = Entry2.m_Time;
	fp32 tf = (_Time - t1) / (t2 - t1);

	Entry1.m_Pos.Lerp(Entry2.m_Pos, tf, _Pos);
	_Speed = LERP(Entry1.m_Speed, Entry2.m_Speed, tf);
	return true;
}

//------------------------------------------------------------------


bool CModelHistory2::GetSplineInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, CVec3Dfp32& _Speed) const
{
	int32 index = GetIndex1(_Time);

#ifndef M_RTM
	if (index == -1)
		return false;
#endif
	
	if (m_nEntries < 3)
	{
		uint index1 = index;
		uint index2 = ( index + 1 ) & 31;
		const CEntry& Entry1 = m_Entry[index1];
		const CEntry& Entry2 = m_Entry[index2];

		fp32 t1 = Entry1.m_Time;
		fp32 t2 = Entry2.m_Time;
		fp32 tf = (_Time - t1) / (t2 - t1);

		CVec3Dfp32 Pos;
		CQuatfp32 Rot;
		Entry1.m_Pos.Lerp(Entry2.m_Pos, tf, Pos);
		Entry1.m_Rot.Lerp(Entry2.m_Rot, tf, Rot);
		Rot.CreateMatrix(_LocalToWorld);
		Pos.SetMatrixRow(_LocalToWorld, 3);
		
		_Speed = LERP(Entry1.m_Speed, Entry2.m_Speed, tf);
	}
	else
	{
		uint index0 = ( index - 1 ) & 31;
		uint index1 = index;
		uint index2 = ( index + 1 ) & 31;
		const CEntry& Entry0 = m_Entry[index0];
		const CEntry& Entry1 = m_Entry[index1];
		const CEntry& Entry2 = m_Entry[index2];

		fp32 t1 = Entry1.m_Time;
		fp32 t2 = Entry2.m_Time;
		fp32 tf = (_Time - t1) / (t2 - t1);

		/*
		CMat4Dfp32 FutureMat;
		MatrixReflect(m_Entry[index1].m_Matrix, m_Entry[index2].m_Matrix, FutureMat);
		MatrixSpline(m_Entry[index0].m_Matrix, m_Entry[index1].m_Matrix, Entry2.m_Matrix, FutureMat, tf, _LocalToWorld);
		*/

		{
			CVec3Dfp32 Pos;
			CQuatfp32 Rot;

			CVec3Dfp32 FuturePos;
			CQuatfp32 FutureRot;

			uint index3 = ( index2 + 1 ) & 31;
			const CEntry& Entry3 = m_Entry[index3];
			if (((m_iEntry < m_iOldestEntry) && ((index3 <= m_iEntry) || (index3 > m_iOldestEntry))) || ((m_iEntry > m_iOldestEntry) && (index3 <= m_iEntry)))
			{
				Entry1.m_Pos.Lerp(Entry2.m_Pos, 2, FuturePos); // FIXME: This is not the most natural extrapolation.
				Entry1.m_Rot.Lerp(Entry2.m_Rot, 2, FutureRot); // This, though, should do fine.
				FuturePos = LERP(FuturePos, Entry3.m_Pos, 0.8f);
				FutureRot.Lerp(Entry3.m_Rot, 0.8f, FutureRot);
			}
			else
			{
				Entry1.m_Pos.Lerp(Entry2.m_Pos, 2, FuturePos); // FIXME: This is not the most natural extrapolation.
				Entry1.m_Rot.Lerp(Entry2.m_Rot, 2, FutureRot); // This, though, should do fine.
			}

			fp32 dt01 = 1;
			fp32 dt12 = 1;
			fp32 dt23 = 1;
			
			/*
			CVec3Dfp32::Spline(&m_Entry[index0].m_Pos, &Entry1.m_Pos, &Entry2.m_Pos, 
							  &Entry1.m_Pos, &Entry2.m_Pos, &FuturePos,
							  &Pos, tf, dt01, dt12, dt12, dt23, 1);
			*/
			CVec3Dfp32::Spline3(Entry0.m_Pos, Entry1.m_Pos, Entry2.m_Pos, FuturePos, tf, Pos);

			CQuatfp32::Spline(&Entry0.m_Rot, &Entry1.m_Rot, &Entry2.m_Rot, 
							 &Entry1.m_Rot, &Entry2.m_Rot, &FutureRot,
							 &Rot, tf, dt01, dt12, dt12, dt23, 1);

			Rot.CreateMatrix(_LocalToWorld);
			Pos.SetMatrixRow(_LocalToWorld, 3);
		}

		_Speed = LERP(Entry1.m_Speed, Entry2.m_Speed, tf);
	}

	return true;
}


//------------------------------------------------------------------
//------------------------------------------------------------------
//------------------------------------------------------------------


MRTC_IMPLEMENT(CModelHistoryFX, CModelHistory2);


void CModelHistoryFX::operator=(const CXR_ModelInstance &_Instance)
{
	CModelHistory2::operator =(_Instance);

	const CModelHistoryFX* pHistory = safe_cast<const CModelHistoryFX>(&_Instance);
	if (pHistory)
	{
		// Duplicate FX data
		const int Len = pHistory->m_lPos.Len();

		m_lPos.SetLen(Len);
		m_lTimeFrac.SetLen(Len);

		memcpy(m_lPos.GetBasePtr(), pHistory->m_lPos.GetBasePtr(), sizeof(CVec3Dfp32) * Len);
		memcpy(m_lTimeFrac.GetBasePtr(), pHistory->m_lTimeFrac.GetBasePtr(), sizeof(fp32) * Len);
	}
}

void CModelHistoryFX::AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero)
{
	if(m_bUseUnit)
	{
		CMat4Dfp32 Unit;
		Unit.Unit();
		CModelHistory2::AddEntry(Unit, _Time, _Flags, _bAddZero);
	}
	else
		CModelHistory2::AddEntry(_Matrix, _Time, _Flags, _bAddZero);
}

