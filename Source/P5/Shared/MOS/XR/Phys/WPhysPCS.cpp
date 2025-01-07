
//-------------------------------------------------------------------------------------------
//
// Physics Potential Collision Set		Created and Added by Joacim Jonsson 03/02/26 (yymmdd)
//
// Dont fucking touch the structs and variables unless you _really_ know what you
// are doing as there is probably a ton of PS2 specific assembler accessing them.
//
// Class was created in order to optimize physics/collision engine, it is using a lot
// of types such as float and int, but this is not considered to violate SBZ code standard
// since it is on _purpose_ to ensure platform _native_ types.
//
//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------



/*

Ofta förekommande notation:

u, v, w -> vektorer.
W 		-> plan ekvation
T0, T1	-> Transformations matris vid tidpunkt t = 0, t = 1 (lokal tid)



Det finns en #define USE_PCS i filen, kommenteras den bort så körs gamla fysik koden.

Den gamla fysik koden är modifierad med CWorld_PhysState::Object_MovePhysical som entrypoint.

Intressanta ställen i koden har en kommentar "//MUPPJOKKO - IMPLEMENT ME!" man kan söka på för att
snabbt hitta.



Flöde:

En instans av klassen CPotColSet  (Potential Collision Set) deklareras där och Selection_GetArray anropas.

	Selection_GetArray (aningen missvisande namn!) tar en Selection går egenom alla objekt och gör
	CollectPCS på dem.

		CollectPCS adderar alla fysik-primitiver i ett objekt som (delvis) omfattas av kollisions mängdens
		gränser. (PCS bounds)


Eftersom primitiverna i mängden kan komma från olika objekt och kan ha olika flaggar (intersect, notify1/2)
lagras även dessa i CPotColSet instansen. Ytors planekvationer lagras också, det är tänkt att de skall
användas för att snabba upp lågnivå kollisions testerna senare.



Sedan följer fysiken samma flöde som den gamla, men skickar en pekare till kollisions mängden istället
för selection index till alla funktioner. (överlagrade så de gamla finns kvar)


När ett nytt slideplan läggs till används det för att sålla bort ytor ur kollisions mängden.
(antas omöjligt att kollidera med primitiver bakom slideplan) Detta görs av funktionen CPotColSet::PlaneSieve.

Se kommentarer i koden för mer specifika detaljer om vad individuella funktioner gör.








Buggar:

	- uppdatering av PCS bounds, just nu tas bara bounds en gång + lite marginal.


Fixa:

	- stödja alla primitivtyper
	
		I filen WPhysPCS.cpp, funktionerna som anropas när man vill kollidera en primitiv mot ett PCS:

		CollideClosestPoint		-	kolliderar inte mot några primitiver
		CollideClosestSphere	-	kolliderar inte mot några primitiver
		CollideClosestBox		-	kolliderar bara mot Faces och Boxes


	- stödja roterade boxar mera?

		Boxar är just nu axis aligned i världskordinater mer eller mindre genomgående,
		det lagras ingen orientering på dem i PCS intansen.
		



Algoritm Opta***:

	- transformera box till bsp'ns lokala rymd vid CollectPCS. (som gamla fysiken gör)
		-> måste hantera roterade boxar överallt, alternativ räkna om axisaligned bounds av transformerade boxar.

	- hantera multipla slide-plan simultant -> kraftfull aningen besvärlig utökning av algoritm...




Kod Opta:

	- bygga bort dynamic casts!!

	- PS2: explodera PCS operationer så att beräkningsvägarna blir klasslokala.

	- PS2: optimera PCS operationer i (vu0) assembler.




Data/Struktur Opta:

	- low-quality primitiver / faces för kollision kanske borde införas, en karaktär som rör sig behöver
		bara väldigt approximativa kollisioner. Medans kanske ett armborst / gevärs skott etc
		behöver mer exakta. Karaktärer skulle alltså kunna enbart ta hänsyn till de faces som
		är speciellt flaggade för det osv.

	- för mycket parametrar som tas hänsyn till som inte verkar ge mycket effekt.
		exempelvis get user acceleration, impact hantering m.m. långsamma.
		Kanske förenklas och ignorera massa parametrar?


*/


#include "PCH.h"
#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

#include "WPhysPCS.h"
#include "../../Classes/GameWorld/WPhysState.h"


#undef COLL_RASTER

static void PCSDebug_RenderAABB(const CBox3Dfp32* _pBox, const CVec3Dfp32& _Pos, CPixel32 _Color);
static void PCSDebug_RenderFace(const CVec3Dfp32* _pVerts, CPixel32 _Color);
#ifndef M_RTM
# define DEBUG_RENDER_AABB(box, pos, color) PCSDebug_RenderAABB(box, pos, color)
# define DEBUG_RENDER_FACE(verts, color)    PCSDebug_RenderFace(verts, color)
#else
# define DEBUG_RENDER_AABB(box, pos, color)
# define DEBUG_RENDER_FACE(verts, color)
#endif


void MCCDLLEXPORT CPotColSet::Clear()
{
	m_nFaces = 0; 
	m_nPoints = 0; 
	m_nSpheres = 0; 
	m_nBoxes = 0; 

#ifdef PCSCHECK_INSOLID
	m_bIsInSolid = false; 
	m_iSolidObj = -1;
#endif
}


/*

	A primitive is Valid for the set if it is (partially) within the bounds of the set.

*/

int MCCDLLEXPORT CPotColSet::ValidPoint( const float *_fPoint ) const
{
	if( _fPoint[0] <= m_BoxMinMax[0] || _fPoint[0] >= m_BoxMinMax[3] ||
		_fPoint[1] <= m_BoxMinMax[1] || _fPoint[1] >= m_BoxMinMax[4] ||
		_fPoint[2] <= m_BoxMinMax[2] || _fPoint[2] >= m_BoxMinMax[5] )
	{
		return 0;
	}
	
	return 1;
}


int MCCDLLEXPORT CPotColSet::ValidPlane(const float *_fPlaneEq) const
{
	register float eq = PlanePointDistance( _fPlaneEq, m_fCenter[0], m_fCenter[1], m_fCenter[2] );

	if( M_Fabs( eq ) > m_fRadius )
	{ // plane of face does not slice approximating sphere
		return 0;
	}

	if( PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[1], m_BoxMinMax[2] ) > 0.f )
	{
		if( PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[1], m_BoxMinMax[2] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[1], m_BoxMinMax[5] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[1], m_BoxMinMax[5] ) <= 0.f ||
//			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[1], m_BoxMinMax[2] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[4], m_BoxMinMax[5] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[4], m_BoxMinMax[5] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[4], m_BoxMinMax[2] ) <= 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[4], m_BoxMinMax[2] ) <= 0.f )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if( PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[1], m_BoxMinMax[2] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[1], m_BoxMinMax[5] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[1], m_BoxMinMax[5] ) > 0.f ||
//			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[1], m_BoxMinMax[2] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[4], m_BoxMinMax[5] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[4], m_BoxMinMax[5] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[3], m_BoxMinMax[4], m_BoxMinMax[2] ) > 0.f ||
			PlanePointDistance( _fPlaneEq, m_BoxMinMax[0], m_BoxMinMax[4], m_BoxMinMax[2] ) > 0.f )
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	return 1;
}


int MCCDLLEXPORT CPotColSet::ValidSphere( const float *_fSphere ) const
{ // input is on the form [x,y,z,r]

	register float x = _fSphere[0] - m_fCenter[0];
	register float y = _fSphere[1] - m_fCenter[1];
	register float z = _fSphere[2] - m_fCenter[2];

	register float r = _fSphere[3] + m_fRadius;

	if( (x * x + y * y + z * z) > (r * r) )
	{
		return 0;
	}
	
	// MUPPJOKKO - IMPLEMENT ME!	more accurate tests could be added here, but might be better off without
	
	return 1;
}



int MCCDLLEXPORT CPotColSet::ValidFace( const float *_fPlaneEq, const float *_fFace ) const
{ // does the triangle intersect with the Box of this set?

	if( !ValidPlane( _fPlaneEq ) )
	{ // plane of face does not bounds at all
		return 0;
	}

	// all points in the Face on the outer any side of the boundbox? (21 compares)

	if( _fFace[0] < m_BoxMinMax[0] && _fFace[3] < m_BoxMinMax[0] && _fFace[6] < m_BoxMinMax[0] ) return 0;
	if( _fFace[1] < m_BoxMinMax[1] && _fFace[4] < m_BoxMinMax[1] && _fFace[7] < m_BoxMinMax[1] ) return 0;
	if( _fFace[2] < m_BoxMinMax[2] && _fFace[5] < m_BoxMinMax[2] && _fFace[8] < m_BoxMinMax[2] ) return 0;
	if( _fFace[0] > m_BoxMinMax[3] && _fFace[3] > m_BoxMinMax[3] && _fFace[6] > m_BoxMinMax[3] ) return 0;
	if( _fFace[1] > m_BoxMinMax[4] && _fFace[4] > m_BoxMinMax[4] && _fFace[7] > m_BoxMinMax[4] ) return 0;
	if( _fFace[2] > m_BoxMinMax[5] && _fFace[5] > m_BoxMinMax[5] && _fFace[8] > m_BoxMinMax[5] ) return 0;

	// as long as all exits below returns 1 there is no point of conducting them.
	// i.e remove this return if adding more in-depth tests that can return 0
	return 1; 

	if( _fFace[0] < m_BoxMinMax[3] && _fFace[1] < m_BoxMinMax[4] && _fFace[2] < m_BoxMinMax[5] &&
		_fFace[0] > m_BoxMinMax[0] && _fFace[1] < m_BoxMinMax[1] && _fFace[2] < m_BoxMinMax[2] )
	{ // point is inside m_BoxMinMax
		return 1;
	}

	if( _fFace[3] < m_BoxMinMax[3] && _fFace[4] < m_BoxMinMax[4] && _fFace[5] < m_BoxMinMax[5] &&
		_fFace[3] > m_BoxMinMax[0] && _fFace[4] < m_BoxMinMax[1] && _fFace[5] < m_BoxMinMax[2] )
	{ // point is inside m_BoxMinMax
		return 1;
	}

	if( _fFace[6] < m_BoxMinMax[3] && _fFace[7] < m_BoxMinMax[4] && _fFace[8] < m_BoxMinMax[5] &&
		_fFace[6] > m_BoxMinMax[0] && _fFace[7] < m_BoxMinMax[1] && _fFace[8] < m_BoxMinMax[2] )
	{ // point is inside m_BoxMinMax
		return 1;
	}


	// MUPPJOKKO - IMPLEMENT ME!	more accurate tests could be added here, but might be better off without

	return 1;
}


int MCCDLLEXPORT CPotColSet::ValidBox( const float *_BoxMinMax ) const
{
	if( _BoxMinMax[0] < m_BoxMinMax[0] || _BoxMinMax[1] < m_BoxMinMax[1] || _BoxMinMax[2] < m_BoxMinMax[2] ||
		_BoxMinMax[3] > m_BoxMinMax[3] || _BoxMinMax[4] > m_BoxMinMax[4] || _BoxMinMax[5] > m_BoxMinMax[5] )
	{ // _BoxMinMax is not within set boundingbox
		return 0;
	}
	else
	{ // _BoxMinMax is within set boundingbox
		return 1;
	}
}


int MCCDLLEXPORT CPotColSet::ValidBound( const float *_BoxMinMax ) const
{
	if( _BoxMinMax[0] > m_BoxMinMax[0] && _BoxMinMax[1] > m_BoxMinMax[1] && _BoxMinMax[2] > m_BoxMinMax[2] &&
		_BoxMinMax[3] < m_BoxMinMax[3] && _BoxMinMax[4] < m_BoxMinMax[4] && _BoxMinMax[5] < m_BoxMinMax[5] )
	{ // _BoxMinMax is completely within set bounds
		return 1;
	}
	else
	{
		return 0;
	}
}






void MCCDLLEXPORT CPotColSet::SubSet( const float *_BoxMinMax,  CPotColSet *_pcs )
{
	M_ASSERT( _pcs->ValidBound( _BoxMinMax ), "CPotColSet::SubSet Box was not within super set" );
	
	int i, n;
	
	m_nFaces = 0;
	SetBox( _BoxMinMax );

	for( i = 0, n = _pcs->m_nPoints; i < n; i++ )
	{
		if( ValidPoint( _pcs->m_fPoints[i] ) )
		{
			AddPoint( _pcs->m_PointFlags[i], _pcs->m_iObjPoints[i], _pcs->m_fPoints[i] );
		}
	}


	for( i = 0, n = _pcs->m_nSpheres; i < n; i++ )
	{
		if( ValidSphere( _pcs->m_fSpheres[i] ) )
		{
			AddSphere( _pcs->m_SphereFlags[i], _pcs->m_iObjSpheres[i], _pcs->m_fSpheres[i], _pcs->m_fSpheres[i][3] );
		}
	}


	for( i = 0, n = _pcs->m_nFaces; i < n; i++ )
	{
		if( ValidFace( _pcs->m_fPlaneEqs[i], _pcs->m_fFaces[i] ) )
		{
			AddFace( _pcs->m_FaceFlags[i], _pcs->m_iObjFaces[i], _pcs->m_fPlaneEqs[i], _pcs->m_fFaces[i], _pcs->m_pSurface[i] );
		}
	}

	for( i = 0, n = _pcs->m_nBoxes; i < n; i++ )
	{
		if( ValidBox( _pcs->m_fBoxes[i] ) )
		{
			AddBox( _pcs->m_BoxFlags[i], _pcs->m_iObjBoxes[i], _pcs->m_fBoxes[i] );
		}
	}

}





/*

	private helper functions, theese should be written in assembler for the PS2.
	Also more functions for other possible primitive combinations should be added.
	The FaceBox function should take advantage of the already known plane equation.


*/


// MUPPJOKKO - IMPLEMENT ME!	wrapping old code for now
int MCCDLLEXPORT CPotColSet::IntersectFaceBox( const float *_fFace, const CMat4Dfp32 *_T0, const CMat4Dfp32 *_T1, const float *_BoxMinMax, CCollisionInfo *_pCollInfo)
{

	CVec3Dfp32 Extent( 0.5f * (_BoxMinMax[3] - _BoxMinMax[0]), 0.5f * (_BoxMinMax[4] - _BoxMinMax[1]), 0.5f * (_BoxMinMax[5] - _BoxMinMax[2]) );
#ifdef PLATFORM_PS2_
	CPhysAABB  AABBT0(CVec3Dfp32::GetMatrixRow(*_T0, 3), Extent);
	CPhysAABB  AABBT1(CVec3Dfp32::GetMatrixRow(*_T1, 3), Extent);

	return Phys_Intersect_TriAABB( (CVec3Dfp32 *)_fFace, AABBT0, AABBT1, false, _pCollInfo );
#else
	CPhysOBB  BoxT0;
	CPhysOBB  BoxT1;

	BoxT0.SetDimensions( Extent );
	BoxT1.SetDimensions( Extent );

	BoxT0.SetPosition( *_T0 );
	BoxT1.SetPosition( *_T1 );

	return Phys_Intersect_TriOBB( (CVec3Dfp32 *)_fFace, BoxT0, BoxT1, false, _pCollInfo );

#endif
}



// MUPPJOKKO - IMPLEMENT ME!	wrapping old code for now
int MCCDLLEXPORT CPotColSet::IntersectBoxBox( const float *_Box, const CMat4Dfp32 *_T0, const CMat4Dfp32 *_T1, const float *_BoxMinMax, CCollisionInfo *_pCollInfo)
{
	CPhysOBB  Box;
	CPhysOBB  BoxT0;
	CPhysOBB  BoxT1;

	CVec3Dfp32 Extent( 0.5f * (_BoxMinMax[3] - _BoxMinMax[0]), 0.5f * (_BoxMinMax[4] - _BoxMinMax[1]), 0.5f * (_BoxMinMax[5] - _BoxMinMax[2]) );

	BoxT0.SetDimensions( Extent );
	BoxT1.SetDimensions( Extent );

	BoxT0.SetPosition( *_T0 );
	BoxT1.SetPosition( *_T1 );

	Box.m_C = CVec3Dfp32(  0.5f * (_Box[3] + _Box[0]), 0.5f * (_Box[4] + _Box[1]), 0.5f * (_Box[5] + _Box[2]) );
	Box.m_E = CVec3Dfp32(  0.5f * (_Box[3] - _Box[0]), 0.5f * (_Box[4] - _Box[1]), 0.5f * (_Box[5] - _Box[2]) );
	Box.m_A[0] = CVec3Dfp32( 1.f, 0.f, 0.f );
	Box.m_A[1] = CVec3Dfp32( 0.f, 1.f, 0.f );
	Box.m_A[2] = CVec3Dfp32( 0.f, 0.f, 1.f );

	return Phys_Intersect_OBB( Box, BoxT0, BoxT1, _pCollInfo );
}






//AR-NOTE: This might be a bit too slow if there are a lot of hits.. (linear search for old entries)
static int AddNewOrImprove(const CCollisionInfo& _CInfo, int _nNumCollInfo, CCollisionInfo* _pCollInfo, int _nMaxCollInfo)
{
	CCollisionInfo* pExisting = NULL;
	for (int j=0; j<_nNumCollInfo; j++)
	{
		if (_pCollInfo[j].m_iObject == _CInfo.m_iObject)
		{
			pExisting = _pCollInfo + j;
			pExisting->Improve(_CInfo);
			return 0; // no new added
		}
	}

	if (_nNumCollInfo >= _nMaxCollInfo)
		return -1; // output buffer is full!

	_pCollInfo[_nNumCollInfo] = _CInfo;
	return 1; // new entry added
}




/*

	Functions for finding the closest intersection between a primitive and
	all the primitives within the set.


*/




// MUPPJOKKO - IMPLEMENT ME!
int MCCDLLEXPORT CPotColSet::CollidePoint( const float *_u, const float *_v, CCollisionInfo *_pCollisionInfo, int _nMaxCollInfo)
{ // _u, _v -> [x(t0),y(t0),z(t0)], [x(t1),y(t1),z(t1)] 
	MSCOPESHORT(CPotColSet::CollidePoint_NotImplemented);
	int i;

	bool ret = false;

	for( i = 0; i < m_nPoints; i++ )
	{
	}

	for( i = 0; i < m_nSpheres; i++ )
	{
	}

	for( i = 0; i < m_nFaces; i++ )
	{
	}

	return ret;
}

// MUPPJOKKO - IMPLEMENT ME!
int MCCDLLEXPORT CPotColSet::CollideSphere( const float *_u, const float *_v, const float _radius, CCollisionInfo *_pCollisionInfo, int _nMaxCollInfo)
{ // _u, _v -> [x(t0),y(t0),z(t0)], [x(t1),y(t1),z(t1)] 
	MSCOPESHORT(CPotColSet::CollideSphere_NotImplemented);
	int i;
	
	bool ret = false;

	for( i = 0; i < m_nPoints; i++ )
	{
	}

	for( i = 0; i < m_nSpheres; i++ )
	{

	}

	for( i = 0; i < m_nFaces; i++ )
	{

	}

	return ret;
}



int MCCDLLEXPORT CPotColSet::CollideBox(const CMat4Dfp32 *_T0, const CMat4Dfp32 *_T1, const float *_BoxMinMax, CCollisionInfo* _pCollInfo, int _nMaxCollInfo)
{ // _T0, _T1 -> matrices at time t = 0 and t = 1.
	MSCOPESHORT(CPotColSet::CollideBox);

	DEBUG_GS_BGCOLOR( 0xff0000 );

	/*bool bBoxIsInsidePCS = ((_BoxMinMax[0]+_T1->k[3][0] >= m_BoxMinMax[0]) && 
	                        (_BoxMinMax[1]+_T1->k[3][1] >= m_BoxMinMax[1]) && 
	                        (_BoxMinMax[2]+_T1->k[3][2] >= m_BoxMinMax[2]) && 
	                        (_BoxMinMax[3]+_T1->k[3][0] <= m_BoxMinMax[3]) && 
	                        (_BoxMinMax[4]+_T1->k[3][1] <= m_BoxMinMax[4]) && 
	                        (_BoxMinMax[5]+_T1->k[3][2] <= m_BoxMinMax[5]));
	M_ASSERT(bBoxIsInsidePCS, "Box outside PCS!");*/

	int i;
	int nColl = 0;

	CCollisionInfo CInfo;
	CCollisionInfo *pCInfo = _pCollInfo ? &CInfo : NULL;

	CVec3Dfp32 N = CVec3Dfp32::GetMatrixRow( *_T1, 3 ) - CVec3Dfp32::GetMatrixRow( *_T0, 3 ); // Normal Sieve

	// MUPPJOKKO - IMPLEMENT ME!	- implementera resten av primitiverna också!

	DEBUG_RENDER_AABB((const CBox3Dfp32*)_BoxMinMax, CVec3Dfp32::GetMatrixRow(*_T0, 3), 0xffff0000);
	DEBUG_RENDER_AABB((const CBox3Dfp32*)_BoxMinMax, CVec3Dfp32::GetMatrixRow(*_T1, 3), 0xff7f00ff);

	for (i = 0; i < m_nFaces; i++)
	{
		if ( (m_FaceFlags[i] & (PCS_DO_NOTIFY1 | PCS_DO_NOTIFY2)) || (N * *(CVec3Dfp32 *)m_fPlaneEqs[i] < PCS_SIGMA) )
//		if( N * *(CVec3Dfp32 *)m_fPlaneEqs[i] < PCS_SIGMA )
		{ // passed Normal Sieve...
	
			CInfo.m_bIsValid = false;

			if (IntersectFaceBox(m_fFaces[i], _T0, _T1, _BoxMinMax, pCInfo))
			{
				DEBUG_RENDER_FACE((const CVec3Dfp32*)m_fFaces[i], 0xffff0000);

				if (!_pCollInfo)
				{ // function call was a yes-no query only
					return 1;
				}

				CInfo.m_iObject = m_iObjFaces[i];
				CInfo.m_IN1N2Flags = m_FaceFlags[i];
				CInfo.m_pSurface = const_cast<CXW_Surface*>(m_pSurface[i]);

				int result = AddNewOrImprove(CInfo, nColl, _pCollInfo, _nMaxCollInfo);
				if (result == -1)
					return nColl; // sorry, but there's no room for more collision information (should not happen!)
				else
					nColl += result; // 0 or 1
			}
		}
	}


	for (i = 0; i < m_nBoxes; i++)
	{
		CInfo.m_bIsValid = false;
	
		if (IntersectBoxBox(m_fBoxes[i], _T0, _T1, _BoxMinMax, pCInfo))
		{ 
			if (!_pCollInfo)
			{ // function call was a yes-no query only
				return 1;
			}

			CInfo.m_iObject = m_iObjBoxes[i];
			CInfo.m_IN1N2Flags = m_BoxFlags[i];

			int result = AddNewOrImprove(CInfo, nColl, _pCollInfo, _nMaxCollInfo);
			if (result == -1)
				return nColl; // sorry, but there's no room for more collision information (should not happen!)
			else
				nColl += result; // 0 or 1
		}
	}

	DEBUG_GS_BGCOLOR( 0x00ff00 );

	return nColl;
}







/*

	Functions for adding primitives to the set

*/



void MCCDLLEXPORT CPotColSet::AddPoint( const uint8 _iFlags, const int _iObj, const float *_fPoint )
{
	M_ASSERT(m_nPoints < MAX_PCS_POINTS, "CPotColSet::Add  MAX_PCS_POINTS too small");

	register float *ptr = m_fPoints[m_nPoints];

	*ptr++ = *_fPoint++;
	*ptr++ = *_fPoint++;
	*ptr++ = *_fPoint++;

	m_iObjPoints[m_nPoints] = _iObj;
	m_PointFlags[m_nPoints] = _iFlags;
	m_nPoints++;
}


void MCCDLLEXPORT CPotColSet::AddSphere( const uint8 _iFlags, const int _iObj, const float *_fPos, const float _radius )
{
	M_ASSERT(m_nSpheres < MAX_PCS_SPHERES, "CPotColSet::Add  MAX_PCS_SPHERES too small");

	register float *ptr = m_fSpheres[m_nSpheres];

	*ptr++ = *_fPos++;
	*ptr++ = *_fPos++;
	*ptr++ = *_fPos++;
	*ptr++ = _radius;

	m_iObjSpheres[m_nSpheres] = _iObj;
	m_SphereFlags[m_nSpheres] = _iFlags;
	m_nSpheres++;
}


void MCCDLLEXPORT CPotColSet::AddBox( const uint8 _iFlags, const int _iObj, const float *_fMinMax)
{
	// *** FULFIX *** M_ASSERT(m_nBoxes < MAX_PCS_BOXES, "CPotColSet::AddBox  MAX_PCS_BOXES too small");
	if (m_nBoxes >= MAX_PCS_BOXES)
	{	
		return;
	}
	
	register float *ptr = m_fBoxes[m_nBoxes];

	*ptr++ = *_fMinMax++;
	*ptr++ = *_fMinMax++;
	*ptr++ = *_fMinMax++;
	*ptr++ = *_fMinMax++;
	*ptr++ = *_fMinMax++;
	*ptr++ = *_fMinMax++;

	m_iObjBoxes[m_nBoxes] = _iObj;
	m_BoxFlags[m_nBoxes] = _iFlags;
	m_nBoxes++;
}


void MCCDLLEXPORT CPotColSet::AddFace( const uint8 _iFlags, const int _iObj, const float *_fPlaneEq, const float *_fFace, const CXW_Surface *_pSurface )
{
	//M_ASSERT(m_nFaces < MAX_PCS_FACES, "CPotColSet::Add  MAX_PCS_FACES too small");
	if (m_nFaces >= MAX_PCS_FACES)
		return;

	register float *ptr;
	
	ptr = m_fPlaneEqs[m_nFaces];
	*ptr++ = *_fPlaneEq++;
	*ptr++ = *_fPlaneEq++;
	*ptr++ = *_fPlaneEq++;
	*ptr++ = *_fPlaneEq++;

	ptr = m_fFaces[m_nFaces];
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;
	*ptr++ = *_fFace++;

	m_iObjFaces[m_nFaces] = _iObj;
	m_FaceFlags[m_nFaces] = _iFlags;
	m_pSurface[m_nFaces] = _pSurface;
	m_nFaces++;
}

void MCCDLLEXPORT CPotColSet::GetBox(float *_BoxMinMax) const
{
	register const float *ptr = m_BoxMinMax;
	
	*_BoxMinMax++ = *ptr++;
	*_BoxMinMax++ = *ptr++;
	*_BoxMinMax++ = *ptr++;
	*_BoxMinMax++ = *ptr++;
	*_BoxMinMax++ = *ptr++;
	*_BoxMinMax++ = *ptr++;
}


void MCCDLLEXPORT CPotColSet::SetBox( const float *_BoxMinMax )
{
	register float *ptr = m_BoxMinMax;
	
	*ptr++ = *_BoxMinMax++;
	*ptr++ = *_BoxMinMax++;
	*ptr++ = *_BoxMinMax++;
	*ptr++ = *_BoxMinMax++;
	*ptr++ = *_BoxMinMax++;
	*ptr++ = *_BoxMinMax++;

	register float h = 0.5f;
	
	m_fCenter[0] = h * (m_BoxMinMax[0] + m_BoxMinMax[3]);
	m_fCenter[1] = h * (m_BoxMinMax[1] + m_BoxMinMax[4]);
	m_fCenter[2] = h * (m_BoxMinMax[2] + m_BoxMinMax[5]);

	register float x,y,z;

	x = m_BoxMinMax[3] - m_fCenter[0];
	y = m_BoxMinMax[4] - m_fCenter[1];
	z = m_BoxMinMax[5] - m_fCenter[2];

	m_fRadius = M_Sqrt( x * x + y * y + z * z );
}

void MCCDLLEXPORT CPotColSet::SetBox( const CBox3Dfp32 *_pBox )
{
	register float *ptr = m_BoxMinMax;
	
	*ptr++ = _pBox->m_Min.k[0];
	*ptr++ = _pBox->m_Min.k[1];
	*ptr++ = _pBox->m_Min.k[2];
	*ptr++ = _pBox->m_Max.k[0];
	*ptr++ = _pBox->m_Max.k[1];
	*ptr++ = _pBox->m_Max.k[2];

	register float h = 0.5f;
	
	m_fCenter[0] = h * (m_BoxMinMax[0] + m_BoxMinMax[3]);
	m_fCenter[1] = h * (m_BoxMinMax[1] + m_BoxMinMax[4]);
	m_fCenter[2] = h * (m_BoxMinMax[2] + m_BoxMinMax[5]);

	register float x,y,z;

	x = m_BoxMinMax[3] - m_fCenter[0];
	y = m_BoxMinMax[4] - m_fCenter[1];
	z = m_BoxMinMax[5] - m_fCenter[2];

	m_fRadius = M_Sqrt( x * x + y * y + z * z );
}

void MCCDLLEXPORT CPotColSet::SetBox( const CVec3Dfp32 *_pMin, const CVec3Dfp32* _pMax )
{
	register float *ptr = m_BoxMinMax;
	
	*ptr++ = _pMin->k[0];
	*ptr++ = _pMin->k[1];
	*ptr++ = _pMin->k[2];
	*ptr++ = _pMax->k[0];
	*ptr++ = _pMax->k[1];
	*ptr++ = _pMax->k[2];

	register float h = 0.5f;
	
	m_fCenter[0] = h * (m_BoxMinMax[0] + m_BoxMinMax[3]);
	m_fCenter[1] = h * (m_BoxMinMax[1] + m_BoxMinMax[4]);
	m_fCenter[2] = h * (m_BoxMinMax[2] + m_BoxMinMax[5]);

	register float x,y,z;

	x = m_BoxMinMax[3] - m_fCenter[0];
	y = m_BoxMinMax[4] - m_fCenter[1];
	z = m_BoxMinMax[5] - m_fCenter[2];

	m_fRadius = M_Sqrt( x * x + y * y + z * z );
}




void MCCDLLEXPORT CPotColSet::PlaneSieve(const float a, const float b, const float c, const float d)
{
return; //AR-TEMP: disabled for now (causes bugs).

	register int i;

	{ // in place copy all faces that passes the sieve
		register int nFaces = 0;

		for (i = 0; i < m_nFaces; i++)
		{
			register const float *s = (float *)m_fFaces[i]; 	// source pointer
		
			if( a * s[0] + b * s[1] + c * s[2] + d > PCS_SIGMA ||
				a * s[3] + b * s[4] + c * s[5] + d > PCS_SIGMA ||
				a * s[6] + b * s[7] + c * s[8] + d > PCS_SIGMA )
			{
				m_fFaces[nFaces][0] = m_fFaces[i][0];		// copy face
				m_fFaces[nFaces][1] = m_fFaces[i][1];
				m_fFaces[nFaces][2] = m_fFaces[i][2];
				m_fFaces[nFaces][3] = m_fFaces[i][3];
				m_fFaces[nFaces][4] = m_fFaces[i][4];
				m_fFaces[nFaces][5] = m_fFaces[i][5];
				m_fFaces[nFaces][6] = m_fFaces[i][6];
				m_fFaces[nFaces][7] = m_fFaces[i][7];
				m_fFaces[nFaces][8] = m_fFaces[i][8];
				m_fPlaneEqs[nFaces][0] = m_fPlaneEqs[i][0];	// copy associated plane equation
				m_fPlaneEqs[nFaces][1] = m_fPlaneEqs[i][1];
				m_fPlaneEqs[nFaces][2] = m_fPlaneEqs[i][2];
				m_fPlaneEqs[nFaces][3] = m_fPlaneEqs[i][3];
				m_FaceFlags[nFaces] = m_FaceFlags[i];		// copy associated flags
				nFaces++;
			}
		}
		
		m_nFaces = nFaces;
	}


	// let all other primitives pass the sieve,
	// probably wont give much boost if any to check them
}



////////////////////////////////////// DEBUG-RENDER ///////////////////////////////////



CWorld_PhysState *CPotColSet::ms_pCurrPhysState;

void MCCDLLEXPORT CPotColSet::DebugRender(CWorld_PhysState* _pPhysState)
{
#ifndef M_RTM
	ms_pCurrPhysState = _pPhysState;

	int i;

	// render faces
	const CVec3Dfp32 offset(0.0f, 0.0f, 0.1f);
	const CPixel32 color(0xff0000ff);
	for (i=0; i<m_nFaces; i++)
		DEBUG_RENDER_FACE((const CVec3Dfp32*)m_fFaces[i], color);

	// render boxes
	const CVec3Dfp32 origo(0.0f);
	for (i=0; i<m_nBoxes; i++)
		DEBUG_RENDER_AABB((const CBox3Dfp32*)m_fBoxes[i], origo, 0xffff0000);

	// render boundbox for this set
	if (m_nFaces || m_nBoxes)
		DEBUG_RENDER_AABB((const CBox3Dfp32*)m_BoxMinMax, origo, 0xff7f7f7f);
#endif
}


static void PCSDebug_RenderAABB(const CBox3Dfp32* _pBox, const CVec3Dfp32& _Pos, CPixel32 _Color)
{
	if (CPotColSet::ms_pCurrPhysState)
	{
		CVec3Dfp32 v[8];
		for (int i=0; i<8; i++)
		{
			v[i].k[0] = _Pos.k[0] +        ((i&1) ? _pBox->m_Max.k[0] : _pBox->m_Min.k[0]);
			v[i].k[1] = _Pos.k[1] +        ((i&2) ? _pBox->m_Max.k[1] : _pBox->m_Min.k[1]);
			v[i].k[2] = _Pos.k[2] + 0.1f + ((i&4) ? _pBox->m_Max.k[2] : _pBox->m_Min.k[2]);
		}
		const fp32 time = 0.06f;
		for (int e=0; e<4; e++)
		{
			CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v[e*2], v[e*2+1], _Color, time, false);
			CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v[(e&2)*2+(e&1)], v[(e&2)*2+(e&1)+2], _Color, time, false);
			CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v[e], v[e+4], _Color, time, false);

		}
	}
}

static void PCSDebug_RenderFace(const CVec3Dfp32* _pVerts, CPixel32 _Color)
{
	if (CPotColSet::ms_pCurrPhysState)
	{
		const CVec3Dfp32 offset(0.0f, 0.0f, 0.1f);
		CVec3Dfp32 v0 = _pVerts[0] + offset;
		CVec3Dfp32 v1 = _pVerts[1] + offset;
		CVec3Dfp32 v2 = _pVerts[2] + offset;
		CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v0, v1, _Color, 0.2f, false);
		CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v1, v2, _Color, 0.2f, false);
		CPotColSet::ms_pCurrPhysState->Debug_RenderWire(v2, v0, _Color, 0.2f, false);
	}
}
