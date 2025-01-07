
#ifndef __MRENDERPS3_FRAGMENTPROGRAM_H_INCLUDED
#define __MRENDERPS3_FRAGMENTPROGRAM_H_INCLUDED

namespace NRenderPS3GCM
{

	class CContext_FragmentProgram
	{
	protected:

		class CRenderPS3Resource_FragmentProgram_Local : public CRenderPS3Resource_Local<GPU_FRAGMENTPROGRAM_ALIGNMENT>
		{
			class CRenderPS3Resource_FragmentProgram_Local_BlockInfo : public CRenderPS3Resource_Local<GPU_FRAGMENTPROGRAM_ALIGNMENT>::CRenderPS3Resource_Local_BlockInfo
			{
			public:
				uint32 GetRealSize() {return m_Size;}
				uint32 m_Size;
			};
		public:
			class CTreeCompare
			{
			public:
				M_INLINE static int Compare(const CRenderPS3Resource_FragmentProgram_Local* _pFirst, const CRenderPS3Resource_FragmentProgram_Local* _pSecond, void* _pContext)
				{
					if(_pFirst->m_Hash != _pSecond->m_Hash)
					{
						return (_pFirst->m_Hash<_pSecond->m_Hash)?-1:1;
					}
					return 0;
				}
				M_INLINE static int Compare(const CRenderPS3Resource_FragmentProgram_Local* _pFirst, const uint32& _Key, void* _pContext)
				{
					if(_pFirst->m_Hash != _Key)
					{
						return (_pFirst->m_Hash<_Key)?-1:1;
					}
					return 0;
				}
			};

			CRenderPS3Resource_FragmentProgram_Local()
			{
				m_Program = 0;
				m_Hash = 0;
				m_UCodeSize = 0;
				m_nParamCount = 0;
				m_bInvalidNumberOfParams = 0;
			}

			void Create(void* _pProgram, uint _MemSize)
			{
				m_Program = (CGprogram&)_pProgram;
				m_MemSize = _MemSize;
				gcmCgInitProgram(m_Program);
				uint32 ucode_size;
				void *ucode;
				gcmCgGetUCode(m_Program, &ucode, &ucode_size);

				m_UCodeSize = ucode_size;

				Alloc(ucode_size, GPU_FRAGMENTPROGRAM_ALIGNMENT, &m_BlockInfo, false);
				memcpy((void*)m_pLocalBlock->GetMemory(), ucode, ucode_size); 
			}

			CRenderPS3Resource_FragmentProgram_Local_BlockInfo m_BlockInfo;
			DAVLAligned_Link(CRenderPS3Resource_FragmentProgram_Local, m_Link, const uint32, CTreeCompare);
			CGprogram m_Program;
			uint32 m_MemSize;
			uint32 m_Hash;
			uint32 m_UCodeSize:16;
			uint32 m_nParamCount:8;
			uint32 m_bInvalidNumberOfParams:1;
			CStr m_Name;
		};

		DAVLAligned_Tree(CRenderPS3Resource_FragmentProgram_Local, m_Link, const uint32, CRenderPS3Resource_FragmentProgram_Local::CTreeCompare) m_ProgramTree;
		CRenderPS3Resource_FragmentProgram_Local* m_pCurrentProgram;

		uint LoadBinary(const uint8* _pData, uint32 _Size, uint32 _Hash);
		uint LoadBinaryFromFile(const char* _pFile, uint32 _Hash);
	public:
		MRTC_CriticalSection m_Lock;
		uint32	m_bDirtyCache:1;

		void PrepareFrame();
		void Create();
		void Disable();
		void DeleteAllObjects();
		void LoadCache();
		void SaveCache();
		void Bind(CRC_ExtAttributes_FragmentProgram20* _pExtAttr);

		void PostDefrag();
	};

};

#endif	// __MRENDERPS3_FRAGMENTPROGRAM_H_INCLUDED
