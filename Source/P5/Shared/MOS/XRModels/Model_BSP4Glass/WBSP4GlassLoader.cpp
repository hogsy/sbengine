#include "PCH.h"

#include "WBSP4Glass.h"
//#include "MFloat.h"
//#include "../../MSystem/Raster/MTextureContainers.h"

#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


void CXR_Model_BSP4Glass::Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const CBSP_CreateInfo& _CreateInfo)
{
	if(!_pDFile) Error("Create", "Datafile expected.");
	
	// Push data file position and read in mapping and face index mapping data
	_pDFile->PushPosition();

	CCFile* pFile = _pDFile->GetFile();

	if(!_pDFile->GetNext("VERSION"))
		Error("Read", "No VERSION entry.");
	const int XWVersion = _pDFile->GetUserData();
	if(XWVersion > XW_VERSION)
		Error("Read", CStrF("Invalid XW-version. %4x should be less or equal to %.4x", _pDFile->GetUserData(), XW_VERSION));

	// Read in face index mapping
	{
		GLASS_MSCOPE(m_lFaces, XR_BSPMODEL4);
		if (!_pDFile->GetNext("FACES")) Error("Read", "No FACES entry.");

		int nFaces = _pDFile->GetUserData();
		m_liFaceMapping.SetLen(nFaces);
		ReadFaceMapping(nFaces, pFile, _pDFile->GetUserData2());
	}

	// Read in mappings
	{
		if(!_pDFile->GetNext("MAPPINGS")) Error("Read", "No MAPPINGS entry.");
		m_lMappings.SetLen(_pDFile->GetUserData());
		for(int iMap = 0; iMap < m_lMappings.Len(); iMap++)
			m_lMappings[iMap].Read(pFile);
	}

	_pDFile->PopPosition();

	// Get glass version
	int32 GlassVersion = GLASS_VERSION_100;
	_pDFile->PushPosition();
	if(_pDFile->GetNext("GLASS_VERSION"))
		GlassVersion = _pDFile->GetUserData();
	_pDFile->PopPosition();

	if(GlassVersion >= GLASS_VERSION_110)
	{
		// Read face instance data
		_pDFile->PushPosition();
		if(_pDFile->GetNext("GLASSFACEINSTANCE"))
		{
			m_liFaceInstance.SetLen(_pDFile->GetUserData());
			pFile->ReadLE(&m_liFaceInstance[0], m_liFaceInstance.Len());
		}
		_pDFile->PopPosition();

		// Read instance data
		_pDFile->PushPosition();
		if(_pDFile->GetNext("GLASSINDICES"))
		{
			int32 nIndices = _pDFile->GetUserData();
			if (GlassVersion >= GLASS_VERSION_120)
			{
				TThinArray<CBSP4Glass_IndexLoad> lIndices;
				lIndices.SetLen(nIndices);
				m_lGlassIndices.SetLen(nIndices);
				CBSP4Glass_IndexLoad* pIndicesLoad = lIndices.GetBasePtr();
				CBSP4Glass_Index* pIndices = m_lGlassIndices.GetBasePtr();
				for(int32 iIndice = 0; iIndice < nIndices; iIndice++)
				{
					pIndicesLoad[iIndice].Read(pFile);
					pIndices[iIndice] = pIndicesLoad[iIndice];
				}
			}
			else
			{
				m_lGlassIndices.SetLen(nIndices);
				CBSP4Glass_Index* pIndices = m_lGlassIndices.GetBasePtr();
				for (int32 iIndice = 0; iIndice < nIndices; iIndice++)
					pIndices[iIndice].Read(pFile);
			}
		}
		_pDFile->PopPosition();
	}

	CXR_Model_BSP4::Create(_pParam, _pDFile, _pFile, _CreateInfo);

	// Setup master model if we are using glass version 2
	if(GlassVersion >= GLASS_VERSION_110)
	{
		CXR_Model_BSP2* pMasterBSP2Model = (_CreateInfo.m_spMaster) ? safe_cast<CXR_Model_BSP2>((CXR_Model*)_CreateInfo.m_spMaster) : NULL;
		m_spMaster = pMasterBSP2Model;
	}

	// Post creation and information logging, for the moment
	#if GLASS_DEBUG
		// If this is an old version, setup a timer for post creation
		CMTime tm;
		if(GlassVersion == GLASS_VERSION_100)
		{
			M_TRACEALWAYS("Post creating glass...");
			TStart(tm);
		}

	#endif

	// If we are using an old version we need to post create some data for the glass bsp
	if(GlassVersion == GLASS_VERSION_100)
		PostCreate();

	#if GLASS_DEBUG
		// Only log postcreation timing for old version since it's not done in newer versions
		if(GlassVersion == GLASS_VERSION_100)
		{
			TStop(tm);
			M_TRACEALWAYS(" Done! (%.1f seconds)\n", tm.GetTime());
		}

		uint nIndices = m_lGlassIndices.Len();
		uint nFaces = m_lFaces.Len();
		fp32 MemNodes = fp32(sizeof(CBSP4Glass_Index) * nIndices) / 1024.0f;
		fp32 MemFaces = fp32(sizeof(uint16) * nFaces) / 1024.0f;
		uint HiVersion = (GlassVersion >> 8) & 0xff;
		uint LoVersion = (GlassVersion & 0xff);
		aint MasterModel = ((_CreateInfo.m_spMaster) ? ((aint)((CXR_Model*)_CreateInfo.m_spMaster)) : 0);
		uint Flags = _CreateInfo.m_Flags;
		uint iSubModel = _CreateInfo.m_iSubModel;
		uint nSubModels = _CreateInfo.m_nSubModels;
		M_TRACEALWAYS("   Glass Version(%x.%.2x), pParam(\"%s\"), CreateInfo(Flags(%d), iSubModel(%d), nSubModels(%d), spMaster(0x%.8x))\n   Mem(Nodes(%d, %.2f KiB), Faces(%d, %.2f KiB))\n",
			HiVersion, LoVersion, _pParam, Flags, iSubModel, nSubModels, MasterModel, nIndices, MemNodes, nFaces, MemFaces);
	#endif
}


void CXR_Model_BSP4Glass::CreateLinkContext(const CMat4Dfp32& _WMat)
{
	if(m_spMaster)
	{
		// Fetch number of glass indices, approximate number of links and fetch structured leaves and glass indices pointer
		const int nIndices = m_lGlassIndices.Len();
		const int nLinks = ((nIndices + 31) & ~31) * 2;
		const int nPL = m_spMaster->m_nStructureLeaves;
		CBSP4Glass_Index* pGlassIndices = m_lGlassIndices.GetBasePtr();

		m_spLinkContext = MNew(CBSP4Glass_LinkContext);
		if(!m_spLinkContext)
		{
			M_TRACEALWAYS("Glass: Failed to create link context. Reverting to expensive rendering!\n");
			return;
		}

		m_spLinkContext->Create(m_spMaster, nIndices+1, nLinks, nPL, true);

		// Walk through all indices and add them to the link context
		for(uint i = 0, j = 1; i < nIndices; i++, j++)
		{
			// Transform glass index bounding box to world space
			CBox3Dfp32 BBoxW;
			pGlassIndices[i].m_BoundingBox.Transform(_WMat, BBoxW);

			// Link
			m_spLinkContext->Insert(j, BBoxW, 0);
		}
	}
#ifndef M_RTM
	else
	{
		M_TRACEALWAYS("Glass: Mo master model available. Reverting to expensive rendering!\n");
	}
#endif
}


void CXR_Model_BSP4Glass::PostCreate()
{
	// Bind faces to instances
	const int& nFaces = m_lFaces.Len();

	m_liFaceInstance.SetLen(nFaces);
	m_piFaceInstance = m_liFaceInstance.GetBasePtr();
	TAP_RCD<uint16> liFaceIndices(m_liFaceInstance);
	//m_liFaceInstance.GetBasePtr();

	// Setup instances
	uint nInstance = 0;
	uint nUnlinked = nFaces;
	for(uint i = 0; i < nFaces; i++)
	{
		const CBSP4_Face& Face = m_pFaces[i];
		liFaceIndices[i] = GLASS_MAX_INSTANCE;

		// If this is the actual building face for glass
		if(m_lspSurfaces[Face.m_iSurface]->m_Name.CompareNoCase("*GLASS"))
		{
			liFaceIndices[i] = nInstance++;
			nUnlinked--;
		}
	}

	//M_TRACEALWAYS("Initalized %d base instances\n", nInstance);
	//M_TRACEALWAYS("Links remaining to fix: %d\n", nUnlinked);

	// Loop until all faces are connected. This is not allowed to fail!
	uint nLast = ~0;
    while(nUnlinked != 0 || nLast != 0)
	{
		nLast = 0;
		for(uint i = 0; i < nFaces; i++)
		{
			// Skip if no source exist
			if(liFaceIndices[i] == GLASS_MAX_INSTANCE) continue;

			// Try to link this face to an instance
			const CBSP4_Face& SrcFace = m_pFaces[i];
			for(int j = 0; j < nFaces; j++)
			{
				// Can't link with already linked or itself
				if(i==j || liFaceIndices[j] != GLASS_MAX_INSTANCE) continue;

				// Check if faces should link by testing shared vertices
				const CBSP4_Face& DstFace = m_pFaces[j];
				for(uint k = 0; k < SrcFace.m_nVertices; k++)
				{
					const int iSrcVertex = m_piVertices[SrcFace.m_iiVertices+k];
					uint l = 0;
					for(; l < DstFace.m_nVertices; l++)
					{
						const uint iDstVertex = m_piVertices[DstFace.m_iiVertices+l];
						if(iSrcVertex == iDstVertex)
						{
							// Link faces, and make sure we do a new loop
							//M_TRACEALWAYS("Linking face %d with source face %d\n", j, i);
							liFaceIndices[j] = liFaceIndices[i];
							nLast++;
							nUnlinked--;
							break;
						}
					}

					// If we found a link, break.
					if(l != DstFace.m_nVertices) break;
				}
			}
		}

		//M_TRACEALWAYS("Link Info:\n   Linked %d objects.\n   Remaining unlinked: %d\n", nLast, nUnlinked);

		// No more unlinked object, stop linkage procedure
		if(nUnlinked == 0) nLast = 0;
	}

	// Just spit out the link information
	//for(int i = 0; i < nFaces; i++)
	//{
	//	M_TRACEALWAYS("Face: %d\tInstance: %d\n", i, m_piFaceInstance[i]);
	//}

	const uint nNodes = m_lNodes.Len();
	//const int& nFaces = m_lFaces.Len();

	uint nIndices = 0;
	for(uint i = 0; i < nNodes; i++)
	{
		CBSP4_Node& Node = m_pNodes[i];
		if(Node.IsLeaf())
		{
			const uint iiFaces = Node.m_iiFaces;
			const uint nF = iiFaces + Node.m_nFaces;

			CBSP4Glass_Index GlassIndex;
			for(uint j = iiFaces; j < nF; j++)
			{
				const uint iFace = m_piFaces[j];
				const CBSP4_Face& Face = m_pFaces[iFace];

				if(m_lspSurfaces[Face.m_iSurface]->m_Name.CompareNoCase("*GLASS") == 0)
					continue;

				//M_TRACEALWAYS("Found glass node: %i\n", i);

				const uint iiVertices = Face.m_iiVertices;
				const uint nVertices = Face.m_nVertices;

				if(GlassIndex.m_nInstance == 0)
				{
					// First thing added
					const uint32* piVertices = m_piVertices + iiVertices;
					GlassIndex.m_BoundingBox = CBox3Dfp32(m_pVertices[piVertices[0]],m_pVertices[piVertices[0]]);
					for(int k = 1; k < nVertices; k++)
						GlassIndex.m_BoundingBox.Expand(m_pVertices[piVertices[k]]);

					//M_TRACEALWAYS("   Fetching face instance: ");
                    uint16 iInstance = 0;
					for(int k = 0; k < nFaces; k++)
					{
						if(m_lspSurfaces[m_pFaces[k].m_iSurface]->m_Name.CompareNoCase("*GLASS") == 0)
							continue;

						if(k == iFace)
							break;
						//{
						//	M_TRACEALWAYS("-=( %i )=-\n", k);
						//	break;
						//}

						//M_TRACEALWAYS("%i, ", k);
						iInstance++;
					}

					GlassIndex.m_iInstance = iInstance;
					GlassIndex.m_nInstance = 1;
				}
				else
				{
					const uint32* piVertices = m_piVertices + iiVertices;
					for(int k = 0; k < nVertices; k++)
						GlassIndex.m_BoundingBox.Expand(m_pVertices[piVertices[k]]);
					GlassIndex.m_nInstance++;
				}
			}

			if(GlassIndex.m_nInstance > 0)
			{
				GlassIndex.m_BoundingBox.GetCenter(GlassIndex.m_Center);
				GlassIndex.m_Radius = (GlassIndex.m_BoundingBox.m_Max - GlassIndex.m_BoundingBox.m_Min).Length();	//JK-NOTE: This is the largest possible diameter, not the radius

				m_lGlassIndices.SetLen(nIndices + 1);
				m_lGlassIndices[nIndices] = GlassIndex;
				nIndices++;
			}
		}
	}

	InitializeListPtrs();
}

void CXR_Model_BSP4Glass::ReadFaceMapping(const int& _nFaces, CCFile* _pFile, int _Version)
{
	switch(_Version)
	{
		case 0x0203:
		{
			struct
			{
				int32 m_iiEdges;

				uint32 m_VertexIO;
				uint32 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint32 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				uint32 m_LightInfoIO;
			} TempRead;

			for(int i = 0; i < _nFaces; i++)
			{
				_pFile->Read(&TempRead, sizeof(TempRead));
				::SwapLE(TempRead.m_iMapping);
				m_liFaceMapping[i] = TempRead.m_iMapping;
			}
		}
		break;

		case 0x0202:
		{
			struct
			{
				int32 m_iiEdges;

				M_BITFIELD2(uint32, m_iiVertices, 24, m_nVertices, 8) m_VertexIO;

				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint32 m_iiNormals;
				uint16 m_iBackMedium;
				uint16 m_iFrontMedium;

				M_BITFIELD2(uint32, m_iLightInfo, 28, m_nLightInfo, 4) m_LightInfoIO;
			} TempRead;

			for(int i = 0; i < _nFaces; i++)
			{
				_pFile->Read(&TempRead, sizeof(TempRead));
				::SwapLE(TempRead.m_iMapping);
				TempRead.m_VertexIO.SwapLE();

				m_liFaceMapping[i] = TempRead.m_iMapping;
			}
		}
		break;

		case 0x0201:
		{
			struct CIOFormat
			{
				uint32 m_iiEdges;

				uint32 m_iiVertices;
				uint16 m_nVertices;
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint16 m_iiNormals;
				uint16 m_iBackMedium;
				uint16 m_iFrontMedium;

				M_BITFIELD2(uint32, m_iLightInfo, 28, m_nLightInfo, 4) m_LightInfoIO;
			};

			CIOFormat Face;

			for(int i = 0; i < _nFaces; i++)
			{
				_pFile->Read(&Face, sizeof(Face));
				
				::SwapLE(Face.m_iMapping);
				m_liFaceMapping[i] = Face.m_iMapping;
			}
		}
		break;

		case 0x0200:
			// Continue down to 0x0000 purposly
		
		case 0x0000:
		{
			struct
			{
				uint32 m_iiVertices;
				uint16 m_nVertices;
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;
				uint16 m_iPlane;
				uint16 m_iiNormals;
				uint16 m_iBackMedium;
				uint16 m_iFrontMedium;
				uint16 m_iLightInfo;
				uint16 m_nLightInfo;
			} TempRead;

			for(int i = 0; i < _nFaces; i++)
			{
				uint32 iiEdges;

				if(_Version == 0x0200)
					_pFile->ReadLE(iiEdges);

				_pFile->Read(&TempRead, sizeof(TempRead));

				::SwapLE(TempRead.m_iMapping);
				m_liFaceMapping[i] = TempRead.m_iMapping;
			}
		}
		break;

		default:
			Error_static("ReadFaceMapping", CStrF("Invalid version: %.4x", _Version));
	}
}

void CXR_Model_BSP4Glass::InitializeListPtrs()
{
	//m_pGlassNodes		= (m_lGlassNodes.Len())		? &m_lGlassNodes[0]		: NULL;
	//m_piGlassNodes	= (m_liGlassNodes.Len())	? &m_piGlassNodes[0]	: NULL;

	// Setup pointers
	m_piFaceMapping		= (m_liFaceMapping.Len())	? &m_liFaceMapping[0]	: NULL;
	m_pMappings			= (m_lMappings.Len())		? &m_lMappings[0]		: NULL;
	m_piFaceInstance	= (m_liFaceInstance.Len())	? &m_liFaceInstance[0]	: NULL;
	m_pGlassIndices		= (m_lGlassIndices.Len())	? &m_lGlassIndices[0]	: NULL;

	CXR_Model_BSP4::InitializeListPtrs();
}

void CXR_Model_BSP4Glass_Instance::CreateVertexBufferData(const CGlassCreateInfo& _GlassInfo)
{
	const int nModelInfos = _GlassInfo.m_lModels.Len();
	const CGlassCreateInfo::SGlassCreateInfo* pModelInfo = _GlassInfo.m_lModels.GetBasePtr();

	m_lModels.SetLen(nModelInfos);
	CGlassModel* pModels = m_lModels.GetBasePtr();

	for(int i = 0; i < nModelInfos; i++)
	{
		const CGlassCreateInfo::SGlassCreateInfo& ModelInfo = pModelInfo[i];
		
		const int nV = ModelInfo.m_lV.Len();
		int nVerts = nV;

		CGlassModel& Model = pModels[i];
		
		// Set variables
		Model.m_Attrib.m_Plane = ModelInfo.m_Plane;
		Model.m_Attrib.m_Thickness = ModelInfo.m_Thickness;
        Model.m_iSurface = ModelInfo.m_iSurface;
		Model.m_nRenderMainPrim = ModelInfo.m_nRenderMainPrim;//FrontBackPrim = ModelInfo.m_nRenderFrontBackPrim;
		Model.m_nRenderEdgePrim = ModelInfo.m_nRenderEdgePrim;
		Model.m_lFrontP = ModelInfo.m_lFrontP;
		//Model.m_lBackP = ModelInfo.m_lBackP;
		Model.m_lEdgeP = ModelInfo.m_lEdgeP;
		Model.m_Attrib.m_TxtWidthInv = ModelInfo.m_TxtWidthInv;
		Model.m_Attrib.m_TxtHeightInv = ModelInfo.m_TxtHeightInv;
		Model.m_Attrib.m_Durability = ModelInfo.m_Durability;
		Model.m_Attrib.m_UserFlags = ModelInfo.m_UserFlags;
		
		if (!ModelInfo.m_bValid)
		{
			Model.m_Attrib.Attrib_SetBaseRender(false);
			Model.m_Attrib.Attrib_SetPhys(false);
			continue;
		}

		// Setup thickness attributes
		//if(Model.m_Attrib.Attrib_UseThickness())
			nVerts *= 2;
		//else
		if(!Model.m_Attrib.Attrib_UseThickness())
			Model.m_Attrib.m_Thickness = 0.0f;

		// Allocate data for model
		Model.m_lV.SetLen(nVerts);
		Model.m_lN.SetLen(nVerts);
		Model.m_lC.SetLen(nVerts);
		Model.m_lTV.SetLen(nVerts);
		Model.m_lTangU.SetLen(nVerts);
		Model.m_lTangV.SetLen(nVerts);
		
		CVec3Dfp32* pV = Model.m_lV.GetBasePtr();
		CPixel32*  pC = Model.m_lC.GetBasePtr();
		CVec3Dfp32* pN = Model.m_lN.GetBasePtr();
		CVec2Dfp32* pTV = Model.m_lTV.GetBasePtr();
		CVec3Dfp32* pTangU = Model.m_lTangU.GetBasePtr();
		CVec3Dfp32* pTangV = Model.m_lTangV.GetBasePtr();

		// Create solid shards (the glass)
		{
			CBox3Dfp32 Bound(_FP32_MAX, -_FP32_MAX);
			CVec2Dfp32 Min(_FP32_MAX);
			CVec2Dfp32 Max(-_FP32_MAX);
			CVec2Dfp32 Length;

			// Determine components to use
			const CVec3Dfp32 AbsN = CVec3Dfp32(M_Fabs(ModelInfo.m_Plane.n.k[0]), M_Fabs(ModelInfo.m_Plane.n.k[1]), M_Fabs(ModelInfo.m_Plane.n.k[2]));
			const uint8 liComps[6] = { 1, 2, 0, 2, 0, 1 };
			const uint8 iIgnoreComp = ((AbsN.k[0] > AbsN.k[1]) ? ((AbsN.k[0] > AbsN.k[2]) ? 0 : 2) : ((AbsN.k[1] > AbsN.k[2]) ? 1 : 2));
			const uint8 iComp1 = Model.m_Attrib.m_liComp[0] = liComps[(iIgnoreComp*2)+0];
			const uint8 iComp2 = Model.m_Attrib.m_liComp[1] = liComps[(iIgnoreComp*2)+1];

			CXR_PlaneMapping& PlaneMapping = *ModelInfo.m_pMapping;
			Model.m_pMapping = ModelInfo.m_pMapping;

			// Mapping and tangent data setup
			CBSP4Glass_MappingSetup Mapping(PlaneMapping, Model.m_Attrib);
			CBSP4Glass_TangentSetup Tangent(Mapping, Model.m_Attrib.m_Plane.n);
			const fp32 UOffset = Mapping.GetUOffset();
			const fp32 VOffset = Mapping.GetVOffset();
			const fp32 UVecLenSqrInv = Mapping.GetUVecLenSqrInv();
			const fp32 VVecLenSqrInv = Mapping.GetVVecLenSqrInv();
			const CVec3Dfp32& UVec = Mapping.GetUVec();
			const CVec3Dfp32& VVec = Mapping.GetVVec();
			const CVec3Dfp32& TangFrontU = Tangent.GetFrontSideTangU();
			const CVec3Dfp32& TangFrontV = Tangent.GetFrontSideTangV();
			const CVec3Dfp32& TangBackU = Tangent.GetBackSideTangU();
			const CVec3Dfp32& TangBackV = Tangent.GetBackSideTangV();
			const bool bUseThickness = Model.m_Attrib.Attrib_UseThickness();

			for(int k = 0, l = nV; k < nV; k++, l++)
			{
				const CVec3Dfp32& V = ModelInfo.m_lV[k];
				CVec2Dfp32 TV;
				{
					const fp32 UProj = (V.k[0]*UVec.k[0] + V.k[1]*UVec.k[1] + V.k[2]*UVec.k[2])*UVecLenSqrInv + UOffset;
					const fp32 VProj = (V.k[0]*VVec.k[0] + V.k[1]*VVec.k[1] + V.k[2]*VVec.k[2])*VVecLenSqrInv + VOffset;
					Mapping.ImproveProj(UProj, VProj);
					TV.k[0] = UProj * ModelInfo.m_TxtWidthInv;
					TV.k[1] = VProj * ModelInfo.m_TxtHeightInv;
				}

				// Setup vertex data
				pN[k] = Model.m_Attrib.m_Plane.n;
				pV[k] = V;
				pC[k] = 0xffffffff;
				pTangU[k] = TangFrontU;
				pTangV[k] = TangFrontV;
				pTV[k] = TV;

				// Dual sided
				//if(bUseThickness)
				{
					pN[l] = -pN[k];
					pV[l] = V + (pN[l] * Model.m_Attrib.m_Thickness);
					pC[l] = 0xffffffff;
					pTangU[l] = TangBackU;
					pTangV[l] = TangBackV;
					pTV[l] = TV;
				}

				Bound.Expand(V);
				Min.k[0] = MinMT(Min.k[0], V.k[iComp1]);
				Min.k[1] = MinMT(Min.k[1], V.k[iComp2]);
				Max.k[0] = MaxMT(Max.k[0], V.k[iComp1]);
				Max.k[1] = MaxMT(Max.k[1], V.k[iComp2]);
			}

			// Mapping pass
			{
				CVec2Dfp32 TMid = Mapping.GetMid();
                for(int k = 0; k < nVerts; k++)
					pTV[k] -= TMid;
			}

			// Get middle point of glass
			Model.m_Attrib.m_MiddlePoint = Bound.m_Min + ((Bound.m_Max - Bound.m_Min) / 2);

			Length = (Max - Min);
			
			Model.m_Attrib.m_liComp[0] = iComp1;
			Model.m_Attrib.m_liComp[1] = iComp2;

			Model.m_Attrib.m_DimRcp[0] = 1.0f / Length[0];
			Model.m_Attrib.m_DimRcp[1] = 1.0f / Length[1];

			Model.m_Attrib.m_Min = Min;
		}

		// After vertices have been fetched,
		/*
		if(nV > 0)
		{	
			CBox3Dfp32 Bound;
			CVec2Dfp32 Min, Max, Length;
			Min = Max = CVec2Dfp32(pV[0].k[iComp1], pV[0].k[iComp2]);
			Bound = CBox3Dfp32(pV[0], pV[0]);
			for(int j = 1; j < nV; j++)
			{
				Bound.Expand(pV[j]);
				Min.k[0] = MinMT(Min.k[0], pV[j].k[iComp1]);
				Min.k[1] = MinMT(Min.k[1], pV[j].k[iComp2]);

				Max.k[0] = MaxMT(Max.k[0], pV[j].k[iComp1]);
				Max.k[1] = MaxMT(Max.k[1], pV[j].k[iComp2]);
			}

			// Get middle point of glass
			Model.m_Attrib.m_MiddlePoint = Bound.m_Min + ((Bound.m_Max - Bound.m_Min) / 2);

			Length = (Max - Min);
			
			Model.m_Attrib.m_liComp[0] = iComp1;
			Model.m_Attrib.m_liComp[1] = iComp2;

			Model.m_Attrib.m_DimRcp[0] = 1.0f / Length[0];
			Model.m_Attrib.m_DimRcp[1] = 1.0f / Length[1];

			Model.m_Attrib.m_Min = Min;
		}
		*/
	}
}

