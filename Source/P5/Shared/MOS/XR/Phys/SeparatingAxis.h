#ifndef SEPARATINGAXIS_H
#define SEPARATINGAXIS_H

//#pragma optimize( "", off )
//#pragma inline_depth(0)

template <class T>
class TLine2_
{
public:
	TLine2_(const TVector2<T>& p1, const TVector2<T>& p2)
	{
		TVector2<T> dir = p2 - p1;
		dir.Normalize();
		normal[0] = dir[1];
		normal[1] = -dir[0];

		d = - (normal * p1);
	}

	T Distance(const TVector2<T>& V) const
	{
		return (normal * V) + d;
	}

	int GetSide(const TVector2<T>& V, T Epsilon) const
	{
		T dist = Distance(V);
		if (dist < -Epsilon) return -1;
		else if (dist > Epsilon) return 1;
		return 0;
	}

	void Intersect(const TVector2<T>& _p0, const TVector2<T>& _p1, TVector2<T>& _RetV) const
	{
		T dvx = (_p1.k[0] - _p0.k[0]);
		T dvy = (_p1.k[1] - _p0.k[1]);

		T s = dvx * normal.k[0] + dvy * normal.k[1];

		if (s == (T)0)
		{
			_RetV = _p0;
			return;
		}
		T sp = _p0 * normal + d;
		T t = -sp/s;

		_RetV.k[0] = _p0.k[0] + dvx * t;
		_RetV.k[1] = _p0.k[1] + dvy * t;
	}


	TVector2<T> normal;
	T d;
};


template <class T>
class TClipFunctions2D
{
public:
	static int PolyPoly(const TVector2<T> *_pVertexList1, int _nVertices1,
						const TVector2<T> *_pVertexList2, int _nVertices2,
						TVector2<T> *_pResult);

	M_FORCEINLINE static int CutPoly(TVector2<T> *_pVertexList, int _nVertices, const TLine2_<T>& _HalfSpace);


};


template <class T>
int TClipFunctions2D<T>::PolyPoly(const TVector2<T> *_pVertexList1, int _nVertices1,
								  const TVector2<T> *_pVertexList2, int _nVertices2,
								  TVector2<T> *_pResult)
{

	int nEdges1 = _nVertices1;
	int nEdges2 = _nVertices2;

	//_Result.SetLen(nEdges1);
	for (int i = 0; i < nEdges1; i++)
	{
		_pResult[i] = _pVertexList1[i];
	}

	int N = nEdges1;

	for (int i = 0; i < nEdges2; i++)
	{
		const TVector2<T> &E2a = _pVertexList2[i % nEdges2];
		const TVector2<T> &E2b = _pVertexList2[(i+1) % nEdges2];

		N = CutPoly(_pResult, N, TLine2_<T>(E2a, E2b));
	}

	return N;
}


template <class T>
int TClipFunctions2D<T>::CutPoly(TVector2<T> *_pVertexList, int _nVertices, const TLine2_<T>& _HalfSpace)
{
	int lVertSide[128];
	int nVertices = _nVertices;
	if (nVertices == 0) return 0;

	for (int i = 0; i < nVertices; i++)
	{
		lVertSide[i] = _HalfSpace.GetSide(_pVertexList[i], 0.0001); 
	}

	TVector2<T> NewVertices[200];
	int nVerticesNew = 0;

	for (int i = 0; i < nVertices; i++)
	{
		int iv1 = i;
		int iv2 = (i + 1) % nVertices;

		if (lVertSide[iv1] >= 0)
		{
			NewVertices[nVerticesNew++] = _pVertexList[iv1];

			if (lVertSide[iv2] < 0 && lVertSide[iv1] > 0)
			{
				TVector2<T> Intersection;
				_HalfSpace.Intersect(_pVertexList[iv1], _pVertexList[iv2], Intersection);
				NewVertices[nVerticesNew++] = Intersection;
			}
		}
		else
		{
			if (lVertSide[iv2] > 0)
			{
				TVector2<T> Intersection;
				_HalfSpace.Intersect(_pVertexList[iv1], _pVertexList[iv2], Intersection);
				NewVertices[nVerticesNew++] = Intersection;
			}
		}
	}

	for (int i = 0; i < nVerticesNew; i++)
	{
		_pVertexList[i] = NewVertices[i];
	}

	return nVerticesNew;
}


class TSeparatingAxisTestConstants
{
public:
	enum Feature
	{
		CENTER = (1 << 0),
		FACE = (1 << 1),
		EDGE = (1 << 2)
	};

	enum Bits	
	{
		FLIPPEDAXISBIT = 31
	};

	enum Masks
	{
		FEATUREINDEXMASK = ~((1 << FLIPPEDAXISBIT))
	};
};

template <class C1, class C2, class R, int AxisTests>
class TSeparatingAxisTest
{
public:

	static R Collide(C1 *_pConvex1, const TMatrix4<R>& _T1,
					 C2 *_pConvex2, const TMatrix4<R>& _T2,
					 const TVector3<R>& _CenterAxis,
					 TVector3<R>& _SeparatingAxis,
					 int& _iFeature, int& _FeatureType, 
					 int& _iReferenceObject)					 
	{
		//const TVector3<R>& Center1 = _T1.GetRow(3);
		//const TVector3<R>& Center2 = _T2.GetRow(3);
		//TVector3<R> CenterAxis = Center2 - Center1;
		//R CenterCenterDistance  = CenterAxis.Length();
		//CenterAxis.Normalize();

		_iReferenceObject = 0;


		TMatrix4<R> T1Inv, T2Inv;

		_T1.InverseOrthogonal(T1Inv);
		_T2.InverseOrthogonal(T2Inv);

		/*
		  Center-Center-axis test
		*/
		R CenterDistance = TNumericProperties<R>::Max();
		if (AxisTests & TSeparatingAxisTestConstants::CENTER)
		{
			R Distance = GetSeparatingDistance(_pConvex1, _T1, T1Inv, _pConvex2, _T2, T2Inv, _CenterAxis);
			if (Distance != TNumericProperties<R>::Max())
			{
				CenterDistance = Distance;
				//TVector3<R> CollisionPoint  = Center1 + CenterAxis * (CenterCenterDistance  - Distance) * R(1.0 / 2.0);
				//return Distance;
			}
			else
			{
				return TNumericProperties<R>::Max();
			}
		}

		/*
		  Face test
		*/

		R FaceDistance = TNumericProperties<R>::Max();
		TVector3<R> MinFaceAxis;
		int iMinFaceAxis = -1;
		int iFaceReferenceObject = 0;
		if (AxisTests & TSeparatingAxisTestConstants::FACE)
		{
			int iMinFaceAxis1 = -1;
			int iMinFaceAxis2 = -1;

			TVector3<R> MinFaceAxis1, MinFaceAxis2;

			R MinDistance1 = TNumericProperties<R>::Max();
			R MinDistance2 = TNumericProperties<R>::Max();

			if (!IterateFaces(_pConvex1, _T1, T1Inv, _pConvex2, _T2, T2Inv, _CenterAxis, MinFaceAxis1, iMinFaceAxis1, MinDistance1))
				return TNumericProperties<R>::Max();

			if (!TSeparatingAxisTest<C2, C1, R, AxisTests>::IterateFaces(_pConvex2, _T2, T2Inv, _pConvex1, _T1, T1Inv, -_CenterAxis, MinFaceAxis2, iMinFaceAxis2, MinDistance2))
				return TNumericProperties<R>::Max();

			FaceDistance = Min(MinDistance1, MinDistance2);

			if (MinDistance1 < MinDistance2)
			{
				iMinFaceAxis = iMinFaceAxis1;
				iFaceReferenceObject = 0;
				MinFaceAxis = MinFaceAxis1;
			}
			else
			{
				iMinFaceAxis = iMinFaceAxis2;
				iFaceReferenceObject = 1;
				MinFaceAxis = MinFaceAxis2;
			}
		}

		/*
		  Edge x Edge test
		*/

		R EdgeDistance = TNumericProperties<R>::Max();
		int MinEdge1 = -1, MinEdge2 = -1;
		TVector3<R> MinEdgeAxis;

		if (AxisTests & TSeparatingAxisTestConstants::EDGE)
		{
			IterateEdges(_pConvex1, _T1, T1Inv, _pConvex2, _T2, T2Inv, _CenterAxis, MinEdgeAxis, MinEdge1, MinEdge2, EdgeDistance);
		}

		/*
		  Find best axis
		*/

		R Distance = TNumericProperties<R>::Max();
		if ((AxisTests & TSeparatingAxisTestConstants::CENTER) && CenterDistance < Distance)
		{
			_iFeature = 0;
			_FeatureType = TSeparatingAxisTestConstants::CENTER;
			Distance = CenterDistance;
			_SeparatingAxis = _CenterAxis;
		}

		if ((AxisTests & TSeparatingAxisTestConstants::FACE) && FaceDistance < Distance)
		{
			_iFeature = iMinFaceAxis;
			_FeatureType = TSeparatingAxisTestConstants::FACE;
			Distance = FaceDistance;
			_SeparatingAxis = MinFaceAxis;

			_iReferenceObject = iFaceReferenceObject;
		}

		if ((AxisTests & TSeparatingAxisTestConstants::EDGE) && EdgeDistance < Distance)
		{
			_iFeature = (MinEdge1 << 16) | MinEdge2;
			_FeatureType = TSeparatingAxisTestConstants::EDGE;
			Distance = EdgeDistance;
			_SeparatingAxis = MinEdgeAxis;
		}

		M_ASSERT(Distance < TNumericProperties<R>::Max(), "");

		return Distance;
	}

	/*
	template <class T, int N>
	struct FaceIterator<T, N>
	{
		static void Iterate()
		{
			__vector4 Axes[N];
			__vector4 Distances[N];
			__vector4 IsSeparating;

			TUnroll<>::Iterate(..., Axes);
			TUnroll<>::Iterate(..., Distances, Axes, IsSeparating);

			if (IsSeparating.x == 1)
			{
				return false;
			}


		}
	};
*/


	static bool IterateFaces(C1 *_pConvex1, const TMatrix4<R>& _T1, const TMatrix4<R>& _T1Inv, 
							 C2 *_pConvex2, const TMatrix4<R>& _T2, const TMatrix4<R>& _T2Inv, 
							 const TVector3<R>& _CenterDir,
							 TVector3<R>& _SeparatingAxis,
							 int& _iMinAxis, R& _MinDistance)

	{
		int nF = _pConvex1->GetColinearFaceDirectionCount();

		TVector3<R> SeparatingAxis;
		int iMinAxis;
		R MinDistance;

		MinDistance = TNumericProperties<R>::Max();
		iMinAxis = -1;

		for (int i = 0; i < nF; i++)
		{
			bool HasOpposite;
			TVector3<R> Axis = _pConvex1->GetColinearFaceDirection(i, _T1, HasOpposite);

			int FlippedMask = 0;

			if (HasOpposite && _CenterDir * Axis < 0.0)
			{
				Axis *= R(-1);
				FlippedMask = (1 << TSeparatingAxisTestConstants::FLIPPEDAXISBIT);
			}

			if (_CenterDir * Axis > 0.0)
			{
				R Distance = GetSeparatingDistance(_pConvex1, _T1, _T1Inv, _pConvex2, _T2, _T2Inv, Axis);

				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					iMinAxis = i | FlippedMask;
					SeparatingAxis = Axis;
				}

				if (Distance == TNumericProperties<R>::Max()) 
				{
					_MinDistance = MinDistance;
					_iMinAxis = iMinAxis;
					_SeparatingAxis = SeparatingAxis;

					return false;
				}
			}
		}

		_MinDistance = MinDistance;
		_iMinAxis = iMinAxis;
		_SeparatingAxis = SeparatingAxis;

		// TODO: Egentligen borde return true; funkar här men det gör det inte
		// Kanske pga av precisionsfel.
		return MinDistance < TNumericProperties<R>::Max();
	}

	static bool IterateEdges(const C1 *_pConvex1, const TMatrix4<R>& _T1, const TMatrix4<R>& _T1Inv, 
							 const C2 *_pConvex2, const TMatrix4<R>& _T2, const TMatrix4<R>& _T2Inv,  
							 const TVector3<R>& _CenterDir,
							 TVector3<R>& _MinAxis, 
							 int& _MinEdge1, 
							 int& _MinEdge2, 
							 R& _MinDistance)
	{
		int nE1 = _pConvex1->GetColinearEdgeCount();
		int nE2 = _pConvex2->GetColinearEdgeCount();

		TVector3<R> MinAxis;
		int MinEdge1;
		int MinEdge2; 
		R MinDistance;

		MinAxis.SetScalar(0);
		MinEdge1 = 0;
		MinEdge2 = 0; 
		MinDistance = TNumericProperties<R>::Max();

		for (int i = 0; i < nE1; i++)
		{
			for (int j = 0; j < nE2; j++)
			{
				TVector3<R> edge1, edge2;

				_pConvex1->GetColinearEdge(i, _T1, edge1);
				_pConvex2->GetColinearEdge(j, _T2, edge2);

				TVector3<R> Axis = edge1 / edge2;
				Axis.Normalize();

				//int FlippedMask = 0;
				if (_CenterDir * Axis < 0.0)
				{
					//FlippedMask = (1 << TSeparatingAxisTestConstants::FLIPPEDAXISBIT);
					Axis *= -1;
				}

				if (_CenterDir * Axis > 0.0)
				{
					R Distance = GetSeparatingDistance(_pConvex1, _T1, _T1Inv, _pConvex2, _T2, _T2Inv, Axis);

					if (Distance < MinDistance)
					{
						MinDistance = Distance;
						MinAxis = Axis;
						MinEdge1 = i;
						MinEdge2 = j;
					}

					if (Distance == TNumericProperties<R>::Max()) 
					{
						//_MinAxis = MinAxis;
						//_MinEdge1 = MinEdge1;
						//_MinEdge2 = MinEdge2; 
						//_MinDistance = MinDistance;

						return false;
					}
				}
			}
		}

		_MinAxis = MinAxis;
		_MinEdge1 = MinEdge1;
		_MinEdge2 = MinEdge2; 
		_MinDistance = MinDistance;

		// TODO: Egentligen borde return true; funkar här men det gör det inte
		// Kanske pga av precisionsfel
		return MinDistance < TNumericProperties<R>::Max();
	}

	static R GetSeparatingDistance(const C1 *_Convex1, const TMatrix4<R>& _T1, const TMatrix4<R>& _T1Inv, 
								   const C2 *_Convex2, const TMatrix4<R>& _T2, const TMatrix4<R>& _T2Inv, 
								   const TVector3<R>& _Axis)								  
	{
		R MinPH1, MaxPH1;
		R MinPH2, MaxPH2;

		R d1 = _T1.GetRow(3) * _Axis;
		R d2 = _T2.GetRow(3) * _Axis;

		TVector3<R> A;
		//A = _Axis;
		_Axis.MultiplyMatrix3x3(_T1Inv, A);
		_Convex1->GetMinMax(A, MinPH1, MaxPH1);
		MinPH1 += d1;
		MaxPH1 += d1;

		//A = _Axis;
		_Axis.MultiplyMatrix3x3(_T2Inv, A);
		_Convex2->GetMinMax(A, MinPH2, MaxPH2);
		MinPH2 += d2;
		MaxPH2 += d2;

                // TODO: Detta mycket oklart. Nedan borde gå att ta bort,
                // borde räkna ut samma som ovan. Detta måste dock testas ordentligt!

#if 0
		R _MinPH1, _MaxPH1;
		R _MinPH2, _MaxPH2;

		_Convex1->GetMinMax(_T1, _Axis, _MinPH1, _MaxPH1);
		_Convex2->GetMinMax(_T2, _Axis, _MinPH2, _MaxPH2);

		if ( (M_Fabs(MinPH1 - _MinPH1) > R(0.001)) || 
			(M_Fabs(MaxPH1 - _MaxPH1) > R(0.001)) || 
			(M_Fabs(MinPH2 - _MinPH2) > R(0.001)) || 
			(M_Fabs(MaxPH2 - _MaxPH2) > R(0.001)) )
		{
			int breakme = 0;
		}

/*		MinPH1 = _MinPH1;
		MaxPH1 = _MaxPH1;
		MinPH2 = _MinPH2;
		MaxPH2 = _MaxPH2;*/
#endif

		if (MinPH1 > MaxPH2 || MinPH2 > MaxPH1)
		{
			return TNumericProperties<R>::Max();
		}

		R Distance;
		if (MaxPH2 > MaxPH1 && MinPH1 > MinPH2)
		{
			Distance = Min(MaxPH2 - MinPH1, MaxPH1 - MinPH2);
		}
		else if(MaxPH1 > MaxPH2 && MinPH2 > MinPH1)
		{
			Distance = Min(MaxPH1 - MinPH2, MaxPH2 - MinPH1);
		}
		else
		{
			R mi = Min(MaxPH1, MaxPH2);
			R ma = Max(MinPH1, MinPH2);
			Distance = mi - ma;
		}

		return Distance;
		//return mi - ma;
	}


};

template <class C1, class C2, class R>
class TPolyhedraPolyhedraCollider
{
public:

	typedef TSeparatingAxisTest<C1, C2, R, 
								TSeparatingAxisTestConstants::FACE | TSeparatingAxisTestConstants::EDGE> SeparatingAxisTest;

	static int Collide(C1 *_Polyhedron1, const TMatrix4<R>& _T1, 
					   C2 *_Polyhedron2, const TMatrix4<R>& _T2, 
					   const TVector3<R>& _CenterAxis,
					   TVector3<R> *_pPointOfCollisions, TVector3<R> *_pNormals, R *_pDepths, int _nMaxCollisions);

//protected:
	static int ClipFaces(const C1 *_Polyhedron1, const TMatrix4<R>& _T1, int _iFace1, const TVector3<R>& _Normal, 
						 const C2 *_Polyhedron2, const TMatrix4<R>& _T2, int _iFace2, TVector3<R>* _pResult, R* _pDepths, int _MaxLen);


	static void ClosestPointLineToLine(const TVector3<R>& pa, const TVector3<R>& ua,
									   const TVector3<R>& pb, const TVector3<R>& ub,
									   R& alpha, R& beta);

};

template <class C1, class C2, class R>
int TPolyhedraPolyhedraCollider<C1, C2, R>::ClipFaces(const C1 *_Polyhedron1, const TMatrix4<R>& _T1, int _iFace1, const TVector3<R>& _Normal, 
													  const C2 *_Polyhedron2, const TMatrix4<R>& _T2, int _iFace2, TVector3<R>* _pResult, R* _pDepths, int _MaxLen)
{
	const int MaxVertices = 100;

	TVector3<R> V1[MaxVertices], V2[MaxVertices];
	// TODO: GetFace ska inte finnas i C1 eller C2!!!
	//_Polyhedron1->GetFace(_iFace1, _T1, V1[0], V1[1], V1[2], V1[3]);
	//_Polyhedron2->GetFace(_iFace2, _T2, V2[0], V2[1], V2[2], V2[3]);

	// TODO: Kolla först hur många vertices det är! 
	// Kan skriva över stacken annars

	TPlane3<R> Plane1, Plane2;

	int iN1 = _Polyhedron1->GetFace(_iFace1, _T1, V1, Plane1);
	int iN2 = _Polyhedron2->GetFace(_iFace2, _T2, V2, Plane2);

	M_ASSERT(iN1 < MaxVertices && iN2 < MaxVertices, "!");


#if 1
	//TPlane3<R> Plane1(V1[0], V1[1], V1[2]);
	//TPlane3<R> Plane2(V2[0], V2[1], V2[2]);

#else

	TPlane3<R> Plane1(V1[2], V1[1], V1[0]);
	TPlane3<R> Plane2(V2[2], V2[1], V2[0]);

#endif

#if 1
	TVector3<R> VectorInFace1 = V2[1] - V2[0];
	VectorInFace1.Normalize();

	TVector3<R> VectorInFace2 = VectorInFace1 / Plane2.n;
	VectorInFace2.Normalize();

#else

	TVector3<R> VectorInFace1 = V2[0] - V2[1];
	VectorInFace1.Normalize();

	TVector3<R> VectorInFace2 = VectorInFace1 / Plane2.n;
	VectorInFace2.Normalize();

#endif


	R x1[MaxVertices], y1[MaxVertices];
	R x2[MaxVertices], y2[MaxVertices];

	for (int i = 0; i < iN1; i++)
	{
		x1[i] = VectorInFace1 * V1[i];
		y1[i] = VectorInFace2 * V1[i];
	}

	for (int i = 0; i < iN2; i++)
	{
		x2[i] = VectorInFace1 * V2[i];
		y2[i] = VectorInFace2 * V2[i];
	}


/*
	for (int i = 0; i < 4; i++)
	{
		x1[i] = VectorInFace1 * V1[i];
		y1[i] = VectorInFace2 * V1[i];

		x2[i] = VectorInFace1 * V2[i];
		y2[i] = VectorInFace2 * V2[i];
	}*/

	// TODO: Hur många här?
	TVector2<R> VertexList1[128];
	TVector2<R> VertexList2[128];
	TVector2<R> VertexResult[128];

	for (int i = 0; i < iN1; i++)
	{
#if 1
		VertexList1[i] = TVector2<R>(x1[i], y1[i]);
//		VertexList2[i] = TVector2<R>(x2[3-i], y2[3-i]);
#else
		VertexList1[i] = TVector2<R>(x1[iN1 - 1 -i], y1[iN1 - 1 -i]);
//		VertexList2[i] = TVector2<R>(x2[i], y2[i]);
#endif
	}

	for (int i = 0; i < iN2; i++)
	{
#if 1
//		VertexList1[i] = TVector2<R>(x1[i], y1[i]);
		VertexList2[i] = TVector2<R>(x2[iN2 - 1 -i], y2[iN2 - 1 - i]);
#else
//		VertexList1[i] = TVector2<R>(x1[3-i], y1[3-i]);
		VertexList2[i] = TVector2<R>(x2[i], y2[i]);
#endif
	}

	int nResultVertices = TClipFunctions2D<R>::PolyPoly(VertexList1, 4, VertexList2, 4, VertexResult);

	// TODO: QUICK HACK DUE TO CW AND CCW!!!
	if (nResultVertices == 0)
	{
		for (int i = 0; i < iN1; i++)
		{
#if 1
			VertexList1[i] = TVector2<R>(x1[i], y1[i]);
//			VertexList2[i] = TVector2<R>(x2[i], y2[i]);
#else
			VertexList1[i] = TVector2<R>(x1[3-i], y1[3-i]);
//			VertexList2[i] = TVector2<R>(x2[3-i], y2[3-i]);
#endif
		}

		for (int i = 0; i < iN2; i++)
		{
#if 1
//			VertexList1[i] = TVector2<R>(x1[i], y1[i]);
			VertexList2[i] = TVector2<R>(x2[i], y2[i]);
#else
//			VertexList1[i] = TVector2<R>(x1[3-i], y1[3-i]);
			VertexList2[i] = TVector2<R>(x2[3-i], y2[3-i]);
#endif
		}

	}

	/*
	if (fabs(Plane1.n * CVec3Dfp32(0.0f, 0.0f, 1.0f)) > 0.9f)
	{
		int breakme = 0;
	}*/


	nResultVertices = TClipFunctions2D<R>::PolyPoly(VertexList1, iN1, VertexList2, iN2, VertexResult);

	int nVertices = 0;

//	if (nResultVertices > 0)
//			M_TRACEALWAYS("-----------------------\n");

	for (int i = 0; i < nResultVertices; i++)
	{
		const TVector2<R>& Vertex = VertexResult[i];

		TVector3<R> P = VectorInFace1 * Vertex[0] + VectorInFace2 * Vertex[1] - Plane2.n * Plane2.d;

		R D = Plane1.Distance(P); 
		if (D <= 0.0f)
		{
			_pDepths[nVertices] = fabs(D);
			_pResult[nVertices] = P;
			nVertices++;

			if (nVertices >= _MaxLen)
				return nVertices;

//			M_TRACEALWAYS("%s\n",P.GetString().Str());

			/*if (fabs(D) > 2.0f)
			{
				M_TRACEALWAYS("fooooooooo: %f\n", fabs(D));
				int breakeme = 0;
			}*/
		}

	}

	return nVertices; 
}


template <class C1, class C2, class R>
void TPolyhedraPolyhedraCollider<C1, C2, R>::ClosestPointLineToLine(const TVector3<R>& pa, const TVector3<R>& ua,
																	const TVector3<R>& pb, const TVector3<R>& ub,
																	R& alpha, R& beta)
{
	TVector3<R> p = pb - pa;

	R uaub = ua * ub;
	R q1 =  ua * p;
	R q2 = -(ub * p);
	R d = R(1) - uaub*uaub;
	if (d <= R(0.0001)) {
		alpha = 0;
		beta  = 0;
	}
	else {
		d = R(1.0) / d;
		alpha = (q1 + uaub*q2)*d;
		beta  = (uaub*q1 + q2)*d;
	}
}


template <class C1, class C2, class R>
int TPolyhedraPolyhedraCollider<C1, C2, R>::Collide(C1 *_pPolyhedron1, const TMatrix4<R>& _T1, 
													C2 *_pPolyhedron2, const TMatrix4<R>& _T2, 
													const TVector3<R>& _CenterAxis,
													TVector3<R> *_pPointOfCollisions, TVector3<R>* _pNormals, R* _pDepths, int _nMaxCollisions)
{
	MSCOPESHORT(TPolyhedraPolyhedraCollider::Collide);

	int iFeature=0, FeatureType=0, iReferenceObject=0;
	TVector3<R> SeparatingAxis=0;

	R Distance = SeparatingAxisTest::Collide(_pPolyhedron1, _T1, _pPolyhedron2, _T2, _CenterAxis,
											 SeparatingAxis, iFeature, FeatureType, iReferenceObject);

	if (Distance < TNumericProperties<R>::Max())
	{
		int nPoints = 0;

		int iFeatureSave = iFeature;
		if (FeatureType == TSeparatingAxisTestConstants::FACE)
		{
#if 1
			TVector3<R> ResultPoints[20];
			R Depths[20];
			TVector3<R> TheSeparatingAxis;

			bool Flipped = (iFeature & (1 << TSeparatingAxisTestConstants::FLIPPEDAXISBIT)) != 0;
			iFeature &= TSeparatingAxisTestConstants::FEATUREINDEXMASK;

			if (iReferenceObject == 0)
			{
				bool HasOpposite;
				TheSeparatingAxis = _pPolyhedron1->GetColinearFaceDirection(iFeature, _T1, HasOpposite);
				if (Flipped)
					TheSeparatingAxis *= -1;

				int iReferenceFace = _pPolyhedron1->GetClosestFace(_T1, TheSeparatingAxis);
				int iIncidentFace = _pPolyhedron2->GetClosestFace(_T2, -TheSeparatingAxis);

				nPoints = TPolyhedraPolyhedraCollider<C1, C2, R>::ClipFaces(_pPolyhedron1, _T1, iReferenceFace, TheSeparatingAxis,
																			_pPolyhedron2, _T2, iIncidentFace, ResultPoints, Depths, 20);
			}
			else
			{
				bool HasOpposite;
				TheSeparatingAxis = _pPolyhedron2->GetColinearFaceDirection(iFeature, _T2, HasOpposite);
				if (Flipped)
					TheSeparatingAxis *= -1;

//				if (Flipped)
//					TheSeparatingAxis *= -1;

				int iReferenceFace = _pPolyhedron2->GetClosestFace(_T2, TheSeparatingAxis);
				int iIncidentFace = _pPolyhedron1->GetClosestFace(_T1, -TheSeparatingAxis);

				TheSeparatingAxis *= -1;


				nPoints = TPolyhedraPolyhedraCollider<C2, C1, R>::ClipFaces(_pPolyhedron2, _T2, iReferenceFace, TheSeparatingAxis,
																			_pPolyhedron1, _T1, iIncidentFace, ResultPoints, Depths, 20);
			}

#else
			iFeature = iFeatureSave;

			const C1 *pReference = iReferenceObject == 0 ? _pPolyhedron1 : _pPolyhedron2;
			const C2 *pIncident = iReferenceObject == 0 ? _pPolyhedron2 : _pPolyhedron1;

			const TMatrix4<R> &ReferenceT = iReferenceObject == 0 ? _T1 : _T2;
			const TMatrix4<R> &IncidentT = iReferenceObject == 0 ? _T2 : _T1;

			bool Flipped = (iFeature & (1 << TSeparatingAxisTestConstants::FLIPPEDAXISBIT)) != 0;
			iFeature &= TSeparatingAxisTestConstants::FEATUREINDEXMASK;

			TVector3<R> TheSeparatingAxis = pReference->GetColinearFaceDirection(iFeature, ReferenceT);
			if (Flipped)
				TheSeparatingAxis *= -1;

			int iReferenceFace = pReference->GetClosestFace(ReferenceT, TheSeparatingAxis);
			int iIncidentFace = pIncident->GetClosestFace(IncidentT, -TheSeparatingAxis);

			if (iReferenceObject == 1)
				TheSeparatingAxis *= -1;

			TVector3<R> ResultPoints[20];
			R Depths[20];
			nPoints = TPolyhedraPolyhedraCollider<C1, C2, R>::ClipFaces(pReference, ReferenceT, iReferenceFace, TheSeparatingAxis,
																		pIncident, IncidentT, iIncidentFace, ResultPoints, Depths, 20);

#endif

			nPoints = Min(nPoints, _nMaxCollisions);
			for (int i = 0; i < nPoints; i++)
			{
				_pDepths[i] = Depths[i];
				_pNormals[i] = -TheSeparatingAxis;
				_pPointOfCollisions[i] = ResultPoints[i];
 
				/*
				_pCollisionInfo[i].m_distance = Depths[i];
				_pCollisionInfo[i].m_isColliding = true;
				_pCollisionInfo[i].m_Normal = -SeparatingAxis;
				_pCollisionInfo[i].m_PointOfCollision = ResultPoints[i];
				_pCollisionInfo[i].m_UserData = 0;*/
			}
		}
		else if (FeatureType == TSeparatingAxisTestConstants::EDGE)
		{
			TVector3<R> Support1, Support2;

			_pPolyhedron1->Support(SeparatingAxis, _T1, Support1);
			_pPolyhedron2->Support(-SeparatingAxis, _T2, Support2);

			int iEdge1 = iFeature >> 16 & TSeparatingAxisTestConstants::FEATUREINDEXMASK;
			int iEdge2 = iFeature & 0xffff;

			TVector3<R> Edge1 = 0.0f;
			TVector3<R> Edge2 = 0.0f;
			_pPolyhedron1->GetColinearEdge(iEdge1, _T1, Edge1);
			_pPolyhedron2->GetColinearEdge(iEdge2, _T2, Edge2);

			R s,t;
			TPolyhedraPolyhedraCollider<C1,C2,R>::ClosestPointLineToLine(Support1, Edge1, Support2, Edge2, s, t);
			TVector3<R> P1 = Support1 + Edge1 * s;
			TVector3<R> P2 = Support2 + Edge2 * t;

			_pDepths[0] = Distance;
			_pNormals[0] = -SeparatingAxis;
			_pPointOfCollisions[0] = (P1 + P2) * R(0.5);

			/*
			if (_pCollisionInfo)
			{
				_pCollisionInfo[0].m_distance = Distance;
				_pCollisionInfo[0].m_isColliding = true;
				_pCollisionInfo[0].m_Normal = -SeparatingAxis;
				_pCollisionInfo[0].m_PointOfCollision = (P1 + P2) * R(0.5);
				_pCollisionInfo[0].m_UserData = 1;
			}
			*/

			nPoints = 1;
		}

		return nPoints;
	}

	return 0;
}


// SEP-AXIS-TEST END

#if 0

class CPolyhedra 
{
public:
	class Face
	{
	public:
		Face() {};
		Face(int _iv1, int _iv2, int _iv3, int _iv4, int _iNormal) : iv1(_iv1), iv2(_iv2), iv3(_iv3), iv4(_iv4), iNormal(_iNormal) {}
	
		int iv1, iv2, iv3, iv4;
		int iNormal;
	};


	CPolyhedra();
	
//	virtual void UpdateBoundingSphere();

	int AddQuad(const CVec3Dfp64& V1, const CVec3Dfp64& V2, const CVec3Dfp64& V3, const CVec3Dfp64& V4);

	const TArray<CVec3Dfp64 >& GetVertices() const;
	const TArray<Face>& GetFaces() const;

	int GetVertexCount() const
	{
		return m_lVertices.Len();
	}

	const CVec3Dfp64& GetVertex(int _iVertex) const
	{
		return m_lVertices[_iVertex];
	}

	const Face& GetFace(int _iFace) const
	{
		return m_lFaces[_iFace];
	}

	int GetFaceCount() const 
	{
		return m_lFaces.Len();
	}

	const CVec3Dfp64& GetNormal(int _iNormal) const
	{
		return m_lNormals[_iNormal];
	}

protected:

	int AddVertex(const CVec3Dfp64& V, fp64 epsilon);
	
	TArray<CVec3Dfp64 > m_lVertices;
	TArray<Face> m_lFaces;
	TArray<CVec3Dfp64> m_lNormals;
};


class SeparatingAxisTest
{
public:

	static void Collide(const CPolyhedra& _Poly1, const CMat4Dfp64& _T1, const CPolyhedra& _Poly2, const CMat4Dfp64& _T2);

};

#endif

#endif
