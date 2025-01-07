
#ifndef __MRENDERPS3_VERTEXPROGRAM_H_INCLUDED
#define __MRENDERPS3_VERTEXPROGRAM_H_INCLUDED

namespace NRenderPS3GCM
{

	class CContext_VertexProgram
	{
	protected:
		class CRenderPS3Resource_VertexProgram_Main : public CRenderPS3Resource_Main
		{
		public:
			class CTreeCompare
			{
			public:
				M_INLINE static int Compare(const CRenderPS3Resource_VertexProgram_Main* _pFirst, const CRenderPS3Resource_VertexProgram_Main* _pSecond, void* _pContext)
				{
					return memcmp(&_pFirst->m_Format, &_pSecond->m_Format, sizeof(CRC_VPFormat::CProgramFormat));
				}
				M_INLINE static int Compare(const CRenderPS3Resource_VertexProgram_Main* _pFirst, const CRC_VPFormat::CProgramFormat& _Key, void* _pContext)
				{
					return memcmp(&_pFirst->m_Format, &_Key, sizeof(CRC_VPFormat::CProgramFormat));
				}
			};

			CRenderPS3Resource_VertexProgram_Main() : m_Program(0), m_pUCode(0), m_UCodeSize(0) {}

			void Create(void* _pProgram, uint _MemSize)
			{
				m_MemSize = _MemSize;
				m_Program		= (CGprogram&)_pProgram;

				gcmCgInitProgram(m_Program);

				uint32 ucode_size;
				void *ucode;
				gcmCgGetUCode(m_Program, &ucode, &ucode_size);
				m_pUCode = ucode;
				m_UCodeSize = ucode_size;
			}

			DAVLAligned_Link(CRenderPS3Resource_VertexProgram_Main, m_Link, const CRC_VPFormat::CProgramFormat, CTreeCompare);
			CRC_VPFormat::CProgramFormat m_Format;
			CGprogram m_Program;
			void* m_pUCode;
			uint32 m_UCodeSize;
			uint32 m_MemSize;
		};

		// Vertex program
		CRC_VPGenerator m_ProgramGenerator;
		DAVLAligned_Tree(CRenderPS3Resource_VertexProgram_Main, m_Link, const CRC_VPFormat::CProgramFormat, CRenderPS3Resource_VertexProgram_Main::CTreeCompare) m_ProgramTree;
		CRenderPS3Resource_VertexProgram_Main* m_pCurrentProgram;

		uint LoadBinary(const uint8* _pBinary, uint32 _Size, const CRC_VPFormat::CProgramFormat& _Format);
		uint LoadBinaryFromFile(const char* _pFile, const CRC_VPFormat::CProgramFormat& _Format);
	public:
		MRTC_CriticalSection m_Lock;
		uint32 m_bDirtyCache:1;

		enum
		{
			EMaxConstants = 468
		};
		void PrepareFrame();
		void Create();
		void Disable();
		void DeleteAllObjects();
		void LoadCache();
		void SaveCache();
		void Bind(const CRC_VPFormat& _Format);
		static void SetConstants(uint _iStart, uint _nCount, const CVec4Dfp32* _pData);

		void PostDefrag();
	};

};

#endif	// __MRENDERPS3_VERTEXPROGRAM_H_INCLUDED
