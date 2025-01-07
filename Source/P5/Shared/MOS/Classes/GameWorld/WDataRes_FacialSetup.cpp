/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WDataRes_FacialSetup.cpp

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CFacialSetup

	Comments:

	History:		
		050917:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WDataRes_FacialSetup.h"

MRTC_IMPLEMENT_DYNAMIC(CWRes_FacialSetup, CWResource);

#define IsValidFloat(v) ((v) > -1000000.0f && (v) < 1000000.0f)
#define Quat_AlmostEqual(a, b, e) (M_Fabs((a).DotProd(b)) > (1.0f - e))


M_FORCEINLINE bint IsNonZeroVec3(vec128 _v)
{
	return M_VCmpAnyGT(M_VDp3(_v, _v), M_VScalar(0.01f));		// -- _v.LengthSqr() > 0.01f
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFacialSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CFacialSetup::Init(const CXR_Anim_Base& _Anim)
{
	const uint NumFrames = 2;
	const uint MaxJoints = 100;
	CVec4Dfp32 MoveData[NumFrames][MaxJoints];
	CQuatfp32 RotData[NumFrames][MaxJoints];
	TStaticArray<uint8, MaxJoints> liMoveTracks;
	TStaticArray<uint8, MaxJoints> liRotTracks;

	m_MoveMask.Clear();
	m_RotMask.Clear();
	m_iMaxMove = 0;
	m_iMaxRot = 0;

	CXR_Anim_TrackMask FilledMask;
	FilledMask.Fill();

	uint nSeq = _Anim.GetNumSequences();
	m_lGroups.SetLen(nSeq);
	if (nSeq == 0)
		return false;


	for (uint iSeq = 0; iSeq < nSeq; iSeq++)
	{
		const spCXR_Anim_SequenceData spSeq = _Anim.GetSequence(iSeq);
		const CXR_Anim_SequenceData& Seq = *spSeq;

		fp32 Duration = Seq.GetDuration();
		for (uint i = 0; i < NumFrames; i++)
		{
			fp32 t = i * (Duration / (NumFrames - 1));
			Seq.Eval(t, RotData[i], MaxJoints, &MoveData[i]->v, MaxJoints, FilledMask);
		}

		liMoveTracks.Clear();
		liRotTracks.Clear();
		for (uint i = 0; i < MaxJoints; i++)
		{
			const CVec4Dfp32& Move0 = MoveData[0][i];
			const CQuatfp32&  Rot0 =  RotData[0][i];
			bool bMove = false, bRot = false;

			for (uint j = 1; j < NumFrames && !(bRot && bMove); j++)
			{
				const CVec4Dfp32& Move = MoveData[j][i];
				const CQuatfp32&  Rot =  RotData[j][i];

				vec128 Delta = M_VSub(Move, Move0);
				if (!bMove && IsValidFloat(Move.k[0]) && IsNonZeroVec3(Delta))
				{
					bMove = true;
					liMoveTracks.Add(i);
					m_MoveMask.Set1(i);
				}

				if (!bRot && IsValidFloat(Rot.k[0]) && !Quat_AlmostEqual(Rot, Rot0, 0.001f))
				{
					bRot = true;
					liRotTracks.Add(i);
					m_RotMask.Set1(i);
				}
			}
		}

		Group& g = m_lGroups[iSeq];

		uint nMoveTracks = liMoveTracks.Len();
		g.m_lMoveKeys.SetLen(nMoveTracks);// * NumFrames);
		g.m_liMoveTracks.SetLen(nMoveTracks);
		memcpy(g.m_liMoveTracks.GetBasePtr(), liMoveTracks.GetBasePtr(), nMoveTracks * sizeof(uint8));

		uint nRotTracks = liRotTracks.Len();
		g.m_lRotKeys.SetLen(nRotTracks);// * NumFrames);
		g.m_liRotTracks.SetLen(nRotTracks);
		memcpy(g.m_liRotTracks.GetBasePtr(), liRotTracks.GetBasePtr(), nRotTracks * sizeof(uint8));

		CVec3Dfp32* pMoveKeys = g.m_lMoveKeys.GetBasePtr();
		CQuatfp32* pRotKeys = g.m_lRotKeys.GetBasePtr();
		for (uint iFrame = 1; iFrame < NumFrames; iFrame++)
		{
			for (uint i = 0; i < nMoveTracks; i++)
			{
				CVec4Dfp32 v = MoveData[iFrame][liMoveTracks[i]];
				v -= MoveData[0][liMoveTracks[i]];
				CVec3Dfp32 tmp;
				tmp.k[0] = v.k[0];
				tmp.k[1] = v.k[1];
				tmp.k[2] = v.k[2];
				*pMoveKeys++ = tmp;
			}

			for (uint i = 0; i < nRotTracks; i++)
			{
				CQuatfp32 q0 = RotData[0][liRotTracks[i]];
				CQuatfp32 q = RotData[iFrame][liRotTracks[i]];
				q0.Inverse();
				q *= q0;
				*pRotKeys++ = q;
			}
		}
	}

	// Store Idle pose
	{
		const spCXR_Anim_SequenceData spSeq = _Anim.GetSequence(0);
		const CXR_Anim_SequenceData& Seq = *spSeq;
		Seq.Eval(0.0f, RotData[0], MaxJoints, &MoveData[0]->v, MaxJoints, FilledMask); // Use sequence 0 frame 0  as idle pose

		CQuatfp32 QUnit; QUnit.Unit();

		liMoveTracks.Clear();
		liRotTracks.Clear();
		for (uint i = 0; i < MaxJoints; i++)
		{
			const CVec4Dfp32& Move = MoveData[0][i];
			const CQuatfp32& Rot = RotData[0][i];

			bool bMoveHeadTrack = (i >= 5);
			if (m_MoveMask.Get(i) || (bMoveHeadTrack && IsValidFloat(Move.k[0]) && IsNonZeroVec3(Move)))
			{
				m_iMaxMove = Max(i, m_iMaxMove);
				liMoveTracks.Add(i);
			}

			// This is a hack to strip out trash body data
			bool bRotHeadTrack = (i == 20) || (i >= 25 && i <= 37) || (i >= 70 && i <= 90);
			if (m_RotMask.Get(i) || (bRotHeadTrack && IsValidFloat(Rot.k[0]) && !Quat_AlmostEqual(Rot, QUnit, 0.01f)))
			{
				m_iMaxRot = Max(i, m_iMaxRot);
				liRotTracks.Add(i);
			}
		}

		uint nMoveTracks = liMoveTracks.Len();
		m_IdlePose.m_lMoveKeys.SetLen(nMoveTracks);
		m_IdlePose.m_liMoveTracks.SetLen(nMoveTracks);
		memcpy(m_IdlePose.m_liMoveTracks.GetBasePtr(), liMoveTracks.GetBasePtr(), nMoveTracks * sizeof(uint8));

		uint nRotTracks = liRotTracks.Len();
		m_IdlePose.m_lRotKeys.SetLen(nRotTracks);
		m_IdlePose.m_liRotTracks.SetLen(nRotTracks);
		memcpy(m_IdlePose.m_liRotTracks.GetBasePtr(), liRotTracks.GetBasePtr(), nRotTracks * sizeof(uint8));

		for (uint i = 0; i < nMoveTracks; i++)
		{
			const CVec4Dfp32& v = MoveData[0][liMoveTracks[i]];
			//M_TRACE("MoveTrack %d: %d - %s\n", i, liMoveTracks[i], v.GetString().Str());
			CVec3Dfp32 tmp;
			tmp.k[0]=v.k[0];
			tmp.k[1]=v.k[1];
			tmp.k[2]=v.k[2];
			m_IdlePose.m_lMoveKeys[i] = tmp;
		}

		for (uint i = 0; i < nRotTracks; i++)
		{
			CQuatfp32 q0 = RotData[0][liRotTracks[i]];
			m_IdlePose.m_lRotKeys[i] = q0;

			/*CMat4Dfp32 m;
			r0.CreateMatrixFromAngles(0, m);
			CQuatfp32 q;
			q.Create(m);
			fp32 k = q0.DotProd(q);
			M_TRACE("RotTrack %d: %d - %s  ([%s], %.3f)\n", i, liRotTracks[i], r0.GetString().Str(), q0.GetString().Str(), k);*/
		}
	}

	return true;
}




void CFacialSetup::Eval(const fp32* _pInput, CXR_SkeletonInstance* _pOutput) const
{
	CVec4Dfp32* pDestMove = _pOutput->m_pTracksMove;
	CQuatfp32* pDestRot = _pOutput->m_pTracksRot;

	if (_pOutput->m_nTracksRot <= m_iMaxRot)
		return;
	if (_pOutput->m_nTracksMove <= m_iMaxMove)
		return;

	// Apply idle pose
	TAP<const uint8> piMoveTracks = m_IdlePose.m_liMoveTracks;
	TAP<const CVec3Dfp32> pMoveData = m_IdlePose.m_lMoveKeys;
	for (uint i = 0; i < piMoveTracks.Len(); i++)
	{
		uint iTrack = piMoveTracks[i];
		M_ASSERT(iTrack < _pOutput->m_nTracksMove, "!");
		pDestMove[iTrack] = pMoveData[i];
	}

	TAP<const uint8> piRotTracks = m_IdlePose.m_liRotTracks;
	TAP<const CQuatfp32> pRotData = m_IdlePose.m_lRotKeys;
	for (uint i = 0; i < piRotTracks.Len(); i++)
	{
		uint iTrack = piRotTracks[i];
		M_ASSERT(iTrack < _pOutput->m_nTracksRot, "!");
		pDestRot[iTrack] = pRotData[i];
	}

	CVec3Dfp32 Tmp; 
	CQuatfp32 QTmp, QUnit; 
	QUnit.Unit();

	// Eval and add muscle groups
	TAP<const Group> pGroups = m_lGroups;
	for (uint8 iGroup = 0; iGroup < pGroups.Len(); iGroup++)
	{
		const Group& g = pGroups[iGroup];
		TAP<const uint8> piMoveTracks = g.m_liMoveTracks;
		TAP<const uint8> piRotTracks = g.m_liRotTracks;
		TAP<const CVec3Dfp32> pMoveKeys = g.m_lMoveKeys;
		TAP<const CQuatfp32> pRotKeys = g.m_lRotKeys;

		fp32 t = _pInput[iGroup];
		if (t < 0.00001f)
			continue;

		for (uint i = 0; i < piMoveTracks.Len(); i++)
		{
			pMoveKeys[i].Scale(t, Tmp);

			uint iTrack = piMoveTracks[i];
			M_ASSERT(iTrack < _pOutput->m_nTracksMove, "!");
			pDestMove[iTrack] += M_VLd_V3_Slow(&Tmp);
		}

		for (uint i = 0; i < piRotTracks.Len(); i++)
		{
			uint iTrack = piRotTracks[i];
			M_ASSERT(iTrack < _pOutput->m_nTracksRot, "!");

			QUnit.Lerp(pRotKeys[i], t, QTmp);	//TODO: optimize!
			pDestRot[iTrack] *= QTmp;
		}
	}
}


#define GetValidFloat(v) (((v) > -1000.0f && (v) < 1000.0f) ? Clamp01(v * 1.0f) : 0.0f)

void CFacialSetup::GetFaceData(CXR_SkeletonInstance* _pInput, int _BaseTrack, int _nTracks, fp32* _pDest) const
{
	if(!_pInput->m_pTracksMove || _pInput->m_nTracksMove < _BaseTrack + _nTracks / 3)
		return;

	for (uint i = 0; i < _nTracks/3; i++)
	{
		CVec4Dfp32& v = _pInput->m_pTracksMove[_BaseTrack + i];
		_pDest[i*3+0] = GetValidFloat(v.k[0]);
		_pDest[i*3+1] = GetValidFloat(v.k[1]);
		_pDest[i*3+2] = GetValidFloat(v.k[2]);
		v =  M_VConst(0,0,0,1.0f); // just in case...
	}
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_FacialSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWRes_FacialSetup::CWRes_FacialSetup()
{
}



CFacialSetup* CWRes_FacialSetup::GetData()
{
	return &m_FacialSetup;
}


bool CWRes_FacialSetup::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) 
		return false;

	CFStr Name = _pName + 4;
	CFStr FileName = _pWData->ResolveFileName("ANIM/" + Name);
	if (!CDiskUtil::FileExists(FileName))
	{
		ConOutL(CStrF("§cf80WARNING: (CWRes_FacialSetup::Create) Could not find %s", FileName.Str()));
		return false;
	}

	spCReferenceCount spObjLoader = MRTC_GOM()->CreateObject("CXR_Anim_Base");
	spCXR_Anim_Base spAnim = safe_cast<CXR_Anim_Base>((CReferenceCount*)spObjLoader);
	if (!spAnim)
		Error("Create", "Unable to instance CXR_Anim_Base-object.");

	spAnim = spAnim->ReadMultiFormat(FileName, ANIM_READ_NONAMES | ANIM_READ_NOCOMMENTS);
	if (!spAnim || !m_FacialSetup.Init(*spAnim))
	{
		ConOutL(CStrF("§cf80WARNING: (CWRes_FacialSetup::Create) Could not read %s", FileName.Str()));
		return false;
	}
	return true;
}


void CWRes_FacialSetup::OnLoad()
{
	CWResource::OnLoad();
}


void CWRes_FacialSetup::OnPrecache(CXR_Engine* _pEngine)
{
	CWResource::OnPrecache(_pEngine);
}
