#ifndef __WOBJ_LIGHTS_H
#define __WOBJ_LIGHTS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Light game-objects

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Light
					CWObject_Light2
					CWObject_DynamicLight
					CWObject_DynamicLight2
\*____________________________________________________________________________________________*/

#include "../Server/WServer.h"
#include "WObj_System.h"
#include "WObj_LightIntens.h"
#include "WObj_Packer.h"

class CWO_Light_MessageData
{
public:
	typedef TFixed<uint16, 3> TFreq;

	uint8  m_FlickerType;
	uint8  m_FlickerSpread;
	TFreq  m_FlickerFreq;

	CWO_Light_MessageData()
		: m_FlickerType(CLightIntens::ENone), m_FlickerSpread(128), m_FlickerFreq(1.0f) {}
};


//
// Light
//
class CWObject_Light : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	CVec3Dfp32 m_Light;
	fp32 m_Intensity;
	CVec3Dfp32 m_ProjMapOrigin;
	uint16 m_Flags;
	uint16 m_MasterLightID;
	CNameHash m_MasterLightName;

	uint8 m_bActive : 1;
	uint8 m_bAutoTurnOffRefresh : 1;

public:
	enum
	{
		// This flags must not collide with CXR_Light flags, and not exceed 16 bits
		LIGHT_FLAREABS =		CXR_LIGHT_LAST << 0,	// 13
		LIGHT_PROJMAPORIGIN =	CXR_LIGHT_LAST << 1,	// 14
		LIGHT_NOFLICKERSOUND =	CXR_LIGHT_LAST << 2,	// 15

		MSG_OFF =				0,
		MSG_ON =				1,
		MSG_TOGGLE =			2,
		MSG_BREAK =				3,
		MSG_FLICKER =			4,
		MSG_FLICKERFREQ =		5,
		MSG_FLICKERSPREAD =		6,
		MSG_GETFLICKERDATA =	7,

		MSG_SLAVE_OFF =			100,
		MSG_SLAVE_ON =			101,
		MSG_SLAVE_REG =			102,
	};

protected:
	//
	// Declare data packers
	//

	// m_Data[0] (2bit left)
	static CDataPacker<0,0xFF,24> m_ColorR;
	static CDataPacker<0,0xFF,16> m_ColorG;
	static CDataPacker<0,0xFF,8> m_ColorB;
	static CDataPacker<0,0x3,6> m_UNUSED; // YAY \0/
	static CDataPacker<0,0xF,2> m_FlickerType;
	static CDataPacker<0,0x1,1> m_Broken;
	static CDataPacker<0,0x1,0> m_OnFlag;
	
	// m_Data[1] (full)
	static CDataPacker<1,0xFFF,20> m_LightIndex;
	static CDataPacker<1,0x0F,16> m_LightType;
	static CDataPacker<1,0xFF,8> m_FlareDepthOffset;
	static CDataPacker<1,0xFF,0> m_LightFlare;

	// m_Data[2] (full)
	static CDataPacker<2,0xFFFF,16> m_Range;
	static CDataPacker<2,0xFFFF,0> m_SurfaceResourceID;

	// m_Data[3] (full)
	static CDataPacker<3,0xFFFFFFFF,0> m_StartTime;	// TODO: optimize this!

	// m_Data[4] (full)
	static CDataPacker<4,0xFFFF,16> m_SpotHeight;
	static CDataPacker<4,0xFFFF,0> m_SpotWidth;

	// m_Data[5] (full)
	static CDataPacker<5,0xFF,24> m_NumSlaveLights;
	static CDataPacker<5,0xFF,16> m_NumActiveSlaveLights;
	static CDataPacker<5,0xFF,8> m_FlareWidth;
	static CDataPacker<5,0xFF,0> m_FlareHeight;

	// m_Data[6] (full)
	static CDataPacker<6,0xFFF,20> m_FlareRange;	// range/16
	static CDataPacker<6,0xFFF,8> m_FlickerFreq;	// freq*8
	static CDataPacker<6,0xFF,0> m_FlickerSpread;

	// m_Data[7] (full)
	static CDataPacker<7,0xFF,24> m_LastColorR;
	static CDataPacker<7,0xFF,16> m_LastColorG;
	static CDataPacker<7,0xFF,8> m_LastColorB;
	static CDataPacker<7,0xFF,0> m_LastFlickerSpread;


	// m_iAnim0 = Last flicker freq (full)
	// m_iAnim1 = duration (full)
	// m_iAnim2 = light flicker sound (full)

	// Flicker Types
	/*
	enum
	{
		ENone		= 0,
		EPulse		= 1,
		ESine		= 2,
		ERandPulse	= 3,
	};*/

public:
	CWObject_Light();
	virtual int32 DoAutoDestroy();
	virtual int32 DoReplicate();
	
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual void OnRefresh();

	void UpdateColor(int _ExtrapolateTick = 0);
	bool RegisterToMaster();
	void SendSignalToMasterLight(int32 _Signal, int32 _Param=0);

	virtual aint OnMessage(const CWObject_Message& _Msg);

	virtual uint8 GetFlickerType(CWObject_CoreData* _pObj) { return m_FlickerType.Get(_pObj); }
	virtual int8  IsBroken(CWObject_CoreData* _pObj) { return m_Broken.Get(_pObj); }

	//inline static fp32 EvaluatePulse(fp32 _Time, fp32 _Speed);
	//inline static fp32 EvaluateSine(fp32 _Time, fp32 _Speed);

	static void UpdateFlickerSound(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void RenderModels(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, CMat4Dfp32 *_pMatrix=NULL);
	static bool GetIntens(CWObject_CoreData* _pObj, CVec3Dfp32 &_Intens, const CMTime& _Time, fp32 _TickTime);
	bool GetIntensServer(CVec3Dfp32 &_Intens, const CMTime& _Time, fp32 _TickTime);
	static bool GetActive(CWObject_CoreData* _pObj, const CMTime& _Time, fp32 _TickTime);

	static void OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
};

//
// Light2
//
class CWObject_Light2 : public CWObject_Light
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual int32 DoAutoDestroy();
	virtual int32 DoReplicate();
	void OnFinishEvalKeys();
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

//
// Dynamic Light
//
class CWObject_DynamicLight : public CWObject_Light
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual int32 DoAutoDestroy();
	virtual int32 DoReplicate();
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

// For backwards compatibility
class CWObject_DynamicLight2 : public CWObject_DynamicLight
{
	MRTC_DECLARE_SERIAL_WOBJECT;
};

#endif // _INC_WOBJ_LIGHTS
