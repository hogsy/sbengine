#ifndef CModelHistory2_h
#define CModelHistory2_h

//----------------------------------------------------------------------

#include "MFloat.h"
#include "../../../../Shared/mos/xr/XRCustomModel.h"
//----------------------------------------------------------------------

enum IPMethod
{
	IPMethod_Linear, 
	IPMethod_Spline,
};

//----------------------------------------------------------------------

class CModelData : public CXR_ModelInstance
{
public:

	class CXR_Model_Custom*	m_pCustomModel;

	CMat4Dfp32			m_LastLocalToWorld;
	CMat4Dfp32			m_LastWorldToLocal;
	bool				m_LastLocalToWorld_bIsValid;
	bool				m_LastWorldToLocal_bIsValid;

	int32				m_CreationTick;

public:

	CModelData()
	{
		m_pCustomModel = NULL;
		m_LastLocalToWorld_bIsValid = false;
		m_LastWorldToLocal_bIsValid = false;
		m_CreationTick = 0;
	}

	virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		m_CreationTick = (int32)_Context.m_GameTick;

		if (_pModel != NULL)
		{
			m_pCustomModel = safe_cast<CXR_Model_Custom>(_pModel);
		}

		if (m_pCustomModel == NULL)
			ConOutL("Create() - Invalid CustomModel!");
	}

	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32 *_pMat, int _nMat, int _Flags)
	{
		if (!_Context.m_pObj)
			return;

		if(_nMat == 0)
			return;

		_pMat->InverseOrthogonal(m_LastWorldToLocal);
		m_LastWorldToLocal_bIsValid = true;
	}

	virtual void operator=(const CXR_ModelInstance &_Instance)
	{
		const CModelData* pData = safe_cast<const CModelData>(&_Instance);
		if (pData)
		{
			m_pCustomModel = pData->m_pCustomModel;
			m_LastLocalToWorld = pData->m_LastLocalToWorld;
			m_LastWorldToLocal = pData->m_LastWorldToLocal;
			m_LastLocalToWorld_bIsValid = pData->m_LastLocalToWorld_bIsValid;
			m_LastWorldToLocal_bIsValid = pData->m_LastWorldToLocal_bIsValid;
		}
	}

	virtual TPtr<CXR_ModelInstance> Duplicate() const
	{
		TPtr<CModelData> spObj = MNew(CModelData);
		*spObj = *(CXR_ModelInstance*)this;
		return TPtr<CXR_ModelInstance>(spObj);
	}

};

//----------------------------------------------------------------------

class CModelHistory2: public CModelData
{
	friend class CModelHistoryFX;

	MRTC_DECLARE;

	enum { Length = 32 };

public:

	class CEntry
	{

	public:

		CVec3Dfp32	m_Pos;
		CQuatfp32	m_Rot;
		fp32			m_Time;
		CVec3Dfp32	m_Speed;
		int			m_Flags;

		CEntry()
		{
			Clear();
		}

		void Clear()
		{
			m_Pos = 0;
			m_Rot.Unit();

			m_Time = 0;
			m_Speed = 0;
			m_Flags = 0;
		}
	};

	//------------------------------------------------------------------

private:

	fp32					m_EntryDuration;
	fp32					m_MaxEntryDelay;
		
	CEntry				m_Entry[Length];
	int					m_iEntry, m_iOldestEntry;
	int					m_nEntries;

	CBox3Dfp32			m_BoundBox;
	bool				m_BoundBox_bIsEmpty;
	bool				m_BoundBox_bStraightMovement;
	
	int					m_NoChange_iLastEntry;
	int					m_NoChange_EntryCount;

public:

	//------------------------------------------------------------------

	CModelHistory2()
	{
		m_EntryDuration = 0;
		m_MaxEntryDelay = 0;
		m_BoundBox_bStraightMovement = false;

		Clear();
	}

	//------------------------------------------------------------------

	CModelHistory2(fp32 _EntryDuration, fp32 _MaxEntryDelay, bool _bStraightMovement = false)
	{
		m_EntryDuration = _EntryDuration;
		m_MaxEntryDelay = _MaxEntryDelay;
		m_BoundBox_bStraightMovement = _bStraightMovement;

		Clear();
	}

	//------------------------------------------------------------------

	void SetEntryDuration(fp32 _EntryDuration)
	{
		m_EntryDuration = _EntryDuration;
	}

	//------------------------------------------------------------------

	virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		CModelData::Create(_pModel, _Context);
	}

	//------------------------------------------------------------------

	void Clear();
	virtual void operator=(const CXR_ModelInstance &_Instance);
	virtual void AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero = true);
	int GetIndex1(fp32 _Time) const;
	bool GetFlags(fp32 _Time, int& _Flags) const;
	bool GetInterpolatedSpeed(fp32 _Time, CVec3Dfp32& _Speed) const;
	bool GetInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, CVec3Dfp32& _Speed) const;
	bool GetInterpolatedPos(fp32 _Time, CVec3Dfp32& _Pos, CVec3Dfp32& _Speed) const;
	bool GetSplineInterpolatedMatrix(fp32 _Time, CMat4Dfp32& _LocalToWorld, CVec3Dfp32& _Speed) const;

	//------------------------------------------------------------------

	virtual TPtr<CXR_ModelInstance> Duplicate()
	{
		TPtr<CModelHistory2> spObj = MNew2(CModelHistory2, m_EntryDuration, m_MaxEntryDelay);
		*spObj = *(CXR_ModelInstance*)this;
		return TPtr<CXR_ModelInstance>(spObj);
	}

	//------------------------------------------------------------------

	virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		return true;
	}

	//------------------------------------------------------------------

	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32 *_pMat, int _nMat, int _Flags)
	{
		if (!_Context.m_pObj)
			return;

		if(_nMat == 0)
			return;

	//		ConOutL(CStrF("OnRefresh: Time = %f", _GameTick * SERVER_TIMEPERFRAME));

		fp32 AnimTime = _Context.m_GameTime.GetTime();

		CMat4Dfp32 Mat = *_pMat;
		
		if (m_pCustomModel != NULL)
			m_pCustomModel->ModifyEntry(Mat, AnimTime);

		AddEntry(Mat, AnimTime, _Flags);
	}

	//------------------------------------------------------------------

/*
	const CEntry& GetMostRecentEntry()
	{
		if (m_nEntries > 0)
			return m_Entry[m_iEntry];
		else
			return m_Entry[0];
	}

	int GetSecondEntry()
	{
		if (m_nEntries < 2)
			return -1;

		int iSecondEntry = m_iEntry - 1;

		if (iSecondEntry >= Length)
			iSecondEntry -= Length;

		if (iSecondEntry <= 0) iSecondEntry += Length;

		return iSecondEntry;
	}
*/

	//------------------------------------------------------------------

	fp32 GetLatestTime() const
	{
		return m_Entry[m_iEntry].m_Time;
	}

	//------------------------------------------------------------------

	fp32 GetOldestTime() const
	{
		return m_Entry[m_iOldestEntry].m_Time;
	}

	//------------------------------------------------------------------

	bool IsEmpty() const
	{
		return (m_nEntries == 0);
	}

	//------------------------------------------------------------------

	bool IsSingularity() const;

	//------------------------------------------------------------------

	bool HasValidBoundBox() const
	{
		return !m_BoundBox_bIsEmpty;
	}

	//------------------------------------------------------------------

	const CBox3Dfp32& GetBoundBox()
	{
		return m_BoundBox;
	}

	//------------------------------------------------------------------

private:

	//------------------------------------------------------------------

	void ClearBoundBox()
	{
		m_BoundBox_bIsEmpty = true;
	}

	//------------------------------------------------------------------

	void ExpandBoundBox(const CVec3Dfp32& _WorldPoint, const CMat4Dfp32& _WorldToLocal)
	{
		CVec3Dfp32 LocalPoint;
		_WorldPoint.MultiplyMatrix(_WorldToLocal, LocalPoint);

		if (m_BoundBox_bIsEmpty)
		{
			m_BoundBox.m_Min = LocalPoint;
			m_BoundBox.m_Max = LocalPoint;
			m_BoundBox_bIsEmpty = false;
		}
		else
		{
			m_BoundBox.Expand(LocalPoint);
		}
/*
		if (fabs(m_BoundBox.m_Max[0] - m_BoundBox.m_Min[0]) > 2000.0f ||
			fabs(m_BoundBox.m_Max[1] - m_BoundBox.m_Min[1]) > 2000.0f ||
			fabs(m_BoundBox.m_Max[2] - m_BoundBox.m_Min[2]) > 2000.0f)
		{
			ConOut(CStrF("(%.8x) Fuckad HISTORY boundingbox %s", this, m_BoundBox.GetString().Str() ));
			CVec3Dfp32 Center;
			m_BoundBox.GetCenter(Center);
			m_BoundBox.m_Min = Center;
			m_BoundBox.m_Max = Center;
			m_BoundBox.m_Min -= 64;
			m_BoundBox.m_Max += 64;
		}
*/	
	}

	//------------------------------------------------------------------

	void CalculateBoundBox(const CMat4Dfp32& _WorldToLocal)
	{
		ClearBoundBox();

		int c = 0;
		int i = m_iEntry;
		const CEntry* M_RESTRICT pEntries = m_Entry;
		while (c < m_nEntries)
		{
			ExpandBoundBox(pEntries[i].m_Pos, _WorldToLocal);
			c++;
//			i--;
//			if (i < 0) i += Length;
			i	= ( i - 1 ) & 31;
		}
	}

};

//----------------------------------------------------------------------

class CModelHistoryFX : public CModelHistory2
{
	MRTC_DECLARE;

private:
	TThinArray<CVec3Dfp32>	m_lPos;
	TThinArray<fp32>			m_lTimeFrac;
	bool					m_bUseUnit;

public:
	CModelHistoryFX()
		: CModelHistory2()
	{
		m_lPos.Clear();
		m_lTimeFrac.Clear();
		m_bUseUnit = false;
	}

	CModelHistoryFX(fp32 _EntryDuration, fp32 _MaxEntryDelay, bool _bStraightMovement = false)
		: CModelHistory2(_EntryDuration, _MaxEntryDelay, _bStraightMovement)
	{
		m_lPos.Clear();
		m_lTimeFrac.Clear();
		m_bUseUnit = false;
	}

	M_INLINE void CreateFXHistory(const uint32& _Len)
	{
		m_lPos.SetLen(_Len);
		m_lTimeFrac.SetLen(_Len);

		fp32* pTimeFrac = m_lTimeFrac.GetBasePtr();
		for(int i = 0; i < _Len; i++)
			pTimeFrac[i] = 2.0f;
	}

	M_INLINE CVec3Dfp32* GetPositionHistory()
	{
        return m_lPos.GetBasePtr();
	}

	M_INLINE fp32* GetTimeFracHistory()
	{
		return m_lTimeFrac.GetBasePtr();
	}

	M_INLINE void UseUnit(const bool _bUseUnit)
	{
		m_bUseUnit = _bUseUnit;
	}

	virtual void operator=(const CXR_ModelInstance &_Instance);

	virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		CModelHistory2::Create(_pModel, _Context);
	}

	virtual TPtr<CXR_ModelInstance> Duplicate()
	{
		TPtr<CModelHistoryFX> spObj = MNew2(CModelHistoryFX, m_EntryDuration, m_MaxEntryDelay);
		*spObj = *(CXR_ModelInstance*)this;
		return TPtr<CXR_ModelInstance>(spObj);
	}

	virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
	{
		return CModelHistory2::NeedRefresh(_pModel, _Context);
	}

	virtual void AddEntry(const CMat4Dfp32& _Matrix, fp32 _Time, int _Flags, bool _bAddZero = true);
};

#endif /* CModelHistory2_h */
