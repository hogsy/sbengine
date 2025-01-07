#ifndef __XRVERTEXBUFFER_VPUSHARED_H
#define __XRVERTEXBUFFER_VPUSHARED_H

#include "../MSystem/Raster/MRender_Classes_VPUShared.h"


#define CRC_RIP_STREAM				-1
#define CRC_RIP_WIRES				-2
#define CRC_RIP_VBID				-3
#define CRC_RIP_VBID_IBTRIANGLES	-4


class CXR_VBManager;


class CXR_VBChain : public CRC_VertexBuffer 
{
public:
	CXR_VBChain* m_pNextVB;
	uint16* m_piVertUse;
	int32 m_nVertUse;
	uint16 m_TaskId;

	void Clear()
	{
		CRC_VertexBuffer::Clear();
		m_pNextVB = NULL;
		m_piVertUse = NULL;
		m_nVertUse = 0;
		m_TaskId = 0xffff;
	}

	bool BuildVertexUsage(CXR_VBManager* _pVBM);

	bool IsValid()
	{
		if (m_TaskId!=InvalidVpuTask)
			return true;
		if (!m_piPrim || !m_nPrim) 
			return false;
		if (!m_pV) 
			return false;

		return true;
	}

	void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
	{
		m_piPrim = _pTriVertIndices;
		m_nPrim = _nTriangles;
		m_PrimType = CRC_RIP_TRIANGLES;
	}

	void Render_IndexedWires(uint16* _pIndices, int _Len)
	{
		m_piPrim = _pIndices;
		m_nPrim = _Len;
		m_PrimType = CRC_RIP_WIRES;
	};

	void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
	{
		m_piPrim = _pPrimStream;
		m_nPrim = _StreamLen;
		m_PrimType = CRC_RIP_STREAM;
	}
};


#endif
