#ifndef WObj_CharWaterEffect_h
#define WObj_CharWaterEffect_h

//--------------------------------------------------------------------------------

#include "Models/CMultiModelInstance.h"

//--------------------------------------------------------------------------------

#define WATEREFFECT_RIPPLE_SOUND	NULL
#define WATEREFFECT_RIPPLE_MODEL0	"Disc"
#define WATEREFFECT_RIPPLE_MODEL1	NULL
#define WATEREFFECT_RIPPLE_MODEL2	NULL

//#define WATEREFFECT_SPLASHCLASS		"WaterSplashMedium"

#define WATEREFFECT_SPLASH_SOUND	"imp_water02b"
#define WATEREFFECT_SPLASH_MODEL0	"Particles:MP=300,SU=dp1a,CO=0x40D0E0F0,TS=6,DU=2,DUN=2,FI=0.1,VE=12,VEN=4,LO=0 0 -7,AX=0 0 -3,OF=10,OFN=2,SZ0=2,SZ1=5,SZ2=2,SZ3=4,RT0=-2,RT1=2,RT2=-2,RT3=2,SPB=0.05,OFB=0.15,PP=4,ES=0.5,LS=99,FL=nh+asq"
#define WATEREFFECT_SPLASH_MODEL1	"Particles:MP=300,SU=dp1a,CO=0x40D0E0F0,TS=6,TO=-0.15,DU=2,DUN=2,FI=0.1,VE=10,VEN=4,LO=0 0 -11,AX=0 0 -2,OF=15,OFN=10,SZ0=2,SZ1=5,SZ2=2,SZ3=4,RT0=-2,RT1=2,RT2=-2,RT3=2,SPB=0.1,OFB=0.22,PP=4,ES=0.5,LS=99,FL=nh+asq"
#define WATEREFFECT_SPLASH_MODEL2	NULL

//--------------------------------------------------------------------------------

#define WATEREFFECT_RIPPLEDURATION 1.5f

#define MAXNUMWATEREFFECTENTRIES 20

//--------------------------------------------------------------------------------

enum
{
	WATEREFFECTTYPE_RIPPLE = 0x01,
	WATEREFFECTTYPE_SPLASH = 0x02,
};

//--------------------------------------------------------------------------------

class CWaterEffectEntry
{
	private:

		uint32	m_PosX;
		uint32	m_PosY;
		uint16	m_PosZ;
		uint8	m_Fwd;
		uint8	m_FadeAndType;
		fp32		m_Time;

	public:

		CWaterEffectEntry()
		{
			m_PosX = 0;
			m_PosY = 0;
			m_PosZ = 0;
			m_Time = 0;
			m_FadeAndType = 0;
		}

		CWaterEffectEntry(CVec3Dfp32& _Pos, CVec3Dfp32& _Fwd, fp32 _Fade, fp32 _Time, uint8 _Type);

		CVec3Dfp32 GetPos();
		CVec3Dfp32 GetFwd();
		fp32 GetFade();
		fp32 GetTime();
		uint8 GetType();

};

//--------------------------------------------------------------------------------

class CWaterEffectManager: public CMultiModelInstance
{
	private:

		int8					m_iEntry;
		int8					m_nEntries;
		fp32						m_EntryDuration;

		CWaterEffectEntry		m_lEntries[MAXNUMWATEREFFECTENTRIES];

	public:

		CStr					m_WaterSplashClass;
		fp32						m_WaterTimer;

	private:

		bool RemapEntryIndex(int& _iEntry);

	public:

		CWaterEffectManager() { Clear(); }
		void Clear();
		void CopyFrom(const CWaterEffectManager& _WEM);

		void SetEntryDuration(fp32 _EntryDuration) { m_EntryDuration = _EntryDuration; }

		void AddEntry(CVec3Dfp32& _Pos, CVec3Dfp32& _Fwd, fp32 _Fade, fp32 _Time, uint8 _Type);
		void Refresh(fp32 _GameTime);

		int8 GetNumEntries() { return m_nEntries; }
		CWaterEffectEntry* GetEntry(int _iEntry);

		uint32 GetNumInstances();
		bool GetInstanceAnimState(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState);
		bool GetInstanceWorldMatrix(uint32 _iInstance, CMat43fp32* _pMatrix);
		bool GetInstance(uint32 _iInstance, fp32 _GameTime, CXR_AnimState* _pAnimState, CMat43fp32* _pMatrix);
};

//--------------------------------------------------------------------------------

#endif /* WObj_CharWaterEffect_h */
