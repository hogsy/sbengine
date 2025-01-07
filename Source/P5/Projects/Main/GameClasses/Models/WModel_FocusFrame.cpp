#include "PCH.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/Win/MWinGrph.h"
//#include "../WObj_Char.h"

// -------------------------------------------------------------------
//  CXR_Model_Number
// -------------------------------------------------------------------
class CXR_Model_FocusFrame : public CXR_Model_Custom
{
#define PINCIRCLESEGS (10)
#define PINCIRCLERADIUS (0.05f)
#define PINBORDERWIDTH (0.035f)
#define USEV2

	MRTC_DECLARE;
protected:
#ifdef USEV2
	//int32 m_iSurfaceBottomRight;
	// Don't know what's best. We only have one of this model so a little 
	// more memory shouldn't harm so much? Maybe this will speed things up a bit?
	// We don't have to do 10 sins and 10 cos's per frame atleast
	fp32 m_PreCalcedCos[PINCIRCLESEGS+1];
	fp32 m_PreCalcedSin[PINCIRCLESEGS+1];
#else
	int32 m_iSurfaceTopLeft;
	//int32 m_iSurfaceTopRight;
	int32 m_iSurfaceTopMiddle;
	int32 m_iSurfaceBottomLeft;
#endif

private:
	virtual void OnCreate(const char *surface)
	{
		MAUTOSTRIP(CXR_Model_FocusFrame_OnCreate, MAUTOSTRIP_VOID);
#ifdef USEV2	
		PrecalculatePinCircleAngles(PINBORDERWIDTH, PINCIRCLERADIUS);
#else
		// Get surfaces for all the textures
		m_iSurfaceTopLeft = GetSurfaceID("GUI_FOCUS_TL");
		//m_iSurfaceTopRight = GetSurfaceID("GUI_FOCUS_TR");
		m_iSurfaceTopMiddle = GetSurfaceID("GUI_FOCUS_TM");
		m_iSurfaceBottomLeft = GetSurfaceID("GUI_FOCUS_BL");
		//m_iSurfaceBottomRight = GetSurfaceID("GUI_FOCUS_BR");
#endif	
	}

#ifdef USEV2
	// Precalculate circle angles
	void PrecalculatePinCircleAngles(const fp32& _BorderWidth, const fp32& _Radius)
	{
		fp32 HalfBorder = _BorderWidth *(1.0f/2.0f);
		// These values should be precalced, borderwidth/radius as defines...
		
		fp32 StartAngle = M_ASin(HalfBorder/_Radius);
		fp32 Frag = (_PI2-StartAngle*2.0f) * (1.0f/ ((fp32)PINCIRCLESEGS));
		// Start with -45 degree angle + startpos (the pin stuff)
		StartAngle -= _PI*(3.0f/4.0f);

		for (int32 i = 0; i <= PINCIRCLESEGS; i++)
		{
			fp32 Angle = StartAngle + Frag * i;

			m_PreCalcedCos[i] = M_Cos(Angle);
			m_PreCalcedSin[i] = M_Sin(Angle);
		}
	}
#endif

	//----------------------------------------------------------------------

	M_INLINE void AddVertex(int32& _iVertex, CVec3Dfp32* _pVertexPos, const CVec3Dfp32& _Pos, 
		CPixel32* _pVertexColor, int32 _Color, int32 _Alpha, CVec2Dfp32* _pVertexTex = NULL, 
		fp32 _Tu = 0.0f, fp32 _Tv = 0.0f)
	{
		MAUTOSTRIP(CXR_Model_FocusFrame_AddVertex, MAUTOSTRIP_VOID);
		_pVertexPos[_iVertex] = _Pos;
		if (_pVertexTex)
		{
			_pVertexTex[_iVertex][0] = _Tu;
			_pVertexTex[_iVertex][1] = _Tv;
		}
		if (_pVertexColor)
			_pVertexColor[_iVertex] = _Color | (_Alpha << 24); // (m_LightColor & 0x00FFFFFF) + 
		_iVertex++;
	}

	//----------------------------------------------------------------------

	M_INLINE void AddTriangle(int32 _V1, int32 _V2, int32 _V3, uint16* _pIndex, const int32& _iVertex,
		int32& _iIndex)
	{
		MAUTOSTRIP(CXR_Model_FocusFrame_AddTriangle, MAUTOSTRIP_VOID);
		_pIndex[_iIndex + 0] = _iVertex + _V1;
		_pIndex[_iIndex + 1] = _iVertex + _V2;
		_pIndex[_iIndex + 2] = _iVertex + _V3;
		_iIndex += 3;
	}

	M_INLINE CXR_VertexBuffer* GetVB(CXR_Model_Custom_RenderParams* _pRenderParams, int32 _NumVerts, int32 _NumTris,
		CVec3Dfp32*& _pVertexPos, CPixel32*& _pVertexColor, uint16*& _pIndex)
	{
		CXR_VertexBuffer* pVB = AllocVB(_pRenderParams);
		if (pVB == NULL)
			return NULL;

		_pVertexPos = _pRenderParams->m_pVBM->Alloc_V3(_NumVerts);
		if (!_pVertexPos)
			return NULL;
		_pVertexColor = _pRenderParams->m_pVBM->Alloc_CPixel32(_NumVerts);
		if (!_pVertexColor)
			return NULL;
		_pIndex = _pRenderParams->m_pVBM->Alloc_Int16(_NumTris*3);
		if (!_pIndex)
			return NULL;
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return NULL;
		pVB->Geometry_VertexArray(_pVertexPos, _NumVerts, true);
		pVB->Geometry_ColorArray(_pVertexColor);

		return pVB;
	}

	M_INLINE CXR_VertexBuffer* GetVB(CXR_Model_Custom_RenderParams* _pRenderParams, int32 _NumVerts, int32 _NumTris,
		CVec3Dfp32*& _pVertexPos, CPixel32*& _pVertexColor, uint16*& _pIndex,
		CVec2Dfp32*& _pVertexTex)
	{
		_pVertexTex = _pRenderParams->m_pVBM->Alloc_V2(_NumVerts);

		CXR_VertexBuffer* pVB = GetVB(_pRenderParams, _NumVerts,_NumTris,_pVertexPos,_pVertexColor,_pIndex);
		pVB->Geometry_TVertexArray(_pVertexTex,0);

		return pVB;
	}
#ifndef USEV2
	bool BuildFocusFrameV1Part1(const CXR_AnimState* _pAnimState, const CVec3Dfp32& _ScreenPos, 
		fp32 _TextWidth, fp32 _YOffset, fp32 _BorderWidth)
	{
		// Sadly it seems that at this point we must have separate surfaces for the three different
		// parts, should be able to change this soon (when I get new textures)

		// Part 1 (top left/top right)
		int32 iVertex, iIndex;
		iVertex = iIndex = 0;
		const int NumVerts = 8;
		const int NumTris = 4;

		CVec3Dfp32* pVertexPos;
		CVec2Dfp32* pVertexTex;
		CPixel32* pVertexColor;
		uint16* pIndex;
		CXR_VertexBuffer* pVB = GetVB(NumVerts,NumTris,pVertexPos,pVertexColor,pIndex,pVertexTex);
		if (!pVB || !pVertexPos || !pVertexColor || !pIndex || !pVertexTex)
			return false;

		AddTriangle(4, 0, 1, pIndex, iVertex, iIndex);
		AddTriangle(4, 1, 5, pIndex, iVertex, iIndex);
		AddTriangle(6, 2, 3, pIndex, iVertex, iIndex);
		AddTriangle(6, 3, 7, pIndex, iVertex, iIndex);

		int32 Color = 0x00ffffff;
		int32 Alpha = 255;

		const fp32& Border = _BorderWidth;
		fp32 TextHalf = _TextWidth *(1.0f/2.0f);
		CVec3Dfp32 Pos = _ScreenPos + CVec3Dfp32(-(TextHalf + Border),-_YOffset,0);;
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(-TextHalf,-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf,-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf+Border,-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(-(TextHalf+Border),+Border-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(-(TextHalf),Border-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf,Border-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf+Border,Border-_YOffset,0);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 1.0f);

		CMat4Dfp32 Temp;
		Temp.Unit();
		CMat4Dfp32* pMat = _pRenderParams->m_pVBM->Alloc_M4(Temp);
		pVB->Matrix_Set(pMat);
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(pVertexPos, NumVerts, true);
		pVB->Geometry_TVertexArray(pVertexTex, 0);
		pVB->Geometry_ColorArray(pVertexColor);
		pVB->Render_IndexedTriangles(pIndex, NumTris);

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		//pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		Render_Surface(m_iSurfaceTopLeft, pVB, _pAnimState->m_AnimTime0);

		return false;
	}

	bool BuildFocusFrameV1Part2(const CXR_AnimState* _pAnimState, const CVec3Dfp32& _ScreenPos, 
		fp32 _TextWidth, fp32 _YOffset, fp32 _BorderWidth)
	{
		// Sadly it seems that at this point we must have separate surfaces for the three different
		// parts, should be able to change this soon (when I get new textures)

		// Part 1 (top left/top right)
		int32 iVertex, iIndex;
		iVertex = iIndex = 0;
		const int NumVerts = 4;
		const int NumTris = 2;

		CVec3Dfp32* pVertexPos;
		CVec2Dfp32* pVertexTex;
		CPixel32* pVertexColor;
		uint16* pIndex;
		CXR_VertexBuffer* pVB = GetVB(NumVerts,NumTris,pVertexPos,pVertexColor,pIndex,pVertexTex);
		if (!pVB || !pVertexPos || !pVertexColor || !pIndex || !pVertexTex)
			return false;

		AddTriangle(0, 1, 2, pIndex, iVertex, iIndex);
		AddTriangle(0, 2, 3, pIndex, iVertex, iIndex);

		int32 Color = 0x00ffffff;
		int32 Alpha = 255;

		// Text part
		const fp32& Border = _BorderWidth;
		fp32 TextHalf = _TextWidth *(1.0f/2.0f);
		CVec3Dfp32 Pos = _ScreenPos + CVec3Dfp32(-TextHalf, -_YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf, -_YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf, Border - _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(-(TextHalf),Border - _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 1.0f);

		CMat4Dfp32 Temp;
		Temp.Unit();
		CMat4Dfp32 *pMat = _pRenderParams->m_pVBM->Alloc_M4(Temp);
		pVB->Matrix_Set(pMat);
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(pVertexPos, NumVerts, true);
		pVB->Geometry_TVertexArray(pVertexTex, 0);
		pVB->Geometry_ColorArray(pVertexColor);
		pVB->Render_IndexedTriangles(pIndex, NumTris);

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		//pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		Render_Surface(m_iSurfaceTopMiddle, pVB, _pAnimState->m_AnimTime0);

		return false;
	}

	bool BuildFocusFrameV1Part3(const CXR_AnimState* _pAnimState, const CVec3Dfp32& _ScreenPos, 
		fp32 _TextWidth, fp32 _YOffset, fp32 _BorderWidth)
	{
		// Sadly it seems that at this point we must have separate surfaces for the three different
		// parts, should be able to change this soon (when I get new textures)

		// Part 1 (top left/top right)
		int32 iVertex, iIndex;
		iVertex = iIndex = 0;
		const int NumVerts = 8;
		const int NumTris = 4;

		CVec3Dfp32* pVertexPos;
		CVec2Dfp32* pVertexTex;
		CPixel32* pVertexColor;
		uint16* pIndex;
		CXR_VertexBuffer* pVB = GetVB(NumVerts,NumTris,pVertexPos,pVertexColor,pIndex,pVertexTex);
		if (!pVB || !pVertexPos || !pVertexColor || !pIndex || !pVertexTex)
			return false;

		AddTriangle(4, 0, 1, pIndex, iVertex, iIndex);
		AddTriangle(4, 1, 5, pIndex, iVertex, iIndex);
		AddTriangle(6, 2, 3, pIndex, iVertex, iIndex);
		AddTriangle(6, 3, 7, pIndex, iVertex, iIndex);

		int32 Color = 0x00ffffff;
		int32 Alpha = 255;

		const fp32& Border = _BorderWidth;
		fp32 TextHalf = _TextWidth *(1.0f/2.0f);
		CVec3Dfp32 Pos = _ScreenPos + CVec3Dfp32(-(TextHalf + Border),_YOffset,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(-TextHalf,_YOffset,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf,_YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf+Border,_YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 0.0f);
		Pos = _ScreenPos + CVec3Dfp32(-(TextHalf+Border),Border + _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(-(TextHalf), Border + _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf, Border + _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 1.0f, 1.0f);
		Pos = _ScreenPos + CVec3Dfp32(TextHalf+Border, Border + _YOffset, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha, pVertexTex, 0.0f, 1.0f);

		CMat4Dfp32 Temp;
		Temp.Unit();
		CMat4Dfp32 *pMat = _pRenderParams->m_pVBM->Alloc_M4(Temp);
		pVB->Matrix_Set(pMat);
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(pVertexPos, NumVerts, true);
		pVB->Geometry_TVertexArray(pVertexTex, 0);
		pVB->Geometry_ColorArray(pVertexColor);
		pVB->Render_IndexedTriangles(pIndex, NumTris);

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		//pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		Render_Surface(m_iSurfaceBottomLeft, pVB, _pAnimState->m_AnimTime0);

		return false;
	}
#else
	void DoPinCircle(CVec3Dfp32* _pVertexPos, CPixel32* _pVertexColor, uint16* _pIndex, 
		int32& _iVertex, int32& _iIndex, const CVec3Dfp32& _ScreenPos, const fp32& _BorderWidth, const fp32& _Radius, 
		const int32& _Segs, const int32 _Color, const int32 _Alpha)
	{
		if (!_pVertexPos || !_pVertexColor || !_pIndex)
			return;
		
		/*fp32 HalfBorder = _BorderWidth / 2.0f;
		fp32 A = M_Sqrt(_Radius*_Radius - HalfBorder*HalfBorder);
		// These values should be precalced, borderwidth/radius as defines...
		
		fp32 StartAngle = M_ASin(HalfBorder/_Radius);
		fp32 Frag = (_PI2-StartAngle*2.0f) / (fp32)_Segs;
		// Start with -45 degree angle + startpos (the pin stuff)
		StartAngle -= (_PI/2.0f + _PI/4.0f);*/

		CVec3Dfp32 Dir(0.0f,0.0f,0.0f);

		// Add the segment triangles
		for (int32 i = 0; i < _Segs; i++)
		{
			AddTriangle(0, i+1, i+2, _pIndex, _iVertex, _iIndex);
		}

		// Add a closing triangle
		AddTriangle(0, _Segs+1, 1, _pIndex, _iVertex, _iIndex);

		// Add center vertex
		AddVertex(_iVertex, _pVertexPos, _ScreenPos, _pVertexColor, _Color, _Alpha);
		for (int32 i = 0; i <= _Segs; i++)
		{
			//fp32 Angle = StartAngle + Frag * i;
			Dir.k[0] = m_PreCalcedCos[i];//M_Cos(Angle);
			Dir.k[1] = m_PreCalcedSin[i];//M_Sin(Angle);
			
			CVec3Dfp32 Pos = Dir * _Radius + _ScreenPos;

			AddVertex(_iVertex, _pVertexPos, Pos, _pVertexColor, _Color, _Alpha);
		} 
	}

	bool BuildFocusFrameV2(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CVec3Dfp32& _ScreenPos,
		const fp32& _Width, const fp32& _Height, const fp32& _ExtraWidth, const fp32& _ExtraHeight, 
		const fp32& _SlopeWidth, const fp32& _InfoHeight, const fp32& _BorderWidth, 
		const fp32& _PinCircleRadius, const fp32& _PinExtraWidth, const fp32& _PinSlopeWidth, 
		const bool& _bFull)
	{
		//ConOut(Temp.GetString());
		int32 iIndex,iVertex;
		iIndex = iVertex = 0;
		// How many vertices in the "pin"
		const int32 PinCircleSegs = PINCIRCLESEGS;
		const int32 PinOffset = 6 + PinCircleSegs;
		const int32 PinTris = 5 + PinCircleSegs + 1; // Circlesegs + closing triangle
		const int32 NumVerts = PinOffset + (_bFull ? 16 : 6);
		const int32 NumTris = PinTris + (_bFull ? 12 : 4);

		CVec3Dfp32* pVertexPos;
		CPixel32* pVertexColor;
		uint16* pIndex;
		CXR_VertexBuffer* pVB = GetVB(_pRenderParams, NumVerts,NumTris,pVertexPos,pVertexColor,pIndex);
		if (!pVB || !pVertexPos || !pVertexColor || !pIndex)
			return false;

		// Description part
		AddTriangle(0, 1, 2, pIndex, iVertex, iIndex);
		AddTriangle(2, 3, 4, pIndex, iVertex, iIndex);
		AddTriangle(2, 4, 5, pIndex, iVertex, iIndex);
		AddTriangle(2, 5, 0, pIndex, iVertex, iIndex);

		// Pin Part
		AddTriangle(4, 6, 7, pIndex, iVertex, iIndex);
		AddTriangle(7, 8, 9, pIndex, iVertex, iIndex);
		AddTriangle(4, 7, 9, pIndex, iVertex, iIndex);
		
		// done from circle now
		AddTriangle(8, 11, 9, pIndex, iVertex, iIndex);
		AddTriangle(9, 11, 11 + PinCircleSegs, pIndex, iVertex, iIndex);

		// Text area part
		if (_bFull)
		{
			AddTriangle(5, 6+PinOffset, 7+PinOffset, pIndex, iVertex, iIndex);
			AddTriangle(7+PinOffset, 8+PinOffset, 5, pIndex, iVertex, iIndex);
			AddTriangle(8+PinOffset, 9+PinOffset, 10+PinOffset, pIndex, iVertex, iIndex);
			AddTriangle(8+PinOffset, 7+PinOffset, 10+PinOffset, pIndex, iVertex, iIndex);

			AddTriangle(9+PinOffset, 10+PinOffset, 4, pIndex, iVertex, iIndex);
			AddTriangle(10+PinOffset, 11+PinOffset, 4, pIndex, iVertex, iIndex);
			AddTriangle(12+PinOffset, 13+PinOffset, 14+PinOffset, pIndex, iVertex, iIndex);
			AddTriangle(12+PinOffset, 14+PinOffset, 15+PinOffset, pIndex, iVertex, iIndex);
		}
		
		// Color/alpha by Matthies
		int32 Alpha = int(0.86f * 255.0f);
		int32 Color = CPixel32((80/2),(100/2),(115/2));
		
		CVec3Dfp32 ScreenOffset(-(_PinSlopeWidth+_PinExtraWidth),-_PinSlopeWidth,0.0f);
		CVec3Dfp32 BasePos = _ScreenPos + ScreenOffset;//CVec3Dfp32(-5,0,5);//+_OrgPos;//_OrgPos * _Mat;
		// (0)
		CVec3Dfp32 Pos = BasePos + CVec3Dfp32(-(_Width + _ExtraWidth),-_Height,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (1)
		Pos = BasePos + CVec3Dfp32(-_ExtraWidth,-_Height,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (2)
		Pos = BasePos + CVec3Dfp32(-_ExtraWidth+_SlopeWidth,-_ExtraHeight,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (3)
		Pos = BasePos + CVec3Dfp32(0.0f,-_ExtraHeight,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (4)
		Pos = BasePos;
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (5)
		Pos = BasePos + CVec3Dfp32(-(_Width+_ExtraWidth),0.0f,0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);

		// Pin stuff
		const fp32 PinWidth = M_Fabs(ScreenOffset.k[0]) - _PinSlopeWidth;
//		const fp32 PinHeight = M_Fabs(ScreenOffset.k[1]);
		// (6)
		Pos = BasePos + CVec3Dfp32(0, -_BorderWidth, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (7)
		Pos = BasePos + CVec3Dfp32(PinWidth, -_BorderWidth, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// Intermediate (8)
		fp32 A = _BorderWidth*(1.0f/_SQRT2);
		Pos = BasePos + CVec3Dfp32(PinWidth+A, -A, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		// (9) (Old 11)
		Pos = BasePos + CVec3Dfp32(PinWidth, 0.0f, 0.0f);
		AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
		

		// TEST
		DoPinCircle(pVertexPos, pVertexColor, pIndex, iVertex, iIndex, _ScreenPos, _BorderWidth, 
			_PinCircleRadius, PinCircleSegs, Color, Alpha);

		if (_bFull)
		{
			// Border part
			Pos = BasePos + CVec3Dfp32(-(_Width + _ExtraWidth) + _BorderWidth,0.0f,0.0f);
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			Pos.k[1] += _InfoHeight;
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			Pos += CVec3Dfp32(-_BorderWidth,_BorderWidth,0.0f);
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			Pos.k[0] += _Width+_ExtraWidth;
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			Pos += CVec3Dfp32(-_BorderWidth,-_BorderWidth,0.0f);
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			Pos = BasePos;
			Pos.k[0] -= _BorderWidth;
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, Alpha);
			
			// Text area, uses already defined positions
			int32 AreaAlpha = int(0.45f * 255.0f);
			Pos = pVertexPos[6+PinOffset];
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, AreaAlpha);
			Pos = pVertexPos[11+PinOffset];
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, AreaAlpha);
			Pos = pVertexPos[10+PinOffset];
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, AreaAlpha);
			Pos = pVertexPos[7+PinOffset];
			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, Color, AreaAlpha);
		}

		CMat4Dfp32 Unit;
		Unit.Unit();
		CMat4Dfp32 *pMat = _pRenderParams->m_pVBM->Alloc_M4(Unit);
		pVB->Matrix_Set(pMat);
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(pVertexPos, NumVerts, true);
		pVB->Geometry_ColorArray(pVertexColor);

		// Render triangles
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->Render_IndexedTriangles(pIndex, NumTris);

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		//pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		//pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		//pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
		//Render_Surface(m_iSurfaceTopLeft, pVB, _pAnimState->m_AnimTime0);
		_pRenderParams->m_pVBM->AddVB(pVB);

		return false;
	}
#endif
	
	// Ah, very nice and ugly function, REMAKE REMAKE REDO!!!
	fp32 GetObjectScreenSize(const fp32 _Radius, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		/*CMat4Dfp32 IVMat;
		_VMat.InverseOrthogonal(IVMat);
		CVec3Dfp32 Dir = CVec3Dfp32::GetRow(IVMat, 0);
		_WMat.Multiply(_VMat, IVMat);
		CVec3Dfp32 Right;
		Dir.CrossProd(CVec3Dfp32(0,0,1),Right);
		CVec3Dfp32 ObjPos = CVec3Dfp32::GetMatrixRow(_WMat,3);
		CVec3Dfp32 ObjPos2 = ObjPos + Right * _Radius;
		ObjPos = ObjPos * IVMat;
		ObjPos2 = ObjPos2 * IVMat;
		return 2*(ObjPos2 - ObjPos).Length();*/
		CMat4Dfp32 Mat;
		_WMat.Multiply(_VMat, Mat);
		CVec3Dfp32 Pos(0.0f,0.0f,0.0f);
		Pos = Pos * Mat;
		return (1.33f*4.0f)*_Radius / Pos.k[2];
	}

	int TextFit(CRC_Font* _pFont, const wchar* _pStr, int _Width, bool _bWordWrap)
	{
		return _pFont->GetFit(_pFont->GetOriginalSize(), _pStr, _Width, _bWordWrap);
	}

	int IsControlCode(const wchar *_pStr, int _iPos)
	{
		int CodeLen = 0;

		if(_pStr[_iPos] == (uint8)'§')
		{
			CodeLen = 1;
			switch((char)_pStr[_iPos + 1])
			{
				case 'd':
				case 'D':
					CodeLen = 2;

				case 'a':
				case 'A':
					CodeLen = 3;
					break;

				case 'z':
				case 'Z':
					CodeLen = 4;
					break;

				case 'c':
				case 'C':
				case 'x':
				case 'X':
				case 'y':
				case 'Y':
					CodeLen = 5;
			}
		}

		return CodeLen;
	}

	int GetControlCodes(const wchar *_pStr, wchar *_pRes, int _ResLen)
	{
		if(_ResLen == 0)
			return 0;

		int iIndex = 0;

		int iLen = CStrBase::StrLen(_pStr);

		for(int i = 0; i < iLen; i++)
		{
			int CodeLen = IsControlCode(_pStr, i);
			if(CodeLen != 0)
			{
				if(iIndex + CodeLen + 1 >= _ResLen)
					return iIndex;

				for(int j = 0; j < CodeLen; j++)
					_pRes[iIndex++] = _pStr[i++];
				i -= 1;
			}
		}

		//Add terminator. Range checking for terminator already assured.
		_pRes[iIndex] = 0;

		return iIndex;
	}
	
	int Text_WordWrap(CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines)
	{
		int last = 0;
		int nLines = 0;
		wchar CtrlCodesBuf[512];
		int CtrlCodesBufLen = 0;
		
		while(last < _Len)
		{
			int n = _Len-last;
			if (n > 511)
			{
				ConOut("CMWnd_Text_WordWrap, Tried to draw a string longer than 511 chars");
				return nLines;
			}
			
			if(CtrlCodesBufLen > 0)
				//Copy all controlcodes to new line
				memcpy(&_ppLines[nLines][0], CtrlCodesBuf, CtrlCodesBufLen*sizeof(wchar));
			
			memcpy(&_ppLines[nLines][CtrlCodesBufLen], &_pStr[last], n*sizeof(wchar));
			_ppLines[nLines][n + CtrlCodesBufLen] = 0;
			
			//LogFile(CStrF("Ick       %d, %d, %d", last, _Len, n));
			int nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, true);
			//LogFile(CStrF("Bla   %d, %d, %d, %d", nFit, last, _Len, n));
			if (!nFit) nFit = TextFit(_pF, &_ppLines[nLines][0], _Width, false);
			if (!nFit) return nLines;
			_ppLines[nLines][nFit] = 0;
			
			for(int j = 0; j < nFit; j++)
				if(_ppLines[nLines][j] == '|')
				{
					_ppLines[nLines][j] = 0;
					nFit = j + 1;
					break;
				}
				
				last += nFit - CtrlCodesBufLen;
				
				if(last < _Len)
					CtrlCodesBufLen = GetControlCodes(_ppLines[nLines], CtrlCodesBuf, sizeof(CtrlCodesBuf));
				
				nLines++;
				if (nLines >= _MaxLines) return _MaxLines;
		}
		
		return nLines;
	}

	int32 GetTextLen(wchar* _pStr)
	{
		int32 i = 0;
		while (_pStr[i] != NULL)
			i++;

		return i;
	}

#ifndef USEV2
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_FocusFrame_Render, MAUTOSTRIP_VOID);
		MSCOPESHORT(CXR_Model_FocusFrame::Render);

		// Hmm, ok then, we want to draw a window around our object showing some useful hints
		CRC_Font *pFont = safe_cast<CRC_Font >((CReferenceCount *)_pAnimState->m_pContext);
		if(!pFont)
			return;

		CXR_VertexBuffer *pVBUseText = AllocVB();
		if(!pVBUseText)
			return;

		CMat4Dfp32 FMat;
		_WMat.Multiply(_VMat, FMat);

		CVec3Dfp32 ScreenPos = CVec3Dfp32(0,0,0) * FMat;
		const fp32 Z = 4.0f;
		const fp32 Distance = ScreenPos.k[2];
//		fp32 W = ScreenPos.k[2] * 1.0f/Z;
		const fp32 invW = Z / ScreenPos.k[2];
		ScreenPos.k[0] *= invW;
		ScreenPos.k[1] *= invW;
		ScreenPos.k[2] = Z;
		
		int32 Col = CPixel32((226/2),(224/2),(220/2),255);
		char* pFocusUseText = (char *)_pAnimState->m_Colors[0];
		wchar UseBuffer[1024];
		Localize_Str(pFocusUseText, UseBuffer, 1024);

		// Find object size as well if this crosses over textsize we need to increase it
		// probably set in the animstate.....
		fp32 ObjectWidth = GetObjectScreenSize(_pAnimState->m_AnimAttr1,_WMat,_VMat);
		//ConOut(CStrF("ObjWidth: %f",ObjectWidth));
		fp32 TextSize = 0.25f;
		CVec2Dfp32 V(TextSize, TextSize);

		fp32 TextWidth = pFont->GetWidth(TextSize, UseBuffer);
		if (TextWidth < ObjectWidth)
			TextWidth = ObjectWidth;

		fp32 BorderWidth = 0.375f;
		fp32 YOffset = BorderWidth + (TextWidth/2.0f);
	
		CVec3Dfp32 UseTextPos = ScreenPos + CVec3Dfp32(-TextWidth/(2.0f),-(YOffset-0.1),0.0f);
		pFont->Write(_pRenderParams->m_pVBM, pVBUseText, UseTextPos, CVec3Dfp32(1,0,0),CVec3Dfp32(0,1,0), UseBuffer, V, Col);
		
		
		CMat4Dfp32 Unit;
		Unit.Unit();
		CMat4Dfp32 *pMat = _pRenderParams->m_pVBM->Alloc_M4(Unit);
		if (pMat == NULL)
			return;

		// Print text..
		pVB->Matrix_Set(pMat);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		_pRenderParams->m_pVBM->AddVB(pVB);
		pVB->m_Priority += 0.001f;

		BuildFocusFrameV1Part1(_pAnimState, ScreenPos, TextWidth,
			YOffset, BorderWidth);
		BuildFocusFrameV1Part2(_pAnimState, ScreenPos, TextWidth,
			YOffset, BorderWidth);
		BuildFocusFrameV1Part3(_pAnimState, ScreenPos, TextWidth,
			YOffset-BorderWidth, BorderWidth);
	}
#else
	#define SELECTION_ISCHAR 1
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_FocusFrame_Render, MAUTOSTRIP_VOID);
		MSCOPESHORT(CXR_Model_FocusFrame::Render);

		// Hmm, ok then, we want to draw a window around our object showing some useful hints
		CRC_Font *pFont = safe_cast<CRC_Font >((CReferenceCount *)_pAnimState->m_pContext);
		if(!pFont)
			return;

		CMat4Dfp32 FMat;
		_WMat.Multiply(_VMat, FMat);
		
		int32 FocusFrameOffsetY = 0;//_pAnimState->m_SurfAnim0;

		CVec3Dfp32 ScreenPos = CVec3Dfp32(0,0,FocusFrameOffsetY) * FMat;
		const fp32 Z = 4.0f;
//		fp32 Distance = ScreenPos.k[2];
		const fp32 invW = Z / ScreenPos.k[2];
		ScreenPos.k[0] *= invW;
		ScreenPos.k[1] *= invW;
		ScreenPos.k[2] = Z;
		
		char moo = '§';
		int32 Col = CPixel32((226/2),(224/2),(220/2),255);
		char* pFocusUseText = (char *)_pAnimState->m_Data[0];
		// If the key doesn't exist, just return
		{
			bool bHasLoc = (pFocusUseText && 
				(pFocusUseText[0] == moo && pFocusUseText[1] == 'L'));
			int i = (bHasLoc ? 2 : 0);
			// Remove any extra '§'
			while(pFocusUseText[i] != '\0')
			{
				if (pFocusUseText[i] == moo)
				{
					pFocusUseText[i] = '\0';
					break;
				}
				i++;
			}
			
			if ((!bHasLoc && !Localize_KeyExists(pFocusUseText)) ||
				(bHasLoc && !Localize_KeyExists(pFocusUseText+2)))
				return;
		}

		wchar UseBuffer[1024];
		Localize_Str(pFocusUseText, UseBuffer, 1024);

		char* pFocusDescText = (char *)_pAnimState->m_Data[1];
		wchar DescBuffer[1024];
		// Check if description text should be used
		bool bFull = false; //(pFocusDescText[0] != '\0' && DescBuffer[0] != '\0');
		
		{
			bool bHasLoc = (pFocusDescText && 
				(pFocusDescText[0] == moo && pFocusDescText[1] == 'L'));
			if (bHasLoc && Localize_KeyExists(pFocusDescText+2))
			{
				Localize_Str(pFocusDescText, DescBuffer, 1024);
				bFull = (DescBuffer[0] != '\0');
			}
		}

		//ConOut(CStrF("ObjWidth: %f",ObjectWidth));
		fp32 TextSize = 0.25f;
		CVec2Dfp32 V(TextSize, TextSize);
		
		//CVec3Dfp32 ScreenOffset(-0.5f/*-ObjectWidth*/,-0.5f/*-ObjectWidth*/,0);
		const fp32 V2Height = 0.45f;
		const fp32 V2ExtraWidth = 0.5f;
		const fp32 V2ExtraHeight = 0.35f;
		const fp32 V2SlopeWidth = 0.1f;
		//const fp32 V2BorderWidth = 0.07f;
		const fp32 V2BorderWidth = PINBORDERWIDTH;
		const fp32 V2PinCircleRadius = PINCIRCLERADIUS;
		fp32 V2InfoHeight = 1.0f;
		const fp32 V2PinExtraWidth = 0.4f;
		const fp32 V2PinSlopeWidth = 0.4f;

		// With is controlled by usetext
		const fp32 UseTextWidth = pFont->GetWidth(TextSize, UseBuffer);

//		const fp32 BorderWidth = 0.375f;
//		fp32 YOffset = BorderWidth + (UseTextWidth*(1.0f/2.0f));

		int32 UseTextLen = GetTextLen(UseBuffer);
		// Ok, assume 4 vertices and 2 triangles per letter 
		int32 NeededTextVerts = UseTextLen * 4;
		int32 NeededTextTris = UseTextLen * 2;

		if (bFull)
		{
			int32 DescTextLen = GetTextLen(DescBuffer);
			NeededTextVerts += DescTextLen * 4;
			NeededTextTris += DescTextLen * 2;
		}

		// Allocate text buffers
		CVec3Dfp32* pVertexPos = NULL;
		CVec2Dfp32* pVertexTex = NULL;
		CPixel32* pVertexColor = NULL;
		uint16* pIndex = NULL;
		CXR_VertexBuffer* pVBText = GetVB(_pRenderParams, NeededTextVerts, NeededTextTris, pVertexPos, 
			pVertexColor, pIndex, pVertexTex);
		if (!pVBText || !pVertexPos || !pVertexColor || !pIndex || ! pVertexTex || !pVBText->m_pAttrib)
			return;

		CVec3Dfp32 UseTextPos = ScreenPos + CVec3Dfp32(0.07f-(UseTextWidth+V2PinExtraWidth+V2PinSlopeWidth+V2ExtraWidth),-(V2PinSlopeWidth+0.3f),0.0f);

		int32 nTextTris = pFont->Write(NeededTextVerts, pVertexPos, pVertexTex, pVertexColor,
			pIndex, Col, UseTextPos, CVec3Dfp32(1,0,0),CVec3Dfp32(0,1,0), UseBuffer, V);

		if (bFull)
		{
			CVec3Dfp32 DescTextPos = ScreenPos + CVec3Dfp32(0.07f-(UseTextWidth+V2PinExtraWidth+V2PinSlopeWidth+V2ExtraWidth),-(V2PinSlopeWidth-0.05f),0.0f);
			TextSize = 0.2f;
			V = CVec2Dfp32(TextSize,TextSize);

			wchar lDescBuffers[4][512];
			wchar* lpLines[4];
			for(int i = 0; i < 4; i++)
				lpLines[i] = &lDescBuffers[i][0];

			// Hmm, ok len is number of chars
			int32 Len = GetTextLen(DescBuffer);
			int32 nLines = Text_WordWrap(pFont, (int)(((UseTextWidth+V2PinExtraWidth) / TextSize) * pFont->GetOriginalSize()), DescBuffer, Len, lpLines, 4);
			const fp32 LineSpacing = 0.05f;

			// Calculate new info heigth
			V2InfoHeight = ((fp32)nLines) * (TextSize + LineSpacing) + 0.05f;
			// Alrighty then, we must create some vertexbuffers for the text to be in (hmm, shouldn't 
			// it be enough with only one vb?)
			for (int32 i = 0; i < nLines; i++)
			{
				const int32 NextIndex = nTextTris*2;
				const int32 nTris = pFont->Write(NeededTextVerts-NextIndex, &pVertexPos[NextIndex], 
					&pVertexTex[NextIndex], &pVertexColor[NextIndex], &pIndex[nTextTris*3], 
					Col, DescTextPos, CVec3Dfp32(1,0,0),CVec3Dfp32(0,1,0), lDescBuffers[i], V);

				DescTextPos.k[1] += TextSize + LineSpacing;

				// Offset index buffer
				for (int32 i = 0; i < nTris * 3; i++)
					pIndex[nTextTris*3+i] += NextIndex;

				nTextTris += nTris;
			}
		}

		// Print text..
		if (pFont->m_TextureID)
			pVBText->m_pAttrib->Attrib_TextureID(0, pFont->m_TextureID);
		else
			pVBText->m_pAttrib->Attrib_TextureID(0, pFont->m_spTC->GetTextureID(0));
		
		CMat4Dfp32 Unit;
		Unit.Unit();
		CMat4Dfp32 *pMatUse = _pRenderParams->m_pVBM->Alloc_M4(Unit);
		if (!pMatUse)
			return;

		pVBText->Matrix_Set(pMatUse);
		pVBText->Render_IndexedTriangles(pIndex, nTextTris);
		pVBText->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVBText->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pVBText->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVBText->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVBText->m_Priority += 0.001f;
		_pRenderParams->m_pVBM->AddVB(pVBText);
		
		BuildFocusFrameV2(_pRenderParams, _pAnimState, ScreenPos, UseTextWidth, V2Height, V2ExtraWidth, 
			V2ExtraHeight,V2SlopeWidth,V2InfoHeight, V2BorderWidth,V2PinCircleRadius, V2PinExtraWidth,V2PinSlopeWidth,bFull);
	}
#endif
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FocusFrame, CXR_Model_Custom);
