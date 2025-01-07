
#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| ARB_Vertex_Buffer_Object
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define VBO_VBID(VBID) (VBID*2)
#define VBO_IBID(VBID) (VBID*2+1)

void GetMinMax(const uint16* _pIndices, int _nCount, uint16& _Min, uint16& _Max)
{
	// Starting on a primitive restart index is not valid (i.e. first index cannot be 0xffff)
	_Min = _pIndices[0];
	_Max = _pIndices[0];
	for(int i = 1; i < _nCount; i++)
	{
		uint16 Idx = _pIndices[i];
		if(Idx != 0xffff)
		{
			_Min = Min(_Min, Idx);
			_Max = Max(_Min, Idx);
		}
	}
}

static bool IsSinglePrimType(const uint16* _piIndices, int _nPrim)
{
	CRCPrimStreamIterator StreamIterate(_piIndices, _nPrim);
	if(StreamIterate.IsValid())
	{
		int CurrentType = StreamIterate.GetCurrentType();
		do
		{
			if(CurrentType != StreamIterate.GetCurrentType())
				return false;
		}
		while(StreamIterate.Next());
	}

	return true;
}

static void GetPrimStats(const uint16* _piIndices, int _nPrim, int& _PrimCount, int& _IndexCount)
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

static void AssemblePrimStream(CRCGL_VBInfo& _VBI, const uint16* _piIndices, int _nPrim)
{
	int nPrimCount = 0;
	int nIndexCount = 0;
	GetPrimStats(_piIndices, _nPrim, nPrimCount, nIndexCount);
	_VBI.m_liPrim.SetLen(nIndexCount + nPrimCount - 1);

	CRCPrimStreamIterator StreamIterate(_piIndices, _nPrim);
	if(StreamIterate.IsValid())
	{
		int CurrentType = StreamIterate.GetCurrentType();
		uint16* pStream = _VBI.m_liPrim.GetBasePtr();
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
		int nPrim = _VBI.m_liPrim.Len();
		GetMinMax(_VBI.m_liPrim.GetBasePtr(), _VBI.m_liPrim.Len(), _VBI.m_Min, _VBI.m_Max);
		_VBI.m_VB.m_PrimType = CurrentType;
		_VBI.m_VB.m_nPrim = nPrim;
		_VBI.m_nTri = nTri;
	}
}

void CRenderContextPS3::VBO_CreateVB(int _VBID, CRC_BuildVertexBuffer& _VBB)
{
//	ConOut(CStrF("(CRenderContextPS3::VAO_CreateVB) %d, Free %d", _VBID, m_VAO_spVBHeap->GetAvail()));
//	M_TRACE("(CRenderContextPS3::VAO_CreateVB) VBID %d\n", _VBID);

	if (!m_lspVB[_VBID])
	{
		m_lspVB[_VBID] = MNew(CRCGL_VBInfo);
		if (!m_lspVB[_VBID])
			MemError("VAO_CreateVB");
		m_lspVB[_VBID]->Create(_VBID);
	}
	else
	{
		GLuint liDelete[2];
		liDelete[0] = VBO_VBID(_VBID);
		liDelete[1] = VBO_IBID(_VBID);
		int nCount = 1;
		if(m_lspVB[_VBID]->m_nTri)
			nCount = 2;

		glDeleteBuffers(nCount, liDelete);
	}

	// Lets create a temporary buffer
	CRC_VertexBuffer _VB;
	TThinArray<uint8> Temp;
	Temp.SetLen(_VBB.CRC_VertexBuffer_GetSize());
	_VBB.CRC_VertexBuffer_ConvertTo(Temp.GetBasePtr(), _VB);

	spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
	CRCGL_VBInfo& VBI = *spVBI;

	int iVtxCollectFunc;
	int VtxStride;	// This might be bogus cause it will always add 4 for color, regardless of presence of a color array in _VB
	{
		// PS3 does not support 1D texcoords, patch patch
		Geometry_GetVertexFormat(_VB, VBI.m_VtxFmt, VtxStride, iVtxCollectFunc);
	}

	int nV = _VB.m_nV;

	// Calculate vertex size (stride)
	int Stride = sizeof(CVec3Dfp4);

	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			if (_VB.m_pTV[i]) Stride += sizeof(fp4) * _VB.m_nTVComp[i];
	}

	if (_VB.m_pN) Stride += sizeof(CVec3Dfp4);
	if (_VB.m_pCol) Stride += sizeof(CPixel32);
	if (_VB.m_pSpec) Stride += sizeof(CPixel32);
	if (_VB.m_pMI) Stride += sizeof(uint32); // MIFIXME
	if (_VB.m_pMW) Stride += sizeof(fp4) * _VB.m_nMWComp;

	VBI.m_Stride = Stride;
	int Size = Stride*nV;

//	GLuint liDelete[2];
//	liDelete[0]= VBO_VBID(_VBID);
//	liDelete[1]= VBO_IBID(_VBID);
//	glDeleteBuffers(2, liDelete);
//	GLErr("VBO_CreateVB (glDeleteBuffers)");

	glBindBuffer(GL_ARRAY_BUFFER, VBO_VBID(_VBID));
	GLErr("VBO_CreateVB (glBindBuffer, 0)");

	glBufferData(GL_ARRAY_BUFFER, Size, (const GLvoid *)NULL, GL_STATIC_DRAW);
	GLErr("VBO_CreateVB (glBufferData)");

	void* pBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	GLErr("VBO_CreateVB (glMapBuffer)");

	if (!pBuffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		GLErr("VBO_CreateVB (glBindBuffer, 1)");
		m_lspVB[_VBID] = NULL;
//		MemError("VBO_CreateVB");
		return;
	}

	{
		uint8* pWrite = (uint8*)pBuffer;
		CVec3Dfp4* pV = _VB.m_pV;
		CVec3Dfp4* pN = _VB.m_pN;
		fp4* lpTV[CRC_MAXTEXCOORDS];
		memcpy(&lpTV, &_VB.m_pTV, sizeof(lpTV));

		CPixel32* pCol = _VB.m_pCol;
		CPixel32* pSpec = _VB.m_pSpec;
		uint32* pMI = _VB.m_pMI;
		fp4* pMW = _VB.m_pMW;

		for(int iV = 0; iV < nV; iV++)
		{
			*((CVec3Dfp4*)pWrite) = *pV++; pWrite += sizeof(CVec3Dfp4);
			if (pN) { *((CVec3Dfp4*)pWrite) = *pN++; pWrite += sizeof(CVec3Dfp4); }
			if (pCol) { *((CPixel32*)pWrite) = *pCol++; pWrite += sizeof(CPixel32); }
			if (pSpec) { *((CPixel32*)pWrite) = *pSpec++; pWrite += sizeof(CPixel32); }
			for(int iTV = 0; iTV < CRC_MAXTEXCOORDS; iTV++)
			{
				if (lpTV[iTV])
				{
					for(int i = 0; i < _VB.m_nTVComp[iTV]; i++) { *((fp4*)pWrite) = *((lpTV[iTV])++); pWrite += sizeof(fp4); }
				}
			}
			if (pMI) { *((uint32*)pWrite) = *pMI++; ::SwapLE((uint32&)*pWrite); pWrite += sizeof(uint32); }
			if (pMW) { for(int i = 0; i < _VB.m_nMWComp; i++) { *((fp4*)pWrite) = *pMW++; pWrite += sizeof(fp4); } }
		}

		M_ASSERT((pWrite - (uint8*)pBuffer) == nV*Stride, "!");
	}

	VBI.m_pVB = (void*)-1;	// Indicate it's not empty

	glUnmapBuffer(GL_ARRAY_BUFFER);
	GLErr("VBO_CreateVB (glUnmapBuffer)");
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLErr("VBO_CreateVB (glBindBuffer, 4)");

	// Copy primitives?
	if (_VB.m_piPrim)
	{
		if(_VB.m_PrimType == CRC_RIP_STREAM)
		{
			if(IsSinglePrimType(_VB.m_piPrim, _VB.m_nPrim))
			{
				AssemblePrimStream(VBI, _VB.m_piPrim, _VB.m_nPrim);
			}
			else
			{
				// Convert to triangles
				VBI.m_liPrim.Clear();

				uint16 lTriIndices[1024*3];
				
				CRCPrimStreamIterator StreamIterate(_VB.m_piPrim, _VB.m_nPrim);
				
				while(StreamIterate.IsValid())
				{
					int nTriIndices = 1024*3;
					bool bDone = Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
					if (nTriIndices)
					{
						int iDst = VBI.m_liPrim.Len();
						VBI.m_liPrim.SetLen(iDst + nTriIndices);
						memcpy(&VBI.m_liPrim[iDst], lTriIndices, nTriIndices*2);
	//					Internal_VAIndxTriangles(lTriIndices, nTriIndices/3, 0);
					}
					
					if (bDone) break;
				}

				VBI.m_VB.m_nPrim = VBI.m_liPrim.Len() / 3;
				VBI.m_VB.m_PrimType = CRC_RIP_TRIANGLES;
				VBI.m_nTri = VBI.m_VB.m_nPrim;
			}
		}
		else
		{
			VBI.m_liPrim.SetLen((_VB.m_PrimType == CRC_RIP_TRIANGLES) ? _VB.m_nPrim*3 : _VB.m_nPrim);
			VBI.m_VB.m_nPrim = _VB.m_nPrim;
			VBI.m_VB.m_PrimType = _VB.m_PrimType;
			VBI.m_nTri = _VB.m_nPrim;
			memcpy(VBI.m_liPrim.GetBasePtr(), _VB.m_piPrim, VBI.m_liPrim.Len()*2);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_IBID(_VBID));
		GLErr("VBO_CreateVB (glBindBuffer, 2)");
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, VBI.m_liPrim.ListSize(), VBI.m_liPrim.GetBasePtr(), GL_STATIC_DRAW);
		GLErr("VBO_CreateVB (glBufferData)");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLErr("VBO_CreateVB (glBindBuffer, 3)");
		GetMinMax(VBI.m_liPrim.GetBasePtr(), VBI.m_liPrim.Len(), VBI.m_Min, VBI.m_Max);
		VBI.m_liPrim.Destroy();
	}
}

void CRenderContextPS3::VBO_DestroyVB(int _VBID)
{
	Internal_VA_Disable();
	if (m_lspVB[_VBID])
	{
		spCRCGL_VBInfo spVB = m_lspVB[_VBID];

		// VB is not "fresh" anymore
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		IDInfo.m_Fresh &= ~1;

		GLuint liDelete[2];
		liDelete[0]= VBO_VBID(_VBID);
		liDelete[1]= VBO_IBID(_VBID);
		glDeleteBuffers(2, liDelete);
		GLErr("VBO_DestroyVB (glDeleteBuffers)");

		// Destroy info block
		m_lspVB[_VBID] = NULL;
	}
}

void CRenderContextPS3::VBO_Disable()
{
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CRenderContextPS3::VBO_Begin(int _VBID)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO_VBID(_VBID));
	GLErr("VBO_Begin (glBindBuffer, 0)");

	spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
	if (!spVBI)
		return;

	CRCGL_VBInfo& VBI = *spVBI;
	int VtxFmt = VBI.m_VtxFmt;

	Internal_VA_SetArrays(_VBID, VtxFmt, VBI.m_Stride, NULL);

	m_VAEnable |= CRCGL_VA_VBO_ENABLE,

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLErr("VBO_Begin (glBindBuffer, 1)");
}

void CRenderContextPS3::VBO_RenderVB(int _VBID)
{
	spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
	CRCGL_VBInfo* pVBI = spVBI;
	if (!pVBI)
		return;
	VBO_Begin(_VBID);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_IBID(_VBID));
	GLErr("VBO_RenderVB (glBindBuffer, 0)");
	switch(pVBI->m_VB.m_PrimType)
	{
	default:
		DebugBreak();
		break;

	case CRC_RIP_TRISTRIP:
	case CRC_RIP_TRIFAN:
		{
			int GLPrimType = (pVBI->m_VB.m_PrimType == CRC_RIP_TRISTRIP)?GL_TRIANGLE_STRIP:GL_TRIANGLE_FAN;
			Internal_VAIndx(GLPrimType, pVBI->m_liPrim.GetBasePtr(), pVBI->m_VB.m_nPrim, pVBI->m_Min, pVBI->m_Max);
			m_nVertices += pVBI->m_VB.m_nPrim;
			m_nTriangles += pVBI->m_nTri;
			m_nTriTotal += pVBI->m_nTri;
		}
		break;

	case CRC_RIP_TRIANGLES :
		{
			int nTri = pVBI->m_nTri;
			int nV = (nTri << 1) + nTri;
			Internal_VAIndx(GL_TRIANGLES, pVBI->m_liPrim.GetBasePtr(), nV, pVBI->m_Min, pVBI->m_Max);
			m_nVertices += nV;
			m_nTriangles += nTri;
			m_nTriTotal += nTri;
		}
		break;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	GLErr("VBO_RenderVB (glBindBuffer, 1)");
}

void CRenderContextPS3::VBO_RenderVB_IndexedTriangles(int _VBID, uint16* _piPrim, int _nTri)
{
	VBO_Begin(_VBID);
	Internal_VAIndxTriangles(_piPrim, _nTri);
}

void CRenderContextPS3::VBO_RenderVB_IndexedPrimitives(int _VBID, uint16* _piPrim, int _nPrim)
{
	VBO_Begin(_VBID);
	Internal_VAIndxPrimitives(_piPrim, _nPrim, 0);
}


