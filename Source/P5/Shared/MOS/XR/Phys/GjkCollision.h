#ifndef GJKCOLLISION_H
#define GJKCOLLISION_H

#include "GjkSupport.h" 

template <class T, class Pred>
M_INLINE void PushHeap(T *_pArray, int _nElem, Pred _Pred)
{
	if (_nElem <= 0) return;

	_nElem--;
	T value = _pArray[_nElem];

	int ichild = _nElem;
	int iparent = (ichild - 1) / 2;

	while (ichild > 0 && _Pred(_pArray[iparent], value))
	{
		_pArray[ichild] = _pArray[iparent];
		ichild = iparent;
		iparent = (ichild - 1) / 2;
	}

	_pArray[ichild] = value;
}

template <class T, class Pred>
M_INLINE void PopHeap(T *_pArray, int _nElem, Pred _Pred)
{
	_nElem--;
	T value = _pArray[_nElem];

	int iparent = 0;
	int ichild = 2 * iparent + 1;
	while (ichild < _nElem)
	{
		if (ichild + 1 < _nElem && _Pred(_pArray[ichild],_pArray[ichild+1]))
		{
			ichild++;
		}
		if (_Pred(value,_pArray[ichild])) 
		{
			_pArray[iparent] = _pArray[ichild];
			iparent = ichild;
		} 
		else {
			break;
		}
		ichild = 2 * iparent + 1;
	}
	_pArray[iparent] = value;
}

class SimplexSet
{
public:
	SimplexSet()
	{
		m_N = 0;
		m_Mask = 0;

		m_Dirty = true;
		m_Degen = false;

		// Set "invalid"
		m_PointA = CVec3Dfp32(_FP32_MIN,_FP32_MIN,_FP32_MIN);
		m_PointB = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
		m_LastV = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
	}

	/*
	SimplexSet(const CVec3Dfp32& _P, const CVec3Dfp32& _A, const CVec3Dfp32& _B)
	{
		m_Points[0] = _P;
		m_A[0] = _A;
		m_B[0] = _B;
		m_N = 1;
		m_LastPoint = _P;

		m_Dirty = true;
		m_Degen = false;

		m_Mask = 0;

		// Set "invalid"
		m_PointA = CVec3Dfp32(_FP32_MIN,_FP32_MIN,_FP32_MIN);
		m_PointB = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
		m_LastV = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);

	}*/

	int GetSimplex(CVec3Dfp32 *pBuf, CVec3Dfp32 *qBuf, CVec3Dfp32 *yBuf) const 
	{
		for (int i = 0; i < m_N; i++)
		{
			pBuf[i] = m_A[i];
			qBuf[i] = m_B[i];
			yBuf[i] = m_Points[i];
		} 

		return m_N;
	}

	fp32 LargestVertexSq() const
	{
		fp32 max = 0.0f;

		for (int i = 0; i < m_N; i++)
		{
			fp32 tmp = m_Points[i].LengthSqr();
			if (tmp > max)
				max = tmp;
		}
		return max;
	}

	void Compute(CVec3Dfp32& _P1, CVec3Dfp32& _P2)
	{
		CVec3Dfp32 dummy;
		ClosestPointAndReduce(dummy);

		_P1 = m_PointA;
		_P2 = m_PointB;
	}

	bool Contains(const CVec3Dfp32& _W)
	{

#if 1
		for (int i = 0; i < m_N; i++)
		{
			if (_W.DistanceSqr(m_Points[i]) < 1e-4)
				return true;
		}

		if (_W.DistanceSqr(m_LastPoint) < 1e-4)
			return true;

#else
		for (int i = 0; i < m_N; i++)
		{
			if (_W == m_Points[i])
				return true;
		}

		if (m_LastPoint == _W)
			return true;
#endif

		return false;
	}

	void Backup(CVec3Dfp32& V)
	{
		V = m_LastV;
	}

	void Backup(CVec3Dfp32& V, CVec3Dfp32& A, CVec3Dfp32& B)
	{
		V = m_LastV;
		A = m_LastA;
		B = m_LastB;
	}

	bool ClosestPointAndReduce(CVec3Dfp32& _V)
	{

		/*
		I.   Räkna ut det som delas
		II.  Räkna ut närmsta punkt
		III. Reducera mängden map vilken typ av punkt det blev
		*/

		m_LastV = _V;
		m_LastA = m_PointA;
		m_LastB = m_PointB;

		if (m_Dirty)
		{
			m_Dirty = false;

			if (m_N == 0)
			{
				return false;
			}
			else if (m_N == 1)
			{
				m_PointA = m_A[0];
				m_PointB = m_B[0];
				_V = m_PointA - m_PointB;
				//					m_LastV = _V = m_PointA - m_PointB;
				m_Mask = (1 << 0);
			}
			else if (m_N == 2)
			{
				CVec3Dfp32 Closest;
				fp32 t;
				m_Mask = TClosestPoint<fp32>::ToLineSegment(CVec3Dfp32(0,0,0), m_Points[0], m_Points[1], t, Closest);

				m_PointA = m_A[0] + (m_A[1] - m_A[0]) * t;
				m_PointB = m_B[0] + (m_B[1] - m_B[0]) * t;
				_V = m_PointA - m_PointB;
				//					m_LastV = _V = m_PointA - m_PointB;

				Reduce();
			}
			else if (m_N == 3)
			{
				CVec3Dfp32 Closest;
				fp32 ta, tb, tc;
				m_Mask = TClosestPoint<fp32>::ToTriangle(CVec3Dfp32(0,0,0), m_Points[0], m_Points[1], m_Points[2], ta, tb, tc, Closest);

				m_PointA = m_A[0] * ta + m_A[1] * tb + m_A[2] * tc;
				m_PointB = m_B[0] * ta + m_B[1] * tb + m_B[2] * tc;
				//					m_LastV = _V = m_PointA - m_PointB;
				_V = m_PointA - m_PointB;

				Reduce();
			}
			else if (m_N == 4)
			{
				CVec3Dfp32 Closest;
				fp32 ta, tb, tc, td;

				bool degenerated = false;
				// TODO: Här borde man kolla degenererade fall. Outside i ToTetra. ska kollas samt om ingen ligger utanför...
				m_Mask = TClosestPoint<fp32>::ToTetrahedron(CVec3Dfp32(0,0,0), m_Points[0], m_Points[1], m_Points[2], m_Points[3], ta, tb, tc, td, Closest);

#ifndef PLATFORM_CONSOLE
				if (ta < 0 || tb < 0 || tc < 0 || td < 0)
				{
					int breakme1111 = 1;
				}
#endif

				m_Degen = false;
				/*
				if (m_Mask == -1)
				{
				m_Degen = true;
				return false;
				}*/

				if (m_Mask == 0)
				{
					if (degenerated)
						return false;
					else
					{
						//m_N = 3;
						//m_LastV = _V = CVec3Dfp32(0,0,0);

						return false;
					}
				}

				m_PointA = m_A[0] * ta + m_A[1] * tb + m_A[2] * tc + m_A[3] * td;
				m_PointB = m_B[0] * ta + m_B[1] * tb + m_B[2] * tc + m_B[3] * td;
				//					m_LastV = _V = m_PointA - m_PointB;
				_V = m_PointA - m_PointB;

				Reduce();
			}

		}


		return true;
	}

	void Add(const CVec3Dfp32& _w, const CVec3Dfp32& _a, const CVec3Dfp32& _b)
	{
		m_Points[m_N] = _w;
		m_A[m_N] = _a;
		m_B[m_N] = _b;
		m_N++;
		M_ASSERT(m_N <= 4, "");

		m_LastPoint = _w;

		m_Dirty = true;
	}

	bool IsFull()
	{
		return m_N == 4;
	}


	int m_N;
	bool m_Degen;

protected:

	void Remove(int index)
	{
		m_N--;
		m_Points[index] = m_Points[m_N];
		m_A[index] = m_A[m_N];
		m_B[index] = m_B[m_N];
	}

	void Reduce()
	{
		if (m_N >= 4 && !(m_Mask & (1 << 3)))
		{
			Remove(3);
		}

		if (m_N >= 3 && !(m_Mask & (1 << 2)))
		{
			Remove(2);
		}

		if (m_N >= 2 && !(m_Mask & (1 << 1)))
		{
			Remove(1);
		}

		if (m_N >= 1 && !(m_Mask & (1 << 0)))
		{
			Remove(0);
		}
	}

	CVec3Dfp32 m_Points[4], m_A[4], m_B[4];

	bool m_Dirty;

	CVec3Dfp32 m_PointA, m_PointB;

	int m_Mask;
	CVec3Dfp32 m_LastPoint, m_LastV, m_LastA, m_LastB;
};

class Polyhedron
{
public:

	Polyhedron()
	{
	}

	Polyhedron(const CVec3Dfp32 *_pVertices, int _nVertices)
	{
		m_Vertices.SetLen(_nVertices);
		for (int i = 0; i < _nVertices; i++)
		{
			m_Vertices[i] = _pVertices[i];

		}
	}

	Polyhedron(const TArray<CVec3Dfp32 >& _Vertices)
	{
		m_Vertices.SetLen(_Vertices.Len());
		for (int i = 0; i < _Vertices.Len(); i++)
		{
			m_Vertices[i] = _Vertices[i];

		}
	}

	Polyhedron(const TArray<CVec3Dfp32 >& _Vertices, const CVec3Dfp32& _Offset)
	{
		m_Vertices.SetLen(_Vertices.Len());
		for (int i = 0; i < _Vertices.Len(); i++)
		{
			m_Vertices[i] = _Vertices[i] + _Offset;

		}
	}

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
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

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P) const
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

	virtual CVec3Dfp32 GetVertex(int index) const
	{
		return m_Vertices[index];
	}

	virtual int GetNumberOfVertices() const
	{
		return m_Vertices.Len();
	}

protected:
	TArray<CVec3Dfp32 > m_Vertices;

};

class PolyhedronIndirectIndexed : public Polyhedron
{
public:
	PolyhedronIndirectIndexed(const CVec3Dfp32 *_pVertices, const unsigned long *_piVertices, int _nVertices, const CVec3Dfp32& _Translate)
	{
		m_pVertices = _pVertices;
		m_piVertices = _piVertices;
		m_nVertices = _nVertices;
		m_Translate = _Translate;
	}

	virtual CVec3Dfp32 GetVertex(int index) const
	{
		if (index < m_nVertices)
			return m_pVertices[m_piVertices[index]];
		else
			return m_pVertices[m_piVertices[index - m_nVertices]] + m_Translate;
	}

	virtual int GetNumberOfVertices() const
	{
		return m_nVertices * 2;
	}

protected:
	const CVec3Dfp32 *m_pVertices;
	const unsigned long *m_piVertices;
	CVec3Dfp32 m_Translate;
	int m_nVertices;
};


class SphereSolid : public Polyhedron
{
public:
	SphereSolid(fp32 _Radius)
	{
		m_Radius = _Radius;
	}

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		fp32 s = _P.Length();

		if (s > 0.0f)
		{
			s = m_Radius / s;
			return CVec3Dfp32(_P[0] * s, _P[1] * s, _P[2] * s);
		}
		else
		{
			return CVec3Dfp32(m_Radius, 0.0f, 0.0f);
		}

	}

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		fp32 s = _P.Length();

		if (s > 0.0f)
		{
			s = m_Radius / s;
			return CVec3Dfp32(_P[0] * s, _P[1] * s, _P[2] * s);
		}
		else
		{
			return CVec3Dfp32(m_Radius, 0.0f, 0.0f);
		}
	}


	fp32 m_Radius;

};

class MinSumSolid : public Polyhedron
{
public:
	MinSumSolid(Polyhedron *p1, Polyhedron *p2)
	{
		m_p1 = p1;
		m_p2 = p2;
	}

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		return m_p1->Support(_P, _Trans) + m_p2->Support(_P, _Trans);
	}

	virtual CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		return m_p1->Support(_P) + m_p2->Support(_P);

	}

	Polyhedron *m_p1, *m_p2;

};

class TSphereSolid 
{
public:
	TSphereSolid(fp32 _Radius)
	{
		m_Radius = _Radius;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		fp32 s = _P.Length();

		if (s > 0.0f)
		{
			s = m_Radius / s;
			return CVec3Dfp32(_P[0] * s, _P[1] * s, _P[2] * s);
		}
		else
		{
			return CVec3Dfp32(m_Radius, 0.0f, 0.0f);
		}

	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		fp32 s = _P.Length();

		if (s > 0.0f)
		{
			s = m_Radius / s;
			return CVec3Dfp32(_P[0] * s, _P[1] * s, _P[2] * s);
		}
		else
		{
			return CVec3Dfp32(m_Radius, 0.0f, 0.0f);
		}
	}


	fp32 m_Radius;

};

template <class TConvex1, class TConvex2>
class TMinSumSolid
{
public:
	TMinSumSolid(TConvex1 *p1, TConvex2 *p2)
	{
		m_p1 = p1;
		m_p2 = p2;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		return m_p1->Support(_P, _Trans) + m_p2->Support(_P, _Trans);
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		return m_p1->Support(_P) + m_p2->Support(_P);

	}

	TConvex1 *m_p1;
	TConvex2 *m_p2;
};




class GJKTriangle
{
public:
	M_INLINE GJKTriangle()
	{
#ifndef M_RTM
		m_iV1 = -1;
		m_iV2 = -1;
		m_iV3 = -1;
#endif

		m_BaryA = -1000;
		m_BaryB = -1000;
		m_BaryC = -1000;
		m_ClosestToOrigoDistanceSq = _FP32_MAX;
	}

	M_INLINE GJKTriangle(const GJKTriangle& _GJKTriangle)
	{
		m_iV1 = _GJKTriangle.m_iV1;
		m_iV2 = _GJKTriangle.m_iV2;
		m_iV3 = _GJKTriangle.m_iV3;

		m_ClosestToOrigoDistanceSq = _GJKTriangle.m_ClosestToOrigoDistanceSq;
		m_ClosestToOrigo = _GJKTriangle.m_ClosestToOrigo;
		m_BaryA = _GJKTriangle.m_BaryA;
		m_BaryB = _GJKTriangle.m_BaryB;
		m_BaryC = _GJKTriangle.m_BaryC;
	}

	M_INLINE GJKTriangle(int _iV1, int _iV2, int _iV3, const CVec3Dfp32& _ClosestToOrigo, fp32 _BaryA, fp32 _BaryB, fp32 _BaryC)
	{
		m_iV1 = _iV1;
		m_iV2 = _iV2;
		m_iV3 = _iV3;
		m_ClosestToOrigo = _ClosestToOrigo;
		m_ClosestToOrigoDistanceSq = _ClosestToOrigo.LengthSqr();
		m_BaryA = _BaryA;
		m_BaryB = _BaryB;
		m_BaryC = _BaryC;
	}

	M_INLINE void operator = (const GJKTriangle& _GJKTriangle)
	{
		m_iV1 = _GJKTriangle.m_iV1;
		m_iV2 = _GJKTriangle.m_iV2;
		m_iV3 = _GJKTriangle.m_iV3;

		m_ClosestToOrigoDistanceSq = _GJKTriangle.m_ClosestToOrigoDistanceSq;
		m_ClosestToOrigo = _GJKTriangle.m_ClosestToOrigo;
		m_BaryA = _GJKTriangle.m_BaryA;
		m_BaryB = _GJKTriangle.m_BaryB;
		m_BaryC = _GJKTriangle.m_BaryC;
	}

	M_INLINE const CVec3Dfp32& GetClosestPointToOrigo()
	{
		return m_ClosestToOrigo;
	}

	M_INLINE fp32 GetDistanceSq() const
	{
		return m_ClosestToOrigoDistanceSq;
	}

protected:
	friend class GJKTriangleHeap;
	friend class GJKTriangleHeap2;

	int m_iV1, m_iV2, m_iV3;
	fp32 m_ClosestToOrigoDistanceSq;
	CVec3Dfp32 m_ClosestToOrigo;
	fp32 m_BaryA, m_BaryB, m_BaryC;
};

//#include <vector>
//#include <algorithm>

M_INLINE static bool TrianglePred(const GJKTriangle *_Tri1, const GJKTriangle *_Tri2)
{
	return _Tri1->GetDistanceSq() > _Tri2->GetDistanceSq();
}


#define GJKTRIANGLEHEAP2_MAXTRIANGLES 100
#define GJKTRIANGLEHEAP2_MAXPOINTS 100

class GJKTriangleHeap2
{
public:
	GJKTriangleHeap2()
	{
		m_nTriangles = 0;
		m_nTrianglesUsed = 0;
		m_nPoints = 0;

//		m_Points.reserve(100);
//		m_A.reserve(100);
//		m_B.reserve(100);
	}

	void Clear()
	{
		m_nTriangles = 0;
		m_nTrianglesUsed = 0;
		m_nPoints = 0;

//		m_Points.resize(0);
//		m_A.resize(0);
//		m_B.resize(0);

//		M_TRACEALWAYS("--------------\n");
//		M_TRACEALWAYS("%d\n",m_Points.capacity());
//		M_TRACEALWAYS("%d\n",m_A.capacity());
//		M_TRACEALWAYS("%d\n",m_B.capacity());
	}

	M_INLINE void AddTriangle(int _iV1, int _iV2, int _iV3)
	{
		M_ASSERT(m_nTriangles < GJKTRIANGLEHEAP2_MAXTRIANGLES, "");

		CVec3Dfp32 Closest;
		fp32 ta, tb, tc;
//		int mask = TClosestPoint<fp32>::ToTriangle(CVec3Dfp32(0.0f, 0.0f, 0.0f), m_Points[_iV1], m_Points[_iV2], m_Points[_iV3], ta, tb, tc, Closest);
		TClosestPoint<fp32>::ToTriangle(CVec3Dfp32(0.0f, 0.0f, 0.0f), m_Points[_iV1], m_Points[_iV2], m_Points[_iV3], ta, tb, tc, Closest);

		GJKTriangle *pTriangle = &m_TriangleHeap[m_nTrianglesUsed++];
		*pTriangle = GJKTriangle(_iV1, _iV2, _iV3, Closest, ta, tb, tc);

		m_Triangles[m_nTriangles++] = pTriangle;
		PushHeap(&m_Triangles[0], m_nTriangles, TrianglePred);
		//std::push_heap(&m_Triangles[0], &m_Triangles[m_nTriangles], TrianglePred);
	}

	M_INLINE int AddPoint(const CVec3Dfp32& _P, const CVec3Dfp32& _A, const CVec3Dfp32& _B)
	{
		M_ASSERT(m_nPoints < GJKTRIANGLEHEAP2_MAXPOINTS, "");

		m_Points[m_nPoints] = _P;
		m_A[m_nPoints] = _A;
		m_B[m_nPoints] = _B;

		m_nPoints++;

		return m_nPoints - 1;

		//m_Points.push_back(_P);
		//m_A.push_back(_A);
		//m_B.push_back(_B);

		//return (int) m_Points.size() - 1;
	}

	M_INLINE void Split(int _iTriangle, int _iV)
	{
		const GJKTriangle *Triangle = m_Triangles[_iTriangle];
		AddTriangle(Triangle->m_iV1, Triangle->m_iV2, _iV);
		AddTriangle(Triangle->m_iV2, Triangle->m_iV3, _iV);
		AddTriangle(Triangle->m_iV3, Triangle->m_iV1, _iV);
	}

	M_INLINE void Split(const GJKTriangle& _Triangle, int _iV)
	{
		AddTriangle(_Triangle.m_iV1, _Triangle.m_iV2, _iV);
		AddTriangle(_Triangle.m_iV2, _Triangle.m_iV3, _iV);
		AddTriangle(_Triangle.m_iV3, _Triangle.m_iV1, _iV);
	}

	M_INLINE GJKTriangle *GetClosestAndRemove()
	{
		GJKTriangle *pClosest = m_Triangles[0];
		PopHeap(&m_Triangles[0], m_nTriangles, TrianglePred);
		//std::pop_heap(&m_Triangles[0], &m_Triangles[m_nTriangles], TrianglePred);
		m_nTriangles--;

		return pClosest;

		/*
		fp32 dist = _FP32_MAX;
		int nTriangles = m_Triangles.Len();
		if (nTriangles == 0) return NULL;
		int index = -1;
		for (int i = 0; i < nTriangles; i++)
		{
			const GJKTriangle& Tri = m_Triangles[i];
			if (Tri.m_ClosestToOrigoDistanceSq < dist)
			{
				index = i;
				dist = Tri.m_ClosestToOrigoDistanceSq;
			}
		}
		*/

		/*
		TODO: index kan bli -1 om m_ClosestToOrigoDistanceSq är ogilig
		Det verkar komma sig ifrån att translationsdelen ibland för ett fysikobjekt
		kan vara "NaN" av någon oklar anledning.
		if-satsen ska vara en assert sen.
		*/
		/*
		if (index == -1)
		{
			return NULL;
		}*/

		//m_Slask.Add(m_Triangles[index]);
		//m_Triangles.Del(index);
		//return &m_Slask[m_Slask.Len()-1];
	}

	M_INLINE void ComputePoints(const GJKTriangle& _Triangle, CVec3Dfp32& _A, CVec3Dfp32& _B)
	{
		fp32 ta = _Triangle.m_BaryA;
		fp32 tb = _Triangle.m_BaryB;
		fp32 tc = _Triangle.m_BaryC;

		int i1 = _Triangle.m_iV1;
		int i2 = _Triangle.m_iV2;
		int i3 = _Triangle.m_iV3;

		_A = m_A[i1] * ta + m_A[i2] * tb + m_A[i3] * tc;
		_B = m_B[i1] * ta + m_B[i2] * tb + m_B[i3] * tc;
	}

protected:
	//GJKTriangle m_Triangles[100];
	//GJKTriangle m_Slask[100];


	/*
	CVec3Dfp32 m_Points[100];
	CVec3Dfp32 m_A[100];
	CVec3Dfp32 m_B[100];
	*/

	GJKTriangle m_TriangleHeap[GJKTRIANGLEHEAP2_MAXTRIANGLES];
	GJKTriangle *m_Triangles[GJKTRIANGLEHEAP2_MAXTRIANGLES];

	int m_nTrianglesUsed;
	int m_nTriangles;
	//std::vector m_Triangles;
	int m_nPoints;
	CVec3Dfp32 m_Points[GJKTRIANGLEHEAP2_MAXPOINTS];
	CVec3Dfp32 m_A[GJKTRIANGLEHEAP2_MAXPOINTS];
	CVec3Dfp32 m_B[GJKTRIANGLEHEAP2_MAXPOINTS];

	//std::vector<CVec3Dfp32> m_Points;
	//std::vector<CVec3Dfp32> m_A;
	//std::vector<CVec3Dfp32> m_B;
};

class GJKTriangleHeap
{
public:
	void AddTriangle(int _iV1, int _iV2, int _iV3)
	{
		CVec3Dfp32 Closest;
		fp32 ta, tb, tc;
//		int mask = TClosestPoint<fp32>::ToTriangle(CVec3Dfp32(0.0f, 0.0f, 0.0f), m_Points[_iV1], m_Points[_iV2], m_Points[_iV3], ta, tb, tc, Closest);
		TClosestPoint<fp32>::ToTriangle(CVec3Dfp32(0.0f, 0.0f, 0.0f), m_Points[_iV1], m_Points[_iV2], m_Points[_iV3], ta, tb, tc, Closest);
		m_Triangles.Add(GJKTriangle(_iV1, _iV2, _iV3, Closest, ta, tb, tc));
	}

	int AddPoint(const CVec3Dfp32& _P, const CVec3Dfp32& _A, const CVec3Dfp32& _B)
	{
		m_Points.Add(_P);
		m_A.Add(_A);
		m_B.Add(_B);

		return m_Points.Len() - 1;
	}

	void Split(int _iTriangle, int _iV)
	{
		const GJKTriangle& Triangle = m_Triangles[_iTriangle];
		AddTriangle(Triangle.m_iV1, Triangle.m_iV2, _iV);
		AddTriangle(Triangle.m_iV2, Triangle.m_iV3, _iV);
		AddTriangle(Triangle.m_iV3, Triangle.m_iV1, _iV);
	}

	void Split(const GJKTriangle& _Triangle, int _iV)
	{
		AddTriangle(_Triangle.m_iV1, _Triangle.m_iV2, _iV);
		AddTriangle(_Triangle.m_iV2, _Triangle.m_iV3, _iV);
		AddTriangle(_Triangle.m_iV3, _Triangle.m_iV1, _iV);
	}

	GJKTriangle *GetClosestAndRemove()
	{
		fp32 dist = _FP32_MAX;
		int nTriangles = m_Triangles.Len();
		if (nTriangles == 0) return NULL;
		int index = -1;
		for (int i = 0; i < nTriangles; i++)
		{
			const GJKTriangle& Tri = m_Triangles[i];
			if (Tri.m_ClosestToOrigoDistanceSq < dist)
			{
				index = i;
				dist = Tri.m_ClosestToOrigoDistanceSq;
			}
		}

		/*
			TODO: index kan bli -1 om m_ClosestToOrigoDistanceSq är ogilig
			Det verkar komma sig ifrån att translationsdelen ibland för ett fysikobjekt
			kan vara "NaN" av någon oklar anledning.
			if-satsen ska vara en assert sen.
		*/
		if (index == -1)
		{
			return NULL;
		}

		m_Slask.Add(m_Triangles[index]);
		m_Triangles.Del(index);
		return &m_Slask[m_Slask.Len()-1];
	}

	void ComputePoints(const GJKTriangle& _Triangle, CVec3Dfp32& _A, CVec3Dfp32& _B)
	{
		fp32 ta = _Triangle.m_BaryA;
		fp32 tb = _Triangle.m_BaryB;
		fp32 tc = _Triangle.m_BaryC;

		int i1 = _Triangle.m_iV1;
		int i2 = _Triangle.m_iV2;
		int i3 = _Triangle.m_iV3;

		_A = m_A[i1] * ta + m_A[i2] * tb + m_A[i3] * tc;
		_B = m_B[i1] * ta + m_B[i2] * tb + m_B[i3] * tc;
	}


protected:
	TArray<GJKTriangle> m_Triangles;
	TArray<GJKTriangle> m_Slask;

	TArray<CVec3Dfp32> m_Points;
	TArray<CVec3Dfp32> m_A;
	TArray<CVec3Dfp32> m_B;
};


static GJKTriangleHeap2 gTriangleHeap;

template <class TConvex1, class TConvex2>
class TGJK
{
public:
	static bool PenetrationDepth(const SimplexSet& _gjk, 
								 const TConvex1& _Polyhedron1, 
								 const TConvex2& _Polyhedron2,
								 CVec3Dfp32& _P1, CVec3Dfp32& _P2,
								 int _MaxIter = 20,
								 fp32 _RelativeErrorSq = 1.0e-6)
	{
//		GJKTriangleHeap2 TriHeap;
		gTriangleHeap.Clear();
		CVec3Dfp32 Points[4], A[4], B[4];

		// Dynamisk tolerans. Ok?
		fp32 DistanceTol = _FP32_EPSILON * _gjk.LargestVertexSq();

		int nInSimplex = _gjk.GetSimplex(A,B,Points);
		if (nInSimplex != 4)
		{
			// TODO: Not supported yet... extremely rare....
			return false;
		}

		for (int i = 0; i < nInSimplex; i++)
		{
			gTriangleHeap.AddPoint(Points[i], A[i], B[i]);
		}

		gTriangleHeap.AddTriangle(0,1,2);
		gTriangleHeap.AddTriangle(0,3,1);
		gTriangleHeap.AddTriangle(1,3,2);
		gTriangleHeap.AddTriangle(0,3,2);

		int nIter = 0;

		GJKTriangle *Triangle = gTriangleHeap.GetClosestAndRemove();
		while (Triangle != NULL && nIter < _MaxIter)
		{
			nIter++;

			CVec3Dfp32 ClosestPoint = Triangle->GetClosestPointToOrigo();

			// Tecken har skiljer sig från andra...
			CVec3Dfp32 p = _Polyhedron1.Support(ClosestPoint);
			CVec3Dfp32 q = _Polyhedron2.Support(-ClosestPoint);
			CVec3Dfp32 w = p - q; 

			fp32 delta = Triangle->GetClosestPointToOrigo() * w;
//			fp32 MaxError = Max(delta * _RelativeErrorSq, DistanceTol);
			Max(delta * _RelativeErrorSq, DistanceTol);
			if (delta - Triangle->GetDistanceSq() <= DistanceTol) 
				break;

			int iV = gTriangleHeap.AddPoint(w, p, q);
			gTriangleHeap.Split(*Triangle, iV);

			Triangle = gTriangleHeap.GetClosestAndRemove();
		}

/*
		// TODO: Vi når inte ett acceptablt fel. 
		// Något är snett! Bäst att returnera false ELLER INTE?
		if (nIter == _MaxIter)
		{
			return false;
		}
*/

		// TODO: Detta ska väl egentligen inte "kunna" ske?
		// Kanske en assert istället.
		if (Triangle != NULL)
		{
			gTriangleHeap.ComputePoints(*Triangle, _P1, _P2);
			return true;
		}
		else
		{
			return false;
		}
	}
	static bool PenetrationDepth2(const TConvex1& _Polyhedron1, const CMat4Dfp32 &_Transform1, 
								  const TConvex2& _Polyhedron2, const CMat4Dfp32 &_Transform2, 
								  CVec3Dfp32& _P1, CVec3Dfp32& _P2, 
								  fp32 _RelativeErrorSq = 1.0e-2, fp32 _MaximumDistanceSq = 1e20)
	{

		CVec3Dfp32 v(0.0f, 0.0f, 0.0f);
		fp32 DistanceSq = _FP32_MAX;  // TODO: _FP32_MAX

		SimplexSet simplex;

		int nIterations = 0;
		const int MaxIter = 30;

		while (nIterations < MaxIter)
		{
			nIterations++;
			CVec3Dfp32  p = _Polyhedron1.Support(-v, _Transform1);
			CVec3Dfp32  q = _Polyhedron2.Support(v, _Transform2);
			CVec3Dfp32 w = p - q; 

			fp32 delta = v * w;
			//			if (delta > 0.0f && delta * delta > DistanceSq * _MaximumDistanceSq) 
			if (delta > 0.0f)
			{
				return false;
			}

			if (simplex.Contains(w) || DistanceSq - delta <= DistanceSq * _RelativeErrorSq) 
			{
				break;
			}

			simplex.Add(w, p, q);

			if (!simplex.ClosestPointAndReduce(v))
			{
				break;
				//return false;
			}

			fp32 PreviousDistanceSq = DistanceSq;
			DistanceSq = v.LengthSqr();

#if 1
			if (PreviousDistanceSq - DistanceSq <= _FP32_EPSILON * PreviousDistanceSq) 
			{ 
				//simplexSet.Backup(v);
				break;
			}
#endif

			if (simplex.IsFull())
			{
				break;
			}

		}

		bool ret = PenetrationDepth(simplex, _Polyhedron1, _Polyhedron2, _P1, _P2);
		return ret;
	};


	static bool MarginPenetrationDepth(const TConvex1& _Polyhedron1, const CMat4Dfp32 &_Transform1, 
									   const TConvex2& _Polyhedron2, const CMat4Dfp32 &_Transform2, 
									   CVec3Dfp32& _P1, CVec3Dfp32& _P2, 
									   fp32 _Margin1, fp32 _Margin2, 
									   fp32 _RelativeErrorSq = 1.0e-6, fp32 _MaximumDistanceSq = 1e20)

	{
		fp32 Margin = _Margin1 + _Margin2;
		fp32 MarginSq = Margin * Margin;

		CVec3Dfp32 v(0.0f, 0.0f, 0.0f);
		fp32 DistanceSq = _FP32_MAX;

		SimplexSet simplex;

		int nIterations = 0;
		const int MaxIter = 30;

		while (nIterations < MaxIter)
		{
			nIterations++;
			CVec3Dfp32  p = _Polyhedron1.Support(-v, _Transform1);
			CVec3Dfp32  q = _Polyhedron2.Support(v, _Transform2);
			CVec3Dfp32 w = p - q; 

			fp32 delta = v * w;
			//			if (delta > 0.0f && delta * delta > DistanceSq * _MaximumDistanceSq) 
			if (delta > 0.0f && delta * delta > DistanceSq * MarginSq)
			{
				return false;
			}


			// TODO: TEST FÖR ATT m_PointA och m_PointB är odef om det är första iterationen...

            // TODO: DENNA LÖSNING ÄR EN TEMPORÄR FIX!
			if (nIterations > 1)
			if (simplex.Contains(w) || DistanceSq - delta <= DistanceSq * _RelativeErrorSq) 
			{
				// TODO: Om DistanceSq == 0 går det åt pipan!
				// Samma gäller längre ner!
				//if (nIterations == 0) return false;

				simplex.Compute(_P1, _P2);
				fp32 d = M_Sqrt(DistanceSq);

				// TODO: Temp fix då d ibland kan vara 0.
				if (M_Fabs(d) > 0.0001f)
				{
					_P1 = _P1 - v * (_Margin1 / d);
					_P2 = _P2 + v * (_Margin2 / d);
				}
				return true;
			}

			simplex.Add(w, p, q);

			if (!simplex.ClosestPointAndReduce(v))
			{
				if (simplex.m_Degen)
				{
					simplex.Compute(_P1, _P2);
					fp32 d = M_Sqrt(DistanceSq);

					// TODO: Temp fix då d ibland kan vara 0.
					if (M_Fabs(d) > 0.0001f)
					{
						_P1 = _P1 - v * (_Margin1 / d);
						_P2 = _P2 + v * (_Margin2 / d);
					}
					return true;
				}
				else break;

			}

			fp32 PreviousDistanceSq = DistanceSq;
			DistanceSq = v.LengthSqr();

#if 1
			if (PreviousDistanceSq - DistanceSq <= _FP32_EPSILON * PreviousDistanceSq) 
			{ 
				simplex.Backup(v, _P1, _P2);
				DistanceSq = v.LengthSqr();

				//simplex.Compute(_P1, _P2);
				fp32 d = M_Sqrt(DistanceSq);
				// TODO: Temp fix då d ibland kan vara 0.
				if (M_Fabs(d) > 0.0001f)
				{
					_P1 = _P1 - v * (_Margin1 / d);
					_P2 = _P2 + v * (_Margin2 / d);
				}
				return true;
			}
#endif

			if (simplex.IsFull())
			{
				break;
			}

		}

		TSphereSolid ss1(_Margin1);
		TSphereSolid ss2(_Margin2);

		TMinSumSolid<TSphereSolid, TConvex1> ms1(&ss1,  (TConvex1 *) &_Polyhedron1);
		TMinSumSolid<TSphereSolid, TConvex2> ms2(&ss2,  (TConvex2 *) &_Polyhedron2);

		return TGJK<TMinSumSolid<TSphereSolid, TConvex1>, TMinSumSolid<TSphereSolid, TConvex2> >::PenetrationDepth2(ms1, _Transform1, ms2, _Transform2, _P1, _P2);
	}

};

class GJK
{
public:

	static bool PenetrationDepth(const SimplexSet& _gjk, 
								 const Polyhedron& _Polyhedron1, 
								 const Polyhedron& _Polyhedron2,
								 CVec3Dfp32& _P1, CVec3Dfp32& _P2,
								 int _MaxIter = 20,
								 fp32 _RelativeErrorSq = 1.0e-6)
	{
		GJKTriangleHeap TriHeap;
		CVec3Dfp32 Points[4], A[4], B[4];

		// Dynamisk tolerans. Ok?
		fp32 DistanceTol = _FP32_EPSILON * _gjk.LargestVertexSq();

		int nInSimplex = _gjk.GetSimplex(A,B,Points);
		if (nInSimplex != 4)
		{
			// TODO: Not supported yet... extremely rare....
			return false;
		}

		for (int i = 0; i < nInSimplex; i++)
		{
			TriHeap.AddPoint(Points[i], A[i], B[i]);
		}

		TriHeap.AddTriangle(0,1,2);
		TriHeap.AddTriangle(0,3,1);
		TriHeap.AddTriangle(1,3,2);
		TriHeap.AddTriangle(0,3,2);

		int nIter = 0;

		GJKTriangle *Triangle = TriHeap.GetClosestAndRemove();
		while (Triangle != NULL && nIter < _MaxIter)
		{
			nIter++;

			CVec3Dfp32 ClosestPoint = Triangle->GetClosestPointToOrigo();

			// Tecken har skiljer sig från andra...
			CVec3Dfp32 p = _Polyhedron1.Support(ClosestPoint);
			CVec3Dfp32 q = _Polyhedron2.Support(-ClosestPoint);
			CVec3Dfp32 w = p - q; 

			fp32 delta = Triangle->GetClosestPointToOrigo() * w;
//			fp32 MaxError = Max(delta * _RelativeErrorSq, DistanceTol);
			Max(delta * _RelativeErrorSq, DistanceTol);
			if (delta - Triangle->GetDistanceSq() <= DistanceTol) 
				break;

			int iV = TriHeap.AddPoint(w, p, q);
			TriHeap.Split(*Triangle, iV);

			Triangle = TriHeap.GetClosestAndRemove();
		}

		// TODO: Detta ska väl egentligen inte "kunna" ske?
		// Kanske en assert istället.
		if (Triangle != NULL)
		{
			TriHeap.ComputePoints(*Triangle, _P1, _P2);
			return true;
		}
		else
		{
			return false;
		}
	}
	static bool PenetrationDepth2(const Polyhedron& _Polyhedron1, const CMat4Dfp32 &_Transform1, 
								  const Polyhedron& _Polyhedron2, const CMat4Dfp32 &_Transform2, 
								  CVec3Dfp32& _P1, CVec3Dfp32& _P2, 
								  fp32 _RelativeErrorSq = 1.0e-2, fp32 _MaximumDistanceSq = 1e20)
	{

		CVec3Dfp32 v(0.0f, 0.0f, 0.0f);
		fp32 DistanceSq = _FP32_MAX;  // TODO: _FP32_MAX

		SimplexSet simplex;

		int nIterations = 0;
		const int MaxIter = 30;

		while (nIterations < MaxIter)
		{
			nIterations++;
			CVec3Dfp32  p = _Polyhedron1.Support(-v, _Transform1);
			CVec3Dfp32  q = _Polyhedron2.Support(v, _Transform2);
			CVec3Dfp32 w = p - q; 

			fp32 delta = v * w;
			//			if (delta > 0.0f && delta * delta > DistanceSq * _MaximumDistanceSq) 
			if (delta > 0.0f)
			{
				return false;
			}

			if (simplex.Contains(w) || DistanceSq - delta <= DistanceSq * _RelativeErrorSq) 
			{
				break;
			}

			simplex.Add(w, p, q);

			if (!simplex.ClosestPointAndReduce(v))
			{
				break;
				//return false;
			}

			fp32 PreviousDistanceSq = DistanceSq;
			DistanceSq = v.LengthSqr();

#if 1
			if (PreviousDistanceSq - DistanceSq <= _FP32_EPSILON * PreviousDistanceSq) 
			{ 
				//simplexSet.Backup(v);
				break;
			}
#endif

			if (simplex.IsFull())
			{
				break;
			}

		}

		bool ret = PenetrationDepth(simplex, _Polyhedron1, _Polyhedron2, _P1, _P2);
		return ret;
	};


	static bool MarginPenetrationDepth(const Polyhedron& _Polyhedron1, const CMat4Dfp32 &_Transform1, 
									   const Polyhedron& _Polyhedron2, const CMat4Dfp32 &_Transform2, 
									   CVec3Dfp32& _P1, CVec3Dfp32& _P2, 
									   fp32 _Margin1, fp32 _Margin2, 
									   fp32 _RelativeErrorSq = 1.0e-6, fp32 _MaximumDistanceSq = 1e20)

	{
		fp32 Margin = _Margin1 + _Margin2;
		fp32 MarginSq = Margin * Margin;

		CVec3Dfp32 v(0.0f, 0.0f, 0.0f);
		fp32 DistanceSq = _FP32_MAX;

		SimplexSet simplex;

		int nIterations = 0;
		const int MaxIter = 30;

		while (nIterations < MaxIter)
		{
			nIterations++;
			CVec3Dfp32  p = _Polyhedron1.Support(-v, _Transform1);
			CVec3Dfp32  q = _Polyhedron2.Support(v, _Transform2);
			CVec3Dfp32 w = p - q; 

			fp32 delta = v * w;
			//			if (delta > 0.0f && delta * delta > DistanceSq * _MaximumDistanceSq) 
			if (delta > 0.0f && delta * delta > DistanceSq * MarginSq)
			{
				return false;
			}


			// TODO: TEST FÖR ATT m_PointA och m_PointB är odef om det är första iterationen...

                        // TODO: DENNA LÖSNING ÄR EN TEMPORÄR FIX!
			if (nIterations > 1)
			if (simplex.Contains(w) || DistanceSq - delta <= DistanceSq * _RelativeErrorSq) 
			{
				// TODO: Om DistanceSq == 0 går det åt pipan!
				// Samma gäller längre ner!
				//if (nIterations == 0) return false;

				simplex.Compute(_P1, _P2);
				fp32 d = M_Sqrt(DistanceSq);

				// TODO: Temp fix då d ibland kan vara 0.
				if (M_Fabs(d) > 0.0001f)
				{
					_P1 = _P1 - v * (_Margin1 / d);
					_P2 = _P2 + v * (_Margin2 / d);
				}
				return true;
			}

			simplex.Add(w, p, q);

			if (!simplex.ClosestPointAndReduce(v))
			{
				if (simplex.m_Degen)
				{
					simplex.Compute(_P1, _P2);
					fp32 d = M_Sqrt(DistanceSq);

					// TODO: Temp fix då d ibland kan vara 0.
					if (M_Fabs(d) > 0.0001f)
					{
						_P1 = _P1 - v * (_Margin1 / d);
						_P2 = _P2 + v * (_Margin2 / d);
					}
					return true;
				}
				else break;

			}

			fp32 PreviousDistanceSq = DistanceSq;
			DistanceSq = v.LengthSqr();

#if 1
			if (PreviousDistanceSq - DistanceSq <= _FP32_EPSILON * PreviousDistanceSq) 
			{ 
				simplex.Backup(v, _P1, _P2);
				DistanceSq = v.LengthSqr();

				//simplex.Compute(_P1, _P2);
				fp32 d = M_Sqrt(DistanceSq);
				// TODO: Temp fix då d ibland kan vara 0.
				if (M_Fabs(d) > 0.0001f)
				{
					_P1 = _P1 - v * (_Margin1 / d);
					_P2 = _P2 + v * (_Margin2 / d);
				}
				return true;
			}
#endif

			if (simplex.IsFull())
			{
				break;
			}

		}

		SphereSolid ss1(_Margin1);
		SphereSolid ss2(_Margin2);

		MinSumSolid ms1(&ss1, (Polyhedron *) &_Polyhedron1);
		MinSumSolid ms2(&ss2, (Polyhedron *) &_Polyhedron2);

		return PenetrationDepth2(ms1, _Transform1, ms2, _Transform2, _P1, _P2);
	}

};
#endif


