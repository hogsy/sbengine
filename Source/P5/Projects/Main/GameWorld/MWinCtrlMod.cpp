#include "PCH.h"

#include "MWinCtrlMod.h"

//#include "../GameClasses/WObj_CharMsg.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../Exe/WGameContextMain.h"
#include "WFrontEndMod.h"

#include "../../../Shared/MOS/MSystem/Raster/MRender_nVidia.h"

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModButton, CMWnd_Button);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeButton, CMWnd_ModButton);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeOptionButton, CMWnd_CubeButton);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeSwitchButton, CMWnd_CubeButton);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeText, CMWnd_CubeRgn);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMeter, CMWnd_CubeRgn);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeOptionMeter, CMWnd_CubeRgn);
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeLayout, CMWnd_ModTexture);

/*
const char aTranslationString[] = "!""#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~×àáâãäåæçèéêëìíîïðñòóôõöøùúûüýÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝ";
void Write(uint16 *_pMap, int32 x, int32 y, const char *_pString)
{
	_pMap += y*20+x;

	while(*_pString)
	{
		for(int32 i = 0; aTranslationString[i] != 0; i++)
		{
			if(aTranslationString[i] == *_pString)
			{
				*_pMap = //(16*22+1)+i;
							(16*12+1)+i;
				break;
			}
		}

		_pMap++;
		_pString++;
	}
}
*/
//#define RANDOM_BELOW (16*12-2)

// -------------------------------------------------------------------
//  CMWnd_CubeButton
// -------------------------------------------------------------------
CMWnd_CubeButton::CMWnd_CubeButton()
{
	m_AlwaysPaint = false;
}

void CMWnd_CubeButton::EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg)
{
	CMWnd::EvaluateRegisty(_pParam, _pReg);
}

void CMWnd_CubeButton::EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if(_Key == "RGN")
	{
		_pParam->m_Pos.p0.x = int(Value.GetStrSep(",").Val_fp64()*(640/20.0f));
		_pParam->m_Pos.p0.y = int(Value.GetStrSep(",").Val_fp64()*(480/20.0f));
		_pParam->m_Pos.p1.x = int(Value.GetStrSep(",").Val_fp64()*(640/20.0f));
		_pParam->m_Pos.p1.y = int(Value.GetStrSep(",").Val_fp64()*(480/20.0f));
		_pParam->m_Pos.p1 += _pParam->m_Pos.p0;
	}
	else
		CMWnd::EvaluateKeyOrdered(_pParam, _Key, _Value);
}

void CMWnd_CubeButton::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if(_Key == "TEXT")
	{
		CMWnd_ModButton::EvaluateKey(_pParam, _Key, _Value);
		CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
		if(pLayout)
			pLayout->Layout_WriteText(_pParam->m_Pos, m_Text, CMWnd_CubeLayout::FLAG_ADJUST_SIZE);
	}
	else if(_Key == "ALWAYSPAINT")
		m_AlwaysPaint = true;
	else
		CMWnd_ModButton::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeButton::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	if(m_AlwaysPaint)
	{
		CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
		if(pLayout)
		{
			pLayout->Layout_Clear(GetPosition(), 0);
			CRct Pos = GetPosition();
			pLayout->Layout_WriteText(Pos, m_Text, 0);
		}
	}
}

void CMWnd_CubeButton::OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
}

void CMWnd_CubeButton::SoundEvent(uint _SoundEvent, const char* _pSoundName)
{
	switch (_SoundEvent)
	{
	case WSND_SELECT:
		{
			m_FrontEndInfo.m_pFrontEnd->m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_SELECT);
			break;
		}

	default:
		{
			CMWnd_ModButton::SoundEvent(_SoundEvent, _pSoundName);
			break;
		}
	}
}



// -------------------------------------------------------------------
//  CMWnd_CubeButton
// -------------------------------------------------------------------

CMWnd_CubeOptionButton::CMWnd_CubeOptionButton()
{
	m_Max = 1;
	m_Min = 0;
	m_StepUp = 0.1f;
	m_StepDown =-0.09998f;
	m_SfxVol = false;
	m_Enviroment = false;
}

void CMWnd_CubeOptionButton::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "OPTION")
		m_Option = _Value;
	if(_Key == "OPTIONLIST")
		m_OptionList = _Value;
	if(_Key == "ENVIROMENT")
		m_Enviroment = true;
	if(_Key == "MAX")
		m_Max = _Value.Val_fp64();
	if(_Key == "MIN")
		m_Max = _Value.Val_fp64();
	if(_Key == "STEPS")
		m_Steps = _Value.Val_int();
	if(_Key == "STEP")
	{
		m_StepUp = _Value.Val_fp64();
		m_StepDown = -_Value.Val_fp64();
	}
	if(_Key == "USE_SFXVOL")
		m_SfxVol = true;
	if(_Key == "STEPDOWN")
		m_StepDown = _Value.Val_fp64();
	if(_Key == "STEPUP")
		m_StepUp = _Value.Val_fp64();
	else
		CMWnd_CubeButton::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeOptionButton::SoundEvent(uint _SoundEvent, const char* _pSoundName)
{
	switch (_SoundEvent)
	{
	case WSND_SWITCH:
		{
			m_FrontEndInfo.m_pFrontEnd->m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_FLIP_CTRL, NULL, false);
			break;
		}

	default:
		{
			CMWnd_CubeButton::SoundEvent(_SoundEvent, _pSoundName);
			break;
		}
	}
}

aint CMWnd_CubeOptionButton::OnMessage(const CMWnd_Message* _pMsg)
{
	if(_pMsg->m_Msg == WMSG_KEY)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys)
			return 0;

		CScanKey Key;
		Key.m_ScanKey32 = _pMsg->m_Param0;
		Key.m_Char = _pMsg->m_Param1;

		CScanKey OriginalKey;
		OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
		OriginalKey.m_Char = _pMsg->m_Param3;

		if (!Key.IsDown())
			return CMWnd_ModButton::OnMessage(_pMsg);

		//MACRO_GetRegisterObject(CConsole*, pCon, "SYSTEM.CONSOLE");
		//m_pSystem
		int Direction = 0;
		if (Key.GetKey9() == SKEY_MOUSE2)
		{
			if (GetFrontEndInfo() && GetFrontEndInfo()->m_pFrontEnd)
			{
				CMWnd *pCursor = GetFrontEndInfo()->m_pFrontEnd->GetCursorWindow(false);
				if (!pCursor || pCursor->m_Group != m_Group)
					return CMWnd_ModButton::OnMessage(_pMsg);
			}
			Direction = -1;
		}
		else if(Key.GetKey9() == SKEY_GUI_OK)
		{
			// get system

			if (OriginalKey.GetKey9() == SKEY_MOUSE1)
			{
				if (GetFrontEndInfo() && GetFrontEndInfo()->m_pFrontEnd)
				{
					CMWnd *pCursor = GetFrontEndInfo()->m_pFrontEnd->GetCursorWindow(false);
					if (!pCursor || pCursor->m_Group != m_Group)
						return CMWnd_ModButton::OnMessage(_pMsg);
				}
			}
			Direction = 2;
		}
		else if (Key.GetKey9() == SKEY_GUI_LEFT)
			Direction = -1;
		else if (Key.GetKey9() == SKEY_GUI_RIGHT)
			Direction = 1;

		if(Direction > 1)
		{
			if (m_OptionList.Len())
			{
				CStr CurrentValue = pSys->GetRegistry()->GetValue(m_Option);
				CRegistry *pOptionList = pSys->GetRegistry()->GetDir(m_OptionList);
				if (pOptionList)
				{
					int SetValue = 0;
					for (int i = 0; i < pOptionList->GetNumChildren(); ++i)
					{
						if (pOptionList->GetValue(i) == CurrentValue)
						{
							SetValue = i + 1;
							break;
						}
					}

					if (SetValue >= pOptionList->GetNumChildren())
						SetValue = 0;

					pSys->GetRegistry()->SetValue(m_Option, pOptionList->GetValue(SetValue));

					SoundEvent(WSND_SWITCH);
					//ConExecute("cg_playsound (\"GUI_Select_01\")");
					//ConExecute("cg_playsound(\"gui_m_open\")");
				}
			}
			else if(m_Steps == 1)
			{
				fp32 Value = pSys->GetRegistry()->GetValuef(m_Option, m_Min, 1);

				fp32 Amount = (Value - m_Min) / (m_Max-m_Min);
				if(Amount > 0.5f)
					pSys->GetRegistry()->SetValuef(m_Option, m_Min);
				else
					pSys->GetRegistry()->SetValuef(m_Option, m_Max);

				SoundEvent(WSND_SWITCH);
				//ConExecute("cg_playsound (\"GUI_Select_01\")");
				//ConExecute("cg_playsound(\"gui_m_open\")");
			}
			else
			{
				MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
				if(!pGame || pGame->m_spFrontEnd == NULL)
					return 0;

				CWFrontEnd_Mod *pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>(pGame->m_spFrontEnd.p);
				if(!pFrontEndMod)
					return 0;


				// we must find the option meter to get the correct rect
				CRct Rect = GetPosition();

				CMWnd *pParent = GetParent();
				if(pParent)
				{
					for(int32 i = 0; i < pParent->GetNumItems(); i++)
					{
						CMWnd *pChild = pParent->GetItem(i);
						if(pChild && pChild->m_Group == m_Group && pChild != this)
						{
							CMWnd_CubeOptionMeter *pMeter = TDynamicCast<CMWnd_CubeOptionMeter>(pChild);
							if(pMeter)
							{
								Rect = pMeter->GetPosition();
								break;
							}
						}
					}
				}
				//CubeOptionMeter

				fp32 Change = 640.0f/20.0f;
				//Rect.p0.x-=Change;
				//Rect.p1.x-=Change;
				CPnt MousePos = pFrontEndMod->GetTransformedMousePosition();

				MousePos.x *= int(640/(fp32)pFrontEndMod->m_ScreenSize.x); // GUI is always in 640x480
				MousePos.y *= int(480/(fp32)pFrontEndMod->m_ScreenSize.y);

//				fp32 Value = pSys->GetRegistry()->GetValuef(m_Option, m_Min);
				fp32 Amount =  (MousePos.x-(Rect.p0.x + Change*0.5))/(fp32)(Rect.p1.x - Rect.p0.x - Change);// + 1/(fp32)m_Steps;
				Amount = Amount < 0 ? 0 : (Amount > 1 ? 1 : Amount); // 0 <= Amount <= 1

				pSys->GetRegistry()->SetValuef(m_Option, m_Min+Amount*(m_Max-m_Min));

				SoundEvent(WSND_SWITCH);
				//ConExecute("cg_playsound (\"GUI_Select_01\")");
				//ConExecute("cg_playsound(\"gui_m_open\")");
			}

			// update
			MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			if (pCon)
				pCon->ExecuteString("option_update()");
		}
		else if (Direction == -1 || Direction == 1)
		{
			// get system
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (!pSys)
				return 0;
			if (m_OptionList.Len())
			{
				CStr CurrentValue = pSys->GetRegistry()->GetValue(m_Option);
				CRegistry *pOptionList = pSys->GetRegistry()->GetDir(m_OptionList);
				if (pOptionList)
				{
					int SetValue = 0;
					int CurValue = -1;
					for (int i = 0; i < pOptionList->GetNumChildren(); ++i)
					{
						if (pOptionList->GetValue(i) == CurrentValue)
						{
							CurValue = i;
							break;
						}
					}

					SetValue = CurValue + Direction;

					SetValue = Clamp(SetValue, 0, pOptionList->GetNumChildren()-1);

					if(SetValue != CurValue)
					{
						if(m_SfxVol)
							ConExecute("cg_playsound_sfxvol(\"gui_m_mosl001\")");
						else
						{
							SoundEvent(WSND_SWITCH);
							//ConExecute("cg_playsound (\"GUI_Select_01\")");
							//ConExecute("cg_playsound(\"gui_m_open\")");
						}
					}

					pSys->GetRegistry()->SetValue(m_Option, pOptionList->GetValue(SetValue));

					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if (pCon)
						pCon->ExecuteString("option_update()");
				}
			}
			else 
			{
				// get value
				fp32 Value = pSys->GetRegistry()->GetValuef(m_Option, m_Min, 1);
				fp32 Amount = (Value - m_Min) / (m_Max-m_Min);

				int32 Step = RoundToInt(Amount*m_Steps);
				int32 Before = Step;
				// Value = 0-1

				Step += Direction;
				// clamp
				Step = Min(Step, m_Steps);
				Step = Max(Step, (int32)0);

				// clamp
				//Value = Min(Value, m_Max);
				//Value = Max(Value, m_Min);
				fp32 SetValue = 0;

				if(m_Steps == 1)
					SetValue = (Step/(fp32)m_Steps);
				else
					SetValue = (Step/(fp32)m_Steps)*(m_Max-m_Min)+m_Min;//+(1/(fp32)m_Steps)/2;

				if(Step == 0)
					SetValue = m_Min;
				//pSys->GetRegistry()->SetValuef(m_Option, Value);

				SetValue = Min(m_Max, SetValue);
				SetValue = Max(m_Min, SetValue);
				
				pSys->GetRegistry()->SetValuef(m_Option, SetValue);
				
				
				// update
				MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
				if (pCon)
					pCon->ExecuteString("option_update()");

				if(Before != Step)
				{
					if(m_SfxVol)
						ConExecute("cg_playsound_sfxvol(\"gui_m_mosl001\")");
					else
					{
						SoundEvent(WSND_SWITCH);
						//ConExecute("cg_playsound (\"GUI_Select_01\")");
						//ConExecute("cg_playsound(\"gui_m_open\")");
					}
				}
			}

		}
		else
			return CMWnd_ModButton::OnMessage(_pMsg);
		
		return true;
	}

	return CMWnd_CubeButton::OnMessage(_pMsg);
}

// -------------------------------------------------------------------
//  CMWnd_CubeSwitchButton
// -------------------------------------------------------------------

CMWnd_CubeSwitchButton::CMWnd_CubeSwitchButton()
{
	m_iSelected = 0;
	m_AlwaysPaint = true;
}

void CMWnd_CubeSwitchButton::OnCreate()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPTG");

	if(m_bStrValue)
	{
		CStr Value = pGlobalOpt->GetValue(m_OptionKey);
		for(int i = 0; i < m_lStrValues.Len(); i++)
		{
			if(Value == m_lStrValues[i])
			{
				m_iSelected = i;
				if(m_bDisplayStrValue)
					m_Text = CStrF("sc, %s", m_lDisplayStrValues[m_iSelected].Str());
				else
					m_Text = CStrF("sc, %i", m_lDisplayIntValues[m_iSelected]);
			}
		}
	}
	else
	{
		int32 Value = pGlobalOpt->GetValuei(m_OptionKey);
		for(int i = 0; i < m_lIntValues.Len(); i++)
		{
			if(Value == m_lIntValues[i])
			{
				m_iSelected = i;
				if(m_bDisplayStrValue)
					m_Text = CStrF("sc, %s", m_lDisplayStrValues[m_iSelected].Str());
				else
					m_Text = CStrF("sc, %i", m_lDisplayIntValues[m_iSelected]);
			}
		}
	}
}

void CMWnd_CubeSwitchButton::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "OPTION")
		m_OptionKey = _Value;
	else if(_Key == "VALUES")
	{
		CStr Value = _Value;
		int num_values = Value.GetNumMatches(",") + 1;
		CStr t = Value.GetStrSep(":");

		if(t == "str")
			m_bStrValue = true;
		else
			m_bStrValue = false;

		if(m_bStrValue)
			m_lStrValues.SetLen(num_values);
		else
			m_lIntValues.SetLen(num_values);
		for(int i = 0; i < num_values; i++)
		{
			if(m_bStrValue)
			{
				CStr v = Value.GetStrSep(",");
				m_lStrValues[i] = v.Str();
			}
			else
			{
				int v = Value.GetIntSep(",");
				m_lIntValues[i] = v;
			}
		}
	}
	else if(_Key == "DISPLAYVALUES")
	{
		CStr Value = _Value;
		int num_values = Value.GetNumMatches(",") + 1;
		CStr t = Value.GetStrSep(":");

		if(t == "str")
			m_bDisplayStrValue = true;
		else
			m_bDisplayStrValue = false;

		if(m_bDisplayStrValue)
			m_lDisplayStrValues.SetLen(num_values);
		else
			m_lDisplayIntValues.SetLen(num_values);
		for(int i = 0; i < num_values; i++)
		{
			if(m_bDisplayStrValue)
			{
				CStr v = Value.GetStrSep(",");
				m_lDisplayStrValues[i] = v.Str();
			}
			else
			{
				int v = Value.GetIntSep(",");
				m_lDisplayIntValues[i] = v;
			}
		}
	}
	else
		CMWnd_CubeButton::EvaluateKey(_pParam, _Key, _Value);
}

aint CMWnd_CubeSwitchButton::OnMessage(const CMWnd_Message* _pMsg)
{
	if(_pMsg->m_Msg == WMSG_KEY)
	{
		CScanKey Key;
		Key.m_ScanKey32 = _pMsg->m_Param0;
		Key.m_Char = _pMsg->m_Param1;

		if (!Key.IsDown())
			return CMWnd_CubeButton::OnMessage(_pMsg);
		if(Key.GetKey9() == SKEY_GUI_OK)
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPTG");

			m_iSelected++;
			if(m_bDisplayStrValue)
			{
				if(m_lDisplayStrValues.Len() == m_iSelected)
				{
					m_iSelected = 0;
				}
				m_Text = CStrF("sc, %s", m_lDisplayStrValues[m_iSelected].Str());
			}
			else
			{
				if(m_lDisplayIntValues.Len() == m_iSelected)
				{
					m_iSelected = 0;
				}
				m_Text = CStrF("sc, %i", m_lDisplayIntValues[m_iSelected]);
			}

			if(m_bStrValue)
				pGlobalOpt->SetValue(m_OptionKey.Str(), m_lStrValues[m_iSelected].Str());
			else
				pGlobalOpt->SetValuei(m_OptionKey.Str(), m_lIntValues[m_iSelected]);
		}
	}
	return CMWnd_CubeButton::OnMessage(_pMsg);
}

// -------------------------------------------------------------------
//  CMWnd_CubeText
// -------------------------------------------------------------------
void CMWnd_CubeText::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "TEXT")
	{
		CStr Value = _Value;
		m_Text = _Value;

		CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
		if(pLayout)
			pLayout->Layout_WriteText(_pParam->m_Pos, Value);
	}
	else if(_Key == "ALWAYSPAINT")
		m_AlwaysPaint = true;

	CMWnd_CubeRgn::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeText::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	if(m_AlwaysPaint)
	{
		CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
		if(pLayout)
		{
			pLayout->Layout_Clear(GetPosition(), 0);
			CRct Pos = GetPosition();
			pLayout->Layout_WriteText(Pos, m_Text, 0);
		}
	}
}



// -------------------------------------------------------------------
//  CMWnd_CubeMeter
// -------------------------------------------------------------------
void CMWnd_CubeMeter::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CMWnd_CubeRgn::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMeter::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
	if(pLayout)
	{
		fp32 Amount = GetValue();

		CRct Pos = GetPositionCube();
//		int32 w = RoundToInt((Pos.p1.x-Pos.p0.x)*Amount);
		for(int32 i = Pos.p0.x+1; i < Pos.p1.x-1; i++)
			pLayout->Layout_SetMapping(i, Pos.p0.y, 16*16+11+(i%4), CCellInfo::FLAG_FASTSWITCH);

		pLayout->Layout_SetMapping(Pos.p0.x, Pos.p0.y, 16*16+11-1, CCellInfo::FLAG_FASTSWITCH);
		pLayout->Layout_SetMapping(Pos.p1.x-1, Pos.p0.y, 16*16+11+4, CCellInfo::FLAG_FASTSWITCH);

		int32 pos = Pos.p0.x + (int32)RoundToInt(Amount*(Pos.p1.x-Pos.p0.x-1));
		
		if(pos == Pos.p0.x)
			pLayout->Layout_SetMapping(pos, Pos.p0.y, 16*16+11-1-3, CCellInfo::FLAG_FASTSWITCH);
		else if(pos == Pos.p1.x-1)
			pLayout->Layout_SetMapping(pos, Pos.p0.y, 16*16+11-1-1, CCellInfo::FLAG_FASTSWITCH);
		else
			pLayout->Layout_SetMapping(pos, Pos.p0.y, 16*16+11-1-2, CCellInfo::FLAG_FASTSWITCH);

		//for(int32 i = 0; i < w; i++)
		//	
	}
}

// -------------------------------------------------------------------
//  CMWnd_CubeOptionMeter
// -------------------------------------------------------------------
CMWnd_CubeOptionMeter::CMWnd_CubeOptionMeter()
{
	m_Max = 1;
	m_Min = 0;
}

void CMWnd_CubeOptionMeter::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "OPTION")
		m_Option = _Value;
	else if(_Key == "MAX")
		m_Max = _Value.Val_fp64();
	else if(_Key == "MIN")
		m_Max = _Value.Val_fp64();
	else
		CMWnd_CubeMeter::EvaluateKey(_pParam, _Key, _Value);
}

fp32 CMWnd_CubeOptionMeter::GetValue()
{
	// get system
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys)
		return 0;

	// get value
	fp32 Value = pSys->GetRegistry()->GetValuef(m_Option, m_Min, 1);
	return (Value-m_Min)/(m_Max-m_Min);
}

// -------------------------------------------------------------------
//  CMWnd_CubeLayout
// -------------------------------------------------------------------
CMWnd_CubeLayout::CMWnd_CubeLayout()
{
	Layout_SetSecondary(-1, CRC_RASTERMODE_NONE);

	for(int32 x = 0; x < 20; x++)
		for(int32 y = 0; y < 20; y++)
		{
			m_aMap[x][y].m_Mode = CCellInfo::MODE_NORMAL;
			m_aMap[x][y].Cell()	= CCellInfo::Empty();
			m_aMap[x][y].Flags() = 0;
//			m_aMap[x][y].IsOK();
			m_aDepth[x][y] = 0; //Random*0.0075f;
		}

	m_iFirstListItem = 0;
	m_iCurrentListItem = 0;
	m_iTentacleTemplate = 0; // default
}

CMWnd_CubeLayout::~CMWnd_CubeLayout()
{
}

void CMWnd_CubeLayout::Layout_SetSecondary(int32 _iSomething, int32 _Blend)
{
	m_iSecondaryID = _iSomething;
	m_BlendMode = _Blend;
}


void CMWnd_CubeLayout::Layout_AddLookup(int32 _x, int32 _y)
{
	CCellInfo Info;
	Info.m_Mode = CCellInfo::MODE_NORMAL;
	Info.Cell() = _x+_y*16;
	Info.Flags() = 0;
	m_lLookup.Add(Info);
}

bool CMWnd_CubeLayout::Layout_WndIsButton(CMWnd *pWnd)
{
	for(int32 i = 0; i < m_lButtons.Len(); i++)
		if(m_lButtons[i] == pWnd)
			return true;
	return false;
}

void CMWnd_CubeLayout::EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg)
{
	CMWnd::EvaluateRegisty(_pParam, _pReg);

	if (_pReg->FindChild("DEFAULTLOOKUP"))
	{
		Layout_AddLookup(14,15); // 1
		Layout_AddLookup(15,15); // 2
		Layout_AddLookup(0,16); // 3
		Layout_AddLookup(1,16); // 4
		Layout_AddLookup(2,16); // 5
		Layout_AddLookup(3,16); // 6

		Layout_AddLookup(10,16); // 7
		Layout_AddLookup(11,16); // 8
		Layout_AddLookup(12,16); // 9
		Layout_AddLookup(13,16); // 10
		Layout_AddLookup(14,16); // 11
		Layout_AddLookup(15,16); // 12

		Layout_AddLookup(0,15); // 13
		
		Layout_AddLookup(6,16); // 14
	}

	/*
	CStr Value = _Value;
	if(_Key == "LOOKUP")
	{
		int32 x = Value.GetStrSep(",").Val_int();
		int32 y = Value.GetStrSep(",").Val_int();
		Layout_AddLookup(x, y);
	}
	else 
	*/
	for (int i = 0; i < 20; ++i)
	{
		if(_pReg->FindChild(CStrF("CELLROW%d",i)))
		{
			CStr Value = _pReg->GetValue(CStrF("CELLROW%d",i), "");
			int32 Row = i;
			if(Row >= 0 && Row < 20)
			{
				for(int32 i = 0; i < 20; i++)
				{
					int32 Lookup = Value.GetStrSep(" ").Val_int()-1;
					
					if(Lookup >= 0 && Lookup < m_lLookup.Len())
						m_aMap[Row][i] = m_lLookup[Lookup];
				}
			}
		}
	}
	/*
	else if(_Key.CompareSubStr("DEPTHROW") == 0)
	{
		CStr Value = _Value;
		int32 Row = _Key.SubStr(8,2).Val_int();
		if(Row >= 0 && Row < 20)
		{
			for(int32 i = 0; i < 20; i++)
			{
				fp32 Depth = Value.GetStrSep(" ").Val_fp64();

				if(Depth > 0.00001f)
					m_aDepth[Row][i] = Depth*(1/20.0f);
			}
		}
	}*/
	
	if(_pReg->FindChild("CREATE_TEXT_INPUT_BUTTON_MAP"))
	{
		CRegistry *Reg = REGISTRY_CREATE;
		int8 aChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789_";
		//int8 aChars[] = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		//int8 aChars[] = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

		int32 StartX = 4;
		int32 x = StartX, y = 8;
		for(int32 i = 0; i < sizeof(aChars)-1; i++)
		{

			if(aChars[i] != '#' && aChars[i] != '\n')
			{
				Reg->SetValue("ID", CStrF("KEY_%c", aChars[i]));
				Reg->SetValue("RGN", CStrF("%d,%d,1,1", x, y));
				
				if(aChars[i] == ' ')
					Reg->SetValue("TEXT", CStr("n, _"));
				else
					Reg->SetValue("TEXT", CStrF("n, %c", aChars[i]));

				CStr Class = "CMWnd_CubeButton";
				spCMWnd spWnd = (CMWnd*)MRTC_GetObjectManager()->CreateObject(Class);
				if(!spWnd)
					Error("CreateChildren", "Invalid classname: " + Class);
				spWnd->Create(Reg, this);
			}

			x += 1;
			if(x > 12+StartX || aChars[i] == '\n')
			{
				x = StartX;
				y += 1;
			}
		}
		delete Reg;
	}

}

void CMWnd_CubeLayout::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CMWnd::EvaluateKey(_pParam, _Key, _Value);
}


// dog slow
int32 Translate(int32 _Char, int32 Font)
{
	const int aFontStart[] = {(16*17), (32*27)};
	char aTranslationString[] = "\
!\"#$%&'()*+,-./0\
123456789:;<=>?@\
ABCDEFGHIJKLMNOP\
QRSTUVWXYZ[\\]^_`\
abcdefghijklmnop\
qrstuvwxyz{|}~×à\
áâãäåæçèéêëìíîïð\
ñòóôõöøùúûüýÀÁÂÃ\
ÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓ\
ÔÕÖ×ØÙÚÛÜÝ¿ßºvæ¦";
	wchar aWideTrans[1024];
	Localize_Str(aTranslationString, aWideTrans, 1023);

	for(int32 t = 0; aWideTrans[t] != 0; t++)
	{
		if(aWideTrans[t] == _Char)
			return aFontStart[Font]+t;
	}

	return CCellInfo::Empty();
}

void WriteSmall(CMWnd_CubeLayout *_pLayout, int32 x, int32 y, wchar c, int32 FontIndex, int32 _Flags)
{
	if(y < 0 || y >= 20)
		return;
	if(x < 0 || (x>>1) >= 20)
		return;

	M_ASSERT(y >= 0 && y < 20, "layout error");
	M_ASSERT(x >= 0 && (x>>1) < 20, "layout error");

	if((x%2) == 0)
	{
		_pLayout->m_aMap[y][x>>1].m_Mode = CCellInfo::MODE_SMALL;
		_pLayout->m_aMap[y][x>>1].SubCell(0) = CCellInfo::Empty();
		_pLayout->m_aMap[y][x>>1].SubCell(1) = CCellInfo::Empty();
		_pLayout->m_aMap[y][x>>1].Flags() = _Flags;
		//_pLayout->m_aMap[y][x>>1].IsOK();
	}

	if(c != wchar(' '))
	{
		_pLayout->m_aMap[y][x>>1].SubCell(x%2) = Translate(c, FontIndex);
		_pLayout->m_aMap[y][x>>1].Flags() = _Flags;
		//_pLayout->m_aMap[y][x>>1].IsOK();
	}
}

//
//
//
/*
void CMWnd_CubeLayout::Layout_Clear(CRct &_rPos)
{

}*/


CRct CMWnd_CubeLayout::Layout_Viewspace2CubeSpace(CRct _Pos)
{
	_Pos.p0.x = int(_Pos.p0.x / (640/20.0f));
	_Pos.p0.y = int(_Pos.p0.y / (480/20.0f));
	_Pos.p1.x = int(_Pos.p1.x / (640/20.0f));
	_Pos.p1.y = int(_Pos.p1.y / (480/20.0f));
	return _Pos;
}

CRct CMWnd_CubeLayout::Layout_CubeSpace2ViewSpace(CRct _Pos)
{
	_Pos.p0.x = int(_Pos.p0.x / (640/20.0f));
	_Pos.p0.y = int(_Pos.p0.y / (480/20.0f));
	_Pos.p1.x = int(_Pos.p1.x / (640/20.0f));
	_Pos.p1.y = int(_Pos.p1.y / (480/20.0f));
	return _Pos;
}


//
//
//
void CMWnd_CubeLayout::Layout_WriteText(CRct &_rPos, int32 FontSize, const wchar *aText, int32 _Flags)
{
	int32 TextLen = CStrBase::StrLen(aText);

	int32 x = _rPos.p0.x;
	int32 y = _rPos.p0.y;
	int32 ex = _rPos.p1.x;
	int32 ey = _rPos.p1.y;

	if(!(_Flags&FLAG_NO_CONVERT))
	{
		x = int(x / (640/20.0f));
		y = int(y / (480/20.0f));
		ex = int(ex / (640/20.0f));
		ey = int(ey / (480/20.0f));
	}

	int32 Width = 0;
	int32 Height = 0;
	int32 FontIndex = 0;

	if(FontSize == 'n')
	{
		Width = TextLen;
		Height = 1;
	}
	else if(FontSize == 's')
	{
		Width = (TextLen+1)/2;
		Height = 1;
		FontIndex = 1;
	}
	else if(FontSize == 'h')
	{
		Width = TextLen*2;
		Height = 2;
	}

	bool bAddFirstSmall = false;

	if(_Flags&FLAG_CENTER)
	{
		if (FontSize == 's')
		{
			int Temp = (ex-x)-Width;
			bAddFirstSmall = (Temp/2)*2 != Temp;

			x = x + Temp/2;
		}
		else
			x = x+((ex-x)-Width) / 2;
	}

	if(x < 0) x = 0;
	if(y < 0) y = 0;

	int32 sx = x;
	int32 sy = y;

	if(FontSize == 'n')
	{
		CStr Text = aText;

		//x <<= 1;
		//ex <<= 1;
		//sx <<= 1;

		while(Text.Len())
		{
			CStr CompleteLine = Text.GetStrSep("\n");
			CStr Line = CompleteLine.GetStrSep("||");

			if(CompleteLine.Len())
				Text = CompleteLine+"\n"+Text;

			while(Line.Len())
			{
				//CStr Word = Text.GetStrSep(" ");
				CStr Word = Line.GetStrSep(" ");
				int32 wordlen = Word.Len();
				if(!(_Flags&FLAG_NO_WRAP))
				{
					if(wordlen > ex-x)
					{
						//for(int32 i = x; i < ex; i++)
						//	WriteSmall(_pLayout, x++, y, wchar(' '), FontIndex);
						x = sx;
						y++;
					}
				}

				for(int32 i = 0; i < wordlen; i++)
				{
					m_aMap[y][x+i].m_Mode = CCellInfo::MODE_NORMAL;
					m_aMap[y][x+i].Cell() = Translate(Word.StrW()[i], FontIndex);
					m_aMap[y][x+i].Flags() = _Flags&FLAG_FASTSWITCH ? CCellInfo::FLAG_FASTSWITCH : 0;
					//m_aMap[y][x+i].IsOK();
				}
				/*
				WriteSmall(this, x++, y, Word.StrW()[i], FontIndex, _Flags);
				if(x < ex)
					WriteSmall(this, x++, y, wchar(' '), FontIndex, _Flags);
					*/
				x++; // space

				x += wordlen;
				//Word = Text.GetStrSep(" ");
			}

			if(Text.Len())
			{
				x = sx;
				y++;
			}
		}

		//sx >>= 1;


		/*
		// normal
		if(y >= 0 && y < 20)
		{
			for(int32 i = 0; i < TextLen; i++)
			{
				if(x+i >= 20)
					break;

				//if(_Flags&FLAG_KEEP_BOUNDS && x+i > _rPos.p1.x)
				//	break;

				m_aMap[y][x+i].m_Mode = CCellInfo::MODE_NORMAL;
				m_aMap[y][x+i].Cell() = Translate(aText[i], FontIndex);
				m_aMap[y][x+i].Flags() = _Flags;
			}
		}
		*/
	}
	else if(FontSize == 's')
	{
		// small
		CStr Text = aText;

		x <<= 1;
		ex <<= 1;
		sx <<= 1;

		if (bAddFirstSmall)
			WriteSmall(this, x++, y, wchar(' '), FontIndex, _Flags&FLAG_FASTSWITCH ? CCellInfo::FLAG_FASTSWITCH : 0);

		while(Text.Len())
		{
			CStr CompleteLine = Text.GetStrSep("\n");
			CStr Line = CompleteLine.GetStrSep("||");

			if(CompleteLine.Len())
				Text = CompleteLine+"\n"+Text;

			Line.Trim();

			while(Line.Len())
			{
				//CStr Word = Text.GetStrSep(" ");
				CStr Word = Line.GetStrSep(" ");
				int32 wordlen = Word.Len();
				if(!(_Flags&FLAG_NO_WRAP))
				{
					if(wordlen > ex-x)
					{
						//for(int32 i = x; i < ex; i++)
						//	WriteSmall(_pLayout, x++, y, wchar(' '), FontIndex);
						x = sx;
						y++;
					}
				}

				for(int32 i = 0; i < wordlen; i++)
					WriteSmall(this, x++, y, Word.StrW()[i], FontIndex, _Flags&FLAG_FASTSWITCH ? CCellInfo::FLAG_FASTSWITCH : 0);
				if(Line.Len())
					WriteSmall(this, x++, y, wchar(' '), FontIndex, _Flags&FLAG_FASTSWITCH ? CCellInfo::FLAG_FASTSWITCH : 0);
				//x++; // space

				//x += wordlen;
				//Word = Text.GetStrSep(" ");
			}

			if(Text.Len())
			{
				x = sx;
				y++;
			}
		}

		sx >>= 1;

		/*
		for(int32 i = 0; i < TextLen; i += 2)
		{
			_pLayout->m_aMap[y][x+i/2].m_Mode = CCellInfo::MODE_SMALL;

			for(int32 c = 0; c < 2; c++)
			{
				if(aText[i+c] == ' ')
					_pLayout->m_aMap[y][x+i/2].SubCell(c) = Random*RANDOM_BELOW;
				else
					_pLayout->m_aMap[y][x+i/2].SubCell(c) = Translate(aText[i+c], FontIndex);
			}
		}*/
	}
	else if(FontSize == 'h')
	{
		// huge
		for(int32 i = 0; i < TextLen; i++)
		{
			if(aText[i] == ' ')
				continue;

			if(y+1 >= 20)
				continue;

			if(x+i*2+1 >= 20)
				continue;

			M_ASSERT(y >= 0 && y < 20, "layout error");
			M_ASSERT(x >= 0 && x < 20, "layout error");
			M_ASSERT((y+1) < 20, "layout error");
			M_ASSERT((x+i*2+1) < 20, "layout error");

			m_aMap[y  ][x+i*2  ].m_Mode = CCellInfo::MODE_HUGE;
			m_aMap[y+1][x+i*2  ].m_Mode = CCellInfo::MODE_HUGE;
			m_aMap[y  ][x+i*2+1].m_Mode = CCellInfo::MODE_HUGE;
			m_aMap[y+1][x+i*2+1].m_Mode = CCellInfo::MODE_HUGE;

			m_aMap[y  ][x+i*2  ].Part() = 0;//CCellInfo::PART_TL;
			m_aMap[y+1][x+i*2  ].Part() = 1;//CCellInfo::PART_BL;
			m_aMap[y  ][x+i*2+1].Part() = 2;//CCellInfo::PART_TR;
			m_aMap[y+1][x+i*2+1].Part() = 3;//CCellInfo::PART_BR;
			

			uint16 c = Translate(aText[i], FontIndex);
			m_aMap[y  ][x+i*2  ].Cell() = c;
			m_aMap[y+1][x+i*2  ].Cell() = c;
			m_aMap[y  ][x+i*2+1].Cell() = c;
			m_aMap[y+1][x+i*2+1].Cell() = c;
		}
	}

	if(_Flags&FLAG_ADJUST_SIZE)
	{
		_rPos.p0.x = int(sx*(640/20.0f));
		_rPos.p0.y = int(sy*(480/20.0f));
		_rPos.p1.x = int(_rPos.p0.x + Width*(640/20.0f));
		//_rPos.p1.y = _rPos.p0.y + Height*(480/20.0f)+1;
	}
}

void CMWnd_CubeLayout::Layout_SetMappingBig(int32 _x, int32 _y, int32 _ID, int32 _Flags)
{
	if(_x < 0 || _y < 0 || _x >= 19 || _y >= 19)
		return;

	for(int32 ay = 0; ay < 2; ay++)
		for(int32 ax = 0; ax < 2; ax++)
		{
			m_aMap[_y+ay][_x+ax].m_Mode = CCellInfo::MODE_HUGE;
			m_aMap[_y+ay][_x+ax].Cell() = _ID;
		}

	m_aMap[_y][_x].Part() = 0;
	m_aMap[_y+1][_x].Part() = 1;
	m_aMap[_y][_x+1].Part() = 2;
	m_aMap[_y+1][_x+1].Part() = 3;
}

void CMWnd_CubeLayout::Layout_SetMapping(int32 _x, int32 _y, int32 _ID, int32 _Flags)
{
	if(_x < 0 || _y < 0 || _x >= 20 || _y >= 20)
		return;
	m_aMap[_y][_x].m_Mode = CCellInfo::MODE_NORMAL;
	m_aMap[_y][_x].Cell() = _ID;
	m_aMap[_y][_x].Flags() = _Flags;
	//m_aMap[_y][_x].IsOK();
}

void CMWnd_CubeLayout::Layout_SetMapping(CRct _rPos, int32 _ID, int32 _Flags)
{
	int32 sx = _rPos.p0.x;//(640/20.0f);
	int32 sy = _rPos.p0.y;//(480/20.0f);
	int32 ex = _rPos.p1.x;//(640/20.0f);
	int32 ey = _rPos.p1.y;//(480/20.0f);

	if(sx < 0) sx = 0;
	if(sy < 0) sy = 0;
	if(ex < 0) ex = 0;
	if(ey < 0) ey = 0;
	if(ex > 20) ex = 20;
	if(ey > 20) ey = 20;

	for(int32 y = sy; y < ey; y++)
		for(int32 x = sx; x < ex; x++)
		{
			m_aMap[y][x].m_Mode = CCellInfo::MODE_NORMAL;
			m_aMap[y][x].Cell() = _ID;
			if(_ID == 0)
				m_aMap[y][x].Cell() = CCellInfo::Empty();
			m_aMap[y][x].Flags() = _Flags;
			//m_aMap[y][x].IsOK();
		}
}



void CMWnd_CubeLayout::Layout_Copy(CMWnd_CubeLayout *pLayout)
{
	for(int32 y = 0; y < 20; y++)
		for(int32 x = 0; x < 20; x++)
		{
			m_aMap[y][x] = pLayout->m_aMap[y][x];
			m_aDepth[y][x] = pLayout->m_aDepth[y][x];
		}

}

//
//
//
void CMWnd_CubeLayout::Layout_SetDepth(CRct _rPos, fp32 _Depth)
{
	int32 sx = _rPos.p0.x;//(640/20.0f);
	int32 sy = _rPos.p0.y;//(480/20.0f);
	int32 ex = _rPos.p1.x;//(640/20.0f);
	int32 ey = _rPos.p1.y;//(480/20.0f);

	if(sx < 0) sx = 0;
	if(sy < 0) sy = 0;
	if(ex < 0) ex = 0;
	if(ey < 0) ey = 0;
	if(ex > 20) ex = 20;
	if(ey > 20) ey = 20;

	for(int32 y = sy; y < ey; y++)
		for(int32 x = sx; x < ex; x++)
			m_aDepth[y][x] = _Depth;
}

void CMWnd_CubeLayout::Layout_Clear(CRct _rPos, int32 _Flags)
{
	int32 sx = int32(_rPos.p0.x/(640/20.0f));
	int32 sy = int32(_rPos.p0.y/(480/20.0f));
	int32 ex = int32(_rPos.p1.x/(640/20.0f));
	int32 ey = int32(_rPos.p1.y/(480/20.0f));

	if(sx < 0) sx = 0;
	if(sy < 0) sy = 0;
	if(ex < 0) ex = 0;
	if(ey < 0) ey = 0;
	if(ex > 20) ex = 20;
	if(ey > 20) ey = 20;

	for(int32 y = sy; y < ey; y++)
		for(int32 x = sx; x < ex; x++)
		{
			M_ASSERT(y >= 0 && y < 20, "layout error");
			M_ASSERT(x >= 0 && x < 20, "layout error");
			m_aMap[y][x].m_Mode = CCellInfo::MODE_NORMAL;
			m_aMap[y][x].Cell() = CCellInfo::Empty();
			m_aMap[y][x].Flags() = _Flags;
///			m_aMap[y][x].IsOK();
		}
}

//
//
//
void CMWnd_CubeLayout::Layout_WriteText(CRct &_rPos, CStr _Value, int32 _Flags)
{
	CStr Font = _Value.GetStrSep(",");
	CStr TextStr = _Value;
	int32 Center = Font.Find("c") >= 0 ? FLAG_CENTER : 0;
	
	CStr FontSize = Font.SubStr(0, 1);

	TextStr.Trim();
	if(TextStr.IsUnicode())
		Layout_WriteText(_rPos, FontSize[0], TextStr, _Flags|Center);
	else
	{
		wchar aText[1024];
		Localize_Str(TextStr, aText, 1023);
		Layout_WriteText(_rPos, FontSize[0], aText, _Flags|Center);
	}
}

//
//
//
void CMWnd_CubeLayout::OnCreate()
{
	// fetch buttons
	CMWnd *pWnd = GetItem("Button");
	int32 Count = 0;

	while(pWnd)
	{
		pWnd->m_ID = "";
		m_lButtons.Add(pWnd);
		pWnd = GetItem("Button");
		Count++;
	};


	CMWnd::OnCreate();
}

// this can override the normal GUI cursor movement to handle the list
int CMWnd_CubeLayout::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	int32 NumItems = GetNumListItems(false);
	if(NumItems && _Key.IsDown())
	{
		bool ListHasFocus = false;

		for(int32 i = 0; i < m_lButtons.Len(); i++)
			if(m_lButtons[i] == GetFocusWnd())
			{
				ListHasFocus = true;
				break;
			}

		if(ListHasFocus)
		{
			int32 k = _Key.GetKey9();

#ifdef PLATFORM_CONSOLE
# define BORDER_STOP (0)
#else
# define BORDER_STOP (1)
#endif
			int32 NumButtons = m_lButtons.Len();

			bool bProcessed = false;
			if ((k == SKEY_GUI_UP || k == SKEY_PAGEUP) && m_iCurrentListItem != 0)
			{
				int nSteps = (k == SKEY_GUI_UP) ? 1 : NumButtons;
				m_iCurrentListItem = Max(int(m_iCurrentListItem - nSteps), 0);
				if (m_iCurrentListItem == 0)
					m_iFirstListItem = 0;
				else 
					m_iFirstListItem = Min(m_iFirstListItem, m_iCurrentListItem-BORDER_STOP);

				int32 index = m_iCurrentListItem - m_iFirstListItem;
				M_ASSERT(index >= 0 && index < m_lButtons.Len(), "!");
				m_lButtons[index]->SetFocus();
				bProcessed = true;
			}
			else if ((k == SKEY_GUI_DOWN || k == SKEY_PAGEDOWN) && m_iCurrentListItem != NumItems-1)
			{
				int nSteps = (k == SKEY_GUI_DOWN) ? 1 : NumButtons;
				m_iCurrentListItem = Min(m_iCurrentListItem + nSteps, NumItems-1);
				if (m_iCurrentListItem == (NumItems-1))
					m_iFirstListItem = m_iCurrentListItem - Min(NumItems, NumButtons) + 1;
				else
					m_iFirstListItem = Max(m_iFirstListItem, m_iCurrentListItem - Min(NumItems, NumButtons) + 1 + BORDER_STOP);
				int32 index = m_iCurrentListItem - m_iFirstListItem;
				M_ASSERT(index >= 0 && index < m_lButtons.Len(), "!");
				m_lButtons[index]->SetFocus();
				bProcessed = true;
			}

			if (bProcessed)
			{
				SoundEvent(WSND_NAVIGATE);
				return true;
			}
		}
	}

	CMWnd* pOldFocus = GetFocusWnd();
	int Result = CMWnd::ProcessKey(_Key, _OriginalKey);
	if (Result)
	{
		if (pOldFocus != GetFocusWnd())
			SoundEvent(WSND_NAVIGATE);
	}
	
	return Result;
}


//
//
//
void CMWnd_CubeLayout::OnRefresh()
{
	for(int32 i = 0; i < m_lButtons.Len(); i++)
	{
		if(m_lButtons[i] == GetFocusWnd())
		{
			m_iCurrentListItem = m_iFirstListItem+i;
		}

		m_lButtons[i]->OnRefresh();
	}

	m_iCurrentListItem	= MaxMT(0, MinMT(GetNumListItems(true), m_iCurrentListItem));

	int32 NumItems = GetNumListItems(true);
	int32 NumButtons = m_lButtons.Len();

	if(NumItems != -1 && NumButtons > 0)
	{
#ifdef PLATFORM_CONSOLE
		// Auto-scroll
		CMTime Time = CMTime::GetCPU();
		if(m_LastListScroll.IsInvalid() || Time.Compare(m_LastListScroll + CMTime::CreateFromSeconds(0.065f)) > 0)
		{
			if(m_iCurrentListItem <= m_iFirstListItem) 
			{
				m_iFirstListItem = m_iCurrentListItem-1;
				if(m_iFirstListItem < 0)
					m_iFirstListItem = 0;
			}


			if(m_iCurrentListItem >= m_iFirstListItem+NumButtons-1) 
			{
				m_iFirstListItem = m_iCurrentListItem - NumButtons+2;
				if(NumItems-m_iFirstListItem < NumButtons)
					m_iFirstListItem = NumItems - NumButtons; 
			}

			m_LastListScroll = Time;
		}
#endif
	}
}

void CMWnd_CubeLayout::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	//CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_pClient->Game_GetObjectIndex());

	bool ListHasFocus = false;

	int32 NumItems = GetNumListItems(true);
	int32 NumButtons = m_lButtons.Len();
	CMWnd *pFocus = GetFocusWnd();

	if(pFocus == this)
	{
		ListHasFocus = true;
	}
	else if(m_lButtons.ValidPos(m_iCurrentListItem - m_iFirstListItem) && pFocus == m_lButtons[m_iCurrentListItem - m_iFirstListItem])
	{
		ListHasFocus = true;
	}

	if(NumItems != -1 && NumButtons)
	{
		int32 FirstButton = 0;
		int32 LastButton = NumButtons;

		if(m_iFirstListItem > 0)
		{
			CRct PosView = m_lButtons[0]->GetPosition();
			CRct PosCube = Layout_Viewspace2CubeSpace(PosView);
			Layout_Clear(PosView, CCellInfo::FLAG_FASTSWITCH);
			int32 x = PosCube.p0.x + (PosCube.p1.x - PosCube.p0.x)/2;
			Layout_SetMapping(x-2, PosCube.p0.y, 15*16+12, CCellInfo::FLAG_FASTSWITCH);
			Layout_SetMapping(x, PosCube.p0.y, 15*16+12, CCellInfo::FLAG_FASTSWITCH);
			Layout_SetMapping(x+2, PosCube.p0.y, 15*16+12, CCellInfo::FLAG_FASTSWITCH);
			FirstButton = 1;
		}

		if(NumItems-m_iFirstListItem > LastButton)
		{
			LastButton--;
			CRct PosView = m_lButtons[LastButton]->GetPosition();
			CRct PosCube = Layout_Viewspace2CubeSpace(PosView);
			Layout_Clear(PosView, CCellInfo::FLAG_FASTSWITCH);

			int32 x = PosCube.p0.x + (PosCube.p1.x - PosCube.p0.x)/2;
			Layout_SetMapping(x-2, PosCube.p0.y, 15*16+13, CCellInfo::FLAG_FASTSWITCH);
			Layout_SetMapping(x, PosCube.p0.y, 15*16+13, CCellInfo::FLAG_FASTSWITCH);
			Layout_SetMapping(x+2, PosCube.p0.y, 15*16+13, CCellInfo::FLAG_FASTSWITCH);
		}

		for(int32 i = m_iFirstListItem+FirstButton, b = FirstButton; b < LastButton; i++, b++)
		{
			bool bFocus = (i == m_iCurrentListItem) && ListHasFocus;
			CStr Caption;
			if(i < NumItems)
				GetListItem(i, Caption, bFocus);

			//if(Caption.Len())
			//{
			CRct PosView = m_lButtons[b]->GetPosition();
			CRct PosCube = Layout_Viewspace2CubeSpace(PosView);
			//while(Caption.Len() < Pos.p1.x-Pos.p0.x)
			//	Caption += " ";

			m_CurrentItemRect = PosCube;
			Layout_Clear(PosView, CCellInfo::FLAG_FASTSWITCH);
			Layout_WriteText(PosCube, Caption, FLAG_NO_CONVERT|FLAG_NO_WRAP|FLAG_FASTSWITCH); //FLAG_ADJUST_SIZE|

			AfterItemRender(i);

			//m_lButtons[b]->SetPosition(Pos);
			m_lButtons[b]->SetStatus(WSTATUS_DISABLED, !(i < NumItems));

			//if(bFocus)
			//if(bFocus)
			//	m_lButtons[b]->SetFocus();//SetStatus(WSTATUS_FOCUS, bFocus);
				
			//}
		}
	}


	CMWnd::OnPaint(_pRCUtil, _Clip, _Client);
}
// -------------------------------------------------------------------
//  CMWnd_ModEdit
// -------------------------------------------------------------------

CM_Mod_ColorController::CM_Mod_ColorController()
{
	m_fpTransparentScale = 1.0f;
	m_fpFreqScale = 1.0f;
	m_iSlot = 0;
	m_bDisabled = false;
	m_iColorId = 0;
	m_fpTime = 0.0f;
}

int CM_Mod_ColorController::iGetColor(fp32 fpTransparentScale, fp32 fpFreqScale, int iSlot, bool bDisabled, int iColorId)
{
	// Time

	CMTime time = CMTime::GetCPU();

	// Period
	
	fp32 col1 = (M_Sin(time.GetTimeModulusScaled(5.0f*fpFreqScale, _PI2) + (fp32)iSlot*15.0f)*128.0f+128.0f)*fpTransparentScale;

	// Pixel 32

	int col2 = 0;

	int n = 1;

	if (bDisabled)
		n = 2;

	if (iColorId == CM_Mod_ColorController_Blue)
		col2 = CPixel32::From_fp32(180/n, 200/n, 250/n, col1/7 + 105);
	else if (iColorId == CM_Mod_ColorController_Yellow)
		col2 = CPixel32::From_fp32(245/n, 215/n, 190/n, col1/7 + 105);
	else
	{
		//(iColorId == CM_Mod_ColorController_White)
		col2 = CPixel32::From_fp32(220/n, 220/n, 220/n, col1/7 + 105);
	}

	return col2;
}

int CM_Mod_ColorController::iGetColorFrameTimed(fp32 fpTransparentScale, fp32 fpFreqScale, int iSlot, bool bDisabled, int iColorId)
{
	// Time

	m_fpTime += 0.002f;

	// Period
	
	fp32 col1 = (M_Sin(m_fpTime + (fp32)iSlot*15.0f)*128.0f+128.0f)*fpTransparentScale;

	// Pixel 32

	int col2 = 0;

	int n = 1;

	if (bDisabled)
		n = 2;

	if (iColorId == CM_Mod_ColorController_Blue)
		col2 = CPixel32::From_fp32(180/n, 200/n, 250/n, col1/7 + 105);
	else if (iColorId == CM_Mod_ColorController_Yellow)
		col2 = CPixel32::From_fp32(245/n, 215/n, 190/n, col1/7 + 105);
	else
	{
		//(iColorId == CM_Mod_ColorController_White)
		col2 = CPixel32::From_fp32(220/n, 220/n, 220/n, col1/7 + 105);
	}

	return col2;
}

// -------------------------------------------------------------------
//  CMWnd_CubeRgn
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeRgn, CMWnd);

void CMWnd_CubeRgn::EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "RGN")
	{
		CStr Value = _Value;
		_pParam->m_Pos.p0.x = int(Value.GetStrSep(",").Val_fp64()*(640/20));
		_pParam->m_Pos.p0.y = int(Value.GetStrSep(",").Val_fp64()*(480/20));
		_pParam->m_Pos.p1.x = int(Value.GetStrSep(",").Val_fp64()*(640/20));
		_pParam->m_Pos.p1.y = int(Value.GetStrSep(",").Val_fp64()*(480/20));
		_pParam->m_Pos.p1 += _pParam->m_Pos.p0;
		return;
	}

	CMWnd::EvaluateKeyOrdered(_pParam, _Key, _Value);
}

void CMWnd_CubeRgn::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{

	CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

CRct CMWnd_CubeRgn::GetPositionCube()
{
	CRct Rct = GetPosition();
	Rct.p0.x /= (640/20);
	Rct.p1.x /= (640/20);
	Rct.p0.y /= (480/20);
	Rct.p1.y /= (480/20);
	return Rct;
}

// -------------------------------------------------------------------
//  CMWnd_CubeRect
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeSecondaryRect, CMWnd_CubeRgn);

CMWnd_CubeSecondaryRect::CMWnd_CubeSecondaryRect()
{
	m_Set = true;
}

void CMWnd_CubeSecondaryRect::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "SET")
		m_Set = _Value.Val_int() != 0;
	CMWnd_CubeRgn::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeSecondaryRect::OnRefresh()
{
	CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(GetParent());
	pLayout->m_SecondaryRect = GetPositionCube();
	if(pLayout && m_Set)
	{
		CRct Pos = GetPositionCube();
		int32 c = 0;
		for(int32 y = Pos.p0.y; y < Pos.p1.y; y++)
			for(int32 x = Pos.p0.x; x < Pos.p1.x; x++, c++)
			{
				pLayout->m_aMap[y][x].m_Mode = CCellInfo::MODE_SECONDARY;
				pLayout->m_aMap[y][x].Cell() = CCellInfo::Empty();
				pLayout->m_aMap[y][x].Flags() = 0;
			}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModInfoScreen
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModInfoScreen, CMWnd_Edit);

CMWnd_ModInfoScreen::CMWnd_ModInfoScreen()
{
	m_ShouldCreate = true;
	m_bActive = false;
	m_Result = 0;
	m_LastFocus = NULL;
	m_bTimerMode = false;
/*
#ifdef PLATFORM_DOLPHIN	
	m_iButtonDescriptorVCorrection = - 150;
#else
	m_iButtonDescriptorVCorrection = - 180;
#endif	
	*/
	m_bCloseAfterQuestion = false;
}

CMWnd_ModInfoScreen::~CMWnd_ModInfoScreen()
{
}

void CMWnd_ModInfoScreen::vActivate(const CStr& _sText, const CStr& _sAButton, const CStr& _sBButton, int _iId,  CMWnd * _pFocus)
{
	m_Information = _sText;
	m_bActive = true;
	m_Result = 0;
	m_ScreenId = _iId;
	m_bTimerMode = false;
	m_HasRendered = false;

	SetFocus();
	
	//m_AButtonDescriptor = _sAButton;
	//m_BButtonDescriptor = _sBButton;
	
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	//pCon->ExecuteString("cg_playsound(\"menu_select\")");
	
	m_LastFocus = _pFocus;
}

void CMWnd_ModInfoScreen::vActivate(const CStr& _sText, int _nType, CMWnd * _pFocus)
{
	m_Information = _sText;
	m_bActive = true;
	m_Result = 0;
	m_ScreenId = 0;
	m_bTimerMode = false;
	m_HasRendered = false;

	if (_nType == 0)
	{
		SetFocus();

		//m_AButtonDescriptor = "§LMENU_YES";
		//m_BButtonDescriptor = "§LMENU_NO";

		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		//pCon->ExecuteString("cg_playsound(\"menu_select\")");
	}

	m_LastFocus = _pFocus;
}

void CMWnd_ModInfoScreen::vActivate(const CStr& _sText, fp32 fpTimeInfoMessage)
{
	m_Information = _sText;
	m_bActive = true;
	m_Result = 0;
	m_ScreenId = 0;
	m_HasRendered = false;
	
	m_bTimerMode = true;

	//m_AButtonDescriptor = "";
	//m_BButtonDescriptor = "";

	m_StartTime = CMTime::GetCPU();
	m_TotalTime = fpTimeInfoMessage;
}

void CMWnd_ModInfoScreen::vDeactivate()
{
	m_bActive = false;

	if (m_LastFocus)
		m_LastFocus->SetFocus();
}

void CMWnd_ModInfoScreen::OnRefresh()
{
	if (m_bTimerMode)
	{
		CMTime CurrentTime = CMTime::GetCPU();

		if (CurrentTime.Compare(m_StartTime+CMTime::CreateFromSeconds(m_TotalTime)) > 0)
		{
			vDeactivate();
		}
	}
}

void CMWnd_ModInfoScreen::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	if (!m_bActive)
		return;

#ifdef PLATFORM_DOLPHIN
	int x0 = 30;
	int y0 = m_Pos.GetHeight()/2-80;
	int x1 = m_Pos.GetWidth() - 25;
	int y1 = m_Pos.GetHeight()/2+80;
#else
	int x0 = 0;
	int y0 = m_Pos.GetHeight()/2-60;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight()/2+60;
#endif	

	_pRCUtil->SetTexture(0);

	_pRCUtil->SetTextureScale(1,1);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y0));

	_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xe0202020, 0xe0202020);
	_pRCUtil->Rect3D(_Clip, CRct(x0++, y0++, x1--, y1--), 0xc0000000, 0xc0000000, 0xc0000000);

	CRC_Font* pF = GetFont("TEXT");
	
	if (pF) 
	{	
		// Text
#ifdef PLATFORM_DOLPHIN			
		int StyleCenter = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY;
		int StyleLeft = WSTYLE_TEXT_WORDWRAP;

		int Height = y1 - y0;
		/*
		if (m_AButtonDescriptor.Len() || m_BButtonDescriptor.Len())
			Height -= 35;
		if (m_XButtonDescriptor.Len())
			Height -= 35;
			*/

		CMWnd_Text_DrawFormated(
			_pRCUtil, _Clip, pF, 
			"§Z22"+m_Information,
			x0, 
			y0,
			StyleCenter, 
			m_CC.iGetColor(1.0f, 0.5f, 1, false, CM_Mod_ColorController_Blue), 0, 0, x1-x0, Height);
#else
			
		int StyleCenter = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY;
//		int StyleLeft = WSTYLE_TEXT_WORDWRAP;

		CMWnd_Text_DrawFormated(
			_pRCUtil, _Clip, pF, 
			"§Z22"+m_Information,
			0, 
			-25,
			StyleCenter, 
			m_CC.iGetColor(1.0f, 0.5f, 1, false, CM_Mod_ColorController_Blue), 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
#endif			
	}

	m_HasRendered = true;
}

aint CMWnd_ModInfoScreen::OnMessage(const CMWnd_Message* _pMsg)
{
	if (!m_bActive)
	{
		return CMWnd::OnMessage(_pMsg);
	}

	switch(_pMsg->m_Msg)
	{
		
	case WMSG_KEY :
		{
			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			Key.m_Char = _pMsg->m_Param1;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;

			if (m_Status & WSTATUS_FOCUS && GetParent() && Key.IsDown())
			{
				/*
				if ((Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START) && m_AButtonDescriptor.Len() > 0)
				{
					vDeactivate();
					m_Result = 1;
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if (pCon && !m_bNoSound)
						pCon->ExecuteString("cg_playsound(\"menu_ok\")");
				}
				
				else if ((Key.GetKey9() == SKEY_GUI_CANCEL || Key.GetKey9() == SKEY_GUI_BACK) && m_BButtonDescriptor.Len() > 0)
				{
					vDeactivate();
					m_Result = 2;
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if (pCon && !m_bNoSound)
						pCon->ExecuteString("cg_playsound(\"menu_ok\")");
				}
				*/
			}

			return true;
		}
		
	default:
		return CMWnd::OnMessage(_pMsg);
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModEdit
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModEdit, CMWnd_Edit);

CMWnd_ModEdit::CMWnd_ModEdit()
{
	m_ScrollPos = 0;
	m_FocusEdit = false;
	m_SpawnedWindow = NULL;
	m_RemoveSpawnedWindow = false;
}

aint CMWnd_ModEdit::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{

	case WMSG_KEY :
		{
			if (m_FocusEdit || m_SpawnedWindow)
			{
				CScanKey Key;
				Key.m_ScanKey32 = _pMsg->m_Param0;
				Key.m_Char = _pMsg->m_Param1;
				CScanKey OriginalKey;
				OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
				OriginalKey.m_Char = _pMsg->m_Param3;

				if (m_SpawnedWindow && Key.GetKey9() == SKEY_GUI_OK)
				{
//					CMWnd * Wnd = GetChildFocusWnd();
				}
				
				if ((Key.m_ScanKey32 != SKEY_ESC) && (Key.m_ScanKey32 != SKEY_GUI_BACK))
				{	
					// If focused, process key inside textbox
					
					CMWnd* pWFocus = (m_pWndRoot) ? m_pWndRoot->GetFocusWnd() : GetFocusWnd();
					
					if ((pWFocus == this && Key.IsDown()) || m_SpawnedWindow)
						ProcessTextKey(Key);
				}
				else
				{
					if (m_SpawnedWindow)
					{
						m_RemoveSpawnedWindow = true;
					}

				}
				
				return true;
			}
			else
			{
				return CMWnd::OnMessage(_pMsg);
			}
		}

	default:
		return CMWnd::OnMessage(_pMsg);

	}
}

void CMWnd_ModEdit::Execute(const CStr& _Script)
{
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
//ConOutL("(CMWnd_ModButton::Execute) " + _Script);
	pCon->ExecuteString(_Script);
}


int CMWnd_ModEdit::OnPressed(const CMWnd_Message* _pMsg)
{
	m_FocusEdit = true;

	if ((char*)m_Text)
	{
		CRC_Font* pF = GetFont("TEXT");
		
		// Relative mouse coordinates
		
		CPnt MousePos = GetCursorPos(_pMsg);
		
		// Mouse click
		
		int Len = m_Text.Len();

		bool Fixed = false;
		
		for(int i = 0; i < Len; ++i)
		{
			int PosX1 = int(pF->GetWidthOfCL(i,m_Text)-m_ScrollPos);
			int PosX2 = int(pF->GetWidthOfCL(i+1,m_Text)-m_ScrollPos);
			int Size = (PosX2-PosX1)/2;
			
			if (MousePos.x >= (PosX1-Size-1) && MousePos.x <= (PosX2-Size-1))
			{
				m_CursorPos = i;
				Fixed = true;
				break;
			}
		}

		// End if not fixed

		if (!Fixed && MousePos.x > pF->GetWidthOfCL(m_Text.Len(),m_Text))
			m_CursorPos = m_Text.Len();

		// Console

		m_CursorPos = m_Text.Len();

		// Get mapdata
		
		CMWnd * ParentIterator = this;
		CMapData * MapData = NULL;

		while(ParentIterator)
		{
			CMWnd_Message Msg(WMSG_GETMAPDATA, NULL, 0);
			MapData = (CMapData *)ParentIterator->OnMessage(&Msg);

			if (MapData)
				break;

			ParentIterator = ParentIterator->GetParent();
		}

		/*
		
		  Dont use spawn!

		*/

		//return CMWnd_Edit::OnPressed(_pMsg);

		/*
		
		  Console Spawn

		*/

		if (MapData)
		{
			int iRc = MapData->GetResourceIndex_Registry("GUI\\ModWnd");
			
			CRegistry* pRc = MapData->GetResource_Registry(iRc);
			
			if (pRc)
			{
				// Create

				spCMWnd spWnd = CMWnd_Create(pRc, "PADEDIT",this);

				if (!spWnd)
				{
					ConOutL("(CMWnd_ModEdit::Create) Could not create window PADEDIT");
					return CMWnd_Edit::OnPressed(_pMsg);
				}
				else
				{
					ConOutL("Created PADEDIT window...");
				}

				// Position under

				spWnd->SetPosition(CPnt(0,GetHeight()));

				// Super focus

				if (CMWnd * RootWindow = spWnd->GetRootWnd())
					RootWindow->m_SuperFocusWindow = spWnd;

				// Spawned window

				m_SpawnedWindow = spWnd;

				// Focus first button

				if (CMWnd * FirstChild = spWnd->GetFirstChild())
				{
					KillFocus();
					FirstChild->SetFocus();
				}
			}
		}
	}

	int Ret = CMWnd_Edit::OnPressed(_pMsg);

	return Ret;
}

void CMWnd_ModEdit::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
//	if (_Key == "SCRIPT_PRESSED")
//	{
//		m_Script_Pressed = _Value;
//		return;
//	}

	CMWnd_Edit::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_ModEdit::OnRefresh()
{
	if (m_SpawnedWindow && m_RemoveSpawnedWindow)
	{
		m_SpawnedWindow->KillFocus();
		SetFocus();
		m_SpawnedWindow->GetParent()->RemoveChild(m_SpawnedWindow);
		m_SpawnedWindow = NULL;
		m_RemoveSpawnedWindow = false;

		// Super focus

		if (CMWnd * RootWindow = GetRootWnd())
			RootWindow->m_SuperFocusWindow = NULL;
	}
}

void CMWnd_ModEdit::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	CClipRect Clip = _Clip;

	_pRCUtil->SetTexture(0);

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;

	bDown = 0; // Edit box

	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	if (m_Style & WSTYLE_CLIENTEDGE)
	{
		_pRCUtil->Frame(Clip, x0++, y0++, x1--, y1--, 0xc0000000, 0x20000000, true);
		_pRCUtil->Frame(Clip, x0++, y0++, x1--, y1--, 0xff000000, 0xff000000, true);
	}

	_pRCUtil->SetTexture("GROUND_TILE04_07");

	if (bDown)
	{
		_pRCUtil->Frame(Clip, x0++, y0++, x1--, y1--, 0xff101010, 0xff080808);
	}
	else
	{
		_pRCUtil->Frame(Clip, x0++, y0++, x1--, y1--, 0xff202020, 0xff080808);
		_pRCUtil->Rect3D(Clip, CRct(x0++, y0++, x1--, y1--), 0xff404040, 0xff202020, 0xff101010);
	}
	_pRCUtil->SetTexture(0);

	if ((char*)m_Text)
	{
		CRC_Font* pF = GetFont("TEXT");
		if (pF) 
		{
			int Style = m_TextStyle & (WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);

			// Cursor Pos

			int PosOfCursor = int(pF->GetWidthOfCL(m_CursorPos,m_Text));

			// Scroll right

			if (PosOfCursor > ((m_Pos.GetWidth()-10) + m_ScrollPos))
				m_ScrollPos = PosOfCursor - (m_Pos.GetWidth()-10);

			// Scroll left

			if ((PosOfCursor-2) < m_ScrollPos)
				m_ScrollPos = PosOfCursor-40;

			if (m_ScrollPos<0)
				m_ScrollPos = 0;

			// Fit cliprect

			Clip.clip.p0.x +=2;
			Clip.clip.p1.x -=3;

			// Out of focus = no scroll

			CMWnd* pWFocus = (m_pWndRoot) ? m_pWndRoot->GetFocusWnd() : GetFocusWnd();
			
			int DrawnScrollPos = -4;

			if ((pWFocus == this && m_FocusEdit) || m_SpawnedWindow)
			{
				DrawnScrollPos = m_ScrollPos-4;
			}

			// Draw

			CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos, bDown,
				Style, m_TextColorM, m_TextColorH, m_TextColorD, m_Pos.GetWidth(), m_Pos.GetHeight());

			if (m_Status & WSTATUS_MOUSEOVER)
			{
				int Style = m_TextStyle & (WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);
				_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);

				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos+1, bDown,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos-1, bDown,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos+2, bDown,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos-2, bDown,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos+3, bDown,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, Clip, pF, m_Text, -DrawnScrollPos-3, bDown,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
			}

			// Edit Mode if focused

			if ((pWFocus == this && m_FocusEdit) || m_SpawnedWindow)
			{
				// Check cursor; we want no edit on the controlcode
				
				int min = pF->IsControlCode(m_Text, 0);
				
				if (m_CursorPos < min)
				{
					m_ScrollPos = 0;
					m_CursorPos = min;
				}

				// Draw cursor

				CMTime Time = CMTime::GetCPU();
				
				if (Time.GetTimeModulusScaled(10.0f, 6.0f) > 2.0)
				{
					int Width = int(pF->GetWidthOfCL(m_CursorPos,m_Text));

					_pRCUtil->Rect3D(Clip, CRct::From_fp32(
						Width-m_ScrollPos+4, 
						3, 
						Width+2-m_ScrollPos+4, 
						pF->GetHeight(0.0f,m_Text)+1), 0xff808080, 0xff808080, 0xff808080);
				}
			}

			if (pWFocus != this && m_FocusEdit)
			{
				m_FocusEdit = false;
			}
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModButton
// -------------------------------------------------------------------

CMWnd_ModButton::CMWnd_ModButton()
{
	m_NoBackground  = false;
	m_NoOnPaintBackground = false;
	m_NoSounds = false;
	m_OffsetX = 0;
}

void CMWnd_ModButton::Execute(const CStr& _Script)
{
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
//ConOutL("(CMWnd_ModButton::Execute) " + _Script);
	pCon->ExecuteString(_Script);
}

aint CMWnd_ModButton::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_KEY :
		{
			if (m_Padedit_Scankey != "")
			{
				// Send cancel message to edit control

				CScanKey Key;
				Key.m_ScanKey32 = _pMsg->m_Param0;
				CScanKey OriginalKey;
				OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
				OriginalKey.m_Char = _pMsg->m_Param3;
				
				if ((m_Status & WSTATUS_FOCUS) && GetParent() && Key.IsDown())
				{
					if (Key.GetKey9() == SKEY_GUI_CANCEL)
					{
						if (CMWnd * Par1 = GetParent())
						{
							if (CMWnd * Par2 = Par1->GetParent())
							{
								CScanKey _Key;
								
								_Key.m_ScanKey32 = SKEY_ESC;
								_Key.m_Char = 0;
								CMWnd_Message Msg(WMSG_KEY, "", _Key.m_ScanKey32, _Key.m_Char, _Key.m_ScanKey32, _Key.m_Char);
								Par2->OnMessage(&Msg);
								return CMWnd_Button::OnMessage(_pMsg);
							}
						}
					}
				}

				// Send message to edit control

				if (CMWnd * Par1 = GetParent())
				{
					if (CMWnd * Par2 = Par1->GetParent())
					{
						if (Key.GetKey9() != SKEY_CURSOR_LEFT &&
							Key.GetKey9() != SKEY_CURSOR_RIGHT &&
							Key.GetKey9() != SKEY_GUI_LEFT &&
							Key.GetKey9() != SKEY_GUI_RIGHT
							
							)
						{
							Par2->OnMessage(_pMsg);
						}		
					}
				}
			}
		}
		break;
	}
	
	return CMWnd_Button::OnMessage(_pMsg);
}

void CMWnd_ModButton::OnSetFocus()
{
	if (!m_NoSounds)
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		//pCon->ExecuteString("cg_playsound(\"menu_select\")");
	}
}

int CMWnd_ModButton::OnPressed(const CMWnd_Message* _pMsg)
{
	CScanKey OriginalKey;
	OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
	OriginalKey.m_Char = _pMsg->m_Param3;
	if (OriginalKey.GetKey9() == SKEY_MOUSE1)
	{
		if (GetFrontEndInfo() && GetFrontEndInfo()->m_pFrontEnd)
		{
			CMWnd *pCursor = GetFrontEndInfo()-> m_pFrontEnd->GetCursorWindow(false);
			if (!pCursor || ((((pCursor->m_Group.Len()||this->m_Group.Len())&& pCursor->m_Group != this->m_Group)) || (!(pCursor->m_Group.Len()||this->m_Group.Len()) && pCursor != this)))
				return CMWnd_Button::OnPressed(_pMsg);
		}
	}

	if (m_Script_Pressed != "")
	{
		Execute(m_Script_Pressed);

		if (!m_NoSounds)
		{
			SoundEvent(WSND_SELECT);
			//MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			//pCon->ExecuteString("cg_playsound(\"menu_ok\")");
		}
		return true;
	}

	int Ret = CMWnd_Button::OnPressed(_pMsg);

	return Ret;
}

void CMWnd_ModButton::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if (_Key == "SCRIPT_PRESSED")
	{
		m_Script_Pressed = _Value;
		return;
	}

	if (_Key == "PADEDIT_SCANKEY")
	{
		m_Padedit_Scankey = _Value;
		return;
	}
	if (_Key == "NO_PAINT_BACKGROUND")
	{
		m_NoOnPaintBackground = true;
		return;
	}
	
	CMWnd_Button::EvaluateKey(_pParam, _Key, _Value);
}

// Paint focus

void CMWnd_ModButton::OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	return;
	CRC_Font* pF = GetFont("TEXT");
	
	if (pF)	
	{
//		if (!m_NoBackground)
//		{
//			int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;
			
			_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			
			CMTime Time = CMTime::GetCPU();
			fp32 coll = M_Sin(Time.GetTimeModulusScaled(4.0f, _PI2))*32.0f+128.0f;
			int col2 = CPixel32::From_fp32(coll, coll, coll, 128);
			
			_pRCUtil->SetTexture(0);
			_pRCUtil->Rect3D(_Clip, CRct(0, 0, GetWidth(), GetHeight()), col2, col2, col2);
			
			if (GetRootWnd())
				_pRCUtil->SetTextureOrigo(GetRootWnd()->m_AbsPos,CPnt(0,0));
//		}
	}
}

// Paint

void CMWnd_ModButton::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	_pRCUtil->SetTexture(0);

	if (GetRootWnd())					
		_pRCUtil->SetTextureOrigo(GetRootWnd()->m_AbsPos,CPnt(0,0));

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;


	if ((m_Status & WSTATUS_FOCUS) && !(m_Style & WSTYLE_HIDDENFOCUS))
	{
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		
		CMTime Time = CMTime::GetCPU();
		fp32 coll = M_Sin(Time.GetTimeModulusScaled(4.0f, _PI2))*64.0f+128.0f;
		int col2 = CPixel32::From_fp32(coll, coll, coll, 128);
		
		_pRCUtil->SetTexture(0);
		_pRCUtil->Rect3D(_Clip, CRct(0, 0, GetWidth(), GetHeight()), col2, col2, col2);
	}


	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	if (m_Style & WSTYLE_CLIENTEDGE)
	{
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xc0000000, 0x20000000, true);
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xff000000, 0xff000000, true);
	}

	if (!m_NoBackground && !m_NoOnPaintBackground)
	{
//		_pRCUtil->SetTexture("GROUND_TILE04_07");
		_pRCUtil->SetTexture(0);
		if (bDown)
		{
			_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xff101010, 0xff080808);
			_pRCUtil->Rect3D(_Clip, CRct(x0++, y0++, x1--, y1--), 0xff202020, 0xff202020, 0xff202020);
		}
		else
		{
			_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xff202020, 0xff080808);
			_pRCUtil->Rect3D(_Clip, CRct(x0++, y0++, x1--, y1--), 0xff404040, 0xff202020, 0xff101010);
		}
	}

	_pRCUtil->SetTexture(0);

	if ((char*)m_Text)
	{
		CRC_Font* pF = GetFont("TEXT");
		if (pF) 
		{
			int Style = m_TextStyle & (WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);

			CStr sDrawText;
			sDrawText = m_Text;

			for(int y = -1; y <= 1; y++)
				for(int x = -1; x <= 1; x++)
				{
					CMWnd_Text_DrawFormated(
						_pRCUtil, 
						_Clip, 
						pF,
						sDrawText,
						bDown+m_OffsetX+x+m_TextPos.x,
						bDown+y+m_TextPos.y,
						Style, 
						CPixel32(0,0,0,255), 
						m_TextColorH, 
						m_TextColorD, 
						m_Pos.GetWidth(), 
						m_Pos.GetHeight());
				}

			CMWnd_Text_DrawFormated(
				_pRCUtil, 
				_Clip, 
				pF, 
				sDrawText, 
				bDown+m_OffsetX+m_TextPos.x, 
				bDown+m_TextPos.y,
				Style, 
				m_CC.iGetColor(1.0f, 0.5f, m_CC.m_iSlot, false, m_CC.m_iColorId), 
				m_TextColorH, 
				m_TextColorD, 
				m_Pos.GetWidth(), 
				m_Pos.GetHeight());

			if (m_Status & WSTATUS_MOUSEOVER)
			{
				int Style = m_TextStyle & (WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);
				_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown+1+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown-1+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown+2+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown-2+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown+3+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown-3+m_OffsetX+m_TextPos.x, bDown+m_TextPos.y,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
			}
		}
	}
}

// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModText, CMWnd_Text);

void CMWnd_ModText::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	int ColH = m_Col_Highlight;
	int ColM = m_Col_Background;
	int ColD = m_Col_Shadow;
//	int ColDD = m_Col_DarkShadow;

	CRC_Font* pF = GetFont("TEXT");
	if (pF) 
	{
		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 0, 0,
			m_Style, ColM, ColH, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
	}
}

 /*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Phobos: Added Texture implementation
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModTexture, CMWnd);

CMWnd_ModTexture::CMWnd_ModTexture()
{
	m_MovieController.m_iRenderMode = 0;

	// Set some default values (should their keys not be given)
	m_Color = 0xffffffff;
	m_Offset.k[0] = 0;
	m_Offset.k[1] = 0;
	m_PixelAspect = 1.0;
	m_TextureMode = CMWnd_ModTextureMode_Texture;
	m_Repeat = 0;
}

void CMWnd_ModTexture::LinkWindow()
{
	if (m_TextureMode == CMWnd_ModTextureMode_Link)
	{
		// Ok, close video and "open" next window
		m_MovieController.vCloseVideo(m_TextureName);
		m_MovieController.bConExecute(m_LinkWindow);
	}
}

void CMWnd_ModTexture::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if (_Key == "TEXTURE_NAME")
	{
		m_TextureName = _Value;
	}
	else if (_Key == "OFFSET")
	{
		m_Offset.k[0] = Value.GetStrSep(",").Val_int();
		m_Offset.k[1] = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "PIXELASPECT")
	{
		m_PixelAspect = Value.GetStrSep(",").Val_fp64();
	}	
	else if (_Key == "COLOR")
	{
		m_Color = Value.Val_int();
	}	
	else if (_Key == "TEXTURE_MODE")
	{
		// Make upper case to more easily find matches in sub strings
		CStr Upper = Value.UpperCase();

		if (Upper.GetNumMatches("TEXTURE"))
		{
			// Ordinary texture (no video)
			m_TextureMode = CMWnd_ModTextureMode_Texture;
			m_Repeat = 0;
		}
		else
		if (Upper.GetNumMatches("REPEAT_FOREVER"))
		{
			// The video is repeated forever
			m_TextureMode = CMWnd_ModTextureMode_Repeat_Forever;
			m_Repeat = 0;
		}
		else
		if (Upper.GetNumMatches("REPEAT "))
		{
			// The video is repeated N number of times
			m_TextureMode = CMWnd_ModTextureMode_Repeat_N;
			Value.GetStrSep(" ");
			m_Repeat = Value.GetStrSep(" ").Val_int();
		}
		else
		if (Upper.GetNumMatches("LINK"))
		{
			// When the movie has finished playing, we close this
			// window and open up another
			m_TextureMode = CMWnd_ModTextureMode_Link;
			m_Repeat = 0;
			Value.GetStrSep(" ");
			m_LinkWindow = Value;
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, Value);
}

void CMWnd_ModTexture::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	if (m_TextureMode == CMWnd_ModTextureMode_Texture)
	{
		PaintTexture(_pRCUtil, _Clip, _Text);
	}
	else
	{
		PaintVideo(_pRCUtil, _Clip, _Text);
	}
}

void CMWnd_ModTexture::PaintVideo(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	// Most of this code should be moved to CMWnd_CenterImage
	// The code here should only handle rendering of videos on exactly the specified coordinated
	// and not do any type of rescaling based on pixel-aspect or viewport

	int nCurrentFrame, nNumFrames;
	int iStatus = m_MovieController.iGetStatusOfBink(m_TextureName, nCurrentFrame, nNumFrames );
	
	switch (m_TextureMode)
	{
		case CMWnd_ModTextureMode_Repeat_Forever:
		{
			// The movie should be repeated forever, so when it has finished playing
			// just restart
			if (iStatus == 1)
			{
				m_MovieController.vFastRewindBINK(m_TextureName, false, false, false);	
			}
			break;
		}
		case CMWnd_ModTextureMode_Repeat_N:
		{
			// If the movie has finished playing, and it should be repeated, make it so
			if (iStatus == 1 && m_Repeat > 0)
			{
				m_MovieController.vFastRewindBINK(m_TextureName, false, false, false);	
				m_Repeat--;
			}
			break;
		}
		case CMWnd_ModTextureMode_Link:
		{
			// If the movie has finished playing, link to the next window
			if (iStatus == 1)
			{
				LinkWindow();
				return;
			}
			break;
		}
		default:
			break;
	};


	if (m_MovieController.m_iRenderMode == 0)
	{
		m_MovieController.vFastRewindBINK(m_TextureName, false, false, false);
		m_MovieController.m_iRenderMode = 1;
	}

	CRenderContext* pRC = _pRCUtil->GetRC();

	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();
	
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));
	
	_pRCUtil->GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	_pRCUtil->GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	CClipRect Clip(0, 0, 640, 480);

	_pRCUtil->SetTexture(NULL);
	_pRCUtil->Rect(Clip, CRct(0, 0, 640, 480), 0x0);

	CStr YFrameName = m_TextureName + "_y";
	CStr UVFrameName = m_TextureName + "_uv";
	int YFrameID = _pRCUtil->GetTC()->GetTextureID(YFrameName.Str());
	int UVFrameID = _pRCUtil->GetTC()->GetTextureID(UVFrameName.Str());

	bool bRender = false;
	// If YUV textures exist and renderer is shadercapable then let the GPU do all the work
	if( YFrameID && UVFrameID && (pRC->Caps_Flags() & (CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 | CRC_CAPS_FLAGS_FRAGMENTPROGRAM14 | CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10)) )
	{
		bRender = true;
		_pRCUtil->SetTexture(YFrameID);
		CRC_Attributes Attrib;
		Attrib = *_pRCUtil->GetAttrib();
		if( pRC->Caps_Flags() & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 )
		{
			CRC_ExtAttributes_FragmentProgram20* pShaderAttrib = (CRC_ExtAttributes_FragmentProgram20*)pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			pShaderAttrib->Clear();
			Attrib.m_pExtAttrib	= pShaderAttrib;

			pShaderAttrib->SetProgram( "CMWnd_ModTexture_PaintVideo_YUV2RGB", 0 );
		}
		else if( pRC->Caps_Flags() & CRC_CAPS_FLAGS_FRAGMENTPROGRAM14 )
		{
			CRC_ExtAttributes_FragmentProgram14* pShaderAttrib = (CRC_ExtAttributes_FragmentProgram14*)pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram14));
			static CVec4Dfp32 aYUV2RGBParams[3] = {
				CVec4Dfp32( 0.0f, 0.582f, 1.164f * 16.0f / 255.0f, 0.0f ),
				CVec4Dfp32( 0.0f, 0.391f, 0.99999999999999f, 0.0f ), 
				CVec4Dfp32( 0.768f, 0.813f, 0.0f, 0.0f )
			};
			pShaderAttrib->Clear();
			Attrib.m_pExtAttrib	= pShaderAttrib;
			Attrib.Attrib_TexCoordSet( 1, 0 );

			pShaderAttrib->SetProgram( "CMWnd_ModTexture_PaintVideo_YUV2RGB", 0 );
			pShaderAttrib->SetParameters( aYUV2RGBParams, 3 );
		}
#ifdef SUPPORT_REGISTERCOMBINERS
		else if( pRC->Caps_Flags() & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10 )
		{
			CRC_ExtAttributes_NV10* pShaderAttrib = (CRC_ExtAttributes_NV10*)pVBM->Alloc(sizeof(CRC_ExtAttributes_NV10));
			pShaderAttrib->Clear();
			Attrib.m_pExtAttrib	= pShaderAttrib;
			Attrib.Attrib_TexCoordSet( 1, 0 );

			int nComb = 0;

			// Perform everything in colorspace / 2
			pShaderAttrib->Clear(nComb);
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_TEXTURE0, NV_INPUT_ZERO | NV_MAPPING_INVERT, NV_INPUT_TEXTURE1 | NV_MAPPING_HALFBIASNORMAL, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false );
			pShaderAttrib->SetInputAlpha( nComb, NV_INPUT_TEXTURE1 | NV_COMP_ALPHA | NV_MAPPING_HALFBIASNORMAL, NV_INPUT_ZERO | NV_MAPPING_INVERT  );
			pShaderAttrib->SetOutputAlpha( nComb, NV_OUTPUT_SPARE1, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false );
			nComb++;
			// r0 = t0_half, r1 = t1_bias_half

			pShaderAttrib->Clear( nComb );
			pShaderAttrib->SetConst0( nComb, 0x94949494 );
			pShaderAttrib->SetConst1( nComb, 0x09090909 );
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_COLOR0, NV_INPUT_COLOR1 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
			nComb++;
			// mad r0.rgb, r0, c0, c1_neg;

			pShaderAttrib->Clear( nComb );
			pShaderAttrib->SetConst0( nComb, CPixel32( 0, 0x31, 0, 0 ) );
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_COLOR0 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
			nComb++;
			// mad r0.rgb c0_neg.rgb, r1.g, r0;

			pShaderAttrib->Clear( nComb );
			pShaderAttrib->SetConst0( nComb, CPixel32( 0, 0, 0xff, 0 ) );
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_COLOR0, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
			nComb++;
			// mad r0.rgb, one.rgb, r1.b, r0;

			pShaderAttrib->Clear( nComb );
			pShaderAttrib->SetConst0( nComb, CPixel32( 0xc3, 0, 0, 0 ) );
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_COLOR0, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY | NV_COMP_ALPHA, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_ONE, false, false, false );
			nComb++;
			// mad r0.rgb, c0.rgb, r1.a, r0;

			pShaderAttrib->Clear( nComb );
			pShaderAttrib->SetConst0( nComb, CPixel32( 0, 0x67, 0, 0 ) );
			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_COLOR0 | NV_MAPPING_SIGNEDNEGATE, NV_INPUT_SPARE1 | NV_MAPPING_SIGNEDIDENTITY | NV_COMP_ALPHA, NV_INPUT_SPARE0 | NV_MAPPING_SIGNEDIDENTITY, NV_INPUT_ZERO | NV_MAPPING_INVERT );
			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_TWO, false, false, false );
			nComb++;
			// mad r0_x2.rgb, c0_neg.rgb, r1.a, r0;

//			pShaderAttrib->Clear( nComb );
//			pShaderAttrib->SetInputRGB( nComb, NV_INPUT_SPARE0, NV_INPUT_ZERO | NV_MAPPING_INVERT );
//			pShaderAttrib->SetOutputRGB( nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_TWO, false, false, false );
//			nComb++;
//			// mov r0, r0_x2;

			pShaderAttrib->SetNumCombiners(nComb, true);
			pShaderAttrib->ClearFinal();
//			pShaderAttrib->SetFinal( NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO );
			pShaderAttrib->SetFinal( NV_INPUT_SPARE0, NV_INPUT_PRIMARY, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO );
		}
#endif
		Attrib.Attrib_TextureID(0, YFrameID );
		Attrib.Attrib_TextureID(1, UVFrameID );
		_pRCUtil->SetAttrib(&Attrib);
	}
	else
	{
		// Do nasty YUV->RGB conversion
		if(_pRCUtil->SetTexture(m_TextureName))
			bRender = true;
	}

	if( bRender )
	{
		// Find size from movie controller (_pRCUtil gives wrong size)
		CPnt SourceSize;
		m_MovieController.vGetMovieSize(m_TextureName, SourceSize.x, SourceSize.y);

		_pRCUtil->AspectRect(Clip, GetPosition(), SourceSize, m_PixelAspect);

/*		int Width = Position.p1.x - Position.p0.x;
		int Height = Position.p1.y - Position.p0.y;
		fp32 OffsetX = 0;
		fp32 OffsetY = 0;

		fp32 Scale = m_PixelAspect / (_pRCUtil->GetRC()->GetDC()->GetScreenAspect() * _pRCUtil->GetRC()->GetDC()->GetPixelAspect());
		if (Scale < 1.0)
		{
			OffsetX = (Width - ((Scale) * Width))/2;
			_pRCUtil->SetTextureScale((SourceSize.x / (fp32) Width) * (1.0f / Scale), (SourceSize.y / (fp32) Height));
		}
		else
		{
			OffsetY = (Height - ((1.0f / Scale) * Height))/2;
			_pRCUtil->SetTextureScale((SourceSize.x / (fp32) Width), (SourceSize.y / (fp32) Height) * (Scale));
		}

		_pRCUtil->SetTextureOrigo(Clip, CPnt(Position.p0.x + m_Offset.k[0] + OffsetX, Position.p0.y + m_Offset.k[1] + OffsetY));

		_pRCUtil->Rect(Clip, CRct(Position.p0.x + m_Offset.k[0] + OffsetX, Position.p0.y + m_Offset.k[1] + OffsetY, Position.p1.x + m_Offset.k[0] - OffsetX, Position.p1.y + m_Offset.k[1] - OffsetY),  0x80808080);
		_pRCUtil->SetAttrib(NULL);*/
	}


	//m_MovieController.GetMovieTime();
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetOptions() && pSys->GetOptions()->GetValuei("GAME_SUBTITLE_CUTSCENE", 0))
	{
		do
		{
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			int iRc = pGame->m_spFrontEnd->m_spMapData->GetResourceIndex_Registry("Registry\\FMVSubtitles");
			CRegistry *pReg = pGame->m_spFrontEnd->m_spMapData->GetResource_Registry(iRc);

			if(!pReg)
				break;

			CRegistry *pSubs = pReg->FindChild(m_TextureName.RightFrom(7)); // strlen("*VIDEO_") = 5

			if(!pSubs)
				break;

			CStr Subtitle = "";
			fp32 MovieTime = m_MovieController.GetMovieTime(m_TextureName);
//			CRegistry *pPrevChild = NULL;
			//fp32 SubTime;
			for(int32 i = 0; i < pSubs->GetNumChildren(); i++)
			{
				CRegistry *pChild = pSubs->GetChild(i);
				if(!pChild)
					continue;

				CStr Sub = pChild->GetThisValue();
				fp32 Time = Sub.GetStrSep(",").Val_fp64();

				if(Time > MovieTime)
					break;

				Subtitle = Sub;
			}

			CRC_Font *pFont = GetFont("TEXT");
			_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			
			int32 SubtitleSize = DEFAULT_SIZE;
			int32 SubtitleColor = 0x00808080;

			CStr Text = CStrF("§Z%i", SubtitleSize) + Subtitle;
			//if(Subtitle != "" && Subtitle.Ansi().Str()[0] == '§')
			//	Subtitle += CStrF("§p0%i§pq", _Param);
			
			int y = 376;

			CClipRect Clip(0, 0, 640, 480);
			int Alpha = 0xff<<24;
			int Col = SubtitleColor | Alpha;
			int ShadowCol = Alpha;

			int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER;

			int Border = 120;
			int XStart = Border / 2;
			_pRCUtil->Text_DrawFormatted(Clip, pFont, Text, XStart, y, Style, ShadowCol, ShadowCol, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), true);
			_pRCUtil->Text_DrawFormatted(Clip, pFont, Text, XStart, y, Style, Col, Col, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), false);
		}
		while(0);
	}
}

void CMWnd_ModTexture::PaintTexture(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();

	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));

	_pRCUtil->GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	_pRCUtil->GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	CClipRect Clip(0, 0, 640, 480);

	if(_pRCUtil->SetTexture(m_TextureName))
	{
		CRct Position = GetPosition();
		int Width = Position.p1.x - Position.p0.x;
		int Height = Position.p1.y - Position.p0.y;

		_pRCUtil->SetTextureScale(_pRCUtil->GetTextureWidth() / (fp32) Width, _pRCUtil->GetTextureHeight() / (fp32) Height);
		_pRCUtil->SetTextureOrigo(Clip, CPnt::From_fp32(Position.p0.x + m_Offset.k[0], Position.p0.y + m_Offset.k[1]));
		
		_pRCUtil->Rect(Clip, CRct::From_fp32(Position.p0.x + m_Offset.k[0], Position.p0.y + m_Offset.k[1], Position.p1.x + m_Offset.k[0], Position.p1.y + m_Offset.k[1]),  0xff7f7f7f);
	}
}

aint CMWnd_ModTexture::OnMessage(const CMWnd_Message* _pMsg)
{
	// Only process "Skip" messages when in link mode
	if (m_TextureMode == CMWnd_ModTextureMode_Link)
	{
		switch(_pMsg->m_Msg)
		{
		case WMSG_KEY :
			{
				CScanKey Key;
				Key.m_ScanKey32 = _pMsg->m_Param0;
				CScanKey OriginalKey;
				OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
				OriginalKey.m_Char = _pMsg->m_Param3;
				
				if (Key.IsDown())
				{
					// Key

					if (Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START)
					{
						// Somebody wants to skip the video, so let them, link
						// to the next window
						LinkWindow();
					}
				}
			}
		}
	}

	return CMWnd::OnMessage(_pMsg);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| 
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModOptionBox, CMWnd_Text);


CMWnd_ModOptionBox::CMWnd_ModOptionBox()
{
	m_iOption = 0;
}

void CMWnd_ModOptionBox::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if (_Key == "OPTIONS")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			m_lOptions.Add(s);
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

aint CMWnd_ModOptionBox::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_SETSTATE:
		{
			m_iOption = _pMsg->m_Param0;
			return 1;
		}
	case WMSG_GETSTATE:
		{
			return m_iOption;
		}
	default :
		return CMWnd_Text::OnMessage(_pMsg);
	}
}

void CMWnd_ModOptionBox::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	int ColH = m_Col_Highlight;
	int ColM = m_Col_Background;
	int ColD = m_Col_Shadow;
//	int ColDD = m_Col_DarkShadow;

	_pRCUtil->Rect(_Clip, _Text, ColD);

	if (m_lOptions.ValidPos(m_iOption))
	{
		CRC_Font* pF = GetFont("TEXT");
		if (pF) 
		{
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_lOptions[m_iOption], 3, 0,
				m_Style, ColM, ColH, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
		}
	}
}

int CMWnd_ModOptionBox::OnPressed(const CMWnd_Message* _pMsg)
{
	if (m_lOptions.Len())
		m_iOption = (m_iOption + 1) % m_lOptions.Len();
	return m_iOption;
}

// -------------------------------------------------------------------
/*
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModInvObject, CMWnd_Viewport);


CMTime CMWnd_ModInvObject::ms_Time;

void CMWnd_ModInvObject::Clear()
{
	m_Viewport.SetFOV(20);
	m_Viewport.SetFOVAspect(1.0f);

	FillChar(m_iModels, sizeof(m_iModels), 0);
	m_Flags = 0;
	m_ItemNr = 0;
	m_ModelPos.Unit();

	m_Camera.Unit();
	m_Camera.RotX_x_M(0.25f);
	m_Camera.RotY_x_M(-0.25f);
	m_Camera.RotZ_x_M(0.5f);
	m_Camera.k[3][0] = 60.0f;
	m_Camera.k[3][2] = 32.0f;
}

CMWnd_ModInvObject::CMWnd_ModInvObject()
{
	Clear();
}

CMWnd_ModInvObject::CMWnd_ModInvObject(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID) : 
	CMWnd_Viewport(_pWndParent, _Param, _ID)
{
	Clear();
}

void CMWnd_ModInvObject::SetModel(spCMapData _spMapData, int _iModel, int _ModelNr)
{
	if (_ModelNr < 0 || _ModelNr >= MODINVOBJ_MAXMODELS) Error("SetModel", "Invalid model-nr.");
	m_spMapData = _spMapData;
	m_iModels[_ModelNr] = _iModel;

}

aint CMWnd_ModInvObject::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_CURSOR :
		{
			if (_pMsg->IsPressed() && _pMsg->IsMouse1())
			{
				if (m_pWndParent)
				{
					CMWnd_Message Msg(WMSG_COMMAND, m_ID, 1, _pMsg->GetCursorBtn());
					return m_pWndParent->OnMessage(&Msg);
				}
				else
					return 0;
			}

			return false;
		}
	default :
		return CMWnd_Viewport::OnMessage(_pMsg);
	}
}

void CMWnd_ModInvObject::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();
	
	CRct Client = _Client;
	CClipRect Clip = _Clip;

	CRenderContext* pRC = _pRCUtil->GetRC();


	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pRCUtil->GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	_pRCUtil->GetAttrib()->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	_pRCUtil->Rect(Clip, CRct(x0,y0,x1,y1), 0x00000000);

	if (!m_spMapData) return;
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	if (!pVBM) return;

	CBox3Dfp32 Bound;
	Bound.m_Min = 1000000.0f;
	Bound.m_Max = -1000000.0f;
	int bModelExist = false;
	{
		for(int i = 0; i < MODINVOBJ_MAXMODELS; i++)
		{
			int iModel = m_iModels[i];
			CXR_Model* pModel = m_spMapData->GetResource_Model(iModel);

			if (pModel)
			{
				bModelExist = true;
				CBox3Dfp32 Box;
				pModel->GetBound_Box(Box);
				Bound.Expand(Box);
			}
		}
	}

	if (!bModelExist) return;

	CVec3Dfp32 Size;
	Bound.m_Max.Sub(Bound.m_Min, Size);
	fp32 SizeX = Length2(Size[0], Size[1]);
	fp32 SizeY = Size[2];

	fp32 Fov = m_Viewport.GetFOV() / 180 * _PI;
	fp32 Dist = SizeX * M_Cos(Fov) / M_Sin(Fov);
	fp32 DistY = SizeY * M_Cos(Fov) / M_Sin(Fov);
	fp32 w = m_Pos.GetWidth();
	fp32 h = m_Pos.GetHeight();

	if (w > h)
	{
		if (Dist / w > DistY / h)
			DistY = Dist / w * h;
	}
	else
	{
		if (Dist / w < DistY / h)
			Dist = DistY / h * w;
	}

//	fp32 Dist = Max(SizeX / m_Pos.GetWidth(), SizeY / m_Pos.GetHeight());

	m_Camera.k[3][0] = Max(Dist, DistY) * 1.0f;
	m_Camera.k[3][2] = 0;


	CMat4Dfp32 Mat;
	Mat.Unit();
	if (m_Flags & 2)
	{
	
		CVec3Dfp32 v(0, 0, ms_Time.GetTimeModulusScaled(0.25f, 1.0f));

		v.CreateMatrixFromAngles(0, Mat);
	}
	else
	{
	}


	CVec3Dfp32 Center;
	Bound.GetCenter(Center);
	Center.MultiplyMatrix3x3(Mat);
//	CMat4Dfp32 Mat(m_ModelPos);
	CVec3Dfp32::GetMatrixRow(Mat, 3) -= Center;

	CXR_AnimState Anim(ms_Time, CMTime(), 0, 0, 0, 0);
	Anim.m_AnimTime1 = CMTime::CreateFromSeconds(10000.0f);

	CVec2Dfp32 Scale = _pRCUtil->GetCoordinateScale();
	Client += Clip.ofs;

	Client.p0.x *= Scale[0];
	Client.p0.y *= Scale[1];
	Client.p1.x *= Scale[0];
	Client.p1.y *= Scale[1];
	Clip.clip.p0.x *= Scale[0];
	Clip.clip.p0.y *= Scale[1];
	Clip.clip.p1.x *= Scale[0];
	Clip.clip.p1.y *= Scale[1];
	Clip.ofs.x *= Scale[0];
	Clip.ofs.y *= Scale[1];
	m_Viewport.SetView(Clip, Client);

	pVBM->ScopeBegin(true);
	pVBM->Viewport_Push(&m_Viewport);
	CMat4Dfp32 VMat;
	m_Camera.InverseOrthogonal(VMat);

	// Get occlusion mask
	{
		int OcclusionMask = 0;
		for(int i = 0; i < MODINVOBJ_MAXMODELS; i++)
		{
			CXR_Model* pModel = m_spMapData->GetResource_Model(m_iModels[i]);
			if (pModel)
				OcclusionMask |= pModel->GetParam(CTM_PARAM_OCCLUSIONMASK);
		}

		Anim.m_SurfaceOcclusionMask = OcclusionMask;
	}

	for(int i = 0; i < MODINVOBJ_MAXMODELS; i++)
	{
		CXR_Model* pModel = m_spMapData->GetResource_Model(m_iModels[i]);
		if (!pModel) continue;

//		pModel->SetParam(CTM_PARAM_RENDERFLAGS, CTM_RFLAGS_NOLIGHTING | pModel->GetParam(CTM_PARAM_RENDERFLAGS));

		if (m_Flags & 1)
		{
			pModel->SetParam(CTM_PARAM_RENDERFLAGS, CTM_RFLAGS_WIRE | pModel->GetParam(CTM_PARAM_RENDERFLAGS));
			pModel->OnRender(NULL, _pRCUtil->GetRC(), pVBM, NULL, m_spWLS, &Anim, Mat, VMat, 0);
			pModel->SetParam(CTM_PARAM_RENDERFLAGS, pModel->GetParam(CTM_PARAM_RENDERFLAGS) & ~CTM_RFLAGS_WIRE);
		}
		else
			pModel->OnRender(NULL, _pRCUtil->GetRC(), pVBM, NULL, m_spWLS, &Anim, Mat, VMat, 0);

//		if (m_RenderMode & 2)
//		{
//			pModel->SetParam(CTM_PARAM_RENDERFLAGS, CTM_RFLAGS_WIRE | pModel->GetParam(CTM_PARAM_RENDERFLAGS));
//			pModel->Render(NULL, _pRCUtil->GetRC(), pVBM, NULL, NULL, &Anim, Mat, VMat, 0);
//			pModel->SetParam(CTM_PARAM_RENDERFLAGS, pModel->GetParam(CTM_PARAM_RENDERFLAGS) & ~CTM_RFLAGS_WIRE);
//		}

//		pModel->SetParam(CTM_PARAM_RENDERFLAGS, pModel->GetParam(CTM_PARAM_RENDERFLAGS) & ~CTM_RFLAGS_NOLIGHTING);
	}
	pVBM->Viewport_Pop();
	pVBM->ScopeEnd();
}
*/
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModInvSquare, CMWnd);


void CMWnd_ModInvSquare::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();
	_pRCUtil->SetTexture("GROUND_TILE08");
	_pRCUtil->Rect3D(_Clip, CRct(x0,y0,x1,y1), 0xff1f1f1f, 0xff3f3f3f, 0xff7f7f7f);
	_pRCUtil->SetTexture(0);
}



MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModFrame, CMWnd);


void CMWnd_ModFrame::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(0,0));
//	_pRCUtil->SetTexture("GUI_WINBG");
//	_pRCUtil->SetTexture("GROUND_TILE04_07");
	_pRCUtil->SetTexture("GROUND_TILE08");
	

	int fw = 64;	// Frame width
//	int cw = 32;	// Corner width

	int bgb = fw / 2;	// Background border
	_pRCUtil->Rect(_Clip, CRct(x0+bgb,y0+bgb,x1-bgb,y1-bgb), 0xff5f5f5f);

	_pRCUtil->SetTextureScale(1,1);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y0));
	_pRCUtil->SetTexture("GUI_FRAMEU");
	_pRCUtil->Rect(_Clip, CRct(x0,y0,x1,y0+fw), 0xff7f7f7f);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y1-fw));
	_pRCUtil->SetTexture("GUI_FRAMEU");
	_pRCUtil->Rect(_Clip, CRct(x0,y1-fw,x1,y1), 0xff7f7f7f);

	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y0+bgb));
	_pRCUtil->SetTexture("GUI_FRAMEL");
	_pRCUtil->Rect(_Clip, CRct(x0,y0+bgb,x0+fw,y1-bgb), 0xff7f7f7f);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x1-fw,y0+bgb));
	_pRCUtil->SetTexture("GUI_FRAMEL");
	_pRCUtil->Rect(_Clip, CRct(x1-fw,y0+bgb,x1,y1-bgb), 0xff7f7f7f);

	_pRCUtil->SetTextureOrigo(_Clip, CPnt(0, 0));
	_pRCUtil->SetTextureScale(1,1);

/*	_pRCUtil->SetTexture("GUI_WINCORNER_UL");
	_pRCUtil->Rect(_Clip, CRct(x0,y0,x0+cw,y0+cw), 0xff7f7f7f);
	_pRCUtil->SetTexture("GUI_WINCORNER_UR");
	_pRCUtil->Rect(_Clip, CRct(x1,y0,x1-cw,y0+cw), 0xff7f7f7f);

	_pRCUtil->SetTexture("GUI_WINCORNER_UL");
	_pRCUtil->Rect(_Clip, CRct(x0,y1-cw,x0+cw,y1), 0xff7f7f7f);
	_pRCUtil->SetTexture("GUI_WINCORNER_UR");
	_pRCUtil->Rect(_Clip, CRct(x1,y1-cw,x1-cw,y1), 0xff7f7f7f);*/
}

// -------------------------------------------------------------------


MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModChainFrame, CMWnd);


void CMWnd_ModChainFrame::DrawTile(const CStr& TileName, const CPnt& Point, const CPnt& Size, CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	_pRCUtil->SetSurface(TileName,CMTime());
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(Point.x, Point.y));
	_pRCUtil->Rect(_Clip, CRct(Point.x, Point.y, Point.x+Size.x*32, Point.y+Size.y*32), 0xff7f7f7f);
}

void CMWnd_ModChainFrame::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	return;

	// Variable init
	
	CClipRect Clip = _Clip;

	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	int TileSize = 32;

	int PixelSizexX = (x1/TileSize)*TileSize;
	int PixelSizexY = (y1/TileSize)*TileSize;

	Clip.clip.p0.x = 0;
	Clip.clip.p0.y = 0;
	Clip.clip.p1.x = 640;
	Clip.clip.p1.y = 480;

	// Back plate

	_pRCUtil->SetTexture("GROUND_TILE04_07");

	_pRCUtil->SetTextureScale(1,1);
	_pRCUtil->SetTextureOrigo(Clip, CPnt(x0,y0));

	int in = 4;

	_pRCUtil->Rect3D(Clip, CRct(x0+in, y0+in-1, PixelSizexX+19, PixelSizexY+19), 0xb0202020, 0xb0202020, 0xb0202020);

	// Right corner

	int basex = -12;
	int basey = -16;

	// Horizontal top tiles

	int xtiles = x1/TileSize-2;

	int xx = basex+21+TileSize;

	if (m_Title.Len())
	{
		int middletile = xtiles/2;
		
		if (!(xtiles%2))
		{
			for(int i=0; i < xtiles; ++i)
			{
				if (!(i >= (middletile-m_TopTitleTiles) && i <= middletile+m_TopTitleTiles-1))
					DrawTile("GUI_GenW_03_01",CPnt(xx,basey),CPnt(1,1),_pRCUtil,Clip,_Client);
				
				xx += TileSize;
			}
		}
		else
		{
			for(int i=0; i < xtiles; ++i)
			{
				if (!(i >= middletile-m_TopTitleTiles && i <= middletile+m_TopTitleTiles))
					DrawTile("GUI_GenW_03_01",CPnt(xx,basey),CPnt(1,1),_pRCUtil,Clip,_Client);
				
				xx += TileSize;
			}
		}
	}
	else
	{
		for(int i=0; i < xtiles; ++i)
		{
			DrawTile("GUI_GenW_03_01",CPnt(xx,basey),CPnt(1,1),_pRCUtil,Clip,_Client);

			xx += TileSize;
		}
	}

	// Text
		
	int StyleCenter = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER;
//	int StyleLeft = WSTYLE_TEXT_WORDWRAP;
	int m_TextColorM = 0xffffffff;
	int m_TextColorH = 0x60ffffff;
	int m_TextColorD = 0x60ffffff;
		
	// Top text

	CRC_Font* pF = GetFont("TEXT");
		
	if (m_Title.Len())
	{
		if (pF) 
		{	
			// Title
			
			CMWnd_Text_DrawFormated(
				_pRCUtil, 
				Clip, 
				pF, 
				CStr("§Z20")+m_Title, 
				3, -10,
				StyleCenter, 
				m_TextColorM, m_TextColorH, m_TextColorD, 
				m_Pos.GetWidth(), m_Pos.GetHeight());
		}
	}

	// Vertical tiles

	int ytiles = y1/TileSize-2;

	int yy = basey+16+41;

	for(int i=0; i < ytiles; ++i)
	{
		DrawTile("GUI_GenW_04_01",CPnt(basex-4, yy),CPnt(1,1),_pRCUtil,Clip,_Client);
		DrawTile("GUI_GenW_04_01",CPnt(basex+xx+TileSize+6, yy),CPnt(1,1),_pRCUtil,Clip,_Client);

		yy += TileSize;
	}

	// Horizontal bottom tiles

	if (m_BottomLine.Len())
	{
		int middletile = xtiles/2;
		
		if (!(xtiles%2))
		{
			for(int i=0; i < xtiles; ++i)
			{
				if (!(i >= (middletile-m_BottomTitleTiles) && i <= middletile+m_BottomTitleTiles-1))
					DrawTile("GUI_GenW_03_01",CPnt(basex+21+TileSize+i*TileSize,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
			}
		}
		else
		{
			for(int i=0; i < xtiles; ++i)
			{
				if (!(i >= middletile-m_BottomTitleTiles && i <= middletile+m_BottomTitleTiles))
					DrawTile("GUI_GenW_03_01",CPnt(basex+21+TileSize+i*TileSize,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
			}
		}
	}
	else
	{
		for(int i=0; i < xtiles; ++i)
		{
			DrawTile("GUI_GenW_03_01",CPnt(basex+21+TileSize+i*TileSize,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
		}
	}
		
	// Bottom text

	if (m_BottomLine.Len())
	{
		if (pF) 
		{	
			// Title
			
			CMWnd_Text_DrawFormated(
				_pRCUtil, 
				Clip, 
				pF, 
				CStr("§Z20")+m_BottomLine, 
				3, -9+yy+40,
				StyleCenter, 
				m_TextColorM, m_TextColorH, m_TextColorD, 
				m_Pos.GetWidth(), m_Pos.GetHeight());
		}
	}

	// Upper left

	DrawTile("GUI_GenW_02_01",CPnt(basex-11,basey-7),CPnt(2,2),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_02_02",CPnt(basex+TileSize-11,basey),CPnt(1,1),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_02_03",CPnt(basex-1,basey+TileSize-9),CPnt(1,1),_pRCUtil,Clip,_Client);

	// Upper right

	DrawTile("GUI_GenW_02b_01",CPnt(xx,basey-7),CPnt(2,2),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_02b_02",CPnt(TileSize+xx-TileSize-11,basey),CPnt(1,1),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_02b_03",CPnt(TileSize+xx-6-14,basey+TileSize-8),CPnt(1,1),_pRCUtil,Clip,_Client);

	// Lower left

	DrawTile("GUI_GenW_01_01",CPnt(basex-11,yy),CPnt(2,2),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_01_02",CPnt(basex-1,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_01_03",CPnt(basex-1+TileSize-10,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);

	// Lower right

	DrawTile("GUI_GenW_01b_01",CPnt(xx+1,yy),CPnt(2,2),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_01b_02",CPnt(TileSize+xx-TileSize+12,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
//	DrawTile("GUI_GenW_01b_03",CPnt(TileSize+xx-TileSize-11,yy+TileSize-6),CPnt(1,1),_pRCUtil,Clip,_Client);
}

// -------------------------------------------------------------------


MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModPlaneFrame, CMWnd);


void CMWnd_ModPlaneFrame::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	_pRCUtil->SetTexture(0);

	_pRCUtil->SetTextureScale(1,1);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y0));

	_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xe0202020, 0xe0202020);
	int mCol = 0xe010101a;
	_pRCUtil->Rect3D(_Clip, CRct(x0++, y0++, x1--, y1--), mCol, mCol, mCol);
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModBitmap, CMWnd);


void CMWnd_ModBitmap::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	_pRCUtil->SetTexture(m_Texture);

//	int x0 = 0;
//	int y0 = 0;
//	int x1 = m_Pos.GetWidth();
//	int y1 = m_Pos.GetHeight();
}

// -------------------------------------------------------------------
//  CMWnd_List2
// -------------------------------------------------------------------
CMWnd_L2Client::CMWnd_L2Client()
{
	m_pList = NULL;
}

int CMWnd_L2Client::Pos2Item(const CPnt& _Pos)
{
	if (!m_pList) return -1;
	if (!PntInWnd(_Pos)) return -1;

	CPnt Pos = _Pos;

	if (m_pList->m_Style & WSTYLE_CLIENT_SCROLLY)
	{
		Pos.y -= CMWND_LIST_YPADDING;
		int iItem = Pos.y / m_pList->m_ItemHeight;
		int nItems = m_pList->GetItemCount();
		if ((iItem >= 0) && (iItem < nItems)) return iItem;
		return -1;
	}
	else
	{
		Pos.x -= CMWND_LIST_XPADDING;
		Pos.y -= CMWND_LIST_YPADDING;
		int iItem = Pos.y / m_pList->m_ItemHeight;
/*		if (iItem < 0) iItem = 0;
		if (iItem >= m_pList->m_nItemPerCol) iItem = m_pList->m_nItemPerCol-1;
		int iCol = Pos.x / Max(1, m_pList->m_ListWidth);
		iItem += iCol*m_pList->m_nItemPerCol;*/

		int nItems = m_pList->GetItemCount();
		if ((iItem >= 0) && (iItem < nItems)) return iItem;
		return -1;
	}
}

void CMWnd_L2Client::SetContext(CMWnd_List2* _pContext)
{
	m_pList = _pContext;
}

aint CMWnd_L2Client::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_KEY :
		{
			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;

			if (m_pList && (m_Status & WSTATUS_FOCUS) && GetParent() && Key.IsDown())
			{
//ConOutL(CStrF("(CMWnd_L2Client::OnMessage) %d", Key.GetKey16()));
				if (Key.GetKey9() == SKEY_GUI_UP)
				{
					m_pList->OnDecrementItemFocus();
					return 1;
				}
				else if (Key.GetKey9() == SKEY_GUI_DOWN)
				{
					m_pList->OnIncrementItemFocus();
					return 1;
				}
				else if (Key.GetKey9() == SKEY_GUI_LEFT)
					return m_pList->GetParent()->OnChangeChildFocus(-1, 0);
				else if (Key.GetKey9() == SKEY_GUI_RIGHT)
					return m_pList->GetParent()->OnChangeChildFocus(1, 0);

				else if (Key.GetKey9() == SKEY_GUI_BUTTON0)
				{
					m_pList->ToggleItem(m_pList->GetItemFocus());
					return 1;
				}
			}
		}
		break;

	case WMSG_LOCKEDCURSORMOVE :
		{
			if (CursorInWnd(_pMsg))
			{
				CPnt Pos = GetCursorPos(_pMsg);
				int iItem = Pos2Item(Pos);
				if (iItem >= 0)
				{
					if (m_Action < 0)
						m_Action = (m_pList->m_lSelect[iItem] & 1) ? 0 : 1;
					else
					{
						if (m_Action)
						{
							m_pList->SelectItem(iItem);
						}
						else
						{
							m_pList->DeselectItem(iItem);
						}
					}
				}
			}
			return true;
		}

	case WMSG_LOCKEDCURSOR :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
			{
				CPnt Pos = GetCursorPos(_pMsg);
				int iItem = Pos2Item(Pos);

				if (_pMsg->IsReleased() && _pMsg->IsMouse1())
				{
					OnPress(_pMsg);
					UnlockInput();
					if (CursorInWnd(_pMsg))
					{
						// Pressed!
						if (iItem >= 0) m_pList->OnTouchItem(iItem, 0);
					}
				}
				else
				{
					if (iItem >= 0) m_pList->OnTouchItem(iItem, 1);
				}
			}
			return true;
		}


	case WMSG_CURSOR :
//		Error("OnMessage", "This item was not converted for non-blocking cursor interaction.");

		if (_pMsg->IsPressed() && _pMsg->IsMouse1())
		{
			CPnt Pos = GetCursorPos(_pMsg);
			int iItem = Pos2Item(Pos);

			if (iItem >= 0)
			{
				m_Action = (m_pList->m_lSelect[iItem] & 1) ? 0 : 1;
				if (!(m_pList->m_Style & WSTYLE_LIST_MULTISELECT)) m_pList->DeselectAll();
				m_pList->m_lSelect[iItem] ^= 1;
				LockInput();
			}
			else
				m_Action = -1;
		}
		break;

	}
	return CMWnd::OnMessage(_pMsg);
}

void CMWnd_L2Client::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	if (!m_pList) return;

	int nItems = m_pList->GetItemCount();
	const int itemh = m_pList->m_ItemHeight;

	int Alpha = m_pList->m_Col_Alpha << 24;
//	int ColH = m_pList->m_Col_Highlight + Alpha;
//	int ColD = m_pList->m_Col_Shadow + Alpha;
	int ColM = m_pList->m_Col_Background + Alpha;

//	int ColBG_Sel = m_pList->m_Col_Custom1_Focus + Alpha;
//	int ColT = m_pList->m_Col_Custom0 + Alpha;
//	int ColT_Sel = m_pList->m_Col_Custom0_Focus + Alpha;

	int x0 = 0;
//	int y0 = 0;
	int x1 = _Client.GetWidth();
//	int y1 = _Client.GetHeight();
//	int Style = m_pList->m_Style;
/*	if (Style & WSTYLE_CLIENTEDGE)
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);

	_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xff000000, 0xff000000, true);
*/
//	CClipRect Clip(x0, y0, x1, y1);
//	Clip += _Clip;

	{
		int y1 = 0;
		int y2 = 1;
		_pRCUtil->Rect(_Clip, CRct(x0, y1, x1, y2), ColM);
	}

	m_pList->OnPaintList(_pRCUtil, _Clip, _Client);

	{
//		int w = m_pList->m_ListWidth;
		int y1 = nItems*itemh;
		int y2 = _Client.GetHeight();
		_pRCUtil->Rect(_Clip, CRct(0, y1, _Client.GetWidth(), y2), ColM);
	}
}



MRTC_IMPLEMENT_DYNAMIC(CMWnd_L2Client, CMWnd);


// -------------------------------------------------------------------
CMWnd_L2ClientContainer::CMWnd_L2ClientContainer()
{
}

void CMWnd_L2ClientContainer::PreCreate(CMWnd_Param* _pParam)
{
	_pParam->m_Style |= WSTYLE_CLIENT_SCROLLY;
}

void CMWnd_L2ClientContainer::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Class)
{
	m_ClassName = _Class;
	CMWnd_Client::Create(_pWndParent, _Param, _ID);
}

void CMWnd_L2ClientContainer::SetContext(CMWnd_List2* _pContext)
{
	CMWnd_L2Client* pClient = (CMWnd_L2Client*)m_pWndClient;
	if (pClient) pClient->SetContext(_pContext);
}

spCMWnd CMWnd_L2ClientContainer::CreateClient()
{
	spCMWnd spWnd = (CMWnd*)(CReferenceCount*) MRTC_GOM()->CreateObject(m_ClassName);
	if (!spWnd) Error("CreateClient", "Unable to create client-window.");
	return spWnd;
}


MRTC_IMPLEMENT_DYNAMIC(CMWnd_L2ClientContainer, CMWnd_Client);


// -------------------------------------------------------------------
void CMWnd_List2::PreCreate(CMWnd_Param* _pParam)
{
	CMWnd::PreCreate(_pParam);
	_pParam->m_Status |= WSTATUS_MSGTOPARENT;
}

void CMWnd_List2::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	m_pWndClient = MNew(CMWnd_L2ClientContainer);
	spCMWnd spWnd = m_pWndClient;
	m_pWndClient->Create(NULL, CMWnd_Param(0,0,GetWidth(), GetHeight(), m_ClientStyle), "LISTCLIENT", GetClientClass());
	m_pWndClient->SetContext(this);
	AddChild(spWnd);
}

CMWnd_List2::CMWnd_List2()
{
	m_iItemFocus = -1;
	m_ClientStyle = 0;
	m_pWndClient = NULL;
	m_ItemHeight = 12;
	m_TextHeight = 12;
	m_nItems = 0;
	m_MinListWidth = 1;
	m_ListWidth = 1;
	m_bUpdate = false;
}

void CMWnd_List2::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if (_Key == "CLIENTSTYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "CLIENT_SCROLLX")
				m_ClientStyle |= WSTYLE_CLIENT_SCROLLX;
			else if (s == "CLIENT_SCROLLY")
				m_ClientStyle |= WSTYLE_CLIENT_SCROLLY;
			else if (s == "CLIENT_RESIZEPAD")
				m_ClientStyle |= WSTYLE_CLIENT_RESIZEPAD;
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, Value);
}

void CMWnd_List2::OnMove()
{
	if (m_pWndClient) m_pWndClient->SetPosition(CRct(0, 0, GetWidth(), GetHeight()));
	CMWnd::OnMove();
	Update();
}

void CMWnd_List2::OnRefresh()
{
	if (m_bUpdate) Update();
}

CStr CMWnd_List2::GetClientClass()
{
	return "CMWnd_L2Client";
}

void CMWnd_List2::Update()
{
//LogFile(CStrF("ClipClient %d, %d", m_pWndClipClient->GetWidth(), m_pWndClipClient->GetHeight()));

	if (!m_pWndClient) return;

	m_bUpdate = false;
	int nItems = m_nItems;
	if (m_pWndClient->m_Style & WSTYLE_CLIENT_SCROLLY)
		m_pWndClient->SetClientSize(GetWidth(), nItems*m_ItemHeight);
	else
	{
		CMWnd* pClipClient = m_pWndClient->GetClipClient();
		int h = (pClipClient) ? pClipClient->GetHeight() : 0;
		m_pWndClient->SetClientSize(GetWidth(), h);
	}
}

int CMWnd_List2::IsValidItem(int _iItem)
{
	return (_iItem >= 0) && (_iItem < m_nItems);
}

int CMWnd_List2::GetItemCount()
{
	return m_nItems;
}

void CMWnd_List2::SetItemCount(int _nItems)
{
	if (m_nItems != _nItems)
	{
		m_lSelect.SetLen(_nItems);
		if(_nItems > m_nItems)
			FillChar(&m_lSelect[m_nItems], _nItems - m_nItems, 0);
		m_nItems = _nItems;
		AdjustItemFocus();
		Update();
	}
}

void CMWnd_List2::SetColumnCount(int _nColumns)
{
}

void CMWnd_List2::SetColumn(const CStr& _Name, int _OrgWidth, int _Flags)
{
}

int CMWnd_List2::GetSelected(int _iLast)
{
	int iSearch = _iLast + 1;
	int nItems = m_lSelect.Len();
	while((iSearch < nItems) && !(m_lSelect[iSearch] & 1)) iSearch++;
	if (iSearch == nItems) return -1;
	return iSearch;
}

void CMWnd_List2::SelectItem(int _iItem)
{
	if (!IsValidItem(_iItem)) return;

	if (!(m_lSelect[_iItem] & 1))
	{
		if (!(m_Style & WSTYLE_LIST_MULTISELECT)) DeselectAll();
		m_lSelect[_iItem] |= 1;
		OnTouchItem(_iItem, 1);
	}
}

void CMWnd_List2::DeselectItem(int _iItem)
{
	if (!IsValidItem(_iItem)) return;

	if ((m_lSelect[_iItem] & 1))
	{
		m_lSelect[_iItem] &= ~1;
		OnTouchItem(_iItem, 0);
	}
}

void CMWnd_List2::DeselectAll()
{
	int nItems = m_lSelect.Len();
	for(int i = 0; i < nItems; i++) m_lSelect[i] &= ~1;
}

int CMWnd_List2::ToggleItem(int _iItem)
{
	if (!IsValidItem(_iItem)) return 0;

	if (m_lSelect[_iItem] & 1)
	{
		DeselectItem(_iItem);
		return 0;
	}
	else
	{
		SelectItem(_iItem);
		return 1;
	}
}


void CMWnd_List2::AdjustItemFocus()
{
	if (m_iItemFocus < 0) m_iItemFocus = 0;
	if (m_iItemFocus >= m_nItems) m_iItemFocus = m_nItems-1;
}

int CMWnd_List2::GetItemFocus()
{
	AdjustItemFocus();
	return m_iItemFocus;
}

void CMWnd_List2::OnIncrementItemFocus()
{
	m_iItemFocus++;
	AdjustItemFocus();
	OnMakeVisible(m_iItemFocus);
}

void CMWnd_List2::OnDecrementItemFocus()
{
	m_iItemFocus--;
	AdjustItemFocus();
	OnMakeVisible(m_iItemFocus);
}

void CMWnd_List2::OnPaintList(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	int nItems = GetItemCount();
	int Width = _Client.GetWidth();

	{
		CRct Total(CPnt(0,0), CPnt(Width, nItems*m_ItemHeight));
		if (!_Clip.Visible(Total)) return;
	}

	int Alpha = m_Col_Alpha << 24;
	int ColM = m_Col_Background + Alpha;

	int iItem = 0;
	for(int i = 0; i < nItems; i++, iItem++)
	{
		int x = 0;
		int y = i * m_ItemHeight;
		CClipRect ItemClip;
		CRct ItemRct(0, 0, Width, m_ItemHeight);
		CPnt ItemPos(x, y);
		ItemClip.clip = ItemRct;
		ItemClip.clip.p0 += ItemPos;
		ItemClip.clip.p1 += ItemPos;
		ItemClip.ofs = ItemPos;
		if (!_Clip.Visible(ItemClip.clip)) continue;
		ItemClip += _Clip;

		if (iItem < nItems)
			OnPaintItem(_pRCUtil, ItemClip, ItemRct, iItem);
		else
			_pRCUtil->Rect(ItemClip, ItemRct, ColM);
	}
}

void CMWnd_List2::OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client, int _iItem)
{
	CRC_Font* pF = GetFont("TEXT");

	int Alpha = m_Col_Alpha << 24;
//	int ColH = m_Col_Highlight + Alpha;
//	int ColD = m_Col_Shadow + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColBG_Sel = m_Col_Custom1_Focus + Alpha;
	int ColT = m_Col_Custom0 + Alpha;
	int ColT_Sel = m_Col_Custom0_Focus + Alpha;

	int bSelected = (m_lSelect[_iItem] & 1);
	int TCol = ColT;
	int BGCol = ColM;
	if (bSelected) 
	{
		TCol = ColT_Sel;
		BGCol = ColBG_Sel;
	}
	_pRCUtil->Rect(_Clip, _Client, BGCol);
	_pRCUtil->Text(_Clip, pF, 0, 1, CStrF("Item %d", _iItem), TCol, m_TextHeight);
}

void CMWnd_List2::OnTouchItem(int _iItem, int _bSelected)
{
	if (GetParent())
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, _iItem, _bSelected);
		m_pWndParent->OnMessage(&Msg);
	}
}

void CMWnd_List2::OnAction(int _iItem)
{
	if (GetParent())
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, _iItem);
		m_pWndParent->OnMessage(&Msg);
	}
}

void CMWnd_List2::OnMakeVisible(int _iItem)
{
	if (!IsValidItem(_iItem)) return;
	if (!m_pWndClient) return;
	m_pWndClient->MakeVisible(CRct(0,_iItem*m_ItemHeight, GetWidth(), (_iItem+1)*m_ItemHeight - 1));
}


MRTC_IMPLEMENT_DYNAMIC(CMWnd_List2, CMWnd);



