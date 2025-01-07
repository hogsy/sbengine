
#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

#include "../../XR/XRVBContext.h"

namespace NRenderPS3GCM
{

void CRCPS3GCM::Geometry_Color(uint32 _Col)
{
	CRC_Core::Geometry_Color(_Col);
	CVec4Dfp32 Color = M_VMul(M_VLd_Pixel32_f32(&_Col), M_VScalar(1.0f / 255.0f));

	CRCPS3GCM::ms_This.m_ContextVP.SetConstants(10, 1, &Color);
}

void CRCPS3GCM::Geometry_Precache(int _VBID)
{
	MSCOPE(CRCPS3GCM::Geometry_Precache, RENDER_PS3);
	m_ContextGeometry.Precache(_VBID);
}



static int IsSinglePrimType(const uint16* _piIndices, int _nPrim)
{
	CRCPrimStreamIterator StreamIterate(_piIndices, _nPrim);
	if(StreamIterate.IsValid())
	{
		int CurrentType = StreamIterate.GetCurrentType();
		do
		{
			if(CurrentType != StreamIterate.GetCurrentType())
				return 0;
		}
		while(StreamIterate.Next());
		return CurrentType;
	}

	return 0;
}

static void GetPrimStats(const uint16* _piIndices, uint32 _nPrim, uint32& _PrimCount, uint32& _IndexCount)
{
	CRCPrimStreamIterator StreamIterate(_piIndices, _nPrim);
	if(StreamIterate.IsValid())
	{
		int CurrentType = StreamIterate.GetCurrentType();
		do
		{
			_PrimCount++;
			const uint16* pPrim = StreamIterate.GetCurrentPointer();
			_IndexCount += *pPrim;
		}
		while(StreamIterate.Next());
	}
}

static uint32 AssemblePrimStream(TThinArray<uint16>& _liPrim, const uint16* _piIndices, int _nPrim)
{
	uint32 nPrimCount = 0;
	uint32 nIndexCount = 0;
	GetPrimStats(_piIndices, _nPrim, nPrimCount, nIndexCount);
	_liPrim.SetLen(nIndexCount + nPrimCount - 1);

	CRCPrimStreamIterator StreamIterate(_piIndices, _nPrim);
	if(StreamIterate.IsValid())
	{
		int CurrentType = StreamIterate.GetCurrentType();
		uint16* pStream = _liPrim.GetBasePtr();
		int iP = 0;
		int nTri = 0;
		do
		{
			if(iP > 0)
			{
				// Insert prim restart token
				pStream[iP++] = 0xffff;
			}
			const uint16* pPrim = StreamIterate.GetCurrentPointer();
			int nV = *pPrim;
			memcpy(pStream + iP, pPrim + 1, nV * 2);
			iP += nV;
			nTri += (nV - 2);
		}
		while(StreamIterate.Next());

		return CurrentType;
	}

	return 0;
}
void CContext_Geometry::Create(CXR_VBContext* _pVBC, uint32 _MaxVB, TArray<CRC_VBIDInfo> _VBIDInfo)
{
	m_Collector.Create(4 * 1024 * 1024);
	m_CurrentVBID = 0;
	m_pVBC = _pVBC;
	m_lpVB.SetLen(_MaxVB);
	m_ppVB = m_lpVB.GetBasePtr();
	m_lVBIDInfo = _VBIDInfo;
	memset(m_lpVB.GetBasePtr(), 0, m_lpVB.ListSize());
}

void CContext_Geometry::PrepareFrame()
{
	Disable();
}

void CContext_Geometry::PostDefrag()
{
	M_LOCK(m_Lock);
	TAP_RCD<CRenderPS3Resource_VertexBuffer_Local* > lpVB(m_lpVB);
	for(uint i = 0; i < lpVB.Len(); i++)
	{
		if(lpVB[i])
		{
			lpVB[i]->PostDefrag();
		}
	}
}
/*
static void GetAllBits(const CRC_VertexBuffer& _VB, uint32& _FmtBits, uint32& _SizeBits, uint32& _CompBits)
{
	uint32 FmtBits = 0;
	uint32 SizeBits = 0;
	uint32 CompBits = 0;

	if(_VB.m_pV)
	{
		FmtBits	|= FMT_POSITION;
		SizeBits |= 0 << SIZEBITS_POS_POSITION;
	}
	if(_VB.m_pN)
	{
		FmtBits |= FMT_NORMAL;
		SizeBits = 0 << SIZEBITS_POS_NORMAL;
	}
	if(_VB.m_pCol)
	{
		FmtBits |= FMT_COLOR0;
	}
	if(_VB.m_pSpec)
	{
		FmtBits |= FMT_COLOR1;
	}
	if(_VB.m_pMW)
	{
		FmtBits |= FMT_BLENDWEIGHT1;
		CompBits |= ((_VB.m_nMWComp>4)?3:(_VB.m_nMWComp - 1)) << COMPBITS_POS_BLENDWEIGHT1;
		if(_VB.m_nMWComp > 4)
		{
			FmtBits |= FMT_BLENDWEIGHT2;
			CompBits |= (_VB.m_nMWComp - 4 - 1) << COMPBITS_POS_BLENDWEIGHT2;
		}
	}
	if(_VB.m_pMI)
	{
		FmtBits |= FMT_BLENDINDICES1;
		if(_VB.m_nMWComp > 4)
		{
			FmtBits |= FMT_BLENDINDICES2;
		}
	}
	if(_VB.m_pTV[0] && _VB.m_nTVComp[0])
	{
		FmtBits |= FMT_TEXCOORD0;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD0;
		CompBits |= (_VB.m_nTVComp[0] - 1) << COMPBITS_POS_TEXCOORD0;
	}
	if(_VB.m_pTV[1] && _VB.m_nTVComp[1])
	{
		FmtBits |= FMT_TEXCOORD1;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD1;
		CompBits |= (_VB.m_nTVComp[1] - 1) << COMPBITS_POS_TEXCOORD1;
	}
	if(_VB.m_pTV[2] && _VB.m_nTVComp[2])
	{
		FmtBits |= FMT_TEXCOORD2;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD2;
		CompBits |= (_VB.m_nTVComp[2] - 1) << COMPBITS_POS_TEXCOORD2;
	}
	if(_VB.m_pTV[3] && _VB.m_nTVComp[3])
	{
		FmtBits |= FMT_TEXCOORD3;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD3;
		CompBits |= (_VB.m_nTVComp[3] - 1) << COMPBITS_POS_TEXCOORD3;
	}
	if(_VB.m_pTV[4] && _VB.m_nTVComp[4])
	{
		FmtBits |= FMT_TEXCOORD4;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD4;
		CompBits |= (_VB.m_nTVComp[4] - 1) << COMPBITS_POS_TEXCOORD4;
	}
	if(_VB.m_pTV[5] && _VB.m_nTVComp[5])
	{
		FmtBits |= FMT_TEXCOORD5;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD5;
		CompBits |= (_VB.m_nTVComp[5] - 1) << COMPBITS_POS_TEXCOORD5;
	}
	if(_VB.m_pTV[6] && _VB.m_nTVComp[6])
	{
		FmtBits |= FMT_TEXCOORD6;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD6;
		CompBits |= (_VB.m_nTVComp[6] - 1) << COMPBITS_POS_TEXCOORD6;
	}
	if(_VB.m_pTV[7] && _VB.m_nTVComp[7])
	{
		FmtBits |= FMT_TEXCOORD7;
		SizeBits |= 0 << SIZEBITS_POS_TEXCOORD7;
		CompBits |= (_VB.m_nTVComp[7] - 1) << COMPBITS_POS_TEXCOORD7;
	}

	_FmtBits = FmtBits;
	_SizeBits = SizeBits;
	_CompBits = CompBits;
}
*/
void CContext_Geometry::Disable()
{
	m_CurrentVBID = 0;
	gcmSetVertexDataArray(0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(2, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(3, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(4, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(5, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(6, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(7, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(8, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(9, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(10, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(11, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(12, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(13, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(14, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetVertexDataArray(15, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
	gcmSetInvalidateVertexCache();
}

static const fp32 M_ALIGN(16) DefaultScalers[] =
{
	1, 1, 1, 1, 0, 0, 0, 0,	// Pos
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex0
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex1
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex2
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex3
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex4
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex5
	1, 1, 1, 1, 0, 0, 0, 0,	// Tex6
	1, 1, 1, 1, 0, 0, 0, 0	// Tex7
};

bool CContext_Geometry::Bind(const CRC_VertexBuffer& _VB)
{
	Disable();

	uint32 nV = _VB.m_nV;
	if(nV == 0)
		return false;

	if(m_CurrentDefaultScalers != 18)
	{
		uint nCount = 18 - m_CurrentDefaultScalers;
		memcpy(m_lScalers, DefaultScalers, sizeof(CVec4Dfp32) * nCount);
		CRCPS3GCM::ms_This.m_ContextVP.SetConstants(11, nCount, ((CVec4Dfp32*)DefaultScalers));
		m_CurrentDefaultScalers = 18;
	}

	uint32 OffsetV = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pV)?m_Collector.Collect(_VB.m_pV, nV):gcmAddressToOffset(_VB.m_pV));
	uint32 OffsetN = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pN)?m_Collector.Collect(_VB.m_pN, nV):gcmAddressToOffset(_VB.m_pN));
	uint32 OffsetC0 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pCol)?m_Collector.Collect(_VB.m_pCol, nV):gcmAddressToOffset(_VB.m_pCol));
	uint32 OffsetC1 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pSpec)?m_Collector.Collect(_VB.m_pSpec, nV):gcmAddressToOffset(_VB.m_pSpec));
	uint32 OffsetMI1 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pMI)?m_Collector.Collect(_VB.m_pMI, nV * (_VB.m_nMWComp + 3) >> 2):gcmAddressToOffset(_VB.m_pMI));
	uint32 OffsetMI2 = (_VB.m_nMWComp>4)?(OffsetMI1 + sizeof(uint32) * nV):0;
	uint32 OffsetMW1 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pMW)?m_Collector.Collect(_VB.m_pMW, nV * _VB.m_nMWComp):gcmAddressToOffset(_VB.m_pMW));
	uint32 OffsetMW2 = (_VB.m_nMWComp>4)?(OffsetMW1 + sizeof(CVec4Dfp32) * nV):0;
	uint32 OffsetTV0 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[0])?m_Collector.Collect(_VB.m_pTV[0], nV * _VB.m_nTVComp[0]):gcmAddressToOffset(_VB.m_pTV[0]));
	uint32 OffsetTV1 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[1])?m_Collector.Collect(_VB.m_pTV[1], nV * _VB.m_nTVComp[1]):gcmAddressToOffset(_VB.m_pTV[1]));
	uint32 OffsetTV2 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[2])?m_Collector.Collect(_VB.m_pTV[2], nV * _VB.m_nTVComp[2]):gcmAddressToOffset(_VB.m_pTV[2]));
	uint32 OffsetTV3 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[3])?m_Collector.Collect(_VB.m_pTV[3], nV * _VB.m_nTVComp[3]):gcmAddressToOffset(_VB.m_pTV[3]));
	uint32 OffsetTV4 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[4])?m_Collector.Collect(_VB.m_pTV[4], nV * _VB.m_nTVComp[4]):gcmAddressToOffset(_VB.m_pTV[4]));
	uint32 OffsetTV5 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[5])?m_Collector.Collect(_VB.m_pTV[5], nV * _VB.m_nTVComp[5]):gcmAddressToOffset(_VB.m_pTV[5]));
	uint32 OffsetTV6 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[6])?m_Collector.Collect(_VB.m_pTV[6], nV * _VB.m_nTVComp[6]):gcmAddressToOffset(_VB.m_pTV[6]));
	uint32 OffsetTV7 = (CRenderPS3Resource_Collect_Main::NeedCollect(_VB.m_pTV[7])?m_Collector.Collect(_VB.m_pTV[7], nV * _VB.m_nTVComp[7]):gcmAddressToOffset(_VB.m_pTV[7]));
	
	uint32 nMW1 = Min(_VB.m_nMWComp, (uint32)4);
	uint32 nMW2 = _VB.m_nMWComp - nMW1;

	if(OffsetV == 0)
	{
		// Unable to collect position so ignore this bind completely
		return false;
	}

	gcmSetVertexDataArray(ATTRIB_POSITION, 0, sizeof(fp32) * 3, 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetV);
	if(OffsetN) gcmSetVertexDataArray(ATTRIB_NORMAL, 0, sizeof(fp32) * 3, 3, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetN);
	if(OffsetC0) gcmSetVertexDataArray(ATTRIB_COLOR0, 0, sizeof(uint32), 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_MAIN, OffsetC0);
	else
	{
		static fp32 DefaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		gcmSetVertexData4f(ATTRIB_COLOR0, DefaultColor);
	}
	if(OffsetC1) gcmSetVertexDataArray(ATTRIB_COLOR1, 0, sizeof(uint32), 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_MAIN, OffsetC1);
	if(OffsetMI1) gcmSetVertexDataArray(ATTRIB_BLENDINDICES1, 0, sizeof(uint32), 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_MAIN, OffsetMI1);
	if(OffsetMI2) gcmSetVertexDataArray(ATTRIB_BLENDINDICES2, 0, sizeof(uint32), 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_MAIN, OffsetMI2);
	if(OffsetMW1) gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT1, 0, sizeof(fp32) * nMW1, nMW1, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetMW1);
	if(OffsetMW2) gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT2, 0, sizeof(fp32) * nMW2, nMW2, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetMW2);
	if(OffsetTV0) gcmSetVertexDataArray(ATTRIB_TEXCOORD0, 0, sizeof(fp32) * _VB.m_nTVComp[0], _VB.m_nTVComp[0], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV0);
	if(OffsetTV1) gcmSetVertexDataArray(ATTRIB_TEXCOORD1, 0, sizeof(fp32) * _VB.m_nTVComp[1], _VB.m_nTVComp[1], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV1);
	if(OffsetTV2) gcmSetVertexDataArray(ATTRIB_TEXCOORD2, 0, sizeof(fp32) * _VB.m_nTVComp[2], _VB.m_nTVComp[2], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV2);
	if(OffsetTV3) gcmSetVertexDataArray(ATTRIB_TEXCOORD3, 0, sizeof(fp32) * _VB.m_nTVComp[3], _VB.m_nTVComp[3], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV3);
	if(OffsetTV4) gcmSetVertexDataArray(ATTRIB_TEXCOORD4, 0, sizeof(fp32) * _VB.m_nTVComp[4], _VB.m_nTVComp[4], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV4);
	if(OffsetTV5) gcmSetVertexDataArray(ATTRIB_TEXCOORD5, 0, sizeof(fp32) * _VB.m_nTVComp[5], _VB.m_nTVComp[5], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV5);
	if(OffsetTV6) gcmSetVertexDataArray(ATTRIB_TEXCOORD6, 0, sizeof(fp32) * _VB.m_nTVComp[6], _VB.m_nTVComp[6], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV6);
	if(OffsetTV7) gcmSetVertexDataArray(ATTRIB_TEXCOORD7, 0, sizeof(fp32) * _VB.m_nTVComp[7], _VB.m_nTVComp[7], CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_MAIN, OffsetTV7);
	gcmSetInvalidateVertexCache();

	m_CurrentMWComp = _VB.m_nMWComp;
	return true;
}

CRenderPS3Resource_VertexBuffer_Local* CContext_Geometry::GetVB(uint _VBID)
{
	//JK-NOTE: Theoretically possible that the VB is destroyed before Touch is called... always lock to be sure?
	CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
	CRenderPS3Resource_VertexBuffer_Local* pVB = m_lpVB[_VBID];
	if(!(IDInfo.m_Fresh & 1) || !pVB)
	{
		M_LOCK(m_Lock);
		if(m_CurrentVBID == _VBID)
			Disable();

		Build(_VBID);
		pVB = m_lpVB[_VBID];
		M_ASSERT(pVB, "!");
	}
	pVB->Touch();
	return pVB;
}

CRenderPS3Resource_VertexBuffer_Local* CContext_Geometry::Bind(uint32 _VBID)
{
	if(_VBID == 0)
	{
		Disable();
		return NULL;
	}
//	CMTime T;
//	TStart(T);

/*	{
		MTRACE_SCOPECOMMANDS("cellGcmSetNopCommand");
		cellGcmSetNopCommand(4);
	}*/

	// What if we're rebuilding the current VBID?
	if(_VBID == m_CurrentVBID)
		return m_lpVB[_VBID];

//	CRenderPS3Resource_VertexBuffer_Local* pVB = m_lpVB[_VBID];
//	M_ASSERT(pVB != NULL, "!");

	CRenderPS3Resource_VertexBuffer_Local* pVB = GetVB(_VBID);

	m_CurrentVBID = _VBID;

//	uint32 FmtBits = pVB->m_FmtBits;
	uint64 DataFmtBits = pVB->m_DataFmtBits;
	uint64 TexFmtBits = pVB->m_TexFmtBits;

	// No bits set means this is an indexbuffer, don't change the state (should be able to fail on just DataFmtBits since we always need a position)
	if(DataFmtBits || TexFmtBits)
	{

#ifdef GCM_COMMANDBUFFERWRITE
/*		uint32 CmdSize = (RSX_SETINVALIDATEVERTEXCACHE_SIZE + 8 + 2*16);

		uint32* pCmdBegin = RSX_AllocateCommandAlign16(CmdSize);
		uint32 M_RESTRICT* pCmdBuffer = pCmdBegin;

		vec128 vofsbase = M_VLd32(&pVB->m_LocalOffset);
		vec128 vofs8 = pVB->m_VOffset;
		vec128 vofs16_0 = M_VCnvL_u8_u16(vofs8);
		vec128 vofs16_1 = M_VCnvH_u8_u16(vofs8);
		vec128 vofs0 = M_VAdd_u32(M_VCnvL_u16_u32(vofs16_0), vofsbase);
		vec128 vofs1 = M_VAdd_u32(M_VCnvH_u16_u32(vofs16_0), vofsbase);
		vec128 vofs2 = M_VAdd_u32(M_VCnvL_u16_u32(vofs16_1), vofsbase);
		vec128 vofs3 = M_VAdd_u32(M_VCnvH_u16_u32(vofs16_1), vofsbase);

		vec128 vfmt16_0 = pVB->m_RSXFormat_v[0];
		vec128 vfmt16_1 = pVB->m_RSXFormat_v[1];
		vec128 vfmt0 = M_VCnvL_u16_u32(vfmt16_0);
		vec128 vfmt1 = M_VCnvH_u16_u32(vfmt16_0);
		vec128 vfmt2 = M_VCnvL_u16_u32(vfmt16_1);
		vec128 vfmt3 = M_VCnvH_u16_u32(vfmt16_1);

		vec128 vfmtcmd = M_VConst_u32(0x00040000, 0x00040000, 0x00040000, ((16*4) << 16) + 0x1740);
		vec128 vofscmd = M_VConst_u32(0x00040000, 0x00040000, 0x00040000, ((16*4) << 16) + 0x1680);

		vec128 M_RESTRICT* pCmdV = (vec128 M_RESTRICT*)pCmdBuffer;
		pCmdV[0] = vfmtcmd;
		pCmdV[1] = vfmt0;
		pCmdV[2] = vfmt1;
		pCmdV[3] = vfmt2;
		pCmdV[4] = vfmt3;
		pCmdV[5] = vofscmd;
		pCmdV[6] = vofs0;
		pCmdV[7] = vofs1;
		pCmdV[8] = vofs2;
		pCmdV[9] = vofs3;

		pCmdBuffer += (8 + 2*16);
*/

		uint32 CmdSize = (RSXCMD_INVALIDATEVERTEXCACHE_SIZE + (RSX_SETATTRIBFORMAT_SIZE + RSX_SETATTRIBOFFSET_SIZE)*16);

		uint32* pCmdBegin = RSX_AllocateCommandAlign16(CmdSize);
		uint32 M_RESTRICT* pCmdBuffer = pCmdBegin;

		vec128 vofsbase = M_VLd32(&pVB->m_LocalOffset);
		vec128 vofs8 = pVB->m_VOffset;
		vec128 vofs16_0 = M_VCnvL_u8_u16(vofs8);
		vec128 vofs16_1 = M_VCnvH_u8_u16(vofs8);
		vec128 vofs0 = M_VAdd_u32(M_VCnvL_u16_u32(vofs16_0), vofsbase);
		vec128 vofs1 = M_VAdd_u32(M_VCnvH_u16_u32(vofs16_0), vofsbase);
		vec128 vofs2 = M_VAdd_u32(M_VCnvL_u16_u32(vofs16_1), vofsbase);
		vec128 vofs3 = M_VAdd_u32(M_VCnvH_u16_u32(vofs16_1), vofsbase);

		vec128 vfmt16_0 = pVB->m_RSXFormat_v[0];
		vec128 vfmt16_1 = pVB->m_RSXFormat_v[1];
		vec128 vfmt0 = M_VCnvL_u16_u32(vfmt16_0);
		vec128 vfmt1 = M_VCnvH_u16_u32(vfmt16_0);
		vec128 vfmt2 = M_VCnvL_u16_u32(vfmt16_1);
		vec128 vfmt3 = M_VCnvH_u16_u32(vfmt16_1);

		vec128 vfmtcmd0 = M_VConst_u32(RSXCMD_ATTRIBFORMAT0 + 0x00, RSXCMD_ATTRIBFORMAT0 + 0x04, RSXCMD_ATTRIBFORMAT0 + 0x08, RSXCMD_ATTRIBFORMAT0 + 0x0c);
		vec128 vfmtcmd1 = M_VConst_u32(RSXCMD_ATTRIBFORMAT0 + 0x10, RSXCMD_ATTRIBFORMAT0 + 0x14, RSXCMD_ATTRIBFORMAT0 + 0x18, RSXCMD_ATTRIBFORMAT0 + 0x1c);
		vec128 vfmtcmd2 = M_VConst_u32(RSXCMD_ATTRIBFORMAT0 + 0x20, RSXCMD_ATTRIBFORMAT0 + 0x24, RSXCMD_ATTRIBFORMAT0 + 0x28, RSXCMD_ATTRIBFORMAT0 + 0x2c);
		vec128 vfmtcmd3 = M_VConst_u32(RSXCMD_ATTRIBFORMAT0 + 0x30, RSXCMD_ATTRIBFORMAT0 + 0x34, RSXCMD_ATTRIBFORMAT0 + 0x38, RSXCMD_ATTRIBFORMAT0 + 0x3c);
		vec128 vofscmd0 = M_VConst_u32(RSXCMD_ATTRIBOFFSET0 + 0x00, RSXCMD_ATTRIBOFFSET0 + 0x04, RSXCMD_ATTRIBOFFSET0 + 0x08, RSXCMD_ATTRIBOFFSET0 + 0x0c);
		vec128 vofscmd1 = M_VConst_u32(RSXCMD_ATTRIBOFFSET0 + 0x10, RSXCMD_ATTRIBOFFSET0 + 0x14, RSXCMD_ATTRIBOFFSET0 + 0x18, RSXCMD_ATTRIBOFFSET0 + 0x1c);
		vec128 vofscmd2 = M_VConst_u32(RSXCMD_ATTRIBOFFSET0 + 0x20, RSXCMD_ATTRIBOFFSET0 + 0x24, RSXCMD_ATTRIBOFFSET0 + 0x28, RSXCMD_ATTRIBOFFSET0 + 0x2c);
		vec128 vofscmd3 = M_VConst_u32(RSXCMD_ATTRIBOFFSET0 + 0x30, RSXCMD_ATTRIBOFFSET0 + 0x34, RSXCMD_ATTRIBOFFSET0 + 0x38, RSXCMD_ATTRIBOFFSET0 + 0x3c);

		M_VTranspose4x4(vfmtcmd0, vfmt0, vofscmd0, vofs0);
		M_VTranspose4x4(vfmtcmd1, vfmt1, vofscmd1, vofs1);
		M_VTranspose4x4(vfmtcmd2, vfmt2, vofscmd2, vofs2);
		M_VTranspose4x4(vfmtcmd3, vfmt3, vofscmd3, vofs3);

		vec128 M_RESTRICT* pCmdV = (vec128 M_RESTRICT*)pCmdBuffer;
		pCmdV[0] = vfmtcmd0;	pCmdV[1] = vfmt0;	pCmdV[2] = vofscmd0;	pCmdV[3] = vofs0;
		pCmdV[4] = vfmtcmd1;	pCmdV[5] = vfmt1;	pCmdV[6] = vofscmd1;	pCmdV[7] = vofs1;
		pCmdV[8] = vfmtcmd2;	pCmdV[9] = vfmt2;	pCmdV[10] = vofscmd2;	pCmdV[11] = vofs2;
		pCmdV[12] = vfmtcmd3;	pCmdV[13] = vfmt3;	pCmdV[14] = vofscmd3;	pCmdV[15] = vofs3;
		pCmdBuffer += (4*16);

/*
		RSX_SETATTRIBFORMAT(pCmdBuffer+0, 0, pVB->m_RSXFormat[0]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+2, 0, pVB->m_RSXOffset[0]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+4, 1, pVB->m_RSXFormat[1]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+6, 1, pVB->m_RSXOffset[1]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+8, 2, pVB->m_RSXFormat[2]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+10, 2, pVB->m_RSXOffset[2]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+12, 3, pVB->m_RSXFormat[3]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+14, 3, pVB->m_RSXOffset[3]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+16, 4, pVB->m_RSXFormat[4]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+18, 4, pVB->m_RSXOffset[4]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+20, 5, pVB->m_RSXFormat[5]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+22, 5, pVB->m_RSXOffset[5]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+24, 6, pVB->m_RSXFormat[6]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+26, 6, pVB->m_RSXOffset[6]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+28, 7, pVB->m_RSXFormat[7]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+30, 7, pVB->m_RSXOffset[7]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+32, 8, pVB->m_RSXFormat[8]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+34, 8, pVB->m_RSXOffset[8]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+36, 9, pVB->m_RSXFormat[9]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+38, 9, pVB->m_RSXOffset[9]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+40, 10, pVB->m_RSXFormat[10]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+42, 10, pVB->m_RSXOffset[10]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+44, 11, pVB->m_RSXFormat[11]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+46, 11, pVB->m_RSXOffset[11]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+48, 12, pVB->m_RSXFormat[12]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+50, 12, pVB->m_RSXOffset[12]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+52, 13, pVB->m_RSXFormat[13]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+54, 13, pVB->m_RSXOffset[13]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+56, 14, pVB->m_RSXFormat[14]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+58, 14, pVB->m_RSXOffset[14]);
		RSX_SETATTRIBFORMAT(pCmdBuffer+60, 15, pVB->m_RSXFormat[15]);
		RSX_SETATTRIBOFFSET(pCmdBuffer+62, 15, pVB->m_RSXOffset[15]);*/

		RSX_InvalidateVertexCache(pCmdBuffer);
//		RSX_SETINVALIDATEVERTEXCACHE(pCmdBuffer);
//		pCmdBuffer += RSX_SETINVALIDATEVERTEXCACHE_SIZE;

		M_ASSERT((pCmdBuffer - pCmdBegin) == CmdSize, "!");

//		RSX_EndCommand(CmdSize);


#else
		uint64 DataCompBits = pVB->m_DataCompBits;
		uint64 TexCompBits = pVB->m_TexCompBits;
		uint32 Stride = pVB->m_Stride;
		static fp32 DefaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

		if(DataFmtBits & DATAFMTBITS_MASK_POSITION)
			gcmSetVertexDataArray(ATTRIB_POSITION, 0, Stride, 1 + ((DataCompBits & DATACOMPBITS_MASK_POSITION) >> DATACOMPBITS_POS_POSITION), (DataFmtBits & DATAFMTBITS_MASK_POSITION) >> DATAFMTBITS_POS_POSITION, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_POSITION));


		if(DataFmtBits & DATAFMTBITS_MASK_NORMAL)
			gcmSetVertexDataArray(ATTRIB_NORMAL, 0, Stride, 1 + ((DataCompBits & DATACOMPBITS_MASK_NORMAL) >> DATACOMPBITS_POS_NORMAL), (DataFmtBits & DATAFMTBITS_MASK_NORMAL) >> DATAFMTBITS_POS_NORMAL, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_NORMAL));
		else
			gcmSetVertexDataArray(ATTRIB_NORMAL, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(DataFmtBits & DATAFMTBITS_MASK_COLOR0)
			gcmSetVertexDataArray(ATTRIB_COLOR0, 0, Stride, 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_COLOR0));
		else
		{
			gcmSetVertexDataArray(ATTRIB_COLOR0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);
			gcmSetVertexData4f(ATTRIB_COLOR0, DefaultColor);
		}

		if(DataFmtBits & DATAFMTBITS_MASK_COLOR1)
			gcmSetVertexDataArray(ATTRIB_COLOR1, 0, Stride, 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_COLOR1));
		else
			gcmSetVertexDataArray(ATTRIB_COLOR1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(DataFmtBits & DATAFMTBITS_MASK_BLENDWEIGHT1)
			gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT1, 0, Stride, 1 + ((DataCompBits & DATACOMPBITS_MASK_BLENDWEIGHT1) >> DATACOMPBITS_POS_BLENDWEIGHT1), (DataFmtBits & DATAFMTBITS_MASK_BLENDWEIGHT1) >> DATAFMTBITS_POS_BLENDWEIGHT1, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_BLENDWEIGHT1));
		else
			gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(DataFmtBits & DATAFMTBITS_MASK_BLENDWEIGHT2)
			gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT2, 0, Stride, 1 + ((DataCompBits & DATACOMPBITS_MASK_BLENDWEIGHT2) >> DATACOMPBITS_POS_BLENDWEIGHT2), (DataFmtBits & DATAFMTBITS_MASK_BLENDWEIGHT2) >> DATAFMTBITS_POS_BLENDWEIGHT2, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_BLENDWEIGHT2));
		else
			gcmSetVertexDataArray(ATTRIB_BLENDWEIGHT2, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(DataFmtBits & DATAFMTBITS_MASK_BLENDINDICES1)
			gcmSetVertexDataArray(ATTRIB_BLENDINDICES1, 0, Stride, 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_BLENDINDICES1));
		else
			gcmSetVertexDataArray(ATTRIB_BLENDINDICES1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(DataFmtBits & DATAFMTBITS_MASK_BLENDINDICES2)
			gcmSetVertexDataArray(ATTRIB_BLENDINDICES2, 0, Stride, 4, CELL_GCM_VERTEX_UB, CELL_GCM_LOCATION_LOCAL, pVB->GetDataAttribOffset(ATTRIB_BLENDINDICES2));
		else
			gcmSetVertexDataArray(ATTRIB_BLENDINDICES2, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);



		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD0)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD0, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD0) >> TEXCOMPBITS_POS_TEXCOORD0), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD0) >> TEXFMTBITS_POS_TEXCOORD0, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD0));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD0, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD1)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD1, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD1) >> TEXCOMPBITS_POS_TEXCOORD1), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD1) >> TEXFMTBITS_POS_TEXCOORD1, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD1));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD1, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD2)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD2, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD2) >> TEXCOMPBITS_POS_TEXCOORD2), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD2) >> TEXFMTBITS_POS_TEXCOORD2, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD2));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD2, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD3)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD3, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD3) >> TEXCOMPBITS_POS_TEXCOORD3), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD3) >> TEXFMTBITS_POS_TEXCOORD3, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD3));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD3, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD4)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD4, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD4) >> TEXCOMPBITS_POS_TEXCOORD4), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD4) >> TEXFMTBITS_POS_TEXCOORD4, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD4));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD4, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD5)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD5, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD5) >> TEXCOMPBITS_POS_TEXCOORD5), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD5) >> TEXFMTBITS_POS_TEXCOORD5, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD5));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD5, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD6)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD6, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD6) >> TEXCOMPBITS_POS_TEXCOORD6), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD6) >> TEXFMTBITS_POS_TEXCOORD6, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD6));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD6, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		if(TexFmtBits & TEXFMTBITS_MASK_TEXCOORD7)
			gcmSetVertexDataArray(ATTRIB_TEXCOORD7, 0, Stride, 1 + ((TexCompBits & TEXCOMPBITS_MASK_TEXCOORD7) >> TEXCOMPBITS_POS_TEXCOORD7), (TexFmtBits & TEXFMTBITS_MASK_TEXCOORD7) >> TEXFMTBITS_POS_TEXCOORD7, CELL_GCM_LOCATION_LOCAL, pVB->GetTexAttribOffset(ATTRIB_TEXCOORD7));
		else
			gcmSetVertexDataArray(ATTRIB_TEXCOORD7, 0, 0, 0, CELL_GCM_VERTEX_F, CELL_GCM_LOCATION_LOCAL, 0);

		{
			MTRACE_SCOPECOMMANDS("gcmSetInvalidateVertexCache");
			gcmSetInvalidateVertexCache();
		}
#endif // GCM_COMMANDBUFFERWRITE

#ifdef _DEBUG
/*		for(int i = 0; i < 16; i++)
		{
			M_TRACEALWAYS("Attr %d: %.8x, %.8x, %.8x, %.8x\n", i, pCmdBegin[0],pCmdBegin[1],pCmdBegin[2],pCmdBegin[3]);
			pCmdBegin += 4;
		}*/
#endif

		uint nMWComp = 0;
		switch(DataFmtBits & (DATAFMTBITS_MASK_BLENDINDICES1 | DATAFMTBITS_MASK_BLENDINDICES2))
		{
		case DATAFMTBITS_MASK_BLENDINDICES1: nMWComp = 1 + ((DataCompBits & DATACOMPBITS_MASK_BLENDWEIGHT1) >> DATACOMPBITS_POS_BLENDWEIGHT1); break;
		case DATAFMTBITS_MASK_BLENDINDICES1 | DATAFMTBITS_MASK_BLENDINDICES2: nMWComp = 5 + ((DataCompBits & DATACOMPBITS_MASK_BLENDWEIGHT2) >> DATACOMPBITS_POS_BLENDWEIGHT2); break;
		}
		m_CurrentMWComp = nMWComp;
	}

	if(pVB->m_nTransform > 0)
	{
		uint nCount = pVB->m_nTransform << 1;
		memcpy(m_lScalers, pVB->m_pTransform, nCount * 4 * sizeof(fp32));

		// Restore remaining scalers
		int nLeft = 18 - m_CurrentDefaultScalers - nCount;
		if(nLeft > 0)
		{
			memcpy(m_lScalers + nCount, DefaultScalers, nLeft * sizeof(CVec4Dfp32));
			nCount += nLeft;
		}

		CRCPS3GCM::ms_This.m_ContextVP.SetConstants(11, nCount, m_lScalers);

		m_CurrentDefaultScalers = 18 - nCount;
	}
	else if(m_CurrentDefaultScalers != 18)
	{
		uint nCount = 18 - m_CurrentDefaultScalers;
		memcpy(m_lScalers, DefaultScalers, sizeof(CVec4Dfp32) * nCount);
		CRCPS3GCM::ms_This.m_ContextVP.SetConstants(11, nCount, ((CVec4Dfp32*)DefaultScalers));
		m_CurrentDefaultScalers = 18;
	}

//	TStop(T);
//	M_TRACEALWAYS("T = %f ms \n", 1000.0f*T.GetTime());
	return pVB;
}

static aint gs_SupportedFormatArray[CRC_VREGFMT_MAX] = {
	CRC_VREGFMT_VOID,
	CRC_VREGFMT_V1_F32,	
	CRC_VREGFMT_V2_F32,	
	CRC_VREGFMT_V3_F32,
	CRC_VREGFMT_V4_F32,
	CRC_VREGFMT_V1_I16,
	CRC_VREGFMT_V2_I16,			
	CRC_VREGFMT_V3_I16,
	CRC_VREGFMT_V4_I16,			
	CRC_VREGFMT_V1_F32,		// CRC_VREGFMT_V1_I16,		// CRC_VREGFMT_V1_U16,
	CRC_VREGFMT_V2_F32,		// CRC_VREGFMT_V2_I16,		// CRC_VREGFMT_V2_U16,			
	CRC_VREGFMT_V3_F32,		// CRC_VREGFMT_V3_I16,		// CRC_VREGFMT_V3_U16,
	CRC_VREGFMT_V4_F32,		// CRC_VREGFMT_V4_I16,		// CRC_VREGFMT_V4_U16,			
	CRC_VREGFMT_NS3_I16,
	CRC_VREGFMT_NS3_P32,
	CRC_VREGFMT_V3_F32,		// CRC_VREGFMT_NS3_P32,	// CRC_VREGFMT_NU3_P32,		
	CRC_VREGFMT_N4_UI8_P32_NORM,
	CRC_VREGFMT_N4_UI8_P32,		
	CRC_VREGFMT_N4_UI8_P32_NORM,	// CRC_VREGFMT_N4_COL,			
	CRC_VREGFMT_V3_F32,		// CRC_VREGFMT_V3_I16,		// CRC_VREGFMT_U3_P32,

	CRC_VREGFMT_NS1_I16,
	CRC_VREGFMT_NS2_I16,
	CRC_VREGFMT_NS4_I16,

	CRC_VREGFMT_V1_F32,		// CRC_VREGFMT_NS1_I16,	// CRC_VREGFMT_NU1_I16
	CRC_VREGFMT_V2_F32,		// CRC_VREGFMT_NS2_I16,	// CRC_VREGFMT_NU2_I16
	CRC_VREGFMT_V3_F32,		// CRC_VREGFMT_NS3_I16,	// CRC_VREGFMT_NU3_I16
	CRC_VREGFMT_V4_F32,		// CRC_VREGFMT_NS4_I16,	// CRC_VREGFMT_NU4_I16
};

static aint gf_GetSupportedFormat(aint _Format)
{
	M_ASSERT(_Format >= 0 && _Format < CRC_VREGFMT_MAX, "Out of bounds");
	M_ASSERT(gs_SupportedFormatArray[_Format], "Unsupported format");
	return gs_SupportedFormatArray[_Format];
}

// _d = _a * _b + _c
static void M_FORCEINLINE Combine(const CVec4Dfp32& _a, const CVec4Dfp32& _b, const CVec4Dfp32& _c, CVec4Dfp32& _d)
{
	_a.CompMul(_b, _d);
	_d += _c;
}

void CContext_Geometry::Build(uint32 _VBID)
{
	// Make sure it isn't created already
	Destroy(_VBID);
	if(!m_lpVB[_VBID])
		m_lpVB[_VBID]	= DNew(CRenderPS3Resource_VertexBuffer_Local) CRenderPS3Resource_VertexBuffer_Local;

	CRenderPS3Resource_VertexBuffer_Local* pVB = m_lpVB[_VBID];
	pVB->Clear();

	CRC_BuildVertexBuffer VBB;
	VBB.Clear();
	m_pVBC->VB_Get(_VBID, VBB, VB_GETFLAGS_BUILD);

	{
		CRC_VertexFormat DestinationFormat = VBB.m_WantFormat;
		CRC_VRegTransform RenderScale[CRC_MAXVERTEXREGSCALE];
		CRC_VRegTransform BuildScale[CRC_MAXVERTEXREGSCALE];
		uint32 TransformEnable = VBB.m_WantTransformEnable;
		for (int i = 0; i < CRC_MAXVERTEXREGSCALE; ++i)
		{
			RenderScale[i] = VBB.m_lWantTransform[i];
			BuildScale[i] = VBB.m_lWantTransform[i];
		}

		for (int i = 0; i < CRC_MAXVERTEXREG; ++i)
		{
			aint DstFormat = DestinationFormat.GetFormat(i);
			if(DstFormat == CRC_VREGFMT_VOID)
				continue;

			switch(DstFormat)
			{
			default:
				{
					aint Format = gf_GetSupportedFormat(DstFormat);

//					if((CRC_VertexFormat::GetRegisterSize(Format) - CRC_VertexFormat::GetRegisterSize(DstFormat)) >= CRC_VertexFormat::GetRegisterSize(DstFormat))
					if(Format != DstFormat)
					{
						M_TRACEALWAYS("  VBID %d, VREG %d, WantedFmt %d (%s), TranslatedFmt %d (%s)\r\n", _VBID, i, DstFormat, CRC_VertexFormat::GetRegisterName(DstFormat), Format, CRC_VertexFormat::GetRegisterName(Format));
					}

					DestinationFormat.SetFormat(i, Format);
					if (Format >= CRC_VREGFMT_V1_F32 && Format <= CRC_VREGFMT_V4_F32)
						TransformEnable = TransformEnable & (~(1 << i));
					break;
				}

			case -1:
				{
					break;
				}
			}
		}

		aint Stride = DestinationFormat.GetStride();
		pVB->m_Stride = Stride;
		aint Size = Stride * VBB.m_nV;


		int nTransform = 0;
		for (int i = 0; i < CRC_MAXVERTEXREGSCALE; ++i)
		{
			if (TransformEnable & (1 << i))
			{
				nTransform = i + 1;
			}
		}

		if (nTransform)
		{
			pVB->m_pTransform = DNew(CRC_VRegTransform) CRC_VRegTransform[nTransform];
			pVB->m_nTransform = nTransform;
			int iTransform = 0;
			for (int i = 0; i < nTransform; ++i)
			{
				if (TransformEnable & (1 << i))
					pVB->m_pTransform[i] = RenderScale[i];
				else
				{
					pVB->m_pTransform[i].m_Scale = CVec4Dfp32(1.0f);
					pVB->m_pTransform[i].m_Offset = CVec4Dfp32(0.0f);
				}
			}
		}

		pVB->CreateVertexDeclaration(DestinationFormat);

		uint32 nPrim = 0;
		uint32 nTriCount = 0;
		if(VBB.m_piPrim)
		{
			if(VBB.m_PrimType == CRC_RIP_STREAM)
			{
				int PrimType;
				bool bConvert = true;
				if((PrimType = IsSinglePrimType(VBB.m_piPrim, VBB.m_nPrim)))
				{
					uint32 PrimCount = 0, IndexCount = 0;
					GetPrimStats(VBB.m_piPrim, VBB.m_nPrim, PrimCount, IndexCount);
					if(PrimType == CRC_RIP_TRIANGLES)
					{
						// Merge all triangles into single primlist
						nPrim = IndexCount;
						bConvert = false;
						nTriCount = nPrim / 3;
					}
					else
					{
						// Merge all strips into single strip
						nPrim = IndexCount + PrimCount - 1;	// Need a restart token for every new prim after first one
						nTriCount = (IndexCount - PrimCount * 2);
						bConvert = false;
					}
				}

				if(bConvert)
				{
					// Convert everything to triangles
					CRCPrimStreamIterator Iter(VBB.m_piPrim, VBB.m_nPrim);
					nPrim = CRC_Core::Geometry_BuildTriangleListFromPrimitivesCount(Iter);
					nTriCount = nPrim / 3;
				}
			}
			else
			{
				if(VBB.m_PrimType == CRC_RIP_TRIANGLES)
				{
					nPrim = VBB.m_nPrim * 3;
					nTriCount = VBB.m_nPrim;
				}
				else
				{
					M_BREAKPOINT;
				}
			}
		}
		pVB->Create(Stride * VBB.m_nV + sizeof(uint16) * nPrim);
		pVB->m_nV = VBB.m_nV;

		VBB.ConvertToInterleaved(pVB->GetMemory(), DestinationFormat, BuildScale, TransformEnable, VBB.m_nV);

		uint16* piPrim = VBB.m_piPrim;
		if(piPrim)
		{
			uint8* pMem = ((uint8*)pVB->GetMemory()) + Stride * VBB.m_nV;
			TThinArray<uint16> liPrim;
			uint32 PrimType = CELL_GCM_PRIMITIVE_TRIANGLES;
			if(VBB.m_PrimType == CRC_RIP_STREAM)
			{
				if(IsSinglePrimType(VBB.m_piPrim, VBB.m_nPrim))
				{
					PrimType = AssemblePrimStream(liPrim, VBB.m_piPrim, VBB.m_nPrim);
					switch(PrimType)
					{
					case CRC_RIP_TRIANGLES: PrimType = CELL_GCM_PRIMITIVE_TRIANGLES; break;
					case CRC_RIP_TRISTRIP: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP; break;
					case CRC_RIP_TRIFAN: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_FAN; break;
					case CRC_RIP_WIRES: PrimType = CELL_GCM_PRIMITIVE_LINES; break;
					}
				}
				else
				{
					// Convert to triangles
					liPrim.Clear();

					uint16 lTriIndices[1024*3];
					
					CRCPrimStreamIterator StreamIterate(VBB.m_piPrim, VBB.m_nPrim);
					
					while(StreamIterate.IsValid())
					{
						int nTriIndices = 1024*3;
						bool bDone = CRC_Core::Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
						if (nTriIndices)
						{
							int iDst = liPrim.Len();
							liPrim.SetLen(iDst + nTriIndices);
							memcpy(&liPrim[iDst], lTriIndices, nTriIndices*2);
		//					Internal_VAIndxTriangles(lTriIndices, nTriIndices/3, 0);
						}
						
						if (bDone) break;
					}

					PrimType = CELL_GCM_PRIMITIVE_TRIANGLES;
				}
			}
			else
			{
				switch(VBB.m_PrimType)
				{
				case CRC_RIP_TRIANGLES: PrimType = CELL_GCM_PRIMITIVE_TRIANGLES; break;
				case CRC_RIP_TRISTRIP: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP; break;
				case CRC_RIP_TRIFAN: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_FAN; break;
				case CRC_RIP_WIRES: PrimType = CELL_GCM_PRIMITIVE_LINES; break;
				}
				liPrim.SetLen((VBB.m_PrimType == CRC_RIP_TRIANGLES) ? VBB.m_nPrim*3 : VBB.m_nPrim);
				memcpy(liPrim.GetBasePtr(), VBB.m_piPrim, liPrim.ListSize());
			}
			pVB->m_nTri = nTriCount;
			pVB->m_PrimType = PrimType;
			pVB->m_PrimOffset = pVB->GetOffset() + (uint32)((uint8*)pMem - (uint8*)pVB->GetMemory());
			pVB->m_PrimLen = liPrim.Len();
			memcpy(pMem, liPrim.GetBasePtr(), sizeof(uint16) * pVB->m_PrimLen);

			pMem += sizeof(uint16) * pVB->m_PrimLen;
		}
	}

/*
	CRC_VertexBuffer VB;
	TThinArray<uint8> Temp;
	Temp.SetLen(VBB.CRC_VertexBuffer_GetSize());
	VBB.CRC_VertexBuffer_ConvertTo(Temp.GetBasePtr(), VB);

	uint32 FmtBits = 0, SizeBits = 0, CompBits = 0;
	GetAllBits(VB, FmtBits, SizeBits, CompBits);
	pVB->SetFormat(FmtBits, SizeBits, CompBits);

	uint32 nPrim = 0;
	uint32 nTriCount = 0;
	if(VB.m_piPrim)
	{
		if(VB.m_PrimType == CRC_RIP_STREAM)
		{
			int PrimType;
			bool bConvert = true;
			if((PrimType = IsSinglePrimType(VB.m_piPrim, VB.m_nPrim)))
			{
				uint32 PrimCount = 0, IndexCount = 0;
				GetPrimStats(VB.m_piPrim, VB.m_nPrim, PrimCount, IndexCount);
				if(PrimType == CRC_RIP_TRIANGLES)
				{
					// Merge all triangles into single primlist
					nPrim = IndexCount;
					bConvert = false;
					nTriCount = nPrim / 3;
				}
				else
				{
					// Merge all strips into single strip
					nPrim = IndexCount + PrimCount - 1;	// Need a restart token for every new prim after first one
					nTriCount = (IndexCount - PrimCount * 2);
					bConvert = false;
				}
			}

			if(bConvert)
			{
				// Convert everything to triangles
				CRCPrimStreamIterator Iter(VB.m_piPrim, VB.m_nPrim);
				nPrim = CRC_Core::Geometry_BuildTriangleListFromPrimitivesCount(Iter);
				nTriCount = nPrim / 3;
			}
		}
		else
		{
			if(VB.m_PrimType == CRC_RIP_TRIANGLES)
			{
				nPrim = VB.m_nPrim * 3;
				nTriCount = VB.m_nPrim;
			}
			else
			{
				M_BREAKPOINT;
			}
		}
	}

	pVB->Create(pVB->m_Stride * VB.m_nV + sizeof(uint16) * nPrim);
	pVB->m_nV = VB.m_nV;

	uint8* pMem = (uint8*)pVB->GetMemory();
	uint8* pStartMem = pMem;
	uint8* pTargetMem = pMem + pVB->m_Stride * VB.m_nV + sizeof(uint16) * nPrim;
	if(VB.m_nV)
	{
		CVec3Dfp32* pV = VB.m_pV;
		CVec3Dfp32* pN = VB.m_pN;
		uint32* pC0 = (uint32*)VB.m_pCol;
		uint32* pC1 = (uint32*)VB.m_pSpec;
		uint32* pMI0 = VB.m_pMI;
		uint32* pMI1 = (VB.m_nMWComp > 4)?(VB.m_pMI + VB.m_nV):NULL;
		fp32* pMW0 = VB.m_pMW;
		fp32* pMW1 = (VB.m_nMWComp > 4)?(VB.m_pMW + VB.m_nV * 4):NULL;
		fp32* pTC[8];
		uint32 nTC[8];
		for(uint i = 0; i < 8; i++)
		{
			pTC[i] = VB.m_pTV[i];
			nTC[i] = VB.m_nTVComp[i];
		}

		for(uint iV = 0; iV < VB.m_nV; iV++)
		{
			if(pV) {*((CVec3Dfp32*)pMem) = *pV++; pMem += sizeof(CVec3Dfp32);}
			if(pN) {*((CVec3Dfp32*)pMem) = *pN++; pMem += sizeof(CVec3Dfp32);}
			if(pC0) {*((uint32*)pMem) = *pC0++; pMem += sizeof(uint32);}
			if(pC1) {*((uint32*)pMem) = *pC1++; pMem += sizeof(uint32);}
			if(pMW0) {uint nComp = (VB.m_nMWComp>4)?4:VB.m_nMWComp; for(uint i = 0; i < nComp; i++) {*((fp32*)pMem) = *pMW0++; pMem += sizeof(fp32);}}
			if(pMW1) {uint nComp = VB.m_nMWComp - 4; for(uint i = 0; i < nComp; i++) {*((fp32*)pMem) = *pMW1++; pMem += sizeof(fp32);}}
			if(pMI0) {*((uint32*)pMem) = *pMI0++; ::SwapLE((uint32&)*pMem); pMem += sizeof(uint32);}
			if(pMI1) {*((uint32*)pMem) = *pMI1++; ::SwapLE((uint32&)*pMem); pMem += sizeof(uint32);}
			for(uint iT = 0; iT < 8; iT++)
			{
				if(pTC[iT])
				{
					uint nComp = nTC[iT];
					for(uint i = 0; i < nComp; i++)
					{
						*((fp32*)pMem)	= *(pTC[iT])++;
						pMem	+= sizeof(fp32);
					}
				}
			}
		}
	}

	uint16* piPrim = VB.m_piPrim;
	if(piPrim)
	{
		TThinArray<uint16> liPrim;
		uint32 PrimType = CELL_GCM_PRIMITIVE_TRIANGLES;
		if(VB.m_PrimType == CRC_RIP_STREAM)
		{
			if(IsSinglePrimType(VB.m_piPrim, VB.m_nPrim))
			{
				PrimType = AssemblePrimStream(liPrim, VB.m_piPrim, VB.m_nPrim);
				switch(PrimType)
				{
				case CRC_RIP_TRIANGLES: PrimType = CELL_GCM_PRIMITIVE_TRIANGLES; break;
				case CRC_RIP_TRISTRIP: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP; break;
				case CRC_RIP_TRIFAN: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_FAN; break;
				case CRC_RIP_WIRES: PrimType = CELL_GCM_PRIMITIVE_LINES; break;
				}
			}
			else
			{
				// Convert to triangles
				liPrim.Clear();

				uint16 lTriIndices[1024*3];
				
				CRCPrimStreamIterator StreamIterate(VB.m_piPrim, VB.m_nPrim);
				
				while(StreamIterate.IsValid())
				{
					int nTriIndices = 1024*3;
					bool bDone = CRC_Core::Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
					if (nTriIndices)
					{
						int iDst = liPrim.Len();
						liPrim.SetLen(iDst + nTriIndices);
						memcpy(&liPrim[iDst], lTriIndices, nTriIndices*2);
	//					Internal_VAIndxTriangles(lTriIndices, nTriIndices/3, 0);
					}
					
					if (bDone) break;
				}

				PrimType = CELL_GCM_PRIMITIVE_TRIANGLES;
			}
		}
		else
		{
			switch(VB.m_PrimType)
			{
			case CRC_RIP_TRIANGLES: PrimType = CELL_GCM_PRIMITIVE_TRIANGLES; break;
			case CRC_RIP_TRISTRIP: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_STRIP; break;
			case CRC_RIP_TRIFAN: PrimType = CELL_GCM_PRIMITIVE_TRIANGLE_FAN; break;
			case CRC_RIP_WIRES: PrimType = CELL_GCM_PRIMITIVE_LINES; break;
			}
			liPrim.SetLen((VB.m_PrimType == CRC_RIP_TRIANGLES) ? VB.m_nPrim*3 : VB.m_nPrim);
			memcpy(liPrim.GetBasePtr(), VB.m_piPrim, liPrim.ListSize());
		}
		pVB->m_nTri = nTriCount;
		pVB->m_PrimType = PrimType;
		pVB->m_PrimOffset = pVB->GetOffset() + (uint32)((uint8*)pMem - (uint8*)pVB->GetMemory());
		pVB->m_PrimLen = liPrim.Len();
		memcpy(pMem, liPrim.GetBasePtr(), sizeof(uint16) * pVB->m_PrimLen);

		pMem += sizeof(uint16) * pVB->m_PrimLen;
	}
	{
		int FmtBits = pVB->m_FmtBits;
		int Stride = pVB->m_Stride;

		pVB->m_RSXFormat[ATTRIB_POSITION] = RSX_MAKEATTRIBFORMAT(Stride, 3, CELL_GCM_VERTEX_F);
		pVB->m_RSXFormat[ATTRIB_NORMAL] = (FmtBits & FMT_NORMAL) ? RSX_MAKEATTRIBFORMAT(Stride, 3, CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_COLOR0] = (FmtBits & FMT_COLOR0) ? RSX_MAKEATTRIBFORMAT(Stride, 4, CELL_GCM_VERTEX_UB) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_COLOR1] = (FmtBits & FMT_COLOR1) ? RSX_MAKEATTRIBFORMAT(Stride, 4, CELL_GCM_VERTEX_UB) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_BLENDWEIGHT1] = (FmtBits & FMT_BLENDWEIGHT1) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_BLENDWEIGHT1) >> COMPBITS_NUM_BLENDWEIGHT1), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_BLENDWEIGHT2] = (FmtBits & FMT_BLENDWEIGHT2) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_BLENDWEIGHT2) >> COMPBITS_NUM_BLENDWEIGHT2), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_BLENDINDICES1] = (FmtBits & FMT_BLENDINDICES1) ? RSX_MAKEATTRIBFORMAT(Stride, 4, CELL_GCM_VERTEX_UB) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_BLENDINDICES2] = (FmtBits & FMT_BLENDINDICES2) ? RSX_MAKEATTRIBFORMAT(Stride, 4, CELL_GCM_VERTEX_UB) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD0] = (FmtBits & FMT_TEXCOORD0) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD0) >> COMPBITS_POS_TEXCOORD0), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD1] = (FmtBits & FMT_TEXCOORD1) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD1) >> COMPBITS_POS_TEXCOORD1), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD2] = (FmtBits & FMT_TEXCOORD2) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD2) >> COMPBITS_POS_TEXCOORD2), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD3] = (FmtBits & FMT_TEXCOORD3) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD3) >> COMPBITS_POS_TEXCOORD3), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD4] = (FmtBits & FMT_TEXCOORD4) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD4) >> COMPBITS_POS_TEXCOORD4), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD5] = (FmtBits & FMT_TEXCOORD5) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD5) >> COMPBITS_POS_TEXCOORD5), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD6] = (FmtBits & FMT_TEXCOORD6) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD6) >> COMPBITS_POS_TEXCOORD6), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
		pVB->m_RSXFormat[ATTRIB_TEXCOORD7] = (FmtBits & FMT_TEXCOORD7) ? RSX_MAKEATTRIBFORMAT(Stride, 1 + ((CompBits & COMPBITS_MASK_TEXCOORD7) >> COMPBITS_POS_TEXCOORD7), CELL_GCM_VERTEX_F) : RSX_MAKEATTRIBFORMATDISABLE;
	}

	M_ASSERT(pTargetMem == pMem, "!");
*/
	m_pVBC->VB_Release(_VBID);

	m_lVBIDInfo[_VBID].m_Fresh |= 1;
}

void CContext_Geometry::Destroy(uint32 _VBID)
{
	delete m_lpVB[_VBID];
	m_lpVB[_VBID] = 0;

	m_lVBIDInfo[_VBID].m_Fresh &= ~1;
}

void CContext_Geometry::PrecacheBegin(uint _Count)
{
}

void CContext_Geometry::PrecacheEnd()
{
}

void CContext_Geometry::PrecacheFlush()
{
	uint32 nTxt = m_pVBC->GetIDCapacity();
	for(uint32 i = 1; i < nTxt; i++)
	{
		uint Flags = m_pVBC->VB_GetFlags(i);
		if(!(Flags & (CXR_VBFLAGS_PRECACHE | CXR_VBFLAGS_ALLOCATED)))
			Destroy(i);
	}
}

void CContext_Geometry::Precache(uint _VBID)
{
	if (m_pVBC->VB_GetFlags(_VBID) & CXR_VBFLAGS_ALLOCATED)
	{
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		if (!(IDInfo.m_Fresh & 1))
		{
			M_LOCK(m_Lock);
			Build(_VBID);
		}
	}
}

static uint8 g_ConvertFormat[CRC_VREGFMT_MAX] =
{
	0,

	CELL_GCM_VERTEX_F,		// V1_F32
	CELL_GCM_VERTEX_F,		// V2_F32
	CELL_GCM_VERTEX_F,		// V3_F32
	CELL_GCM_VERTEX_F,		// V4_F32
	
	CELL_GCM_VERTEX_S32K,	// V1_I16
	CELL_GCM_VERTEX_S32K,	// V2_I16
	CELL_GCM_VERTEX_S32K,	// V3_I16
	CELL_GCM_VERTEX_S32K,	// V4_I16
	
	0,						// V1_U16
	0,						// V2_U16
	0,						// V3_U16
	0,						// V4_U16

	CELL_GCM_VERTEX_S1,		// V3_I16_NORM
	CELL_GCM_VERTEX_CMP,	// V3_N_COMP (11:11:10)
	0,						// V3_U_COMP
	CELL_GCM_VERTEX_UB,		// V4_U8_NORM
	CELL_GCM_VERTEX_UB256,	// V4_U8
	0,						// V4_N8

	0,						// V3_U_COMP

	CELL_GCM_VERTEX_S1,		// V2_I16_NORM
	CELL_GCM_VERTEX_S1,		// V2_I16_NORM
	CELL_GCM_VERTEX_S1,		// V4_I16_NORM

	0,						// V1_U16_NORM
	0,						// V2_U16_NORM
	0,						// V3_U16_NORM
	0,						// V4_U16_NORM
};

static uint ConvertFormat(uint _Format)
{
	M_ASSERT(g_ConvertFormat[_Format], "Unsupported format");
	return g_ConvertFormat[_Format];
}

static uint8 g_ConvertComp[CRC_VREGFMT_MAX] =
{
	0,

	1,						// V1_F32
	2,						// V2_F32
	3,						// V3_F32
	4,						// V4_F32
							
	1,						// V1_I16
	2,						// V2_I16
	3,						// V3_I16
	4,						// V4_I16
							
	0,						// V1_U16
	0,						// V2_U16
	0,						// V3_U16
	0,						// V4_U16

	3,						// V3_I16_NORM
	1,						// V3_N_COMP (11:11:10)
	0,						// V3_U_COMP
	4,						// V4_U8_NORM
	4,						// V4_U8
	0,						// V4_N8

	0,						// V3_U_COMP

	1,						// V1_I16_NORM
	2,						// V2_I16_NORM
	4,						// V4_I16_NORM

	0,						// V1_U16_NORM
	0,						// V2_U16_NORM
	0,						// V3_U16_NORM
	0,						// V4_U16_NORM
};

static uint ConvertNumComp(uint _Format)
{
	M_ASSERT(g_ConvertComp[_Format], "Unsupported format");
	return g_ConvertComp[_Format];
}

static uint8 g_ConvertSize[CRC_VREGFMT_MAX] =
{
	0,						// VOID

	sizeof(fp32) * 1,		// V1_F32
	sizeof(fp32) * 2,		// V2_F32
	sizeof(fp32) * 3,		// V3_F32
	sizeof(fp32) * 4,		// V4_F32

	sizeof(int16) * 1,		// V1_I16
	sizeof(int16) * 2,		// V2_I16
	sizeof(int16) * 3,		// V3_I16
	sizeof(int16) * 4,		// V4_I16

	0,						// V1_I16
	0,						// V2_U16
	0,						// V3_U16
	0,						// V4_U16

	sizeof(int16) * 3,		// NS3_I16
	sizeof(uint32),			// NS3_P32
	0,						// NU3_P32
	sizeof(uint32),			// NU4_P32
	sizeof(uint32),			// NS4_P32
	0,						// COLOR

	0,						// VU3_P32

	sizeof(int16) * 1,		// NS1_I16
	sizeof(int16) * 2,		// NS2_I16
	sizeof(int16) * 4,		// NS4_I16

	0,						// NS1_U16
	0,						// NS2_U16
	0,						// NS3_U16
	0,						// NS4_U16
};

static uint ConvertSize(uint _Format)
{
	M_ASSERT(g_ConvertSize[_Format], "Unsupported format");
	return g_ConvertSize[_Format];
}


void CRenderPS3Resource_VertexBuffer_Local::CreateVertexDeclaration(const CRC_VertexFormat& _VertexFormat)
{
	uint64 DataFmtBits = 0;
	uint64 TexFmtBits = 0;
	uint64 DataCompBits = 0;
	uint64 TexCompBits = 0;
	uint64 Stride = 0;
	if(_VertexFormat.GetFormat(CRC_VREG_POS) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_POS);
		uint Fmt = ConvertFormat(VRegFmt);
		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= Fmt << DATAFMTBITS_POS_POSITION;
		DataCompBits |= (Comp - 1) << DATACOMPBITS_POS_POSITION;
		m_Offset[ATTRIB_POSITION] = Stride;
		Stride += Size;
	}

	for(int iT = 0; iT < CRC_MAXTEXCOORDS; iT++)
	{
		if(_VertexFormat.GetFormat(CRC_VREG_TEXCOORD0 + iT) == CRC_VREGFMT_VOID)
			continue;

		uint Fmt = ConvertFormat(_VertexFormat.GetFormat(CRC_VREG_TEXCOORD0 + iT));
		uint Comp = ConvertNumComp(_VertexFormat.GetFormat(CRC_VREG_TEXCOORD0 + iT));
		uint Size = ConvertSize(_VertexFormat.GetFormat(CRC_VREG_TEXCOORD0 + iT));

		TexFmtBits |= Fmt << (TEXFMTBITS_POS_TEXCOORD0 + iT * TEXFMTBITS_NUM_TEXCOORD0);
		TexCompBits |= (Comp - 1) << (TEXCOMPBITS_POS_TEXCOORD0 + iT * TEXCOMPBITS_NUM_TEXCOORD0);
		m_Offset[ATTRIB_TEXCOORD0 + iT] = Stride;
		Stride += Size;
	}

	if(_VertexFormat.GetFormat(CRC_VREG_NORMAL) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_NORMAL);
		uint Fmt = ConvertFormat(VRegFmt);
		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= Fmt << DATAFMTBITS_POS_NORMAL;
		DataCompBits |= (Comp - 1) << DATACOMPBITS_POS_NORMAL;
		m_Offset[ATTRIB_NORMAL] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_COLOR) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_COLOR);
//		uint Fmt = ConvertFormat(VRegFmt);
//		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= 1 << DATAFMTBITS_POS_COLOR0;
		m_Offset[ATTRIB_COLOR0] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_SPECULAR) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_SPECULAR);
//		uint Fmt = ConvertFormat(VRegFmt);
//		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= 1 << DATAFMTBITS_POS_COLOR1;
		m_Offset[ATTRIB_COLOR1] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_MI0) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_MI0);
//		uint Fmt = ConvertFormat(VRegFmt);
//		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= 1 << DATAFMTBITS_POS_BLENDINDICES1;
		m_Offset[ATTRIB_BLENDINDICES1] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_MW0) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_MW0);
		uint Fmt = ConvertFormat(VRegFmt);
		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= Fmt << DATAFMTBITS_POS_BLENDWEIGHT1;
		DataCompBits |= (Comp - 1) << DATACOMPBITS_POS_BLENDWEIGHT1;
		m_Offset[ATTRIB_BLENDWEIGHT1] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_MI1) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_MI1);
//		uint Fmt = ConvertFormat(VRegFmt);
		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= 1 << DATAFMTBITS_POS_BLENDINDICES2;
		m_Offset[ATTRIB_BLENDINDICES2] = Stride;
		Stride += Size;
	}
	if(_VertexFormat.GetFormat(CRC_VREG_MW1) != CRC_VREGFMT_VOID)
	{
		uint VRegFmt = _VertexFormat.GetFormat(CRC_VREG_MW1);
		uint Fmt = ConvertFormat(VRegFmt);
		uint Comp = ConvertNumComp(VRegFmt);
		uint Size = ConvertSize(VRegFmt);

		DataFmtBits |= Fmt << DATAFMTBITS_POS_BLENDWEIGHT2;
		DataCompBits |= (Comp - 1) << DATACOMPBITS_POS_BLENDWEIGHT2;
		m_Offset[ATTRIB_BLENDWEIGHT2] = Stride;
		Stride += Size;
	}

	M_ASSERT(Stride == _VertexFormat.GetStride(), "!");
	m_DataFmtBits = DataFmtBits;
	m_DataCompBits = DataCompBits;
	m_TexFmtBits = TexFmtBits;
	m_TexCompBits = TexCompBits;
}
/*
void CRenderPS3Resource_VertexBuffer_Local::SetFormat(uint32 _FmtBits, uint32 _SizeBits, uint32 _CompBits)
{
	memset(m_Offset, 0xff, ATTRIB_MAX_NUM);
	m_FmtBits = _FmtBits;
	m_SizeBits = _SizeBits;
	m_CompBits = _CompBits;

	uint32 Stride = 0;
	if(_FmtBits & FMT_POSITION)
	{
		m_Offset[ATTRIB_POSITION]	= Stride;
		switch((_SizeBits & SIZEBITS_MASK_POSITION) >> SIZEBITS_POS_POSITION)
		{
		case 0:	Stride	+= sizeof(fp32) * 3; break;
		case 1:	Stride	+= sizeof(fp16) * 3; break;
		case 2: Stride	+= sizeof(int8) * 4; break;
		case 3: Stride	+= sizeof(uint32); break;
		}
	}
	if(_FmtBits & FMT_NORMAL)
	{
		m_Offset[ATTRIB_NORMAL]	= Stride;
		switch((_SizeBits & SIZEBITS_MASK_NORMAL) >> SIZEBITS_POS_NORMAL)
		{
		case 0:	Stride	+= sizeof(fp32) * 3; break;
		case 1:	Stride	+= sizeof(fp16) * 3; break;
		case 2: Stride	+= sizeof(int8) * 4; break;
		case 3: Stride	+= sizeof(uint32); break;
		}
	}
	if(_FmtBits & FMT_COLOR0)
	{
		m_Offset[ATTRIB_COLOR0]	= Stride;
		Stride	+= sizeof(uint32);
	}
	if(_FmtBits & FMT_COLOR1)
	{
		m_Offset[ATTRIB_COLOR1]	= Stride;
		Stride	+= sizeof(uint32);
	}
	if(_FmtBits & FMT_BLENDWEIGHT1)
	{
		m_Offset[ATTRIB_BLENDWEIGHT1]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_BLENDWEIGHT1) >> COMPBITS_POS_BLENDWEIGHT1);
		switch((_SizeBits & SIZEBITS_MASK_BLENDWEIGHT1) >> SIZEBITS_POS_BLENDWEIGHT1)
		{
		case 0: Stride	+= sizeof(fp32) * NumComp; break;
		case 1: Stride	+= sizeof(fp16) * NumComp; break;
		case 2: Stride	+= sizeof(int8) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_BLENDWEIGHT2)
	{
		m_Offset[ATTRIB_BLENDWEIGHT2]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_BLENDWEIGHT2) >> COMPBITS_POS_BLENDWEIGHT2);
		switch((_SizeBits & SIZEBITS_MASK_BLENDWEIGHT2) >> SIZEBITS_POS_BLENDWEIGHT2)
		{
		case 0: Stride	+= sizeof(fp32) * NumComp; break;
		case 1: Stride	+= sizeof(fp16) * NumComp; break;
		case 2: Stride	+= sizeof(int8) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_BLENDINDICES1)
	{
		m_Offset[ATTRIB_BLENDINDICES1]	= Stride;
		Stride += sizeof(uint32);
	}
	if(_FmtBits & FMT_BLENDINDICES2)
	{
		m_Offset[ATTRIB_BLENDINDICES2]	= Stride;
		Stride += sizeof(uint32);
	}
	if(_FmtBits & FMT_TEXCOORD0)
	{
		m_Offset[ATTRIB_TEXCOORD0]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD0) >> COMPBITS_POS_TEXCOORD0);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD0) >> SIZEBITS_POS_TEXCOORD0)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD1)
	{
		m_Offset[ATTRIB_TEXCOORD1]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD1) >> COMPBITS_POS_TEXCOORD1);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD1) >> SIZEBITS_POS_TEXCOORD1)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD2)
	{
		m_Offset[ATTRIB_TEXCOORD2]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD2) >> COMPBITS_POS_TEXCOORD2);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD2) >> SIZEBITS_POS_TEXCOORD2)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD3)
	{
		m_Offset[ATTRIB_TEXCOORD3]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD3) >> COMPBITS_POS_TEXCOORD3);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD3) >> SIZEBITS_POS_TEXCOORD3)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD4)
	{
		m_Offset[ATTRIB_TEXCOORD4]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD4) >> COMPBITS_POS_TEXCOORD4);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD4) >> SIZEBITS_POS_TEXCOORD4)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD5)
	{
		m_Offset[ATTRIB_TEXCOORD5]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD5) >> COMPBITS_POS_TEXCOORD5);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD5) >> SIZEBITS_POS_TEXCOORD5)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD6)
	{
		m_Offset[ATTRIB_TEXCOORD6]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD6) >> COMPBITS_POS_TEXCOORD6);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD6) >> SIZEBITS_POS_TEXCOORD6)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}
	if(_FmtBits & FMT_TEXCOORD7)
	{
		m_Offset[ATTRIB_TEXCOORD7]	= Stride;
		uint32 NumComp = 1 + ((_CompBits & COMPBITS_MASK_TEXCOORD7) >> COMPBITS_POS_TEXCOORD7);
		switch((_SizeBits & SIZEBITS_MASK_TEXCOORD7) >> SIZEBITS_POS_TEXCOORD7)
		{
		case 0:	Stride	+= sizeof(fp32) * NumComp; break;
		case 1:	Stride	+= sizeof(fp16) * NumComp; break;
		}
	}

	M_ASSERT(Stride < 256, "Stride is larger than allocated number of bits can hold");
	m_Stride = Stride;
}
*/

int CContext_Geometry::GetVBSize(uint _VBID)
{
	int Size = 0;
	if(m_lpVB[_VBID])
	{
		Size = m_lpVB[_VBID]->GetSize();
	}

	return Size;
}

int CRCPS3GCM::Geometry_GetVBSize(int _VBID)
{
	return m_ContextGeometry.GetVBSize(_VBID);
}

};	// namespace NRenderPS3GCM
