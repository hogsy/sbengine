#ifndef WDYNAMICSCONVEXCOLLISION_H
#define WDYNAMICSCONVEXCOLLISION_H

#define MINMAXINLINE M_FORCEINLINE

class CSolidPolyhedron
{
public:
	CSolidPolyhedron(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index,  const CMat4Dfp32& _Trans)
	{
		m_pIndexedSolids = _IndexedSolidContainer;
		m_Trans = _Trans;
		m_SolidIndex = _index;
		m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		m_nVertices = solid.m_nVertices;

		m_pIndexedSolids->m_lSolids.Len();
	}

	CSolidPolyhedron(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index, const CMat4Dfp32& _Trans, const CVec3Dfp32& _Offset)
	{
		m_pIndexedSolids = _IndexedSolidContainer;
		m_Trans = _Trans;
		m_Trans.GetRow(3) = _Offset * _Trans;
		m_SolidIndex = _index;
		m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		m_nVertices = solid.m_nVertices;

		m_pIndexedSolids->m_lSolids.Len();
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport) * _Trans;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		fp32 d = _P * GetVertex(0);
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			fp32 tmp = _P * GetVertex(i);
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport);
	}


	M_INLINE CVec3Dfp32 GetVertex(int index) const
	{
		//const CXR_IndexedSolid32& solid = m_spIndexedSolids->m_lSolids[m_SolidIndex];
		//CXR_IndexedSolidDesc32 soliddesr;
		//m_pIndexedSolids->GetSolid(m_SolidIndex, soliddesr);

		//int nVert = solid.m_nVertices;
		//		CVec3Dfp32 ret = soliddesr.m_pV[soliddesr.m_piV[index]];
		CVec3Dfp32 ret = m_SolidDescr.m_pV[index];
		return ret * m_Trans;
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices;
		//const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		//return solid.m_nVertices;
	}

	void Dump()
	{
		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		CXR_IndexedSolidDesc32 soliddesr;
		m_pIndexedSolids->GetSolid(m_SolidIndex, soliddesr);

		int nVert = solid.m_nVertices;
		M_TRACEALWAYS("%d\n",nVert);
		for (int i = 0; i < nVert; i++)
		{
			M_TRACEALWAYS("%s %d\n",soliddesr.m_pV[i].GetString().Str(), soliddesr.m_piV[i]);
		}

	}

	void Render(CWireContainer *pWireContainer)
	{
		CMat4Dfp32 m;
		m.Unit();
		int nVerts = GetNumberOfVertices();
		for (int i = 0; i < nVerts; i++)
		{
			CVec3Dfp32::GetRow(m,3) = GetVertex(i);
			pWireContainer->RenderVertex(m.GetRow(3), CPixel32(220,0,0), 1.0/10.0, false);
		}
		//Dump();
	}

	CXR_IndexedSolidContainer32 *m_pIndexedSolids;
	CXR_IndexedSolidDesc32 m_SolidDescr;
	CMat4Dfp32 m_Trans;
	int m_SolidIndex;
	int m_nVertices;
};


class CSolidPolyhedron2
{
public:
	CSolidPolyhedron2(CXR_IndexedSolidDesc32 *_pSolidDescr, int _nVertices, const CMat4Dfp32& _Trans)
	{
		m_pSolidDescr = _pSolidDescr;
		m_nVertices = _nVertices;
		m_Trans = _Trans;
	}

	CSolidPolyhedron2(CXR_IndexedSolidDesc32 *_pSolidDescr, int _nVertices, const CMat4Dfp32& _Trans, const CVec3Dfp32& _Offset)
	{
		m_pSolidDescr = _pSolidDescr;
		m_nVertices = _nVertices;
		m_Trans = _Trans;
		m_Trans.GetRow(3) = _Offset * _Trans;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport) * _Trans;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		fp32 d = _P * GetVertex(0);
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			fp32 tmp = _P * GetVertex(i);
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport);
	}


	M_INLINE CVec3Dfp32 GetVertex(int index) const
	{
		CVec3Dfp32 ret = m_pSolidDescr->m_pV[index];
		return ret * m_Trans;
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices;
	}

	void Render(CWireContainer *pWireContainer)
	{
		CMat4Dfp32 m;
		m.Unit();
		int nVerts = GetNumberOfVertices();
		for (int i = 0; i < nVerts; i++)
		{
			CVec3Dfp32::GetRow(m,3) = GetVertex(i);
			pWireContainer->RenderVertex(m.GetRow(3), CPixel32(220,0,0), 1.0/10.0, false);
		}


		TAP_RCD<const CXR_IndexedSolidFace32> pF = m_pSolidDescr->m_pF;

//		M_TRACEALWAYS("-----------------------------------\n");

		for (int i = 0; i < pF.Len(); i++)
		{
			const CXR_IndexedSolidFace32& Face = pF[i];
			int iPlane = Face.m_iPlane;
			CVec3Dfp32 Normal = m_pSolidDescr->m_pP[iPlane].n;



			TAP_RCD<const uint16> piV = m_pSolidDescr->m_piV;

			CVec3Dfp32 Center(0.0f);
			//M_TRACEALWAYS("Face\n");

			for (int iV = 0; iV < Face.m_nV; iV++)
			{
				int iVertexIndex = piV[Face.m_iiV + iV];
				//M_TRACEALWAYS("     %s\n", m_pSolidDescr->m_pV[iVertexIndex].GetString().Str());

				Center += m_pSolidDescr->m_pV[iVertexIndex];
			}
			Center *= (1.0f / Face.m_nV);

			//M_TRACEALWAYS("%s\n", Normal.GetString().Str());


			Center *= m_Trans;
			Normal.MultiplyMatrix3x3(m_Trans);

//			pWireContainer->RenderVector(Center, Normal * 16.0f, CPixel32(0,0,220), 1.0f / 20.0f, false);

			CVec3Dfp32 v1 = m_pSolidDescr->m_pV[piV[Face.m_iiV + 0]];
			CVec3Dfp32 v2 = m_pSolidDescr->m_pV[piV[Face.m_iiV + 1]];
			CVec3Dfp32 v3 = m_pSolidDescr->m_pV[piV[Face.m_iiV + 2]];

			CPlane3Dfp32 fooplane1(v3, v2, v1);

			v1 *= m_Trans;
			v2 *= m_Trans;
			v3 *= m_Trans;

			CPlane3Dfp32 fooplane2(v3, v2, v1);

			pWireContainer->RenderVector(Center, fooplane2.n * 16.0f, CPixel32(0,220,220), 1.0f / 20.0f, false);


			//M_TRACEALWAYS("%s\n", fooplane1.GetString().Str());

			//fooplane.n.MultiplyMatrix3x3(m_Trans);

			int breakme = 0;


		}



		//Dump();
	}

	CXR_IndexedSolidDesc32 *m_pSolidDescr;
	CMat4Dfp32 m_Trans;
	int m_nVertices;
};


class CExtrudedSolidPolyhedronIndirectIndexed 
{
public:
	CExtrudedSolidPolyhedronIndirectIndexed (const CVec3Dfp32 *_pVertices, const uint32 *_piVertices, int _nVertices, const CVec3Dfp32& _Translate)
	{
		m_pVertices = _pVertices;
		m_piVertices = _piVertices;
		m_nVertices = _nVertices;
		m_Translate = _Translate;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport) * _Trans;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		fp32 d = _P * GetVertex(0);
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			fp32 tmp = _P * GetVertex(i);
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport);
	}

	M_INLINE CVec3Dfp32 GetVertex(int index) const
	{
		if (index < m_nVertices)
			return m_pVertices[m_piVertices[index]];
		else
			return m_pVertices[m_piVertices[index - m_nVertices]] + m_Translate;
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices * 2;
	}

protected:
	const CVec3Dfp32 *m_pVertices;
	const uint32 *m_piVertices;
	CVec3Dfp32 m_Translate;
	int m_nVertices;
};

const int CSolidPolyhedronIndirectIndexedSeparating_MaxVert = 100;

class CSolidPolyhedronIndirectIndexedSeparating 
{
public:
	CSolidPolyhedronIndirectIndexedSeparating (const CVec3Dfp32 *_pVertices, const uint32 *_piVertices, int _nVertices, const CPlane3Dfp32& _Plane)
	{
		//m_pVertices = _pVertices;
		//m_piVertices = _piVertices;

#ifndef M_RTM
		if (_nVertices > CSolidPolyhedronIndirectIndexedSeparating_MaxVert)
		{
			M_ASSERT(false, "Too many vertices in polygon");
		}
#endif

		m_nVertices = Min(_nVertices, CSolidPolyhedronIndirectIndexedSeparating_MaxVert);

		CVec3Dfp32 Center(0.0f);
		for (int i = 0; i < m_nVertices; i++)
		{
			Center += _pVertices[_piVertices[i]];
		}
		Center *= 1.0f / m_nVertices;

		for (int i = 0; i < m_nVertices; i++)
		{
			m_lVertices[i] = _pVertices[_piVertices[i]] - Center;
		}

		m_Center = Center;

		m_Plane = _Plane;

		//CPlane3Dfp32 P(m_lVertices[2], m_lVertices[1], m_lVertices[0]);
		//m_Normal = P.n;

//		CPlane3Dfp32 P(_pVertices[_piVertices[2]], _pVertices[_piVertices[1]], _pVertices[_piVertices[0]]);
//		m_Normal = P.n;
	}


	M_FORCEINLINE int GetColinearFaceDirectionCount() const
	{
		return 1;
	}

	CVec3Dfp32 GetCenter() const
	{
		return m_Center;
	}

	M_FORCEINLINE CVec3Dfp32 GetColinearFaceDirection(int i, const CMat4Dfp32& _T, bool& _HasOpposite) const
	{
		_HasOpposite = false;
		CVec3Dfp32 n = m_Plane.n;
		n.MultiplyMatrix3x3(_T);
		return n;
	}

	M_FORCEINLINE int GetClosestFace(const CMat4Dfp32& _Transform, const CVec3Dfp32& _Normal) const
	{
		return 0;
	}

	M_INLINE int GetColinearEdgeCount() const
	{
		return 0;
	}

	M_INLINE void GetColinearEdge(int i, const CMat4Dfp32& _T, CVec3Dfp32& _Edge) const
	{
		M_ASSERT(false, "!");
	}

	MINMAXINLINE void GetMinMax(const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		int nV = GetNumberOfVertices();

		fp32 DaMin = _FP32_MAX;
		fp32 DaMax = -_FP32_MAX;

		int iv = 0;

		for (iv = 0; iv < nV; iv++)
		{
			const CVec3Dfp32& p = GetVertex(iv);
			fp32 sp = _Axis * p;

			DaMin = Min(DaMin, sp);
			DaMax = Max(DaMax, sp);
		}

		_Min = DaMin;
		_Max = DaMax;
	}

	MINMAXINLINE void GetMinMax(const CMat4Dfp32& _T, const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		int nV = GetNumberOfVertices();

		fp32 DaMin = _FP32_MAX;
		fp32 DaMax = -_FP32_MAX;

		int iv = 0;

		for (iv = 0; iv < nV; iv++)
		{
			const CVec3Dfp32& p = GetVertex(iv);
			fp32 sp = _Axis * (p * _T);

			DaMin = Min(DaMin, sp);
			DaMax = Max(DaMax, sp);
		}

		_Min = DaMin;
		_Max = DaMax;
	}

	M_INLINE void Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans, CVec3Dfp32& _SupportVertex) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		_SupportVertex = GetVertex(iSupport) * _Trans;
	}

	M_INLINE int GetFace(int _iF, const CMat4Dfp32& _T, CVec3Dfp32 *_pVertices) const
	{
		int N = GetNumberOfVertices();

		for (int i = 0; i < N; i++)
		{
			_pVertices[i] = GetVertex((N - 1) - i) * _T;
		}

		return N;
	}

	M_INLINE int GetFace(int _iF, const CMat4Dfp32& _T, CVec3Dfp32 *_pVertices, CPlane3Dfp32& _Plane) const
	{
		int N = GetNumberOfVertices();

		for (int i = 0; i < N; i++)
		{
			_pVertices[i] = GetVertex((N - 1) - i) * _T;
		}

		_Plane = m_Plane;
		return N;
	}

	M_INLINE const CVec3Dfp32& GetVertex(int index) const
	{
//		return m_pVertices[m_piVertices[index]];
		return m_lVertices[index];
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices;
	}

protected:
	//const CVec3Dfp32 *m_pVertices;
	//const uint32 *m_piVertices;

	CVec3Dfp32 m_Center;
	CVec3Dfp32 m_lVertices[CSolidPolyhedronIndirectIndexedSeparating_MaxVert];
	int m_nVertices;
	CPlane3Dfp32 m_Plane;
};

class CSolidPolyhedronSeparating
{
public:
	/*CSolidPolyhedronSeparating(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index,  const CMat4Dfp32& _Trans)
	{
	m_pIndexedSolids = _IndexedSolidContainer;
	m_Trans = _Trans;
	m_SolidIndex = _index;
	m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

	const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
	m_nVertices = solid.m_nVertices;

	m_pIndexedSolids->m_lSolids.Len();
	}*/

	/*	CSolidPolyhedronSeparating(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index, const CMat4Dfp32& _Trans, const CVec3Dfp32& _Offset)
	{
	m_pIndexedSolids = _IndexedSolidContainer;
	m_Trans = _Trans;
	m_Trans.GetRow(3) = _Offset * _Trans;
	m_SolidIndex = _index;
	m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

	const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
	m_nVertices = solid.m_nVertices;

	m_pIndexedSolids->m_lSolids.Len();
	}
	*/

	CSolidPolyhedronSeparating(CXR_IndexedSolidDesc32 *_pSolidDescr, int _nVertices)
	{
		m_pSolidDescr = _pSolidDescr;
		m_nVertices = _nVertices;
		//		m_Trans = _Trans;
	}

	M_FORCEINLINE int GetColinearFaceDirectionCount() const
	{
		return m_pSolidDescr->m_piColinearP.Len();
		//return m_pPolyhedron->GetUniqueFaceCount();
	}

	M_FORCEINLINE CVec3Dfp32 GetColinearFaceDirection(int i, const CMat4Dfp32& _T, bool& _HasOpposite) const
	{
		//			return m_pPolyhedron->GetUniqueNormal(i, _T);
		//return m_pPolyhedron->GetUniqueNormalFast(i, _T);
		_HasOpposite = true;
		int index = m_pSolidDescr->m_piColinearP[i];

		CPlane3Dfp32 p = m_pSolidDescr->m_pP[index];
		p.n.MultiplyMatrix3x3(_T);
		return p.n;
	}

	int GetClosestFace(const CMat4Dfp32& _Transform, const CVec3Dfp32& _Normal) const
	{
		int nFaces = m_pSolidDescr->m_pF.Len();
		TAP_RCD<const CXR_IndexedSolidFace32> pF = m_pSolidDescr->m_pF;
		TAP_RCD<const CPlane3Dfp32> pP = m_pSolidDescr->m_pP;

		int iBest = -1;
		fp32 BestDistance = -1;

		for (int i = 0; i < nFaces; i++)
		{
			int iPlane = pF[i].m_iPlane;
			CVec3Dfp32 Normal = pP[iPlane].n;
			Normal.MultiplyMatrix3x3(_Transform);

			fp32 D = Normal * _Normal;
			if (D > BestDistance)
			{
				iBest = i;
				BestDistance = D;
			}
		}
		return iBest;
		//		return m_pPolyhedron->GetClosestFace(_Transform, _Normal);
	}

	M_INLINE int GetColinearEdgeCount() const
	{
		return m_pSolidDescr->m_pColinearE.Len();
		//		return m_pPolyhedron->GetUniqueEdgeCount();
	}

	M_INLINE void GetColinearEdge(int i, const CMat4Dfp32& _T, CVec3Dfp32& _Edge) const
	{
		//int index = m_pSolidDescr->m_piColinearP[i];
		const CIndexedEdge16& edge = m_pSolidDescr->m_pColinearE[i];

		//const CIndexedEdge16& edge = m_pSolidDescr->m_pE[index];

		CVec3Dfp32 v1 = m_pSolidDescr->m_pV[edge.m_liV[0]];
		CVec3Dfp32 v2 = m_pSolidDescr->m_pV[edge.m_liV[1]];

		_Edge = (v2 - v1);
		_Edge.Normalize();
		_Edge.MultiplyMatrix3x3(_T);

		//m_pPolyhedron->GetUniqueEdge(i, _T, _Edge);
	}

	M_INLINE void Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans, CVec3Dfp32& _SupportVertex) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		_SupportVertex = GetVertex(iSupport) * _Trans;
		//return GetVertex(iSupport) * _Trans;
	}

	MINMAXINLINE void GetMinMax(const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		int nV = GetNumberOfVertices();

		fp32 DaMin = _FP32_MAX;
		fp32 DaMax = -_FP32_MAX;

		int iv = 0;

		for (iv = 0; iv < nV; iv++)
		{
			const CVec3Dfp32& p = GetVertex(iv);
			fp32 sp = _Axis * p;

			DaMin = Min(DaMin, sp);
			DaMax = Max(DaMax, sp);
		}

		_Min = DaMin;
		_Max = DaMax;
	}

	MINMAXINLINE void GetMinMax(const CMat4Dfp32& _T, const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		int nV = GetNumberOfVertices();

		fp32 DaMin = _FP32_MAX;
		fp32 DaMax = -_FP32_MAX;

		int iv = 0;
		for (iv = 0; iv < nV; iv++)
		{
			const CVec3Dfp32& p = GetVertex(iv);
			fp32 sp = _Axis * (p * _T);

			DaMin = Min(DaMin, sp);
			DaMax = Max(DaMax, sp);
		}

		_Min = DaMin;
		_Max = DaMax;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		fp32 d = _P * GetVertex(0);
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			fp32 tmp = _P * GetVertex(i);
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport);
	}

	void GetFace(int _iF, const CMat4Dfp32& _T, CVec3Dfp32& _v1, CVec3Dfp32& _v2, CVec3Dfp32& _v3, CVec3Dfp32& _v4) const
	{
		const CXR_IndexedSolidFace32& Face = m_pSolidDescr->m_pF[_iF];

		TAP_RCD<const uint16> piV = m_pSolidDescr->m_piV;

		int i1 = piV[Face.m_iiV + 3];
		int i2 = piV[Face.m_iiV + 2];
		int i3 = piV[Face.m_iiV + 1];
		int i4 = piV[Face.m_iiV + 0];

		_v1 = m_pSolidDescr->m_pV[i1];
		_v2 = m_pSolidDescr->m_pV[i2];
		_v3 = m_pSolidDescr->m_pV[i3];
		_v4 = m_pSolidDescr->m_pV[i4];

		_v1 *= _T;
		_v2 *= _T;
		_v3 *= _T;
		_v4 *= _T;

		//m_pPolyhedron->GetFace(_iF, _T, _v1, _v2, _v3, _v4);
	}

	int GetFace(int _iF, const CMat4Dfp32& _T, CVec3Dfp32 *_pVertices, CPlane3Dfp32& _Plane) const
	{
		const CXR_IndexedSolidFace32& Face = m_pSolidDescr->m_pF[_iF];

		TAP_RCD<const uint16> piV = m_pSolidDescr->m_piV;

		int N = Face.m_nV;
		for (int i = 0; i < N; i++)
		{
			int index = piV[Face.m_iiV + ((N - 1) -i)];
			CVec3Dfp32 v = m_pSolidDescr->m_pV[index];
			v *= _T;
			_pVertices[i] = v;
		}

		_Plane = CPlane3Dfp32(_pVertices[0], _pVertices[1], _pVertices[2]);

		return N;
	}


	M_INLINE const CVec3Dfp32& GetVertex(int index) const
	{
		return m_pSolidDescr->m_pV[index];
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices;
		//const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		//return solid.m_nVertices;
	}

	void Dump()
	{
		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		CXR_IndexedSolidDesc32 soliddesr;
		m_pIndexedSolids->GetSolid(m_SolidIndex, soliddesr);

		int nVert = solid.m_nVertices;
		M_TRACEALWAYS("%d\n",nVert);
		for (int i = 0; i < nVert; i++)
		{
			M_TRACEALWAYS("%s %d\n",soliddesr.m_pV[i].GetString().Str(), soliddesr.m_piV[i]);
		}

	}

	void Render(CWireContainer *pWireContainer)
	{
		CMat4Dfp32 m;
		m.Unit();
		int nVerts = GetNumberOfVertices();
		for (int i = 0; i < nVerts; i++)
		{
			CVec3Dfp32::GetRow(m,3) = GetVertex(i);
			pWireContainer->RenderVertex(m.GetRow(3), CPixel32(220,0,0), 1.0/10.0, false);
		}
		//Dump();
	}

	CXR_IndexedSolidDesc32 *m_pSolidDescr;

	CXR_IndexedSolidContainer32 *m_pIndexedSolids;
	CXR_IndexedSolidDesc32 m_SolidDescr;
	//CMat4Dfp32 m_Trans;
	int m_SolidIndex;
	int m_nVertices;
};


class CSolidBoxSeparating
{
public:

	CVec3Dfp32 m_Scale;

	CSolidBoxSeparating(const CVec3Dfp32 &_Scale)
	{
		m_Scale = _Scale;
	}

	M_FORCEINLINE int GetColinearFaceDirectionCount() const
	{
		return 3;
	}

	M_FORCEINLINE CVec3Dfp32 GetColinearFaceDirection(int i, const CMat4Dfp32& _T, bool& _HasOpposite) const
	{
		_HasOpposite = true;

		return _T.GetRow(i);
	}

	int GetClosestFace(const CMat4Dfp32& _Transform, const CVec3Dfp32& _Normal) const
	{
		int iBest = -1;
		fp32 BestDistance = 0;

		for (int i = 0; i < 3; i++)
		{
			CVec3Dfp32 Axis = _Transform.GetRow(i);

			fp32 D = Axis * _Normal;
			if (Abs(D) > Abs(BestDistance))
			{
				iBest = i;
				BestDistance = D;
			}
		}
		return (BestDistance < 0) ? iBest+3 : iBest;
	}

	M_INLINE int GetColinearEdgeCount() const
	{
		return 3;
	}

	M_INLINE void GetColinearEdge(int i, const CMat4Dfp32& _T, CVec3Dfp32& _Edge) const
	{
		_Edge = _T.GetRow(i);
	}

	M_INLINE void Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans, CVec3Dfp32& _SupportVertex) const
	{
		CVec3Dfp32 Tmp(0,0,0);

		//Get extreme vertex, one axis at a time
		for(int i = 0;i < 3;i++)
		{
			CVec3Dfp32 Axis = _Trans.GetRow(i);
			fp32 Proj = Axis * _P;
			if( Proj < 0 ) Tmp -= Axis * m_Scale.k[i];
			else Tmp += Axis * m_Scale.k[i];
		}

		_SupportVertex = Tmp + _Trans.GetRow(3);
	}

	MINMAXINLINE void GetMinMax(const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		fp32 ProjLen = Abs(_Axis.k[0] * m_Scale.k[0]) + 
			Abs(_Axis.k[1] * m_Scale.k[1]) + 
			Abs(_Axis.k[2] * m_Scale.k[2]);

		_Min = -ProjLen;
		_Max = ProjLen;
	}

	MINMAXINLINE void GetMinMax(const CMat4Dfp32& _T, const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		fp32 ProjLen = Abs(_Axis * _T.GetRow(0) * m_Scale.k[0]) + 
			Abs(_Axis * _T.GetRow(1) * m_Scale.k[1]) + 
			Abs(_Axis * _T.GetRow(2) * m_Scale.k[2]);
		fp32 ProjCen = _Axis * _T.GetRow(3);

		_Min = ProjCen-ProjLen;
		_Max = ProjCen+ProjLen;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		CVec3Dfp32 Tmp(0,0,0);

		//Get extreme vertex, one axis at a time
		for(int i = 0;i < 3;i++)
		{
			if( _P.k[i] < 0 ) Tmp -= m_Scale.k[i];
			else Tmp += m_Scale.k[i];
		}

		return Tmp;
	}

	int GetFace(int _iF, const CMat4Dfp32 &_T, CVec3Dfp32 * _pVertices, CPlane3Dfp32 &_Plane) const
	{
		CVec3Dfp32 Main,Sec,Thr;
		uint8 iSec[2];
		if( _iF >= 3 )
		{
			Main = - _T.GetRow(_iF-3) * m_Scale.k[_iF-3];
			iSec[0] = ((_iF-3) == 0) ? 2 : _iF-4;
			iSec[1] = ((_iF-3) == 2) ? 0 : _iF-2;
		}
		else
		{
			Main = _T.GetRow(_iF) * m_Scale.k[_iF];
			iSec[0] = (_iF == 2) ? 0 : _iF+1;
			iSec[1] = (_iF == 0) ? 2 : _iF-1;
		}
		Main += _T.GetRow(3);
		Sec = _T.GetRow(iSec[0]) * m_Scale.k[iSec[0]];
		Thr = _T.GetRow(iSec[1]) * m_Scale.k[iSec[1]];

		_pVertices[0] = Main + Sec + Thr;
		_pVertices[1] = Main - Sec + Thr;
		_pVertices[2] = Main - Sec - Thr;
		_pVertices[3] = Main + Sec - Thr;

		_Plane = CPlane3Dfp32(_pVertices[0], _pVertices[1], _pVertices[2]);
		return 4;
	}
};


#define PHYSCAPSULE_FACESTACKLEN	16

class CSolidCapsuleSeparating
{
public:

	fp32 m_Len;
	fp32 m_Radius;

	//Used to create colinear face-directions
	const CVec3Dfp32 * m_pV;
	uint32 m_nV;
	const CMat4Dfp32 * m_pVMat;

	//Face stack
	CVec3Dfp32	m_lFaceStack[PHYSCAPSULE_FACESTACKLEN];
	uint16		m_iWritePos;

	CSolidCapsuleSeparating(const CVec3Dfp32 &_Scale,const CVec3Dfp32* _pV,
		uint32 _nV,const CMat4Dfp32* _pMat)
	{
		//Dim
		m_Len = _Scale.k[0];
		m_Radius = _Scale.k[1];

		//Setup Colinear face direction- function
		m_pV = _pV;
		m_nV = _nV;
		m_pVMat = _pMat;

		//Reset facestack
		m_iWritePos = 0;
	}

	M_FORCEINLINE int GetColinearFaceDirectionCount() const
	{
		return m_nV;
	}

	M_FORCEINLINE CVec3Dfp32 GetColinearFaceDirection(int i, const CMat4Dfp32& _T, bool& _HasOpposite)
	{
		_HasOpposite = true;

		//The colinear faces reach to the vertices from the closest points on the capsule

		//Get vertex & Capsule specs
		CVec3Dfp32 Vtx = m_pV[i];
		Vtx.MultiplyMatrix(*m_pVMat);		
		const CVec3Dfp32 &Cen = _T.GetRow(3);
		const CVec3Dfp32 &Dir = _T.GetRow(0);

		//Find closest point along capsule center
		CVec3Dfp32 Delta = Vtx - Cen;
		fp32 t = Clamp(Delta * Dir,-m_Len,m_Len);
		CVec3Dfp32 Pt = Cen + Dir * t;
		
		//Return normal
		Delta = Vtx - Pt;
		Delta.Normalize();
		return Delta;
	}

	int GetClosestFace(const CMat4Dfp32& _Transform, const CVec3Dfp32& _Normal)
	{
		m_iWritePos++;

#ifndef M_RTM
		if( m_iWritePos > PHYSCAPSULE_FACESTACKLEN )
		{
			LogFile(CStrF("ERROR: CSolidCapsuleSeparating::GetClosestFace - FaceStack full, increase stacklen (currently %d)",
				PHYSCAPSULE_FACESTACKLEN));
			m_iWritePos = 1;
		}
#endif
		m_lFaceStack[m_iWritePos-1] = _Normal;

		return m_iWritePos - 1;
	}

	M_INLINE int GetColinearEdgeCount() const
	{
		return 1;
	}

	M_INLINE void GetColinearEdge(int i, const CMat4Dfp32& _T, CVec3Dfp32& _Edge) const
	{
		_Edge = _T.GetRow(0);
	}

	M_INLINE void Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans, CVec3Dfp32& _SupportVertex) const
	{
		CVec3Dfp32 Tmp;

		//Find closest endpoint
		const CVec3Dfp32 &Dir = _Trans.GetRow(0);
		Tmp = Dir * ((_P * Dir > 0) ? m_Len : -m_Len);
		
		//Add center and radius
		_SupportVertex = Tmp + _Trans.GetRow(3) + (_P * m_Radius);
	}

	MINMAXINLINE void GetMinMax(const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		//Assume unit transformation
		fp32 ProjLen = Abs(_Axis.k[0]) * m_Len + m_Radius;
		_Min = -ProjLen;
		_Max = ProjLen;
	}

	MINMAXINLINE void GetMinMax(const CMat4Dfp32& _T, const CVec3Dfp32& _Axis, fp32& _Min, fp32& _Max) const
	{
		//Get projected length and center projection
		fp32 ProjLen = Abs(_Axis * _T.GetRow(0))*m_Len + m_Radius;
		fp32 ProjCen = _Axis * _T.GetRow(3);

		_Min = ProjCen-ProjLen;
		_Max = ProjCen+ProjLen;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		//Assume unit transform
		CVec3Dfp32 Tmp(0);
		Tmp.k[0] = Sign(_P.k[0]) * m_Len;
		Tmp += _P * m_Radius;

		return Tmp;
	}

	int GetFace(int _iF, const CMat4Dfp32 &_T, CVec3Dfp32 * _pVertices, CPlane3Dfp32 &_Plane) const
	{

#ifndef M_RTM
		if( _iF >= m_iWritePos )
		{
			LogFile(CStrF("ERROR: CSolidCapsuleSeparating::GetFace - Tried to access uninitiated face (%d/%d)",
				_iF,m_iWritePos));
			return 0;
		}
#endif

		const CVec3Dfp32 &Nrm = m_lFaceStack[_iF];
		const CVec3Dfp32 &Dir = _T.GetRow(0);
		const CVec3Dfp32 &Cen = _T.GetRow(3);
		_Plane.n = Nrm;

		//If Direction is almost perpendicular, return both points
		if( AlmostEqual(Nrm * Dir,0.0f,m_Len * 0.01f) )
		{
			_pVertices[0] = Cen + Dir * m_Len + (Nrm * m_Radius);
			_pVertices[1] = Cen - Dir * m_Len + (Nrm * m_Radius);
			_Plane.d = -(_pVertices[0] * Nrm);
			return 2;
		}

		//Only one point, get extreme vertex
		//Find closest endpoint, add center and radius
		CVec3Dfp32 Tmp;
		Tmp = Dir * ((Nrm * Dir > 0) ? m_Len : -m_Len);
		_pVertices[0] = Tmp + Cen + (Nrm * m_Radius);
		_Plane.d = -(_pVertices[0] * Nrm);

		//We need at least 2 vertices to conform with ClipFaces...
		//Add a small vector in both directions
		_pVertices[1] = _pVertices[0];
		CVec3Dfp32 PerpVector;
		if( Nrm.AlmostEqual(Dir,m_Len * 0.01f) ) PerpVector = Nrm / _T.GetRow(1);
		else PerpVector = Nrm / Dir;
		PerpVector.Normalize();
		_pVertices[0] -= PerpVector * 0.1f;
		_pVertices[1] += PerpVector * 0.1f;

		return 2;
	}
};


#endif
