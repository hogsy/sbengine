
#include "PCH.h"
#include "XRIndexedSolid.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

void CXR_IndexedSolidFace::Read(CCFile* _pFile, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_nV);
			_pFile->ReadLE(m_iiV);
			_pFile->ReadLE(m_iSurface);
		}
		break;

	default :
		Error_static("CXR_IndexedSolidFace::Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CXR_IndexedSolidFace::Write(CCFile* _pFile)
{
	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_nV);
	_pFile->WriteLE(m_iiV);
	_pFile->WriteLE(m_iSurface);
}

void CXR_IndexedSolidFace::SwapLE()
{
	::SwapLE(m_Flags);
	::SwapLE(m_nV);
	::SwapLE(m_iiV);
	::SwapLE(m_iSurface);
}

// -------------------------------------------------------------------
void CXR_IndexedSolid::Read(CCFile* _pFile, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			m_BoundBox.Read(_pFile);

			_pFile->ReadLE(m_iVertices);
			_pFile->ReadLE(m_iEdges);
			_pFile->ReadLE(m_iFaces);
			_pFile->ReadLE(m_iiVertices);
			_pFile->ReadLE(m_nVertices);
			_pFile->ReadLE(m_nEdges);
			_pFile->ReadLE(m_nFaces);
			_pFile->ReadLE(m_niVertices);

			_pFile->ReadLE(m_iMedium);
		}
		break;

	default :
		Error_static("CXR_IndexedSolid::Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CXR_IndexedSolid::Write(CCFile* _pFile)
{
	m_BoundBox.Write(_pFile);

	_pFile->WriteLE(m_iVertices);
	_pFile->WriteLE(m_iEdges);
	_pFile->WriteLE(m_iFaces);
	_pFile->WriteLE(m_iiVertices);
	_pFile->WriteLE(m_nVertices);
	_pFile->WriteLE(m_nEdges);
	_pFile->WriteLE(m_nFaces);
	_pFile->WriteLE(m_niVertices);

	_pFile->WriteLE(m_iMedium);
}

void CXR_IndexedSolid::SwapLE()
{
#ifndef CPU_LITTLEENDIAN
	m_BoundBox.SwapLE();

	::SwapLE(m_iVertices);
	::SwapLE(m_iEdges);
	::SwapLE(m_iFaces);
	::SwapLE(m_iiVertices);
	::SwapLE(m_nVertices);
	::SwapLE(m_nEdges);
	::SwapLE(m_nFaces);
	::SwapLE(m_niVertices);

	::SwapLE(m_iMedium);
#endif
}



// -------------------------------------------------------------------
#ifndef PLATFORM_CONSOLE

void CXR_IndexedSolidContainer::Create(TArray<spCSolid> _lspSolids)
{
	// Calc space needed

	int nTotalV = 0;
	int nTotalE = 0;
	int nTotalF = 0;
	int nTotaliV = 0;

	TAP_RCD<spCSolid> pspSolids = _lspSolids;

	{
		for(int i = 0; i < pspSolids.Len(); i++)
		{
			CSolid* pS = pspSolids[i];
			nTotalE += pS->GetNumEdges();
			int nF = pS->GetNumFaces();
			nTotalF += nF;

			TArray<CVec3Dfp64> lV = pS->GetVertexArray();
			nTotalV += lV.Len();

			for(int f = 0; f < nF; f++)
			{
				int nV = pS->GetFace(f).m_nV;
				nTotaliV += nV;
			}
		}
	}

	m_lFaces.SetLen(nTotalF);
	m_lPlanes.SetLen(nTotalF);
	m_lEdges.SetLen(nTotalE);
	m_lVertices.SetLen(nTotalV);
	m_liVertices.SetLen(nTotaliV);
	m_lSolids.SetLen(pspSolids.Len());

	TAP_RCD<CXR_IndexedSolid> pSolids = m_lSolids;
	TAP_RCD<CXR_IndexedSolidFace> pFaces = m_lFaces;
	TAP_RCD<CIndexedEdge16> pEdges = m_lEdges;
	TAP_RCD<CPlane3Dfp32> pPlanes = m_lPlanes;
	TAP_RCD<CVec3Dfp32> pVertices = m_lVertices;
	TAP_RCD<uint16> piVertices = m_liVertices;

	int iV = 0;
	int iiV = 0;
	int iF = 0;
	int iE = 0;

	{
		for(int i = 0; i < pspSolids.Len(); i++)
		{
			CSolid* pS = pspSolids[i];
			CXR_IndexedSolid& Solid = m_lSolids[i];

			TArray<CVec3Dfp64> lV8 = pS->GetVertexArray();
			TAP_RCD<CVec3Dfp64> pV8 = lV8;

			int nE = pS->GetNumEdges();
			int nF = pS->GetNumFaces();

			Solid.m_BoundBox = CBox3Dfp32(pS->m_BoundMin, pS->m_BoundMax);

			Solid.m_iVertices = iV;
			Solid.m_nVertices = pV8.Len();
			Solid.m_iEdges = iE;
			Solid.m_nEdges = nE;
			Solid.m_iFaces = iF;
			Solid.m_nFaces = nF;
			Solid.m_iiVertices = iiV;

			Solid.m_iMedium = 0;

			for(int v = 0; v < pV8.Len(); v++)
			{
				pVertices[iV] = pV8[v].Getfp32();
				iV++;
			}

			for(int e = 0; e < nE; e++)
			{
				const CSolid_Edge& Edge = pS->GetEdge(e);
				pEdges[iE] = CIndexedEdge16(Edge.m_iV[0], Edge.m_iV[1]);
				iE++;
			}

			int iiVLocal = 0;
			for(int f = 0; f < nF; f++)
			{
				const CSolid_Face& SFace = pS->GetFace(f);
				CXR_IndexedSolidFace& Face = pFaces[iF]; 

				const CPlane3Dfp64& Plane = pS->GetPlane(f).m_Plane;
				pPlanes[iF] = CPlane3Dfp32(Plane.n.Getfp32(), fp32(Plane.d));

				int nFV = SFace.m_nV;
				Face.m_iiV = iiVLocal;
				Face.m_nV = nFV;
				Face.m_iSurface = 0;
				Face.m_Flags = 0;

				memcpy(&piVertices[iiV], pS->GetFaceVertexIndices(f), sizeof(uint16) * nFV);
				iiV += nFV;
				iiVLocal += nFV;


				iF++;
			}

			Solid.m_niVertices = iiVLocal;
		}
	}

	if (iV != m_lVertices.Len())
		Error("Create", CStrF("Internal error. (iV == %d/%d", iV, m_lVertices.Len()));
	if (iE != m_lEdges.Len())
		Error("Create", CStrF("Internal error. (iE == %d/%d", iE, m_lEdges.Len()));
	if (iiV != m_liVertices.Len())
		Error("Create", CStrF("Internal error. (iiV == %d/%d", iiV, m_liVertices.Len()));
	if (iF != m_lFaces.Len())
		Error("Create", CStrF("Internal error. (iF == %d/%d", iF, m_lFaces.Len()));

}

#endif

void CXR_IndexedSolidContainer::GetSolid(int _iSolid, CXR_IndexedSolidDesc& _Desc) const
{
	TAP_RCD<const CXR_IndexedSolid> pSolids = m_lSolids;
	const CXR_IndexedSolid& Solid = pSolids[_iSolid];

	TAP<const CVec3Dfp32> pV = m_lVertices;
	_Desc.m_pV.m_pArray = pV.m_pArray+Solid.m_iVertices;
	_Desc.m_pV.m_Len = Solid.m_nVertices;
	TAP<const CIndexedEdge16> pE = m_lEdges;
	_Desc.m_pE.m_pArray = pE.m_pArray+Solid.m_iEdges;
	_Desc.m_pE.m_Len = Solid.m_nEdges;
	TAP<const CXR_IndexedSolidFace> pF = m_lFaces;
	int iFaces = Solid.m_iFaces;
	_Desc.m_pF.m_pArray = pF.m_pArray+iFaces;
	_Desc.m_pF.m_Len = Solid.m_nFaces;
	_Desc.m_pP = m_lPlanes.GetBasePtr() + iFaces;
	_Desc.m_piV = m_liVertices.GetBasePtr() + Solid.m_iiVertices;
}

#define WRITEARRAY(_pF, _lArray, _WriteFunc)	\
	{ for(int i = 0; i < _lArray.Len(); i++) _lArray[i]._WriteFunc(_pF); }

#define READARRAY(_pF, _lArray, _ReadFunc, _ArrayLen)	\
	{ _lArray.SetLen(_ArrayLen); for(int i = 0; i < _lArray.Len(); i++) _lArray[i]._ReadFunc(_pF); }

#define READARRAY_VER(_pF, _lArray, _ReadFunc, _ArrayLen, _Ver)	\
	{ _lArray.SetLen(_ArrayLen); for(int i = 0; i < _lArray.Len(); i++) _lArray[i]._ReadFunc(_pF, _Ver); }


void CXR_IndexedSolidContainer::Read(CDataFile* _pDFile)
{
	CCFile* pF = _pDFile->GetFile();

	// SOLIDS
//	_pDFile->ReadArrayEntry_Complex(m_lSolids, "SOLIDS");
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLIDS"))
	{
		READARRAY_VER(pF, m_lSolids, Read, _pDFile->GetUserData(), _pDFile->GetUserData2());
	}
	_pDFile->PopPosition();

	// SOLID_PLANES
//	_pDFile->ReadArrayEntry_NoVer(m_lPlanes, "SOLID_PLANES");
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_PLANES"))
	{
		READARRAY(pF, m_lPlanes, Read, _pDFile->GetUserData());
	}
	_pDFile->PopPosition();

	// SOLID_FACES
//	_pDFile->ReadArrayEntry_Complex(m_lFaces, "SOLID_FACES");
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_FACES"))
	{
		READARRAY_VER(pF, m_lFaces, Read, _pDFile->GetUserData(), _pDFile->GetUserData2());
	}
	_pDFile->PopPosition();

	// SOLID_EDGES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_EDGES"))
	{
		READARRAY(pF, m_lEdges, Read, _pDFile->GetUserData());
	}
	_pDFile->PopPosition();

	// SOLID_VERTICES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_VERTICES"))
	{
		READARRAY(pF, m_lVertices, Read, _pDFile->GetUserData());
	}
	_pDFile->PopPosition();

	// SOLID_VERTEXINDICES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_VERTEXINDICES"))
	{
		m_liVertices.SetLen(_pDFile->GetUserData());
		pF->ReadLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
	}
	_pDFile->PopPosition();
}

void CXR_IndexedSolidContainer::Write(CDataFile* _pDFile)
{
	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginEntry("SOLIDS");
	WRITEARRAY(pF, m_lSolids, Write);
	_pDFile->EndEntry(m_lSolids.Len(), XR_INDEXEDSOLID_VERSION);

	_pDFile->BeginEntry("SOLID_PLANES");
	WRITEARRAY(pF, m_lPlanes, Write);
	_pDFile->EndEntry(m_lPlanes.Len(), 0);

	_pDFile->BeginEntry("SOLID_FACES");
	WRITEARRAY(pF, m_lFaces, Write);
	_pDFile->EndEntry(m_lFaces.Len(), XR_INDEXEDSOLIDFACE_VERSION);

	_pDFile->BeginEntry("SOLID_EDGES");
	WRITEARRAY(pF, m_lEdges, Write);
	_pDFile->EndEntry(m_lEdges.Len(), 0);

	_pDFile->BeginEntry("SOLID_VERTICES");
	WRITEARRAY(pF, m_lVertices, Write);
	_pDFile->EndEntry(m_lVertices.Len(), 0);

	_pDFile->BeginEntry("SOLID_VERTEXINDICES");
	pF->Write(m_liVertices.GetBasePtr(), m_liVertices.Len());
	_pDFile->EndEntry(m_liVertices.Len(), 0);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_IndexedSolidContainer32
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_IndexedSolidFace32::Read(CCFile* _pFile, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			_pFile->ReadLE(m_Flags);
			_pFile->ReadLE(m_nV);
			_pFile->ReadLE(m_iiV);
			_pFile->ReadLE(m_iSurface);
			_pFile->ReadLE(m_iPlane);
		}
		break;

	default :
		Error_static("CXR_IndexedSolidFace::Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CXR_IndexedSolidFace32::Write(CCFile* _pFile)
{
	_pFile->WriteLE(m_Flags);
	_pFile->WriteLE(m_nV);
	_pFile->WriteLE(m_iiV);
	_pFile->WriteLE(m_iSurface);
	_pFile->WriteLE(m_iPlane);
}

void CXR_IndexedSolidFace32::SwapLE()
{
	::SwapLE(m_Flags);
	::SwapLE(m_nV);
	::SwapLE(m_iiV);
	::SwapLE(m_iSurface);
	::SwapLE(m_iPlane);
}

// -------------------------------------------------------------------
void CXR_IndexedSolid32::Read(CCFile* _pFile, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			m_BoundBox.Read(_pFile);

			_pFile->ReadLE(m_iVertices);
			_pFile->ReadLE(m_iEdges);
			_pFile->ReadLE(m_iFaces);
			_pFile->ReadLE(m_iiVertices);
			_pFile->ReadLE(m_nVertices);
			_pFile->ReadLE(m_nEdges);
			_pFile->ReadLE(m_nFaces);
			_pFile->ReadLE(m_niVertices);

			_pFile->ReadLE(m_iMedium);
		}
		break;

	default :
		Error_static("CXR_IndexedSolid::Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CXR_IndexedSolid32::Write(CCFile* _pFile)
{
	m_BoundBox.Write(_pFile);

	_pFile->WriteLE(m_iVertices);
	_pFile->WriteLE(m_iEdges);
	_pFile->WriteLE(m_iFaces);
	_pFile->WriteLE(m_iiVertices);
	_pFile->WriteLE(m_nVertices);
	_pFile->WriteLE(m_nEdges);
	_pFile->WriteLE(m_nFaces);
	_pFile->WriteLE(m_niVertices);

	_pFile->WriteLE(m_iMedium);
}

void CXR_IndexedSolid32::SwapLE()
{
#ifndef CPU_LITTLEENDIAN
	m_BoundBox.SwapLE();

	::SwapLE(m_iVertices);
	::SwapLE(m_iEdges);
	::SwapLE(m_iFaces);
	::SwapLE(m_iiVertices);
	::SwapLE(m_nVertices);
	::SwapLE(m_nEdges);
	::SwapLE(m_nFaces);
	::SwapLE(m_niVertices);

	::SwapLE(m_iMedium);
#endif
}



int CXR_IndexedSolidContainer32::FindPlane(TArray<CPlane3Dfp64>& _lPlanes, const CPlane3Dfp64& _Plane)
{
	TAP<CPlane3Dfp64> pP = _lPlanes;
	const fp64 MaxError = 0.000001f;
	const fp64 SqrMaxError = Sqr(MaxError);

	for(int p = 0; p < pP.Len(); p++)
	{
		if (pP[p].n.DistanceSqr(_Plane.n) > SqrMaxError)
			continue;
		if (fabs(pP[p].d - _Plane.d) > MaxError)
			continue;

		return p;
	}

	return _lPlanes.Add(_Plane);
}

#ifndef PLATFORM_CONSOLE

void CXR_IndexedSolidContainer32::Create(TArray<spCSolid> _lspSolids, TArray<spCXW_Surface>& _lspSurfaces, TArray<CXR_MediumDesc>& _lMediums, TArray<CPlane3Dfp64>& _lPlanes)
{
	// Calc space needed

	int nTotalV = 0;
	int nTotalE = 0;
	int nTotalF = 0;
	int nTotaliV = 0;

	TAP_RCD<spCSolid> pspSolids = _lspSolids;

	{
		for(int i = 0; i < pspSolids.Len(); i++)
		{
			CSolid* pS = pspSolids[i];
			nTotalE += pS->GetNumEdges();
			int nF = pS->GetNumFaces();
			nTotalF += nF;

			TArray<CVec3Dfp64> lV = pS->GetVertexArray();
			nTotalV += lV.Len();

			for(int f = 0; f < nF; f++)
			{
				int nV = pS->GetFace(f).m_nV;
				nTotaliV += nV;
			}
		}
	}
	_lPlanes.SetGrow(nTotalF);

	m_lFaces.SetLen(nTotalF);
	m_lEdges.SetLen(nTotalE);
	m_lVertices.SetLen(nTotalV);
	m_liVertices.SetLen(nTotaliV);
	m_lSolids.SetLen(pspSolids.Len());

	TAP_RCD<CXR_IndexedSolid32> pSolids = m_lSolids;
	TAP_RCD<CXR_IndexedSolidFace32> pFaces = m_lFaces;
	TAP_RCD<CIndexedEdge16> pEdges = m_lEdges;
	TAP_RCD<CVec3Dfp32> pVertices = m_lVertices;
	TAP_RCD<uint16> piVertices = m_liVertices;

	int iV = 0;
	int iiV = 0;
	int iF = 0;
	int iE = 0;

	{
		for(int i = 0; i < pspSolids.Len(); i++)
		{
			CSolid* pS = pspSolids[i];
			CXR_IndexedSolid32& Solid = m_lSolids[i];

			TArray<CVec3Dfp64> lV8 = pS->GetVertexArray();
			TAP_RCD<CVec3Dfp64> pV8 = lV8;

			int nE = pS->GetNumEdges();
			int nF = pS->GetNumFaces();

			Solid.m_BoundBox = CBox3Dfp32(pS->m_BoundMin, pS->m_BoundMax);

			Solid.m_iVertices = iV;
			Solid.m_nVertices = pV8.Len();
			Solid.m_iEdges = iE;
			Solid.m_nEdges = nE;
			Solid.m_iFaces = iF;
			Solid.m_nFaces = nF;
			Solid.m_iiVertices = iiV;

			Solid.m_iMedium = 0;

			TAP<CXR_MediumDesc> pM = _lMediums;
			int iM;
			for(iM = 0; iM < pM.Len(); iM++)
				if (pM[iM].Equal(pS->m_Medium))
				{
					Solid.m_iMedium = iM;
					break;
				}

			if (iM == pM.Len())
				LogFile(CStr("WARNING: (CXR_IndexedSolidContainer32::Create) Medium not found."));

			for(int v = 0; v < pV8.Len(); v++)
			{
				pVertices[iV] = pV8[v].Getfp32();
				iV++;
			}

			for(int e = 0; e < nE; e++)
			{
				const CSolid_Edge& Edge = pS->GetEdge(e);
				pEdges[iE] = CIndexedEdge16(Edge.m_iV[0], Edge.m_iV[1]);
				iE++;
			}

			int iiVLocal = 0;
			for(int f = 0; f < nF; f++)
			{
				const CSolid_Face& SFace = pS->GetFace(f);
				CXR_IndexedSolidFace32& Face = pFaces[iF]; 

				const CPlane3Dfp64& Plane = pS->GetPlane(f).m_Plane;

				int iPlane = FindPlane(_lPlanes, Plane);
				if (iPlane > 65535)
					Error("Create", CStr("Max planes exceeded. (%d > 65535)"));

				int nFV = SFace.m_nV;
				Face.m_iiV = iiVLocal;
				Face.m_nV = nFV;
				Face.m_iSurface = 0;
				Face.m_Flags = 0;
				Face.m_iPlane = iPlane;

				memcpy(&piVertices[iiV], pS->GetFaceVertexIndices(f), sizeof(uint16) * nFV);
				iiV += nFV;
				iiVLocal += nFV;


				iF++;
			}

			Solid.m_niVertices = iiVLocal;
		}
	}

	if (iV != m_lVertices.Len())
		Error("Create", CStrF("Internal error. (iV == %d/%d", iV, m_lVertices.Len()));
	if (iE != m_lEdges.Len())
		Error("Create", CStrF("Internal error. (iE == %d/%d", iE, m_lEdges.Len()));
	if (iiV != m_liVertices.Len())
		Error("Create", CStrF("Internal error. (iiV == %d/%d", iiV, m_liVertices.Len()));
	if (iF != m_lFaces.Len())
		Error("Create", CStrF("Internal error. (iF == %d/%d", iF, m_lFaces.Len()));

	_lPlanes.OptimizeMemory();
}

#endif

void CXR_IndexedSolidContainer32::Create(TArray<spCXW_Surface>& _lspSurfaces, TArray<CXR_MediumDesc>& _lMediums, TAP_RCD<const CPlane3Dfp32> _pPlanes)
{
	m_lspSurfaces = _lspSurfaces;
	m_lMediums = _lMediums;
	m_pPlanes = _pPlanes;

	UpdateColinearDirections();
	UpdateObbs();
	UpdateFriction();
}

void CXR_IndexedSolidContainer32::GetSolid(int _iSolid, CXR_IndexedSolidDesc32& _Desc) const
{
	TAP_RCD<const CXR_IndexedSolid32> pSolids = m_lSolids;
	const CXR_IndexedSolid32& Solid = pSolids[_iSolid];

	TAP<const CVec3Dfp32> pV = m_lVertices;
	_Desc.m_pV.m_pArray = pV.m_pArray+Solid.m_iVertices;
	_Desc.m_pV.m_Len = Solid.m_nVertices;
	TAP<const CIndexedEdge16> pE = m_lEdges;
	_Desc.m_pE.m_pArray = pE.m_pArray+Solid.m_iEdges;
	_Desc.m_pE.m_Len = Solid.m_nEdges;
	TAP<const CXR_IndexedSolidFace32> pF = m_lFaces;
	int iFaces = Solid.m_iFaces;

	_Desc.m_pF.m_pArray = pF.m_pArray+iFaces;
	_Desc.m_pF.m_Len = Solid.m_nFaces;

	_Desc.m_pP.m_pArray = m_pPlanes.m_pArray;
	_Desc.m_pP.m_Len = m_pPlanes.Len();

	_Desc.m_piV.m_pArray = m_liVertices.GetBasePtr() + Solid.m_iiVertices;
	_Desc.m_piV.m_Len = Solid.m_niVertices;

	TAP_RCD<const uint16> piColinearP = m_liColinearPlanes;
	TAP_RCD<const CIndexedEdge16> pColinearE = m_lColinearEdges;

	_Desc.m_piColinearP.m_pArray = piColinearP.m_pArray + Solid.m_iColinearPlanes;
	_Desc.m_piColinearP.m_Len = Solid.m_nColinearPlanes;

	_Desc.m_pColinearE.m_pArray = pColinearE.m_pArray + Solid.m_iColinearEdges;
	_Desc.m_pColinearE.m_Len = Solid.m_nColinearEdges;

	_Desc.m_BoundBox = Solid.m_BoundBox;
	_Desc.m_Friction = Solid.m_Friction;
}

void CXR_IndexedSolidContainer32::Read(CDataFile* _pDFile)
{
	CCFile* pF = _pDFile->GetFile();

	// SOLIDS
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLIDS"))
	{
		READARRAY_VER(pF, m_lSolids, Read, _pDFile->GetUserData(), _pDFile->GetUserData2());
	}
	_pDFile->PopPosition();

	// SOLID_FACES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_FACES"))
	{
		READARRAY_VER(pF, m_lFaces, Read, _pDFile->GetUserData(), _pDFile->GetUserData2());
	}
	_pDFile->PopPosition();

	// SOLID_EDGES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_EDGES"))
	{
		READARRAY(pF, m_lEdges, Read, _pDFile->GetUserData());
	}
	_pDFile->PopPosition();

	// SOLID_VERTICES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_VERTICES"))
	{
		READARRAY(pF, m_lVertices, Read, _pDFile->GetUserData());
	}
	_pDFile->PopPosition();

	// SOLID_VERTEXINDICES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLID_VERTEXINDICES"))
	{
		m_liVertices.SetLen(_pDFile->GetUserData());
		pF->ReadLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
	}
	_pDFile->PopPosition();
}

void CXR_IndexedSolidContainer32::Write(CDataFile* _pDFile)
{
	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginEntry("SOLIDS");
	WRITEARRAY(pF, m_lSolids, Write);
	_pDFile->EndEntry(m_lSolids.Len(), XR_INDEXEDSOLID_VERSION);

	_pDFile->BeginEntry("SOLID_FACES");
	WRITEARRAY(pF, m_lFaces, Write);
	_pDFile->EndEntry(m_lFaces.Len(), XR_INDEXEDSOLIDFACE_VERSION);

	_pDFile->BeginEntry("SOLID_EDGES");
	WRITEARRAY(pF, m_lEdges, Write);
	_pDFile->EndEntry(m_lEdges.Len(), 0);

	_pDFile->BeginEntry("SOLID_VERTICES");
	WRITEARRAY(pF, m_lVertices, Write);
	_pDFile->EndEntry(m_lVertices.Len(), 0);

	_pDFile->BeginEntry("SOLID_VERTEXINDICES");
	pF->WriteLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
	_pDFile->EndEntry(m_liVertices.Len(), 0);
}

#define INDEXEDSOLID_COLINEAR_TOLERANCE (0.99f)

void CXR_IndexedSolidContainer32::UpdateColinearDirections()
{
	TAP_RCD<CXR_IndexedSolid32> pSolids = m_lSolids;

	//m_liColinearFaces.QuickSetLen();

	TArray<uint16> liColinearPlanes;
	TArray<CIndexedEdge16> lColinearEdges;

	CXR_IndexedSolidDesc32 Desc;
	for (int i = 0; i < pSolids.Len(); i++)
	{
		CXR_IndexedSolid32& Solid = pSolids[i];

		GetSolid(i, Desc);

		Solid.m_iColinearPlanes = liColinearPlanes.Len();
		Solid.m_nColinearPlanes = 0;

		for (int iF = 0; iF < Desc.m_pF.Len(); iF++)
		{
			const CXR_IndexedSolidFace32& Face = Desc.m_pF[iF];

			const CPlane3Dfp32& Plane = m_pPlanes[Face.m_iPlane];

			bool found = false;
			for (int iCP = 0; iCP < Solid.m_nColinearPlanes; iCP++)
			{
				int iP = Solid.m_iColinearPlanes + iCP;
				const CPlane3Dfp32& P = m_pPlanes[liColinearPlanes[iP]];
				fp32 D = P.n * Plane.n;

				if (M_Fabs(D) > INDEXEDSOLID_COLINEAR_TOLERANCE)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				liColinearPlanes.Add(Face.m_iPlane);
				Solid.m_nColinearPlanes++;
			}
		}

		Solid.m_iColinearEdges = lColinearEdges.Len();
		Solid.m_nColinearEdges = 0;

		for (int iE = 0; iE < Desc.m_pE.Len(); iE++)
		{
			const CIndexedEdge16& Edge = Desc.m_pE[iE];

			CVec3Dfp32 Dir = Desc.m_pV[Edge.m_liV[1]] - Desc.m_pV[Edge.m_liV[0]];
			Dir.Normalize();

			bool found = false;
			for (int iCE = 0; iCE < Solid.m_nColinearEdges; iCE++)
			{
				int iE2 = Solid.m_iColinearEdges + iCE;
				const CIndexedEdge16& E = lColinearEdges[iE2];
				CVec3Dfp32 CoDir = Desc.m_pV[E.m_liV[1]] - Desc.m_pV[E.m_liV[0]];
				CoDir.Normalize();

				fp32 D = Dir * CoDir;
				if (M_Fabs(D) > INDEXEDSOLID_COLINEAR_TOLERANCE)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				lColinearEdges.Add(Edge);
				Solid.m_nColinearEdges++;
			}
		}
	}

	m_liColinearPlanes.SetLen(liColinearPlanes.Len());
	for (int i = 0; i < liColinearPlanes.Len(); i++)
	{
		m_liColinearPlanes[i] = liColinearPlanes[i];
	}

	m_lColinearEdges.SetLen(lColinearEdges.Len());
	for (int i = 0; i < lColinearEdges.Len(); i++)
	{
		m_lColinearEdges[i] = lColinearEdges[i];
	}
}

static bool FindColinearPair(TArray<CPlane3Dfp32>& _lPlanes, CPlane3Dfp32& _PlaneA, CPlane3Dfp32& _PlaneB, fp32 _Epsilon)
{
	if (_lPlanes.Len() < 2) return false;

	CPlane3Dfp32 FirstP = _lPlanes[0];
	for (int i = 1; i < _lPlanes.Len(); i++)
	{
		CPlane3Dfp32 NextP = _lPlanes[i];
		if ((FirstP.n * NextP.n + 1.0f) < _Epsilon)
		{
			_PlaneA = FirstP;
			_PlaneB = NextP;
			_lPlanes.Del(i);
			_lPlanes.Del(0);
			return true;
		}
	}

	return false;
}

static bool IsOBB(TArray<CPlane3Dfp32>& _lPlanes, CPhysOBB &_Obb)
{
	if (_lPlanes.Len() != 6) return false;

	const fp32 Eps = 0.001f;

	CPlane3Dfp32 lPlanesA[3], lPlanesB[3];

	if (!FindColinearPair(_lPlanes, lPlanesA[0], lPlanesB[0], Eps)) return false;
	if (!FindColinearPair(_lPlanes, lPlanesA[1], lPlanesB[1], Eps)) return false;
	if (!FindColinearPair(_lPlanes, lPlanesA[2], lPlanesB[2], Eps)) return false;

	CVec3Dfp32 e[3];
	e[0] = lPlanesA[0].n;
	e[1] = lPlanesA[1].n;
	e[2] = e[0] / e[1];
	e[2].Normalize();

	CVec3Dfp32 Center(0.0f);
	for (int i = 0 ; i < 3; i++)
	{
		CVec3Dfp32 P1 = lPlanesA[i].GetPointInPlane();
		CVec3Dfp32 P2 = lPlanesB[i].GetPointInPlane();
		//fp32 d1 = M_Fabs(P1 * e[i]);
		//fp32 d1 = M_Fabs(P2 * e[i]);

		//Center += P1;
		//Center += P2;
		Center += (P1 + P2) * 0.5f;
	}
	//Center *= 1.0f / 6.0f;

	fp32 ext1 = M_Fabs(lPlanesA[0].Distance(Center));
	fp32 ext2 = M_Fabs(lPlanesA[1].Distance(Center));
	fp32 ext3 = M_Fabs(lPlanesA[2].Distance(Center));

	_Obb.m_A[0] = e[0];
	_Obb.m_A[1] = e[1];
	_Obb.m_A[2] = e[2];
	_Obb.m_C = Center;
	_Obb.m_E = CVec3Dfp32(ext1, ext2, ext3);

	return true;
}

void CXR_IndexedSolidContainer32::UpdateObbs()
{
	TAP_RCD<CXR_IndexedSolid32> pSolids = m_lSolids;

	//m_liColinearFaces.QuickSetLen();

	TArray<uint16> liColinearPlanes;
	TArray<CIndexedEdge16> lColinearEdges;

	m_lObbs.SetLen(pSolids.Len());
	m_lIsObbs.SetLen(pSolids.Len());

	CXR_IndexedSolidDesc32 Desc;
	for (int i = 0; i < pSolids.Len(); i++)
	{
		CXR_IndexedSolid32& Solid = pSolids[i];

		GetSolid(i, Desc);
		TArray<CPlane3Dfp32> lPlanes;
		for (int iF = 0; iF < Desc.m_pF.Len(); iF++)
		{
			const CXR_IndexedSolidFace32& Face = Desc.m_pF[iF];
			const CPlane3Dfp32& Plane = m_pPlanes[Face.m_iPlane];
			lPlanes.Add(Plane);
		}
		
		CPhysOBB Obb;
		bool bIsObb = IsOBB(lPlanes, Obb);
		m_lObbs[i] = Obb;
		m_lIsObbs[i] = bIsObb;
	}
}

void CXR_IndexedSolidContainer32::UpdateFriction()
{
	TAP_RCD<CXR_IndexedSolid32> pSolids = m_lSolids;

	TArray<uint16> liColinearPlanes;
	TArray<CIndexedEdge16> lColinearEdges;

	m_lObbs.SetLen(pSolids.Len());
	m_lIsObbs.SetLen(pSolids.Len());

	CXR_IndexedSolidDesc32 Desc;
	for (int i = 0; i < pSolids.Len(); i++)
	{
		CXR_IndexedSolid32& Solid = pSolids[i];

		GetSolid(i, Desc);
		fp32 Friction = 0.0f;
		for (int iF = 0; iF < Desc.m_pF.Len(); iF++)
		{
			const CXR_IndexedSolidFace32& Face = Desc.m_pF[iF];

			CXW_Surface *pSurface = m_lspSurfaces[Face.m_iSurface];
			Friction += pSurface->GetBaseFrame()->m_Friction;
		}

		M_ASSERT(Desc.m_pF.Len() > 0, "Invalid solid");
		Solid.m_Friction = Friction / Desc.m_pF.Len();
		int breakme = 0;
	}
}

void CXR_IndexedSolidContainer32::RelocateMediums(const uint16* _piMapping, int _nMapping)
{
}

