
#ifndef	__MRTC_TASK_H_INCLUDED
#define	__MRTC_TASK_H_INCLUDED

#include "../../Classes/Miscellaneous/MMd5.h"

typedef TPtr<class MRTC_TaskBaseArg> spMRTC_TaskBaseArg;

class SYSTEMDLLEXPORT MRTC_TaskBaseArg : public CReferenceCount
{
};

class SYSTEMDLLEXPORT MRTC_TaskArgRemoteTask : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	void Create();
	void Create(TArray<uint8> _lParam, TArray<TArray<uint8> > _llTaskData, TArray<uint8> _lOutput);
	void Close();

	TArray<uint8> m_lParamData;
	TArray<TArray<uint8> > m_llTaskData;
	TArray<uint8> m_lOutput;

//	CDataFile	m_ParamDataFile;
//	spCCFile	m_spParamFile;

//	CDataFile	m_InputDataFile;
//	spCCFile	m_spInputFile;

//	TArray<spCDataFile>	m_lspInputDataFile;
//	TArray<spCCFile>	m_lspInputFile;

//	CDataFile	m_OutputDataFile;
//	spCCFile	m_spOutputFile;

	class MRTC_TaskManager* m_pMgr;
};


class MRTC_TaskLogger : public CReferenceCount
{
public:
	MRTC_CriticalSection	m_Lock;
	TArray<CStr>	m_lLines;

	void AddLine(CStr _Line);
	TArray<CStr> GetLines();
};

class SYSTEMDLLEXPORT MRTC_TaskBase : public CReferenceCount
{
public:
	virtual bool IsDistributable() {return false;}
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg) pure;
};

class SYSTEMDLLEXPORT MRTC_RemoteTaskBase : public MRTC_TaskBase
{
public:
	typedef void (*OnDistribute)(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg, TArray<uint8>& _lParamChunk, TArray<TArray<uint8> >& _lDataChunks);
	virtual bool IsDistributable() {return true;}
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg) pure;
	virtual bool ProcessResult(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg, TArray<uint8> _lOutput) pure;
};

enum
{
	TASK_STATE_IDLING	= 0,		// Fresh task that is completely untouched
	TASK_STATE_WAITING	= 1,		// Task is waiting on subtasks before it can be processed
	TASK_STATE_RUNNING	= 2,		// Task is in progress
	TASK_STATE_CORRUPT	= 3,		// Errors occured while processing task
	TASK_STATE_FINISHED	= 4,		// Task is finished, kill it

	TASK_RETURN_FINISHED	= 0,
	TASK_RETURN_ERROR		= 1,
	TASK_RETURN_INPROGRESS	= 2,

	REMOTETASK_STATE_FRESH		= 0,
	REMOTETASK_STATE_CORRUPT	= 1,
	REMOTETASK_STATE_RUNNING	= 2,
	REMOTETASK_STATE_FINISHED	= 3,
};

typedef TPtr<class MRTC_TaskInstance>	spMRTC_TaskInstance;

typedef TPtr<class MRTC_RemoteTaskInstance> spMRTC_RemoteTaskInstance;

class MRTC_RemoteTaskInstance : public CReferenceCount
{
public:
	MRTC_RemoteTaskInstance() : m_State(REMOTETASK_STATE_FRESH), m_iPacketPos(0), m_pTask(0), m_pTaskArg(0), m_pTaskClass(0), m_pfnOnDistribute(0), m_bOnDistributeDone(false) {}

	void Create(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pTaskArg, MRTC_RemoteTaskBase* _pTaskClass, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int32 _Flags)
	{
//		CMD5 Md5;
//		Md5.f_AddData(_lData.GetBasePtr(), _lData.Len());
//		CMD5Digest md5(Md5);
		Create(_TaskName, _lParam, _lData, CMD5Digest(_lData.GetBasePtr(), _lData.Len()), _pTask, _pTaskArg, _pTaskClass, _pfnOnDistribute, _RamSize, _Flags);
	}

	void Create(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, CMD5Digest _Checksum, class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pTaskArg, MRTC_RemoteTaskBase* _pTaskClass, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int32 _Flags)
	{
		m_State	= REMOTETASK_STATE_FRESH;
		m_iPacketPos	= 0;
		m_TaskName	= _TaskName;
		m_lParamData	= _lParam;
		m_lPacketChecksum.Add(_Checksum);
		m_llPacketData.Add(_lData);

		m_pTask	= _pTask;
		m_pTaskArg	= _pTaskArg;
		m_pTaskClass	= _pTaskClass;
		m_nFailures = 0;
		m_pfnOnDistribute = _pfnOnDistribute;

		m_RamSize	= _RamSize;
		m_Flags		= _Flags;
	}
	void Create(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _llData, class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pTaskArg, MRTC_RemoteTaskBase* _pTaskClass, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int32 _Flags)
	{
		TArray<CMD5Digest> lPacketChecksum;
		for(int i = 0; i < _llData.Len(); i++)
		{
			TArray<uint8> lData = _llData[i];
//			CMD5 Md5;
//			Md5.f_AddData(lData.GetBasePtr(), lData.Len());
//			CMD5Digest md5(Md5);
//			lPacketChecksum.Add(md5);
			lPacketChecksum.Add(CMD5Digest(lData.GetBasePtr(), lData.Len()));
		}
		Create(_TaskName, _lParam, _llData, lPacketChecksum, _pTask, _pTaskArg, _pTaskClass, _pfnOnDistribute, _RamSize, _Flags);
	}
	void Create(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _llData, TArray<CMD5Digest> _lChecksums, class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pTaskArg, MRTC_RemoteTaskBase* _pTaskClass, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int32 _Flags)
	{
		m_State	= REMOTETASK_STATE_FRESH;
		m_iPacketPos	= 0;
		m_TaskName	= _TaskName;
		m_lParamData	= _lParam;
		m_llPacketData = _llData;
		m_lPacketChecksum = _lChecksums;

		m_pTask	= _pTask;
		m_pTaskArg	= _pTaskArg;
		m_pTaskClass	= _pTaskClass;
		m_nFailures = 0;
		m_pfnOnDistribute = _pfnOnDistribute;

		m_RamSize	= _RamSize;
		m_Flags		= _Flags;
	}
	MRTC_CriticalSection m_Lock;

	uint64 m_RamSize;
	int32 m_Flags;

	int m_nFailures;
	int	 m_State;
	CStr m_TaskName;
	TArray<TArray<uint8> > m_llPacketData;
	TArray<CMD5Digest> m_lPacketChecksum;
	TArray<uint8> m_lParamData;

	TArray<uint8> m_lResultData;
	int m_iPacketPos;

	MRTC_TaskInstance* m_pTask;
	MRTC_TaskBaseArg* m_pTaskArg;
	MRTC_RemoteTaskBase* m_pTaskClass;
	MRTC_RemoteTaskBase::OnDistribute m_pfnOnDistribute;
	bool m_bOnDistributeDone;

	bool IsFinished();
	bool IsCorrupt();
	bool IsRunning();

	int ProcessResult();
};

class MRTC_TaskInstance : public CReferenceCount
{
public:
	MRTC_TaskInstance(CStr _Name, spMRTC_TaskBaseArg _spArg)
	{
		m_State	= TASK_STATE_IDLING;
		m_TaskName	= _Name;
		m_spTaskArg	= _spArg;
		m_nFailedSubTasks	= 0;
		m_pManager	= 0;
	}

	int	m_State;
	MRTC_CriticalSection	m_Lock;
	CStr	m_TaskName;				// Runtime class to spawn
	spMRTC_TaskBaseArg	m_spTaskArg;
	TPtr<MRTC_TaskLogger>	m_spLog;
	TArray<spMRTC_TaskInstance>	m_lspDependsOn;
	TArray<spMRTC_RemoteTaskInstance> m_lspRemoteDepend;
	class MRTC_TaskManager* m_pManager;

	TArray<spMRTC_TaskInstance>	m_lspSubTasks;
	int	m_nFailedSubTasks;

	static spMRTC_TaskInstance CreateTaskInstance(CStr _Name, spMRTC_TaskBaseArg _spArg)
	{
		return MNew2(MRTC_TaskInstance, _Name, _spArg);
	}

	bool IsCorrupt()
	{
		M_LOCK(m_Lock);
		return (m_State == TASK_STATE_CORRUPT);
	}

	bool IsFinished()
	{
		M_LOCK(m_Lock);
		return (m_State == TASK_STATE_FINISHED) || (m_State == TASK_STATE_CORRUPT);
	}

	void AddTask(spMRTC_TaskInstance _spTask)
	{
		M_LOCK(m_Lock);
		// Inherit logger
		_spTask->m_spLog	= m_spLog;
		_spTask->m_pManager	= m_pManager;
		m_lspSubTasks.Add(_spTask);
	}

	void DependOn(spMRTC_TaskInstance _spTask)
	{
		M_LOCK(m_Lock);
		m_lspDependsOn.Add(_spTask);
	}

	void DependOn(spMRTC_RemoteTaskInstance _spTask)
	{
		M_LOCK(m_Lock);
		m_lspRemoteDepend.Add(_spTask);
	}

	bool CheckDependencies()
	{
		M_LOCK(m_Lock);
		for(int i = 0; i < m_lspDependsOn.Len(); i++)
		{
			if(m_lspDependsOn[i]->IsCorrupt())
			{
				m_State = TASK_STATE_CORRUPT;
				return false;
			}
			else if(!m_lspDependsOn[i]->IsFinished())
				return false;
		}

		for(int i = 0; i < m_lspRemoteDepend.Len(); i++)
		{
			if(m_lspRemoteDepend[i]->IsCorrupt())
			{
				m_State = TASK_STATE_CORRUPT;
				return false;
			}
			else if(!m_lspRemoteDepend[i]->IsFinished())
				return false;
		}

		return true;
	}

	void Process();
};

class TaskPacket : public CReferenceCount
{
public:
	TaskPacket()
	{
		Clear();
	}
	uint32 m_Type;
	uint32 m_Size;
	uint32	m_iPacketPos;
	uint32	m_State;
	TArray<uint8> m_lData;

	void Clear()
	{
		m_Type	= 0;
		m_Size	= 0;
		m_lData.Destroy();
		m_iPacketPos	= 0;
		m_State	= 0;
	}

	TaskPacket& operator = (const TaskPacket& _Other)
	{
		m_Type	= _Other.m_Type;
		m_Size	= _Other.m_Size;
		m_iPacketPos	= _Other.m_iPacketPos;
		m_lData	= _Other.m_lData;
		m_State	= _Other.m_State;

		return *this;
	}
};


enum
{
	TASKCLIENT_STATE_NEWCONNECTION	= 0,
	TASKCLIENT_STATE_UPLOADING		= 1,
	TASKCLIENT_STATE_COMPLETE		= 2,
	TASKCLIENT_STATE_FINISHED		= 3,
	TASKCLIENT_STATE_CORRUPT		= 4,

	TASKAGENT_STATE_NEWCONNECTION	= 0,
	TASKAGENT_STATE_UPLOADING		= 1,
	TASKAGENT_STATE_COMPLETE		= 2,
	TASKAGENT_STATE_FINISHED		= 3,
	TASKAGENT_STATE_CORRUPT		= 4,

};

class MRTC_TaskManLog : public ILogFile
{
	spCReferenceCount m_spOldLog;
	ILogFile* m_pOldLog;
	class MRTC_TaskManager*	m_pMgr;
	CStr m_OldLog;
	void *m_MainThread;

public:
	MRTC_TaskManLog(class MRTC_TaskManager* _pMgr, const char *_pOldLog);
	~MRTC_TaskManLog();

	virtual void Create(CStr _FileName, bool _bAppend = false);
	virtual void Log(const CStr& _s);
	virtual void Log(const char* _pStr);
	virtual void SetFileName(const CStr& _s, bool _bAppend = false);
};

class SYSTEMDLLEXPORT MRTC_TaskManager : public CReferenceCount
{
	friend class MRTC_TaskManagerAnsweringMachine;
	friend class MRTC_RemoteTaskReturn;
protected:
	MRTC_CriticalSection	m_Lock;
	TArray<spMRTC_TaskInstance>	m_lspTasks;
	TArray<spMRTC_RemoteTaskInstance>	m_lspRemoteTasks;
	TArray<TPtr<class MRTC_TaskClient> > m_lspPotentialClients;
	TArray<TPtr<class MRTC_TaskClient> > m_lspActiveClients;

	MRTC_CriticalSection m_PotenialClientLock;

	TArray<TPtr<class MRTC_TaskAgent> > m_lspPendingAgents;
	TArray<TPtr<class MRTC_TaskAgent> > m_lspActiveAgents;
	MRTC_CriticalSection m_PendingAgentLock;

	TPtr<MRTC_TaskManLog>	m_spLog;
	TPtr<MRTC_TaskManLog>	m_spErrorLog;

	TPtr<MRTC_TaskLogger>	m_spLogger;

	class MRTC_TaskManagerAnsweringMachine* m_pAM;

	bool m_bNetInitiated;
	int32 m_nActiveWorkingThreads;
	int32 m_nMaxWorkingThreads;

	void AddManager(spMRTC_TaskInstance _spTask, MRTC_TaskManager* _pMgr)
	{
		_spTask->m_pManager	= _pMgr;
		for(int i = 0; i < _spTask->m_lspSubTasks.Len(); i++)
			AddManager(_spTask->m_lspSubTasks[i], _pMgr);
	}

	TArray<uint8>	m_lExeFile;
	void Host_SpawnClients();
	void Host_AnswerCalls();

	TArray<spMRTC_RemoteTaskInstance>	m_lspPendingRemoteTasks;

	int m_CoordinatorSocket;		// Connection to coordinator
	int m_HostSocket;				// Host port (xwc connects to this)
	int m_AgentSocket;				// Agent port (used to deliver exe files to agents)

	int	m_ClientSocket;
	spMRTC_TaskInstance m_spClientTask;	// This is a pointer to a task that is currently being recieved (temporary)
	CStr m_ClientCacheFilename;
	//spCCFile m_spFileLock;
	void* m_hFileLock;

	void AcquireFileLock(CStr _Name);
	void ReleaseFileLock();

	bool m_bCorrupt;
	bool m_bThrowExceptionOnCorrupt;

	CMTime m_LastRemoteUpdate;

	bool IsNewClient(int _Address);

	int m_ClientPacketState;
	TaskPacket m_ClientPartialPacket;

	TPtr<class TaskPacket> Client_ReadPacket();

	TPtr<class MRTC_TaskClient> Host_GetFreeClient();
	void Host_FetchPackets();
	TPtr<class TaskPacket> Host_FetchPacket(TPtr<class MRTC_TaskClient> _spClient);
	void Host_SendPacket(TPtr<class MRTC_TaskClient> _spClient, int32 _Type, void* _pData, int32 _Size, int32* _pParam = NULL, int32 _nParams = 0);
	void Host_SendPacket(TPtr<class MRTC_TaskClient> _spClient, int32 _Type, TArray<uint8>& _lData, int32* _pParams = NULL, int32 _nParams = 0);
	void Host_SendData(TPtr<class MRTC_TaskClient> _spClient, void* _pData, int32 _Size);

	void Coord_SendPacket(int _Type, void* _pData, int _Size);
	void Coord_SendData(void* _pData, int _Size);
	
	void Client_SendPacket(int _Type, void* _pData, int _Size);
	void Client_SendData(void* _pData, int _Size);

public:
	MRTC_TaskManager(bool _bIgnoreLogOverride = false);
	~MRTC_TaskManager();

	bool NetworkEnabled();

	void Process();
	void Host_ProcessRemote();

	int Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute = NULL, uint64 _RamSize = 0, int _Flags = 0);
	int Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, CMD5Digest _Checksum, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute = NULL, uint64 _RamSize = 0, int _Flags = 0);
	int Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _lData, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute = NULL, uint64 _RamSize = 0, int _Flags = 0);
	int Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _lData, TArray<CMD5Digest> _lChecksum, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute = NULL, uint64 _RamSize = 0, int _Flags = 0);

	int AddPendingRemoteTask(spMRTC_RemoteTaskInstance _spTask)
	{
		M_LOCK(m_Lock);
		m_lspPendingRemoteTasks.Add(_spTask);
		return m_lspPendingRemoteTasks.Len();
	}

	int AddTask(spMRTC_RemoteTaskInstance _spTask)
	{
		M_LOCK(m_Lock);
		m_lspRemoteTasks.Add(_spTask);
		AddPendingRemoteTask(_spTask);
		return m_lspRemoteTasks.Len();
	}

	int AddTask(spMRTC_TaskInstance _spTask)
	{
		M_LOCK(m_Lock);

		AddManager(_spTask, this);
		m_lspTasks.Add(_spTask);
		return m_lspTasks.Len();
	}

	void EnableExceptionOnCorrupt(bool _bException) {m_bThrowExceptionOnCorrupt = _bException;}
	void LogLine(CStr _Line);

	void Host_Start();
	void Host_UpdateNet();
	void Host_Stop();
	void Host_BlockUntilDone();
	void Host_CloseAllClients();
	void Host_BlockOnNumTasks(int _NumTasks);
	void Host_Flush();

	bool Client_Start(int _Address, int _Port);
	void Client_UpdateNet();
	void Client_Stop();
	bool Client_IsAlive();
	void Client_Log(CStr _Msg);

	void SetMaxWorkingThreads(int _Count);
	int GetMaxWorkingThreads();
	int GetActiveWorkingThreads();
	void IncrementActiveWorkingThreads(int32 _Count);
};


#endif	// __MRTC_TASK_H_INCLUDED
