#include "PCH.h"
#include "MRenderCapture.h"
#include "../../MOS.h"

// -------------------------------------------------------------------
CCaptureOp::CCaptureOp()
{
	MAUTOSTRIP( CCaptureOp_ctor, MAUTOSTRIP_VOID );
	m_iData = 0;
	m_DataSize = 0;
	m_AllocSize = 0;
	m_Opcode = CAPTURE_OP_NOP;
}

CCaptureOp::CCaptureOp(int _Op)
{
	MAUTOSTRIP( CCaptureOp_ctor_2, MAUTOSTRIP_VOID );
	m_iData = 0;
	m_DataSize = 0;
	m_AllocSize = 0;
	m_Opcode = _Op;
}

void CCaptureOp::PreAlloc(int _Size, CCaptureBuffer* _pCB)
{
	MAUTOSTRIP( CCaptureOp_PreAlloc, MAUTOSTRIP_VOID );
	m_iData = _pCB->Alloc(_Size);
	m_AllocSize = _Size;
	m_DataSize = 0;
}

void CCaptureOp::SetSize(int _Size, CCaptureBuffer* _pCB)
{
	MAUTOSTRIP( CCaptureOp_SetSize, MAUTOSTRIP_VOID );
	m_iData = _pCB->Alloc(_Size);
	m_AllocSize = _Size;
	m_DataSize = _Size;
}

void CCaptureOp::AddData(const void* _pData, int _Size, CCaptureBuffer* _pCB)
{
	MAUTOSTRIP( CCaptureOp_AddData, MAUTOSTRIP_VOID );
	if (m_DataSize + _Size > m_AllocSize) 
		Error_static("CCaptureOp::AddData", CStrF("Not enough space allocated. (%d+%d/%d)", _Size, m_DataSize, m_AllocSize));

	memcpy(_pCB->GetPtr(m_iData + m_DataSize), _pData, _Size);
	m_DataSize += _Size;
}

void CCaptureOp::Read(CCFile* _pFile)
{
	MAUTOSTRIP( CCaptureOp_Read, MAUTOSTRIP_VOID );
	_pFile->ReadLE(m_Opcode);
	_pFile->ReadLE(m_DataSize);
	_pFile->ReadLE(m_iData);
}

void CCaptureOp::Write(CCFile* _pFile) const
{
	MAUTOSTRIP( CCaptureOp_Write, MAUTOSTRIP_VOID );
	_pFile->WriteLE(m_Opcode);
	_pFile->WriteLE(m_DataSize);
	_pFile->WriteLE(m_iData);
}

// -------------------------------------------------------------------
//  CCaptureBuffer
// -------------------------------------------------------------------
CCaptureBuffer::CCaptureBuffer()
{
	MAUTOSTRIP( CCaptureBuffer_ctor, MAUTOSTRIP_VOID );
	m_lOp.Clear();
#ifdef PLATFORM_CONSOLE
	m_lOp.SetGrow(65536);
	m_lHeap.SetLen(65536);
#else
	m_lOp.SetGrow(1024*1024);
	m_lHeap.SetLen(16*1024*1024);
#endif
	m_iAlloc = 0;
}

void CCaptureBuffer::Clear()
{
	MAUTOSTRIP( CCaptureBuffer_Clear, MAUTOSTRIP_VOID );
	m_lOp.QuickSetLen(0);
	m_lHeap.QuickSetLen(0);
	m_iAlloc = 0;
}

int CCaptureBuffer::Alloc(int _Size)
{
	MAUTOSTRIP( CCaptureBuffer_Alloc, 0 );
	if (m_iAlloc + _Size > m_lHeap.Len())
		m_lHeap.SetLen(Max(m_iAlloc + _Size, m_lHeap.Len()*2));

	m_iAlloc += _Size;
	return m_iAlloc - _Size;
}

uint8* CCaptureBuffer::GetPtr(int _iData)
{
	MAUTOSTRIP( CCaptureBuffer_GetPtr, NULL );
	return &m_lHeap[_iData];
}

void CCaptureBuffer::AddAttribute(CRC_Attributes& _Attr)
{
	MAUTOSTRIP( CCaptureBuffer_AddAttribute, MAUTOSTRIP_VOID );
	CCaptureOp Op(CAPTURE_OP_ATTRIBUTE);
	int Size = sizeof(_Attr) + sizeof(_Attr.m_nLights);
	if (_Attr.m_pLights && _Attr.m_nLights)
		Size += sizeof(CRC_Light)*_Attr.m_nLights;

	Op.PreAlloc(Size, this);
	Op.AddData(&_Attr, sizeof(_Attr), this);
	if (_Attr.m_pLights && _Attr.m_nLights)
	{
		Op.AddData(&_Attr.m_nLights, sizeof(_Attr.m_nLights), this);
		Op.AddData(_Attr.m_pLights, sizeof(CRC_Light) *  _Attr.m_nLights, this);
	}
	else
	{
		_Attr.m_nLights = 0;
		Op.AddData(&_Attr.m_nLights, sizeof(_Attr.m_nLights), this);
	}
	m_lOp.Add(Op);
}

void CCaptureBuffer::AddWire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _Color)
{
	MAUTOSTRIP( CCaptureBuffer_AddWire, MAUTOSTRIP_VOID );
	CCaptureOp Op(CAPTURE_OP_WIRE);
	Op.PreAlloc(sizeof(_v0)*2 + sizeof(_Color), this);
	Op.AddData((void*)&_v0, sizeof(_v0), this);
	Op.AddData((void*)&_v1, sizeof(_v1), this);
	Op.AddData(&_Color, sizeof(_Color), this);
	m_lOp.Add(Op);
}

void CCaptureBuffer::AddPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, 
								const CVec4Dfp32* _pSpec, 
//const fp32* _pFog, 
	const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color)
{
	MAUTOSTRIP( CCaptureBuffer_AddPolygon, MAUTOSTRIP_VOID );
	CCaptureOp Op(CAPTURE_OP_POLYGON);
	int Flags = 0;
	int Size = sizeof(Flags);	// For flags
	Size += sizeof(_nV);		// For nV
	Size += _nV*sizeof(CVec3Dfp32);
	if (_pCol) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 1; }
	if (_pSpec) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 2; }
	if (_pTV0) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 4; }
	if (_pTV1) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 8; }
	if (_pTV2) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 16; }
	if (_pTV3) { Size += _nV*sizeof(CVec4Dfp32); Flags |= 32; }
	if (_pN) { Size += _nV*sizeof(CVec3Dfp32); Flags |= 64; }
	Size += sizeof(_Color);
	Op.PreAlloc(Size, this);

	Op.AddData(&Flags, sizeof(Flags), this);
	Op.AddData(&_nV, sizeof(_nV), this);
	Op.AddData((const void*)_pV, _nV*sizeof(CVec3Dfp32), this);
	if (_pCol) Op.AddData((const void*)_pCol, _nV*sizeof(CVec4Dfp32), this);
	if (_pSpec) Op.AddData((const void*)_pSpec, _nV*sizeof(CVec4Dfp32), this);
	if (_pTV0) Op.AddData((const void*)_pTV0, _nV*sizeof(CVec4Dfp32), this);
	if (_pTV1) Op.AddData((const void*)_pTV1, _nV*sizeof(CVec4Dfp32), this);
	if (_pTV2) Op.AddData((const void*)_pTV2, _nV*sizeof(CVec4Dfp32), this);
	if (_pTV3) Op.AddData((const void*)_pTV3, _nV*sizeof(CVec4Dfp32), this);
	if (_pN) Op.AddData((const void*)_pN, _nV*sizeof(CVec3Dfp32), this);
	Op.AddData(&_Color, sizeof(_Color), this);
	m_lOp.Add(Op);
}

void CCaptureBuffer::AddTexture(int _ID, CImage* _pImg)
{
/*	CCaptureOp Op(CAPTURE_OP_TEXTURE);

	spCImage spImg;
	if ((CImagePalette*)_pImg->GetPalette())
	{
		// Avoid palette-hazzle...
		int bAlpha = _pImg->GetFormat() & IMAGE_FORMAT_ALPHA;
		spImg = _pImg->Convert((bAlpha) ? IMAGE_FORMAT_BGRA8 : IMAGE_FORMAT_BGR8, (bAlpha) ? IMAGE_CONVERT_RGBA : IMAGE_CONVERT_RGB);
		_pImg = spImg;
	}

#ifndef PLATFORM_DREAMCAST
	CCFile File;
	File.Open("RAM:Image.tga", CFILE_WRITE);
	_pImg->Write(&File);
	int ImgSize = File.Pos();
	spOp->PreAlloc(ImgSize + sizeof(_ID));
	spOp->AddData(&_ID, sizeof(_ID));
	File.Seek(0);
	File.Read(&spOp->m_pData[spOp->m_DataSize], ImgSize);
	spOp->m_DataSize += ImgSize;
	File.Close();
#endif

	m_lOp.Add(spOp);*/
}

void CCaptureBuffer::Write(CCFile* _pFile)
{
	MAUTOSTRIP( CCaptureBuffer_Write_2, MAUTOSTRIP_VOID );
	for(int i = 0; i < m_lOp.Len(); i++)
		m_lOp[i].Write(_pFile);
}

void CCaptureBuffer::Read(CCFile* _pFile)
{
	MAUTOSTRIP( CCaptureBuffer_Read_2, MAUTOSTRIP_VOID );
	for(int i = 0; i < m_lOp.Len(); i++)
		m_lOp[i].Read(_pFile);
}

void CCaptureBuffer::Write(const char* _pFileName)
{
	MAUTOSTRIP( CCaptureBuffer_Write, MAUTOSTRIP_VOID );
	CDataFile DFile;
	DFile.Create(_pFileName);
	DFile.BeginEntry("CAPTURE3");
	DFile.EndEntry(m_lOp.Len());
	DFile.BeginSubDir();
	{
		DFile.BeginEntry("HEAP");
		DFile.GetFile()->Write(m_lHeap.GetBasePtr(), m_lHeap.ListSize());
		DFile.EndEntry(m_lHeap.ListSize());

		DFile.BeginEntry("OPCODES");
		Write(DFile.GetFile());
		DFile.EndEntry(m_lOp.Len());
	}
	DFile.EndSubDir();
	DFile.Close();

	LogFile(CStrF("(CCaptureBuffer::Write) nOps %d", m_lOp.Len() ));
	LogFile(CStrF("(CCaptureBuffer::Write) Heap %d", m_lHeap.ListSize() ));
}

void CCaptureBuffer::Read(const char* _pFileName)
{
	MAUTOSTRIP( CCaptureBuffer_Read, MAUTOSTRIP_VOID );
	CDataFile DFile;
	DFile.Open(_pFileName);
	if (!DFile.GetNext("CAPTURE3")) Error("Read", "Invalid capture-file.");
	if (!DFile.GetSubDir()) Error("Read", "Invalid capture-file.");

	if (!DFile.GetNext("HEAP")) Error("Read", "Invalid capture-file.");
	m_lHeap.SetLen(DFile.GetUserData());
	DFile.GetFile()->Read(m_lHeap.GetBasePtr(), m_lHeap.ListSize());

	if (!DFile.GetNext("OPCODES")) Error("Read", "Invalid capture-file.");
	m_lOp.SetLen(DFile.GetUserData());
	Read(DFile.GetFile());

	if (!DFile.GetParent()) Error("Read", "Invalid capture-file.");

	DFile.Close();

	LogFile(CStrF("(CCaptureBuffer::Read) nOps %d", m_lOp.Len() ));
	LogFile(CStrF("(CCaptureBuffer::Read) Heap %d", m_lHeap.ListSize() ));
}

void CCaptureBuffer::Render(CRenderContext* _pRender, CRC_Viewport* _pVP)
{
	MAUTOSTRIP( CCaptureBuffer_Render, MAUTOSTRIP_VOID );
	CRC_Core* pRCC = safe_cast<CRC_Core>(_pRender);
	if (!pRCC) return;

	MSCOPE(CCaptureBuffer::Render, RENDERCAPTURE);

	for(int i = 0; i < m_lOp.Len(); i++)
	{
		const CCaptureOp* pOp = &m_lOp[i];
		const uint8* pData = GetPtr(pOp->m_iData);

		if (!pOp) continue;
		switch(pOp->m_Opcode)
		{
		case CAPTURE_OP_WIRE : 
			{
				if (pOp->m_DataSize < sizeof(CVec3Dfp32)*2 + sizeof(int)) break;

				const CVec3Dfp32* pV0 = (const CVec3Dfp32*) &pData[0];
				const CVec3Dfp32* pV1 = (const CVec3Dfp32*) &pData[sizeof(CVec3Dfp32)];
				const int* pCol = (const int*) &pData[sizeof(CVec3Dfp32)*2];
				_pRender->Render_Wire(*pV0, *pV1, *pCol);
			}
			break;
		case CAPTURE_OP_POLYGON : 
			{
				CRC_VertexBuffer VB;
				VB.Clear();
				const CVec3Dfp32* pV = NULL;
				const CVec4Dfp32* pCol = NULL;
				const CVec4Dfp32* pSpec = NULL;
//				const fp32* pFog = NULL;
				const CVec4Dfp32* pTV0 = NULL;
				const CVec4Dfp32* pTV1 = NULL;
				const CVec4Dfp32* pTV2 = NULL;
				const CVec4Dfp32* pTV3 = NULL;
				const CVec3Dfp32* pN = NULL;

				int Pos = 0;
				int Flags = *(const int*) &pData[Pos]; Pos += sizeof(Flags);
				int nV = *(const int*) &pData[Pos]; Pos += sizeof(nV);
				VB.m_pV = (CVec3Dfp32*) &pData[Pos]; Pos += sizeof(CVec3Dfp32) * nV;
				if (Flags & 1) { pCol = (CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 2) { pSpec = (const CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 4) { pTV0 = (const CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 8) { pTV1 = (const CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 16) { pTV2 = (const CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 32) { pTV3 = (const CVec4Dfp32*) &pData[Pos]; Pos += sizeof(CVec4Dfp32) * nV; };
				if (Flags & 64) { pN = (const CVec3Dfp32*) &pData[Pos]; Pos += sizeof(CVec3Dfp32) * nV; };
				int Color = *(const int*) &pData[Pos]; Pos += sizeof(Color);

				VB.m_nV = nV;
				_pRender->Geometry_VertexBuffer(VB, true);
				_pRender->Geometry_Color(Color);
				_pRender->Render_IndexedPolygon((uint16*)&g_IndexRamp16, nV);
//				pRCC->Internal_RenderPolygon(nV, pV, pN, pCol, pSpec, 
//pFog, 
//					pTV0, pTV1, pTV2, pTV3, Color);
			}
			break;
		case CAPTURE_OP_ATTRIBUTE : 
			{
				int Pos = 0;
				CRC_Attributes Attr;
				Attr = *(const CRC_Attributes*) &pData[Pos]; Pos += sizeof(CRC_Attributes);

				int nLights = *(int*) &pData[Pos]; Pos += sizeof(int);
				const CRC_Light* pLights = (const CRC_Light*) &pData[Pos]; Pos += sizeof(CRC_Light) * nLights;

				Attr.m_nLights = nLights;
				Attr.m_pLights = pLights;

				for(int i = 0; i < CRC_MAXTEXTURES; i++)
					Attr.m_TextureID[i] = 0;
//				pRCC->Attrib_SetRasterMode(&Attr, CRC_RASTERMODE_ADD);
//				Attr.m_RasterMode = CRC_RASTERMODE_ADD;
//				Attr.m_Flags &= ~CRC_FLAGS_ZWRITE;
				pRCC->Attrib_SetAbsolute(&Attr);
			}
			break;
		case CAPTURE_OP_TEXTURE : 
			{

			}
			break;
		}
	}
}

// -------------------------------------------------------------------
/*void operator *= (CVec2Dfp32& _V, const CMat4Dfp32& _M)
{
	fp32 x = _V.k[0];
	fp32 y = _V.k[1];
	_V.k[0] = x*_M.k[0][0] + y*_M.k[1][0] + _M.k[3][0];
	_V.k[1] = x*_M.k[0][1] + y*_M.k[1][1] + _M.k[3][1];
}
*/
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CRenderContextCapture, CRC_Core);

void CRenderContextCapture::Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, 
												   const CVec4Dfp32* _pSpec, 
//const fp32* _pFog, 
	const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color)
{
	MAUTOSTRIP( CRenderContextCapture_Internal_RenderPolygon, MAUTOSTRIP_VOID );
	CVec3Dfp32 Verts[128];
	CVec3Dfp32 N[128];
	CVec4Dfp32 TVerts0[128];
	CVec4Dfp32 TVerts1[128];
	CVec4Dfp32 TVerts2[128];
	CVec4Dfp32 TVerts3[128];
	for(int i = 0; i < _nV; i++)
	{
		Verts[i] = _pV[i];
		Verts[i] *= m_Transform[CRC_MATRIX_MODEL];
		if (_pN)
		{
			N[i] = _pN[i];
			N[i].MultiplyMatrix3x3(m_Transform[CRC_MATRIX_MODEL]);
		}
		if (_pTV0)
		{
			TVerts0[i] = _pTV0[i];
			TVerts0[i] *= m_Transform[CRC_MATRIX_TEXTURE0];
		}
		if (_pTV1)
		{
			TVerts1[i] = _pTV1[i];
			TVerts1[i] *= m_Transform[CRC_MATRIX_TEXTURE1];
		}
		if (_pTV2)
		{
			TVerts2[i] = _pTV2[i];
			TVerts2[i] *= m_Transform[CRC_MATRIX_TEXTURE2];
		}
		if (_pTV3)
		{
			TVerts3[i] = _pTV3[i];
			TVerts3[i] *= m_Transform[CRC_MATRIX_TEXTURE3];
		}
	}

	m_spCapture->AddPolygon(_nV, Verts, (_pN) ? N : NULL, _pCol, 
		_pSpec, 
//_pFog, 
		(_pTV0) ? TVerts0 : NULL, 
		(_pTV1) ? TVerts1 : NULL, 
		(_pTV2) ? TVerts2 : NULL, 
		(_pTV3) ? TVerts3 : NULL, 
		_Color);
};

CRenderContextCapture::CRenderContextCapture()
{
//	LogFile("(CRenderContextCapture::CRenderContextCapture)");
}

CRenderContextCapture::~CRenderContextCapture()
{
	MAUTOSTRIP( CRenderContextCapture_dtor, MAUTOSTRIP_VOID );
//	LogFile("(CRenderContextCapture::~CRenderContextCapture)");
	if (m_iTC >= 0) { m_pTC->RemoveRenderContext(m_iTC); m_iTC = -1; };
}

void CRenderContextCapture::Create(CObj* _pContext, const char* _pParams)
{
	MAUTOSTRIP( CRenderContextCapture_Create, MAUTOSTRIP_VOID );
	CRC_Core::Create(_pContext, _pParams);

	m_iTC = m_pTC->AddRenderContext(this);

	m_spCapture = MNew(CCaptureBuffer);
}

const char * CRenderContextCapture::GetRenderingStatus()
{
	MAUTOSTRIP( CRenderContextCapture_GetRenderingStatus, NULL );
	return "Capture";
}

void CRenderContextCapture::BeginScene(CRC_Viewport* _pVP)
{
	MAUTOSTRIP( CRenderContextCapture_BeginScene, MAUTOSTRIP_VOID );
	m_Transform[CRC_MATRIX_MODEL].Unit();
	m_Transform[CRC_MATRIX_TEXTURE].Unit();
	CRC_Core::BeginScene(_pVP);
	Attrib_Push();

	if (!m_spCapture) Error("BeginScene", "No capture buffer.");
	m_spCapture->Clear();
}

void CRenderContextCapture::EndScene()
{
	MAUTOSTRIP( CRenderContextCapture_EndScene, MAUTOSTRIP_VOID );
	CRC_Core::EndScene();
}

void CRenderContextCapture::Attrib_Set(CRC_Attributes* _pAttrib)
{
	MAUTOSTRIP( CRenderContextCapture_Attrib_Set, MAUTOSTRIP_VOID );
	m_spCapture->AddAttribute(*_pAttrib);
}

void CRenderContextCapture::Attrib_SetAbsolute(CRC_Attributes* _pAttrib)
{
	MAUTOSTRIP( CRenderContextCapture_Attrib_SetAbsolute, MAUTOSTRIP_VOID );
	Attrib_Set(_pAttrib);
}

void CRenderContextCapture::Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix)
{
	MAUTOSTRIP( CRenderContextCapture_Matrix_SetRender, MAUTOSTRIP_VOID );
	// Don't call super, it's pure virtual.
	if (_pMatrix)
		m_Transform[_iMode] = *_pMatrix;
	else
		m_Transform[_iMode].Unit();
}

CVec4Dfp32 ConvertColorTo4f(uint32 _Col)
{
	CVec4Dfp32 c;
	c.k[0] = fp32((_Col >> 16) & 0xff) / 255.0f;
	c.k[1] = fp32((_Col >> 8) & 0xff) / 255.0f;
	c.k[2] = fp32((_Col >> 0) & 0xff) / 255.0f;
	c.k[3] = fp32((_Col >> 24) & 0xff) / 255.0f;
	return c;
}

void CRenderContextCapture::Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
{
	MAUTOSTRIP( CRenderContextCapture_Render_IndexedTriangles, MAUTOSTRIP_VOID );
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	if (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		Internal_IndexedTriangles2Wires(_pTriVertIndices, _nTriangles);
		return;
	}

/*	int nV = Geometry_GetVertexCount(_pTriVertIndices, _nTriangles*3);
	if (Clip_InitVertexMasks(nV, m_Geom.m_pV, NULL)) return;
	CRC_Core::Render_IndexedTriangles(_pTriVertIndices, _nTriangles);*/

	CVec3Dfp32 lV[3];
	CVec3Dfp32 lN[3];
	CVec4Dfp32 lC[3];

	CVec3Dfp32* pN = (m_Geom.m_pN) ? lN : NULL;
	CVec4Dfp32* pC = (m_Geom.m_pCol) ? lC : NULL;


	for(int i = 0; i < _nTriangles; i++)
	{
		uint16 iv0 = _pTriVertIndices[i*3 + 0];
		uint16 iv1 = _pTriVertIndices[i*3 + 1];
		uint16 iv2 = _pTriVertIndices[i*3 + 2];
		lV[0] = m_Geom.m_pV[iv0];
		lV[1] = m_Geom.m_pV[iv1];
		lV[2] = m_Geom.m_pV[iv2];
		if (pN)
		{
			lN[0] = m_Geom.m_pN[iv0];
			lN[1] = m_Geom.m_pN[iv1];
			lN[2] = m_Geom.m_pN[iv2];
		}
		if (pC)
		{
			lC[0] = ConvertColorTo4f(m_Geom.m_pCol[iv0]);
			lC[1] = ConvertColorTo4f(m_Geom.m_pCol[iv0]);
			lC[2] = ConvertColorTo4f(m_Geom.m_pCol[iv0]);
		}

		m_spCapture->AddPolygon(3, lV, pN, pC, NULL, NULL, NULL, NULL, NULL, m_GeomColor);
	}
}

void CRenderContextCapture::Render_IndexedWires(uint16* _pIndices, int _nIndices)
{
	MAUTOSTRIP( CRenderContextCapture_Render_IndexedWire, MAUTOSTRIP_VOID );
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	if (!m_Geom.m_pV) return;
	_nIndices >>= 1;
	for(int i = 0; i < _nIndices; i++)
	{
		int iv0 = _pIndices[i*2];
		int iv1 = _pIndices[i*2 + 1];
		if (iv0 >= m_Geom.m_nV) continue;
		if (iv1 >= m_Geom.m_nV) continue;

		CVec3Dfp32 v0(m_Geom.m_pV[iv0]);
		CVec3Dfp32 v1(m_Geom.m_pV[iv1]);
		v0 *= m_Transform[CRC_MATRIX_MODEL];
		v1 *= m_Transform[CRC_MATRIX_MODEL];

		if (m_Geom.m_pCol)
			m_spCapture->AddWire(v0, v1, m_Geom.m_pCol[iv0]);
		else
			m_spCapture->AddWire(v0, v1, m_GeomColor);
	}
}

void CRenderContextCapture::Texture_PrecacheFlush( )
{
}

void CRenderContextCapture::Texture_PrecacheBegin( int _Count )
{
}

void CRenderContextCapture::Texture_PrecacheEnd()
{
}

void CRenderContextCapture::Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
{
	MAUTOSTRIP( CRenderContextCapture_Render_IndexedPrimitives, MAUTOSTRIP_VOID );
/*	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();

	if (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		Internal_IndexedPrimitives2Wires(_pPrimStream, _StreamLen);
		return;
	}

	int nV = Geometry_GetVertexCount_Primitives(_pPrimStream, _StreamLen);
	if (Clip_InitVertexMasks(nV, m_Geom.m_pV, NULL)) return;
	CRC_Core::Render_IndexedPrimitives(_pPrimStream, _StreamLen);*/

	uint16 lTriIndices[1024*3];
	CRCPrimStreamIterator StreamIterate(_pPrimStream, _StreamLen);

	while(StreamIterate.IsValid())
	{
		int nTriIndices = 1024*3;
		bool bDone = Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
		if (nTriIndices)
		{
			int nTri = nTriIndices/3;
			Render_IndexedTriangles(lTriIndices, nTri);
		}

		if (bDone) break;
	}
}

void CRenderContextCapture::Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color)
{
	MAUTOSTRIP( CRenderContextCapture_Render_Wire, MAUTOSTRIP_VOID );
	CVec3Dfp32 v0(_v0);
	CVec3Dfp32 v1(_v1);
	v0 *= m_Transform[CRC_MATRIX_MODEL];
	v1 *= m_Transform[CRC_MATRIX_MODEL];
	m_spCapture->AddWire(v0, v1, _Color);
}

void CRenderContextCapture::Render_VB(int _VBID)
{
	// TODO: Implement!

}

/*
*/
void CRenderContextCapture::Render_SortBuffer(fp32 _ZMin, fp32 _ZMax, bool _bFront2Back)
{
}

/*
*/
void CRenderContextCapture::Geometry_VertexBuffer( const CRC_VertexBuffer& _VB, int _bAllUsed )
{
}

/*
*/
void CRenderContextCapture::Geometry_VertexBuffer( int _VBID, int _bAllUsed )
{
}

/*
*/
void CRenderContextCapture::Render_VertexBuffer(int _VBID, int _bAllUsed)
{
}
