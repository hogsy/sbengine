/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			MWireContainer.h

	Copyright:		Starbreeze AB, 2002

	Contents:		CWireContainer, CDebugRenderContainer
\*____________________________________________________________________*/
#ifndef _INC_MWIRECONTAINER
#define _INC_MWIRECONTAINER

#include "MCC.h"

class CXR_Engine;
class CXR_VBManager;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWireContainer

	Comments:		Used for debug rendering
\*____________________________________________________________________*/
class CWireContainer : public CReferenceCount
{
	DECLARE_OPERATOR_NEW
	MRTC_DECLARE;

protected:
	class CWire
	{
	public:
		int m_bInUse : 1;
		int m_bFade : 1;
		fp32 m_Duration;
		fp32 m_Age;

		void Clear()
		{
			m_bInUse = 0;
			m_bFade = 0;
			m_Duration = 0;
			m_Age = 0;
		}

		CWire()
		{
			Clear();
		}
	};

	TArray<CVec3Dfp32> m_lV;
	TArray<CPixel32> m_lCol;
	TArray<CWire> m_lWires;
	CIDHeap m_IDHeap;
	int m_iWNext;

protected:
	dllvirtual int AllocWire();
	dllvirtual void FreeWire(int);

public:
	dllvirtual void Create(int _MaxWires);

	dllvirtual void RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderVertex(const CVec3Dfp32& _p, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderVector(const CVec3Dfp32& _p, const CVec3Dfp32& _v, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderMatrix(const CMat4Dfp32& _Mat, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderMatrix(const CMat4Dfp32& _Mat, fp32 _Duration = 1.0f, bool _bFade = true, CPixel32 _ColorX = 0xff7f0000, CPixel32 _ColorY = 0xff007f00, CPixel32 _ColorZ = 0xff00007f);
	dllvirtual void RenderQuaternion(const CVec3Dfp32& _p, const CQuatfp32& _Quat, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderAxisRot(const CVec3Dfp32& _p, const CAxisRotfp32& _AxisRot, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderAABB(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderAABB(const CBox3Dfp32& _Box, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderAABB(const CMat4Dfp32& _Transform, const CBox3Dfp32& _Box, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderOBB(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Extents, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void RenderSphere(const CMat4Dfp32& _Pos, fp32 _Radius, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);

	virtual void Refresh(fp64 _dTime);
	virtual void Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Transform);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CDebugRenderContainer

	Comments:		Extension of CWireContainer
\*____________________________________________________________________*/
class CDebugRenderContainer : public CWireContainer
{
	DECLARE_OPERATOR_NEW
	MRTC_DECLARE;

public:
	class CText : public CWire
	{
	public:
		CVec3Dfp32 m_Pos;
		CPixel32 m_Col;
		CFStr m_Str;
	};
	TArray<CText> m_lText;
	CIDHeap m_IDTHeap;
	int m_iTNext;
	CRC_Font* m_pFont;

public:
	dllvirtual void Create(int _MaxWires, int _MaxText, CRC_Font* _pFont);
	dllvirtual void RenderText(const CVec3Dfp32& _Pos, const char* _pText, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);

	virtual void Refresh(fp64 _dTime);
	virtual void Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Transform);
};


#endif // _INC_MWIRECONTAINER
