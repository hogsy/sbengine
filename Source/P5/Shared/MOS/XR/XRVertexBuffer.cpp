
#include "PCH.h"


#include "XRVertexBuffer.h"
#include "XRVBManager.h"
#include "../MOS.h"
#include "../MSystem/Raster/MRCCore.h"

//#define CXR_VB_LOGATTRIBUTES

// -------------------------------------------------------------------
//  CXR_VertexBuffer
// -------------------------------------------------------------------

bool CXR_VertexBuffer::SetVBChain(CXR_VBManager *_pVBM, bool _bVBIDs)
{

	if (_bVBIDs)
	{
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBIDCHAIN)
			return true;
#ifdef M_Profile
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBCHAIN)
			M_BREAKPOINT;
#endif
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBCHAIN, "!");

		m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
		m_pVBChain = _pVBM->Alloc(sizeof(CXR_VBIDChain));
		if (!m_pVBChain)
			return false;
		CXR_VBIDChain* pChain = GetVBIDChain();
		pChain->Clear();
	}
	else
	{
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBCHAIN)
			return true;
#ifdef M_Profile
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBIDCHAIN)
			M_BREAKPOINT;
#endif

		m_Flags |= CXR_VBFLAGS_VBCHAIN;
		m_pVBChain = _pVBM->Alloc(sizeof(CXR_VBChain));
		if (!m_pVBChain)
			return false;
		CXR_VBChain* pChain = GetVBChain();
		pChain->Clear();
	}

	return true;
}

bool CXR_VertexBuffer::AllocTextureMatrix(CXR_VBManager *_pVBM)
{
	if (!m_pTextureTransform)
	{
		m_pTextureTransform = _pVBM->Alloc_TextureMatrixArray();
	}
	return m_pTextureTransform != 0;
}

void CXR_VertexBuffer::Clear()
{
	MAUTOSTRIP(CXR_VertexBuffer_Clear, MAUTOSTRIP_VOID);

	m_pVBChain = 0;
	m_Priority = 0;

	m_pAttrib = 0;
	m_pMatrixPaletteArgs = 0;
	m_pPreRender = 0;
	m_Flags = 0;
	m_iLight = 0;
	m_iVP	= 0;
	m_iClip = 0;
	m_pTransform = 0; 
	m_pTextureTransform = 0;
	m_Color = 0xffffffff;

}

bool CXR_VertexBuffer::IsValid()
{
	MAUTOSTRIP(CXR_VertexBuffer_IsValid, false);
	
#ifdef PLATFORM_PS2
	if( m_Flags & CXR_VBFLAGS_COLORBUFFER ) return true;
#endif
	
	if( m_Flags & CXR_VBFLAGS_PRERENDER ) 
		return true;
	
	if (!m_pAttrib) return false;
	if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
	{
		return GetVBIDChain()->IsValid();
	}
	else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
	{
		return GetVBChain()->IsValid();
	}
	else
		return false;

	return true;
}

void CXR_VertexBuffer::SetMatrix(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_VertexBuffer_SetMatrix, MAUTOSTRIP_VOID);
	_pRC->Matrix_SetMode(CRC_MATRIX_MODEL);
	if (m_pTransform) 
		_pRC->Matrix_Set(*m_pTransform);
	else
		_pRC->Matrix_SetUnit();

	if (m_pTextureTransform)
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			_pRC->Matrix_SetMode(CRC_MATRIX_TEXTURE + i);
			if (m_pTextureTransform[i]) 
				_pRC->Matrix_Set(*m_pTextureTransform[i]);
			else
				_pRC->Matrix_SetUnit();
		}
	}
	else
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			_pRC->Matrix_SetMode(CRC_MATRIX_TEXTURE + i);
			_pRC->Matrix_SetUnit();
		}
	}

	_pRC->Matrix_SetMode(CRC_MATRIX_MODEL);
	_pRC->Matrix_SetPalette(m_pMatrixPaletteArgs);
}

void CXR_VertexBuffer::SetAttrib(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_VertexBuffer_SetAttrib, MAUTOSTRIP_VOID);
	_pRC->Geometry_Color(m_Color);
	_pRC->Attrib_Set(*m_pAttrib);
}

int CXR_VertexBuffer::RenderGeometry(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_VertexBuffer_RenderGeometry, 0);
	MSCOPESHORT(CXR_VertexBuffer::RenderGeometry); //AR-SCOPE

	int nBuffers = 0;
	_pRC->Geometry_Color(m_Color);
	if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
	{
		CXR_VBIDChain *pChain = GetVBIDChain();
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_VBIDCHAIN)) == CXR_VBFLAGS_VBIDCHAIN, "");

		while (pChain)
		{
			if (pChain->m_VBID)
			{
				if (pChain->m_PrimType == CRC_RIP_VBID)
				{
					_pRC->Render_VertexBuffer(pChain->m_VBID);
				}
				else
				{
					_pRC->Geometry_VertexBuffer(pChain->m_VBID, !(m_Flags & CXR_VBFLAGS_TRACKVERTEXUSAGE));

					switch(pChain->m_PrimType)
					{
					case CRC_RIP_STREAM :
						{
							_pRC->Render_IndexedPrimitives(pChain->m_piPrim, pChain->m_nPrim);
							break;
						}
					case CRC_RIP_TRIANGLES :
						{
							_pRC->Render_IndexedTriangles(pChain->m_piPrim, pChain->m_nPrim);
							break;
						}
					case CRC_RIP_WIRES :
						{
							_pRC->Render_IndexedWires(pChain->m_piPrim, pChain->m_nPrim);
							break;
						}
					default :
						{
							ConOut(CStrF("CXR_VertexBuffer::Render) Invalid primitive type: %d", pChain->m_PrimType));
							break;
						}
					}

				}
			}
			else
			{
				M_ASSERT(0, "Dumma dig");
			}

			++nBuffers;
			pChain = pChain->m_pNextVB;
		}
	}
	else
	{
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_VBIDCHAIN)) == CXR_VBFLAGS_VBCHAIN, "");

		CXR_VBChain *pChain = GetVBChain();

		while (pChain)
		{
			_pRC->Geometry_VertexBuffer(*pChain, !(m_Flags & CXR_VBFLAGS_TRACKVERTEXUSAGE));
				
			#ifdef PLATFORM_DOLPHIN				
				if (pChain->m_pV)   DCStoreRange(pChain->m_pV, sizeof(CVec3Dfp4) * pChain->m_nV);
				if (pChain->m_pN)   DCStoreRange(pChain->m_pN, sizeof(CVec3Dfp4) * pChain->m_nV);
				if (pChain->m_pCol) DCStoreRange(pChain->m_pCol, sizeof(CPixel32) * pChain->m_nV);

				for (int i=0; i<CRC_MAXTEXCOORDS; i++)
					if (pChain->m_pTV[i])
						DCStoreRange( pChain->m_pTV[i], pChain->m_nTVComp[i]*sizeof(fp4) * pChain->m_nV );
			#endif

			switch(pChain->m_PrimType)
			{
			case CRC_RIP_STREAM :
				{
					_pRC->Render_IndexedPrimitives(pChain->m_piPrim, pChain->m_nPrim);
					break;
				}
			case CRC_RIP_TRIANGLES :
				{
					_pRC->Render_IndexedTriangles(pChain->m_piPrim, pChain->m_nPrim);
					break;
				}
			case CRC_RIP_WIRES :
				{
					_pRC->Render_IndexedWires(pChain->m_piPrim, pChain->m_nPrim);
					break;
				}
			default :
				{
					ConOut(CStrF("CXR_VertexBuffer::Render) Invalid primitive type: %d", pChain->m_PrimType));
					break;
				}
			}

			++nBuffers;
			pChain = pChain->m_pNextVB;
		}
	}

	return nBuffers;
}

int CXR_VertexBuffer::Render(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_VertexBuffer_Render, 0);
	SetAttrib(_pRC);
	SetMatrix(_pRC);
	return RenderGeometry(_pRC);
}

/*
int CXR_VertexBuffer::GetChainLen() const
{
	MAUTOSTRIP(CXR_VertexBuffer_GetChainLen, 0);

	if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
	{
		CXR_VBIDChain *pChain = GetVBIDChain();
		int Len = 0;
		while(pChain)
		{
			++Len;
			pChain = pChain->m_pNextVB;
		}
		return Len;
	}
	else
	{
		CXR_VBChain *pChain = GetVBChain();
		int Len = 0;
		while(pChain)
		{
			++Len;
			pChain = pChain->m_pNextVB;
		}
		return Len;
	}
}
*/
//#define ADDVERTEX(iv) if (iv >= m_nV) ConOut(CStrF("    %d/%d", iv, m_nV)); else if (!(pVMap[iv >> 3] & (1 << (iv & 7)))) { pVMap[iv >> 3] |= 1 << (iv & 7); m_piVertUse[nVU++] = iv; }
#define ADDVERTEX(iv) if (!(pVMap[iv >> 3] & (1 << (iv & 7)))) { pVMap[iv >> 3] |= 1 << (iv & 7); m_piVertUse[nVU++] = iv; }

bool CXR_VBChain::BuildVertexUsage(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CXR_VertexBuffer_BuildVertexUsage, false);

	if (m_piVertUse) 
		return true;

	{
		M_LOCK(_pVBM->m_AllocLock);
		if (_pVBM->GetAvail() < m_nV*2) 
			return false;

		uint8* pVMap = _pVBM->GetVertexUseMap(m_nV);
		m_piVertUse = (uint16*)_pVBM->Alloc_Open();

		int nVU = 0;
		switch(m_PrimType)
		{
		case CRC_RIP_STREAM :
			{
				CRCPrimStreamIterator StreamIterate(m_piPrim, m_nPrim);
		
				if (!StreamIterate.IsValid())
					break;
				
				while(1)
				{
					const uint16* pPrim = StreamIterate.GetCurrentPointer();

					int nV = 0;
					const uint16* piV = 0;
					switch(StreamIterate.GetCurrentType())
					{
					case CRC_RIP_TRIFAN :
					case CRC_RIP_TRISTRIP :
					case CRC_RIP_POLYGON :
					case CRC_RIP_QUADSTRIP:
						{
							nV = (*pPrim);
							piV = pPrim+1;
						}
						break;
					case CRC_RIP_TRIANGLES :
						{
							nV = (*pPrim)*3;
							piV = pPrim+1;
						}
						break;
					case CRC_RIP_QUADS :
						{
							nV = (*pPrim)*4;
							piV = pPrim+1;
						}
						break;
					default :
						{
							Error_static("CXR_VertexBuffer::BuildVertexUsage", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
							break;
						}
					}

					if (piV) 
						for(int v = 0; v < nV; v++)
							ADDVERTEX(piV[v]);

					if (!StreamIterate.Next())
						break;

				}
				break;
			}
		case CRC_RIP_TRIANGLES :
			{
				uint16* piV = m_piPrim;
				for(int t = 0; t < m_nPrim; t++)
				{
					ADDVERTEX(*piV) piV++;
					ADDVERTEX(*piV) piV++;
					ADDVERTEX(*piV) piV++;
				}
				break;
			}
		case CRC_RIP_WIRES :
			{
				uint16* piV = m_piPrim;
				for(int p = 0; p < m_nPrim; p++)
					ADDVERTEX(*piV) piV++;
				break;
			}

		default :
			{
				ConOut(CStrF("CXR_VertexBuffer::BuildVertexUsage) Invalid primitive type: %d", m_PrimType));
				break;
			}
		}

		m_nVertUse = nVU;

		// Clear working map
		for(int v = 0; v < m_nVertUse; v++)
			pVMap[m_piVertUse[v] >> 3] = 0;

	//if (m_nVertUse > m_nV)
	//	ConOut(CStrF("VertUse upknullad! %d/%d", m_nVertUse, m_nV));

		_pVBM->Alloc_Close(nVU*sizeof(uint16));
	}
	return true;
}

