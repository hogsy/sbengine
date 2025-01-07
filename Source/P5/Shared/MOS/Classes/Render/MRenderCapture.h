#ifndef _INC_MRenderCapture
#define _INC_MRenderCapture

#include "../../MSystem/Raster/MRCCore.h"
#include "../../MSystem/Raster/MTextureContainers.h"
#include "../../MOS.h"

// -------------------------------------------------------------------
#define CAPTURE_OP_NOP			0
#define CAPTURE_OP_WIRE			1
#define CAPTURE_OP_POLYGON		2
#define CAPTURE_OP_ATTRIBUTE	3
#define CAPTURE_OP_TEXTURE		4

// -------------------------------------------------------------------
class CCaptureOp
{
public:
	uint32 m_Opcode;
	uint32 m_DataSize;
	uint32 m_AllocSize;
	uint32 m_iData;

	CCaptureOp();
	CCaptureOp(int _Op);
	void PreAlloc(int _Size, class CCaptureBuffer*);
	void SetSize(int _Size, class CCaptureBuffer*);
	void AddData(const void* _pData, int _Size, class CCaptureBuffer*);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};

// -------------------------------------------------------------------
class CCaptureBuffer : public CReferenceCount
{
	int m_iAlloc;
	TArray<uint8> m_lHeap;
	TArray<CCaptureOp> m_lOp;

public:
	CCaptureBuffer();
	virtual void Clear();

	int Alloc(int _Size);
	uint8* GetPtr(int _iData);

	void AddAttribute(CRC_Attributes& _Attr);
	void AddWire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _Color);
	void AddPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, 
		const CVec4Dfp32* _pSpec, 
//const fp32* _pFog, 
		const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color);
	void AddTexture(int _ID, CImage* _pImg);

	void Write(CCFile* _pFile);
	void Read(CCFile* _pFile);

	virtual void Write(const char* _pFileName);
	virtual void Read(const char* _pFileName);

	virtual void Render(CRenderContext* _pRender, CRC_Viewport* _pVP);
};

typedef TPtr<CCaptureBuffer> spCCaptureBuffer;

// -------------------------------------------------------------------
/*class CCaptureRender : public CRC_Core
{
	spCCaptureBuffer m_spCapture;
public:
	void Create(spCCaptureBuffer _spCapture);
	void Render(CRenderContext* _pRender, CRC_Viewport* _pVP);
};*/

// -------------------------------------------------------------------
//  CRenderContextCapture
// -------------------------------------------------------------------
class CRenderContextCapture : public CRC_Core
{
	MRTC_DECLARE;

protected:
	CMat4Dfp32 m_Transform[CRC_MATRIXSTACKS];
	spCCaptureBuffer m_spCapture;
	spCTextureContainer_Plain m_spTCapture;

	virtual void BS_RenderTriangle(int _iTriangle) {};

	virtual void Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol = NULL, 
		const CVec4Dfp32* _pSpec = NULL, 
//const fp32* _pFog = NULL,
		const CVec4Dfp32* _pTV0 = NULL, const CVec4Dfp32* _pTV1 = NULL, const CVec4Dfp32* _pTV2 = NULL, const CVec4Dfp32* _pTV3 = NULL, int _Color = 0xffffffff);

public:
	CRenderContextCapture();
	~CRenderContextCapture();


	virtual CDisplayContext *GetDC() {return NULL;};

	virtual int Texture_GetBackBufferTextureID() {return 0;}
	virtual int Texture_GetFrontBufferTextureID() {return 0;}
	virtual int Texture_GetZBufferTextureID() {return 0;}
	virtual int Geometry_GetVBSize(int _VBID) {return 0;}

	virtual void Create(CObj* _pContext, const char* _pParams);
	virtual spCCaptureBuffer GetCaptureBuffer() { return m_spCapture; };

	virtual void Flip_SetInterval(int _nFrames){};

	virtual const char * GetRenderingStatus();
	virtual void BeginScene(CRC_Viewport* _pVP);
	virtual void EndScene();

	virtual void Texture_PrecacheFlush();
	virtual void Texture_PrecacheBegin( int _Count);
	virtual void Texture_PrecacheEnd();

	virtual void Attrib_Set(CRC_Attributes* _pAttrib);
	virtual void Attrib_SetAbsolute(CRC_Attributes* _pAttrib);
	virtual void Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix);

	// Rendering
	virtual void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles);
	virtual void Render_IndexedWires(uint16* _pIndices, int _nIndices);
 	virtual void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen);

	virtual void Geometry_VertexBuffer(const CRC_VertexBuffer& _VB, int _bAllUsed);
	virtual void Geometry_VertexBuffer(int _VBID, int _bAllUsed);
	virtual void Render_VertexBuffer(int _VBID, int _bAllUsed);

	virtual void Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color);

	virtual void Render_VB(int _VBID);

	virtual void Render_SortBuffer(fp32 _ZMin = 0.0f, fp32 _ZMax = 100000.0f, bool _bFront2Back = false);

	virtual void Register(CScriptRegisterContext& _RegContext) {}
};

typedef TPtr<CRenderContextCapture> spCRenderContextCapture;

#endif


