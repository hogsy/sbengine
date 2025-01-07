
#ifndef __MRENDERPS3_GEOMETRY_H_INCLUDED
#define __MRENDERPS3_GEOMETRY_H_INCLUDED

namespace NRenderPS3GCM
{

	#define DATAFMTBITS_NUM_ZERO	0
	#define DATAFMTBITS_POS_ZERO	0
	#define DATAFMTBITS_POS_DECLARE(This, Parent) DATAFMTBITS_POS_##This	= DATAFMTBITS_POS_##Parent + DATAFMTBITS_NUM_##Parent,
	#define DATAFMTBITS_MASK_DECLARE(This)	DATAFMTBITS_MASK_##This	= ((1 << DATAFMTBITS_NUM_##This) - 1) << DATAFMTBITS_POS_##This,

	#define TEXFMTBITS_NUM_ZERO	0
	#define TEXFMTBITS_POS_ZERO	0
	#define TEXFMTBITS_POS_DECLARE(This, Parent) TEXFMTBITS_POS_##This	= TEXFMTBITS_POS_##Parent + TEXFMTBITS_NUM_##Parent,
	#define TEXFMTBITS_MASK_DECLARE(This)	TEXFMTBITS_MASK_##This	= ((1 << TEXFMTBITS_NUM_##This) - 1) << TEXFMTBITS_POS_##This,

	#define TEXCOMPBITS_NUM_ZERO	0
	#define TEXCOMPBITS_POS_ZERO	0
	#define TEXCOMPBITS_POS_DECLARE(This, Parent) TEXCOMPBITS_POS_##This	= TEXCOMPBITS_POS_##Parent + TEXCOMPBITS_NUM_##Parent,
	#define TEXCOMPBITS_MASK_DECLARE(This)	TEXCOMPBITS_MASK_##This	= ((1 << TEXCOMPBITS_NUM_##This) - 1) << TEXCOMPBITS_POS_##This,

	#define DATACOMPBITS_NUM_ZERO	0
	#define DATACOMPBITS_POS_ZERO	0
	#define DATACOMPBITS_POS_DECLARE(This, Parent) DATACOMPBITS_POS_##This	= DATACOMPBITS_POS_##Parent + DATACOMPBITS_NUM_##Parent,
	#define DATACOMPBITS_MASK_DECLARE(This)	DATACOMPBITS_MASK_##This	= ((1 << DATACOMPBITS_NUM_##This) - 1) << DATACOMPBITS_POS_##This,

	enum
	{
		ATTRIB_POSITION		= 0,
		ATTRIB_NORMAL,
		ATTRIB_COLOR0,
		ATTRIB_COLOR1,
		ATTRIB_BLENDWEIGHT1,
		ATTRIB_BLENDWEIGHT2,
		ATTRIB_BLENDINDICES1,
		ATTRIB_BLENDINDICES2,
		ATTRIB_TEXCOORD0,
		ATTRIB_TEXCOORD1,
		ATTRIB_TEXCOORD2,
		ATTRIB_TEXCOORD3,
		ATTRIB_TEXCOORD4,
		ATTRIB_TEXCOORD5,
		ATTRIB_TEXCOORD6,
		ATTRIB_TEXCOORD7,
		ATTRIB_MAX_NUM,

		FMT_POSITION		= M_Bit(ATTRIB_POSITION),		// 3 component
		FMT_NORMAL			= M_Bit(ATTRIB_NORMAL),		// 3 component
		FMT_COLOR0			= M_Bit(ATTRIB_COLOR0),		// 4 component
		FMT_COLOR1			= M_Bit(ATTRIB_COLOR1),		// 4 component
		FMT_BLENDWEIGHT1	= M_Bit(ATTRIB_BLENDWEIGHT1),		// n component	- if bit is set the components = 1 + 'value of COMP_BLENDWEIGHT1'
		FMT_BLENDWEIGHT2	= M_Bit(ATTRIB_BLENDWEIGHT2),		// n component	- if bit is set the components = 1 + 'value of COMP_BLENDWEIGHT2'
		FMT_BLENDINDICES1	= M_Bit(ATTRIB_BLENDINDICES1),		// n component	- if bit is set the components = 1 + 'value of COMP_BLENDINDICES1'
		FMT_BLENDINDICES2	= M_Bit(ATTRIB_BLENDINDICES2),		// n component	- if bit is set the components = 1 + 'value of COMP_BLENDINDICES2'
		FMT_TEXCOORD0		= M_Bit(ATTRIB_TEXCOORD0),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD0'
		FMT_TEXCOORD1		= M_Bit(ATTRIB_TEXCOORD1),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD1'
		FMT_TEXCOORD2		= M_Bit(ATTRIB_TEXCOORD2),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD2'
		FMT_TEXCOORD3		= M_Bit(ATTRIB_TEXCOORD3),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD3'
		FMT_TEXCOORD4		= M_Bit(ATTRIB_TEXCOORD4),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD4'
		FMT_TEXCOORD5		= M_Bit(ATTRIB_TEXCOORD5),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD5'
		FMT_TEXCOORD6		= M_Bit(ATTRIB_TEXCOORD6),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD6'
		FMT_TEXCOORD7		= M_Bit(ATTRIB_TEXCOORD7),		// n component	- if bit is set the components = 1 + 'value of COMP_TEXCOORD7'

		DATAFMTBITS_NUM_POSITION		= 3,
		DATAFMTBITS_NUM_NORMAL			= 3,
		DATAFMTBITS_NUM_COLOR0			= 1,
		DATAFMTBITS_NUM_COLOR1			= 1,
		DATAFMTBITS_NUM_BLENDWEIGHT1	= 3,
		DATAFMTBITS_NUM_BLENDWEIGHT2	= 3,
		DATAFMTBITS_NUM_BLENDINDICES1	= 1,
		DATAFMTBITS_NUM_BLENDINDICES2	= 1,

		DATAFMTBITS_POS_DECLARE(POSITION, ZERO)
		DATAFMTBITS_POS_DECLARE(NORMAL, POSITION)
		DATAFMTBITS_POS_DECLARE(COLOR0, NORMAL)
		DATAFMTBITS_POS_DECLARE(COLOR1, COLOR0)
		DATAFMTBITS_POS_DECLARE(BLENDWEIGHT1, COLOR1)
		DATAFMTBITS_POS_DECLARE(BLENDWEIGHT2, BLENDWEIGHT1)
		DATAFMTBITS_POS_DECLARE(BLENDINDICES1, BLENDWEIGHT2)
		DATAFMTBITS_POS_DECLARE(BLENDINDICES2, BLENDINDICES1)

		DATAFMTBITS_MASK_DECLARE(POSITION)
		DATAFMTBITS_MASK_DECLARE(NORMAL)
		DATAFMTBITS_MASK_DECLARE(COLOR0)
		DATAFMTBITS_MASK_DECLARE(COLOR1)
		DATAFMTBITS_MASK_DECLARE(BLENDWEIGHT1)
		DATAFMTBITS_MASK_DECLARE(BLENDWEIGHT2)
		DATAFMTBITS_MASK_DECLARE(BLENDINDICES1)
		DATAFMTBITS_MASK_DECLARE(BLENDINDICES2)

		TEXFMTBITS_NUM_TEXCOORD0		= 3,
		TEXFMTBITS_NUM_TEXCOORD1		= 3,
		TEXFMTBITS_NUM_TEXCOORD2		= 3,
		TEXFMTBITS_NUM_TEXCOORD3		= 3,
		TEXFMTBITS_NUM_TEXCOORD4		= 3,
		TEXFMTBITS_NUM_TEXCOORD5		= 3,
		TEXFMTBITS_NUM_TEXCOORD6		= 3,
		TEXFMTBITS_NUM_TEXCOORD7		= 3,

		TEXFMTBITS_POS_DECLARE(TEXCOORD0, ZERO)
		TEXFMTBITS_POS_DECLARE(TEXCOORD1, TEXCOORD0)
		TEXFMTBITS_POS_DECLARE(TEXCOORD2, TEXCOORD1)
		TEXFMTBITS_POS_DECLARE(TEXCOORD3, TEXCOORD2)
		TEXFMTBITS_POS_DECLARE(TEXCOORD4, TEXCOORD3)
		TEXFMTBITS_POS_DECLARE(TEXCOORD5, TEXCOORD4)
		TEXFMTBITS_POS_DECLARE(TEXCOORD6, TEXCOORD5)
		TEXFMTBITS_POS_DECLARE(TEXCOORD7, TEXCOORD6)

		TEXFMTBITS_MASK_DECLARE(TEXCOORD0)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD1)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD2)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD3)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD4)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD5)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD6)
		TEXFMTBITS_MASK_DECLARE(TEXCOORD7)

		DATACOMPBITS_NUM_POSITION		= 2,
		DATACOMPBITS_NUM_NORMAL			= 2,
		DATACOMPBITS_NUM_BLENDWEIGHT1	= 2,
		DATACOMPBITS_NUM_BLENDWEIGHT2	= 2,

		DATACOMPBITS_POS_DECLARE(POSITION, ZERO)
		DATACOMPBITS_POS_DECLARE(NORMAL, POSITION)
		DATACOMPBITS_POS_DECLARE(BLENDWEIGHT1, NORMAL)
		DATACOMPBITS_POS_DECLARE(BLENDWEIGHT2, BLENDWEIGHT1)

		DATACOMPBITS_MASK_DECLARE(POSITION)
		DATACOMPBITS_MASK_DECLARE(NORMAL)
		DATACOMPBITS_MASK_DECLARE(BLENDWEIGHT1)
		DATACOMPBITS_MASK_DECLARE(BLENDWEIGHT2)

		TEXCOMPBITS_NUM_TEXCOORD0		= 2,
		TEXCOMPBITS_NUM_TEXCOORD1		= 2,
		TEXCOMPBITS_NUM_TEXCOORD2		= 2,
		TEXCOMPBITS_NUM_TEXCOORD3		= 2,
		TEXCOMPBITS_NUM_TEXCOORD4		= 2,
		TEXCOMPBITS_NUM_TEXCOORD5		= 2,
		TEXCOMPBITS_NUM_TEXCOORD6		= 2,
		TEXCOMPBITS_NUM_TEXCOORD7		= 2,

		TEXCOMPBITS_POS_DECLARE(TEXCOORD0, ZERO)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD1, TEXCOORD0)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD2, TEXCOORD1)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD3, TEXCOORD2)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD4, TEXCOORD3)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD5, TEXCOORD4)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD6, TEXCOORD5)
		TEXCOMPBITS_POS_DECLARE(TEXCOORD7, TEXCOORD6)

		TEXCOMPBITS_MASK_DECLARE(TEXCOORD0)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD1)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD2)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD3)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD4)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD5)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD6)
		TEXCOMPBITS_MASK_DECLARE(TEXCOORD7)

	};

	class CRenderPS3Resource_VertexBuffer_Local : public CRenderPS3Resource_Local<GPU_VERTEXBUFFER_ALIGNMENT>
	{
		class CRenderPS3Resource_VertexBuffer_Local_BlockInfo : public CRenderPS3Resource_Local<GPU_VERTEXBUFFER_ALIGNMENT>::CRenderPS3Resource_Local_BlockInfo
		{
		public:
			uint32 GetRealSize() {return m_Size;}
			uint32 m_Size;
		};


	public:
		CRenderPS3Resource_VertexBuffer_Local_BlockInfo m_BlockInfo;

		~CRenderPS3Resource_VertexBuffer_Local()
		{
			delete [] m_pTransform;
			m_pTransform = NULL;
		}

		void Create(uint32 _Size)
		{
			m_BlockInfo.m_Size = _Size;
			Alloc(_Size, GPU_VERTEXBUFFER_ALIGNMENT, &m_BlockInfo, false);
		}

		void Clear()
		{
//			m_FmtBits = 0;
//			m_SizeBits = 0;
//			m_CompBits = 0;
			m_DataFmtBits = 0;
			m_DataCompBits = 0;
			m_TexFmtBits = 0;
			m_TexCompBits = 0;
			m_Stride = 0;
			m_PrimOffset = 0;
			m_PrimLen = 0;
			m_nV = 0;
			m_nTri = 0;
			m_PrimType = 0;
			m_pTransform = NULL;
			m_nTransform = 0;
		}

//		uint64 m_FmtBits:16;
//		uint64 m_SizeBits:16;
//		uint64 m_CompBits:24;
		uint64 m_DataFmtBits:(4 * 3) + (4 * 1);
		uint64 m_DataCompBits:(4 * 2);
		uint64 m_TexFmtBits:(8 * 3);
		uint64 m_TexCompBits:(8 * 2);
		uint64 m_Stride:8;
		uint32	m_PrimOffset;
		uint32	m_PrimLen;
		uint32	m_nV;
		uint32	m_nTri;
		uint32	m_PrimType;

//		fp32* m_pScaler;
		CRC_VRegTransform* m_pTransform;
//		uint32 m_ScalerCount;
		uint32 m_nTransform;

		union
		{
			uint8 m_Offset[ATTRIB_MAX_NUM];
			vec128 m_VOffset;
		};
		union
		{
			struct
			{
//				uint32 m_RSXOffset[ATTRIB_MAX_NUM];
				uint16 m_RSXFormat[ATTRIB_MAX_NUM];
			};
			struct
			{
//				vec128 m_RSXOffset_v[4];
				vec128 m_RSXFormat_v[2];
			};
		};

//		void SetFormat(uint32 _FmtBits, uint32 _SizeBits, uint32 _CompBits);
		void CreateVertexDeclaration(const CRC_VertexFormat& _VertexFormat);
		
		int GetSize()
		{
			return m_BlockInfo.GetRealSize();
		}

		uint32 GetDataAttribOffset(uint _Attrib)
		{
			M_ASSERT(_Attrib < 8, "Attrib out of range");
//			M_ASSERT(m_DataFmtBits & (uint64)(1 << _Attrib), "Attrib is not enabled");
			return m_LocalOffset + m_Offset[_Attrib];
		}

		uint GetTexAttribOffset(uint _iTex)
		{
			M_ASSERT(_iTex >= 8 && _iTex < 16, "Attrib out of range");
//			M_ASSERT(m_TexFmtBits & (uint64)(1 << (_iTex + ATTRIB_TEXCOORD0)), "Attrib is not enabled");
			return m_LocalOffset + m_Offset[_iTex];
		}
	};

	class CRenderPS3Resource_Collect_Main
	{
	public:
		MRTC_CriticalSection m_Lock;
		void*	m_pMem;
		uint32	m_MainOffset;
		uint32	m_WritePos;
		uint32	m_Size;

		CRenderPS3Resource_Collect_Main()
		{
			m_pMem = NULL;
			m_MainOffset = 0;
			m_WritePos = 0;
			m_Size = 0;
		}

		void Create(uint32 _Size)
		{
			m_pMem	= MRTC_SystemInfo::OS_AllocGPU(_Size, false);
			m_MainOffset = gcmAddressToOffset(m_pMem);
			m_Size = _Size;

			gcmSetWriteBackEndLabel(GPU_LABEL_COLLECTER, 0);
		}

		static bool NeedCollect(const void* _pMem)
		{
			return true;
			if(!_pMem)
				return true;
			const uint8* pMemory = (const uint8*)_pMem;

			// Heap 0 cannot be used as input as that is the pushbuffer
			for(uint i = gs_nPS3Heaps - 1; i > 0; i--)
			{
				const uint8* pHead = (const uint8*)gs_lpPS3Heaps[i];
				const uint8* pTail = pHead + gs_PS3HeapSize[i];
				if((pHead <= pMemory) && (pTail > pMemory))
					return false;
			}

			return true;
		}

		template <class tType>
		uint32 Collect(const tType* _pData, uint32 _Count)
		{
#ifdef PLATFORM_PS3
			if(!_pData)
				return 0;
			M_LOCK(m_Lock);
			if(!m_pMem)
				Create(4 * 1024 * 1024);

			volatile uint32* pLabel = gcmGetLabelAddress(GPU_LABEL_COLLECTER);
			uint32 DataSize = _Count * sizeof(tType);
			uint32 DataSizeAligned = (DataSize + 15) & ~15;
			uint32 WritePos = m_WritePos;
			uint32 ReadPos = *pLabel;

			if(WritePos < ReadPos && (WritePos + DataSizeAligned) >= ReadPos)
			{
				// Tail caught up to head, need to do some waiting so the GPU can munge through the buffer
				// Make sure processing is being done
				gcmFlush();
				// not enough room, have to restart from begining
				while(WritePos < ReadPos && (WritePos + DataSizeAligned) >= ReadPos)
				{
					// GPU Hasn't fetched enough data yet
					ReadPos = *pLabel;
				}
			}

			if(WritePos + DataSizeAligned > m_Size)
			{
				// Need to wrap damnit
				{
					// No room in collectbuffer, 
					// Make sure processing is being done
					gcmFlush();
					// not enough room, have to restart from begining
					while(ReadPos <= DataSizeAligned)
					{
						// GPU Hasn't fetched enough data yet
						ReadPos = *pLabel;
					}
				}
//				M_TRACEALWAYS("PS3GCM:: Collectbuffer wrapped\n");
				WritePos = 0;
			}

			memcpy((uint8*)m_pMem + WritePos, _pData, DataSize);
			m_WritePos	= WritePos + DataSizeAligned;

			M_EXPORTBARRIER;
			return m_MainOffset + WritePos;
#else
			return m_MainOffset;
#endif
		}

		void CollectLabel()
		{
			M_LOCK(m_Lock);
			gcmSetWriteBackEndLabel(GPU_LABEL_COLLECTER, m_WritePos);
		}
	};

	class CContext_Geometry
	{
		friend class CRCPS3GCM;
	protected:
		TThinArray<CRenderPS3Resource_VertexBuffer_Local* > m_lpVB;
		CRenderPS3Resource_VertexBuffer_Local** m_ppVB;
		TArray<CRC_VBIDInfo> m_lVBIDInfo;	// VBID info
		class CXR_VBContext* m_pVBC;
		uint32 m_CurrentVBID;
		uint32 m_CurrentMWComp;
		uint32 m_CurrentDefaultScalers;
		CVec4Dfp32 m_lScalers[18];

		CRenderPS3Resource_VertexBuffer_Local* GetVB(uint _VBID);
	public:
		MRTC_CriticalSection m_Lock;

		CRenderPS3Resource_Collect_Main m_Collector;

		CContext_Geometry() : m_pVBC(NULL), m_CurrentVBID(0), m_CurrentMWComp(0), m_CurrentDefaultScalers(0) {}
		void Create(class CXR_VBContext* _pVBC, uint32 _MaxVB, TArray<CRC_VBIDInfo> _VBIDInfo);

		void Build(uint32 _VBID);
		CRenderPS3Resource_VertexBuffer_Local* Bind(uint32 _VBID);
		void Disable();
		void Destroy(uint32 _VBID);

		bool Bind(const CRC_VertexBuffer& _VB);
		int GetVBSize(uint _VBID);

		void PrepareFrame();
		void PostDefrag();

		void PrecacheBegin(uint _Count);
		void PrecacheEnd();
		void PrecacheFlush();
		void Precache(uint _VBID);

		uint GetMWComp() const {return m_CurrentMWComp;}
	};

};

#endif
