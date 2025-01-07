#include "PCH.h"

#include "WObj_Func.h"
#include "MFloat.h"

// -------------------------------------------------------------------
//  Func_Float
// -------------------------------------------------------------------
static const char *ms_AttribStr[] =
{
	"X-Pos", "Y-Pos", "Z-Pos", "X-Axis", "Y-Axis", "Z-Axis", NULL
};

CWObject_Func_Float::CWObject_Func_Float()
{
	MAUTOSTRIP(CWObject_Func_Float_ctor, MAUTOSTRIP_VOID);
	m_AnimTime = 1000;
};

void CWObject_Func_Float::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Func_Float_OnEvalKey, MAUTOSTRIP_VOID);
	if(_pKey->GetThisName().Copy(0, 8) == "WAVEFORM")
	{
		CStr st = _pKey->GetThisValue();
		CWaveform wave;
		wave.m_iType = st.GetStrSep(" ").TranslateInt(ms_AttribStr);
		wave.m_Amplitude = st.GetStrSep(" ").Val_fp64();
		wave.m_Duration = (fp32)st.Val_fp64() * ( SERVER_TICKSPERSECOND / _PI2 );
		m_Waveforms.Add(wave);
	}
	else
		CWObject_Model::OnEvalKey(_pKey);
}

void CWObject_Func_Float::OnInitInstance(const int32* _pParam, int _nParam)
{
	MAUTOSTRIP(CWObject_Func_Float_OnInitInstance, MAUTOSTRIP_VOID);
	m_OrgMat = GetPositionMatrix();
}

void CWObject_Func_Float::OnRefresh()
{
	MAUTOSTRIP(CWObject_Func_Float_OnRefresh, MAUTOSTRIP_VOID);
	CMat43fp32 Pos = m_OrgMat;
	int nWaveforms = m_Waveforms.Len();
	for(int w = 0; w < nWaveforms; w++)
	{
		switch(m_Waveforms[w].m_iType)
		{
		case 0: Pos.k[3][0] += QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude; break;
		case 1: Pos.k[3][1] += QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude; break;
		case 2: Pos.k[3][2] += QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude; break;
		case 3:	
		case 4:
		case 5:
			{
				CMat43fp32 Move, Mat, Mat2;
				Move.Unit();
				CVec3Dfp32 P = CVec3Dfp32::GetMatrixRow(m_OrgMat, 3);
				(-P).SetMatrixRow(Move, 3);
				Pos.Multiply(Move, Mat);
				CMat43fp32 Rot;
				if(m_Waveforms[w].m_iType == 3)
					Rot.SetXRotation(QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude / 360.0f);
				else if(m_Waveforms[w].m_iType == 4)
					Rot.SetYRotation(QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude / 360.0f);
				else
					Rot.SetZRotation(QSin(m_AnimTime / m_Waveforms[w].m_Duration) * m_Waveforms[w].m_Amplitude / 360.0f);
				Mat.Multiply(Rot, Mat2);
				(P).SetMatrixRow(Move, 3);
				Mat2.Multiply(Move, Pos);
				break;
			}
		}
	}
	
	m_pWServer->Object_MoveTo(m_iObject, Pos);
	m_AnimTime++;
}

void CWObject_Func_Float::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Func_Float_OnLoad, MAUTOSTRIP_VOID);
	CWObject_Model::OnLoad(_pFile);
}

void CWObject_Func_Float::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Func_Float_OnSave, MAUTOSTRIP_VOID);
	CWObject_Model::OnSave(_pFile);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Func_Float, CWObject_Model, 0x0100);
