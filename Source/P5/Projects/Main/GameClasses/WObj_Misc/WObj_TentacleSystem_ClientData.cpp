/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_TentacleSystem.

	Author:			Anton Ragnarsson, Olle Rosenquist, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CSpline_Vec3Dfp32, CTentacleArmSetup, CTentacleArmState,
					CTentacleSystemAnimGraph, CWO_TentacleSystem_ClientData

	Comments:

	History:		
		050517:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_TentacleSystem_ClientData.h"
#include "WObj_TentacleSystem.h"
#include "../CConstraintSystem.h"
#include "WObj_CreepingDark.h"
#include "../WRPG/WRPGChar.h"

#include "../Models/WModel_EffectSystem.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WDynamicsEngine/WDynamicsEngine2.h"

enum
{
	class_CharPlayer			= MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC				= MHASH5('CWOb','ject','_','Char','NPC'),
};

#define sizeof_buffer(buf) (sizeof(buf)/sizeof(buf[0]))
#define IsValidFloat(v) (v > -_FP32_MAX && v < _FP32_MAX)
#define IsValidVec(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))
#define IsValidMat(m) (IsValidVec(m.GetRow(0)) && IsValidVec(m.GetRow(1)) && IsValidVec(m.GetRow(2)))

#ifndef DEBUG_COLOR_RED
# define DEBUG_COLOR_RED   0xff0000ff
# define DEBUG_COLOR_GREEN 0xff00ff00
# define DEBUG_COLOR_BLUE  0xffff0000
#endif

#ifndef M_RTM
// Debug token validation
# define DEBUG_VALIDATETOKEN(_iToken, _pString)		            Debug_ValidateToken(_iToken, _pString)
# define DEBUG_RENDERSPHERE(pWPhysState, Origin, Radius, Color) (pWPhysState)->Debug_RenderSphere(Origin, Radius, Color)
# define DEBUG_RENDERWIRE(pWPhysState, p0, p1, Color)           (pWPhysState)->Debug_RenderWire(p0, p1, Color)
#else
# define DEBUG_VALIDATETOKEN(_iToken, _pString)
# define DEBUG_RENDERSPHERE(pWPhysState, Origin, Radius, Color)
# define DEBUG_RENDERWIRE(pWPhysState, p0, p1, Color)
#endif

#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * pWServer->GetGameTickTime())
#define DEVOUR_BLEND_TIME_DEBUG 0


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpline_Vec3Dfp32
|__________________________________________________________________________________________________
\*************************************************************************************************/
CSpline_Vec3Dfp32::CSpline_Vec3Dfp32() 
{
}


void CSpline_Vec3Dfp32::AddPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent)
{
	m_EndMat = _Pos;

	uint iPoint = m_lPoints.Len();
	m_lPoints.SetLen(iPoint + 1);
	m_lPoints[iPoint].m_Pos = _Pos.GetRow(3);
	m_lPoints[iPoint].m_Tangent = _Tangent;
}


void CSpline_Vec3Dfp32::RemoveLastPoint()
{
	m_lPoints.SetLen(m_lPoints.Len() - 1);
}


void CSpline_Vec3Dfp32::ModifyLastPoint(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Tangent)
{
	m_EndMat.GetRow(3) = _Pos;

	uint iPoint = m_lPoints.Len() - 1;
	m_lPoints[iPoint].m_Pos = _Pos;
	m_lPoints[iPoint].m_Tangent = _Tangent;
}


// returns whether or not the spline has any real segments
bool CSpline_Vec3Dfp32::IsEmpty() const
{
	return (m_lPoints.Len() <= 1);
}


// calculates in- & out-tangents and cache points
void CSpline_Vec3Dfp32::Finalize(uint8 _nCachePoints)
{
	M_ASSERT(_nCachePoints <= Segment::MaxCachePoints, "");
	uint nPoints = m_lPoints.Len();
	uint nSegments = nPoints - 1;
	m_lSegments.SetLen(nSegments);
	m_nCachePoints = _nCachePoints;
	m_Length = 0.0f;

	if (IsEmpty())
	{
		m_MaxT = 0.0f;
		return;
	}

	// Calculate length of each segment (and total length)
	fp32 tStep = 1.0f / _nCachePoints;
	for (uint iSeg = 0; iSeg < nSegments; iSeg++)
	{
		Segment& s = m_lSegments[iSeg];
		CVec3Dfp32 Pos1 = m_lPoints[iSeg].m_Pos;		// pos at start of segment
		CVec3Dfp32 Pos2 = m_lPoints[iSeg+1].m_Pos;		// pos at end of segment
		s.m_TanScale = Pos1.Distance(Pos2);

		fp32 Len = 0.0f, t = fp32(iSeg);
		for (uint i = 0; i < _nCachePoints; i++)
		{
			Segment::CachedPos& c = s.m_Cache[i];
			c.t = t;
			c.sum = Len;
			t += tStep;
			CalcPos(t - 0.0001f, Pos2);
			c.len = Pos1.Distance(Pos2);
			Len += c.len;
			Pos1 = Pos2;
		}
		s.m_SegLen = Len;
		s.m_InvSegLen = 1.0f / Len;
		m_Length += Len;
	}
	m_MaxT = (fp32)nSegments;
}


void CSpline_Vec3Dfp32::CalcPos(fp32 _Time, CVec3Dfp32& _Result) const
{
//	_Time = Clamp(_Time, 0.0f, m_MaxT);
	uint iSeg = Min(m_lSegments.Len()-1, (int)_Time);

	const Point& p0 = m_lPoints[iSeg];
	const Point& p1 = m_lPoints[iSeg+1];
	fp32 TanScale = m_lSegments[iSeg].m_TanScale;

	fp32 t  = _Time - (fp32)iSeg;
	fp32 t2 = t * t;
	fp32 t3 = t * t * t;

	fp32 h0 = ( 2.0f*t3 + -3.0f*t2 + 1.0f);
	fp32 h1 = (-2.0f*t3 +  3.0f*t2       );
	fp32 h2 = (      t3 + -2.0f*t2 + t   ) * TanScale;
	fp32 h3 = (      t3 -       t2       ) * TanScale;

	_Result.k[0] = h0 * p0.m_Pos.k[0] + h1 * p1.m_Pos.k[0] + h2 * p0.m_Tangent.k[0] + h3 * p1.m_Tangent.k[0];
	_Result.k[1] = h0 * p0.m_Pos.k[1] + h1 * p1.m_Pos.k[1] + h2 * p0.m_Tangent.k[1] + h3 * p1.m_Tangent.k[1];
	_Result.k[2] = h0 * p0.m_Pos.k[2] + h1 * p1.m_Pos.k[2] + h2 * p0.m_Tangent.k[2] + h3 * p1.m_Tangent.k[2];
}


void CSpline_Vec3Dfp32::CalcRot(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const
{
	if (IsEmpty())
	{
		_Result = m_EndMat;
		return;
	}

	//NOTE: This is just a helper method - _Result.GetRow(3) must be valid!
	CalcPos(_Time + 0.01f, _Result.GetRow(0));
	_Result.GetRow(0) -= _Result.GetRow(3);
	_Result.GetRow(2) = _RefUp;
	_Result.RecreateMatrix(0, 2);
}


void CSpline_Vec3Dfp32::CalcMat(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const
{
	_Result.UnitNot3x3();
	CalcPos(_Time, _Result.GetRow(3));
	CalcRot(_Time, _Result, _RefUp); 
}


void CSpline_Vec3Dfp32::FindPos(fp32 _Distance, SplinePos& _Result) const
{
	uint nSegments = m_lSegments.Len();
	const Segment* pSegs = m_lSegments.GetBasePtr();
	// First, step entire segments
	for (uint iSeg = 0; iSeg < nSegments; iSeg++)
	{
		const Segment& s = pSegs[iSeg];
		if (_Distance < s.m_SegLen)
		{
			// Then, step through cached points
			const fp32 tCacheStep = 1.0f / m_nCachePoints;
			for (uint i = 0; i < m_nCachePoints; i++)
			{
				fp32 cLen = s.m_Cache[i].len + 0.001f;
				if (_Distance <= cLen)
				{
					// Then linear interpolate to find t (not exact, but perhaps good enough)
					_Result.t = s.m_Cache[i].t + (_Distance / cLen) * tCacheStep;
					CalcPos(_Result.t, _Result.mat.GetRow(3));
					return;
				}
				_Distance -= s.m_Cache[i].len;
			}
			M_ASSERT(false, "error error error");
		}
		_Distance -= s.m_SegLen;
	}
	// Not found.. set to last point then...
	_Result.t = m_MaxT;
	_Result.mat.GetRow(3) = m_lPoints[nSegments].m_Pos;
}


void CSpline_Vec3Dfp32::FindPos(fp32 _Distance, SplinePos& _Result, const CVec3Dfp32& _RefUp) const
{
	_Result.mat.UnitNot3x3();
	FindPos(_Distance, _Result);
	CalcRot(_Result.t, _Result.mat, _RefUp);
}

fp32 CSpline_Vec3Dfp32::GetDistance(fp32 _Time) const
{
	TAP<const Segment> pSegs = m_lSegments;
	uint iSeg = Min(pSegs.Len()-1, (int)_Time);

	fp32 d = 0.0f;
	for (uint i = 0; i < iSeg; i++)
		d += pSegs[i].m_SegLen;

	fp32 nCP = fp32(m_nCachePoints);
	uint iCP = (uint)( M_FMod(_Time, 1.0f) * nCP );
	const Segment::CachedPos& cp = pSegs[iSeg].m_Cache[iCP];
	d += cp.sum + cp.len * ((_Time - cp.t) * nCP);
	return d;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArmAttachModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
CTentacleArmAttachModel::CTentacleArmAttachModel()
	: parent()
//	: m_iModel(0)
//	, m_iNode(0)
	, m_Offset(0)
{
}


void CTentacleArmAttachModel::Pack(uint8*& _pD) const
{
	parent::Pack(_pD);
	//TAutoVar_Pack(m_iNode, _pD);
	//TAutoVar_Pack(m_iModel, _pD);
	TAutoVar_Pack(m_Offset, _pD);
}


void CTentacleArmAttachModel::Unpack(const uint8*& _pD)
{
	//TAutoVar_Unpack(m_iNode, _pD);
	//TAutoVar_Unpack(m_iModel, _pD);
	parent::Unpack(_pD);
	TAutoVar_Unpack(m_Offset, _pD);
}
*/


/*
CTentacleArmBreathModel::CTentacleArmBreathModel()
	: m_iNode(0)
{
	for (uint i = 0; i < 3; i++)
	{
		m_liModel[i] = -1;
		m_lspModelInstances[i] = NULL;
	}
}


void CTentacleArmBreathModel::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_iNode, _pD);
	for (uint i = 0; i < 3; i++)
		TAutoVar_Pack(m_liModel[i], _pD);
}


void CTentacleArmBreathModel::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_iNode, _pD);
	for (uint i = 0; i < 3; i++)
		TAutoVar_Unpack(m_liModel[i], _pD);
}


CTentacleArmDrainModel::CTentacleArmDrainModel()
	: m_iAttach(0)
	, m_iModel(0)
	, m_spModelInstances(NULL)
{
}


void CTentacleArmDrainModel::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_iAttach, _pD);
	TAutoVar_Pack(m_iModel, _pD);
}


void CTentacleArmDrainModel::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_iAttach, _pD);
	TAutoVar_Unpack(m_iModel, _pD);
}
*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArmSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTentacleArmSetup::CTentacleArmSetup()
	: m_nAttachPoints(0)
	//, m_nTargetAttachPoints(0)
	, m_iAGToken(0)
	, m_Extrude(0.0f)
	, m_Spin(0.0f)
	, m_Scale(1.0f)
	, m_bMirrorAnim(false)
	, m_bPhysLinks(false)
{
	for (uint i = 0; i < sizeof_buffer(m_liTemplateModels); i++)
		m_liTemplateModels[i] = 0;

	m_liSplineBones.Clear();
}


void CTentacleArmSetup::Setup(const CRegistry& _Reg, CWorld_Server& _WServer)
{
	CMapData& MapData = *_WServer.GetMapData();

	m_Name = _Reg.GetThisValue();
	for (uint i = 0; i < _Reg.GetNumChildren(); i++)
	{
		CStr KeyName = _Reg.GetName(i);
		CStr KeyValue = _Reg.GetValue(i);
		uint32 HashName = StringToHash(KeyName.GetStr());
		
		if (KeyName.CompareSubStr("POWER_MODEL") == 0)
		{
			MapData.GetResourceIndex_Model(KeyValue.Str());
		}

		else if (KeyName.CompareSubStr("MODEL") == 0)
		{
			uint8 iModel = KeyName.RightFrom(5).Val_int();
			if (iModel >= sizeof_buffer(m_liTemplateModels))
			{
				ConOutL(CStrF("CTentacleArmSetup::Setup, model index out of range (%d >= %d)", iModel, sizeof_buffer(m_liTemplateModels)));
			}
			else
			{
				m_liTemplateModels[iModel] = MapData.GetResourceIndex_Model(KeyValue.Str());
			}
		}

		else if (KeyName.CompareSubStr("ATTACHMODEL") == 0)
		{
			uint8 iAttach = (uint8)KeyName.RightFrom(11).Val_int();
			if (iAttach > 1)
			{
				ConOutL(CStrF("CTentacleArmSetup::Setup, AttachModel index out of range (%d >= %d)", iAttach, 2));
			}
			else
			{
				m_lAttachModels[iAttach].GetAttachNode() = (uint8)KeyValue.GetStrSep(",").Val_int();
				//m_lAttachModels[iAttach].m_Offset.ParseString(KeyValue.GetStrSep(","));
				m_lAttachModels[iAttach].GetModel(0) = _WServer.GetMapData()->GetResourceIndex_Model(KeyValue);
			}
		}

		else if (KeyName.CompareSubStr("BREATHMODEL") == 0)
		{
			uint8 iAttach = (uint8)KeyName.RightFrom(11).Val_int();
			if (iAttach > 2)
				ConOutL(CStrF("CTentacleArmSetup::Setup, BreathModel index out of range (%d >= %d)", iAttach, 3));
			else
				m_BreathModels.GetModel(iAttach) = _WServer.GetMapData()->GetResourceIndex_Model(KeyValue);
		}

		else if (KeyName.CompareSubStr("BREATHNODE") == 0)
		{
			m_BreathModels.GetAttachNode() = (uint8)KeyValue.Val_int();
		}

		else if (KeyName.CompareSubStr("ATTACH") == 0)
		{
			uint8 iAttach = KeyName.RightFrom(6).Val_int();
			if (iAttach >= sizeof_buffer(m_lAttachPoints))
			{
				ConOutL(CStrF("CTentacleArmSetup::Setup, "
					"Attach index out of range (%d >= %d)", iAttach, sizeof_buffer(m_lAttachPoints)));
			}
			else
			{
				m_lAttachPoints[iAttach].Parse(KeyValue);
				m_nAttachPoints = MaxMT(m_nAttachPoints, iAttach+1);
			}
		}

		/*
		else if (KeyName.CompareSubStr("TARGET") == 0)
		{
			uint8 iTarget = KeyName.RightFrom(6).Val_int();
			if (iTarget >= sizeof_buffer(m_lTargetAttachPoints))
			{
				ConOutL(CStrF("CTentacleArmSetup::Setup, " 
					"Attach index out of range (%d >= %d)", iTarget, sizeof_buffer(m_lTargetAttachPoints)));
			}
			else
			{
				m_lTargetAttachPoints[iTarget].ParseString(KeyValue);
				m_nTargetAttachPoints = MaxMT(m_nTargetAttachPoints, iTarget+1);
			}
		}
		*/

		switch (HashName)
		{
		case MHASH3('DRAI','NMOD','EL'): // "DRAINMODEL"
			{
				m_DrainModel.GetAttachNode() = KeyValue.GetStrSep(",").Val_int();
				m_DrainModel.GetModel(0) = _WServer.GetMapData()->GetResourceIndex_Model(KeyValue);
				break;
			}

		case MHASH2('EXTR','UDE'): // "EXTRUDE"
			{
				m_Extrude = (fp32)KeyValue.Val_fp64();
				break;
			}

		case MHASH1('SPIN'): // "SPIN"
			{
				m_Spin = (fp32)KeyValue.Val_fp64();
				break;
			}

		case MHASH2('SCAL','E'): // "SCALE"
			{
				m_Scale = (fp32)KeyValue.Val_fp64();
				break;
			}

		case MHASH2('MIRR','OR'): // "MIRROR"
			{
				m_bMirrorAnim = (KeyValue.Val_int() != 0);
				break;
			}

		case MHASH2('AGTO','KEN'): // "AGTOKEN"
			{
				m_iAGToken = KeyValue.Val_int();
				break;
			}

		case MHASH3('PHYS','LINK','S'): // "PHYSLINKS"
			{
				//m_bPhysLinks = (KeyValue.Val_int() != 0);
				break;
			}

		case MHASH3('SPLI','NEBO','NES'): // "SPLINEBONES"
			{
				int iEntry = 0;
				int nEntries = KeyValue.GetNumMatches(",") + 1;
				m_liSplineBones.SetLen(nEntries);
				TAP_RCD<uint16> liSplineBones = m_liSplineBones;
				while (KeyValue.Len())
					liSplineBones[iEntry++] = uint16(KeyValue.GetStrSep(",").Val_int());

				break;
			}
		}
	}
}


void CTentacleArmSetup::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Name, _pD);
	for (uint i = 0; i < sizeof_buffer(m_liTemplateModels); i++)
		TAutoVar_Pack(m_liTemplateModels[i], _pD);

	TAutoVar_Pack(m_nAttachPoints, _pD);
	for (uint i = 0; i < m_nAttachPoints; i++)
	{
		TAutoVar_Pack(m_lAttachPoints[i].m_iNode, _pD);
		TAutoVar_Pack(m_lAttachPoints[i].m_LocalPos, _pD);
	}

//	TAutoVar_Pack(m_nTargetAttachPoints, _pD);
//	for (uint i = 0; i < m_nTargetAttachPoints; i++)
//		TAutoVar_Pack(m_lTargetAttachPoints[i], _pD);

	for (uint i = 0; i < 2; i++)
		TAutoVar_Pack(m_lAttachModels[i], _pD);

	uint16 nSplineBones = m_liSplineBones.Len();
	TAutoVar_Pack(nSplineBones, _pD);
	for (uint16 i = 0; i < nSplineBones; i++)
		TAutoVar_Pack(m_liSplineBones[i], _pD);

	TAutoVar_Pack(m_BreathModels, _pD);
	TAutoVar_Pack(m_DrainModel, _pD);

	TAutoVar_Pack(m_iAGToken, _pD);
	TAutoVar_Pack(m_Extrude, _pD);
	TAutoVar_Pack(m_Spin, _pD);
	TAutoVar_Pack(m_Scale, _pD);
	TAutoVar_Pack((int8)m_bMirrorAnim, _pD);
}


void CTentacleArmSetup::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Name, _pD);
	for (uint i = 0; i < sizeof_buffer(m_liTemplateModels); i++)
		TAutoVar_Unpack(m_liTemplateModels[i], _pD);

	TAutoVar_Unpack(m_nAttachPoints, _pD);
	if (m_nAttachPoints > sizeof_buffer(m_lAttachPoints))
		Error_static("CTentacleSetup::Unpack", "Bad data");

	for (uint i = 0; i < m_nAttachPoints; i++)
	{
		TAutoVar_Unpack(m_lAttachPoints[i].m_iNode, _pD);
		TAutoVar_Unpack(m_lAttachPoints[i].m_LocalPos, _pD);
	}

//	TAutoVar_Unpack(m_nTargetAttachPoints, _pD);
//	if (m_nTargetAttachPoints > sizeof_buffer(m_lTargetAttachPoints))
//		Error_static("CTentacleSetup::Unpack", "Bad data");

//	for (uint i = 0; i < m_nTargetAttachPoints; i++)
//		TAutoVar_Unpack(m_lTargetAttachPoints[i], _pD);

	for (uint i = 0; i < 2; i++)
		TAutoVar_Unpack(m_lAttachModels[i], _pD);

	uint16 nSplineBones = 0;
	TAutoVar_Unpack(nSplineBones, _pD);
	m_liSplineBones.SetLen(nSplineBones);
	for (uint16 i = 0; i < nSplineBones; i++)
		TAutoVar_Unpack(m_liSplineBones[i], _pD);

	TAutoVar_Unpack(m_BreathModels, _pD);
	TAutoVar_Unpack(m_DrainModel, _pD);

	TAutoVar_Unpack(m_iAGToken, _pD);
	TAutoVar_Unpack(m_Extrude, _pD);
	TAutoVar_Unpack(m_Spin, _pD);
	TAutoVar_Unpack(m_Scale, _pD);
	TAutoVar_Unpack((int8&)m_bMirrorAnim, _pD);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CPIDRegulator
|__________________________________________________________________________________________________
\*************************************************************************************************/
CVec3Dfp32 CPIDRegulator::Update(const CVec3Dfp32& _CurrPos, const CVec3Dfp32& _WantedPos, fp32 _Mass)
{
	CVec3Dfp32 Diff = _WantedPos - _CurrPos;
	fp32 K = 0.5f * _Mass;
	fp32 Ti = 350.0f;
	fp32 Td = 5.0f;
	fp32 N = 10.0f;
	fp32 h = 1.0f;
	m_IntegrateTerm += Diff * (K/Ti);
	m_DeriveTerm *= (Td / (Td + N*h));
	m_DeriveTerm -= (_CurrPos - m_LastPos) * (K*Td*N/(Td + N*h));
	m_LastPos = _CurrPos;
	return (Diff*K + m_IntegrateTerm + m_DeriveTerm);
}


CObjectGrabber::CObjectGrabber()
{
	m_Pid[0].Reset(CVec3Dfp32(0.0f));
	m_Pid[1].Reset(CVec3Dfp32(0.0f));
}

void CObjectGrabber::Init(const CMat4Dfp32& _CurrTargetMat, const CVec3Dfp32 _lGrabPos[2])
{
	CMat4Dfp32 InvObjMat;
	_CurrTargetMat.InverseOrthogonal(InvObjMat);
	for (uint i = 0; i < 2; i++)
	{
		m_Pid[i].Reset(_lGrabPos[i]);
		m_LocalGrabPos[i] = _lGrabPos[i];
		m_LocalGrabPos[i] *= InvObjMat;
	}
}

void CObjectGrabber::Update(const CMat4Dfp32& _CurrTargetMat, const CVec3Dfp32 _lGrabPos[2], fp32 _Mass, CVec3Dfp32 _lResult[2])
{
	for (uint i = 0; i < 2; i++)
	{
		CVec3Dfp32 CurrPos = m_LocalGrabPos[i];
		CurrPos *= _CurrTargetMat;
		_lResult[i] = m_Pid[i].Update(CurrPos, _lGrabPos[i], _Mass);
	}
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArmState
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTentacleArmState::CTentacleArmState()
{
	m_Task = TENTACLE_TASK_IDLE;
	m_State = TENTACLE_STATE_IDLE;
	m_iRotTrack = PLAYER_ROTTRACK_SPINE;
	m_iTarget = 0;
	m_Speed = 1.0f;
	m_TargetPos = 0.0f;
	m_GrabPoint = 0.0f;
	m_Length = 0.0f;

	m_Pos = 0.0f;
	m_Attach1_Cache.Unit();
	m_Attach1_Cache.GetRow(3).k[2] = 56.0f;
	m_LastTargetPos = 0.0f;
	m_CurrTargetPos = 0.0f;
	m_CurrAttachDir = 0.0f;

	m_iTargetOffset = 0;

	m_BreathCtrl = 0;

	m_BloodTick = 0;

/*
	m_plBones = NULL;
	m_PhysArm_Pid0.Reset(0);
	m_PhysArm_Pid1.Reset(0);*/
}


void CTentacleArmState::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Task, _pD);
	TAutoVar_Pack(m_State, _pD);
	TAutoVar_Pack(m_iTarget, _pD);
	TAutoVar_Pack(m_Speed, _pD);
	TAutoVar_Pack(m_TargetPos, _pD);
	TAutoVar_Pack(m_GrabPoint, _pD);
	TAutoVar_Pack(m_Length, _pD);
	TAutoVar_Pack(m_iRotTrack, _pD);
	TAutoVar_Pack(m_iTargetOffset, _pD);
	TAutoVar_Pack(m_BreathCtrl, _pD);
	TAutoVar_Pack(m_BloodTick, _pD);
}


void CTentacleArmState::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Task, _pD);
	TAutoVar_Unpack(m_State, _pD);
	TAutoVar_Unpack(m_iTarget, _pD);
	TAutoVar_Unpack(m_Speed, _pD);
	TAutoVar_Unpack(m_TargetPos, _pD);
	TAutoVar_Unpack(m_GrabPoint, _pD);
	TAutoVar_Unpack(m_Length, _pD);
	TAutoVar_Unpack(m_iRotTrack, _pD);
	TAutoVar_Unpack(m_iTargetOffset, _pD);
	TAutoVar_Unpack(m_BreathCtrl, _pD);
	TAutoVar_Unpack(m_BloodTick, _pD);
}


bool CTentacleArmState::IsIdle() const
{
	return (m_State == TENTACLE_STATE_OFF || m_State == TENTACLE_STATE_IDLE);
}


bool CTentacleArmState::IsDevouring() const
{
	return (m_Task == TENTACLE_TASK_DEVOURTARGET);
}

bool CTentacleArmState::IsCreepingDark() const
{
	return (m_Task == TENTACLE_TASK_TRAIL && m_State == TENTACLE_STATE_TRAIL);
}

void CTentacleArmState::PassOnTarget()
{
	if (m_pControlArm)
		m_pControlArm->m_iTarget = m_iTarget;

	m_iTarget = 0;
}

void CTentacleArmState::UpdateRunQueue()
{
	m_Task = m_QueueTask;
	m_State = m_QueueState;
	m_Speed = m_QueueSpeed;
}

void CTentacleArmState::UpdateTaskQueue()
{
	m_QueueTask = m_Task;
	m_QueueState = m_State;
	m_QueueSpeed = m_Speed;
}

void CTentacleArmState::UpdateTaskQueue(uint8 _Task, uint8 _State, fp32 _Speed)
{
	m_QueueTask = _Task;
	m_QueueState = _State;
	m_QueueSpeed = _Speed;
}

void CTentacleArmState::UpdateTask(uint8 _Task)
{
	m_Task = _Task;
}

void CTentacleArmState::UpdateTask(uint8 _Task, fp32 _Speed)
{
	m_Task = _Task;
	m_Speed = _Speed;
}

void CTentacleArmState::UpdateTask(uint8 _Task, uint8 _State, fp32 _Speed)
{
	m_Task = _Task;
	m_State = _State;
	m_Speed = _Speed;
}

void CTentacleArmState::UpdateState(uint8 _State, fp32 _Speed)
{
	m_State = _State;
	m_Speed = _Speed;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleSystemAnimGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTentacleSystemAnimGraph::CTentacleSystemAnimGraph()
{
	m_spAGI = MNew(CWAG2I);
	m_spMirror = MNew1(CWAG2I_Mirror, 1);
#ifndef M_RTM
	m_spAGI->m_bDisableDebug = false;
#endif
	AG2_RegisterCallbacks(NULL);
}


void CTentacleSystemAnimGraph::SetInitialProperties(const CWAG2I_Context* _pContext)
{
	SetNumProperties(0, TENTACLE_AG2_PROPERTY_INT_NUMPROPERTIES, TENTACLE_AG2_PROPERTY_BOOL_NUMPROPERTIES);
	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DEVOURTARGET, TENTACLE_AG2_DEVOURTARGET_UNDEFINED);
	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DIRECTION, TENTACLE_AG2_DIRECTION_UNDEFINED);
	SetPropertyBool(TENTACLE_AG2_PROPERTY_BOOL_ALWAYSTRUE, true);
	SetPropertyBool(TENTACLE_AG2_PROPERTY_BOOL_ISSERVER, _pContext->m_pWPhysState->IsServer());
}


void CTentacleSystemAnimGraph::AG2_OnEnterState(const CWAG2I_Context* _pContext, 
                                                CAG2TokenID _TokenID, 
                                                CAG2StateIndex _iState, 
                                                CAG2AnimGraphID _iAnimGraph, 
                                                CAG2ActionIndex _iEnterAction)
{
	if (!m_spAGI || !m_spAGI->AcquireAllResources(_pContext) || _TokenID >= TENTACLE_AG2_TOKEN_NUMTOKENS)
		return;

	const CXRAG2_State* pState = m_spAGI->GetState(_iState,_iAnimGraph);
	if (pState)
		m_lTokenStateFlags[_TokenID] = pState->GetFlags(0);
}


void CTentacleSystemAnimGraph::UpdateImpulseState(const CWAG2I_Context* _pContext)
{
	// Check if we should return from current look state
	for (int8 i = 0; i < TENTACLE_AG2_TOKEN_NUMTOKENS; i++)
	{
		bool bLookEnded = EndTick_Reached(i, _pContext->m_pWPhysState->GetGameTick());

		if (Token_Check(i, TENTACLE_AG2_STATEFLAG_LOOKACTIVE) && bLookEnded)
			SendImpulse_DemonHead(_pContext, i, TENTACLE_AG2_IMPULSEVALUE_LOOKRETURN);
	}
}


void CTentacleSystemAnimGraph::Copy(const CTentacleSystemAnimGraph& _CD)
{
	CWO_ClientData_AnimGraph2Interface::Copy(_CD);
	for (int32 i = 0; i < TENTACLE_AG2_TOKEN_NUMTOKENS; i++)
	{
		m_lEndTick[i] = _CD.m_lEndTick[i];
		m_lTokenStateFlags[i] = _CD.m_lTokenStateFlags[i];
	}
	m_bNeedUpdate = _CD.m_bNeedUpdate;
}


void CTentacleSystemAnimGraph::Clear()
{
	CWO_ClientData_AnimGraph2Interface::Clear();
	for (int32 i = 0; i < TENTACLE_AG2_TOKEN_NUMTOKENS; i++)
	{
		m_lEndTick[i] = 0;
		m_lTokenStateFlags[i] = 0;
	}
	m_bNeedUpdate = false;
}


bool CTentacleSystemAnimGraph::SendImpulse_DemonHead(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CAG2ImpulseValue _Value)
{
	const bool bResult = m_spAGI->SendImpulse(_pContext, CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_DEMONHEAD, _Value), _iToken);
	m_bNeedUpdate |= bResult;
	return bResult;
}


bool CTentacleSystemAnimGraph::Token_Check(CAG2TokenID _iToken, uint32 _TokenFlag)
{ 
	return ((m_lTokenStateFlags[_iToken] & _TokenFlag) != 0); 
}


int CTentacleSystemAnimGraph::EndTick_Get(CAG2TokenID _iToken)
{ 
	return m_lEndTick[_iToken]; 
}


void CTentacleSystemAnimGraph::EndTick_Set(CAG2TokenID _iToken, int _GameTick, const CWAG2I_Context* _pContext)	
{ 
	const CWorld_PhysState& PhysState = *_pContext->m_pWPhysState;
	fp32 Duration = m_spAGI->GetCurrentLoopDuration(_pContext, _iToken);
	int DurationTicks = TruncToInt(PhysState.GetGameTicksPerSecond() * Duration);

	m_lEndTick[_iToken] = _GameTick + DurationTicks + 1; 
}


void CTentacleSystemAnimGraph::EndTick_Set(CAG2TokenID _iToken, int _GameTick)									
{ 
	m_lEndTick[_iToken] = _GameTick; 
}


bool CTentacleSystemAnimGraph::EndTick_Reached(CAG2TokenID _iToken, int _GameTick, int _Subtract)
{ 
	return (m_lEndTick[_iToken] != 0) && ((m_lEndTick[_iToken] - _Subtract) <= _GameTick);
}


bool CTentacleSystemAnimGraph::Anim_Look(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _LookDirection, fp32 _Duration)
{
	DEBUG_VALIDATETOKEN(_iToken, "Anim_Look");

	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_MASK_LOOK))
		return false;

	const CWorld_PhysState& PhysState = *_pContext->m_pWPhysState;
	int DurationTicks = TruncToInt(PhysState.GetGameTicksPerSecond() * _Duration);
	EndTick_Set(_iToken, PhysState.GetGameTick() + DurationTicks);

	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DIRECTION, _LookDirection);
	return SendImpulse_DemonHead(_pContext, _iToken, TENTACLE_AG2_IMPULSEVALUE_LOOK);
}


bool CTentacleSystemAnimGraph::Anim_Hurt(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _ImpactDirection)
{
	DEBUG_VALIDATETOKEN(_iToken, "Anim_Hurt");

	// If we get hurt while already playing animation, switch direction
	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_HURTACTIVE))
	{
		int PreviousDirection = GetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DIRECTION);
		int NewDirection = (PreviousDirection == TENTACLE_AG2_DIRECTION_LEFT) ? TENTACLE_AG2_DIRECTION_RIGHT : TENTACLE_AG2_DIRECTION_LEFT;
		SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DIRECTION, (NewDirection + TENTACLE_AG2_DIRECTION_NUM));
		
		return SendImpulse_DemonHead(_pContext, _iToken, TENTACLE_AG2_IMPULSEVALUE_HURT);
	}

	// Prevent hurt animation during certain animation
	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_MASK_HURT))
		return false;

	// Set random direction for head. //new direction if heads are not already playing any hurt animation
	//if (Token_Check(TENTACLE_AG2_TOKEN_HUGIN, TENTACLE_AG2_STATEFLAG_HURTACTIVE) && 
	//    Token_Check(TENTACLE_AG2_TOKEN_MUNIN, TENTACLE_AG2_STATEFLAG_HURTACTIVE))
	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DIRECTION, _ImpactDirection);
	
	return SendImpulse_DemonHead(_pContext, _iToken, TENTACLE_AG2_IMPULSEVALUE_HURT);
}


bool CTentacleSystemAnimGraph::Anim_DevourTarget(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _DevourVariation)
{
	DEBUG_VALIDATETOKEN(_iToken, "Anim_DevourTarget");

	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_MASK_DEVOUR))
		return false;

	// Send heart animation start
	bool bHeartResult = true;
	if (_iToken == TENTACLE_AG2_TOKEN_HUGIN)
	{
		if (!Token_Check(TENTACLE_AG2_TOKEN_HEART, TENTACLE_AG2_STATEFLAG_DEVOURTARGET))
		{
			bHeartResult = m_spAGI->SendImpulse(_pContext, CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_HEART, TENTACLE_AG2_IMPULSEVALUE_HEART_DEVOUR_01), TENTACLE_AG2_TOKEN_HEART);
			m_bNeedUpdate |= bHeartResult;
		}
	}

	SetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DEVOURTARGET, _DevourVariation);
	return (bHeartResult && SendImpulse_DemonHead(_pContext, _iToken, TENTACLE_AG2_IMPULSEVALUE_DEVOUR));
}


bool CTentacleSystemAnimGraph::Anim_Idle(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, bool _bBoredom)
{
	DEBUG_VALIDATETOKEN(_iToken, "Anim_Boredom");

	// Prevent idle animations if something other is being played
//	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_HURTACTIVE | 
//	                         TENTACLE_AG2_STATEFLAG_LOOKMOVEACTIVE | TENTACLE_AG2_STATEFLAG_LOOKACTIVE))
	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_MASK_IDLE))
		return false;

	CAG2ImpulseValue ImpulseValue = 0;
	if (_bBoredom)
	{
		static int iLastHugin = -1;
		static int iLastMunin = -1;
		const int GameTick = _pContext->m_pWPhysState->GetGameTick();

		int iRandom = 0;
		switch(_iToken)
		{
		case TENTACLE_AG2_TOKEN_HUGIN:
			iRandom = iLastHugin = CFXSysUtil::RandExcludeSeed(3, iLastHugin, GameTick);
			break;

		case TENTACLE_AG2_TOKEN_MUNIN:
			iRandom = iLastMunin = CFXSysUtil::RandExcludeSeed(3, iLastMunin, GameTick);
			break;

		default:
			iRandom = CFXSysUtil::RandExcludeSeed(3, -1, GameTick);
			break;
		}

		//MRTC_GetRand()->InitRand((uint32)GameTick);
		//const int iRandom = (int)MRTC_GetRand()->GenRand32() % 3;
		
		switch (iRandom)
		{
		case 0:
			ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_BOREDOM_SLEEP;
			break;

		case 1:
			ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_BOREDOM_FIGHT;
			break;

		case 2:
			ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_BOREDOM_TTPLAYER;
			break;

		default:
			M_TRACE("CTentacleSystemAnimGraph::Anim_Boredom: Invalid boredom type!\n");
			break;
		}

		// Send impulse and set animation end tick before returning
		bool bReturn = SendImpulse_DemonHead(_pContext, _iToken, ImpulseValue);
		EndTick_Set(_iToken, GameTick, _pContext);
		
		// Talk to player is cut togheter using segments.. *sigh*
		if (iRandom == 2)
		{
			const fp32 TicksPerSec = _pContext->m_pWPhysState->GetGameTicksPerSecond();
			EndTick_Set(_iToken, GameTick + TruncToInt(TicksPerSec * 3.27f));
		}
		return bReturn;
	}
	else
	{
		ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_IDLE;
		EndTick_Set(_iToken, 0);
	}

	// Send impulse if impulse value is valid and return
	return (ImpulseValue) ? SendImpulse_DemonHead(_pContext, _iToken, ImpulseValue) : false;
}


bool CTentacleSystemAnimGraph::Anim_Growl(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, bool _bGrowl)
{
	// Validate token in debug only
	DEBUG_VALIDATETOKEN(_iToken, "Anim_Growl");

	// Prevent growl from start playing if certain animations are played
	if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_MASK_GROWL))
		return false;

	CAG2ImpulseValue ImpulseValue = 0;
	if (_bGrowl)
	{
		// We want to growl, check if we're in base growl
		if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_GROWLINGBASE))
		{
			// Do special thing
			fp32 FloatRand = MFloat_GetRand(_pContext->m_pWPhysState->GetGameTick() * (_iToken+1)) +
							 MFloat_GetRand(_pContext->m_pWPhysState->GetGameTick() * 0xfab5 * (_iToken+1));
			if (TruncToInt(FloatRand * 100.0f) >= 195)
			{
				ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_STARTGROWLVIOLENT;	
			}
			else
				return false;
		}
		else if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_GROWLING))
			return false;
		else
		{
			ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_GROWL;
		}
	}
	else
	{
		// End growling,
		if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_GROWLINGBASE))
		{
			return Anim_Idle(_pContext, _iToken, false);
		}
		else if (Token_Check(_iToken, TENTACLE_AG2_STATEFLAG_GROWLING))
		{
			ImpulseValue = TENTACLE_AG2_IMPULSEVALUE_ENDGROWLVIOLENT;
		}
		else
			return false;
	}

	// Send impulse to demon heads
	return SendImpulse_DemonHead(_pContext, _iToken, ImpulseValue);
}


#ifndef M_RTM
bool CTentacleSystemAnimGraph::Debug_ValidateToken(CAG2TokenID _iToken, const char* _pString)
{
	if (_iToken >= TENTACLE_AG2_TOKEN_NUMTOKENS)
	{
		M_TRACEALWAYS(CStrF("CTentacleSystemAnimGrapg::%s: Token was invalid (%d), Max token allowed is %d.\n", _pString, _iToken, TENTACLE_AG2_TOKEN_NUMTOKENS));
		return false;
	}
	return true;
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_TentacleSystem_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_TentacleSystem_ClientData::CWO_TentacleSystem_ClientData()
	: m_pObj(NULL)
	, m_pWPhysState(NULL)
{
}


void CWO_TentacleSystem_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;

	m_StartTime = _pWPhysState->GetGameTime();
	m_ArmControl = 0.0f;

	m_AnimGraph.Clear();
	m_AnimGraph.GetAG2I()->SetEvaluator(&m_AnimGraph);
	m_AnimGraph.SetAG2I(m_AnimGraph.GetAG2I());

	CWAG2I_Context AG2Context(_pObj, _pWPhysState, CMTime());
	m_AnimGraph.SetInitialProperties(&AG2Context);

	m_TargetBoredom = (int)(m_pWPhysState->GetGameTicksPerSecond() * TENTACLE_BASE_BOREDOM);
	m_CurrentBoredom = 0;

	m_nDamage = 0;
	m_GrabPower = 0.0f;

	m_PowerFade = 0.0f;

	m_liSounds.SetLen(TENTACLE_NumSounds);
	for (uint i = 0; i < TENTACLE_NumSounds; i++)
		m_liSounds[i] = 0;
}


CWObject_Character* CWO_TentacleSystem_ClientData::GetMaster()
{
	M_ASSERT(m_pWPhysState->IsServer(), "This is server-only!");
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);

	CWObject* pObj = pWServer->Object_Get(m_iOwner);
	return TDynamicCast<CWObject_Character>(pObj);
}


bool CWO_TentacleSystem_ClientData::Server_IsControllerIdle(CWObject_Character* _pOwnerChar)
{
	return (_pOwnerChar) ? _pOwnerChar->IsControllerIdle() : false;
}


void CWO_TentacleSystem_ClientData::UpdateModelInstances()
{
	uint nArms = m_lArmSetup.Len();
	for (uint i = 0; i < nArms; i++)
	{
		CTentacleArmSetup& ArmSetup = m_lArmSetup[i];
		ArmSetup.m_lAttachModels[0].UpdateModelInstances(m_pWPhysState, m_pObj);
		ArmSetup.m_lAttachModels[1].UpdateModelInstances(m_pWPhysState, m_pObj);
		ArmSetup.m_DrainModel.UpdateModelInstances(m_pWPhysState, m_pObj);
		ArmSetup.m_BreathModels.UpdateModelInstances(m_pWPhysState, m_pObj);
	}
}


// This is run on both server & client
void CWO_TentacleSystem_ClientData::OnRefresh()
{
	// For client/server specific tasks
	CWorld_Server* pWServer = m_pWPhysState->IsServer() ? safe_cast<CWorld_Server>(m_pWPhysState) : NULL;
	CWorld_Client* pWClient = m_pWPhysState->IsClient() ? safe_cast<CWorld_Client>(m_pWPhysState) : NULL;

	// Make sure we're positioned at the same place as owner
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	CWObject_Character* pOwnerChar = NULL;
	if (pOwner)
	{
		pOwnerChar = (pWServer) ? safe_cast<CWObject_Character>(pOwner) : NULL;
		const CMat4Dfp32& OwnerPos = pOwner->GetPositionMatrix();

		const fp32 DistSqr = OwnerPos.GetRow(3).DistanceSqr(m_pObj->GetPosition());
		if (DistSqr > 8.0f)
			m_pWPhysState->Object_SetPosition(m_pObj->m_iObject, OwnerPos);

		if (pWClient)
		{
			uint8 Darkness = pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDARKNESS), pOwner->m_iObject);
			uint8 MaxDarkness = pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXDARKNESS), pOwner->m_iObject);
			
			fp32 GameTickTime = pWClient->GetGameTickTime();
			fp32 DarknessTarget = Clamp01(1.0f - (fp32(Darkness) / fp32(MaxDarkness)));
			m_Darkness = Clamp01(CFXSysUtil::LerpMT(m_Darkness, DarknessTarget, GameTickTime * 2.0f));
		}
		/*	-------------------------------------------------------------------------------------------------------
			This is a very good idea and looks quite good. But it has some artefact like clipping into
			players view and so on for the moment. Need to rework this a little bit to make it appear as it should.
			-------------------------------------------------------------------------------------------------------
				CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);
				if(pOwnerCD)
				{
					fp32 dHealth = fp32(pOwnerCD->m_Health) / pOwnerCD->m_MaxHealth;
					fp32 Retract = 30 * (1.0f - dHealth);
					if(Retract > m_Retract + 0.04f)
						m_Retract += 0.05f;
					else if(Retract < m_Retract)
						m_Retract -= 1.0f;
				}
			------------------------------------------------------------------------------------------------------ */
	}

	// Animgraph doesn't need update at this moment
	m_AnimGraph.m_bNeedUpdate = false;

	// Get position
	const CVec3Dfp32& ObjPos = m_pObj->GetPosition();
	CBox3Dfp32 BBox(ObjPos - CVec3Dfp32(4.0f), ObjPos + CVec3Dfp32(4.0f));

    // Update all arms
	bool bBoredom = true;
	bool bControllerBoredom = (pWServer) ? Server_IsControllerIdle(pOwnerChar) : false;
	bool bBoredomState = false;
	uint nArms = m_lArmState.Len();
	uint nIdleArms = 0;
	for (uint iArm = 0; iArm < nArms; iArm++)
	{
		CTentacleArmState& Arm = m_lArmState[iArm];
		CTentacleArmSetup& ArmSetup = m_lArmSetup[iArm];

		// Skip if idle
		if (Arm.m_State == TENTACLE_STATE_IDLE || Arm.m_State == TENTACLE_STATE_WIGGLE2)
		{
			// Update animgraphs on server
			if (pWServer)
			{
				nIdleArms++;
				bool bAtTarget = false;
				Server_UpdateArmAnimGraph(Arm, ArmSetup, bAtTarget, 0);
			}
			continue;
		}
		
		// Get tentacle spline
		CSpline_Tentacle Spline;
		GetSpline(iArm, Spline, false);

		// Enlarge bound box
		for (uint iPoint = 0; iPoint < Spline.m_lPoints.Len(); iPoint++)
			BBox.Expand(Spline.m_lPoints[iPoint].m_Pos);

		// Move arm towards target
		Arm.m_Length = Clamp(Arm.m_Length + Arm.m_Speed, 0.0f, Spline.m_Length);
		if (M_Fabs(Arm.m_Speed - TENTACLE_SPEED_SNAPTOTARGET) < 0.1f)
			Arm.m_Length = Spline.m_Length;

		if (Arm.m_State == TENTACLE_STATE_TRAIL && Arm.m_Length > 64.0f)
			Arm.m_Speed = TENTACLE_SPEED_SNAPTOTARGET;

		// Have we reached our target
		bool bAtTarget = (Arm.m_Speed < 0.0f) ? (Arm.m_Length <= 0.0f) : (Arm.m_Length >= Spline.m_Length);

		CSpline_Tentacle::SplinePos Pos;
		Spline.FindPos(Arm.m_Length, Pos, Spline.m_EndMat.GetRow(2));

		Arm.m_Pos = Pos.mat.GetRow(3);

		//CMat4Dfp32 Rot;
		//Spline.CalcMat(1.0f, Rot, CVec3Dfp32(0,0,1));
		//Arm.m_Dir = Rot.GetRow(2);
		Arm.m_Dir = Spline.m_EndMat.GetRow(0);

		// Update animgraphs and arm states on server
		if (pWServer)
		{
			// Update boredom
			if ((!bBoredom || !bControllerBoredom) && (Arm.m_Task == TENTACLE_TASK_BOREDOM && Arm.m_State == TENTACLE_STATE_WIGGLE))
			{
				CWObject_TentacleSystem* pServerObj = safe_cast<CWObject_TentacleSystem>(m_pObj);
				CWAG2I_Context AGI2Context(pServerObj, pWServer, pWServer->GetGameTime());
				m_AnimGraph.Anim_Idle(&AGI2Context, ArmSetup.m_iAGToken, false);
				Arm.UpdateTask(TENTACLE_TASK_WIGGLE, Arm.m_State, Arm.m_Speed);
				bBoredom = false;
				bControllerBoredom = false;
			}
			else if (bBoredom && bControllerBoredom && (Arm.m_Task == TENTACLE_TASK_BOREDOM && Arm.m_State == TENTACLE_STATE_WIGGLE))
				bBoredomState = true;

			// Update animgraph and arm states
			fp32 LengthToTarget = Spline.m_Length - Arm.m_Length;
			Server_UpdateArmAnimGraph(Arm, ArmSetup, bAtTarget, LengthToTarget);
			Server_UpdateArm(Arm, bAtTarget, Pos.mat, ArmSetup, LengthToTarget);

			//if (Arm.m_plBones)
			//	Server_UpdatePhysArm(Arm, Spline);
		}
	}


	// If player isn't doing ANYTHING and demon heads doesn't find any amuzing to do. They get bored (Bastards!)
//	Server_UpdateBoredom(pWServer, 
	if (pWServer)
	{
		// Increase idle tick
		if ((nIdleArms != nArms) && (bBoredom && bControllerBoredom && !bBoredomState))
			m_CurrentBoredom++;

		// Player found something to do
		else
		{
			// Setup new boredom time
			if (m_CurrentBoredom != 0)
			{
				const fp32 TicksPerSec = pWServer->GetGameTicksPerSecond();
				const fp32 BoredomTime = TENTACLE_BASE_BOREDOM + (MFloat_GetRand(pWServer->GetGameTick()) * TENTACLE_RAND_BOREDOM);
				m_TargetBoredom = TruncToInt(TicksPerSec * BoredomTime);
			}

			m_CurrentBoredom = 0;
		}
	}
	

	// Refresh and update impulse states on animgraphs
	CWAG2I_Context AGIContext(m_pObj, m_pWPhysState, m_pWPhysState->GetGameTime());
	m_AnimGraph.m_spAGI->Refresh(&AGIContext);
	m_AnimGraph.m_bNeedUpdate = m_AnimGraph.m_bNeedUpdate || m_AnimGraph.m_spAGI->GetNeedUpdate();
	m_AnimGraph.UpdateImpulseState(&AGIContext);

	if(pWClient && pOwner)
	{
		m_PowerFade = GetDemonHeadFade(pWClient, pOwner);
	}


	// Debug rendering (client)
	#ifndef M_RTM
	{
		if (0 && pWClient)
			pWClient->Debug_RenderAABB(BBox.m_Min, BBox.m_Max, 0xffff0000, 1.0f, true);
	}
	#endif

	// Set new bound box
	CMat4Dfp32 InvObjMat;
	m_pObj->GetPositionMatrix().InverseOrthogonal(InvObjMat);
	BBox.Transform(InvObjMat, BBox);
	if (pWClient)
		m_pWPhysState->Object_SetVisBox(m_pObj->m_iObject, BBox.m_Min, BBox.m_Max);
	m_pObj->SetVisBoundBox(BBox);
}

fp32 CWO_TentacleSystem_ClientData::GetDemonHeadFade(CWorld_Client* _pWClient, CWObject_CoreData* _pOwner, int8* _piPower, int8* _piLastPower)
{
	CWObject_Message OwnerMsg(OBJMSG_CHAR_GETDARKNESSPOWER);
	int8 iPower = _pWClient->ClientMessage_SendToObject(OwnerMsg, _pOwner->m_iObject);
	if(iPower == -1)
		return -1.0f;

	OwnerMsg.m_Msg = OBJMSG_CHAR_GETLASTDARKNESSPOWER;
	int8 iLastPower = (uint8)_pWClient->ClientMessage_SendToObject(OwnerMsg, _pOwner->m_iObject);

	if(iPower == 4) iPower -= 1;
	if(iLastPower == 4) iLastPower -= 1;

	if(_piPower) *_piPower = iPower;
	if(_piLastPower) *_piLastPower = iLastPower;

	fp32 Fade = (fp32)iPower;

	fp32 res_fade = Fade;

	if((iPower == 3 && iLastPower == 0) || (iPower == 0 && iLastPower == 3))
	{
		if(Fade != m_PowerFade)
		{
			if(iPower == 3 && iLastPower == 0)
			{
				if(m_PowerFade > 0.0f && m_PowerFade < 1.0f)
					res_fade = Clamp(m_PowerFade - _pWClient->GetGameTickTime(), 0.0f, m_PowerFade);
				else
				{
					if(m_PowerFade == 0.0f)
						m_PowerFade = 4.0f;
					res_fade = Clamp(m_PowerFade - _pWClient->GetGameTickTime(), Fade, Fade + 1.0f);	
				}
			}
			else
			{
				if(m_PowerFade > 1.0f && m_PowerFade < 2.0f)
					res_fade = Clamp(m_PowerFade + _pWClient->GetGameTickTime(), 1.0f, 2.0f);	
				else
				{
					if(m_PowerFade == 4.0f)
						res_fade = 0.0f;
					else
						res_fade = Clamp(m_PowerFade + _pWClient->GetGameTickTime(), 3.0f, 4.0f);	
				}
			}
		}
	}
	else
	{
		if(Fade < m_PowerFade)
			res_fade = Clamp(m_PowerFade - _pWClient->GetGameTickTime(), Fade, m_PowerFade);
		else if(Fade > m_PowerFade)
			res_fade = Clamp(m_PowerFade + _pWClient->GetGameTickTime(), m_PowerFade, Fade);	
	}

	return res_fade;
}


bool CWO_TentacleSystem_ClientData::DemonArmIntersect(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, uint _iObj, CMat4Dfp32* _pRetMat)
{
	CWObject_CoreData* pTarget = m_pWPhysState->Object_GetCD(_iObj);
	if (!pTarget)
		return false;

	CWO_OnIntersectLineContext IntLineContext;
	IntLineContext.m_pObj = pTarget;
	IntLineContext.m_pPhysState = m_pWPhysState;
	IntLineContext.m_p0 = _Pos0;
	IntLineContext.m_p1 = _Pos1;
	IntLineContext.m_ObjectFlags = OBJECT_FLAGS_PHYSOBJECT;
	IntLineContext.m_ObjectIntersectionFlags = 0;
	IntLineContext.m_MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

	CCollisionInfo CollisionInfo;
	CollisionInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;	

	if (pTarget->m_pRTC->m_pfnOnIntersectLine(IntLineContext, &CollisionInfo))
	{
		_pRetMat->Unit();
		_pRetMat->GetRow(2) = CollisionInfo.m_Plane.n;
		_pRetMat->GetRow(1) = CVec3Dfp32(0, 0, -1);
		_pRetMat->RecreateMatrix(2, 1);
		_pRetMat->GetRow(3) = CollisionInfo.m_Pos + _pRetMat->GetRow(0) * 0.1f; // just a bit away from the collpt
		return true;
	}
	return false;
}


//
// tries to move a sphere of size '_Radius' from '_OldPos' to '_NewPos', 
// and modifies _NewPos if there is a collision,
//
bool CWO_TentacleSystem_ClientData::CheckCollision(const CVec3Dfp32& _OldPos, CVec3Dfp32& _NewPos, fp32 _Radius, int _iExcludeObj)
{
	CMat4Dfp32 Origin, Dest;
	Origin.Unit();
	Origin.GetRow(3) = _OldPos;
	Dest.Unit();
	Dest.GetRow(3) = _NewPos;

#ifndef M_RTM
	m_pWPhysState->Debug_RenderSphere(Origin, _Radius, DEBUG_COLOR_GREEN);
	m_pWPhysState->Debug_RenderSphere(Dest, _Radius, DEBUG_COLOR_RED);
	m_pWPhysState->Debug_RenderWire(_OldPos, _NewPos, DEBUG_COLOR_RED);
#endif

	int Medium = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_DYNAMICSSOLID | XW_MEDIUM_GLASS; //XW_MEDIUM_PLAYERSOLID;
	int ObjIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL;

	// Since 'Phys_IntersectWorld' doesn't do sweeping intersection tests, we first do a line check, and then move on to sphere testing.
	// (this still isn't perfect, but a lot better!)
	CCollisionInfo CInfo;
	if (m_pWPhysState->Phys_IntersectLine(_OldPos, _NewPos, OBJECT_FLAGS_PHYSOBJECT, ObjIntersectFlags, Medium, &CInfo, _iExcludeObj))
	{
		if (!CInfo.m_bIsValid)
			return false;

		_NewPos = CInfo.m_Pos;
		Dest.GetRow(3) = CInfo.m_Pos;
		CInfo.Clear();
	}

	CWO_PhysicsState TmpPhys(OBJECT_PRIMTYPE_SPHERE, 0, Medium, CVec3Dfp32(_Radius), OBJECT_FLAGS_PHYSOBJECT, ObjIntersectFlags); 
	if (m_pWPhysState->Phys_IntersectWorld((CSelection*)NULL, TmpPhys, Origin, Dest, _iExcludeObj, &CInfo))
	{
		if (CInfo.m_bIsValid)
			_NewPos = CInfo.m_Pos + CInfo.m_Plane.n * _Radius;
		else
			_NewPos = _OldPos;
	}
	return true;
}


void CWO_TentacleSystem_ClientData::Server_CreateDevourBlood(CTentacleArmState& _Arm)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject_CoreData* pTarget = pWServer->Object_Get(_Arm.m_iTarget);
	if (pTarget)
	{
		CMat4Dfp32 TargetMat;
		GetCharBoneMat(*pTarget, _Arm.m_iRotTrack, TargetMat);

		SCollisionFlags ColFlags(TargetMat.GetRow(3), TargetMat.GetRow(3) + CVec3Dfp32(0.0f, 0.0f, 16.0f));
		ColFlags.m_iExclude = m_iOwner;
		ColFlags.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
		CreateBloodEffect(pWServer, pTarget->m_iObject, _Arm.m_Pos, &ColFlags, true, "hitEffect_tentacle", &_Arm.m_Pos, NULL, 1, 2, 8, 12);

		// Tell head we want to be gory and bloody in the mouth at this tick
		Server_UpdateMouthBlood(pWServer, _Arm);
	}
}


void CWO_TentacleSystem_ClientData::Server_UpdateMouthBlood(CWorld_Server* _pWServer, CTentacleArmState& _Arm)
{
	_Arm.m_BloodTick = _pWServer->GetGameTick();
	m_lArmState.MakeDirty();
}


// Perhaps move this into the game object class?
void CWO_TentacleSystem_ClientData::Server_UpdateArmAnimGraph(CTentacleArmState& _Arm, CTentacleArmSetup& _ArmSetup, bool _bAtTarget, fp32 _LengthToTarget)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject_TentacleSystem* pServerObj = safe_cast<CWObject_TentacleSystem>(m_pObj);
	CWAG2I_Context AGIContext(pServerObj, pWServer, pWServer->GetGameTime());

	uint8 iToken = _ArmSetup.m_iAGToken;
	bool bHostileNearby = _Arm.m_bHostileNearby;
	int GameTick = pWServer->GetGameTick();
	fp32 TicksPerSec = pWServer->GetGameTicksPerSecond();

	switch (_Arm.m_State)
	{
	case TENTACLE_STATE_WIGGLE:
	case TENTACLE_STATE_INTERESTED:
		{
			if (_Arm.IsTask_DevourTarget())
			{
				// Update animgraph
				m_AnimGraph.Anim_DevourTarget(&AGIContext, iToken, TENTACLE_AG2_DEVOURTARGET_NORMAL_VARIATION_1);
			}
			break;
		}
	}
	
	switch (_Arm.m_State)
	{
	case TENTACLE_STATE_INTERESTED:
		{
			// Play idle or growl animation when Hugin or Munin finds something interesting depending on enemies nearby
			m_AnimGraph.Anim_Growl(&AGIContext, iToken, bHostileNearby);
		}
		break;

	case TENTACLE_STATE_WIGGLE:
		{
			if (_bAtTarget)
			{
				if (_Arm.IsTask_Wiggle())
				{
					if (m_CurrentBoredom >= m_TargetBoredom)
					{
						m_AnimGraph.Anim_Idle(&AGIContext, iToken, true);
						_Arm.UpdateTask(TENTACLE_TASK_BOREDOM);
						m_lArmState.MakeDirty();
					}
					else
					{
						// Only look around while controller isn't moving
						if (Server_IsControllerIdle(GetMaster()) && !bHostileNearby && Random > 0.99f)
						{
							int Direction = TENTACLE_AG2_DIRECTION_LEFT + TruncToInt(Random * 3.99f);
							fp32 Duration = 1.5f + (Random * 2.0f);
							m_AnimGraph.Anim_Look(&AGIContext, iToken, Direction, Duration);
						}
						else
						{
							// Handle growl/idle animation
							m_AnimGraph.Anim_Growl(&AGIContext, iToken, bHostileNearby);
						}
					}
				}

				// Handle boredom -> idle
				else if (_Arm.IsTask_Boredom() && m_AnimGraph.EndTick_Reached(iToken, GameTick))
				{
					m_AnimGraph.Anim_Idle(&AGIContext, iToken, false);
					_Arm.UpdateTask(TENTACLE_TASK_WIGGLE);
					m_lArmState.MakeDirty();
				}
			}
		}
		break;

	case TENTACLE_STATE_RETURN:
		{
			if (_Arm.m_Task == TENTACLE_TASK_TRAIL && _bAtTarget)
				m_AnimGraph.Anim_Idle(&AGIContext, iToken, false);
		}
		break;
	}
}


void CWO_TentacleSystem_ClientData::Server_UpdateArm(CTentacleArmState& _Arm, bool _bAtTarget, const CMat4Dfp32& _EndPos, CTentacleArmSetup& _ArmSetup, fp32 _LengthToTarget)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject_TentacleSystem* pServerObj = safe_cast<CWObject_TentacleSystem>(m_pObj);
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	const CMat4Dfp32& ObjMat = pOwner ? pOwner->GetPositionMatrix() : m_pObj->GetPositionMatrix();
	const CVec3Dfp32& ObjPos = ObjMat.GetRow(3);
	fp32 Time = (m_pWPhysState->GetGameTime() - m_StartTime).GetTime();

	CWObject* pTarget = NULL;
	if (_Arm.m_iTarget > 0)
	{
		pTarget = pWServer->Object_Get(_Arm.m_iTarget);
		if (!pTarget)
		{ // wtf!
			_Arm.m_iTarget = 0;
			fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-6.0f);
			_Arm.UpdateState(TENTACLE_STATE_RETURN, Speed);
			return;
		}
	}

	switch (_Arm.m_State)
	{
	case TENTACLE_STATE_WIGGLE:
	case TENTACLE_STATE_INTERESTED:
		{
			if (_bAtTarget)
			{
				if (M_Fabs(_Arm.m_Speed - TENTACLE_SPEED_SNAPTOTARGET) > 1.0f)
				{
					_Arm.m_Speed = TENTACLE_SPEED_SNAPTOTARGET;
					m_lArmState.MakeDirty();
				}
			}

			uint iAGToken = _ArmSetup.m_iAGToken;
			if (iAGToken == TENTACLE_AG2_TOKEN_HUGIN || iAGToken == TENTACLE_AG2_TOKEN_MUNIN)
			{
				// Check which tick to blow "breath smoke"
				int32 BreathCtrl = _Arm.m_BreathCtrl;
				int32 GameTick = pWServer->GetGameTick();
				
				// Play breath sound
				if (GameTick == BreathCtrl)
				{
					int iObj = (pOwner) ? pOwner->m_iObject : 0;
					bool bHard = ((GetBreathFlags(BreathCtrl) & TENTACLE_BREATH_MODEL0) != 0);
					if (bHard)	pWServer->Sound_On(iObj, m_liSounds[TENTACLE_SOUND_BREATH_HARD_DEMONHEAD1 + iAGToken], WCLIENT_ATTENUATION_3D);
					else		pWServer->Sound_On(iObj, m_liSounds[TENTACLE_SOUND_BREATH_SOFT_DEMONHEAD1 + iAGToken], WCLIENT_ATTENUATION_3D);
				}

				// Set new breath tick
				fp32 TicksPerSec = pWServer->GetGameTicksPerSecond();
				if (GameTick > BreathCtrl + TruncToInt(TicksPerSec * 4.0f))
				{
					int32 WaitTicks = TruncToInt(TicksPerSec * 2.0f) + TruncToInt(TicksPerSec * (MFloat_GetRand(GameTick * (_ArmSetup.m_iAGToken+1)) * 3.0f));
					_Arm.m_BreathCtrl = GameTick + WaitTicks;
					m_lArmState.MakeDirty();
				}
			}
		}
		break;

	case TENTACLE_STATE_REACHPOSITION:
		{
			if (_bAtTarget)
			{
				// Send damage to untargeted stuff
				const CVec3Dfp32& Forward = _EndPos.GetRow(0);
				SendProjectileDamage(pWServer, _Arm.m_Pos + Forward * -16.0f, _Arm.m_Pos + Forward * 5.0f, Forward, 10);

				// Update arm state
				fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-12.0f);
				_Arm.UpdateState(TENTACLE_STATE_RETURN, Speed);
				m_lArmState.MakeDirty();

				// Play 'return' sound (once)
				int iObj = (pOwner) ? pOwner->m_iObject : 0;
				pWServer->Sound_On(iObj, m_liSounds[TENTACLE_SOUND_RETURN], WCLIENT_ATTENUATION_3D);
				//pWServer->Sound_At(_Arm.m_Pos, m_liSounds[TENTACLE_SOUND_RETURN], 0);
			}
		}
		break;

	case TENTACLE_STATE_REACHOBJECT:
		{
			if (_bAtTarget)
			{
				// ExtraLen is how far the demon arm should continue after hitting the char/object
				fp32 ExtraLen = 8.0f;
				CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pTarget); //didn't work: CWObject_Character::IsCharacter(pTarget);
				if (pChar)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
					bint bIsRagdoll = (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_RAGDOLLACTIVE) ;

					if (!bIsRagdoll && (_Arm.m_Task != TENTACLE_TASK_BREAKOBJECT) && (_Arm.m_Task != TENTACLE_TASK_GRABCHAROBJECT) && (_Arm.m_Task != TENTACLE_TASK_PUSHOBJECT))
					{
						// Force move to the behaviour
						bool bOk = false;
						CWAG2I_Context Context(pChar,m_pWPhysState,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWPhysState->GetGameTickTime()));
						const CXRAG2* pAG = pCD->m_AnimGraph2.GetAG2I()->GetAnimGraph(0);
						if (pAG)
						{
							int32 iGB = pAG->GetMatchingGraphBlock(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, AG2_IMPULSEVALUE_BEHAVIOR_DARKNESS_RESPONSE_KNIFE));
							if (iGB != -1)
							{
								const CXRAG2_GraphBlock* pGraphBlock = pAG->GetGraphBlock(iGB);
								if (pGraphBlock)
								{
									pCD->m_AnimGraph2.GetAG2I()->MoveGraphBlock(&Context,0,0,pGraphBlock->GetStartMoveTokenIndex());
									pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,m_nDamage);
									ExtraLen = -(pChar->GetPositionMatrix().GetRow(0) * _Arm.m_Dir) * 40.0f;	// 40.0 is tweaked to match animation
									bOk = true;
								}
							}
						}
						if (!bOk)
							InitCharRagdoll(pChar);
					}
				}

				if (_Arm.m_Task == TENTACLE_TASK_GRABHOLD)
				{
					int iGrabber = pOwner ? pOwner->m_iObject : m_pObj->m_iObject;

					// Create blood effect on character
					if (pChar)
					{
						SCollisionFlags ColFlags(_Arm.m_Pos + (_Arm.m_Dir * -4.0f), _Arm.m_Pos + (_Arm.m_Dir * 4.0f));
						ColFlags.m_iExclude = m_iOwner;
						ColFlags.m_ObjectFlags = OBJECT_FLAGS_CHARACTER;
						CreateBloodEffect(pWServer, pTarget->m_iObject, _Arm.m_Pos, &ColFlags, true, "hitEffect_tentacle", &_Arm.m_Pos, NULL, 1, 2);

						//Ragdoll sheit
						{
							FillChar(_Arm.m_Grabber.m_LocalGrabPos,sizeof(CVec3Dfp32)*2,0);
						}

						pChar->OnMessage(CWObject_Message(OBJMSG_CHAR_GRABBED_BY_DEMONARM, iGrabber));
					}
					else
					{
						if (pTarget->m_pRigidBody2)
						{
							CVec3Dfp32 lGrabPos[2];
							lGrabPos[0] = _Arm.m_GrabPoint;
							lGrabPos[0] *= pTarget->GetPositionMatrix();
							lGrabPos[1] = _EndPos.GetRow(0); // demon arm direction
							lGrabPos[1] *= 16.0f;
							lGrabPos[1] += lGrabPos[0];

							_Arm.m_Grabber.Init(pTarget->GetPositionMatrix(), lGrabPos);

							// Set our ourself (or our owner) as grabbed objects owner
							pTarget->m_iOwner = iGrabber;
						}

						// Damage the targeted object
						CWO_Damage Damage(10, DAMAGETYPE_DARKNESS, 0);
						Damage.SendExt(pTarget->m_iObject, m_iOwner, pWServer, NULL, NULL, 0, 0);
					}

					// Play 'grab' sound (once)
					pWServer->Sound_At(_Arm.m_Pos, m_liSounds[TENTACLE_SOUND_GRAB], WCLIENT_ATTENUATION_3D);

					// Force drop his/her weapon
					if (m_nDamage > pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH),pTarget->m_iObject))
						pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON), pTarget->m_iObject);

					// Enter hold-state
					_Arm.UpdateState(TENTACLE_STATE_GRABOBJECT, TENTACLE_SPEED_SNAPTOTARGET);
					m_ArmControl.k[0] = (_Arm.m_Pos - ObjPos).Length() + ExtraLen;
				}
				else if (_Arm.m_Task == TENTACLE_TASK_BREAKOBJECT)
				{
					// Damage the targeted object
					CVec3Dfp32 Pos = _Arm.m_Pos;
					CVec3Dfp32 Force = _Arm.m_Dir * 10.0f; // tweakme
					CWO_DamageMsg Msg(m_nDamage, DAMAGETYPE_DARKNESS, &Pos, &Force);
					Msg.Send(_Arm.m_iTarget, m_iOwner, pWServer);

					// Go back to idle-state
					fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-6.0f);
					_Arm.UpdateState(TENTACLE_STATE_RETURN, Speed);
				}
				else if (_Arm.m_Task == TENTACLE_TASK_PUSHOBJECT)
				{
					// Damage the targeted object
					CVec3Dfp32 Pos = _Arm.m_Pos;
					CVec3Dfp32 Force = _Arm.m_Dir * 10.0f; // tweakme
					CWO_DamageMsg Msg(m_nDamage, DAMAGETYPE_DARKNESS, &Pos, &Force);
					Msg.Send(_Arm.m_iTarget, m_iOwner, pWServer);

					// Go back to idle-state
					fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-6.0f);
					_Arm.UpdateState(TENTACLE_STATE_RETURN, Speed);

					static fp32 PushForce = 300.0f;
					const CMat4Dfp32& MatLook = (pTarget) ? pTarget->GetPositionMatrix() : m_pObj->GetPositionMatrix();

					Server_ThrowObject(_Arm, MatLook.GetRow(2), &PushForce);
				}
				else if (_Arm.m_Task == TENTACLE_TASK_GRABCHAROBJECT)
				{
					// Grab an object from a character...
					// Play 'grab' sound (once)
					pWServer->Sound_At(_Arm.m_Pos, m_liSounds[TENTACLE_SOUND_GRAB], WCLIENT_ATTENUATION_3D);

					// Force drop his/her weapon and change our target
					_Arm.m_iTarget = pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CREATEGRABBABLEITEMOBJECT,0,1), pTarget->m_iObject);

					// Enter hold-state
					_Arm.UpdateState(TENTACLE_STATE_GRABOBJECT, TENTACLE_SPEED_SNAPTOTARGET);
					m_ArmControl.k[0] = (_Arm.m_Pos - ObjPos).Length();
				}
				m_lArmState.MakeDirty();
			}
		}
		break;

	case TENTACLE_STATE_GRABOBJECT:
		{
			CVec3Dfp32 NewPos = ObjPos;

			NewPos.k[2] += 56.0f; // yeah yeah
			NewPos += ObjMat.GetRow(0) * m_ArmControl.k[0];

			// wiggle it
			NewPos += CVec3Dfp32(8.0f*M_Sin(Time*1.0f + 0.0f), 
								8.0f*M_Sin(Time*1.1f + 1.0f),
								8.0f*M_Sin(Time*1.2f + 2.0f));

			// don't go below "floor"
			NewPos.k[2] = Max(NewPos.k[2], ObjPos.k[2] + 16.0f);
			if (CheckCollision(_Arm.m_Pos, NewPos, 24.0f, _Arm.m_iTarget))
			{
				pWServer->Debug_RenderVector(ObjPos, NewPos - ObjPos, 0xff0000ff, 0.5f, true);
				Server_MoveObject(_Arm, NewPos, _EndPos);
			}

			// Check if the grabbed object is too far away - if so, release it.
			fp32 LenSqr = _Arm.m_Pos.DistanceSqr(ObjPos);
			if (LenSqr > Sqr(pServerObj->GetArmMaxLength() + 108.0f))
			{
				M_TRACE("Demon arm is too long (%.2f units)- relasing object\n", M_Sqrt(LenSqr));
				fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-16.0f - Random);
				_Arm.UpdateState(TENTACLE_STATE_RETURN, Speed);
				m_lArmState.MakeDirty();
			}
		}
		break;

	case TENTACLE_STATE_RETURN:
		{
			CWObject_Character* pCorpse = TDynamicCast<CWObject_Character>(pTarget);

			if (_bAtTarget)
			{
				// Stop 'hold' sound
				pServerObj->iSound(0) = 0;

				if (_Arm.m_Task == TENTACLE_TASK_IDLE)
					_Arm.m_State = TENTACLE_STATE_IDLE;
				
				// Run queued commands such as old states etc.
				else if (_Arm.m_Task == TENTACLE_TASK_TRAIL)
					_Arm.UpdateRunQueue();
				
				// Just update the task to idling
				else
					_Arm.UpdateTask(TENTACLE_TASK_IDLE, TENTACLE_STATE_IDLE, _Arm.m_Speed);
				
				// Reset any targets
				_Arm.m_iTarget = 0;
				m_lArmState.MakeDirty();
			}
		}
		break;
	}
}

#if 0
void CWO_TentacleSystem_ClientData::Server_UpdatePhysArm(CTentacleArmState& _Arm, const CSpline_Vec3Dfp32& _Spline)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject_TentacleSystem* pServerObj = safe_cast<CWObject_TentacleSystem>(m_pObj);

	// Update demon arm phys objects
	const fp32 PhysLinkSize = 16.0f;
	const fp32 MinPhysLen = 32.0f;

	M_ASSERT(_Arm.m_plBones, "no bones - why in here?!");
	TStaticArray<uint16, 100>& Bones = *_Arm.m_plBones;

//	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (_Arm.m_Length > MinPhysLen)
	{
		fp32 ArmLength = _Arm.m_Length - MinPhysLen;
		int nBones = Bones.Len();//m_pWServer->Object_GetNumChildren(m_iObject);
		fp32 PhysLength = nBones * PhysLinkSize;

		if (M_Fabs(PhysLength - ArmLength) > (PhysLinkSize * 0.5f + 2.0f))
		{
			// too few links?
			while ((ArmLength - PhysLength) > (PhysLinkSize * 0.5f + 2.0f))
			{
//				M_TRACE("Too few links (%d, %f, %f), adding one...\n", m_pWServer->Object_GetNumChildren(m_iObject), ArmLength, PhysLength);
				M_TRACE("Too few links (%d, %f, %f), adding one...\n", Bones.Len(), ArmLength, PhysLength);

				CSpline_Vec3Dfp32::SplinePos Pos0, Pos1;
				_Spline.FindPos( nBones      * PhysLinkSize + MinPhysLen, Pos0);
				_Spline.FindPos((nBones + 1) * PhysLinkSize + MinPhysLen, Pos1);

				CMat4Dfp32 PosMat;
				PosMat.GetRow(0) = (Pos1.mat.GetRow(3) - Pos0.mat.GetRow(3));
				PosMat.GetRow(2) = pServerObj->GetPositionMatrix().GetRow(2);
				PosMat.RecreateMatrix(0, 2);
				PosMat.GetRow(3) = (Pos0.mat.GetRow(3) + Pos1.mat.GetRow(3)) * 0.5f;

				CWO_PhysicsPrim Prim(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32((PhysLinkSize-3.0f)*0.5f, 4.0f, 4.0f), 0.0f);
				int iNewObj = pWServer->Object_Create("Tentacle_PhysLink", PosMat, m_pObj->m_iObject);
				M_TRACE(" - %d\n", iNewObj);
				CWObject_Tentacle_PhysLink* pNewObj = safe_cast<CWObject_Tentacle_PhysLink>(pWServer->Object_Get(iNewObj));
				if (pNewObj)
				{
					if (pNewObj->InitPhysics(Prim))
					{
					/*	uint16 iChild = m_pWServer->Object_GetFirstChild(m_iObject);
						if (iChild)
						{
							int iConstraint = m_pWServer->Phys_AddBallJointConstraint(iChild, iNewObj, CVec3Dfp32(PhysLinkSize*0.5f, 0.0f, 0.0f), CVec3Dfp32(-PhysLinkSize*0.5f, 0.0f, 0.0f), 1.0f);
							M_TRACE(" - constraint: %d between %d and %d\n", iConstraint, iChild, iNewObj);
						}
						m_pWServer->Object_AddChild(m_iObject, iNewObj);*/
						if (Bones.Len() > 0)
						{
							int iLastBone = Bones[Bones.Len() - 1];
							int iConstraint = pWServer->Phys_AddBallJointConstraint(iLastBone, iNewObj, CVec3Dfp32(PhysLinkSize*0.5f, 0.0f, 0.0f), CVec3Dfp32(-PhysLinkSize*0.5f, 0.0f, 0.0f), 1.0f);
							M_TRACE(" - constraint: %d between %d and %d\n", iConstraint, iLastBone, iNewObj);
						}
						Bones.Add(iNewObj);
					}
					else
					{
						pWServer->Object_Destroy(iNewObj);
					}
				}
				PhysLength += PhysLinkSize;
				nBones++;
			}

			// too many links?
			while ((PhysLength - ArmLength) > (PhysLinkSize * 0.5f + 2.0f))
			{
				M_TRACE("Too many links (%d, %f, %f), removing one...\n", Bones.Len(), ArmLength, PhysLength); // -- m_pWServer->Object_GetNumChildren(m_iObject)
				M_ASSERT(Bones.Len(), "!");
				uint16 iChild = Bones[Bones.Len()-1];//m_pWServer->Object_GetFirstChild(m_iObject);
				//m_pWServer->Object_RemoveChild(m_iObject, iChild);
				pWServer->Object_Destroy(iChild);
				Bones.Del(Bones.Len()-1);
				M_TRACE(" - %d\n", iChild);
				PhysLength -= PhysLinkSize;
			}
		}
/*
		uint16 iChild = m_pWServer->Object_GetFirstChild(m_iObject);
		CWObject* pChild = m_pWServer->Object_Get(iChild);
		if (pChild)
		{
			fp32 Mass = pChild->GetMass();
			CVec3Dfp32 WantedPos = DemonArm.m_Pos;// - pChild->GetPositionMatrix().GetRow(0) * (PhysLinkSize * 0.5f);
			CVec3Dfp32 Force = m_Pid1.Update(pChild->GetPosition(), WantedPos, Mass);

			m_pWServer->Phys_AddForce(iChild, Force);

			while (iChild)
			{
				m_pWServer->Phys_AddForce(iChild, CVec3Dfp32(0, 0, 9.8f*Mass)); // Cancel out gravity

				uint16 iNext = m_pWServer->Object_GetNextChild(iChild);
				if (!iNext)
				{
					WantedPos = Spline.m_lPoints[1].m_Pos;// - pChild->GetPositionMatrix().GetRow(0);
					CVec3Dfp32 Force = m_Pid1.Update(pChild->GetPosition(), WantedPos, Mass);
					m_pWServer->Phys_AddForce(iChild, Force);
				}
				iChild = iNext;
				CWObject* pChild = m_pWServer->Object_Get(iChild);
			}
		}*/
		nBones = Bones.Len();
		for (uint i = 0; i < nBones; i++)
		{
			uint16 iChild = Bones[i];
			CWObject* pChild = pWServer->Object_Get(iChild);

			fp32 Mass = pChild->GetMass();
			CVec3Dfp32 Force(0, 0, 9.81f*Mass);

			if (i == (nBones - 1))
			{
				CVec3Dfp32 WantedPos = _Arm.m_Pos;
				Force += _Arm.m_PhysArm_Pid1.Update(pChild->GetPosition(), WantedPos, Mass);
			}
			else if (i == 0)
			{
				CVec3Dfp32 WantedPos = _Spline.m_lPoints[1].m_Pos;
				Force += _Arm.m_PhysArm_Pid0.Update(pChild->GetPosition(), WantedPos, Mass);
			}
			pWServer->Phys_AddForce(iChild, Force);
		}
	}
	else
	{
		uint nChildren = Bones.Len();//m_pWServer->Object_GetNumChildren(m_iObject);
		M_ASSERT(nChildren == 0, "physobjs not removed");
	}
}
#endif

CTentacleArmState& CWO_TentacleSystem_ClientData::GetRandomDemonHead(uint32 _Seed)
{
	//static int iLastDemonHead = -1;
	//iLastDemonHead = CFXSysUtil::RandExcludeSeed(2, iLastDemonHead, _Seed);
	//return m_lArmState[(iLastDemonHead == 0) ? TENTACLE_DEMONHEAD1 : TENTACLE_DEMONHEAD2];

	uint32 DemonHead = TENTACLE_DEMONHEAD1;
	if (_Seed == 0 && Random > 0.5f)
		DemonHead = TENTACLE_DEMONHEAD2;
	else if (_Seed > 0 && MFloat_GetRand(_Seed) > 0.5f)
		DemonHead = TENTACLE_DEMONHEAD2;

	return m_lArmState[DemonHead];
}


bool CWO_TentacleSystem_ClientData::SendProjectileDamage(CWorld_Server* _pWServer, const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2, const CVec3Dfp32& _Force, int32 _Damage)
{
	CCollisionInfo CInfo;
	_pWServer->Phys_IntersectLine(_Pos1, _Pos2, OBJECT_FLAGS_PROJECTILE, (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT), 
											  (XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS), &CInfo, m_iOwner);
	if (CInfo.m_bIsValid)
	{
		CWO_Damage Damage(_Damage, DAMAGETYPE_DARKNESS, 0);
		Damage.SendExt(CInfo.m_iObject, m_iOwner, _pWServer, &CInfo, &_Force, 0, 0);
		return true;
	}

	return false;
}


uint CWO_TentacleSystem_ClientData::GetBreathFlags(int32 _BreathCtrl)
{
	static int liBreathModel[9] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
	int iBreathType = CFXSysUtil::RandSeed(7, _BreathCtrl);
	int nBreathModel = iBreathType + 1 + MinMT(2, iBreathType/3);

	uint BreathFlags = 0;
	for (uint iBreathModel = iBreathType; iBreathModel < nBreathModel; iBreathModel++)
		BreathFlags |= M_BitD(liBreathModel[iBreathModel]);

	return BreathFlags;
}


void CWO_TentacleSystem_ClientData::CreateBloodEffect(CWorld_Server* _pServer, int _iObject, const CVec3Dfp32& _Pos, const SCollisionFlags* _pColFlags, bool _bDecal, const char* _pDamageEffect, const CVec3Dfp32* _pPos, const CVec3Dfp32* _pSplattDir, int8 _MinSpawn, int8 _MaxSpawn, int8 _iDecal, int8 _Size)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int bBlood = (pSys && pSys->GetOptions()) ? pSys->GetOptions()->GetValuei("GAME_BLOOD", 1) : 1;
	if (!bBlood)
		return;

	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER);
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	int32 iExclude = 0;//m_iOwner;
	CVec3Dfp32 Pos1 = _Pos;// = _Arm.m_Pos;// + (_Arm.m_Dir * -4.0f);
	CVec3Dfp32 Pos2 = _Pos;// = _Arm.m_Pos;// + (_Arm.m_Dir *  4.0f);

	if (_pColFlags)
	{
		OwnFlags = _pColFlags->m_OwnFlags;
		ObjectFlags = _pColFlags->m_ObjectFlags;
		MediumFlags = _pColFlags->m_MediumFlags;
		iExclude = _pColFlags->m_iExclude;
		Pos1 = _pColFlags->m_Pos1;
		Pos2 = _pColFlags->m_Pos2;
	}
	
//	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE | CXR_COLLISIONRETURNVALUE_LOCALPOSITION);
    //const bool bHit = pWServer->Phys_IntersectLine(Pos2, Pos1, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
	//pWServer->Debug_RenderWire(Pos1, Pos2);

	CWObject_Character* pObject = (CWObject_Character*)_pServer->Object_Get(_iObject);

	bool bHit = false;
	if (_bDecal && pObject) 
	{
		CWO_OnIntersectLineContext Ctx;
		Ctx.m_pObj = pObject;
		Ctx.m_pPhysState = _pServer;
		Ctx.m_p0 = Pos2;
		Ctx.m_p1 = Pos1;
		Ctx.m_ObjectFlags = ObjectFlags;
		Ctx.m_ObjectIntersectionFlags = 0;
		Ctx.m_MediumFlags = MediumFlags;

		bHit = CWObject_Character::OnIntersectLine(Ctx, &CInfo);
	}

	if (_bDecal && CInfo.m_bIsValid)
	{
		int8 iDecal = (_iDecal >= 0) ? _iDecal : TruncToInt(Random * 2.999f);
		CVec3Dfp32 Normal(CInfo.m_Plane.n);
		Normal.MultiplyMatrix3x3(CInfo.m_LocalNodePos);

		// TODO: Rotate normal to unanimated model space.
		Normal *= 127.0f;

		CNetMsg DecalMsg(PLAYER_NETMSG_DECAL);
		DecalMsg.AddVecInt16_Max32768(CInfo.m_LocalPos);
		DecalMsg.AddVecInt8_Max128(Normal);
		DecalMsg.AddInt8(CInfo.m_LocalNode);
		DecalMsg.AddInt8(iDecal);
		DecalMsg.AddInt8(_Size);
		DecalMsg.AddInt16(90);
		_pServer->NetMsg_SendToObject(DecalMsg, _iObject);
	}

	if (_pDamageEffect)
	{
		CMat4Dfp32 DamageEffectMatrix;
		DamageEffectMatrix.Unit();
		if (_pSplattDir)
		{
			DamageEffectMatrix.GetRow(0) = *_pSplattDir;
			DamageEffectMatrix.RecreateMatrix(0, 2);
		}

		CVec3Dfp32 Pos = (_pPos) ? *_pPos : _Pos;
//		if(_pPos)
//			_pPos->SetMatrixRow(DamageEffectMatrix, 3);
//		else
//			_Pos.SetMatrixRow(DamageEffectMatrix, 3);

		CRPG_InitParams InitParams;
		InitParams.m_pCInfo = &CInfo;
		const int nParam = 1;
		aint pParam[nParam] = { (aint)&InitParams };

		const fp32 MaxSubMin = _MaxSpawn - _MinSpawn;
        int nSpawn = _MinSpawn + TruncToInt(Random * (MaxSubMin + 0.999f));
		for (int i=0; i<nSpawn; i++)
		{
			CVec3Dfp32 BloodPos = Pos;
			BloodPos.k[0] += Random*16.0f - 8.0f;
			BloodPos.k[1] += Random*16.0f - 8.0f;
			BloodPos.SetMatrixRow(DamageEffectMatrix, 3);
			_pServer->Object_Create(_pDamageEffect, DamageEffectMatrix, _iObject, pParam, nParam);
		}
	}
}


// helper function to calculate bone position of character
bool CWO_TentacleSystem_ClientData::GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CVec3Dfp32& _Result, fp32 _IPTime)
{
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if (CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, m_pWPhysState, pSkelInstance, pSkel, Anim, _IPTime))
	{
		int iRotTrack = Abs(_RotTrack);
		const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[iRotTrack];
		_Result = pSkel->m_lNodes[iRotTrack].m_LocalCenter;
		_Result *= Mat;

		if (_RotTrack < 0)
		{
			switch (_RotTrack)
			{
				case TENTACLE_DEVOUR_ROTTRACK_HEART:
					_Result += (Mat.GetRow(2) * 7.0f) + (Mat.GetRow(1) * 3.0f) + (Mat.GetRow(0) * 2.0f);
				break;

				default:
					break;
			}
		}
		return true;
	}
	return false;
}


bool CWO_TentacleSystem_ClientData::GetCharBoneRot(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Result, fp32 _IPTime)
{
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;
	if (CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, m_pWPhysState, pSkelInstance, pSkel, Anim, _IPTime))
	{
		int iRotTrack = Abs(_RotTrack);
		_Result = pSkelInstance->m_pBoneTransform[iRotTrack];
		return true;
	}
	return false;
}


bool CWO_TentacleSystem_ClientData::GetCharBoneMat(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Result, fp32 _IPTime)
{
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;
	if (CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, m_pWPhysState, pSkelInstance, pSkel, Anim, _IPTime))
	{
		const int iRotTrack = Abs(_RotTrack);
		const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[iRotTrack];
		_Result =Mat;
		_Result.GetRow(3) = (pSkel->m_lNodes[iRotTrack].m_LocalCenter * Mat);

		// If this is a special bone, apply offset
		if (_RotTrack < 0)
		{
			switch (_RotTrack)
			{
				case TENTACLE_DEVOUR_ROTTRACK_HEART:
					_Result.GetRow(3) += (_Result.GetRow(2) * 7.0f) + (_Result.GetRow(1) * 3.0f) + (_Result.GetRow(0) * 2.0f);
				break;

				default:
					break;
			}
		}

		return true;
	}
	return false;
}


void CWO_TentacleSystem_ClientData::InitCharRagdoll(CWObject_Character* _pChar)
{
	if (_pChar->m_Flags & PLAYER_FLAGS_RAGDOLL)
	{
		/*
		CConstraintSystem* pRagdoll = _pChar->m_spRagdoll;
		if (!pRagdoll)
			pRagdoll = _pChar->m_spRagdoll = MNew(CConstraintSystem);

		CWO_Character_ClientData* pCharCD = CWObject_Character::GetClientData(_pChar);
		if (!pRagdoll->IsInited())
		{
			// To be sure we have a skeletoninstance when creating the ragdoll
			CXR_AnimState Anim;
			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;
			if (_pChar->GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
			{
				CXR_Model* lpModels[CWO_NUMMODELINDICES];
				for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
				{
					_pChar->m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
					if (_pChar->m_iModel[i] > 0)
					{
						lpModels[i] = m_pWPhysState->GetMapData()->GetResource_Model(_pChar->m_iModel[i]);
						if (lpModels[i])
							_pChar->m_RagdollSettings.m_lpClothSkeleton[i] = lpModels[i]->GetSkeleton();
					}
				}

				_pChar->m_RagdollSettings.m_pSkelInstance = pSkelInstance;
				_pChar->m_RagdollSettings.m_pSkeleton = pSkel;
				pRagdoll->SetOrgMat(_pChar->GetPositionMatrix());
				pRagdoll->Init(_pChar->m_iObject, _pChar, m_pWPhysState, pCharCD, &pCharCD->m_RagdollClient);
				pRagdoll->Setup(&_pChar->m_RagdollSettings);
				for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
					_pChar->m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
				pRagdoll->Activate(true);
				_pChar->m_RagdollSettings.m_pSkelInstance = NULL;
			}
		}
		pCharCD->m_ExtraFlags = pCharCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;
		*/
		_pChar->Char_ActivateRagdoll(CWObject_Character::GetClientData(_pChar));
	}
}


void CWO_TentacleSystem_ClientData::Server_MoveObject(CTentacleArmState& _Arm, const CVec3Dfp32& _NewPos, const CMat4Dfp32& _RefMat)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject* pTarget = pWServer->Object_Get(_Arm.m_iTarget);
	if (!pTarget)
		return;

	CWObject_Character* pCorpse = TDynamicCast<CWObject_Character>(pTarget);

#ifdef INCLUDE_OLD_RAGDOLL
	if (pCorpse && pCorpse->m_spRagdoll)
	{
		// don't allow too fast movement
		fp32 MaxSpeed = 16.0f * (20.0f * pWServer->GetGameTickTime());
		CVec3Dfp32 NewPos = _NewPos;
		CVec3Dfp32 Movement = NewPos - _Arm.m_Pos;
		if (Movement.LengthSqr() > Sqr(MaxSpeed))
		{
			Movement.SetLength(MaxSpeed);
			NewPos = _Arm.m_Pos + Movement;
		}
		CWObject_Character* pCorpse = TDynamicCast<CWObject_Character>(pTarget);
		pCorpse->m_spRagdoll->SetPendingBonepos(_Arm.m_iRotTrack, NewPos);
		//pCorpse->m_spRagdoll->SetPendingBonepos(PLAYER_ROTTRACK_SPINE, NewPos);
	}
	else 
#endif // INCLUDE_OLD_RAGDOLL

	if (pCorpse && pCorpse->m_pPhysCluster)
	{

		CWPhys_ClusterObject &PCO = pCorpse->m_pPhysCluster->m_lObjects[0];
		fp32 Mass = Min(m_GrabPower,PCO.m_pRB->m_Mass);
		//fp32 Mass = Min(pTarget->GetMass(), m_GrabPower);

		CVec3Dfp32 lGrabPos[2];
		lGrabPos[0] = _NewPos;
		lGrabPos[1] = _RefMat.GetRow(0);
		lGrabPos[1] *= 16.0f;
		lGrabPos[1] += _NewPos;

		if( _Arm.m_Grabber.m_LocalGrabPos[1].AlmostEqual(CVec3Dfp32(0),0.0001f) )
		{
			_Arm.m_Grabber.m_LocalGrabPos[0] = CVec3Dfp32(0);
			_Arm.m_Grabber.m_LocalGrabPos[1] = CVec3Dfp32(0,0,1);
			_Arm.m_Grabber.m_Pid[0].Reset(CVec3Dfp32(0));
			_Arm.m_Grabber.m_Pid[1].Reset(CVec3Dfp32(0,0,1));
		}

		CVec3Dfp32 lApplyPos[2];
		lApplyPos[0] = _Arm.m_Grabber.m_Pid[0].m_LastPos;
		lApplyPos[1] = _Arm.m_Grabber.m_Pid[1].m_LastPos;

		CVec3Dfp32 lForce[2];
		_Arm.m_Grabber.Update(PCO.m_Transform, lGrabPos, Mass, lForce);

		// Add forces to adjust position
		CVec3Dfp32 AvgForce = (lForce[0] + lForce[1]) * 0.5f;
		pWServer->Phys_AddForce(PCO.m_pRB, CVec3Dfp32(0, 0, 9.81f*pTarget->GetMass())); // Cancel out gravity

		// Hack
		// In order to dampen the effect when the ragdoll "settles", only add force to other objects
		// if avg force is less than required to accelerate the entire bode some amount
		pWServer->Phys_AddForce(PCO.m_pRB, AvgForce);
		if( AvgForce.LengthSqr() < Sqr(pTarget->GetMass() * 10.0f) )
		{
			CVec3Dfp32 Force = AvgForce * (1.0f / Mass);
			TAP<CWPhys_ClusterObject> pPCO = pCorpse->m_pPhysCluster->m_lObjects;
			for(uint i = 1; i < pPCO.Len();i++)
				pWServer->Phys_AddForce(pPCO[i].m_pRB, Force * pPCO[i].m_pRB->m_Mass);
		}

		// Add impulses to adjust rotation
		static fp32 tweak = 10.0f;						// TODO: find out what this value should be!  (the behaviour changed with new physics code...)
		lForce[0] -= AvgForce;
		lForce[1] -= AvgForce;
		pWServer->Phys_AddImpulse(PCO.m_pRB, lApplyPos[0], lForce[0] * tweak);
		pWServer->Phys_AddImpulse(PCO.m_pRB, lApplyPos[1], lForce[1] * tweak);

		// Damp velocity to get rid of spinning
		CVelocityfp32 &Vel =PCO.m_Velocity; //pTarget->GetVelocity();
		Vel.m_Rot.m_Angle *= 0.98f;
		//pWServer->Object_SetVelocity(_Arm.m_iTarget, Vel);
	}
	else if (pTarget->m_pRigidBody2)
	{
		CVec3Dfp32 lApplyPos[2];
		lApplyPos[0] = _Arm.m_Grabber.m_Pid[0].m_LastPos;
		lApplyPos[1] = _Arm.m_Grabber.m_Pid[1].m_LastPos;

		fp32 Mass = Min(pTarget->GetMass(), m_GrabPower);

		CVec3Dfp32 lGrabPos[2];
		lGrabPos[0] = _NewPos;
		lGrabPos[1] = _RefMat.GetRow(0);
		lGrabPos[1] *= 16.0f;
		lGrabPos[1] += _NewPos;

		CVec3Dfp32 lForce[2];
		_Arm.m_Grabber.Update(pTarget->GetPositionMatrix(), lGrabPos, Mass, lForce);

		// Prevent char from lifting the same object he/she's standing on
		static bool bbbb = true;
		if (bbbb && m_iOwner > 0)
		{
			CWObject* pOwner = pWServer->Object_Get(m_iOwner);
			if (pOwner && pOwner->IsClass(class_CharPlayer))
			{
				CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);
				if (_Arm.m_iTarget == pOwnerCD->m_Phys_iLastGroundObject)
				{
					lForce[0].k[2] = Min(0.0f, lForce[0].k[2]);
					lForce[1].k[2] = Min(0.0f, lForce[1].k[2]);
				}
			}
		}

		// Add forces to adjust position
		CVec3Dfp32 AvgForce = (lForce[0] + lForce[1]) * 0.5f;
		pWServer->Phys_AddForce(_Arm.m_iTarget, AvgForce);
		pWServer->Phys_AddForce(_Arm.m_iTarget, CVec3Dfp32(0, 0, 9.81f*Mass)); // Cancel out gravity

		// Add impulses to adjust rotation
		static fp32 tweak = 10.0f;						// TODO: find out what this value should be!  (the behaviour changed with new physics code...)
		lForce[0] -= AvgForce;
		lForce[1] -= AvgForce;
		pWServer->Phys_AddImpulse(_Arm.m_iTarget, lApplyPos[0], lForce[0] * tweak);
		pWServer->Phys_AddImpulse(_Arm.m_iTarget, lApplyPos[1], lForce[1] * tweak);

		// Damp velocity to get rid of spinning
		CVelocityfp32 Vel = pTarget->GetVelocity();
		Vel.m_Rot.m_Angle *= 0.98f;
		pWServer->Object_SetVelocity(_Arm.m_iTarget, Vel);
	}
}


void CWO_TentacleSystem_ClientData::Server_ThrowObject(CTentacleArmState& _Arm, const CVec3Dfp32& _Dir, fp32* _pRigidForce)
{
	CWorld_Server* pWServer = safe_cast<CWorld_Server>(m_pWPhysState);
	CWObject* pTarget = pWServer->Object_Get(_Arm.m_iTarget);
	if (!pTarget)
		return;

	CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pTarget);

#ifdef INCLUDE_OLD_RAGDOLL
	if (pChar && pChar->m_spRagdoll)
	{
		CVec3Dfp32 Impulse = _Dir;
		Impulse *= 30.0f; //tweakme
		pChar->m_spRagdoll->AddImpulse(CConstraintSystem::BODY_PART, Impulse);
	}
	else 
#endif // INCLUDE_OLD_RAGDOLL

	if (pChar && pChar->m_pPhysCluster)
	{
		CWPhys_ClusterObject &PCO = pChar->m_pPhysCluster->m_lObjects[0];
		fp32 Mass = Min(m_GrabPower,PCO.m_pRB->m_Mass);
		Mass *= (_pRigidForce) ? *_pRigidForce : 300.0f;

		CVec3Dfp32 ApplyPos = _Arm.m_Grabber.m_Pid[0].m_LastPos;
		CVec3Dfp32 Force = _Dir * Mass;

		// Add forces to adjust position
		pWServer->Phys_AddForce(PCO.m_pRB, Force);
		Force *= (1.0f / Mass);
		TAP<CWPhys_ClusterObject> pPCO = pChar->m_pPhysCluster->m_lObjects;
		for(uint i = 1; i < pPCO.Len();i++)
			pWServer->Phys_AddForce(pPCO[i].m_pRB, Force * pPCO[i].m_pRB->m_Mass);
	}
	else if (pTarget->m_pRigidBody2)
	{
		fp32 Mass = Min(pTarget->GetMass(), m_GrabPower);
		Mass *= (_pRigidForce) ? *_pRigidForce : 300.0f;

		CVec3Dfp32 ApplyPos = _Arm.m_Grabber.m_Pid[0].m_LastPos;
		CVec3Dfp32 Force = _Dir * Mass;
		pWServer->Phys_AddImpulse(_Arm.m_iTarget, ApplyPos, Force);
	}
}


/*
CXR_ModelInstance* CWO_TentacleSystem_ClientData::GetModelInstance(CXR_Model *_pModel, CTentacleArmSetup& _ArmSetup, int _iModelSlot)
{
	CXR_ModelInstance* pInstance = _ArmSetup.m_BreathModels.m_lspModelInstances[_iModelSlot];
	if (!pInstance)
	{
		// Create model instance
		TPtr<CXR_ModelInstance> spModelInstance = _pModel->CreateModelInstance();
		if (spModelInstance)
		{
			CWorld_Client* pWClient = safe_cast<CWorld_Client>(m_pWPhysState);
			spModelInstance->Create(_pModel, CXR_ModelInstanceContext(pWClient->GetGameTick(), pWClient->GetGameTickTime(), m_pObj, pWClient));
		}
		_ArmSetup.m_BreathModels.m_lspModelInstances[_iModelSlot] = spModelInstance;
		pInstance = spModelInstance;
	}
	return pInstance;
}
*/


static bool GetRotationMatrixToAlignVectors(const CVec3Dfp32& _From, const CVec3Dfp32& _To, CMat4Dfp32& _Result)
{
	CVec3Dfp32 bisec = _From + _To;
	fp32 Len = bisec.Length();
	if (Len < 0.01f)
		return false; // 180 degrees apart

	// ok, there is one axis to rotate around
	bisec *= (1.0f / Len);
	fp32 CosHalfAngle = _From * bisec;
	if (CosHalfAngle > 0.999f)
		return false; // no difference, no need to calculate stuff

	// calculate axis and angle
	CAxisRotfp32 tmp;
	_From.CrossProd(bisec, tmp.m_Axis);
	tmp.m_Axis.Normalize();
	tmp.m_Angle = -M_ACos(CosHalfAngle) * (1.0f/_PI);
	tmp.CreateMatrix(_Result);
	return true;
}


bool CWO_TentacleSystem_ClientData::CanStartCreepingDark(uint8 _iArm, const CVec3Dfp32& _StartPos)
{
	// Get some info about the owner object, if any
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	if (pOwner && (pOwner->IsClass(class_CharPlayer) || pOwner->IsClass(class_CharNPC)))
	{
		fp32 SphereRadius = 14.0f;
		fp32 OffsetForward = 24.0f;
		fp32 OffsetUp = SphereRadius + 0.5f;
		int ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_OBJECT;

		// Get camera matrix
		CMat4Dfp32 EndMat;
		GetCharBoneMat(*pOwner, PLAYER_ROTTRACK_CAMERA, EndMat, 0.0f);
		EndMat.GetRow(2) = CVec3Dfp32(0.0f, 0.0f, 1.0f);
		EndMat.RecreateMatrix(2, 0);

		// Offset matrix
		CVec3Dfp32 EndPos = pOwner->GetPosition() + (EndMat.GetRow(0) * OffsetForward) + (EndMat.GetRow(2) * OffsetUp);
		EndMat.GetRow(3) = EndPos;
		
		// Create a selection
		TSelection<CSelection::MEDIUM_BUFFER> Selection;
		m_pWPhysState->Selection_AddBoundSphere(Selection, ObjectFlags, EndPos, SphereRadius);
		m_pWPhysState->Selection_RemoveOnIndex(Selection, m_iOwner);
		
		// Create a temporary phys to work with
		CWO_PhysicsState TmpPhys = m_pObj->GetPhysState();
		TmpPhys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, SphereRadius, 0.0f);
		TmpPhys.m_nPrim = 1;
		TmpPhys.m_ObjectIntersectFlags = ObjectFlags;
		TmpPhys.m_ObjectFlags = OBJECT_FLAGS_CREEPING_DARK;

		// Run collision on selection
		CCollisionInfo CInfo;
		bool bHit = m_pWPhysState->Phys_IntersectWorld(&Selection, TmpPhys, EndMat, EndMat, m_pObj->m_iObject, &CInfo);
		if (bHit && m_pWPhysState->IsServer())
		{
			if(CInfo.m_bIsValid && CInfo.m_iObject)
			{
				CWObject_CoreData* pObj = m_pWPhysState->Object_GetCD(CInfo.m_iObject);
				CWO_PhysicsState PhysState = pObj->GetPhysState();
				if(PhysState.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
				{
					CWorld_Server *pServer = safe_cast<CWorld_Server>(m_pWPhysState);
					pServer->Phys_AddMassInvariantImpulse(CInfo.m_iObject, pServer->Object_GetPosition(CInfo.m_iObject), EndMat.GetRow(0) * 15.0f);
					return true;
				}
				else
					return false;
			}
			// Something is in the way and we can't start creeping dark at this position, so return false
			return false;
		}

		// No collision, we can start creepingdark here
		return true;
	}

	return false;
}


void CWO_TentacleSystem_ClientData::UpdateDemonHeadCollisions(uint _iArm, const CVec3Dfp32& _AttachPos, const CMat4Dfp32& _EndPos)
{
	CTentacleArmState& Arm = m_lArmState[_iArm];

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_TIME);
	CVec3Dfp32 End = _EndPos.GetRow(3);
	End += _EndPos.GetRow(1) * (_iArm == TENTACLE_DEMONHEAD1 ? -7.0f : 7.0f);
	End += _EndPos.GetRow(0) * 5.0f;

	// predict player movement?
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	if (pOwner && pOwner->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
	{
		CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);

		if (pOwnerCD->m_Control_DeltaLook.k[2] > 0.02f)
		{
			End -= _EndPos.GetRow(1) * 5.0f;
			End += _EndPos.GetRow(0) * -20.0f;
		}
		else if (pOwnerCD->m_Control_DeltaLook.k[2] < -0.02f)
		{
			End += _EndPos.GetRow(1) * 5.0f;
			End += _EndPos.GetRow(0) * -20.0f;
		}
	}

	CVec2Dfp32& Retract = m_lArmRetract[_iArm];
	fp32 CurrRetract = Retract.k[1];
	fp32 LastRetract = CurrRetract;

	uint ObjFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_CHARACTER;
	uint Mediums = XW_MEDIUM_GLASS | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_SOLID | XW_MEDIUM_DYNAMICSSOLID;
	if (m_pWPhysState->Phys_IntersectLine(_AttachPos, End, 0, ObjFlags, Mediums, &CInfo, m_iOwner))
	{
		// reset boredom
		m_CurrentBoredom = 0;

		if (CInfo.m_bIsValid)
		{
			// update retract value
			fp32 NewRetract = 1.0f - CInfo.m_Time;
			NewRetract = Clamp(NewRetract, CurrRetract - 0.1f, CurrRetract + 0.1f); // don't go too fast
			CurrRetract = Clamp01(NewRetract);										// don't go outside range
		}
	}
	else
	{
		// fade back to normal
		if (CurrRetract > 0.0f)
			CurrRetract = Max(0.0f, CurrRetract - 0.1f); 
	}

	// store values
	if (LastRetract != Retract.k[0] || CurrRetract != Retract.k[1])
	{
		Retract.k[0] = LastRetract;
		Retract.k[1] = CurrRetract;
		m_lArmRetract.MakeDirty();
	}
}


fp32 CWO_TentacleSystem_ClientData::GetDevourBlendTime(uint8 _iAGToken, fp32 _IPTime, uint _ReturnType)
{
	// Get time
	const CMTime StateTime = m_AnimGraph.GetAG2I()->GetTokenFromID(_iAGToken)->GetEnterStateTime();
	const fp32 Time = (m_pWPhysState->GetGameTime() - StateTime).GetTime() + (m_pWPhysState->GetGameTickTime() * _IPTime);
	const int32 DevourVariation = m_AnimGraph.GetPropertyInt(TENTACLE_AG2_PROPERTY_INT_DEVOURTARGET);

	// Handle diffrent devour animation hit body and return times ?
#if DEVOUR_BLEND_TIME_DEBUG
	static bool bUseStatic = true;
	static fp32 sPreHold = 0.35f;
	static fp32 sFwdTime = 0.45f;
	static fp32 sHldTime = 0.95f;
	static fp32 sOfsTime = 0.65f;
	static fp32 sHrtTime = 2.80f;
	static fp32 sRwdSpeed = 0.5f;
#endif

	fp32 PreHold = 0.0f;
	fp32 FwdTime = 0.0f;
	fp32 HldTime = 0.0f;
	fp32 OfsTime = 0.0f;
	fp32 HrtTime = 0.0f;
	fp32 RwdSpeed = 1.0f;
	switch (DevourVariation)
	{
	case TENTACLE_AG2_DEVOURTARGET_NORMAL_VARIATION_1:
		{
			PreHold = 0.35f;
			FwdTime = 0.45f;
			HldTime = 0.95f;
			OfsTime = 0.65f;
			HrtTime = 2.80f;
			RwdSpeed = 0.5f;
			break;
		}
	}

#if DEVOUR_BLEND_TIME_DEBUG
	if (bUseStatic)
	{
		PreHold = sPreHold;
		FwdTime = sFwdTime;
		HldTime = sHldTime;
		OfsTime = sOfsTime;
		RwdSpeed = sRwdSpeed;
		HrtTime = sHrtTime;
	}
#endif

	fp32 FwdRwdRcp = 1.0f / (FwdTime - PreHold);

	switch (_ReturnType)
	{
	case TENTACLE_DEVOURBLEND_TIME_DEFAULT:
	case TENTACLE_DEVOURBLEND_TIME_FORWARD_BACKWARD:
		{
			// 0-1 while going to heart and 1-0 when returning after hold is finished
			fp32 ExtrudeFwd = Clamp01((Time - PreHold) * FwdRwdRcp);
			fp32 ExtrudeFwdRwd = ExtrudeFwd - Clamp01((Time - HldTime) * (FwdRwdRcp * RwdSpeed));
			return Clamp01(ExtrudeFwdRwd);
		}

	case TENTACLE_DEVOURBLEND_TIME_HEART:
		{
			// Return 0-1 time to blend heart from body to mouth
			//return Clamp01((Time - FwdTime) * (1.0f / (OfsTime - FwdTime)));
			fp32 HeartTimeBgn = Clamp01((Time - PreHold) * FwdRwdRcp);
			fp32 HeartTimeBgnEnd = HeartTimeBgn - Clamp01((Time - HrtTime) * (FwdRwdRcp * RwdSpeed));
			return Clamp01(HeartTimeBgnEnd);
		}

	case TENTACLE_DEVOURBLEND_TIME_HEADHEARTOFFSET:
		{
			// Blending for offset position to match heart position
			fp32 OfsOutRcp = 1.0f / (OfsTime - FwdTime);
			fp32 OffsetIn = Clamp01((Time - PreHold) * FwdRwdRcp);
			fp32 OffsetInOut = OffsetIn - Clamp01((Time - OfsTime) * OfsOutRcp);
			return Clamp01(OffsetInOut);
		}

	case TENTACLE_DEVOURBLEND_TIME_HOLDING:
		{
			// Return 0-1 time while head is holding for return
			return Clamp01((Time - FwdTime) * (1.0f / (HldTime - FwdTime)));
		}
	}

	// If we have specified an invalid mode, just return 0
	return 0.0f;

	// Return 0-1 time while head is holding for return
	if (_ReturnType == TENTACLE_DEVOURBLEND_TIME_HOLDING)
		return Clamp01((Time - FwdTime) * (1.0f / (HldTime - FwdTime)));

	fp32 ExtrudeFwd = Clamp01((Time - PreHold) * FwdRwdRcp);
	fp32 ExtrudeFwdRwd = ExtrudeFwd - Clamp01((Time - HldTime) * (FwdRwdRcp * RwdSpeed));
	return Clamp01(ExtrudeFwdRwd);
}


void CWO_TentacleSystem_ClientData::GetSpline(uint8 _iArm, CSpline_Tentacle& _Spline, bool _bRendering, CMat4Dfp32* _pLastAttachMat)
{
	CWorld_Client* pWClient = _bRendering ? safe_cast<CWorld_Client>(m_pWPhysState) : NULL;
	fp32 IPTime = pWClient ? pWClient->GetRenderTickFrac() : 1.0f;

	CMat4Dfp32 BasePos;

	// Get some info about the owner object, if any
	CWObject_CoreData* pOwner = m_pWPhysState->Object_GetCD(m_iOwner);
	CWO_Character_ClientData* pOwnerCD = NULL;
	fp32 OwnerIPTime = 1.0f;
	if (pOwner && _bRendering)
	{
		CWObject_Client* pOwnerClientObj = safe_cast<CWObject_Client>(pOwner);
		if (pOwnerClientObj->GetNext())
		{
			pOwner = CWObject_Player::Player_GetLastValidPrediction(pOwnerClientObj);
			BasePos = pOwner->GetPositionMatrix();
			pOwnerCD = CWObject_Character::GetClientData(pOwner);
			OwnerIPTime = pOwnerCD->m_PredictFrameFrac;
		}
		else
		{
			pOwnerCD = CWObject_Character::GetClientData(pOwner);
			OwnerIPTime = pWClient->GetRenderTickFrac();
			Interpolate2(pOwner->GetLastPositionMatrix(), pOwner->GetPositionMatrix(), BasePos, OwnerIPTime);
		}
	}
	else
		BasePos = m_pObj->GetPositionMatrix();


	fp32 dt = 0.0f;
	if (_bRendering)
		dt = (pWClient->GetRenderTime() - m_LastRender).GetTime() / m_pWPhysState->GetTimeScale();

	bool bCreepingDark = false;

	// Find step up, (Needed since camera matrix is a little bit of a hack in here!)
	fp32 StepUp = 0.0f;
	if (_bRendering && pOwnerCD)
		StepUp = LERP(pOwnerCD->m_LastStepUpDamper, pOwnerCD->m_StepUpDamper, OwnerIPTime);

	CMat4Dfp32 CameraMat;
	CVec3Dfp32 CameraDiff;

	CXR_Skeleton* pOwnerSkeleton = NULL;
	CXR_SkeletonInstance* pOwnerSkelInstance = NULL;
	CWObject_CoreData* pCreepingDark = NULL;
	if (pOwner)
	{
		CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);
		CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(pOwner->m_iModel[0]);
		if (pOwnerCD && pModel)
		{
			CXR_AnimState Anim;
			fp32 IPTime = OwnerIPTime;

			if (m_pWPhysState->IsServer())
				IPTime = 0.0f;

			CWO_Character_ClientData* pCDFirst = pOwnerCD;
			CWObject_Character::GetEvaluatedPhysSkeleton(pOwner, m_pWPhysState, pOwnerSkelInstance, pOwnerSkeleton, Anim, IPTime, &BasePos);

			int iUseRotTrack = (pOwnerCD->m_iPlayer != -1) ? PLAYER_ROTTRACK_CAMERA : PLAYER_ROTTRACK_HEAD;

			CameraMat = pOwnerSkelInstance->m_pBoneTransform[iUseRotTrack];
			CameraDiff = 0;

			bool bCutSceneView = (pOwner->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pOwnerCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;

			if (_bRendering && !bThirdPerson && !bCutSceneView)
			{
				if (pOwnerCD->m_iPlayer == -1)
				{
					CVec3Dfp32 x = pOwnerSkeleton->m_lNodes[iUseRotTrack].m_LocalCenter;
					CameraMat.GetRow(3) = x * CameraMat;
				}
				else
				{
					CWObject_CoreData* pObjFirst = ((CWorld_Client *)m_pWPhysState)->Object_GetFirstCopy(m_iOwner);
					if (pObjFirst)
						pCDFirst = CWObject_Character::GetClientData(pObjFirst);

					CVec3Dfp32 x = pOwnerSkeleton->m_lNodes[iUseRotTrack].m_LocalCenter;
					x.MultiplyMatrix3x3(CameraMat);
					CameraDiff = CameraMat.GetRow(3);
					CameraMat.GetRow(3) = pCDFirst->m_Camera_LastPosition - x;
					CameraDiff = CameraMat.GetRow(3) - CameraDiff;
					CameraDiff.k[2] += StepUp;
				}
			}
		}
		if (pOwnerCD && pOwnerCD->m_iCreepingDark)
			pCreepingDark = m_pWPhysState->Object_GetCD(pOwnerCD->m_iCreepingDark);
	}
	const CTentacleArmSetup& ArmSetup = m_lArmSetup[_iArm];
	      CTentacleArmState& ArmState = m_lArmState[_iArm];

	// Calculate attach positions
	CMat4Dfp32 AttachMat[5];
	const uint piAttach[4] = { 0, 1, 2, 3 };
	const fp32 pTanLen[4] = { 4.0f, 2.0f, 1.0f, 1.0f };
	uint nAttach = (_iArm == TENTACLE_SCREENSTUFF || _iArm == TENTACLE_CREEPINGDARK_SQUID) ? 1 : (ArmState.m_State == TENTACLE_STATE_TRAIL) ? 3 : 2;
	for (uint i = 0; i < nAttach; i++)
	{
		uint iAttach = piAttach[i];

		if (pOwnerSkeleton && pOwnerSkelInstance)
		{
			int iRotTrack = ArmSetup.m_lAttachPoints[iAttach].m_iNode;
			const CMat4Dfp32& BoneTransform = pOwnerSkelInstance->m_pBoneTransform[iRotTrack];
			if (iRotTrack == PLAYER_ROTTRACK_CAMERA)
			{
				// Special hack for camera track
				CMat4Dfp32 Mat = ArmSetup.m_lAttachPoints[iAttach].m_LocalPos;
			//	Mat.k[3][0] -= m_Retract;
				Mat.Multiply(CameraMat, AttachMat[i]);
			}
			else if (_iArm == TENTACLE_DEMONARM && i == 1)
			{	//
				// Special for demon arm attach1  
				//  1) I want the position and rotation to match the camera matrix (with prediction correction and stuff)
				//  2) I want the attachpoint z-pos (relative root) to be at the specified height, regardless of rotation
				//  3) I want the rotation to never point downwards (clamp row0.k[2] to 0.0f)
				//
				// TODO: generalize this?
				//
				CMat4Dfp32 Tmp = CameraMat;

				CVec3Dfp32 Pos = ArmSetup.m_lAttachPoints[iAttach].m_LocalPos.GetRow(3);
				fp32 z = Pos.k[2];
				Pos *= CameraMat;
				Pos.k[2] = z + BasePos.GetRow(3).k[2] + CameraDiff.k[2];
				Tmp.GetRow(3) = Pos;

				if (Tmp.GetRow(0).k[2] < 0.0f)
				{
					Tmp.GetRow(0).k[2] = 0.0f;
					Tmp.RecreateMatrix(1, 0);
				}
				AttachMat[i] = Tmp;
			}
			else
			{
				ArmSetup.m_lAttachPoints[iAttach].m_LocalPos.Multiply(BoneTransform, AttachMat[i]);
			}

			// Stupid safety code to fix bad BoneTransforms
			if (!IsValidFloat(AttachMat[i].k[0][0]))
				ArmSetup.m_lAttachPoints[iAttach].m_LocalPos.Multiply(BasePos, AttachMat[i]);
		}
		else
		{
			ArmSetup.m_lAttachPoints[iAttach].m_LocalPos.Multiply(BasePos, AttachMat[i]);
		}

		// Add point to spline
		_Spline.AddPoint(AttachMat[i], AttachMat[i].GetRow(0) * pTanLen[i]);
		m_pWPhysState->Debug_RenderMatrix(AttachMat[i], 0.0f, false);
	}

	// Add Creeping Dark points
	if (pCreepingDark && ArmState.m_State == TENTACLE_STATE_TRAIL)
	{
		bCreepingDark = true;
		CWObject_CreepingDark::CCreepingDarkClientData* pCreepData = CWObject_CreepingDark::GetCreepingDarkClientData(pCreepingDark);

		const CMat4Dfp32& CreepCam = pCreepData->m_LastCamera;
		AttachMat[nAttach] = CreepCam;
		fp32 d = Clamp01(CreepCam.GetRow(0) * pCreepData->m_Gravity);
		AttachMat[nAttach].GetRow(3) -= CreepCam.GetRow(0)*(8.0f + 8.0f*d);
		AttachMat[nAttach].GetRow(3) -= CreepCam.GetRow(1)*4.0f;
		AttachMat[nAttach].GetRow(3) -= CreepCam.GetRow(2)*3.0f;
		_Spline.AddPoint(AttachMat[nAttach], CreepCam.GetRow(0));
		nAttach++;
	}

	// Calculate target pos (if any)
	CVec3Dfp32 TargetPos = ArmState.m_TargetPos;
	if (ArmState.m_iTarget)
	{
		CWObject_CoreData* pTarget = m_pWPhysState->Object_GetCD(ArmState.m_iTarget);
		if (pTarget)
		{
			pTarget->GetLastPosition().Lerp( pTarget->GetPosition(), IPTime, TargetPos);

//			const CWO_PhysicsState& Phys = pTarget->GetPhysState();
			if (pTarget->IsClass(class_CharNPC))
			{
				GetCharBonePos(*pTarget, ArmState.m_iRotTrack, TargetPos, IPTime);
			}
			else
			{
				CVec3Dfp32 Offset = ArmState.m_GrabPoint;
				Offset.MultiplyMatrix3x3(pTarget->GetPositionMatrix());
				TargetPos += Offset;
			}
		}
	}

	CMat4Dfp32 LastAttach = AttachMat[nAttach-1];
	if (_pLastAttachMat)
		*_pLastAttachMat = LastAttach;

	if (ArmState.IsTask_DevourTarget() && TargetPos != CVec3Dfp32(0.0f))
	{
		// Add point for devour so we can send arm down
		CWObject_CoreData* pTarget = m_pWPhysState->Object_GetCD(ArmState.m_iTarget);
		fp32 DevourBlendTime = GetDevourBlendTime(ArmSetup.m_iAGToken, IPTime);
		if (pTarget && !AlmostEqual(DevourBlendTime, 0.0f, 0.0001f))
		{
			CMat4Dfp32 HeartMat;
			GetCharBoneMat(*pTarget, TENTACLE_DEVOUR_ROTTRACK_HEART, HeartMat, IPTime);
			
			// Modify previous point to match heart height position
			const CSpline_Tentacle::Point& p = _Spline.m_lPoints[_Spline.m_lPoints.Len() - 1];

			CMat4Dfp32 Mat4 = LastAttach;
			CVec3Dfp32 PosTemp = LastAttach.GetRow(3);
			fp32 Extrude = ArmSetup.m_Extrude;
			
			// Find bottom position and direction from bottom pos to heart
			PosTemp.k[2] = HeartMat.GetRow(3).k[2];
			CVec3Dfp32 HeadToHeart = HeartMat.GetRow(3) - PosTemp;
			CVec3Dfp32 HeadToHeartN = HeadToHeart;
			HeadToHeartN.Normalize();

			// Set last position so extrusion will be placed in the heart
			fp32 DevourOffset = GetDevourBlendTime(ArmSetup.m_iAGToken, IPTime, TENTACLE_DEVOURBLEND_TIME_HEADHEARTOFFSET);
			Mat4.GetRow(0) = CFXSysUtil::LerpMT(LastAttach.GetRow(0), HeadToHeartN, DevourBlendTime);
			Mat4.GetRow(2) = CVec3Dfp32(0.0f, 0.0f, 1.0f);
			Mat4.RecreateMatrix(0, 2);
			Mat4.GetRow(3) = CFXSysUtil::LerpMT(p.m_Pos, HeartMat.GetRow(3) - (HeadToHeartN * Extrude), DevourBlendTime);
			Mat4.GetRow(3).k[2] += CFXSysUtil::LerpMT(8.0f, 0.0f, (DevourOffset * DevourBlendTime));
			LastAttach = Mat4;
			
			// Add two more points that we can re-configure acording to animation when rendering
			Mat4.GetRow(3) -= (Mat4.GetRow(0) * 6.0f);
			_Spline.AddPoint(Mat4, Mat4.GetRow(0));

			Mat4.GetRow(3) += (Mat4.GetRow(0) * 3.0f);
			_Spline.AddPoint(Mat4, Mat4.GetRow(0));

			Mat4 = LastAttach;
			_Spline.AddPoint(Mat4, Mat4.GetRow(0));
		}

		// Clear target pos
		TargetPos = 0.0f;
	}

	if (TargetPos != CVec3Dfp32(0.0f) && !ArmState.IsDevouring())
	{
		CMat4Dfp32 Mat;

		if (ArmState.m_State == TENTACLE_STATE_INTERESTED)
		{
			// Setup blending positions
			const CVec3Dfp32& AttachPos = LastAttach.GetRow(3);
			if (ArmState.m_LastTargetPos == CVec3Dfp32(0) && _bRendering)
			{
				const CVec3Dfp32& AttachRow = LastAttach.GetRow(0);
				ArmState.m_LastTargetPos = AttachPos + AttachRow * ArmSetup.m_Extrude;
				ArmState.m_CurrTargetPos = AttachRow;
			}

			// Blend from current position to wanted
			if (_bRendering)
			{
				fp32 k = Clamp01(dt * 4.0f);
				ArmState.m_LastTargetPos.Lerp(TargetPos, k, ArmState.m_LastTargetPos);
				ArmState.m_CurrTargetPos.Lerp((ArmState.m_LastTargetPos - AttachPos).Normalize(), k, ArmState.m_CurrTargetPos);
				ArmState.m_CurrTargetPos.Normalize();
			}

			Mat.GetRow(0) = ArmState.m_CurrTargetPos;
			Mat.GetRow(2) = LastAttach.GetRow(2);
			Mat.RecreateMatrix(0, 2);
			Mat.GetRow(3) = AttachPos + Mat.GetRow(0) * ArmSetup.m_Extrude;
			LastAttach = Mat;
			_Spline.AddPoint(Mat, Mat.GetRow(0) * 3.0f);

			// adjust tangent of "attach" point when looking at something
			CSpline_Vec3Dfp32::Point& p = _Spline.m_lPoints[_Spline.m_lPoints.Len() - 2];
			p.m_Tangent = (Mat.GetRow(3) - p.m_Pos).SetLength( p.m_Tangent.Length() );
		}
		else
		{ // move to target
			Mat.GetRow(0) = (TargetPos - LastAttach.GetRow(3)).Normalize();
			CMat4Dfp32 DeltaMat;
			if (GetRotationMatrixToAlignVectors(LastAttach.GetRow(0), Mat.GetRow(0), DeltaMat))	// Create a good end matrix
			{
				LastAttach.Multiply3x3(DeltaMat, Mat);
			}
			else
			{
				Mat.GetRow(2) = LastAttach.GetRow(2);
				Mat.RecreateMatrix(0, 2);
			}
			Mat.GetRow(3) = TargetPos;
			LastAttach = Mat;
			_Spline.AddPoint(Mat, Mat.GetRow(0));
		}
	}
	else if (ArmSetup.m_Extrude != 0.0f)
	{
		CMat4Dfp32 Mat = LastAttach;
		Mat.GetRow(3) += Mat.GetRow(0) * ArmSetup.m_Extrude;

		// Smooth the end position
		if (_bRendering && !bCreepingDark)
		{
			CVec3Dfp32& Cache = ArmState.m_Attach1_Cache.GetRow(3);
			CVec3Dfp32& Pos = Mat.GetRow(3);

			CVec3Dfp32 BaseOffset;
			CameraMat.GetRow(3).Lerp(BasePos.GetRow(3) + CameraDiff, 0.25f, BaseOffset);
			CVec3Dfp32 LocalPos = Pos - BaseOffset;

			fp32 k = dt * 1.0f;		// 6.0f
			fp32 k2 = dt * 12.0f;	// 6.0f

			//fp32 d = Diff.Length();
			//k *= (1.0f + Clamp01(d * 0.001f));
			CVec3Dfp32 Diff = LocalPos - Cache;
			k = Clamp01(k);
			k2 = Clamp01(k2);

			fp32 OldLen = Cache.Length();
			fp32 NewLen = LocalPos.Length();
			fp32 Len = OldLen + (NewLen - OldLen) * k;
			Cache += Diff * k2;
			Cache.SetLength(Len);

			Pos = Cache + BaseOffset;
			M_ASSERT(IsValidVec(Pos), "!");

			if (ArmState.m_LastTargetPos != CVec3Dfp32(0))
			{
				ArmState.m_LastTargetPos.Lerp(Pos, Clamp01(k * 4.0f), ArmState.m_LastTargetPos);
				if (ArmState.m_LastTargetPos.AlmostEqual(Pos, 0.05f))
				{
					ArmState.m_LastTargetPos = 0;
					ArmState.m_CurrTargetPos = 0;
				}
				else
				{
					ArmState.m_CurrTargetPos.Lerp(LastAttach.GetRow(0), Clamp01(k * 4.0f), ArmState.m_CurrTargetPos);
					ArmState.m_CurrTargetPos.Normalize();
					Mat.GetRow(0) = ArmState.m_CurrTargetPos;
					Mat.RecreateMatrix(0, 2);
					Mat.GetRow(3) = LastAttach.GetRow(3) + Mat.GetRow(0) * ArmSetup.m_Extrude;//Pos;
				}
			}
		}

		if ((_iArm == TENTACLE_DEMONHEAD1 || _iArm == TENTACLE_DEMONHEAD2) && !ArmState.IsCreepingDark())
		{
			// detect collision and retract heads if needed
			if (m_pWPhysState->IsServer())
			{
				UpdateDemonHeadCollisions(_iArm, LastAttach.GetRow(3), Mat);
			}
			else if (_bRendering)
			{
				fp32 Retract = LERP(m_lArmRetract[_iArm].k[0], m_lArmRetract[_iArm].k[1], IPTime);
				if (Retract > 0.0f)
					Mat.GetRow(3).Lerp(LastAttach.GetRow(3), Retract, Mat.GetRow(3));
			}
		}
		LastAttach = Mat;
		_Spline.AddPoint(Mat, Mat.GetRow(0));
		//m_pWPhysState->Debug_RenderMatrix(Mat, 0.0f, false);
	}

	// make a smooth blend between different attach directions (it's changed when looking at something interesting)
	if (_bRendering && (_iArm == TENTACLE_DEMONHEAD1 || _iArm == TENTACLE_DEMONHEAD2) && _Spline.m_lPoints.Len() > 1)
	{
		CVec3Dfp32& SplineDir = _Spline.m_lPoints[1].m_Tangent;
		if (ArmState.m_CurrAttachDir.LengthSqr() > 0.01f)
			ArmState.m_CurrAttachDir.Lerp(SplineDir, Clamp01(dt * 0.6f), SplineDir);

		ArmState.m_CurrAttachDir = SplineDir;
	}

	uint nCachePoints = _bRendering ? CSpline_Vec3Dfp32::Segment::MaxCachePoints : (CSpline_Vec3Dfp32::Segment::MaxCachePoints / 2);
	_Spline.Finalize(nCachePoints);
}





/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Tentacle_PhysLink
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Tentacle_PhysLink, CWObject, 0x0100);


bool CWObject_Tentacle_PhysLink::InitPhysics(const CWO_PhysicsPrim& _PhysPrim)
{
	// Turn off OnRefresh() and OnClientRefresh()
	// To get proper client interpolation, the owning object must call UpdateLastPos()
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;


	CWO_PhysicsState Phys;
	Phys.m_ObjectFlags = 0;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL;
	Phys.m_PhysFlags = OBJECT_PHYSFLAGS_PHYSICSCONTROLLED | OBJECT_PHYSFLAGS_PHYSMOVEMENT
	                 | OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_OFFSET | OBJECT_PHYSFLAGS_ROTATION;

	Phys.AddPhysicsPrim(0, _PhysPrim);

	if (m_pWServer->Object_SetPhysics(m_iObject, Phys))
	{
		m_pWServer->Object_InitRigidBody(m_iObject, false);
		m_pWServer->Phys_SetStationary(m_iObject, false);
		return true;
	}

	M_TRACE(" - Error setting physics for object %d\n", m_iObject);
	return false;
}


void CWObject_Tentacle_PhysLink::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _RenderMat)
{
	CVec3Dfp32 Dim = _pObj->GetPhysState().m_Prim[0].GetDim();
	const CMat4Dfp32& PosMat = _pObj->GetPositionMatrix();
	_pWClient->Debug_RenderOBB(PosMat, Dim, 0xffffffff, 1.0f, true);
}

