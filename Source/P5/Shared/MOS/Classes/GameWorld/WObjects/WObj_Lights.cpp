#include "PCH.h"
#include "WObj_System.h"
#include "WObj_Lights.h"

#define DO_IF(x) (!(x)) ? (void)0 :
#define DBG_OUT DO_IF(0) M_TRACE

// -------------------------------------------------------------------
//  LIGHT
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Light, CWObject_Model, 0x0100);

// Make sure these do not collide with CXR_LIGHT_xxxxxxxxxxxxxxx flags
//
// Define data packers
//
CDataPacker<0,0xFF,24> CWObject_Light::m_ColorR;
CDataPacker<0,0xFF,16> CWObject_Light::m_ColorG;
CDataPacker<0,0xFF,8> CWObject_Light::m_ColorB;
CDataPacker<0,0x3,6> CWObject_Light:: m_UNUSED; // YAY \0/
CDataPacker<0,0xF,2> CWObject_Light::m_FlickerType;
CDataPacker<0,0x1,1> CWObject_Light::m_Broken;
CDataPacker<0,0x1,0> CWObject_Light::m_OnFlag;

//
CDataPacker<1,0xFFF,20> CWObject_Light::m_LightIndex;
CDataPacker<1,0xF,16> CWObject_Light::m_LightType;
CDataPacker<1,0xFF,8> CWObject_Light::m_FlareDepthOffset;
CDataPacker<1,0xFF,0> CWObject_Light::m_LightFlare;

//
CDataPacker<2,0xFFFF,16> CWObject_Light::m_Range;
CDataPacker<2,0xFFFF,0> CWObject_Light::m_SurfaceResourceID;

//
CDataPacker<3,0xFFFFFFFF,0> CWObject_Light::m_StartTime;	// TODO: optimize this!

//
CDataPacker<4,0xFFFF,16> CWObject_Light::m_SpotHeight;
CDataPacker<4,0xFFFF,0> CWObject_Light::m_SpotWidth;

//
CDataPacker<5,0xFF,24> CWObject_Light::m_NumSlaveLights;
CDataPacker<5,0xFF,16> CWObject_Light::m_NumActiveSlaveLights;
CDataPacker<5,0xFF,8> CWObject_Light::m_FlareWidth;
CDataPacker<5,0xFF,0> CWObject_Light::m_FlareHeight;

//
CDataPacker<6,0xFFF,20> CWObject_Light::m_FlareRange;	// range/4
CDataPacker<6,0xFFF,8> CWObject_Light::m_FlickerFreq;	// freq*8
CDataPacker<6,0xFF,0> CWObject_Light::m_FlickerSpread;
M_STATIC_ASSERT(CWO_Light_MessageData::TFreq::Scale == 8); // remember to update CWObject_Light::m_FlickerFreq when changing this!

//
CDataPacker<7,0xFF,24> CWObject_Light::m_LastColorR;
CDataPacker<7,0xFF,16> CWObject_Light::m_LastColorG;
CDataPacker<7,0xFF,8> CWObject_Light::m_LastColorB;
CDataPacker<7,0xFF,0> CWObject_Light::m_LastFlickerSpread;




const char* g_LightFlickerTypeTranslate[] = 
{
	"none", "pulse", "sine", "randompulse", "$$break$$", "randomsine", (char*)NULL
};


//
//
//
CWObject_Light::CWObject_Light()
{
	MAUTOSTRIP(CWObject_Light_ctor, MAUTOSTRIP_VOID);

	//m_ClientFlags |= CWO_CLIENTFLAGS_NOREFRESH;
	m_LightIndex.Set(this, 0);
	m_ColorR.Set(this,0xFF);
	m_ColorG.Set(this,0xFF);
	m_ColorB.Set(this,0xFF);
	m_LastColorR.Set(this,0xFF);
	m_LastColorG.Set(this,0xFF);
	m_LastColorB.Set(this,0xFF);
	m_Range.Set(this, 512);

	m_SpotHeight.Set(this, 256);
	m_SpotWidth.Set(this, 256);

	m_LightType.Set(this, 0);

	m_Broken.Set(this,0);
	m_OnFlag.Set(this,1);
	m_StartTime.Set(this, 0);

	m_Light = CVec3Dfp32(1.0f, 1.0f, 1.0f);
	m_Intensity = 1.0f;

	m_FlareWidth.Set(this, 48);
	m_FlareHeight.Set(this, 48);
	m_FlareRange.Set(this, 1000 / 4);
	m_FlareDepthOffset.Set(this,8);

	//
	CWO_Light_MessageData Default;
	m_FlickerFreq.Set(this, Default.m_FlickerFreq.value);
	m_FlickerType.Set(this, Default.m_FlickerType);
	m_FlickerSpread.Set(this, Default.m_FlickerSpread);

	//
	m_MasterLightID = 0;
	m_MasterLightName = "";
	m_NumActiveSlaveLights.Set(this, 0);
	m_NumSlaveLights.Set(this, 0);

	m_iAnim0 = 0;
	m_iAnim1 = 0;
	m_iAnim2 = 0;

	//
	m_Flags = 0;

	m_bActive = true;
	m_bAutoTurnOffRefresh = true;
}

//
//
//
int32 CWObject_Light::DoAutoDestroy() { return 1; }
int32 CWObject_Light::DoReplicate() { return (m_Flags & CXR_LIGHT_FLARE) ? 1 : 0; }

//
//
//
void CWObject_Light::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Light_OnEvalKey, MAUTOSTRIP_VOID);

	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	const fp32 _Valuef = _Value.Val_fp64();
	const int _Valuei = _Value.Val_int();
	switch (_KeyHash)
	{
	case MHASH3('LIGH','T_IN','DEX'): // "LIGHT_INDEX"
		{
			m_LightIndex.Set(this,_Valuei);
			break;
		}
	case MHASH2('LIGH','T'): // "LIGHT"
		{
			m_Intensity = _Valuef * (1.0f / 256.0f);
			break;
		}
	case MHASH3('LIGH','T_RA','NGE'): // "LIGHT_RANGE"
		{
			m_Range.Set(this,int(_Valuef));
			break;
		}
	case MHASH4('LIGH','T_PR','OJMA','P'): // "LIGHT_PROJMAP"
		{
			m_SurfaceResourceID.Set(this, m_pWServer->GetMapData()->GetResourceIndex_Surface(_Value));
			break;
		}
	case MHASH5('LIGH','T_PR','OJMA','PORI','GIN'): // "LIGHT_PROJMAPORIGIN"
		{
			m_ProjMapOrigin.ParseString(_Value);
			m_Flags |= LIGHT_PROJMAPORIGIN;
			break;
		}
	case MHASH3('LIGH','T_TY','PE'): // "LIGHT_TYPE"
		{
			m_LightType.Set(this,_Valuei);
			break;
		}
	case MHASH3('LIGH','T_FL','ARE'): // "LIGHT_FLARE"
		{
			m_LightFlare.Set(this, Min(255, _Valuei));
			if (m_LightFlare.Get(this))
				m_Flags |= CXR_LIGHT_FLARE;
			break;
		}

/*	
	case MHASH6('LIGH','T_FL','ARED','IREC','TION','AL'): // "LIGHT_FLAREDIRECTIONAL"
		{
			if (_Valuei)
				m_Flags |= CXR_LIGHT_FLAREDIRECTIONAL;
			break;
		}
*/
	case MHASH5('LIGH','T_IN','ITAL','STAT','E'): // "LIGHT_INITALSTATE"
		{
			if (_Valuei)
				m_OnFlag.Set(this, 1);
			else
				m_OnFlag.Set(this, 0);
			break;
		}
/*	
	else if (_KeyHash == MHASH5('LIGH','T_FL','AREA','BSOL','UTE'))
	{
		if (_Valuei)
			m_Flags |= LIGHT_FLAREABS;
	} 
*/
	case MHASH4('LIGH','T_FL','AREW','IDTH'): // "LIGHT_FLAREWIDTH"
		{
			m_FlareWidth.Set(this,_Valuei >> 2);
			break;
		}
	case MHASH5('LIGH','T_FL','AREH','EIGH','T'): // "LIGHT_FLAREHEIGHT"
		{
			m_FlareHeight.Set(this,_Valuei >> 2);
			break;
		}
	case MHASH4('LIGH','T_FL','ARER','ANGE'): // "LIGHT_FLARERANGE"
		{
			m_FlareRange.Set(this, _Valuei / 4);
			break;
		}
	case MHASH6('LIGH','T_FL','ARED','EPTH','OFFS','ET'): // "LIGHT_FLAREDEPTHOFFSET"
		{
			m_FlareDepthOffset.Set(this,_Valuei);
			break;
		}
	case MHASH3('LIGH','T_CO','LOR'): // "LIGHT_COLOR"
		{
			CVec3Dfp32 v;
			v.ParseColor(_Value, true);
			m_Light.CompMul(v, m_Light);
			break;
		}
	case MHASH4('LIGH','T_DI','RECT','ION'): // "LIGHT_DIRECTION"
		{
			CVec3Dfp32 p; p.ParseString(_Value);
			p.Normalize();
			CMat4Dfp32 m;
			p.SetMatrixRow(m, 0);
			if (p[0] != 0.0f)
				CVec3Dfp32(0,1,0).SetMatrixRow(m, 1);
			else
				CVec3Dfp32(1,0,0).SetMatrixRow(m, 1);
			m.RecreateMatrix(0, 1);
			GetPosition().SetMatrixRow(m, 3);
			m_pWServer->Object_SetPosition(m_iObject, m);
			break;
		}
	case MHASH4('LIGH','T_SP','OTWI','DTH'): // "LIGHT_SPOTWIDTH"
		{
		
			fp32 SpotWidth = Min(89.0f, Max(1.0f, 0.5f * _Valuef));
			m_SpotWidth.Set(this,Min(65535, RoundToInt(256.0f * M_Tan(SpotWidth * ( _PI / 180.0f ) ))));
			break;
		}
	case MHASH4('LIGH','T_SP','OTHE','IGHT'): // "LIGHT_SPOTHEIGHT"
		{
			fp32 SpotHeight= Min(89.0f, Max(1.0f, 0.5f * _Valuef));
			m_SpotHeight.Set(this,Min(65535, RoundToInt(256.0f * M_Tan(SpotHeight * ( _PI / 180.0f ) ))));
			break;
		}
	case MHASH3('LIGH','T_MA','STER'): // "LIGHT_MASTER"
		{
			m_MasterLightName = _Value;
			break;
		}
	case MHASH5('LIGH','T_FL','ICKE','RFRE','Q'): // "LIGHT_FLICKERFREQ"
		{
			const fp32 Scale = fp32(CWO_Light_MessageData::TFreq::Scale);
			fp32 Freq = Clamp(_Valuef, 0.0f, 511.0f);
			m_FlickerFreq.Set(this, int(Freq * Scale));
			break;
		}
	case MHASH5('LIGH','T_FL','ICKE','RSPR','EAD'): // "LIGHT_FLICKERSPREAD"
		{
			uint FlickerSpread = Clamp(_Valuei, 0, 255);
			m_FlickerSpread.Set(this, FlickerSpread);
			break;
		}
	case MHASH5('LIGH','T_FL','ICKE','RTYP','E'): // "LIGHT_FLICKERTYPE"
		{
			m_FlickerType.Set(this, _Value.TranslateInt(g_LightFlickerTypeTranslate));
			break;
		}
	case MHASH5('LIGH','T_NO','FLIC','KERS','OUND'): // "LIGHT_NOFLICKERSOUND"
		m_Flags |= (_Value.Val_int() ? LIGHT_NOFLICKERSOUND : 0);
	
	case MHASH3('LIGH','T_FL','AGS'): // "LIGHT_FLAGS"
		{
			m_Flags |= CXR_Light::ParseFlags(_Value);
			break;
		}

	case MHASH6('LIGH','T_DO','NTEV','ALTH','ISAG','AIN'): // "LIGHT_DONTEVALTHISAGAIN"
		{
			return;
			break;
		}
	case MHASH4('LIGH','T_TR','ACEL','EN'): // "LIGHT_TRACELEN"
		{
			return;
			break;
		}
	case MHASH3('LIGH','T_PO','WER'): // "LIGHT_POWER"
		{
			return;
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Light::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Light_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject_Model::OnFinishEvalKeys();

	if (m_Flags & LIGHT_PROJMAPORIGIN)
	{
		CMat4Dfp32 PosInv;
		GetPositionMatrix().InverseOrthogonal(PosInv);
		m_ProjMapOrigin *= PosInv;
		CWO_PhysicsState Phys;
		Phys.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_SPHERE, 0, 4, m_ProjMapOrigin));
		Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET | OBJECT_PHYSFLAGS_ROTATION;
		m_pWServer->Object_SetPhysics(m_iObject, Phys);
	}


	m_iAnim2 = m_pWServer->GetMapData()->GetResourceIndex_Sound("Env_Lamp_Flicker_01"); // precache
	m_bActive = m_OnFlag.Get(this) != 0;

	// If it's sky-light, make it's visibility inifite.
	if (m_LightType.Get(this) == CXR_LIGHTTYPE_FAKESKYRADIOSITY || m_LightType.Get(this) == CXR_LIGHTTYPE_PARALLELL) 
		m_ClientFlags |= CWO_CLIENTFLAGS_LINKINFINITE;

	// Set color
	m_ColorR.Set(this, (int32)(m_Light.k[0] * m_Intensity * (0.25f*0xFF)));
	m_ColorG.Set(this, (int32)(m_Light.k[1] * m_Intensity * (0.25f*0xFF)));
	m_ColorB.Set(this, (int32)(m_Light.k[2] * m_Intensity * (0.25f*0xFF)));

	// FIX ME!
	//if((uint32)(m_Data[4] >> 16) == 0xffff) // kma: wtf?
	//	m_Data[4] = (m_Data[4] & 0xffff) + ((m_Data[4] & 0xffff) << 16);

	m_ClientFlags = m_ClientFlags | (m_Flags << CWO_CLIENTFLAGS_USERSHIFT);

	//m_Data[7] = m_SpotWidth + (m_SpotHeight << 16);

	if (!m_iModel[0] && (m_Flags & CXR_LIGHT_FLARE))
		m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("FLARE");

	// Don't replicate unless something is visible.
	//if (!DoReplicate())
	//	m_ClientFlags |= CWO_CLIENTFLAGS_NOUPDATE;

	if (DoAutoDestroy())
	{
		if (!(m_Flags & CXR_LIGHT_FLARE))
		{
			m_pWServer->Object_Destroy(m_iObject);
			return;
		}
	}

	// Object_SetVisBox
	{
		int32 Range = m_Range.Get(this);
//		int32 ID = m_LightIndex.Get(this);
		int32 Type = m_LightType.Get(this);
		int32 Flags = (m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT);

		CVec3Dfp32 Intens(1);

		CXR_Light Light(GetPositionMatrix(), Intens, Range, Flags, Type);
		Light.CalcBoundBoxFast();

		m_pWServer->Object_SetVisBox(m_iObject, Light.m_BoundBox.m_Min, Light.m_BoundBox.m_Max);
	}

}

void CWObject_Light::OnSpawnWorld()
{
	CWObject_Model::OnSpawnWorld();
}

void CWObject_Light::OnSpawnWorld2()
{
	CWObject_Model::OnSpawnWorld2();

	RegisterToMaster();
	if (m_NumSlaveLights.Get(this))
	{
		if (m_OnFlag.Get(this) && !m_NumActiveSlaveLights.Get(this))
			SendSignalToMasterLight(MSG_SLAVE_OFF);
		else if (!m_OnFlag.Get(this) && m_NumActiveSlaveLights.Get(this))
			SendSignalToMasterLight(MSG_SLAVE_ON);
	}

	UpdateColor(int(2 * m_pWServer->GetGameTicksPerSecond()));
}


void CWObject_Light::OnRefresh()
{
	MSCOPESHORT(CWObject_Light::OnRefresh);

	/*CVec3Dfp32 vIntens;
	GetIntens(this, vIntens, 0);
	CPixel32 Intens(vIntens[0] * 255, vIntens[1] * 255, vIntens[2] * 255, 255);
	m_pWServer->Debug_RenderMatrix(GetPositionMatrix(), 1.0f, false, Intens, Intens, Intens);*/

	if (m_FlickerType.Get(this) && m_OnFlag.Get(this))
	{
		UpdateColor(int(2 * m_pWServer->GetGameTicksPerSecond()));
	}
	else if (m_bAutoTurnOffRefresh)
	{
		// Turn off refresh
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
	}

	CWObject_Model::OnRefresh();
}


bool CWObject_Light::RegisterToMaster()
{
	if (m_MasterLightName)
	{
		if (!m_MasterLightID)
		{
			m_MasterLightID = m_pWServer->Selection_GetSingleTarget(m_MasterLightName);
			CWObject* pMasterLight = m_pWServer->Object_Get(m_MasterLightID);
			if (!pMasterLight)
			{
				ConOutL(CStrF("§cf80WARNING [Light %s] RegisterToMaster: Master light not found (%s)", GetName(), m_MasterLightName.DbgName().Str()));
				return false;
			}

			DBG_OUT("[Light %d, %s] RegisterToMaster, Master: %d (%s)\n", 
				m_iObject, GetName(), m_MasterLightID, pMasterLight->GetName());

			uint bActive = m_OnFlag.Get(this);
			CWObject_Message Msg(OBJMSG_IMPULSE, MSG_SLAVE_REG, bActive, m_iObject);
			pMasterLight->OnMessage(Msg);
		}
		return true;
	}
	return false;
}

//
//
//
void CWObject_Light::SendSignalToMasterLight(int32 _Signal, int32 _Param)
{
	if (RegisterToMaster())
	{
		CWObject* pMasterLight = m_pWServer->Object_Get(m_MasterLightID);
		if (!pMasterLight)
		{
			ConOutL(CStrF("§cf80WARNING [Light %s] SendSignalToMasterLight: Master light not found (%s)", GetName(), m_MasterLightName.DbgName().Str()));
			return;
		}

		CWObject_Message Msg;
		Msg.m_iSender = m_iObject;
		Msg.m_Msg = OBJMSG_IMPULSE;
		Msg.m_Param0 = _Signal;
		Msg.m_Param1 = _Param;
		pMasterLight->OnMessage(Msg);
	}
}

//
//
// Note. When slave light registers it self to the master light it assumes that the light is on
//
aint CWObject_Light::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_LIGHT_GETINDEX:
		return m_LightIndex.Get(this);

	case OBJMSG_LIGHT_MORPH:
		{
			{
				int StartTick = m_StartTime.Get(this);
				fp32 Duration = m_iAnim1 * (1.0f/100.0f);
				fp32 Amount = fp32(m_pWServer->GetGameTick() - StartTick) * m_pWServer->GetGameTickTime() / Duration;

				if(Duration && Amount < 1)
				{
					int32 dR = m_ColorR.Get(this);
					int32 dG = m_ColorG.Get(this);
					int32 dB = m_ColorB.Get(this);
					int32 sR = m_LastColorR.Get(this);
					int32 sG = m_LastColorG.Get(this);
					int32 sB = m_LastColorB.Get(this);
					int32 fR = int32(fp32(sR+(dR-sR)*Amount));
					int32 fG = int32(fp32(sG+(dG-sG)*Amount));
					int32 fB = int32(fp32(sB+(dB-sB)*Amount));
					m_ColorR.Set(this, fR);
					m_ColorG.Set(this, fG);
					m_ColorB.Set(this, fB);
				}
			}

			CFStr String = (char *)_Msg.m_pData;
			CFStr Duration = String.GetStrSep(",");
			CFStr ColorStr = String.GetStrSep(",");
			CFStr TypeStr = String.GetStrSep(",");
			CFStr HzStr = String.GetStrSep(",");
			CFStr SpreadStr = String.GetStrSep(",");
			CFStr IntensityStr = String.GetStrSep(",");

			//ConOutL(CStrF("LightMorph, dur=%s col=%s type=%s hz=%s spread=%s",
			//	Duration.Str(), ColorStr.Str(), TypeStr.Str(), HzStr.Str(), SpreadStr.Str()));

			m_LastColorR.Set(this, m_ColorR.Get(this));
			m_LastColorG.Set(this, m_ColorG.Get(this));
			m_LastColorB.Set(this, m_ColorB.Get(this));
			m_LastFlickerSpread.Set(this, m_FlickerSpread.Get(this));
			m_iAnim0 = m_FlickerFreq.Get(this);

			if (ColorStr.Len() > 0)
				m_Light.ParseColor(ColorStr, true);

			if (IntensityStr.Len() > 0)
				m_Intensity = IntensityStr.Val_fp64() * (1.0f / 256.0f);

			m_ColorR.Set(this, Clamp((int)(m_Light.k[0] * m_Intensity * (0.25f*0xFF)), 0, 255));
			m_ColorG.Set(this, Clamp((int)(m_Light.k[1] * m_Intensity * (0.25f*0xFF)), 0, 255));
			m_ColorB.Set(this, Clamp((int)(m_Light.k[2] * m_Intensity * (0.25f*0xFF)), 0, 255));

			if(TypeStr.Len() > 0)
				m_FlickerType.Set(this, TypeStr.TranslateInt(g_LightFlickerTypeTranslate));

			if (HzStr.Len() > 0)
			{
				const fp32 Scale = fp32(CWO_Light_MessageData::TFreq::Scale);
				fp32 Freq = Clamp(HzStr.Val_fp64(), 0.0f, 511.0f);
				m_FlickerFreq.Set(this, int(Freq * Scale));
			}

			if(SpreadStr.Len() > 0)
				m_FlickerSpread.Set(this, SpreadStr.Val_int());

			m_iAnim1 = int32(Duration.Val_fp64()*100);
			m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA | CWO_DIRTYMASK_ANIM);
			m_StartTime.Set(this, m_pWServer->GetGameTick());

			ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
			return 1;
		}

	case OBJMSG_IMPULSE:
	case OBJMSG_LIGHT_IMPULSE:
		{
			if(m_Broken.Get(this))
				return 1;

			switch (_Msg.m_Param0)
			{
			case MSG_OFF:					// Light Message
				if (m_OnFlag.Get(this))
				{
					// Turn OFF
					m_OnFlag.Set(this,0);
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					SendSignalToMasterLight(MSG_SLAVE_OFF);
				}
				break;

			case MSG_ON:					// Light Message
				if (!m_OnFlag.Get(this))
				{
					// Turn ON
					m_OnFlag.Set(this,1);
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					SendSignalToMasterLight(MSG_SLAVE_ON);
				}
				break;

			case MSG_TOGGLE:				// Light Message
				{
					// Toggle State
					m_OnFlag.Set(this,!m_OnFlag.Get(this));
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

					if(m_OnFlag.Get(this))
						SendSignalToMasterLight(MSG_SLAVE_ON);
					else
						SendSignalToMasterLight(MSG_SLAVE_OFF);
				}
				break;

			case MSG_BREAK:					// Light Message
				{
					m_Broken.Set(this, 1);
					m_FlickerType.Set(this, CLightIntens::EBreak);
					m_StartTime.Set(this, m_pWServer->GetGameTick());
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					SendSignalToMasterLight(MSG_SLAVE_OFF, 1);
				}
				break;

			case MSG_FLICKER:				// Light Message
				if (!m_Broken.Get(this))
				{
					if(_Msg.m_pData)
					{
						CWO_Light_MessageData& MsgData = *(CWO_Light_MessageData*)_Msg.m_pData;

						m_FlickerType.Set(this, MsgData.m_FlickerType);
						m_FlickerSpread.Set(this, MsgData.m_FlickerSpread);
						m_FlickerFreq.Set(this, MsgData.m_FlickerFreq.value);
					}
					else
					{
						m_FlickerType.Set(this, _Msg.m_Param1);
					}
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
				}
				break;

			case MSG_FLICKERFREQ:
				m_FlickerFreq.Set(this, _Msg.m_Param1);
				break;

			case MSG_FLICKERSPREAD:
				m_FlickerSpread.Set(this, _Msg.m_Param1);
				break;

			case MSG_SLAVE_OFF:				// Master light message, Slave light off
				{
					int nActive = m_NumActiveSlaveLights.Get(this) - 1;
					DBG_OUT("[Light %d, %s], MSG_SLAVE_OFF, nTotal:%d, nActive:%d\n", 
						m_iObject, GetName(), m_NumSlaveLights.Get(this), nActive);

					m_NumActiveSlaveLights.Set(this, nActive);
					if (m_MasterLightName)
					{
						DBG_OUT("[Light %d, %s], passing MSG_SLAVE_OFF onto supermaster: %d (%s)\n",
							m_iObject, GetName(), m_MasterLightID, m_MasterLightName.DbgName().Str());

						SendSignalToMasterLight(MSG_SLAVE_OFF); // pass on to super-master
						if (nActive == 0)
						{
							DBG_OUT("[Light %d, %s], sending extra MSG_SLAVE_OFF to supermaster: %d (%s)\n",
								m_iObject, GetName(), m_MasterLightID, m_MasterLightName.DbgName().Str());

							// send one extra to master when the whole group is dead
							// (TODO: perhaps the middle-master should never register itself as slave?)
							SendSignalToMasterLight(MSG_SLAVE_OFF);
						}
					}

					if (_Msg.m_Param1)
					{
						// the light broke
						m_FlickerType.Set(this, CLightIntens::EBreak);
						m_StartTime.Set(this, m_pWServer->GetGameTick());
					}

					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
				break;

			case MSG_SLAVE_ON:				// Master light message, Slave light on
				{
					int nActive = m_NumActiveSlaveLights.Get(this) + 1;
					m_NumActiveSlaveLights.Set(this, nActive);
					if (m_MasterLightName)
					{
						SendSignalToMasterLight(MSG_SLAVE_ON); // pass on to super-master
						if (nActive == 1)
						{
							// send one extra to master when the whole group is activated
							// (TODO: perhaps the middle-master should never register itself as slave?)
							SendSignalToMasterLight(MSG_SLAVE_ON);
						}
					}

					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
				break;

			case MSG_SLAVE_REG:				// Master light message, Slave light register
				{
					//ConOutL("(CWObject_Light::OnMessage): Slave light registerd");
					m_NumSlaveLights.Set(this, m_NumSlaveLights.Get(this)+1);
					uint bActive = _Msg.m_Param1;
					if (bActive)
						m_NumActiveSlaveLights.Set(this, m_NumActiveSlaveLights.Get(this)+1);

					DBG_OUT("[Light %d, %s], MSG_SLAVE_REG, nTotal:%d, nActive:%d\n",
						m_iObject, GetName(), m_NumSlaveLights.Get(this), m_NumActiveSlaveLights.Get(this));

					if (m_MasterLightName)
					{
						DBG_OUT("[Light %d, %s], passing MSG_SLAVE_REG onto supermaster: %d (%s)\n",
							m_iObject, GetName(), m_MasterLightID, m_MasterLightName.DbgName().Str());
						SendSignalToMasterLight(MSG_SLAVE_REG, bActive); // pass on to super-master
					}

					m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

					return 1; // Prevent color from being updated
				}
				break;

			case MSG_GETFLICKERDATA:
				if (_Msg.m_pData)
				{
					CWO_Light_MessageData& MsgData = *(CWO_Light_MessageData*)_Msg.m_pData;
					MsgData.m_FlickerType = m_FlickerType.Get(this);
					MsgData.m_FlickerSpread = m_FlickerSpread.Get(this);
					MsgData.m_FlickerFreq.value = m_FlickerFreq.Get(this);
					return 1;
				}
				break;

			default:
				return 1;
			}

#ifndef M_RTM
			// Debug check, shouldn't be needed
			if(m_NumActiveSlaveLights.Get(this) > m_NumSlaveLights.Get(this))
				ConOutL(CStrF("§cf80WARNING (CWObject_Light::OnMessage): More lights are on then there are registered slaves (Active %d, Registered %d)", m_NumActiveSlaveLights.Get(this), m_NumSlaveLights.Get(this)));
#endif

			// We extrapolate a bit, so that broken lights will have time to fade out
			UpdateColor(int(2.0f * m_pWServer->GetGameTicksPerSecond()));
			return 1;
		}

	case OBJMSG_LIGHT_SETPROJMAP:
		{
			// We don't want to override the projection map if it's already set on the model
//			if(m_SurfaceResourceID.Get(this) == 0)
				m_SurfaceResourceID.Set(this, m_pWServer->GetMapData()->GetResourceIndex_Surface((char *)_Msg.m_pData));
				m_pWServer->Object_SetDirty(m_iObject, 1);
			return 1;
		}

	case OBJMSG_LIGHT_INITPROJMAP:
		{
			CWObject* pSender = m_pWServer->Object_Get(_Msg.m_iSender);
			if (!pSender)
				return 0;

			const char* pProjMapName = (const char *)_Msg.m_Param0;
			if (!pProjMapName)
				return 0;

			// Set projection map. Only do this if the Light hasn't already set it..
			if (m_SurfaceResourceID.Get(this) == 0)
			{
				m_SurfaceResourceID.Set(this, m_pWServer->GetMapData()->GetResourceIndex_Surface(pProjMapName));
				m_pWServer->Object_SetDirty(m_iObject, 1);
			}

			// Set projection map origin. Only do this if the Light hasn't already set it..
			if ((m_Flags & LIGHT_PROJMAPORIGIN) == 0)
			{
				// Rotate light to match rotation of the lamp
				m_pWServer->Object_SetRotation(m_iObject, pSender->GetPositionMatrix());

				// Convert the Orgin from sender-local space to our local space
				m_ProjMapOrigin = _Msg.m_VecParam0;
				m_ProjMapOrigin *= pSender->GetPositionMatrix();

				CMat4Dfp32 PosInv;
				GetPositionMatrix().InverseOrthogonal(PosInv);
				m_ProjMapOrigin *= PosInv;

				// Create physics prim
				CWO_PhysicsState Phys;
				Phys.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_SPHERE, 0, 4, m_ProjMapOrigin));
				Phys.m_PhysFlags = OBJECT_PHYSFLAGS_OFFSET | OBJECT_PHYSFLAGS_ROTATION;
				m_pWServer->Object_SetPhysics(m_iObject, Phys);
			}
			return 1;
		}

	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
				if(pMsg->m_Msg == OBJMSG_LIGHT_SETPROJMAP)
					m_pWServer->GetMapData()->GetResourceIndex_Surface((char *)pMsg->m_pData);
			}
		}
	case OBJMSG_LIGHT_GETSTATUS:
		{
			return m_OnFlag.Get(this) != 0 && !m_Broken.Get(this);
		}
	}

	return CWObject_Model::OnMessage(_Msg);
}

//
//
//
void CWObject_Light::UpdateColor(int _ExtrapolateTick)
{
	// I don't think this function is needed anymore
//	return;

	// Well. We need to update the light info on server for sneak, don't we? -JA
//	fp32 Amount = 1;

	//if(m_NumSlaveLights.Get(this))
	//	Amount = m_NumActiveSlaveLights.Get(this)/(fp32)m_NumSlaveLights.Get(this);

/*	m_ColorR.Set(this, (int32)(m_Light.k[0]*(0.25f*0xFF)*Amount));
	m_ColorG.Set(this, (int32)(m_Light.k[1]*(0.25f*0xFF)*Amount));
	m_ColorB.Set(this, (int32)(m_Light.k[2]*(0.25f*0xFF)*Amount));
	m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_DATA);*/

	CXR_SceneGraphInstance* pSGI = m_pWServer->World_GetSceneGraphInstance();
	if (pSGI)
	{
		int32 LightID = m_LightIndex.Get(this);
		if (LightID > 0)
		{
			CVec3Dfp32 Intens;
			CMTime Time = PHYSSTATE_TICKS_TO_TIME(m_pWServer->GetGameTick() + _ExtrapolateTick, m_pWServer);
			bool bIsOn = GetIntensServer(Intens, Time, m_pWServer->GetGameTickTime());

			pSGI->SceneGraph_Light_SetIntensity(LightID, Intens, !bIsOn);
		}
	}
}

bool CWObject_Light::GetIntensServer(CVec3Dfp32 &_Intens, const CMTime& _Time, fp32 _TickTime)
{
	if(!m_OnFlag.Get(this))
	{
		_Intens = 0.0f;
		return false;
	}

	// If the light isn't broken, ignore all flicker
	if (m_FlickerType.Get(this) != CLightIntens::EBreak)
	{
		_Intens = CVec3Dfp32(m_ColorR.Get(this), m_ColorG.Get(this), m_ColorB.Get(this)) * (4.0f/255.0f);
		return true;
	}
	else
	{
		return GetIntens(this,_Intens,_Time,_TickTime);
	}
}

//
//
//
bool CWObject_Light::GetIntens(CWObject_CoreData* _pObj, CVec3Dfp32& _Intens, const CMTime& _Time, fp32 _TickTime)
{
	if(!m_OnFlag.Get(_pObj))
	{
		_Intens = 0;
		return false;
	}

	CMTime StartTime = CMTime::CreateFromTicks(m_StartTime.Get(_pObj), _TickTime);
	fp32 AnimateTime = (_Time - StartTime).GetTime();
	fp32 Duration = _pObj->m_iAnim1 * (1.0f / 100.0f);
	
	const fp32 FreqScale = 1.0f / CWO_Light_MessageData::TFreq::Scale;
	const fp32 SpreadScale = 1.0f / 256.0f;
	const fp32 ColorScale = 4.05f / 255.0f;

	CVec3Dfp32 BaseIntens = CVec3Dfp32(m_ColorR.Get(_pObj), m_ColorG.Get(_pObj), m_ColorB.Get(_pObj)) * ColorScale;
	fp32 FlickerFreq = m_FlickerFreq.Get(_pObj) * FreqScale;
	fp32 FlickerSpread = m_FlickerSpread.Get(_pObj) * SpreadScale;
	int FlickerType = m_FlickerType.Get(_pObj);

	if (_pObj->m_iAnim1 && (AnimateTime < Duration))
	{
		fp32 MorphTime = (Duration == 0.0f) ? 0.0f : (AnimateTime / Duration);

		CVec3Dfp32 sColor = CVec3Dfp32(m_LastColorR.Get(_pObj), m_LastColorG.Get(_pObj), m_LastColorB.Get(_pObj)) * ColorScale;
		fp32 sFreq = _pObj->m_iAnim0 * FreqScale;
		fp32 sSpread = m_LastFlickerSpread.Get(_pObj) * SpreadScale;

		BaseIntens = sColor + (BaseIntens - sColor) * MorphTime;
		FlickerFreq = sFreq + (FlickerFreq - sFreq) * MorphTime;
		FlickerSpread = sSpread + (FlickerSpread - sSpread) * MorphTime;
	}
	fp32 FlickerAmount = CLightIntens::GetIntens(AnimateTime, FlickerType, FlickerFreq, FlickerSpread);

	int NumActiveSlaveLights = m_NumActiveSlaveLights.Get(_pObj);
	int NumSlaveLights = m_NumSlaveLights.Get(_pObj);

	if (NumSlaveLights)
	{
		// fade intensity based on num active slave lights

		// NOTE: this special hack-formula should be removed.
		//       ideally, the master light would gather the intensities from its slave lights,
		//       and divide by total number of slave lights. since this is not possible atm,
		//       the visual workaround is to add some flicker to the masterlight itself.
		fp32 Extra = fp32(NumActiveSlaveLights) / fp32(NumSlaveLights);
		fp32 Total = (fp32(NumActiveSlaveLights) + FlickerAmount * Extra) / (fp32(NumSlaveLights) + Extra);
		_Intens = BaseIntens * Total;
	}
	else
	{
		_Intens = BaseIntens * FlickerAmount;
	}

	return true;
	//
//	if(m_FlickerType.Get(_pObj) == CLightIntens::EBreak)
///		M_TRACEALWAYS("%f-%f=%f   %f\n", _Time, StartTime, _Time-StartTime, Amount);
}


bool CWObject_Light::GetActive(CWObject_CoreData* _pObj, const CMTime& _Time, fp32 _TickTime)
{
	if (!m_OnFlag.Get(_pObj))
		return 0;

	const fp32 FreqScale = 1.0f / CWO_Light_MessageData::TFreq::Scale;
	const fp32 SpreadScale = 1.0f / 256.0f;

	CMTime StartTime = CMTime::CreateFromTicks(m_StartTime.Get(_pObj), _TickTime);
	fp32 AnimTime = (_Time - StartTime).GetTime();
	fp32 Duration = _pObj->m_iAnim1 * (1.0f / 100.0f);
	fp32 FlickerFreq = m_FlickerFreq.Get(_pObj) * FreqScale;
	fp32 FlickerSpread = m_FlickerSpread.Get(_pObj) * SpreadScale;
	if (_pObj->m_iAnim1 && (AnimTime < Duration))
	{
		fp32 Amount = AnimTime / Duration;
		fp32 sFreq = _pObj->m_iAnim0 * FreqScale;
		fp32 sSpread = m_LastFlickerSpread.Get(_pObj) * SpreadScale;
		FlickerFreq = sFreq + (FlickerFreq - sFreq) * Amount;
		FlickerSpread = sSpread + (FlickerSpread - sSpread) * Amount;
	}
	return CLightIntens::GetActive(AnimTime, m_FlickerType.Get(_pObj), FlickerFreq, FlickerSpread);
}

//
//
//
void CWObject_Light::OnClientRender(CWObject_Client *_pObj, CWorld_Client *_pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Light_OnClientRender, MAUTOSTRIP_VOID);
	RenderModels(_pObj, _pWClient, _pEngine);
	UpdateFlickerSound(_pObj, _pWClient, _pEngine);
}

void CWObject_Light::UpdateFlickerSound(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	if(!((_pObj->m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT) & LIGHT_NOFLICKERSOUND))
	{
		// play flicker sound
//		fp32 IPTime = _pWClient->GetRenderTickFrac();
		CMTime Time = _pWClient->GetRenderTime();

		int16 LastValue = _pObj->m_ClientData[0];
		_pObj->m_ClientData[0] = GetActive(_pObj, Time, _pWClient->GetGameTickTime());

		if(_pObj->m_ClientData[0] != LastValue)
			_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), _pObj->m_iAnim2, WCLIENT_ATTENUATION_3D);
	}
}

//
// The matrix parameter so this function doesn't have to calculate a new matrix if the calling function allready have done that 
//
void CWObject_Light::RenderModels(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CMat4Dfp32* _pMatrix)
{
	MAUTOSTRIP(CWObject_Light_RenderModels, MAUTOSTRIP_VOID);

	int32 Flags = (_pObj->m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT);
	int32 LightID = m_LightIndex.Get(_pObj);
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMTime Time = _pWClient->GetRenderTime();

	//bool bIsOn = GetIntens(_pObj, Intens, Time, _pWClient->GetGameTickTime());

	// kma: ok, I got this info from mh
	// If LightID is valid, it's a static light, that means that
	// we don't need to interpolate between Matrix and LastMatrix
	CMat4Dfp32 PositionMatrix;

	if(_pMatrix)
		PositionMatrix = *_pMatrix;
	else
	{
		if(LightID)
		{
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), PositionMatrix, IPTime);
		}
		else
			PositionMatrix = _pObj->GetPositionMatrix();
	}

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
	for(int32 i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		// Get model and check so it exist, else, move on
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(!pModel)
			continue;

		if ((i == 0) && (Flags & CXR_LIGHT_FLARE))
		{
			CVec3Dfp32 VCam;
			CVec3Dfp32::GetMatrixRow(_pEngine->GetVC()->m_CameraWMat, 3).Sub(CVec3Dfp32::GetMatrixRow(PositionMatrix, 3), VCam);
			fp32 VCamLen = VCam.Length();

			fp32 s = 1.0f;
			if(Flags & CXR_LIGHT_FLAREDIRECTIONAL)
			{
				s = CVec3Dfp32::GetMatrixRow(PositionMatrix, 0) * VCam;
				if(s < 0.0f)
					continue;
			}
			else
				s = VCamLen;

			fp32 Range = m_FlareRange.Get(_pObj) * 4.0f;
			if (VCamLen > Range)
				continue;

			fp32 FlareWidth = m_FlareWidth.Get(_pObj) * 4.0f;
			fp32 FlareHeight = m_FlareHeight.Get(_pObj) * 4.0f;
//			fp32 SizeFar = m_FlareHeight.Get(_pObj);
			fp32 Width = FlareWidth * 64.0f / Max(1.0f, VCamLen);
			fp32 Height = FlareHeight * 64.0f / Max(1.0f, VCamLen);

			CXR_AnimState Anim;

			Anim.m_iObject = _pObj->m_iObject;
			
			//Should more space be required, squeeze these into one dword
			Anim.m_Anim0 = int16(Width);
			Anim.m_Data[2] = int16(Height);

			Anim.m_Anim1 = int16(Range);		// Fadeout distance.
			Anim.m_Data[1] = m_FlareDepthOffset.Get(_pObj);	// Depth offset.
			s *= 1.0f / VCamLen;
			s = 1.0f - Sqr(1.0f - s);

			s *= fp32(m_LightFlare.Get(_pObj));

			uint32 FlareColor;
			if (Flags & LIGHT_FLAREABS)
			{
				FlareColor = CPixel32::From_fp32(s, s, s, 255);
			}
			else
			{
				CVec3Dfp32 Intens;
				GetIntens(_pObj, Intens, Time, _pWClient->GetGameTickTime());
				FlareColor = CPixel32::From_fp32(Intens[0]*s, Intens[1]*s, Intens[2]*s, 255);
			}

			if ((FlareColor & 0x00ffffff) && (FlareColor & 0xff000000)) // don't render invisible flare
			{
				Anim.m_Data[0] = FlareColor;
				_pEngine->Render_AddModel(pModel, PositionMatrix, Anim);
			}
		}
		else
			_pEngine->Render_AddModel(pModel, PositionMatrix, AnimState);
	}
}

void CWObject_Light::OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	int iRc = m_SurfaceResourceID.Get(_pObj);
	if (iRc)
	{
		CXW_Surface* pSurf = _pWClient->GetMapData()->GetResource_Surface(iRc);
		if (pSurf)
		{
			pSurf = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}
}

#define MACRO_WRITE(_type, _data) {_type TempApa = _data; _pFile->WriteLE(TempApa);}
#define MACRO_READ(_type, _var) {_type TempApa; _pFile->ReadLE(TempApa); _var = TempApa; }

void CWObject_Light::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_Model::OnDeltaLoad(_pFile, _Flags);

	for(int i = 0; i < CWO_NUMDATA; i++)
		_pFile->ReadLE(m_Data[i]);
	_pFile->ReadLE(m_iAnim0);
	_pFile->ReadLE(m_iAnim1);

	m_bActive = m_OnFlag.Get(this) != 0;
	UpdateColor(int(2 * m_pWServer->GetGameTicksPerSecond()));
}

void CWObject_Light::OnDeltaSave(CCFile* _pFile)
{
	CWObject_Model::OnDeltaSave(_pFile);

	for(int i = 0; i < CWO_NUMDATA; i++)
		_pFile->WriteLE(m_Data[i]);
	_pFile->WriteLE(m_iAnim0);
	_pFile->WriteLE(m_iAnim1);
}

// -------------------------------------------------------------------
// Light2
//
// Difference from Light:
// It updates the intensity of a static light
//
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Light2, CWObject_Light, 0x0100);

//
//
//
int32 CWObject_Light2::DoAutoDestroy()
{
	if (m_Flags & CXR_LIGHT_LIGHTFIELDMAP)
		return 1;
	return 0;
}

int32 CWObject_Light2::DoReplicate() { return 1; }

//
//
//
void CWObject_Light2::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Light2_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWObject_Light::OnFinishEvalKeys();
	m_ClientFlags &= ~CWO_CLIENTFLAGS_NOUPDATE;
	m_ClientFlags |= CWO_CLIENTFLAGS_VISIBILITY;
}

//
//
//
void CWObject_Light2::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_Light2::OnClientRenderVis);

	//Just a flare
	if( (_pObj->m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT) & CXR_LIGHT_FLAREONLY ) return;

	// Get parameters
//	int32 Range = m_Range.Get(_pObj);
	int32 ID = m_LightIndex.Get(_pObj);
//	int32 Type = m_LightType.Get(_pObj);
//	int32 Flags = (_pObj->m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT);
//	fp32 SpotWidth = fp32(m_SpotWidth.Get(_pObj)) * (1.0f/256.0f);
//	fp32 SpotHeight = fp32(m_SpotHeight.Get(_pObj)) * (1.0f/256.0f);

	fp32 IPTime = _pWClient->GetRenderTickFrac();

	CVec3Dfp32 Intens;
	int GameTick = _pWClient->GetGameTick();
	fp32 TickTime = _pWClient->GetGameTickTime();
	bool bIsOn = GetIntens(_pObj, Intens, _pWClient->GetRenderTime(), TickTime);

	// Calculate AnimTime
//	CMTime t = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWClient), SERVER_TIMEPERFRAME, IPTime - _pObj->m_CreationGameTickFraction);
//	t += CMTime::CreateFromSeconds(_pObj->m_iObject*13);

	// Get projection map
	int ProjMapID = 0;
	if (bIsOn)
	{
		int iRc = m_SurfaceResourceID.Get(_pObj);
		if (iRc)
		{
			int SurfID = _pWClient->GetMapData()->GetResource_SurfaceID(iRc);
			if (SurfID)
			{
				CMTime SurfTime = CMTime::CreateFromTicks(GameTick - m_StartTime.Get(_pObj), TickTime, IPTime);
				CXW_SurfaceKeyFrame* pSKF = _pEngine->m_pSC->GetSurfaceKey(SurfID, _pEngine, SurfTime, 0);
				ProjMapID = pSKF->m_lTextures[0].m_TextureID;
			}
		}
	}

	CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
	if (pSGI)
	{
		pSGI->SceneGraph_Light_SetIntensity(ID, Intens, !bIsOn);
		if (bIsOn && ProjMapID)
		{
			int iParentObject = _pObj->GetParent();
			if(iParentObject > 0)
			{
				//fp32 Time = 0;
				// get parent matrix
				CMat4Dfp32 ParentMatrix, Matrix;
				CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,(aint)(&ParentMatrix)); 
				if (_pWClient->Phys_Message_SendToObject(RenderMat,iParentObject))
				{
					_pObj->GetLocalPositionMatrix().Multiply(ParentMatrix, Matrix);
				}
				else
				{
					Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), Matrix, IPTime);
				}
				pSGI->SceneGraph_Light_SetProjectionMap(ID, ProjMapID, &Matrix);
			}
			else
			{
				if (_pObj->GetPhysState().m_nPrim > 0)
				{
					CMat4Dfp32 Matrix = _pObj->GetPositionMatrix();
					_pObj->GetPhysState().m_Prim[0].GetOffset().MultiplyMatrix(_pObj->GetPositionMatrix(), CVec3Dfp32::GetRow(Matrix, 3));

					pSGI->SceneGraph_Light_SetProjectionMap(ID, ProjMapID, &Matrix);
				}
				else
					pSGI->SceneGraph_Light_SetProjectionMap(ID, ProjMapID, NULL);
			}
		}
	}
}

void CWObject_Light2::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Light2_OnClientRender, MAUTOSTRIP_VOID);

	// Render Models
	RenderModels(_pObj, _pWClient, _pEngine);

	// do som esound
	UpdateFlickerSound(_pObj, _pWClient, _pEngine);
}

// -------------------------------------------------------------------
// DynamicLight2
//
// Difference from Light:
// It adds an dynamic light to the engine
//
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DynamicLight, CWObject_Light, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DynamicLight2, CWObject_DynamicLight, 0x0100);

//
// Small functions
//
int32 CWObject_DynamicLight::DoAutoDestroy() { return 0; }
int32 CWObject_DynamicLight::DoReplicate() { return 1; }

void CWObject_DynamicLight::OnFinishEvalKeys()
{
	CWObject_Light::OnFinishEvalKeys();

	m_ClientFlags |= CWO_CLIENTFLAGS_VISIBILITY;

	if (!(m_Flags & CXR_LIGHT_FLAREONLY))
		m_bAutoTurnOffRefresh = false;
}

void CWObject_DynamicLight::OnRefresh()
{
	MAUTOSTRIP(CWObject_Light_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_Light::OnRefresh();

	if (m_Flags & CXR_LIGHT_FLAREONLY)
		return;

	CXR_SceneGraphInstance* pSGI = m_pWServer->World_GetSceneGraphInstance();
	if (pSGI)
	{
		int32 Range = m_Range.Get(this);
		int32 ID = m_LightIndex.Get(this);
		int32 Type = m_LightType.Get(this);
		int32 Flags = m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT;

		CVec3Dfp32 Intens;
		bool bIsOn = GetIntens(this, Intens, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());

		if (bIsOn)
		{
			CXR_Light Light(GetPositionMatrix(), Intens, Range, Flags, Type);
			Light.m_iLight = ID;
			Light.m_LightGUID = m_iObject;

			pSGI->SceneGraph_Light_LinkDynamic(Light);
		}
		else
		{
			int iLight = pSGI->SceneGraph_Light_GetIndex(m_iObject);
			if (iLight)
				pSGI->SceneGraph_Light_Unlink(iLight);
		}
	}
}

void CWObject_DynamicLight::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_DynamicLight_OnClientRender, MAUTOSTRIP_VOID);
	UpdateFlickerSound(_pObj, _pWClient, _pEngine);
}

void CWObject_DynamicLight::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_DynamicLight_OnClientRenderVis, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_DynamicLight::OnClientRenderVis);

	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMTime Time = _pWClient->GetRenderTime();
	fp32 TickTime = _pWClient->GetGameTickTime();

	// Get data
	int32 Range = m_Range.Get(_pObj);
	int32 ID = m_LightIndex.Get(_pObj);
	int32 Type = m_LightType.Get(_pObj);
	int32 Flags = _pObj->m_ClientFlags >> CWO_CLIENTFLAGS_USERSHIFT;

	CVec3Dfp32 Intens;
	bool bIsOn = GetIntens(_pObj, Intens, Time, TickTime);

	// Interpolate matrix
	CMat4Dfp32 Matrix;
	int32 iParentObject = _pObj->GetParent();
	if (iParentObject > 0)
	{
		//fp32 Time = 0;
		// get parent matrix
		CMat4Dfp32 ParentMatrix;
        CWObject_Message RenderMat(OBJMSG_HOOK_GETCURRENTMATRIX,(aint)(&ParentMatrix)); 
		aint ok = _pWClient->Phys_Message_SendToObject(RenderMat, iParentObject);

		if (ok)
			_pObj->GetLocalPositionMatrix().Multiply(ParentMatrix, Matrix);
		else
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), Matrix, IPTime);
	}
	else
	{
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), Matrix, IPTime);
	}

	// Dynamic lights can only be points or spot
	if (bIsOn && (Type == CXR_LIGHTTYPE_POINT || Type == CXR_LIGHTTYPE_SPOT) && !(Flags & CXR_LIGHT_FLAREONLY))
	{
		CXR_Light Light(Matrix, Intens, Range, Flags, Type);
		Light.m_iLight = ID;
		Light.m_LightGUID = _pObj->m_iObject;
		Light.m_SpotWidth = fp32(m_SpotWidth.Get(_pObj)) * (1.0f/256.0f);
		Light.m_SpotHeight = fp32(m_SpotHeight.Get(_pObj)) * (1.0f/256.0f);
		{
			int ProjMapID = 0;
			int iRc = m_SurfaceResourceID.Get(_pObj);
			if (iRc)
			{
				int SurfID = _pWClient->GetMapData()->GetResource_SurfaceID(iRc);
				if (SurfID)
				{
					CMTime SurfTime = CMTime::CreateFromTicks(_pWClient->GetGameTick() - m_StartTime.Get(_pObj), TickTime, IPTime);
					CXW_SurfaceKeyFrame* pSKF = _pEngine->m_pSC->GetSurfaceKey(SurfID, _pEngine, SurfTime, 0);
					ProjMapID = pSKF->m_lTextures[0].m_TextureID;

					if (_pObj->GetPhysState().m_nPrim > 0)
					{
						CMat4Dfp32 ProjMatrix = Matrix;
						_pObj->GetPhysState().m_Prim[0].GetOffset().MultiplyMatrix(Matrix, CVec3Dfp32::GetRow(ProjMatrix, 3));

						Light.SetProjectionMap(ProjMapID, &ProjMatrix);
					}
					else
						Light.SetProjectionMap(ProjMapID, NULL);

//					Light.SetProjectionMap(ProjMapID);
				}
			}
		}
		_pEngine->Render_Light_AddDynamic(Light);

		CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
		if (pSGI)
		{
			pSGI->SceneGraph_Light_LinkDynamic(Light);
		}
	}

	// Render models
	RenderModels(_pObj, _pWClient, _pEngine, &Matrix);
}
