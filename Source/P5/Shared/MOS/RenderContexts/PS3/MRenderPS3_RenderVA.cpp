#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

#include "../../XR/XRVBContext.h"

enum
{
	GL_ATTR_POSITION = 0,			// float4
	GL_ATTR_BLENDWEIGHT = 1,		// float
	GL_ATTR_NORMAL = 2,				// float3
	GL_ATTR_COLOR0 = 3,				// float4 (fixed point precision)
	GL_ATTR_COLOR1 = 4,				// float4 (fixed point precision)
	GL_ATTR_FOGCOORD = 5,			// float
	GL_ATTR_PSIZE = 6,				// float
	GL_ATTR_BLENDINDICES = 7,		// float4
	GL_ATTR_TEXCOORD0 = 8,			// float4
	GL_ATTR_TEXCOORD1 = 9,			// float4
	GL_ATTR_TEXCOORD2 = 10,			// float4
	GL_ATTR_TEXCOORD3 = 11,			// float4
	GL_ATTR_TEXCOORD4 = 12,			// float4
	GL_ATTR_TEXCOORD5 = 13,			// float4
	GL_ATTR_TEXCOORD6 = 14,			// float4
	GL_ATTR_TEXCOORD7 = 15,			// float4

	GL_ATTR_BLENDWEIGHT2 = GL_ATTR_FOGCOORD,
	GL_ATTR_BLENDINDICES2 = GL_ATTR_COLOR1,
};

// Attr0: Position
// Attr1: BlendWeight
// Attr2: Normal
// Attr3: Diffuse, Color0
// Attr4: Specular, Color1
// Attr5: FogCoord, TessFactor
// Attr6: PSize
// Attr7: BlendIndices
// Attr8-13: Texcoord0-Texcoord6
// Attr14: Tangent, Texcoord7
// Attr15: Binromal, Texcoord8

// -------------------------------------------------------------------
void CRenderContextPS3::VB_DeleteAll()
{
	for(int i = 0; i < m_lspVB.Len(); i++)
	{
		if (m_lspVB[i] != NULL)
			VB_Delete(i);
	}

	m_lspVB.Destroy();
}


void CRenderContextPS3::VB_Delete(int _VBID)
{
	if (!m_lspVB.ValidPos(_VBID)) return;
	if (!m_lspVB[_VBID]) return;

	CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
	IDInfo.m_Fresh &= ~1;

	spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
	if (spVBI->IsEmpty()) return;

	VBO_DestroyVB(_VBID);

	spVBI->Clear();
}

void CRenderContextPS3::VB_Create(int _VBID)
{
	m_lspVB.SetLen(m_lVBIDInfo.Len());
	VB_Delete(_VBID);

//	CRCGL_VBInfo& VBI = m_lVB[_VBID];

	CRC_BuildVertexBuffer VBB;
	m_pVBCtx->VB_Get(_VBID, VBB, VB_GETFLAGS_BUILD);

/*	if (VB.m_piPrim)
		VB_CreateDisplayList(_VBID, VB);
	else*/

	VBO_CreateVB(_VBID, VBB);

	m_pVBCtx->VB_Release(_VBID);

	CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
	IDInfo.m_Fresh |= 1;
}

//----------------------------------------------------------------
void CRenderContextPS3::Internal_VA_Disable()
{
	cgGLDisableAttrib(GL_ATTR_POSITION);
	cgGLDisableAttrib(GL_ATTR_BLENDWEIGHT);
	cgGLDisableAttrib(GL_ATTR_NORMAL);
	cgGLDisableAttrib(GL_ATTR_COLOR0);
	cgGLDisableAttrib(GL_ATTR_COLOR1);		// Also MatrixIndex2
	cgGLDisableAttrib(GL_ATTR_FOGCOORD);	// Also MatrixWeight2
	cgGLDisableAttrib(GL_ATTR_PSIZE);
	cgGLDisableAttrib(GL_ATTR_BLENDINDICES);
	for(int i = 0; i < 8; i++)
	{
		cgGLDisableAttrib(GL_ATTR_TEXCOORD0 + i);
	}

	m_VAEnable = 0;

	m_VACurrentVBID = 0;
	m_VACurrentFmt = 0;
	m_VACurrentStride = -1;
	m_pVACurrentFmtBase = 0;
}

#define MACRO_PtrDiff(Ptr, HeapPtr) ((Ptr) ? ((const uint8*)(Ptr) - (const uint8*)(HeapPtr)) : 0)
#define MACRO_ValidPtr(Ptr, HeapPtr, Size) ((MACRO_PtrDiff(Ptr, HeapPtr) >= 0) && (MACRO_PtrDiff(Ptr, HeapPtr) < (Size)))

void CRenderContextPS3::Internal_VA_NormalPtr(const CVec3Dfp4* _pN, int _Stride)
{
	// Normal
	if (_pN)
	{
		cgGLAttribPointer(GL_ATTR_NORMAL, 3, GL_FLOAT, GL_TRUE, _Stride, _pN);
		GLErr("Internal_VA_NormalPtr (cgGLAttribPointer)");
		if (!(m_VAEnable & CRCGL_VA_NORMAL))
		{
			cgGLEnableAttrib(GL_ATTR_NORMAL);
			GLErr("Internal_VA_NormalPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_NORMAL;
		}
#ifdef CRCGL_VADEBUG_ENABLE
		m_VADebugState.m_pN = const_cast<CVec3Dfp4*>(_pN);
#endif
	}
	else
		if (m_VAEnable & CRCGL_VA_NORMAL)
		{
			cgGLDisableAttrib(GL_ATTR_NORMAL);
			GLErr("Internal_VA_NormalPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_NORMAL;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pN = NULL;
#endif
		}
}

void CRenderContextPS3::Internal_VA_ColorPtr(const CPixel32* _pCol, int _Stride)
{
	// Color
	if (_pCol)
	{
		cgGLAttribPointer(GL_ATTR_COLOR0, 4, GL_UNSIGNED_BYTE, GL_TRUE, _Stride, _pCol);
		GLErr("Internal_VA_ColorPtr (cgGLAttribPointer)");
		if (!(m_VAEnable & CRCGL_VA_COLOR))
		{
			cgGLEnableAttrib(GL_ATTR_COLOR0);
			GLErr("Internal_VA_ColorPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_COLOR;
		}
#ifdef CRCGL_VADEBUG_ENABLE
		m_VADebugState.m_pCol = const_cast<CPixel32*>(_pCol);
#endif
	}
	else
		if (m_VAEnable & CRCGL_VA_COLOR)
		{
			cgGLDisableAttrib(GL_ATTR_COLOR0);
			GLErr("Internal_VA_ColorPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_COLOR;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pCol = NULL;
#endif
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
}

/*
void CRenderContextPS3::Internal_VA_SpecPtr(const CPixel32* _pSpec, int _Stride)
{
	// Color
	if (_pSpec)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_COLOR1, 4, GL_UNSIGNED_BYTE, GL_TRUE, _Stride, _pSpec);
		GLErr("Internal_VA_SpecPtr (cgGLAttribPointer)");
		if (!(m_VAEnable & CRCGL_VA_SECONDARYCOLOR))
		{
			cgGLEnableAttrib(GL_ATTR_COLOR1);
			GLErr("Internal_VA_SpecPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_SECONDARYCOLOR;
		}
#ifdef CRCGL_VADEBUG_ENABLE
		m_VADebugState.m_pSpec = const_cast<CPixel32*>(_pSpec);
#endif
	}
	else
		if (m_VAEnable & CRCGL_VA_SECONDARYCOLOR)
		{
			cgGLDisableAttrib(GL_ATTR_COLOR1);
			GLErr("Internal_VA_SpecPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_SECONDARYCOLOR;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pSpec = NULL;
#endif
		}
}

void CRenderContextPS3::Internal_VA_FogPtr(const fp4* _pFog, int _Stride)
{
	// Color
	if (_pFog)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_FOGCOORD, 1, GL_FLOAT, GL_FALSE, _Stride, _pFog);
		GLErr("Internal_VA_FogPtr (cgGLAttribPointer)");
		if (!(m_VAEnable & CRCGL_VA_FOGCOORD))
		{
			cgGLEnableAttrib(GL_ATTR_FOGCOORD);
			GLErr("Internal_VA_FogPtr (cgGLEnableClientState)");
			m_VAEnable |= CRCGL_VA_FOGCOORD;
		}
#ifdef CRCGL_VADEBUG_ENABLE
//		m_VADebugState.m_pFog = const_cast<CPixel32*>(_pFog);
#endif
	}
	else
		if (m_VAEnable & CRCGL_VA_FOGCOORD)
		{
			cgGLDisableAttrib(GL_ATTR_FOGCOORD);
			GLErr("Internal_VA_FogPtr (cgGLDisableClientState)");
			m_VAEnable &= ~CRCGL_VA_FOGCOORD;
#ifdef CRCGL_VADEBUG_ENABLE
//			m_VADebugState.m_pFog = NULL;
#endif
		}
}
*/
void CRenderContextPS3::Internal_VA_TexCoordPtr(const fp4* _pTV, int _nComp, int _iTxt, int _Stride)
{
	int VAMask = CRCGL_VA_TEXCOORD0 << _iTxt;
	if(_pTV)
	{
		cgGLAttribPointer(GL_ATTR_TEXCOORD0 + _iTxt, _nComp, GL_FLOAT, GL_FALSE, _Stride, _pTV);
		GLErr("Internal_VA_TexCoordPtr (cgGLAttribPointer)");
		if(!(m_VAEnable & VAMask))
		{
			cgGLEnableAttrib(GL_ATTR_TEXCOORD0 + _iTxt);
			GLErr("Internal_VA_TexCoordPtr (cgGLEnableAttrib)");
			m_VAEnable |= VAMask;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pTV[_iTxt] = const_cast<fp4*>(_pTV);
#endif
		}
	}
	else
	{
		if(m_VAEnable & VAMask)
		{
			cgGLDisableAttrib(GL_ATTR_TEXCOORD0 + _iTxt);
			GLErr("Internal_VA_TexCoordPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~VAMask;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pTV[_iTxt] = 0;
#endif
		}
	}
}

void CRenderContextPS3::Internal_VA_MatrixIndexPtr(const uint32* _piMatrices, int _nComp, int _Stride)
{
	int nComp0 = Min(4, _nComp);
	int nComp1 = _nComp - nComp0;
	if(_piMatrices)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_BLENDINDICES, 4, GL_UNSIGNED_BYTE, GL_TRUE, _Stride, _piMatrices);
		GLErr("Internal_VA_MatrixIndexPtr (cgGLAttribPointer)");
		if(!(m_VAEnable & CRCGL_VA_MATRIXINDEX))
		{
			cgGLEnableAttrib(GL_ATTR_BLENDINDICES);
			GLErr("Internal_VA_MatrixIndexPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_MATRIXINDEX;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pMI = const_cast<uint32*>(_piMatrices);
#endif
		}
	}
	else
	{
		if(m_VAEnable & CRCGL_VA_MATRIXINDEX)
		{
			cgGLDisableAttrib(GL_ATTR_BLENDINDICES);
			GLErr("Internal_VA_MatrixIndexPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_MATRIXINDEX;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pMI = 0;
#endif
		}
	}

	if(_piMatrices && nComp1 > 0)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_BLENDINDICES2, 4, GL_UNSIGNED_BYTE, GL_TRUE, _Stride, _piMatrices + 1);
		GLErr("Internal_VA_MatrixIndexPtr (cgGLAttribPointer)");
		if(!(m_VAEnable & CRCGL_VA_MATRIXINDEX2))
		{
			cgGLEnableAttrib(GL_ATTR_BLENDINDICES2);
			GLErr("Internal_VA_MatrixIndexPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_MATRIXINDEX2;
		}
	}
	else
	{
		if(m_VAEnable & CRCGL_VA_MATRIXINDEX2)
		{
			cgGLDisableAttrib(GL_ATTR_BLENDINDICES2);
			GLErr("Internal_VA_MatrixIndexPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_MATRIXINDEX2;
		}
	}
}

void CRenderContextPS3::Internal_VA_MatrixWeightPtr(const fp4* _pMatrixWeights, int _nComp, int _Stride)
{
	int nComp0 = Min(_nComp, 4);
	int nComp1 = _nComp - nComp0;
	if(_pMatrixWeights)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_BLENDWEIGHT, nComp0, GL_FLOAT, GL_FALSE, _Stride, _pMatrixWeights);
		GLErr("Internal_VA_MatrixWeightPtr (cgGLAttribPointer)");
		if(!(m_VAEnable & CRCGL_VA_MATRIXWEIGHT))
		{
			cgGLEnableAttrib(GL_ATTR_BLENDWEIGHT);
			GLErr("Internal_VA_MatrixWeightPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_MATRIXWEIGHT;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pMW = const_cast<fp4*>(_pMatrixWeights);
#endif
		}
	}
	else
	{
		if(m_VAEnable & CRCGL_VA_MATRIXWEIGHT)
		{
			cgGLDisableAttrib(GL_ATTR_BLENDWEIGHT);
			GLErr("Internal_VA_MatrixWeightPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_MATRIXWEIGHT;
#ifdef CRCGL_VADEBUG_ENABLE
			m_VADebugState.m_pMW = NULL;
#endif
		}
	}

	if(_pMatrixWeights && nComp1 > 0)
	{
		//UNSUPPORTED
		cgGLAttribPointer(GL_ATTR_BLENDWEIGHT2, nComp1, GL_FLOAT, GL_FALSE, _Stride, _pMatrixWeights + 4);
		GLErr("Internal_VA_MatrixWeightPtr (cgGLAttribPointer)");
		if(!(m_VAEnable & CRCGL_VA_MATRIXWEIGHT2))
		{
			cgGLEnableAttrib(GL_ATTR_BLENDWEIGHT2);
			GLErr("Internal_VA_MatrixWeightPtr (cgGLEnableAttrib)");
			m_VAEnable |= CRCGL_VA_MATRIXWEIGHT2;
		}
	}
	else
	{
		if(m_VAEnable & CRCGL_VA_MATRIXWEIGHT2)
		{
			cgGLDisableAttrib(GL_ATTR_BLENDWEIGHT2);
			GLErr("Internal_VA_MatrixWeightPtr (cgGLDisableAttrib)");
			m_VAEnable &= ~CRCGL_VA_MATRIXWEIGHT2;
		}
	}
}



void CRenderContextPS3::Internal_VA_VertexPtr(const CVec3Dfp4* _pV, int _Stride)
{
	// Vertex
	m_nStateVAAddress++;

	cgGLAttribPointer(GL_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, _Stride, _pV);
	GLErr("Internal_VA_VertexPtr (cgGLAttribPointer)");
	if (!(m_VAEnable & CRCGL_VA_VERTEX))
	{
		m_nStateVAEnable++;
		cgGLEnableAttrib(GL_ATTR_POSITION);

		GLErr("Internal_VA_VertexPtr (cgGLEnableAttrib)");
		m_VAEnable |= CRCGL_VA_VERTEX;
#ifdef CRCGL_VADEBUG_ENABLE
		m_VADebugState.m_pV = const_cast<CVec3Dfp4*>(_pV);
#endif
	}
}

void CRenderContextPS3::Internal_VA_SetArrays(int _VBID, int _VtxFmt, int _Stride, uint8* _pBase)
{
	if (m_VACurrentFmt == _VtxFmt &&
		m_VACurrentStride == _Stride &&
		m_pVACurrentFmtBase == _pBase &&
		m_VACurrentVBID == _VBID)
		return;

	m_VACurrentVBID = _VBID;
	m_VACurrentFmt = _VtxFmt;
	m_VACurrentStride = _Stride;
	m_pVACurrentFmtBase = _pBase;

	if (m_VAEnable & CRCGL_VA_VBO_ENABLE)
		VBO_Disable();

	uint8* pData = _pBase;

	// Vertex
	Internal_VA_VertexPtr((CVec3Dfp4*)pData, _Stride);
	pData += sizeof(CVec3Dfp4);

	// Normal
	if (_VtxFmt & CRC_VF_NORMAL)
	{
		Internal_VA_NormalPtr((CVec3Dfp4*)pData, _Stride);
		pData += sizeof(CVec3Dfp4);
	}
	else
		Internal_VA_NormalPtr(NULL, 0);

	// Color
	if (_VtxFmt & CRC_VF_COLOR)
	{
		Internal_VA_ColorPtr((CPixel32*)pData, _Stride);
		pData += 4;
	}
	else
		Internal_VA_ColorPtr(NULL, 0);

	// TexCoords
	uint8* lpTexCoordBases[CRC_MAXTEXCOORDS];
	{
		for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
		{
			int nComp = CRC_FV_TEXCOORD_NUMCOMP(_VtxFmt, t);
			lpTexCoordBases[t] = pData;
			pData += 4*nComp;
		}
	}

	{
//		for(int t = 0; t < m_Caps_nMultiTexture; t++)
		for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
		{
			int iTexCoord = m_CurrentAttrib.m_iTexCoordSet[t];
			int nComp = CRC_FV_TEXCOORD_NUMCOMP(_VtxFmt, iTexCoord);
			if (nComp)
			{
				Internal_VA_TexCoordPtr((fp4*)lpTexCoordBases[iTexCoord], nComp, t, _Stride);
			}
			else
				Internal_VA_TexCoordPtr(NULL, 0, t, 0);
		}
	}

	// Matrix index
	if (_VtxFmt & CRC_VF_MATRIXI)
	{
		int nComp = CRC_FV_MATRIXW_NUMCOMP(_VtxFmt);
		Internal_VA_MatrixIndexPtr((uint32*)pData, nComp, _Stride);
		pData += 4;
	}
	else
		Internal_VA_MatrixIndexPtr(NULL, 0, 0);

	// Matrix weight
	if (_VtxFmt & CRC_VF_MATRIXW_MASK)
	{
		int nComp = CRC_FV_MATRIXW_NUMCOMP(_VtxFmt);
		Internal_VA_MatrixWeightPtr((fp4*)pData, nComp, _Stride);
		pData += 4*nComp;
	}
	else
		Internal_VA_MatrixWeightPtr(NULL, 0, 0);

}

void CRenderContextPS3::Internal_VA_SetArrays(const CRC_VertexBuffer& _VB)
{
	m_VACurrentVBID = 0;
	m_VACurrentFmt = -1;
	m_VACurrentStride = -1;
	m_pVACurrentFmtBase = 0;

	if (m_VAEnable & CRCGL_VA_VBO_ENABLE)
		VBO_Disable();

	Internal_VA_VertexPtr(_VB.m_pV);
	Internal_VA_NormalPtr(_VB.m_pN);
	Internal_VA_ColorPtr(_VB.m_pCol);
	Internal_VA_MatrixIndexPtr(_VB.m_pMI, _VB.m_nMWComp);
	Internal_VA_MatrixWeightPtr(_VB.m_pMW, _VB.m_nMWComp);
	for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
	{
		int iTVSet = m_CurrentAttrib.m_iTexCoordSet[t];
		Internal_VA_TexCoordPtr(_VB.m_pTV[iTVSet], _VB.m_nTVComp[iTVSet], t);
	}
}

/*void CRenderContextPS3::Internal_VAIndxVert_Begin()
{
	if (!m_Geom.m_pCol) MACRO_GLCOLOR4F_INT32(m_GeomColor);
	Internal_VA_SetArrays(m_Geom);

#ifdef CRCGL_CVA
	if (m_ExtensionsActive & CRCGL_EXT_COMPILEDVERTEXARRAYS)
	{
		if (m_VAEnable & CRCGL_VA_CVA_LOCK)
			gleUnlockArraysEXT();

		gleLockArraysEXT(0, m_Geom.m_nV);
		GLErr("Internal_VAIndxVert_Begin (glLockArraysEXT)");
		m_VAEnable |= CRCGL_VA_CVA_LOCK;
	}
#endif
}
*/

int CRenderContextPS3::Internal_VAIndx(int _glPrimType, const uint16* _piVerts, int _nVerts, int _Offset)
{
	// Return the number of calls to glnDrawElements
	if (!_Offset)
	{
		glnDrawElements(_glPrimType, _nVerts, GL_UNSIGNED_SHORT, _piVerts);
		return 1;
	}
	else
	{
//OutputDebugString(CStrF(", %d/%d", _nVerts, _Offset));

		uint16 liVerts[2048*3];

		int nDE = 0;
		while(_nVerts)
		{
			int nV = Min(2048*3, _nVerts);
			for(int i = 0; i < nV; i++)
				liVerts[i] = _piVerts[i] + _Offset;

			glnDrawElements(_glPrimType, nV, GL_UNSIGNED_SHORT, liVerts);
			nDE++;

			_piVerts += nV;
			_nVerts -= nV;
		}

		return nDE;
	}
}

void CRenderContextPS3::Internal_VAIndxPrimitives(uint16* _pPrimStream, int _StreamLen, int _Offset)
{
	if (!_pPrimStream) return;
#ifdef CRCGL_CONVERT_PRIMITIVES_TO_TRIANGLES
	uint16 lTriIndices[1024*3];
	CRCPrimStreamIterator StreamIterate(_VB.m_piPrim, _VB.m_nPrim);
	
	while(StreamIterate.IsValid())
	{
		int nTriIndices = 1024*3;
		bool bDone = Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
		if (nTriIndices)
		{
			Internal_VAIndxTriangles(lTriIndices, nTriIndices/3, 0);
		}
		
		if (bDone) break;
	}
#else

	CRCPrimStreamIterator Iter(_pPrimStream, _StreamLen);

	int nV = 0;
	const uint16* piV = NULL;
	int iPrim = 0;

	if (!Iter.IsValid())
		return;

	while(1)
	{
		int Prim = Iter.GetCurrentType();
		const uint16* pPrim = Iter.GetCurrentPointer()-1;

		switch(Prim)
		{
		case CRC_RIP_TRIANGLES :
			{
				iPrim = GL_TRIANGLES;
				m_nTriangles += pPrim[1];
				m_nTriTotal += pPrim[1];
				nV = pPrim[1]*3;
				piV = &pPrim[2];
			}
			break;
		case CRC_RIP_QUADS :
			{
				iPrim = GL_QUADS;
				int nQuads = pPrim[1];
				m_nPolygons += nQuads;
				m_nTriTotal += nQuads*2;
				piV = &pPrim[2];
				nV = nQuads*4;
			}
			break;
		case CRC_RIP_TRISTRIP :
		case CRC_RIP_TRIFAN :
		case CRC_RIP_QUADSTRIP :
		case CRC_RIP_POLYGON :
			{
				nV = pPrim[1];
				piV = &pPrim[2];

				if (Prim == CRC_RIP_QUADSTRIP)
				{
					iPrim = GL_QUAD_STRIP;
					m_nPolygons += nV-3;
				}
				else if (Prim == CRC_RIP_POLYGON)
				{
					iPrim = CRCGL_POLYGONENUM;
					m_nPolygons++;
				}
				else
				{
					m_nTriangles += nV-2;
					m_nTriTotal += nV-2;
					iPrim = (Prim == CRC_RIP_TRISTRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLE_FAN;
				}

			}
			break;
		default :
			{
				piV = NULL;
				break;
			}
		}

		if (piV)
		{
			m_nStateVADrawElements += Internal_VAIndx(iPrim, piV, nV, _Offset);

//			glnDrawElements(iPrim, nV, GL_UNSIGNED_SHORT, piV);
		}

		if (!Iter.Next())
			break;
	}
#endif
}

void CRenderContextPS3::Internal_VAIndx(int _glPrimType, const uint16* _piVerts, int _nVerts, uint16 _Min, uint16 _Max)
{
	m_nStateVADrawElements++;
	glDrawRangeElements(_glPrimType, _Min, _Max, _nVerts, GL_UNSIGNED_SHORT, _piVerts);
}

void CRenderContextPS3::Internal_VAIndxTriangles(uint16* _pPrim, int _nTri)
{
	m_nStateVADrawElements += Internal_VAIndx(GL_TRIANGLES, _pPrim, _nTri*3, 0);
}

// -------------------------------------------------------------------
void CRenderContextPS3::Internal_VBIndxTriangles(int _VBID, uint16* _piPrim, int _nTri)
{
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();
	if (m_bUpdateVPConst) VP_Update();

	{
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		if (!(IDInfo.m_Fresh & 1))
			VB_Create(_VBID);

		spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
		if (!spVBI)
		{
			ConOut("(CRenderContextPS3::Internal_VBIndxTriangles) VB_Create failed.");
			return;
		}

		if (spVBI->IsEmpty())
			return;

		VBO_RenderVB_IndexedTriangles(_VBID, _piPrim, _nTri);
	}

//	m_nTriangles += spVBI->m_nTri;
//	m_nTriTotal += spVBI->m_nTri;
}

void CRenderContextPS3::Internal_VBIndxPrimitives(int _VBID, uint16* _piPrim, int _nPrim)
{
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();
	if (m_bUpdateVPConst) VP_Update();

	{
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		if (!(IDInfo.m_Fresh & 1))
			VB_Create(_VBID);

		spCRCGL_VBInfo spVBI = m_lspVB[_VBID];
		if (!spVBI)
		{
			ConOut("(CRenderContextPS3::Internal_VBIndxPrimitives) VB_Create failed.");
			return;
		}

		if (spVBI->IsEmpty())
			return;

		VBO_RenderVB_IndexedPrimitives(_VBID, _piPrim, _nPrim);
	}

//	m_nTriangles += spVBI->m_nTri;
//	m_nTriTotal += spVBI->m_nTri;
}

