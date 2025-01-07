
#ifndef __MRENDERPS3_RESOURCE_H_INCLUDED
#define __MRENDERPS3_RESOURCE_H_INCLUDED

namespace NRenderPS3GCM
{

	class CRenderPS3Resource
	{
	public:
		CRenderPS3Resource(bool _bLocal) : m_bLocal(_bLocal), m_LastUsedFrame(~0) {}
		uint32	m_bLocal:1;
		uint32	m_LastUsedFrame;
		static uint32	ms_ResourceFrameIndex;
		M_FORCEINLINE void Touch() {m_LastUsedFrame = ms_ResourceFrameIndex;}
	};

	typedef TPtr<CRenderPS3Resource> spCRenderPS3Resource;

	class CRenderPS3Resource_Main : public CRenderPS3Resource
	{
	protected:
		M_FORCEINLINE void Alloc(uint32 _Size, uint32 _Alignment)
		{
			m_pMemory = M_ALLOCALIGN(_Size, _Alignment);
			m_MainOffset = gcmAddressToOffset(m_pMemory);
		}
		M_FORCEINLINE void Free()
		{
			if(m_pMemory)
			{
				MRTC_MemFree(m_pMemory);
				m_pMemory = NULL;
				m_MainOffset = 0;
			}
		}

		void* m_pMemory;
		uint32 m_MainOffset;
	public:
		CRenderPS3Resource_Main() : CRenderPS3Resource(false), m_pMemory(NULL) {}
		~CRenderPS3Resource_Main()
		{
			Destroy();
		}

		void Destroy()
		{
#ifdef PLATFORM_PS3
			if(m_LastUsedFrame != ~0)
			{
				// This has been used, need to make sure it hasn't been used lately
				volatile uint32* pLabel = gcmGetLabelAddress(GPU_LABEL_PAGEFLIP);
				uint32 PageFlip = *pLabel;
				while(PageFlip < m_LastUsedFrame)
				{
					PageFlip = *pLabel;
				}
			}
#endif
			Free();
		}

		M_FORCEINLINE void* GetMemory() const {return m_pMemory;}
		M_FORCEINLINE uint32 GetOffset() const {return m_MainOffset;}
		M_FORCEINLINE bool IsAllocated() const {return m_pMemory != NULL;}
	};

	template <uint32 _tAlignment>
	class CRenderPS3Resource_Local : public CRenderPS3Resource
	{
	protected:
		class CRenderPS3Resource_Local_BlockInfo : public CPS3_Heap::CBlockInfo
		{
		public:
			uint32 GetAlignment() {return _tAlignment;}
		};

		M_FORCEINLINE void Alloc(uint32 _Size, uint32 _Alignment, CPS3_Heap::CBlockInfo* _pBlockInfo, bool _bMidBlocks)
		{
			m_pLocalBlock = CPS3_Heap::ms_This.Alloc(_Size, _Alignment, _pBlockInfo, _bMidBlocks);
			m_LocalOffset = gcmAddressToOffset((void*)m_pLocalBlock->GetMemory());
		}

		M_FORCEINLINE void Free()
		{
			if(m_pLocalBlock)
			{
				CPS3_Heap::ms_This.Free(m_pLocalBlock);
				m_pLocalBlock = NULL;
				m_LocalOffset = ~0;
			}
		}

		CPS3_Heap::CBlock* m_pLocalBlock;
	public:
		uint32	m_LocalOffset;

		enum
		{
			tAlignment = _tAlignment
		};

		CRenderPS3Resource_Local() : CRenderPS3Resource(true), m_pLocalBlock(NULL), m_LocalOffset(~0) {}
		~CRenderPS3Resource_Local()
		{
			Destroy();
		}

		void Destroy()
		{
#ifdef PLATFORM_PS3
			if(m_LastUsedFrame != ~0)
			{
				// This has been used, need to make sure it hasn't been used lately
				volatile uint32* pLabel = gcmGetLabelAddress(GPU_LABEL_PAGEFLIP);
				uint32 PageFlip = *pLabel;
				while(PageFlip < m_LastUsedFrame)
				{
					PageFlip = *pLabel;
				}
			}
#endif
			Free();
		}

		M_FORCEINLINE bool IsAllocated() const {return m_pLocalBlock != NULL;}
		M_FORCEINLINE void* GetMemory() const {return (void*)m_pLocalBlock->GetMemory();}
		M_FORCEINLINE uint32 GetOffset() const {return m_LocalOffset;}
		M_FORCEINLINE void PostDefrag()
		{
			if(m_pLocalBlock) m_LocalOffset = gcmAddressToOffset((void*)m_pLocalBlock->GetMemory());
		}
	};

};

#endif
