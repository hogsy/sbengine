#include "PCH.h"

#include "CModelHistory.h"


CModelHistory::CModelHistory()
{
	m_EntryDuration = 0;
	m_MaxEntryDelay = 0;
	m_pCustomModel = NULL;
	m_BoundBox_bStraightMovement = false;

	m_StoredBoundBox_bIsValid = false;

	Clear();
}


CModelHistory::CModelHistory(fp32 _EntryDuration, fp32 _MaxEntryDelay, bool _bStraightMovement)
{
	m_EntryDuration = _EntryDuration;
	m_MaxEntryDelay = _MaxEntryDelay;
	m_pCustomModel = NULL;
	m_BoundBox_bStraightMovement = _bStraightMovement;


	Clear();

	m_StoredBoundBox_bIsValid = false;
}


void CModelHistory::Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	if (_pModel != NULL)
	{
		m_pCustomModel = safe_cast<CXR_Model_Custom>(_pModel);
	}

	if (m_pCustomModel == NULL)
		ConOutL("CModelHistory::Create() - Invalid CustomModel!");
}


void CModelHistory::Clear()
{
	m_iEntry = -1;
	m_iOldestEntry = 0;
	m_nEntries = 0;

	ClearBoundBox();
}


bool CModelHistory::GetInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, fp32& _Speed, int& _Flags, IPMethod ipmethod)
{
	if (false)
	{
		if (m_nEntries > 0)
			ConOutL(CStrF("GetIPM: Time = %f, NewestTime = %f, OldestTime = %f", _Time, m_Entry[m_iEntry].m_Time, m_Entry[m_iOldestEntry].m_Time));
		else
			ConOutL(CStrF("GetIPM: Time = %f", _Time));
	}

	if (m_nEntries == 0)
		return false;

	if ((_Time > m_Entry[m_iEntry].m_Time) || (_Time < m_Entry[m_iOldestEntry].m_Time))
		return false;

	if (_Time == m_Entry[m_iEntry].m_Time) {
		//_LocalToWorld = m_Entry[m_iEntry].m_Matrix;
		m_Entry[m_iEntry].m_Rot.CreateMatrix(_LocalToWorld);
		m_Entry[m_iEntry].m_Pos.SetMatrixRow(_LocalToWorld, 3);
		return true;
	}

	if (_Time == m_Entry[m_iOldestEntry].m_Time) {
		//_LocalToWorld = m_Entry[m_iOldestEntry].m_Matrix;
		m_Entry[m_iOldestEntry].m_Rot.CreateMatrix(_LocalToWorld);
		m_Entry[m_iOldestEntry].m_Pos.SetMatrixRow(_LocalToWorld, 3);
		return true;
	}

	int indexcount = 1;
	int index = m_iEntry - 1;
	if (index < 0) index += Length;

	while ((m_Entry[index].m_Time > _Time) && (indexcount < m_nEntries))
	{
		index--;
		if (index < 0) index += Length;
		indexcount++;
	}
	
	if ((ipmethod == IPMethod_Linear) || (m_nEntries < 3))
	{
		int32 index1 = index;
		int32 index2 = index + 1;
		if (index2 >= Length) index2 -= Length;

		fp32 t1, t2;

		t1 = m_Entry[index1].m_Time;
		t2 = m_Entry[index2].m_Time;

		fp32 tf = (_Time - t1) / (t2 - t1);

		//MatrixLerp(m_Entry[index1].m_Matrix, m_Entry[index2].m_Matrix, tf, _LocalToWorld);

		CVec3Dfp32 Pos;
		CQuatfp32 Rot;
		m_Entry[index1].m_Pos.Lerp(m_Entry[index2].m_Pos, tf, Pos);
		m_Entry[index1].m_Rot.Lerp(m_Entry[index2].m_Rot, tf, Rot);
		Rot.CreateMatrix(_LocalToWorld);
		Pos.SetMatrixRow(_LocalToWorld, 3);
		
		_Speed = LERP(m_Entry[index1].m_Speed, m_Entry[index2].m_Speed, tf);
		_Flags = m_Entry[index1].m_Flags;
	}
	else if (ipmethod == IPMethod_Spline)
	{
		int index0 = index - 1;
		int index1 = index;
		int index2 = index + 1;
		if (index0 < 0) index0 += Length;
		if (index2 >= Length) index2 -= Length;

		fp32 t1, t2;

		t1 = m_Entry[index1].m_Time;
		t2 = m_Entry[index2].m_Time;

		fp32 tf = (_Time - t1) / (t2 - t1);

		/*
		CMat4Dfp32 FutureMat;
		MatrixReflect(m_Entry[index1].m_Matrix, m_Entry[index2].m_Matrix, FutureMat);
		MatrixSpline(m_Entry[index0].m_Matrix, m_Entry[index1].m_Matrix, m_Entry[index2].m_Matrix, FutureMat, tf, _LocalToWorld);
		*/

		{
			CVec3Dfp32 Pos;
			CQuatfp32 Rot;

			CVec3Dfp32 FuturePos;
			CQuatfp32 FutureRot;

			int index3 = index2 + 1;
			if (index3 >= Length) index3 -= Length;
			if (((m_iEntry < m_iOldestEntry) && ((index3 <= m_iEntry) || (index3 > m_iOldestEntry))) || ((m_iEntry > m_iOldestEntry) && (index3 <= m_iEntry)))
			{
				m_Entry[index1].m_Pos.Lerp(m_Entry[index2].m_Pos, 2, FuturePos); // FIXME: This is not the most natural extrapolation.
				m_Entry[index1].m_Rot.Lerp( m_Entry[index2].m_Rot, 2, FutureRot); // This, though, should do fine.
				FuturePos = LERP(FuturePos, m_Entry[index3].m_Pos, 0.8f);
				FutureRot.Lerp(m_Entry[index3].m_Rot, 0.8f, FutureRot);
			}
			else
			{
				m_Entry[index1].m_Pos.Lerp(m_Entry[index2].m_Pos, 2, FuturePos); // FIXME: This is not the most natural extrapolation.
				m_Entry[index1].m_Rot.Lerp(m_Entry[index2].m_Rot, 2, FutureRot); // This, though, should do fine.
			}

			fp32 dt01 = 1;
			fp32 dt12 = 1;
			fp32 dt23 = 1;
			
			/*
			CVec3Dfp32::Spline(&m_Entry[index0].m_Pos, &m_Entry[index1].m_Pos, &m_Entry[index2].m_Pos, 
							  &m_Entry[index1].m_Pos, &m_Entry[index2].m_Pos, &FuturePos,
							  &Pos, tf, dt01, dt12, dt12, dt23, 1);
			*/
			CVec3Dfp32::Spline3(m_Entry[index0].m_Pos, m_Entry[index1].m_Pos, m_Entry[index2].m_Pos, FuturePos, tf, Pos);

			CQuatfp32::Spline(&m_Entry[index0].m_Rot, &m_Entry[index1].m_Rot, &m_Entry[index2].m_Rot, 
							 &m_Entry[index1].m_Rot, &m_Entry[index2].m_Rot, &FutureRot,
							 &Rot, tf, dt01, dt12, dt12, dt23, 1);

			Rot.CreateMatrix(_LocalToWorld);
			Pos.SetMatrixRow(_LocalToWorld, 3);
		}

		_Speed = LERP(m_Entry[index1].m_Speed, m_Entry[index2].m_Speed, tf);
		_Flags = m_Entry[index1].m_Flags;
	}
	else
	{
		// Unsupported IPMethod!
		return false;
	}

	return true;
}


void CModelHistory::AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero)
{
	if (false)
	{
		if (m_nEntries > 0)
			ConOutL(CStrF("AddEntry: Flags = %d, Time = %f, NewestTime = %f, OldestTime = %f", _Flags, _Time, m_Entry[m_iEntry].m_Time, m_Entry[m_iOldestEntry].m_Time));
		else
			ConOutL(CStrF("AddEntry: Flags = %d, Time = %f", _Flags, _Time));
	}

/*
	if (m_nEntries > 1)
	{
		if (_Time > (m_Entry[m_iEntry].m_Time + m_MaxEntryDelay))
			Clear();
	}
*/

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
		fp32 TimeJumpTolerance = 0.2f;	//was: (4.0f * fp32(SERVER_TIMEPERFRAME));
		if (_Time < (m_Entry[m_iEntry].m_Time - TimeJumpTolerance))
			Clear();
	}

	if ((m_nEntries == 0) && (_Time > 0) && _bAddZero)
		AddEntry(_Matrix, 0, _Flags, false);

	if (m_nEntries >= 2)
	{
		int iSecondEntry = m_iEntry - 1;
//		if (iSecondEntry >= e_Length) iSecondEntry -= e_Length; // Should never happend!
		if (iSecondEntry < 0) iSecondEntry += Length;

		fp32 DeltaTime = _Time - m_Entry[iSecondEntry].m_Time;
		if (DeltaTime > m_EntryDuration)
		{
			//m_Entry[m_iEntry].m_Time = _Time;
			m_iEntry++;
			if (m_iEntry >= Length) m_iEntry -= Length; // Wrap around to zero.
			if (m_nEntries < Length)
			{
				m_nEntries++;
				ExpandBoundBox(m_Entry[m_iEntry].m_Pos);
			}
			else
			{
				ClearBoundBox();
			}

			m_iOldestEntry = m_iEntry - m_nEntries + 1;
			if (m_iOldestEntry < 0) m_iOldestEntry += Length;
		}
		else
		{
			// Not enough time has passed since last entry was added.
			// Keep last iEntry and replace it with new entry data.
		}

		//m_Entry[m_iEntry].m_Matrix = _Matrix;
		m_Entry[m_iEntry].m_Pos = CVec3Dfp32::GetMatrixRow(_Matrix, 3);
		m_Entry[m_iEntry].m_Rot.Create(_Matrix);

		m_Entry[m_iEntry].m_Time = _Time;
//			m_Entry[m_iEntry].m_Speed = (CVec3Dfp32::GetMatrixRow(m_Entry[m_iEntry].m_Matrix, 3) - CVec3Dfp32::GetMatrixRow(m_Entry[iSecondEntry].m_Matrix, 3)).e_Length();
		m_Entry[m_iEntry].m_Speed = (m_Entry[m_iEntry].m_Pos - m_Entry[iSecondEntry].m_Pos).Length();
		m_Entry[m_iEntry].m_Flags = _Flags;

		if (m_BoundBox_bIsEmpty)
		{
			if (m_BoundBox_bStraightMovement)
				ExpandBoundBox(m_Entry[m_iOldestEntry].m_Pos);
			else
				CalculateBoundBox();
		}

		ExpandBoundBox(m_Entry[m_iEntry].m_Pos);
	}
	else
	{
		m_iEntry++;
		if (m_iEntry >= Length) m_iEntry -= Length; // Wrap around to zero.
		if (m_nEntries < Length) m_nEntries++;

		//m_Entry[m_iEntry].m_Matrix = _Matrix;
		m_Entry[m_iEntry].m_Pos = CVec3Dfp32::GetMatrixRow(_Matrix, 3);
		m_Entry[m_iEntry].m_Rot.Create(_Matrix);

		m_Entry[m_iEntry].m_Time = _Time;
		m_Entry[m_iEntry].m_Speed = 0;
		m_Entry[m_iEntry].m_Flags = _Flags;

		ExpandBoundBox(m_Entry[m_iEntry].m_Pos);

		m_iOldestEntry = m_iEntry - m_nEntries + 1;
		if (m_iOldestEntry < 0) m_iOldestEntry += Length;
	}

/*
	if (false)
	{
		CWireContainer* pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
		if (pWC)
		{
			int c = 0;
			int i = m_iEntry;
			while (c < m_nEntries)
			{
				CMat4Dfp32 Matrix;
				m_Entry[i].m_Rot.CreateMatrix(Matrix);
				m_Entry[i].m_Pos.SetMatrixRow(Matrix, 3);
				pWC->RenderMatrix(Matrix, 0.001f, true, 0x40FF0000, 0x4000FF00, 0x400000FF);

				if (c < (m_nEntries - 2))
				{
					int i2 = i - 1; if (i2 < 0) i2 += e_Length;
					pWC->RenderWire(m_Entry[i].m_Pos, m_Entry[i2].m_Pos, 0xFF00FF00, 0.01f);
				}

				c++;
				i--;
				if (i < 0) i += e_Length;
			}
		}
	}		
*/
}


void CModelHistory::operator=(const CXR_ModelInstance &_Instance)
{
	const CModelHistory* pHistory = safe_cast<const CModelHistory>(&_Instance);
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

		m_pCustomModel = pHistory->m_pCustomModel;
	}
}


void CModelHistory::OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32 *_pMat, int _nMat, int _Flags)
{
	if (!_Context.m_pObj)
		return;

	if (_nMat == 0)
		return;

//	ConOutL(CStrF("OnRefresh: Time = %f", _GameTick * SERVER_TIMEPERFRAME));

	fp32 AnimTime = _Context.m_GameTime.GetTime();

	CMat4Dfp32 Mat = *_pMat;
	
	if (m_pCustomModel != NULL)
		m_pCustomModel->ModifyEntry(Mat, AnimTime);

	AddEntry(Mat, AnimTime, _Flags);
}

