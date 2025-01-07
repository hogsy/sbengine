
#include "PCH.h"
#include "MRenderPS3_Context.h"

namespace NRenderPS3GCM
{
	enum
	{
		OCID_HASHFRAMES = 3,

		OCID_HASHIDS = 128,
		OCID_HASHBITS = 7,
		OCID_HASHNUM = 1 << OCID_HASHBITS,
		OCID_HASHMASK = (OCID_HASHNUM - 1),
	};

	class COCID
	{
	public:
		int m_QueryID;
	};

	class COSIDHash : public THash<int16, COCID>
	{
	public:
		void Insert(int _OCID, int _QueryID)
		{
			THash<int16, COCID>::Insert(_OCID, _QueryID & OCID_HASHMASK);
			CHashIDInfo* pID = &m_pIDInfo[_OCID];
			pID->m_QueryID = _QueryID;
		}

		int GetOCID(int _QueryID)
		{
			int ID = m_pHash[_QueryID & OCID_HASHMASK];
			while(ID != -1)
			{
				if (m_pIDInfo[ID].m_QueryID == _QueryID)
					return ID;
				ID = m_pIDInfo[ID].m_iNext;
			}
			return 0;
		}
	};

	int m_OC_ActiveQuery;
	int m_OC_iHashInsert;
	int m_OC_iHashQuery;
	COSIDHash m_OC_lQueryIDHash[OCID_HASHFRAMES];
	int m_OC_iHashOCIDNext[OCID_HASHFRAMES];

void CRCPS3GCM::OcclusionQuery_Init()
{
	for(int i = 0; i < OCID_HASHFRAMES; i++)
		m_OC_lQueryIDHash[i].Create(OCID_HASHIDS, OCID_HASHNUM, false);

	m_OC_iHashQuery = 0;
	m_OC_iHashInsert = 1;
	gcmSetWriteBackEndLabel(GPU_LABEL_OCCLUSIONQUERY + 0, 0);
	gcmSetWriteBackEndLabel(GPU_LABEL_OCCLUSIONQUERY + 1, 0);
	gcmSetWriteBackEndLabel(GPU_LABEL_OCCLUSIONQUERY + 2, 0);
}

void CRCPS3GCM::OcclusionQuery_PrepareFrame()
{
	m_OC_iHashQuery = m_OC_iHashInsert;
	m_OC_iHashInsert++;
	if (m_OC_iHashInsert >= OCID_HASHFRAMES)
		m_OC_iHashInsert = 0;

	m_OC_lQueryIDHash[m_OC_iHashInsert].Create(OCID_HASHIDS, OCID_HASHNUM, false);
	m_OC_iHashOCIDNext[m_OC_iHashInsert] = 1;

	m_OC_ActiveQuery = 0;
	gcmSetWriteBackEndLabel(GPU_LABEL_OCCLUSIONQUERY + m_OC_iHashInsert, 0);
}

int CRCPS3GCM::OcclusionQuery_AddQueryID(int _QueryID)
{
	if (m_OC_iHashOCIDNext[m_OC_iHashInsert] >= OCID_HASHIDS)
		return 0;

	m_OC_lQueryIDHash[m_OC_iHashInsert].Insert(m_OC_iHashOCIDNext[m_OC_iHashInsert], _QueryID);

	int OCID = m_OC_iHashOCIDNext[m_OC_iHashInsert] + m_OC_iHashInsert*OCID_HASHIDS;
	m_OC_iHashOCIDNext[m_OC_iHashInsert]++;

	return OCID;
}

int CRCPS3GCM::OcclusionQuery_FindOCID(int _QueryID)
{
	int OCID = m_OC_lQueryIDHash[m_OC_iHashQuery].GetOCID(_QueryID);
	if(OCID > 0)
		return OCID + m_OC_iHashQuery*128;
	return 0;
}

void CRCPS3GCM::OcclusionQuery_Begin(int _QueryID)
{
	int OCID = OcclusionQuery_AddQueryID(_QueryID);

	if(OCID > 0)
	{
		// Set pre-counter
		gcmSetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_OCCLUSIONQUERY + OCID * 2 + 0);
		m_OC_ActiveQuery = OCID;
	}
}

//JK-NOTE: Labels might not be required here but it works with them...
void CRCPS3GCM::OcclusionQuery_End()
{
	if(m_OC_ActiveQuery > 0)
	{
		// Set post-counter
		int OCID = m_OC_ActiveQuery;
		gcmSetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_OCCLUSIONQUERY + OCID * 2 + 1);
		gcmSetWriteBackEndLabel(GPU_LABEL_OCCLUSIONQUERY + m_OC_iHashInsert, OCID);
		m_OC_ActiveQuery = 0;
	}
}

int CRCPS3GCM::OcclusionQuery_GetVisiblePixelCount(int _QueryID)
{
#ifdef PLATFORM_PS3
	int OCID = OcclusionQuery_FindOCID(_QueryID);
	if(OCID > 0)
	{
		volatile uint32* pLabel = gcmGetLabelAddress(GPU_LABEL_OCCLUSIONQUERY + m_OC_iHashQuery);

		uint Label = *pLabel;
		while(Label < OCID)
		{
			Label = *pLabel;
		}

		return gcmGetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_OCCLUSIONQUERY + OCID * 2 + 1) - gcmGetReport(CELL_GCM_ZPASS_PIXEL_CNT, GPU_REPORT_OCCLUSIONQUERY + OCID * 2 + 0);
	}
#endif
	return 0;
}

};	// namespace NRenderPS3GCM
