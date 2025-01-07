#include "PCH.h"

#include "../MSystem.h"
#include "MSound_Core.h"


void *CSC_LipSyncManager::LSD_Get(int16 _WaveID)
{
	if( m_lData.GetBasePtr() && ( _WaveID >= 0 ) && ( _WaveID < m_lLipSyncLUT.Len() ) )
	{
		if( m_lLipSyncLUT[_WaveID].m_Index != -1 )
			return m_lData.GetBasePtr()+m_lLipSyncStartLUT[m_lLipSyncLUT[_WaveID].m_Index] + sizeof(fp32);
	}

	return NULL;
}

fp32 CSC_LipSyncManager::Timecode_Get(int16 _WaveID)
{
	if(_WaveID >= 0 && _WaveID < m_lLipSyncTimeCodes.Len())
		return m_lLipSyncTimeCodes[_WaveID];
	return 0.0f;
}

void CSC_LipSyncManager::LSD_Add(int16 _WaveID, CWaveContext *_pWaveContext)
{
	int nCount = m_lLipSyncLUT.Len();
	if( nCount != _pWaveContext->GetIDCapacity() )
	{
		m_lLipSyncLUT.SetLen( _pWaveContext->GetIDCapacity() );
		if( nCount < m_lLipSyncLUT.Len() )
		{
			for( int i = nCount; i < m_lLipSyncLUT.Len(); i++ )
			{
				m_lLipSyncLUT[i].m_Index = -1;
				m_lLipSyncLUT[i].m_bLoaded = 0;
			}
		}
	}
	m_lLipSyncLUT[_WaveID].m_Index = 0;	// Flag as used
}

void CSC_LipSyncManager::LSD_Clear()
{
//	m_lLipSyncLUT.Clear();
//	m_lLipSyncStartLUT.Clear();
//	m_lData.Clear();
	m_lLipSyncLUT.Destroy();
	m_lLipSyncStartLUT.Destroy();
	m_lData.Destroy();
	m_UsedLSDSize = 0;
}

void CSC_LipSyncManager::Wave_PrecacheFlush()
{
	LSD_Clear();
}

void CSC_LipSyncManager::LSD_Load(uint16 *_pPreCacheOrder, int _nIds, CWaveContext *_pWaveContext)
{
	m_UsedLSDSize	= 0;
	int nIDCount = m_lLipSyncLUT.Len();
	int nCount = 0;

	int nChunkSize = 0;
	int nLargestLSDSize = 0;
	if (nIDCount <= 0)
		return;

	for( int i = 0; i < _nIds; i++ )
	{
		int iIndex = _pPreCacheOrder[i];
		if( m_lLipSyncLUT[iIndex].m_Index != -1)
		{
			M_ASSERT(!m_lLipSyncLUT[iIndex].m_bLoaded, "");
			m_lLipSyncLUT[iIndex].m_Index = -1;	// Flag as no lipsync available (default)
			int iLocal;
			CWaveContainer *pContainer = _pWaveContext->GetWaveContainer(iIndex, iLocal);
			if(pContainer && iLocal >= 0)
			{
				int DataSize;
				if( pContainer->GetLSDSize( iLocal, DataSize ) )
				{
					DataSize = (DataSize + 3) & ~3;
					nLargestLSDSize = Max(nLargestLSDSize, DataSize);
					nChunkSize	+= DataSize;
					m_lLipSyncLUT[iIndex].m_Index = nCount;
					nCount++;
				}
			}
		}
		m_lLipSyncLUT[iIndex].m_bLoaded = true;
	}
	for( int i = 0; i < nIDCount; i++ )
	{
		if (!m_lLipSyncLUT[i].m_bLoaded)
		{
			if( m_lLipSyncLUT[i].m_Index != -1)
			{
				m_lLipSyncLUT[i].m_Index = -1;	// Flag as no lipsync available (default)
				m_lLipSyncLUT[i].m_bLoaded = true;	// Flag as no lipsync available (default)			
				int iLocal;
				CWaveContainer *pContainer = _pWaveContext->GetWaveContainer(i, iLocal);
				if(pContainer && iLocal >= 0)
				{
					int DataSize;
					if( pContainer->GetLSDSize( iLocal, DataSize ) )
					{
						DataSize = (DataSize + 3) & ~3;
						nLargestLSDSize = Max(nLargestLSDSize, DataSize);
						nChunkSize	+= DataSize;
						m_lLipSyncLUT[i].m_Index = nCount;
						nCount++;
					}
				}
			}
			m_lLipSyncLUT[i].m_bLoaded = true;
		}
	}

	if( nCount > 0 && nChunkSize > 0 )
	{
		TThinArray<uint8> lLoadBuffer;
		lLoadBuffer.SetLen(nLargestLSDSize);
		void* pLoadBuffer = lLoadBuffer.GetBasePtr();

		m_lData.SetLen( nChunkSize );
		m_lLipSyncStartLUT.SetLen( nCount );
		m_lLipSyncTimeCodes.SetLen( nCount );

		for( int i = 0; i < _nIds; i++ )
		{
			int iIndex = _pPreCacheOrder[i];
			if( m_lLipSyncLUT[iIndex].m_Index != -1)
			{
				M_ASSERT(m_lLipSyncLUT[iIndex].m_bLoaded, "");
				int iLocal;
				CWaveContainer *pContainer = _pWaveContext->GetWaveContainer(iIndex, iLocal);
				if(pContainer && iLocal >= 0)
				{
					int DataSize;
					if(pContainer->GetLSD(iLocal, pLoadBuffer, DataSize))
					{
						DataSize = (DataSize + 3) & ~3;
						m_lLipSyncStartLUT[m_lLipSyncLUT[iIndex].m_Index]	= m_UsedLSDSize;
						memcpy( m_lData.GetBasePtr() + m_UsedLSDSize, pLoadBuffer, DataSize );
						m_UsedLSDSize	+= DataSize;
					}
				}
			}
			m_lLipSyncLUT[iIndex].m_bLoaded = false;
		}

		for( int i = 0; i < nIDCount; i++ )
		{
			if (m_lLipSyncLUT[i].m_bLoaded)
			{
				if( m_lLipSyncLUT[i].m_Index != -1 )
				{
					int iLocal;
					CWaveContainer *pContainer = _pWaveContext->GetWaveContainer(i, iLocal);
					if(pContainer && iLocal >= 0)
					{
						int DataSize;
						if(pContainer->GetLSD(iLocal, pLoadBuffer, DataSize))
						{
							DataSize = (DataSize + 3) & ~3;
							m_lLipSyncStartLUT[m_lLipSyncLUT[i].m_Index]	= m_UsedLSDSize;
							memcpy( m_lData.GetBasePtr() + m_UsedLSDSize, pLoadBuffer, DataSize );
							m_UsedLSDSize	+= DataSize;
						}
					}
				}
				m_lLipSyncLUT[i].m_bLoaded = false;
			}

			// fetch a permanatly store the lipsync timecodes
			uint16 WaveID = i;
			if(m_lData.GetBasePtr() && WaveID < m_lLipSyncLUT.Len())
			{
				if(m_lLipSyncLUT[WaveID].m_Index != -1)
				{
					// extend the timecode list if needed
					if(WaveID >= m_lLipSyncTimeCodes.Len())
					{
						int k = m_lLipSyncTimeCodes.Len();
						m_lLipSyncTimeCodes.SetLen(WaveID+1);
						for(;k< m_lLipSyncTimeCodes.Len(); k++)
							m_lLipSyncTimeCodes[k] = 0.0f;
					}

					// fetch time code
					fp32 tc = *((fp32 *)(m_lData.GetBasePtr()+m_lLipSyncStartLUT[m_lLipSyncLUT[WaveID].m_Index]));
					::SwapLE(tc);
					m_lLipSyncTimeCodes[WaveID] = tc;
// -- die spammer, DIE! --					M_TRACEALWAYS("Timecode[%d] = %f\n", WaveID, tc);
				}
			}
		}

		_pWaveContext->LSD_Flush();
	}

}

void CSC_LipSyncManager::Wave_PrecacheBegin()
{
	m_bAllowLSDRead = true;

}

void CSC_LipSyncManager::Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds, CWaveContext *_pWaveContext)
{
	m_bAllowLSDRead = false;
	// Remove index data from containers
	LSD_Load(_pPreCacheOrder, _nIds, _pWaveContext);
}

void CSC_LipSyncManager::Wave_Precache(int _WaveID, CWaveContext *_pWaveContext)
{
	if( m_bAllowLSDRead )
	{
		int iLocal;
		CWaveContainer *pContainer = _pWaveContext->GetWaveContainer(_WaveID, iLocal);
		if(!LSD_Get(_WaveID) && pContainer && iLocal >= 0)
		{
			LSD_Add( _WaveID, _pWaveContext);
		}
	}
}

