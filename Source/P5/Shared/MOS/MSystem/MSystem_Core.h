#ifndef __INC_MSYSTEMCORE
#define __INC_MSYSTEMCORE

#include "MSystem.h"

#include "MFloat.h"

#include "../XR/XRSurf.h"
#include "../XR/XRSurfaceContext.h"
#include "../XR/XRVBContext.h"


// -------------------------------------------------------------------
//  CSystemCore
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CSystemCore : public CSystem
{
//	MRTC_DECLARE;
protected:
	char* m_pCmdLine;
	int m_RefreshRecursion;
	bool m_bIsRendering;
	bool m_bReg;
	bool m_bMXRLoaded;
	bool m_bEnvLoaded;
	CStr m_AppClassName;
	spCApplication m_spApp;
	CStr m_EnvFileName;
	CStr m_OptionsFilename;
	CStr m_DefaultSystem;

	TList_Vector<spCDisplayContextDesc> m_lspDC;
	int m_iCurrentDC;

	TList_Vector<CSubSystem*> m_lpSubSystems;

	TPtr<class CXR_SurfaceContext> m_spSurfCtx;
	TPtr<class CXR_VBContext> m_spVBCtx;

public:
	bool m_bMemoryCheck;
	// -------------------------------
/*
	DECLARE_OPERATOR_NEW
*/
	CSystemCore();
	~CSystemCore();
	virtual void Create(char* cmdline, const char* _pAppClassName);
	virtual void Destroy();
protected:
	virtual void CreateSystems();
public:
	virtual void CreateInput();
	virtual void CreateXR();
	virtual void CreateTextureContext(int _TextureIDHeap, int _TextureCacheHeap);
	
	virtual void System_Add(CSubSystem* _pSubSys);
	virtual void System_Remove(CSubSystem* _pSubSys);
	virtual void System_BroadcastMessage(const CSS_Msg& _Msg);
	virtual int System_SendMessage(const char* _pSysName, const CSS_Msg& _Msg);

	virtual aint OnMessage(const CSS_Msg& _Msg);

	virtual void DoModal();

	virtual void WriteEnvironment();

	virtual CStr GetUserFolder() {return "";}
	virtual CStr TranslateSavePath(CStr _Path) {return _Path;}

	virtual void Parser_Quit();
	virtual void Parser_CaptureScreenshot();
	virtual void Parser_CPU();
	virtual void Parser_CPUEnable(int _Feature);
	virtual void Parser_CPUDisable(int _Feature);
	virtual void Parser_SysShowMemory();
	virtual void Parser_MemoryDebug(int _Flags);
	virtual void Parser_TestMemory();
	virtual void Parser_MemReport(int Verbose);
	virtual void Parser_MemoryCheckEnable(int On);
	virtual void Parser_MT_Enable(int enable);
	virtual void Parser_MemShowAllocs(CStr _Str);
	virtual void Parser_MemHideAllocs();
	void Register(CScriptRegisterContext &_RegContext);
	virtual void ExitProcess();

	virtual bool IsRendering() { return m_bIsRendering; };
	virtual void Render(CDisplayContext* _pDC = NULL, int _Context = 0);
	virtual void Refresh();
	virtual void InitCPUFrequencyConstant();

	virtual CApplication* GetApp();
	virtual void DestroyApp();

	virtual CRegistry* GetRegistry();
	virtual CRegistry* GetEnvironment();
	virtual CRegistry* GetOptions(bool _create = false);
	virtual int ReadOptions();				// 0 == Corrupt, 1 == Ok, 2 == NoFile
	virtual int WriteOptions();				// 0 == Failed, 1 == Ok, 2 == Corrupt

	virtual spCDisplayContextContainer CreateDisplayContext(const char* _pName);

	virtual void DC_InitList() pure;
	virtual void DC_Set(int _nr);
	virtual void DC_SetName(const char *_pClassName);
	virtual void DC_SetPtr(spCReferenceCount _spDisplayContext);
	virtual int DC_Find(CStr _Name);
	virtual int DC_GetN();
	virtual CStr DC_GetName(int _nr);
	virtual CStr DC_GetDesc(int _nr);
	virtual void DC_InitVidModeFromEnvironment();
	virtual void DC_InitFromEnvironment();

	virtual CStr GetCaption();
	virtual void SetCaption(CStr _Caption);
};

#endif // __INC_MSYSTEMCORE
