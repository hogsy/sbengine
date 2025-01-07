#include "PCH.h"
#if 0 // LEGACY
#include "WModel_Tentacles.h"
#include "../../../../Shared/MOS/XR/XRVBContext.h"

#define sizeof_buffer(buf) (sizeof(buf)/sizeof((buf)[0]))

#ifdef M_RTM
# define DEBUG_WIRE(v0, v1, color)
#else
# define DEBUG_WIRE(v0, v1, color) if (pWC) pWC->RenderWire(v0, v1, color, 0.2f, false)
#endif

typedef CTentaclesData::SegTemp::SplinePoint SplinePoint;

#define VERSION1

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Tentacles, CXR_Model_Custom);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Misc functions...
|__________________________________________________________________________________________________
\*************************************************************************************************/
void Quat_RotateZ(const CQuatfp32& q, CVec3Dfp32& dest) // rotates Vec3::ez
{
	fp32 s = 2.0f;	// assuming normalized quat
	fp32 xs = q.k[0] * s;  
	fp32 ys = q.k[1] * s;  
	fp32 zs = q.k[2] * s;
	fp32 xz = q.k[0] * zs;
	fp32 wy = q.k[3] * ys; 
	fp32 yz = q.k[1] * zs; 
	fp32 wx = q.k[3] * xs; 
	fp32 xx = q.k[0] * xs; 
	fp32 yy = q.k[1] * ys; 

	dest.k[0] = xz - wy;
	dest.k[1] = yz + wx;
	dest.k[2] = 1.0f - (xx+yy);
}

static void Quat_Align(CQuatfp32& q, const CVec3Dfp32& v1, const CVec3Dfp32& v2)
{
	// assuming v1, v2 normalized
	CVec3Dfp32 bisec = (v1 + v2).Normalize();
	fp32 cos_halfangle = v1 * bisec;		// dot product
	if (cos_halfangle == 0.0f)
	{
		q.k[3] = 1.0f;
		q.k[0] = q.k[1] = q.k[2] = 0.0f;
	}
	else
	{
		CVec3Dfp32 cross; 
		bisec.CrossProd(v1, cross);
	    q.k[3] = cos_halfangle;
		q.k[0] = cross.k[0];	// = axis.x * sin(halfangle)
		q.k[1] = cross.k[1];	// = axis.y * sin(halfangle)
		q.k[2] = cross.k[2];	// = axis.z * sin(halfangle)
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentaclesData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTentaclesData::CTentaclesData()
	: m_bNeedRefresh(false)
	, m_nSegs(0)
{
}


void CTentaclesData::Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	m_bNeedRefresh = true;
}


void CTentaclesData::OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat43fp32* _pMat, int _nMat, int _Flags)
{
	CWObject_CoreData* pObj = safe_cast<CWObject_CoreData>((CReferenceCount*)_Context.m_pObj);

	// set dummy data for free tentacles
	const CMat43fp32& Pos = pObj->GetPositionMatrix();
	m_Segs[0].m_Start.Pos = CVec3Dfp32::GetRow(Pos, 3);
	m_Segs[0].m_Start.Rot.Create(Pos);
	m_Segs[0].m_Start.Size = 1.0f;
	m_Segs[0].m_Start.Wiggle = 0.1f;
	m_Segs[0].m_End.Size = 0.1f;
	m_Segs[0].m_End.Wiggle = 0.1f;
	m_Segs[0].m_bFree = true;
	m_nSegs = 1;

	m_bNeedRefresh = false;
}


bool CTentaclesData::NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	return m_bNeedRefresh;
}


TPtr<CXR_ModelInstance> CTentaclesData::Duplicate() const
{
	CXR_ModelInstance* pNew = MNew(CTentaclesData);
	*pNew = *this;
	return TPtr<CXR_ModelInstance>(pNew);
}

void CTentaclesData::operator=(const CXR_ModelInstance& _Instance)
{
	const CTentaclesData& From = *safe_cast<const CTentaclesData>(&_Instance);
	*this = From;

}

void CTentaclesData::operator= (const CTentaclesData &_Instance)
{
	memcpy(m_Segs, _Instance.m_Segs, sizeof(_Instance.m_Segs));
	m_nSegs = _Instance.m_nSegs;
	m_bNeedRefresh = _Instance.m_bNeedRefresh;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Tentacles
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Model_Tentacles::SetTemplateData(uint8 _Index, spCXR_Model_TriangleMesh _spMesh)
{
	if (_Index >= MaxTemplates)
		Error("CXR_Model_Tentacles::SetTemplateData", "template index out of range!");

	if (_spMesh)
		SnapMeshData(*_spMesh);

	m_lspTemplates[_Index] = _spMesh;
}


//
//TODO: This doesn't really belong here.
//      Could perhaps be done by a XWC-script instead..
//
void CXR_Model_Tentacles::SnapMeshData(CXR_Model_TriangleMesh& _Mesh)
{
	MACRO_GetRegisterObject(CXR_VBContext, pVBContext, "SYSTEM.VBCONTEXT");

	uint nClusters = _Mesh.GetNumClusters();
	for (uint i=0; i < nClusters; i++)
	{
	  #ifndef PLATFORM_CONSOLE
		CTM_Cluster& c = *_Mesh.m_lspClusters[i];
	  #else
		CTM_Cluster& c = _Mesh.m_lClusters[i];
	  #endif

		CTM_BDVertexInfo* pBDVI = c.GetBDVertexInfo();
		CTM_BDInfluence* pBDI = c.GetBDInfluence();
		if (!pBDI || !pBDVI)
			continue;

		uint nVerts = c.GetNumVertices();
		CVec3Dfp32* pVerts = c.GetVertexPtr(0);
		CVec3Dfp32* pNormals = c.GetNormalPtr(0);
		CVec3Dfp32* pTangU = c.m_lVFrames[0].m_lTangentU.GetBasePtr();
		CVec3Dfp32* pTangV = c.m_lVFrames[0].m_lTangentV.GetBasePtr();
		fp32* pWeights = c.m_lMatrixWeights.GetBasePtr();
		uint32* pIndices = c.m_lMatrixIndices.GetBasePtr();

		for (uint j=0; j<nVerts; j++)
		{
			CVec3Dfp32& v = pVerts[j];
			CVec3Dfp32& n = pNormals[j];
			fp32* pW = pWeights + 3*j;			// caution!!
			uint8* pI = (uint8*)(pIndices+j);

			if (M_Fabs(v.k[2]) < 0.1f)
			{
				v.k[2] = 0.0f;	// snap to z=0
			}
			else if (M_Fabs(v.k[2] - 12.0f) < 0.1f)
			{
				v.k[2] = 12.0f;	// snap to z=12
				if (M_Fabs(n.k[2]) < 0.1f)
				{
					for (uint k=0; k<nVerts; k++)
					{
						const CVec3Dfp32& v0 = pVerts[k];
						const CVec3Dfp32& n0 = pNormals[k];
						fp32 dXY2 = Sqr(v.k[0] - v0.k[0]) + Sqr(v.k[1] - v0.k[1]);
						if (dXY2 < 0.1f && M_Fabs(v0.k[2]) < 0.1f && M_Fabs(n0.k[2]) < 0.1f)
						{
							v = v0;		// snap x,y
							v.k[2] = 12.0f;
							n = n0;		// snap normal
							if (pTangU) pTangU[j] = pTangU[k];
							if (pTangV) pTangV[j] = pTangV[k];
						}
					}
				}
			}

			CTM_BDInfluence* pInf = pBDI + pBDVI[j].m_iBoneInfluence;
			uint nBones = pBDVI[j].m_nBones;
			for (uint k=0; k < nBones; k++)
			{
				fp32 w = pInf[k].m_Influence;
				if (w > 0.99f)
				{
					for (uint l=0; l < nBones; l++)
						pInf[l].m_Influence = 0.0f;

					pInf[k].m_Influence = 1.0f;
				}
			}
		}
		pVBContext->VB_MakeDirty(c.m_VBID); // Force the renderer to re-read the data
	}
}


void CXR_Model_Tentacles::OnPrecache(CXR_Engine* _pEngine)
{
}

struct VecSplineSegment
{
	CVec3Dfp32 m_v0, m_t0; // start value & out-tangent
	CVec3Dfp32 m_v1, m_t1; // end value & in-tangent

	// t = [0:1]
	void CalcPos(fp32 t, CVec3Dfp32& dest) const
	{
		fp32 t2 = t*t;
		fp32 t3 = t2*t;

		fp32 h[4] = { (2*t3 - 3*t2 + 1), (-2*t3 + 3*t2),
		             (t3 - 2*t2 + t),   (t3 - t2) };

		dest.k[0] = h[0]*m_v0.k[0] + h[1]*m_v1.k[0] + h[2]*m_t0.k[0] + h[3]*m_t1.k[0];
		dest.k[1] = h[0]*m_v0.k[1] + h[1]*m_v1.k[1] + h[2]*m_t0.k[1] + h[3]*m_t1.k[1];
		dest.k[2] = h[0]*m_v0.k[2] + h[1]*m_v1.k[2] + h[2]*m_t0.k[2] + h[3]*m_t1.k[2];
	}

	// Approximate arc-length
	fp32 GetLength(fp32 _tmax = 1.0f) const
	{
		CVec3Dfp32 v1, v0 = m_v0;
		enum { Steps = 10 };
		fp32 scale = _tmax / Steps;
		fp32 len = 0.0f;
		for (uint i=0; i<Steps; i++, v0 = v1)
		{
			CalcPos((i+1)*scale, v1);
			len += (v1 - v0).Length();
		}
		return len;
	}

	// walks the spline, and places points at given intervals
	uint GetPositions(fp32 _SegLen, SplinePoint* _pBuffer, uint _nBufSize) const
	{
		fp32 len = GetLength();
		fp32 step = (_SegLen / len) * (1.0f / 30);	// step one 5th of a segment

		CVec3Dfp32 v1, v0 = m_v0;
		uint nSegs = 0;
		fp32 sum = 0.0f;
		for (uint i=0; nSegs < _nBufSize; i++, v0 = v1)
		{
			fp32 t = (i+1) * step;
			if (t >= 1.0f)
			{
				_pBuffer[nSegs].t = 1.0f;
				_pBuffer[nSegs].pos = m_v1;
				_pBuffer[nSegs].len = sum + (m_v1 - v0).Length();
				nSegs++;
				break;
			}
			else
			{
				CalcPos(t, v1);
				sum += (v1 - v0).Length();
				if (sum >= _SegLen)
				{
				//	fp32 adjust = sum / _SegLen;	// test
				//	t /= adjust;
				//	sum = 0.0f;
					_pBuffer[nSegs].t = t;
					_pBuffer[nSegs].pos = v1;
					_pBuffer[nSegs].len = sum;
					nSegs++;
					sum -= _SegLen;
				}
			}
		}
		return nSegs;
	}

	struct SplinePoint2
	{
		fp32 t;				// [0..1]
		fp32 len;			// length to previous point
		CVec3Dfp32 pos;
		CVec3Dfp32 forward;
		CVec3Dfp32 up;
		CVec3Dfp32 side;
	};
	uint GetMatrices(fp32 _SegLen, SplinePoint2* _pBuffer, uint _nBufSize, const CVec3Dfp32& _FirstUp) const
	{
		SplinePoint tmp[100];
		uint nPts = GetPositions(_SegLen, tmp, sizeof_buffer(tmp));
		nPts = Min(nPts, _nBufSize);
		CVec3Dfp32 p, up = _FirstUp;
		for (uint i=0; i<nPts; i++)
		{
			fp32 t = tmp[i].t;
			const CVec3Dfp32& pos = tmp[i].pos;
			_pBuffer[i].t = t;
			_pBuffer[i].pos = pos;
			_pBuffer[i].len = tmp[i].len;

			CalcPos(t+0.001f, p);
			_pBuffer[i].forward = (p - pos).Normalize();

			up.CrossProd(_pBuffer[i].forward, p);
			_pBuffer[i].side = p.Normalize();

			_pBuffer[i].forward.CrossProd(_pBuffer[i].side, up);
			_pBuffer[i].up = up;
		}
		return nPts;
	}
};


void CXR_Model_Tentacles::OnRender(CXR_Engine* _pEngine, 
                                   CRenderContext* _pRender, 
                                   CXR_VBManager* _pVBM, 
                                   CXR_ViewClipInterface* _pViewClip, 
                                   spCXR_WorldLightState _spWLS, 
                                   const CXR_AnimState* _pAnimState, 
                                   const CMat43fp32& _WMat, 
                                   const CMat43fp32& _VMat, 
                                   int _Flags)
{
	//testing testing (only support for straight segments)
	CXR_Model_TriangleMesh* pMesh = m_lspTemplates[0];
	if (pMesh)
	{
		m_BoundRadius = 16.0f;
		m_BoundBox = CBox3Dfp32(-16.0f, 16.0f);

	  #ifndef M_RTM
		CWireContainer* pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
	  #endif

		CXR_AnimState animstate = *_pAnimState;	// temporary animstate
		CMat43fp32 unit; 
		unit.Unit();

		const CVec3Dfp32& ModelPos = CVec3Dfp32::GetRow(_WMat, 3);
		fp32 ZMin = ModelPos.k[2] + 4.0f;

		CTentaclesData& data = *safe_cast<CTentaclesData>(_pAnimState->m_pModelInstance);
		fp32 time = _pAnimState->m_AnimTime0.GetTime();	// TODO: check for precision issues

		uint nSegs = data.m_nSegs;
		for (uint j=0; j<nSegs; j++)
		{
			CTentaclesData::SegTemp& s = data.m_Segs[j];
			CVec3Dfp32 vGlobal = s.m_Start.Pos;
			CQuatfp32 qGlobal = s.m_Start.Rot;
			CVec3Dfp32 EndPos;

#ifndef M_RTM
			if (pWC) pWC->RenderAABB(vGlobal-2.0f, vGlobal+2.0f, 0xff7f7f7f, 0.0f, false);
#endif

			uint nBones = 2;
			enum { MaxBones = 30 };//sizeof_buffer(s.m_LastSplinePoints) };

#ifdef VERSION2
			VecSplineSegment::SplinePoint2 splinepoints[MaxBones];
#endif
#ifdef VERSION1
			SplinePoint splinepoints[MaxBones];
#endif

			CVec3Dfp32 vtmp1, vtmp2;
			CMat43fp32 mtmp, transform1, transform2;

			VecSplineSegment spline;
#ifdef VERSION1
			fp32 endadjust; // 0:last part has zero length .. 1:last part has full length
#endif
			if (!s.m_bFree)
			{
				// Create spline segment
				spline.m_v0 = vGlobal;
				spline.m_v1 = s.m_End.Pos;
				Quat_RotateZ(qGlobal, vtmp1);
				Quat_RotateZ(s.m_End.Rot, vtmp2);
				fp32 len = (spline.m_v1 - spline.m_v0).Length();
				spline.m_t0 = vtmp1 * Min(len*3.0f, 100.0f);
				spline.m_t1 = vtmp2 * Min(len*3.0f, 100.0f);	//0.0f;//temp--- 

				// Calculate spline points
#ifdef VERSION2
				nBones = spline.GetMatrices(12.0f, splinepoints, sizeof_buffer(splinepoints), CVec3Dfp32(0,0,1)); //temp - use real up vector
#endif
#ifdef VERSION1
				nBones = spline.GetPositions(12.0f, splinepoints, sizeof_buffer(splinepoints));
				fp32 endlen = splinepoints[nBones-1].len;
				endadjust = (endlen < 12.0f) ? (1.0f - endlen * (1.0f/12.0f)) : 0.0f;

		/*		// Adjust spline points
				for (int i=0; i<MaxBones; i++)
					s.m_LastSplinePoints[i] = splinepoints[i];	*/
#endif 

			  #ifndef M_RTM
				if (pWC)
				{
					CVec3Dfp32 v1, v0 = spline.m_v0;
					for (int i=0; i<10; i++, v0 = v1)
					{
						spline.CalcPos((i+1)*0.1f, v1);
						DEBUG_WIRE(v0, v1, 0xff7F0000);
					}
				}
			  #endif
			}

			fp32 InvNumBones = 1.0f / nBones;

			for (int i=0; i<nBones; i++)
			{
			//	animstate.m_pSkeletonInst = &data.m_Segs[j].m_FinalBones[i];	// yuck!
			//	CMat43fp32_MP* pBones =  data.m_Segs[j].m_FinalBones[i].GetBoneTransforms();
				animstate.m_pSkeletonInst = _pVBM->Alloc_SkeletonInst(4, 0, 0);
				CMat43fp32_MP* pBones =  animstate.m_pSkeletonInst->GetBoneTransforms();
				pBones[0].Unit();
				vGlobal.SetRow(pBones[0], 3);

				// Prevent errors to propagate
				qGlobal.Normalize();

				// create start transform
				qGlobal.SetMatrix(transform1);
				CVec3Dfp32::GetMatrixRow(transform1, 3) = vGlobal;
				CQuatfp32 q0 = qGlobal;

				fp32 t = (i+1) * InvNumBones;
				if (s.m_bFree)
				{
					fp32 wiggle = s.m_Start.Wiggle + (s.m_End.Wiggle - s.m_Start.Wiggle) * t;
					wiggle *= 2;
					fp32 wt = i * 0.2f, wo = s.m_ID * 1.0f;
					CQuatfp32 qX(CVec3Dfp32(1,0,0), wiggle * M_Sin(time*(0.9f+wt*0.53f)+wt*0.71f+wo*2.93f) );
					CQuatfp32 qY(CVec3Dfp32(0,1,0), wiggle * M_Sin(time*(0.8f+wt*0.81f)+wt*1.37f+wo*2.17f) );

					vGlobal += CVec3Dfp32::GetRow(transform1, 2) * 12.0f;
					vGlobal.k[2] = Max(vGlobal.k[2], ZMin);
					qGlobal *= qX;
					qGlobal *= qY;
				}
				else
				{
#ifdef VERSION2
					const VecSplineSegment::SplinePoint2& pnt = splinepoints[i];
					fp32 u = pnt.t;

					fp32 wiggle = s.m_Start.Wiggle + (s.m_End.Wiggle - s.m_Start.Wiggle) * t;
					wiggle *= 200.0f * M_Sin(u * _PI);

					fp32 wt = i * 0.2f, wo = j * 1.0f;
					fp32 wX = wiggle * M_Sin(time*(0.9f+wt*0.53f)+wt*0.71f+wo*2.93f);
					fp32 wY = wiggle * M_Sin(time*(0.8f+wt*0.81f)+wt*1.37f+wo*2.17f);
					CVec3Dfp32 offset = pnt.side * wX + pnt.up * wY;
					//offset = 0.0f;
					vtmp1 = pnt.pos + offset;

					vtmp2 = vtmp1 - vGlobal;
					fp32 jlen = vtmp2.Length();
					vtmp2 *= (1.0f / jlen);		// new direction
					CQuatfp32 q; 
					Quat_Align(q, CVec3Dfp32::GetRow(transform1, 2), vtmp2);	// align from old to new dir
					qGlobal = qGlobal * q;			// new dir
					Quat_RotateZ(qGlobal, vtmp2);		// calc modified direction
					vGlobal += vtmp2 * jlen;	// move to new position
					vGlobal.k[2] = Max(vGlobal.k[2], ZMin);
#endif
#ifdef VERSION1
					fp32 wiggle = s.m_Start.Wiggle + (s.m_End.Wiggle - s.m_Start.Wiggle) * t;
					wiggle *= 2;
					fp32 wt = i * 0.2f, wo = j * 1.0f;
					CQuatfp32 qX(CVec3Dfp32(1,0,0), wiggle * M_Sin(time*(0.9f+wt*0.53f)+wt*0.71f+wo*2.93f) );
					CQuatfp32 qY(CVec3Dfp32(0,1,0), wiggle * M_Sin(time*(0.8f+wt*0.81f)+wt*1.37f+wo*2.17f) );

					//fp32 u = s.m_LastSplinePoints[i].t;
					fp32 u = splinepoints[i].t;
					spline.CalcPos(u, vtmp1);
					DEBUG_WIRE(vGlobal, vtmp1, 0xff00007F);

					vtmp2 = vtmp1 - vGlobal;
					fp32 jlen = vtmp2.Length();
					vtmp2 *= (1.0f / jlen);		// new direction
					CQuatfp32 q; 
					Quat_Align(q, CVec3Dfp32::GetRow(transform1, 2), vtmp2);	// align from old to new dir
					q = qGlobal * q;			// new dir
					q *= qX;					// apply wiggle
					q *= qY;
					qGlobal = q;
					Quat_RotateZ(q, vtmp2);		// calc modified direction
					vGlobal += vtmp2 * jlen;	// move to new position
					vGlobal.k[2] = Max(vGlobal.k[2], ZMin);

					if (i == (nBones-2))
					{
						// Prepare for landing..
						vGlobal.Lerp(s.m_End.Pos, endadjust, vGlobal);
						qGlobal.Interpolate(s.m_End.Rot, qGlobal, endadjust);
						//		CVec3Dfp32 v05; CVec3Dfp32::GetRow(transform1, 3).Lerp(s.m_End.Pos, 0.5f, v05);
						//		vGlobal.Lerp(v05, endadjust, vGlobal);
						//		CQuatfp32 q05; q0.Interpolate(s.m_End.Rot, q05, 0.5f);
						//		qGlobal.Interpolate(q05, qGlobal, endadjust);
					}
					if (i == (nBones-1))
					{
						// Snap to end (prevent "errors" to propagate)
						qGlobal = s.m_End.Rot;
						vGlobal = s.m_End.Pos;
					}
#endif
				}

				if (i == (nBones-1))
					EndPos = vGlobal;

				// Create end transform
				qGlobal.SetMatrix(transform2);
				CVec3Dfp32::GetMatrixRow(transform2, 3) = vGlobal;

			#ifndef M_RTM
				if (pWC) pWC->RenderMatrix(transform1, 0.0f, false);
				if (pWC) pWC->RenderMatrix(transform2, 0.0f, false);
			#endif

				// Calculate thickness
				fp32 size0 = s.m_Start.Size, dsize = (s.m_End.Size - s.m_Start.Size) * InvNumBones;
				fp32 thick1 = size0 + dsize*i;
				fp32 thick2 = thick1 + dsize;
				fp32 length = (vGlobal - CVec3Dfp32::GetRow(transform1, 3)).Length();
				fp32 zscale = length * (1.0f / 12.0f); // compensate for different segment lengths
				zscale = 1.0f;  //-- with zscale, compressed segments looks better, but unfortunately this breaks lighting (becomes to bright)

				// Create scale matrices (thickness & length)
				CMat4Dfp32 scale1, scale2;
				scale1.Unit();
				scale1.k[0][0] = thick1;
				scale1.k[1][1] = thick1;
				scale1.k[2][2] = zscale;
				scale2.Unit();
				scale2.k[0][0] = thick2;
				scale2.k[1][1] = thick2;
				scale2.k[2][2] = zscale;

				// Create render matrices
				scale1.Multiply(transform1, pBones[1]);
				m_Inv1.Multiply(scale2, mtmp);
				mtmp.Multiply(transform2, pBones[2]);

				pMesh->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, &animstate, transform1, _VMat, _Flags);
			}

			CVec3Dfp32 p0 = s.m_Start.Pos - ModelPos;
			CVec3Dfp32 p1 = EndPos - ModelPos;
			m_BoundRadius = Max(m_BoundRadius, p0.Length());
			m_BoundRadius = Max(m_BoundRadius, p1.Length());
			m_BoundBox.Expand(p0);
			m_BoundBox.Expand(p1);
		}
	#ifndef M_RTM
		if (pWC) pWC->RenderAABB(CBox3Dfp32(m_BoundBox.m_Min + ModelPos, m_BoundBox.m_Max + ModelPos), 0xff800000, 0.2f, false);
	#endif
	}
}


void CXR_Model_Tentacles::OnEvalKey(uint32 _KeyHash, const CRegistry* _pReg)
{
	CXR_Model_Custom::OnEvalKey(_pReg);
}


void CXR_Model_Tentacles::OnCreate(const char* _keys)
{
	m_Keys = _keys;
	m_BoundRadius = 128.0f; // will be changed during rendering

	m_Inv1.Unit();
	CVec3Dfp32::GetMatrixRow(m_Inv1, 3) = CVec3Dfp32(0, 0, -12.0f);
}


TPtr<CXR_ModelInstance> CXR_Model_Tentacles::CreateModelInstance()
{
	return MNew(CTentaclesData);
}


aint CXR_Model_Tentacles::GetParam(int _Param)
{
	switch (_Param)
	{
	case CXR_MODEL_PARAM_ISSHADOWCASTER:
		return 1;

	default:
		return CXR_Model_Custom::GetParam(_Param);
	}
}


void CXR_Model_Tentacles::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState*)
{
	_Box = m_BoundBox;
}

#endif