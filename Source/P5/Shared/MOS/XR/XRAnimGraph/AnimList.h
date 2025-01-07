#ifndef AnimList_h
#define AnimList_h

//--------------------------------------------------------------------------------

//#include "../../Classes/GameWorld/WData.h"
#include "../../Classes/GameWorld/WDataRes_Anim.h"
#include "../../Classes/GameWorld/WMapData.h"

#define WAGI_USEAGRESOURCEMANAGEMENT
#define WAGI_USEAGRESOURCEMANAGEMENT_ONLY
#ifndef M_RTM
#if defined(PLATFORM_WIN_PC)
#if defined(WAGI_USEAGRESOURCEMANAGEMENT)
#define WAGI_RESOURCEMANAGEMENT_LOG
#endif
#endif
#endif

//--------------------------------------------------------------------------------

class CXRAG_Animation
{
	public:
		int16		m_iAnimContainerResource;
		int16		m_iAnimSeq; // Withing container.
		CXR_Anim_SequenceData* m_pSeq;
		uint8		m_Flags;
	// Flag field for tracking (debug)
//	#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		enum
		{
			XRAG_TAGLOADED				= 1 << 0,
			XRAG_TAGUSEDWHENNOTLOADED	= 1 << 1,
		};
//	#endif


		CXRAG_Animation() { Clear(); }

		CXRAG_Animation(int16 _iAnimContainerResource, int16 _iAnimSeq)
		{
			m_iAnimContainerResource = _iAnimContainerResource;
			m_iAnimSeq = _iAnimSeq;
			m_pSeq = NULL;
			m_Flags = 0;
		}

		void Clear()
		{
			m_iAnimContainerResource = 0;
			m_iAnimSeq = -1;
			m_pSeq = NULL;
		//#ifdef WAGI_RESOURCEMANAGEMENT_LOG
			m_Flags = 0;
		//#endif
		}

		void ClearCache()
		{
			m_pSeq = NULL;
		}

		bool IsValid() const
		{
			return ((m_iAnimContainerResource != 0) && (m_iAnimSeq >= 0));
		}

		spCXR_Anim_SequenceData GetAnimSequenceData(CWorldData* _pWData) const
		{
			if (m_pSeq)
				return m_pSeq;

			if (!IsValid())
				return NULL;

			spCWResource spAnimContainerResource = _pWData->GetResourceRef(m_iAnimContainerResource);
			if (spAnimContainerResource == NULL)
				return NULL;

			CWRes_Anim* pAnimRes = (CWRes_Anim*)(const CWResource*)spAnimContainerResource;
			pAnimRes->m_TouchTime = _pWData->m_TouchTime; // Added by Anton

			CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
			if (pAnimBase == NULL)
				return NULL;

			const_cast<CXRAG_Animation*>(this)->m_pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(m_iAnimSeq);

			return m_pSeq;
		}

		spCXR_Anim_SequenceData GetAnimSequenceData_MapData(CMapData* _pWData) const
		{
			if (m_pSeq)
				return m_pSeq;

			if (!IsValid())
				return NULL;

			spCWResource spAnimContainerResource = _pWData->GetResource(m_iAnimContainerResource);
			if (spAnimContainerResource == NULL)
				return NULL;

			CWRes_Anim* pAnimRes = (CWRes_Anim*)(const CWResource*)spAnimContainerResource;
			pAnimRes->m_TouchTime = _pWData->m_spWData->m_TouchTime; // Added by Anton

			CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
			if (pAnimBase == NULL)
				return NULL;

			const_cast<CXRAG_Animation*>(this)->m_pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(m_iAnimSeq);

			return m_pSeq;
//			return (CXR_Anim_SequenceData*)pAnimBase->GetSequence(m_iAnimSeq);
		}

};

//--------------------------------------------------------------------------------

class CAnimList_ContainerNames
{
public:
	TThinArray<CStr> m_lNames;

	void Clear()
	{
		m_lNames.Clear();
	}

	int32 GetNameIndex(CStr _ContainerName)
	{
		int32 Len = m_lNames.Len();
		CStr Upper = _ContainerName.UpperCase();
		for (int32 i = 0; i < Len; i++)
		{
			if (m_lNames[i] == Upper)
				return i;
		}
		int32 Index = m_lNames.Len();
		m_lNames.SetLen(Index+1);
		m_lNames[Index] = Upper;
		
		return Index;
	}

	CStr GetContainerName(int32 _iContainerName) const
	{
		if (m_lNames.ValidPos(_iContainerName))
			return m_lNames[_iContainerName];
		return CStr("");
	}
};

typedef TPtr<class CXRAG_AnimList> spCXRAG_AnimList;
class CXRAG_AnimList : public CReferenceCount
{
	public:
		
		TArray<spCWResource> m_lResources;
		mutable TArray<CXRAG_Animation>	m_lAnims;
#ifdef WAGI_USEAGRESOURCEMANAGEMENT
		// Should only be used during loading? But also used at respawn :/
		TThinArray<int16>	m_liNames;
		CAnimList_ContainerNames m_ContainerNames;
#endif
#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		//CLogFile  m_LogFile;
		bool	  m_bStartLogging;
#endif
	public:

#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		CXRAG_AnimList::CXRAG_AnimList()
		{
			m_bStartLogging = false;
		}

		void StartLogging()
		{
			m_bStartLogging = true;
		}
#endif

		spCXR_Anim_SequenceData GetAnimSequenceData(CWorldData* _pWData, int _iAnim)			
		{
			if (!m_lAnims.ValidPos(_iAnim))
				return NULL;

			if (m_lAnims[_iAnim].m_iAnimContainerResource <= 0 || !(m_lAnims[_iAnim].m_Flags & CXRAG_Animation::XRAG_TAGLOADED))
			{
				// Ok, new strategy, find the resource (might have been loaded already)
				//CStr LoadStr = "ANM:" + m_ContainerNames.GetContainerName(m_liNames[_iAnim]);
				int32 iRes = _pWData->ResourceExistsPartial("ANM:" + m_ContainerNames.GetContainerName(m_liNames[_iAnim]));
				//int iRes = _pWData->GetResourceIndex(LoadStr, WRESOURCE_CLASS_XSA,NULL);
				if (iRes > 0)
				{
					CWResource *pRes = _pWData->GetResource(iRes);
					SetContainer(_iAnim,iRes,pRes);
				}

				
	#ifdef WAGI_RESOURCEMANAGEMENT_LOG				
				//ConOutL(CStrF("§cf80WARNING Animation: %s:%d Not loaded (iAnim: %d)",GetContainerName(_iAnim).GetStr(),m_lAnims[_iAnim].m_iAnimSeq,_iAnim));
				if (/*m_bStartLogging &&*/ !(m_lAnims[_iAnim].m_Flags & CXRAG_Animation::XRAG_TAGUSEDWHENNOTLOADED))
				{
					ConOutL(CStrF("§cf80WARNING Animation: %s:%d Not loaded (iAnim: %d)",GetContainerName(_iAnim).GetStr(),m_lAnims[_iAnim].m_iAnimSeq,_iAnim));
					//CStr Log = CStrF("WARNING Animation used without being loaded: %s:%d\t(iAnim: %d)",GetContainerName(_iAnim).GetStr(),m_lAnims[_iAnim].m_iAnimSeq,_iAnim);
					//m_LogFile.Log(Log);
					m_lAnims[_iAnim].m_Flags |= CXRAG_Animation::XRAG_TAGUSEDWHENNOTLOADED;
				}
	#endif
			}

			return m_lAnims[_iAnim].GetAnimSequenceData(_pWData);
		}

#ifdef WAGI_USEAGRESOURCEMANAGEMENT
#ifndef M_RTM
		void LoadAllContainers(CMapData *_pMapData)
		{
			for(int i = 0; i < m_ContainerNames.m_lNames.Len(); i++)
			{
				CStr LoadStr = "ANM:" + m_ContainerNames.GetContainerName(i);
				int iRes = -1;
				M_TRY
				{
					iRes = _pMapData->m_spWData->GetResourceIndex(LoadStr, WRESOURCE_CLASS_XSA, _pMapData);
				}
				M_CATCH(
				catch (CCException &)
				{
					ConOutL(CStrF("Failed to load animation container %s", m_ContainerNames.m_lNames[i].Str()));
				}
				)
				CWResource *pRes = _pMapData->m_spWData->GetResource(iRes);
				if(pRes)
				{
					for(int j = 0; j < m_liNames.Len(); j++)
						if(m_liNames[j] == i)
							SetContainer(j, iRes, pRes);
				}
			}
		}
#endif

		void ClearContainerNames()
		{
			m_liNames.Clear();
			m_ContainerNames.Clear();
		}

		CStr GetContainerName(int32 _iAnim) const
		{
			if (m_liNames.ValidPos(_iAnim))
				return m_ContainerNames.GetContainerName(m_liNames[_iAnim]);
			return CStr();
		}

		void SetContainer(int _iAnim, int16 _iAnimContainerResource, CWResource *_pRes)
		{
			if (!m_lAnims.ValidPos(_iAnim))
				return;

		//#ifdef WAGI_RESOURCEMANAGEMENT_LOG
			if (!(m_lAnims[_iAnim].m_Flags & CXRAG_Animation::XRAG_TAGLOADED))
			{
				/*CStr Log = CStrF("Loading animation: %s:%d\t(iAnim: %d)",GetContainerName(_iAnim).GetStr(),m_lAnims[_iAnim].m_iAnimSeq,_iAnim);
				m_LogFile.Log(Log);*/
				m_lAnims[_iAnim].m_Flags |= CXRAG_Animation::XRAG_TAGLOADED;
			}
		//#endif

			m_lAnims[_iAnim].m_iAnimContainerResource = _iAnimContainerResource;
			m_lResources[_iAnim] = _pRes;
		}


		void SetAnim(int _iAnim, int16 _iAnimContainerResource, int16 _iAnimSeq, CWResource *_pRes, const CStr& _ContainerName)
		{
			if (!m_liNames.ValidPos(_iAnim))
			{
				m_liNames.SetLen(_iAnim + 1);
			}

			m_liNames[_iAnim] = m_ContainerNames.GetNameIndex(_ContainerName);

			SetAnim(_iAnim, _iAnimContainerResource, _iAnimSeq, _pRes);
		}

		int32 GetAnimSeqIndex(int32 _iAnim)
		{
			if (!m_lAnims.ValidPos(_iAnim))
				return 0;

			return m_lAnims[_iAnim].m_iAnimSeq;
		}
#endif
		void SetAnim(int _iAnim, int16 _iAnimContainerResource, int16 _iAnimSeq, CWResource *_pRes)
		{
			if (!m_lAnims.ValidPos(_iAnim))
			{
				m_lAnims.SetLen(_iAnim + 1);
				m_lResources.SetLen(_iAnim + 1);
			}

			m_lAnims[_iAnim].m_iAnimContainerResource = _iAnimContainerResource;
			m_lAnims[_iAnim].m_iAnimSeq = _iAnimSeq;
			m_lResources[_iAnim] = _pRes;
		}

		void ClearCache()
		{
			int32 Len = m_lAnims.Len();
			CXRAG_Animation* pBase = m_lAnims.GetBasePtr();
			for (int32 i = 0; i < Len; i++)
				pBase[i].ClearCache();
		}

};

//--------------------------------------------------------------------------------

#endif /* AnimList_h */