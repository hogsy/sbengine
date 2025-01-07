#ifndef _INC_XW2COMMON_VPUSHARED
#define _INC_XW2COMMON_VPUSHARED


#define XW_FACE2_INVALIDINDEX 0xffffff

class CBSP2_CoreFace
{
public:
	uint32 m_iiEdges : 24;
	uint32 m_iDiameterClass : 8;

	uint32 m_iiVertices : 24;	// Index to first vertex-index
	uint32 m_nVertices : 8;		// Number vertices

	uint32 m_iMapping : 20;
	uint32 m_iBackMedium : 12;	// What's behind the face.

	uint32 m_iiNormals;			// Index to first normal-index

	uint32 m_iLightInfo:28;		// Index of first lightmap/lightvertices
	uint32 m_nLightInfo:4;

	uint32 m_iPlane;

	uint16 m_iSurface;
	uint16 m_Flags;

	CBSP2_CoreFace();
#ifndef M_COMPILING_ON_VPU
	void Read(CCFile* _pF, int _Version);
	void Write(CCFile* _pF);

#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
#endif
};

class CBSP2_Face : public CBSP2_CoreFace
{
public:
	//	uint32 m_iTVertices;	// Texture coordinate start index
	//	uint32 m_iLMTVertices;	// Lightmap texture coordinate start index
	uint16 m_iiVBVertices;
	//	uint16 m_nVBVertices
	int16 m_iVB;

	uint32 m_nPhysV : 8;
	uint32 m_iiPhysV : 24;

	CBSP2_Face()
	{
		//		m_iTVertices = 0;
		//		m_iLMTVertices = 0;
		m_iiVBVertices = 0;
		//		m_nVBVertices = 0;
		m_iVB = -1;
		m_nPhysV = 0;
		m_iiPhysV = 0;
	}
};


// -------------------------------------------------------------------
class CBSP2_Edge
{
public:
	uint32 m_iV[2];		// Start and End vertex for this edge

	CBSP2_Edge() {};

	CBSP2_Edge(int _iv0, int _iv1)
	{
		m_iV[0] = _iv0;
		m_iV[1] = _iv1;
	}

#ifndef M_COMPILING_ON_VPU
	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iV[0]);
		_pF->ReadLE(m_iV[1]);
	}

	void Write(CCFile* _pF)
	{
		_pF->WriteLE(m_iV[0]);
		_pF->WriteLE(m_iV[1]);
	}
#endif
};

// -------------------------------------------------------------------
class CBSP2_EdgeFaces
{
public:
	uint32 m_iFaces[2];	// Faces using this edge.  0x80000000 == no face, 0x40000000 == used backwards/flipped

	CBSP2_EdgeFaces()
	{
		m_iFaces[0] = 0x80000000;
		m_iFaces[1] = 0x80000000;
	}

	int IsValid(int _iRef) { return (m_iFaces[_iRef] & 0x80000000) ? 0 : 1; };
	int GetFlip(int _iRef) { return (m_iFaces[_iRef] & 0x40000000) ? 1 : 0; };
	int GetFace(int _iRef) { return m_iFaces[_iRef] & ~(0x40000000); };
	void SetFace(int _iRef, int _iFace, int _bFlip) { m_iFaces[_iRef] = _iFace | ((_bFlip) ? 0x40000000 : 0); };
	
#ifndef M_COMPILING_ON_VPU
	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iFaces[0]);
		_pF->ReadLE(m_iFaces[1]);
	}

	void Write(CCFile* _pF)
	{
		_pF->WriteLE(m_iFaces[0]);
		_pF->WriteLE(m_iFaces[1]);
	}
#endif
};


#endif // _INC_XW2COMMON_VPUSHARED
