#include "PCH.h"

#include "WFrontEndMod.h"
#include "WFrontEndMod_Cube.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../../Shared/MOS/Classes/Win/MWinCtrlGfx.h"
#include "../../Shared/MOS/XR/XREngineImp.h"
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/MSystem/Raster/MRCCore.h"
#include "../../Shared/MOS/MSystem/Raster/MTexture.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"
#include "../Exe/WGameContextMain.h"
#include "MFloat.h"
//#include "../../Shared/MOS/Classes/GameWorld/Server/WServer_Core.h"
#include "../GameClasses/Models/CSinRandTable.h"
#include "WFrontEndMod_Cube_VB.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AnimUtils.h"
#include "../GameClasses/Models/WModel_EffectSystem.h"
#include "WClientMod.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

static const char* s_lpPrecacheModels[] = { "Weapons\\gun_1911_02", "Weapons\\Ass_Automatic" };


MRTC_IMPLEMENT_DYNAMIC(CWFrontEnd_Mod, CWFrontEnd);

#define DEBUG_ONE_SIDE 0
#define MOVEMENT_DAMPING 6
#define TENTACLE_BLEND_SPEED 3.0f

static int LEPow2(int _Val)
{
	int v = 1;
	while(v < _Val)
		v	<<= 1;
	v >>= 1;
	return v;
}

static bool ValIsPow2(int _Val)
{
	int w = Log2(_Val);
	return (_Val == (1 << w));
}

void ResetRenderContext(CRenderContext* _pRC)
{
	// attrib
	CRC_Attributes Attrib;
	Attrib.SetDefault();
	_pRC->Attrib_Set(Attrib);

	// matrix
	_pRC->Matrix_SetMode(CRC_MATRIX_MODEL);
	_pRC->Matrix_SetUnit();

	_pRC->Geometry_Clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

//
// ---------------------------------------------------------------------------------------------
//
CSequence::CSequence()
{
	m_pTracks = NULL;
	m_NumTracks = 0;
}

//
CSequence::~CSequence()
{
	if(m_pTracks)
		delete [] m_pTracks;
}

//
void CSequence::CreateTracks(int32 _NumTracks)
{
	m_pTracks = DNew(CTrack) CTrack[_NumTracks];
	m_NumTracks = _NumTracks;
}

//
CVec3Dfp32 CSequence::GetVector(int32 _StartTrack, fp32 _Time)
{
	CVec3Dfp32 Vec;
	Vec.k[0] = GetValue(_StartTrack, _Time);
	Vec.k[1] = GetValue(_StartTrack+1, _Time);
	Vec.k[2] = GetValue(_StartTrack+2, _Time);
	return Vec;
}


//
void CSequence::SetVector(int32 _StartTrack, int32 _Index, fp32 _Time, CVec3Dfp32 _Vector)
{
	M_ASSERT(_StartTrack >= 0 && m_NumTracks, "CubeGUI: Track error");
	m_pTracks[_StartTrack].SetValue(_Index, _Time, _Vector.k[0]);
	m_pTracks[_StartTrack+1].SetValue(_Index, _Time, _Vector.k[1]);
	m_pTracks[_StartTrack+2].SetValue(_Index, _Time, _Vector.k[2]);
}

//
void CSequence::CreateFromSeqSingleFrame(CSequence &_rSeq, fp32 _Time)
{
	CreateTracks(_rSeq.m_NumTracks);
	for(int32 i = 0; i < m_NumTracks; i++)
	{
		m_pTracks[i].CreateKeys(1);
		m_pTracks[i].m_pKeys[0].Set(_Time, _rSeq.m_pTracks[i].GetValue(_Time));
	}
}

//
void CSequence::CopyLastKey(CSequence &_rSeq)
{
	//CreateTracks(_rSeq.m_NumTracks);
	for(int32 i = 0; i < m_NumTracks; i++)
	{
		if(!_rSeq.m_pTracks[i].m_NumKeys)
			continue;

		if(!m_pTracks[i].m_NumKeys)
			m_pTracks[i].CreateKeys(1);

		m_pTracks[i].m_pKeys[m_pTracks[i].m_NumKeys-1].m_Value = _rSeq.m_pTracks[i].m_pKeys[_rSeq.m_pTracks[i].m_NumKeys-1].m_Value;
	}
}

void CSequence::Copy(CSequence &_rSeq)
{
	CreateTracks(_rSeq.m_NumTracks);
	for(int32 i = 0; i < m_NumTracks; i++)
	{
		m_pTracks[i].CreateKeys(_rSeq.m_pTracks[i].m_NumKeys);
		for(int32 k = 0; k < m_pTracks[i].m_NumKeys; k++)
			m_pTracks[i].SetValue(k, _rSeq.m_pTracks[i].m_pKeys[k].m_Time, _rSeq.m_pTracks[i].m_pKeys[k].m_Value);
	}
}

//
// ---------------------------------------------------------------------------------------------
//
CSequence::CTrack::CTrack()
{
	m_pKeys = NULL;
	m_NumKeys = 0;
}

//
CSequence::CTrack::~CTrack()
{
	if(m_pKeys)
		delete [] m_pKeys;
}

//
void CSequence::CTrack::CreateKeys(int32 _NumEntries)
{
	if(_NumEntries <= 0)
		return;

	if(m_pKeys)
	{
		delete [] m_pKeys;
		m_pKeys = NULL;
	}

	m_pKeys = DNew(CKey) CKey[_NumEntries];
	m_NumKeys = _NumEntries;
}

//
void CSequence::CTrack::Clear()
{
	if(m_pKeys)
	{
		delete [] m_pKeys;
		m_pKeys = NULL;
	}

	m_NumKeys = 0;
}

//
fp32 CSequence::CTrack::GetValue(fp32 _Time)
{
	// no keys
	if(!m_NumKeys)
		return 0;

	// before the first key
	if(_Time < m_pKeys[0].m_Time)
		return m_pKeys[0].m_Value;

	// behind the last key
	if(_Time > m_pKeys[m_NumKeys-1].m_Time)
		return m_pKeys[m_NumKeys-1].m_Value;

	// in the middle
	for(int32 i = 1; i < m_NumKeys; i++)
	{
		if(_Time < m_pKeys[i].m_Time)
		{
			// get keys
			CKey &rNextKey = m_pKeys[i];
			CKey &rPrevKey = m_pKeys[i-1];

			// linear interpolate, should have diffrent methods here
			fp32 Amount = (_Time-rPrevKey.m_Time)/(rNextKey.m_Time-rPrevKey.m_Time);
			fp32 Value = rPrevKey.m_Value+(rNextKey.m_Value-rPrevKey.m_Value)*Amount;
			return Value;
		}
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// Move to CXR_Shader or some other appropriate place.
static void CalcTangentSpace(int _iv0, int _iv1, int _iv2, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec2Dfp32* _pTV, CVec3Dfp32& _TangU, CVec3Dfp32& _TangV)
{
	CVec3Dfp32 E0,E1;
	_pV[_iv1].Sub(_pV[_iv0], E0);
	_pV[_iv2].Sub(_pV[_iv0], E1);

	CVec2Dfp32 TE0,TE1;
	TE0 = _pTV[_iv1] - _pTV[_iv0];
	TE1 = _pTV[_iv2] - _pTV[_iv0];

	CMat4Dfp32 Unit2R3;
	Unit2R3.Unit();
	E0.SetRow(Unit2R3, 0);
	E1.SetRow(Unit2R3, 1);

	CMat4Dfp32 Unit2UV;
	Unit2UV.Unit();
	Unit2UV.k[0][0] = TE0[0];
	Unit2UV.k[0][1] = TE0[1];
	Unit2UV.k[1][0] = TE1[0];
	Unit2UV.k[1][1] = TE1[1];

	CMat4Dfp32 UV2Unit;
	Unit2UV.Inverse3x3(UV2Unit);

	CMat4Dfp32 UV2R3;
	UV2Unit.Multiply3x3(Unit2R3, UV2R3);

	_TangU = CVec3Dfp32::GetRow(UV2R3, 0);
	_TangV = CVec3Dfp32::GetRow(UV2R3, 1);
	_TangU.Normalize();
	_TangV.Normalize();
}

static void CalcTangents(uint16* _piTri, int _nTri, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec2Dfp32* _pTV, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV)
{
	MAUTOSTRIP(CalcTangents, MAUTOSTRIP_VOID);

	uint8 aInitV[1024];
	memset(aInitV, 0, 1024);
	for(int t = 0; t < _nTri; t++)
	{
		int iv0 = _piTri[t*3 + 0];
		int iv1 = _piTri[t*3 + 1];
		int iv2 = _piTri[t*3 + 2];
		M_ASSERT(iv0 < 1024 && iv1 < 1024 && iv2 < 1024, "!");

		aInitV[iv0] = 1;
		aInitV[iv1] = 1;
		aInitV[iv2] = 1;

		CVec3Dfp32 TgUMapping;
		CVec3Dfp32 TgVMapping;
		CalcTangentSpace(iv0, iv1, iv2, _pV, _pN, _pTV, TgUMapping, TgVMapping);

		_pTangU[iv0] += TgUMapping;
		_pTangU[iv1] += TgUMapping;
		_pTangU[iv2] += TgUMapping;
		_pTangV[iv0] += TgVMapping;
		_pTangV[iv1] += TgVMapping;
		_pTangV[iv2] += TgVMapping;
	}

	// Ortonormalize tangent coordinate system for each vertex
	{
		//JK-NOTE: One problem here is that all vertices might not be used so their tangents will be 0 (hence, normalize breaks)
		for(int v = 0; v < _nV; v++)
		{
			if(aInitV[v] == 0)
				continue;

			const CVec3Dfp32& N = _pN[v];
			CVec3Dfp32& TgU = _pTangU[v];
			CVec3Dfp32& TgV = _pTangV[v];
			TgU.Normalize();
			TgV.Normalize();
			TgU.Combine(N, -(N * TgU), TgU);
			TgV.Combine(N, -(N * TgV), TgV);
			TgU.Normalize();
			TgV.Normalize();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CMat4Dfp32 operator *(const CMat4Dfp32 &_b, const CMat4Dfp32 &_a)
{
	CMat4Dfp32 Res;
	_a.Multiply(_b, Res);
	return Res;
}

//
//
//
void CCube::CSide::SetMap(CCellInfo *_pMap, bool _ForceExact, bool _QuickChange)
{
	MSCOPESHORT(CCube::CSide::SetMap);

	CCellInfo CellInfo;

	// P5 prototype hack:
	_QuickChange = true;

	if(_QuickChange)
	{
		for(int32 y = 0; y < CUBE_RES; y++)
			for(int32 x = 0; x < CUBE_RES; x++, _pMap ? _pMap++ : 0)
			{
				if(_pMap)
					CellInfo = *_pMap;
				else
				{	
					CCellInfo CleanInfo;
					CleanInfo.m_Mode = CCellInfo::MODE_NORMAL;
					CleanInfo.Cell() = CCellInfo::Empty();
					CleanInfo.Flags() = 0;
				}

				if(CellInfo == m_aElements[x][y].m_Cell)
				{
				}
				else
					m_aElements[x][y].m_Cell = CellInfo;

				if(m_aElements[x][y].m_iActive != -1)
					DeactivateElement(x, y);
			}
	}
	else
	{
		for(int32 y = 0; y < CUBE_RES; y++)
		{
			for(int32 x = 0; x < CUBE_RES; x++)
			{
				if(_pMap)
					CellInfo = _pMap[y*20+x];
				else
				{	
					CCellInfo CleanInfo;
					CleanInfo.m_Mode = CCellInfo::MODE_NORMAL;
					CleanInfo.Cell() = CCellInfo::Empty();
					CleanInfo.Flags() = 0;
					//CleanInfo.IsOK();
				}

				// check if it need an update
				if(CellInfo == m_aElements[x][y].m_Cell)
				{
					// make sure that the flags are transfered (fastswitch)
					if(CellInfo.Flags()&CCellInfo::FLAG_FASTSWITCH)
						m_aElements[x][y].m_Flags |= FLAG_FASTSWITCH;
					else
						m_aElements[x][y].m_Flags &= ~FLAG_FASTSWITCH;

					continue;
				}

				if(CellInfo.Flags()&CCellInfo::FLAG_FASTSWITCH)
				{
					// fast switch
					if(m_aElements[x][y].m_Flags&FLAG_FASTSWITCH)
					{
						if(m_aElements[x][y].m_iActive != -1)
						{
							if(m_pCube->m_aActiveData[m_aElements[x][y].m_iActive].m_WantedDepth > 0.01f)
								m_aElements[x][y].m_Cell = CellInfo;
							else
								m_pCube->m_aActiveData[m_aElements[x][y].m_iActive].m_WantedCell = CellInfo;
						}
						else
							m_aElements[x][y].m_Cell = CellInfo;
						continue;
					}
				}

				const int32 DirX[] = {1,-1,0,0};
				const int32 DirY[] = {0,0,1,-1};
				int32 Dir = int32(Random*4);

				int32 iActive = ActivateElement(x,y,DirX[Dir],DirY[Dir]);
				if(iActive != -1 && !(m_pCube->m_aActiveData[iActive].m_WantedCell == CellInfo))
				{
					M_ASSERT(iActive >= 0 && iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
					m_pCube->m_aActiveData[iActive].m_DirX = DirX[Dir];
					m_pCube->m_aActiveData[iActive].m_DirY = DirY[Dir];
					m_pCube->m_aActiveData[iActive].m_WantedCell = CellInfo;
					m_pCube->m_aActiveData[iActive].m_Time = 0;
					m_pCube->m_aActiveData[iActive].m_Stage = 1;
					m_pCube->m_aActiveData[iActive].m_Scale = 0.5f+Random*0.5f*m_pCube->m_SpeedModifyer;
					m_pCube->m_aActiveData[iActive].m_WantedNumber = int8(3+Random*4);

					if(CellInfo.Flags()&CCellInfo::FLAG_FASTSWITCH)
						m_aElements[x][y].m_Flags |= FLAG_FASTSWITCH;
					else
						m_aElements[x][y].m_Flags &= ~FLAG_FASTSWITCH;
				}
			}
		}
	}
}



//
//
//
void CCube::CSide::SetDepthMap(fp32 *_pMap)
{
	MSCOPESHORT(CCube::CSide::SetDepthMap);

	fp32 aCleanMap[CUBE_RES][CUBE_RES];
	if(_pMap == NULL)
	{
		for(int32 y = 0; y < CUBE_RES; y++)
			for(int32 x = 0; x < CUBE_RES; x++)
				aCleanMap[x][y] = 0;

		_pMap = (fp32*)aCleanMap;
	}

	for(int32 y = 0; y < CUBE_RES; y++)
		for(int32 x = 0; x < CUBE_RES; x++, _pMap++)
		{
			if(m_aElements[x][y].m_iActive != -1)
			{
				M_ASSERT(m_aElements[x][y].m_iActive >= 0 && m_aElements[x][y].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
				CActiveElement &rActive = m_pCube->m_aActiveData[m_aElements[x][y].m_iActive];

				if(rActive.m_Stage > 0)
					continue;

				rActive.m_WantedDepth = *_pMap;
			}
			else if(*_pMap > 0.0001f)
			{
				int32 iActive = ActivateElement(x,y);
				if(iActive != -1)
				{
					M_ASSERT(m_aElements[x][y].m_iActive >= 0 && m_aElements[x][y].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
					CActiveElement &rActive = m_pCube->m_aActiveData[iActive];
					rActive.m_WantedDepth = *_pMap;
				}
			}
		}
}

//
//
//
void CCube::CSide::SetSecondary(int32 _iSomething, int32 _Blend)
{
	m_iSecondaryID = _iSomething;
	m_BlendMode = _Blend;
}

void CCube::CSide::SetSecondaryArea(int32 _x, int32 _y, int32 _w, int32 _h)
{
	CMat4Dfp32 Matrix, Translate, Scale, FinalTranslation, Flip;

	if(_w == 0 || _h == 0)
		return;

	if(0)
	{
		CMTime time = CMTime::GetCPU();
		//  matrices
		Translate.Unit();
		Translate.k[3][0] = 0.0f; //+time.GetTimeModulusScaled(2.0f, 1.0f);
		Translate.k[3][1] = 0.0f;
		Scale.Unit();
		Scale.k[0][0] =-CUBE_RES/(fp32)_w;//(fp32)_w;
		Scale.k[1][1] = CUBE_RES/(fp32)_h;//(fp32)_h;
		//Scale.k[3][0] =1;//(fp32)_w;
		FinalTranslation.Unit();
		FinalTranslation.k[3][0] = 0.5f-_x/(fp32)CUBE_RES;
		FinalTranslation.k[3][1] = 0.5f-_y/(fp32)CUBE_RES;

		// combine
		Matrix = Translate*Scale*FinalTranslation;
	}
	else
	{
		//  matrices
		Flip.Unit();
		Flip.k[0][0] = -1;
		Translate.Unit();
		Translate.k[3][0] = 0.5f;
		Translate.k[3][1] = 0.5f;
		Scale.Unit();
		Scale.k[0][0] = CUBE_RES/(fp32)_w;//(fp32)_w;
		Scale.k[1][1] = CUBE_RES/(fp32)_h;//(fp32)_h;
		FinalTranslation.Unit();
		FinalTranslation.k[3][0] = -_x/(fp32)_w;
		FinalTranslation.k[3][1] = -_y/(fp32)_h;

		// combine
		Matrix = FinalTranslation*Scale*Translate*Flip;
	}


	//static CVec4Dfp32 MulTexTrans[4];
	m_aSecondaryTextureTransform[0][0] = Matrix.k[0][0];
	m_aSecondaryTextureTransform[0][1] = Matrix.k[1][0];
	m_aSecondaryTextureTransform[0][2] = Matrix.k[2][0];
	m_aSecondaryTextureTransform[0][3] = Matrix.k[3][0];
	m_aSecondaryTextureTransform[1][0] = Matrix.k[0][1];
	m_aSecondaryTextureTransform[1][1] = Matrix.k[1][1];
	m_aSecondaryTextureTransform[1][2] = Matrix.k[2][1];
	m_aSecondaryTextureTransform[1][3] = Matrix.k[3][1];
	m_aSecondaryTextureTransform[2][0] = Matrix.k[0][2];
	m_aSecondaryTextureTransform[2][1] = Matrix.k[1][2];
	m_aSecondaryTextureTransform[2][2] = Matrix.k[2][2];
	m_aSecondaryTextureTransform[2][3] = Matrix.k[3][2];
	m_aSecondaryTextureTransform[3][0] = Matrix.k[0][3];
	m_aSecondaryTextureTransform[3][1] = Matrix.k[1][3];
	m_aSecondaryTextureTransform[3][2] = Matrix.k[2][3];
	m_aSecondaryTextureTransform[3][3] = Matrix.k[3][3];
}


//
//
//
void CCube::CSide::Reset()
{
	MSCOPESHORT(CCube::CSide::Reset);
	m_iSecondaryID = -1;
	m_BlendMode = CRC_RASTERMODE_NONE;

	for(int32 y = 0; y < CUBE_RES; y++)
		for(int32 x = 0; x < CUBE_RES; x++)
		{
			m_aElements[x][y].m_Flags = 0;
			if (m_aElements[x][y].m_iActive != -1)
				DeactivateElement(x, y);
			m_aElements[x][y].m_iActive = -1;

			m_aElements[x][y].m_Cell.m_Mode = CCellInfo::MODE_NORMAL;
			m_aElements[x][y].m_Cell.Cell() = CCellInfo::Empty();
			m_aElements[x][y].m_Cell.Flags() = 0;
			//m_aElements[x][y].m_Cell.IsOK();
		}
}


//
//
//
int32 CCube::CSide::ActivateElement(int32 x, int32 y, int32 dx, int32 dy)
{
	MSCOPESHORT(CCube::CSide::ActivateElement);
	if(m_aElements[x][y].m_iActive != -1)
	{
		M_ASSERT(m_aElements[x][y].m_iActive >= 0 && m_aElements[x][y].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
		return m_aElements[x][y].m_iActive;
	}

	if(m_aElements[x][y].m_Flags&FLAG_BLOCKED) //
		return -1;

	if(x+dx >= CUBE_RES || x+dx < 0) return -1;
	if(y+dy >= CUBE_RES || y+dy < 0) return -1;

	if(m_aElements[x+dx][y+dy].m_Flags&FLAG_BLOCKED)
		return -1;

	if(m_aElements[x+dx][y+dy].m_iActive != -1)
	{
		M_ASSERT(m_aElements[x+dx][y+dy].m_iActive >= 0 && m_aElements[x+dx][y+dy].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
		if(m_pCube->m_aActiveData[m_aElements[x+dx][y+dy].m_iActive].m_Stage > STATE_WAITING)
			return -1;
	}

	int32 iElement = m_pCube->AllocateActiveElement();

	if(iElement != -1)
	{
		M_ASSERT(iElement >= 0 && iElement < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
		CActiveElement &rActive = m_pCube->m_aActiveData[iElement];

		//		if(dx || dy)
		//			m_aElements[x+dx][y+dy].m_Flags |= FLAG_BLOCKED;
		//		m_aElements[x][y].m_Flags |= FLAG_BLOCKED;

		rActive.m_DirX = dx;
		rActive.m_DirY = dy;
		rActive.m_WantedCell = m_aElements[x][y].m_Cell;
		rActive.m_WantedNumber = 0;
		rActive.m_Scale = 1;
		rActive.m_Stage = 0;
		rActive.m_Time = 0;
		rActive.m_CurrentDepth = 0;
		rActive.m_WantedDepth = 0;

		m_aElements[x][y].m_iActive = iElement;

		// sound
		if(&m_pCube->m_aSides[m_pCube->m_iCurrentSide] == this)
			m_pCube->PlayEvent(CCube::SOUNDEVENT_UP, 0.3f+(1-m_pCube->m_NumActiveElements/(fp32)CUBE_MAX_ACTIVE_ELEMENTS)*0.7f);
		else
			m_pCube->PlayEvent(CCube::SOUNDEVENT_UP, 0.2f);
	}

	return iElement;
}

//
//
//
void CCube::CSide::UnblockElement(int32 x, int32 y)
{
	if(m_aElements[x][y].m_iActive == -1)
		return;

	M_ASSERT(m_aElements[x][y].m_iActive >= 0 && m_aElements[x][y].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
	//	CActiveElement &rActive = m_pCube->m_aActiveData[m_aElements[x][y].m_iActive];
	//m_aElements[x+rActive.m_DirX][y+rActive.m_DirY].m_Flags &= ~FLAG_BLOCKED;
	//m_aElements[x][y].m_Flags &= ~FLAG_BLOCKED;
}

//
//
//
void CCube::CSide::DeactivateElement(int32 x, int32 y)
{
	MSCOPESHORT(CCube::CSide::DeactivateElement);
	if(m_aElements[x][y].m_iActive == -1)
		return;

	M_ASSERT(m_aElements[x][y].m_iActive >= 0 && m_aElements[x][y].m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element error");
	m_pCube->FreeActiveElement(m_aElements[x][y].m_iActive);
	UnblockElement(x,y);
	m_aElements[x][y].m_iActive = -1;

	// sound
	//if(&m_pCube->m_aSides[m_pCube->m_iCurrentSide] == this)
	//	m_pCube->PlayEvent(CCube::SOUNDEVENT_CLOSE, 0.3f+(1-m_pCube->m_NumActiveElements/(fp32)CUBE_MAX_ACTIVE_ELEMENTS)*0.7f);
	//else
	//	m_pCube->PlayEvent(CCube::SOUNDEVENT_CLOSE, 0.2f);
}


//
//
//
void CCube::ViewSide(int32 _Side, bool _Instant)
{
	//P5 prototype hack:
	_Instant = true;

	// set side
	if(_Side > 5) _Side = 5;
	if(_Side < 0) _Side = 0;

	if(m_CurrentSide == _Side)
		return;

	m_CurrentSide = _Side;

	// fix matrix
	m_Current = GetCurrentMatrix();
	m_aSides[_Side].m_Transform.Transpose(m_Wanted);

	// only rotation.. nothing else
	m_Wanted.k[3][0] = m_Wanted.k[3][1] = m_Wanted.k[3][2] = 0;
	m_Wanted.k[0][3] = m_Wanted.k[1][3] = m_Wanted.k[2][3] = 0;
	m_MoveTime = 0;

	if(_Instant)
	{
		m_Current = m_Wanted;
		m_MoveTime = -10;
	}
}

//
//
//
void CCube::Move(int32 _Dir)
{
	ViewSide(ms_aSideSwapIndex[m_CurrentSide][_Dir], false);
}

//
//
//
CMat4Dfp32 CCube::CreateTranslationMatrix(fp32 x, fp32 y, fp32 z)
{
	CMat4Dfp32 Mat;
	Mat.Unit();
	Mat.k[3][0] = x;
	Mat.k[3][1] = y;
	Mat.k[3][2] = z;
	return Mat;
}

//
//
//
void CCube::CreateRotationMatrix(fp32 x, fp32 y, fp32 z, CMat4Dfp32 *_pRetMat)
{
	CMat4Dfp32 mX, mY, mZ, mTemp;
	mX.SetXRotation(x/_PI2);
	mY.SetYRotation(y/_PI2);
	mZ.SetZRotation(z/_PI2);
	mX.Multiply(mY, mTemp);
	mTemp.Multiply(mZ, *_pRetMat);
}

//
//
//
#define PROJECTIONFLICKERINTIME 0.15f
#define PROJECTIONFLICKERTOTALTIME 0.23f
#define BACKGROUNDPROCESSTIME 1.0f
#define ADDSPOTFLICKERTIME 0.15f
#define ADDSPOTFLICKERRESTORETIME 2.0f
#define ADDSPOTFLICKERPERMISSIONTIME 10.0f
#define FADETIME 1.5f
#define DEATH_FADETIME 0.25f

CCube::CCube()
{
	//	spCReferenceCount spRef = MRTC_GOM()->GetClassRegistry()->CreateObject("CXR_VBManager");
	//	m_spVBM = safe_cast<CXR_VBManager>((CReferenceCount*)spRef);

	m_TextureID_ProjectionTexture = 0;
	m_TextureID_ScreencapToProcess = 0;
	m_TextureID_FinalImage = 0;
	m_TextureID_PostProcessTemp = 0;
	m_GUIFlags = 0;
	m_iGUIWorld = 0; 
	m_iLoadingWorld = 0;
	m_BackgroundSoundLoopID = -1;
	m_iModelJackie = 0;
	m_iCurrentAnim = 0;
	m_LoadingSceneAnimTime = 0;
	m_LastRenderTime = 0;
	m_FadeTimer = 0;
	m_iModelWeaponMain = 0;
	m_iModelWeaponSecondary =0;
	m_iModelExtra = 0;
	m_ProjectionFlickerTimer = 0;
	m_ProjLightPos = 0;
	m_hVoice = -1;
	m_LastiActive = 0;
	m_LastSide = 0;
	m_iTentacleModel = 0;
	m_iTentacleAnim = 0;

	m_LoadingSceneFOV = 86.0f;

	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv->GetValuei("DISABLE_NEW_GUI", 0))
        m_GUIFlags |= GUI_FLAGS_DISABLENEWGUI;

	if (pEnv->GetValuei("DISABLE_LOADINGMONOLOG", 0))
		m_GUIFlags |= GUI_FLAGS_DISABLELOADINGMONOLOG;

	if (pEnv->GetValuei("DISABLE_GUI_TENTACLES", 0))
		m_GUIFlags |= GUI_FLAGS_DISABLE_TENTACLES;

	//
	m_Offset = 0;
	m_FacingAway = false;
	m_RotationOffset = 0;
	m_SequenceOverride = 0;
	m_SpeedModifyer = 1;

	//m_Offset = CVec3Dfp32(-0.34f, 1.9f, 6);
	//m_RotationOffset = CVec3Dfp32(0.2f+0.2f,-0.05f-0.2f,0);
	//m_StartSequenceState = 1;
	m_pCurrentSequence = NULL;
	m_SequenceTime = 0;

	m_Seq_moveout.CreateTracks(TRACK_NUM);

	//
	{
		const fp32 StartTime = 0;
		const fp32 EndTime = 1;
		int32 i;

		m_Seq_moveout.m_pTracks[TRACK_OFFSET_X].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_OFFSET_Y].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_OFFSET_Z].CreateKeys(2);

		i = 0;
		m_Seq_moveout.SetVector(TRACK_OFFSET_X, i++, StartTime, CVec3Dfp32(12.0f, -2.5f, 6));
		m_Seq_moveout.SetVector(TRACK_OFFSET_X, i++, EndTime, CVec3Dfp32(0, 0, 0));

		//
		m_Seq_moveout.m_pTracks[TRACK_ROT_X].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_ROT_Y].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_ROT_Z].CreateKeys(2);

		i = 0;
		m_Seq_moveout.SetVector(TRACK_ROT_X, i++, StartTime, CVec3Dfp32(0.2f, -0.2f, 0));
		m_Seq_moveout.SetVector(TRACK_ROT_X, i++, EndTime, CVec3Dfp32(0.0f, 0.1f, 0));

		// add layer
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_R].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_G].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_B].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_INTENS].CreateKeys(2);

		i = 0;
		m_Seq_moveout.SetVector(TRACK_ADDLAYER_R, i++, StartTime, CVec3Dfp32(1, 1, 1));
		m_Seq_moveout.SetVector(TRACK_ADDLAYER_R, i++, EndTime, CVec3Dfp32(0, 0, 0));
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_INTENS].SetValue(i++, StartTime, 1);
		m_Seq_moveout.m_pTracks[TRACK_ADDLAYER_INTENS].SetValue(i++, EndTime, 0);

		// move
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_MOVEBLOCK].CreateKeys(4);
		m_Seq_moveout.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, StartTime, 0);
		m_Seq_moveout.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, StartTime+0.5f, 0);
		m_Seq_moveout.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, StartTime+0.5f, 1);
		m_Seq_moveout.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, EndTime, 1);

		// map
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_MAPBLOCK].CreateKeys(4);
		m_Seq_moveout.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, StartTime, 0);
		m_Seq_moveout.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, StartTime+1.0f, 0);
		m_Seq_moveout.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, StartTime+1.01f, 1);
		m_Seq_moveout.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, EndTime, 1);

		// time
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_TIMESCALE].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_TIMESCALE].SetValue(i++, StartTime, 0);
		m_Seq_moveout.m_pTracks[TRACK_TIMESCALE].SetValue(i++, EndTime, 1);

		// light intens
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_LIGHT_INTENS].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_LIGHT_INTENS].SetValue(i++, StartTime, 0.5f);
		m_Seq_moveout.m_pTracks[TRACK_LIGHT_INTENS].SetValue(i++, EndTime, 1);

		// bg intens
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_BGINTENS].CreateKeys(2);
		m_Seq_moveout.m_pTracks[TRACK_BGINTENS].SetValue(i++, StartTime, 1);
		m_Seq_moveout.m_pTracks[TRACK_BGINTENS].SetValue(i++, EndTime, 1);

		// keylock
		i = 0;
		m_Seq_moveout.m_pTracks[TRACK_KEYLOCK].CreateKeys(3);
		m_Seq_moveout.m_pTracks[TRACK_KEYLOCK].SetValue(i++, StartTime, 1);
		m_Seq_moveout.m_pTracks[TRACK_KEYLOCK].SetValue(i++, EndTime-0.4f, 1);
		m_Seq_moveout.m_pTracks[TRACK_KEYLOCK].SetValue(i++, EndTime, 0.0f);
	}

	m_Seq_startup.CreateFromSeqSingleFrame(m_Seq_moveout, 0);
	m_Seq_startup.m_pTracks[TRACK_KEYLOCK].Clear();

	m_Seq_beginload.CreateTracks(TRACK_NUM);
	{
		const fp32 StartTime = 0;
		const fp32 EndTime = 0.3f;
		int32 i;

		m_Seq_beginload.m_pTracks[TRACK_OFFSET_X].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_OFFSET_Y].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_OFFSET_Z].CreateKeys(2);

		i = 0;
		m_Seq_beginload.SetVector(TRACK_OFFSET_X, i++, StartTime, CVec3Dfp32(0, 0, 0));
		m_Seq_beginload.SetVector(TRACK_OFFSET_X, i++, EndTime, CVec3Dfp32(0.10f, 1.5f, 2.5f));

		//
		m_Seq_beginload.m_pTracks[TRACK_ROT_X].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_ROT_Y].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_ROT_Z].CreateKeys(2);

		i = 0;
		m_Seq_beginload.SetVector(TRACK_ROT_X, i++, StartTime, CVec3Dfp32(0.0, 0.2f, 0));
		m_Seq_beginload.SetVector(TRACK_ROT_X, i++, EndTime, CVec3Dfp32(0.2f, 0.0f, -_PI/4));

		// time
		i = 0;
		m_Seq_beginload.m_pTracks[TRACK_TIMESCALE].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_TIMESCALE].SetValue(i++, StartTime, 1);
		m_Seq_beginload.m_pTracks[TRACK_TIMESCALE].SetValue(i++, EndTime, 1);

		// light intens
		i = 0;
		m_Seq_beginload.m_pTracks[TRACK_LIGHT_INTENS].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_LIGHT_INTENS].SetValue(i++, StartTime, 1.0f);
		m_Seq_beginload.m_pTracks[TRACK_LIGHT_INTENS].SetValue(i++, EndTime, 1);

		// bg intens
		i = 0;
		m_Seq_beginload.m_pTracks[TRACK_BGINTENS].CreateKeys(2);
		m_Seq_beginload.m_pTracks[TRACK_BGINTENS].SetValue(i++, StartTime, 1);
		m_Seq_beginload.m_pTracks[TRACK_BGINTENS].SetValue(i++, EndTime/2, 0.0f);
	}

	//
	m_Seq_fromgame.CreateTracks(TRACK_NUM);
	{
		const fp32 StartTime = 0;
		const fp32 EndTime = 0.5f;
		int32 i;

		//
		m_Seq_fromgame.m_pTracks[TRACK_ROT_X].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ROT_Y].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ROT_Z].CreateKeys(2);

		i = 0;
		m_Seq_fromgame.SetVector(TRACK_ROT_X, i++, StartTime, CVec3Dfp32(0, _PI/3, 0));
		m_Seq_fromgame.SetVector(TRACK_ROT_X, i++, EndTime, CVec3Dfp32(0, 0, 0));

		//
		m_Seq_fromgame.m_pTracks[TRACK_OFFSET_X].CreateKeys(3);
		m_Seq_fromgame.m_pTracks[TRACK_OFFSET_Y].CreateKeys(3);
		m_Seq_fromgame.m_pTracks[TRACK_OFFSET_Z].CreateKeys(3);

		i = 0;
		m_Seq_fromgame.SetVector(TRACK_OFFSET_X, i++, StartTime, CVec3Dfp32(-2.2f, 0, 2-1.3f));
		m_Seq_fromgame.SetVector(TRACK_OFFSET_X, i++, EndTime/2, CVec3Dfp32(-2.2f, 1, 2));
		m_Seq_fromgame.SetVector(TRACK_OFFSET_X, i++, EndTime, CVec3Dfp32(0, 0, 0));

		//
		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_VIEWPORT].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_VIEWPORT].SetValue(i++, StartTime, 1);
		m_Seq_fromgame.m_pTracks[TRACK_VIEWPORT].SetValue(i++, EndTime, 0);

		// add layer
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_R].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_G].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_B].CreateKeys(2);

		i = 0;
		m_Seq_fromgame.SetVector(TRACK_ADDLAYER_R, i++, StartTime, CVec3Dfp32(100/255.0f, 100/255.0f, 100/255.0f*(220/255.0f)));
		m_Seq_fromgame.SetVector(TRACK_ADDLAYER_R, i++, EndTime, CVec3Dfp32(0, 0, 0));

		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_INTENS].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_INTENS].SetValue(i++, StartTime, 1);
		m_Seq_fromgame.m_pTracks[TRACK_ADDLAYER_INTENS].SetValue(i++, EndTime, 0);

		// time
		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_TIMESCALE].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_TIMESCALE].SetValue(i++, StartTime, 0.1f);
		m_Seq_fromgame.m_pTracks[TRACK_TIMESCALE].SetValue(i++, EndTime, 1);

		//
		m_Seq_fromgame.m_pTracks[TRACK_ROT_X].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ROT_Y].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_ROT_Z].CreateKeys(2);

		i = 0;
		m_Seq_fromgame.SetVector(TRACK_ROT_X, i++, StartTime, CVec3Dfp32(0.0f, 0.0f, 0));
		m_Seq_fromgame.SetVector(TRACK_ROT_X, i++, EndTime, CVec3Dfp32(0.0f, 0.0f, 0));

		// map
		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_MAPBLOCK].CreateKeys(4);
		m_Seq_fromgame.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, StartTime, 0);
		m_Seq_fromgame.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, EndTime/2, 0);
		m_Seq_fromgame.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, EndTime/2+0.01f, 1);
		m_Seq_fromgame.m_pTracks[TRACK_MAPBLOCK].SetValue(i++, EndTime, 1);

		// move
		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_MOVEBLOCK].CreateKeys(4);
		m_Seq_fromgame.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, StartTime, 0);
		m_Seq_fromgame.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, EndTime/2, 0);
		m_Seq_fromgame.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, EndTime/2+0.01f, 1);
		m_Seq_fromgame.m_pTracks[TRACK_MOVEBLOCK].SetValue(i++, EndTime, 1);

		// bg intens
		i = 0;
		m_Seq_fromgame.m_pTracks[TRACK_BGINTENS].CreateKeys(2);
		m_Seq_fromgame.m_pTracks[TRACK_BGINTENS].SetValue(i++, StartTime, 1);
		m_Seq_fromgame.m_pTracks[TRACK_BGINTENS].SetValue(i++, EndTime, 0.25f);

		m_Seq_fromgame.CopyLastKey(m_Seq_moveout);
	}

	m_Seq_backtogame.CreateTracks(TRACK_NUM);
	{
		//const fp32 StartTime = 0;
		const fp32 EndTime = 0.5f;
		//int32 i;

		for(int32 t = 0; t < m_Seq_backtogame.m_NumTracks; t++)
		{
			CSequence::CTrack *pDstTrack = &m_Seq_backtogame.m_pTracks[t];
			CSequence::CTrack *pSrcTrack = &m_Seq_fromgame.m_pTracks[t];
			int32 NumKeys = pSrcTrack->m_NumKeys;
			pDstTrack->CreateKeys(NumKeys);
			for(int32 k = 0; k < NumKeys; k++)
			{
				CSequence::CTrack::CKey *pDstKey = &pDstTrack->m_pKeys[NumKeys-k-1];
				CSequence::CTrack::CKey *pSrcKey = &pSrcTrack->m_pKeys[k];

				pDstKey->m_Time = EndTime-pSrcKey->m_Time;
				pDstKey->m_Value = pSrcKey->m_Value;
				pDstKey->m_Flag = pSrcKey->m_Flag;
			}
		}

		// copy everything except the dropgui part
		m_Seq_removeloading.Copy(m_Seq_backtogame);


		m_Seq_backtogame.m_pTracks[TRACK_DROPGUI].CreateKeys(3);
		int32 i = 0;
		m_Seq_backtogame.m_pTracks[TRACK_DROPGUI].SetValue(i++, 0, 0);
		m_Seq_backtogame.m_pTracks[TRACK_DROPGUI].SetValue(i++, EndTime-0.01f, 0);
		m_Seq_backtogame.m_pTracks[TRACK_DROPGUI].SetValue(i++, EndTime, 1);
	}

	m_Seq_inside.CreateFromSeqSingleFrame(m_Seq_moveout, 0);
	m_Seq_normal.CreateFromSeqSingleFrame(m_Seq_moveout, 100);
	//m_pCurrentSequence = &m_StartSequence;
	UpdateSequence(0); // inital update

	//if(!m_spVBM)
	//	Error("Create", "Unable to create CXR_VBManager");

	// Get system
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	//if (!pSys) Error("Create", "No system.");

	m_SurfaceName = pSys->GetEnvironment()->GetValue("CUBE_SURFACE", "GUI_CubeMaster006");
	m_MulTextureName = pSys->GetEnvironment()->GetValue("CUBE_MULTEXTURE", "GUI_CubeMul01");
	//m_MulTextureName = pSys->GetEnvironment()->GetValue("CUBE_MULTEXTURE", "GUI_CubeMaster05_C");
	m_MulMode = pSys->GetEnvironment()->GetValuei("CUBE_MULMODE", CRC_RASTERMODE_MULTIPLY2);

	// init sides
	CMat4Dfp32 Base = CreateTranslationMatrix(0,0, 0.5f);
	m_aSides[0].m_Transform = Base;

	CreateRotationMatrix(0,_PI/2,0, &m_aSides[1].m_Transform);
	m_aSides[1].m_Transform = m_aSides[1].m_Transform*Base;

	CreateRotationMatrix(0,_PI,0, &m_aSides[2].m_Transform);
	m_aSides[2].m_Transform = m_aSides[1].m_Transform*Base;

	CreateRotationMatrix(0,-_PI/2,0, &m_aSides[3].m_Transform);
	m_aSides[3].m_Transform = m_aSides[1].m_Transform*Base;

	CreateRotationMatrix(_PI/2,0,0, &m_aSides[4].m_Transform);
	m_aSides[4].m_Transform = m_aSides[1].m_Transform*Base;

	CreateRotationMatrix(-_PI/2,0,0, &m_aSides[5].m_Transform);
	m_aSides[5].m_Transform = m_aSides[1].m_Transform*Base;

	for(int32 i = 0; i < 6; i++)
	{
		//m_aSides[i].m_Transform.NormalizeRows3x3();
		m_aSides[i].m_pCube = this;
		m_aSides[i].Reset();
	}

	ResetActiveElementData();

	//
	m_MoveTime = -10;
	m_CurrentSide = -1;
	m_FrameTime = 0.000000001f;
	//	m_Zoom = 0;
	ViewSide(0, true);
	m_MoveTime = -1;

	// adjust these buffers
	InitStaticAttribs();

	//	const int32 CubePerSide = CUBE_RES*CUBE_RES;
	//	const int32 TotalCubes = CubePerSide*6;
	//	const int32 ActiveCubes = CUBE_MAX_ACTIVE_ELEMENTS*3; // 3 because when we split the subcube into three

	/*
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("-- Cube GUI memory usage --\n");
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("ActiveElements=%d\n", sizeof(CActiveElement)*CUBE_MAX_ACTIVE_ELEMENTS);
	M_TRACEALWAYS("Sides=%d\n", sizeof(CSide)*6);
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("Cube=%d\n", sizeof(CCube));
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("TOTAL %d kib\n", sizeof(CCube)/1024);
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("\n");
	M_TRACEALWAYS("-- Cube GUI buffer inits --\n");
	M_TRACEALWAYS("\n\n");
	*/

	m_CubeSize = 1/20.0f;

	//
	CCellInfo Cell;
	Cell.m_Mode = CCellInfo::MODE_SMALL; // we need small mode so all tangents will be calculated
	Cell.SubCell(0) = 0;
	Cell.SubCell(1) = 0;

	//	m_spEngine = GetFrontEndEngine();
	m_BackgroundSoundLoopID = 0;
}

CCube::~CCube()
{
	if(m_hVoice != -1)
		StopSoundFX(m_hVoice, true);
}


static void PrecacheSurface(CXW_Surface* _pSurf, CXR_Engine* _pEngine, int _TxtIdFlags = 0)
{
	MAUTOSTRIP(PrecacheSurface, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf;
	if (_pEngine)
		pSurf = _pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
	else
		pSurf = _pSurf->GetSurface(XW_SURFOPTION_HQ0, XW_SURFREQ_MULTITEX4|XW_SURFREQ_NV20|XW_SURFREQ_NV10|XW_SURFOPTION_SHADER);

	pSurf->InitTextures(false);
	pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE | _TxtIdFlags);
}

static void PrecacheSurface(const char* _pSurf, CXR_Engine* _pEngine, CXR_SurfaceContext *_pSC, int _TxtIdFlags = 0)
{
	MAUTOSTRIP(PrecacheSurface_2, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf = _pSC->GetSurface(_pSurf);
	if (pSurf)
		PrecacheSurface(pSurf, _pEngine, _TxtIdFlags);
}

static void RemoveResidentSurface(const char* _pSurf, CXR_Engine* _pEngine, CXR_SurfaceContext *_pSC)
{
	CXW_Surface* pSurf = _pSC->GetSurface(_pSurf);
	if (_pEngine)
		pSurf = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
	else
		pSurf = pSurf->GetSurface(XW_SURFOPTION_HQ0, XW_SURFREQ_MULTITEX4|XW_SURFREQ_NV20|XW_SURFREQ_NV10|XW_SURFOPTION_SHADER);

	pSurf->SetTextureParam(CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_RESIDENT);
}

static void PrecacheTexture(const char* _pTextureName, CTextureContext* _pTC, int _Flags = 0)
{
	_pTC->SetTextureParam(_pTC->GetTextureID(_pTextureName), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE | _Flags);
}

static void RemoveResidentTexture(const char* _pTextureName, CTextureContext* _pTC)
{
	_pTC->SetTextureParam(_pTC->GetTextureID(_pTextureName), CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_RESIDENT);
}

//void PrecacheSound(CSC_SFXDesc *_pSound, CWaveContext *_pWC);

void CCube::PrecacheSound(CSC_SFXDesc *_pSound, CWaveContext *_pWC)
{
	if(_pSound && _pWC)
	{
		for (int i = 0; i < _pSound->GetNumWaves(); ++i)
		{
			int16 iWave = _pSound->GetWaveId(i);
			if(iWave >= 0)
			{
				_pWC->AddWaveIDFlag(iWave, CWC_WAVEIDFLAGS_PRECACHE);
			}
		}
	}
}


void CCube::OnPrecache(CXR_Engine* _pEngine, CWorld_Server *_pServer)
{
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC)
		return;

	CXR_Model* pGUIWorld = m_spMapData->GetResource_Model(m_iGUIWorld);
	if (pGUIWorld)
		pGUIWorld->OnPrecache(_pEngine, 0);

	CXR_Model* pLoadingWorld = m_spMapData->GetResource_Model(m_iLoadingWorld);
	if (pLoadingWorld)
		pLoadingWorld->OnPrecache(_pEngine, 0);

	// precache sounds
	if(m_spSoundContext != NULL)
	{
		//InitSound(m_spMapData, m_spSoundContext,1) etc
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if (pWC)
		{
			for(int32 i = 0; i < CCube::NUMSOUNDEVENTS; i++)
			{
				if(m_aSounds[i] > 0)
				{
					CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(m_aSounds[i]);
					PrecacheSound(pDesc, pWC);
				}
			}
		}

		CStr AllSounds = m_AllVocapSounds;
		while (!AllSounds.IsEmpty())
		{
			CStr Sound = AllSounds.GetStrSep(",");
			if (!Sound.IsEmpty())
			{
				int WaveID = m_spMapData->GetResourceIndex_Sound(Sound);
				CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(WaveID);
				PrecacheSound(pDesc, pWC);
			//iSoundPrecacheCounter++;
			}
		}
	}

	// Textures, surfaces
	PrecacheSurface("GUI_CubeMaster006", _pEngine, pSC);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
		PrecacheTexture("GUI_CubeMul01", pTC);
		PrecacheTexture("Special_000000", pTC);
		PrecacheTexture("SPECIAL_DEFAULTLENS", pTC);

		if (m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG)
			PrecacheTexture("GUI_MPController", pTC);

	//	if (m_GUIFlags & GUI_FLAGS_DISABLENEWGUI)
	//		PrecacheTexture("GUI_X06logo", pTC);

		CStr AllTextures = m_AllTextures;
		while (!AllTextures.IsEmpty())
		{
			CStr Tex = AllTextures.GetStrSep(",");
			if (!Tex.IsEmpty())
				PrecacheTexture(Tex, pTC);
		}
	}

	MACRO_GetRegisterObject(CXR_VBContext, pVBC, "SYSTEM.VBCONTEXT");
	m_VertexBuffers.Precache();


	if(!m_spGUICubeMaps && !(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
	{
		MRTC_SAFECREATEOBJECT_NOEX(spGUIProcess, "CTextureContainer_Screen", CTextureContainer_Screen);
		m_spGUICubeMaps = spGUIProcess;
		if(!m_spGUICubeMaps)
			ConOut("§cf80WARNING: Unable to instance CTextureContainer_Screen for CCube GUI.");
		else
		{
			spGUIProcess->Create(1, _pEngine);
			SetGUIProcessProperties(spGUIProcess);
		}
	}

	// Jackie models
	CStr AllJackieModels = m_AllJackieModels;
	while (!AllJackieModels.IsEmpty())
	{
		CStr Model = AllJackieModels.GetStrSep(",");
		if (!Model.IsEmpty())
		{
			int iModel = m_spMapData->GetResourceIndex_Model("Characters\\J_E\\" + Model);
			CXR_Model* pModel = m_spMapData->GetResource_Model(iModel);
			if (pModel)
				pModel->OnPrecache(_pEngine, 0);
		}
	}

	// Weapon models
	for (uint i = 0; i < sizeof(s_lpPrecacheModels) / sizeof(s_lpPrecacheModels[0]); i++)
	{
		int iModelWeaponMain = m_spMapData->GetResourceIndex_Model(s_lpPrecacheModels[i]);
		CXR_Model* pModel = m_spMapData->GetResource_Model(iModelWeaponMain);
		if (pModel)
			pModel->OnPrecache(_pEngine, 0);
	}

	CXR_Model* pModel = m_spMapData->GetResource_Model(m_iTentacleModel);
	if (pModel)
		pModel->OnPrecache(_pEngine, 0);

	// precache particlesystem
	pModel = m_spMapData->GetResource_Model(m_iLSMParticles);
	if (pModel)
		pModel->OnPrecache(_pEngine, 0);

	PrecacheDeathScene(_pEngine, _pServer);
	m_GUIFlags &= ~GUI_FLAGS_VALUESINITIATED;
	m_GUIFlags &= ~GUI_FLAGS_SOUNDINITIALIZED;

	m_LastCurrentSide = m_CurrentSide;
}


void CCube::LoadAllResources()
{
	InitSound(m_spMapData, m_spSoundContext, m_iChannel);
	LoadAndSetupGUITentacles();
	SetGUIAndLoadingXRWorlds(NULL);

	if (!(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
		ResetGuiProjectionValues();

	// Get resource index for all vocap sounds
	CStr AllSounds = m_AllVocapSounds;
	while (!AllSounds.IsEmpty())
	{
		CStr Sound = AllSounds.GetStrSep(",");
		if (!Sound.IsEmpty())
			m_spMapData->GetResourceIndex_Sound(Sound);
	}

	// Jackie models
	CStr AllJackieModels = m_AllJackieModels;
	while (!AllJackieModels.IsEmpty())
	{
		CStr Model = AllJackieModels.GetStrSep(",");
		if (!Model.IsEmpty())
			m_spMapData->GetResourceIndex_Model("Characters\\J_E\\" + Model);
	}

	// Weapon models
	for (uint i = 0; i < sizeof(s_lpPrecacheModels) / sizeof(s_lpPrecacheModels[0]); i++)
		m_spMapData->GetResourceIndex_Model(s_lpPrecacheModels[i]);

	m_iAnimGripBasePistol = m_spMapData->GetResourceIndex_Anim("HumanMovement\\DualPistols\\DPis_PistolGrip");

	m_iLSMParticles = m_spMapData->GetResourceIndex_Model("particles:MP=501,SU=sb01aa,CO=0x44C0D1E2,TO=10,DU=10,FI=5,VE=1,LO=20 0 60,DI=cube,DIS=10 10 12,SZ0=30,SZ1=40,SZ2=30,SZ3=40,RT0=-8,RT1=8,RT2=-8,RT3=8,RT4=0.005,TC=0.02,FL=nl+lt");
}


void CCube::SetGUIProcessProperties(CTextureContainer_Screen* _pGUIProcess)
{
	if(_pGUIProcess)
	{
		CTC_TextureProperties Prop;
		_pGUIProcess->GetTextureProperties(0, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_CUBEMAP;
#ifndef PLATFORM_XENON
		Prop.m_Flags |= CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT;
#endif
		/*
		#ifdef PLATFORM_XENON
		_pGUIProcess->SetTextureFormat(0, IMAGE_FORMAT_I8);
		#endif
		*/

		_pGUIProcess->SetTextureProperties(0, Prop);
	}
}

//
//
//
void CCube::Reset()
{
	ResetActiveElementData();

	for(int32 i = 0; i < 6; i++)
		m_aSides[i].Reset();
	m_MoveTime = -10;
}


//
//
//
void CCube::ResetActiveElementData()
{
	MSCOPESHORT(CCube::ResetActiveElementData);
	m_ActiveList.Clear();
	m_InactiveActiveList.Clear();

	m_NumActiveElements = CUBE_MAX_ACTIVE_ELEMENTS;

	for(int32 i = 0; i < CUBE_MAX_ACTIVE_ELEMENTS; i++)
	{
		m_InactiveActiveList.Insert(m_aActiveData[i]);
	}
}

//
//
//
int16 CCube::AllocateActiveElement()
{
	MSCOPESHORT(CCube::AllocateActiveElement);

	CActiveElement *pActive = m_InactiveActiveList.Pop();
	if (!pActive)
		return -1;

	m_ActiveList.Insert(pActive);

	m_NumActiveElements--;

	return pActive - m_aActiveData;
}


//
//
//
void CCube::FreeActiveElement(int16 _Element)
{
	M_ASSERT(_Element >= 0 && _Element < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI(FreeActiveElement): Element error");

	m_NumActiveElements++;

	m_InactiveActiveList.Push(m_aActiveData + _Element);
}

//
//
//
void CCube::UpdateElements(fp32 _Delta)
{
	MSCOPESHORT(CCube::UpdateElements);
	//return;

	DLinkD_Iter(CActiveElement, m_Link) Iter = m_ActiveList;

	while (Iter)
	{
		CActiveElement &rActive = *Iter;

		//if(rActive.m_Stage > STATE_WAITING && rActive.m_Stage < STATE_DONE)
		//	rActive.m_Time += 1/(ms_StateTime[rActive.m_Stage]/1000.0f)*_DeltaTime*rElement.m_Scale;

		if(rActive.m_CurrentDepth > rActive.m_WantedDepth)
		{
			rActive.m_CurrentDepth -= _Delta * (0.5f*rActive.m_Scale);
			if(rActive.m_CurrentDepth < rActive.m_WantedDepth)
				rActive.m_CurrentDepth = rActive.m_WantedDepth;
		}
		else if(rActive.m_CurrentDepth < rActive.m_WantedDepth)
		{
			rActive.m_CurrentDepth += _Delta * (0.75f*rActive.m_Scale);
			if(rActive.m_CurrentDepth > rActive.m_WantedDepth)
				rActive.m_CurrentDepth = rActive.m_WantedDepth;
		}


		if(rActive.m_Stage != STATE_WAITING)
		{
			rActive.m_Time += _Delta*5*rActive.m_Scale;
			if(rActive.m_Time >= 1)
			{

				rActive.m_Stage++;

				rActive.m_Time = 0;
			}
		}

		++Iter;
	}
}

//
//
//
int32 CCube::RenderSide(CXR_VBManager* _pVBM, int32 _Side, int32 _StartIndex)
{
	MSCOPESHORT(CCube::RenderSide);
	CSide *pSide = &m_aSides[_Side];
	m_RenderingSide = _Side;
	//if(!OPT_ALL_AT_ONCE)
	//	ResetBuilders();

	if(DEBUG_ONE_SIDE)
	{
		if(pSide != &m_aSides[m_CurrentSide])
			return -1;
	}

	CVec3Dfp32	Camera = CVec3Dfp32::GetRow(pSide->m_CameraMatrix,3);
	Camera.Normalize();
	if(CVec3Dfp32::GetRow(pSide->m_CameraMatrix,2)*Camera < 0)
		m_FacingAway = false;
	else
		m_FacingAway = true;

	//
	fp32 TotalCubeSize = 1;
	const fp32 CubeSize = TotalCubeSize/CUBE_RES;
	const fp32 CubeDepth2 = CubeSize;//*2;
	///	const fp32 TotalDepth = CubeSize*CUBE_RES;
	//	const fp32 BackPlane = -TotalDepth;

	//m_iCell
	m_CubeSize = CubeSize;
	int32 Starty = _StartIndex/CUBE_RES;
	int32 Startx = _StartIndex%CUBE_RES;

	//M_TRACEALWAYS("starting at %d,%d\n", Startx, Starty);
	fp32 fy = Starty;
	for(int32 y = Starty; y < CUBE_RES; y++, fy++)
	{
		fp32 fx = Startx;
		for(int32 x = Startx; x < CUBE_RES; x++, fx++)
		{
			CElement &rElement = pSide->m_aElements[x][y];

			if(rElement.m_iActive != -1)
			{
				M_ASSERT(rElement.m_iActive >= 0 && rElement.m_iActive < CUBE_MAX_ACTIVE_ELEMENTS, "CubeGUI: Element Error");
				CActiveElement &rActive = m_aActiveData[rElement.m_iActive];
				fp32 TargetDepth = rActive.m_WantedNumber*CubeDepth2;
				fp32 Time = rActive.m_Time;
				fp32 InvTime = 1-Time;
				fp32 DirX = rActive.m_DirX;
				fp32 DirY = rActive.m_DirY;

				if(rActive.m_Stage == 0)
				{
					AddCube(rElement.m_Cell, 0, rActive.m_CurrentDepth, fx, fy, 0, 0);
					//AddCube(rElement.m_iCell, 0, 0, fx, fy, 0, 0, 0);
				}
				else if(rActive.m_Stage == 1)
				{
					// move out
					AddCube(rElement.m_Cell, 0, Time*TargetDepth, fx, fy,0,0);
				}
				else if(rActive.m_Stage == 2)
				{
					AddCube(rElement.m_Cell, 0, CubeDepth2, fx, fy,0,0);
					AddCube(rActive.m_WantedCell, CubeDepth2, CubeDepth2, fx, fy,DirX*Time,DirY*Time); // new peice
					AddCube(rElement.m_Cell, CubeDepth2*2, TargetDepth-CubeDepth2*2, fx, fy,0,0);
				}
				else if(rActive.m_Stage == 3)
				{
					AddCube(rElement.m_Cell, 0, CubeDepth2, fx, fy,0,0);
					AddCube(rElement.m_Cell, CubeDepth2*2-CubeDepth2*Time, TargetDepth-CubeDepth2*2, fx, fy,0,0);
					AddCube(rActive.m_WantedCell, CubeDepth2+(TargetDepth-CubeDepth2*2)*Time, CubeDepth2, fx, fy,DirX,DirY); // new peice
				}
				else if(rActive.m_Stage == 4)
				{
					AddCube(rElement.m_Cell, 0, TargetDepth-CubeDepth2, fx, fy, 0,0);
					AddCube(rActive.m_WantedCell, TargetDepth-CubeDepth2, CubeDepth2, fx, fy, DirX*InvTime,DirY*InvTime); // new peice
				}
				else if(rActive.m_Stage >= 5) // happens when the block is done changing
				{
					rElement.m_Cell = rActive.m_WantedCell;
					rActive.m_CurrentDepth = TargetDepth;
					rActive.m_Stage = 0;
					//pSide->UnblockElement(x,y);

					AddCube(rElement.m_Cell, 0, rActive.m_CurrentDepth, fx, fy, 0, 0);
				}

				if(rActive.m_CurrentDepth < 0.0001f && rActive.m_WantedDepth < 0.0001f && rActive.m_Stage == 0)
					pSide->DeactivateElement(x,y);
			}
			else
			{
				AddCube(rElement.m_Cell, 0, 0, fx, fy, 0, 0);
			}

			if (m_bBufferFull)
				return y * CUBE_RES + x + 1;

			//if(y != CUBE_RES-1 && x != CUBE_RES-1)
			//	return _StartIndex+1;
		}

		//if(y != CUBE_RES-1)
		//	return _StartIndex+CUBE_RES;
		Startx = 0;
	}

	//M_TRACEALWAYS("checkpoint\n");
	return -1;
};

//
//
//
fp32 PulseIn(fp32 _Time)
{
	return (1/0.89662745f)*(3*M_Cos(_Time-1.5f) + 5*M_Cos(_Time))*0.17f*M_Pow((fp32)_NLOG, -0.2f*_Time);
}

//
//
//
CMat4Dfp32 CCube::GetCurrentMatrix()
{
	MSCOPESHORT(CCube::GetCurrentMatrix);
	//	if(0)
	if(m_MoveTime > 0)
	{
		if(m_MoveTime > 1)
			m_MoveTime = 1;


		// rotation
		CMat4Dfp32 MatTrans;
		m_Current.Transpose(MatTrans);
		CMat4Dfp32 Mat = MatTrans * m_Wanted;
		CQuatfp32 Quat;
		Quat.Create(Mat);

		CVec3Dfp32 Axis;

		fp32 Angle = Quat.CreateAxisAngle(Axis);
		CMat4Dfp32 Temp;
		Axis.CreateAxisRotateMatrix(Angle*(1-PulseIn(m_MoveTime*15)*M_Pow(1-m_MoveTime, MOVEMENT_DAMPING)), Temp);
		Temp = m_Current*Temp;
		//		fp32 AdjustedTime = m_MoveTime*5;

		CVec3Dfp32::GetRow(Temp,3) = CVec3Dfp32(0,0,0);

		return Temp;
	}

	return m_Current;
}

void CCube::SetDefaultSequenceParams() const
{
	// default values
	m_Offset = CVec3Dfp32(0,0,0);
	m_RotationOffset = CVec3Dfp32(0,0,0);
	m_CanDoLayout = true;
	m_CanMove = true;
	m_KeyLock = false;
	m_TimeScale = 1;
	m_LightIntens = 1;
	m_LightRange = 5;
	m_BgItens = 1;
	m_ViewportMorph = 0;

	m_AddLayer_R = 0;
	m_AddLayer_G = 0;
	m_AddLayer_B = 0;
	m_AddLayer_Intens = 0;
}
//
//
//
void CCube::UpdateSequence(fp32 _DeltaTime) const
{
	if(m_SequenceOverride)
	{
		m_SequenceOverride--;
		return;
	}

	// update sequence
	if(!m_pCurrentSequence)
	{
		SetDefaultSequenceParams();
		return;
	}

	// update time
	m_SequenceTime += _DeltaTime*1.0f;

	m_AddLayer_R = m_pCurrentSequence->GetValue(TRACK_ADDLAYER_R, m_SequenceTime);
	m_AddLayer_G = m_pCurrentSequence->GetValue(TRACK_ADDLAYER_G, m_SequenceTime);
	m_AddLayer_B = m_pCurrentSequence->GetValue(TRACK_ADDLAYER_B, m_SequenceTime);
	m_AddLayer_Intens = m_pCurrentSequence->GetValue(TRACK_ADDLAYER_INTENS, m_SequenceTime);

	m_Offset = m_pCurrentSequence->GetVector(TRACK_OFFSET_X, m_SequenceTime);
	m_RotationOffset = m_pCurrentSequence->GetVector(TRACK_ROT_X, m_SequenceTime);
	m_TimeScale = m_pCurrentSequence->GetValue(TRACK_TIMESCALE, m_SequenceTime);

	m_CanDoLayout = m_pCurrentSequence->GetValue(TRACK_MAPBLOCK, m_SequenceTime) > 0.5f;
	m_CanMove = m_pCurrentSequence->GetValue(TRACK_MOVEBLOCK, m_SequenceTime) > 0.5f;

	m_LightIntens = m_pCurrentSequence->GetValue(TRACK_LIGHT_INTENS, m_SequenceTime);
	m_LightRange = m_pCurrentSequence->GetValue(TRACK_LIGHT_RANGE, m_SequenceTime);

	if(m_LightRange == 0)
		m_LightRange = 5;

	m_BgItens = m_pCurrentSequence->GetValue(TRACK_BGINTENS, m_SequenceTime);

	m_KeyLock = m_pCurrentSequence->GetValue(TRACK_KEYLOCK, m_SequenceTime) > 0.1f;

	m_ViewportMorph = m_pCurrentSequence->GetValue(TRACK_VIEWPORT, m_SequenceTime);

	if(m_pCurrentSequence->GetValue(TRACK_DROPGUI, m_SequenceTime) > 0.5f)
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if(pCon)
			pCon->ExecuteString("cg_clearmenus()");
	}

	//m_CanChange
}


//
//
//
void CCube::SetSequence(CSequence *_pSequence)
{
	m_pCurrentSequence = _pSequence;
	m_SequenceTime = 0;
}

//
//
//

int CCube::CRenderBatch::AddEntry(CMatrixPaletteEntry &_Entry)
{
	M_ASSERT(m_iMatrixPalette < ECubeMaxMPEntries, "Overstep");

	m_pMatrixPalette[m_iMatrixPalette] = _Entry;
	++m_iMatrixPalette;
	return Full();
}

void CCube::CRenderBatch::BeginBatch(CXR_VBManager *_pVBM)
{
	m_pMatrixPalette = (CMatrixPaletteEntry *)_pVBM->Alloc(ECubeMaxMPEntries * sizeof(CMatrixPaletteEntry));
	M_ASSERT(m_pMatrixPalette, "Alloc failed");
	m_iMatrixPalette = 0;
}

//#define DShadowDebug
void CCube::InitStaticAttribs()
{
	m_Attrib_RenderShading.SetDefault();

	m_Attrib_RenderShading.m_iTexCoordSet[0] = 0;
	m_Attrib_RenderShading.m_iTexCoordSet[1] = 0;
	m_Attrib_RenderShading.m_iTexCoordSet[2] = 1;
	m_Attrib_RenderShading.m_iTexCoordSet[3] = 2;

	m_Attrib_Shadow0.SetDefault();

	// general parameters
	m_Attrib_Shadow0.m_iTexCoordSet[1] = 0; m_Attrib_Shadow0.m_iTexCoordSet[2] = 1; m_Attrib_Shadow0.m_iTexCoordSet[3] = 2;
	m_Attrib_Shadow0.Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME, CRC_TEXGENCOMP_ALL);
	for( int i = 1; i < CRC_MAXTEXTURES; i++ )
		m_Attrib_Shadow0.Attrib_TexGen(i, CRC_TEXGENMODE_VOID, 0);
	m_Attrib_Shadow0.Attrib_ZCompare(CRC_COMPARE_LESS);

	m_Attrib_Shadow0.Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL | CRC_FLAGS_CULLCW | CRC_FLAGS_STENCIL);
	m_Attrib_Shadow0.Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
	m_Attrib_Shadow0.Attrib_RasterMode(CRC_RASTERMODE_NONE);

	// stencil operation
	m_Attrib_Shadow0.Attrib_StencilRef(1, 255);
	m_Attrib_Shadow0.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
	m_Attrib_Shadow0.Attrib_StencilWriteMask(255);

	m_Attrib_Shadow1 = m_Attrib_Shadow0;

	m_Attrib_Shadow1.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
	m_Attrib_Shadow1.Attrib_Enable(CRC_FLAGS_CULL);
	m_Attrib_Shadow1.Attrib_Disable(CRC_FLAGS_CULLCW);

#ifdef DShadowDebug
	// general parameters
	m_Attrib_Shadow0.m_iTexCoordSet[1] = 0; m_Attrib_Shadow0.m_iTexCoordSet[2] = 1; m_Attrib_Shadow0.m_iTexCoordSet[3] = 2;
	m_Attrib_Shadow0.Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME, CRC_TEXGENCOMP_ALL);
	for( int i = 1; i < CRC_MAXTEXTURES; i++ )
		m_Attrib_Shadow0.Attrib_TexGen(i, CRC_TEXGENMODE_VOID, 0);
	m_Attrib_Shadow0.Attrib_ZCompare(CRC_COMPARE_LESS);

	m_Attrib_Shadow0.Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL | CRC_FLAGS_CULLCW | CRC_FLAGS_STENCIL);
	m_Attrib_Shadow0.Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ALPHAWRITE);
	m_Attrib_Shadow0.Attrib_RasterMode(CRC_RASTERMODE_ADD);

	// stencil operation
	m_Attrib_Shadow0.Attrib_StencilRef(1, 255);
	m_Attrib_Shadow0.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
	m_Attrib_Shadow0.Attrib_StencilWriteMask(255);

	m_Attrib_Shadow1 = m_Attrib_Shadow0;

	m_Attrib_Shadow1.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
	m_Attrib_Shadow1.Attrib_Enable(CRC_FLAGS_CULL);
	m_Attrib_Shadow1.Attrib_Disable(CRC_FLAGS_CULLCW);
#endif

	// Mul
	m_Attrib_Mul.SetDefault();
	m_Attrib_Mul.Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_BLEND | CRC_FLAGS_CULL);
	m_Attrib_Mul.Attrib_Disable(CRC_FLAGS_ZWRITE);
	m_Attrib_Mul.Attrib_ZCompare(CRC_COMPARE_EQUAL);
	m_Attrib_Mul.Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
	for( int i = 1; i < CRC_MAXTEXTURES; i++ )
		m_Attrib_Mul.Attrib_TexGen(i, CRC_TEXGENMODE_VOID, 0);

	CMat4Dfp32 Matrix;
	Matrix.Unit();
	Matrix.k[3][0] = 0.5;
	Matrix.k[3][1] = 0.5;
	static CVec4Dfp32 MulTexTrans[4];
	MulTexTrans[0][0] = Matrix.k[0][0];
	MulTexTrans[0][1] = Matrix.k[1][0];
	MulTexTrans[0][2] = Matrix.k[2][0];
	MulTexTrans[0][3] = Matrix.k[3][0];
	MulTexTrans[1][0] = Matrix.k[0][1];
	MulTexTrans[1][1] = Matrix.k[1][1];
	MulTexTrans[1][2] = Matrix.k[2][1];
	MulTexTrans[1][3] = Matrix.k[3][1];
	MulTexTrans[2][0] = Matrix.k[0][2];
	MulTexTrans[2][1] = Matrix.k[1][2];
	MulTexTrans[2][2] = Matrix.k[2][2];
	MulTexTrans[2][3] = Matrix.k[3][2];
	MulTexTrans[3][0] = Matrix.k[0][3];
	MulTexTrans[3][1] = Matrix.k[1][3];
	MulTexTrans[3][2] = Matrix.k[2][3];
	MulTexTrans[3][3] = Matrix.k[3][3];
	m_Attrib_Mul.Attrib_TexGenAttr((fp32 *)&MulTexTrans);

	// ColorAdd
	m_Attrib_ColorAdd.SetDefault();
	m_Attrib_ColorAdd.Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_BLEND | CRC_FLAGS_CULL);
	m_Attrib_ColorAdd.Attrib_Disable(CRC_FLAGS_ZWRITE);
	m_Attrib_ColorAdd.Attrib_ZCompare(CRC_COMPARE_EQUAL);
	m_Attrib_ColorAdd.Attrib_RasterMode(CRC_RASTERMODE_ADD);

	// secondary
	m_Attrib_ColorSecondary = m_Attrib_Mul;
	//m_Attrib_ColorSecondary.Attrib_RasterMode(CRC_RASTERMODE_ADD);
	//m_Attrib_ColorSecondary.Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);
	m_Attrib_ColorSecondary.Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	//m_Attrib_ColorSecondary.Attrib_TextureID(0, m_MulTextureID);
}

void CCube::UpdateAttribsSideDependant(CRenderContext *_pRC, CXR_VBManager* _pVBM)
{
	// Shadow
	{
		m_pAttrib_Shadow0 = NULL;
		m_pAttrib_Shadow1 = NULL;
		m_pAttrib_Shadow0PolyOffset = NULL;
		m_pAttrib_Shadow1PolyOffset = NULL;

		CVec4Dfp32 *pLight = _pVBM->Alloc_V4(1);
		if(!pLight)
			return;
		// set light
		CVec3Dfp32 Temp;
		CVec4Dfp32 Light;
		m_LightPosition.MultiplyMatrix(m_aSides[m_iCurrentSide].m_InvCameraMatrix, Temp);
		Light = Temp;
		Light.k[3] = 0.5;
		*pLight = Light;

		for(int32 p = 0; p < 2; p++) // two passes
		{
			CRC_Attributes *pAttrib = _pVBM->Alloc_Attrib();;

			if (!pAttrib)
				break;

			if(p == 0)
			{
				*pAttrib = m_Attrib_Shadow0;
				m_pAttrib_Shadow0 = pAttrib;
			}
			else
			{
				*pAttrib = m_Attrib_Shadow1;
				m_pAttrib_Shadow1 = pAttrib;
			}

			pAttrib->Attrib_TexGenAttr(pLight->k);
		}

		for(int32 p = 0; p < 2; p++) // two passes
		{
			CRC_Attributes *pAttrib = _pVBM->Alloc_Attrib();;

			if (!pAttrib)
				break;

			if(p == 0)
			{
				*pAttrib = m_Attrib_Shadow0;
				m_pAttrib_Shadow0PolyOffset = pAttrib;
			}
			else
			{
				*pAttrib = m_Attrib_Shadow1;
				m_pAttrib_Shadow1PolyOffset = pAttrib;
			}

			pAttrib->Attrib_PolygonOffset(1, 1);
			pAttrib->Attrib_TexGenAttr(pLight->k);
		}
	}
	// Light
	{
		m_RenderShadingLight = CXR_Light(m_LightPosition, CVec3Dfp32(m_LightIntens,m_LightIntens,m_LightIntens), m_LightRange);
		m_RenderShadingLight.Transform(m_aSides[m_iCurrentSide].m_InvCameraMatrix);
	}

	// Matrix
	{
		m_pMatrix_CameraMatrix = NULL;
		CMat4Dfp32 *pMatrix = _pVBM->Alloc_M4();
		if(!pMatrix)
			return;

		*pMatrix = m_aSides[m_iCurrentSide].m_CameraMatrix;
		m_pMatrix_CameraMatrix = pMatrix;
	}


	// Update secondary shader here
	m_pSurfaceSecondary = NULL; // Set to surface for current side

	if(m_aSides[m_iCurrentSide].m_iSecondaryID >= 0)
	{/*
	 if(m_aSides[m_iCurrentSide].m_SecondaryIsSurface)
	 {
	 MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	 if(pSC)
	 m_pSurfaceSecondary = pSC->GetSurface(m_aSides[m_iCurrentSide].m_iSecondaryID);
	 m_ShaderSecondary.PrepareFrame(_pRC, m_spVBM);

	 // create shader parameters
	 {
	 m_RenderShadingParamsSecondary.Clear();

	 CXW_SurfaceKeyFrame *pKeyFrame = m_pSurfaceSecondary->GetBaseFrame();
	 if(pKeyFrame)
	 m_RenderShadingParamsSecondary.Create(*pKeyFrame);
	 m_RenderShadingParamsSecondary.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST; //|XR_SHADERFLAGS_USEZLESS;
	 }
	 }
	 else
	 {*/
		m_Attrib_ColorSecondary.Attrib_TexGenAttr((fp32 *)&m_aSides[m_iCurrentSide].m_aSecondaryTextureTransform);
		m_Attrib_ColorSecondary.Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
		m_Attrib_ColorSecondary.Attrib_TextureID(0, m_aSides[m_iCurrentSide].m_iSecondaryID);
		m_Attrib_ColorSecondary.Attrib_RasterMode(m_aSides[m_iCurrentSide].m_BlendMode);

		//if(m_aSides[m_iCurrentSide].m_iSecondaryID > 0)
		//}
	}
}

void CCube::UpdateAttribs(CRenderContext *_pRC, CXR_VBManager* _pVBM)
{
	m_pAttrib_ZBuffer = NULL;
	m_pAttrib_ZBufferWholeSide = NULL;
	m_pAttrib_Shading = NULL;
	// ZBuffer
	{

		CRC_Attributes *pAttrib = _pVBM->Alloc_Attrib();;
		if (!pAttrib)
			return;
		m_pAttrib_ZBuffer = pAttrib;

		pAttrib->SetDefault();
		pAttrib->m_iTexCoordSet[0] = 0;
		pAttrib->m_iTexCoordSet[1] = 0;
		pAttrib->m_iTexCoordSet[2] = 1;
		pAttrib->m_iTexCoordSet[3] = 2;
		pAttrib->Attrib_Enable(CRC_FLAGS_STENCIL | CRC_FLAGS_ZWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL); // 
		pAttrib->Attrib_StencilRef(128, -1);
		pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
		pAttrib->Attrib_StencilWriteMask(255);
	}
	// ZBuffer Whole side
	{

		CRC_Attributes *pAttrib = _pVBM->Alloc_Attrib();;
		if (!pAttrib)
			return;
		m_pAttrib_ZBufferWholeSide = pAttrib;
		*m_pAttrib_ZBufferWholeSide = *m_pAttrib_ZBuffer;
		//		m_pAttrib_ZBufferWholeSide->Attrib_PolygonOffset(2.0, 2.0);
	}


	// create shader parameters
	{
		m_SSP.Clear();
		m_RenderShadingParams.Clear();

		CXW_SurfaceKeyFrame *pKeyFrame = m_pSurface->GetBaseFrame();
		if(pKeyFrame)
			m_SSP.Create(m_pSurface, pKeyFrame);
		m_RenderShadingParams.Create(NULL, NULL, &m_Shader);
		m_RenderShadingParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST; //|XR_SHADERFLAGS_USEZLESS;
	}

	m_MulTextureID = _pRC->Texture_GetTC()->GetTextureID(m_MulTextureName);

	m_Attrib_Mul.Attrib_RasterMode(m_MulMode);
	m_Attrib_Mul.Attrib_TextureID(0, m_MulTextureID);
}


void CCube::StartMPBuild(CXR_VBManager* _pVBM)
{
	for (int i = 0; i < ERenderBatch_Max; ++i)
	{
		if (m_RenderBatches[i].Full())
			m_RenderBatches[i].BeginBatch(_pVBM);
	}

	m_bBufferFull = false;
}

CRC_MatrixPalette *CCube::AllocMatrixPalette(CXR_VBManager* _pVBM, uint32 _Flags, CMatrixPaletteEntry *_pEntries)
{
	CRC_MatrixPalette *pMP = _pVBM->Alloc_MP();
	if(!pMP)
		return NULL;
	pMP->m_Flags = _Flags;
	pMP->m_piMatrices = NULL;
	pMP->m_pMatrices = _pEntries;
	pMP->m_nMatrices = ECubeMaxMPEntries;
	return pMP;
}

void CCube::EndMPBuild(CXR_VBManager* _pVBM, bool _bFlush)
{
	{
		const uint32 FlagsPos = ERC_MatrixPaletteFlags_SpecialCubePosition | ERC_MatrixPaletteFlags_DoNotCache;
		const uint32 FlagsTxt = ERC_MatrixPaletteFlags_SpecialCubePosition | ERC_MatrixPaletteFlags_SpecialCubeTexture | ERC_MatrixPaletteFlags_DoNotCache;
		const uint32 FlagsSclTxt = ERC_MatrixPaletteFlags_SpecialCubePosition | ERC_MatrixPaletteFlags_SpecialCubeTexture | ERC_MatrixPaletteFlags_SpecialCubeTextureScaleByZ | ERC_MatrixPaletteFlags_DoNotCache;
		const uint32 FlagsScl2 = ERC_MatrixPaletteFlags_SpecialCubeTextureScale2 | ERC_MatrixPaletteFlags_DoNotCache;
		const static uint32 MeshFlags[][5] = 
		{
			{FlagsPos, FlagsSclTxt, FlagsSclTxt, FlagsPos, FlagsPos},
			{FlagsPos, FlagsSclTxt, FlagsSclTxt, FlagsPos, FlagsPos},
			{FlagsPos, FlagsTxt, FlagsSclTxt, FlagsPos, FlagsPos},
			{FlagsPos, FlagsTxt, FlagsSclTxt, FlagsPos, FlagsPos},
			{FlagsPos, FlagsSclTxt, FlagsPos, FlagsPos, FlagsPos},
			{FlagsPos, FlagsSclTxt, FlagsPos, FlagsPos, FlagsPos},
			{FlagsPos, FlagsSclTxt|FlagsScl2, FlagsSclTxt, FlagsPos, FlagsPos},
			{FlagsPos, FlagsTxt|FlagsScl2, FlagsSclTxt, FlagsPos, FlagsPos},
		};
#if 1
		const static int MeshTranslate[][5] = 
		{
			{ECubeMesh_Cube,		ECubeMesh_Cube,			-1,					ECubeMesh_ShadowCube,		ECubeMesh_Cube},
			{ECubeMesh_SplitCube,	ECubeMesh_SplitCube,	-1,					ECubeMesh_ShadowCube,		ECubeMesh_SplitCube},
			{ECubeMesh_Front,		ECubeMesh_Front,		-1,					-1,							ECubeMesh_Front},
			{ECubeMesh_SplitFront,	ECubeMesh_SplitFront,	-1,					ECubeMesh_ShadowFront,		ECubeMesh_SplitFront},
			{ECubeMesh_Cube,		ECubeMesh_Cube,			ECubeMesh_Front,	ECubeMesh_ShadowCube,		ECubeMesh_Cube},
			{ECubeMesh_Front,		ECubeMesh_Front/*-1*/,	ECubeMesh_Front,	ECubeMesh_ShadowFront,		ECubeMesh_Front},
			{ECubeMesh_Cube,		ECubeMesh_Cube,			-1,					ECubeMesh_ShadowCube,		ECubeMesh_Cube},
			{ECubeMesh_Front,		ECubeMesh_Front,		-1,					-1,							ECubeMesh_Front},			
		};
#else

		int MeshTranslate[][5] = 
		{
			{-1,		-1,			-1,			ECubeMesh_ShadowCube,			-1},
			{-1,		-1,			-1,			-1,			-1},
			{-1,		-1,			-1,			-1,			-1},
			{-1,		-1,			-1,			-1,			-1},
			{-1,		-1,			-1,			-1,			-1},
			{-1,		-1,			-1,			-1,			-1},
		};
#endif

		for(int i = 0; i < ERenderBatch_Max; ++i)		
		{
			if ((m_RenderBatches[i].Full() || (_bFlush && (m_RenderBatches[i].m_iMatrixPalette != 0))) && m_RenderBatches[i].m_pMatrixPalette)
			{
				m_RenderBatches[i].FillRemaining();

				if (MeshTranslate[i][0] != -1)
					RenderZ(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][0], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][0]));


				if (MeshTranslate[i][1] != -1)
					RenderShading(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][1], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][1]));

				if (MeshTranslate[i][2] != -1)
				{
					if(m_aSides[m_iCurrentSide].m_iSecondaryID >= 0)
					{
						/*
						if(m_aSides[m_iCurrentSide].m_SecondaryIsSurface)
						RenderShadingSecondary(AllocMatrixPalette(MeshFlags[i][2], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][2]));
						else
						*/
						RenderShadingSecondaryTexture(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][2], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][2]));
					}
				}

				if (MeshTranslate[i][3] != -1)
					RenderShadow(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][3], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][3]));

				if (MeshTranslate[i][4] != -1)
				{
					if(m_AddLayer_Intens > 0.01f)
					{
						fp32 Amount = m_AddLayer_Intens; // 0-1
						fp32 InvAmount = 1-Amount;
						CPixel32 ColorMul = CPixel32::From_fp32(InvAmount*255,InvAmount*255,InvAmount*255,255);
						CPixel32 ColorAdd = CPixel32::From_fp32(Amount*0xFF*m_AddLayer_R,
							Amount*0xFF*m_AddLayer_G,
							Amount*0xFF*m_AddLayer_B,Amount*100);
						RenderMul(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][4], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][4]), ColorMul);
						RenderColorAdd(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][4], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][4]), ColorAdd);
					}
					/*
					if(m_ViewportMorph > 0.5f)
					{
					fp32 Amount = 1-(m_ViewportMorph-0.5f)*2; // 0-1
					fp32 InvAmount = 1-Amount;
					CPixel32 ColorMul(Amount*255,Amount*255,Amount*255,255);
					CPixel32 ColorAdd(InvAmount*100,InvAmount*100,InvAmount*100*(220/255.0f),InvAmount*100);
					RenderMul(AllocMatrixPalette(MeshFlags[i][4], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][4]), ColorMul);
					RenderColorAdd(AllocMatrixPalette(MeshFlags[i][4], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][4]), ColorAdd);
					}*/
					else
					{
						RenderMul(_pVBM, AllocMatrixPalette(_pVBM, MeshFlags[i][4], m_RenderBatches[i].m_pMatrixPalette), m_VertexBuffers.GetID(MeshTranslate[i][4]), CPixel32(255,255,255,255));
					}
				}


				m_RenderBatches[i].m_pMatrixPalette = NULL;
			}
		}
	}
}

void CCube::RenderShading(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID)
{
	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	pBuffer->m_pMatrixPaletteArgs = _pMP;
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	pBuffer->m_pAttrib = &m_Attrib_RenderShading;

	const CXR_SurfaceShaderParams* lpSSP[1] = { &m_SSP };
	m_Shader.RenderShading(m_RenderShadingLight, pBuffer, &m_RenderShadingParams, lpSSP);
}

void CCube::RenderShadingSecondaryTexture(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID)
{
	if(!_pMP)
		return;

	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();

	pBuffer->m_pMatrixPaletteArgs = _pMP;
	pBuffer->m_pAttrib = _pVBM->Alloc_Attrib();

	if(!pBuffer->m_pAttrib)
		return;

	*pBuffer->m_pAttrib = m_Attrib_ColorSecondary;
	pBuffer->m_Color = CPixel32(0xFF,0xFF,0xFF,0xff);
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);

	// render
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	pBuffer->m_Priority = 1000000-1; // last thing, almost
	_pVBM->AddVB(pBuffer);


	/*
	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
	return;

	pBuffer->Clear();
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	pBuffer->m_pMatrixPaletteArgs = _pMP;
	pBuffer->Render_VertexBuffer(_VBID);
	//	m_RenderShadingParams.m_lTextureIDs
	m_ShaderSecondary.RenderShading(m_RenderShadingLight, pBuffer, &m_RenderShadingParams);
	*/
}

/*
void CCube::RenderShadingSecondary(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID)
{
	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	pBuffer->m_pMatrixPaletteArgs = _pMP;
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	m_ShaderSecondary.RenderShading(m_RenderShadingLight, pBuffer, &m_RenderShadingParamsSecondary);
}
*/

void CCube::RenderZ(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID)
{
	if(!_pMP)
		return;

	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	pBuffer->m_pMatrixPaletteArgs = _pMP;
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	pBuffer->m_Color = CPixel32(0,0,0,255);
	pBuffer->m_Priority = 0.0; // first thing
	pBuffer->m_pAttrib = m_pAttrib_ZBuffer;
	_pVBM->AddVB(pBuffer);
}

void CCube::RenderShadow(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID)
{
	for(int32 p = 0; p < 2; p++) // two passes
	{
		CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

		if(!pBuffer)
			break;
		pBuffer->Clear();
		if(p == 0)
			pBuffer->m_pAttrib = m_pAttrib_Shadow0PolyOffset;
		else
			pBuffer->m_pAttrib = m_pAttrib_Shadow1PolyOffset;
		// matrix and then render
		pBuffer->m_Priority = 10000000; // last thing
		pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
		pBuffer->m_pMatrixPaletteArgs = _pMP;
		if (!pBuffer->AllocVBChain(_pVBM, true))
			return;
		pBuffer->Render_VertexBuffer(_VBID);
#ifdef DShadowDebug
		pBuffer->Geometry_Color(CPixel32(p*40,(1-p) * 40,0,0));
#endif

		_pVBM->AddVB(pBuffer);
	}
}

void CCube::RenderMul(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID, CPixel32 _Color)
{
	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();

	pBuffer->m_pMatrixPaletteArgs = _pMP;
	pBuffer->m_pAttrib = &m_Attrib_Mul;
	pBuffer->m_Color = _Color;
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	pBuffer->m_Priority = 1000000; // last thing
	_pVBM->AddVB(pBuffer);
}

void CCube::RenderColorAdd(CXR_VBManager* _pVBM, CRC_MatrixPalette *_pMP, int _VBID, CPixel32 _Color)
{
	CXR_VertexBuffer *pBuffer = _pVBM->Alloc_VB();

	if(!pBuffer)
		return;

	pBuffer->Clear();

	pBuffer->m_pMatrixPaletteArgs = _pMP;
	pBuffer->m_pAttrib = &m_Attrib_ColorAdd;
	pBuffer->m_Color = _Color;
	pBuffer->Matrix_Set(m_pMatrix_CameraMatrix);
	if (!pBuffer->AllocVBChain(_pVBM, true))
		return;
	pBuffer->Render_VertexBuffer(_VBID);
	pBuffer->m_Priority = 1000001; // last thing
	_pVBM->AddVB(pBuffer);
}


void CCube::CVertexBuffers::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	// We need:
	// Vertices
	// Normals
	// Texture coordinates
	// Tangents
	// Matrix palette indices

	// Matrix palette indices are as follows:
	// 0: Palette index
	// 1: Texture translate 0 scale
	// 2: Texture translate 1 scale
	// 2: Texture translate Scale Z scale

	switch (_iLocal)
	{
	case ECubeMesh_SplitFront:
	case ECubeMesh_Front:
	case ECubeMesh_SplitCube:
	case ECubeMesh_Sides:
	case ECubeMesh_Cube:
		{

			int nVertPerPrim = ms_nVertices[_iLocal];
			int nIndPerPrim = ms_nIndices[_iLocal];

			int nVertices = nVertPerPrim * ECubeMaxMPEntries;
			int nIndices = nIndPerPrim * ECubeMaxMPEntries;

			int NeededMemory = nIndices * 2 + nVertices * (sizeof(CVec3Dfp32) + sizeof(CVec3Dfp32)+ sizeof(CVec2Dfp32) + sizeof(CVec3Dfp32) * 2 + sizeof(uint32));
			m_TempData.SetLen(NeededMemory);
			uint8 *pData = m_TempData.GetBasePtr();

			_VB.m_nV = nVertices;
			_VB.Geometry_VertexArray((CVec3Dfp32 *)pData); pData += sizeof(CVec3Dfp32) * nVertices;
			_VB.Geometry_NormalArray((CVec3Dfp32 *)pData); pData += sizeof(CVec3Dfp32) * nVertices;
			_VB.Geometry_MatrixIndex0Int((uint32 *)pData); pData += sizeof(uint32) * nVertices;
			_VB.Geometry_TVertexArray((CVec2Dfp32 *)pData, 0); pData += sizeof(CVec2Dfp32) * nVertices;
			_VB.Geometry_TVertexArray((CVec3Dfp32 *)pData, 1); pData += sizeof(CVec3Dfp32) * nVertices;
			_VB.Geometry_TVertexArray((CVec3Dfp32 *)pData, 2); pData += sizeof(CVec3Dfp32) * nVertices;
			//			_VB.Geometry_TVertexArray(NULL, 3);
			_VB.m_piPrim = (uint16 *)pData;
			_VB.m_nPrim = nIndices / 3;
			_VB.m_PrimType = CRC_RIP_TRIANGLES;

			memset(_VB.m_lpVReg[CRC_VREG_TEXCOORD1], 0, nVertPerPrim * sizeof(CVec3Dfp32));
			memset(_VB.m_lpVReg[CRC_VREG_TEXCOORD2], 0, nVertPerPrim * sizeof(CVec3Dfp32));
			CalcTangents(ms_pIndices[_iLocal], nIndPerPrim / 3, nVertPerPrim, (CVec3Dfp32 *)ms_pVertices[_iLocal], (CVec3Dfp32 *)ms_pNormals[_iLocal],
				(CVec2Dfp32 *)ms_pDiffuseTexture[_iLocal], (CVec3Dfp32 *)((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD1]), (CVec3Dfp32 *)((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD2]));

			// Fill buffers
			for (int i = 0; i < ECubeMaxMPEntries; ++i)
			{
				// Indices
				for (int j = 0; j < nIndPerPrim; ++j)
					_VB.m_piPrim[i * nIndPerPrim + j] = ms_pIndices[_iLocal][j] + i*nVertPerPrim;
				// Vertices
				memcpy(((CVec3Dfp32 *)_VB.m_lpVReg[CRC_VREG_POS]) + i * nVertPerPrim, ms_pVertices[_iLocal], nVertPerPrim * sizeof(CVec3Dfp32));
				// Normals
				memcpy(((CVec3Dfp32 *)_VB.m_lpVReg[CRC_VREG_NORMAL]) + i * nVertPerPrim, ms_pNormals[_iLocal], nVertPerPrim * sizeof(CVec3Dfp32));
				// Indices
				for (int j = 0; j < nVertPerPrim; ++j)
					((uint32 *)_VB.m_lpVReg[CRC_VREG_MI0])[i * nVertPerPrim + j] = (i) | ((uint32)((ms_pTextureMatrixPalette[_iLocal][j])&1) << 8) | ((uint32)((ms_pTextureMatrixPalette[_iLocal][j])>>1) << 16) | ((uint32)ms_pTextureScaleZ[_iLocal][j] << 24);
				// Diffuse texture
				memcpy(((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD0]) + i * nVertPerPrim * 2, ms_pDiffuseTexture[_iLocal], nVertPerPrim * sizeof(CVec2Dfp32));
				if (i)
				{
					// TangentU
					memcpy(((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD1]) + i * nVertPerPrim * 3, ((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD1]), nVertPerPrim * sizeof(CVec3Dfp32));
					// TangentV
					memcpy(((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD2]) + i * nVertPerPrim * 3, ((fp32 *)_VB.m_lpVReg[CRC_VREG_TEXCOORD2]), nVertPerPrim * sizeof(CVec3Dfp32));
				}
			}

			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD1, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD2, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_MI0, CRC_VREGFMT_N4_UI8_P32);
		}
		break;
	case ECubeMesh_ShadowFront:
	case ECubeMesh_ShadowCube:
		{

			int nVertPerPrim = ms_nVertices[_iLocal];
			int nIndPerPrim = ms_nIndices[_iLocal];

			int nVertices = nVertPerPrim * ECubeMaxMPEntries;
			int nIndices = nIndPerPrim * ECubeMaxMPEntries;

			int NeededMemory = nIndices * 2 + nVertices * (sizeof(CVec3Dfp32) + sizeof(CVec3Dfp32) + sizeof(uint32));
			m_TempData.SetLen(NeededMemory);
			uint8 *pData = m_TempData.GetBasePtr();

			_VB.m_nV = nVertices;
			_VB.Geometry_VertexArray((CVec3Dfp32 *)pData); pData += sizeof(CVec3Dfp32) * nVertices;
			_VB.Geometry_NormalArray((CVec3Dfp32 *)pData); pData += sizeof(CVec3Dfp32) * nVertices;
			_VB.Geometry_MatrixIndex0((uint32 *)pData); pData += sizeof(uint32) * nVertices;
			_VB.m_piPrim = (uint16 *)pData;
			_VB.m_nPrim = nIndices / 3;
			_VB.m_PrimType = CRC_RIP_TRIANGLES;

			// Fill buffers
			for (int i = 0; i < ECubeMaxMPEntries; ++i)
			{
				// Indices
				for (int j = 0; j < nIndPerPrim; ++j)
					_VB.m_piPrim[i * nIndPerPrim + j] = ms_pIndices[_iLocal][j] + i*nVertPerPrim;
				// Vertices
				memcpy(((CVec3Dfp32 *)_VB.m_lpVReg[CRC_VREG_POS]) + i * nVertPerPrim, ms_pVertices[_iLocal], nVertPerPrim * sizeof(CVec3Dfp32));
				// Normals
				memcpy(((CVec3Dfp32 *)_VB.m_lpVReg[CRC_VREG_NORMAL]) + i * nVertPerPrim, ms_pNormals[_iLocal], nVertPerPrim * sizeof(CVec3Dfp32));
				// Indices
				for (int j = 0; j < nVertPerPrim; ++j)
					((uint32 *)_VB.m_lpVReg[CRC_VREG_MI0])[i * nVertPerPrim + j] = (i);
			}

			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
			_VB.Geometry_SetWantFormat(CRC_VREG_MI0, CRC_VREGFMT_N4_UI8_P32);
		}
		break;
	}


	/*			m_TempData;
	//_VB.Geometry_VertexArray(m_pVB->m_pVertex, m_pVB->m_Index, false);
	_VB.m_nV = m_pVB->m_Index;
	_VB.m_pV = m_pVB->m_pVertex;
	_VB.m_pN = m_pVB->m_pNormal;
	_VB.m_pTV[0] = (fp32*)m_pVB->m_pTexture;
	_VB.m_pTV[1] = (fp32*)m_pVB->m_pTangU;
	_VB.m_pTV[2] = (fp32*)m_pVB->m_pTangV;
	_VB.m_pTV[3] = (fp32*)m_pVB->m_pMulTexture;
	_VB.m_nTVComp[0] = 2;
	_VB.m_nTVComp[1] = 3;
	_VB.m_nTVComp[2] = 3;
	_VB.m_nTVComp[3] = 2;

	_VBGetOptions.m_Flags |=	EXR_VBGetFlags_CompressMinus1to1_Tex1 |
	EXR_VBGetFlags_CompressMinus1to1_Tex2;*/

}



//
//
//
void CCube::AddCube(CCellInfo &_rCell, fp32 _StartDepth, fp32 _Depth, fp32 _x, fp32 _y, fp32 _ox, fp32 _oy)
{
	MSCOPESHORT(CCube::AddCube);

	bool Flat = _Depth < 0.001f;
	_Depth *= CUBE_RES;
	_StartDepth *= CUBE_RES;
	/*	fp32 TimeScale = 0.2f;
	fp32 FrequencyScale = 0.65f;
	if (m_CurrentSide == m_RenderingSide)
	_Depth += .2 *
	(g_SinRandTable.GetRand((m_AnimTime + CMTime::CreateFromSeconds(((_x/7.0f)/TimeScale) * FrequencyScale)).GetTimeModulusScaled(TimeScale * 0.07f, 1.0f))
	+g_SinRandTable.GetRand((m_AnimTime + CMTime::CreateFromSeconds(((_y/11.0f)/TimeScale) * FrequencyScale)).GetTimeModulusScaled(TimeScale * 0.11f, 1.0f))
	+g_SinRandTable.GetRand((m_AnimTime + CMTime::CreateFromSeconds(((_x/13.0f)/TimeScale) * FrequencyScale)).GetTimeModulusScaled(TimeScale * 0.19f, 1.0f))
	+g_SinRandTable.GetRand((m_AnimTime + CMTime::CreateFromSeconds(((_y/7.0f)/TimeScale) * FrequencyScale)).GetTimeModulusScaled(TimeScale * 0.13f, 1.0f))
	);*/



	fp32 sd = _StartDepth + _Depth;
	fp32 x=CUBE_RES/2 - _x - 0.5f;
	fp32 y=_y - CUBE_RES/2 + 0.5f;

	fp32 tx, ty;

	x += _ox;
	y += _oy;

	// set texturing
	if(_rCell.m_Mode == CCellInfo::MODE_HUGE)
	{
		tx = (_rCell.Cell()%16)/16.0f+(_rCell.Part()/2)*(1/16.0f/2);
		ty = (_rCell.Cell()/16)/32.0f+(_rCell.Part()%2)*(1/32.0f/2);
	}
	else
	{
		tx = (_rCell.Cell()%16)/16.0f;
		/*
		if(_rCell.m_Mode == CCellInfo::MODE_SECONDARY)
		ty = (_rCell.Cell()/16)/16.0f; // adjust when using secondary
		else
		*/
		ty = (_rCell.Cell()/16)/32.0f; // adjust when using secondary
	}

	//x = 1-x;
	//y = 1-y;

	if(Flat)
	{
		if(m_FacingAway) // cull faces that are pointing away
			return;

		if(_rCell.m_Mode == CCellInfo::MODE_SMALL)
		{
			CMatrixPaletteEntry Entry0;
			Entry0.m_TranslateX = x;
			Entry0.m_TranslateY = y;
			Entry0.m_TranslateZ = sd;
			Entry0.m_ScaleZ = 1.0f; // Only a face, no scaling necesarry
			Entry0.m_TextureTranslateX0 = (_rCell.SubCell(0)%32)/32.0f;;
			Entry0.m_TextureTranslateY0 = (_rCell.SubCell(0)/32)/32.0f;//-shift;;
			Entry0.m_TextureTranslateX1 = (_rCell.SubCell(1)%32)/32.0f;
			Entry0.m_TextureTranslateY1 = (_rCell.SubCell(1)/32)/32.0f;
			m_bBufferFull |= m_RenderBatches[ERenderBatch_SplitFace].AddEntry(Entry0);
		}
		else
		{
			CMatrixPaletteEntry Entry0;
			Entry0.m_TranslateX = x;
			Entry0.m_TranslateY = y;
			Entry0.m_TranslateZ = sd;
			Entry0.m_ScaleZ = 1.0f; // Only a face, no scaling necesarry
			Entry0.m_TextureTranslateX0 = tx;
			Entry0.m_TextureTranslateY0 = ty;
			Entry0.m_TextureTranslateX1 = tx; // Not used
			Entry0.m_TextureTranslateY1 = ty; // Not used

			if (_rCell.m_Mode == CCellInfo::MODE_HUGE)
				m_bBufferFull |= m_RenderBatches[ERenderBatch_HugeFace].AddEntry(Entry0);
			else if(_rCell.m_Mode == CCellInfo::MODE_SECONDARY && (m_aSides[m_iCurrentSide].m_iSecondaryID > 0))
				m_bBufferFull |= m_RenderBatches[ERenderBatch_SecondaryFace].AddEntry(Entry0);
			else
				m_bBufferFull |= m_RenderBatches[ERenderBatch_Face].AddEntry(Entry0);
		}
	}
	else
	{
		if(_rCell.m_Mode == CCellInfo::MODE_SMALL)
		{
			CMatrixPaletteEntry Entry0;
			Entry0.m_TranslateX = x;
			Entry0.m_TranslateY = y;
			Entry0.m_TranslateZ = sd;
			Entry0.m_ScaleZ = _Depth;
			Entry0.m_TextureTranslateX0 = (_rCell.SubCell(0)%32)/32.0f;;
			Entry0.m_TextureTranslateY0 = (_rCell.SubCell(0)/32)/32.0f;//-shift;;
			Entry0.m_TextureTranslateX1 = (_rCell.SubCell(1)%32)/32.0f;
			Entry0.m_TextureTranslateY1 = (_rCell.SubCell(1)/32)/32.0f;
			m_bBufferFull |= m_RenderBatches[ERenderBatch_SplitCube].AddEntry(Entry0);
		}
		else
		{
			CMatrixPaletteEntry Entry0;
			Entry0.m_TranslateX = x;
			Entry0.m_TranslateY = y;
			Entry0.m_TranslateZ = sd;
			Entry0.m_ScaleZ = _Depth;
			Entry0.m_TextureTranslateX0 = tx;
			Entry0.m_TextureTranslateY0 = ty;
			Entry0.m_TextureTranslateX1 = tx; // Not used
			Entry0.m_TextureTranslateY1 = ty; // Not used

			if (_rCell.m_Mode == CCellInfo::MODE_HUGE)
				m_bBufferFull |= m_RenderBatches[ERenderBatch_HugeCube].AddEntry(Entry0);
			else if(_rCell.m_Mode == CCellInfo::MODE_SECONDARY)
				m_bBufferFull |= m_RenderBatches[ERenderBatch_SecondaryCube].AddEntry(Entry0);
			else
				m_bBufferFull |= m_RenderBatches[ERenderBatch_Cube].AddEntry(Entry0);
		}
	}
}
//*/


extern fp32 g_CubeGlobalTimeScale; // WFrontEndMod.cpp
void CCube::Update()
{
	// timer update
	//static CMTime fTime;


	CMTime Time = CMTime::GetCPU();
	CMTime Delta = Time-m_LastTime;
	if(Delta.Compare(0.1f) > 0)
		Delta = CMTime::CreateFromSeconds(0.1f);
	m_LastTime = Time;


	/*
	CMTime Time = CMTime::GetCPU();
	m_FrameTime = Time.GetTime() - m_LastTime.GetTime();
	m_FrameTime = Min(0.1f, Max(m_FrameTime, 0.03f));
	m_LastTime = Time;
	*/


	/*

	int64 Time = GetCPUClock();
	int64 Delta = Time-m_LastTime;
	if(Delta > GetCPUFrequencyu()/10)
	Delta = GetCPUFrequencyu()/10;
	m_LastTime = Time;*/
	m_FrameTime = Delta.GetTime()*g_CubeGlobalTimeScale;// * 0.15f;
	m_AnimTime += CMTime::CreateFromSeconds(m_FrameTime);
	//m_FrameTime = Delta/(fp32)GetCPUFrequencyu();

	UpdateSound(m_FrameTime);
	UpdateElements(m_FrameTime*2.5f);
}

//
//
//
void CCube::InitSound(spCMapData _spMapData, spCSoundContext _Context, int _Channel)
{
//	if(m_GUIFlags & GUI_FLAGS_SOUNDINITIALIZED)
//		return;

	/*
	GUI_Enter_01	När GUI kommer in
	GUI_Exit_01		När man går ut ur GUI
	GUI_Loop_01		Loopande ljud för tentaklerna. Skall spelas som loop så 
	länge tentaklerna finns framme. Du börjar spela det samtidigt som Enter ljudet, 
	och klipper det ca 100ms efter Exit ljudet börjar spela. Då slipper man en glipa och 
	det känns som at loopen smälter in i exit ljudet

	GUI_Select_01	När man går runt i menyerna. Ett rätt subtilt ljud
	GUI_Shift_01    Ett 6x slumpljud, varje gång tentaklerna shiftar när man går in och ut ur submenyer.
	*/

	m_spSoundContext = _Context;
	m_spMapData = _spMapData;
	m_iChannel = _Channel;

	memset(m_aSounds, 0, sizeof(m_aSounds));

	m_aSounds[SOUNDEVENT_TENTACLES_MOVING_OUT]	= m_spMapData->GetResourceIndex_Sound("GUI_Moving_Out");
	m_aSounds[SOUNDEVENT_TENTACLES_MOVING_IN]	= m_spMapData->GetResourceIndex_Sound("GUI_Moving_In");
	m_aSounds[SOUNDEVENT_TENTACLES_IDLE]		= m_spMapData->GetResourceIndex_Sound("GUI_Idle");
	m_aSounds[SOUNDEVENT_TENTACLES_CHANGEPAGE]	= m_spMapData->GetResourceIndex_Sound("GUI_ChangePage");
	m_aSounds[SOUNDEVENT_TENTACLES_SELECT]		= m_spMapData->GetResourceIndex_Sound("GUI_Select");
	m_aSounds[SOUNDEVENT_TENTACLES_BACK]		= m_spMapData->GetResourceIndex_Sound("GUI_Back");
	m_aSounds[SOUNDEVENT_TENTACLES_HILITE_SWAP]	= m_spMapData->GetResourceIndex_Sound("GUI_Hilite_Swap");
	//m_aSounds[SOUNDEVENT_TENTACLES_SCROLL_UP]	= m_spMapData->GetResourceIndex_Sound("GUI_Scroll_Up");
	//m_aSounds[SOUNDEVENT_TENTACLES_SCROLL_DOWN]	= m_spMapData->GetResourceIndex_Sound("GUI_Scroll_Down");
	m_aSounds[SOUNDEVENT_TENTACLES_FLIP_CTRL]	= m_spMapData->GetResourceIndex_Sound("GUI_Flip_Ctrl");
	m_aSounds[SOUNDEVENT_TENTACLES_TAB]			= m_spMapData->GetResourceIndex_Sound("GUI_Tab");

	//m_aSounds[SOUNDEVENT_LIGHT_FLICKER_1] = m_spMapData->GetResourceIndex_Sound("");
	//m_aSounds[SOUNDEVENT_LIGHT_FLICKER_2] = m_spMapData->GetResourceIndex_Sound("");
	//m_aSounds[SOUNDEVENT_LIGHT_FLICKER_BACKONLINE] = m_spMapData->GetResourceIndex_Sound("");

	//	m_aSounds[SOUNDEVENT_UP] = m_spMapData->GetResourceIndex_Sound("");
	//	m_aSounds[SOUNDEVENT_DOWN] = m_spMapData->GetResourceIndex_Sound("");

	//	m_aSounds[SOUNDEVENT_CLOSE] = m_spMapData->GetResourceIndex_Sound("gui_m_close");
	//	m_aSounds[SOUNDEVENT_OPEN] = m_spMapData->GetResourceIndex_Sound("gui_m_open");
	//	m_aSounds[SOUNDEVENT_UP] = m_spMapData->GetResourceIndex_Sound("gui_m_up");
	//	m_aSounds[SOUNDEVENT_DOWN] = m_spMapData->GetResourceIndex_Sound("gui_m_down");
	//	m_aSounds[SOUNDEVENT_ROTATEFAST] = m_spMapData->GetResourceIndex_Sound("gui_m_mofa001");
	//	m_aSounds[SOUNDEVENT_ROTATESLOW] = m_spMapData->GetResourceIndex_Sound("gui_m_mosl001");

	//	m_aSounds[SOUNDEVENT_MOVEDOWN] = m_spMapData->GetResourceIndex_Sound("");
	//	m_aSounds[SOUNDEVENT_MOVEUP] = m_spMapData->GetResourceIndex_Sound("");
	//	m_aSounds[SOUNDEVENT_SLIDELEFT] = m_spMapData->GetResourceIndex_Sound("gui_m_mofa001");
	//	m_aSounds[SOUNDEVENT_SLIDERIGHT] = m_spMapData->GetResourceIndex_Sound("gui_m_mosl001");
	//	m_aSounds[SOUNDEVENT_SELECT] = m_spMapData->GetResourceIndex_Sound("");


	//	m_spMapData->GetResourceIndex_Sound("extra_access_denied");
	//	m_spMapData->GetResourceIndex_Sound("extra_access_granted");

	for(int32 i = 0; i < NUMSOUNDEVENTS; i++)
		m_aEventTimes[i] = 0;

	m_GUIFlags |= GUI_FLAGS_SOUNDINITIALIZED;
}


void CCube::Init()
{
	{
		//-------------------------------------------------------------------------------------
		// Precahce all sounds
//		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
//		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		
		// parse out all monolog-sounds from Levelkeys.xrg
		// get a list of all script-layers from LevelKeys.xrg
		// find the right map/layers for this script-layer
		spCRegistry spRegistry;
		//spRegistry = CWorld_ServerCore::ReadServerReg("Sv");

		MACRO_GetSystem;
		CStr Params;
		CStr ContentPath = pSys->GetEnvironment()->GetValue("CONTENTPATH");
		if (ContentPath == "")
			ContentPath = pSys->m_ExePath + "Content\\";

		CStr RegisterFile = ContentPath + "Registry\\";
		RegisterFile += "Sv.xrg";

		// Read register
		TPtr<CRegistryCompiled> spRegCompiled;
		CStr FileName = RegisterFile.GetPath() + RegisterFile.GetFilenameNoExt() + ".xcr";
		if (CDiskUtil::FileExists(FileName))
		{
			spRegCompiled = MNew(CRegistryCompiled);
			if (spRegCompiled)
			{
				spRegCompiled->Read_XCR(FileName);
				spRegistry = spRegCompiled->GetRoot();
			}
		}
		else if (CDiskUtil::FileExists(RegisterFile))
		{
			spRegistry = REGISTRY_CREATE;
			if(spRegistry)
				spRegistry->XRG_Read(RegisterFile);
		}
		else
		{
			ConOutL(CStrF("§cf00ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!", RegisterFile.Str()));
			M_TRACEALWAYS("ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!\n", RegisterFile.Str());
		}

		if(!spRegistry)
		{
			ConOut("Error reading Registry.");
			return;
		}

		CRegistry *pLevelKeys = spRegistry->Find("LEVELKEYS");
		if(!pLevelKeys)
		{
			ConOut("Error retrieving LevelKeys. Registry not found");
			spRegistry = NULL;
			return;
		}

		//------------------------------------------------------------------------------------------------

		M_ASSERTHANDLER(m_AllJackieModels.IsEmpty(), "This should only be run ONCE (at startup)", m_AllJackieModels.Clear());
		M_ASSERTHANDLER(m_AllVocapSounds.IsEmpty(), "This should only be run ONCE (at startup)", m_AllVocapSounds.Clear());

		CStr Tmp;
		CStr Tmp2;
		//int iSoundPrecacheCounter = 0;
		int iNrOfChildren = pLevelKeys->GetNumChildren();
		for(int i = 0; i < iNrOfChildren; i++)
		{
			bool bNoLayer = true;
			CRegistry *pReg = pLevelKeys->GetChild(i);
			if(pReg && pReg->GetNumChildren() > 0)
			{
				int iNumScriptLayers = pReg->GetNumChildren();
				for(int j = 0; j < iNumScriptLayers; j++)
				{
					CRegistry* pScriptLayer = pReg->GetChild(j);
					CRegistry* pLoadingStream = NULL;
					CStr ScriptLayer =  pScriptLayer->GetThisName();
					if (ScriptLayer == "LAYER")
					{
						bNoLayer = false;
						CRegistry* pGame = pScriptLayer->Find("GAME");
						if (pGame)
							pLoadingStream = pGame->FindChild("LOADINGSTREAM");
					}
					else if (ScriptLayer == "GAME")
						pLoadingStream = pScriptLayer->FindChild("LOADINGSTREAM");

					if (pLoadingStream)
					{
						int NrOfCh = pLoadingStream->GetNumChildren();
						for(int k = 0; k < NrOfCh; k++)
						{
							CRegistry *pVocap = pLoadingStream->GetChild(k);
							CStr VoCap = pVocap->GetThisName();
							if (VoCap == "VOCAP" && !(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
							{
								Tmp = pVocap->GetThisValue();
								Tmp2 = Tmp.GetStrSep(",");
								if (m_AllJackieModels.Find(Tmp2.Str()) == -1)
									m_AllJackieModels += Tmp2 + ",";
								VoCap = Tmp.GetStrSep(",");
								m_AllVocapSounds += VoCap + ",";
							}
							else if ((m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG) && VoCap == "LOADINGBG")
							{
								VoCap = pVocap->GetThisValue();
								m_AllTextures += VoCap + ",";
							}
						}
					}
				}
			}
		}

		//M_TRACEALWAYS("CCube::LoadAndSetupLoadingScene: Precaching %d sounds\n", iSoundPrecacheCounter);
		// End of monolog sound precache
		//-------------------------------------------------------------------------------------

		if(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG)
			m_iModelJackie = 0;
		else
		{
			//m_iModelJackie = m_spMapData->GetResourceIndex_Model("Characters\\J_E\\J_E");
			//CXR_Model *pTmpModelJackie = m_spMapData->GetResource_Model(m_iModelJackie);

			//if(pTmpModelJackie)
			//	pTmpModelJackie->OnPrecache(_pEngine, 0);

			/*
			int iModelExta = m_spMapData->GetResourceIndex_Model("Furnitures\\int_chair_03");
			CXR_Model *pTmpModel3 = m_spMapData->GetResource_Model(iModelExta);

			if(pTmpModel3)
			pTmpModel3->OnPrecache(_pEngine, 0);
			*/

//			m_pAnimGripBasePistol = iGripAnim ? m_spMapData->GetResource_Anim(iGripAnim) : NULL;

			//iGripAnim = m_spMapData->GetResourceIndex_Anim("HumanMovement\\DualPistols\\DPis_PistolGrip");
			//m_pAnimGripBaseRifle = iGripAnim ? m_spMapData->GetResource_Anim(iGripAnim) : NULL;

		}

		spRegistry = NULL;
	}



	LoadAllResources();
}

//
//
//
void CCube::UpdateSound(fp32 _Delta)
{
	for(int32 i = 0; i < NUMSOUNDEVENTS; i++)
		if(m_aEventTimes[i] > 0)
			m_aEventTimes[i] -= _Delta;
}

//
//
//
void CCube::PlayEvent(int32 _Event, fp32 _Volume, bool _MuteAll, bool _LoopIt)
{
	if(m_spSoundContext == NULL)
		return;

	if(_Event < 0 || _Event >= NUMSOUNDEVENTS)
		return;

	if(m_aEventTimes[_Event] > 0)
		return;

	if(_MuteAll)
		for(int32 i = 0; i < NUMSOUNDEVENTS; i++)
			m_aEventTimes[i] = 0.075f;

	// set time and play sound
	m_aEventTimes[_Event] = 0.05f;
	CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(m_aSounds[_Event]);

	if(pDesc)
	{
		int16 iWave = pDesc->GetWaveId(pDesc->GetPlayWaveId());
		if(iWave >= 0)
		{
			CSC_VoiceCreateParams CreateParams;
			m_spSoundContext->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
			CreateParams.m_hChannel = m_iChannel;
			uint32 Prio = pDesc->GetPriority();
			if (Prio)
				CreateParams.m_Priority = Prio;
			CreateParams.m_WaveID = iWave;
			CreateParams.m_bLoop = _LoopIt;
			CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
			CreateParams.m_Volume *= pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp();

			m_spSoundContext->Voice_Create(CreateParams);
		}
	}
}

int CCube::PlaySoundFXEvent(uint _SoundEvent, const CVec3Dfp32* _pPos, bool _LoopIt, fp32 _Delay)
{
	return PlaySoundFX(m_aSounds[_SoundEvent], _pPos, _LoopIt, _Delay);
}

int CCube::PlaySoundFX(int32 _SoundID, const CVec3Dfp32* _pPos, bool _LoopIt, fp32 _Delay)
{
	if(((m_GUIFlags & GUI_FLAGS_EXITCOMPLETE) && !_pPos) || !m_spSoundContext) // loadingscene needs to pass this validation
		return -1;

	int iVoiceNr = -1;
	CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(_SoundID);
	if(pDesc)
	{
		int16 iWave = pDesc->GetWaveId(pDesc->GetPlayWaveId());
		if(iWave >= 0)
		{
			if(!_pPos || (m_GUIFlags & GUI_FLAGS_LOADINGSCENERUNNING))
			{
				CSC_VoiceCreateParams CreateParams;
				m_spSoundContext->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
				CreateParams.m_hChannel = m_iChannel;

				uint32 Prio = pDesc->GetPriority();
				if (Prio)
					CreateParams.m_Priority = Prio;

				CreateParams.m_WaveID = iWave;
				CreateParams.m_bLoop = _LoopIt;
				CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
				CreateParams.m_Volume *= pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp();
				CreateParams.m_MinDelay = _Delay;

				iVoiceNr = m_spSoundContext->Voice_Create(CreateParams);
			}
			else
			{

				CSC_VoiceCreateParams CreateParams;
				m_spSoundContext->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
				CreateParams.m_hChannel = m_iChannel;
				uint32 Prio = pDesc->GetPriority();
				if (Prio)
					CreateParams.m_Priority = Prio;
				CreateParams.m_WaveID = iWave;
				CreateParams.m_bLoop = _LoopIt;
				CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
				CreateParams.m_Volume *= pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp();
				CreateParams.m_MinDelay = _Delay;
				CreateParams.m_3DProperties.m_Position = *_pPos;
				CreateParams.m_3DProperties.m_MinDist += pDesc->GetAttnMinDist();
				CreateParams.m_3DProperties.m_MaxDist += pDesc->GetAttnMaxDist();

				iVoiceNr = m_spSoundContext->Voice_Create(CreateParams);
			}
		}
	}
	return iVoiceNr;
}

void CCube::StopSoundFX(int _Voice, bool _bNow)
{
	if(_bNow && m_spSoundContext)
		m_spSoundContext->Voice_Destroy(_Voice);

}

// 0, out, takeout, move to front, move in, in, 0
const int CCube::ms_StateTime[] ={0, 150, 200, 200, 200, 350, 0};
//const int CCube::ms_StateTime[] ={0, 150, 50, 400, 50, 200, 0};
//const int CCube::ms_StateTime[] ={0, 150, 100, 250, 100, 400, 0};

const int CCube::ms_aSideSwapIndex[6][4] = {{4,5,3,1}, {4,5,0,2}, {4,5,1,3}, {4,5,2,0}, {2,0,3,1}, {0,2,3,1}};



CCubeUser::CCubeUser()
{
	m_pFrontEndMod = NULL;
	m_Using = false;

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(pGame)
	{
		m_pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd); // UGLY
		m_pGameContextMod = TDynamicCast<CGameContextMod>(pGame);
	}

	Use(true);

	if(m_pFrontEndMod == NULL || m_pGameContextMod == NULL)
	{
		for(int32 i = 0; i < 20; i++)
			M_TRACEALWAYS("ARRRRRRRRRRRRRRRRGHHHHHHHHHHHHH!!!!!!!!1\n");;
	}
}


CCubeUser::~CCubeUser()
{
	Use(false);
}


void CCubeUser::Use(bool _Use)
{
	if(!m_pFrontEndMod) // we need the front end
		return;

	if(_Use == m_Using) // don't do anything if we are already doing it
		return;

	if(_Use)
		m_pFrontEndMod->m_CubeReferneces++;
	else
		m_pFrontEndMod->m_CubeReferneces--;

	m_Using = _Use;
}


// -------------------------------------------------------------------
// Jakob: call back for VBM
class CXR_PreRenderData_InitiateTexturePrecache
{
public:
	uint16 m_TextureID;

	static void InitiateTexturePrecache(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		MSCOPESHORT(InitiateTexturePrecache);
		CXR_PreRenderData_InitiateTexturePrecache* pData = (CXR_PreRenderData_InitiateTexturePrecache*)_pContext;

		_pRC->Texture_GetTC()->MakeDirty(pData->m_TextureID);
		_pRC->Texture_Precache(pData->m_TextureID);
	}
};

void CCube::AddProjImage(CRect2Duint16 _Rect, int _TexID)
{
	SProjImage Image;
	Image.Rect = _Rect;
	Image.TexID = _TexID;
	m_lProjImages.Add(Image);
}

void CCube::SetupProjectionTexture(CRenderContext* _pRC, CXR_VBManager* _pVBM, bool _bStoreImage, fp32 _DeltaTime, CXR_Engine *_pEngine, bool _bInGame)
{
	fp32 StartPrio = 10.0f;
	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC)
		return;

	if(!(m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGIN))
	{
		CRC_Util2D util;
		util.Begin(_pRC, pViewport, _pVBM);

		const CRct& ViewRect = pViewport->GetViewRect();
		fp32 Width = ViewRect.GetWidth();
		fp32 Height = ViewRect.GetHeight();

		if(Width / Height * (3.0f / 4)*_pRC->GetDC()->GetPixelAspect() > (1.0f + 1.3333333f) / 2)
			m_GUIFlags |= GUI_FLAGS_ISWIDESCREEN;
		else
			m_GUIFlags &= ~GUI_FLAGS_ISWIDESCREEN;

		//------------------------ TEXT START--------------------------------------------------------
		const CMat4Dfp32& transform = util.GetTransform();

		uint16 aIndices[6] = { 0,1,2,0,2,3 };
		int TextureID = pTC->GetTextureID("GUI_Proto_Text");

		CMat4Dfp32* pTransform = _pVBM->Alloc_M4(transform);
		if(!pTransform) 
			return;

		CXR_VertexBuffer* pVB = _pVBM->Alloc_VBAttrib();
		if(!pVB || !pVB->AllocVBChain(_pVBM, false))
			return;

		CXR_VertexBuffer* pVBAdd = _pVBM->Alloc_VBAttrib();
		if(!pVBAdd || !pVBAdd->AllocVBChain(_pVBM, false))
			return;

		CXR_VertexBuffer* pVBInvText = _pVBM->Alloc_VBAttrib();
		if(!pVBInvText || !pVBInvText->AllocVBChain(_pVBM, false))
			return;

		// Alloc data so all tiles can be small
		CVec3Dfp32* pV  = _pVBM->Alloc_V3(CUBE_RES*CUBE_RES * 8);
		CVec2Dfp32* pTV = _pVBM->Alloc_V2(CUBE_RES*CUBE_RES * 8);
		uint16* piPrim = _pVBM->Alloc_Int16(CUBE_RES*CUBE_RES * 12);

		CVec3Dfp32* pAddV  = _pVBM->Alloc_V3(CUBE_RES*CUBE_RES * 4);
		uint16* piAddPrim = _pVBM->Alloc_Int16(CUBE_RES*CUBE_RES * 6);

		CVec3Dfp32* pInvTextV  = _pVBM->Alloc_V3(CUBE_RES*CUBE_RES * 8);
		CVec2Dfp32* pInvTextTV = _pVBM->Alloc_V2(CUBE_RES*CUBE_RES * 8);
		uint16* piInvTextPrim = _pVBM->Alloc_Int16(CUBE_RES*CUBE_RES * 12);


		if(!pV || !pTV || !piPrim || !pAddV ||!piAddPrim || !pInvTextV || !pInvTextTV || !piInvTextPrim)
			return;

		pVB->m_pTransform    = pTransform;
		pVBAdd->m_pTransform = pTransform;
		pVBInvText->m_pTransform = pTransform;

		/*
		if((m_GUIFlags & GUI_FLAGS_DISABLENEWGUI) && !_bInGame)
		{
			CClipRect Clip(0, 0, int(Width), int(Height));
			util.SetTexture("GUI_X06logo");
			util.AspectRect(Clip, Clip.clip, CPnt(util.GetTextureWidth(), util.GetTextureHeight()), 1.0f);
		}
		*/
		
		int iAddV = 0;
		int iAddP = 0;
		int iV = 0;
		int iP = 0;
		int iTextInvV = 0;
		int iTextInvP = 0;

		int iActive = 0;
		const CSide& side = m_aSides[m_CurrentSide];
		for (int y=0; y<CUBE_RES; y++)
		{
			for (int x=0; x<CUBE_RES; x++)
			{
				const CElement& elem = side.m_aElements[x][y];
				const CCellInfo& cellinfo = elem.m_Cell;

				fp32 depth = 0.0f;
				if (elem.m_iActive != -1)
				{
					const CActiveElement& active = m_aActiveData[elem.m_iActive];
					depth = active.m_WantedDepth;//active.m_CurrentDepth;//active.m_WantedNumber * 0.1f;//CubeDepth2;
				}

				fp32 txscale = (1.0f / 16.0f);
				fp32 tyscale = (1.0f / 32.0f);
				bool bSmall = false;

				fp32 tx0, ty0, tx1, ty1;
				if (cellinfo.m_Mode == CCellInfo::MODE_HUGE)
				{
					tx0 = (cellinfo.Cell() % 16) * txscale + (cellinfo.Part() / 2) * (txscale/2);
					ty0 = (cellinfo.Cell() / 16) * tyscale + (cellinfo.Part() % 2) * (tyscale/2);
					tx1 = tx0 + txscale;
					ty1 = ty0 + tyscale;
				}
				else if (cellinfo.m_Mode == CCellInfo::MODE_SMALL)
				{
					txscale = (1.0f / 32.0f);
					tx0 = (cellinfo.SubCell(0) % 32) * txscale;
					ty0 = (cellinfo.SubCell(0) / 32) * tyscale;
					tx1 = (cellinfo.SubCell(1) % 32) * txscale;
					ty1 = (cellinfo.SubCell(1) / 32) * tyscale;
					bSmall = true;
				}
				else
				{
					tx0 = (cellinfo.Cell() % 16) * txscale;
					ty0 = (cellinfo.Cell() / 16) * tyscale;
					tx1 = tx0 + txscale;
					ty1 = ty0 + tyscale;
				}

				uint16 aSmallIndices[12] = { 0,1,2,0,2,3, 4,5,6,4,6,7 };

				// Change in get GetTransformedMousePosition if these are changed
				//fp32 scale = 20.0f * Min(Width / 640.0f, Height / 480.0f);

				// center of screen
				//fp32 ox = Width / 2 - scale*(CUBE_RES*1.15f) / 2 + 35;
				//fp32 oy = Height / 2 - scale*(CUBE_RES*1.10f) / 2 + 40;

				//align stuff so we get it in the center of the projectionmap

				fp32 scale = m_CubeProjectionSize / 20.0f;
				fp32 TmpValue = scale * 20.0f;
				fp32 ox = Max(((m_CubeProjectionSize - TmpValue) / 2.0f), -30.0f);
				fp32 oy = ((m_CubeProjectionSize - TmpValue) / 2.0f);
				if (m_GUIFlags & GUI_FLAGS_DISABLENEWGUI || !_bStoreImage)
				{
					ox += (Width - m_CubeProjectionSize) * 0.5f;
					oy += (Height - m_CubeProjectionSize) * 0.5f;
				}
				
				if (bSmall)
				{
					if(depth > 0.0f)
					{
						pInvTextV[iTextInvV+0] = CVec3Dfp32(ox+(x     )*scale, oy+(y  )*scale, 0.0f);
						pInvTextV[iTextInvV+1] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
						pInvTextV[iTextInvV+2] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);
						pInvTextV[iTextInvV+3] = CVec3Dfp32(ox+(x     )*scale, oy+(y+1)*scale, 0.0f);
						pInvTextV[iTextInvV+4] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
						pInvTextV[iTextInvV+5] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y  )*scale, 0.0f);
						pInvTextV[iTextInvV+6] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y+1)*scale, 0.0f);
						pInvTextV[iTextInvV+7] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);

						pInvTextTV[iTextInvV+0] = CVec2Dfp32(tx0        , ty0);
						pInvTextTV[iTextInvV+1] = CVec2Dfp32(tx0+txscale, ty0);
						pInvTextTV[iTextInvV+2] = CVec2Dfp32(tx0+txscale, ty0+tyscale);
						pInvTextTV[iTextInvV+3] = CVec2Dfp32(tx0        , ty0+tyscale);
						pInvTextTV[iTextInvV+4] = CVec2Dfp32(tx1        , ty1);
						pInvTextTV[iTextInvV+5] = CVec2Dfp32(tx1+txscale, ty1);
						pInvTextTV[iTextInvV+6] = CVec2Dfp32(tx1+txscale, ty1+tyscale);
						pInvTextTV[iTextInvV+7] = CVec2Dfp32(tx1        , ty1+tyscale);
						for (int ii = 0; ii < 12; ii++)
							piInvTextPrim[iTextInvP+ii] = iTextInvV + aSmallIndices[ii];

						iTextInvV += 8;
						iTextInvP += 12;
					}
					else
					{
						pV[iV+0] = CVec3Dfp32(ox+(x     )*scale, oy+(y  )*scale, 0.0f);
						pV[iV+1] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
						pV[iV+2] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);
						pV[iV+3] = CVec3Dfp32(ox+(x     )*scale, oy+(y+1)*scale, 0.0f);
						pV[iV+4] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
						pV[iV+5] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y  )*scale, 0.0f);
						pV[iV+6] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y+1)*scale, 0.0f);
						pV[iV+7] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);

						pTV[iV+0] = CVec2Dfp32(tx0        , ty0);
						pTV[iV+1] = CVec2Dfp32(tx0+txscale, ty0);
						pTV[iV+2] = CVec2Dfp32(tx0+txscale, ty0+tyscale);
						pTV[iV+3] = CVec2Dfp32(tx0        , ty0+tyscale);
						pTV[iV+4] = CVec2Dfp32(tx1        , ty1);
						pTV[iV+5] = CVec2Dfp32(tx1+txscale, ty1);
						pTV[iV+6] = CVec2Dfp32(tx1+txscale, ty1+tyscale);
						pTV[iV+7] = CVec2Dfp32(tx1        , ty1+tyscale);
						for (int ii = 0; ii < 12; ii++)
							piPrim[iP+ii] = iV + aSmallIndices[ii];

						iV += 8;
						iP += 12;
					}
				}
				else
				{
					if(depth > 0.0f)
					{
						pInvTextV[iTextInvV+0] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+0)*scale, 0.0f);
						pInvTextV[iTextInvV+1] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+0)*scale, 0.0f);
						pInvTextV[iTextInvV+2] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+1)*scale, 0.0f);
						pInvTextV[iTextInvV+3] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+1)*scale, 0.0f);

						pInvTextTV[iTextInvV+0] = CVec2Dfp32(tx0, ty0);
						pInvTextTV[iTextInvV+1] = CVec2Dfp32(tx1, ty0);
						pInvTextTV[iTextInvV+2] = CVec2Dfp32(tx1, ty1);
						pInvTextTV[iTextInvV+3] = CVec2Dfp32(tx0, ty1);

						for (int ii = 0; ii < 6; ii++)
							piInvTextPrim[iTextInvP+ii] = iTextInvV + aIndices[ii];

						iTextInvV += 4;
						iTextInvP += 6;
					}
					else
					{
						pV[iV+0] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+0)*scale, 0.0f);
						pV[iV+1] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+0)*scale, 0.0f);
						pV[iV+2] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+1)*scale, 0.0f);
						pV[iV+3] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+1)*scale, 0.0f);

						pTV[iV+0] = CVec2Dfp32(tx0, ty0);
						pTV[iV+1] = CVec2Dfp32(tx1, ty0);
						pTV[iV+2] = CVec2Dfp32(tx1, ty1);
						pTV[iV+3] = CVec2Dfp32(tx0, ty1);

						for (int ii = 0; ii < 6; ii++)
							piPrim[iP+ii] = iV + aIndices[ii];

						iV += 4;
						iP += 6;
					}
				}

				if (depth > 0.0f)
				{
					iActive = y;
					pAddV[iAddV+0] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+0)*scale, 0.0f);
					pAddV[iAddV+1] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+0)*scale, 0.0f);
					pAddV[iAddV+2] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+1)*scale, 0.0f);
					pAddV[iAddV+3] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+1)*scale, 0.0f);

					for (int ii = 0; ii < 6; ii++)
						piAddPrim[iAddP+ii] = iAddV + aIndices[ii];

					iAddV += 4;
					iAddP += 6;
				}
			}
		}

		if(m_LastiActive != iActive)
		{
			m_LastiActive = iActive;
			if(m_CurrentSide == m_LastSide)
			{
				//M_TRACEALWAYS("SetupProjectionTexture (m_CurrentSide == m_LastSide\n");
				//PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_HILITE_SWAP, NULL, false);
			}
			else
				m_LastSide = m_CurrentSide;
		}

		// Flashbar
		{
			CXR_VBChain* pAddChain = pVBAdd->GetVBChain();
			pAddChain->Render_IndexedTriangles(piAddPrim, iAddP / 3);
			pAddChain->m_pV	= pAddV;
			pAddChain->m_nV	= iAddV;

			pVBAdd->m_pAttrib->Attrib_TextureID(0, 0);
			pVBAdd->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pVBAdd->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			pVBAdd->m_Color	= 0xFFFFFFFF;
			pVBAdd->m_Priority = StartPrio++;
			_pVBM->AddVB(pVBAdd);
		}
		
		// black text
		{
			CXR_VBChain* pInxTextChain = pVBInvText->GetVBChain();
			pInxTextChain->Render_IndexedTriangles(piInvTextPrim, iTextInvP / 3);

			pInxTextChain->m_pV = pInvTextV;
			pInxTextChain->m_nV = iTextInvV;
			pInxTextChain->m_pTV[0] = pInvTextTV->k;
			pInxTextChain->m_nTVComp[0] = 2;

			pVBInvText->m_pAttrib->Attrib_TextureID(0, TextureID);
			pVBInvText->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pVBInvText->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			//if (m_GUIFlags & GUI_FLAGS_DISABLENEWGUI)
			//	pVBInvText->m_Color = 0xFFFFFFFF;
			//else
				pVBInvText->m_Color = 0xFF000000;
			pVBInvText->m_Priority = StartPrio++;
			_pVBM->AddVB(pVBInvText);
		}
		
		// white text
		{
			CXR_VBChain* pChain = pVB->GetVBChain();
			pChain->Render_IndexedTriangles(piPrim, iP / 3);
			pChain->m_pV = pV;
			pChain->m_nV = iV;
			pChain->m_pTV[0] = pTV->k;
			pChain->m_nTVComp[0] = 2;

			pVB->m_pAttrib->Attrib_TextureID(0, TextureID);
			pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			if(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI)
				pVB->m_Color = 0xFFAAAAAA;
			else
				pVB->m_Color = 0xFFFFFFFF;
			pVB->m_Priority = StartPrio++;
			_pVBM->AddVB(pVB);
		}
		
		CVec2Dfp32 UVMin(0.0f, 0.0f);
		CVec2Dfp32 UVMax(1.0f, 1.0f);

		CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
		if(!pMat2D)	return;

		CVec2Dfp32* pTVProjImage = CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

		for(int i = 0; i < m_lProjImages.Len(); i++)
		{

			CRC_Attributes* pA = _pVBM->Alloc_Attrib();
			if(!pA) return;
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_TextureID(0, m_lProjImages[i].TexID);

			CRect2Duint16 Rect = m_lProjImages[i].Rect;
			if (m_GUIFlags & GUI_FLAGS_DISABLENEWGUI)
			{
				Rect.m_Min.k[0] += TruncToInt((Width - m_CubeProjectionSize) * 0.5f);
				Rect.m_Min.k[1] += TruncToInt((Height - m_CubeProjectionSize) * 0.5f);
				Rect.m_Max.k[0] += TruncToInt((Width - m_CubeProjectionSize) * 0.5f);
				Rect.m_Max.k[1] += TruncToInt((Height - m_CubeProjectionSize) * 0.5f);
			}

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, Rect, 0xffffffff, StartPrio++, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTVProjImage, 0);
			_pVBM->AddVB(pVBScreenQuad);
		}
		util.End();
	}
	else 
	{
		// clear the texture...
		SetTextureBlack(_pRC, _pVBM, m_TextureID_ProjectionTexture, _pEngine);
		//_pVBM->AddCopyToTexture(0.0f, CRct(0, 0, 1, 1), CPnt(0,0), m_TextureID_ProjectionTexture, true, 0);
		for(uint8 i = 1; i < 6; i++)
			_pVBM->AddCopyToTexture(0.0f, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_ProjectionTexture, false, i);
	}

	//-------------------------- TEXT END --------------------------------------------------------------

	if(_bStoreImage && _pEngine)
	{	
		if(m_ProjectionFlickerTimer == PROJECTIONFLICKERTOTALTIME)
			m_ProjectionFlickerTimer = 0.0f;
		else if(m_ProjectionFlickerTimer != 0.0f)
		{
			if(m_ProjectionFlickerTimer > PROJECTIONFLICKERINTIME)
				m_ProjectionFlickerTimer += _DeltaTime * 0.12f;
			else
			{
				m_ProjectionFlickerTimer += _DeltaTime;
				return; 
			}

			m_ProjectionFlickerTimer = Min(m_ProjectionFlickerTimer, PROJECTIONFLICKERTOTALTIME);

			fp32 GlowTime = M_Sin((m_ProjectionFlickerTimer / PROJECTIONFLICKERTOTALTIME) * _PI);

			int _TextureIDFinal = m_TextureID_FinalImage;
			int _TextureIDTmp = m_TextureID_PostProcessTemp;

			//----------------------------------------------------------------
			// Distort image here (if needed)

			CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();
			int bRenderTextureVertFlip  = _pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

			CRC_Viewport* pVP = _pVBM->Viewport_Get();
			CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
			if (!pMat2D)
				return;

			int sw = pVP->GetViewRect().GetWidth();
			int sh = pVP->GetViewRect().GetHeight();

			CVec2Dfp32 UVMin;
			CVec2Dfp32 UVMax;
			if (_pRC->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
			{
				UVMin = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
				UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(ScreenSize.x), (fp32(sh) - 0.0f) / fp32(ScreenSize.y));
			}
			else
			{
				UVMin = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
				UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(sh) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
			}

			CRct VPRect = pVP->GetViewRect();
			CRect2Duint16 VPRect16;
			VPRect16.m_Min.k[0] = VPRect.p0.x;
			VPRect16.m_Min.k[1] = VPRect.p0.y;
			VPRect16.m_Max.k[0] = VPRect.p1.x;
			VPRect16.m_Max.k[1] = VPRect.p1.y;

			CRct VPRectScreen(CPnt(0, 0), ScreenSize);
			CRect2Duint16 VPRectScreen16;
			VPRectScreen16.m_Min.k[0] = VPRectScreen.p0.x;
			VPRectScreen16.m_Min.k[1] = VPRectScreen.p0.y;
			VPRectScreen16.m_Max.k[0] = VPRectScreen.p1.x;
			VPRectScreen16.m_Max.k[1] = VPRectScreen.p1.y;

			do
			{				
				//----------------------------------------------------------------------------------------------------------------------
				// the blur
				// Capture screen

				StartPrio += 0.01f;
				_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, VPRectScreen16.m_Max[0], VPRectScreen16.m_Max[1]), CPnt(0, 0), _TextureIDFinal, false);

				CVec2Dfp32* pTVBlur = NULL;
				{

					CRect2Duint16 VPBlur;
					CVec2Dfp32 UVBlurMin;
					CVec2Dfp32 UVBlurMax;

					VPBlur.m_Min[0] = VPRectScreen16.m_Min[0]  >> 1;
					VPBlur.m_Min[1] = VPRectScreen16.m_Min[1]  >> 1;
					VPBlur.m_Max[0] = VPRectScreen16.m_Max[0]  >> 1;
					VPBlur.m_Max[1] = VPRectScreen16.m_Max[1]  >> 1;
					VPBlur.m_Min[0] = VPBlur.m_Min[0]  >> 1;
					VPBlur.m_Min[1] = VPBlur.m_Min[1]  >> 1;
					VPBlur.m_Max[0] = VPBlur.m_Max[0]  >> 1;
					VPBlur.m_Max[1] = VPBlur.m_Max[1]  >> 1;

					UVBlurMin = UVMin * 0.25f;
					UVBlurMax = UVMax * 0.25f;

					fp32 GlowExp = 2.0f; 
					CVec4Dfp32 GlowBias(0.0f);
					CVec3Dfp32 GlowScale((10.0f * GlowTime), (10.0f * GlowTime), (7.0f * GlowTime));
					CVec3Dfp32 GlowGamma(1.0f);
					CVec2Dfp32 GlowCenter(0.5f);
					CVec2Dfp32 *pTV = NULL;
					{
						StartPrio += 0.01f;
						CXR_GaussianBlurParams GP;
						GP.m_Rect = VPRect16;
						GP.m_Bias	= GlowBias;
						GP.m_Gamma	= GlowGamma;
						GP.m_UVCenter = GlowCenter;
						GP.m_Scale	= GlowScale;
						GP.m_Exp	= GlowExp;
						GP.m_SrcUVMin = UVMin;
						GP.m_SrcUVMax = UVMax;
						GP.m_DstUVMin = UVMin;
						GP.m_DstUVMax = UVMax;
						GP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
						GP.m_DstPixelUV = _pEngine->m_Screen_PixelUV;
						GP.m_pVBM = _pVBM;
						GP.m_Priority = StartPrio;
						GP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
						GP.m_TextureID_Src = _TextureIDFinal;
						GP.m_TextureID_Dst = _TextureIDTmp;
						GP.m_Flags = 0;
						GP.m_nShrink = 2;
						GP.SetFilter();

						if (!CXR_Util::GaussianBlur(&GP, pTV))
							break;

						StartPrio += (((GP.m_nShrink * 2) + 4) *  0.01f);
					}

					//----------------------------------------------------------------------------------------------
					// store the bloomed image in _TextureIdTemp
					{
						{
							// render out background
							CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
								CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) : 
							CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

							CRC_Attributes* pA = _pVBM->Alloc_Attrib();
							if (!pTV || !pA)
								break;

							pA->SetDefault();
							pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
							pA->Attrib_TextureID(0, _TextureIDFinal);

							StartPrio += 0.01f;
							CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, StartPrio, pA);
							if (!pVB)
								break;

							pVB->Geometry_TVertexArray(pTV, 0);
							_pVBM->AddVB(pVB);
						}

						{
							// Reposition/stretch screen
							CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
								CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVBlurMin, UVBlurMax) :
							CXR_Util::VBM_CreateRectUV(_pVBM, UVBlurMin, UVBlurMax);

							CRC_Attributes* pA = _pVBM->Alloc_Attrib();
							if (!pTV || !pA)
								break;

							pA->SetDefault();
							pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
							pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
							pA->Attrib_TextureID(0, _TextureIDTmp);

							StartPrio += 0.01f;
							CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, StartPrio, pA);
							if (!pVB)
								break;

							pVB->Geometry_TVertexArray(pTV, 0);
							_pVBM->AddVB(pVB);
						}

						StartPrio += 0.01f;
						_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, VPRectScreen16.m_Max[0], VPRectScreen16.m_Max[1]), CPnt(0, 0), _TextureIDTmp, false);
					}

					//-------------------------------------------------------------------------------------------------------------------------
					// streaks			
					{	
						StartPrio += 0.01f;
						CXR_ShrinkParams SP;
						SP.m_SrcUVMin = UVMin;
						SP.m_SrcUVMax = UVMax;
						SP.m_DstUVMin = UVMin;
						SP.m_DstUVMax = UVMax;
						SP.m_nShrink = 2;
						SP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
						SP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
						SP.m_Priority = StartPrio;
						SP.m_pVBM = _pVBM;
						SP.m_TextureID_Src = _TextureIDFinal;
						SP.m_TextureID_Dst = _TextureIDFinal;
						SP.m_Rect = VPRect16;
						if (!CXR_Util::ShrinkTexture(&SP, pTV))
							break;

						StartPrio += ((SP.m_nShrink * 2) *  0.01f);

						CVec2Dfp32 UVTexelDownsample = CVec2Dfp32(1.0f / (fp32(VPBlur.m_Max[0] - VPBlur.m_Min[0])),
							1.0f / (fp32(VPBlur.m_Max[1] - VPBlur.m_Min[1])));

						CRC_Attributes* pA = _pVBM->Alloc_Attrib();
						if(!pA)
							break;

						pA->SetDefault();

						CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) _pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
						if (!pFP)
							break;
						pFP->Clear();
						//pFP->SetProgram("GUIRadialBlur", 0);
						pFP->SetProgram("GUIRadialBlur2", 0);
						//pFP->SetProgram("XREngine_RadialBlur", 0);							

						int nFPParams = 2;
						CVec4Dfp32* pFPParams = _pVBM->Alloc_V4(nFPParams);

						if(!pFPParams)
							break;

						// Center point in 2d space
						pFPParams[0][0] = ((m_CubeProjectionSize * 0.25) / sw) * 0.5f;
						//pFPParams[0][0] = 0.05f;
						pFPParams[0][1] = (bRenderTextureVertFlip) ? 1.0f - ((UVBlurMax[1] - UVBlurMin[1]) / 2) :
							(UVBlurMax[1] - UVBlurMin[1]) / 2;

						pFPParams[0][2] = 0;
						pFPParams[0][3] = 0;

						pFPParams[1][0] = UVTexelDownsample[0];
						pFPParams[1][1] = UVTexelDownsample[1];


						pFPParams[1][2] = 20.0f * GlowTime;
						pFPParams[1][3] = 30.0f * GlowTime;

						pFP->SetParameters(pFPParams, nFPParams);

						pA->m_pExtAttrib = pFP;

						pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
						pA->Attrib_TextureID(0, _TextureIDFinal);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

						StartPrio += 0.01f;
						CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPBlur, 0xffffffff, StartPrio, pA);
						if(!pVB)
							break;

						pVB->Geometry_TVertexArray((fp32*)pTV, 0, 2);
						_pVBM->AddVB(pVB);

						StartPrio += 0.01f;
						_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, VPBlur.m_Max[0], VPBlur.m_Max[1]), CPnt(0,0), _TextureIDFinal, false);
					}

					pTVBlur = pTV;
				}

				// Reposition/stretch screen
				CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
					CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
				CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

				CRC_Attributes* pA = _pVBM->Alloc_Attrib();
				if (!pTV || !pA)
					break;

				const int nFPParams = 1;
				uint8* pFPMem = (uint8*) _pVBM->Alloc(2 * (sizeof(CRC_ExtAttributes_FragmentProgram20) + sizeof(CVec4Dfp32)*nFPParams));

				if(!pFPMem)
					break;

				CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pFPMem; pFPMem += sizeof(CRC_ExtAttributes_FragmentProgram20);

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, 1, pA);
				if (!pVB)
					break;

				pFP->Clear();
				pFP->SetProgram("XREngine_FinalNoExposure", 0);
				pA->m_pExtAttrib = pFP;

				pA->Attrib_TextureID(0, _TextureIDFinal);
				pA->Attrib_TextureID(1, _TextureIDTmp);
				pVB->Geometry_TVertexArray(pTVBlur, 0);
				pVB->Geometry_TVertexArray(pTV, 1);

				StartPrio += 0.01f;
				pVB->m_Priority = StartPrio;
				_pVBM->AddVB(pVB);
			}
			while(0);
		}

		// D3D - must be startpoint (0,0) and POW2		
		StartPrio += 0.01f;
		_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_PostProcessTemp, false);
		
		int iLensTextureID = pTC->GetTextureID("special_defaultlens2d");
		StartPrio += 0.01f;
		RenderFullScreenImage(_pRC, _pEngine,_pVBM, iLensTextureID, StartPrio, 0xffffffff, m_CubeProjectionSize, m_CubeProjectionSize, false,CRC_RASTERMODE_NONE, true);

		StartPrio += 0.01f;
		RenderFullScreenImage(_pRC, _pEngine,_pVBM, m_TextureID_PostProcessTemp, StartPrio, 0xffffffff, m_CubeProjectionSize, m_CubeProjectionSize, false, CRC_RASTERMODE_LIGHTMAPBLEND,false, 0.75f);

		StartPrio += 0.01f;
		_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_ProjectionTexture, false, 0);
		for(uint8 i = 1; i < 6; i++)
			_pVBM->AddCopyToTexture(0.0f, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_ProjectionTexture, false, i);
	}	
}

void CCube::RenderFullScreenImage(CRenderContext* _pRC, CXR_Engine *_pEngine, CXR_VBManager* _pVBM, int _TextureID, fp32 _Prio, CPixel32 _PixColor, 
								  fp32 _Width, fp32 _Height, bool _FullScreen, int _RasterMode, bool _UVMin01, fp32 _Scale, const CVec2Dfp32 *_pUseTextureSize, 
								  fp32 _VOffset)
{
	CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();
	int bRenderTextureVertFlip  = _pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	CVec2Dfp32 UVMin = 0;
	CVec2Dfp32 UVMax = 1;

	CRect2Duint16 ScreenCoord;
	if(_FullScreen)
	{
		int sw = pVP->GetViewRect().GetWidth();
		int sh = pVP->GetViewRect().GetHeight();
		ScreenCoord.m_Max = CVec2Duint16(sw, sh);
		ScreenCoord.m_Min.SetScalar(0);
		_Width = fp32(sw);
		_Height = fp32(sh);
	}
	else
	{
		fp32 WOffset = (_Width - (_Width*_Scale)) * 0.5f;
		fp32 HOffset = (_Height - (_Height*_Scale)) * 0.5f;
		
		ScreenCoord.m_Min.k[0] = TruncToInt(WOffset);
		ScreenCoord.m_Min.k[1] = TruncToInt(HOffset);

		ScreenCoord.m_Max.k[0] = TruncToInt(_Width-WOffset);
		ScreenCoord.m_Max.k[1] = TruncToInt(_Height-HOffset);
	}

	if(_pUseTextureSize)
	{
		fp32 PixelU = (1.0f / _pUseTextureSize->k[0]);
		fp32 PixelV = (1.0f / _pUseTextureSize->k[1]);

		UVMin = CVec2Dfp32(0.5f - (PixelU*(_Width*0.5f)), (0.5f - _VOffset) - (PixelV*(_Height*0.5f)));
		UVMax = CVec2Dfp32(0.5f + (PixelU*(_Width*0.5f)), (0.5f - _VOffset) + (PixelV*(_Height*0.5f)));
		
	}
	else if(!_UVMin01)
		UVMax = CVec2Dfp32(_pEngine->m_Screen_PixelUV.k[0] * _Width, _pEngine->m_Screen_PixelUV.k[1] * _Height);

	CVec2Dfp32* pTV = (m_GUIFlags & GUI_FLAGS_FLIPUV) ? CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) : CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if(!pA)
		return;

	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	
	pA->Attrib_RasterMode(_RasterMode);

	if(_TextureID)
		pA->Attrib_TextureID(0, _TextureID);

	CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, ScreenCoord, _PixColor, _Prio, pA);
	pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
	_pVBM->AddVB(pVBScreenQuad);
}

/*
void CCube::SetBackgroundImage(CRenderContext* _pRC, CXR_VBManager* _pVBM, int _DestTextureID, fp32 _Prio)
{
CRC_Viewport* pViewport = _pVBM->Viewport_Get();
const CRct& ViewRect = pViewport->GetViewRect();
int Width = ViewRect.GetWidth();
int Height = ViewRect.GetHeight();

CRC_Viewport TmpViewport = *pViewport;
//TmpViewport.SetBackPlane(500.0f);

// for flip or no-flip UV cords
CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();
int sw = TmpViewport.GetViewRect().GetWidth();
int sh = TmpViewport.GetViewRect().GetHeight();

CVec2Dfp32 UVMin;
CVec2Dfp32 UVMax;
CVec2Dfp32 UVMinFullScreen;
CVec2Dfp32 UVMaxFullScreen;

if (_pRC->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
{
UVMin = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(ScreenSize.x), (fp32(sh) - 0.0f) / fp32(ScreenSize.y));
UVMinFullScreen = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
UVMaxFullScreen = CVec2Dfp32((fp32(ScreenSize.x) - 0.0f) / fp32(ScreenSize.x), (fp32(ScreenSize.y) - 0.0f) / fp32(ScreenSize.y));
}
else
{
UVMin = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(sh) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
UVMinFullScreen = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
UVMaxFullScreen = CVec2Dfp32((fp32(ScreenSize.x) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(ScreenSize.y) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
}

CRect2Duint16 ScreenCoord;
ScreenCoord.m_Max = CVec2Duint16(Width, Height);
ScreenCoord.m_Min = 0;

CVec2Dfp32* pTV = (!(m_GUIFlags & GUI_FLAGS_FLIPUV)) ? 
CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

CRC_Attributes* pA = _pVBM->Alloc_Attrib();
pA->SetDefault();
pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);

MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
pA->Attrib_TextureID(0, pTC->GetTextureID("GUI_MS8BG_WS"));

CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(&TmpViewport, _pVBM, ScreenCoord, 0xffffffff, 10, pA);
pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
pVBScreenQuad->m_Priority = _Prio;

_pVBM->AddVB(pVBScreenQuad);

CRC_Attributes* pA2 = _pVBM->Alloc_Attrib();
if(pA2)
{
pA2->SetDefault();
CXR_VertexBuffer* pVB2 = CXR_Util::VBM_RenderRect(&TmpViewport, _pVBM, ScreenCoord, 0xffffffff, 10, pA2);
if(pVB2)
{
pVB2->m_Priority = _Prio + 0.5f;
_pVBM->AddVB(pVB2);
}
}

_pVBM->AddCopyToTexture(_Prio, CRct(0, 0, Width, Height), CPnt(0,0), _DestTextureID, false);
}
*/
void CCube::ResetGuiProjectionValues()
{
	if(!(m_GUIFlags & GUI_FLAGS_VALUESINITIATED))
	{
		m_CurrentTentacleTemplate = TENTACLE_TEMPLATE_DEFAULT;
		m_LastTentacleTemplate = m_CurrentTentacleTemplate;

		m_GUIFlags &= ~GUI_FLAGS_BACKGROUNDPROCESSDONE;
		m_GUIFlags &= ~GUI_FLAGS_RUNTENTACLEANIMATION;

		m_GUIFlags &= ~GUI_FLAGS_SCREENGRAB_OK;

		m_BackgrounProcessTimer = 0.0f;

		m_GUIFlags |= GUI_FLAGS_VALUESINITIATED;
		m_TextureID_ScreencapToProcess = 0;
		m_TextureID_ProjectionTexture = 0;
		m_TextureID_FinalImage = 0;
		m_TextureID_PostProcessTemp = 0;

		if(!(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
			LoadAndSetupGUITentacles();

		m_LightPos = CVec3Dfp32(25.0f, 0.0f, -10.0f);
		m_OrigLightRotation = CVec3Dfp32(0.0f, 0.0f, 0.0f);
		m_LightRotation = m_OrigLightRotation;	
		m_LightRotationSpeed = 2.0f;

		//m_LightIntesity = CVec3Dfp32(1.5f, 1.5f, 1.05f);
		m_LightIntesity = CVec3Dfp32(1.5f, 1.5f, 1.5f);
		m_LightSpotRange      = 256.0f;//165.0f;

		m_LightSpotSizeOrig = CVec2Dfp32(0.5f, 0.5f);
		m_LightSpotSize = m_LightSpotSizeOrig;

		// the additional spot
		m_AddSpotFlickerPermissionTimer = 0.0f;
		m_AddSpotFlickerTimer			= 0.0f;

		m_AddSpotPos = CVec3Dfp32(25.0f, 0.0f, -10.0f);
		m_OrigAddSpotRotation =  CVec3Dfp32(0.0f, 0.0f, 0.0f);
		m_AddSpotRotation = m_OrigAddSpotRotation;
		m_AddSpotRotationSpeed = 2.0f;

		m_AddSpotIntesity = CVec3Dfp32(0.6375f, 0.7225f, 0.85f);
		m_OrigAddSpotIntesity = m_AddSpotIntesity;
		m_AddSpotRange = 512.0f;
		m_AddSpotSizeOrig =  CVec2Dfp32(0.5f, 0.5f);
		m_AddSpotSize = m_AddSpotSizeOrig;

		// camera
		m_GuiCameraMatrix.Unit();
		m_GUIFlags |= GUI_FLAGS_CAMERAMOVINGRIGHT;
		m_GUIFlags |= GUI_FLAGS_CAMERAMOVINGUP;
		m_GUIFlags |= GUI_FLAGS_NEEDONEFRAMEFAKEBG;
	}
}

void CCube::UpdateProjectionMatrix(fp32 _DeltaTime, int8 _Scroll)
{
	if(m_GUIFlags & GUI_FLAGS_DISABLEPROJECTIONANIM)
		return;

	m_LightRotation.k[0] = 0;

	if(_Scroll && !(m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGIN) && !(m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGOUT))
	{
		m_GUIFlags |= GUI_FLAGS_PROJECTIONMOVINGIN;

	}


	if((m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGIN) || (m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGOUT))
	{
		if(m_ProjectionFlickerTimer != 0.0f)
		{
			// [1] pos down
			// [0] pos is right
			m_ProjLightPos.k[0] = (MRTC_GetRand()->GenRand1Inclusive_fp32() - 0.5f) * 0.36f * M_Sin(m_ProjectionFlickerTimer / PROJECTIONFLICKERTOTALTIME);
			m_ProjLightPos.k[1] = (MRTC_GetRand()->GenRand1Inclusive_fp32() - 0.5f) * 0.36f * M_Sin(m_ProjectionFlickerTimer / PROJECTIONFLICKERTOTALTIME);
		}
	}
	else
		m_ProjLightPos = 0;


	if(m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGIN)
	{

		if(m_ProjectionFlickerTimer > PROJECTIONFLICKERINTIME)
		{
			m_GUIFlags &= ~GUI_FLAGS_PROJECTIONMOVINGIN;
			m_GUIFlags |= GUI_FLAGS_PROJECTIONMOVINGOUT;
		}		
	}
	else if(m_GUIFlags & GUI_FLAGS_PROJECTIONMOVINGOUT)
	{
		if(m_ProjectionFlickerTimer >= PROJECTIONFLICKERTOTALTIME)
		{
			m_GUIFlags &= ~GUI_FLAGS_PROJECTIONMOVINGOUT;
		}

	}
}

void CCube::UpdateAddlight(fp32 _DeltaTime)
{
	if(m_AddSpotFlickerTimer > 0.0f)
	{
		m_AddSpotFlickerTimer += _DeltaTime;
		if(m_AddSpotFlickerTimer > ADDSPOTFLICKERTIME)
		{
			if(m_AddSpotFlickerTimer > (ADDSPOTFLICKERRESTORETIME + ADDSPOTFLICKERTIME))
			{
				m_AddSpotFlickerTimer = 0.0f;
				m_AddSpotIntesity = m_OrigAddSpotIntesity;
				//m_GUIFlags &= ~GUI_FLAGS_SOUNDFLICKER1ACTIVE;
				//m_GUIFlags &= ~GUI_FLAGS_SOUNDFLICKER3ACTIVE;
			}
			else
			{
				//if(!(m_GUIFlags & GUI_FLAGS_SOUNDFLICKER3ACTIVE))
				//{
				//	m_GUIFlags |= GUI_FLAGS_SOUNDFLICKER3ACTIVE;
				//	PlaySoundFXEvent(SOUNDEVENT_LIGHT_FLICKER_BACKONLINE);
				//}
				// restore the addlight values
				fp32 val = m_AddSpotFlickerTimer / (ADDSPOTFLICKERRESTORETIME + ADDSPOTFLICKERTIME);

				if((m_GUIFlags & GUI_FLAGS_FLICKERHALFTHISFRAME) && val < 0.3f)
				{
					val *= 0.5;
					m_GUIFlags &= ~GUI_FLAGS_FLICKERHALFTHISFRAME;
				}
				else
					m_GUIFlags |= GUI_FLAGS_FLICKERHALFTHISFRAME;

				m_AddSpotIntesity.k[0] = val * m_OrigAddSpotIntesity.k[0];
				m_AddSpotIntesity.k[1] = val * m_OrigAddSpotIntesity.k[1];
				m_AddSpotIntesity.k[2] = val * m_OrigAddSpotIntesity.k[2];
			}

		}
		else
		{
			//if(!(m_GUIFlags & GUI_FLAGS_SOUNDFLICKER1ACTIVE))
			//{
			//	m_GUIFlags |= GUI_FLAGS_SOUNDFLICKER1ACTIVE;
			//	PlaySoundFXEvent(SOUNDEVENT_LIGHT_FLICKER_1);
			//}

			// do some flickering or something
			if(m_AddSpotIntesity.k[0] == 0.001f)
				m_AddSpotIntesity = m_OrigAddSpotIntesity;
			else
				m_AddSpotIntesity = 0.001f;
		}
	}
	else
	{
		m_AddSpotFlickerPermissionTimer += _DeltaTime;
		if(m_AddSpotFlickerPermissionTimer > ADDSPOTFLICKERPERMISSIONTIME)
		{
			// now do a random check
			if(MRTC_GetRand()->GenRand1Inclusive_fp32() > 0.5f)
			{
				m_AddSpotFlickerTimer = 0.0001f;
			}
			m_AddSpotFlickerPermissionTimer = 0.0f;
		}
	}
}


void CCube::SetTextureBlack(CRenderContext* _pRC, CXR_VBManager* _pVBM, int _iTextureID, CXR_Engine *_pEngine)
{

	RenderFullScreenImage(_pRC, _pEngine,_pVBM, 0, 0.0f, CPixel32(0, 0, 0, 255));

	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	const CRct& ViewRect = pViewport->GetViewRect();
	int Width = ViewRect.GetWidth();
	int Height = ViewRect.GetHeight();
	_pVBM->AddCopyToTexture(0.01, CRct(CPnt(0,0), CPnt(Width,Height)), CPnt(0, 0), _iTextureID, false);

	// TODO: Jakob! on Xenon, the first time you go to the menu in-game, theres a freeze, and this is probably cause by the aboove code
}

//--------------------------------------------------------------------------------
// returns true if fading still ongoing
//
bool CCube::FadeProcessImageScreen(CRenderContext* _pRC, CXR_VBManager* _pVBM, int _iTextureID, fp32 _DeltaTime, bool _bFadeIn)
{
	if(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG && m_LoadingBGImage.IsEmpty())
		return false;

	if(m_FadeTimer > FADETIME)
	{
		m_FadeTimer = 0.0f;
		return false;
	}
	else
	{
		m_FadeTimer += _DeltaTime;
		int Fadeval;

		if(_bFadeIn)
			Fadeval = TruncToInt((1.0f - (m_FadeTimer / FADETIME)) * 255.0f);
		else
			Fadeval = TruncToInt((m_FadeTimer / FADETIME) * 255.0f);

		Fadeval = Min(Max(0, Fadeval), 255);

		CRC_Util2D util;
		CRC_Viewport* pViewport = _pVBM->Viewport_Get();
		util.Begin(_pRC, pViewport, _pVBM);
		util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

		const CRct& ViewRect = pViewport->GetViewRect();
		int Width = ViewRect.GetWidth();
		int Height = ViewRect.GetHeight();

		util.Rect(pViewport->GetViewClip(), CRct(CPnt(0, 0), CPnt(Width, Height)), CPixel32(0, 0, 0, Fadeval));
		util.End();

		return true;
	}
}

void CCube::Render_P5(CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _DeltaTime, bool _bIsIngame, 
					  CXR_Engine *_pOtherEngine, bool _bGAmeIsLoading, bool _bMoveOut)
{
	if(!_bMoveOut && !(m_GUIFlags & GUI_FLAGS_SCREENGRAB_OK))
	{
		m_GUIFlags |= GUI_FLAGS_WAIT_FOR_SCREENGRAB;
	}
	else
	{
		m_GUIFlags &= ~GUI_FLAGS_WAIT_FOR_SCREENGRAB;
	}

	if(_bIsIngame && !_bMoveOut && (m_GUIFlags & GUI_FLAGS_WAIT_FOR_SCREENGRAB) && !(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
		return;

	if(!_pOtherEngine)
		return;

	if(m_GUIFlags & GUI_FLAGS_READYTOSTART_LS)
	{
		StartLoadingScene();
		m_GUIFlags &= ~GUI_FLAGS_READYTOSTART_LS;
	}

	if(m_GUIFlags & GUI_FLAGS_LOADINGSCENERUNNING)
	{
		if(!_bGAmeIsLoading)
			m_GUIFlags |= GUI_FLAGS_READYTOSKIP_LS;
		else
			m_GUIFlags &= ~GUI_FLAGS_READYTOSKIP_LS;


		if(!(m_GUIFlags & GUI_FLAGS_PAUSEDGAME))
		{
			M_TRACEALWAYS("++ Paused Game\n");
			ConExecuteMT("pause(1)");
			m_GUIFlags |= GUI_FLAGS_PAUSEDGAME;
		}	

		m_GUIFlags &= ~GUI_FLAGS_EXITCOMPLETE; // exit is NOT complete (only tentacleexit is complete)
		UpdateLoadingScene(_pVBM, _DeltaTime, _bGAmeIsLoading);
		if(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE)
			RenderDeathScene(_pOtherEngine, _pVBM, _pRC);
		else
			RenderLoadingScene(_pOtherEngine, _pVBM, _pRC);

		if(m_GUIFlags & GUI_FADE_IN_IN_PROGRESS)
		{
			_pVBM->ScopeBegin("CCube::Render_P5", false);
			if(!(FadeProcessImageScreen(_pRC, _pVBM, 0, _DeltaTime, true)))
				m_GUIFlags &= ~GUI_FADE_IN_IN_PROGRESS;
			_pVBM->ScopeEnd();
		}
		else if(m_GUIFlags & GUI_FADE_OUT_IN_PROGRESS)
		{
			_pVBM->ScopeBegin("CCube::Render_P5", false);
			if(!(FadeProcessImageScreen(_pRC, _pVBM, 0, _DeltaTime, false)))
			{
				m_GUIFlags &= ~GUI_FADE_OUT_IN_PROGRESS;
				m_GUIFlags |= GUI_FADE_IN_IN_PROGRESS;
				m_GUIFlags &= ~GUI_FLAGS_LOADINGSCENERUNNING;
				m_GUIFlags &= ~GUI_FLAGS_DEATHSEQUENCE;
				m_GUIFlags |= GUI_FLAGS_EXITCOMPLETE; 

				LoadAndSetupLoadingScene(); // reset values with same function

				SetTextureBlack(_pRC, _pVBM, m_TextureID_ScreencapToProcess, _pOtherEngine);

				if(m_GUIFlags & GUI_FLAGS_PAUSEDGAME)
				{
					M_TRACEALWAYS("-- Unpaused Game\n");
					ConExecuteMT("pause(0)");
					m_GUIFlags &= ~GUI_FLAGS_PAUSEDGAME;
				}
			}
			_pVBM->ScopeEnd();
		}

		//RenderSafeBorders(_pRC, _pVBM);
		return;
	}

	// fade in game (not paused while fading)
	if((m_GUIFlags & GUI_FADE_IN_IN_PROGRESS))// && !_bGAmeIsLoading)
	{
		if(!(FadeProcessImageScreen(_pRC, _pVBM, 0, _DeltaTime, true)))
		{
			m_GUIFlags &= ~GUI_FADE_IN_IN_PROGRESS;
			ClearResourcesAfterLoadingScene(_pOtherEngine);  // clear our internal map data (needed to release resource refcounts etc)
		}

		//RenderSafeBorders(_pRC, _pVBM);
		return;
	}
	else if(_bGAmeIsLoading)
		return;

	//------------------------------------------------------------------
	// hack to disable gui and its memory consumption
	if((m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
	{
		if(_bMoveOut)
		{
			m_GUIFlags |= GUI_FLAGS_EXITCOMPLETE;
			return;
		}

		static fp64 time = 0;
		time += _DeltaTime;

		CRC_Viewport* pViewport = _pVBM->Viewport_Get();
		const CRct& ViewRect = pViewport->GetViewRect();
		int Width = ViewRect.GetWidth();
		int Height = ViewRect.GetHeight();

		m_CubeProjectionSize = Min(Width, Height);

		if(!ValIsPow2(m_CubeProjectionSize))
			m_CubeProjectionSize = LEPow2(m_CubeProjectionSize);

		UpdateSequence(_DeltaTime * 1000.0f);		
		SetupProjectionTexture(_pRC, _pVBM, false, 0.0f, NULL, _bIsIngame);
		m_GUIFlags &= ~GUI_FLAGS_EXITCOMPLETE;
		m_lProjImages.Clear();
		return;
	}
	// end of hack
	//------------------------------------------------------------------

	m_DeltaTime = _DeltaTime;

	//if(_IsJournal)
	//	m_GUIFlags |= GUI_FLAGS_ISJOURNAL;
	//else 
	//	m_GUIFlags &= ~GUI_FLAGS_ISJOURNAL;

	if(_bGAmeIsLoading)
		_bMoveOut = true;

	//if(!_bGAmeIsLoading)
	{
		if(!_bMoveOut)
		{
			if(m_GUIFlags & GUI_FLAGS_EXITINGGUI)
			{
				if(m_GUIFlags & GUI_FLAGS_PAUSEDGAME)
				{
					M_TRACEALWAYS("-- Unpaused Game\n");
					ConExecuteMT("pause(0)");
					m_GUIFlags &= ~GUI_FLAGS_PAUSEDGAME;
				}
				ResetGuiProjectionValues();
			}

			m_GUIFlags &= ~GUI_FLAGS_EXITINGGUI;
		}

		if((m_GUIFlags & GUI_FLAGS_EXITCOMPLETE) && _bMoveOut)
		{
			//RenderSafeBorders(_pRC, _pVBM);
			ResetGuiProjectionValues();
			return;			
		}
		else if(_bMoveOut && !(m_GUIFlags & GUI_FLAGS_EXITINGGUI))
		{
			SetTentaclesMovingOut();
		}
		else if(_bMoveOut && (m_GUIFlags & GUI_FLAGS_EXITINGGUI))
		{
			if(GetExitComplete())
				m_GUIFlags |= GUI_FLAGS_EXITCOMPLETE;
			else
				m_GUIFlags &= ~GUI_FLAGS_EXITCOMPLETE;

			if(m_GUIFlags & GUI_FLAGS_EXITCOMPLETE)
			{
				if(m_GUIFlags & GUI_FLAGS_PAUSEDGAME)
				{
					M_TRACEALWAYS("-- Unpaused Game\n");
					ConExecuteMT("pause(0)");
					m_GUIFlags &= ~GUI_FLAGS_PAUSEDGAME;
				}

				ResetGuiProjectionValues();
				//return;
			}
		}
		else
			m_GUIFlags &= ~GUI_FLAGS_EXITCOMPLETE;
	}

	// Timer stuff
	UpdateSequence(_DeltaTime * 1000.0f);

	m_GUIFlags &= ~GUI_FLAGS_VALUESINITIATED;

	if(_pOtherEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP)
		m_GUIFlags |= GUI_FLAGS_FLIPUV;
	else
		m_GUIFlags &= ~GUI_FLAGS_FLIPUV;


	//--------------------------------------------------------------------------
	// Normal texture container
	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	const CRct& ViewRect = pViewport->GetViewRect();
	int Width = ViewRect.GetWidth();
	int Height = ViewRect.GetHeight();

	m_CubeProjectionSize = Min(Width, Height);

	if(!ValIsPow2(m_CubeProjectionSize))
		m_CubeProjectionSize = LEPow2(m_CubeProjectionSize);

	// do we have the screencap to process or not?		
	CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(_pOtherEngine->GetInterface(XR_ENGINE_TCSCREEN));

	if(!m_TextureID_ScreencapToProcess)
		m_TextureID_ScreencapToProcess = (pTCScreen) ? pTCScreen->GetTextureID(2) : 0;

	if(!_bIsIngame && m_TextureID_ScreencapToProcess)
		SetTextureBlack(_pRC, _pVBM, m_TextureID_ScreencapToProcess, _pOtherEngine);

	if(!m_TextureID_FinalImage)
		m_TextureID_FinalImage =  (pTCScreen) ? pTCScreen->GetTextureID(0) : 0;

	if(!m_TextureID_PostProcessTemp)
		m_TextureID_PostProcessTemp =  (pTCScreen) ? pTCScreen->GetTextureID(6) : 0;

	GetGUIProcessContainerCM()->PrepareFrame(CPnt(m_CubeProjectionSize, m_CubeProjectionSize));

	//----------------------------------------------------------------------------------
	// Cubemap texture container

	if(!m_TextureID_ProjectionTexture)
		m_TextureID_ProjectionTexture = (m_spGUICubeMaps) ? m_spGUICubeMaps->GetTextureID(0) : 0;

	//---------------------------------------------------------------------------------------
	// update process timer and set triggers
	int8 ScrollPage = 0;
	if(_bMoveOut)
	{
		m_GUIFlags &= ~GUI_FLAGS_BACKGROUNDPROCESSDONE;
		m_GUIFlags |= GUI_FLAGS_RUNTENTACLEANIMATION;
		m_BackgrounProcessTimer -= _DeltaTime * 2.0f;
		m_BackgrounProcessTimer = Max(m_BackgrounProcessTimer, 0.0f);
	}
	else
	{
		if(!(m_GUIFlags & GUI_FLAGS_BACKGROUNDPROCESSDONE) && (m_BackgrounProcessTimer > BACKGROUNDPROCESSTIME))
			m_GUIFlags |= GUI_FLAGS_BACKGROUNDPROCESSDONE;

		if(!(m_GUIFlags & GUI_FLAGS_RUNTENTACLEANIMATION))
			m_GUIFlags |= GUI_FLAGS_RUNTENTACLEANIMATION;

		if(!(m_GUIFlags & GUI_FLAGS_BACKGROUNDPROCESSDONE) || !(m_GUIFlags & GUI_FLAGS_RUNTENTACLEANIMATION))
			m_BackgrounProcessTimer += _DeltaTime;

		if(m_LastCurrentSide > m_CurrentSide)
		{
			m_ProjectionFlickerTimer = 0.001f;
			ScrollPage = 1;
		}
		else if(m_LastCurrentSide < m_CurrentSide)
		{
			m_ProjectionFlickerTimer = 0.001f;
			ScrollPage = -1;
		}
	}

	if(m_GUIFlags & GUI_FLAGS_RUNTENTACLEANIMATION)
	{
		UpdateTentacles(_DeltaTime, _pOtherEngine, _pRC, _pVBM,  ScrollPage);		
		UpdateProjectionMatrix(_DeltaTime, ScrollPage);
		UpdateAddlight(_DeltaTime);
	}

	if(!(m_GUIFlags & GUI_FLAGS_ISJOURNAL) && ReadyToDrawMenu())
	{
		SetupProjectionTexture(_pRC, _pVBM, true, _DeltaTime, _pOtherEngine, false);
	}
	else if((m_GUIFlags & GUI_FLAGS_ISJOURNAL))
	{
		// this also set in projectionsetup, but journal does not call that
		const CRct& ViewRect = pViewport->GetViewRect();
		fp32 Width = ViewRect.GetWidth();
		fp32 Height = ViewRect.GetHeight();

		if(Width / Height * (3.0f / 4)*_pRC->GetDC()->GetPixelAspect() > (1.0f + 1.3333333f) / 2)
			m_GUIFlags |= GUI_FLAGS_ISWIDESCREEN;
		else
			m_GUIFlags &= ~GUI_FLAGS_ISWIDESCREEN;

		fp32 StartPrio = 10.0f;
		int iLastFrameJournalImage = pTCScreen->GetTextureID(4);

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		int iLensTextureID = pTC->GetTextureID("special_defaultlens2d");
		StartPrio += 0.01f;
		RenderFullScreenImage(_pRC, _pOtherEngine,_pVBM, iLensTextureID, StartPrio, 0xffffffff, m_CubeProjectionSize, m_CubeProjectionSize, false,CRC_RASTERMODE_NONE, true);

		if(ReadyToDrawMenu())
		{
			StartPrio += 0.01f;
			CRC_Viewport* pVP = _pVBM->Viewport_Get();
			fp32 sw = fp32(pVP->GetViewRect().GetWidth());
			fp32 sh = fp32(pVP->GetViewRect().GetHeight());
			CVec2Dfp32 UseTextureSize(sw, sh);
			RenderFullScreenImage(_pRC, _pOtherEngine,_pVBM, iLastFrameJournalImage, StartPrio, 0xffffffff, m_CubeProjectionSize, m_CubeProjectionSize, false, 
				CRC_RASTERMODE_LIGHTMAPBLEND, true, 0.85f, &UseTextureSize, 0.0666f);
		}
		
		StartPrio += 0.01f;
		_pVBM->AddCopyToTexture(StartPrio, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_ProjectionTexture, false, 0);
		for(uint8 i = 1; i < 6; i++)
			_pVBM->AddCopyToTexture(0.0f, CRct(0, 0, m_CubeProjectionSize, m_CubeProjectionSize), CPnt(0,0), m_TextureID_ProjectionTexture, false, i);
	}
	
	// i have no idea why this is needed...
	if(m_GUIFlags & GUI_FLAGS_NEEDONEFRAMEFAKEBG) 
	{ 
		if(!(m_GUIFlags & GUI_FLAGS_PAUSEDGAME) && !(m_GUIFlags & GUI_FLAGS_EXITCOMPLETE))
		{
			PlaySoundFXEvent(SOUNDEVENT_TENTACLES_MOVING_IN);

			if(m_BackgroundSoundLoopID == -1)
				m_BackgroundSoundLoopID = PlaySoundFXEvent(SOUNDEVENT_TENTACLES_IDLE, NULL, true);

			M_TRACEALWAYS("++ Paused Game\n");
			ConExecuteMT("pause(1)");
			m_GUIFlags |= GUI_FLAGS_PAUSEDGAME;
		}

		//RenderFullScreenImage(_pRC, _pVBM, m_TextureID_ScreencapToProcess, 16.0f, CPixel32(255, 255, 255, 255));
		m_GUIFlags &= ~GUI_FLAGS_NEEDONEFRAMEFAKEBG;
	}

	// Render the frontend engine	
	if(m_GUIFlags & GUI_FLAGS_RUNTENTACLEANIMATION)
	{
        _pOtherEngine->m_EngineMode = XR_MODE_UNIFIED;
		RenderTentacles(_pOtherEngine, _pVBM, _pRC);
	}

	m_LastCurrentSide = m_CurrentSide;

	// restore some flags
	m_GUIFlags &= ~GUI_FLAGS_DISABLEPROJECTIONANIM;
	m_GUIFlags &= ~GUI_FLAGS_DONTPROJECT;
	

	// render the menu as a quad infront of the tentacles
	//if(m_iCurrentTentacleAnim != TENTACLE_ANIM_IN && m_iCurrentTentacleAnim != TENTACLE_ANIM_OUT)
	if(!(m_GUIFlags & GUI_FLAGS_ISJOURNAL))
	{
		if(m_iCurrentTentacleAnim != TENTACLE_ANIM_IN && m_iCurrentTentacleAnim != TENTACLE_ANIM_OUT)
		{
			_pVBM->ScopeBegin("CCube::rendermenuquad", false);

			int iTextureID = pTCScreen->GetTextureID(4);

			CRct VRect = _pVBM->Viewport_Get()->GetViewRect();
			_pVBM->AddCopyToTexture(0.0f, VRect, CPnt(0,0), iTextureID, false);

			// 1. Rerender text
			// 2. Copy text to m_TextureID_PostProcessTemp
			_pVBM->AddClearRenderTarget(0.0f, CDC_CLEAR_COLOR, 0x0, 0, 0);

			SetupProjectionTexture(_pRC, _pVBM, true, _DeltaTime, _pOtherEngine);

			// 3. Restore finalimage
			RenderFullScreenImage(_pRC, _pOtherEngine,_pVBM, iTextureID, 0.0f, 0xffffffff);

			// 4. Render Quad
			CVec3Dfp32 Origo(0.0f,0.0f,0.0f);
			CMat4Dfp32 UnitMat;
			UnitMat.Unit();
			UnitMat.GetRow(3) = CVec3Dfp32(5.0f, 0.0f, -0.3f);

			CVec2Dfp32 UVMax(m_CubeProjectionSize-1.0f, m_CubeProjectionSize-1.0f); //  quad uv does not match! :( !?
			Render_Quad(_pVBM, _pOtherEngine, m_TextureID_PostProcessTemp, Origo, 6.5f, 6.5f, UnitMat, m_GuiCameraMatrix, true, 0.0f, UVMax);

			_pVBM->ScopeEnd();
		}
	}
	else
	{
		_pVBM->ScopeBegin("CCube::removeme", false);
		// when in journal, the tentacles are rendered first, then the rest of the stuff, 
		// so store the current screen for later use
		int iTextureID = pTCScreen->GetTextureID(0);
		CRct VRect = _pVBM->Viewport_Get()->GetViewRect();
		_pVBM->AddCopyToTexture(0.0f, VRect, CPnt(0,0), iTextureID, false);
		_pVBM->ScopeEnd();
	}

	m_lProjImages.Clear();

	// this one is set in the journal rendercalls
	m_GUIFlags &= ~GUI_FLAGS_ISJOURNAL;
	
	//RenderSafeBorders(_pRC, _pVBM);
}

void CCube::Render_Quad(CXR_VBManager* _pVBM, CXR_Engine* _pEngine, int _TextureID, const CVec3Dfp32& _LocalPos, fp32 _Width, fp32 _Height, 
						const CMat4Dfp32& _WMat, const CMat4Dfp32& _CameraMat, bool _bNoZ, fp32 _Priority, CVec2Dfp32 _UVMax)
{
	CFXVBMAllocUtil AllocUtil;

	AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
	AllocUtil.Alloc_M4();

	if (AllocUtil.Alloc(_pVBM))
	{
		// Get allocated data
		CMat4Dfp32 M2V, VMat;
		_CameraMat.InverseOrthogonal(VMat);
		_WMat.Multiply(VMat, M2V);
		CMat4Dfp32* pM2V = AllocUtil.Get_M4(M2V);
		CXR_VertexBuffer* pVB = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
		pVB->Matrix_Set(pM2V);

		// Get pointers
		CXR_VBChain* pVBChain = pVB->GetVBChain();
		CVec3Dfp32* pV = pVBChain->m_pV;
		CVec2Dfp32* pTV = (CVec2Dfp32*)pVBChain->m_pTV[0];

		fp32 hw = _Width * 0.5f;
		fp32 hh = _Height * 0.5f;
		pV[0] = CVec3Dfp32(0, -hw, -hh);
		pV[1] = CVec3Dfp32(0, hw, -hh);
		pV[2] = CVec3Dfp32(0, hw, hh);
		pV[3] = CVec3Dfp32(0, -hw, hh);

		// Setup texture coordinates
		int bFlip = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		CXR_Util::Init();
		CXR_Util::VBM_CreateRectUV(CVec2Dfp32(0.0f), CVec2Dfp32(_pEngine->m_Screen_PixelUV.k[0] * _UVMax.k[0], _pEngine->m_Screen_PixelUV.k[1] * _UVMax.k[1]), bFlip, pTV);

		CRC_Attributes* pA = pVB->m_pAttrib;
		pA->SetDefault();
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->Attrib_Disable(CRC_FLAGS_CULL | ((_bNoZ) ? (CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE) : 0));
		pA->Attrib_TextureID(0, _TextureID);

		// Setup triangle indices
		pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
		pVB->Geometry_Color(0xffffffff);
		pVB->m_Priority = _Priority;

		// Add vertex buffer
		_pVBM->AddVB(pVB);
	}
}



void CCube::ClearResourcesAfterLoadingScene(CXR_Engine *_pEngine)
{
	// clear internal indices
	m_spGUISGI = NULL;
	m_iGUIWorld = 0;
	m_spLoadingSGI = NULL;
	m_iLoadingWorld = 0;
	m_iAnimGripBasePistol = 0;
	m_iModelWeaponMain = 0;
	m_iModelWeaponSecondary = 0;
	m_iModelExtra = 0;
	m_iLSMParticles = 0;
	m_iModelJackie = 0;
	m_iDialogue = 0;
	m_iFacialSetup = 0;
	m_lCloth.Clear();

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	for(int i = 0; i < m_lUsedDeathSurfaces.Len(); i++)
		RemoveResidentSurface(m_lUsedDeathSurfaces[i], _pEngine, pSC);
	m_lUsedDeathSurfaces.Clear();

	// empty local map data   (NOTE: this won't acually delete the resources since the world data keeps an internal refcount)
	m_spMapData->SetNumResources(1);

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(pGame)
	{
		CWFrontEnd* pFrontEnd = pGame->m_spFrontEnd; // UGLY
		pFrontEnd->LoadAllResources();
	}
	// re-init map data for the resources we want
	LoadAllResources();
}


//----------------------------------------------------------------------------------------------------
// Jackies monologue
//
void CCube::LoadAndSetupLoadingScene()
{
	if(!(m_GUIFlags & GUI_FLAGS_LOADINGSCENERUNNING))
	{
		m_iModelWeaponMain = 0;
		m_iModelWeaponSecondary = 0;
		m_iModelExtra = 0;

		m_GUIFlags &= ~GUI_FLAGS_READYTOSKIP_LS;
		m_GUIFlags &= ~GUI_FLAGS_READYTOSTART_LS;

		m_FadeTimer = 0;
		m_TotalAnimLength = 0;
/*
		// clear cloth for memory
		if(!(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
			m_lCloth.Clear();
		*/
	}
}

void CCube::PrecacheDeathScene(CXR_Engine * _pEngine, CWorld_Server *_pServer)
{
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");

	if(_pServer)
	{
		CRegistry* pReg = _pServer->Registry_GetLevelKeys("Game");
		if(pReg)
		{
			CStr DeathSurfaces = pReg->GetValue("DEATHSURFACES");

			int iDialogue = m_spMapData->GetResourceIndex_Dialogue("Dlg_Player_Base");
			CWRes_Dialogue *pDialogue = m_spMapData->GetResource_Dialogue(iDialogue);
			if (!pDialogue)
				return;

			MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
			while(!DeathSurfaces.IsEmpty())
			{
				CStr DeathSurface = DeathSurfaces.GetStrSep(",");
				PrecacheSurface(DeathSurface, _pEngine, pSC, CTC_TXTIDFLAGS_RESIDENT);
				m_lUsedDeathSurfaces.Add(DeathSurface);
			}

			for(int i = 0; i < 23; i++)
			{
				uint32 Hash = StringToHash(CStrF("DS_Darkness_%i", i));
				int WaveID = pDialogue->GetSoundIndex_Hash(Hash, m_spMapData);
				CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(WaveID);
				PrecacheSound(pDesc, pWC);
			}
		}
	}
}

//----------------------------------------------------------------------
//
//	Called from void CGameContextMod::DoLoadingStream(). This actually triggers the whole thing...
//
void CCube::SetLoadingSceneAnimation(CStr _LoadingSceneAnim, CStr MapName, CStr _LoadingBGImage)
{
	if(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE)
		return;
	if(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG)
	{
		m_GUIFlags |= GUI_FLAGS_LOADINGSCENERUNNING;
		m_LoadingBGImage = _LoadingBGImage;
		int WaveID = m_spMapData->GetResourceIndex_Sound(_LoadingSceneAnim.GetStrSep(","));
		CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(WaveID);
		int16 iWave = -1;
		if(pDesc)
		{
			iWave = pDesc->GetWaveId(pDesc->GetPlayWaveId());
			if(m_hVoice != -1)
				StopSoundFX(m_hVoice);
			m_hVoice = -1;
			CVec3Dfp32 Tmper(0.0f, 0.0f, 0.0f);
			m_hVoice = PlaySoundFX(WaveID, &Tmper, false, 3.0f); // GetTimeCodeDiff	
		}

		_LoadingSceneAnim = "";

		m_LoadingSceneAnimTime = 30.0f;
		//m_GUIFlags |= GUI_FLAGS_READYTOSTART_LS; -- this flag is set from gamecontext after precache is done...
		return;
	}

	m_LoadingSceneAnimTime = 0.0f;

	// No info give, set a default anim
	//if(_LoadingSceneAnim == "")
	//	_LoadingSceneAnim = "D_NY1_Jackie_959_1,D_NY1_Jackie,Gun_DesertEagle_01,Gun_DesertEagle_01,"; // sitting in a chair with guns. silent
	//_LoadingSceneAnim = "D_SUB_jackie_162,D_SUB_Jackie_Monologs,gun_1911_02,gun_1911_02,"; // 171, 166

	CStr TmpStr = _LoadingSceneAnim.GetStrSep(",");
	TmpStr = "Characters\\J_E\\" + TmpStr;
	m_iModelJackie = m_spMapData->GetResourceIndex_Model(TmpStr.Str());

	m_LoadingSceneAnim = _LoadingSceneAnim.GetStrSep(",");
	CStr AnimFile = _LoadingSceneAnim.GetStrSep(",");

	// no amination. play sound and dont stup anything else
	if(AnimFile == "")
	{
		int WaveID = m_spMapData->GetResourceIndex_Sound(m_LoadingSceneAnim);
		CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(WaveID);
		int16 iWave = -1;
		if(pDesc)
		{
			iWave = pDesc->GetWaveId(pDesc->GetPlayWaveId());
			if(m_hVoice != -1)
				StopSoundFX(m_hVoice);
			m_hVoice = -1;
			CVec3Dfp32 pos(0.0f, 0.0f, 50.0f);
			m_hVoice = PlaySoundFX(WaveID, &pos, false, 0); // GetTimeCodeDiff	
		}

		m_LoadingSceneAnim = "";
	}
	else
	{
		CStr DlgPath1 = AnimFile;
		CStr DlgPath2 = AnimFile;
		DlgPath2.GetStrSep("_");
		DlgPath1 = DlgPath2.GetStrSep("_");

		CStr FinalPathName = CStrF("%s\\%s\\Dlg_Jackie", DlgPath1.Str(), MapName.Str());
		m_iDialogue = m_spMapData->GetResourceIndex_Dialogue(FinalPathName);

		CFStr Name = CFStrF("FaceSetup/%s.xsa", "J_E");
		CFStr FileName = m_spMapData->ResolveFileName("ANIM/" + Name);
		m_iFacialSetup = m_spMapData->GetResourceIndex_FacialSetup(Name);

		//-------------------------------------------------------------------------------------------
		Name = CFStrF("Vocap\\Jackie\\%s", AnimFile.Str());
		m_VoCap.Init(Name, m_spMapData);

		TmpStr = _LoadingSceneAnim.GetStrSep(",");
		if(!m_iModelWeaponMain && TmpStr != "")
		{
			Name = CFStrF("Weapons\\%s", TmpStr.Str());
			m_iModelWeaponMain = m_spMapData->GetResourceIndex_Model(Name.Str());
		}

		TmpStr = _LoadingSceneAnim.GetStrSep(",");
		if(!m_iModelWeaponSecondary && TmpStr != "")
		{
			Name = CFStrF("Weapons\\%s", TmpStr.Str());
			m_iModelWeaponSecondary = m_spMapData->GetResourceIndex_Model(Name.Str());
		}

		TmpStr = _LoadingSceneAnim.GetStrSep(",");
		if(!m_iModelExtra && TmpStr != "")
		{
			Name = CFStrF("Furnitures\\%s", TmpStr.Str());
			m_iModelExtra = m_spMapData->GetResourceIndex_Model(Name.Str());
		}
	}

	//m_GUIFlags |= GUI_FLAGS_READYTOSTART_LS; -- this flag is set from gamecontext after precache is done...
	//StartLoadingScene();
}

void CCube::SetDeathScene(CStr _Past, CStr _Present, CStr _Future, CStr _Sound)
{
	m_GUIFlags |= GUI_FLAGS_DEATHSEQUENCE;

	if(_Sound.IsEmpty())
	{
		int iSound = MRTC_RAND() % 23;
		m_DeathDialogueHash = StringToHash(CStrF("DS_Darkness_%i", iSound));
	}
	else
		m_DeathDialogueHash = StringToHash(_Sound);

	m_DeathSeqPast = _Past;
	m_DeathSeqPresent = _Present;
	m_DeathSeqFuture = _Future;
	m_DeathCurrentSurface = 0;
	m_DeathAlpha = 0.0f;
	m_DeathLastSwitchTime = 0.0f;
	m_DeathFadeTimer = 0.0f;
	m_DeathFadeStatus = DS_FADE_IN;

	m_LoadingSceneAnimTime = 0.0f;
}

void CCube::StartLoadingScene()
{
	m_GUIFlags |= GUI_FLAGS_LOADINGSCENERUNNING;
	m_GUIFlags |= GUI_FADE_IN_IN_PROGRESS;

	bool bDeathSeq = (m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE) ? true : false;

	if((m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG) || (m_LoadingSceneAnim == "" && !bDeathSeq))
		return;

	if(m_hVoice != -1)
		StopSoundFX(m_hVoice);

	int iDialogue = m_iDialogue;
	if(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE)
		iDialogue = m_spMapData->GetResourceIndex_Dialogue("Dlg_Player_Base");
	CWRes_Dialogue *pDialogue = m_spMapData->GetResource_Dialogue(iDialogue);
	if(pDialogue)
	{
		CStr TheId;
		uint32 DialogueHash = 0;
		if(!bDeathSeq)
		{
			TheId = m_LoadingSceneAnim;
			TheId = TheId.DelTo(TheId.Len() - 4);
			DialogueHash = pDialogue->IntToHash(atoi(TheId));
		}
		else
			DialogueHash = m_DeathDialogueHash;
		
		fp32 SampleLength = pDialogue->GetSampleLength_Hash(DialogueHash);
		m_DialogueInstance.Reset(CMTime::CreateFromSeconds(0.0f), DialogueHash, SampleLength, 0);
	}

	CXR_Anim_SequenceData* pSeq = NULL;
	bool bAnimOK = false;
	if(!bDeathSeq)
	{
		CXR_Anim_Base* pAnim = m_spMapData->GetResource_Anim(m_VoCap.m_lAnimResources[m_iCurrentAnim]);

		if(!pAnim)
			return;

		// find the right animation sequence or default back to 0
		m_iAnimID = 0;
		
		for(int i = 0; i < pAnim->m_lspSequences.Len(); i++)
		{
			pSeq = pAnim->GetSequence(i);
#ifdef PLATFORM_CONSOLE
			uint32 iAnimHash = pSeq->m_NameHash;
#else
			uint32 iAnimHash = StringToHash(pSeq->m_Name);
#endif
			if(iAnimHash == StringToHash(m_LoadingSceneAnim))
			{
				bAnimOK = true;
				m_iAnimID = i;
				break;
			}
		}

		m_iAnimID = Max(m_iAnimID, 0);
		pSeq = pAnim->GetSequence(m_iAnimID);
	}

	CStr WaveName;
	int WaveID = 0;
	if(!bDeathSeq)
	{
		WaveName = m_LoadingSceneAnim;
		WaveID = m_spMapData->GetResourceIndex_Sound(WaveName);
	}
	else
	{
		int iDialogue = m_spMapData->GetResourceIndex_Dialogue("Dlg_Player_Base");
		CWRes_Dialogue *pDialogue = m_spMapData->GetResource_Dialogue(iDialogue);
		if(pDialogue)
			WaveID = pDialogue->GetSoundIndex_Hash(m_DeathDialogueHash, m_spMapData);
	}

	CSC_SFXDesc *pDesc = m_spMapData->GetResource_SoundDesc(WaveID);
	int16 iWave = -1;
	m_SoundDelay = 0.0f;
	if (pDesc && m_spSoundContext && (pSeq || bDeathSeq))
	{
		iWave = pDesc->GetWaveId(pDesc->GetPlayWaveId());
		if(pSeq)
			m_SoundDelay = m_VoCap.GetTimeCodeDiff(pSeq, m_spSoundContext, iWave);

		if(m_hVoice != -1)
			StopSoundFX(m_hVoice);
		m_hVoice = -1;
		//CVec3Dfp32 pos(0.0f, 0.0f, 50.0f);
		//m_hVoice = PlaySoundFX(WaveID, &pos, false, -m_SoundDelay);
		CVec3Dfp32 Tmper(0.0f, 0.0f, 0.0f);
		m_hVoice = PlaySoundFX(WaveID, &Tmper, false, -m_SoundDelay);

		if(!bDeathSeq)
		{
			if(m_hVoice != -1 && WaveID > 0)
				m_VoCap.SetVoice(m_spSoundContext, m_spMapData, iWave, 0, 0, m_hVoice, CMTime::CreateFromSeconds(0.0f), 0.0f, DIALOGUEANIMFLAGS_VOCAP_SPECIALSYNC);

			m_TotalAnimLength = pSeq->GetDuration();	
		}
		else
			m_LipSync.SetVoice(iWave, m_hVoice);
	}

	if(bAnimOK != true && !bDeathSeq)
		m_LoadingSceneAnim = "";	
}

void CCube::UpdateLoadingScene(CXR_VBManager *_pVBM, fp32 _fDeltaTime, bool _bGameIsLoading)
{
	if((m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG) || m_LoadingSceneAnim == "")
	{
		if(!_bGameIsLoading && m_LoadingBGImage.IsEmpty())// && m_LoadingSceneAnimTime > 30.0f)
		{
			m_GUIFlags |= GUI_FADE_OUT_IN_PROGRESS;
			m_LoadingSceneAnimTime = 0.0f;
		}
		return;
	}

	if (!m_spSoundContext)
		return;

	fp32 CurrentSoundTime = m_spSoundContext->Voice_GetPlayPos(m_hVoice).GetTime();
	if((CurrentSoundTime == 0.0f || (CurrentSoundTime-m_SoundDelay) > (m_TotalAnimLength-2.5f))&& !(m_GUIFlags & GUI_FADE_IN_IN_PROGRESS))
		m_GUIFlags |= GUI_FADE_OUT_IN_PROGRESS;
	else
		m_LoadingSceneAnimTime = -m_SoundDelay + m_spSoundContext->Voice_GetPlayPos(m_hVoice).GetTime();
	
	if(!(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE))
	{
		UpdateLSAnimation(_pVBM, _fDeltaTime);
		UpdateLSCameraPos(&m_GuiCameraMatrix);
	}
	else
	{
		m_DeathTime += _fDeltaTime;
		if(m_DeathFadeStatus)
		{
			m_DeathFadeTimer += _fDeltaTime;
			if(m_DeathFadeTimer >= DEATH_FADETIME)
			{
				if(m_DeathFadeStatus == DS_FADE_OUT)
				{
					m_DeathLastSwitchTime = m_LoadingSceneAnimTime;
					m_DeathCurrentSurface++;
					m_DeathTime = 0.0f;
					M_TRACEALWAYS("Switching death image to: %i time: %f\n", m_DeathCurrentSurface, m_LoadingSceneAnimTime);
				}

				m_DeathFadeTimer = 0.0f;
				m_DeathFadeStatus++;
				if(m_DeathFadeStatus > DS_FADE_IN)
					m_DeathFadeStatus = DS_FADE_NONE;
			}
		}

		int num = 0;
		m_DeathAlpha = Min(1.0f, m_LipSync.GetBlendValues(m_spSoundContext, num));
//		M_TRACEALWAYS("m_Surface: %i m_LastTime: %f m_Time :%f m_Alpha: %f Num: %i Avg: %f\n", m_DeathCurrentSurface, m_DeathLastSwitchTime, m_LoadingSceneAnimTime, m_DeathAlpha, num, m_DeathAlpha / fp32(num));
		if(m_DeathFadeStatus == DS_FADE_NONE && m_DeathCurrentSurface < 2 && m_DeathLastSwitchTime + 1.0f < m_LoadingSceneAnimTime && ((num && (m_DeathAlpha / fp32(num)) < 0.3f) || !num))
		{
			m_DeathFadeStatus = DS_FADE_OUT;
			m_DeathFadeTimer = 0.0f;
		}
	}
	m_spSoundContext->Listen3D_SetOrigin(m_GuiCameraMatrix);

	//---------------------------------------------------------------------------------
	// dialogue

	fp32 Time = 0.0f;
	if(!(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE))
		Time = m_CharAnimState.m_AnimTime0.GetTime();
	else
		Time = m_LoadingSceneAnimTime;
	if (m_DialogueInstance.IsValid() && (Time > -m_SoundDelay))
	{
		int iDialogue = m_iDialogue;
		if(m_GUIFlags & GUI_FLAGS_DEATHSEQUENCE)
			iDialogue = m_spMapData->GetResourceIndex_Dialogue("Dlg_Player_Base");
		CWRes_Dialogue *pDialogue = m_spMapData->GetResource_Dialogue(iDialogue);
		if(pDialogue)
		{
			pDialogue->Refresh(CMTime::CreateFromSeconds(Max(Time + m_SoundDelay, 0.0f)), _fDeltaTime, m_DialogueInstance);
		}
	}

	//------------------------------------------------------------------------------------------------------

}

void CCube::RenderLoadingScene(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC)
{
	if(!(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
	{
		_pEngine->m_EngineMode = XR_MODE_UNIFIED;

		//---------------------------------------------------------------------------
		// render the engine
		CRC_Viewport *pViewPort = _pVBM->Viewport_Get();
		CRC_Viewport TmpViewport = *pViewPort;

		fp32 StoredFov = TmpViewport.GetFOV();
		fp32 StoredFrontPlane = TmpViewport.GetFrontPlane(); 

		TmpViewport.SetFOV(m_LoadingSceneFOV);
		//if(m_GUIFlags & GUI_FLAGS_ISWIDESCREEN)
			TmpViewport.SetFOVAspect(1.78f);
		//else
		//	TmpViewport.SetFOVAspect(1.33f);
		TmpViewport.SetFrontPlane(2.1f); 

		_pVBM->Viewport_Push(&TmpViewport);

		CMat4Dfp32 CameraVelocity; CameraVelocity.Unit();
		_pEngine->Engine_Render(this, _pVBM, _pRC, m_GuiCameraMatrix, CameraVelocity, m_spLoadingSGI);

		CXR_Engine_PostProcessParams PPP;
		PPP = _pEngine->m_PostProcessParams_Current;
		PPP.m_bDynamicExposure = false;
		if(m_GUIFlags & GUI_FLAGS_ISWIDESCREEN)
			PPP.m_ViewFlags |= XR_VIEWFLAGS_WIDESCREEN;
		else
			PPP.m_ViewFlags &= ~XR_VIEWFLAGS_WIDESCREEN;

		PPP.m_pGlowFilter = "XREngine_GaussClamped";
		_pEngine->Engine_PostProcess(_pVBM, TmpViewport, &PPP);

		_pVBM->Viewport_Pop();

		//---------------------------------------------------------------------------
	}

	// X06 hack... RenderLoadingScene

	if((m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG) && !m_LoadingBGImage.IsEmpty())
	{
		_pVBM->ScopeBegin("CCube::RenderLoadingScene0", false);
		CRC_Util2D Util2D;
		CRC_Viewport* pVP = _pVBM->Viewport_Get();
		Util2D.Begin(_pRC, pVP, _pVBM);
		CRct Rect = pVP->GetViewRect();
		Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
		CClipRect Clip(0, 0, 640, 480);

		Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

		CStr BGTextureName;
		if(m_GUIFlags & GUI_FLAGS_READYTOSKIP_LS)
			BGTextureName = "GUI_MPController";
		else
			BGTextureName = m_LoadingBGImage;

		Util2D.SetTexture(BGTextureName.Str());
		Util2D.AspectRect(Clip, Clip.clip, CPnt(Util2D.GetTextureWidth(), Util2D.GetTextureHeight()), 1.0f);

		Util2D.End();
		_pVBM->ScopeEnd();
	}

	RenderSubtitles(_pVBM, _pRC);

	// this one is a bit messy, but is supposed to be result int he same positions as the outside-buttons in frontendmod
	if(m_GUIFlags & CCube::GUI_FLAGS_READYTOSKIP_LS)
	{
		CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
		CStr Text = "§Z16§LMENU_CUTSCENEQUIT";

		if(pFont)
		{
			_pVBM->ScopeBegin("CCube::RenderLoadingScene", false);
			CRC_Util2D Util2D;
			CRC_Viewport* pVP = _pVBM->Viewport_Get();
			Util2D.Begin(_pRC, pVP, _pVBM);
			CRct VRect = pVP->GetViewRect();
			Util2D.SetCoordinateScale(CVec2Dfp32(VRect.GetWidth() / 640.0f , VRect.GetHeight() / 480.0f));
			CClipRect Clip(0, 0, 640, 480);

			Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			
			int PictureWidthBase =  16;
			fp32 Aspect = Util2D.GetRC()->GetDC()->GetScreenAspect() * Util2D.GetRC()->GetDC()->GetPixelAspect();
			fp32 ButtonScale = 0.6f;
			fp32 ButtonYAdjust = 2.0f * ButtonScale;
			CRct Rect;
			Rect.p0.x = 640 - TruncToInt((64 + 64*ButtonScale) * ButtonScale);
#ifdef PLATFORM_CONSOLE
			Rect.p0.x -= TruncToInt(640 * 0.15f / 4);
#endif
			Rect.p1.x = Rect.p0.x+ TruncToInt(64*ButtonScale);
			Rect.p0.y = TruncToInt(32 * Aspect);
#ifdef PLATFORM_CONSOLE
			Rect.p0.y += int(480 * 0.15f / 4);
#endif
			Rect.p1.y = TruncToInt(Rect.p0.y + 64*ButtonScale * Aspect);
			
			Util2D.DrawTexture(Clip, Rect.p0, "GUI_Button_A", 0xFFFFFFFF, CVec2Dfp32(1.0f/ButtonScale,1.0f/(ButtonScale*Aspect)));

			int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW;
			wchar wText[1024];
			Localize_Str(Text, wText, 1023);
			fp32 w = Min(pFont->GetWidth(16, wText), 120.0f);
			fp32 FontSize = 16;
			CPnt Pos = Rect.p0;
			Pos.y += TruncToInt(ButtonYAdjust*Aspect);
			Pos.x = TruncToInt(Rect.p0.x -6.0f);
			Pos.x -= TruncToInt(w);
			Util2D.SetFontScale(1.0f, Aspect);

			int TextColorM = CPixel32(255,255,255, 255);
			int TextColorH = CPixel32(255,255,255, 255);;
			int TextColorD = 0xff000000;
			CMWnd_Text_DrawFormated(&Util2D, Clip, pFont,Text, Pos.x, Pos.y, Style, TextColorM, TextColorH, TextColorD, 280, 50);

			Util2D.End();
			_pVBM->ScopeEnd();
		}
	}
}

void CCube::RenderSubtitles(CXR_VBManager *_pVBM, CRenderContext *_pRC)
{
	if(m_DialogueInstance.m_Subtitle != "")
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys->GetOptions()->GetValuei("GAME_SUBTITLE_CUTSCENE", 0))
		{
			CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("TEXT"));
			if(pFont)
			{
				_pVBM->ScopeBegin("CCube::RenderLoadingScene1", false);
				CRC_Util2D Util2D;
				CRC_Viewport* pVP = _pVBM->Viewport_Get();
				Util2D.Begin(_pRC, pVP, _pVBM);
				CRct Rect = pVP->GetViewRect();
				Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
				CClipRect Clip(0, 0, 640, 480);

				Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

				m_DialogueInstance.Render(&Util2D, pFont, true, 0);

				Util2D.End();
				_pVBM->ScopeEnd();
			}	
		}
	}
}

void CCube::RenderDeathScene(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC)
{
	if(!(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
	{
		_pVBM->ScopeBegin("CCube::RenderDeathScene0", false);
		CRC_Util2D Util2D;
		CRC_Viewport* pVP = _pVBM->Viewport_Get();
		Util2D.Begin(_pRC, pVP, _pVBM);
		CRct Rect = pVP->GetViewRect();
		Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
		CClipRect Clip(0, 0, 640, 480);

		Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

		fp32 p = 1.0f;
		if(m_DeathFadeStatus == DS_FADE_OUT)
			p = 1.0f - (m_DeathFadeTimer / DEATH_FADETIME);
		else if(m_DeathFadeStatus == DS_FADE_IN)
			p = m_DeathFadeTimer / DEATH_FADETIME;

		int Alpha = TruncToInt(p * ((m_DeathAlpha + M_Sin(m_LoadingSceneAnimTime * 10.0f) / 10.0f) * 100.0f) * 2.55f);
		Alpha = Max(Min(Alpha, 255), 0);

		if(m_DeathCurrentSurface == 0)
			Util2D.SetSurface(m_DeathSeqPast.Str(), CMTime::CreateFromSeconds(m_DeathTime));
		else if(m_DeathCurrentSurface == 1)
			Util2D.SetSurface(m_DeathSeqPresent.Str(), CMTime::CreateFromSeconds(m_DeathTime));
		else
			Util2D.SetSurface(m_DeathSeqFuture.Str(), CMTime::CreateFromSeconds(m_DeathTime));
		if(!m_DeathSeqFuture.IsEmpty())
			Util2D.AspectRect(Clip, Clip.clip, CPnt(Util2D.GetTextureWidth(), Util2D.GetTextureHeight()), 1.0f, CPixel32(255,255,255,Alpha));
		Util2D.End();

		// Fetch texture id's to work with (Outside of main rendering scope)
		CTextureContainer_Screen* pTCScreen = (CTextureContainer_Screen*)_pEngine->GetInterface(XR_ENGINE_TCSCREEN);
		if (!pTCScreen)
			return;

		int TextureID_Screen = pTCScreen->GetTextureID(0); // _pEngine->m_TextureID_Screen;
		int TextureID_LastScreen = pTCScreen->GetTextureID(6); // _pEngine->m_TextureID_PostProcess;

		// These 4 texture id's share the same texture
		uint TextureID_4xTexture = pTCScreen->GetTextureID(4); // _pEngine->m_TextureID_DeferredDiffuse; 
		uint TextureID_ScreenHalf = TextureID_4xTexture;		
		uint TextureID_RadialBlur = TextureID_4xTexture;		

		CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
		if (!pMat2D)
			return;

		CRct VPRect = pVP->GetViewRect();;
		CRect2Duint16 VPRect16;
		VPRect16 << VPRect;
		int sw = pVP->GetViewRect().GetWidth();
		int sh = pVP->GetViewRect().GetHeight();
		fp32 swf = fp32(sw);
		fp32 shf = fp32(sh);

		CVec2Dfp32 UVMin(0.0f, 0.0f);
		CVec2Dfp32 UVMax(swf * _pEngine->m_Screen_PixelUV[0], shf * _pEngine->m_Screen_PixelUV[1]);
		CVec2Dfp32 UVMinHalf = UVMin*0.5f;
		CVec2Dfp32 UVMaxHalf = UVMax*0.5f;

		int bRenderTextureVertFlip  = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

		CVec2Dfp32* pTVWhole = (bRenderTextureVertFlip) ? CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) : CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);
		CVec2Dfp32* pTVHalf = (bRenderTextureVertFlip) ? CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMinHalf, UVMaxHalf) : CXR_Util::VBM_CreateRectUV(_pVBM, UVMinHalf, UVMaxHalf);

		fp32 DVPriority = CXR_VBPRIORITY_UNIFIED_MATERIAL + 5000;
		do
		{
			_pVBM->AddCopyToTexture(DVPriority + 0.000f, VPRect, VPRect.p0, TextureID_Screen, false);

			CVec2Dfp32* pTVBlur = NULL;

			CRect2Duint16 VPRectTile11 = VPRect16;
			VPRectTile11.m_Min[0] = VPRect16.m_Min[0] >> 1;
			VPRectTile11.m_Min[1] = VPRect16.m_Min[1] >> 1;
			VPRectTile11.m_Max[0] = VPRect16.m_Max[0] >> 1;
			VPRectTile11.m_Max[1] = VPRect16.m_Max[1] >> 1;

			if (!CXR_Model_CameraEffects::RenderDownsample(1, bRenderTextureVertFlip , pVP, _pVBM, VPRectTile11, DVPriority + 0.100f, TextureID_Screen, TextureID_ScreenHalf, UVMin, UVMax, 0xffffffff))
				break;

			CVec2Dfp32* pTVRadialBlur = NULL;
			CRect2Duint16 VPRectTile21;
			VPRectTile21 = VPRectTile11;
			VPRectTile21.m_Min[1] = VPRectTile11.m_Max[1];
			VPRectTile21.m_Max[1] = VPRect16.m_Max[1];

			CXR_RadialBlurParams RB;
			RB.m_Priority = DVPriority + 1.2f;
			RB.m_ColorScale = CVec3Dfp32(1.0f, 1.0f, 1.0f);
			fp32 Power = m_DeathAlpha + M_Sin(m_LoadingSceneAnimTime * 10.0f) / 10.0f;
			RB.m_Power = Max(Power, 0.0f);
			RB.m_PowerScale = 6.0f;
			RB.m_PixelScale = 4.0f;
			RB.m_Affection = 0.3f;
			RB.m_ColorIntensity = 0.0f;
			RB.m_RadialCenter = 0.5f;
			RB.m_UVCenter = CVec2Dfp32(0.5f, 0.5f);
			RB.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
			RB.m_TextureID_Src = TextureID_ScreenHalf;
			RB.m_TextureID_Dst = TextureID_RadialBlur;
			RB.m_SrcUVMin = CVec2Dfp32(UVMin[0], UVMin[1]);
			RB.m_SrcUVMax = CVec2Dfp32(UVMaxHalf[0], UVMaxHalf[1]);
			RB.m_Rect = VPRectTile21;
			RB.m_pVBM = _pVBM;
			RB.m_Screen_PixelUV = _pEngine->m_Screen_PixelUV;
			RB.SetFilterParams(NULL, 0);

			RB.SetFilter("XREngine_RadialBlurInvert");
			RB.SetBlurSpace();

			if (!CXR_Util::RadialBlur(&RB, pTVRadialBlur))
				break;

			CRC_Attributes* pA = _pVBM->Alloc_Attrib();
			if (!pA)
				break;

			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 2.002f, pA);
			if (!pVB)
				break;

			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_TextureID(0, TextureID_Screen);
			pA->Attrib_TextureID(1, TextureID_RadialBlur);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexEnvMode(1, CRC_TEXENVMODE_MULTIPLY);

			pVB->Geometry_TVertexArray(pTVWhole, 0);
			pVB->Geometry_TVertexArray(pTVRadialBlur, 1);

			_pVBM->AddVB(pVB);
		} while(0);

		_pVBM->ScopeEnd();
	}

	RenderSubtitles(_pVBM, _pRC);
}

void CCube::Render_P6(CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _DeltaTime, CTextureContainer_Screen* _pGUIProcess, bool _bIsIngame,  CXR_Engine *_pOtherEgine, bool _bGAmeIsLoading, bool _bMoveOut, CStr _BG, bool _IsJournal)
{
	static fp64 time = 0;
	time += _DeltaTime;

	_DeltaTime *= 1000.0f;
	UpdateSequence(_DeltaTime);
	_DeltaTime *= CUBE_SPEED/100.0f;
	_DeltaTime *= m_TimeScale;
	if (m_MoveTime > -1)
		m_MoveTime += _DeltaTime*0.1f*1.25f;


	CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("P6_text"));
	if (!pFont)
		return;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC)
		return;

	CRC_Util2D util;
	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	util.Begin(_pRC, pViewport, _pVBM);
	util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

	const CRct& ViewRect = pViewport->GetViewRect();
	fp32 Width = ViewRect.GetWidth();
	fp32 Height = ViewRect.GetHeight();

	// Prepare container using specified with/height
	if(_pGUIProcess)
		_pGUIProcess->PrepareFrame(CPnt(TruncToInt(Width), TruncToInt(Height)));

	int TextureID_GUIProcess = 0;//(_pGUIProcess) ? _pGUIProcess->GetTextureID(0) : 0;

	CClipRect Clip(0, 0, int(Width), int(Height));
	//	CVec3Dfp32 aVerts[4];
	//	CVec2Dfp32 aTVerts[4];
	uint16 aIndices[6] = { 0,1,2,0,2,3 };

	const CMat4Dfp32& transform = util.GetTransform();

	int TextureID = pTC->GetTextureID("GUI_Proto_Text");

	uint8 nFlashIntensity = uint8( 0xA0 * (0.5f+0.3f*M_Sin(time*5.0f)) );
	CPixel32 FlashColor(nFlashIntensity, nFlashIntensity, nFlashIntensity);

	CMat4Dfp32* pTransform = _pVBM->Alloc_M4(transform);

	if(!pTransform) 
		return;

	CXR_VertexBuffer* pVB = _pVBM->Alloc_VBAttrib();
	if(!pVB || !pVB->AllocVBChain(_pVBM, false))
		return;

	CXR_VertexBuffer* pVBAdd = _pVBM->Alloc_VBAttrib();
	if(!pVBAdd || !pVBAdd->AllocVBChain(_pVBM, false))
		return;

	// Alloc data so all tiles can be small
	CVec3Dfp32* pV  = _pVBM->Alloc_V3(CUBE_RES*CUBE_RES * 8);
	CVec2Dfp32* pTV = _pVBM->Alloc_V2(CUBE_RES*CUBE_RES * 8);
	uint16* piPrim = _pVBM->Alloc_Int16(CUBE_RES*CUBE_RES * 12);

	CVec3Dfp32* pAddV  = _pVBM->Alloc_V3(CUBE_RES*CUBE_RES * 4);
	uint16* piAddPrim = _pVBM->Alloc_Int16(CUBE_RES*CUBE_RES * 6);

	if(!pV || !pTV || !piPrim || !pAddV ||!piAddPrim)
		return;

	pVB->m_pTransform    = pTransform;
	pVBAdd->m_pTransform = pTransform;

	if(Width / Height * (3.0f / 4)*_pRC->GetDC()->GetPixelAspect() > (1.0f + 1.3333333f) / 2)
	{
		if(!util.SetTexture(CStrF("%s_w", _BG.Str()).Str()))
			util.SetTexture(_BG.Str());
	}
	else
		util.SetTexture(_BG.Str());
	util.AspectRect(Clip, Clip.clip, CPnt(util.GetTextureWidth(), util.GetTextureHeight()), 1.0f);

	int iAddV = 0;
	int iAddP = 0;
	int iV = 0;
	int iP = 0;

	const CSide& side = m_aSides[m_CurrentSide];
	for (int y=0; y<CUBE_RES; y++)
	{
		for (int x=0; x<CUBE_RES; x++)
		{
			const CElement& elem = side.m_aElements[x][y];
			const CCellInfo& cellinfo = elem.m_Cell;

			fp32 depth = 0.0f;
			if (elem.m_iActive != -1)
			{
				const CActiveElement& active = m_aActiveData[elem.m_iActive];
				depth = active.m_WantedDepth;//active.m_CurrentDepth;//active.m_WantedNumber * 0.1f;//CubeDepth2;
			}

			fp32 txscale = (1.0f / 16.0f);
			fp32 tyscale = (1.0f / 32.0f);
			bool bSmall = false;

			fp32 tx0, ty0, tx1, ty1;
			if (cellinfo.m_Mode == CCellInfo::MODE_HUGE)
			{
				tx0 = (cellinfo.Cell() % 16) * txscale + (cellinfo.Part() / 2) * (txscale/2);
				ty0 = (cellinfo.Cell() / 16) * tyscale + (cellinfo.Part() % 2) * (tyscale/2);
				tx1 = tx0 + txscale;
				ty1 = ty0 + tyscale;
			}
			else if (cellinfo.m_Mode == CCellInfo::MODE_SMALL)
			{
				txscale = (1.0f / 32.0f);
				tx0 = (cellinfo.SubCell(0) % 32) * txscale;
				ty0 = (cellinfo.SubCell(0) / 32) * tyscale;
				tx1 = (cellinfo.SubCell(1) % 32) * txscale;
				ty1 = (cellinfo.SubCell(1) / 32) * tyscale;
				bSmall = true;
			}
			else
			{
				tx0 = (cellinfo.Cell() % 16) * txscale;
				ty0 = (cellinfo.Cell() / 16) * tyscale;
				tx1 = tx0 + txscale;
				ty1 = ty0 + tyscale;
			}

			//			CVec3Dfp32 aSmallVerts[8];
			//			CVec2Dfp32 aSmallTVerts[8];
			uint16 aSmallIndices[12] = { 0,1,2,0,2,3, 4,5,6,4,6,7 };

			// Change in get GetTransformedMousePosition if these are changed
			fp32 scale = 20.0f * Min(Width / 640.0f, Height / 480.0f);
			fp32 ox = Width / 2 - scale*(CUBE_RES*1.15f) / 2 + 35;
			fp32 oy = Height / 2 - scale*(CUBE_RES*1.10f) / 2 + 40;

			if (bSmall)
			{
				pV[iV+0] = CVec3Dfp32(ox+(x     )*scale, oy+(y  )*scale, 0.0f);
				pV[iV+1] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
				pV[iV+2] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);
				pV[iV+3] = CVec3Dfp32(ox+(x     )*scale, oy+(y+1)*scale, 0.0f);
				pV[iV+4] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y  )*scale, 0.0f);
				pV[iV+5] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y  )*scale, 0.0f);
				pV[iV+6] = CVec3Dfp32(ox+(x+1.0f)*scale, oy+(y+1)*scale, 0.0f);
				pV[iV+7] = CVec3Dfp32(ox+(x+0.5f)*scale, oy+(y+1)*scale, 0.0f);

				pTV[iV+0] = CVec2Dfp32(tx0        , ty0);
				pTV[iV+1] = CVec2Dfp32(tx0+txscale, ty0);
				pTV[iV+2] = CVec2Dfp32(tx0+txscale, ty0+tyscale);
				pTV[iV+3] = CVec2Dfp32(tx0        , ty0+tyscale);
				pTV[iV+4] = CVec2Dfp32(tx1        , ty1);
				pTV[iV+5] = CVec2Dfp32(tx1+txscale, ty1);
				pTV[iV+6] = CVec2Dfp32(tx1+txscale, ty1+tyscale);
				pTV[iV+7] = CVec2Dfp32(tx1        , ty1+tyscale);
				for (int ii = 0; ii < 12; ii++)
					piPrim[iP+ii] = iV + aSmallIndices[ii];

				iV += 8;
				iP += 12;
			}
			else
			{
				pV[iV+0] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+0)*scale, 0.0f);
				pV[iV+1] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+0)*scale, 0.0f);
				pV[iV+2] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+1)*scale, 0.0f);
				pV[iV+3] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+1)*scale, 0.0f);

				pTV[iV+0] = CVec2Dfp32(tx0, ty0);
				pTV[iV+1] = CVec2Dfp32(tx1, ty0);
				pTV[iV+2] = CVec2Dfp32(tx1, ty1);
				pTV[iV+3] = CVec2Dfp32(tx0, ty1);

				for (int ii = 0; ii < 6; ii++)
					piPrim[iP+ii] = iV + aIndices[ii];

				iV += 4;
				iP += 6;
			}

			if (depth > 0.0f)
			{
				pAddV[iAddV+0] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+0)*scale, 0.0f);
				pAddV[iAddV+1] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+0)*scale, 0.0f);
				pAddV[iAddV+2] = CVec3Dfp32(ox+(x+1)*scale, oy+(y+1)*scale, 0.0f);
				pAddV[iAddV+3] = CVec3Dfp32(ox+(x+0)*scale, oy+(y+1)*scale, 0.0f);

				for (int ii = 0; ii < 6; ii++)
					piAddPrim[iAddP+ii] = iAddV + aIndices[ii];

				iAddV += 4;
				iAddP += 6;
			}
		}
	}

	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->Render_IndexedTriangles(piPrim, iP / 3);
	pChain->m_pV = pV;
	pChain->m_nV = iV;
	pChain->m_pTV[0] = pTV->k;
	pChain->m_nTVComp[0] = 2;

	pVB->m_pAttrib->Attrib_TextureID(0, TextureID);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	pVB->m_Color = 0xFFFFFFFF;

	CXR_VBChain* pAddChain = pVBAdd->GetVBChain();
	pAddChain->Render_IndexedTriangles(piAddPrim, iAddP / 3);
	pAddChain->m_pV	= pAddV;
	pAddChain->m_nV	= iAddV;

	pVBAdd->m_pAttrib->Attrib_TextureID(0, 0);
	pVBAdd->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
	pVBAdd->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	pVBAdd->m_Color	= FlashColor;

	_pVBM->AddVB(pVB);
	_pVBM->AddVB(pVBAdd);

	util.End();
}


void CCube::SetGUIAndLoadingXRWorlds(CXR_Engine* _pEngine)
{
	MSCOPESHORT(CCube::SetGUIAndLoadingXRWorlds);

	if (m_iGUIWorld == 0 && !(m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
	{
		m_iGUIWorld = m_spMapData->GetResourceIndex_Model("cube.xw");

		CXR_Model* pGUIWorld = m_spMapData->GetResource_Model(m_iGUIWorld);
		if (pGUIWorld)
		{
			CXR_SceneGraphInterface* pSGInterface = pGUIWorld->SceneGraph_GetInterface();
			if (pSGInterface)
			{
				m_spGUISGI = pSGInterface->SceneGraph_CreateInstance();
				m_spGUISGI->Create(4096, 16);//, CXR_SCENEGRAPH_NOPORTALPVSCULL);		
			}
		}
	}


	if (m_iLoadingWorld == 0 && !(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
	{
		m_iLoadingWorld = m_spMapData->GetResourceIndex_Model("load.xw");

		CXR_Model* pLoadingWorld = m_spMapData->GetResource_Model(m_iLoadingWorld);
		if (pLoadingWorld)
		{
			CXR_SceneGraphInterface* pSGInterface = pLoadingWorld->SceneGraph_GetInterface();
			if (pSGInterface)
			{
				m_spLoadingSGI = pSGInterface->SceneGraph_CreateInstance();
				m_spLoadingSGI->Create(4096, 16);//, CXR_SCENEGRAPH_NOPORTALPVSCULL);		
			}
		}
	}
}


void CCube::EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType)
{
	if (m_GUIFlags & GUI_FLAGS_LOADINGSCENERUNNING)
	{
		if (!(m_GUIFlags & GUI_FLAGS_DISABLELOADINGMONOLOG))
			LoadingScene_EnumerateView(_pEngine, _iVC, _EnumViewType);
		return;
	}

	if (!m_iGUIWorld || !m_spGUISGI || (m_GUIFlags & GUI_FLAGS_DISABLENEWGUI))
		return;

	CXR_Model* pGUIWorld = m_spMapData->GetResource_Model(m_iGUIWorld);

	if(_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
	{
		m_spGUISGI->SceneGraph_Light_ClearDynamics();
		CVec3Dfp32 LightIntesity;

		// light 1 (above spot)
		if(1)
		{
			// for rotate in/out projectionmaps
			CMat4Dfp32 LightPos;
			LightPos.Unit();
			LightPos.RotX_x_M(-0.25f);

			LightPos.GetRow(3).k[2] = 2.0f; // first additional light a bit up and rotated downwards		
			LightPos.RotZ_x_M(0.09f);

			// Projection light
			//LightIntesity = m_LightIntesity;
			LightIntesity = m_LightIntesity * 0.35f;
			CXR_Light Light(LightPos, LightIntesity, m_LightSpotRange, 0, CXR_LIGHTTYPE_SPOT);
			Light.m_LightGUID = 0x4008;
			Light.m_SpotHeight = m_LightSpotSize.k[1] * 1.25f;
			Light.m_SpotWidth  = m_LightSpotSize.k[0] * 1.25f;

			m_spGUISGI->SceneGraph_Light_LinkDynamic(Light);
		}

		if(1)
		{
			// for rotate in/out projectionmaps
			CMat4Dfp32 LightPos;
			LightPos.Unit();
			LightPos.RotX_x_M(-0.25f);

			LightPos.GetRow(3).k[2] = -0.25f; // first additional light a bit up and rotated downwards		
			LightPos.RotZ_x_M(0.01f);

			// Projection light
			//LightIntesity = m_LightIntesity;
			LightIntesity = m_LightIntesity * 0.75f;
			CXR_Light Light(LightPos, LightIntesity, m_LightSpotRange, 0, CXR_LIGHTTYPE_SPOT);
			Light.m_LightGUID = 0x4009;
			Light.m_SpotHeight = m_LightSpotSize.k[1] * 1.25f;
			Light.m_SpotWidth  = m_LightSpotSize.k[0] * 1.25f;

			CMat4Dfp32 ProjTransform;
			ProjTransform = LightPos;
			if(!(m_GUIFlags & GUI_FLAGS_FLIPUV)) // makes sense..
				ProjTransform.GetRow(1) = -LightPos.GetRow(1);

			if(m_TextureID_ProjectionTexture)
				Light.SetProjectionMap(m_TextureID_ProjectionTexture, &ProjTransform);
			m_spGUISGI->SceneGraph_Light_LinkDynamic(Light);
		}

		if(0)
		{
			// additional light
			CMat4Dfp32 LightPos;
			LightPos.Unit();

			LightPos.GetRow(3).k[2] = 6.0f; // first additional light a bit up and rotated downwards		
			LightPos.RotY_x_M(0.15f);


			CXR_Light AddLight(LightPos, m_AddSpotIntesity * 1.2f, m_AddSpotRange, 0, CXR_LIGHTTYPE_SPOT);
			//AddLight.m_Flags |= CXR_LIGHT_NOSHADOWS;
			AddLight.m_LightGUID = 0x4010;
			AddLight.m_SpotHeight = 0.45f;
			AddLight.m_SpotWidth  = 0.45f;

			m_spGUISGI->SceneGraph_Light_LinkDynamic(AddLight);
		}

		if(0)
		{
			// additional light
			CMat4Dfp32 LightPos;
			LightPos.Unit();

			LightPos.GetRow(3).k[2] = -20.0f;
			LightPos.GetRow(3).k[1] = -50.0f;

			CXR_Light AddLight(LightPos, m_AddSpotIntesity, 512, 0, CXR_LIGHTTYPE_SPOT);
			AddLight.m_LightGUID = 0x4013;
			AddLight.m_SpotHeight = m_AddSpotSize.k[1];
			AddLight.m_SpotWidth  = m_AddSpotSize.k[0];

			m_spGUISGI->SceneGraph_Light_LinkDynamic(AddLight);
		}

		CMat4Dfp32 Unit;
		Unit.Unit();
		_pEngine->Render_AddModel(pGUIWorld, Unit, CXR_AnimState(), XR_MODEL_WORLD);

		// Add background shader model
		CXR_AnimState AnimState;
		AnimState.Clear();
		AnimState.m_AnimAttr0 = m_BackgrounProcessTimer;
		AnimState.m_AnimAttr1 = BACKGROUNDPROCESSTIME;
		AnimState.m_Data[0] = m_TextureID_ScreencapToProcess;
		AnimState.m_Data[1] = m_TextureID_FinalImage;
		AnimState.m_Data[2] = m_TextureID_PostProcessTemp;
		_pEngine->Render_AddModel(&m_BG_GUI, Unit, AnimState, XR_MODEL_STANDARD);
		return;
	}


	int NrOfObjects = 0;
	CXR_Model* pTentacleModel = m_spMapData->GetResource_Model(m_iTentacleModel);
	if(pTentacleModel)
	{
		CBox3Dfp32 Box;
		//pModelChar->GetBound_Box(Box, &m_CharAnimState);

		// Aaargh!!
		Box.m_Max = 535.0f;
		Box.m_Min = -535.0f;

		NrOfObjects++;
		m_TentacleAnimState.m_iObject = NrOfObjects; // 1

		m_spGUISGI->SceneGraph_UnlinkElement(m_TentacleAnimState.m_iObject);
		m_spGUISGI->SceneGraph_LinkElement(m_TentacleAnimState.m_iObject, Box, CXR_SCENEGRAPH_SHADOWCASTER);
		m_spGUISGI->SceneGraph_CommitDeferred();

		CMat4Dfp32 CharPos;
		CharPos.Unit();
		_pEngine->Render_AddModel(pTentacleModel, CharPos, m_TentacleAnimState);
	}


}

void CCube::LoadingScene_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType)
{
	if (m_LoadingSceneAnim == "" || !m_spLoadingSGI)
		return;

	if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
	{
		CXR_Model* pLoadingWorld = m_spMapData->GetResource_Model(m_iLoadingWorld);
		if (!pLoadingWorld)
			return;

		m_spLoadingSGI->SceneGraph_Light_ClearDynamics();

		{
			// "ambient" point light
			CMat4Dfp32 LightPos;
			LightPos.Unit();
			LightPos.GetRow(3) = CVec3Dfp32(30.0f, 10.0f, 20.0f);

			CVec3Dfp32 LightIntesity = CVec3Dfp32(0.5f, 0.61f, 0.69f) * 0.36f;
			CXR_Light AddLight(LightPos, LightIntesity, 80, 0, CXR_LIGHTTYPE_POINT);
			AddLight.m_Flags |= CXR_LIGHT_NOSHADOWS;
			AddLight.m_LightGUID = 0x4014;
			m_spLoadingSGI->SceneGraph_Light_LinkDynamic(AddLight);
		}

		{
			CMat4Dfp32 LightPos;
			LightPos.Unit();
			LightPos.GetRow(3) = CVec3Dfp32(50.0f, 15.0f, 175.0f);
			LightPos.GetRow(0) = (CVec3Dfp32(-15.0f,0.0f,0.0f) -LightPos.GetRow(3)).Normalize();
			LightPos.RecreateMatrix(0,1);
			
			CVec3Dfp32 LightIntesity = CVec3Dfp32(1.22f, 1.2f, 1.14f) * 1.12f;
			CXR_Light AddLight(LightPos, LightIntesity, 1024, 0, CXR_LIGHTTYPE_SPOT);

			AddLight.m_SpotHeight = 0.18f;
			AddLight.m_SpotWidth  = 0.18f;

			AddLight.m_LightGUID = 0x4015;
			m_spLoadingSGI->SceneGraph_Light_LinkDynamic(AddLight);
		}

		CMat4Dfp32 Unit;
		Unit.Unit();
		_pEngine->Render_AddModel(pLoadingWorld, Unit, CXR_AnimState(), XR_MODEL_WORLD);
		return;
	}

	int NrOfObjects = 0;

	CXR_Model* pModelChar = m_spMapData->GetResource_Model(m_iModelJackie);
	if (pModelChar)
	{
		CBox3Dfp32 Box;
		//pModelChar->GetBound_Box(Box, &m_CharAnimState);

		// don't render special hair-shadow polygons
		m_CharAnimState.m_SurfaceOcclusionMask = 4;
		m_CharAnimState.m_SurfaceShadowOcclusionMask = 4;

		// Aaargh!!
		Box.m_Max = 135.0f;
		Box.m_Min = -135.0f;

		NrOfObjects++;
		m_CharAnimState.m_iObject = NrOfObjects; // 1

		m_spLoadingSGI->SceneGraph_UnlinkElement(m_CharAnimState.m_iObject);
		m_spLoadingSGI->SceneGraph_LinkElement(m_CharAnimState.m_iObject, Box, CXR_SCENEGRAPH_SHADOWCASTER);
		m_spLoadingSGI->SceneGraph_CommitDeferred();

		CMat4Dfp32 CharPos;
		CharPos.Unit();
		_pEngine->Render_AddModel(pModelChar, CharPos, m_CharAnimState);
	}

	if (!m_CharAnimState.m_pSkeletonInst)
		return;

	CXR_Model *pParticleSystem = m_spMapData->GetResource_Model(m_iLSMParticles);
	if(pParticleSystem)
	{
		CXR_AnimState AnimState;
		AnimState.Clear();
		AnimState.m_AnimTime0 = m_CharAnimState.m_AnimTime0;

		NrOfObjects++;
		AnimState.m_iObject = NrOfObjects;

		CMat4Dfp32 PartSystemPos;
		PartSystemPos.Unit();
		PartSystemPos.GetRow(3) = CVec3Dfp32(0.0f, 0.0f, 0.0f);

		_pEngine->Render_AddModel(pParticleSystem, PartSystemPos, AnimState);
	}

	CXR_Model* pModel = m_spMapData->GetResource_Model(m_iModelWeaponMain);
	if(pModel)
	{
		do
		{
			CXR_AnimState AnimState;
			AnimState.Clear();
			//--------------------------------------------------------------------
			// Get the position

			CMat4Dfp32 WeaponMainPos;
			const CXR_SkeletonAttachPoint* pHand = pModelChar->GetSkeleton()->GetAttachPoint(0);
			if(!pHand)
				break;

			CMat4Dfp32 Pos;
			const CMat4Dfp32& BoneTransform = m_CharAnimState.m_pSkeletonInst->m_pBoneTransform[pHand->m_iNode];
			pHand->m_LocalPos.Multiply(BoneTransform, Pos);

			CXR_Skeleton* pSkelGun = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(!pSkelGun)
				break;

			CMat4Dfp32 Handle;
			const CXR_SkeletonAttachPoint* pGunHandle = (pSkelGun) ? pSkelGun->GetAttachPoint(0) : NULL;

			if (pGunHandle)
				pGunHandle->m_LocalPos.InverseOrthogonal(Handle);
			else
				Handle.Unit();

			Handle.Multiply(Pos, WeaponMainPos);

			//--------------------------------------------------------------------
			//  Render calls
			CBox3Dfp32 Box;
			//pModel->GetBound_Box(Box);
			Box.m_Max = 135.0f;
			Box.m_Min = -135.0f;

			NrOfObjects++;
			AnimState.m_iObject = NrOfObjects;

			m_spLoadingSGI->SceneGraph_UnlinkElement(AnimState.m_iObject);
			m_spLoadingSGI->SceneGraph_LinkElement(AnimState.m_iObject, Box, CXR_SCENEGRAPH_SHADOWCASTER);
			m_spLoadingSGI->SceneGraph_CommitDeferred();

			_pEngine->Render_AddModel(pModel, WeaponMainPos, AnimState);
			break;
		}
		while(0);
	}

	pModel  = m_spMapData->GetResource_Model(m_iModelWeaponSecondary);
	if(pModel)
	{
		do
		{
			CXR_AnimState AnimState;
			AnimState.Clear();

			//--------------------------------------------------------------------
			// Get the position

			CMat4Dfp32 WeaponSecondaryPos;
			const CXR_SkeletonAttachPoint* pHand = pModelChar->GetSkeleton()->GetAttachPoint(1);
			if(!pHand)
				break;

			CMat4Dfp32 Pos;
			const CMat4Dfp32& BoneTransform = m_CharAnimState.m_pSkeletonInst->m_pBoneTransform[pHand->m_iNode];
			pHand->m_LocalPos.Multiply(BoneTransform, Pos);

			CXR_Skeleton* pSkelGun = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(!pSkelGun)
				break;

			CMat4Dfp32 Handle;
			const CXR_SkeletonAttachPoint* pGunHandle = (pSkelGun) ? pSkelGun->GetAttachPoint(0) : NULL;

			if (pGunHandle)
				pGunHandle->m_LocalPos.InverseOrthogonal(Handle);
			else
				Handle.Unit();

			Handle.Multiply(Pos, WeaponSecondaryPos);

			//--------------------------------------------------------------------
			//  Render calls
			CBox3Dfp32 Box;
			//pModel->GetBound_Box(Box);
			Box.m_Max = 135.0f;
			Box.m_Min = -135.0f;

			NrOfObjects++;
			AnimState.m_iObject = NrOfObjects;

			m_spLoadingSGI->SceneGraph_UnlinkElement(AnimState.m_iObject);
			m_spLoadingSGI->SceneGraph_LinkElement(AnimState.m_iObject, Box, CXR_SCENEGRAPH_SHADOWCASTER);
			m_spLoadingSGI->SceneGraph_CommitDeferred();

			_pEngine->Render_AddModel(pModel, WeaponSecondaryPos, AnimState);
			break;
		}
		while(0);
	}

	pModel  = m_spMapData->GetResource_Model(m_iModelExtra);
	if(pModel)
	{
		CXR_AnimState AnimState;
		AnimState.Clear();

		CMat4Dfp32 ExtraModelPos;
		CBox3Dfp32 Box;
		pModel->GetBound_Box(Box);

		NrOfObjects++;
		AnimState.m_iObject = NrOfObjects;

		m_spLoadingSGI->SceneGraph_UnlinkElement(AnimState.m_iObject);
		m_spLoadingSGI->SceneGraph_LinkElement(AnimState.m_iObject, Box, CXR_SCENEGRAPH_SHADOWCASTER);
		m_spLoadingSGI->SceneGraph_CommitDeferred();

		CMat4Dfp32 ThePos;
		ThePos.Unit();
		ThePos.RotZ_x_M(-0.25f);
		ThePos.GetRow(3).k[2] -= 2.0f;
		ThePos.GetRow(3).k[1] += 7.0f;
		ThePos.GetRow(3).k[0] += 1.0f;
		_pEngine->Render_AddModel(pModel, ExtraModelPos, AnimState);
	}
}

CXR_Model_GUIBackground::CXR_Model_GUIBackground()
{
}

void CXR_Model_GUIBackground::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip,
									   spCXR_WorldLightState _spWLS, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat,
									   const CMat4Dfp32& _VMat, int _Flags)
{
	if(!_pAnimState || _pAnimState->m_Data[0] == 0)
		return;

	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	const CRct& ViewRect = pViewport->GetViewRect();
	int Width = ViewRect.GetWidth();
	int Height = ViewRect.GetHeight();

	// We need to clear the zbuffer before rendering starts
#ifdef PLATFORM_XENON
	_pVBM->AddClearRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER - 0.1, CDC_CLEAR_COLOR|CDC_CLEAR_ZBUFFER |CDC_CLEAR_STENCIL, 0x00000000, 1.0f, 0);
#endif
	int TextureID = _pAnimState->m_Data[0];
	// render the processed BG image
	if(TextureID)
	{
		// for flip or no-flip UV cords
		CPnt ScreenSize = _pRender->GetDC()->GetScreenSize();
		int sw = pViewport->GetViewRect().GetWidth();
		int sh = pViewport->GetViewRect().GetHeight();

		CXR_SceneGraphInstance* pSGI = _pEngine->m_pSceneGraphInstance;
		int iMinLight = MinMT(pSGI->SceneGraph_Light_GetIndex(0x4009), pSGI->SceneGraph_Light_GetIndex(0x4010));

		CVec2Dfp32 UVMin;
		CVec2Dfp32 UVMax;

		if (_pRender->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
		{
			UVMin = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
			UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(ScreenSize.x), (fp32(sh) - 0.0f) / fp32(ScreenSize.y));
		}
		else
		{
			UVMin = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
			UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(sh) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
		}

		CRect2Duint16 ScreenCoord;
		ScreenCoord.m_Max = CVec2Duint16(Width, Height);
		ScreenCoord.m_Min.SetScalar(0);

		int bRenderTextureVertFlip  = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
			CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
		CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

		CRC_Attributes* pA = _pVBM->Alloc_Attrib();

		if(!pA)
			return;

		pA->SetDefault();
		//pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, TextureID);

		// process image
		const fp32 ProcessTime = _pAnimState->m_AnimAttr0;
		const fp32 ProcessDone = _pAnimState->m_AnimAttr1;

		CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) _pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));

		if(!pFP)
			return;

		pFP->Clear();
		pFP->SetProgram("GUIFadeToWhite", 0);
		CVec4Dfp32* pParams = _pVBM->Alloc_V4(8);

		if(!pParams)
			return;

		// FP: X-size (0)	
		pParams[0] = 0.5f / Width;//1.0f / Width;

		// FP: Y-size (1)
		pParams[1] = 0.5f / Height;//1.0f / Height;

		// FP: Timer (3)
		fp32 param2val = Min((1.0f / ProcessDone) * ProcessTime, 2.0f);
		//param2val = 0.0f;
		pParams[2] = param2val;

		// fade everthing to white
		fp32 param3value = Max((1.0f /( ProcessDone - 0.3f)) * (ProcessTime - 0.3f), 0.0f);
		param3value = Min(1.0f, param3value);
		pParams[3] = param3value ;

		pFP->SetParameters(pParams, 8);
		pA->m_pExtAttrib = pFP;

		CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2D(500.0f-16.0f);
		if (!pMat2D)
			return;

		CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, ScreenCoord, 0xffffffff, (fp32)(iMinLight - 1), pA);
		pVBScreenQuad->Geometry_TVertexArray(pTV, 0);

		_pVBM->AddVB(pVBScreenQuad);

		fp32 GlowTimer = MinMT(MaxMT((pParams[3].k[0] - (ProcessDone*0.25f))*2.0f, 0.0f), 1.0f);
		GlowTimer = param2val;

		if(GlowTimer > 0.0f)
			RenderFinalGlow(_pEngine, GlowTimer, _pEngine->m_pRender, _pEngine->GetVBM(), (fp32)(iMinLight)-0.5f, (int)_pAnimState->m_Data[1], (int)_pAnimState->m_Data[2],  param3value);
	}
}

void CXR_Model_GUIBackground::RenderFinalGlow(CXR_Engine* _pEngine, fp32 _GlowTime, CRenderContext* _pRC, CXR_VBManager* _pVBM, fp32 _StartPrio, int _TextureIDFinal, int _TextureIDTmp, fp32 _RadialVal)
{
	// 1. Copy Screen to FIN (Original image)
	// 2. Shrink once to TMP
	// 3. Shrink twice to TMP
	// 4. Blur X/Y to TMP
	// 5. Blend TMP Blur on top of FIN
	// 6. Copy screen to TMP
	// 7. Shrink FIN once to FIN
	// 8. Shrink FIN twice to FIN
	// 9. Streak FIN to FIN
	//10. Blend FIN on top of TMP (voila)

	// We need to rerender the zbuffer
#ifdef PLATFORM_XENON
	_pVBM->AddNextClearParams(_StartPrio, CDC_CLEAR_COLOR|CDC_CLEAR_ZBUFFER |CDC_CLEAR_STENCIL, 0x00000000, 1.0f, 0);
#endif

	if(_GlowTime < 0.0001f)
	{
#ifdef PLATFORM_XENON
		_StartPrio += 0.01f;
		_pVBM->AddClearRenderTarget(_StartPrio, CDC_CLEAR_COLOR|CDC_CLEAR_ZBUFFER |CDC_CLEAR_STENCIL, 0x00000000, 1.0f, 0);

		_StartPrio += 0.01f;
		_pVBM->AddRenderRange(_StartPrio, CXR_VBPRIORITY_UNIFIED_ZBUFFER, CXR_VBPRIORITY_UNIFIED_ZBUFFER);
#endif
		return;
	}

	CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();
	int bRenderTextureVertFlip  = _pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	int sw = pVP->GetViewRect().GetWidth();
	int sh = pVP->GetViewRect().GetHeight();

	CVec2Dfp32 UVMin;
	CVec2Dfp32 UVMax;

	if (_pRC->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
	{
		UVMin = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
		UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(ScreenSize.x), (fp32(sh) - 0.0f) / fp32(ScreenSize.y));
	}
	else
	{
		UVMin = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
		UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(sh) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
	}

	CRct VPRect = pVP->GetViewRect();
	CRect2Duint16 VPRect16;
	VPRect16.m_Min.k[0] = VPRect.p0.x;
	VPRect16.m_Min.k[1] = VPRect.p0.y;
	VPRect16.m_Max.k[0] = VPRect.p1.x;
	VPRect16.m_Max.k[1] = VPRect.p1.y;

	CRct VPRectScreen(CPnt(0, 0), ScreenSize);
	CRect2Duint16 VPRectScreen16;
	VPRectScreen16.m_Min.k[0] = VPRectScreen.p0.x;
	VPRectScreen16.m_Min.k[1] = VPRectScreen.p0.y;
	VPRectScreen16.m_Max.k[0] = VPRectScreen.p1.x;
	VPRectScreen16.m_Max.k[1] = VPRectScreen.p1.y;

	do
	{
		//----------------------------------------------------------------------------------------------------------------------
		// the blur
		// Capture screen

		_StartPrio += 0.01f;
		_pVBM->AddCopyToTexture(_StartPrio, CRct(0, 0, VPRectScreen16.m_Max[0], VPRectScreen16.m_Max[1]), CPnt(0, 0), _TextureIDFinal, false);

		CVec2Dfp32* pTVBlur = NULL;
		{
			//TextureID_BlurScreen = _TextureIDTmp;
			// Render gauss X & Y
			{
				fp32 GlowExp = 2.0f; 
				CVec4Dfp32 GlowBias(0.0f);
				CVec3Dfp32 GlowScale(0.0f + (2.5f * _GlowTime), 0.0f + (2.5f * _GlowTime), 0.0f + (2.7f * _GlowTime));
				CVec3Dfp32 GlowGamma(1.0f);
				CVec2Dfp32 GlowCenter(0.5f);
				CRect2Duint16 VPBlur;
				CVec2Dfp32 UVBlurMin;
				CVec2Dfp32 UVBlurMax;

				VPBlur.m_Min[0] = VPRectScreen16.m_Min[0]  >> 1;
				VPBlur.m_Min[1] = VPRectScreen16.m_Min[1]  >> 1;
				VPBlur.m_Max[0] = VPRectScreen16.m_Max[0]  >> 1;
				VPBlur.m_Max[1] = VPRectScreen16.m_Max[1]  >> 1;
				VPBlur.m_Min[0] = VPBlur.m_Min[0]  >> 1;
				VPBlur.m_Min[1] = VPBlur.m_Min[1]  >> 1;
				VPBlur.m_Max[0] = VPBlur.m_Max[0]  >> 1;
				VPBlur.m_Max[1] = VPBlur.m_Max[1]  >> 1;

				UVBlurMin = UVMin * 0.25f;
				UVBlurMax = UVMax * 0.25f;

				CVec2Dfp32 *pTV = NULL;
				{
					_StartPrio += 0.01f;
					CXR_GaussianBlurParams GP;
					GP.m_Rect = VPRect16;
					GP.m_Bias	= GlowBias;
					GP.m_Gamma	= GlowGamma;
					GP.m_UVCenter = GlowCenter;
					GP.m_Scale	= GlowScale;
					GP.m_Exp	= GlowExp;
					GP.m_SrcUVMin = UVMin;
					GP.m_SrcUVMax = UVMax;
					GP.m_DstUVMin = UVMin;
					GP.m_DstUVMax = UVMax;
					GP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
					GP.m_DstPixelUV = _pEngine->m_Screen_PixelUV;
					GP.m_pVBM = _pVBM;
					GP.m_Priority = _StartPrio;
					GP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
					GP.m_TextureID_Src = _TextureIDFinal;
					GP.m_TextureID_Dst = _TextureIDTmp;
					GP.m_Flags = 0;
					GP.m_nShrink = 2;
					GP.SetFilter();

					if (!CXR_Util::GaussianBlur(&GP, pTV))
						break;

					_StartPrio += (((GP.m_nShrink * 2) + 4) *  0.01f);
				}

				//----------------------------------------------------------------------------------------------
				// store the bloomed image in _TextureIdTemp
				{
					{
						// render out background
						CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
							CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) : 
						CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

						CRC_Attributes* pA = _pVBM->Alloc_Attrib();
						if (!pTV || !pA)
							break;

						pA->SetDefault();
						pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
						pA->Attrib_TextureID(0, _TextureIDFinal);

						_StartPrio += 0.01f;
						CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, _StartPrio, pA);
						if (!pVB)
							break;

						pVB->Geometry_TVertexArray(pTV, 0);
						_pVBM->AddVB(pVB);
					}

					{
						// Reposition/stretch screen
						CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
							CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVBlurMin, UVBlurMax) :
						CXR_Util::VBM_CreateRectUV(_pVBM, UVBlurMin, UVBlurMax);

						CRC_Attributes* pA = _pVBM->Alloc_Attrib();
						if (!pTV || !pA)
							break;

						pA->SetDefault();
						pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
						pA->Attrib_TextureID(0, _TextureIDTmp);

						_StartPrio += 0.01f;
						CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, _StartPrio, pA);
						if (!pVB)
							break;

						pVB->Geometry_TVertexArray(pTV, 0);
						_pVBM->AddVB(pVB);
					}

					_StartPrio += 0.01f;
					_pVBM->AddCopyToTexture(_StartPrio, CRct(0, 0, VPRectScreen16.m_Max[0], VPRectScreen16.m_Max[1]), CPnt(0, 0), _TextureIDTmp, false);				
				}

				//-------------------------------------------------------------------------------------------------------------------------
				// streaks
				{	
					_StartPrio += 0.01f;
					CXR_ShrinkParams SP;
					SP.m_SrcUVMin = UVMin;
					SP.m_SrcUVMax = UVMax;
					SP.m_DstUVMin = UVMin;
					SP.m_DstUVMax = UVMax;
					SP.m_nShrink = 2;
					SP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
					SP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
					SP.m_Priority = _StartPrio;
					SP.m_pVBM = _pVBM;
					SP.m_TextureID_Src = _TextureIDFinal;
					SP.m_TextureID_Dst = _TextureIDFinal;
					SP.m_Rect = VPRect16;
					if (!CXR_Util::ShrinkTexture(&SP, pTV))
						break;

					_StartPrio += ((SP.m_nShrink * 2) *  0.01f);

					CVec2Dfp32 UVTexelDownsample = CVec2Dfp32(1.0f / (fp32(VPBlur.m_Max[0] - VPBlur.m_Min[0])),
						1.0f / (fp32(VPBlur.m_Max[1] - VPBlur.m_Min[1])));

					CRC_Attributes* pA = _pVBM->Alloc_Attrib();

					if(!pA)
						break;

					pA->SetDefault();

					CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) _pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
					if (!pFP)
						break;
					pFP->Clear();
					pFP->SetProgram("GUIRadialBlur", 0);

					int nFPParams = 2;
					CVec4Dfp32* pFPParams = _pVBM->Alloc_V4(nFPParams);

					if(!pFPParams)
						break;

					// Center point in 2d space
					pFPParams[0][0] = ((UVBlurMax[0] - UVBlurMin[0]) / 2);
					pFPParams[0][1] = (bRenderTextureVertFlip) ? 1.0f - ((UVBlurMax[1] - UVBlurMin[1]) / 2) :
						(UVBlurMax[1] - UVBlurMin[1]) / 2;
					pFPParams[0][2] = 0;
					pFPParams[0][3] = 0;

					pFPParams[1][0] = UVTexelDownsample[0];
					pFPParams[1][1] = UVTexelDownsample[1];


					pFPParams[1][2] = _RadialVal * 1.0f;
					pFPParams[1][3] = _RadialVal * 2.0f;

					pFP->SetParameters(pFPParams, nFPParams);

					pA->m_pExtAttrib = pFP;

					pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
					pA->Attrib_TextureID(0, _TextureIDFinal);
					pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

					_StartPrio += 0.01f;
					CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPBlur, 0xffffffff, _StartPrio, pA);
					if(!pVB)
						break;

					pVB->Geometry_TVertexArray((fp32*)pTV, 0, 2);
					_pVBM->AddVB(pVB);

					_StartPrio += 0.01f;
					_pVBM->AddCopyToTexture(_StartPrio, CRct(0, 0, VPBlur.m_Max[0], VPBlur.m_Max[1]), CPnt(0,0), _TextureIDFinal, false);
				}

				pTVBlur = pTV;				
			}
		}

		// We need to rerender the zbuffer
#ifdef PLATFORM_XENON
		_StartPrio += 0.01f;
		_pVBM->AddClearRenderTarget(_StartPrio, CDC_CLEAR_COLOR|CDC_CLEAR_ZBUFFER |CDC_CLEAR_STENCIL, 0x00000000, 1.0f, 0);

		_StartPrio += 0.01f;
		_pVBM->AddRenderRange(_StartPrio, CXR_VBPRIORITY_UNIFIED_ZBUFFER, CXR_VBPRIORITY_UNIFIED_ZBUFFER);
#endif

		// Reposition/stretch screen
		CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
			CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
		CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pTV || !pA)
			break;

		const int nFPParams = 1;
		uint8* pFPMem = (uint8*) _pVBM->Alloc(2 * (sizeof(CRC_ExtAttributes_FragmentProgram20) + sizeof(CVec4Dfp32)*nFPParams));

		if(!pFPMem)
			break;

		CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pFPMem; pFPMem += sizeof(CRC_ExtAttributes_FragmentProgram20);

		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
		CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0xffffffff, 1, pA);
		if (!pVB)
			break;

		pFP->Clear();
		pFP->SetProgram("XREngine_FinalNoExposure", 0);
		pA->m_pExtAttrib = pFP;

		pA->Attrib_TextureID(0, _TextureIDFinal);
		pA->Attrib_TextureID(1, _TextureIDTmp);
		pVB->Geometry_TVertexArray(pTVBlur, 0);
		pVB->Geometry_TVertexArray(pTV, 1);

		_StartPrio += 0.01f;
		pVB->m_Priority = _StartPrio;
		_pVBM->AddVB(pVB);		
	}
	while(0);
}


void CCube::UpdateLSAnimation(CXR_VBManager* _pVBM, fp32 _DeltaTime)
{
	CXR_Model* pModel  = m_spMapData->GetResource_Model(m_iModelJackie);
	if(pModel)
	{
		int NrOfObjects = 0;
		CXR_Skeleton *pSkel;
		m_CharAnimState.Clear();

		pSkel = pModel->GetSkeleton();

		CMat4Dfp32 ThePos;
		ThePos.Unit();
		ThePos.GetRow(3) = CVec3Dfp32(0.0f, 0.0f, 0.0f);

		if(pSkel)
		{
			m_CharAnimState.m_pSkeletonInst = 
				CXR_VBMHelper::Alloc_SkeletonInst(_pVBM, pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
			
			if(!m_CharAnimState.m_pSkeletonInst)
				return;

			CMTime Time = CMTime::GetCPU();
			fp32 dt = Time.GetTime() - m_LastRenderTime;
			dt = Min(0.1f, Max(dt, 0.03f));
			m_LastRenderTime = Time.GetTime();

			if(m_GUIFlags & GUI_FADE_OUT_IN_PROGRESS)
				m_LoadingSceneAnimTime += dt;

			m_LoadingSceneAnimTime = Min(m_LoadingSceneAnimTime, m_TotalAnimLength);
			m_CharAnimState.m_AnimTime0 = CMTime::CreateFromSeconds(m_LoadingSceneAnimTime);

			CXR_AnimLayer Layers[4];
			int nMaxLayers = 4;
			int nLayers = 0;

			if(m_hVoice != -1)// && !(m_GUIFlags & GUI_FADE_OUT_IN_PROGRESS))
			{
				if(!m_VoCap.Eval(m_spSoundContext, m_hVoice, Time, -1, Layers, nLayers, nMaxLayers))
					return;
			}
			else// for those monologs without sound.
			{
				CXR_Anim_SequenceData* pSeq;
				CXR_Anim_Base* pAnim = m_spMapData->GetResource_Anim(m_VoCap.m_lAnimResources[m_iCurrentAnim]);
				pSeq = pAnim ? pAnim->GetSequence(m_iAnimID) : NULL;

				if(!pSeq)
					return;

				Layers[nLayers].Create3(pSeq, m_CharAnimState.m_AnimTime0, 1.0f, 1.0f, 0);
				nLayers++;
			}
			Layers[0].m_Blend = 1.0f; // Make sure main anim has full blend

			CXR_Anim_Base* pPistolGripAnim = m_spMapData->GetResource_Anim(m_iAnimGripBasePistol);
			CXR_Anim_SequenceData* pGripSeq = pPistolGripAnim ? pPistolGripAnim->GetSequence(6) : NULL;

			// basejoint for left hand is 21, and 16 for right hand. index 06 in DPis_PistolGrip is "main weapon"
			if(m_iModelWeaponSecondary)
			{
				if(pGripSeq && nLayers < nMaxLayers)
				{
					Layers[nLayers].Create3(pGripSeq, CMTime(), 1.0f, 1.0f, 21, CXR_ANIMLAYER_IGNOREBASENODE);
					nLayers++;
				}
			}

			if(m_iModelWeaponMain)
			{
				if(pGripSeq && nLayers < nMaxLayers)
				{
					Layers[nLayers].Create3(pGripSeq, CMTime(), 1.0f, 1.0f, 16, CXR_ANIMLAYER_IGNOREBASENODE);
					nLayers++;
				}
			}

			pSkel->EvalAnim(Layers, nLayers, m_CharAnimState.m_pSkeletonInst, ThePos, 0, CWO_AnimUtils::GetFaceDataTrackMask()); // 2?

			// face setup
			CFacialSetup *pFacialSetup = m_spMapData->GetResource_FacialSetup(m_iFacialSetup);
			if (pFacialSetup && pSkel && m_CharAnimState.m_pSkeletonInst)
			{
				CXR_SkeletonInstance* pSkelInst = m_CharAnimState.m_pSkeletonInst;

				enum { FACEDATA_BASETRACK = 21 };
				enum { FACEDATA_MAXCHANNELS = 51 };
				fp32 lFaceData[FACEDATA_MAXCHANNELS];
				pFacialSetup->GetFaceData(pSkelInst, FACEDATA_BASETRACK, FACEDATA_MAXCHANNELS, lFaceData);

				// Set blend values to get somewhat matching eye direction
				CMat4Dfp32 MatLook;
				MatLook.Unit();
				if(m_CharAnimState.m_pSkeletonInst->m_nBoneTransform > 11)
				{
					const CMat4Dfp32& MatHead = m_CharAnimState.m_pSkeletonInst->m_pBoneTransform[11];
					CVec3Dfp32 Pos = (pSkel->m_lNodes[11].m_LocalCenter + CVec3Dfp32(5, 0, 1)) * MatHead;

					CVec3Dfp32 LookPos = m_GuiCameraMatrix.GetRow(3);
					CVec3Dfp32 EyeLookDir = (Pos - LookPos).Normalize();

					fp32 Fwd = EyeLookDir * MatLook.GetRow(0);
					if ((EyeLookDir.k[0] <= 1.0f) && (Fwd > 0.1f))
					{
						fp32 Right = -(M_ACos((EyeLookDir * MatHead.GetRow(1))) * (1.0f / _PI) - 0.5f) * (1.0f / 0.4f);
						fp32 Up =    (M_ACos((EyeLookDir * MatHead.GetRow(2))) * (1.0f / _PI) - 0.5f) * (1.0f / 0.3f);
						lFaceData[45] = Clamp01(Up);     // Up
						lFaceData[46] = Clamp01(-Up);    // Down
						lFaceData[47] = Clamp01(Right);  // Right
						lFaceData[48] = Clamp01(-Right); // Left
					}

					for(int i = 0; i < FACEDATA_MAXCHANNELS; i++)
						lFaceData[i] = Clamp01(lFaceData[i] * 1.5f);
				}

				pFacialSetup->Eval(lFaceData, pSkelInst);

				// Re-evaluate from head joint and up
				//PLAYER_ROTTRACK_NECK = 10,
				//PLAYER_ROTTRACK_HEAD = 11,
				const CMat4Dfp32& BaseMat = pSkelInst->m_pBoneTransform[10];
				pSkel->InitEvalNode(11, BaseMat, pSkelInst);
			}

			//-----------------------------------------------------------------------------------------------
			// setup cloth
			if(pSkel->m_lCloth.Len() > 0 && pSkel->m_lCloth.Len() != m_lCloth.Len())
				m_lCloth.SetLen(pSkel->m_lCloth.Len());

			for(int j = 0; j < pSkel->m_lCloth.Len(); j++)
			{

				// Check if we need to recreate cloth data
				const CXR_SkeletonCloth& Cloth = m_lCloth[j];
				bool bNeedCreate = !Cloth.m_IsCreated;
				bNeedCreate |= (Cloth.m_lJointPosition.Len() != (pSkel->m_lCloth[j].m_MaxID+1));

				if (bNeedCreate && !Cloth.m_bInvalid)
				{
					M_TRACEALWAYS("recreate monolog cloth %d\n", j);
					m_lCloth[j].Create(j, pSkel, m_CharAnimState.m_pSkeletonInst, &pModel, 1);
					m_lCloth[j].m_LastUpdate.MakeInvalid();
				}

				int niter = 6;
				if(dt <= 0)
				{
					m_lCloth[j].Reset();
					if(!m_lCloth[j].m_LastUpdate.IsInvalid())
					{
						dt = (CMTime::GetCPU() - m_lCloth[j].m_LastUpdate).GetTime();
						if(dt > 0.02)
							dt = 0.02;
					}
					else
					{
						niter = 30;
						dt = niter * 0.005f;
					}
				}
				else if(dt > 0.02)
					dt = 0.02;

				m_lCloth[j].Step(&pModel, 1, pSkel, m_CharAnimState.m_pSkeletonInst, dt);
				m_lCloth[j].m_LastUpdate = CMTime::GetCPU();
			}
		}
	}
}

static fp32 Sinc(fp32 _x)
{
	MAUTOSTRIP(Sinc, 0.0f);
	return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;
}

void CCube::UpdateLSCameraPos(CMat4Dfp32* _pCameraMat)
{
	CXR_Model* pModel  = m_spMapData->GetResource_Model(m_iModelJackie);
	if(pModel)
	{
		bool bOverrideAnim = true;

		CXR_Skeleton *pSkel;
		pSkel = pModel->GetSkeleton();
		if(pSkel &&  m_CharAnimState.m_pSkeletonInst)
		{
			CMat4Dfp32 CameraMat;
			fp32 GetFov;
			if(CWO_AnimUtils::GetCamera(pSkel, m_CharAnimState.m_pSkeletonInst, CameraMat, &GetFov))
			{
				*_pCameraMat =CameraMat;

				// and since my axis are fubar...
				_pCameraMat->RotX_x_M(-0.25f);
				_pCameraMat->RotY_x_M(0.25f);

				if(GetFov != -1)
					m_LoadingSceneFOV = GetFov;
				
				// Plan-B
				// this butt-ugly fulhack should be removed when all cameras are animated
				// set debug-camera position
				//PLAYER_ROTTRACK_HEAD = 11,
				CVec3Dfp32 HeadMatPos = pSkel->m_lNodes[11].m_LocalCenter * m_CharAnimState.m_pSkeletonInst->m_pBoneTransform[11];
				CVec3Dfp32 HeadToCamera = (CameraMat.GetRow(3) - HeadMatPos);
				if(HeadToCamera.LengthSqr() > 50.0f)
					bOverrideAnim = false;
			}
		}
				
		//--------------------------------------------------------------------------------
		if(bOverrideAnim)
		{
			// dont ask.. this WILL be removed when all camera animations are in place. /Jakob
			m_LoadingSceneFOV = 86.0f;
			CMat4Dfp32 CameraMat;
			CameraMat.Unit();
			CameraMat.GetRow(3) = CVec3Dfp32(50.0f, 0.0f,  25.0f);

			*_pCameraMat = CameraMat;
			_pCameraMat->GetRow(0) =  (CVec3Dfp32(0.0f, 0.0f,  40.0f) - CameraMat.GetRow(3)).Normalize();
			_pCameraMat->RecreateMatrix(0,2);
			_pCameraMat->RotX_x_M(0.75);
			_pCameraMat->RotY_x_M(0.25);
		}		
	}
}

// newthing
void CCube::LoadAndSetupGUITentacles()
{
	m_TentacleBlendVal = 0.0f;
	m_bForceTentacleBlend = false;

	m_TentacleAnimTime = 0.0f;
	m_iCurrentTentacleAnim = TENTACLE_ANIM_IN;
	m_iNextTentacleAnim = TENTACLE_ANIM_DEFAULT_IDLE;
	m_GUIFlags &= ~GUI_FLAGS_SETFIRSTTEMPLATE;

	if (!(m_GUIFlags & GUI_FLAGS_DISABLE_TENTACLES))
	{
		m_iTentacleModel = m_spMapData->GetResourceIndex_Model("Characters\\Darkness\\GUI_Tentacle_Test");
		CXR_Model *pTentacleModel = m_spMapData->GetResource_Model(m_iTentacleModel);

		m_iTentacleAnim = m_spMapData->GetResourceIndex_Anim("GUI\\Gui_Tentakle_Outgame");
	}

	// setup camera values
	// _pCameraMat->GetRow(3) = CVec3Dfp32(-0.5f, 0.0f,  0.0f); // in+, right, up+
	m_GuiCameraMatrix.Unit();
	m_CameraMaxPos = CVec3Dfp32(-0.3f,  1.3f,  1.0f);
	m_CameraMinPos = CVec3Dfp32(-1.0f, -1.3f, -1.0f);
	m_CurRot	   = CVec3Dfp32( 0.0f,  0.0f,  0.0f);
	m_CurPos	   = CVec3Dfp32( 0.0f,  0.0f,  0.0f);
	m_CameraMaxRot = CVec3Dfp32( 0.0f,  0.0f,  0.0f);
	m_CameraMinRot = CVec3Dfp32( 0.0f,  0.0f,  0.0f);

}


void CCube::UpdateTentacles(fp32 _DeltaTime, CXR_Engine * _pEngine, CRenderContext* _pRC, CXR_VBManager* _pVBM, int8 _Scroll)
{
	
	if((_Scroll && m_iCurrentTentacleAnim != TENTACLE_ANIM_IN && m_iCurrentTentacleAnim != TENTACLE_ANIM_OUT) || 
		(m_GUIFlags & GUI_FLAGS_SETFIRSTTEMPLATE))
	{
		M_TRACEALWAYS("Setting new template %d\n", m_CurrentTentacleTemplate);
		m_GUIFlags &= ~GUI_FLAGS_SETFIRSTTEMPLATE;

		m_TentacleBlendVal = 0.0f;
		m_bForceTentacleBlend = true;

		// run swith to other template or same
		switch(m_CurrentTentacleTemplate)
		{
		//--------------------------------------------------------------
		case TENTACLE_TEMPLATE_DEFAULT:
			switch(m_LastTentacleTemplate)
			{
			case TENTACLE_TEMPLATE_DEFAULT:
				m_iNextTentacleAnim = TENTACLE_ANIM_DEFAULT_IDLE;
				break;

			case TENTACLE_TEMPLATE_KNOT:
				m_iNextTentacleAnim = TENTACLE_ANIM_DEFAULT_IDLE;//TENTACLE_ANIM_TRANS_KNOT_TO_DEFAULT;
				break;

			case TENTACLE_TEMPLATE_FULLWALL:
				m_iNextTentacleAnim = TENTACLE_ANIM_DEFAULT_IDLE;
			    break;

			default:
			    break;
			}
			break;
		//--------------------------------------------------------------
		case TENTACLE_TEMPLATE_KNOT:
			switch(m_LastTentacleTemplate)
			{
			case TENTACLE_TEMPLATE_DEFAULT:
				m_iNextTentacleAnim = TENTACLE_ANIM_KNOT_IDLE;//TENTACLE_ANIM_TRANS_DEFAULT_TO_KNOT;
				break;

			case TENTACLE_TEMPLATE_KNOT:
				m_iNextTentacleAnim = TENTACLE_ANIM_KNOT_IDLE;
				break;

			case TENTACLE_TEMPLATE_FULLWALL:
				m_iNextTentacleAnim = TENTACLE_ANIM_KNOT_IDLE;
				break;

			default:
				break;
			}
			break;
		//--------------------------------------------------------------
		case TENTACLE_TEMPLATE_FULLWALL:
			switch(m_LastTentacleTemplate)
			{
			case TENTACLE_TEMPLATE_DEFAULT:
				m_iNextTentacleAnim = TENTACLE_ANIM_WALL;
				break;

			case TENTACLE_TEMPLATE_KNOT:
				m_iNextTentacleAnim = TENTACLE_ANIM_WALL;
				break;

			case TENTACLE_TEMPLATE_FULLWALL:
				m_iNextTentacleAnim = TENTACLE_ANIM_WALL;
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
		m_CurrentTentacleTemplate = m_LastTentacleTemplate;
	}

	UpdateTentacleAnimation(_pVBM, _DeltaTime);
	UpdateTentacleCameraPos(&m_GuiCameraMatrix, _DeltaTime);
}

void CCube::UpdateTentacleCameraPos(CMat4Dfp32* _pCameraMat, fp32 _DeltaTime)
{
	fp32 SpeedVal = 0.0f;
	fp32 MaxSpeed = _DeltaTime * 0.10f;
	fp32 MinSpeed = 0.0001f;
	fp32 Modifier = 0.5f;
	//if(m_GUIFlags & GUI_FLAGS_ISJOURNAL)
	//	Modifier = 0.5f;

	//-------------------------------------------------------------------------------
	// left/right movement

	if(m_GUIFlags & GUI_FLAGS_CAMERAMOVINGRIGHT)
	{
		SpeedVal = _DeltaTime * (m_CameraMaxPos - m_CurPos).Length();
		SpeedVal = Min(SpeedVal, MaxSpeed*Modifier);
		SpeedVal = Max(SpeedVal, MinSpeed*Modifier);

		if(m_CurPos.k[1] > m_CameraMaxPos.k[1]*Modifier)
		{
			m_CurPos.k[1] = m_CameraMaxPos.k[1]*Modifier;
			m_GUIFlags &= ~GUI_FLAGS_CAMERAMOVINGRIGHT;
		}
		else
			m_CurPos.k[1] += SpeedVal;
	}
	else
	{
		SpeedVal = _DeltaTime * (m_CameraMinPos - m_CurPos).Length();
		SpeedVal = Min(SpeedVal, MaxSpeed*Modifier);
		SpeedVal = Max(SpeedVal, MinSpeed*Modifier);

		if(m_CurPos.k[1] < m_CameraMinPos.k[1]*Modifier)
		{
			m_CurPos.k[1] = m_CameraMinPos.k[1]*Modifier;
			m_GUIFlags |= GUI_FLAGS_CAMERAMOVINGRIGHT;
		}
		else
			m_CurPos.k[1] -= SpeedVal;
	}

	//-------------------------------------------------------------------------------
	// up/down movement
	if(m_GUIFlags & GUI_FLAGS_CAMERAMOVINGUP)
	{
		SpeedVal = _DeltaTime * (m_CameraMaxPos - m_CurPos).Length();
		SpeedVal = Min(SpeedVal, MaxSpeed*Modifier);
		SpeedVal = Max(SpeedVal, MinSpeed*Modifier);

		if(m_CurPos.k[2] > m_CameraMaxPos.k[2]*Modifier)
		{
			m_CurPos.k[2] = m_CameraMaxPos.k[2]*Modifier;
			m_GUIFlags &= ~GUI_FLAGS_CAMERAMOVINGUP;
		}
		else
			m_CurPos.k[2] += SpeedVal;
	}
	else
	{
		SpeedVal = _DeltaTime * (m_CameraMinPos - m_CurPos).Length();
		SpeedVal = Min(SpeedVal, MaxSpeed)*Modifier;
		SpeedVal = Max(SpeedVal, MinSpeed)*Modifier;

		if(m_CurPos.k[2] < m_CameraMinPos.k[2]*Modifier)
		{
			m_CurPos.k[2] = m_CameraMinPos.k[2]*Modifier;
			m_GUIFlags |= GUI_FLAGS_CAMERAMOVINGUP;
		}
		else
			m_CurPos.k[2] -= SpeedVal;
	}

	// Rotations
	_pCameraMat->Unit();
	CVec3Dfp32 CenterPos(5.0f, 0.0f, 0.0f);
	CVec3Dfp32 DaDir = (CenterPos - m_CurPos).Normalize();

	_pCameraMat->GetRow(2) = DaDir;
	_pCameraMat->GetRow(0) = CVec3Dfp32(0.0f, -1.0f, 0.0f);
	_pCameraMat->RecreateMatrix(2,0);

	// get the camera position from the skeleton
	_pCameraMat->GetRow(3) = m_CurPos;
	CXR_Model *pTentacleModel = m_spMapData->GetResource_Model(m_iTentacleModel);
	if(pTentacleModel)
	{
		CXR_Skeleton *pSkel = pTentacleModel->GetSkeleton();
		if(pSkel &&  m_TentacleAnimState.m_pSkeletonInst)
		{
			CVec3Dfp32 SkelCamPos = pSkel->m_lNodes[1].m_LocalCenter * m_TentacleAnimState.m_pSkeletonInst->m_pBoneTransform[1];
			_pCameraMat->GetRow(3) += SkelCamPos;
		}
	}
}

void CCube::UpdateTentacleAnimation(CXR_VBManager* _pVBM, fp32 _DeltaTime)
{
	CXR_Model* pTentacleModel  = m_spMapData->GetResource_Model(m_iTentacleModel);
	CXR_Anim_Base* pTentacleAnim = m_spMapData->GetResource_Anim(m_iTentacleAnim);

	if (!pTentacleModel || !pTentacleAnim)
	{
		m_TentacleTotalAnimLength = 0.0f;
		m_iCurrentTentacleAnim = TENTACLE_ANIM_DEFAULT_IDLE;
		return;
	}

	if (pTentacleModel && pTentacleAnim)
	{
		int NrOfObjects = 0;
		CXR_Skeleton *pSkel;
		m_TentacleAnimState.Clear();
		pSkel = pTentacleModel->GetSkeleton();

		CMat4Dfp32 ThePos;
		ThePos.Unit();
		ThePos.GetRow(3) = CVec3Dfp32(0.0f, 0.0f, 0.0f);

		if(pSkel)
		{
			m_TentacleAnimState.m_pSkeletonInst = 
				CXR_VBMHelper::Alloc_SkeletonInst(_pVBM, pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);


			CXR_Anim_SequenceData* pSeq;
			pSeq = pTentacleAnim->GetSequence(m_iCurrentTentacleAnim);
			if(!pSeq)
				return;

			m_TentacleTotalAnimLength = pSeq->GetDuration();


			CXR_AnimLayer Layers[2];
			int nLayers = 1;
			fp32 Differ = 0.0f;

			//  blend between animations
			if(m_bForceTentacleBlend)
			{
				m_TentacleBlendVal += _DeltaTime * TENTACLE_BLEND_SPEED;
				if(m_TentacleBlendVal < 1.0f)
				{
					fp32 BlendVal = Sinc(m_TentacleBlendVal);				

					CXR_Anim_SequenceData* pSeq2;
					pSeq2 = pTentacleAnim->GetSequence(m_iNextTentacleAnim);

					if(!pSeq2)
						return;

					nLayers++;
					Layers[1].Create3(pSeq2, CMTime(), 1.0f, BlendVal, 0);

					//M_TRACEALWAYS("-------- Blending between %d and %d. blendval %f\n",m_iCurrentTentacleAnim, m_iNextTentacleAnim, BlendVal);
				}
				else
				{
					m_iCurrentTentacleAnim = m_iNextTentacleAnim;
					if(m_iCurrentTentacleAnim == TENTACLE_ANIM_OUT)
						m_iNextTentacleAnim = TENTACLE_ANIM_OUT;
					else if(m_iCurrentTentacleAnim == TENTACLE_ANIM_IN)
						m_GUIFlags |= GUI_FLAGS_SETFIRSTTEMPLATE;

					m_TentacleBlendVal = 0.0f;
					m_bForceTentacleBlend = false;

					m_TentacleAnimTime = 0.0f;
					pSeq = pTentacleAnim->GetSequence(m_iCurrentTentacleAnim);

					if(!pSeq)
						return;

					m_TentacleTotalAnimLength = pSeq->GetDuration();				
				}
			}
			else
			{
				// continue with animation
				m_TentacleAnimTime += _DeltaTime;
				Differ = -Min(m_TentacleTotalAnimLength - m_TentacleAnimTime, 0.0f);
				m_TentacleAnimTime = Min(m_TentacleAnimTime, m_TentacleTotalAnimLength);
			}

			if(m_TentacleAnimTime == m_TentacleTotalAnimLength)
			{
				if(m_iCurrentTentacleAnim == TENTACLE_ANIM_OUT)
				{
					m_TentacleAnimTime = m_TentacleTotalAnimLength;
					m_bOutAnimDone = true;
				}
				else
				{
					if(m_iCurrentTentacleAnim == TENTACLE_ANIM_IN && m_CurrentTentacleTemplate != TENTACLE_TEMPLATE_DEFAULT)
						m_GUIFlags |= GUI_FLAGS_SETFIRSTTEMPLATE;
					
					if(m_iCurrentTentacleAnim == m_iNextTentacleAnim)
						m_TentacleAnimTime = Differ;
				
					m_iCurrentTentacleAnim = m_iNextTentacleAnim;

					if(m_iCurrentTentacleAnim == TENTACLE_ANIM_OUT)
						m_iNextTentacleAnim = TENTACLE_ANIM_OUT;
				}
			}

			m_TentacleAnimState.m_AnimTime0 = CMTime::CreateFromSeconds(m_TentacleAnimTime);

			Layers[0].Create3(pSeq, m_TentacleAnimState.m_AnimTime0, 1.0f, 1.0f, 0);
			pSkel->EvalAnim(Layers, nLayers, m_TentacleAnimState.m_pSkeletonInst, ThePos); // 2?	
		}
	}
}
void  CCube::RenderTentacles(CXR_Engine *_pEngine, CXR_VBManager *_pVBM, CRenderContext *_pRC)
{
	_pEngine->m_EngineMode = XR_MODE_UNIFIED;

	CRC_Viewport *pViewPort = _pVBM->Viewport_Get();
	CRC_Viewport TmpViewport = *pViewPort;

	fp32 StoredFov = TmpViewport.GetFOV();
	fp32 StoredFrontPlane = TmpViewport.GetFrontPlane(); 

	TmpViewport.SetFOV(86.0f);
	//if(m_GUIFlags & GUI_FLAGS_ISWIDESCREEN)
		TmpViewport.SetFOVAspect(1.78f);
	//else
	//	TmpViewport.SetFOVAspect(1.33f);
	TmpViewport.SetFrontPlane(0.8f); 

	_pVBM->Viewport_Push(&TmpViewport);
	CMat4Dfp32 CameraVelocity; CameraVelocity.Unit();
	_pEngine->Engine_Render(this, _pVBM, _pRC, m_GuiCameraMatrix, CameraVelocity, m_spGUISGI);
	_pVBM->Viewport_Pop();

}

bool CCube::GetTentaclesMovingOut()
{
	if(m_GUIFlags & GUI_FLAGS_EXITINGGUI)
		return true;
	return false;
}

void CCube::SetTentaclesMovingOut()
{
	PlaySoundFXEvent(SOUNDEVENT_TENTACLES_MOVING_OUT);
	StopSoundFX(m_BackgroundSoundLoopID, true);
	m_BackgroundSoundLoopID = -1;
	m_bOutAnimDone = false;

	m_iNextTentacleAnim = TENTACLE_ANIM_OUT;
	m_bForceTentacleBlend = true;

	m_GUIFlags |= GUI_FLAGS_EXITINGGUI;
	return;
}

bool CCube::GetExitComplete()
{
	if(m_bOutAnimDone)
		return true;
	else
		return false;
}

/*
void CCube::RenderSafeBorders(CRenderContext* _pRC, CXR_VBManager* _pVBM)
{
	fp32 PrecVis = 0.85f;
	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	const CRct& ViewRect = pViewport->GetViewRect();
	int Width = ViewRect.GetWidth();
	int Height = ViewRect.GetHeight();

	int32 Boxw = TruncToInt((Width-(Width * 0.85f))*0.5f);
	int32 Boxh = TruncToInt((Height-(Height * 0.85f))*0.5f);

	_pVBM->ScopeBegin("CCube::RenderSafeBorders",false);
	CRC_Util2D Util2D;
	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	Util2D.Begin(_pRC, pVP, _pVBM);

	CRct Rect;
	Rect.p0.x = 0;
	Rect.p0.y = 0;
	Rect.p1.x = Boxw;
	Rect.p1.y = Height;

	for(int i = 0; i < 4; i++)
	{
		if(i== 1)
		{
			Rect.p0.x = Width-Boxw;
			Rect.p0.y = 0;
			Rect.p1.x = Width;
			Rect.p1.y = Height;
		}
		else if(i == 2)
		{
			Rect.p0.x = Boxw;
			Rect.p0.y = 0;
			Rect.p1.x = Width-Boxw;
			Rect.p1.y = Boxh;
		}
		else if(i == 3)
		{
			Rect.p0.x = Boxw;
			Rect.p0.y = Height-Boxh;
			Rect.p1.x = Width-Boxw;
			Rect.p1.y = Height;
		}

		CVec2Dfp32 UVMin(0.0f, 0.0f);
		CVec2Dfp32 UVMax(1.0f, 1.0f);
		CVec2Dfp32* pTV = CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pTV || !pA)
			break;

		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
		if (!pMat2D)
			break;

		CRect2Duint16 VPRectScreen16;
		VPRectScreen16.m_Min.k[0] = Rect.p0.x;
		VPRectScreen16.m_Min.k[1] = Rect.p0.y;
		VPRectScreen16.m_Max.k[0] = Rect.p1.x;
		VPRectScreen16.m_Max.k[1] = Rect.p1.y;

		CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRectScreen16, 0x55400000, 0.0f, pA);
		if (!pVB)
			break;

		pVB->Geometry_TVertexArray(pTV, 0);
		_pVBM->AddVB(pVB);
	}


	Util2D.End();
	_pVBM->ScopeEnd();
}
*/

