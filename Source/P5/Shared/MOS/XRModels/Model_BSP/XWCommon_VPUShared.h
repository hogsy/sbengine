// -------------------------------------------------------------------
//  The XW Fileformat
// -------------------------------------------------------------------

#ifndef _INC_XWCOMMON_VPUSHARED
#define _INC_XWCOMMON_VPUSHARED


// -------------------------------------------------------------------
class CBSP_Edge
{
public:
	uint32 m_iV[2];		// Start and End vertex for this edge

	CBSP_Edge() {};

	CBSP_Edge(int _iv0, int _iv1)
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

	void SwapLE()
	{
		::SwapLE(m_iV[0]);
		::SwapLE(m_iV[1]);
	}
#endif

};

// -------------------------------------------------------------------
class CBSP_EdgeFaces
{
public:
	uint32 m_iFaces[2];	// Faces using this edge.  0x80000000 == no face, 0x40000000 == used backwards/flipped

	CBSP_EdgeFaces()
	{
		m_iFaces[0] = 0x80000000;
		m_iFaces[1] = 0x80000000;
	}

	int IsValid(int _iRef) const { return (m_iFaces[_iRef] & 0x80000000) ? 0 : 1; };
	int GetFlip(int _iRef) const { return (m_iFaces[_iRef] & 0x40000000) ? 1 : 0; };
	int GetFace(int _iRef) const { return m_iFaces[_iRef] & ~(0x40000000); };
	void SetFace(int _iRef, int _iFace, int _bFlip) { m_iFaces[_iRef] = _iFace | ((_bFlip) ? 0x40000000 : 0); };

#ifndef M_COMPILING_ON_VPU
	void Read(CCFile* _pF)
	{
		_pF->ReadLE(m_iFaces[0]);
		_pF->ReadLE(m_iFaces[1]);
	}

	void Write(CCFile* _pF) const
	{
		_pF->WriteLE(m_iFaces[0]);
		_pF->WriteLE(m_iFaces[1]);
	}

	void SwapLE()
	{
		::SwapLE(m_iFaces[0]);
		::SwapLE(m_iFaces[1]);
	}
#endif
};



#endif // _INC_XWCOMMON_VPUSHARED

