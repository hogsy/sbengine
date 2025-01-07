#ifndef CModelHistory_h
#define CModelHistory_h

//----------------------------------------------------------------------

#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WClass.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WPhysState.h"

//----------------------------------------------------------------------

enum IPMethod
{
	IPMethod_Linear, 
	IPMethod_Spline,
};

//----------------------------------------------------------------------
// CModelHistory
//----------------------------------------------------------------------

class CModelHistory : public CXR_ModelInstance
{
	enum { Length = 20 };

public:

	class CEntry
	{

	public:

//		CMat4Dfp32	m_Matrix;
		CVec3Dfp32	m_Pos;
		CQuatfp32	m_Rot;
		fp32			m_Time;
		fp32			m_Speed;
		int			m_Flags;

		CEntry()
		{
			Clear();
		}

		void Clear()
		{
			//m_Matrix.Unit();
			m_Pos = 0;
			m_Rot.Unit();

			m_Time = 0;
			m_Speed = 0;
			m_Flags = 0;
		}
	};

private:

	fp32					m_EntryDuration;
	fp32					m_MaxEntryDelay;
		
	CEntry				m_Entry[Length];
	int					m_iEntry, m_iOldestEntry;
	int					m_nEntries;

	CBox3Dfp32			m_BoundBox;
	bool				m_BoundBox_bIsEmpty;
	bool				m_BoundBox_bStraightMovement;

	CBox3Dfp32			m_StoredBoundBox;
	bool				m_StoredBoundBox_bIsValid;

	CXR_Model_Custom*	m_pCustomModel;

public:

	CModelHistory();
	CModelHistory(fp32 _EntryDuration, fp32 _MaxEntryDelay, bool _bStraightMovement = false);
	void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	void Clear();
	void AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero = true);
	bool GetInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, fp32& _Speed, int& _Flags, IPMethod ipmethod = IPMethod_Linear);
	void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32 *_pMat, int _nMat, int _Flags);
	void operator=(const CXR_ModelInstance &_Instance);

	void SetEntryDuration(fp32 _EntryDuration)
	{
		m_EntryDuration = _EntryDuration;
	}

	~CModelHistory()
	{
	}

	TPtr<CXR_ModelInstance> Duplicate() const
	{
		TPtr<CModelHistory> spObj = MNew2(CModelHistory, m_EntryDuration, m_MaxEntryDelay);
		*spObj = *(CXR_ModelInstance*)this;
		return TPtr<CXR_ModelInstance>(spObj);
	}
	
	virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		return true;
	}
	
	const CEntry& GetMostRecentEntry()
	{
		if (m_nEntries > 0)
			return m_Entry[m_iEntry];
		else
			return m_Entry[0];
	}

	int
	GetSecondEntry()
	{
		if (m_nEntries < 2)
			return -1;

		int iSecondEntry = m_iEntry - 1;

		if (iSecondEntry >= Length)
			iSecondEntry -= Length;

		if (iSecondEntry <= 0) iSecondEntry += Length;

		return iSecondEntry;
	}

	fp32
	GetLatestTime()
	{
		return m_Entry[m_iEntry].m_Time;
	}

	fp32
	GetOldestTime()
	{
		return m_Entry[m_iOldestEntry].m_Time;
	}

	bool
	IsEmpty()
	{
		return (m_nEntries == 0);
	}
	
	bool
	IsSingularity()
	{
		int c = 0;
		int i1 = m_iEntry;
		int i2 = i1 - 1;

		while (c < (m_nEntries - 1))
		{
			if (i2 < 0) i2 += Length;

//			if ((CVec3Dfp32::GetMatrixRow(m_Entry[i2].m_Matrix, 3) - CVec3Dfp32::GetMatrixRow(m_Entry[i1].m_Matrix, 3)) != 0)
			if ((m_Entry[i2].m_Pos - m_Entry[i1].m_Pos) != CVec3Dfp32(0))
				return false;

			c++;
			i1 = i2;
			i2--;
		}

		return true;
	}

	bool HasStoredBoundBox()
	{
		return m_StoredBoundBox_bIsValid;
	}

	void StoreBoundBox(CBox3Dfp32& _BBox)
	{
		m_StoredBoundBox = _BBox;
		m_StoredBoundBox_bIsValid = true;
	}

	CBox3Dfp32 GetStoredBoundBox()
	{
		return m_StoredBoundBox;
	}

	bool IsStoredBoundBoxValid()
	{
		return m_StoredBoundBox_bIsValid;
	}

	CBox3Dfp32 GetBoundBox()
	{
		return m_BoundBox;
	}

private:

	void ClearBoundBox()
	{
		m_BoundBox_bIsEmpty = true;
	}

	void ExpandBoundBox(CVec3Dfp32& _Point)
	{
		// Slow, yes, but I'm lazy...(or stupid?)

		if (m_BoundBox_bIsEmpty)
		{
			m_BoundBox.m_Min = _Point;
			m_BoundBox.m_Max = _Point;
			m_BoundBox_bIsEmpty = false;
		}
		else
		{
			if (_Point.k[0] < m_BoundBox.m_Min.k[0]) m_BoundBox.m_Min.k[0] = _Point.k[0];
			if (_Point.k[1] < m_BoundBox.m_Min.k[1]) m_BoundBox.m_Min.k[1] = _Point.k[1];
			if (_Point.k[2] < m_BoundBox.m_Min.k[2]) m_BoundBox.m_Min.k[2] = _Point.k[2];
			if (_Point.k[0] > m_BoundBox.m_Max.k[0]) m_BoundBox.m_Max.k[0] = _Point.k[0];
			if (_Point.k[1] > m_BoundBox.m_Max.k[1]) m_BoundBox.m_Max.k[1] = _Point.k[1];
			if (_Point.k[2] > m_BoundBox.m_Max.k[2]) m_BoundBox.m_Max.k[2] = _Point.k[2];
		}
	}

	void CalculateBoundBox()
	{
		ClearBoundBox();

		int c = 0;
		int i = m_iEntry;
		while (c < m_nEntries)
		{
			ExpandBoundBox(m_Entry[i].m_Pos);
			c++;
			i--;
			if (i < 0) i += Length;
		}
	}

};

//----------------------------------------------------------------------

#endif /* CModelHistory_h */
