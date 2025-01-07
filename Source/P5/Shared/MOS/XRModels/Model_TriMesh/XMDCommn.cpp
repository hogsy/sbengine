#include "PCH.h"

#include "XMDCommn.h"
#include "MFloat.h"
#include "WTriMesh.h"
#include "../../XR/XRAnim.h"

#ifndef PLATFORM_CONSOLE
# include "../../Classes/Render/MWireContainer.h"
#endif

//#pragma optimize("",off)
//#pragma inline_depth(0)

#define	DEBUGCOLOR_OPENEDGE CPixel32(255, 0, 0, 255)

enum
{
	LOGLEVEL_BASE = 1,
	LOGLEVEL_DEBUG = 2,
	LOGLEVEL_WARNING = 4,
	LOGLEVEL_ERRORS = 8,
	LOGLEVEL_SEVEREERROR = 16,
};

int g_LogFileLevel = (LOGLEVEL_SEVEREERROR | LOGLEVEL_ERRORS | LOGLEVEL_WARNING | LOGLEVEL_BASE);

static void ModelLogFile(int LogLevel, CStr _String)
{
	const char* pTag = "MC: ";
	if(!(LogLevel & g_LogFileLevel))
		pTag = "";

	const char* ErrorCodes[] =
	{
		"%s%s",
		"%sWARNING: %s",
		"%sERROR: %s",
		"%sSEVERE ERROR: %s",
		NULL
	};

	const char* pError = NULL;

	if(LogLevel & LOGLEVEL_SEVEREERROR)
		pError = ErrorCodes[3];
	else if(LogLevel & LOGLEVEL_ERRORS)
		pError = ErrorCodes[2];
	else if(LogLevel & LOGLEVEL_WARNING)
		pError = ErrorCodes[1];
	else
		pError = ErrorCodes[0];

	LogFile(CStrF(pError, pTag, _String.GetStr()));
}


#define ModelError(St) ModelLogFile(LOGLEVEL_ERRORS, St)
#define ModelWarning(St) ModelLogFile(LOGLEVEL_WARNING, St)
#define ModelLog(St) ModelLogFile(LOGLEVEL_BASE, St)
#define ModelDebug(St) ModelLogFile(LOGLEVEL_DEBUG, St)
#define ModelNote(St) ModelLogFile(LOGLEVEL_BASE, St)

//#define ModelLog(St) LogFile(CStr("MC: ") + St)
#define Model_SetProgressText(St) { MRTC_SetProgressText(St); ModelLog(St); }
#define Model_InitProgressCount(i, St) { MRTC_InitProgressCount(i, St); ModelLog(St); }


// -------------------------------------------------------------------
void CTM_BDVertexInfo::Read(CCFile* _pF)
{
	MAUTOSTRIP(CTM_BDVertexInfo_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_Flags);
	_pF->ReadLE(m_nBones);
	_pF->ReadLE(m_iBoneInfluence);
}

#ifndef PLATFORM_CONSOLE
void CTM_BDVertexInfo::Write(CCFile* _pF)
{
	MAUTOSTRIP(CTM_BDVertexInfo_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_Flags);
	_pF->WriteLE(m_nBones);
	_pF->WriteLE(m_iBoneInfluence);
}
#endif

#ifndef CPU_LITTLEENDIAN
void CTM_BDVertexInfo::SwapLE()
{
	MAUTOSTRIP(CTM_BDVertexInfo_SwapLE, MAUTOSTRIP_VOID);
	::SwapLE(m_Flags);
	::SwapLE(m_nBones);
	::SwapLE(m_iBoneInfluence);
}
#endif

// -------------------------------------------------------------------
void CTM_BDInfluence::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CTM_BDInfluence_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0 :
		{
			_pF->ReadLE(m_iBone);
			_pF->ReadLE(m_Influence);
		}
		break;

	case CTM_BDINFLUENCE_VERSION :
		{
			uint8 iBone;
			_pF->ReadLE(iBone);
			m_iBone = iBone;
			uint8 Influence;
			_pF->ReadLE(Influence);
			m_Influence = fp32(Influence) / 255.0f;
		}
		break;
	default :
		Error_static("CTM_BDInfluence::Read", CStrF("Unsupported version %.4", _Version))
	}
}

void CTM_BDInfluence::ReadArray(CCFile* _pF, int _Version, CTM_BDInfluence* _pBDI, int _nBDI)
{
	MAUTOSTRIP(CTM_BDInfluence_ReadArray, MAUTOSTRIP_VOID);

	if (_Version >= 0x0200)
	{
		int v = 0;
		while(v < _nBDI)
		{
			const int nVertPerRead = 128;
			uint8 VertInt[nVertPerRead][2];
			int nVRead = MinMT(nVertPerRead, _nBDI-v);

			_pF->ReadLE((uint8*)VertInt, nVRead*2);

			for(int i = 0; i < nVRead; i++)
			{
				_pBDI[v].m_iBone = VertInt[i][0];
				_pBDI[v].m_Influence = fp32(VertInt[i][1]) / 255.0f;
				v++;
			}
		}
	}
	else
	{
		for(int i = 0; i < _nBDI; i++)
			_pBDI[i].Read(_pF, _Version);
	}
}


#ifndef PLATFORM_CONSOLE
void CTM_BDInfluence::Write(CCFile* _pF)
{
	MAUTOSTRIP(CTM_BDInfluence_Write, MAUTOSTRIP_VOID);
	uint8 iBone = m_iBone;
	_pF->WriteLE(iBone);
	uint8 Influence = RoundToInt(m_Influence * 255.0f);
	_pF->WriteLE(Influence);
}
#endif

// -------------------------------------------------------------------
/*
void CTM_BDVertexGroup::Read(CCFile* _pF)
{
	MAUTOSTRIP(CTM_BDVertexGroup_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iBaseVertex);
	_pF->ReadLE(m_nVertices);
	_pF->ReadLE(m_nBones);
	if (m_nBones >= CTM_MAXVERTEXGROUPBONES) Error_static("CTM_BDVertexGroup::Read", "Invalid bone-count.");
	for(int i = 0; i < m_nBones; i++) m_BoneInfluence[i].Read(_pF);
}

void CTM_BDVertexGroup::Write(CCFile* _pF)
{
	MAUTOSTRIP(CTM_BDVertexGroup_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iBaseVertex);
	_pF->WriteLE(m_nVertices);
	_pF->WriteLE(m_nBones);
	for(int i = 0; i < m_nBones; i++) m_BoneInfluence[i].Write(_pF);
}
*/
// -------------------------------------------------------------------
template <class T>
void CopyVertices(const TArray<T>& _Src, TArray<T>& _Dest, const int32* _piV, int _nV)
{
	for(int i = 0; i < _nV; i++)
		_Dest[i] = _Src[_piV[i]];
}

template<class T>
void CopyVertices(const T* _pSrc, T* _pDst, const int32* _piV, int _nV)
{
	for(int i = 0; i < _nV; i++)
		_pDst[i] = _pSrc[_piV[i]];
}

#ifndef PLATFORM_CONSOLE
TPtr<CTM_VertexFrame> CTM_VertexFrame::Duplicate(const int32* _piV, int _nV)
{
	spCTM_VertexFrame spNew = MNew(CTM_VertexFrame);
	if (!spNew)
		MemError("Duplicate");

	spNew->m_BoundRadius = m_BoundRadius;
	spNew->m_BoundBox = m_BoundBox;
//	spNew->m_Scale = m_Scale;
	spNew->m_Translate = m_Translate;

	if (!_piV)
	{
		spNew->m_lVertices.Add(&m_lVertices);
		spNew->m_lNormals.Add(&m_lNormals);
		spNew->m_lTangentU.Add(&m_lTangentU);
		spNew->m_lTangentV.Add(&m_lTangentV);
	}
	else
	{
		spNew->m_lVertices.SetLen(_nV);
//		CopyVertices(m_lVertices.GetBasePtr(), spNew->m_lVertices.GetBasePtr(), _piV, _nV);
		CopyVertices(m_lVertices, spNew->m_lVertices, _piV, _nV);
		if (m_lNormals.Len())
		{
			spNew->m_lNormals.SetLen(_nV);
//			CopyVertices(m_lNormals.GetBasePtr(), spNew->m_lNormals.GetBasePtr(), _piV, _nV);
			CopyVertices(m_lNormals, spNew->m_lNormals, _piV, _nV);
		}
		if (m_lTangentU.Len())
		{
			spNew->m_lTangentU.SetLen(_nV);
//			CopyVertices(m_lTangentU.GetBasePtr(), spNew->m_lTangentU.GetBasePtr(), _piV, _nV);
			CopyVertices(m_lTangentU, spNew->m_lTangentU, _piV, _nV);
		}
		if (m_lTangentV.Len())
		{
			spNew->m_lTangentV.SetLen(_nV);
//			CopyVertices(m_lTangentV.GetBasePtr(), spNew->m_lTangentV.GetBasePtr(), _piV, _nV);
			CopyVertices(m_lTangentV, spNew->m_lTangentV, _piV, _nV);
		}
	}

	return spNew;
}
#endif

void CTM_VertexFrame::OldRead(CCFile* _pF)
{
	MAUTOSTRIP(CTM_VertexFrame_OldRead, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_BoundRadius);
	m_BoundBox.m_Min.Read(_pF);
	m_BoundBox.m_Max.Read(_pF);
//	_pF->ReadLE(m_Scale);
	fp32 TmpScale;
	_pF->ReadLE(TmpScale);
	m_Translate.Read(_pF);

	//Note: Bounding boxes was invalid in many old models. Now it is calculated. -JA
	m_BoundBox.m_Min = _FP32_MAX;
	m_BoundBox.m_Max = -_FP32_MAX;

	int32 nV;
	_pF->ReadLE(nV);
	m_lVertices.SetLen(nV);
	m_lNormals.SetLen(nV);
	for(int v = 0; v < nV; v++)
	{
		m_lVertices[v].Read(_pF);
		m_lNormals[v].Read(_pF);

		m_BoundBox.m_Min = CVec3Dfp32(Min(m_BoundBox.m_Min[0], m_lVertices[v][0]),
									 Min(m_BoundBox.m_Min[1], m_lVertices[v][1]), 
									 Min(m_BoundBox.m_Min[2], m_lVertices[v][2]));
		m_BoundBox.m_Max = CVec3Dfp32(Max(m_BoundBox.m_Max[0], m_lVertices[v][0]),
									 Max(m_BoundBox.m_Max[1], m_lVertices[v][1]), 
									 Max(m_BoundBox.m_Max[2], m_lVertices[v][2]));
	}
}

void CTM_VertexFrame::Read(CCFile* _pF, const CBox3Dfp32 &_BoundBox, bool _bDelayLoad)
{
	MAUTOSTRIP(CTM_VertexFrame_Read, MAUTOSTRIP_VOID);
	int32 Version;
	_pF->ReadLE(Version);
	m_bFileTangents = 0;
	switch(Version)
	{
	case 0x0200 :
		{
			_pF->ReadLE(m_BoundRadius);
			m_BoundBox.m_Min.Read(_pF);
			m_BoundBox.m_Max.Read(_pF);
			fp32 TmpScale;
			_pF->ReadLE(TmpScale);
			m_Translate.Read(_pF);
			m_Duration = 1.0f; // Added by Mondelore.

			int32 nV;
			_pF->ReadLE(nV);
			m_lVertices.SetLen(nV);
			m_lNormals.SetLen(nV);
			{ for(int v = 0; v < nV; v++) m_lVertices[v].Read(_pF); }
			{ for(int v = 0; v < nV; v++) m_lNormals[v].Read(_pF); }
		}
		break;

	case 0x0201 :
		{
			_pF->ReadLE(m_BoundRadius);
			m_BoundBox.m_Min.Read(_pF);
			m_BoundBox.m_Max.Read(_pF);
			fp32 TmpScale;
			_pF->ReadLE(TmpScale);
			m_Translate.Read(_pF);
			_pF->ReadLE(m_Duration);

			int32 nV;
			_pF->ReadLE(nV);
			m_lVertices.SetLen(nV);
			m_lNormals.SetLen(nV);
			{ for(int v = 0; v < nV; v++) m_lVertices[v].Read(_pF); }
			{ for(int v = 0; v < nV; v++) m_lNormals[v].Read(_pF); }
		}
		break;

	case 0x0202 :
	case 0x0203 :
		{
			_pF->ReadLE(m_BoundRadius);
			m_BoundBox.m_Min.Read(_pF);
			m_BoundBox.m_Max.Read(_pF);
			if (Version == 0x0202)
			{
				fp32 TmpScale;
				_pF->ReadLE(TmpScale);
			}
			m_Translate.Read(_pF);
			_pF->ReadLE(m_Duration);

			int32 nV;
			_pF->ReadLE(nV);
			m_lVertices.SetLen(nV);
			m_lNormals.SetLen(nV);
			{
				CVec3Dfp32 Scale;
				m_BoundBox.m_Max.Sub(m_BoundBox.m_Min, Scale);
				Scale *= 1.0f / 65535;

				CVec3Dfp32* pV = m_lVertices.GetBasePtr();

				int v = 0;
				while(v < nV)
				{
					const int nVertPerRead = 128;
					uint16 VertInt[nVertPerRead][3];
					int nVRead = MinMT(nVertPerRead, nV-v);

					_pF->ReadLE((uint16*)VertInt, nVRead*3);

					for(int i = 0; i < nVRead; i++)
					{
						pV[v][0] = fp32(VertInt[i][0])*Scale[0] + m_BoundBox.m_Min[0];
						pV[v][1] = fp32(VertInt[i][1])*Scale[1] + m_BoundBox.m_Min[1];
						pV[v][2] = fp32(VertInt[i][2])*Scale[2] + m_BoundBox.m_Min[2];
						v++;
					}
				}
			}

			{ 
				CVec3Dfp32* pN = m_lNormals.GetBasePtr();

				int v = 0;
				while(v < nV)
				{
					const int nVertPerRead = 128;
					int8 VertInt[nVertPerRead][3];
					int nVRead = MinMT(nVertPerRead, nV-v);

					_pF->ReadLE((int8*)VertInt, nVRead*3);

					for(int i = 0; i < nVRead; i++)
					{
						pN[v][0] = fp32(VertInt[i][0])/127.0f;
						pN[v][1] = fp32(VertInt[i][1])/127.0f;
						pN[v][2] = fp32(VertInt[i][2])/127.0f;
						pN[v].Normalize();
						v++;
					}
				}
			}
		}
		break;

	case 0x0204:
	case 0x0205:
		{
			_pF->ReadLE(m_BoundRadius);
			m_BoundBox.m_Min.Read(_pF);
			m_BoundBox.m_Max.Read(_pF);
			m_Translate.Read(_pF);
			_pF->ReadLE(m_Duration);

			M_ASSERT(_pF->Pos() < 0x4FFFFFFF,"Vertexframe file offset out-of-bounds (30 bit)");
			m_FilePos = _pF->Pos();
			if (_bDelayLoad)
			{
				int32 nV;
				_pF->ReadLE(nV);
				m_bHasVertices = nV > 0;
				m_bFileTangents = (Version == 0x0205);
				_pF->RelSeek(nV * 6 + nV * 3 + (Version == 0x0205) ? nV * 6 : 0);
			}
			else
			{
				ReadVertices(_pF, _BoundBox);
				ReadNormals(_pF);
				if( Version == 0x0205 )
				{
					ReadTangents(_pF);
				}
			}
		}
		break;

	default :
		Error("Read", CStrF("Unsupported vertex-frame version %.4x", Version));
	}
}

void CTM_VertexFrame::ReadVertices(CCFile* _pF, const CBox3Dfp32 &_BoundBox)
{
	if (!m_FilePos)
		return;

	_pF->Seek(m_FilePos);

	int32 nV;
	_pF->ReadLE(nV);
	m_lVertices.SetLen(nV);
	{
		CVec3Dfp32 Scale;
		_BoundBox.m_Max.Sub(_BoundBox.m_Min, Scale);
		Scale *= 1.0f / 65535;

		CVec3Dfp32* pV = m_lVertices.GetBasePtr();

		int v = 0;
		while(v < nV)
		{
			const int nVertPerRead = 128;
			uint16 VertInt[nVertPerRead][3];
			int nVRead = MinMT(nVertPerRead, nV-v);

			_pF->ReadLE((uint16*)VertInt, nVRead*3);

			for(int i = 0; i < nVRead; i++)
			{
				pV[v][0] = fp32(VertInt[i][0])*Scale[0] + _BoundBox.m_Min[0];
				pV[v][1] = fp32(VertInt[i][1])*Scale[1] + _BoundBox.m_Min[1];
				pV[v][2] = fp32(VertInt[i][2])*Scale[2] + _BoundBox.m_Min[2];
				v++;
			}
		}
	}

	m_bHasVertices = nV > 0;

}

void CTM_VertexFrame::ReadNormals(CCFile* _pF)
{
	if (!m_bHasVertices || !m_FilePos)
		return;

	_pF->Seek(m_FilePos);
	int32 nV;
	_pF->ReadLE(nV);
	_pF->RelSeek(nV * 6);
	m_lNormals.SetLen(nV);
	{ 
		CVec3Dfp32* pN = m_lNormals.GetBasePtr();

		int v = 0;
		while(v < nV)
		{
			const int nVertPerRead = 128;
			int8 VertInt[nVertPerRead][3];
			int nVRead = MinMT(nVertPerRead, nV-v);

			_pF->ReadLE((int8*)VertInt, nVRead*3);

			for(int i = 0; i < nVRead; i++)
			{
				CVec3Dfp32 Tmp;
				Tmp.k[0] = fp32(VertInt[i][0]) * (1.0f / 127.0f);
				Tmp.k[1] = fp32(VertInt[i][1]) * (1.0f / 127.0f);
				Tmp.k[2] = fp32(VertInt[i][2]) * (1.0f / 127.0f);
				Tmp.SafeNormalize();
				pN[v++] = Tmp;
			}
		}
	}

}

void CTM_VertexFrame::ReadTangents(CCFile* _pF)
{
	if (!m_bHasVertices || !m_FilePos)
		return;

	_pF->Seek(m_FilePos);
	int32 nV;
	_pF->ReadLE(nV);
	_pF->RelSeek(nV * 3 * 3);
	m_lTangentU.SetLen(nV);
	m_lTangentV.SetLen(nV);
	{
		CVec3Dfp32 *pTanU,*pTanV;
		pTanU = m_lTangentU.GetBasePtr();
		pTanV = m_lTangentV.GetBasePtr();

		int v = 0;
		while(v < nV)
		{
			const int nVertPerRead = 128;
			int8 VertInt[nVertPerRead][6];
			int nVRead = MinMT(nVertPerRead, nV-v);

			_pF->ReadLE((int8*)VertInt, nVRead*6);

			//If tangents are zero, something is wrong
			if( (VertInt[0][0] == 0) && (VertInt[0][1] == 0) && (VertInt[0][2] == 0) &&
			    (VertInt[0][3] == 0) && (VertInt[0][4] == 0) && (VertInt[0][5] == 0) )
			{
				m_lTangentU.Clear();
				m_lTangentV.Clear();
				m_bFileTangents = false;
				return;
			}

			const fp32 scale = 1.0f / 127.0f;
			for(int i = 0;i < nVRead;i++)
			{
				pTanU[v][0] = fp32(VertInt[i][0]) * scale;
				pTanU[v][1] = fp32(VertInt[i][1]) * scale;
				pTanU[v][2] = fp32(VertInt[i][2]) * scale;
				pTanV[v][0] = fp32(VertInt[i][3]) * scale;
				pTanV[v][1] = fp32(VertInt[i][4]) * scale;
				pTanV[v][2] = fp32(VertInt[i][5]) * scale;
				pTanV[v].Normalize();
				pTanU[v].Normalize();
				v++;
			}
		}
	}
}


#ifndef PLATFORM_CONSOLE
void CTM_VertexFrame::Write(CCFile* _pF, const CBox3Dfp32 &_BoundBox)
{
	MAUTOSTRIP(CTM_VertexFrame_Write, MAUTOSTRIP_VOID);
	int32 Version = CTM_VFRAME_VERSION;
	_pF->WriteLE(Version);
	_pF->WriteLE(m_BoundRadius);
	m_BoundBox.m_Min.Write(_pF);
	m_BoundBox.m_Max.Write(_pF);
	m_Translate.Write(_pF);
	_pF->WriteLE(m_Duration); // Added by Mondelore.

	int32 nV = m_lVertices.Len();
	if (m_lNormals.Len() != nV) Error("Write", "Invalid normal-count.");
	_pF->WriteLE(nV);
	{
		CVec3Dfp32 Scale = _BoundBox.m_Max - _BoundBox.m_Min;
		Scale[0] = 1.0f / Scale[0];
		Scale[1] = 1.0f / Scale[1];
		Scale[2] = 1.0f / Scale[2];
		CVec3Dfp32 Offset = -_BoundBox.m_Min;

// LogFile("BoundBox: " + m_BoundBox.GetString());
// LogFile("Scale: " + Scale.GetString());

		for(int v = 0; v < nV; v++)
		{
			CVec3Dfp32 Vert = m_lVertices[v];
			Vert += Offset;
			Vert.CompMul(Scale, Vert);
			Vert *= 65535.0f;
			uint16 VertInt[3];
			VertInt[0] = RoundToInt(Vert[0]);
			VertInt[1] = RoundToInt(Vert[1]);
			VertInt[2] = RoundToInt(Vert[2]);
			_pF->WriteLE(VertInt[0]);
			_pF->WriteLE(VertInt[1]);
			_pF->WriteLE(VertInt[2]);
		}
	}

	{
		for(int v = 0; v < nV; v++)
		{
			CVec3Dfp32 Norm = m_lNormals[v];
			Norm *= 127.0f;
			int8 NormInt[3];
			NormInt[0] = RoundToInt(Norm[0]);
			NormInt[1] = RoundToInt(Norm[1]);
			NormInt[2] = RoundToInt(Norm[2]);
			_pF->WriteLE((const uint8&)NormInt[0]);
			_pF->WriteLE((const uint8&)NormInt[1]);
			_pF->WriteLE((const uint8&)NormInt[2]);
		}
	}

	{
		for(int v = 0; v < nV; v++)
		{
			if( m_lTangentU.Len() == m_lVertices.Len() )
			{
				CVec3Dfp32 TanU,TanV;
				TanU = m_lTangentU[v] * 127.0f;
				TanV = m_lTangentV[v] * 127.0f;
				int8 lTanInt[6] =
				{
					RoundToInt(TanU[0]),RoundToInt(TanU[1]),RoundToInt(TanU[2]),
					RoundToInt(TanV[0]),RoundToInt(TanV[1]),RoundToInt(TanV[2]),
				};
				_pF->WriteLE(lTanInt,6);
			}

			//Might want to push this back a version to save some space
			else
			{
				uint16 Zero = 0;
				_pF->WriteLE(Zero);
				_pF->WriteLE(Zero);
				_pF->WriteLE(Zero);
			}
		}
	}

//	{ for(int v = 0; v < nV; v++) m_lVertices[v].Write(_pF); };
//	{ for(int v = 0; v < nV; v++) m_lNormals[v].Write(_pF); }
}

// -------------------------------------------------------------------
TPtr<CTM_TVertexFrame> CTM_TVertexFrame::Duplicate(const int32* _piV, int _nV)
{
	spCTM_TVertexFrame spNew = MNew(CTM_TVertexFrame);
	if (!spNew)
		MemError("Duplicate");

	if (!_piV)
	{
		spNew->m_lVertices.Add(&m_lVertices);
	}
	else
	{
		spNew->m_lVertices.SetLen(_nV);
//		CopyVertices(m_lVertices.GetBasePtr(), spNew->m_lVertices.GetBasePtr(), _piV, _nV);
		CopyVertices(m_lVertices, spNew->m_lVertices, _piV, _nV);
	}

	return spNew;
}

#endif

void CTM_TVertexFrame::Read(CCFile* _pF, int _Version, bool _bDelayLoad)
{
	MAUTOSTRIP(CTM_TVertexFrame_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0 :
		{
			int32 nV;
			_pF->ReadLE(nV);
			m_lVertices.SetLen(nV);
			for(int v = 0; v < nV; v++) 
				m_lVertices[v].Read(_pF);
		}
		break;

	case 0x0200 :
		{
			int32 nV;
			_pF->ReadLE(nV);
			m_lVertices.SetLen(nV);

			CVec2Dfp32 TVMin, TVMax;
			TVMin.Read(_pF);
			TVMax.Read(_pF);

			m_Bound[0] = TVMin[0];
			m_Bound[1] = TVMin[1];
			m_Bound[2] = TVMax[0];
			m_Bound[3] = TVMax[1];

			CVec2Dfp32 Scale;
			TVMax.Sub(TVMin, Scale);
			Scale *= 1.0f / 65535;

			CVec2Dfp32* pV = m_lVertices.GetBasePtr();

			int v = 0;
			while(v < nV)
			{
				const int nVertPerRead = 128;
				uint16 VertInt[nVertPerRead][2];
				int nVRead = MinMT(nVertPerRead, nV-v);

				_pF->ReadLE((uint16*)VertInt, nVRead*2);

				for(int i = 0; i < nVRead; i++)
				{
					pV[v][0] = fp32(VertInt[i][0])*Scale[0] + TVMin[0];
					pV[v][1] = fp32(VertInt[i][1])*Scale[1] + TVMin[1];
					v++;
				}
			}
		};
		break;
	case 0x0201 :
		{
			m_FilePos = _pF->Pos();
			if (_bDelayLoad)
			{
				int32 nV;
				_pF->ReadLE(nV);
				m_bHasVertices = nV > 0;
				_pF->RelSeek(nV * 4 + sizeof(CVec2Dfp32) * 2);
			}
			else
			{
				ReadVertices(_pF);
			}

		};
		break;

	default :
		Error("Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CTM_TVertexFrame::ReadVertices(CCFile* _pF)
{
	if (!m_FilePos)
		return;

	_pF->Seek(m_FilePos);

	int32 nV;

	_pF->ReadLE(nV);

	m_lVertices.SetLen(nV);

	CVec2Dfp32 TVMin, TVMax;
	TVMin.Read(_pF);
	TVMax.Read(_pF);
	m_Bound[0] = TVMin[0];
	m_Bound[1] = TVMin[1];
	m_Bound[2] = TVMax[0];
	m_Bound[3] = TVMax[1];

	CVec2Dfp32 Scale;
	TVMax.Sub(TVMin, Scale);
	Scale *= 1.0f / 65535;

	CVec2Dfp32* pV = m_lVertices.GetBasePtr();

	int v = 0;
	while(v < nV)
	{
		const int nVertPerRead = 128;
		uint16 VertInt[nVertPerRead][2];
		int nVRead = MinMT(nVertPerRead, nV-v);

		_pF->ReadLE((uint16*)VertInt, nVRead*2);

		for(int i = 0; i < nVRead; i++)
		{
			pV[v][0] = fp32(VertInt[i][0])*Scale[0] + TVMin[0];
			pV[v][1] = fp32(VertInt[i][1])*Scale[1] + TVMin[1];
			v++;
		}
	}

	m_bHasVertices = nV > 0;
}


#ifndef PLATFORM_CONSOLE
void CTM_TVertexFrame::Write(CCFile* _pF)
{
	MAUTOSTRIP(CTM_TVertexFrame_Write, MAUTOSTRIP_VOID);
	int32 nV = m_lVertices.Len();
	_pF->WriteLE(nV);

	CVec2Dfp32 TVMin, TVMax, TVDelta;
	CVec2Dfp32::GetMinBoundRect(m_lVertices.GetBasePtr(), TVMin, TVMax, m_lVertices.Len());
	TVMin.Write(_pF);
	TVMax.Write(_pF);

	CVec2Dfp32 Scale;
	Scale[0] = 1.0f / (TVMax[0]-TVMin[0]);
	Scale[1] = 1.0f / (TVMax[1]-TVMin[1]);

	m_Bound[0] = TVMin[0];
	m_Bound[1] = TVMin[1];
	m_Bound[2] = TVMax[0];
	m_Bound[4] = TVMax[1];

	for(int v = 0; v < nV; v++)
	{
		CVec2Dfp32 Vert = m_lVertices[v];
		Vert -= TVMin;
		Vert.CompMul(Scale, Vert);
		Vert *= 65535.0f;
		uint16 VertInt[2];
		VertInt[0] = RoundToInt(Vert[0]);
		VertInt[1] = RoundToInt(Vert[1]);
		_pF->WriteLE(VertInt[0]);
		_pF->WriteLE(VertInt[1]);
	}
//	for(int v = 0; v < nV; v++) m_lVertices[v].Write(_pF);
}
#endif

// -------------------------------------------------------------------
void CTM_Triangle::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTM_Triangle_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	pF->ReadLE(m_iV[0]);
	pF->ReadLE(m_iV[1]);
	pF->ReadLE(m_iV[2]);
}

#ifndef PLATFORM_CONSOLE
void CTM_Triangle::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTM_Triangle_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE(m_iV[0]);
	pF->WriteLE(m_iV[1]);
	pF->WriteLE(m_iV[2]);
}
#endif


//-------------------------------------------------------------------------------------------------
// Collision code

// grabbed from MCSolid.cpp
static inline bool IntersectTriangle(const CVec3Dfp32& _p0,
									 const CVec3Dfp32& _p1,
									 const CVec3Dfp32& _v0,
									 const CVec3Dfp32& _v1,
									 const CVec3Dfp32& _v2,
									 CVec3Dfp32& _Pos,
									 bool _bIntersectBackSide = true)
{
	MAUTOSTRIP(IntersectTriangle, false);

	CVec3Dfp32 dir = _p1 - _p0;
	fp32 u,v,t;
	CVec3Dfp32 edge01,edge02,tvec,pvec,qvec;
	fp32 det,inv_det;

	// Calculate edge vectors from vert0
	edge02 = _v2 - _v0;
	edge01 = _v1 - _v0;

	// Begin calc determinant
	pvec = dir / edge02;
	det = edge01 * pvec;

	if (_bIntersectBackSide == true)
	{	
		if ((det > -0.01f) && (det < 0.01f))
		{
			return(false);
		}

		// we go through this sign hoopla under the assumption that the function mostly
		// will return false and that we wish to avoid 'inv_det = 1.0f / det;'
		fp32 signDet = Sign(det);
		fp32 absDet = M_Fabs(det);
		tvec = _p0 - _v0;

		// Calc u and check its bounds
		// We don't normalize u,v as the divide cost us precious cycles
		// We need to get the sign of det to make sure the comparisons work
		u = (tvec * pvec) * signDet;
		if ((u < 0.0f)||(u > absDet))
		{
			return(false);
		}

		// Calc v param and check bounds
		qvec = tvec / edge01;
		v = (dir * qvec) * signDet;

		// Q: Why test against det instead of 1.0?
		// A: This way we may bail before doing the costly inv_det division
		if ((v < 0.0f)||((u) + (v) > absDet))
		{
			return(false);
		}

		// We now know that the ray hits the triangle, we scale the u,v,t params
		t = (edge02 * qvec) * signDet;
		if ((t < 0.0f)||(t > absDet))
		{
			return(false);
		}

		// We divide here hoping most cases return false
		// (If most cases return true we should divide as early as possible to help pipelining)
		inv_det = 1.0f / det;
		_Pos = _p0 + dir * t * inv_det;

		return(true);
	}
	else	// Ignore backfaced polys
	{
		// If determinant is near zero ray is parallell to triangle plane
		// Use fabs(det) if we don't want backface culling
		if (det < 0.01f)
		{
			return(false);
		}

		// Calculate parameterized distance from vert0 to ray origin
		tvec = _p0 - _v0;

		// Calc u and check its bounds
		u = tvec * pvec;
		if ((u < 0.0f)||(u > det))
		{
			return(false);
		}

		// Calc v param and check bounds
		qvec = tvec / edge01;
		v = dir * qvec;

		// Q: Why test against det instead of 1.0?
		// A: This way we may bail before doing the costly inv_det division
		if ((v < 0.0f)||((u) + (v) > det))
		{
			return(false);
		}

		// We now know that the ray hits the triangle, we scale the u,v,t params
		t = edge02 * qvec;
		inv_det = 1.0f / det;
		t *= inv_det;

		_Pos = _p0 + dir * t;

		return(true);
	}
}


//Same as above, though without division and position determination
static inline bool IntersectTriangle(
									 const CVec3Dfp32& _p0,
									 const CVec3Dfp32& _p1,
									 const CVec3Dfp32& _v0,
									 const CVec3Dfp32& _v1,
									 const CVec3Dfp32& _v2,
									 bool _bIntersectBackSide = true)
{
	MAUTOSTRIP(IntersectTriangle, false);

	CVec3Dfp32 dir = _p1 - _p0;
	fp32 u,v,t;
	CVec3Dfp32 edge01,edge02,tvec,pvec,qvec;
	fp32 det;

	edge02 = _v2 - _v0;
	edge01 = _v1 - _v0;

	pvec = dir / edge02;
	det = edge01 * pvec;

	if (_bIntersectBackSide == true)
	{	
		if ((det > -0.01f) && (det < 0.01f))
		{
			return(false);
		}

		fp32 signDet = Sign(det);
		fp32 absDet = M_Fabs(det);
		tvec = _p0 - _v0;

		u = (tvec * pvec) * signDet;
		if ((u < 0.0f)||(u > absDet))
		{
			return(false);
		}

		qvec = tvec / edge01;
		v = (dir * qvec) * signDet;

		if ((v < 0.0f)||((u) + (v) > absDet))
		{
			return(false);
		}

		t = (edge02 * qvec) * signDet;
		if ((t < 0.0f)||(t > absDet))
		{
			return(false);
		}

		return(true);
	}
	else	// Ignore backfaced polys
	{
		if (det < 0.01f)
		{
			return(false);
		}

		tvec = _p0 - _v0;

		u = tvec * pvec;
		if ((u < 0.0f)||(u > det))
		{
			return(false);
		}

		qvec = tvec / edge01;
		v = dir * qvec;

		if ((v < 0.0f)||((u) + (v) > det))
		{
			return(false);
		}

		return(true);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Brute-force intersection for a single Cluster	

Parameters:		
_Start:		Start point of line
_End:		Endpoint of line
_iFrame:	Frame to check

Returns:	true on intersection, false otherwise

Comments:	This function implemented for timing purposes,
			in most cases, others are more effective
\*____________________________________________________________________*/ 
/*
bool CTM_VertexBuffer::BruteForceIntersect(CTriangleMeshCore* _pTriCore, const CVec3Dfp32 &_Start,const CVec3Dfp32 &_End,int _iFrame)
{
	CVec3Dfp32 Pos;
	if( !m_BoundBox.IntersectLine(_Start,_End,Pos) )
		return false;

	CVec3Dfp32 * pVerts = GetVertexPtr(_pTriCore, _iFrame);
	for(int i=0;i < m_lTriangles.Len();i++)
	{
		CTM_Triangle &Tri = m_lTriangles[i];
		if( IntersectTriangle(_Start,_End,pVerts[Tri.m_iV[0]],pVerts[Tri.m_iV[1]],pVerts[Tri.m_iV[2]]) )
		{
			return true;
		}
	}

	return false;
}
*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Brute-force intersection for a single Cluster, stores result

Parameters:		
_Start:		Startpoint of line
_End:		Endpoint of line
_Pos:		Collision position
_iFrame:	Frame to check

Returns:	true on intersection, false otherwise

Comments:	This function implemented for timing purposes,
			in most cases, others are more effective
\*____________________________________________________________________*/ 
/*
bool CTM_VertexBuffer::BruteForceIntersect(CTriangleMeshCore* _pTriCore, const CVec3Dfp32 &_Start,const CVec3Dfp32 &_End,CVec3Dfp32 &_Pos,int _iFrame)
{
	CVec3Dfp32 Pos;
	if( !m_BoundBox.IntersectLine(_Start,_End,Pos) )
		return false;

	//Pick a principal axis where Dir.Axis != 0
	CVec3Dfp32 Dir = _End-_Start;

	_Pos = _End;
	CVec3Dfp32 * pVerts = GetVertexPtr(_pTriCore, _iFrame);
	bool bRet = false;
	for(int i=0;i < m_lTriangles.Len();i++)
	{
		CTM_Triangle &Tri = m_lTriangles[i];
		if( IntersectTriangle(_Start,_End,pVerts[Tri.m_iV[0]],pVerts[Tri.m_iV[1]],pVerts[Tri.m_iV[2]],Pos) )
		{
			//Set return value and determine least distance
			if( Pos.DistanceSqr(_Start) < _Pos.DistanceSqr(_Start) )
			{
				_Pos = Pos;
			}
			bRet = true;
		}
	}

	return bRet;
}
*/

#ifndef PLATFORM_CONSOLE

typedef TList_Linked<uint16> TMBTTriangleList;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Generate a Covariance Matrix

Parameters:		
_Dst:		Destination Matrix
_liTriangles:	List of Triangles to use
_Cluster:	Cluster holding the points
_iFrame:	Points to use
\*____________________________________________________________________*/ 
static void TMBTCovarianceMatrix(CTriangleMeshCore* _pTriCore, CMat4Dfp32 & _Dst, const TMBTTriangleList & _liTriangles, CTM_VertexBuffer & _VertexBuffer, int _iFrame = 0)
{
	fp32 Inverse = 1.0f / _liTriangles.Len();	
	CVec3Dfp32 Centroid(0.0f,0.0f,0.0f);
	fp32	lValues[6] = {0};
	CVec3Dfp32 *pVec = _VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);

	//Determine the centroid
	int i;
	for(i = 0;i < _liTriangles.Len();i++)
	{
		const CTM_Triangle &Tri = _VertexBuffer.m_lTriangles[_liTriangles[i]];
		Centroid += pVec[Tri.m_iV[0]] + pVec[Tri.m_iV[1]] + pVec[Tri.m_iV[2]];
	}
	Centroid *= Inverse;

	//Use the centroid to determine covariance
	for(i = 0;i < _liTriangles.Len();i++)
	{
		for(int j = 0;j < 3;j++)
		{
			CVec3Dfp32 &Pt = pVec[_VertexBuffer.m_lTriangles[_liTriangles[i]].m_iV[j]];
			lValues[0] += Sqr(Pt.k[0]);
			lValues[1] += Pt.k[0] * Pt.k[1];
			lValues[2] += Pt.k[0] * Pt.k[2];
			lValues[3] += Sqr(Pt.k[1]);
			lValues[4] += Pt.k[1] * Pt.k[2];
			lValues[5] += Sqr(Pt.k[2]);
		}
	}

	_Dst.k[0][0] = lValues[0];
	_Dst.k[0][1] = _Dst.k[1][0] = lValues[1];
	_Dst.k[0][2] = _Dst.k[2][0] = lValues[2];
	_Dst.k[1][1] = lValues[3];
	_Dst.k[1][2] = _Dst.k[2][1] = lValues[4];
	_Dst.k[2][2] = lValues[5];
	_Dst.k[3][0] = _Dst.k[3][1] = _Dst.k[3][2] = 0.0f;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	2-by-2 Symmetric Schur decomposition	

Parameters:		
_Mat:		Input matrix
_I1:		Index 1
_I2:		Index 2

Returns:	cosine/sine pair vector
\*____________________________________________________________________*/ 
static CVec2Dfp32 TMBTSymSchur2(const CMat4Dfp32 & _Mat, int _Index1, int _Index2)
{
	fp32 i12 = _Mat.k[_Index1][_Index2];
	fp32 i11 = _Mat.k[_Index1][_Index1];
	fp32 i22 = _Mat.k[_Index2][_Index2];
	if( Abs(i12) < 0.001f )
	{
		return CVec2Dfp32(1.0f,0.0f);
	}
	else
	{
		fp32 Foo = (i11 - i22) / (2.0f * i12);
		fp32 SqFoo = Sqr(Foo);
		fp32 Bar = (Foo >= 0.0f) ? 1.0f / (Foo + M_Sqrt(1.0f + SqFoo)) :
								  -1.0f / (-Foo + M_Sqrt(1.0f + SqFoo));
		fp32 Baz = 1.0f / M_Sqrt(1.0f + Sqr(Bar));
		return CVec2Dfp32(Baz,Bar * Baz);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Compute Eigenvectors and Eigenvalues	

Parameters:		
_Values:	Input matrix, output is eigenvalues
_Vectors:	Eigenvectors output
_nItrMax:	Max iterations
\*____________________________________________________________________*/ 
static void TMBTJacobi(CMat4Dfp32 &_Values, CMat4Dfp32 &_Vectors, int _nItrMax = 48)
{
	_Vectors.Unit();
	fp32 Prev = 3.4e38f;

	for(int itr = 0;itr < _nItrMax;itr++)
	{
		int i,j,p=0,q=1;
		for(i = 0;i < 3;i ++)
		{
			for(j = 0;j < 3;j ++)
			{
				if(i == j) continue;
				if( Abs(_Values.k[i][j]) > Abs(_Values.k[p][q]) )
				{
					p = i;
					q = j;
				}
			}
		}

		CMat4Dfp32 Jacobi,TJacobi,Temp;
		CVec2Dfp32 CS = TMBTSymSchur2(_Values,p,q);
		Jacobi.Unit();
		Jacobi.k[p][p] = CS.k[0];
		Jacobi.k[p][q] = CS.k[1];
		Jacobi.k[q][q] = CS.k[0];
		Jacobi.k[q][p] = -CS.k[1];
		_Vectors.Multiply(Jacobi,Temp);
		_Vectors = Temp;

		Jacobi.Transpose(TJacobi);
		TJacobi.Multiply(_Values,Temp);
		Temp.Multiply(Jacobi,_Values);

		fp32 Current = 0.0f;
		for(i = 0;i < 3;i++)
		{
			for(j = 0;j < 3;j++)
			{
				if(i == j) continue;
				Current += _Values.k[i][j] * _Values.k[j][i];
			}
		}

		if( (itr > 2) && (Current >= Prev) )
			return;

		Prev = Current;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Generate a sorted list of eigenvectors

Parameters:		
_liInput:		Input points
_Dst:		Destination vector (must hold 3 points)
_Cluster:	Cluster holding points
_iFrame:	Frame in cluster

Comments:	Returns the eigenvectors of the triangles' pointset
			sorted in descending order
\*____________________________________________________________________*/ 
static void TMBTGetEigenVectors(CTriangleMeshCore* _pTriCore, const TMBTTriangleList &_liInput, CVec3Dfp32 * _Dst, CTM_Cluster &_Cluster, CTM_VertexBuffer& _VertexBuffer, int _iFrame = 0)
{
	CMat4Dfp32	Vectors,Values;

	TMBTCovarianceMatrix(_pTriCore, Values, _liInput, _VertexBuffer, _iFrame);
	TMBTJacobi(Values, Vectors);

	CVec3Dfp32 Val(Abs(Values.k[0][0]),Abs(Values.k[1][1]),Abs(Values.k[2][2]));
	
	uint8 iPrio[3];
	iPrio[0] = (Val.k[0] > Val.k[1]) ? ((Val.k[0] > Val.k[2]) ? 0 : 2) : ((Val.k[1] > Val.k[2]) ? 1 : 2);
	iPrio[1] = (Val.k[(iPrio[0]+1)%3] > Val.k[(iPrio[0]+2)%3]) ? (iPrio[0]+1) % 3 : (iPrio[0]+2) % 3;
	iPrio[2] = ((iPrio[0]+1)%3 == iPrio[1]) ? (iPrio[0]+2)%3 : (iPrio[0]+1)%3;

	for(int i=0;i < 3;i++)
	{
		_Dst[i] = CVec3Dfp32(Vectors.k[0][iPrio[i]], Vectors.k[1][iPrio[i]], Vectors.k[2][iPrio[i]]);
	}
}


//Sort function
int	TMBTFloatSort(const void * _Elm1,const void * _Elm2)
{
	return (*((fp32*)_Elm1) < *((fp32*)_Elm2)) ? -1 : (((*(fp32*)_Elm1) > *((fp32*)_Elm2)) ? 1 : 0);
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Generate end values, median and quartiles

Parameters:		
_Axis:		Axis to sort against
_liInput:		Input points
_Dst:		Destination vector (must hold 5 points)
_Cluster:	Cluster holding points
_iFrame:	Frame in cluster

Comments:	using triangle centroids, generates a set of median, end
			and quartile values sorted against a vector
\*____________________________________________________________________*/ 
static void TMBTGenerateQuartiles(CTriangleMeshCore* _pTriCore, const CVec3Dfp32 &_Axis, const TMBTTriangleList &_liInput,
								  CVec3Dfp32 *_pDst, CTM_VertexBuffer& _VertexBuffer, int _iFrame = 0)
{
	uint32 nElem = _liInput.Len();
	TThinArray<uint32>	lData;
	lData.SetLen(nElem * 2);
	uint32 i;
	CVec3Dfp32 * pVec = _VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);

	if( !nElem ) return;

	//Create list
	for(i = 0;i < nElem;i++)
	{
		CTM_Triangle &Tri = _VertexBuffer.m_lTriangles[_liInput[i]];
		lData[(i << 1)+1] = i;
		CVec3Dfp32 Centroid = (pVec[ Tri.m_iV[0] ] + pVec[ Tri.m_iV[1] ] + 
							  pVec[ Tri.m_iV[2] ]) / 3.0f;
		*((fp32*)(lData.GetBasePtr() + (i<<1))) = Centroid * _Axis;
	}

	//Sort list...
	qsort(lData.GetBasePtr(), nElem, sizeof(uint32) + sizeof(fp32), TMBTFloatSort);

	//Return data...
	for(i = 0;i < 5;i++)
	{
		uint32 iCurrent = ((i * nElem) / 4) - ((i == 4) ? 1 : 0);
		const CTM_Triangle &Tri = _VertexBuffer.m_lTriangles[_liInput[ lData[(iCurrent << 1) + 1] ] ];
		_pDst[i] = (pVec[ Tri.m_iV[0] ] + pVec[ Tri.m_iV[1] ] + pVec[ Tri.m_iV[2] ]) / 3.0f;
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Grow a Box, adding an entire triangle

Parameters:		
_SrcBox:	Original box
_DstBox:	Destination box
_Tri:		Triangle
_Cluster:	Cluster containing the points
_iFrame:	Frame to use
\*____________________________________________________________________*/ 
static void TMBTGrowBox(CTriangleMeshCore* _pTriCore, const CBox3Dfp32 &_SrcBox, CBox3Dfp32 &_DstBox,
						   const CTM_Triangle & _Tri, CTM_VertexBuffer & _VertexBuffer, int _iFrame = 0)
{
	CVec3Dfp32 *pVec = _VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);
	_DstBox = _SrcBox;
	for(int i=0;i < 3;i++)
	{
		_DstBox.Expand(pVec[ _Tri.m_iV[i] ]);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Sort triangles

Parameters:		
_pPoints:	Points to use
_nPoints:	Number of points
_liTriangles:Triangles to sort
_pDst:		Array to hold the result indices
_Cluster:	Cluster to use
_iFrame:	Frame to use

Comments:	Sorts a number of triangles in ascending order, 
			depending on their distance to any of the vectors in a list
\*____________________________________________________________________*/ 
static void TMBTSortFromDistance(CTriangleMeshCore* _pTriCore, const CVec3Dfp32 * _pPoints, uint16 _nPoints,
								 const TMBTTriangleList & _liTriangles, uint16 *_pDst,
								 CTM_VertexBuffer & _VertexBuffer, int _iFrame = 0)
{
	uint32 nElem = _liTriangles.Len();
	TThinArray<uint32>	lData;
	lData.SetLen(nElem * 2);
	uint32 i;
	CVec3Dfp32 * pVec = _VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);

	//Create list
	for(i = 0;i < nElem;i++)
	{
		CTM_Triangle &Tri = _VertexBuffer.m_lTriangles[_liTriangles[i]];
		lData[(i << 1)+1] = i;
		CVec3Dfp32 Centroid = (pVec[ Tri.m_iV[0] ] + pVec[ Tri.m_iV[1] ] + 
							  pVec[ Tri.m_iV[2] ]) / 3.0f;
		fp32 Smallest = Centroid.DistanceSqr(*_pPoints);
		for(int j=1;j < _nPoints;j++)
		{
			fp32 NDist = Centroid.DistanceSqr(_pPoints[j]);
			Smallest = Min(Smallest,NDist);
		}
		*((fp32*)(lData.GetBasePtr() + (i<<1))) = Smallest;
	}

	//Sort list...
	qsort(lData.GetBasePtr(),nElem,sizeof(uint32)+sizeof(fp32),TMBTFloatSort);

	for(i = 0;i < nElem;i ++)
	{
		_pDst[i] = lData[(i << 1) + 1]; 
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CTMTempBoxNode

Comments:		Class used to hold a temporary BoxNode used
				when building a BoxTree
\*____________________________________________________________________*/
class CTMTempBoxNode : public CReferenceCount
{
	
public:

	TMBTTriangleList	m_liTriangles;
	uint8				m_Generation;

};

typedef TPtr<CTMTempBoxNode> spCTMTempBoxNode;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	"Clamp" a box to 8-bit mode

Parameters:		
_Box:		Box to clamp
_Cluster:	Cluster to use
_InvRadius:	Local unit

Returns:	6 bytes with the quantized box values, in a uint64

Comments:	Rounds off the values of a box to the values they
			would have when quantized and returns the quantized
			values
\*____________________________________________________________________*/ 
uint64 TMBTLimitBox(CBox3Dfp32& _Box, CTM_Cluster& _Cluster, fp32 _InvRadius)
{
	uint64 Ret;
	uint8 *pRet = (uint8*)(&Ret);

	//Find min and max byte equivalents
	CVec3Duint8 ByteMin(Clamp((int16)((_Box.m_Min.k[0] - _Cluster.m_BoundBox.m_Min.k[0]) * _InvRadius), 0, 255),
						Clamp((int16)((_Box.m_Min.k[1] - _Cluster.m_BoundBox.m_Min.k[1]) * _InvRadius), 0, 255),
						Clamp((int16)((_Box.m_Min.k[2] - _Cluster.m_BoundBox.m_Min.k[2]) * _InvRadius), 0, 255));

	CVec3Duint8 ByteMax(Clamp((int16)((_Box.m_Max.k[0] - _Cluster.m_BoundBox.m_Min.k[0]) * _InvRadius), 0, 255),
						Clamp((int16)((_Box.m_Max.k[1] - _Cluster.m_BoundBox.m_Min.k[1]) * _InvRadius), 0, 255),
						Clamp((int16)((_Box.m_Max.k[2] - _Cluster.m_BoundBox.m_Min.k[2]) * _InvRadius), 0, 255));

	//For each dimension, see if we need to grow the box
	int i;
	for(i=0;i < 3;i++)
	{
		_Box.m_Min.k[i] = ((fp32)ByteMin.k[i]) / _InvRadius + _Cluster.m_BoundBox.m_Min.k[i];
		fp32 NewVal = ((fp32)ByteMax.k[i]) / _InvRadius + _Cluster.m_BoundBox.m_Min.k[i];
		if( (((fp32)NewVal) < _Box.m_Max.k[i]) && (ByteMax.k[i] < 255) ) ByteMax.k[i]++;
		_Box.m_Max.k[i] = ((fp32)ByteMax.k[i]) / _InvRadius + _Cluster.m_BoundBox.m_Min.k[i];
	}

	//Create return value
	pRet[0] = (ByteMin.k[0] + ByteMax.k[0]) >> 1;
	pRet[1] = (ByteMin.k[1] + ByteMax.k[1]) >> 1;
	pRet[2] = (ByteMin.k[2] + ByteMax.k[2]) >> 1;
	pRet[3] = (ByteMax.k[0] - ByteMin.k[0]) >> 1;
	pRet[4] = (ByteMax.k[1] - ByteMin.k[1]) >> 1;
	pRet[5] = (ByteMax.k[2] - ByteMin.k[2]) >> 1;

	//Again, see if we need to grow the value
	for(i = 0;i < 3;i++)
	{
		if( ((ByteMin.k[i]+ByteMax.k[i]) % 2 == 1) && (pRet[i+3] < 255) )
		{
			pRet[i+3] ++;
		}
	}

	return Ret;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Create child nodes

Parameters:		
_pSrcNode:	Source Node
_pSrcTNode:	Source TempNode
_pDstNode:	Destination Nodes
_pDstTNode:	Destination TempNodes
_pQuart	:	Quartiles (if NULL, auto-generated)
_Cluster:	Cluster
_iQuart0:	Quartile to use
_iQuart1:	Quartile to use
_Exp:		Exponent of diff
_iFrame:	Frame ID

Returns:	true if successful, false if failed (all triangles in one child)
\*____________________________________________________________________*/ 
static bool TMBTSpawnChildren(CTriangleMeshCore* _pTriCore, const CTM_ShadowData::CBTNode * _pSrcNode, CTMTempBoxNode * _pSrcTNode,
							  CTM_ShadowData::CBTNode * _pDstNode, spCTMTempBoxNode * _pDstTNode, CVec3Dfp32 *_pQuart,
							  CTM_Cluster & _Cluster, CTM_VertexBuffer& _VertexBuffer, uint8 _iQuart0, uint8 _iQuart1, fp32 _Exp, fp32 _InvRadius,
							  int _iFrame = 0)
{
	//Create good origin points
	CVec3Dfp32 Eigen[3],Quart[5],Centers[2];

	if( !_pQuart )
	{
		TMBTGetEigenVectors(_pTriCore, _pSrcTNode->m_liTriangles, Eigen, _Cluster, _VertexBuffer, _iFrame);
		TMBTGenerateQuartiles(_pTriCore, Eigen[0], _pSrcTNode->m_liTriangles, Quart, _VertexBuffer, _iFrame);
		_pQuart = Quart;
	}

	Centers[0] = _pQuart[_iQuart0];
	Centers[1] = _pQuart[_iQuart1];

	//Create a sorted list
	TThinArray<uint16>	lIndices;
	lIndices.SetLen(_pSrcTNode->m_liTriangles.Len());
	TMBTSortFromDistance(_pTriCore, Centers, 2, _pSrcTNode->m_liTriangles, lIndices.GetBasePtr(), _VertexBuffer, _iFrame);

	CBox3Dfp32 Boxes[2];
	Boxes[0].m_Max = Boxes[0].m_Min = Centers[0];
	Boxes[1].m_Max = Boxes[1].m_Min = Centers[1];

	//Sort faces
	uint32 i;
	for(i = 0;i < _pSrcTNode->m_liTriangles.Len();i++)
	{
		uint16 iTriangle = _pSrcTNode->m_liTriangles[lIndices[i]];
		CBox3Dfp32 NewBoxes[2];
		TMBTGrowBox(_pTriCore, Boxes[0], NewBoxes[0], _VertexBuffer.m_lTriangles[iTriangle], _VertexBuffer, _iFrame);
		TMBTGrowBox(_pTriCore, Boxes[1], NewBoxes[1], _VertexBuffer.m_lTriangles[iTriangle], _VertexBuffer, _iFrame);

		CVec3Dfp32 Delta[4] = {Boxes[0].m_Max - Boxes[0].m_Min,Boxes[1].m_Max - Boxes[1].m_Min,
							  NewBoxes[0].m_Max - NewBoxes[0].m_Min,NewBoxes[1].m_Max - NewBoxes[1].m_Min};
		Delta[0] += CVec3Dfp32(0.1f,0.1f,0.1f);
		Delta[1] += CVec3Dfp32(0.1f,0.1f,0.1f);
		Delta[2] += CVec3Dfp32(0.1f,0.1f,0.1f);
		Delta[3] += CVec3Dfp32(0.1f,0.1f,0.1f);
		fp32 Volumes[2] = { Delta[0].k[0] * Delta[0].k[1] * Delta[0].k[2], Delta[1].k[0] * Delta[1].k[1] * Delta[1].k[2] };
		fp32 NewVolumes[2] = { Delta[2].k[0] * Delta[2].k[1] * Delta[2].k[2], Delta[3].k[0] * Delta[3].k[1] * Delta[3].k[2] };

		
		// Determine child. If no radii increased, pick smallest child.
		// Otherwise, pick least increase - bias towards equal-sized nodes
		uint8 iNode;
		if( (Volumes[0] == NewVolumes[0]) && (Volumes[1] == NewVolumes[1]) )
		{
			iNode = (Volumes[0] < Volumes[1]) ? 0 : 1;
		}
		else
		{
			fp32 RadDif[2] = {NewVolumes[0]-Volumes[0],NewVolumes[1]-Volumes[1]};
			RadDif[0] *= M_Pow((Volumes[0])/(Volumes[1]),_Exp);
			iNode = (RadDif[0] < RadDif[1]) ? 0 : 1;
		}

		_pDstTNode[iNode]->m_liTriangles.Add(iTriangle);
		Boxes[iNode] = NewBoxes[iNode];
	}

	uint64 NewBox = TMBTLimitBox(Boxes[0], _Cluster, _InvRadius);
	memcpy(_pDstNode[0].m_lBox, &NewBox, 6);
	
	NewBox = TMBTLimitBox(Boxes[1], _Cluster, _InvRadius);
	memcpy(_pDstNode[1].m_lBox, &NewBox, 6);

	//Check if nodes were of similar size and position 
	//ie: it is unlikely that a line would hit one but not the other
	// This only made things unhappy =(
	/*
	fp32 MinSize = Min(Radii[0],Radii[1]);
	fp32 RadDif = (Radii[0] - Radii[1]) / MinSize;
	fp32 PosDif = (Centers[0] - Centers[1]).LengthSqr() / Sqr(MinSize);
	if( (RadDif < 0.1f) && (PosDif < Sqr(0.1f)) )
	{
		return false;
	}
	//*/

	if( (_pDstTNode[0]->m_liTriangles.Len() == 0) || (_pDstTNode[1]->m_liTriangles.Len() == 0) )
	{
		return false;
	}
	else
	{
		return true;
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Get "error" of nodes

Parameters:		
_pNodes:	2 source nodes
_pTNodes:	2 source tempnodes

Returns:	Pair error value. Value is high if nodepair will result
			in many triangle checks (large spheres with many faces)
\*____________________________________________________________________*/ 
static uint32 TMBTGetNodeValues(CTM_ShadowData::CBTNode * _pNodes,spCTMTempBoxNode * _pTNodes)
{
	uint32 Ret = 0;

	uint8 *pBox = _pNodes[0].m_lBox;
	Ret += (pBox[3]-pBox[0])*(pBox[4]-pBox[1])*(pBox[5]-pBox[2]) * _pTNodes[0]->m_liTriangles.Len();
	pBox = _pNodes[0].m_lBox;
	Ret += (pBox[3]-pBox[0])*(pBox[4]-pBox[1])*(pBox[5]-pBox[2]) * _pTNodes[1]->m_liTriangles.Len();

	return Ret;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Create box tree

Parameters:		
_MaxGeneration:	Max generation to allow
_MinFaces:		Min faces needed to split
_iFrame:		Frame to use
\*____________________________________________________________________*/ 
void CTM_ShadowData::CreateBoxTree(CTriangleMeshCore* _pTriCore, uint8 _MaxGeneration /* = 0 */,uint16 _MinFaces /* = 8 */,int _iFrame)
{
	if( !m_VertexBuffer.m_lTriangles.Len() )
	{
		m_lBoxNodes.Clear();
		return;
	}

	uint16 nTri = m_VertexBuffer.m_lTriangles.Len();

	if( !_MaxGeneration )
	{
		_MaxGeneration = 9;
	}

	uint16 nNodes = 1;
	TThinArray<spCTMTempBoxNode>	lTempNodes;
	m_lBoxNodes.SetLen(nTri);
	lTempNodes.SetLen(nTri);

	CBTNode * pNode = &m_lBoxNodes[0];
	lTempNodes[0] = MNew(CTMTempBoxNode);
	lTempNodes[0]->m_Generation = 0;

	int i;
	
	{
		//Don't trust the bounding box!
		m_Cluster.m_BoundBox.m_Max = *m_VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);
		m_Cluster.m_BoundBox.m_Min = *m_VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);
		for( i = 0; i < m_VertexBuffer.GetNumVertices(_pTriCore); i++ )
		{
			m_Cluster.m_BoundBox.Expand(m_VertexBuffer.GetVertexPtr(_pTriCore, _iFrame)[i]);
		}
		for( i = 0; i < m_VertexBuffer.m_lTriangles.Len(); i++ )
		{
			lTempNodes[0]->m_liTriangles.Add(i);
		}

		//Grab a copy of the bounding box to modify
		CBox3Dfp32 BoundBox = m_Cluster.m_BoundBox;
		CVec3Dfp32 Sz = m_Cluster.m_BoundBox.m_Max - m_Cluster.m_BoundBox.m_Min;

		//Local unit is longest side / 255
		m_InvBoundRadius = 255.0f / Max( Sz.k[0], Max(Sz.k[1],Sz.k[2]) );
		uint64 Data = TMBTLimitBox(BoundBox, m_Cluster, m_InvBoundRadius);
		memcpy(m_lBoxNodes[0].m_lBox,&Data,6);
	}

	//To avoid a recursive function, create "process" queue
	TList_Linked<uint16>	lProcess;
	lProcess.Add(0);

	while(lProcess.Len())
	{
		uint16 iCurrent = lProcess[0];
		lProcess.Del(0);
		pNode = &m_lBoxNodes[iCurrent];

		//*

		//Create 8 different pairs of nodes and find the best pair
		spCTMTempBoxNode lTTestNodes[8][2];
		CBTNode			 lTestNodes[8][2];
		uint32	ErrVal = 0xFFFFFFFF;
		uint8	iBestPair = 8;
		uint8	nFail = 0;

		//Pregenerate Quartiles to save some time
		CVec3Dfp32 lEigen[3],lQuart[5];
		TMBTGetEigenVectors(_pTriCore, lTempNodes[iCurrent]->m_liTriangles, lEigen, m_Cluster, m_VertexBuffer, _iFrame);
		TMBTGenerateQuartiles(_pTriCore, lEigen[0], lTempNodes[iCurrent]->m_liTriangles, lQuart, m_VertexBuffer, _iFrame);

		for(i = 0;i < 8;i++)
		{
			lTTestNodes[i][0] = MNew(CTMTempBoxNode);
			lTTestNodes[i][1] = MNew(CTMTempBoxNode);

			if( TMBTSpawnChildren(_pTriCore, pNode, lTempNodes[iCurrent], lTestNodes[i], lTTestNodes[i], lQuart, m_Cluster, m_VertexBuffer,
				i & 1, 4 - (i & 1), (i < 2) ? 0.0f : M_Pow(2.0f, (fp32)((i >> 1) - 1)), m_InvBoundRadius, _iFrame) )
			{
				fp32 CurrentErr = TMBTGetNodeValues(lTestNodes[i], lTTestNodes[i]);
				if( CurrentErr < ErrVal )
				{
					iBestPair = i;
					ErrVal = CurrentErr;
				}
			}
			else
			{
				nFail++;
			}
		}

		if( (iBestPair < 8) && (nFail < 6) )
		{
			lTempNodes[nNodes] = lTTestNodes[iBestPair][0];
			lTempNodes[nNodes+1] = lTTestNodes[iBestPair][1];
			m_lBoxNodes[nNodes] = lTestNodes[iBestPair][0];
			m_lBoxNodes[nNodes+1] = lTestNodes[iBestPair][1];
		
		//*/
		/*

		Old code used before auto-detection of "best" nodes was enabled

		//Create childnodes
		lTempNodes[nNodes] = MNew(CTMTempSphereNode);
		lTempNodes[nNodes+1] = MNew(CTMTempSphereNode);
		
		if( TMSTSpawnChildren(pNode,lTempNodes[iCurrent],&m_lSphereNodes[nNodes],
							  &lTempNodes[nNodes],m_Cluster,1,3,2.0f,m_BoundRadius,_iFrame) )
		{
		//*/

			lTempNodes[iCurrent]->m_liTriangles.Clear();
			pNode->m_iChildren = nNodes;

			//Tag children as leafnodes
			m_lBoxNodes[nNodes].m_iChildren = 0xFFFF;
			lTempNodes[nNodes]->m_Generation = lTempNodes[iCurrent]->m_Generation+1;
			m_lBoxNodes[nNodes+1].m_iChildren = 0xFFFF;
			lTempNodes[nNodes+1]->m_Generation = lTempNodes[iCurrent]->m_Generation+1;

			//Mark for "recursion"
			if( lTempNodes[nNodes+1]->m_Generation < _MaxGeneration )
			{
				if( lTempNodes[nNodes]->m_liTriangles.Len() >= _MinFaces ) lProcess.Add(nNodes);
				if( lTempNodes[nNodes+1]->m_liTriangles.Len() >= _MinFaces ) lProcess.Add(nNodes+1);
			}

			nNodes+=2;
		}
		else
		{
			pNode->m_iChildren = 0xFFFF;
		}
	}

	m_lBoxNodes.SetLen(nNodes);

	/*
	// Old code intended to avoid keeping an index-list
	// Unfortunately, this couldn't be avoided
	TThinArray<uint16> lTriangleIndices;
	TThinArray<CTM_Triangle> lTriangles;
	lTriangleIndices.SetLen(nTri);
	lTriangles.SetLen(nTri);
	memcpy(lTriangles.GetBasePtr(),m_Cluster.m_lTriangles.GetBasePtr(),lTriangles.ListSize());
	//*/

	m_liBTTriangles.SetLen( nTri + 32*nNodes );
	uint16 iFaceStart = 0;
	for(i = 0;i < nNodes;i++)
	{
		pNode = &m_lBoxNodes[i];
		pNode->m_iiTriangles = iFaceStart;
		pNode->m_nTriangles = lTempNodes[i]->m_liTriangles.Len();

		TArray<uint16> lTriMasked[16];

		for(int j=0;j < pNode->m_nTriangles;j++)
		{
			int k;
			//*
			//Determine cluster
			uint16 iC = 0;
			uint16 iTri = lTempNodes[i]->m_liTriangles[j];
			while( iTri >= m_lTriangleCluster[iC] )
			{
				iTri -= m_lTriangleCluster[iC];
				iC ++;
			}

			CTM_Cluster* pC = _pTriCore->GetCluster(m_lRenderData[iC].m_iShadowCluster);
			//*/

			/*
			//Clusterfinding- the hard way =P
			CVec3Dfp32 Vec[3];
			CTM_Triangle &Tri = m_Cluster.m_lTriangles[lTempNodes[i]->m_liTriangles[j]];
			Vec[0] = m_Cluster.GetVertexPtr(0)[Tri.m_iV[0]];
			Vec[1] = m_Cluster.GetVertexPtr(0)[Tri.m_iV[1]];
			Vec[2] = m_Cluster.GetVertexPtr(0)[Tri.m_iV[2]];
			CTM_VertexBuffer *pC = NULL;
			for(k = 0;k < pMesh->GetNumClusters();k++)
			{
				pC = pMesh->GetVertexBuffer(k);
				int l;
				for(l = 0;l < pC->GetNumTriangles();l++)
				{
					CTM_Triangle &Tri2 = pC->m_lTriangles[l];
					if( Vec[0].AlmostEqual( pC->GetVertexPtr(0)[Tri2.m_iV[0]],0.01f ) &&
						Vec[1].AlmostEqual( pC->GetVertexPtr(0)[Tri2.m_iV[1]],0.01f ) &&
						Vec[2].AlmostEqual( pC->GetVertexPtr(0)[Tri2.m_iV[2]],0.01f ) ) break;
				}
				if( l != pC->GetNumTriangles() ) break;
			}
			if( k == pMesh->GetNumClusters() ) continue;
			//*/

			//Find correct mask
			for(k=0;k < 32;k++)
			{
				if( lTriMasked[k].Len() == 0 )
				{
					lTriMasked[k].SetLen(2);
					lTriMasked[k].GetBasePtr()[0] = pC->m_OcclusionIndex;
				}
				if( lTriMasked[k].GetBasePtr()[0] == pC->m_OcclusionIndex )
				{
					lTriMasked[k].Add(lTempNodes[i]->m_liTriangles[j]);
					break;
				}
			}
		}

		//Copy triangles
		uint16 iPos = 0;
		for(int j=0;j < 32;j++)
		{
			if( lTriMasked[j].Len() == 0 ) break;
			memcpy(m_liBTTriangles.GetBasePtr() + iFaceStart + iPos,lTriMasked[j].GetBasePtr(),lTriMasked[j].ListSize());
			m_liBTTriangles[ iFaceStart + iPos + 1 ] = lTriMasked[j].Len() - 2;
			iPos += lTriMasked[j].Len();
			pNode->m_nTriangles += 2;
		}
		iFaceStart += pNode->m_nTriangles;
	}
	m_liBTTriangles.SetLen( iFaceStart );
	/*
	for(i = 0;i < m_Cluster.m_lEdgeTris.Len();i++)
	{
		CTM_EdgeTris &Edge = m_Cluster.m_lEdgeTris[i];
		if( Edge.m_iTri[0] < nTri ) Edge.m_iTri[0] = lTriangleIndices[Edge.m_iTri[0]];
		if( Edge.m_iTri[1] < nTri ) Edge.m_iTri[1] = lTriangleIndices[Edge.m_iTri[1]];
	}

	DTriMeshArray<CTM_TriEdges> lTriEdges;
	lTriEdges.SetLen(m_Cluster.m_lTriEdges.Len());
	memcpy(lTriEdges.GetBasePtr(),m_Cluster.m_lTriEdges.GetBasePtr(),lTriEdges.ListSize());
	for(i = 0;i < m_Cluster.m_lTriEdges.Len();i++)
	{
		m_Cluster.m_lTriEdges[i] = lTriEdges[lTriangleIndices[i]];
	}
	//*/
}

//ifndef PLATFORM_CONSOLE
#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	LineSegment-box intersection

Parameters:		
_Middle:		Middle point of line
_Dir:			Direction of line
_Bounds:		Bounding box of line
_pBox:			Box in a CTM_ShadowData::CBTNode

Returns:		true on intersection, false otherwise

Comments:		This function uses SAT and should be slightly faster than
				the one in TBox; mostly since this one does not return
				intersection point
\*____________________________________________________________________*/ 
M_INLINE static bool TMBTLineBoxIntersect(const CVec3Dfp32 &_Middle,const CVec3Dfp32 &_Dir,const CBox3Dfp32 &_Bounds,
											 const uint8 * _pBox)
{
	//Ordinary aabb-test, any principal axis can be a separating axis
	if( (_Bounds.m_Min.k[0] > _pBox[0] + _pBox[3]) || (_Bounds.m_Min.k[1] > _pBox[1] + _pBox[4]) || 
		(_Bounds.m_Min.k[2] > _pBox[2] + _pBox[5]) || (_Bounds.m_Max.k[0] < _pBox[0] - _pBox[3]) || 
		(_Bounds.m_Max.k[1] < _pBox[1] - _pBox[4]) || (_Bounds.m_Max.k[2] < _pBox[2] - _pBox[5]) )
	{
		return false;
	}

	//The crossproducts of the box's axes (the principal axes) and the direction between
	//the box and the line can all be separating axes
	CVec3Dfp32 Delta;
	Delta.k[0] = (_pBox[0]) - _Middle.k[0];
	Delta.k[1] = (_pBox[1]) - _Middle.k[1];
	Delta.k[2] = (_pBox[2]) - _Middle.k[2];

	//X- axis
	fp32 Proj = _pBox[4]*Abs(_Dir.k[2]) + _pBox[5]*Abs(_Dir.k[1]);
	if( Abs(Delta.k[1]*_Dir.k[2] - Delta.k[2]*_Dir.k[1]) > Proj ) return false;

	//Y- axis
	Proj = _pBox[3]*Abs(_Dir.k[2]) + _pBox[5]*Abs(_Dir.k[0]);
	if( Abs(Delta.k[2]*_Dir.k[0] - Delta.k[0]*_Dir.k[2]) > Proj ) return false;

	//Z- axis
	Proj = _pBox[4]*Abs(_Dir.k[0]) + _pBox[3]*Abs(_Dir.k[1]);
	if( Abs(Delta.k[0]*_Dir.k[1] - Delta.k[1]*_Dir.k[0]) > Proj ) return false;

	return true;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Line-Trimesh intersection test

Parameters:		
_Start:		Start of line
_End:		End of line
_iFrame:	Frame (note that it must be same as when generated)

Returns:	true on intersection, false otherwise
\*____________________________________________________________________*/ 
bool CTM_ShadowData::IntersectLine(CTriangleMeshCore* _pTriCore, const CVec3Dfp32 & _Start,const CVec3Dfp32 & _End,uint16 _Mask,int _iFrame /* = 0 */)
{
	CVec3Dfp32 Pos,Dir;
	
	// If this test is to be performed externally, remove!
	if( !m_Cluster.m_BoundBox.IntersectLine(_Start,_End,Pos) )
		return false;

	CVec3Dfp32 Start = (_Start - m_Cluster.m_BoundBox.m_Min) * m_InvBoundRadius;
	CVec3Dfp32 End = (_End - m_Cluster.m_BoundBox.m_Min) * m_InvBoundRadius;
	Dir = End - Start;
	CVec3Dfp32 Middle = (End + Start) * 0.5f;

	CBox3Dfp32 LineBox;
	LineBox.m_Max = Start;
	LineBox.m_Min = Start;
	LineBox.Expand(End);


	CVec3Dfp32 * pVerts = m_VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);
	uint16 iCurrent = 0;

	uint16 lGeneration[16];
	uint8 iGeneration = 0;

	uint16 * piTriangles = m_liBTTriangles.GetBasePtr();

	do 
	{
		CBTNode &node = m_lBoxNodes[iCurrent];

		if( !TMBTLineBoxIntersect(Middle,Dir,LineBox,node.m_lBox) )
		{
			while( (iGeneration > 0) && (lGeneration[iGeneration-1] == 0) )
				iGeneration--;
			if(!iGeneration) break;
			iCurrent = lGeneration[iGeneration-1];
			lGeneration[iGeneration-1] = 0;
			continue;
		}

		if( node.m_iChildren == 0xFFFF )
		{
			int i = node.m_iiTriangles;
			while( i < node.m_iiTriangles + node.m_nTriangles )
			{
				//Masked
				if( _Mask & piTriangles[i] )
				{
					i += piTriangles[i+1] + 2;
					continue;
				}

				for(int j = 0;j < piTriangles[i+1];j++)
				{
					CTM_Triangle &Tri = m_VertexBuffer.m_lTriangles[piTriangles[i + j + 2]];
					if( IntersectTriangle(_Start, _End, pVerts[Tri.m_iV[0]], pVerts[Tri.m_iV[1]], pVerts[Tri.m_iV[2]]) )
					{
						return true;
					}
				}
				i += piTriangles[i+1] + 2;
			}

			while( (iGeneration > 0) && (lGeneration[iGeneration - 1] == 0) )
				iGeneration--;
			if(!iGeneration) break;
			iCurrent = lGeneration[iGeneration-1];
			lGeneration[iGeneration-1] = 0;
		}
		else
		{
			iCurrent = node.m_iChildren;
			lGeneration[iGeneration] = node.m_iChildren+1;
			iGeneration++;
		}
	} 
	while(iGeneration);

	return false;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Line-Trimesh intersection test

Parameters:		
_Start:		Start of line
_End:		End of line
_Pos:		Hit position
_Plane:		Plane of collision
_Mask:		Occlusion mask
_piCluster:	Cluster hit
_iFrame:	Frame (note that it must be same as when generated)

Returns:	true on intersection, false otherwise
\*____________________________________________________________________*/ 
bool CTM_ShadowData::IntersectLine(CTriangleMeshCore* _pTriCore, const CVec3Dfp32& _Start, const CVec3Dfp32& _End, CVec3Dfp32& _Pos,
								   CPlane3Dfp32& _Plane, uint16 _Mask, int* _piCluster, int _iFrame /* = 0 */)
{
	CVec3Dfp32 Pos,Dir;
	if( !m_Cluster.m_BoundBox.IntersectLine(_Start,_End,Pos) )
		return false;
	CVec3Dfp32 Start = (_Start - m_Cluster.m_BoundBox.m_Min) * m_InvBoundRadius;
	CVec3Dfp32 End = (_End - m_Cluster.m_BoundBox.m_Min) * m_InvBoundRadius;
	Dir = End - Start;
	CVec3Dfp32 Middle = (End + Start) * 0.5f;

	CBox3Dfp32 LineBox;
	LineBox.m_Max = Start;
	LineBox.m_Min = Start;
	LineBox.Expand(End);

	//Find arbitrary principal axis where Dir.Axis != 0
	uint8 iAxis = (Dir.k[0] != 0.0f) ? 0 : ((Dir.k[1] != 0.0f) ? 1 : 2);
	_Pos = _End;

	uint16 iCurrent = 0;

	uint16 lGeneration[16];
	uint16 iTriHit = 0xFFFF;
	uint8 iGeneration = 0;
	bool bRet = false;

	TAP_RCD<const uint16> piTriangles = m_liBTTriangles;
	TAP_RCD<const CTM_Triangle> pTriangles = m_VertexBuffer.m_lTriangles;
	TAP_RCD<const CBTNode> pBoxNodes = m_lBoxNodes;
	TAP_RCD<const CVec3Dfp32> pVerts;
	pVerts.m_pArray = m_VertexBuffer.GetVertexPtr(_pTriCore, _iFrame);
	pVerts.m_Len = m_VertexBuffer.GetNumVertices(_pTriCore);

	do 
	{
		const CBTNode& node = pBoxNodes[iCurrent];

		if (!TMBTLineBoxIntersect(Middle, Dir, LineBox, node.m_lBox))
		{
			while( (iGeneration > 0) && (lGeneration[iGeneration-1] == 0) )
				iGeneration--;
			if(!iGeneration) break;
			iCurrent = lGeneration[iGeneration-1];
			lGeneration[iGeneration-1] = 0;
			continue;
		}

		if (node.m_iChildren == 0xFFFF)
		{
			int i = node.m_iiTriangles;
			int iEnd = node.m_iiTriangles + node.m_nTriangles;
			while (i < iEnd)
			{
				uint nTri = piTriangles[i+1];

				//Masked
				if (!(_Mask & (1 << piTriangles[i])))
				{
					i += (nTri + 2);
					continue;
				}

				for (uint j = 0; j < nTri; j++)
				{
					uint iTriangle = piTriangles[i+j+2];
					const CTM_Triangle& Tri = pTriangles[iTriangle];
					uint iV0 = Tri.m_iV[0];
					uint iV1 = Tri.m_iV[1];
					uint iV2 = Tri.m_iV[2];
					if (IntersectTriangle(_Start, _End, pVerts[iV0], pVerts[iV1], pVerts[iV2], Pos))
					{
						if (Pos.DistanceSqr(_Start) < _Pos.DistanceSqr(_Start))
						{
							_Pos = Pos;
							iTriHit = iTriangle;
						}
						bRet = true;
					}
				}
				i += nTri + 2;
			}

#define TMST_EXACT_INTERSECTION_PT
#ifndef TMST_EXACT_INTERSECTION_PT
			if( bRet ) return true;		
#endif

			while ((iGeneration > 0) && (lGeneration[iGeneration-1] == 0))
				iGeneration--;
			if (!iGeneration) break;
			iCurrent = lGeneration[iGeneration-1];
			lGeneration[iGeneration-1] = 0;
		}
		else
		{

#ifndef TMST_EXACT_INTERSECTION_PT
			uint8 *pC1 = m_lBoxNodes[node.m_iChildren].m_lBox;
			uint8 *pC2 = m_lBoxNodes[node.m_iChildren+1].m_lBox;
			if( Start.k[iAxis] - pC1[iAxis] <
				Start.k[iAxis] - pC2[iAxis] )
			{
				iCurrent = node.m_iChildren;
				lGeneration[iGeneration] = node.m_iChildren+1;
			}
			else
			{
				iCurrent = node.m_iChildren+1;
				lGeneration[iGeneration] = node.m_iChildren;
			}
#else
			iCurrent = node.m_iChildren;
			lGeneration[iGeneration] = node.m_iChildren+1;
#endif
			iGeneration++;
		}
	} 
	while(iGeneration);

	if (bRet)
	{
		const CTM_Triangle& Tri = pTriangles[iTriHit];
		_Plane.Create(pVerts[Tri.m_iV[0]], pVerts[Tri.m_iV[2]], pVerts[Tri.m_iV[1]]);
		
		if (_piCluster)
		{
			//Find the right cluster
			uint16 nTri = iTriHit,iC = 0;
			while( nTri >= m_lTriangleCluster[iC] )
			{
				nTri -= m_lTriangleCluster[iC];
				iC++;
			}

			//Indexed into array
			*_piCluster = m_lRenderData[iC].m_iRenderCluster;
		}
	}

	return bRet;
}

#ifndef M_RTM

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Boxtree validity test
Returns:	true on valid boxtree, false otherwise
\*____________________________________________________________________*/ 
bool CTM_ShadowData::IsBoxTreeValid() const
{
	if( (m_lBoxNodes.Len() == 0) || (m_liBTTriangles.Len() == 0) ) return false;

	//Early out... not that we really need it
	if( m_liBTTriangles.Len() < m_lTriangle.Len() + 2 ) return false;

	const uint16 * piTriangles = m_liBTTriangles.GetBasePtr();

	for(uint16 i = 0;i < m_lBoxNodes.Len();i++)
	{
		const CBTNode &node = m_lBoxNodes[i];

		int j = node.m_iiTriangles;
		
		uint16 nProcessed = 0;
		while( j < node.m_iiTriangles + node.m_nTriangles )
		{
			//Make sure the mask is valid
			if( j + piTriangles[j+1] > m_liBTTriangles.Len() ) return false;

			//Make sure every triangle is valid
			for(int k = 0;k < piTriangles[j+1];k++)
			{
				uint16 iTri = piTriangles[j+k+2];
				if( (iTri >= m_lTriangle.Len()) && (m_lTriangle.Len() > 0) ) return false;
				nProcessed++;
			}
			nProcessed += 2;
			j += piTriangles[j+1] + 2;
		}

		//Make sure we got a valid set
		if( nProcessed != node.m_nTriangles ) return false;
	}
	
	return true;
}

#endif


void CTM_ShadowData::CBTNode::Read(CCFile * _pF,int _Version)
{
	switch(_Version)
	{

	case 0x0100:
		{
			_pF->Read(m_lBox,6);
			_pF->ReadLE(m_iChildren);
			_pF->ReadLE(m_iiTriangles);
			_pF->ReadLE(m_nTriangles);
		}

	default:
		Error_static("CTM_ShadowData::CBTNode::Read",CStrF("Unsupported version %.4",_Version));
	}
}


#ifndef PLATFORM_CONSOLE

void CTM_ShadowData::CBTNode::Write(CCFile* _pF)
{
	_pF->Write(m_lBox,6);
	_pF->WriteLE(m_iChildren);
	_pF->WriteLE(m_iiTriangles);
	_pF->WriteLE(m_nTriangles);
}

#endif


void CTM_ShadowData::CBTNode::SwapLE()
{
	::SwapLE(m_iChildren);
	::SwapLE(m_iiTriangles);
	::SwapLE(m_nTriangles);
}

//-------------------------------------------------------------------------------------------------


void CTM_ShadowData::Read(CTriangleMeshCore* _pTriCore, CDataFile* _pDFile, int _Version)
{
	if (_pDFile->GetNext("DATA"))
	{
		CCFile* pF = _pDFile->GetFile();
		int32 Len;
		pF->ReadLE(Len);
		m_lEdge.SetLen(Len);
		pF->ReadLE((uint16 *)m_lEdge.GetBasePtr(), Len * 2);
		pF->ReadLE(Len);
		m_lEdgeCluster.SetLen(Len);
		pF->ReadLE(m_lEdgeCluster.GetBasePtr(), Len);
		pF->ReadLE(Len);
		m_lTriangle.SetLen(Len);
		for (int i = 0; i < Len; ++i)
		{
			m_lTriangle[i].Read(_pDFile);
		}
		pF->ReadLE(Len);
		m_lTriangleCluster.SetLen(Len);
		pF->ReadLE(m_lTriangleCluster.GetBasePtr(), Len);
		pF->ReadLE(Len);
		m_lRenderData.SetLen(Len);
		if (_Version >= 0x0303)
		{
			for (int i = 0; i < Len; ++i)
			{
				pF->ReadLE(m_lRenderData[i].m_iShadowCluster);
				pF->ReadLE(m_lRenderData[i].m_iRenderCluster);
			}
		}
		else
		{
			int i;
			for (i = 0; i < Len; ++i)
			{
				pF->ReadLE(m_lRenderData[i].m_iShadowCluster);
				m_lRenderData[i].m_iRenderCluster = m_lRenderData[i].m_iShadowCluster;
			}
		}
	}

	_pDFile->PushPosition();
	if(_pDFile->GetNext("CLUSTERDATA") && _pDFile->GetSubDir())
	{
		if(!_pDFile->GetNext("CLUSTER"))
			Error_static("CTM_ShadowData::Read", "Missing data 1");
		m_Cluster.Read(_pDFile, _pDFile->GetUserData2());

		if(!_pDFile->GetNext("VERTEXBUFFER"))
			Error_static("CTM_ShadowData::Read", "Missing data 1");
		{
			int nVB = _pDFile->GetUserData();
			int Version = _pDFile->GetUserData2();
			_pDFile->GetSubDir();
			m_VertexBuffer.Read(_pTriCore, _pDFile, Version);
			_pDFile->GetParent();
		}
	}
	else
	{
		_pDFile->PopPosition();
		_pDFile->PushPosition();
		if (_pDFile->GetNext("CLUSTER") && _pDFile->GetSubDir())
		{
			m_VertexBuffer.ReadOld(_pTriCore, &m_Cluster, _pDFile, false);
		}
	}
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	if (_pDFile->GetNext("BOXTREE") && _pDFile->GetSubDir())
	{
		CCFile* pF = _pDFile->GetFile();

		_pDFile->PushPosition();
		if (_pDFile->GetNext("INVRADIUS"))
		{
			pF->ReadLE(m_InvBoundRadius);
		}
		else
		{
			Error_static("CTM_ShadowData::Read", "no INVRADIUS node");
		}
		_pDFile->PopPosition();

		_pDFile->PushPosition();
		if (_pDFile->GetNext("NODES"))
		{
			int nBoxes = _pDFile->GetUserData();
			int Version = _pDFile->GetUserData2();
			m_lBoxNodes.SetLen(nBoxes);
			CBTNode * pNodes = m_lBoxNodes.GetBasePtr();
			if (Version == CTM_CLSTREE_VERSION)
			{
				pF->Read(pNodes, m_lBoxNodes.ListSize());
				for(int i = 0;i < nBoxes;i++)
					pNodes[i].SwapLE();
			}
			else
			{
				for(int i = 0;i < nBoxes;i++)
					pNodes[i].Read(pF,Version);
			}
		}
		_pDFile->PopPosition();

		_pDFile->PushPosition();
		if( _pDFile->GetNext("TRIANGLES") )
		{
			int nTriangles = _pDFile->GetUserData();
			m_liBTTriangles.SetLen(nTriangles);
			pF->ReadLE(m_liBTTriangles.GetBasePtr(),nTriangles);
		}
		_pDFile->PopPosition();

		if( (!m_liBTTriangles.Len()) && m_lBoxNodes.Len() )
		{
			Error_static("CTM_ShadowData::Read", "ShadowData cls-tree contained nodes but no triangles");
		}

#ifndef M_RTM
		if( !IsBoxTreeValid() )
		{
			CStr Warning = CStrF("§cf80WARNING: Model '%s' - Boxtree is corrupted, removing!", _pTriCore->m_FileName.Str());
			ConOutL(Warning.Str());
			LogFile(Warning.Str());
			m_lBoxNodes.Clear();
			m_liBTTriangles.Clear();
		}
#endif
	}
	_pDFile->PopPosition();

//Autodetermine render clusters
	if( _Version < 0x0303 )
	{
		_pDFile->PushPosition();
		CCFile* pFile = _pDFile->GetFile();

		uint16 Len = m_lRenderData.Len();
		uint16 i;
		uint16 nRenderClusters = 0xFFFF;
		for (i = 0;i < Len; i++) nRenderClusters = Min(nRenderClusters,m_lRenderData[i].m_iShadowCluster);

		for (i = 0;i < Len; i++)
		{
			CRenderData &RD = m_lRenderData[i];
			CTM_VertexBuffer * pSC = _pTriCore->GetVertexBuffer(m_lRenderData[i].m_iShadowCluster);
			uint16 nTri = pSC->GetNumTriangles(_pTriCore, pFile);

			uint16 nEqual = 0;
			uint16 iEqual = 0;
			uint16 j;
			for(j = 0;j < nRenderClusters;j++)
			{
				CTM_VertexBuffer * pC = _pTriCore->GetVertexBuffer(j);
				if( pC->GetNumTriangles(_pTriCore, pFile) == nTri )
				{
					iEqual = (nEqual) ? iEqual : j;
					nEqual++;
				}
			}

			//Only one cluster with same amount of triangles found, no need for further comparison
			if( nEqual < 2 )
			{
				RD.m_iRenderCluster = iEqual;
				continue;
			}

			//Several clusters, look through all
			uint16 * pTri = pSC->GetTriangles(_pTriCore, pFile);
			CVec3Dfp32 * pV = pSC->GetVertexPtr(_pTriCore, 0, pFile);
			for( j = iEqual; j < nRenderClusters;j++ )
			{
				CTM_VertexBuffer * pC = _pTriCore->GetVertexBuffer(j);
				if( pC->GetNumTriangles(_pTriCore, pFile) != nTri ) continue;
				
				uint16 * pRTri = pC->GetTriangles(_pTriCore, pFile);
				CVec3Dfp32 * pRVec = pC->GetVertexPtr(_pTriCore, 0, pFile);

				uint32 k;
				for(k = 0; k < nTri * 3;k++)
				{
					if( !pRVec[pRTri[k]].AlmostEqual(pV[pTri[k]],0.001f) ) break;
				}
				if( k < nTri * 3 ) continue;
				
				RD.m_iRenderCluster = j;
				break;
			}
		}
		_pDFile->PopPosition();
	}
}

#ifndef PLATFORM_CONSOLE
void CTM_ShadowData::Write(CTriangleMeshCore* _pTriCore, CDataFile* _pDFile)
{
	_pDFile->BeginEntry("DATA");

	CCFile* pF = _pDFile->GetFile();
	int32 Len = m_lEdge.Len();
	pF->WriteLE(Len);
	pF->WriteLE((uint16 *)m_lEdge.GetBasePtr(), Len * 2);
	Len = m_lEdgeCluster.Len();
	pF->WriteLE(Len);
	pF->WriteLE(m_lEdgeCluster.GetBasePtr(), Len);
	Len = m_lTriangle.Len();
	pF->WriteLE(Len);
	int i;
	for (i = 0; i < Len; ++i)
	{
		m_lTriangle[i].Write(_pDFile);
	}
	Len = m_lTriangleCluster.Len();
	pF->WriteLE(Len);
	pF->WriteLE(m_lTriangleCluster.GetBasePtr(), Len);
	Len = m_lRenderData.Len();
	pF->WriteLE(Len);
	for (i = 0; i < Len; ++i)
	{
		pF->WriteLE(m_lRenderData[i].m_iShadowCluster);
		pF->WriteLE(m_lRenderData[i].m_iRenderCluster);
	}

	_pDFile->EndEntry(0);

	_pDFile->BeginEntry("CLUSTERDATA");
	_pDFile->EndEntry(0);

	_pDFile->BeginSubDir();
	{
		_pDFile->BeginEntry("CLUSTER");
		m_Cluster.Write(_pDFile);
		_pDFile->EndEntry(1, CTM_CLUSTER_VERSION);
		
		_pDFile->BeginEntry("VERTEXBUFFER");
		_pDFile->EndEntry(1, CTM_VERTEXBUFFER_VERSION);
		{
			_pDFile->BeginSubDir();
			m_VertexBuffer.Write(_pTriCore, _pDFile);
			_pDFile->EndSubDir();
		}
	}
	_pDFile->EndSubDir();

	if( m_lBoxNodes.Len() )
	{
		_pDFile->BeginEntry("BOXTREE");
		_pDFile->EndEntry(CTM_CLSTREE_VERSION);
		_pDFile->BeginSubDir();

		_pDFile->BeginEntry("INVRADIUS");
		pF->WriteLE(m_InvBoundRadius);
		_pDFile->EndEntry(CTM_CLSTREE_VERSION);

		_pDFile->BeginEntry("NODES");
		for(i=0;i < m_lBoxNodes.Len();i++)
			m_lBoxNodes[i].Write(pF);
		_pDFile->EndEntry(m_lBoxNodes.Len(),CTM_CLSTREE_VERSION);

		_pDFile->BeginEntry("TRIANGLES");
		pF->WriteLE(m_liBTTriangles.GetBasePtr(),m_liBTTriangles.Len());
		_pDFile->EndEntry(m_liBTTriangles.Len(),CTM_CLSTREE_VERSION);

		_pDFile->EndSubDir();
	}
}
#endif

// -------------------------------------------------------------------
CTM_Cluster::CTM_Cluster()
{
	m_iSurface = 0;
	m_iIBOffset = 0;
	m_nIBPrim = 0;
	m_iVBVert = 0;
	m_nVBVert = 0;
	m_iVB = 0;
	m_iIB = 0;
	m_Flags = 0;
	m_OcclusionIndex = 0;
	m_iSharedMatrixMap = -1;
	m_BoundBox.m_Min = 0;
	m_BoundBox.m_Max = 0;
	m_BoundRadius = 0;
}

int CTM_Cluster::GetNumBDMatrixMap(CTriangleMeshCore* _pTriCore)
{
	if (m_iSharedMatrixMap >= 0)
	{
		CTM_Cluster *pCluster = _pTriCore->GetCluster(m_iSharedMatrixMap);
		if(!pCluster)
		{
			ConOutL("(CTM_Cluster::GetNumBDMatrixMap) Invalid shared cluster");
			return 0;
		}
		return pCluster->GetNumBDMatrixMap(_pTriCore);
	}

	int Len = m_lBDMatrixMap.Len();
	return Len;
}

uint16* CTM_Cluster::GetBDMatrixMap(CTriangleMeshCore* _pTriCore)
{
	if (m_iSharedMatrixMap >= 0)
		return _pTriCore->GetCluster(m_iSharedMatrixMap)->GetBDMatrixMap(_pTriCore);

	return m_lBDMatrixMap.GetBasePtr();
}
// -------------------------------------------------------------------
CTM_VertexBuffer::CTM_VertexBuffer()
{
	MAUTOSTRIP(CTM_VertexBuffer_ctor, MAUTOSTRIP_VOID);
	m_VBID = 0;

	m_VBFlags = 0;
//	m_iSurface = 0;

	m_FilePosBoneDeform = 0;
	m_FilePosTriangles = 0;
	m_FilePosPrimitives = 0;
	m_FilePosEdges = 0;
	m_FilePosEdgeTris = 0;
	m_FilePosTriEdges = 0;
	m_bDelayLoad = 0;
	m_bSave_Vertices = 0;
	m_bSave_Normals = 0;
	m_bSave_TangentU = 0;
	m_bSave_TangentV = 0;
	m_bSave_TVertices = 0;
	m_bSave_BoneDeform = 0;
	m_bSave_BoneDeformMatrixMap = 0;
	m_bSave_Triangles = 0;
	m_bSave_Primitives = 0;
	m_bSave_Edges = 0;
	m_bSave_EdgeTris = 0;
	m_bSave_TriEdges = 0;
	m_bSave_VBBoneDeform = 0;
	m_bHaveBoneMatrixMap = 0;
	m_bHaveBones = 0;
//	m_iSharedMatrixMap = -1;
//	m_OcclusionIndex = 0;

	m_TotalDuration = 0; // Added by Mondelore.

//	m_BoundRadius = 0;
//	m_BoundBox.m_Min = 0;
//	m_BoundBox.m_Max = 0;
}

#ifndef PLATFORM_CONSOLE
spCTM_VertexBuffer CTM_VertexBuffer::DuplicateVertices(CTriangleMeshCore* _pTriCore, const int32* _piV, int _nV)
{
	spCTM_VertexBuffer spVBNew = MNew(CTM_VertexBuffer);
	if (!spVBNew)
		MemError("Cluster_SplitByMaxBones");

	if (m_lBDInfluence.Len())
	{
		if (_piV)
		{
			int nBDI = 0;
			CTM_BDVertexInfo* pBDVI = m_lBDVertexInfo.GetBasePtr();
			{
				for(int i = 0; i < _nV; i++)
					nBDI += pBDVI[_piV[i]].m_nBones;
			}

			spVBNew->m_lBDInfluence.SetLen(nBDI);
			spVBNew->m_lBDVertexInfo.SetLen(_nV);

			int iBDI = 0;
			for(int i = 0; i < _nV; i++)
			{
				int iV = _piV[i];
				const CTM_BDVertexInfo& BDVISrc = m_lBDVertexInfo[iV];
				CTM_BDVertexInfo BDVI = BDVISrc;
				BDVI.m_iBoneInfluence = iBDI;

				for(int b = 0; b < BDVI.m_nBones; b++)
					spVBNew->m_lBDInfluence[iBDI++] = m_lBDInfluence[BDVISrc.m_iBoneInfluence + b];

				spVBNew->m_lBDVertexInfo[i] = BDVI;
			}

			if (iBDI != nBDI)
				Error("Duplicate", CStrF("Model '%s' - Internal error.", _pTriCore->GetMeshName()));
		}
		else
		{
			spVBNew->m_lBDInfluence.Add(&m_lBDInfluence);
			spVBNew->m_lBDVertexInfo.Add(&m_lBDVertexInfo);
		}

	}

//	spVBNew->m_lBDMatrixMap = m_lBDMatrixMap;
//	spVBNew->m_iSharedMatrixMap = m_iSharedMatrixMap;
//	spVBNew->m_OcclusionIndex = m_OcclusionIndex;

	int nVF = GetNumFrames();
	int nTVF = GetNumTFrames();
	spVBNew->SetVFrames(nVF);
	spVBNew->SetTVFrames(nTVF);

	for(int iVF = 0; iVF < nVF; iVF++)
	{
		spVBNew->m_lVFrames[iVF] = *(m_lVFrames[iVF].Duplicate(_piV, _nV));
//		m_lVFrames[iVF].Duplicate(spVBNew->m_lVFrames[iVF], _piV, _nV);
	}

	spVBNew->m_lTVFrames.SetLen(nTVF);
	for(int iTVF = 0; iTVF < nTVF; iTVF++)
	{
		spVBNew->m_lTVFrames[iTVF] = *(m_lTVFrames[iTVF].Duplicate(_piV, _nV));
//		m_lTVFrames[iTVF].Duplicate(spVBNew->m_lTVFrames[iTVF], _piV, _nV);
	}


	spVBNew->m_VBFlags = m_VBFlags;
//	spVBNew->m_iSurface = m_iSurface;
//	spVBNew->m_BoundRadius = m_BoundRadius;
//	spVBNew->m_BoundBox = m_BoundBox;

	return spVBNew;
}

spCTM_VertexBuffer CTM_VertexBuffer::Duplicate(CTriangleMeshCore* _pTriCore)
{
	spCTM_VertexBuffer spVBNew = DuplicateVertices(_pTriCore);
	spVBNew->m_lTriangles.Add(&m_lTriangles);
	spVBNew->m_lTriangles32.Add(&m_lTriangles32);
	spVBNew->m_lShadowGroup.Add(&m_lShadowGroup);
	spVBNew->m_lPrimitives.Add(&m_lPrimitives);

	return spVBNew;
}
#endif

#ifndef PLATFORM_CONSOLE
void CTM_VertexBuffer::CreateTriFromTri32()
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateTriFromTri32, MAUTOSTRIP_VOID);

	int nTri = m_lTriangles32.Len();
	m_lTriangles.SetLen(nTri);
	CTM_Triangle* pT16 = m_lTriangles.GetBasePtr();
	CTM_Triangle32* pT32 = m_lTriangles32.GetBasePtr();

	for(int i = 0; i < nTri; i++)
	{
		M_ASSERT(pT32->m_iV[0] < 0x10000, "!");
		M_ASSERT(pT32->m_iV[1] < 0x10000, "!");
		M_ASSERT(pT32->m_iV[2] < 0x10000, "!");
		pT16->m_iV[0] = pT32->m_iV[0];
		pT16->m_iV[1] = pT32->m_iV[1];
		pT16->m_iV[2] = pT32->m_iV[2];
		pT16++;
		pT32++;
	}
}

void CTM_VertexBuffer::SetVFrames(int _nFrames)
{
	MAUTOSTRIP(CTM_VertexBuffer_SetVFrames, MAUTOSTRIP_VOID);
	int nV = 0;
	if (m_lVFrames.Len()) 
		nV = m_lVFrames[0].m_lVertices.Len();

	m_lVFrames.SetLen(_nFrames);
	for(int i = 0; i < _nFrames; i++)
		if (m_lVFrames[i].m_lVertices.Len() != nV)
		{
			m_lVFrames[i].m_lVertices.SetLen(nV);
			m_lVFrames[i].m_lNormals.SetLen(nV);
		}
}

void CTM_VertexBuffer::SetTVFrames(int _nFrames)
{
	MAUTOSTRIP(CTM_VertexBuffer_SetTVFrames, MAUTOSTRIP_VOID);
	int nV = 0;
	if (m_lTVFrames.Len()) 
		nV = m_lTVFrames[0].m_lVertices.Len();

	m_lTVFrames.SetLen(_nFrames);
	for(int i = 0; i < _nFrames; i++)
		if (m_lTVFrames[i].m_lVertices.Len() != nV)
		{
			m_lTVFrames[i].m_lVertices.SetLen(nV);
		}
}
#endif


int CTM_VertexBuffer::GetNumFrames()
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNumFrames, 0);
	return m_lVFrames.Len();
}

int CTM_VertexBuffer::GetNumTFrames()
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNumTFrames, 0);
	return m_lTVFrames.Len();
}

#ifndef PLATFORM_CONSOLE

CTM_VertexFrame* CTM_VertexBuffer::GetFrame(int _iFrame)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetFrame, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lVFrames.Len()) return NULL;
	return &m_lVFrames[_iFrame];
}

CTM_TVertexFrame* CTM_VertexBuffer::GetTFrame(int _iFrame)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetTFrame, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lTVFrames.Len()) return NULL;
	return &m_lTVFrames[_iFrame];
}
#endif


// Added by Mondelore.
void CTM_VertexBuffer::GetFrameAndTimeFraction(const CMTime& _Time, int& _iFrame0, int& _iFrame1, fp32& _Fraction)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetFrameAndTimeFraction, MAUTOSTRIP_VOID);
	_iFrame0 = 0;
	_iFrame1 = 0;
	_Fraction = 0.0f;

	int numFrames = m_lVFrames.Len();

	if (numFrames <= 1) return;
	if (m_TotalDuration <= 0.0f) return;

	// Hack to fix infinite loop - JA
	{
		int i;
		for(i = 0; i < m_lVFrames.Len(); i++)
			if(m_lVFrames[i].m_Duration != 0)
				break;

		if(i == m_lVFrames.Len())
			return;
	}
	
	fp32 Time = _Time.GetTimeModulus(m_TotalDuration);

	while(Time >= m_lVFrames[_iFrame0].m_Duration)
	{
		Time -= m_lVFrames[_iFrame0].m_Duration;
		_iFrame0++;
		if (_iFrame0 >= numFrames)
			_iFrame0 -= numFrames;
	}

	if (m_lVFrames[_iFrame0].m_Duration > 0.0f)
		_Fraction = Time / m_lVFrames[_iFrame0].m_Duration;
	else
		_Fraction = 0.0f;

	_iFrame1 = _iFrame0 + 1;

	if (_iFrame1 == numFrames)
		_iFrame1 = 0;
}


void CTM_VertexBuffer::ReadOld(CTriangleMeshCore* _pTriCore, CTM_Cluster* _pC, CDataFile* _pDFile, int _bOldVersion)
{
#ifdef PLATFORM_CONSOLE
	bool bDelayLoad = true;
#else
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif


	MAUTOSTRIP(CTM_VertexBuffer_ReadOld, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	uint32 Version = 0x0100;

	// MISCELLANEOUS
	{
		if (!_pDFile->GetNext("MISCELLANEOUS")) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Corrupt model. (0)", _pTriCore->GetMeshName()));
		if (_pDFile->GetUserData() >= 0x0002)
			pF->ReadLE(Version);

		if(Version >= 0x0200)
		{
			pF->ReadLE(m_TotalDuration);
		}
		else
		{
			if (Version >= 0x0101)
			{
				uint32 Temp;
				pF->ReadLE(Temp);
				_pC->m_Flags = Temp;
				pF->ReadLE(Temp);
				_pC->m_iSurface = Temp;
			}
			else
			{
				uint16 Tmp;
				pF->ReadLE(Tmp);
				_pC->m_Flags = Tmp;
				pF->ReadLE(Tmp);
				_pC->m_iSurface = Tmp;
			}

			if (Version < 0x0101)
			{
				uint32 Tmp;
				pF->ReadLE(Tmp);
				//m_EnableMask
			}
			pF->ReadLE(_pC->m_BoundRadius);
			_pC->m_BoundBox.Read(pF);
			// Added by Mondelore.
			if (_pDFile->GetUserData() >= 0x0001)
				pF->ReadLE(m_TotalDuration);

			if (Version >= 0x0103)
			{
				uint8 Temp;
				pF->ReadLE(Temp);
				_pC->m_OcclusionIndex = Temp;
			}

			m_VBFlags = _pC->m_Flags;
//			m_iSurface = _pC->m_iSurface;
//			m_OcclusionIndex = _pC->m_OcclusionIndex;
//			m_BoundBox = _pC->m_BoundBox;
//			m_BoundRadius = _pC->m_BoundRadius;
		}
	}

	// Don't support delayload for old formats
	if (Version < 0x0101)
		bDelayLoad = false;

	m_FileVersion = Version;

	m_bDelayLoad = bDelayLoad;

	// VERTEXFRAMES
	{
		if (!_pDFile->GetNext("VERTEXFRAMES")) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Corrupt model. (1)", _pTriCore->GetMeshName()).GetStr());

		m_lVFrames.SetLen(_pDFile->GetUserData());
		if (Version >= 0x0101)
		{
			uint32 Num;
			pF->ReadLE(Num);
		}

		for(int i = 0; i < m_lVFrames.Len(); i++)
		{
			if (_bOldVersion)
				m_lVFrames[i].OldRead(pF);
			else
			{                
				m_lVFrames[i].Read(pF, _pTriCore->m_BoundBox, bDelayLoad);
			}
		}

		// Apply global scale
		fp32 GlobalScale = _pTriCore->m_GlobalScale;
		if (M_Fabs(GlobalScale - 1.0f) > 0.001f)
		{
			for (uint iVFrame = 0; iVFrame < m_lVFrames.Len(); iVFrame++)
			{
				CTM_VertexFrame& VFrame = m_lVFrames[iVFrame];
				VFrame.m_BoundRadius *= GlobalScale;
				VFrame.m_BoundBox.m_Min *= GlobalScale;
				VFrame.m_BoundBox.m_Max *= GlobalScale;
				VFrame.m_Translate *= GlobalScale;
			}
		}
	}

	// TVERTEXFRAMES
	{
		if (!_pDFile->GetNext("TVERTEXFRAMES")) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Corrupt model. (2)", _pTriCore->GetMeshName()));

		m_lTVFrames.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lTVFrames.Len(); i++)
		{
			m_lTVFrames[i].Read(pF, _pDFile->GetUserData2(), bDelayLoad);
		}
	}

	if (Version <= 0x0100)
	{
		// BONEVERTEXINFO (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("BONEVERTEXINFO"))
			{
				m_lBDVertexInfo.SetLen(_pDFile->GetUserData());
				M_ASSERT(m_lBDVertexInfo.ListSize() == _pDFile->GetEntrySize(), "!");
				pF->Read(m_lBDVertexInfo.GetBasePtr(), m_lBDVertexInfo.ListSize());

#ifndef CPU_LITTLEENDIAN
				for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
					m_lBDVertexInfo[i].SwapLE();
#endif
			}
			_pDFile->PopPosition();
		}

		// BONEINFLUENCE (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("BONEINFLUENCE"))
			{
				m_lBDInfluence.SetLen(_pDFile->GetUserData());
				CTM_BDInfluence::ReadArray(pF, _pDFile->GetUserData2(), m_lBDInfluence.GetBasePtr(), m_lBDInfluence.Len());
	/*			{
					for(int i = 0; i < m_lBDInfluence.Len(); i++)
						m_lBDInfluence[i].Read(pF, _pDFile->GetUserData2());
				}*/

				// Normalize weights so that the total weight is 1. 
				{
					for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
					{
						const CTM_BDVertexInfo& BDI = m_lBDVertexInfo[i];
						CTM_BDInfluence* pBI = &m_lBDInfluence[BDI.m_iBoneInfluence];

						{
							fp32 wSum = 0;
							for(int b = 0; b < BDI.m_nBones; b++)
							{
								wSum += pBI[b].m_Influence;
							}
//							if (M_Fabs(wSum - 1.0f) > 0.01f)
//							{
//								int a = 1;
//							}
						}

						fp32 wSum = 0;
						for(int b = 0; b < BDI.m_nBones-1; b++)
						{
							if (wSum + pBI->m_Influence > 1.0f)
								pBI->m_Influence = 1.0f - wSum;
							wSum += pBI->m_Influence;
							pBI++;
						}

						pBI->m_Influence = 1.0f - wSum;				
					}
				}

			}
			_pDFile->PopPosition();
		}

		// BONEMATRIXMAP (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("BONEMATRIXMAP"))
			{
				int Len = _pDFile->GetUserData();
				_pC->m_lBDMatrixMap.SetLen(Len);
				uint8 lMap[256];
				uint16 *pBDMatrixMap = _pC->m_lBDMatrixMap.GetBasePtr();
				pF->Read(lMap, Len);
				for(int i = 0; i < Len; i++)
					pBDMatrixMap[i] = lMap[i];
			}
			_pDFile->PopPosition();
		}

		if (m_lBDVertexInfo.Len())
		{
			m_bHaveBones = true;
			if (_pC->m_lBDMatrixMap.Len())
				m_bHaveBoneMatrixMap = true;
		}
	}
	else
	{
		_pDFile->PushPosition();

		if (_pDFile->GetNext("BONEDEFORM"))
		{
			m_bHaveBones = true;
			m_FilePosBoneDeform = pF->Pos();

			if (!bDelayLoad)
			{
				ReadBoneDeformOld(pF, _pC);
			}
			else
				ReadBoneDeformOldBDMatrix(pF, _pC);
		}

		_pDFile->PopPosition();
		
	}

	// TRIANGLES
	{
		if (!_pDFile->GetNext("TRIANGLES")) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Corrupt model. (24)", _pTriCore->GetMeshName()));

		if (Version <= 0x0100)
		{
			m_lTriangles.SetLen(_pDFile->GetUserData());
			pF->ReadLE((uint16*)m_lTriangles.GetBasePtr(), m_lTriangles.Len()*3);
		}
		else
		{
			m_FilePosTriangles = pF->Pos();
			if (!bDelayLoad)
				ReadTriangles(pF);
		}
//		for(int iTri = 0; iTri < m_lTriangles.Len(); iTri++) 
//			m_lTriangles[iTri].Read(_pDFile);
	}

	// PRIMITIVES (Optional)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("PRIMITIVES"))
		{
			if (Version <= 0x0100)
			{
				m_lPrimitives.SetLen(_pDFile->GetUserData());
				_pDFile->GetFile()->Read(m_lPrimitives.GetBasePtr(), m_lPrimitives.ListSize());
				SwitchArrayLE_uint16(m_lPrimitives.GetBasePtr(), m_lPrimitives.Len());
			}
			else
			{
				m_FilePosPrimitives = pF->Pos();
				if (!bDelayLoad)
					ReadPrimitives(pF);
			}
		}
		_pDFile->PopPosition();
	}

	if (Version >= 0x0101)
	{
		// EDGES (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("EDGES"))
			{
				m_FilePosEdges = pF->Pos();
				if (!bDelayLoad)
					ReadEdges(pF);

			}
			_pDFile->PopPosition();
		}

		// EDGETRIS (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("EDGETRIS"))
			{
				m_FilePosEdgeTris = pF->Pos();
				if (!bDelayLoad)
					ReadEdgeTris(pF);
			}
			_pDFile->PopPosition();
		}

		// TRIEDGES (Optional)
		{
			_pDFile->PushPosition();
			if (_pDFile->GetNext("TRIEDGES"))
			{
				m_FilePosTriEdges = pF->Pos();
				if (!bDelayLoad)
					ReadTriEdges(pF);
			}
			_pDFile->PopPosition();
		}
	}
}


void CTM_VertexBuffer::ReadBoneDeformOld(CCFile* _pF, CTM_Cluster* _pC)
{
	if (!m_FilePosBoneDeform)
		return;

	_pF->Seek(m_FilePosBoneDeform);

	// Vertex Info
	{
		uint32 Len;
		_pF->ReadLE(Len);
		m_lBDVertexInfo.SetLen(Len);

		_pF->Read(m_lBDVertexInfo.GetBasePtr(), m_lBDVertexInfo.ListSize());

#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
			m_lBDVertexInfo[i].SwapLE();
#endif
	}

	// Influence
	{
		uint32 Len;
		_pF->ReadLE(Len);

		m_lBDInfluence.SetLen(Len);

		CTM_BDInfluence::ReadArray(_pF, CTM_BDINFLUENCE_VERSION, m_lBDInfluence.GetBasePtr(), m_lBDInfluence.Len());

		for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
		{
			const CTM_BDVertexInfo& BDI = m_lBDVertexInfo[i];
			CTM_BDInfluence* pBI = &m_lBDInfluence[BDI.m_iBoneInfluence];


			{
				fp32 wSum = 0;
				for(int b = 0; b < BDI.m_nBones; b++)
				{
					wSum += pBI[b].m_Influence;
				}
//				if (M_Fabs(wSum - 1.0f) > 0.01f)
//				{
//					int a = 1;
//				}
			}

			fp32 wSum = 0;
			for(int b = 0; b < BDI.m_nBones-1; b++)
			{
				if (wSum + pBI->m_Influence > 1.0f)
					pBI->m_Influence = 1.0f - wSum;
				wSum += pBI->m_Influence;
				pBI++;
			}

			pBI->m_Influence = 1.0f - wSum;				
		}
	}

	if (m_FileVersion >= 0x0102)
	{
		_pF->ReadLE(_pC->m_iSharedMatrixMap);
	}
	// Deform Matrix
	if (_pC->m_iSharedMatrixMap < 0)
	{
		uint32 Len;
		_pF->ReadLE(Len);

		if (Len & 0x7ffffff)
		{
			m_bHaveBoneMatrixMap = true;
			if(Len & 0x80000000)
			{
				// 16 bit indicies
				_pC->m_lBDMatrixMap.SetLen(Len & 0x7ffffff);
				_pF->ReadLE(_pC->m_lBDMatrixMap.GetBasePtr(), _pC->m_lBDMatrixMap.Len());
			}
			else
			{
				_pC->m_lBDMatrixMap.SetLen(Len);

				uint8 lMap[256];
				uint16 *pBDMatrixMap = _pC->m_lBDMatrixMap.GetBasePtr();
				_pF->Read(lMap, Len);
				for(int i = 0; i < Len; i++)
					pBDMatrixMap[i] = lMap[i];
			}
		}
	}

}

void CTM_VertexBuffer::ReadBoneDeformOldBDMatrix(CCFile* _pF, CTM_Cluster* _pC)
{
	if (!m_FilePosBoneDeform)
		return;

	_pF->Seek(m_FilePosBoneDeform);

	// Vertex Info
	{
		// Skip vertex info
		uint32 Len;
		_pF->ReadLE(Len);
		_pF->RelSeek(Len * sizeof(CTM_BDVertexInfo));
	}

	// Influence
	{
		uint32 Len;
		_pF->ReadLE(Len);

		TThinArray<CTM_BDInfluence> lBDInfluence;
		lBDInfluence.SetLen(Len);

		CTM_BDInfluence::ReadArray(_pF, CTM_BDINFLUENCE_VERSION, lBDInfluence.GetBasePtr(), lBDInfluence.Len());
	}

	if (m_FileVersion >= 0x0102)
	{
		_pF->ReadLE(_pC->m_iSharedMatrixMap);
	}
	// Deform Matrix
	if (_pC->m_iSharedMatrixMap < 0)
	{
		uint32 Len;
		_pF->ReadLE(Len);

		if (Len & 0x7ffffff)
		{
			m_bHaveBoneMatrixMap = true;
			if(Len & 0x80000000)
			{
				// 16 bit indicies
				_pC->m_lBDMatrixMap.SetLen(Len & 0x7ffffff);
				_pF->ReadLE(_pC->m_lBDMatrixMap.GetBasePtr(), _pC->m_lBDMatrixMap.Len());
			}
			else
			{
				_pC->m_lBDMatrixMap.SetLen(Len);

				uint8 lMap[256];
				uint16 *pBDMatrixMap = _pC->m_lBDMatrixMap.GetBasePtr();
				_pF->Read(lMap, Len);
				for(int i = 0; i < Len; i++)
					pBDMatrixMap[i] = lMap[i];
			}
		}
	}

}

void CTM_VertexBuffer::ReadBoneDeform_v2(CCFile* _pF)
{
	if (!m_FilePosBoneDeform)
		return;

	_pF->Seek(m_FilePosBoneDeform);

	// Vertex Info
	{
		uint32 Len;
		_pF->ReadLE(Len);
		m_lBDVertexInfo.SetLen(Len);

		_pF->Read(m_lBDVertexInfo.GetBasePtr(), m_lBDVertexInfo.ListSize());

#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
			m_lBDVertexInfo[i].SwapLE();
#endif
	}

	// Influence
	{
		uint32 Len;
		_pF->ReadLE(Len);

		m_lBDInfluence.SetLen(Len);

		CTM_BDInfluence::ReadArray(_pF, CTM_BDINFLUENCE_VERSION, m_lBDInfluence.GetBasePtr(), m_lBDInfluence.Len());

		for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
		{
			const CTM_BDVertexInfo& BDI = m_lBDVertexInfo[i];
			CTM_BDInfluence* pBI = &m_lBDInfluence[BDI.m_iBoneInfluence];


			{
				fp32 wSum = 0;
				for(int b = 0; b < BDI.m_nBones; b++)
				{
					wSum += pBI[b].m_Influence;
				}
//				if (M_Fabs(wSum - 1.0f) > 0.01f)
//				{
//					int a = 1;
//				}
			}

			fp32 wSum = 0;
			for(int b = 0; b < BDI.m_nBones-1; b++)
			{
				if (wSum + pBI->m_Influence > 1.0f)
					pBI->m_Influence = 1.0f - wSum;
				wSum += pBI->m_Influence;
				pBI++;
			}

			pBI->m_Influence = 1.0f - wSum;				
		}
	}
}

void CTM_VertexBuffer::ReadEdges(CCFile* _pF)
{
	if (!m_FilePosEdges)
		return;
	_pF->Seek(m_FilePosEdges);
	uint32 Len;
	_pF->ReadLE(Len);
	m_lEdges.SetLen(Len);
	_pF->ReadLE((uint16 *)m_lEdges.GetBasePtr(), m_lEdges.Len() * 2);
}

void CTM_VertexBuffer::ReadEdgeTris(CCFile* _pF)
{
	if (!m_FilePosEdgeTris)
		return;

	_pF->Seek(m_FilePosEdgeTris);
	uint32 Len;
	_pF->ReadLE(Len);

	m_lEdgeTris.SetLen(Len);
	_pF->ReadLE((uint16 *)m_lEdgeTris.GetBasePtr(), m_lEdgeTris.Len() * 2);
}

void CTM_VertexBuffer::ReadTriEdges(CCFile* _pF)
{
	if (!m_FilePosTriEdges)
		return;

	_pF->Seek(m_FilePosTriEdges);
	uint32 Len;
	_pF->ReadLE(Len);
	m_lTriEdges.SetLen(Len);
	_pF->ReadLE((uint16 *)m_lTriEdges.GetBasePtr(), m_lTriEdges.Len() * 3);
}

void CTM_VertexBuffer::ReadTriangles(CCFile* _pF)
{
	if (!m_FilePosTriangles)
		return;

	_pF->Seek(m_FilePosTriangles);
	uint32 Len;
	_pF->ReadLE(Len);

	m_lTriangles.SetLen(Len);
	_pF->ReadLE((uint16*)m_lTriangles.GetBasePtr(), Len*3);
}

void CTM_VertexBuffer::ReadPrimitives(CCFile* _pF)
{
	if (!m_FilePosPrimitives)
		return;

	_pF->Seek(m_FilePosPrimitives);
	uint32 Len;
	_pF->ReadLE(Len);

	m_lPrimitives.SetLen(Len);
	_pF->ReadLE(m_lPrimitives.GetBasePtr(), Len);
}


struct CTM_Cluster_IOStruct_v200
{
	uint32 m_iSurface;
	uint32 m_iIBOffset;
	uint32 m_nIBPrim;
	uint32 m_iVBVert;
	uint32 m_nVBVert;
	uint32 m_iVB;
	uint32 m_iIB;
	uint32 m_Flags;
	uint32 m_OcclusionIndex;
	int32 m_iSharedMatrixMap;

	CBox3Dfp32 m_BoundBox;
	fp32 m_BoundRadius;

	void SwapLE()
	{
		::SwapLE(m_iSurface);
		::SwapLE(m_iIBOffset);
		::SwapLE(m_nIBPrim);
		::SwapLE(m_iVBVert);
		::SwapLE(m_nVBVert);
		::SwapLE(m_iVB);
		::SwapLE(m_iIB);
		::SwapLE(m_Flags);
		::SwapLE(m_OcclusionIndex);
		::SwapLE(m_iSharedMatrixMap);

		::SwapLE(m_BoundBox.m_Min.k[0]);
		::SwapLE(m_BoundBox.m_Min.k[1]);
		::SwapLE(m_BoundBox.m_Min.k[2]);
		::SwapLE(m_BoundBox.m_Max.k[0]);
		::SwapLE(m_BoundBox.m_Max.k[1]);
		::SwapLE(m_BoundBox.m_Max.k[2]);
		::SwapLE(m_BoundRadius);
	}
};

struct CTM_Cluster_IOStruct_v201
{
	uint32 m_iSurface;
	uint32 m_iIBOffset;
	uint32 m_nIBPrim;
	uint32 m_iVBVert;
	uint32 m_nVBVert;
	uint32 m_iEdges;
	uint32 m_nEdges;
	uint32 m_iVB;
	uint32 m_iIB;
	uint32 m_Flags;
	uint32 m_OcclusionIndex;
	int32 m_iSharedMatrixMap;

	CBox3Dfp32 m_BoundBox;
	fp32 m_BoundRadius;

	void SwapLE()
	{
		::SwapLE(m_iSurface);
		::SwapLE(m_iIBOffset);
		::SwapLE(m_nIBPrim);
		::SwapLE(m_iVBVert);
		::SwapLE(m_nVBVert);
		::SwapLE(m_iEdges);
		::SwapLE(m_nEdges);
		::SwapLE(m_iVB);
		::SwapLE(m_iIB);
		::SwapLE(m_Flags);
		::SwapLE(m_OcclusionIndex);
		::SwapLE(m_iSharedMatrixMap);

		::SwapLE(m_BoundBox.m_Min.k[0]);
		::SwapLE(m_BoundBox.m_Min.k[1]);
		::SwapLE(m_BoundBox.m_Min.k[2]);
		::SwapLE(m_BoundBox.m_Max.k[0]);
		::SwapLE(m_BoundBox.m_Max.k[1]);
		::SwapLE(m_BoundBox.m_Max.k[2]);
		::SwapLE(m_BoundRadius);
	}
};

void CTM_Cluster::Read(CDataFile* _pDFile, uint _Version)
{
	CCFile* pF = _pDFile->GetFile();

	switch(_Version)
	{
	case 0x0201:
		{
			CTM_Cluster_IOStruct_v201 IOStruct;

			pF->Read(&IOStruct, sizeof(IOStruct));
			IOStruct.SwapLE();

			m_iSurface = IOStruct.m_iSurface;
			m_iIBOffset = IOStruct.m_iIBOffset;
			m_nIBPrim = IOStruct.m_nIBPrim;
			m_iVBVert = IOStruct.m_iVBVert;
			m_nVBVert = IOStruct.m_nVBVert;
			m_iEdges = IOStruct.m_iEdges;
			m_nEdges = IOStruct.m_nEdges;
			m_iVB = IOStruct.m_iVB;
			m_iIB = IOStruct.m_iIB;
			m_Flags = IOStruct.m_Flags;
			m_OcclusionIndex = IOStruct.m_OcclusionIndex;
			m_iSharedMatrixMap = IOStruct.m_iSharedMatrixMap;
			m_BoundBox = IOStruct.m_BoundBox;
			m_BoundRadius = IOStruct.m_BoundRadius;

			if(m_iSharedMatrixMap < 0)
			{
				uint32 BDLen;
				pF->ReadLE(BDLen);
				if(BDLen)
				{
					m_lBDMatrixMap.SetLen(BDLen);
					pF->ReadLE(m_lBDMatrixMap.GetBasePtr(), BDLen);
				}
			}
			break;
			break;
		}
	case 0x0200:
		{
			CTM_Cluster_IOStruct_v200 IOStruct;

			pF->Read(&IOStruct, sizeof(IOStruct));
			IOStruct.SwapLE();

			m_iSurface = IOStruct.m_iSurface;
			m_iIBOffset = IOStruct.m_iIBOffset;
			m_nIBPrim = IOStruct.m_nIBPrim;
			m_iVBVert = IOStruct.m_iVBVert;
			m_nVBVert = IOStruct.m_nVBVert;
			m_iEdges = 0;
			m_nEdges = 0;
			m_iVB = IOStruct.m_iVB;
			m_iIB = IOStruct.m_iIB;
			m_Flags = IOStruct.m_Flags;
			m_OcclusionIndex = IOStruct.m_OcclusionIndex;
			m_iSharedMatrixMap = IOStruct.m_iSharedMatrixMap;
			m_BoundBox = IOStruct.m_BoundBox;
			m_BoundRadius = IOStruct.m_BoundRadius;

			if(m_iSharedMatrixMap < 0)
			{
				uint32 BDLen;
				pF->ReadLE(BDLen);
				if(BDLen)
				{
					m_lBDMatrixMap.SetLen(BDLen);
					pF->ReadLE(m_lBDMatrixMap.GetBasePtr(), BDLen);
				}
			}
			break;
		}
	}
}

#ifndef PLATFORM_CONSOLE
void CTM_Cluster::Write(CDataFile* _pDFile)
{
	CCFile* pF = _pDFile->GetFile();

	CTM_Cluster_IOStruct_v201 IOStruct;

	IOStruct.m_iSurface = m_iSurface;
	IOStruct.m_iIBOffset = m_iIBOffset;
	IOStruct.m_nIBPrim = m_nIBPrim;
	IOStruct.m_iVBVert = m_iVBVert;
	IOStruct.m_nVBVert = m_nVBVert;
	IOStruct.m_iEdges = m_iEdges;
	IOStruct.m_nEdges = m_nEdges;
	IOStruct.m_iVB = m_iVB;
	IOStruct.m_iIB = m_iIB;
	IOStruct.m_Flags = m_Flags;
	IOStruct.m_OcclusionIndex = m_OcclusionIndex;
	IOStruct.m_iSharedMatrixMap = m_iSharedMatrixMap;
	IOStruct.m_BoundBox = m_BoundBox;
	IOStruct.m_BoundRadius = m_BoundRadius;

	IOStruct.SwapLE();
	pF->Write(&IOStruct, sizeof(IOStruct));

	if(m_iSharedMatrixMap < 0)
	{
		uint32 BDLen = m_lBDMatrixMap.Len();
		pF->WriteLE(BDLen);
		if(BDLen)
			pF->WriteLE(m_lBDMatrixMap.GetBasePtr(), BDLen);
	}
}
#endif

//#define CTM_BDMATRIXMAP_VERSION 0x0100

void CTM_VertexBuffer::Read(CTriangleMeshCore* _pTriCore, CDataFile* _pDFile, int _Version)
{
#ifdef PLATFORM_CONSOLE
	bool bDelayLoad = true;
#else
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif
	MAUTOSTRIP(CTM_VertexBuffer_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	m_bDelayLoad = bDelayLoad;

	if(_Version < 0x0100)
		Error_static("CTM_VertexBuffer::Read", "This should use the old path for reading")

	if(!_pDFile->GetNext("MISCELLANEOUS"))
		Error_static("CTM_VertexBuffer::Read", "Error 1");
	
	{
		// MISC
		uint32 Flags;
		pF->ReadLE(Flags);
		m_VBFlags = Flags;
//		pF->ReadLE(m_BoundRadius);
//		m_BoundBox.Read(pF);
		pF->ReadLE(m_TotalDuration);
//		uint8 OcclusionIndex;
//		pF->ReadLE(OcclusionIndex);
//		m_OcclusionIndex = OcclusionIndex;
	}

	if(!_pDFile->GetNext("VERTEXFRAMES"))
		Error_static("CTM_VertexBuffer::Read", "Error 2");
	{
		int Num = _pDFile->GetUserData();
		m_lVFrames.SetLen(Num);

		for(int i = 0; i < m_lVFrames.Len(); i++)
			m_lVFrames[i].Read(pF, _pTriCore->m_BoundBox, bDelayLoad);

		// Apply global scale
		fp32 GlobalScale = _pTriCore->m_GlobalScale;
		if (M_Fabs(GlobalScale - 1.0f) > 0.001f)
		{
			for (uint iVFrame = 0; iVFrame < m_lVFrames.Len(); iVFrame++)
			{
				CTM_VertexFrame& VFrame = m_lVFrames[iVFrame];
				VFrame.m_BoundRadius *= GlobalScale;
				VFrame.m_BoundBox.m_Min *= GlobalScale;
				VFrame.m_BoundBox.m_Max *= GlobalScale;
				VFrame.m_Translate *= GlobalScale;
			}
		}
	}

	if(!_pDFile->GetNext("TVERTEXFRAMES"))
		Error_static("CTM_VertexBuffer::Read", "Error 3");
	{
		int Num = _pDFile->GetUserData();
		m_lTVFrames.SetLen(Num);
		for(int i = 0; i < Num; i++)
			m_lTVFrames[i].Read(pF, _pDFile->GetUserData2(), bDelayLoad);
	}

	{
		_pDFile->PushPosition();
		// Optional BoneDeform
		if (_pDFile->GetNext("BONEDEFORM"))
		{
			m_bHaveBones = true;
			m_FilePosBoneDeform = pF->Pos();

			if (!bDelayLoad)
			{
				ReadBoneDeform_v2(pF);
			}
		}
		_pDFile->PopPosition();
	}


	// TRIANGLES
	{
		if (!_pDFile->GetNext("TRIANGLES")) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Corrupt model. (24)", _pTriCore->GetMeshName()));

		m_FilePosTriangles = pF->Pos();
		if (!bDelayLoad)
			ReadTriangles(pF);
	}

	// PRIMITIVES (Optional)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("PRIMITIVES"))
		{
			m_FilePosPrimitives = pF->Pos();
			if (!bDelayLoad)
				ReadPrimitives(pF);
		}
		_pDFile->PopPosition();
	}

	// EDGES (Optional)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("EDGES"))
		{
			m_FilePosEdges = pF->Pos();
			if (!bDelayLoad)
				ReadEdges(pF);

		}
		_pDFile->PopPosition();
	}

	// EDGETRIS (Optional)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("EDGETRIS"))
		{
			m_FilePosEdgeTris = pF->Pos();
			if (!bDelayLoad)
				ReadEdgeTris(pF);
		}
		_pDFile->PopPosition();
	}

	// TRIEDGES (Optional)
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("TRIEDGES"))
		{
			m_FilePosTriEdges = pF->Pos();
			if (!bDelayLoad)
				ReadTriEdges(pF);
		}
		_pDFile->PopPosition();
	}
}

#ifndef PLATFORM_CONSOLE
void CTM_VertexBuffer::Write(CTriangleMeshCore* _pTriCore, CDataFile* _pDFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	// MISCELLANEOUS
	{
		_pDFile->BeginEntry("MISCELLANEOUS");
			uint32 Flags = m_VBFlags;
			pF->WriteLE(Flags);
//			pF->WriteLE(m_BoundRadius);
//			m_BoundBox.Write(pF);

			pF->WriteLE(m_TotalDuration); // Added by Mondelore.

//			uint8 Temp = m_OcclusionIndex;
//			pF->WriteLE(Temp);

		_pDFile->EndEntry(CTM_MISCENTRY_VERSION);
	}
	
	// VERTEXFRAMES
	{
		_pDFile->BeginEntry("VERTEXFRAMES");
		uint32 Num = m_lVFrames.Len();
		for(int i = 0; i < Num; i++)
			m_lVFrames[i].Write(pF, _pTriCore->m_BoundBox);
		_pDFile->EndEntry(m_lVFrames.Len());
	}

	// TVERTEXFRAMES
	{
		_pDFile->BeginEntry("TVERTEXFRAMES");
		for(int i = 0; i < m_lTVFrames.Len(); i++)
			m_lTVFrames[i].Write(pF);
		_pDFile->EndEntry(m_lTVFrames.Len(), CTM_TVFRAME_VERSION);
	}

	// BONEDEFORM (Optional)
	if (m_lBDVertexInfo.Len())
	{
		_pDFile->BeginEntry("BONEDEFORM");

		int Start = -(pF->Length());

		// Vertex Info
		uint32 Num = m_lBDVertexInfo.Len();
		pF->WriteLE(Num);
		for(int i = 0; i < m_lBDVertexInfo.Len(); i++)
			m_lBDVertexInfo[i].Write(pF);

		// Infulence
		Num = m_lBDInfluence.Len();
		pF->WriteLE(Num);
		for(int i = 0; i < m_lBDInfluence.Len(); i++)
			m_lBDInfluence[i].Write(pF);

		Start += (pF->Length());

		_pDFile->EndEntry(Start, CTM_BONEDEFORM_VERSION);

	}

	// TRIANGLES
	{
		_pDFile->BeginEntry("TRIANGLES");
		uint32 Num = m_lTriangles.Len();
		pF->WriteLE(Num);
		for(int iTri = 0; iTri < m_lTriangles.Len(); m_lTriangles[iTri++].Write(_pDFile))
		{
		}
		_pDFile->EndEntry(m_lTriangles.Len());
	}

	// PRIMITIVES (Optional)
	if (m_lPrimitives.Len())
	{
		_pDFile->BeginEntry("PRIMITIVES");
		uint32 Num = m_lPrimitives.Len();
		pF->WriteLE(Num);
		_pDFile->GetFile()->WriteLE(m_lPrimitives.GetBasePtr(), m_lPrimitives.Len());
		_pDFile->EndEntry(m_lPrimitives.Len());
	}

	// EDGES (Optional)
	if (m_lEdges.Len())
	{
		_pDFile->BeginEntry("EDGES");
		uint32 Num = m_lEdges.Len();
		pF->WriteLE(Num);
		_pDFile->GetFile()->WriteLE((uint16 *)m_lEdges.GetBasePtr(), m_lEdges.Len() * 2);
		_pDFile->EndEntry(m_lEdges.Len());
	}
	// EDGETRIS (Optional)
	if (m_lEdgeTris.Len())
	{
		_pDFile->BeginEntry("EDGETRIS");
		uint32 Num = m_lEdgeTris.Len();
		pF->WriteLE(Num);
		_pDFile->GetFile()->WriteLE((uint16 *)m_lEdgeTris.GetBasePtr(), m_lEdgeTris.Len() * 2);
		_pDFile->EndEntry(m_lEdgeTris.Len());
	}

	// TRIEDGES (Optional)
	if (m_lTriEdges.Len())
	{
		_pDFile->BeginEntry("TRIEDGES");
		uint32 Num = m_lTriEdges.Len();
		pF->WriteLE(Num);
		_pDFile->GetFile()->WriteLE((uint16 *)m_lTriEdges.GetBasePtr(), m_lTriEdges.Len() * 3);
		_pDFile->EndEntry(m_lTriEdges.Len());
	}
}

bool CompareVert(const CVec3Dfp32 &_1, const CVec3Dfp32 &_2)
{
	if (_1.k[0] > _2.k[0])
	{
		return true;
	}
	else if (_1.k[0] == _2.k[0])
	{
		if (_1.k[1] > _2.k[1])
		{
			return true;
		}
		else if (_1.k[1] == _2.k[1])
		{
			if (_1.k[2] > _2.k[2])
			{
				return true;
			}
			else if (_1.k[2] == _2.k[2])
			{
			}
			return false;
		}
		return false;
	}
	return false;
}

// -------------------------------------------------------------------
int CTM_VertexBuffer::AddEdge(CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri, int *_iSubCluster)
{
	MAUTOSTRIP(CTM_VertexBuffer_AddEdge, 0);
	M_ASSERT(_iSubCluster[_iv0] == _iSubCluster[_iv1], "");
	int e;
	for(e = 0; e < m_lEdges.Len(); e++)
	{
		CTM_Edge* pE = &m_lEdges[e];
		if (_iSubCluster[pE->m_iV[0]] != _iSubCluster[_iv0])
			continue;

		if (pE->m_iV[0] == 0xffff)
		{
			if ((pE->m_iV[0] == _iv0) && (pE->m_iV[1] == _iv1))
			{
				CTM_EdgeTris* pET = &m_lEdgeTris[e];
				pET->m_iTri[0] = _iTri;
				return e;
			}
		}
		else if (pE->m_iV[1] == 0xffff)
		{
			if ((pE->m_iV[0] == _iv1) && (pE->m_iV[1] == _iv0))
			{
				CTM_EdgeTris* pET = &m_lEdgeTris[e];
				pET->m_iTri[1] = _iTri;
				return e;
			}
		}
	}

	e = m_lEdges.Add(CTM_Edge());
	m_lEdgeTris.Add(CTM_EdgeTris());
	CVec3Dfp32 *pV = GetVertexPtr(_pTriCore, 0);
	int iTri1, iTri2;
	if (CompareVert(pV[_iv0],pV[_iv1]))
	{
		iTri1 = 0xffff;
		iTri2 = _iTri;
		Swap(_iv0, _iv1);
	}
	else
	{
		iTri1 = _iTri;
		iTri2 = 0xffff;
	}

	m_lEdgeTris[e].m_iTri[0] = iTri1;
	m_lEdgeTris[e].m_iTri[1] = iTri2;
//	m_lEdgeTris[e].m_iTri[0] = _iTri;
//	m_lEdgeTris[e].m_iTri[1] = 0xffff;
	m_lEdges[e].m_iV[0] = _iv0;
	m_lEdges[e].m_iV[1] = _iv1;
	return e;
}

int CTM_VertexBuffer::AddEdge(CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri)
{

	int e;
	for(e = 0; e < m_lEdges.Len(); e++)
	{
		CTM_Edge* pE = &m_lEdges[e];
		CTM_EdgeTris* pET = &m_lEdgeTris[e];
		if (pET->m_iTri[0] == 0xffff)
		{
			if ((pE->m_iV[0] == _iv0) && (pE->m_iV[1] == _iv1))
			{
				pET->m_iTri[0] = _iTri;
				return e;
			}
		}
		else if (pET->m_iTri[1] == 0xffff)
		{
			if ((pE->m_iV[0] == _iv1) && (pE->m_iV[1] == _iv0))
			{
				pET->m_iTri[1] = _iTri;
				return e;
			}
		}
	}

	e = m_lEdges.Add(CTM_Edge());
	m_lEdgeTris.Add(CTM_EdgeTris());
	CVec3Dfp32 *pV = GetVertexPtr(_pTriCore, 0);
	int iTri1, iTri2;
	if (CompareVert(pV[_iv0],pV[_iv1]))
	{
		iTri1 = 0xffff;
		iTri2 = _iTri;
		Swap(_iv0, _iv1);
	}
	else
	{
		iTri1 = _iTri;
		iTri2 = 0xffff;
	}

	m_lEdgeTris[e].m_iTri[0] = iTri1;
	m_lEdgeTris[e].m_iTri[1] = iTri2;
//	m_lEdgeTris[e].m_iTri[0] = _iTri;
//	m_lEdgeTris[e].m_iTri[1] = 0xffff;
	m_lEdges[e].m_iV[0] = _iv0;
	m_lEdges[e].m_iV[1] = _iv1;
	return e;
}

int CTM_VertexBuffer::AddShadowEdge(CTriangleMeshCore* _pTriCore, int _iv0, int _iv1, int _iTri, TArray<int> _lHideIndex)
{
	if (_iv0 == _iv1)
		Error_static(M_FUNCTION, CStrF("Model '%s' - Degenerate triangle found, stopping shadow generation", _pTriCore->GetMeshName()));

	TAP_RCD<int> plHideIndex(_lHideIndex);
	CVec3Dfp32 *pV = GetVertexPtr(_pTriCore, 0);
	CTM_Triangle* pT1 = &m_lTriangles[_iTri];
//	CVec3Dfp32 N1 = (pV[pT1->m_iV[2]] - pV[pT1->m_iV[0]]) / (pV[pT1->m_iV[1]] - pV[pT1->m_iV[0]]);

	int e;
	for(e = 0; e < m_lEdges.Len(); e++)
	{
		CTM_Edge* pE = &m_lEdges[e];
		CTM_EdgeTris* pET = &m_lEdgeTris[e];
		if (pET->m_iTri[0] == 0xffff)
		{
			if(plHideIndex[_iTri] != plHideIndex[pET->m_iTri[1]])
				continue;
			CTM_Triangle* pT2 = &m_lTriangles[pET->m_iTri[1]];
//			CVec3Dfp32 N2 = (pV[pT2->m_iV[2]] - pV[pT2->m_iV[0]]) / (pV[pT2->m_iV[1]] - pV[pT2->m_iV[0]]);
//			if(N1 * N2 > 0)
			if(!pT1->IsReversed(*pT2))
				if ((pE->m_iV[0] == _iv0) && (pE->m_iV[1] == _iv1))
				{
					pET->m_iTri[0] = _iTri;
					return e;
				}
		}
		else if (pET->m_iTri[1] == 0xffff)
		{
			if(plHideIndex[_iTri] != plHideIndex[pET->m_iTri[0]])
				continue;
			CTM_Triangle* pT2 = &m_lTriangles[pET->m_iTri[0]];
//			CVec3Dfp32 N2 = (pV[pT2->m_iV[2]] - pV[pT2->m_iV[0]]) / (pV[pT2->m_iV[1]] - pV[pT2->m_iV[0]]);
//			if(N1 * N2 > 0)
			if(!pT1->IsReversed(*pT2))
				if ((pE->m_iV[0] == _iv1) && (pE->m_iV[1] == _iv0))
				{
					pET->m_iTri[1] = _iTri;
					return e;
				}
		}
	}

	e = m_lEdges.Add(CTM_Edge());
	m_lEdgeTris.Add(CTM_EdgeTris());
	//	CVec3Dfp32 *pV = GetVertexPtr(0);
	int iTri1, iTri2;
	if (CompareVert(pV[_iv0],pV[_iv1]))
	{
		iTri1 = 0xffff;
		iTri2 = _iTri;
		Swap(_iv0, _iv1);
	}
	else
	{
		iTri1 = _iTri;
		iTri2 = 0xffff;
	}

	m_lEdgeTris[e].m_iTri[0] = iTri1;
	m_lEdgeTris[e].m_iTri[1] = iTri2;
	//	m_lEdgeTris[e].m_iTri[0] = _iTri;
	//	m_lEdgeTris[e].m_iTri[1] = 0xffff;
	m_lEdges[e].m_iV[0] = _iv0;
	m_lEdges[e].m_iV[1] = _iv1;
	return e;
}

void CTM_VertexBuffer::CreateEdges(CTriangleMeshCore* _pTriCore, int *_iSubCluster)
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateEdges, MAUTOSTRIP_VOID);
	if (m_lTriEdges.Len()) return;

	m_lEdges.SetGrow(m_lTriangles.Len() * 2);
	m_lEdgeTris.SetGrow(m_lTriangles.Len() * 2);
	m_lTriEdges.SetLen(m_lTriangles.Len());

	for(int t = 0; t < m_lTriangles.Len(); t++)
	{
		CTM_Triangle* pT = &m_lTriangles[t];
		CTM_TriEdges* pE = &m_lTriEdges[t];
		for(int e = 0; e < 3; e++)
		{
			pE->m_iEdges[e] = AddEdge(_pTriCore, pT->m_iV[e], pT->m_iV[(e+1) % 3], t);
		}
	}

	m_lEdges.OptimizeMemory();
	m_lEdgeTris.OptimizeMemory();
}

void CTM_VertexBuffer::CreateShadowEdges(CTriangleMeshCore* _pTriCore, CWireContainer* _pDebugWires, TArray<int> _lHideIndex, int _Flags)
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateShadowEdges, MAUTOSTRIP_VOID);
	if (m_lTriEdges.Len()) return;

	m_lEdges.SetGrow(m_lTriangles.Len() * 2);
	m_lEdgeTris.SetGrow(m_lTriangles.Len() * 2);
	m_lTriEdges.SetLen(m_lTriangles.Len());

	TAP_RCD<int> plHideIndex(_lHideIndex);

	for(int t = 0; t < m_lTriangles.Len(); t++)
	{
		// Find 2 triangles that share an edge and the dot of their normals are above 0
		CTM_Triangle* pT = &m_lTriangles[t];
		CTM_TriEdges* pE = &m_lTriEdges[t];

		for(int e = 0; e < 3; e++)
		{
			pE->m_iEdges[e] = AddShadowEdge(_pTriCore, pT->m_iV[e], pT->m_iV[(e+1) % 3], t, _lHideIndex);
		}
	}

	// Time to merge all "open" edges

	int nOpenEdges = 0;
	int nDeletedEdges = 0;
	for(int e = 0; e < m_lEdges.Len(); e++)
	{
		CTM_EdgeTris& Edge0 = m_lEdgeTris[e];
		if((Edge0.m_iTri[0] != 0xffff && Edge0.m_iTri[1] != 0xffff) || (Edge0.m_iTri[0] == 0xffff && Edge0.m_iTri[1] == 0xffff))
			continue;

		uint16 iT0 = Edge0.m_iTri[0];
		if(iT0 == 0xffff)
			iT0 = Edge0.m_iTri[1];

		nOpenEdges++;

		uint16 iT0V0 = m_lEdges[e].m_iV[0];
		uint16 iT0V1 = m_lEdges[e].m_iV[1];
		if(Edge0.m_iTri[1] == 0xffff)
			::Swap(iT0V0, iT0V1);

		for(int e2 = e + 1; e2 < m_lEdges.Len(); e2++)
		{
			CTM_EdgeTris& Edge1 = m_lEdgeTris[e2];
			if((Edge1.m_iTri[0] != 0xffff && Edge1.m_iTri[1] != 0xffff) || (Edge1.m_iTri[0] == 0xffff && Edge1.m_iTri[1] == 0xffff))
				continue;

			uint16 iT1 = Edge1.m_iTri[0];
			if(iT1 == 0xffff)
				iT1 = Edge1.m_iTri[1];

			if(plHideIndex[iT0] != plHideIndex[iT1])
				continue;

		//	if (Edge0.m_iTri[0] == 0xffff && Edge1.m_iTri[1] != 0xffff)
		//		continue;
		//	if (Edge0.m_iTri[1] == 0xffff && Edge1.m_iTri[0] != 0xffff)
		//		continue;

			uint16 iT1V0 = m_lEdges[e2].m_iV[0];
			uint16 iT1V1 = m_lEdges[e2].m_iV[1];
			if(Edge1.m_iTri[1] == 0xffff)
				::Swap(iT1V0, iT1V1);

			if(iT0V0 == iT1V1 && iT0V1 == iT1V0)
			{
				// Merge this edge
				uint16 iTri = Edge1.m_iTri[0];
				if(iTri == 0xffff)
					iTri = Edge1.m_iTri[1];

				if(Edge0.m_iTri[0] == 0xffff)
					Edge0.m_iTri[0] = iTri;
				else
					Edge0.m_iTri[1] = iTri;

				CTM_TriEdges& TriEdges = m_lTriEdges[iTri];
				if(TriEdges.m_iEdges[0] == e2)
					TriEdges.m_iEdges[0] = e;
				else if(TriEdges.m_iEdges[1] == e2)
					TriEdges.m_iEdges[1] = e;
				else if(TriEdges.m_iEdges[2] == e2)
					TriEdges.m_iEdges[2] = e;
				else
				{
					M_BREAKPOINT;
				}

				nDeletedEdges++;
				// delete edge1
				m_lEdgeTris[e2].m_iTri[0] = 0xffff;
				m_lEdgeTris[e2].m_iTri[1] = 0xffff;
				m_lEdges[e2].m_iV[0] = 0xffff;
				m_lEdges[e2].m_iV[1] = 0xffff;
				break;
			}
		}
	}

	if(nOpenEdges - nDeletedEdges > 0)
	{
		if(!(_Flags & CTM_CREATESHADOWEDGES_NOOPENEDGESWARNINGS))
			ModelWarning(CStrF("Model '%s' - Shadow cluster has %d open edges (%d merged)", _pTriCore->GetMeshName(), nOpenEdges - nDeletedEdges, nDeletedEdges));
		CVec3Dfp32* pV = GetVertexPtr(_pTriCore, 0);
		for(int iE = 0; iE < m_lEdges.Len(); iE++)
		{
			CTM_EdgeTris& EdgeTri = m_lEdgeTris[iE];
			if(EdgeTri.m_iTri[0] == 0xffff || EdgeTri.m_iTri[1] == 0xffff)
			{
				if(EdgeTri.m_iTri[0] == EdgeTri.m_iTri[1])
					continue;

				CTM_Edge& Edge = m_lEdges[iE];
				uint iTri = EdgeTri.m_iTri[0];
				if(iTri == 0xffff)
					iTri = EdgeTri.m_iTri[1];
				uint iHideIndex = _lHideIndex[iTri];

				if(!(_Flags & CTM_CREATESHADOWEDGES_NOOPENEDGESWARNINGS))
					ModelLog(CStrF("Open Edge %d: Vert (%d, %d) %s %s, Occlusion %u", iE, Edge.m_iV[0], Edge.m_iV[1], pV[Edge.m_iV[0]].GetString().Str(), pV[Edge.m_iV[1]].GetString().Str(), iHideIndex));
				_pDebugWires->RenderWire(pV[Edge.m_iV[0]], pV[Edge.m_iV[1]], DEBUGCOLOR_OPENEDGE);

				if(m_lBDVertexInfo.Len() > 0)
				{
					uint nBones = m_lBDVertexInfo[Edge.m_iV[0]].m_nBones;
					uint iInfluence = m_lBDVertexInfo[Edge.m_iV[0]].m_iBoneInfluence;
					for(uint i = 0; i < nBones; i++)
					{
						const CTM_BDInfluence& Inf = m_lBDInfluence[iInfluence + i];
						ModelDebug(CStrF("  Vertex %d: Bone %d, Weight %.3f", Edge.m_iV[0], Inf.m_iBone, Inf.m_Influence));
					}

					nBones = m_lBDVertexInfo[Edge.m_iV[1]].m_nBones;
					iInfluence = m_lBDVertexInfo[Edge.m_iV[1]].m_iBoneInfluence;
					for(uint i = 0; i < nBones; i++)
					{
						const CTM_BDInfluence& Inf = m_lBDInfluence[iInfluence + i];
						ModelDebug(CStrF("  Vertex %d: Bone %d, Weight %.3f", Edge.m_iV[1], Inf.m_iBone, Inf.m_Influence));
					}

					uint nMirror = 0;
					uint nDups = 0;
					for(uint iEdge = 0; iEdge < m_lEdges.Len(); iEdge++)
					{
						if(iEdge == iE)
							continue;

						const CTM_Edge& Edge2 = m_lEdges[iEdge];

						CTM_EdgeTris& EdgeTri2 = m_lEdgeTris[iEdge];
						if(Edge2.m_iV[0] == Edge.m_iV[0] && Edge2.m_iV[1] == Edge.m_iV[1])
						{
							ModelDebug(CStrF("    Dup edge %d: Tri (%d, %d)", iEdge, EdgeTri2.m_iTri[0], EdgeTri2.m_iTri[1]));
							nDups++;
						}
						else if(Edge2.m_iV[1] == Edge.m_iV[0] && Edge2.m_iV[0] == Edge.m_iV[1])
						{
							ModelDebug(CStrF("    Mir edge %d: Tri (%d, %d)", iEdge, EdgeTri2.m_iTri[0], EdgeTri2.m_iTri[1]));
							nMirror++;
						}
					}

					if(nDups > 0 || nMirror > 0)
					{
						ModelDebug(CStrF("  Open Edge has %d duplicates and %d mirrors", nDups, nMirror));
					}
				}
			}
		}
	}
	else if(nDeletedEdges > 0)
		ModelLog(CStrF("Shadow cluster merged %d open edges", nDeletedEdges));

	int nNukedEdges = 0;
	for(int e = m_lEdges.Len() - 1; e >= 0; e--)
	{
		CTM_EdgeTris& Edge = m_lEdgeTris[e];
		if(Edge.m_iTri[0] == 0xffff && Edge.m_iTri[1] == 0xffff)
		{
			// nuke this edge
			for(int t = 0; t < m_lTriEdges.Len(); t++)
			{
				CTM_TriEdges& TriEdges = m_lTriEdges[t];
				if(TriEdges.m_iEdges[0] > e)
					TriEdges.m_iEdges[0]--;
				if(TriEdges.m_iEdges[1] > e)
					TriEdges.m_iEdges[1]--;
				if(TriEdges.m_iEdges[2] > e)
					TriEdges.m_iEdges[2]--;
			}

			nNukedEdges++;
			m_lEdgeTris.Del(e);
			m_lEdges.Del(e);
		}
	}
/*
	for(int t0 = 0; t0 < m_lTriEdges.Len(); t0++)
	{
		const CTM_TriEdges& TriEdges0 = m_lTriEdges[t0];
		for(int t1 = t0 + 1; t1 < m_lTriEdges.Len(); t1++)
		{
			const CTM_TriEdges& TriEdges1 = m_lTriEdges[t1];
			int nShared = 0;
			for(int e0 = 0; e0 < 3; e0++)
			{
				for(int e1 = 0; e1 < 3; e1++)
				{
					if(TriEdges0.m_iEdges[e0] == TriEdges1.m_iEdges[e1])
						nShared++;
				}
			}

			if(nShared > 2)
			{
				LogFile(CStrF("ERROR: Triangles sharing more than 2 edges, shadows are borken %d/%d, %d", t0, t1, nShared));
			}
		}
	}
*/
	m_lEdges.OptimizeMemory();
	m_lEdgeTris.OptimizeMemory();
}

void CTM_VertexBuffer::CreateEdges(CTriangleMeshCore* _pTriCore)
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateEdges, MAUTOSTRIP_VOID);
	if (m_lTriEdges.Len()) return;

	m_lEdges.SetGrow(m_lTriangles.Len() * 2);
	m_lEdgeTris.SetGrow(m_lTriangles.Len() * 2);
	m_lTriEdges.SetLen(m_lTriangles.Len());

	for(int t = 0; t < m_lTriangles.Len(); t++)
	{
		CTM_Triangle* pT = &m_lTriangles[t];
		CTM_TriEdges* pE = &m_lTriEdges[t];
		for(int e = 0; e < 3; e++)
		{
			pE->m_iEdges[e] = AddEdge(_pTriCore, pT->m_iV[e], pT->m_iV[(e+1) % 3], t);
		}
	}

	m_lEdges.OptimizeMemory();
	m_lEdgeTris.OptimizeMemory();
}
#endif


void CTM_VertexBuffer::CreateMatrixIW(CTriangleMeshCore* _pTriCore, CCFile * _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateMatrixIW, MAUTOSTRIP_VOID);
	if (!m_bHaveBones)
		return;

	CCFile ModelFile;
	if (!_pFile) _pFile = &ModelFile;

//	int nBd = GetNumBDVertexInfo();
	GetNumBDVertexInfo(_pTriCore, _pFile);

	const CTM_BDVertexInfo* pVI = GetBDVertexInfo(_pTriCore, _pFile);
	const CTM_BDInfluence* pI = GetBDInfluence(_pTriCore, _pFile);

	int nV = GetNumVertices(_pTriCore, _pFile);
	int MaxV = 0;
	{
		for(int v = 0; v < nV; v++)
			MaxV = MaxMT(MaxV, pVI[v].m_nBones);
	}

	if (MaxV > 8)
		Error("CreateMatrixIW", CStrF("Too many bones per vertex: %d (max is 8)", MaxV));	

//LogFile(CStrF("Matrices per vertex %d", MaxV));

//	MaxV = 3;
//	MaxV = 4;
//	MaxV = 8;

	uint8 MaxV2 = (MaxV > 4) ? 4 : 0;
	MaxV = 4;
	
	m_lMatrixIndices.SetLen(nV + (nV * (MaxV2>0)));
	m_lMatrixWeights.SetLen(nV * (MaxV + MaxV2));

	uint32* pMI = m_lMatrixIndices.GetBasePtr();
	fp32* pMW = m_lMatrixWeights.GetBasePtr();

	uint32* pMI2 = pMI + nV;
	fp32 * pMW2 = pMW + nV * MaxV;

	{
		for(int v = 0; v < nV; v++)
		{
			const CTM_BDVertexInfo& VI = pVI[v];
			pMI[v] = 0;
			if( MaxV2 > 0 ) pMI2[v] = 0;
			int b = 0;
			fp32 wsum = 0.0f;

			for(; b < VI.m_nBones; b++)
			{
#ifdef _DEBUG
/*				if (pI[VI.m_iBoneInfluence+ b].m_iBone >= 16)
				{
					M_ASSERT(0, "!");
				}*/

				fp32 w = pI[VI.m_iBoneInfluence + b].m_Influence;
				if (w < 0.0f || w > 1.0f)
				{
					M_ASSERT(0, "!");
				}
				wsum += w;
#endif

				if( b < 4 )
				{
					pMI[v] |= (pI[VI.m_iBoneInfluence+ b].m_iBone) << (b*8);
					pMW[v*MaxV + b] = pI[VI.m_iBoneInfluence + b].m_Influence;
				}
				else
				{
					pMI2[v] |= (pI[VI.m_iBoneInfluence+b].m_iBone) << ((b-4)*8);
					pMW2[v*MaxV2 + b-4] = pI[VI.m_iBoneInfluence + b].m_Influence;
				}
			}


#ifdef _DEBUG
			if (wsum > 1.0001f)
			{
				M_ASSERT(0, "!");
			}
#endif

			for(;b < MaxV; b++)
			{
				pMW[v*MaxV + b] = 0;
			}

			for(;(b-4) < MaxV2; b++)
			{
				pMW2[v*MaxV2 + b-4] = 0;
			}
			
/*			pMI[v] = 0x01010101;
			pMW[v*MaxV + 0] = 1.0f;
			pMW[v*MaxV + 1] = 0.0f;
			pMW[v*MaxV + 2] = 0.0f;*/
		}
	}
}

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

static void CalcTangents(uint16* _piTri, int _nTri, int _nV, CVec3Dfp32* _pV, CVec3Dfp32* _pN, const CVec2Dfp32* _pTV, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV)
{
	MAUTOSTRIP(CalcTangents, MAUTOSTRIP_VOID);
	// Clear tangents
	{
		for(int v = 0; v < _nV; v++)
		{
			_pTangU[v] = 0;
			_pTangV[v] = 0;
		}
	}

	for(int t = 0; t < _nTri; t++)
	{
		int iv0 = _piTri[t*3 + 0];
		int iv1 = _piTri[t*3 + 1];
		int iv2 = _piTri[t*3 + 2];
		if (iv0 < 0 || iv0 >= _nV) Error_static("CalcTangents", CStrF("Invalid vertex index: %d/%d", iv0, _nV));
		if (iv1 < 0 || iv1 >= _nV) Error_static("CalcTangents", CStrF("Invalid vertex index: %d/%d", iv1, _nV));
		if (iv2 < 0 || iv2 >= _nV) Error_static("CalcTangents", CStrF("Invalid vertex index: %d/%d", iv2, _nV));

		CVec3Dfp32 TgUMapping;
		CVec3Dfp32 TgVMapping;
		CalcTangentSpace(iv0, iv1, iv2, _pV, _pN, _pTV, TgUMapping, TgVMapping);

		/*
		for(int i = 0;i < _nV;i++)
		{

			if( (_pN[i].AlmostEqual(_pN[iv0],0.001f) && _pV[i].AlmostEqual(_pV[iv0],0.001f)) ||
				(_pN[i].AlmostEqual(_pN[iv1],0.001f) && _pV[i].AlmostEqual(_pV[iv1],0.001f)) ||
				(_pN[i].AlmostEqual(_pN[iv2],0.001f) && _pV[i].AlmostEqual(_pV[iv2],0.001f)) )
			{
				_pTangU[i] += TgUMapping;
				_pTangV[i] += TgVMapping;
			}
		}
		//*/
		//*
		_pTangU[iv0] += TgUMapping;
		_pTangU[iv1] += TgUMapping;
		_pTangU[iv2] += TgUMapping;
		_pTangV[iv0] += TgVMapping;
		_pTangV[iv1] += TgVMapping;
		_pTangV[iv2] += TgVMapping;
		//*/
	}

	// Ortonormalize tangent coordinate system for each vertex
	{
		for(int v = 0; v < _nV; v++)
		{
			
			const CVec3Dfp32& N = _pN[v];
			CVec3Dfp32& TgU = _pTangU[v];
			CVec3Dfp32& TgV = _pTangV[v];
			TgU.Normalize();
			TgV.Normalize();
			TgU.Combine(N, -(N * TgU), TgU);
			TgV.Combine(N, -(N * TgV), TgV);
			TgU.Normalize();
			TgV.Normalize();

/*			if (M_Fabs(N * TgU) > 0.01f ||
				M_Fabs(N * TgV) > 0.01f)
				LogFile("Fuckade tangenter: " + N.GetString() + TgU.GetString() + TgV.GetString());
*/
		}
	}
}


void CTM_VertexBuffer::InternalCreateTangents(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_CreateTangents, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTM_VertexBuffer::CreateTangents);
	CCFile ModelFile;
	if (!_pFile) _pFile = &ModelFile;
	int nv = GetNumVertices(_pTriCore, _pFile);
	for(int f = 0; f < m_lVFrames.Len(); f++)
	{
		CTM_VertexFrame& Frame = m_lVFrames[f];

		CVec3Dfp32* pV = GetVertexPtr(_pTriCore, f, _pFile);
		CVec3Dfp32* pN = GetNormalPtr(_pTriCore, f, _pFile);

		Frame.m_lTangentU.SetLen(nv);
		Frame.m_lTangentV.SetLen(nv);
		CVec3Dfp32* pTangU = Frame.m_lTangentU.GetBasePtr();
		CVec3Dfp32* pTangV = Frame.m_lTangentV.GetBasePtr();

		CalcTangents((uint16*) GetTriangles(_pTriCore, _pFile), GetNumTriangles(_pTriCore, _pFile), nv, pV, pN, 
					 GetTVertexPtr(_pTriCore, Min(f, m_lTVFrames.Len()-1), _pFile), pTangU, pTangV);
	}
}

// -------------------------------------------------------------------
#ifdef NEVER
CTM_Node::CTM_Node()
{
	MAUTOSTRIP(CTM_Node_ctor, MAUTOSTRIP_VOID);
	m_LocalCenter = 0;
	m_Flags = 0;
	m_iiNodeChildren = 0;
	m_nChildren = 0;
	m_iNodeParent = -1;
	m_RotationScale = 1.0f;
	m_MovementScale = 1.0f;
	m_iRotationSlot = 0;
	m_iMovementSlot = 0;
	m_iVFrameCounter = 0;
	m_iTVFrameCounter = 0;
	FillW(&m_iVertexCluster, CTM_MAXLOD, -1);
}

void CTM_Node::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTM_Node_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	int16 Version;
	pF->ReadLE(Version);
	switch(Version)
	{
	case 0x0102 :
		{
			pF->Read(&m_LocalCenter, sizeof(m_LocalCenter));
			pF->ReadLE(m_Flags);
			pF->ReadLE(m_iiNodeChildren);
			pF->ReadLE(m_nChildren);
			pF->ReadLE(m_iNodeParent);
			pF->Read(&m_RotationScale, sizeof(m_RotationScale));
			pF->Read(&m_MovementScale, sizeof(m_MovementScale));

			pF->ReadLE(m_iRotationSlot);
			pF->ReadLE(m_iMovementSlot);
			pF->ReadLE(m_iVFrameCounter);
			pF->ReadLE(m_iTVFrameCounter);
			{ for(int i = 0; i < CTM_MAXLOD; pF->ReadLE(m_iVertexCluster[i++])); }
		}
		break;
	default :
		Error_static("Read", CStrF("Unsupported node version. (%.4x)", Version));
	}
}

void CTM_Node::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTM_Node_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	pF->WriteLE((int16)CTM_NODE_VERSION);
	pF->Write(&m_LocalCenter, sizeof(m_LocalCenter));
	pF->WriteLE(m_Flags);
	pF->WriteLE(m_iiNodeChildren);
	pF->WriteLE(m_nChildren);
	pF->WriteLE(m_iNodeParent);
	pF->Write(&m_RotationScale, sizeof(m_RotationScale));
	pF->Write(&m_MovementScale, sizeof(m_MovementScale));

	pF->WriteLE(m_iRotationSlot);
	pF->WriteLE(m_iMovementSlot);
	pF->WriteLE(m_iVFrameCounter);
	pF->WriteLE(m_iTVFrameCounter);
	{ for(int i = 0; i < CTM_MAXLOD; pF->WriteLE(m_iVertexCluster[i++])); }
}

#endif

// -------------------------------------------------------------------
void CTM_Variation::Read(CCFile* _pF, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			m_Name.Read(_pF);
			_pF->ReadLE(m_nClusters);
			_pF->ReadLE(m_iClusterRef);
		}
		break;

	default :
		Error_static("CTM_Variation::Read", CStrF("Unsupported version %.4", _Version))
	}
}

#ifndef PLATFORM_CONSOLE
void CTM_Variation::Write(CCFile* _pF)
{
	m_Name.Write(_pF);
	_pF->WriteLE(m_nClusters);
	_pF->WriteLE(m_iClusterRef);
}
#endif

// -------------------------------------------------------------------
void CTM_ClusterRef::Read(CCFile* _pF, int _Version)
{
	switch(_Version)
	{
	case 0x0100 :
		{
			_pF->ReadLE(m_iCluster);
			_pF->ReadLE(m_iSurface);
		}
		break;

	default :
		Error_static("CTM_ClusterRef::Read", CStrF("Unsupported version %.4", _Version))
	}
}

#ifndef PLATFORM_CONSOLE
void CTM_ClusterRef::Write(CCFile* _pF)
{
	_pF->WriteLE(m_iCluster);
	_pF->WriteLE(m_iSurface);
}
#endif

void CTM_ClusterRef::SwapLE()
{
	::SwapLE(m_iCluster);
	::SwapLE(m_iSurface);
}

// -------------------------------------------------------------------
CTriangleMeshCore::CTriangleMeshCore()
{
	/*
#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY)
	m_pCompressedClusters = NULL;
#endif
	*/
	m_iLOD = 0;
	m_GlobalScale = 1.0f;
}


CTriangleMeshCore::~CTriangleMeshCore()
{
	/*
#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY)
	if (m_pCompressedClusters)
		g_AramManagerMisc.Free(m_pCompressedClusters);
	m_pCompressedClusters = NULL;
#endif
	*/
}
/*
bool CTriangleMeshCore::HaveCompressedClusters() const
{
#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY)
	return (m_pCompressedClusters != NULL);
#else
	return (m_lCompressedClusters.Len() != 0);
#endif
}

void CTriangleMeshCore::DestroyCompressedClusters()
{
	m_lCompressedClusters.Destroy();
}
*/

void CTriangleMeshCore::Clear()
{
	MAUTOSTRIP(CTriangleMeshCore_Clear, MAUTOSTRIP_VOID);
	m_RenderFlags = 0;
//	m_ClusterTouchTime = 0;
//	m_nClusters = 0;
	m_lLODBias.Destroy();
#ifdef PLATFORM_CONSOLE
	m_lVertexBuffers.Destroy();
#else
	m_lspVertexBuffers.Destroy();
#endif
	m_lspSurfaces.Destroy();
	m_lSolids.Clear();
	m_lCapsules.Clear();
	m_spSkeleton = NULL;
	m_spTC = NULL;
	m_spShadowData = NULL;

	m_lVariations.Clear();
	m_lClusterRefs.Clear();

#ifndef	PLATFORM_CONSOLE
	while (m_Keys.GetnKeys())
		m_Keys.DeleteKey(0);
#endif

}

CTM_Cluster* CTriangleMeshCore::GetCluster(int _iC)
{
	return &m_lClusters[_iC];
}

CTM_VertexBuffer *CTriangleMeshCore::GetVertexBuffer(int _iVB)
{
#ifdef PLATFORM_CONSOLE
	return &m_lVertexBuffers[_iVB];
#else
	if(m_lspVertexBuffers.Len() <= _iVB)
		return NULL;
	return m_lspVertexBuffers[_iVB];
#endif
}

const char* CTriangleMeshCore::GetMeshName()
{
#ifdef M_RTM
	return "(RTM)";
#else
	if(this) return m_MeshName.GetStr();
	return "Unknown";
#endif
}

void CTriangleMeshCore::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTriangleMeshCore_Read, MAUTOSTRIP_VOID);
	MSCOPE(CTriangleMeshCore::Read, MODEL_TRIMESH);
	m_FileName = _pDFile->GetFile()->GetFileName();

	int Version = 0x100;

#ifndef M_RTM
	m_MeshName = _pDFile->GetFile()->GetFileName();
#endif

	// VERSION
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("VERSION"))
			Version = _pDFile->GetUserData();

		if (_pDFile->GetEntrySize() == 8)
		{
			_pDFile->GetFile()->ReadLE(m_RenderFlags);
			uint32 Tmp;
			_pDFile->GetFile()->ReadLE(Tmp);
//			m_MeshFlags &= ~(CTM_MESHFLAGS_EDGES | CTM_MESHFLAGS_MATRIXIW | CTM_MESHFLAGS_TANGENTS);
		}
		else if (_pDFile->GetEntrySize() == 4 && Version >= 0x0302)
		{
			_pDFile->GetFile()->ReadLE(m_RenderFlags);
		}

		_pDFile->PopPosition();
	}

	if (Version > CTM_VERSION)
		Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - Unsupported file version %.4x (Can read up to %.4x)", GetMeshName(), Version, CTM_VERSION));

	// IMAGELIST
	{
		MSCOPESHORT(IMAGELIST);
		_pDFile->PushPosition();
		if (_pDFile->GetNext("IMAGELIST"))
		{
			if (!_pDFile->GetSubDir())
				Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - No IMAGELIST sub-directory.", GetMeshName()));

			spCTextureContainer_Plain spTC = MNew(CTextureContainer_Plain);
			if (spTC == NULL) Error_static("CTriangleMeshCore::Read", "Out of memory.");
			spTC->AddFromImageList(_pDFile);
			m_spTC = spTC;

			_pDFile->GetParent();
		}
		_pDFile->PopPosition();
	}

	// SURFACES
	{
		MSCOPESHORT(SURFACES);
		_pDFile->PushPosition();
		if (_pDFile->GetNext("SURFACES"))
			CXW_Surface::Read(_pDFile->GetFile(), m_lspSurfaces, _pDFile->GetUserData());
		_pDFile->PopPosition();
	}

	// BOUND
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("BOUND"))
		{
			m_BoundBox.Read(_pDFile->GetFile());
			_pDFile->GetFile()->ReadLE(m_BoundRadius);

			m_BoundBox.m_Min *= m_GlobalScale;
			m_BoundBox.m_Max *= m_GlobalScale;
			m_BoundRadius *= m_GlobalScale;
		}
		_pDFile->PopPosition();
	}

	_pDFile->PushPosition();

	// CLUSTERS
	if(_pDFile->GetNext("CLUSTERDATA"))
	{
		_pDFile->GetSubDir();

		if(!_pDFile->GetNext("CLUSTERS"))
			Error_static("CTriangleMeshCore::Read", "Missing clusters");

		{
			int nClusters = _pDFile->GetUserData();
			int nVersion = _pDFile->GetUserData2();

			m_lClusters.SetLen(nClusters);

			int iC = 0;
			while(iC < nClusters)
			{
				m_lClusters[iC].Read(_pDFile, nVersion);
				iC++;
			}
		}

		if(!_pDFile->GetNext("VERTEXBUFFERS"))
			Error_static("CTriangleMeshCore::Read", "Missing clusters");

		{
			int nVB = _pDFile->GetUserData();
			int nVersion = _pDFile->GetUserData2();

			_pDFile->GetSubDir();

#ifdef PLATFORM_CONSOLE
			m_lVertexBuffers.SetLen(nVB);
#else
			m_lspVertexBuffers.SetGrow(Max(16, nVB));
#endif

			uint iVB = 0;
			while(_pDFile->GetNext("VERTEXBUFFER"))
			{
				_pDFile->GetSubDir();
#ifdef PLATFORM_CONSOLE
				m_lVertexBuffers[iVB].Read(this, _pDFile, nVersion);
#else
				spCTM_VertexBuffer spVB = MNew(CTM_VertexBuffer);
				spVB->Read(this, _pDFile, nVersion);
				m_lspVertexBuffers.Add(spVB);
#endif
				_pDFile->GetParent();

				iVB++;
			}

			_pDFile->GetParent();
		}

		_pDFile->GetParent();
	}
	else
	{
		_pDFile->PopPosition();
		
		_pDFile->PushPosition();
		if (_pDFile->GetNext("COMPRESSEDCLUSTERS"))
		{
			MSCOPESHORT(COMPRESSEDCLUSTERS);

			int nClusters = _pDFile->GetUserData();
	/*#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY)
			// read and decompress data
			TArray<uint8> lCompressedClusters;
			lCompressedClusters = _pDFile->ReadEntry();
			lCompressedClusters = CDiskUtil::Decompress(lCompressedClusters);
			m_nCompressedClusterSize = lCompressedClusters.Len();
			int nAlignedSize = (lCompressedClusters.Len() + 31) & ~31;

			// align the data (for transfer)
			uint8* pAligned = (uint8*)MRTC_GetMemoryManager()->AllocAlign(nAlignedSize, 32);
			memcpy(pAligned, lCompressedClusters.GetBasePtr(), m_nCompressedClusterSize);
			lCompressedClusters.Destroy();

			// allocate ARAM
			if (m_pCompressedClusters)
				g_AramManagerMisc.Free(m_pCompressedClusters);
			m_pCompressedClusters = g_AramManagerMisc.Alloc(nAlignedSize);

			// move data to ARAM
			CARQRequest ARQ;
			ARQ.TransferToAram(m_pCompressedClusters->m_pMemory, pAligned, nAlignedSize);
			while (!ARQ.Done()) {}

			// free aligned data
			MRTC_GetMemoryManager()->Free(pAligned);
	#else*/
			TArray<uint8> lData = _pDFile->ReadEntry();;
			lData = CDiskUtil::Decompress(lData);
	#ifdef PLATFORM_CONSOLE
			m_lVertexBuffers.Clear();
			m_lVertexBuffers.SetLen(nClusters);
			m_lClusters.Clear();
			m_lClusters.SetLen(nClusters);
	#else
			m_lspVertexBuffers.Clear();
			m_lClusters.Clear();
	#endif

	//		int nCompressedSize = lData.Len();
			spCCFile spFile = CDiskUtil::CreateCCFile(lData, CFILE_READ | CFILE_BINARY);

			CDataFile DFile;
			DFile.Open(spFile, 0);

			CDataFile* pDFile = &DFile;

			int iC = 0;
			while(pDFile->GetNext("CLUSTER"))
			{
				if (!pDFile->GetSubDir()) Error_static("CTriangleMeshCore::DecompressAll", CStrF("Model '%s' - No CLUSTER sub-directory.", GetMeshName()));

	#ifndef PLATFORM_CONSOLE
				spCTM_VertexBuffer spC = spCTM_VertexBuffer( MNew(CTM_VertexBuffer) );

				if (!spC) 
					Error_static("CTriangleMeshCore::DecompressAll", "Out of memory.");

				CTM_Cluster Cluster;
				spC->ReadOld(this, &Cluster, pDFile, false);

				Cluster.m_iVB = m_lspVertexBuffers.Len();
				m_lspVertexBuffers.Add(spC);
				m_lClusters.Add(Cluster);
	#else
				m_lVertexBuffers[iC].ReadOld(this, &m_lClusters[iC], pDFile, false);
				m_lClusters[iC].m_iVB = iC;
	#endif

				pDFile->GetParent();
				iC++;
			}

	#ifndef PLATFORM_CONSOLE
			if (m_lspVertexBuffers.Len() != nClusters)
				Error_static("DecompressAll", CStrF("Model '%s' - m_lspVertexBuffers.Len() != m_nClusters", GetMeshName()));

			m_lspVertexBuffers.OptimizeMemory();
	#else
			if (m_lVertexBuffers.Len() != nClusters)
				Error_static("DecompressAll", CStrF("Model '%s' - m_lspVertexBuffers.Len() != m_nClusters", GetMeshName()));
	#endif

	//		DetermineStatic();
	//#endif
		}
		else
		{
			MSCOPESHORT(CLUSTERS);
			_pDFile->PopPosition();
			_pDFile->PushPosition();
	//		m_nClusters = 0;
			if (_pDFile->GetNext("CLUSTERS"))
			{
				int nClusters = _pDFile->GetUserData();
				int iC = 0;
	#ifdef PLATFORM_CONSOLE
				m_lVertexBuffers.SetLen(nClusters);
				m_lClusters.SetLen(nClusters);
	#endif

				if (!_pDFile->GetSubDir())
					Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - No CLUSTERS sub-directory.", GetMeshName()));

				while(_pDFile->GetNext("CLUSTER"))
				{
					if (!_pDFile->GetSubDir()) Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - No CLUSTER sub-directory.", GetMeshName()));

	#ifndef PLATFORM_CONSOLE
					spCTM_VertexBuffer spC = MNew(CTM_VertexBuffer);
					if (!spC) Error_static("CTriangleMeshCore::Read", "Out of memory.");
					CTM_Cluster Cluster;
					spC->ReadOld(this, &Cluster, _pDFile, Version == 0x100);
					Cluster.m_iVB = m_lspVertexBuffers.Len();
					m_lspVertexBuffers.Add(spC);
					m_lClusters.Add(Cluster);
	#else
					m_lVertexBuffers[iC].ReadOld(this, &m_lClusters[iC], _pDFile, Version == 0x100);
					m_lClusters[iC].m_iVB = iC;
	#endif
	//				m_nClusters++;
					++iC;
					_pDFile->GetParent();
				}
				_pDFile->GetParent();
			}
			else
				Error_static("CTriangleMeshCore::Read", CStrF("Model '%s' - No CLUSTERS or COMPRESSEDCLUSTERS entry.", GetMeshName()));
		}
	}

	_pDFile->PopPosition();

	// SOLIDS
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLIDS"))
	{
		MSCOPESHORT(SOLIDS);
		uint nSolids = _pDFile->GetUserData();
		m_lSolids.SetLen(nSolids);
		for (int i = 0; i < nSolids; i++)
		{
			m_lSolids[i].ReadGeometry(_pDFile->GetFile());

			// Scale solids
			if (M_Fabs(m_GlobalScale - 1.0f) > 0.001f)
			{
				TAP<CPlane3Dfp32> pPlanes = m_lSolids[i].m_lPlanes;
				for (uint iP = 0; iP < pPlanes.Len(); iP++)
					pPlanes[iP].d *= m_GlobalScale;
			}
		}

	}
	_pDFile->PopPosition();

	// CAPSULES
	_pDFile->PushPosition();
	if (_pDFile->GetNext("PHYSCAPSULES"))
	{
		MSCOPESHORT(PHYSCAPSULES);
		m_lCapsules.SetLen(_pDFile->GetUserData());
		const int32 CapsuleVersion=_pDFile->GetUserData2();
		for(int i = 0; i < _pDFile->GetUserData(); i++)
		{
			m_lCapsules[i].Read(_pDFile->GetFile(),CapsuleVersion);
			m_lCapsules[i].m_Point1 *= m_GlobalScale;
			m_lCapsules[i].m_Point2 *= m_GlobalScale;
			m_lCapsules[i].m_Radius *= m_GlobalScale;
			m_lCapsules[i].UpdateInternal();
		}
	}
	_pDFile->PopPosition();

	// SKELETON
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SKELETON") && _pDFile->GetSubDir())
	{
		MSCOPESHORT(SKELETON);
		m_spSkeleton = MNew(CXR_Skeleton);
		if (!m_spSkeleton) Error_static("CTriangleMeshCore::Read", "Out of memory.");
		m_spSkeleton->Read(_pDFile);
		/*if (!m_spSkeleton->m_lNodes.Len()) m_spSkeleton = NULL;*/

		// Scale skeleton nodes 
		if (M_Fabs(m_GlobalScale - 1.0f) > 0.001f)
		{
			TAP<CXR_SkeletonNode> pNodes = m_spSkeleton->m_lNodes;
			for (uint i = 0; i < pNodes.Len(); i++)
				pNodes[i].m_LocalCenter *= m_GlobalScale;

			TAP<CXR_SkeletonAttachPoint> pAttachPoints = m_spSkeleton->m_lAttachPoints;
			for (uint i = 0; i < pAttachPoints.Len(); i++)
				pAttachPoints[i].m_LocalPos.GetRow(3) *= m_GlobalScale;

			TAP<CXR_Cloth> pCloth = m_spSkeleton->m_lCloth;
			for (uint i = 0; i < pCloth.Len(); i++)
			{
				TAP<CXR_ClothConstraint> pConstraints = pCloth[i].m_lConstraints;
				for (uint j = 0; j < pConstraints.Len(); j++)
					pConstraints[j].m_restlength *= m_GlobalScale;
			}
		}
	}
	_pDFile->PopPosition();

	// CLOTH
#if 0
	_pDFile->PushPosition();
	if (_pDFile->GetNext("CLOTH") && _pDFile->GetSubDir())
	{
		MSCOPESHORT(CLOTH);
		m_spSkeleton = MNew(CXR_Skeleton);
		if (!m_spSkeleton) Error_static("CTriangleMeshCore::Read", "Out of memory.");
		m_spSkeleton->Read(_pDFile);
		/*if (!m_spSkeleton->m_lNodes.Len()) m_spSkeleton = NULL;*/
	}
	_pDFile->PopPosition();
#endif

	// VARIATIONS
	_pDFile->PushPosition();
	if (_pDFile->GetNext("VARIATIONS"))
	{
		MSCOPESHORT(VARIATIONS);
		CCFile* pF = _pDFile->GetFile();
		int Ver = _pDFile->GetUserData2();
		int nV = _pDFile->GetUserData();
		m_lVariations.SetLen(nV);
		CTM_Variation* pV = m_lVariations.GetBasePtr();
		for(int i = 0; i < nV; i++)
			pV[i].Read(pF, Ver);
	}
	_pDFile->PopPosition();

	// CLUSTERREF
	_pDFile->PushPosition();
	if (_pDFile->GetNext("CLUSTERREF"))
	{
		MSCOPESHORT(CLUSTERREF);
		CCFile* pF = _pDFile->GetFile();
		int Ver = _pDFile->GetUserData2();
		int nCR = _pDFile->GetUserData();
		m_lClusterRefs.SetLen(nCR);
		CTM_ClusterRef* pCR = m_lClusterRefs.GetBasePtr();
		if (Ver == CTM_CLUSTERREF_VERSION)
		{
			pF->Read(pCR, m_lClusterRefs.ListSize());
			for(int i = 0; i < nCR; i++)
				pCR[i].SwapLE();
		}
		else
		{
			for(int i = 0; i < nCR; i++)
				pCR[i].Read(pF, Ver);
		}
	}
	_pDFile->PopPosition();

#if	defined (PLATFORM_WIN_PC)
	{
		int bXDF = D_MXDFCREATE;
		int Platform = D_MPLATFORM;
		if( !(bXDF && Platform != 0))
		{
			// KEYS
			MSCOPESHORT(KEYS);
			_pDFile->PushPosition();
			if (_pDFile->GetNext("KEYS"))
				m_Keys.Read(_pDFile->GetFile());
			_pDFile->PopPosition();
		}
	}
#endif	// PLATFORM_CONSOLE

	{
		MSCOPESHORT(LODBIAS);
		// LODBIAS
		_pDFile->PushPosition();
		if (_pDFile->GetNext("LODBIAS"))
		{
			m_lLODBias.SetLen(_pDFile->GetUserData());
			for(int i = 0; i < _pDFile->GetUserData(); i++)
				_pDFile->GetFile()->ReadLE(m_lLODBias[i]);
		}
		_pDFile->PopPosition();
	}

	// SHADOWDATA (Optional)
#if !defined(PLATFORM_PS2)
// Check if we're creating XDF's for PS2.. if we are then this data should be skipped
#ifndef PLATFORM_CONSOLE
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	if( !bXDF || ( bXDF && Platform != 2/*e_Platform_PS2*/ )  )
#endif
	{
		MSCOPESHORT(SHADOWDATA);
		_pDFile->PushPosition();
		if (_pDFile->GetNext("SHADOWDATA") && _pDFile->GetSubDir())
		{
			m_spShadowData = MNew(CTM_ShadowData);
			if (!m_spShadowData)
				Error_static("CTriangleMeshCore::Read", "Out of memory");
			m_spShadowData->Read(this, _pDFile, Version);
			if (!m_spShadowData->m_VertexBuffer.m_FilePosEdges)
			{
				// Guard for old version
				m_spShadowData = NULL;
			}
			
		}
		_pDFile->PopPosition();
	}
#endif	// PLATFORM_PS2

	int nVB = GetNumVertexBuffers();
	m_bVertexAnim = false;
	m_bTVertexAnim = false;
	m_bMatrixPalette = false;
	m_iLOD = 0;
	for (int i = 0; i < nVB; ++i)
	{
		CTM_VertexBuffer *pVB = GetVertexBuffer(i);
		if (pVB->m_lVFrames.Len() > 1)
		{
			m_bVertexAnim = true;
		}
		if (pVB->m_lTVFrames.Len() > 1)
		{
			m_bTVertexAnim = true;
		}
		if (pVB->m_bHaveBones)
		{
			m_bMatrixPalette = true;
		}
	}

	// Create a default variation if there was none in the file
	if (!m_lVariations.Len())
		CreateDefaultVariation();
}

#ifndef PLATFORM_CONSOLE

void CTriangleMeshCore::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CTriangleMeshCore_Write, MAUTOSTRIP_VOID);
//	DecompressAll(-1);
//	DetermineStatic();

	// VERSION
	{
		_pDFile->BeginEntry("VERSION");
		_pDFile->GetFile()->WriteLE(m_RenderFlags);
//		_pDFile->GetFile()->WriteLE(m_MeshFlags);
		_pDFile->EndEntry(CTM_VERSION);
	}

	// IMAGELIST
	if (m_spTC != NULL)
	{
		LogFile("-------------------------------------------------------------------");
		LogFile("WARNING: Ignoring texture images in model!");
		LogFile("-------------------------------------------------------------------");
	}
/*	if (m_spTC != NULL)
	{
		_pDFile->BeginEntry("IMAGELIST");
		_pDFile->EndEntry(m_spTC->GetNumTextures());
		_pDFile->BeginSubDir();
		m_spTC->WriteImageList(_pDFile);
		_pDFile->EndSubDir();
	}*/

	// SURFACES
	if (m_lspSurfaces.Len())
	{
		_pDFile->BeginEntry("SURFACES");
		CXW_Surface::Write(_pDFile->GetFile(), m_lspSurfaces);
		_pDFile->EndEntry(m_lspSurfaces.Len());
	}

	// BOUND
	{
		_pDFile->BeginEntry("BOUND");
		m_BoundBox.Write(_pDFile->GetFile());
		_pDFile->GetFile()->WriteLE(m_BoundRadius);
		_pDFile->EndEntry(0);
	}

/*	// COMPRESSEDCLUSTERS
	{
		TArray<uint8> lFile;
		spCCFile spFile = CDiskUtil::CreateCCFile(lFile, CFILE_WRITE | CFILE_BINARY);
		CDataFile DFile;

		DFile.Create(spFile);
		for(int i = 0; i < m_lspVertexBuffers.Len(); i++)
		{
			DFile.BeginEntry("CLUSTER");
			DFile.EndEntry(0);
			DFile.BeginSubDir();
			m_lspVertexBuffers[i]->Write(&DFile);
			DFile.EndSubDir();
		}
		DFile.Close();
		spFile->Close();

		TArray<uint8> lCompressed = CDiskUtil::Compress(lFile, LZSS_HUFFMAN);

		_pDFile->BeginEntry("COMPRESSEDCLUSTERS");
		_pDFile->GetFile()->Write(lCompressed.GetBasePtr(), lCompressed.Len());
		_pDFile->EndEntry(m_lspVertexBuffers.Len());
		
#ifdef _DEBUG
//		TArray<uint8> lDecompressed = CDiskUtil::Decompress(lCompressed);
		TArray<uint8> lDecompressed;
		{
			CCompressorInterface CI;
			CCompress* pC = CI.GetCompressor(LZSS_HUFFMAN, HIGH_COMPRESSION);
			if (!pC) Error_static("CDiskUtil::CreateFromCompressedFile", "No compressor.");

			lDecompressed;
			lDecompressed.SetLen(pC->GetUncompressedLength(lCompressed.GetBasePtr()) + 16);
			FillChar(lDecompressed.GetBasePtr(), lDecompressed.ListSize(), 0xdd);
			pC->Decompress(lCompressed.GetBasePtr(), lDecompressed.GetBasePtr());
			for(int i = 0; i < 16; i++)
				if (lDecompressed[lDecompressed.Len()-1-i] != 0xdd)
					ConOutL(CStrF("WARNING: Padding differ at pos %d, %d != %d", i, lDecompressed[lDecompressed.Len()-1-i], 0xdd));
			lDecompressed.SetLen(lDecompressed.Len() - 16);
		}

		if (lFile.Len() != lDecompressed.Len())
			ConOutL(CStrF("WARNING: Decompressed length missmatch. %d != %d", lDecompressed.Len(), lFile.Len()));
		else
		{
			uint8* pSrc1 = lFile.GetBasePtr();
			uint8* pSrc2 = lDecompressed.GetBasePtr();
			int Len = lFile.Len();
			for(int i = 0; i < Len; i++)
				if (pSrc1[i] != pSrc2[i])
					ConOutL(CStrF("WARNING: Bytes differ at pos %d, %d != %d", i, pSrc1[i], pSrc2[i]));
		}
#endif


		

	}*/

	// CLUSTERS
	{
		_pDFile->BeginEntry("CLUSTERDATA");
		_pDFile->EndEntry(0);
		_pDFile->BeginSubDir();
		{
			_pDFile->BeginEntry("CLUSTERS");
			for(int i = 0; i < m_lClusters.Len(); i++)
			{
				m_lClusters[i].Write(_pDFile);
			}
			_pDFile->EndEntry(m_lClusters.Len(), CTM_CLUSTER_VERSION);
		}
		{
			_pDFile->BeginEntry("VERTEXBUFFERS");
			_pDFile->EndEntry(m_lspVertexBuffers.Len(), CTM_VERTEXBUFFER_VERSION);
			_pDFile->BeginSubDir();
			for(int i = 0; i < m_lspVertexBuffers.Len(); i++)
			{
				_pDFile->BeginEntry("VERTEXBUFFER");
				_pDFile->EndEntry(0);
				_pDFile->BeginSubDir();
				m_lspVertexBuffers[i]->Write(this, _pDFile);
				_pDFile->EndSubDir();
			}
			_pDFile->EndSubDir();
		}
		_pDFile->EndSubDir();
	}
	if(0)
	{
		_pDFile->BeginEntry("CLUSTERS");
		_pDFile->EndEntry(m_lspVertexBuffers.Len());
		_pDFile->BeginSubDir();
		for(int i = 0; i < m_lspVertexBuffers.Len(); i++)
		{
			_pDFile->BeginEntry("CLUSTER");
			_pDFile->EndEntry(0);
			_pDFile->BeginSubDir();
			m_lspVertexBuffers[i]->Write(this, _pDFile);
			_pDFile->EndSubDir();
		}
		_pDFile->EndSubDir();
	}

	// SOLIDS
	if (m_lSolids.Len())
	{
		_pDFile->BeginEntry("SOLIDS");
		for(int i = 0; i < m_lSolids.Len(); i++)
		{
			m_lSolids[i].WriteGeometry(_pDFile->GetFile());
		}
		_pDFile->EndEntry(m_lSolids.Len());
	}

	// PHYSCAPSULES
	if (m_lCapsules.Len())
	{
		_pDFile->BeginEntry("PHYSCAPSULES");
		for(int i = 0; i < m_lCapsules.Len(); i++)
		{
			m_lCapsules[i].Write(_pDFile->GetFile());
		}
		_pDFile->EndEntry(m_lCapsules.Len(),CAPSULE_VERSION);
	}

	// SKELETON
	if (m_spSkeleton != NULL && (m_spSkeleton->m_lNodes.Len() || m_spSkeleton->m_lAttachPoints.Len()))
	{
		_pDFile->BeginEntry("SKELETON");
		_pDFile->EndEntry(0);
		_pDFile->BeginSubDir();
		m_spSkeleton->Write(_pDFile);
		_pDFile->EndSubDir();
	}

	// VARIATIONS
	if (m_lVariations.Len())
	{
		_pDFile->BeginEntry("VARIATIONS");
		CCFile* pF = _pDFile->GetFile();
		int nV = m_lVariations.Len();
		CTM_Variation* pV = m_lVariations.GetBasePtr();
		for(int i = 0; i < nV; i++)
			pV[i].Write(pF);
		_pDFile->EndEntry(nV, CTM_VARIATION_VERSION);
	}

	// CLUSTERREF
	if (m_lClusterRefs.Len())
	{
		_pDFile->BeginEntry("CLUSTERREF");
		CCFile* pF = _pDFile->GetFile();
		int nCR = m_lClusterRefs.Len();
		CTM_ClusterRef* pCR = m_lClusterRefs.GetBasePtr();
		for(int i = 0; i < nCR; i++)
			pCR[i].Write(pF);
		_pDFile->EndEntry(nCR, CTM_CLUSTERREF_VERSION);

	}

	// KEYS
	{
		_pDFile->BeginEntry("KEYS");
		m_Keys.Write(_pDFile->GetFile());
		_pDFile->EndEntry(0);
	}

	// LODBIAS
	if (m_lLODBias.Len())
	{
		_pDFile->BeginEntry("LODBIAS");
		for(int i = 0; i < m_lLODBias.Len(); i++)
			_pDFile->GetFile()->WriteLE(m_lLODBias[i]);
		_pDFile->EndEntry(m_lLODBias.Len());
	}

	// SHADOWDATA (Optional)
	if (m_spShadowData)
	{
		_pDFile->BeginEntry("SHADOWDATA");
		_pDFile->EndEntry(0);
		_pDFile->BeginSubDir();
		m_spShadowData->Write(this, _pDFile);
		_pDFile->EndSubDir();
	}
}
#endif
#ifndef PLATFORM_CONSOLE
void CTriangleMeshCore::CreateTriFromTri32()
{
	MAUTOSTRIP(CTriangleMeshCore_CreateTriFromTri32, MAUTOSTRIP_VOID);
	MSCOPE(CTriangleMeshCore::CreateTriFromTri32, MODEL_TRIMESH);

	for(int iC = 0; iC < m_lspVertexBuffers.Len(); iC++)
		m_lspVertexBuffers[iC]->CreateTriFromTri32();
}

#endif

void CTriangleMeshCore::CreateDefaultVariation()
{
	int nC = GetNumClusters();
	m_lClusterRefs.SetLen(nC);
	m_lVariations.SetLen(1);
	CTM_Variation& V = m_lVariations[0];
	V.m_iClusterRef = 0;
	V.m_nClusters = nC;

	CTM_ClusterRef* pCR = m_lClusterRefs.GetBasePtr();
	for(int i = 0; i < nC; i++)
	{
		CTM_Cluster* pC = GetCluster(i);
		pCR[i].m_iCluster = i;
		pCR[i].m_iSurface = pC->m_iSurface;
	}
}

int CTM_VertexBuffer::GetNumVertices(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNumVertices, 0);
	if (!m_lVFrames.Len())
	{
		return 0;
	}
		//M_ASSERT(0, "?");
		//Error_static("CTriangleMeshCore::GetNumVertices", "No frames.");
	int Len = m_lVFrames[0].m_lVertices.Len();
	if (m_bDelayLoad && !Len && m_lVFrames[0].m_bHasVertices)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		m_lVFrames[0].ReadVertices(_pFile, _pTriCore->m_BoundBox);
		Len = m_lVFrames[0].m_lVertices.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumPrimitives(CTriangleMeshCore* _pTriCore, CCFile * _pFile)
{
	int Len = m_lPrimitives.Len();
	if (m_bDelayLoad && !Len && m_FilePosPrimitives)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadPrimitives(_pFile);
		Len = m_lPrimitives.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumTriangles(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lTriangles.Len();
	if (m_bDelayLoad && !Len && m_FilePosTriangles)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadTriangles(_pFile);
		Len = m_lTriangles.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumEdges(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lEdges.Len();
	if (m_bDelayLoad && !Len && m_FilePosEdges)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadEdges(_pFile);
		Len = m_lEdges.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumTriEdges(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lTriEdges.Len();
	if (m_bDelayLoad && !Len && m_FilePosTriEdges)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadTriEdges(_pFile);
		Len = m_lTriEdges.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumEdgeTris(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lEdgeTris.Len();
	if (m_bDelayLoad && !Len && m_FilePosEdgeTris)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadEdgeTris(_pFile);
		Len = m_lEdgeTris.Len();
	}
	return Len;
}

static void LoadBoneDeform(CTriangleMeshCore* _pTriCore, CTM_VertexBuffer* _pC, CCFile* _pFile)
{
	CCFile NewFile;
	if( !_pFile ) _pFile = &NewFile;
	if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
	_pC->ReadBoneDeform_v2(_pFile);
}

int CTM_VertexBuffer::GetNumBDVertexInfo(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lBDVertexInfo.Len();
	if (m_bDelayLoad && !Len && m_bHaveBones && m_FilePosBoneDeform)
	{
		LoadBoneDeform(_pTriCore, this,_pFile);
		Len = m_lBDVertexInfo.Len();
	}
	return Len;
}

int CTM_VertexBuffer::GetNumBDInfluence(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	int Len = m_lBDInfluence.Len();
	if (m_bDelayLoad && !m_lBDVertexInfo.Len() && m_bHaveBones && m_FilePosBoneDeform)
	{
		LoadBoneDeform(_pTriCore, this,_pFile);
		Len = m_lBDInfluence.Len();
	}
	return Len;
}

CVec3Dfp32* CTM_VertexBuffer::GetVertexPtr(CTriangleMeshCore* _pTriCore, int _iFrame, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetVertexPtr, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lVFrames.Len()) return NULL;

	if (m_bDelayLoad && !m_lVFrames[_iFrame].m_lVertices.Len() && m_lVFrames[_iFrame].m_bHasVertices)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		m_lVFrames[_iFrame].ReadVertices(_pFile, _pTriCore->m_BoundBox);
	}

	return m_lVFrames[_iFrame].m_lVertices.GetBasePtr();
}

CVec3Dfp32* CTM_VertexBuffer::GetNormalPtr(CTriangleMeshCore* _pTriCore, int _iFrame, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNormalPtr, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lVFrames.Len()) return NULL;

	if (m_bDelayLoad && !m_lVFrames[_iFrame].m_lNormals.Len() && m_lVFrames[_iFrame].m_bHasVertices)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		m_lVFrames[_iFrame].ReadNormals(_pFile);
	}

	return m_lVFrames[_iFrame].m_lNormals.GetBasePtr();
}

CVec2Dfp32* CTM_VertexBuffer::GetTVertexPtr(CTriangleMeshCore* _pTriCore, int _iFrame, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetTVertexPtr, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lTVFrames.Len()) return NULL;

	if (m_bDelayLoad && !m_lTVFrames[_iFrame].m_lVertices.Len() && m_lVFrames[_iFrame].m_bHasVertices)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		m_lTVFrames[_iFrame].ReadVertices(_pFile);
	}

	return m_lTVFrames[_iFrame].m_lVertices.GetBasePtr();
}

CVec3Dfp32* CTM_VertexBuffer::GetTangentUPtr(CTriangleMeshCore* _pTriCore, int _iFrame, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNormalPtr, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lVFrames.Len()) return NULL;

	if (!m_lVFrames[_iFrame].m_lTangentU.Len())
	{
		if( m_bDelayLoad && m_lVFrames[_iFrame].m_bHasVertices && m_lVFrames[_iFrame].m_bFileTangents )
		{
			CCFile NewFile;
			if( !_pFile ) _pFile = &NewFile;
			if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
			m_lVFrames[_iFrame].ReadTangents(_pFile);
		}
		else
		{
			InternalCreateTangents(_pTriCore, _pFile);
		}
	}

	return m_lVFrames[_iFrame].m_lTangentU.GetBasePtr();
}

CVec3Dfp32* CTM_VertexBuffer::GetTangentVPtr(CTriangleMeshCore* _pTriCore, int _iFrame, CCFile* _pFile)
{
	MAUTOSTRIP(CTM_VertexBuffer_GetNormalPtr, NULL);
	if (_iFrame < 0) return NULL;
	if (_iFrame >= m_lVFrames.Len()) return NULL;

	if (!m_lVFrames[_iFrame].m_lTangentV.Len())
	{
		if( m_bDelayLoad && m_lVFrames[_iFrame].m_bHasVertices && m_lVFrames[_iFrame].m_bFileTangents )
		{
			CCFile NewFile;
			if( !_pFile ) _pFile = &NewFile;
			if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
			m_lVFrames[_iFrame].ReadTangents(_pFile);
		}
		else
		{
			InternalCreateTangents(_pTriCore, _pFile);
		}
	}

	return m_lVFrames[_iFrame].m_lTangentV.GetBasePtr();
}

uint16 *CTM_VertexBuffer::GetTriangles(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lTriangles.Len() && m_FilePosTriangles)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadTriangles(_pFile);
	}
	return (uint16 *)m_lTriangles.GetBasePtr();
}

uint16 *CTM_VertexBuffer::GetPrimitives(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lPrimitives.Len() && m_FilePosPrimitives)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadPrimitives(_pFile);
	}
	return m_lPrimitives.GetBasePtr();
}

CTM_Edge *CTM_VertexBuffer::GetEdges(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lEdges.Len() && m_FilePosEdges)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadEdges(_pFile);
	}
	return m_lEdges.GetBasePtr();
}

CTM_TriEdges *CTM_VertexBuffer::GetTriEdges(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lTriEdges.Len() && m_FilePosTriEdges)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadTriEdges(_pFile);
	}
	return m_lTriEdges.GetBasePtr();
}

CTM_EdgeTris *CTM_VertexBuffer::GetEdgeTris(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lEdgeTris.Len() && m_FilePosEdgeTris)
	{
		CCFile NewFile;
		if( !_pFile ) _pFile = &NewFile;
		if( !_pFile->IsOpen() ) _pFile->Open(_pTriCore->m_FileName, CFILE_READ|CFILE_BINARY);
		ReadEdgeTris(_pFile);
	}
	return m_lEdgeTris.GetBasePtr();
}

CTM_BDVertexInfo *CTM_VertexBuffer::GetBDVertexInfo(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lBDVertexInfo.Len() && m_bHaveBones && m_FilePosBoneDeform)
	{
		LoadBoneDeform(_pTriCore, this,_pFile);
	}
	return m_lBDVertexInfo.GetBasePtr();
}

CTM_BDInfluence *CTM_VertexBuffer::GetBDInfluence(CTriangleMeshCore* _pTriCore, CCFile* _pFile)
{
	if (m_bDelayLoad && !m_lBDVertexInfo.Len() && m_bHaveBones && m_FilePosBoneDeform)
	{
		LoadBoneDeform(_pTriCore, this,_pFile);
	}
	return m_lBDInfluence.GetBasePtr();;
}



void CTM_VertexBuffer::SetMaxBoneInfluenceNum(uint8 _nInf /* = 4 */)
{
	TArray<CTM_BDInfluence> lBDInfluence;

	uint i;
	for(i = 0;i < m_lBDVertexInfo.Len();i++)
	{
		CTM_BDVertexInfo &Inf = m_lBDVertexInfo[i];
		uint j;

		if( Inf.m_nBones > _nInf )
		{
			//Bubblesort the influences
			bool bDone = false;
			while(!bDone)
			{
				bDone = true;
				for(j = Inf.m_iBoneInfluence;
					j < Inf.m_nBones+Inf.m_iBoneInfluence-1;
					j++)
				{
					if( (m_lBDInfluence[j].m_Influence < m_lBDInfluence[j+1].m_Influence) ||
						((m_lBDInfluence[j].m_Influence == m_lBDInfluence[j+1].m_Influence) &&
						 (m_lBDInfluence[j].m_iBone < m_lBDInfluence[j+1].m_iBone)) )
					{
						CTM_BDInfluence Swap = m_lBDInfluence[j];
						m_lBDInfluence[j] = m_lBDInfluence[j+1];
						m_lBDInfluence[j+1] = Swap;
						bDone = false;
					}
				}
			}

			//Normalize
			fp32 Total = 0.0f;
			for(j = 0;j < _nInf;j++) Total += m_lBDInfluence[Inf.m_iBoneInfluence + j].m_Influence;
			fp32 InvTotal = 1.0f / Total;
			for(j = 0;j < _nInf;j++) m_lBDInfluence[Inf.m_iBoneInfluence + j].m_Influence *= InvTotal;

			//Set max
			Inf.m_nBones = _nInf;
		}

		//Copy bones
		uint16 iInfluence = lBDInfluence.Len();
		for(j = 0;j < Inf.m_nBones;j++)
		{
			lBDInfluence.Add(m_lBDInfluence[Inf.m_iBoneInfluence+j]);
		}
		Inf.m_iBoneInfluence = iInfluence;
	}

	m_lBDInfluence.SetLen(lBDInfluence.Len());
	for(i = 0;i < lBDInfluence.Len();i++)
	{
		m_lBDInfluence[i] = lBDInfluence[i];
	}
}

#ifndef PLATFORM_CONSOLE
/*
void CTriangleMeshCore::WeldTangentSpaces(fp32 _VSplitAngle)
{
	fp32 SplitFactor = M_Cos(Clamp(_VSplitAngle,0.0f ,180.0f) * (_PI / 180.0f));

	for(int iC = 0;iC < GetNumClusters();iC++)
	{
		CTM_VertexBuffer * pC = GetVertexBuffer(iC);
		TArray<uint32>	lnVertices;
		uint32 nV = pC->GetNumVertices();
		lnVertices.SetLen(nV);
		FillChar(lnVertices.GetBasePtr(),lnVertices.ListSize(),0);

		//Count triangles
		int i;
		for(i = 0;i < pC->m_lTriangles32.Len();i++)
		{
			CTM_Triangle32 &Tri = pC->m_lTriangles32[i];
			lnVertices[Tri.m_iV[0]] ++;
			lnVertices[Tri.m_iV[1]] ++;
			lnVertices[Tri.m_iV[2]] ++;
		}

		//Loop through vertices
		CVec3Dfp32 *pV = pC->GetVertexPtr(0);
		CVec3Dfp32 *pN = pC->GetNormalPtr(0);
		CVec3Dfp32 *pTanU = pC->GetTangentUPtr(0);
		CVec3Dfp32 *pTanV = pC->GetTangentVPtr(0);
		for(i = 0;i < nV-1;i++)
		{
			for(int j = i+1;j < nV;j++)
			{
				if( pV[i].AlmostEqual(pV[j],0.01f) &&
					pN[i].AlmostEqual(pN[j],0.01f) )// &&
				//	(pTanU[i] * pTanU[j] >= SplitFactor) &&
				//	(pTanV[i] * pTanV[j] >= SplitFactor) )
				{
					CVec3Dfp32 TanU,TanV;
					fp32 nVerts[2] = { lnVertices[i], lnVertices[j] };
	
					TanU = ((pTanU[i] * nVerts[0]) + (pTanU[j] * nVerts[1])).Normalize();
					TanV = ((pTanV[i] * nVerts[0]) + (pTanV[j] * nVerts[1])).Normalize();
					pTanU[i] = TanU; pTanU[j] = TanU;
					pTanV[i] = TanV; pTanV[j] = TanV;

				//	break;
				}
			}
		}
	}
}
*/

/*
#pragma optimize("",off)
uint32 CTriangleMeshCore::TraceVtxMismatch()
{
	uint32 nRet = 0;
	for(int i = 0;i < GetNumClusters();i++)
	{
		CTM_VertexBuffer * pC = GetVertexBuffer(i);

		for(int j = 0;j < pC->GetNumVertices();j++)
		{
			CVec3Dfp32 Pos = pC->GetVertexPtr(0)[j];
			CTM_BDInfluence * pInf = pC->GetBDInfluence() + pC->m_lBDVertexInfo[j].m_iBoneInfluence;
			uint8 nInf = pC->m_lBDVertexInfo[j].m_nBones;

			for(int iC = i;iC < GetNumClusters();iC++)
			{
				CTM_VertexBuffer *pC2 = GetVertexBuffer(iC);

				for(int iV = ((iC == i) ? j+1 : 0); iV < pC2->GetNumVertices();iV++)
				{
					CVec3Dfp32 Pos2 = pC2->GetVertexPtr(0)[iV];
					if( !Pos.AlmostEqual(Pos2,0.01f) ) continue;

					CTM_BDInfluence * pInf2 = pC2->GetBDInfluence() + pC2->m_lBDVertexInfo[iV].m_iBoneInfluence;
					uint8 nInf2 = pC2->m_lBDVertexInfo[iV].m_nBones;

					bool bEq = (nInf == nInf2);
					if( bEq )
					{
						if( pC->m_lBDMatrixMap.Len() )
						{
							for(int k = 0;k < nInf;k++)
							{
								if( (pC->m_lBDMatrixMap[pInf[k].m_iBone] != pC2->m_lBDMatrixMap[pInf2[k].m_iBone]) ||
									!AlmostEqual(pInf[k].m_Influence,pInf2[k].m_Influence,1.0f/256.0f) )
								{
									bEq = false;
									break;
								}
							}
						}
						else
						{
							for(int k = 0;k < nInf;k++)
							{
								if( (pInf[k].m_iBone != pInf2[k].m_iBone) ||
									!AlmostEqual(pInf[k].m_Influence,pInf2[k].m_Influence,1.0f/256.0f) )
								{
									bEq = false;
									break;
								}
							}
						}
					}
					if( !bEq ) nRet++;
				}
			}
		}
	}

	return nRet;
}
*/

#endif
