
#include "PCH.h"

#include "WPhysOBB.h"
#include "WPhys.h"
#include "../XRClass.h"
#include "MFloat.h"

/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	Unshared functions							  
|____________________________________________________________________________________|
\************************************************************************************/

void Phys_LineTest(const CVec3Dfp32 &_p1,const CVec3Dfp32 &_p2,const CVec3Dfp32 &_Cen,
				   const CVec3Dfp32 &_Dir,fp32 _Len,CVec3Dfp32 &_r1,CVec3Dfp32 &_r2)
{
	CVec3Dfp32 d1 = _p2 - _p1;
	CVec3Dfp32 r = _p1 - _Cen;
	fp32 a = d1 * d1;
	fp32 f = _Dir * r;
	fp32 c = d1 * r;
	fp32 b = d1 * _Dir;

	fp32 denom = a - b*b;

	fp32 s,t;

	if( denom != 0.0f)
		s = Clamp((b*f - c) / denom,0.0f,1.0f);
	else 
		s = 0.0f;

	t = (b*s + f);

	if( t < -_Len) 
	{
		t = -_Len;
		s = Clamp((-b*_Len-c) / a,0.0f,1.0f);
	} else if (t > _Len) {
		t = _Len;
		s = Clamp((b*_Len - c) / a,0.0f,1.0f);
	}

	_r1 = _p1 + d1 * s;
	_r2 = _Cen + _Dir * t;
}


M_FORCEINLINE void Phys_LineTest(const vec128 &_p1,const vec128 &_p2,const vec128 &_cen,
								 const vec128 &_dir,const vec128 &_len,
								 vec128 &_r1,vec128 &_r2)
{
	vec128 d1 = M_VSub(_p2,_p1);
	vec128 r = M_VSub(_p1,_cen);
	vec128 a = M_VDp3(d1,d1);
	vec128 f = M_VDp3(_dir,r);
	vec128 c = M_VDp3(d1,r);
	vec128 b = M_VDp3(d1,_dir);

	vec128 denom = M_VSub(a,M_VMul(b,b));

	vec128 s,t;
	vec128 v_zero = M_VZero();

	if( M_VCmpAllEq(denom,v_zero) )
		s = v_zero;
	else
		s = M_VClamp01( M_VMul( M_VSub( M_VMul(b,f), c), M_VRcp_Est(denom) ) );

	t = M_VAdd( M_VMul(b,s), f );

	if( M_VCmpAnyLT(t, M_VNeg(_len)) )
	{
		t = M_VNeg(_len);
		s = M_VClamp01( M_VMul( M_VSub( M_VMul(b,t), c), M_VRcp_Est(a) ) );
	}
	else if( M_VCmpAnyGT(t,_len) )
	{
		t = _len;
		s = M_VClamp01( M_VMul( M_VSub( M_VMul(b,t), c), M_VRcp_Est(a) ) );
	}

	_r1 = M_VAdd(_p1,M_VMul(d1,s));
	_r2 = M_VAdd(_cen,M_VMul(_dir,t));
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	Shared functions							  
|____________________________________________________________________________________|
\************************************************************************************/


int Phys_Collide_CapsuleFace(const CPhysCapsule &_Cps,const CVec3Dfp32 *_pV,const uint32 *_piV,uint _nV,const CPlane3Dfp32 &_Plane,
					 CVec3Dfp32 * _pPoints,CVec3Dfp32 * _pNormals,fp32 * _pDepths)
{
	//Point closest to plane
	CVec3Dfp32 PtClose;
	fp32 Rad = _Cps.m_E.k[1];
	fp32 Ln = _Cps.m_E.k[0];
	const CVec3Dfp32 &Dir = _Cps.m_Pos.GetRow(0);
	const CVec3Dfp32 &Cen = _Cps.m_Pos.GetRow(3);

	fp32 Least;

	{
		fp32 Dist = _Plane.Distance(Cen);
		fp32 Proj = Abs(_Plane.n * Dir) * Ln;
		if( Abs(Dist) > Proj + Rad ) return 0;

		fp32 aDist = Abs(Dist);
			
		//Both on one side
		if( aDist > Proj )
		{
			if( (_Plane.n * Dir) * (Dist) < 0 )	//Capsule pointing *towards* plane
				PtClose = Cen + Dir * Ln;
			else								//Capsule pointing *away from* plane
				PtClose = Cen - Dir * Ln;
			PtClose -= _Plane.n * _Plane.Distance(PtClose);
			Least = Proj + Rad - Dist;			//Replace "Dist" with "aDist" to allow separation both ways
		}

		//Intersecting plane
		else
		{
			fp32 t = Dist / (_Plane.n * Dir);
			PtClose = Cen - Dir * t;
			Least = Rad + (Proj - Dist);
		}
	}

	//Determine if we collide at all
	{
		bool bTestState = 0;

		CVec3Dfp32 Prev = _pV[_piV[_nV-1]];
		for(uint i = 0;i < _nV;i++)
		{
			const CVec3Dfp32 &Vtx = _pV[_piV[i]];
			CVec3Dfp32 Delta = Vtx - Prev;
			CVec3Dfp32 SA = -Delta / _Plane.n;
			fp32 Dp = SA * Vtx;

			//If we're on the inside of this plane, don't bother...
			if( Dp > PtClose * SA ) { Prev = Vtx; continue; }

			//Projected outside - need to test edges
			CVec3Dfp32 r1,r2,delta;
			Phys_LineTest(Prev,Vtx,Cen,Dir,Ln,r1,r2);
			delta = r2-r1;

			if( delta.LengthSqr() > Sqr(Rad) )
				bTestState = true;
			else
			{
				bTestState = false;
				break;
			}

			Prev = Vtx;
		}

		//the point was not inside the poly and no edge collision detected
		if( bTestState )
			return 0;
	}

	//Find optimal separating axis
	{
		fp32 Mn = _FP32_MAX;
		CVec3Dfp32 Nrm,TPt;

		/*
		CVec3Dfp32 Cnt = _pV[0];
		for(uint i = 1;i < _nV;i++)
		Cnt += _pV[i];
		Cnt *= (1.0f / _nV);
		*/

		CVec3Dfp32 Prev = _pV[_piV[_nV-1]];
		for(uint i = 0;i < _nV;i++)
		{
			const CVec3Dfp32 &Vtx = _pV[_piV[i]];

			//Try point
			/*
			{
			CVec3Dfp32 Delta = Vtx - Cen;
			fp32 t = Clamp(Delta * Dir,-Ln,Ln);
			CVec3Dfp32 Pt = Cen + Dir * t;
			Delta = Pt - Vtx;
			CVec3Dfp32 Nrml = Vtx - Cnt;
			fp32 Test = (Vtx * Nrml < Pt * Nrml) ? -Delta.LengthSqr() : Delta.LengthSqr();
			if( (Test < Mn) && (Abs(Test) < Sqr(Rad)) )
			{
			Mn = Test;
			TPt = Pt;
			Nrm = (Test < 0) ? Delta : -Delta;
			}
			}*/

			//Try line
			{
				CVec3Dfp32 Delta = Vtx - Prev;
				CVec3Dfp32 SA = -Delta / _Plane.n;
				CVec3Dfp32 Pt,LPt;

				//Hack Ray/Capsule test
				{
					CVec3Dfp32 r = Cen - Vtx;
					fp32 e = Delta.LengthSqr();
					fp32 f = Delta * r;
					fp32 c = Dir * r;
					fp32 b = Dir * Delta;
					fp32 denom = e - b*b;

					fp32 s,t;
					if( denom != 0.0f) s = Clamp((b*f - c*e) / denom,-Ln,Ln);
					else s = 0.0f;

					t = (b*s + f) / e;

					Pt = Cen + Dir * s;
					LPt = Vtx + Delta * t;
					Delta = Pt - LPt;
				}	

				//No valid axis
				if( Delta.LengthSqr() < Ln * 0.001f )
				{
					Prev = Vtx;
					continue;
				}

				fp32 Test = (Vtx * SA < Pt * SA) ? -Delta.LengthSqr() : Delta.LengthSqr();
				if( Test < Mn )
				{
					Mn = Test;
					TPt = Pt;
					Nrm = (Test < 0) ? Delta : -Delta;
				}
			}

			Prev = Vtx;
		}

		//Improvement?
		Mn = (Mn < 0) ? Rad - M_Sqrt(-Mn) : Rad + M_Sqrt(Mn);

		//Edge/Point collision
		if( Mn < Least )
		{
			_pPoints[0] = TPt;
			_pNormals[0] = Nrm.Normalize();
			_pDepths[0] = Mn;
			return 1;
		}

		//Plane-Point collision
		if( !AlmostEqual(Dir*_Plane.n,0.0f,Rad * 0.01f) )
		{
			CVec3Dfp32 Normal = _Plane.n;//(_Plane.Distance(Cen) > 0.0f) ? _Plane.n : -_Plane.n;
			_pPoints[0] = PtClose;//Cen + Dir * Ln * -Sign(Normal * Dir) - Normal * Rad;
			_pDepths[0] = Least;
			_pNormals[0] = Normal;
			return 1;
		}

		//Plane-Edge collision
		{
			CVec3Dfp32 a,b;
			_Cps.GetPoints(a,b);

			CVec3Dfp32 Prev = _pV[_piV[_nV-1]];
			for(uint i = 0;i < _nV;i++)
			{
				const CVec3Dfp32 &Vtx = _pV[_piV[i]];
				CVec3Dfp32 Delta = Vtx - Prev;
				CPlane3Dfp32 Pln;
				Pln.n = -Delta / _Plane.n;
				Pln.d = - Vtx * Pln.n;

				fp32 da = Pln.Distance(a);
				fp32 db = Pln.Distance(b);

				if( da > 0 )
				{
					CVec3Dfp32 c;
					Pln.GetIntersectionPoint(a,b,c);
					a = c;
				}
				else if( db > 0 )
				{
					CVec3Dfp32 c;
					Pln.GetIntersectionPoint(a,b,c);
					b = c;
				}

				Prev = Vtx;
			}	

			fp32 da = _Plane.Distance(a);
			CVec3Dfp32 Norm = _Plane.n;// * Sign(da);
			Least = Rad - da;
			_pPoints[0] = a - _Plane.n * da;
			_pNormals[0] = Norm;
			_pDepths[0] = Least;
			_pPoints[1] = b - _Plane.n * da;
			_pNormals[1] = Norm;
			_pDepths[1] = Least;
			return 2;
		}
	}
}


//vec128 - version of above
int Phys_Collide_CapsuleFace(const vec128 &_Cen,const vec128 &_Dir,const vec128 &_Dim,const vec128 &_Plane,const vec128 * M_RESTRICT _lVtx,uint32 _nVtx,
							 vec128 * _pPoints,vec128 & _Normal,vec128 & _Depth)
{
	//Closest point on plane
	vec128 PtClose;

	vec128 Rad = M_VSplatY(_Dim);
	vec128 Ln = M_VSplatX(_Dim);

	vec128 Least;
	vec128 Cen = _Cen;
	vec128 Dir = _Dir;
	vec128 Plane = _Plane;

	//Plane distance
	{
		vec128 Dist = M_VAdd( M_VSplatW(Plane), M_VDp3(Cen,Plane) );
		vec128 Proj = M_VMul( M_VAbs(M_VDp3(Dir,Plane)), Ln );

		if( M_VCmpAnyGT(M_VAbs(Dist),M_VAdd(Proj,Rad)) ) return 0;

		//Both on one side
		if( M_VCmpAnyGT(M_VAbs(Dist),Proj) )
		{
			vec128 v_zero = M_VZero();
			if( M_VCmpAnyLT( M_VMul( M_VDp3(Plane,Dir),Dist ), v_zero ) )
				PtClose = M_VAdd(Cen, M_VMul(Dir,Ln));	//Pointing towards plane
			else
				PtClose = M_VSub(Cen, M_VMul(Dir,Ln));	//Pointing away from plane

			vec128 PtDist = M_VAdd( M_VSplatW(Plane), M_VDp3(PtClose,Plane) );
			PtClose = M_VSub( PtClose, M_VMul(Plane,PtDist) );
			Least = M_VSub( M_VAdd(Proj,Rad), Dist );
		}

		//Intersecting plane
		else
		{
			vec128 t = M_VMul( Dist, M_VRcp_Est(M_VDp3(Plane,Dir)) );
			PtClose = M_VSub(Cen,M_VMul(Dir,t));
			Least = M_VAdd(Rad,M_VSub(Proj,Dist));
		}
	}

	//Secure collision
	{
		bool bTestState = false;

		vec128 Prev = _lVtx[_nVtx - 1];
		for(uint i = 0;i < _nVtx;i++)
		{
			vec128 Vtx = _lVtx[i];
			vec128 Delta = M_VSub(Prev,Vtx);
			vec128 SA = M_VXpd(Delta,Plane);
			vec128 Dp = M_VDp3(SA,Vtx);

			//Don't check if we're on the inside
			if( M_VCmpAnyGT(Dp,M_VDp3(PtClose,SA)) )
			{
				Prev = Vtx;
				continue;
			}

			//Projection is outside face - test edges
			vec128 r1,r2;
			Phys_LineTest(Prev,Vtx,Cen,Dir,Ln,r1,r2);
			vec128 delta = M_VSub(r2,r1);

			if( M_VCmpAnyGT( M_VLen3(delta), Rad ) )
				bTestState = true;
			else
			{
				bTestState = false;
				break;	//Edge intersection found!
			}

			Prev = Vtx;
		}

		//No edge collisions
		if( bTestState ) return 0;
	}

	//Find optimal separating axis
	{
		vec128 Mn = M_VScalar(3.0e16f);

		//These are only initialized to satisfy GCC
		vec128 Nrm = M_VZero(),TPt = M_VZero();

		vec128 Prev = _lVtx[_nVtx-1];
		for(uint i = 0;i < _nVtx;i++)
		{
			vec128 Vtx = _lVtx[i];

			vec128 Delta = M_VSub(Prev,Vtx);
			vec128 SA = M_VXpd(Delta,Plane);

			vec128 Pt,LPt;

			//Ray/Capsule test
			{
				vec128 r = M_VSub(Vtx,Cen);
				vec128 e = M_VDp3(Delta,Delta);
				vec128 f = M_VDp3(Delta,r);
				vec128 c = M_VDp3(Dir,r);
				vec128 b = M_VDp3(Dir,Delta);
				vec128 denom = M_VSub(e,M_VMul(b,b));

				vec128 s,t;
				vec128 v_zero = M_VZero();
				if( M_VCmpAllEq(denom,v_zero) ) s = v_zero;
				else s = M_VClamp(M_VMul(M_VSub(M_VMul(b,f),M_VMul(c,e)),M_VRcp_Est(denom)),
					M_VNeg(Ln),Ln);

				t = M_VMul( M_VAdd( M_VMul(b,s), f ), M_VRcp_Est(e) );

				Pt = M_VSub(Cen,M_VMul(Dir,s));
				LPt = M_VSub(Vtx,M_VMul(Delta,t));
				Delta = M_VSub(Pt,LPt);
			}

			//No Valid Axis
			vec128 AlmostZero = M_VScalar(0.001f);
			if( M_VCmpAnyLT(M_VDp3(Delta,Delta),M_VMul(Ln,AlmostZero)) )
			{
				Prev = Vtx;
				continue;
			}

			vec128 Test;
			if( M_VCmpAnyLT(M_VDp3(Vtx,SA),M_VDp3(Pt,SA)) ) Test = M_VNeg(M_VDp3(Delta,Delta));
			else Test = M_VDp3(Delta,Delta);
			if( M_VCmpAnyLT( Test, Mn ) )
			{
				Mn = Test;
				TPt = Pt;
				vec128 v_zero = M_VZero();
				if( M_VCmpAnyLT(Test,v_zero) ) Nrm = Delta;
				else Nrm = M_VNeg(Delta);
			}

			Prev = Vtx;
		}

		//Finalize
		{
			Nrm = M_VNrm3(Nrm);

			vec128 v_zero = M_VZero();
			if( M_VCmpAnyLT(Mn,v_zero) ) Mn = M_VSub(Rad,M_VSqrt_Est(M_VNeg(Mn)));
			else Mn = M_VAdd(Rad,M_VSqrt_Est(Mn));
		}

		//Edge/Point
		if( M_VCmpAnyLT(Mn,Least) )
		{
			_pPoints[0] = TPt;
			_Normal = Nrm;
			_Depth = Mn;
			return 1;
		}

		//Plane/Point
		{
			//vec128 v_epsilon = M_VMul(Rad,M_VScalar(0.01f));

			//if( M_VCmpAnyGT( M_VAbs(M_VDp3(Dir,Plane)), v_epsilon ) )
			{
				_pPoints[0] = PtClose;
				_Normal = Plane;
				_Depth = Least;
				return 1;
			}
		}

		//Plane/Edge
		{
			vec128 a,b;
			a = M_VAdd( Cen, M_VMul(Dir,Ln) );
			b = M_VSub( Cen, M_VMul(Dir,Ln) );
			vec128 dv = M_VSub(b,a);

			vec128 Prev = _lVtx[_nVtx-1];
			vec128 Mask = M_VConst_u32(0,0,0,0xFFFFFFFF);
			vec128 v_zero = M_VZero();
			for(uint i = 0;i < _nVtx;i++)
			{
				vec128 Vtx = _lVtx[i];
				vec128 Delta = M_VSub(Prev,Vtx);
				vec128 Pln = M_VXpd(Delta,Plane);
				Pln = M_VSelMsk(Mask,M_VDp3(M_VNeg(Vtx),Pln),Pln);

				vec128 da = M_VAdd( M_VSplatW(Pln), M_VDp3(a,Pln) );
				vec128 db = M_VAdd( M_VSplatW(Pln), M_VDp3(b,Pln) );

				if( M_VCmpAnyGT(da,v_zero) )
				{
					vec128 s = M_VDp3(dv,Pln);
					vec128 t = M_VNeg(M_VMul(da,M_VRcp_Est(s)));
					a = M_VAdd(a,M_VMul(dv,t));
				}
				else if( M_VCmpAnyGT(db,v_zero) )
				{
					vec128 s = M_VDp3(dv,Pln);
					vec128 t = M_VNeg(M_VMul(da,M_VRcp_Est(s)));
					b = M_VAdd(a,M_VMul(dv,t));
				}

				Prev = Vtx;
			}

			vec128 da = M_VAdd( M_VSplatW(Plane), M_VDp3(a,Plane) );
			Least = M_VSub(Rad,da);
		
			a = M_VSub(a,M_VMul(Plane,da));
			b = M_VSub(b,M_VMul(Plane,da));

			_Normal = Plane;
			_Depth = Least;
			_pPoints[0] = a;
			_pPoints[1] = b;

			return 2;
		}
	}
}

