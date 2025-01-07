
#include "PCH.h"

#define FACECUT3_EPSILON 0.01f

int BSPModel_CutFace3(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, int& _bClip)
{
	MAUTOSTRIP(BSPModel_CutFace3, 0);
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp32 VClip[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec3Dfp32* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
		int Side = 0;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = -pP->Distance(pVSrc[v]);
			if (VertPDist[v] < -FACECUT3_EPSILON) Side |= 2;
			else if (VertPDist[v] > FACECUT3_EPSILON) Side |= 1;
			else Side |= 4;

//			if (VertPDist[v] < 0.0f) bBehind = true; else bFront = true;
		}

		// If all points are on one side, return either all or none.
		if ((Side & 3) == 2)
		{
			_bClip = true;
			return 0;
		}
		if ((Side & 3) != 3) continue;

/*		if (!(bFront && bBehind))
		{
			if (bFront) continue;
			_bClip = true;
			return 0;
		}
*/
		_bClip = true;
		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -FACECUT3_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					nClip++;

					if ((VertPDist[v2] < -FACECUT3_EPSILON) && (VertPDist[v] > FACECUT3_EPSILON))
					{
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = -(dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2]);
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;
							nClip++;
						}
					}
				}
				else
				{
					if (VertPDist[v2] > FACECUT3_EPSILON)
					{
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = -(dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2]);
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;
							nClip++;
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (!nClip) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
	return _nv;
}

void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox)
{
	MAUTOSTRIP(TransformAABB, MAUTOSTRIP_VOID);
	// Transform axis-aligned bounding box.

	CVec3Dfp32 E;
	CVec3Dfp32 C;
	_Box.m_Max.Lerp(_Box.m_Min, 0.5f, C);
	_Box.m_Max.Sub(_Box.m_Min, E);
	E *= 0.5f;

	C *= _Mat;

	_DestBox.m_Max = 0;
	for(int axis = 0; axis < 3; axis++)
		for(int k = 0; k < 3; k++)
			_DestBox.m_Max.k[k] += M_Fabs(_Mat.k[axis][k]*E[axis]);

	_DestBox.m_Min = -_DestBox.m_Max;
	_DestBox.m_Min += C;
	_DestBox.m_Max += C;
}

//#define MODEL_BSP_DISABLENHF

// -------------------------------------------------------------------
//  Move somewhere:
// -------------------------------------------------------------------
/*int BoxInClipVolume(const CBox3Dfp32& _Box, const CRC_ClipVolume& _Volume)
{
	MAUTOSTRIP(BoxInClipVolume, NULL);
	for (int p = 0; p < _Volume.m_nPlanes; p++)
		if (_Volume.m_Planes[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > MODEL_BSP_EPSILON) return 0;
	return 1;
}
*/

int BoxInClipVolume(const CBox3Dfp32& _Box, const CRC_ClipVolume& _Volume);
int BoxInClipVolume(const CBox3Dfp32& _Box, const CRC_ClipVolume& _Volume)
{
	MAUTOSTRIP(BoxInClipVolume_2, 0);
	CVec3Dfp32 Center;
	Center.k[0] = (_Box.m_Min.k[0] + _Box.m_Max.k[0]) * 0.5f;
	Center.k[1] = (_Box.m_Min.k[1] + _Box.m_Max.k[1]) * 0.5f;
	Center.k[2] = (_Box.m_Min.k[2] + _Box.m_Max.k[2]) * 0.5f;
//	fp32 RSqr = Center.DistanceSqr(_Box.m_Min);

	for (int p = 0; p < _Volume.m_nPlanes; p++)
	{
		fp32 d = _Volume.m_Planes[p].Distance(Center);
		if (d > 0.0f)
			if (_Volume.m_Planes[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > MODEL_BSP_EPSILON) return 0;
//		if (d > 0.0f) 
//			if (Sqr(d) > RSqr) return 0;

	}
	return 1;
}

// -------------------------------------------------------------------
bool CalcDetailMapIntensity(int _Flags, fp32 _FadeOutFront, fp32 _FadeOutBack, int _Alpha, int _nV, 
	const CVec3Dfp32* _pV, CPixel32* _pDest, const CPlane3Dfp32& _ProjPlane);
bool CalcDetailMapIntensity(int _Flags, fp32 _FadeOutFront, fp32 _FadeOutBack, int _Alpha, int _nV, 
	const CVec3Dfp32* _pV, CPixel32* _pDest, const CPlane3Dfp32& _ProjPlane)
{
	MAUTOSTRIP(CalcDetailMapIntensity, false);
	// true == draw, false == skip
	
	fp32 SpanInv = 1.0f / (_FadeOutBack - _FadeOutFront);

	fp32 MinDist = 1000000.0f;
	for(int v = 0; v < _nV; v++)
	{
		fp32 Dist = _ProjPlane.Distance(_pV[v]);
		if (Dist < 0.0f)
			Dist = 0.0f;
		if (Dist < MinDist) MinDist = Dist;
		if (Dist > _FadeOutBack) Dist = _FadeOutBack;

		int Alpha;
		if (Dist < _FadeOutFront)
			Alpha = _Alpha;
		else
		{
			fp32 k = (1.0f - (Dist - _FadeOutFront) * SpanInv);
			Alpha = (int)(k * fp32(_Alpha));
		}

		if (_Flags & 1)
			_pDest[v] = CPixel32(Alpha, Alpha, Alpha, Alpha >> 1);
		else if (_Flags & 2)
			_pDest[v] = CPixel32(Alpha, Alpha, Alpha, 255);
		else
			_pDest[v] = CPixel32(255, 255, 255, Alpha);
	}
	if (MinDist > _FadeOutBack) return false;
	return true;
}
