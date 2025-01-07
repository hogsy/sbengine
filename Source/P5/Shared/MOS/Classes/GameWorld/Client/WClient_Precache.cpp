#include "PCH.h"

#include "WClient_Core.h"
#include "../WPackets.h"
#include "../WDataRes_Core.h"
#include "../FrontEnd/WFrontEnd.h"
#include "../../GameContext/WGameContext.h"
#include "../../../XR/XRVBContext.h"
#include "../../../XR/XREngineVar.h"
#include "../../../XRModels/Model_TriMesh/WTriMesh.h"  // for DumpGeometryUsage
#include "../../../XRModels/Model_BSP2/WBSP2Model.h"  // for DumpGeometryUsage



void CWorld_ClientCore::Precache_Init()
{
	MAUTOSTRIP(CWorld_ClientCore_Precache_Init, MAUTOSTRIP_VOID);
	if (m_ClientMode & WCLIENT_MODE_MIRROR)
		return;

//	M_TRY
//	{
		ConOutL("(CWorld_ClientCore::Precache_Init)");
		m_Precache = PRECACHE_DONE+1;
		m_Precache_iItem = 0;
		m_Precache_nItems = 0;
		m_Precache_nItemsDone = 0;
		m_Precache_ResourceID = 0;
		m_PrecacheProgress = 0;
		m_LoadStatus = 1;

		if(Render_GetEngine())
		{
			m_Precache_OldSurfOptions = Render_GetEngine()->m_SurfOptions;
			Render_GetEngine()->m_SurfOptions &= ~XW_SURFOPTION_NOGORE;
		}
		else
			m_Precache_OldSurfOptions = -1;

		// Remove precache flag from all textures
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if (pTC)
		{
			int nTxt = pTC->GetIDCapacity();
			for(int i = 0; i < nTxt; i++)
			{
				int Flags = pTC->GetTextureFlags(i);
				if (Flags & CTC_TXTIDFLAGS_RESIDENT)
				{
						pTC->SetTextureParam(i, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
				}
				else if (Flags & CTC_TXTIDFLAGS_PRECACHE)
				{
	//				CStr Name = pTC->GetName(i);
	//				ConOutL(CStrF("§cf80WARNING: Illegally precached texture: %s", Name.Str()));
					pTC->SetTextureParam(i, CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_PRECACHE);
				}
			}
		}

		// Remove precache flag from all waves
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if (pWC)
		{
			pWC->ClearWaveIDFlags(CWC_WAVEIDFLAGS_PRECACHE);
		}

		// Remove precache flag from all vertex buffers
		CXR_VBContext* pVBC = Render_GetEngine()->m_pVBC;
		if (pVBC)
		{
			int nVB = pVBC->GetIDCapacity();
			for(int i = 0; i < nVB; i++)
			{
				int Flags = pVBC->VB_GetFlags(i);
				if (Flags & CXR_VBFLAGS_PRECACHE)
				{
					pVBC->VB_SetFlags(i, Flags & ~CXR_VBFLAGS_PRECACHE);
				}
			}
		}


		// Set XR Mode
		int XRMode = 0;
		if (m_spWorldSpawnKeys != NULL)
			XRMode = m_spWorldSpawnKeys->GetValuei("XR_MODE", 0);

		Render_GetEngine()->SetVar(XR_ENGINE_MODE, XRMode);


		// Precache debugfont
	#ifndef PLATFORM_CONSOLE
		if (m_spGUIData != NULL)
		{
			CRC_Font* pFont = m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("MONOPRO"));
			pTC->SetTextureParam(pFont->m_TextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	#endif

		m_spWData->Resource_WorldNeedXDF();

		Render_GetEngine()->OnPrecache();

		if (m_ClientState == WCLIENT_STATE_CHANGELEVEL)
		{
		}
/*	}
	M_CATCH(
	catch(CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		m_Precache = PRECACHE_DONE;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch(CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		m_Precache = PRECACHE_DONE;
	}
	)
#endif*/
	
}

void CWorld_ClientCore::DumpGeometryUsage(CStr _Filename, bool _bAppend)
{
	MAUTOSTRIP(CWorld_ClientCore_DumpGemoetryUsage, MAUTOSTRIP_VOID);
	if(m_WorldName.CompareNoCase("campaign") == 0)
		return;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys || !pSys->m_spDisplay)
		return;
	
	CRCLock RCLock;
	CRenderContext *pRC = pSys->m_spDisplay->GetRenderContext(&RCLock);
	if(!pRC)
		return;

	M_TRY
	{

		CCFile File;
		File.Open(_Filename, CFILE_WRITE | (_bAppend ? CFILE_APPEND : 0));

		MACRO_GetRegisterObject(CXR_VBContext, pVBC, "SYSTEM.VBCONTEXT");
		if (!pVBC) 
			Error("Engine_Render", "No VB-context in system.");

		if (pVBC)
		{
			File.Writeln("");
			File.Writeln("");
			File.Writeln("Geometry:");
			File.Writeln("");

			// First, count mem of all VBs to get summary
			int AllMem = 0;
			uint nVB = pVBC->GetIDCapacity();
			for (uint iVB = 0; iVB < nVB; iVB++)
			{
				if (pVBC->VB_GetFlags(iVB) & CXR_VBFLAGS_PRECACHE)
					AllMem += pRC->Geometry_GetVBSize(iVB);
			}
			File.Writeln("  ----------------------------------------------------------------------------------------");
			File.Writeln(CStrF("  All Geometry: %i KB", AllMem / 1024));
			File.Writeln("  ----------------------------------------------------------------------------------------");

			// Then, go through all model resources and count their VBMem
			TBitArray<10000> lUsedVBs;
			lUsedVBs.Clear();
			CWorldData* pWD = m_spMapData->m_spWData;
			uint nRc = pWD->Resource_GetMaxIndex();
			for (uint iRc = 0; iRc < nRc; iRc++)
			{
				CWRes_Model* pRes = TDynamicCast<CWRes_Model>( pWD->GetResource(iRc) );
				CXR_Model* pModel = pRes ? pRes->GetModel() : NULL;
				if (!pModel)
					continue;

				// get real model
				CXR_AnimState Anim;
				pModel = pModel->OnResolveVariationProxy(Anim, Anim);

				CXR_Model* lpModels[100] = { pModel };
				uint nModels = 1;

				if (pModel->GetModelClass() == CXR_MODEL_CLASS_TRIMESH)
				{ // add lods
					CXR_Model_TriangleMesh* pTriMesh = safe_cast<CXR_Model_TriangleMesh>(pModel);
					TAP<spCXR_Model_TriangleMesh> pLODs = pTriMesh->m_lspLOD;
					for (uint iLOD = 0; iLOD < pLODs.Len(); iLOD++)
						lpModels[nModels++] = pLODs[iLOD];
				}

				int TotModelSize = 0;
				for (uint iModel = 0; iModel < nModels; iModel++)
				{
					//NOTE: I would like to do a dynamic cast to catch all models that inherit from CXR_VBContainer, but not all TDynamicCast implementations work with CXR_VBContainer
					//                                                       (TODO: perhaps create a new interface: CXR_MODEL_INTERFACE_VBCONTAINER?)
					CXR_VBContainer* pVBContainer = NULL;
					switch (lpModels[iModel]->GetModelClass())
					{
					case CXR_MODEL_CLASS_TRIMESH:
						pVBContainer = safe_cast<CXR_Model_TriangleMesh>(lpModels[iModel]);
						break;
					case CXR_MODEL_CLASS_BSP2:
						pVBContainer = safe_cast<CXR_Model_BSP2>(lpModels[iModel]);
						break;
					default:
						continue;
					}

					uint nLocal = pVBContainer->GetNumLocal();
					for (uint i = 0; i < nLocal; i++)
					{
						int VBID = pVBContainer->GetID(i);
						if (!lUsedVBs.Get(VBID))
						{
							lUsedVBs.Set1(VBID);
							int VBSize = pRC->Geometry_GetVBSize(VBID);
							TotModelSize += VBSize;
							AllMem -= VBSize;
						}
					}
				}

				if (TotModelSize)
				{
					const char* pResName = pRes->GetName();
					File.Writeln(CStrF("  %-70s %6i KB", pResName, TotModelSize / 1024));
				}
			}

			// Finally, write summary of unknown vb mem (not found while looking in resources)
			File.Writeln(CStrF("  %-70s %6i KB", "Other", AllMem / 1024));
			File.Writeln("  ----------------------------------------------------------------------------------------");
			File.Writeln("");
		}
					
		File.Close();
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
}

void CWorld_ClientCore::DumpSoundUsage(CStr _Filename, bool _bAppend)
{
	MAUTOSTRIP(CWorld_ClientCore_DumpSoundUsage, MAUTOSTRIP_VOID);
	if(m_WorldName.CompareNoCase("campaign") == 0)
		return;

	CSoundContext *pSC = m_spSound;

	if(!pSC)
		return;

	M_TRY
	{
		CCFile File;
		File.Open(_Filename, CFILE_WRITE | (_bAppend ? CFILE_APPEND : 0));
		
		int AllMem = 0;
		
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if(pWC)
		{		
			int i;
			int nWav = pWC->GetIDCapacity();
			for(i = 0; i < nWav; i++)
				if(pWC->GetWaveIDFlag(i) & CWC_WAVEIDFLAGS_PRECACHE)
				{				
					AllMem += pSC->Wave_GetMemusage(i);
				}

			File.Writeln("");
			File.Writeln("");
			File.Writeln("Waves:");
			File.Writeln("");

			File.Writeln("  --------------------------------------------");
			File.Writeln(CStrF("  All waves: %i KB", AllMem / 1024));
			File.Writeln("  --------------------------------------------");
			File.Writeln("");
				
			for(i = 0; i < nWav; i++)
			{
				if(pWC->GetWaveIDFlag(i) & CWC_WAVEIDFLAGS_PRECACHE)
				{
					int iLocal;
					CWaveContainer *pContainer = pWC->GetWaveContainer(i, iLocal);
					CWaveContainer_Plain *pContainerPlain = NULL;
					if (pContainer)
						pContainerPlain = TDynamicCast<CWaveContainer_Plain >(pContainer);

					if (pContainerPlain)
					{
						CWaveData WaveData;
						pWC->GetWaveLoadedData(i, WaveData);
						
						File.Writeln(CStrF("  %-23s Space in memory: %5i KB Channels: %2i Length: %4.1fs SampleRate: %6i Streamed(%d) ADPCM(%d)", pContainerPlain->GetName(iLocal), pSC->Wave_GetMemusage(i) / 1024,
							WaveData.GetChannels(), (fp32)WaveData.GetNumSamples() / (fp32)WaveData.GetSampleRate(), WaveData.GetSampleRate(), (WaveData.Get_Flags() & ESC_CWaveDataFlags_Stream) != 0, !WaveData.GetCanDecode()
							));
					}
					else
					{
						File.Writeln(CStrF("  %-23s %4i KB", "Unknown", pSC->Wave_GetMemusage(i) / 1024));
					}
				}
			}

			File.Writeln("");
		}
					
		File.Close();

	}
	M_CATCH(
	catch(CCException)
	{

	}
	)
	/*
	{
		CCFile File;
		File.Open(_Filename.GetPath() + "_Textures_Log.txt", CFILE_WRITE | CFILE_APPEND);
		CFStr sPicmip;
		CFStr PicmipSize;
		for(int i = 0; i < 8; i++)
		{
			if(i != 0)
			{
				sPicmip += ",";
				PicmipSize += ",";
			}
			sPicmip += CFStrF("%i", pRC->Texture_GetPicmipFromGroup(i));
			PicmipSize += CFStrF("%6.i KB", PicmipMem[i] / 1024);
		}

		File.Writeln(CStrF("%-20s Picmip: %s  %6.s MB,  PicmipMem %s", 
			m_WorldName.Str(), sPicmip.Str(), CStr::GetFilteredString(fp32(AllMem) / 1024 / 1024, 2).Str(),
			PicmipSize.Str() ));
		File.Close();
	}
	*/
}

void CWorld_ClientCore::DumpTextureUsage(CStr _Filename, bool _bAppend, bool _bIncludeNonPrecached)
{
	MAUTOSTRIP(CWorld_ClientCore_DumpTextureUsage, MAUTOSTRIP_VOID);
	if(m_WorldName.CompareNoCase("campaign") == 0)
		return;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys || !pSys->m_spDisplay)
		return;
	
	CRCLock RCLock;
	CRenderContext *pRC = pSys->m_spDisplay->GetRenderContext(&RCLock);
	if(!pRC)
		return;
	
	M_TRY
	{
		CCFile File;
		File.Open(_Filename, CFILE_WRITE | (_bAppend ? CFILE_APPEND : 0));
		
		int AllMemBestCase = 0;
		int AllMemWorstCase = 0;
		int AllMemTheoretical = 0;
		int PicmipMemBestCase[16];
		int PicmipMemWorstCase[16];
		int PicmipMemTheoretical[16];
		int PicmipNum[16];
		MemSet(PicmipMemBestCase, 0, sizeof(PicmipMemBestCase));
		MemSet(PicmipMemWorstCase, 0, sizeof(PicmipMemWorstCase));
		MemSet(PicmipMemTheoretical, 0, sizeof(PicmipMemTheoretical));
		MemSet(PicmipNum, 0, sizeof(PicmipNum));
		
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if(pTC)
		{		
			int i;
			int nTxt = pTC->GetIDCapacity();
			for(i = 0; i < nTxt; i++)
				if (_bIncludeNonPrecached || (pTC->GetTextureFlags(i) & CTC_TXTIDFLAGS_PRECACHE))
				{				
					CTC_TextureProperties Properties;
					pTC->GetTextureProperties(i, Properties);
					CRC_TextureMemoryUsage TextureSize = pRC->Texture_GetMem(i);
					if (TextureSize.m_BestCase <= 1)
						continue;

					PicmipMemBestCase[Properties.m_iPicMipGroup] += TextureSize.m_BestCase;
					PicmipMemWorstCase[Properties.m_iPicMipGroup] += TextureSize.m_WorstCase;
					PicmipMemTheoretical[Properties.m_iPicMipGroup] += TextureSize.m_Theoretical;
					PicmipNum[Properties.m_iPicMipGroup]++;
				}

			int p;
			File.Writeln("----------------------------------------------");
			File.Writeln("Texture summary:");
			File.Writeln("");
			for(p = 0; p < 16; p++)
				if(PicmipNum[p])
				{
					File.Writeln(CStrF("  Picmipgroup %2i, Picmip %i    BC(%3i%%): %8i KB    WC(%3i%%): %8i KB    T: %8i KB", p, pRC->Texture_GetPicmipFromGroup(p), 
						int((fp32(PicmipMemBestCase[p] - PicmipMemTheoretical[p]) / fp32(PicmipMemTheoretical[p])) * 100.0), PicmipMemBestCase[p] / 1024, 
						int((fp32(PicmipMemWorstCase[p] - PicmipMemTheoretical[p]) / fp32(PicmipMemTheoretical[p])) * 100.0), PicmipMemWorstCase[p] / 1024, 
						PicmipMemTheoretical[p] / 1024));
					AllMemBestCase += PicmipMemBestCase[p];
					AllMemWorstCase += PicmipMemWorstCase[p];
					AllMemTheoretical += PicmipMemTheoretical[p];
				}
			File.Writeln("  --------------------------------------------");
			File.Writeln(CStrF("  All textures                BC(%3i%%): %8i KB    WC(%3i%%): %8i KB    T: %8i KB", 
				int((fp32(AllMemBestCase - AllMemTheoretical) / fp32(AllMemTheoretical)) * 100.0), AllMemBestCase / 1024, 
				int((fp32(AllMemWorstCase - AllMemTheoretical) / fp32(AllMemTheoretical)) * 100.0), AllMemWorstCase / 1024, 
				AllMemTheoretical / 1024));
				
			File.Writeln("");
			File.Writeln("");
			File.Writeln("Textures:");
			File.Writeln("");
			for(p = 0; p < 16; p++)
			{
				if(PicmipNum[p])
				{
					File.Writeln(CStrF("  Picmipgroup %2i, Picmip %i    BC(%3i%%): %8i KB    WC(%3i%%): %8i KB    T: %8i KB", p, pRC->Texture_GetPicmipFromGroup(p), 
						int((fp32(PicmipMemBestCase[p] - PicmipMemTheoretical[p]) / fp32(PicmipMemTheoretical[p])) * 100.0), PicmipMemBestCase[p] / 1024, 
						int((fp32(PicmipMemWorstCase[p] - PicmipMemTheoretical[p]) / fp32(PicmipMemTheoretical[p])) * 100.0), PicmipMemWorstCase[p] / 1024, 
						PicmipMemTheoretical[p] / 1024));
					File.Writeln("  -------------------------------------------------------");
					for(i = 0; i < nTxt; i++)
						if (_bIncludeNonPrecached || (pTC->GetTextureFlags(i) & CTC_TXTIDFLAGS_PRECACHE))
						{
							CTC_TextureProperties Properties;
							pTC->GetTextureProperties(i, Properties);
							CRC_TextureMemoryUsage TextureSize = pRC->Texture_GetMem(i);
							if (TextureSize.m_BestCase <= 1)
								continue;

							if(Properties.m_iPicMipGroup == p)
							{
								CImage Img;
								int nMip;
								pTC->GetTextureDesc(i, &Img, nMip);
								CStr Name = pTC->GetName(i);
								CStr Res = CStrF("%ix%i", Img.GetHeight(), Img.GetPixelSize() * 8);
								int8 iOfs = (int8)Properties.m_PicMipOffset;
								if(iOfs > 0)
									Res += CStrF(" /%i", 1 << iOfs);
								else if(iOfs < 0)
									Res += CStrF(" *%i", 1 << (-iOfs));

								if (TextureSize.m_Theoretical == 0)
									TextureSize.m_Theoretical = 1;
								if (TextureSize.m_WorstCase == 0)
									TextureSize.m_WorstCase = 1;
								if (TextureSize.m_BestCase == 0)
									TextureSize.m_BestCase = 1;

								File.Writeln(CStrF("  %-23s %4ix%-14s BC(%3i%%): %5i KB, WC(%3i%%): %5i KB, T: %5i KB, %-40s %s",
												Name.Str(), Img.GetWidth(), Res.Str(),
												int((fp32(TextureSize.m_BestCase - TextureSize.m_Theoretical) / fp32(TextureSize.m_Theoretical)) * 100.0),
												TextureSize.m_BestCase / 1024, 
												int((fp32(TextureSize.m_WorstCase - TextureSize.m_Theoretical) / fp32(TextureSize.m_Theoretical)) * 100.0),
												TextureSize.m_WorstCase / 1024, 
												TextureSize.m_Theoretical / 1024, 
												Properties.GetFlagsString().Str(), TextureSize.m_pFormat));
							}
						}
						File.Writeln("");
				}
			}
		}
					
		File.Close();

		{
			CCFile File;
			CStr Path = _Filename.GetPath() + "_Textures_Log.txt";
			if(CDiskUtil::FileExists(Path))
				File.Open(Path, CFILE_WRITE | CFILE_APPEND);
			else
			{
				File.Open(Path, CFILE_WRITE);
				File.Writeln("--- Name --------------------------- Picmip groups -------------- Total --- World ---- Char ---- Item -- WModel --- Flora ----- Sky - Effects ----- GUI --- World ---- Char ---- Item -- WModel --- Flora ----- Sky - Effects ----- GUI");
			}
			
			CFStr sPicmip;
			CFStr PicmipSize;
			for(int i = 0; i < 16; i++)
			{
				if(i != 0)
				{
					sPicmip += ",";
					PicmipSize += ",";
				}
				sPicmip += CFStrF("%i", pRC->Texture_GetPicmipFromGroup(i));
				PicmipSize += CFStrF("%6.i KB", PicmipMemBestCase[i] / 1024);
			}

			File.Writeln(CStrF("%-20s Picmip: %s  %6.2f MB,%s", 
				m_WorldName.Str(), sPicmip.Str(), fp32(AllMemBestCase) / 1024 / 1024,
				PicmipSize.Str() ));
			File.Close();
		}
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)

/*	{
		CCFile File;
		File.Open(_Filename.GetPath() + "AllTextures_Log.txt", CFILE_WRITE | CFILE_APPEND);
		CFStr sPicmip;
		int nTxt = pTC->GetIDCapacity();
		for(int i = 0; i < nTxt; i++)
		{
			CTC_TextureProperties Prop;
			pTC->GetTextureProperties(i, Prop);
			if(pTC->IsValidID(i) && Prop.m_iPicMip == 0)
				File.Writeln(pTC->GetName(i));
		}
		
		File.Close();
	}*/
}


void CWorld_ClientCore::Precache_Perform(CRenderContext* _pRC)
{
	if (m_Precache)
		Precache_Perform(_pRC, 1.0f);
}


void CWorld_ClientCore::XDFStart(const char* _pBaseName)
{
	// make sure that we catch any strange messages
	M_TRY
	{
		CStr XDFName = _pBaseName;

		// Only create XDL if we have a complete spawnmask
		if (D_MXDFCREATE == 2)
		{
			// We are being called to create the XDF
			MACRO_GetSystemEnvironment(pEnv);

			CStr XDFBase = pEnv->GetValue("XDF_PATH", m_spWData->ResolvePath("XDF/"));
			CStr XDFPath = XDFBase + "/" + XDFName + ".XDL";
			if (!CDiskUtil::FileExists(XDFPath))
			{
				CDiskUtil::XDF_Record(XDFPath, m_spWData->ResolvePath(""));
			}
			else
			{
				ConOutL(CStrF("Skippng creating of XDL file, because it already exists (%s.XDL)", XDFName.Str()));
			}
		}
		else
		{
			CStr XDFPath = m_spWData->ResolveFileName("XDF/" + XDFName);
			if (CDiskUtil::FileExists(XDFPath))
				CDiskUtil::XDF_Use(XDFPath, m_spWData->ResolvePath(""));
		}
	}
	M_CATCH(
	catch(CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch(CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
	}
	)
#endif
}


void CWorld_ClientCore::XDFStop()
{
	CDiskUtil::XDF_Stop();
}


void CWorld_ClientCore::NextPrecacheStep()
{
	m_Precache_nItems = 0;
	m_Precache_iItem = 0;
	m_Precache_ResourceID = 0;
	m_PrecacheProgress = 0;
	m_Precache++; // next step
}


void CWorld_ClientCore::Precache_Perform(CRenderContext* _pRC, fp32 _dTime)
{
	MAUTOSTRIP(CWorld_ClientCore_Precache_Perform, MAUTOSTRIP_VOID);
	if (m_ClientMode & WCLIENT_MODE_MIRROR)
		return;

//	M_TRY
//	{
		CTextureContext* pTC = _pRC->Texture_GetTC();
		CXR_VBContext* pVBC = GetEngine()->m_pVBC;
		CMTime Time = GetTime();

		while(m_Precache != PRECACHE_DONE)
		{
			switch(m_Precache)
			{
			case PRECACHE_INIT:
				{
					// setup of precache (should be moved)
					if(_pRC)
					{
						_pRC->Render_PrecacheFlush();
						_pRC->Texture_PrecacheFlush();
						_pRC->Texture_PrecacheBegin( m_Precache_nItems );
					}

					// start using the common XDF
					XDFStart(m_WorldName + "_Precache");

					// Next step
					NextPrecacheStep();
				}
				break;

			case PRECACHE_ONPRECACHE :
				{
					int nObj = m_lspObjects.Len();
					for(int iObj = 0; iObj < nObj; iObj++)
					{
						m_PrecacheProgress = iObj;
						CWObject_Client* pObj = Object_Get(iObj);
						if (pObj)
							pObj->m_pRTC->m_pfnOnClientPrecache(pObj, this, Render_GetEngine());
					}

					MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
					if (pGame && pGame->m_spFrontEnd)
						pGame->m_spFrontEnd->OnPrecache(Render_GetEngine());

					NextPrecacheStep();
				}
				break;

			case PRECACHE_ONPRECACHECLASS :
				{
					if (m_spMapData)
					{
						int nRc = m_spMapData->GetNumResources();
						for(int iRc = 0; iRc < nRc; iRc++)
						{
							m_PrecacheProgress = iRc;
							CWResource* pRc = m_spMapData->GetResourceAsync(iRc);
							if (pRc && pRc->GetClass() == WRESOURCE_CLASS_WOBJECTCLASS)
							{
								CWRes_Class* pRcClass = (CWRes_Class*)pRc;
								pRcClass->GetRuntimeClass()->m_pfnOnClientPrecacheClass(this, Render_GetEngine());
							}
						}

						NextPrecacheStep();
					}
				}
				break;

			case PRECACHE_RESOURCES :
				{
					CXR_Engine* pEngine = Render_GetEngine();

					if (m_spMapData)
					{
						CWResource* pRc = m_spMapData->GetResourceAsync(m_Precache_ResourceID);
						if (pRc && pRc->m_Flags & WRESOURCE_PRECACHE)
						{
							int WRcID = m_spMapData->GetWorldResourceIndex(m_Precache_ResourceID);
							m_spWData->Resource_Load(WRcID);
							m_spWData->Resource_Precache(WRcID, pEngine);
						}

						m_Precache_ResourceID++;
						if (m_Precache_ResourceID >= m_spMapData->GetNumResources())
						{
							NextPrecacheStep();
						}
					}
					else
						NextPrecacheStep();
				}
				break;
				
			case PRECACHE_TEXTURES :
				{
//					int nTxt = pTC->GetIDCapacity();
					int TxtID = m_Precache_iItem;

					// If we just began caching textures, count the precache-taged textures so we can
					// display a proper progress bar.
					if (!TxtID)
					{
						m_PrecacheProgress = 0;
						m_pLastPrecacheTC = NULL;

						// Hack
	/*					MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
						if (pTC)
						{
							int nTxt = pTC->GetIDCapacity();
							for(int i = 0; i < nTxt; i++)
							{
								if (pTC->GetTextureFlags(i) & CTC_TXTIDFLAGS_WASPRECACHED)
									pTC->SetTextureFlags(i, pTC->GetTextureFlags(i) & (~(CTC_TXTIDFLAGS_WASPRECACHED)) | CTC_TXTIDFLAGS_PRECACHE);
							}
						}*/
						
						// Get the total number of textures that need precache.
						m_Precache_nItems = 0;
						if (pTC)
						{
							int nTxt = pTC->GetIDCapacity();
							m_Precache_lPrecacheOrder.SetLen(nTxt);
							for(int i = 0; i < nTxt; i++)
							{
								int Flags = pTC->GetTextureFlags(i);
								if (Flags & CTC_TXTIDFLAGS_PRECACHE)
								{
									if (!(Flags & CTC_TXTIDFLAGS_ALLOCATED))
									{
										CStr Name = pTC->GetName(i);
										M_TRACE("Texture not allocated %d, %s\n", i, Name.Str());
										M_ASSERT(0, "!");
										pTC->SetTextureParam(i, CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_PRECACHE);
										continue;
									}

									CTC_TextureProperties Properties;
									pTC->GetTextureProperties(i, Properties);
									if (Properties.m_Flags & CTC_TEXTUREFLAGS_RENDER)
									{
										if(!(Flags & CTC_TXTIDFLAGS_RESIDENT))
										{
											CStr Name = pTC->GetName(i);
											M_TRACE("Precache on render texture %d, %s\n", i, Name.Str());
											M_ASSERT(0, "!");
										}
										pTC->SetTextureParam(i, CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_PRECACHE);
										continue;
									}

									m_Precache_lPrecacheOrder[m_Precache_nItems] = i;
									m_Precache_nItems++;
								}
							}
							m_Precache_lPrecacheOrder.SetLen(m_Precache_nItems);
							m_Precache_lPrecacheOrder.QSort<CPrecacheTextureCompare>(pTC, m_Precache_nItems);

							ConOutL(CStrF("§c0f0NOTE: (CWorld_ClientCore::Precache_Perform) %d textures to precache.", m_Precache_nItems));
						}
					}
					
					m_PrecacheProgress++;

					if (m_Precache_iItem < m_Precache_nItems)
					{
						int iPrecache = m_Precache_lPrecacheOrder[m_Precache_iItem];
						
						if(_pRC)
						{
							CTextureContainer *pTContainer = pTC->GetTextureContainer(iPrecache);

							if (pTContainer != m_pLastPrecacheTC)
							{
								if (m_pLastPrecacheTC)
									m_pLastPrecacheTC->ClosePrecache();
								if (pTContainer)
									pTContainer->OpenPrecache();
								m_pLastPrecacheTC = pTContainer;
							}
							
							_pRC->Texture_Precache(iPrecache);
						}
					}

					m_Precache_nItemsDone++;
					m_Precache_iItem++;

					if (m_Precache_iItem >= m_Precache_nItems)
					{
						if(_pRC)
						{
							if (m_pLastPrecacheTC)
								m_pLastPrecacheTC->ClosePrecache();
							m_pLastPrecacheTC = NULL;
							_pRC->Texture_PrecacheEnd();
						}

						NextPrecacheStep();
					}
				}
				break;
				
			case PRECACHE_SOUND:
				{
					if (!m_spSound)
					{
						m_Precache++;
						break;
					}

					MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
					if (!pWC)
						Error("Precache", "Could not get wavecontext");

					int WaveID = m_Precache_iItem;

					// If we just began caching textures, count the precache-taged textures so we can
					// display a proper progress bar.
					int nWaves = pWC->GetIDCapacity();
					if (!WaveID)
					{
						m_PrecacheProgress = 0;
						m_spSound->Wave_PrecacheFlush( );
						// Get the total number of textures that need precache.
						m_Precache_nItems = 0;
						m_Precache_nItemsDone = 0;
						m_Precache_lPrecacheOrder.SetLen(nWaves);
						for(int i = 0; i < nWaves; i++)
						{
							if (pWC->GetWaveIDFlag(i) & CWC_WAVEIDFLAGS_PRECACHE)
							{
								m_Precache_lPrecacheOrder[m_Precache_nItems] = i;
								m_Precache_nItems++;
							}
						}
						m_spSound->Wave_PrecacheBegin( m_Precache_nItems );

						m_Precache_lPrecacheOrder.SetLen(m_Precache_nItems);
						m_Precache_lPrecacheOrder.QSort<CPrecacheWaveCompare>(pWC, m_Precache_nItems);

						ConOutL(CStrF("§c0f0NOTE: (CWorld_ClientCore::Precache_Perform) %d sounds to precache.", m_Precache_nItems));
					}

					if (m_Precache_iItem < m_Precache_nItems)
					{
						m_spSound->Wave_Precache(m_Precache_lPrecacheOrder[m_Precache_iItem]);
						m_Precache_nItemsDone++;
						m_Precache_iItem++;
						m_PrecacheProgress++;
					}

					if (m_Precache_iItem >= m_Precache_nItems)
					{					
						m_spSound->Wave_PrecacheEnd(m_Precache_lPrecacheOrder.GetBasePtr(), m_Precache_nItems);
						NextPrecacheStep();
					}
				}
				break;

			case PRECACHE_VERTEXBUFFERS :
				{
					int VBID = m_Precache_iItem;
					int nVBs = pVBC->GetIDCapacity();

					// If we just began caching VBs, count the precache-taged VBs so we can
					// display a proper progress bar.
					if (!VBID)
					{
						if(_pRC)
							_pRC->Geometry_PrecacheFlush();

						// Get the total number of vertex buffers that need precache.
						m_PrecacheProgress = 0;
						m_Precache_nItems = 0;
						m_Precache_nItemsDone = 0;
						m_Precache_lPrecacheOrder.SetLen(nVBs);
						{
							for(int i = 0; i < nVBs; i++)
							{
								int Flags = pVBC->VB_GetFlags(i);
								if (Flags & CXR_VBFLAGS_PRECACHE)
								{
									if (!(Flags & CXR_VBFLAGS_ALLOCATED))
									{
										CStr Name = pVBC->VB_GetName(i);
										M_TRACE("VB not allocated %d, %s\n", i, Name.Str());
										M_ASSERT(0, "!");
										pVBC->VB_SetFlags(i, Flags & ~CXR_VBFLAGS_PRECACHE);
										continue;
									}

									m_Precache_lPrecacheOrder[m_Precache_nItems] = i;
									m_Precache_nItems++;
								}
							}

							ConOutL(CStrF("§c0f0NOTE: (CWorld_ClientCore::Precache_Perform) %d vertex buffers to precache.", m_Precache_nItems));
						}

						if(_pRC)
							_pRC->Geometry_PrecacheBegin( m_Precache_nItems );

						m_Precache_lPrecacheOrder.SetLen(m_Precache_nItems);
						m_Precache_lPrecacheOrder.QSort<CPrecacheVBCompare>(pVBC, m_Precache_nItems);

					}

#if 0
					CStr Str = g_pVC->VB_GetName(m_Precache_lPrecacheOrder[m_Precache_iItem]);
					LogFile(CStrF("Precaching VB: %s", Str.Str()));
					M_TRACEALWAYS("Precaching VB: %s\n", Str.Str());
#endif

					m_PrecacheProgress++;

					if (m_Precache_iItem < m_Precache_nItems)
					{
						if(_pRC)
							_pRC->Geometry_Precache(m_Precache_lPrecacheOrder[m_Precache_iItem]);
					}
					m_Precache_nItemsDone++;
					m_Precache_iItem++;

					if (m_Precache_iItem >= m_Precache_nItems)
					{
						if(_pRC)
							_pRC->Geometry_PrecacheEnd();
						NextPrecacheStep();
					}
				}
				break;
				
			
			case PRECACHE_ASYNCCACHEBLOCK:
				{
	#ifdef PLATFORM_XBOX1

					if (m_Precache_iItem == 0)
					{
						m_spWData->Resource_AsyncCacheEnable(0x70000000);

						if (m_spWData->Resource_AsyncCacheCategoryDone(1))
						{
							m_Precache_iItem = 2;
						}
						else
						{
							m_Precache_iItem = 1;
						}
					}
					else if (m_Precache_iItem == 1)
					{
						if (m_spWData->Resource_AsyncCacheCategoryDone(1))
						{
							m_Precache_iItem = 2;
						}
					}

					if (m_Precache_iItem == 2)
					{
						m_spWData->Resource_AsyncCacheFileDone(2, CStrF("Content\\XDF\\%s.swwc", m_WorldName.Str()));
						++m_Precache_iItem;
					}
					
					if (m_Precache_iItem == 3)
					{
						bool Done = m_spWData->Resource_AsyncCacheFileDone(-1, "");
						if (Done)
						{
							m_spWData->Resource_AsyncCacheDisable(0x70000000);
							NextPrecacheStep();
						}
					}
	#else
					NextPrecacheStep();
	#endif
				}
				break;
				
			case PRECACHE_PERFORMPOSTPRECACHE:
				{
					// Objects
					{
						int nObj = m_lspObjects.Len();
						for(int iObj = 0; iObj < nObj; iObj++)
						{
							CWObject_Client* pObj = Object_Get(iObj);
							if (pObj)
								pObj->m_pRTC->m_pfnOnClientPostPrecache(pObj, this, Render_GetEngine());
						}
					}

					if (m_spMapData)
					{

						// Resources
						CXR_Engine* pEngine = Render_GetEngine();
						int nResources = m_spMapData->GetNumResources();
						for( int i = 0; i < nResources; i++ )
						{
							CWResource* pRc = m_spMapData->GetResourceAsync(i);
							if (pRc && pRc->m_Flags & WRESOURCE_PRECACHE)
							{
								int WRcID = m_spMapData->GetWorldResourceIndex(i);
								m_spWData->Resource_PostPrecache(WRcID, pEngine);
							}
						}
					}
					
					if(Render_GetEngine() && m_Precache_OldSurfOptions != -1)
						Render_GetEngine()->m_SurfOptions = m_Precache_OldSurfOptions;

					NextPrecacheStep();
				}
				break;
				
			default :
				{
					m_Precache_lPrecacheOrder.Destroy();
					// We get here when precache is complete.
					m_spWData->m_DialogContainer.CloseFile();
					CDiskUtil::XDF_Stop();

					m_spWData->Resource_AsyncCacheEnable(4);

					M_TRY
					{
						if(GetClientState() == WCLIENT_STATE_PRECACHE)
						{
#ifdef M_Profile
 #if defined(PLATFORM_XENON)
							//CStr Path = "Cache:\\MemUsage\\";
							CStr Path = m_spMapData->ResolvePath("MemUsage\\");
 #elif defined(PLATFORM_PS3)
							CStr Path = "MemUsage/";
 #else
							CStr Path = m_spMapData->ResolvePath("MemUsage\\");
 #endif
							CDiskUtil::CreatePath(Path);

							CStr Filename = CStrF("%sMem_%s_%08x.txt", Path.Str(), m_WorldName.Str(), m_SpawnMask);
							DumpTextureUsage(Filename, true);
							DumpSoundUsage(Filename, true);
							DumpGeometryUsage(Filename, true);
#endif // M_Profile
						}
					}
					M_CATCH(
					catch (CCException)
					{
					}
					)


					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	/*#ifdef PLATFORM_XBOX
					if (pCon) pCon->ExecuteString("rs_classes");
					if (pCon) pCon->ExecuteString("rs_resources");
	#endif*/

					if (m_RecMode != WCLIENT_RECMODE_PLAYBACK && GetClientState() == WCLIENT_STATE_PRECACHE)
					{
						if (Net_PutMsg(CNetMsg(WPACKET_DONEPRECACHE)))
							m_Precache = PRECACHE_DONE;
					}
					else
					{
						m_Precache = PRECACHE_DONE;
					}
					if (m_Precache == PRECACHE_DONE)
					{
						if(_pRC)
						{
							_pRC->Render_PrecacheEnd();
							_pRC->Render_SetOnlyAllowExternalMemory(false);
						}
					}
				}
			}

			CMTime NewTime = GetTime();
			if ((NewTime - Time).GetTime() > _dTime) break;
		}
/*	}
	M_CATCH(
	catch(CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		m_Precache = PRECACHE_DONE;
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
	M_CATCH(
	catch(CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		m_Precache = PRECACHE_DONE;
	}
	)
#endif*/
}

CFStr CWorld_ClientCore::Precache_GetItemName()
{
	MAUTOSTRIP(CWorld_ClientCore_Precache_GetItemName, CFStr());
	switch(m_Precache)
	{
		case PRECACHE_ONPRECACHE : return "-";
		case PRECACHE_ONPRECACHECLASS : return "-";
		case PRECACHE_RESOURCES : return "resources";	// <-- Localize?
		case PRECACHE_TEXTURES : return "textures";
		case PRECACHE_SOUND: return "sounds";
		case PRECACHE_VERTEXBUFFERS : return "vertex buffers";
		default : return "";
	}
}

fp32 CWorld_ClientCore::Precache_GetProgress()
{
	MAUTOSTRIP(CWorld_ClientCore_Precache_GetProgress, 0.0f);
	switch(m_Precache)
	{
		case PRECACHE_RESOURCES : 
			{
				if (m_spMapData)
					return fp32(m_Precache_ResourceID) / fp32(m_spMapData->GetNumResources());
				else
					return 0;
			}
		case PRECACHE_VERTEXBUFFERS : ;
		case PRECACHE_TEXTURES : return fp32(m_Precache_nItemsDone) / fp32(m_Precache_nItems);
		case PRECACHE_SOUND : return fp32(m_Precache_nItemsDone) / fp32(m_Precache_nItems);
		default : return 1.0f;
	}
}

int CWorld_ClientCore::Precache_Status()
{
	MAUTOSTRIP(CWorld_ClientCore_Precache_Status, 0);
	return m_Precache;
}

const fp32 sPrecacheBias[] = 
{
		0.0f,	// PRECACHE_ONPRECACHE = 1,					5%
		0.05f,	// PRECACHE_ONPRECACHECLASS = 2,			1%
		0.06f,	// PRECACHE_RESOURCES = 3,					1%
		0.07f,	// PRECACHE_TEXTURES = 4,					43%
		0.50f,	// PRECACHE_SOUND = 5,						40%
		0.90f,	// PRECACHE_VERTEXBUFFERS = 6,				7%
		0.97f,	// PRECACHE_ASYNCCACHEBLOCK = 7,			1%
		0.99f,	// PRECACHE_PERFORMPOSTPRECACHE = 8,		1%
};

const fp32 sPrecacheFraction[] = 
{
		0.05f,	// PRECACHE_ONPRECACHE = 1,
		0.01f,	// PRECACHE_ONPRECACHECLASS = 2,
		0.01f,	// PRECACHE_RESOURCES = 3,
		0.43f,	// PRECACHE_TEXTURES = 4,
		0.40f,	// PRECACHE_SOUND = 5,
		0.07f,	// PRECACHE_VERTEXBUFFERS = 6,
		0.01f,	// PRECACHE_ASYNCCACHEBLOCK = 7,
		0.01f,	// PRECACHE_PERFORMPOSTPRECACHE = 8,
};

fp32 CWorld_ClientCore::Load_GetProgress()
{
	if (!m_LoadStatus)
		return 0.0f;

	if(m_Precache == PRECACHE_DONE)
		return 1.0f;
	
	return m_Precache/(fp32)PRECACHE_NUMSTEPS;

	/*
	switch (m_Precache)
	{
	case PRECACHE_DONE:
		//ConOutL("PreCache Done");
		return 1.0f;
	case PRECACHE_ONPRECACHE:
		{
			int nObj = m_lspObjects.Len();
			//ConOutL(CStrF("PRECACHE_ONPRECACHE: nObj: %d Progress: %d",nObj,m_PrecacheProgress));
			if (nObj > 0)
				return sPrecacheFraction[PRECACHE_ONPRECACHE-1] * ((fp32)m_PrecacheProgress / (fp32)nObj);
			else
				return 0.0f;
		}

	case PRECACHE_ONPRECACHECLASS:
		{
			fp32 Status = sPrecacheBias[PRECACHE_ONPRECACHECLASS-1];

			if(m_spMapData == NULL)
				return 0.0f;
			int nRc = m_spMapData->GetNumResources();
			//ConOutL(CStrF("PRECACHE_ONPRECACHECLASS: nObj: %d Progress: %d",nRc,m_PrecacheProgress));
			if (nRc > 0)
				return Status + sPrecacheFraction[PRECACHE_ONPRECACHECLASS-1] * ((fp32)m_PrecacheProgress / (fp32)nRc);
			else
				return Status;
		}
		break;

	case PRECACHE_RESOURCES :
		{
			fp32 Status = sPrecacheBias[PRECACHE_RESOURCES-1];

			int nRc = m_spMapData->GetNumResources();
			//ConOutL(CStrF("PRECACHE_RESOURCES: nObj: %d Progress: %d",nRc,m_PrecacheProgress));
			if (nRc > 0)
				return Status + sPrecacheFraction[PRECACHE_RESOURCES-1] * ((fp32)m_PrecacheProgress / (fp32)nRc);
			else
				return Status;
		}
		break;

	case PRECACHE_TEXTURES :
		{
			fp32 Status = sPrecacheBias[PRECACHE_TEXTURES-1];
			// Total num items
			//ConOutL(CStrF("PRECACHE_TEXTURES: nObj: %d Progress: %d",m_Precache_nItems,m_PrecacheProgress));
			if (m_Precache_nItems > 0)
				return Status + sPrecacheFraction[PRECACHE_TEXTURES-1] * ((fp32)m_PrecacheProgress / (fp32)m_Precache_nItems);
			else
				return Status;
		}

	case PRECACHE_SOUND:
		{
			fp32 Status = sPrecacheBias[PRECACHE_SOUND-1];
			// Total num items
			//ConOutL(CStrF("PRECACHE_SOUND: nObj: %d Progress: %d",m_Precache_nItems,m_PrecacheProgress));
			if (m_Precache_nItems > 0)
				return Status + sPrecacheFraction[PRECACHE_SOUND-1] * ((fp32)m_PrecacheProgress / (fp32)m_Precache_nItems);
			else
				return Status;
		}

	case PRECACHE_VERTEXBUFFERS:
		{
			fp32 Status = sPrecacheBias[PRECACHE_VERTEXBUFFERS-1];
			// Total num items
			//ConOutL(CStrF("PRECACHE_VERTEXBUFFERS: nObj: %d Progress: %d",m_Precache_nItems,m_PrecacheProgress));
			if (m_Precache_nItems > 0)
				return Status + sPrecacheFraction[PRECACHE_VERTEXBUFFERS-1] * ((fp32)m_PrecacheProgress / (fp32)m_Precache_nItems);
			else
				return Status;
		}

	case PRECACHE_ASYNCCACHEBLOCK:
		{
			return sPrecacheBias[PRECACHE_VERTEXBUFFERS-1];
		}

	case PRECACHE_PERFORMPOSTPRECACHE:
		{
			fp32 Status = sPrecacheBias[PRECACHE_PERFORMPOSTPRECACHE-1];
			int nObj = m_lspObjects.Len() + m_spMapData->GetNumResources();
			//ConOutL(CStrF("PRECACHE_PERFORMPOSTPRECACHE: nObj: %d Progress: %d",nObj,m_PrecacheProgress));
			if (nObj > 0)
				return Status + sPrecacheFraction[PRECACHE_PERFORMPOSTPRECACHE-1] * ((fp32)m_PrecacheProgress / (fp32)nObj);
			else
				return Status;
		
		}
	default:
		return 1.0f;
	}
	*/
}


void CWorld_ClientCore::Load_ProgressReset()
{
	m_PrecacheProgress = 0;
	m_LoadStatus = 0;
}
