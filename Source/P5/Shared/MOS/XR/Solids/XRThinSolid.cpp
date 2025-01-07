#include "PCH.h"
#include "XRThinSolid.h"
#ifndef PLATFORM_CONSOLE
#include "XWSolid.h"
#endif

/*
	IMPLEMENT_OPERATOR_NEW(CXR_ThinSolid);
*/

CXR_ThinSolid::CXR_ThinSolid()
{
	MAUTOSTRIP(CXR_ThinSolid_ctor, MAUTOSTRIP_VOID);
	m_iNode = 0;
	m_iSurface = 0;
}

void CXR_ThinSolid::operator=(const CXR_ThinSolid& _Solid)
{
	MAUTOSTRIP(CXR_ThinSolid_operator_assign, MAUTOSTRIP_VOID);
	m_lPlanes.SetLen(_Solid.m_lPlanes.Len());
	memcpy(m_lPlanes.GetBasePtr(), _Solid.m_lPlanes.GetBasePtr(), m_lPlanes.ListSize());
	m_iNode = _Solid.m_iNode;
	m_iSurface = _Solid.m_iSurface;
}

void CXR_ThinSolid::ReadGeometry(CCFile* _pF)
{
	MAUTOSTRIP(CXR_ThinSolid_ReadGeometry, MAUTOSTRIP_VOID);
	int16 Ver = 0;
	_pF->ReadLE(Ver);

	switch(Ver)
	{
	case 0x0100 :
		{
			int16 nP;
			_pF->ReadLE(nP);
			_pF->ReadLE(m_iNode);
			_pF->ReadLE(m_iSurface);

			m_lPlanes.SetLen(nP);
			for(int i = 0; i < nP; i++)
			{
				m_lPlanes[i].n.Read(_pF);
				_pF->ReadLE(m_lPlanes[i].d);
			}
		}
		break;

/*	case 0x0100 :
		{
			int16 nP;
			_pF->ReadLE(nP);
			int32 iNode;
			_pF->ReadLE(iNode);
			m_iNode = iNode;

			m_lPlanes.SetLen(nP);
			for(int i = 0; i < nP; i++)
			{
				m_lPlanes[i].n.Read(_pF);
				_pF->ReadLE(m_lPlanes[i].d);
			}
		}
		break;
*/
	default :
		Error_static("CXR_ThinSolid::ReadGeometry", CStrF("Unsupported version (%.4x > %.4x)", Ver, XWSOLID_GEOMETRY_VERSION));
	}

}

void CXR_ThinSolid::WriteGeometry(CCFile* _pF)
{
	MAUTOSTRIP(CXR_ThinSolid_WriteGeometry, MAUTOSTRIP_VOID);
	int16 Ver = XWSOLID_GEOMETRY_VERSION;
	_pF->WriteLE(Ver);
	int16 nP = m_lPlanes.Len();
	_pF->WriteLE(nP);
	_pF->WriteLE(m_iNode);
	_pF->WriteLE(m_iSurface);
	for(int i = 0; i < nP; i++)
	{
		m_lPlanes[i].n.Write(_pF);
		_pF->WriteLE(m_lPlanes[i].d);
	}
}

#ifndef PLATFORM_CONSOLE
void CXR_ThinSolid::Create(CSolid& _Solid)
{
	MAUTOSTRIP(CXR_ThinSolid_Create, MAUTOSTRIP_VOID);
	m_lPlanes.SetLen(_Solid.m_lspPlanes.Len());
	m_iNode = _Solid.m_iNode;
	m_iSurface = 0;

	for(int i = 0; i < m_lPlanes.Len(); i++)
	{
		m_lPlanes[i].n = _Solid.m_lspPlanes[i]->m_Plane.n.Getfp32();
		m_lPlanes[i].d = _Solid.m_lspPlanes[i]->m_Plane.d;
	}
}
#endif

static M_INLINE vec128 M_VPlaneCutLine(vec128 _Plane, vec128 _p0, vec128 _p1)
{
	vec128 vd = M_VSub(_p1, _p0);

	vec128 s = M_VDp3(_Plane, vd);
	vec128 sp = M_VDp4(_Plane, _p0);
	// This could cause a division by 0 but the selection below handles that case
	vec128 t = M_VNeg(M_VMul(sp, M_VRcp(s)));

	return M_VSelMsk(M_VCmpEqMsk(s, M_VZero()), _p0, M_VMAdd(vd, t, _p0));
}

fp32 CXR_ThinSolid::IntersectRay(const CVec3Dfp32& _Origin, const CVec3Dfp32& _Ray)
{
	vec128 zeroone = M_VConst(0.0f, 0.0f, 0.0f, 1.0f);
	vec128 oneu32 = M_VOne_u32();
	vec128 selmask = M_VPerm(M_VNegOne_i32(), M_VZero(), 0, 0, 0, 4);
	vec128 OutsideEpsilon = M_VConst(CSOLID_EPSILON, CSOLID_EPSILON, 0, 0);
	vec128 OutInCut = M_VConst(CSOLID_EPSILON, -CSOLID_EPSILON, 0, 0);
	vec128 InOutCut = M_VConst(-CSOLID_EPSILON, CSOLID_EPSILON, 0, 0);
	vec128 invalid = M_VZero();

	vec128 casesel = M_VPerm(M_VNegOne_i32(), M_VZero(), 0, 4, 4, 4);

	const CPlane3Dfp32* M_RESTRICT plPlanes = m_lPlanes.GetBasePtr();
	int nPlanes = m_lPlanes.Len();

	// Make sure we have 2 valid points (set w to 1.0f)
	vec128 origin = M_VLdU(&_Origin);
	vec128 v0 = M_VSelMsk(selmask, origin, zeroone);
	vec128 v1 = M_VSelMsk(selmask, M_VMAdd(M_VNrm3(M_VLdU(&_Ray)), M_VScalar(10000000.0f), v0), zeroone);

	for(int p = 0; p < nPlanes; p++)
	{
		const CPlane3Dfp32* M_RESTRICT pPi = &plPlanes[p];
		vec128 Plane = M_VLdU(pPi);

		vec128 Dist01 = M_VDp4x2(Plane, v0, Plane, v1);

		// If both distances are positive then we're outside the solid, abort or keep running?
		vec128 invalidtemp = M_VCmpGTMsk(Dist01, OutsideEpsilon);
		invalid = M_VSelMsk(invalid, invalid, M_VAnd(M_VSplatX(invalidtemp), M_VSplatY(invalidtemp)));

		vec128 case1temp = M_VSelMsk(casesel, M_VCmpGTMsk(Dist01, OutInCut), M_VCmpLTMsk(Dist01, OutInCut));
		vec128 case1mask = M_VAnd(M_VSplatX(case1temp), M_VSplatY(case1temp));

		vec128 case2temp = M_VSelMsk(casesel, M_VCmpLTMsk(Dist01, InOutCut), M_VCmpGTMsk(Dist01, InOutCut));
		vec128 case2mask = M_VAnd(M_VSplatX(case2temp), M_VSplatY(case2temp));

		vec128 CutPoint = M_VPlaneCutLine(Plane, v0, v1);

		// Place Cutpoint in either v0 or v1 depending of where the line cut (or it could be entirely outside and invalid)
		v0 = M_VSelMsk(case1mask, CutPoint, v0);
		v1 = M_VSelMsk(case2mask, CutPoint, v1);
	}

	// Sadly this part causes a LHS :(
	vec128 Result = M_VSelMsk(invalid, M_VScalar(-1.0f), M_VLen3(M_VSub(origin, v0)));
	CVec4Dfp32 Data;
	M_VSt(Result, &Data);
	return Data.k[0];
}
