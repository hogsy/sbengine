#include "PCH.h"

#include "WPhysCollision.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

#define CONSTRUCTPLANE(plane,normal,d) do{ \
	plane[0]=normal[0];\
	plane[1]=normal[1];\
	plane[2]=normal[2];\
	plane[3]=d; }while(0)

int CCollisionFunctions::BoxPolygon(const TOBB<fp64>& _Box,
									const CVec3Dfp64 *_pVertices,
									const uint32 *_pVertIndices,
									int _nVertices,
									CCollisionInfo* _pCollisionInfo,
									int _MaxCollisions)
{
	MSCOPESHORT(CCollisionFunctions::BoxPolygon_fp64);
	CVec3Dfp64 tri[3], trirev[3];
	tri[0] = _pVertices[_pVertIndices[0]];
	tri[1] = _pVertices[_pVertIndices[1]];

	int nTotCollisions = 0;
	for( int v = 2; v < _nVertices; v++ )
	{
		tri[2] = _pVertices[_pVertIndices[v]];

		// TODO: Duhh, fixa detta...
		trirev[2] = tri[0];
		trirev[1] = tri[1];
		trirev[0] = tri[2];

		// TODO: Ska man logga om inte alla får plats?
		if (nTotCollisions >= _MaxCollisions)
		{
			return nTotCollisions;
		}

		int nCollisions = BoxTriangle(_Box, trirev, &_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions);
		nTotCollisions += nCollisions;

		tri[1]	= tri[2];
	}
	return nTotCollisions;
}

int CCollisionFunctions::BoxPolygon(const TOBB<fp32>& _Box,
									const CVec3Dfp32 *_pVertices,
									const uint32 *_pVertIndices,
									int _nVertices,
									CCollisionInfo* _pCollisionInfo,
									int _MaxCollisions)
{
	MSCOPESHORT(CCollisionFunctions::BoxPolygon_fp32);
	CVec3Dfp32 tri[3];
	CVec3Dfp64 trirev[3];
	tri[0] = _pVertices[_pVertIndices[0]];
	tri[1] = _pVertices[_pVertIndices[1]];

	TOBB<fp64> _Boxfp64;
	_Boxfp64.m_A[0] = _Box.m_A[0].Getfp64();
	_Boxfp64.m_A[1] = _Box.m_A[1].Getfp64();
	_Boxfp64.m_A[2] = _Box.m_A[2].Getfp64();

	_Boxfp64.m_C = _Box.m_C.Getfp64();
	_Boxfp64.m_E = _Box.m_E.Getfp64();

	int nTotCollisions = 0;
	for( int v = 2; v < _nVertices; v++ )
	{
		tri[2] = _pVertices[_pVertIndices[v]];

		// TODO: Duhh, fixa detta...
		trirev[2] = tri[0].Getfp64();
		trirev[1] = tri[1].Getfp64();
		trirev[0] = tri[2].Getfp64();

		// TODO: Ska man logga om inte alla får plats?

		if (nTotCollisions >= _MaxCollisions)
		{
			return nTotCollisions;
		}
		int nCollisions = BoxTriangle(_Boxfp64, trirev, 
			&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions);

		nTotCollisions += nCollisions;

		tri[1]	= tri[2];
	}
	return nTotCollisions;
}


CVec3Dfp64 GetColumn(const TOBB<fp64> _Box, int iColumn)
{
	CVec3Dfp64 ret;
	ret[0] = _Box.m_A[0][iColumn];
	ret[1] = _Box.m_A[1][iColumn];
	ret[2] = _Box.m_A[2][iColumn];
	return ret;
}

int CCollisionFunctions::BoxTriangle(const TOBB<fp64>& _Box,
									 const CVec3Dfp64* _Vertices,
									 CCollisionInfo* _pCollisionInfo,
									 int _MaxCollisions)
{
	int iBestAxis = 0;
	int iExitAxis = -1;
	fp64 BestDepth = _FP64_MAX;
	CVec3Dfp64 BestNormal(0,0,0);

	CVec3Dfp64 vE0 = _Vertices[1]- _Vertices[0];
	CVec3Dfp64 vE1 = _Vertices[2] - _Vertices[0];
	CVec3Dfp64 vE2 = vE1 - vE0;

	CVec3Dfp64 vN = vE0 / vE1;

	// TODO: vAX ska bort sedan...
	const CVec3Dfp64& vA0 = _Box.m_A[0];
	const CVec3Dfp64& vA1 = _Box.m_A[1];
	const CVec3Dfp64& vA2 = _Box.m_A[2];

	/*	const CVec3Dfp64 vA0 = GetColumn(_Box, 0);
	const CVec3Dfp64 vA1 = GetColumn(_Box, 1);
	const CVec3Dfp64 vA2 = GetColumn(_Box, 2);
	*/

	fp64 fa0 = _Box.m_E.k[0] * 0.5;
	fp64 fa1 = _Box.m_E.k[1] * 0.5;
	fp64 fa2 = _Box.m_E.k[2] * 0.5;

	// calculate relative position between box and triangle
	CVec3Dfp64 vD = _Vertices[0] - _Box.m_C;

	// calculate length of face normal
	fp64 fNLen = vN.Length();

	CVec3Dfp64 vL;
	fp64 fp0, fp1, fp2, fR, fD;


	// Axis 1 - Triangle Normal 
	vL = vN;
	fp0  = vL * vD;
	fp1  = fp0;
	fp2  = fp0;
	fR = fa0*M_Fabs(vN*vA0) + fa1*M_Fabs(vN*vA1) + fa2*M_Fabs(vN*vA2);

	// Axis 1 - Triangle Normal
	if (!BoxTriangle_TestNormal(fp0, fR, vL, 1, BestNormal, iBestAxis, BestDepth))
	{
		iExitAxis = 1;
		return 0;
	}

	// Axis 2 - Box X-Axis
	vL = vA0;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 + vA0 * vE0;
	fp2 = fp0 + vA0 * vE1;
	fR  = fa0;

	if( !BoxTriangle_TestFace( fp0, fp1, fp2, fR, fD, vL, 2, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=2;
		return 0;
	}

	// Axis 3 - Box Y-Axis
	vL = vA1;
	fD = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 + vA1 * vE0;
	fp2 = fp0 + vA1 * vE1;
	fR  = fa1;

	if( !BoxTriangle_TestFace( fp0, fp1, fp2, fR, fD, vL, 3, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=3;
		return 0; 
	}

	// Axis 4 - Box Z-Axis
	vL = vA2;
	fD = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 + vA2 * vE0;
	fp2 = fp0 + vA2 * vE1;
	fR  = fa2;

	if( !BoxTriangle_TestFace( fp0, fp1, fp2, fR, fD, vL, 4, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=4;
		return 0; 
	}

	// Axis 5 - Box X-Axis cross Edge0
	vL = vA0 / vE0;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0;
	fp2 = fp0 + vA0 * vN;
	fR  = fa1 * M_Fabs(vA2 * vE0) + fa2 * M_Fabs(vA1 * vE0);

	if( !BoxTriangle_TestEdge(fp1, fp2, fR, fD, vL, 5, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=5;
		return 0;
	}

	// Axis 6 - Box X-Axis cross Edge1
	vL = vA0 / vE1;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 - vA0 * vN;
	fp2 = fp0;
	fR  = fa1 * M_Fabs(vA2 * vE1) + fa2 * M_Fabs(vA1 * vE1);

	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 6, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=6;
		return 0; 
	}

	// Axis 7 - Box X-Axis cross Edge2
	vL = vA0 / vE2;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 - vA0 * vN;
	fp2 = fp0 - vA0 * vN;
	fR  = fa1 * M_Fabs(vA2 * vE2) + fa2 * M_Fabs(vA1 * vE2);

	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 7, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=7;
		return 0;
	}

	// Axis 8 - Box Y-Axis cross Edge0
	vL = vA1 / vE0;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0;
	fp2 = fp0 + vA1 * vN;
	fR  = fa0 * M_Fabs(vA2 * vE0) + fa2 * M_Fabs(vA0 * vE0);

	if( !BoxTriangle_TestEdge( fp0, fp2, fR, fD, vL, 8, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=8;
		return 0; 
	}

	// Axis 9 - Box Y-Axis cross Edge1
	vL = vA1 / vE1;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 - vA1 * vN;
	fp2 = fp0;
	fR  = fa0 * M_Fabs( vA2 * vE1) + fa2 * M_Fabs(vA0 * vE1);

	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 9, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=9;
		return 0;
	}

	// Axis 10 - Box Y-Axis cross Edge2
	vL = vA1 / vE2;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 - vA1 * vN;
	fp2 = fp0 - vA1 * vN;
	fR  = fa0 * M_Fabs(vA2 * vE2) + fa2 * M_Fabs(vA0 * vE2);

	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 10, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=10;
		return 0;
	}

	// Axis 11 - Box Z-Axis cross Edge0
	vL = vA2 / vE0;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0;
	fp2 = fp0 + vA2 * vN;
	fR  = fa0 * M_Fabs(vA1 * vE0) + fa1 * M_Fabs(vA0 * vE0);


	if( !BoxTriangle_TestEdge( fp0, fp2, fR, fD, vL, 11, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=11;
		return 0; 
	}

	// Axis 12 - Box Z-Axis cross Edge1
	vL = vA2 / vE1;
	fD  = (vL * vN)/fNLen;
	fp0 = (vL * vD);
	fp1 = fp0 - vA2 * vN;
	fp2 = fp0;
	fR  = fa0 * M_Fabs(vA1 * vE1) + fa1 * M_Fabs(vA0 * vE1);

	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 12, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=12;
		return 0; 
	}

	// Axis 13 - Box Z-Axis cross Edge2
	vL = vA2 / vE2;
	fD  = (vL * vN)/fNLen;
	fp0 = vL * vD;
	fp1 = fp0 - vA2 * vN;
	fp2 = fp0 - vA2 * vN;
	fR  = fa0 * M_Fabs(vA1 * vE2) + fa1 * M_Fabs(vA0 * vE2);


	if( !BoxTriangle_TestEdge( fp0, fp1, fR, fD, vL, 13, BestNormal, iBestAxis, BestDepth) ) { 
		iExitAxis=13;
		return 0; 
	}

	if ( iBestAxis == 0 ) {
		return 0;
	}

	return BoxTriangle_Clip(_Box,_Vertices[0], _Vertices[1], _Vertices[2], 
		vE0,vE1,vE2,vN,BestNormal, iBestAxis, BestDepth, _pCollisionInfo, _MaxCollisions);
}

#define CREATEPLANE(plane,normal,d) do{ \
	plane.k[0]=normal.k[0];\
	plane.k[1]=normal.k[1];\
	plane.k[2]=normal.k[2];\
	plane.k[3]=d; }while(0)

//static void _cldClipping(const dVector3 &v0, const dVector3 &v1, const dVector3 &v2) {
int CCollisionFunctions::BoxTriangle_Clip(const TOBB<fp64>& _Box,
										  const CVec3Dfp64& v0, 
										  const CVec3Dfp64& v1, 
										  const CVec3Dfp64& v2,
										  const CVec3Dfp64& _vE0, 
										  const CVec3Dfp64& _vE1, 
										  const CVec3Dfp64& _vE2, 
										  const CVec3Dfp64& _vN,
										  const CVec3Dfp64& _BestNormal,
										  int _iBestAxis,
										  fp64 _BestDepth,
										  CCollisionInfo *_pCollisionInfo,
										  int _MaxCollisions)
{
	int nTotContacts = 0;

	if ( _iBestAxis > 4 )
	{

		CVec3Dfp64 vub,vPb,vPa;

		vPa = _Box.m_C;

		for( int i=0; i<3; i++) {
			CVec3Dfp64 vRotCol = _Box.m_A[i];
			fp64 fSign = _BestNormal * vRotCol > 0 ? 1.0f : -1.0f;

			vPa[0] += fSign * _Box.m_E.k[i]*0.5 * vRotCol.k[0];
			vPa[1] += fSign * _Box.m_E.k[i]*0.5 * vRotCol.k[1];
			vPa[2] += fSign * _Box.m_E.k[i]*0.5 * vRotCol.k[2];
		}

		int iEdge = (_iBestAxis-5)%3;

		if ( iEdge == 0 ) {
			vPb = v0;
			vub = _vE0;
		} else if ( iEdge == 1) {
			vPb = v2;
			vub= _vE1;
		} else {
			vPb = v1;
			vub = _vE2;
		}

		vub.Normalize();

		fp64 fParam1, fParam2;

		CVec3Dfp64 vua;
		int col=(_iBestAxis-5)/3;
		vua = _Box.m_A[col];
		ClosestPointOnTwoLines( vPa, vua, vPb, vub, fParam1, fParam2 );
		vPa = vPa + vua*fParam1;
		vPb = vPb + vub*fParam2;

		CVec3Dfp64 vPntTmp = vPa + vPb;
		vPntTmp *= 0.5;

		if (BoxTriangle_AddCollision(vPntTmp,_BestNormal, -_BestDepth,
			&_pCollisionInfo[0], nTotContacts, _MaxCollisions))
		{
			nTotContacts++;
		}

	} else if ( _iBestAxis == 1 ) {


		CVec3Dfp64 vNormal2 = -_BestNormal;

		CMat4Dfp64 Rotation;
		Rotation.Unit();
		CVec3Dfp64::GetRow(Rotation,0) = _Box.m_A[0];
		CVec3Dfp64::GetRow(Rotation,1) = _Box.m_A[1];
		CVec3Dfp64::GetRow(Rotation,2) = _Box.m_A[2];
		Rotation.Transpose();

		CVec3Dfp64 vNr = vNormal2;
		vNr *= Rotation;

		CVec3Dfp64 vAbsNormal(M_Fabs(vNr.k[0]), M_Fabs(vNr.k[1]), M_Fabs(vNr.k[2]));

		int iB0, iB1, iB2;
		if (vAbsNormal.k[1] > vAbsNormal.k[0]) {
			if (vAbsNormal.k[1] > vAbsNormal.k[2]) {
				iB1 = 0;  iB0 = 1;  iB2 = 2;
			} else {
				iB1 = 0;  iB2 = 1;  iB0 = 2;
			}
		} else {

			if (vAbsNormal.k[0] > vAbsNormal.k[2]) {
				iB0 = 0;  iB1 = 1;  iB2 = 2;
			} else {
				iB1 = 0;  iB2 = 1;  iB0 = 2;
			}
		}

		CVec3Dfp64 vCenter;
		CVec3Dfp64 vRotCol = _Box.m_A[iB0];

		if (vNr[iB0] > 0) {
			// TODO: Hur är det med halva här...

			vCenter.k[0] = _Box.m_C.k[0] - v0.k[0] - _Box.m_E[iB0]*0.5 * vRotCol.k[0];
			vCenter.k[1] = _Box.m_C.k[1] - v0.k[1] - _Box.m_E[iB0]*0.5 * vRotCol.k[1];
			vCenter.k[2] = _Box.m_C.k[2] - v0.k[2] - _Box.m_E[iB0]*0.5 * vRotCol.k[2];
		} else {

			vCenter.k[0] = _Box.m_C.k[0] - v0.k[0] + _Box.m_E[iB0]*0.5 * vRotCol.k[0];
			vCenter.k[1] = _Box.m_C.k[1] - v0.k[1] + _Box.m_E[iB0]*0.5 * vRotCol.k[1];
			vCenter.k[2] = _Box.m_C.k[2] - v0.k[2] + _Box.m_E[iB0]*0.5 * vRotCol.k[2];
		}  

		CVec3Dfp64 avPoints[4];
		CVec3Dfp64 vRotCol2;

		vRotCol = _Box.m_A[iB1];
		vRotCol2 = _Box.m_A[iB2];

		for(int x=0;x<3;x++) {
			avPoints[0].k[x] = vCenter.k[x] + (_Box.m_E.k[iB1]*0.5 * vRotCol.k[x]) - (_Box.m_E.k[iB2]*0.5 * vRotCol2.k[x]);
			avPoints[1].k[x] = vCenter.k[x] - (_Box.m_E.k[iB1]*0.5 * vRotCol.k[x]) - (_Box.m_E.k[iB2]*0.5 * vRotCol2.k[x]);
			avPoints[2].k[x] = vCenter.k[x] - (_Box.m_E.k[iB1]*0.5 * vRotCol.k[x]) + (_Box.m_E.k[iB2]*0.5 * vRotCol2.k[x]);
			avPoints[3].k[x] = vCenter.k[x] + (_Box.m_E.k[iB1]*0.5 * vRotCol.k[x]) + (_Box.m_E.k[iB2]*0.5 * vRotCol2.k[x]);
		}

		CVec3Dfp64 avTempArray1[9];
		CVec3Dfp64 avTempArray2[9];
		CVec4Dfp64 plPlane;

		int iTempCnt1=0;
		int iTempCnt2=0;

		for(int i=0; i<9; i++) {
			avTempArray1[i] = CVec3Dfp64(0,0,0);
			avTempArray2[i] = CVec3Dfp64(0,0,0);
		}

		CVec3Dfp64 vTemp = -_vN;
		vTemp.Normalize();
		CREATEPLANE(plPlane,vTemp,0);

		ClipPolyToPlane( avPoints, 4, avTempArray1, iTempCnt1, plPlane  );

		// Plane p0
		CVec3Dfp64 vTemp2 = v1 - v0;
		vTemp = _vN / vTemp2;
		vTemp.Normalize();
		CREATEPLANE(plPlane,vTemp,0);

		ClipPolyToPlane( avTempArray1, iTempCnt1, avTempArray2, iTempCnt2, plPlane  );


		// Plane p1
		vTemp2 = v2 - v1;
		vTemp = _vN / vTemp2;
		vTemp.Normalize();
		vTemp2 = v0 - v2;
		CREATEPLANE(plPlane,vTemp,vTemp2 * vTemp);

		ClipPolyToPlane( avTempArray2, iTempCnt2, avTempArray1, iTempCnt1, plPlane  );


		// Plane p2
		vTemp2 = v0 - v2;
		vTemp = _vN / vTemp2;
		vTemp.Normalize();
		CREATEPLANE(plPlane,vTemp,0);
		ClipPolyToPlane( avTempArray1, iTempCnt1, avTempArray2, iTempCnt2, plPlane  );

		for ( int i=0; i<iTempCnt2; i++ ) {
			fp64 fTempDepth = vNormal2 * avTempArray2[i];

			if (fTempDepth > 0) {
				fTempDepth = 0;
			}

			CVec3Dfp64 vPntTmp = avTempArray2[i] + v0;

			if (BoxTriangle_AddCollision(vPntTmp,_BestNormal, fTempDepth,
				&_pCollisionInfo[0], nTotContacts, _MaxCollisions))
			{
				nTotContacts++;
			} else {
				break;
			}
		}

	} else { // 2 <= if _iBestAxis <= 4

		CVec3Dfp64 vNormal2 = _BestNormal;

		int iA0,iA1,iA2;
		iA0 = _iBestAxis-2;
		if ( iA0 == 0 ) {
			iA1 = 1; iA2 = 2;
		} else if ( iA0 == 1 ) {
			iA1 = 0; iA2 = 2;
		} else {
			iA1 = 0; iA2 = 1;
		}

		CVec3Dfp64 avPoints[3];

		avPoints[0] = v0 - _Box.m_C;
		avPoints[1] = v1 - _Box.m_C;
		avPoints[2] = v2 - _Box.m_C;

		CVec3Dfp64 avTempArray1[9];
		CVec3Dfp64 avTempArray2[9];

		int iTempCnt1, iTempCnt2;

		for(int i=0; i<9; i++) {
			avTempArray1[i] = CVec3Dfp64(0,0,0);
			avTempArray2[i] = CVec3Dfp64(0,0,0);
		}


		CVec4Dfp64 plPlane;

		CVec3Dfp64 vTemp = -vNormal2;
		CONSTRUCTPLANE(plPlane,vTemp,_Box.m_E[iA0]*0.5);

		ClipPolyToPlane( avPoints, 3, avTempArray1, iTempCnt1, plPlane );


		vTemp = _Box.m_A[iA1];
		CONSTRUCTPLANE(plPlane,vTemp,_Box.m_E[iA1]*0.5);

		ClipPolyToPlane( avTempArray1, iTempCnt1, avTempArray2, iTempCnt2, plPlane );


		// Plane p1
		vTemp = -_Box.m_A[iA1];
		CONSTRUCTPLANE(plPlane,vTemp,_Box.m_E[iA1]*0.5);

		ClipPolyToPlane( avTempArray2, iTempCnt2, avTempArray1, iTempCnt1, plPlane );


		// Plane p2
		vTemp = _Box.m_A[iA2];
		CONSTRUCTPLANE(plPlane,vTemp,_Box.m_E[iA2]*0.5);

		ClipPolyToPlane( avTempArray1, iTempCnt1, avTempArray2, iTempCnt2, plPlane );


		// Plane p3
		vTemp = -_Box.m_A[iA2];
		CONSTRUCTPLANE(plPlane,vTemp,_Box.m_E[iA2]*0.5);

		ClipPolyToPlane( avTempArray2, iTempCnt2, avTempArray1, iTempCnt1, plPlane );

		for ( int i=0; i<iTempCnt1; i++ ) {
			fp64 fTempDepth = vNormal2 * avTempArray1[i] - _Box.m_E[iA0]*0.5;

			if (fTempDepth > 0) {
				fTempDepth = 0;
			}

			CVec3Dfp64 vPntTmp = avTempArray1[i] + _Box.m_C;

			if (BoxTriangle_AddCollision(vPntTmp,_BestNormal, fTempDepth,
				&_pCollisionInfo[0], nTotContacts, _MaxCollisions))
			{
				nTotContacts++;
			} else {
				break;
			}
		}
	}

	return nTotContacts;
}

bool CCollisionFunctions::BoxTriangle_TestNormal(fp64 _fp0, 
												 fp64 _fR, 
												 const CVec3Dfp64& _vNormal, 
												 int _iAxis,
												 CVec3Dfp64& _BestNormal,
												 int& _BestAxis,
												 fp64& _BestDepth) 
{
	fp64 fDepth = _fR + _fp0;

	if ( fDepth<0 ) { 
		return false;
	}

	fp64 fLength = _vNormal.Length();

	if ( fLength > 0.0f ) {

		fp64 fOneOverLength = 1.0f / fLength;

		fDepth = fDepth*fOneOverLength;

		if (fDepth < _BestDepth) {
			_BestNormal.k[0] = -_vNormal.k[0]*fOneOverLength;
			_BestNormal.k[1] = -_vNormal.k[1]*fOneOverLength;
			_BestNormal.k[2] = -_vNormal.k[2]*fOneOverLength;
			_BestAxis = _iAxis;
			M_ASSERT(fDepth >= 0,"");
			_BestDepth = fDepth;
		}
	}
	return true;
}


bool CCollisionFunctions::BoxTriangle_TestFace(fp64 _fp0, fp64 _fp1, fp64 _fp2, 
											   fp64 _fR, fp64 _fD, 
											   CVec3Dfp64& _vNormal, 
											   int _iAxis,
											   CVec3Dfp64& _BestNormal,
											   int& _BestAxis,
											   fp64& _BestDepth) 

{
	fp64 fMin, fMax;

	if ( _fp0 < _fp1 ) {
		if ( _fp0 < _fp2 ) {
			fMin = _fp0;
		} else {
			fMin = _fp2;
		}
	} else {
		if( _fp1 < _fp2 ) {
			fMin = _fp1; 
		} else {
			fMin = _fp2;
		}
	}

	if ( _fp0 > _fp1 ) {
		if ( _fp0 > _fp2 ) {
			fMax = _fp0;
		} else {
			fMax = _fp2;
		}
	} else {
		if( _fp1 > _fp2 ) {
			fMax = _fp1; 
		} else {
			fMax = _fp2;
		}
	}

	fp64 fDepthMin = _fR - fMin;
	fp64 fDepthMax = fMax + _fR;

	if ( fDepthMin < 0 || fDepthMax < 0 ) {
		return false;
	}

	fp64 fDepth = 0;

	if ( fDepthMin > fDepthMax ) {
		fDepth = fDepthMax;
		_vNormal *= -1.0;
		_fD = -_fD;
	} else {
		fDepth = fDepthMin;   
	}

	if (fDepth<_BestDepth) {
		_BestNormal = _vNormal;
		_BestAxis    = _iAxis;
		M_ASSERT(fDepth >= 0, "");
		_BestDepth   = fDepth;
	}

	return true;
}


bool CCollisionFunctions::BoxTriangle_TestEdge(fp64 _fp0, fp64 _fp1, 
											   fp64 _fR, fp64 _fD, 
											   const CVec3Dfp64& _vNormal, 
											   int _iAxis,
											   CVec3Dfp64& _BestNormal,
											   int& _BestAxis,
											   fp64& _BestDepth)
{
	fp64 fMin, fMax;

	if ( _fp0 < _fp1 ) {
		fMin = _fp0;
		fMax = _fp1;
	} else {
		fMin = _fp1;
		fMax = _fp0;    
	}

	fp64 fDepthMin = _fR - fMin;
	fp64 fDepthMax = fMax + _fR;

	if ( fDepthMin < 0 || fDepthMax < 0 ) {
		return false;
	}

	fp64 fDepth;

	bool FlipNormal = false;

	if ( fDepthMin > fDepthMax ) {
		fDepth = fDepthMax;
		FlipNormal = true;
		_fD = -_fD;
	} else {
		fDepth = fDepthMin;   
	}

	fp64 fLength = _vNormal.Length();

	if ( fLength > 0.0 ) {

		fp64 fOneOverLength = 1.0 / fLength;
		fDepth = fDepth*fOneOverLength;
		_fD *= fOneOverLength;


		if (fDepth*1.5 < _BestDepth) {
			_BestNormal = _vNormal * fOneOverLength;
			if (FlipNormal)
				_BestNormal *= -1;

			_BestAxis    = _iAxis;
			_BestDepth   = fDepth;
		}
	}

	return true;
}
