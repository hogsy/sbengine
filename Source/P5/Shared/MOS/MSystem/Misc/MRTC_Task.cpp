
#include "PCH.h"
#include "MRTC_Task.h"

#ifdef PLATFORM_WIN_PC
#include "winsock2.h"
#include "ws2tcpip.h"
#elif defined (PLATFORM_XENON)
#include "xtl.h"
#endif

//#pragma optimize("",off)
//#pragma inline_depth(0)


enum
{
	PACKET_STATE_NEWPACKET	= 0,
	PACKET_STATE_PACKETSIZE	= 1,
	PACKET_STATE_PACKETDATA	= 2,

	PACKET_TYPE_LOG			= 0,
	PACKET_TYPE_PROGRESS	= 1,
	PACKET_TYPE_TASKNAME	= 2,
	PACKET_TYPE_TASKPARAM	= 3,
	PACKET_TYPE_TASKDATA	= 4,
	PACKET_TYPE_TASKEXECUTE	= 5,
	PACKET_TYPE_RESULT		= 6,
	PACKET_TYPE_REQUESTDATA	= 7,
	PACKET_TYPE_DATACHECKSUM	= 8,
};

static CStr CMTimeToStr(CMTime _Time)
{
	int Days = _Time.GetNumTicks(1.0f / (24.0f*60.0f*60.0f));
	if(Days > 0)
		_Time -= CMTime::CreateFromTicks(Days, (24.0f * 60.0f * 60.0f));
	int Hours = _Time.GetNumTicks(1.0f / (60.0f * 60.0f));
	if(Hours > 0)
		_Time -= CMTime::CreateFromTicks(Hours, (60.0f * 60.0f));
	int Minutes = _Time.GetNumTicks(1.0f / 60.0f);
	if(Minutes > 0)
		_Time -= CMTime::CreateFromTicks(Minutes, 60.0f);

	bool bAddAll = false;
	CStr Tid;
	if(Days > 0)
		Tid = CStrF("%dd", Days), bAddAll = true;
	if(Hours > 0 || bAddAll)
		Tid += CStrF("%s%dh", bAddAll?" ":"", Hours), bAddAll = true;
	if(Minutes > 0 || bAddAll)
		Tid += CStrF("%s%dm", bAddAll?" ":"", Minutes), bAddAll = true;

	Tid += CStrF("%s%.2fs", bAddAll?" ":"", _Time.GetTime());

	return Tid;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskAgent
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MRTC_TaskAgent_WorkerThread : public MRTC_Thread_Core
{
public:
	class MRTC_TaskAgent* m_pAgent;
	MRTC_TaskAgent_WorkerThread() : m_pAgent(NULL) {}
	void Create(class MRTC_TaskAgent* _pAgent)
	{
		m_pAgent = _pAgent;
	}
	virtual int Thread_Main();
};

class MRTC_TaskAgent : public CReferenceCount
{
public:
	MRTC_TaskAgent();
	int m_Socket;
	int	m_Host;
	TArray<uint8>	m_lExeFile;
	int	m_iPacketPos;
	int	m_State;
	CStr m_HostName;
	CMTime	m_RetryTime;
	int m_nRetryCount;

	TaskPacket	m_PartialPacket;

	MRTC_TaskAgent_WorkerThread m_WorkerThread;

	bool IsFinished()
	{
		return ((m_State == TASKAGENT_STATE_FINISHED) || (m_State == TASKAGENT_STATE_CORRUPT));
	}

	bool IsCorrupt()
	{
		return (m_State == TASKAGENT_STATE_CORRUPT);
	}

	void Update();
};

typedef TPtr<MRTC_TaskAgent> spMRTC_TaskAgent;


int MRTC_TaskAgent_WorkerThread::Thread_Main()
{
	while(!m_pAgent->IsFinished())
	{
		m_pAgent->Update();
		MRTC_SystemInfo::OS_Sleep(10);
	}

	return 0;
}


class MRTC_TaskClient : public CReferenceCount
{
public:
	MRTC_TaskClient();
	int m_Socket;
	int	m_Host;
	int	m_State;
	CStr m_HostName;
	CMTime m_StartTime;
	CMTime m_TimeOfLastPacket;

	TaskPacket	m_PartialPacket;

	spMRTC_RemoteTaskInstance	m_spTask;

	void Close();
	void Update();
};

typedef TPtr<MRTC_TaskClient> spMRTC_TaskClient;


class MRTC_TaskManagerAnsweringMachine : public MRTC_Thread_Core
{
protected:
	MRTC_TaskManager* m_pTaskManager;
public:
	MRTC_TaskManagerAnsweringMachine(MRTC_TaskManager* _pMan) : m_pTaskManager(_pMan) {}

	virtual int Thread_Main()
	{
		while(!Thread_IsTerminating())
		{
#ifndef PLATFORM_CONSOLE
			int Connection = accept(m_pTaskManager->m_HostSocket, NULL, 0);
			while(Connection != INVALID_SOCKET)
			{
				do
				{
					uint32 NonBlocking = 1;
					if(ioctlsocket(Connection, FIONBIO, &NonBlocking))
					{
						LogFile("Host: Failed to set non-blocking on host connection");
						closesocket(Connection);
						break;
					}

					int KeepAlive = 1;
					if(setsockopt(Connection, SOL_SOCKET, SO_KEEPALIVE, (const char*)&KeepAlive, sizeof(KeepAlive)))
					{
						LogFile("Host: Failed to set keepalive on host connection");
						closesocket(Connection);
						break;
					}

					sockaddr_in sock;
					int size = sizeof(sock);
					getpeername(Connection, (sockaddr*)&sock, &size);

					spMRTC_TaskClient spClient = MNew(MRTC_TaskClient);
					spClient->m_Socket	= Connection;
					spClient->m_Host	= sock.sin_addr.S_un.S_addr;
					spClient->m_State	= TASKCLIENT_STATE_NEWCONNECTION;

//					char aHost[256];
//					if(getnameinfo((sockaddr*)&sock, sizeof(sock), aHost, 256, NULL, 0, 0) == 0)
//						spClient->m_HostName	= aHost;
//					else
						spClient->m_HostName	= CStrF("%d.%d.%d.%d", (spClient->m_Host & 0xff), ((spClient->m_Host>>8) & 0xff), ((spClient->m_Host>>16) & 0xff), ((spClient->m_Host>>24) & 0xff));

					{
						M_LOCK(m_pTaskManager->m_PotenialClientLock);
						m_pTaskManager->m_lspPotentialClients.Add(spClient);
					}
				} while(0);

				Connection = accept(m_pTaskManager->m_HostSocket, NULL, 0);
			}

			Connection = accept(m_pTaskManager->m_AgentSocket, NULL, 0);
			while(Connection != INVALID_SOCKET)
			{
				do
				{
					uint32 NonBlocking = 1;
					if(ioctlsocket(Connection, FIONBIO, &NonBlocking))
					{
						LogFile("Host: Failed to set non-blocking on agent connection");
						closesocket(Connection);
						break;
					}

					int KeepAlive = 1;
					if(setsockopt(Connection, SOL_SOCKET, SO_KEEPALIVE, (const char*)&KeepAlive, sizeof(KeepAlive)))
					{
						LogFile("Host: Failed to set keepalive on host connection");
						closesocket(Connection);
						break;
					}

					sockaddr_in sock;
					int size = sizeof(sock);
					getpeername(Connection, (sockaddr*)&sock, &size);


					spMRTC_TaskAgent spAgent = MNew(MRTC_TaskAgent);
					spAgent->m_lExeFile	= m_pTaskManager->m_lExeFile;
					spAgent->m_Socket	= Connection;
					spAgent->m_Host	= sock.sin_addr.S_un.S_addr;
					spAgent->m_State	= TASKCLIENT_STATE_NEWCONNECTION;

//					char aHost[256];
//					if(getnameinfo((sockaddr*)&sock, sizeof(sock), aHost, 256, NULL, 0, 0) == 0)
//						spAgent->m_HostName	= aHost;
//					else
						spAgent->m_HostName	= CStrF("%d.%d.%d.%d", (spAgent->m_Host & 0xff), ((spAgent->m_Host>>8) & 0xff), ((spAgent->m_Host>>16) & 0xff), ((spAgent->m_Host>>24) & 0xff));

					spAgent->m_WorkerThread.Create(spAgent);
					spAgent->m_WorkerThread.Thread_Create();
					{
						M_LOCK(m_pTaskManager->m_PendingAgentLock);
						m_pTaskManager->m_lspPendingAgents.Add(spAgent);
					}
				} while(0);

				Connection = accept(m_pTaskManager->m_AgentSocket, NULL, 0);
			}
#endif
			m_QuitEvent.WaitTimeout(0.001);
		}

		return 0;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskWorkerThread
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MRTC_TaskWorkerThread : public MRTC_Thread_Core
{
public:
	NThread::CEventAutoResetReportable	m_Event;
	MRTC_TaskInstance* m_pTask;
	class MRTC_TaskThreadContainer* m_pContainer;
	DLinkDS_Link(MRTC_TaskWorkerThread, m_Link);
	virtual int Thread_Main();

	MRTC_TaskWorkerThread() : MRTC_Thread_Core(), m_pTask(0), m_pContainer(0) {}
};

class MRTC_TaskThreadContainer : public CReferenceCount
{
	friend class MRTC_TaskWorkerThread;
protected:
	TArray<MRTC_TaskWorkerThread>	m_lThreads;
	DLinkDS_List(MRTC_TaskWorkerThread, m_Link) m_AvailableThreads;
	MRTC_CriticalSection	m_Lock;

	NThread::CEventAutoReset	m_AvailableEvent;
public:
	MRTC_TaskThreadContainer() {}
	void Create(int _nThreads)
	{
		if(m_lThreads.Len() != 0)
			return;

		M_LOCK(m_Lock);
		m_lThreads.SetLen(_nThreads);
		for(int i = 0; i < _nThreads; i++)
		{
			m_AvailableThreads.Push(&m_lThreads[i]);
			m_lThreads[i].m_pContainer = this;
			m_lThreads[i].Thread_Create();
		}
	}

	MRTC_TaskWorkerThread* GetAvailThread()
	{
		while(1)
		{
			MRTC_TaskWorkerThread* pThread;

			{
				M_LOCK(m_Lock);
				pThread = m_AvailableThreads.Pop();
			}

			if(pThread)
				return pThread;

			m_AvailableEvent.Wait();
		}

		return NULL;
	}
};

int MRTC_TaskWorkerThread::Thread_Main()
{
	m_QuitEvent.ReportTo(&m_Event);

	while(!Thread_IsTerminating())
	{
		m_Event.Wait();
		if(m_pTask)
		{
			MRTC_TaskInstance* pTask = m_pTask;
			MRTC_SystemInfo::Thread_SetName(CStr("MRTC Task worker ") + pTask->m_TaskName);
			MRTC_TaskManager* pManager = pTask->m_pManager;
			m_pTask	= NULL;
			{
				TPtr<MRTC_TaskBase> spClass = TDynamicCast<MRTC_TaskBase>(MRTC_GetObjectManager()->CreateObject(pTask->m_TaskName));
				if(spClass == 0) Error_static("MRTC_TaskWorkerThread::Thread_Main", CStrF("Unable to create task class '%s'", pTask->m_TaskName));
				int nRet = spClass->Process(pTask, pTask->m_spTaskArg);
				M_LOCK(pTask->m_Lock);
				switch(nRet)
				{
				case TASK_RETURN_ERROR:
					{
						pTask->m_State	= TASK_STATE_CORRUPT;
						break;
					}

				case TASK_RETURN_FINISHED:
					{
						pTask->m_State	= TASK_STATE_FINISHED;
						break;
					}

				case TASK_RETURN_INPROGRESS:
					{
						pTask->m_State	= TASK_STATE_WAITING;
						break;
					}
				}
			}

			{
				M_LOCK(m_pContainer->m_Lock);
				m_pContainer->m_AvailableThreads.Push(this);
				m_pContainer->m_AvailableEvent.Signal();
			}
		}
	}

	return 0;
}

MRTC_TaskThreadContainer ThreadContainer;

void SpawnTask(MRTC_TaskManager* _pManager, MRTC_TaskInstance* _pTask, bool _bSync = false)
{
	// Spawn a thread to do the work
	//NOTE: This is broken when using distributed tasks
	if(_pManager->GetMaxWorkingThreads() && !_bSync)
	{
/*		if(_pManager->GetActiveWorkingThreads() < _pManager->GetMaxWorkingThreads())
		{
			_pManager->IncrementActiveWorkingThreads(1);
			_pTask->m_State	= TASK_STATE_RUNNING;
			MRTC_TaskWorkerThread* pThread = DNew(MRTC_TaskWorkerThread) MRTC_TaskWorkerThread;
			pThread->m_pTask	= _pTask;
			pThread->Thread_Create((void*)_pTask);
		}
*/
		MRTC_TaskWorkerThread* pThread = ThreadContainer.GetAvailThread();
		_pTask->m_State	= TASK_STATE_RUNNING;
		pThread->m_pTask	= _pTask;
		pThread->m_Event.Signal();
	}
	else
	{
		MRTC_TaskBase* pClass = TDynamicCast<MRTC_TaskBase>(MRTC_GetObjectManager()->CreateObject(_pTask->m_TaskName));
		if(!pClass) Error_static("SpawnTask", CStrF("Unable to create task class '%s'", _pTask->m_TaskName));
		int nRet = pClass->Process(_pTask, _pTask->m_spTaskArg);
		switch(nRet)
		{
		case TASK_RETURN_ERROR:
			{
				_pTask->m_State	= TASK_STATE_CORRUPT;
				break;
			}

		case TASK_RETURN_FINISHED:
			{
				_pTask->m_State	= TASK_STATE_FINISHED;
				break;
			}

		case TASK_RETURN_INPROGRESS:
			{
				_pTask->m_State	= TASK_STATE_WAITING;
				break;
			}
		}
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskLogger
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_TaskLogger::AddLine(CStr _Line)
{
	M_LOCK(m_Lock);
	m_lLines.Add(_Line);
}

TArray<CStr> MRTC_TaskLogger::GetLines()
{
	M_LOCK(m_Lock);
	TArray<CStr> lLines = m_lLines;
	m_lLines.Destroy();

	return lLines;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskInstance
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_TaskInstance::Process()
{
	if(CheckDependencies())
	{
		if(m_lspSubTasks.Len() > 0)
		{
			// This task has subtasks, process them before processing this task
			M_LOCK(m_Lock);
			for(int i = 0; i < m_lspSubTasks.Len(); i++)
			{
				if(m_nFailedSubTasks > 0 )
					break;
				if(m_lspSubTasks[i]) m_lspSubTasks[i]->Process();
			}

			for(int i = m_lspSubTasks.Len() - 1; i >= 0; i--)
			{
				MRTC_TaskInstance* pTask = m_lspSubTasks[i];
				switch(pTask->m_State)
				{
				case TASK_STATE_FINISHED:
					{
						// Finished task, remove it
						m_lspSubTasks[i]	= 0;
						break;
					}

				case TASK_STATE_CORRUPT:
					{
						// Corrupt task, increment errors and remove it
						m_nFailedSubTasks++;
						m_lspSubTasks[i]	= 0;
						break;
					}
				}
				if(m_lspSubTasks[i] == 0 ) m_lspSubTasks.Del(i);
			}

			if(m_nFailedSubTasks > 0)
				m_State	= TASK_STATE_CORRUPT;
		}

		if(m_lspSubTasks.Len() == 0)
		{
			// No subtasks left, process this task
			switch(m_State)
			{
			case TASK_STATE_IDLING:
				{
					// Run the task
					SpawnTask(m_pManager, this);
					break;
				}

			case TASK_STATE_WAITING:
				{
					if(m_nFailedSubTasks > 0)
						m_State	= TASK_STATE_CORRUPT;
					else
						m_State	= TASK_STATE_FINISHED;
					break;
				}

			case TASK_STATE_CORRUPT:
				{
					break;
				}

			case TASK_STATE_FINISHED:
				{
					break;
				}
			}
		}
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskGroup
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MRTC_TaskGroup : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		if(_pTask->m_lspSubTasks.Len() > 0 )
			return TASK_RETURN_INPROGRESS;
		else
			return TASK_RETURN_FINISHED;
	}
};

void MRTC_TaskArgRemoteTask::Create()
{
}

void MRTC_TaskArgRemoteTask::Create(TArray<uint8> _lParamData, TArray<TArray<uint8> > _llTaskData, TArray<uint8> _lOutput)
{
	m_lParamData = _lParamData;
	m_llTaskData = _llTaskData;
	m_lOutput	= _lOutput;
}

void MRTC_TaskArgRemoteTask::Close()
{
}

class MRTC_RemoteTaskReturn : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		MRTC_TaskArgRemoteTask* pRemoteArg = TDynamicCast<MRTC_TaskArgRemoteTask>(_pArg);
		if(!pRemoteArg)
		{
			int State = TASK_RETURN_ERROR;
			_pTask->m_pManager->Client_SendPacket(PACKET_TYPE_RESULT, &State, sizeof(State));
			return TASK_RETURN_ERROR;
		}

		pRemoteArg->Close();

		// Say that we're done
		if(pRemoteArg->m_lOutput.Len() > 0)
		{
			int State = TASK_RETURN_FINISHED;
			TArray<uint8> lPacket;
			lPacket.SetLen(4 + pRemoteArg->m_lOutput.Len());
			memcpy(lPacket.GetBasePtr(), &State, 4);
			memcpy(lPacket.GetBasePtr() + 4, pRemoteArg->m_lOutput.GetBasePtr(), pRemoteArg->m_lOutput.Len());
			_pTask->m_pManager->Client_SendPacket(PACKET_TYPE_RESULT, lPacket.GetBasePtr(), lPacket.Len());
		}
		else
		{
			int State = TASK_RETURN_ERROR;
			_pTask->m_pManager->Client_SendPacket(PACKET_TYPE_RESULT, &State, sizeof(State));
		}

		return TASK_RETURN_FINISHED;
	}
};

MRTC_IMPLEMENT(MRTC_TaskArgRemoteTask, MRTC_TaskBaseArg);
MRTC_IMPLEMENT_DYNAMIC(MRTC_TaskGroup, MRTC_TaskBaseArg);
MRTC_IMPLEMENT_DYNAMIC(MRTC_RemoteTaskReturn, MRTC_TaskBase);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_RemoteTaskInstance
|__________________________________________________________________________________________________
\*************************************************************************************************/

bool MRTC_RemoteTaskInstance::IsFinished()
{
	return ((m_State == REMOTETASK_STATE_FINISHED) || (m_State == REMOTETASK_STATE_CORRUPT));
}

bool MRTC_RemoteTaskInstance::IsCorrupt()
{
	return (m_State == REMOTETASK_STATE_CORRUPT);
}

bool MRTC_RemoteTaskInstance::IsRunning()
{
	return (m_State != REMOTETASK_STATE_FRESH);
}

int MRTC_RemoteTaskInstance::ProcessResult()
{
	bool bRet = m_pTaskClass->ProcessResult(m_pTask, m_pTaskArg, m_lResultData);

	if(bRet == true)
		m_State	= REMOTETASK_STATE_FINISHED;
	else
		m_State	= REMOTETASK_STATE_CORRUPT;

	return bRet;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskManLog
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_TaskManLog::MRTC_TaskManLog(class MRTC_TaskManager* _pMgr, const char *_pOldLog)
{
	m_MainThread = MRTC_SystemInfo::OS_GetThreadID();
	m_OldLog = _pOldLog;
	m_pMgr = _pMgr;
	m_pOldLog = NULL;
	MACRO_GetRegisterObject(CReferenceCount, pLog, m_OldLog);
	m_spOldLog = pLog;
	if (m_spOldLog)
	{
		MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spOldLog, m_OldLog);
		m_pOldLog = TDynamicCast<ILogFile>(pLog);
	}
}

MRTC_TaskManLog::~MRTC_TaskManLog()
{
	if(m_spOldLog)
	{
		MRTC_GOM()->RegisterObject(m_spOldLog, m_OldLog.GetStr());
	}
}

void MRTC_TaskManLog::Create(CStr _FileName, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->Create(_FileName, _bAppend);
}

void MRTC_TaskManLog::Log(const CStr& _s)
{
/*	if (MRTC_SystemInfo::OS_GetThreadID() != m_MainThread)
	{
		m_pMgr->LogLine(_s);
	}
	else*/
	{
		if(m_pOldLog) m_pOldLog->Log(_s);
	}
}

void MRTC_TaskManLog::Log(const char* _pStr)
{
	MRTC_TaskManLog::Log(CStr(_pStr));
}

void MRTC_TaskManLog::SetFileName(const CStr& _s, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->SetFileName(_s, _bAppend);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_TaskManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_TaskManager::MRTC_TaskManager(bool _bIgnoreLogOverride)
{
	m_pAM = NULL;
	m_bNetInitiated = false;
	m_ClientPacketState = 0;
	m_nMaxWorkingThreads = 0;
	m_nActiveWorkingThreads = 0;
	m_bCorrupt = false;
	m_bThrowExceptionOnCorrupt = true;
	m_CoordinatorSocket	= INVALID_SOCKET;
	m_AgentSocket	= INVALID_SOCKET;
	m_ClientSocket	= INVALID_SOCKET;
	m_spLogger	= MNew(MRTC_TaskLogger);
	m_hFileLock = NULL;
	MACRO_GetSystem;
	m_RestartTaskTime = pSys->GetEnvironment()->GetValuef("XWC_DIST_TIMEOUT", 45) * 60;
	m_ReportTaskTime = pSys->GetEnvironment()->GetValuef("XWC_DIST_REPORTINTERVAL", 5) * 60;
	if(_bIgnoreLogOverride == false)
	{
		m_spLog	= MNew2(MRTC_TaskManLog, this, "SYSTEM.LOG");
		m_spErrorLog	= MNew2(MRTC_TaskManLog, this, "SYSTEM.ERROR");

		MRTC_GOM()->RegisterObject((CReferenceCount*)m_spLog, "SYSTEM.LOG");
		MRTC_GOM()->RegisterObject((CReferenceCount*)m_spErrorLog, "SYSTEM.ERROR");
	}
}

MRTC_TaskManager::~MRTC_TaskManager()
{
	if(m_spErrorLog) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spErrorLog, "SYSTEM.ERROR");
	if(m_spLog) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spLog, "SYSTEM.LOG");
}

bool MRTC_TaskManager::NetworkEnabled()
{
	return m_CoordinatorSocket != INVALID_SOCKET;
}

void MRTC_TaskManager::Host_Start()
{
	// Don't allow this in non-static XWC
#ifndef PLATFORM_CONSOLE
	// Disable distribution by default
	MACRO_GetSystem;
	if(pSys->GetEnvironment()->GetValuei("XWC_ENABLEDISTRIBUTION",1) == 0)
		return;

	if(m_lExeFile.Len() == 0)
	{
		// Cache executable
		CCFile ExeFile;
		MACRO_GetSystem;
		CRegistry* pReg = pSys->GetEnvironment();
		CStr ExeFilename = pReg->GetValue("XWC_EXEFILE");
		try
		{
			if(ExeFilename.Len() == 0)
			{
				char aBuf[1024];
				int nLen = GetModuleFileName(NULL, aBuf, 1024);
				ExeFilename = aBuf;
			}
			ExeFile.Open(ExeFilename, CFILE_READ);
			m_lExeFile.SetLen(ExeFile.Length());
			ExeFile.Read(m_lExeFile.GetBasePtr(), m_lExeFile.ListSize());
			ExeFile.Close();
		}
		catch(CCException)
		{
			// Can't do distributed compilation
			return;
		}
	}

	if(!m_bNetInitiated)
	{
		m_bNetInitiated	= true;
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD( 2, 2 );

		err = WSAStartup( wVersionRequested, &wsaData );

		if ( err != 0 ) 
		{
			return;
		}

		if (LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) 
		{
			WSACleanup( );
			return; 
		}
	}

	CStr XWCCoordinator = pSys->GetEnvironment()->GetValue("XWC_COORDINATOR", "amd6408");
	HOSTENT* pent = gethostbyname(XWCCoordinator.GetStr());
	if(pent == 0)
		return;

	// try connecting to coordinator first, no point in setting up if there is no coordinator
	SOCKET CoordSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(CoordSocket == INVALID_SOCKET)
	{
		return;
	}

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family	= AF_INET;
	address.sin_port	= htons(TASKMAN_PORT_COORDINATOR);
//	address.sin_addr.S_un.S_addr	= htonl(0xac121ac5);
	address.sin_addr.S_un.S_addr	= *((int*)(pent->h_addr_list[0]));
	if(connect(CoordSocket, (sockaddr*)&address, sizeof(address)))
	{
		closesocket(CoordSocket);
		return;
	}

	uint32 NonBlocking = 1;
	if(ioctlsocket(CoordSocket, FIONBIO, &NonBlocking))
	{
		closesocket(CoordSocket);
		return;
	}

	SOCKET HostSocket = socket(AF_INET, SOCK_STREAM, 0);
	SOCKET AgentSocket = socket(AF_INET, SOCK_STREAM, 0);
	if((HostSocket == INVALID_SOCKET) || (AgentSocket == INVALID_SOCKET))
	{
		closesocket(CoordSocket);
		if(HostSocket != INVALID_SOCKET) closesocket(HostSocket);
		if(AgentSocket != INVALID_SOCKET) closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to open service socket");
		return;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family	= AF_INET;
//	address.sin_port	= htons(TASKMAN_PORT_HOST);
	address.sin_port	= 0;

	if(bind(HostSocket, (sockaddr*)&address, sizeof(address)))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to bind service socket");
		return;
	}

	address.sin_family	= AF_INET;
//	address.sin_port	= htons(TASKMAN_PORT_AGENT);
	address.sin_port	= 0;
	if(bind(AgentSocket, (sockaddr*)&address, sizeof(address)))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to bind service socket");
		return;
	}

	if(listen(HostSocket, 64) || listen(AgentSocket, 64))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to listen service socket");
		return;
	}

	if(ioctlsocket(HostSocket, FIONBIO, &NonBlocking) || ioctlsocket(AgentSocket, FIONBIO, &NonBlocking))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to non-block service socket");
		return;
	}

	uint16 AgentPort, HostPort;

	int nLen = sizeof(address);
	memset(&address, 0, nLen);
	if(getsockname(AgentSocket, (sockaddr*)&address, &nLen))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to retrieve agentsocket info");
		return;
	}
	AgentPort	= htons(address.sin_port);
	memset(&address, 0, nLen);
	if(getsockname(HostSocket, (sockaddr*)&address, &nLen))
	{
		closesocket(CoordSocket);
		closesocket(HostSocket);
		closesocket(AgentSocket);
		Error_static("MRTC_TaskManager::Service_Start", "Failed to retrieve hostsocket info");
		return;
	}
	HostPort	= htons(address.sin_port);

	m_AgentSocket	= AgentSocket;
	m_HostSocket	= HostSocket;
	m_CoordinatorSocket	= CoordSocket;

	// Send information about host
	char aHostInfo[16];
	memcpy(aHostInfo, &AgentPort, sizeof(uint16));
	memcpy(aHostInfo + 2, &HostPort, sizeof(uint16));
	Coord_SendPacket(HOST_PACKET_HOST_INFO, aHostInfo, 4);

	// Start a session
	Coord_SendPacket(HOST_PACKET_REQUEST_SESSION, 0, 0);

	LogFile(CStrF("Host: Distribution enabled, AgentPort %d, HostPort %d", AgentPort, HostPort));

	{
		CCFile ExeFile;
		MACRO_GetSystem;
		CRegistry* pReg = pSys->GetEnvironment();
		CStr ExeFilename = pReg->GetValue("XWC_EXEFILE");
		CStr XWCCoordinator = pSys->GetEnvironment()->GetValue("XWC_COORDINATOR", "amd6408");

		// Next spawn XWC_Agent locally to get a private instance which means you'll always get work done even if all others are busy
		TArray<uint8> lXWCAgentBuffer;
		bool bFailed = true;
		try
		{
			if(ExeFilename.Len() == 0)
			{
				char aBuf[1024];
				int nLen = GetModuleFileName(NULL, aBuf, 1024);
				ExeFilename = aBuf;
			}
			CStr XWCAgentFilename = ExeFilename.GetPath() + "XWC_Agent.exe";
			if(CDiskUtil::FileExists(XWCAgentFilename))
			{
				bFailed = false;
				ExeFile.Open(XWCAgentFilename, CFILE_READ);
				lXWCAgentBuffer.SetLen(ExeFile.Length());
				ExeFile.Read(lXWCAgentBuffer.GetBasePtr(), ExeFile.Length());
				ExeFile.Close();

				{
					SHELLEXECUTEINFO ExecInfo;
					ZeroMemory(&ExecInfo, sizeof(ExecInfo));
					ExecInfo.cbSize	= sizeof(ExecInfo);
					ExecInfo.fMask	= SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
					ExecInfo.hwnd	= 0;
					ExecInfo.lpVerb = "open";
					ExecInfo.lpFile	= XWCAgentFilename.GetStr();
					CStr Parameters = (CStr("-private -host ") + XWCCoordinator);
					ExecInfo.lpParameters = Parameters.GetStr();
					if(!ShellExecuteEx(&ExecInfo))
					{
						bFailed = true;
					}
					else if(ExecInfo.hProcess == 0)
						bFailed = true;
				}
			}
		}
		catch(CCException)
		{
			bFailed = true;
		}
		if(bFailed)
			LogFile("WARNING!!! Failed to spawn local XWC_Agent, you won't get any processing done locally!");
	}

	m_LastRemoteUpdate.Start();
#endif

	return;
}

void MRTC_TaskManager::Host_Flush()
{
	if(!NetworkEnabled())
		return;

	Coord_SendPacket(HOST_PACKET_KILL_SESSION, NULL, 0);
//	Host_AnswerCalls();
	MRTC_SystemInfo::OS_Sleep(100);
	Host_CloseAllClients();
	// Start a new session
	Coord_SendPacket(HOST_PACKET_REQUEST_SESSION, 0, 0);
}

void MRTC_TaskManager::Host_CloseAllClients()
{
	if(!NetworkEnabled())
		return;

	for(int i = 0; i < m_lspActiveClients.Len(); i++)
	{
		spMRTC_TaskClient spClient = m_lspActiveClients[i];
		if(spClient == 0 )
			continue;

		spClient->Close();
		m_lspActiveClients[i]	= 0;
	}

	for(int i = 0; i < m_lspPotentialClients.Len(); i++)
	{
		spMRTC_TaskClient spClient = m_lspPotentialClients[i];
		if(spClient == 0 )
			continue;

		spClient->Close();
		m_lspPotentialClients[i]	= 0;
	}

	m_lspActiveClients.Destroy();
	m_lspPotentialClients.Destroy();
}

void MRTC_TaskManager::Host_Stop()
{
	if(!NetworkEnabled())
		return;

	Coord_SendPacket(HOST_PACKET_KILL_SESSION, 0, 0);
	Host_CloseAllClients();

	if(m_pAM)
	{
		m_pAM->Thread_RequestTermination();
		while(!m_pAM->Thread_IsTerminated())
			MRTC_SystemInfo::OS_Sleep(100);
	}

	if(m_HostSocket != INVALID_SOCKET)
	{
		closesocket(m_HostSocket);
		m_HostSocket	= INVALID_SOCKET;
	}
}

TPtr<TaskPacket> MRTC_TaskManager::Host_FetchPacket(spMRTC_TaskClient _spClient)
{
	MRTC_TaskClient* pClient = _spClient;
	TPtr<TaskPacket> spPacket;

	{
		int SendState = send(pClient->m_Socket, NULL, 0, 0);
		if(SendState < 0)
		{
			int SendError = WSAGetLastError();
			if(SendError == WSAENOBUFS)
			{
				// Out of resources, return empty packet
				return spPacket;
			}
			else if(SendError != WSAEWOULDBLOCK)
			{
				// Some error on socket.
				if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
				{
					LogFile(CStrF("ERROR: Error %d occured on client '%s(%d)' when polling socket-send-state, adding assigned task to pending queue.", SendError, pClient->m_HostName.GetStr(), pClient->m_Socket));
					AddPendingRemoteTask(pClient->m_spTask);
				}
				pClient->Close();
				return spPacket;
			}
		}
		int RecvState = recv(pClient->m_Socket, NULL, 0, 0);
		if(RecvState < 0)
		{
			int RecvError = WSAGetLastError();
			if(RecvError == WSAENOBUFS)
			{
				// Out of resources, return empty packet
				return spPacket;
			}
			else if(RecvError != WSAEWOULDBLOCK)
			{
				// Some error on socket.
				if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
				{
					LogFile(CStrF("ERROR: Error %d occured on client '%s(%d)' when polling socket-recv-state, adding assigned task to pending queue.", RecvError, pClient->m_HostName.GetStr(), pClient->m_Socket));
					AddPendingRemoteTask(pClient->m_spTask);
				}
				pClient->Close();
				return spPacket;
			}
		}
	}

	switch(pClient->m_PartialPacket.m_State)
	{
	case PACKET_STATE_NEWPACKET:
		{
			uint32 PacketType;
			int nRet = recv(pClient->m_Socket, (char*)&PacketType, 4, 0);
			if(nRet == -1)
			{
				int ErrorCode = WSAGetLastError();
				if(ErrorCode == WSAENOBUFS)
				{
					// No resources, do nothing
					break;
				}
				else if (ErrorCode != WSAEWOULDBLOCK)
				{
					if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
					{
						LogFile(CStrF("ERROR: Error %d occured on client '%s(%d)' when fetching packet-type, adding assigned task to pending queue.", ErrorCode, pClient->m_HostName.GetStr(), pClient->m_Socket));
						AddPendingRemoteTask(pClient->m_spTask);
					}
					pClient->Close();
					break;
				}
			}
			else if(nRet == 0)
			{
				if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
				{
					LogFile(CStrF("ERROR: Client '%s(%d)' closed connection while fetching packet-type, adding assigned task to pending queue.", pClient->m_HostName.GetStr(), pClient->m_Socket));
					AddPendingRemoteTask(pClient->m_spTask);
				}
				pClient->Close();
				break;
			}
			else
			{
				pClient->m_PartialPacket.m_Type	= PacketType;
				pClient->m_PartialPacket.m_State	= PACKET_STATE_PACKETSIZE;
			}
			break;
		}

	case PACKET_STATE_PACKETSIZE:
		{
			uint32 PacketSize;
			int nRet = recv(pClient->m_Socket, (char*)&PacketSize, 4, 0);
			if(nRet == -1)
			{
				int ErrorCode = WSAGetLastError();
				if(ErrorCode == WSAENOBUFS)
				{
					// No resources, do nothing
					break;
				}
				else if (ErrorCode != WSAEWOULDBLOCK)
				{
					if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
					{
						LogFile(CStrF("ERROR: Error %d occured on client '%s(%d)' when fetching packet-size, adding assigned task to pending queue.", ErrorCode, pClient->m_HostName.GetStr(), pClient->m_Socket));
						AddPendingRemoteTask(pClient->m_spTask);
					}
					pClient->Close();
					break;
				}
			}
			else if(nRet == 0)
			{
				if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
				{
					LogFile(CStrF("ERROR: Client '%s(%d)' closed connection while fetching packet-size, adding assigned task to pending queue.", pClient->m_HostName.GetStr(), pClient->m_Socket));
					AddPendingRemoteTask(pClient->m_spTask);
				}
				pClient->Close();
				break;
			}
			else
			{
				pClient->m_PartialPacket.m_Size	= PacketSize;
				pClient->m_PartialPacket.m_State	= PACKET_STATE_PACKETDATA;

				pClient->m_PartialPacket.m_lData.SetLen(PacketSize);
			}
			break;
		}

	case PACKET_STATE_PACKETDATA:
		{
			int nData = pClient->m_PartialPacket.m_Size - pClient->m_PartialPacket.m_iPacketPos;
			if(nData > 0)
			{
				int nRet = recv(pClient->m_Socket, (char*)pClient->m_PartialPacket.m_lData.GetBasePtr() + pClient->m_PartialPacket.m_iPacketPos, nData, 0);
				if(nRet == -1)
				{
					int ErrorCode = WSAGetLastError();
					if(ErrorCode == WSAENOBUFS)
					{
						// No resources, do nothing
						break;
					}
					else if (ErrorCode != WSAEWOULDBLOCK)
					{
						if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
						{
							LogFile(CStrF("ERROR: Error %d occured on client '%s(%d)' when fetching packet-data, adding assigned task to pending queue.", ErrorCode, pClient->m_HostName.GetStr(), pClient->m_Socket));
							AddPendingRemoteTask(pClient->m_spTask);
						}
						pClient->Close();
						break;
					}
				}
				else if(nRet == 0)
				{
					if(pClient->m_spTask && pClient->m_spTask->m_nFailures < 3)
					{
						LogFile(CStrF("ERROR: Client '%s(%d)' closed connection while fetching packetdata, adding assigned task to pending queue.", pClient->m_HostName.GetStr(), pClient->m_Socket));
						AddPendingRemoteTask(pClient->m_spTask);
					}
					pClient->Close();
					break;
				}
				else
				{
					pClient->m_PartialPacket.m_iPacketPos	+= nRet;
				}
			}

			if(pClient->m_PartialPacket.m_iPacketPos == pClient->m_PartialPacket.m_Size)
			{
				spPacket = MNew(TaskPacket);
				*spPacket	= pClient->m_PartialPacket;
				pClient->m_PartialPacket.Clear();
				pClient->m_PartialPacket.m_State	= PACKET_STATE_NEWPACKET;
			}
			break;
		}
	}

	if(spPacket)
	{
		CMTime Now;
		Now.Snapshot();
		pClient->m_TimeOfLastPacket = Now;
	}

	return spPacket;
}

void MRTC_TaskManager::Host_FetchPackets()
{
	if(!NetworkEnabled())
		return;

	for(int iClient = 0; iClient < m_lspActiveClients.Len(); iClient++)
	{
		spMRTC_TaskClient spClient = m_lspActiveClients[iClient];
		if(spClient == 0 )
			continue;

		TPtr<TaskPacket> spPacket = Host_FetchPacket(spClient);
		if(spPacket)
		{
			switch(spPacket->m_Type)
			{
			case PACKET_TYPE_LOG:
				{
					CStr LogStr = CStrF("%s(%d): %s", spClient->m_HostName.GetStr(), spClient->m_Socket, (const char*)spPacket->m_lData.GetBasePtr());
					LogFile(LogStr);
					break;
				}

			case PACKET_TYPE_PROGRESS:
				{
					// Progress report from a client
					break;
				}

			case PACKET_TYPE_REQUESTDATA:
				{
					// Client request task data (either just param or everything)

					TArray<TArray<uint8> > llPacketData = spClient->m_spTask->m_llPacketData;
					TArray<uint8> lParamData = spClient->m_spTask->m_lParamData;
					if(spClient->m_spTask->m_pfnOnDistribute && !spClient->m_spTask->m_bOnDistributeDone)
					{
						// Need to re-create the damn data again
						spClient->m_spTask->m_pfnOnDistribute(spClient->m_spTask->m_pTask, spClient->m_spTask->m_pTaskArg, lParamData, llPacketData);
					}

					uint32 FilesNeeded = *((uint32*)spPacket->m_lData.GetBasePtr());
					if(FilesNeeded != 0)
					{
						uint32* pFiles = ((uint32*)spPacket->m_lData.GetBasePtr()) + 1;
						for(int32 iFile = 0; spClient->m_spTask && (iFile < llPacketData.Len()) && (FilesNeeded != 0); iFile++)
						{
							bool bSend = false;
							if(FilesNeeded == 0xffffffff)
								bSend = true;
							else
							{
								if(*pFiles == iFile)
								{
									// This index is requested
									pFiles++;
									bSend = true;
									FilesNeeded--;
								}
							}

							if(bSend)
								Host_SendPacket(spClient, PACKET_TYPE_TASKDATA, llPacketData[iFile], &iFile, 1);
						}

					}
					if(!spClient->m_spTask)
					{
						// An error occured while sending data to client
						break;
					}
//					uint32 Everything = *((uint32*)spPacket->m_lData.GetBasePtr());
//					if(Everything)
//					{
//						if(spClient->m_spTask->m_llPacketData.Len() > 0)
//							Host_SendPacket(spClient, PACKET_TYPE_TASKDATA, spClient->m_spTask->m_llPacketData);
//					}
					if(spClient->m_spTask->m_lParamData.Len() > 0)
						Host_SendPacket(spClient, PACKET_TYPE_TASKPARAM, spClient->m_spTask->m_lParamData);

					if(spClient->m_spTask->m_pfnOnDistribute)
					{
						// Nuke data and recreate on demand?
						spClient->m_spTask->m_bOnDistributeDone = false;
						spClient->m_spTask->m_llPacketData.Destroy();
						spClient->m_spTask->m_lPacketChecksum.Destroy();
					}

					Host_SendPacket(spClient, PACKET_TYPE_TASKEXECUTE, 0, 0);
					break;
				}

			case PACKET_TYPE_RESULT:
				{
					// Result data from a client
					uint32 Result = *((uint32*)spPacket->m_lData.GetBasePtr());
					switch(Result)
					{
					case TASK_RETURN_FINISHED:
						{
//							LogFile(CStrF("Host %s finished a task successfully", spClient->m_HostName.GetStr()));
							LogFile(CStrF("XWCDIST: Finished %s(%d)", spClient->m_HostName.Str(), spClient->m_Socket));
							if(spPacket->m_lData.Len() > 4)
							{
								spClient->m_spTask->m_lResultData.SetLen(spPacket->m_lData.Len() - 4);
								memcpy(spClient->m_spTask->m_lResultData.GetBasePtr(), spPacket->m_lData.GetBasePtr() + 4, spPacket->m_lData.Len() - 4);

								spClient->m_spTask->ProcessResult();
							}
							else
							{
								spClient->m_spTask->m_State	= REMOTETASK_STATE_FINISHED;
							}
							spClient->m_spTask	= 0;
							spClient->Close();
							spClient	= 0;
							break;
						}

					case TASK_RETURN_INPROGRESS:
						{
							spClient->m_spTask->m_State	= REMOTETASK_STATE_RUNNING;
							break;
						}

					case TASK_RETURN_ERROR:
						{
//							LogFile(CStrF("Host %s finished a task unsuccessfully", spClient->m_HostName.GetStr()));
							M_ASSERT(spClient->m_spTask->m_pTask, "RemoteTask with no callee class assigned");
							// Flag callee task as corrupt
							LogFile(CStrF("XWCDIST: Error %s(%d)", spClient->m_HostName.Str(), spClient->m_Socket));
							spClient->m_spTask->m_pTask->m_State = TASK_STATE_CORRUPT;
							spClient->m_spTask->m_State	= REMOTETASK_STATE_CORRUPT;
							spClient->m_spTask	= 0;
							spClient->Close();
							spClient	= 0;
							break;
						}
					}
					break;
				}

			}
		}
		if(spClient && spClient->m_Socket == INVALID_SOCKET)
			spClient	= NULL;
		m_lspActiveClients[iClient]	= spClient;
	}

	for(int iClient = m_lspActiveClients.Len() - 1; iClient >= 0; iClient--)
		if(m_lspActiveClients[iClient] == 0)
			m_lspActiveClients.Del(iClient);
}

void MRTC_TaskManager::Host_UpdateNet()
{
	Host_SpawnClients();
	Host_AnswerCalls();
	Host_FetchPackets();
}

void MRTC_TaskManager::Host_BlockUntilDone()
{
	LogFile("XWCDIST: Start processing");
	if(m_AgentSocket == INVALID_SOCKET)
	{
		// If not started yet then try to start networking
		Host_Start();
	}
	while(m_lspTasks.Len() > 0)
	{
		Host_UpdateNet();
		Process();
		Host_ProcessRemote();
		if(NetworkEnabled())
			MRTC_SystemInfo::OS_Sleep(1);
	}

	Host_CloseAllClients();
	LogFile("XWCDIST: Finished processing");

	if(m_bThrowExceptionOnCorrupt && m_bCorrupt)
	{
		Error("MRTC_TaskManager::Host_BlockUntilDone", "Task returned error when processing");
	}
}

void MRTC_TaskManager::Host_BlockOnNumTasks(int _NumTasks)
{
	while(m_lspRemoteTasks.Len() > _NumTasks)
	{
		Host_UpdateNet();
		Host_ProcessRemote();
		if(NetworkEnabled())
			MRTC_SystemInfo::OS_Sleep(1);
		else
			Process();

	}
}



MRTC_TaskAgent::MRTC_TaskAgent()
{
	m_Socket	= INVALID_SOCKET;
	m_Host	= -1;
	m_State	= 0;
	m_iPacketPos	= 0;
	m_nRetryCount = 0;
	m_RetryTime = CMTime::GetCPU();
}

void MRTC_TaskAgent::Update()
{
	if(CMTime::GetCPU().Compare(m_RetryTime) < 0)
	{
		if(m_nRetryCount >= 30)
		{
			LogFile("Agent task flagged as broken after failing to send exe for 5 minutes");
			// Retry for 5 minutes then fail
			closesocket(m_Socket);
			m_Socket	= INVALID_SOCKET;
			m_State	= TASKAGENT_STATE_CORRUPT;
		}
		return;
	}

	switch(m_State)
	{
	case TASKAGENT_STATE_NEWCONNECTION:
		{
			{
				fd_set fd_read, fd_write, fd_exc;
				timeval timeout;
				timeout.tv_sec	= 0;
				timeout.tv_usec	= 0;
				FD_ZERO(&fd_read);
				FD_ZERO(&fd_write);
				FD_ZERO(&fd_exc);
				FD_SET(m_Socket, &fd_read);
				FD_SET(m_Socket, &fd_write);
				FD_SET(m_Socket, &fd_exc);
				if(select(1, &fd_read, &fd_write, &fd_exc, &timeout) < 0)
				{
					int Err = WSAGetLastError();
					LogFile("Error while selecting");
					return;
				}

				// Check if socket is read for writing
				if(!FD_ISSET(m_Socket, &fd_write))
					return;

			}
			int nLen = m_lExeFile.Len();
			int nRet = send(m_Socket, (char*)&nLen, 4, 0);
			if(nRet == -1)
			{
				int SocketError = WSAGetLastError();
				if(SocketError == WSAENOBUFS)
				{
					// No enough buffers available, sleep for a while
					m_RetryTime = CMTime::GetCPU() + CMTime::CreateFromSeconds(10);
					m_nRetryCount++;
					return;
				}
				else if (SocketError != WSAEWOULDBLOCK)
				{
					LogFile(CStrF("Socket error %d when trying to send size of XWC", SocketError));
					m_State	= TASKAGENT_STATE_CORRUPT;
					closesocket(m_Socket);
					m_Socket	= INVALID_SOCKET;
					return;
				}
			}
			else
			{
				m_State	= TASKAGENT_STATE_UPLOADING;
				m_nRetryCount	= 0;
			}
			break;
		}

	case TASKAGENT_STATE_UPLOADING:
		{
			int nData = m_lExeFile.Len() - m_iPacketPos;
			int nRet = send(m_Socket, (char*)m_lExeFile.GetBasePtr() + m_iPacketPos, nData, 0);
			if(nRet == -1)
			{
				int SocketError = WSAGetLastError();
				if(SocketError == WSAENOBUFS)
				{
					// No enough buffers available, sleep for a while
					m_RetryTime = CMTime::GetCPU() + CMTime::CreateFromSeconds(10);
					m_nRetryCount++;
					return;
				}
				else if (SocketError != WSAEWOULDBLOCK)
				{
					LogFile(CStrF("Socket error %d when trying to send file of XWC", SocketError));
					m_State	= TASKAGENT_STATE_CORRUPT;
					closesocket(m_Socket);
					m_Socket	= INVALID_SOCKET;
					return;
				}
			}
			else
			{
				m_iPacketPos	+= nData;
				m_nRetryCount	= 0;
			}

			if(m_iPacketPos == m_lExeFile.Len())
			{
				m_State	= TASKAGENT_STATE_COMPLETE;
			}
			break;
		}

	case TASKAGENT_STATE_COMPLETE:
		{
			// Wait for the transfer to complete
			char aData[1];
			int nRet = recv(m_Socket, aData, 1, 0);
			if(nRet == -1)
			{
				int ErrorCode = WSAGetLastError();
				if(ErrorCode == WSAENOBUFS)
				{
					// No resources, do nothing
					break;
				}
				else if (ErrorCode != WSAEWOULDBLOCK)
				{
					LogFile(CStrF("Socket error %d when polling completion of XWC file sending", ErrorCode));
					m_State	= TASKAGENT_STATE_CORRUPT;
					closesocket(m_Socket);
					m_Socket	= INVALID_SOCKET;
					return;
				}
			}
			else if(nRet == 0)
			{
				// Connection close from client
				closesocket(m_Socket);
				m_Socket	= INVALID_SOCKET;
				m_State	= TASKAGENT_STATE_FINISHED;
			}
			break;
		}
	}
}

MRTC_TaskClient::MRTC_TaskClient()
{
	m_Socket	= INVALID_SOCKET;
	m_Host	= -1;
	m_State	= 0;
}

void MRTC_TaskClient::Close()
{
	if(m_spTask)
	{
		m_spTask->m_State	= REMOTETASK_STATE_FRESH;
		m_spTask->m_nFailures++;
		m_spTask	= 0;
	}
	if(m_Socket != INVALID_SOCKET)
		closesocket(m_Socket);
	m_Socket	= INVALID_SOCKET;
}

void MRTC_TaskClient::Update()
{
}

bool MRTC_TaskManager::IsNewClient(int _Address)
{
	for(int i = 0; i < m_lspActiveClients.Len(); i++)
	{
		if(m_lspActiveClients[i] && (m_lspActiveClients[i]->m_Host == _Address))
			return false;
	}

	for(int i = 0; i < m_lspPotentialClients.Len(); i++)
	{
		if(m_lspPotentialClients[i] && (m_lspPotentialClients[i]->m_Host == _Address))
			return false;
	}

	return true;
}

void MRTC_TaskManager::Host_SpawnClients()
{
	if(!NetworkEnabled())
		return;

	if(m_lspPendingRemoteTasks.Len() > 0)
	{
		M_LOCK(m_Lock);
		int nTaskCount = m_lspPendingRemoteTasks.Len();
		int nTasks = 1;
		int iCurrent = 0;

		while(iCurrent < nTaskCount)
		{
			uint64 RamSize = m_lspPendingRemoteTasks[iCurrent]->m_RamSize;
			int32 Flags = (m_lspPendingRemoteTasks[iCurrent]->m_Flags & HOST_REQUEST_FL_MASK);
			iCurrent = iCurrent + 1;
			for(; iCurrent < nTaskCount; iCurrent++)
			{
				if(RamSize != m_lspPendingRemoteTasks[iCurrent]->m_RamSize)
					break;
				if(Flags != (m_lspPendingRemoteTasks[iCurrent]->m_Flags & HOST_REQUEST_FL_MASK))
					break;

				nTasks++;
			}

			if(RamSize > 0)
				Flags |= HOST_REQUEST_FL_MEMORY;

			int nData = 0;
			char aPacketData[256];
			*((int32*)(aPacketData + 0x00)) = nTasks; nData += sizeof(int32);
			*((int32*)(aPacketData + 0x04)) = Flags; nData += sizeof(int32);
			if(RamSize > 0)
			{
				*((uint64*)aPacketData + 0x08) = RamSize; nData += sizeof(uint64);
			}

			Coord_SendPacket(HOST_PACKET_REQUEST_AGENTS_EXT, aPacketData, nData);
		}

		m_lspPendingRemoteTasks.Destroy();
	}

	for(int i = m_lspActiveAgents.Len() - 1; i >= 0; i--)
	{
		if(m_lspActiveAgents[i])
		{
//			m_lspActiveAgents[i]->Update();
			if(m_lspActiveAgents[i]->m_WorkerThread.Thread_IsTerminated())
				m_lspActiveAgents.Del(i);
		}
	}
}

void MRTC_TaskManager::Host_AnswerCalls()
{
#ifndef PLATFORM_CONSOLE
	if(!NetworkEnabled())
		return;

	if(!m_pAM)
		m_pAM = DNew(MRTC_TaskManagerAnsweringMachine) MRTC_TaskManagerAnsweringMachine(this);
	if(!m_pAM->Thread_IsCreated())
		m_pAM->Thread_Create();

	{
		M_LOCK(m_PotenialClientLock);
		for(int i = 0; i < m_lspPotentialClients.Len(); i++)
		{
			if(m_lspPotentialClients[i])
				m_lspActiveClients.Add(m_lspPotentialClients[i]);
		}
		m_lspPotentialClients.Destroy();
	}
	{
		M_LOCK(m_PendingAgentLock);
		for(int i = 0; i < m_lspPendingAgents.Len(); i++)
		{
			if(m_lspPendingAgents[i])
				m_lspActiveAgents.Add(m_lspPendingAgents[i]);
		}
		m_lspPendingAgents.Destroy();
	}
/*
	int Connection = accept(m_HostSocket, NULL, 0);
	while(Connection != INVALID_SOCKET)
	{
		do
		{
			uint32 NonBlocking = 1;
			if(ioctlsocket(Connection, FIONBIO, &NonBlocking))
			{
				LogFile("Host: Failed to set non-blocking on host connection");
				closesocket(Connection);
				break;
			}

			int KeepAlive = 1;
			if(setsockopt(Connection, SOL_SOCKET, SO_KEEPALIVE, (const char*)&KeepAlive, sizeof(KeepAlive)))
			{
				LogFile("Host: Failed to set keepalive on host connection");
				closesocket(Connection);
				break;
			}

			sockaddr_in sock;
			int size = sizeof(sock);
			getpeername(Connection, (sockaddr*)&sock, &size);

			hostent* pent = gethostbyaddr((const char*)&sock.sin_addr.S_un.S_addr, 4, AF_INET);

			spMRTC_TaskClient spClient = MNew(MRTC_TaskClient);
			spClient->m_Socket	= Connection;
			spClient->m_Host	= sock.sin_addr.S_un.S_addr;
			spClient->m_State	= TASKCLIENT_STATE_NEWCONNECTION;
			if(pent)
				spClient->m_HostName	= pent->h_name;
			else
				spClient->m_HostName	= CStrF("%d.%d.%d.%d", (spClient->m_Host & 0xff), ((spClient->m_Host>>8) & 0xff), ((spClient->m_Host>>16) & 0xff), ((spClient->m_Host>>24) & 0xff));

			m_lspPotentialClients.Add(spClient);
		} while(0);

		Connection = accept(m_HostSocket, NULL, 0);
	}

	Connection = accept(m_AgentSocket, NULL, 0);
	while(Connection != INVALID_SOCKET)
	{
		do
		{
			uint32 NonBlocking = 1;
			if(ioctlsocket(Connection, FIONBIO, &NonBlocking))
			{
				LogFile("Host: Failed to set non-blocking on agent connection");
				closesocket(Connection);
				break;
			}

			int KeepAlive = 1;
			if(setsockopt(Connection, SOL_SOCKET, SO_KEEPALIVE, (const char*)&KeepAlive, sizeof(KeepAlive)))
			{
				LogFile("Host: Failed to set keepalive on host connection");
				closesocket(Connection);
				break;
			}

			sockaddr_in sock;
			int size = sizeof(sock);
			getpeername(Connection, (sockaddr*)&sock, &size);

			hostent* pent = gethostbyaddr((const char*)&sock.sin_addr.S_un.S_addr, 4, AF_INET);

			spMRTC_TaskAgent spAgent = MNew(MRTC_TaskAgent);
			spAgent->m_lExeFile	= m_lExeFile;
			spAgent->m_Socket	= Connection;
			spAgent->m_Host	= sock.sin_addr.S_un.S_addr;
			spAgent->m_State	= TASKCLIENT_STATE_NEWCONNECTION;
			if(pent)
				spAgent->m_HostName	= pent->h_name;
			else
				spAgent->m_HostName	= CStrF("%d.%d.%d.%d", (spAgent->m_Host & 0xff), ((spAgent->m_Host>>8) & 0xff), ((spAgent->m_Host>>16) & 0xff), ((spAgent->m_Host>>24) & 0xff));

			m_lspActiveAgents.Add(spAgent);
		} while(0);

		Connection = accept(m_AgentSocket, NULL, 0);
	}
*/
#endif
}

void MRTC_TaskManager::Process()
{
	for(int i = 0; i < m_lspTasks.Len(); i++)
	{
		if(!m_lspTasks[i]->IsFinished())
		{
			m_lspTasks[i]->Process();
		}
	}

	for(int i = m_lspTasks.Len() - 1; i >= 0; i--)
	{
		if(m_lspTasks[i]->IsCorrupt())
			m_bCorrupt = true;

		if(m_lspTasks[i]->IsFinished())
			m_lspTasks[i]	= 0;

		if(m_lspTasks[i] == 0 )
			m_lspTasks.Del(i);
	}

	TArray<CStr> lLines = m_spLogger->GetLines();
	for(int i = 0; i < lLines.Len(); i++)
		LogFile(lLines[i]);
}

void MRTC_TaskManager::LogLine(CStr _Line)
{
	m_spLogger->AddLine(_Line);
}

void MRTC_TaskManager::FlushLog()
{
	TArray<CStr> lLines = m_spLogger->GetLines();
	for(int i = 0; i < lLines.Len(); i++)
		LogFile(lLines[i]);
}

class MRTC_TaskArgVirtualRemoteTask : public MRTC_TaskBaseArg
{
	MRTC_DECLARE;
public:
	MRTC_TaskArgVirtualRemoteTask(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _llData, spMRTC_RemoteTaskInstance _spInstance) : m_TaskName(_TaskName), m_lParam(_lParam), m_llData(_llData), m_spRemoteInstance(_spInstance) {}

	CStr m_TaskName;
	TArray<uint8>	m_lParam;
	TArray<TArray<uint8> >	m_llData;
	spMRTC_RemoteTaskInstance	m_spRemoteInstance;

	TArray<uint8>	m_lResult;
};

class MRTC_VirtualResultRemoteTask : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		MRTC_TaskArgVirtualRemoteTask* pArg = TDynamicCast<MRTC_TaskArgVirtualRemoteTask>(_pArg);
		if(!pArg) return TASK_RETURN_ERROR;

		pArg->m_spRemoteInstance->m_lResultData	= pArg->m_lResult;
		if(pArg->m_spRemoteInstance->ProcessResult())
		{
			pArg->m_spRemoteInstance->m_State	= REMOTETASK_STATE_FINISHED;
			return TASK_RETURN_FINISHED;
		}

		pArg->m_spRemoteInstance->m_State	= REMOTETASK_STATE_CORRUPT;
		return TASK_RETURN_ERROR;
	}
};

class MRTC_VirtualRemoteTask : public MRTC_TaskBase
{
	MRTC_DECLARE;
public:
	virtual int Process(class MRTC_TaskInstance* _pTask, MRTC_TaskBaseArg* _pArg)
	{
		MRTC_TaskArgVirtualRemoteTask* pArg = TDynamicCast<MRTC_TaskArgVirtualRemoteTask>(_pArg);
		if(!pArg) return TASK_RETURN_ERROR;

		MRTC_TaskArgRemoteTask* pRemoteArg = MNew(MRTC_TaskArgRemoteTask);
		// Need to force the array to be created

		if(pArg->m_spRemoteInstance->m_pfnOnDistribute)
		{
			spMRTC_RemoteTaskInstance spRemoteTask = pArg->m_spRemoteInstance;
			TArray<uint8> lParamData = spRemoteTask->m_lParamData;
			TArray<TArray<uint8> > llPacketData = spRemoteTask->m_llPacketData;
			if(spRemoteTask->m_pfnOnDistribute && !spRemoteTask->m_bOnDistributeDone)
			{
				lParamData.Destroy();
				llPacketData.Destroy();
				spRemoteTask->m_pfnOnDistribute(spRemoteTask->m_pTask, spRemoteTask->m_pTaskArg, lParamData, llPacketData);
			}

			pArg->m_lResult.SetGrow(4*1024*1024);
			pRemoteArg->Create(lParamData, llPacketData, pArg->m_lResult);

			spMRTC_TaskBaseArg spArg = pRemoteArg;
			spMRTC_TaskInstance spTask = MRTC_TaskInstance::CreateTaskInstance(pArg->m_TaskName, spArg);

			SpawnTask(_pTask->m_pManager, spTask, true);
		}
		else
		{
			pArg->m_lResult.SetGrow(4*1024*1024);
			pRemoteArg->Create(pArg->m_lParam, pArg->m_llData, pArg->m_lResult);

			spMRTC_TaskBaseArg spArg = pRemoteArg;
			spMRTC_TaskInstance spTask = MRTC_TaskInstance::CreateTaskInstance(pArg->m_TaskName, spArg);

			_pTask->AddTask(spTask);
		}

		return TASK_RETURN_INPROGRESS;
	}
};

MRTC_IMPLEMENT(MRTC_TaskArgVirtualRemoteTask, MRTC_TaskBaseArg);
MRTC_IMPLEMENT_DYNAMIC(MRTC_VirtualRemoteTask, MRTC_TaskBase);
MRTC_IMPLEMENT_DYNAMIC(MRTC_VirtualResultRemoteTask, MRTC_TaskBase);

void MRTC_TaskManager::Host_ProcessRemote()
{
	// Do cleanup first so any tasks flagged as corrupt will make their owners inherit this state
	for(int i = m_lspRemoteTasks.Len() - 1; i >= 0; i--)
	{
		if(m_lspRemoteTasks[i]->IsFinished())
			m_lspRemoteTasks[i]	= 0;

		if(m_lspRemoteTasks[i] == 0 )
			m_lspRemoteTasks.Del(i);
	}

	if(!NetworkEnabled())
	{
		for(int i = 0; i < m_lspRemoteTasks.Len(); i++)
		{
			spMRTC_RemoteTaskInstance spTask = m_lspRemoteTasks[i];
			if(!spTask->IsRunning())
			{
				if(spTask->m_nFailures > 3)
				{
					// This task has failed 3 times, guessing that it will never work
					spTask->m_State = REMOTETASK_STATE_CORRUPT;
				}
				else
				{
					spMRTC_TaskBaseArg spArg = MNew4(MRTC_TaskArgVirtualRemoteTask, spTask->m_TaskName, spTask->m_lParamData, spTask->m_llPacketData, spTask);
					spMRTC_TaskInstance spVirtual = MRTC_TaskInstance::CreateTaskInstance("MRTC_VirtualRemoteTask", spArg);
					spMRTC_TaskInstance spVirtualResult = MRTC_TaskInstance::CreateTaskInstance("MRTC_VirtualResultRemoteTask", spArg);
					spVirtualResult->DependOn(spVirtual);

					AddTask(spVirtual);
					AddTask(spVirtualResult);
					spTask->m_State	= REMOTETASK_STATE_RUNNING;
				}
			}
		}
	}
	else
	{
		for(int i = 0; i < m_lspRemoteTasks.Len(); i++)
		{
			spMRTC_RemoteTaskInstance spTask = m_lspRemoteTasks[i];
			if(!spTask->IsRunning())
			{
				if(spTask->m_nFailures > 3)
				{
					// This task has failed 3 times, guessing that it will never work
					spTask->m_State = REMOTETASK_STATE_CORRUPT;
				}
				else
				{
					// Task isn't running, allocate a client if there is one available
					spMRTC_TaskClient spClient = Host_GetFreeClient();
					if(spClient != 0)
					{
						if(spTask->m_pfnOnDistribute && !spTask->m_bOnDistributeDone)
						{
							spTask->m_pfnOnDistribute(spTask->m_pTask, spTask->m_pTaskArg, spTask->m_lParamData, spTask->m_llPacketData);
							spTask->m_bOnDistributeDone = true;
							int nPackets = spTask->m_llPacketData.Len();
							spTask->m_lPacketChecksum.SetGrow(nPackets);
							for(int i = 0; i < nPackets; i++)
							{
								TArray<uint8> lData = spTask->m_llPacketData[i];
								spTask->m_lPacketChecksum.Add(CMD5Digest(lData.GetBasePtr(), lData.Len()));
							}
						}
						MRTC_TaskClient* pClient = spClient;
						//LogFile(CStrF("Distributing task '%s' to agent '%s'", spTask->m_TaskName.GetStr(), spClient->m_HostName.GetStr()));
						// Task is executed the instant the taskparam packet arrives
						Host_SendPacket(pClient, PACKET_TYPE_TASKNAME, spTask->m_TaskName.GetStr(), spTask->m_TaskName.Len());
//						Host_SendPacket(spClient, PACKET_TYPE_DATACHECKSUM, &spTask->m_DataChecksum, sizeof(spTask->m_DataChecksum));
						int32 nPackets = spTask->m_lPacketChecksum.Len();
						Host_SendPacket(pClient, PACKET_TYPE_DATACHECKSUM, spTask->m_lPacketChecksum.GetBasePtr(), spTask->m_lPacketChecksum.ListSize(), &nPackets, 1);
						pClient->m_spTask	= spTask;
						pClient->m_StartTime.Snapshot();
						pClient->m_TimeOfLastPacket.Snapshot();
						spTask->m_State	= REMOTETASK_STATE_RUNNING;

						LogFile(CStrF("XWCDIST: Start %s(%d)", pClient->m_HostName.Str(), pClient->m_Socket));
					}
				}
			}
		}
	}

	CMTime TimeNow;
	TimeNow.Snapshot();
	if((TimeNow - m_LastRemoteUpdate).Compare(CMTime::CreateFromSeconds(5 * 60)) > 0)
	{
		for(int i = 0; i < m_lspActiveClients.Len(); i++)
		{
			MRTC_TaskClient* pClient = m_lspActiveClients[i];
			if(pClient && pClient->m_spTask)
			{
				CMTime DeltaTime = TimeNow - pClient->m_TimeOfLastPacket;
				if((DeltaTime.Compare(CMTime::CreateFromSeconds(m_RestartTaskTime)) > 0) && !(pClient->m_spTask->m_Flags & CLIENT_REQUEST_FL_SLOWTASK))
				{
					LogFile(CStrF("Agent '%s(%d)' has not updated itself in %s, terminating task and relocating to other host.", pClient->m_HostName.GetStr(), pClient->m_Socket, pClient->m_spTask->m_TaskName.GetStr(), CMTimeToStr(DeltaTime).GetStr()));
					closesocket(pClient->m_Socket);
					pClient->m_Socket = INVALID_SOCKET;
				}
				else if(DeltaTime.Compare(CMTime::CreateFromSeconds(m_ReportTaskTime)) > 0)
				{
					CMTime Measure = TimeNow - pClient->m_StartTime;

					LogFile(CStrF("Agent '%s(%d)' running task '%s', active for %s, time since last packet %s", pClient->m_HostName.GetStr(), pClient->m_Socket, pClient->m_spTask->m_TaskName.GetStr(), CMTimeToStr(Measure).GetStr(), CMTimeToStr(DeltaTime).GetStr()));
				}
			}
		}
		m_LastRemoteUpdate.Snapshot();
	}
}

bool MRTC_TaskManager::Client_Start(int _Address, int _Port)
{
	if(!m_bNetInitiated)
	{
		m_bNetInitiated	= true;
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD( 2, 2 );

		err = WSAStartup( wVersionRequested, &wsaData );

		if ( err != 0 ) 
		{
			return false;
		}

		if (LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) 
		{
			WSACleanup( );
			return false; 
		}
	}

	m_ClientSocket	= INVALID_SOCKET;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
		return false;

	LogFile(CStrF("Client: Connecting to host %d.%d.%d.%d %d", (_Address >> 24) & 0xff, (_Address >> 16) & 0xff, (_Address >> 8) & 0xff, (_Address >> 0) & 0xff, _Port));

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_addr.S_un.S_addr	= htonl(_Address);
	address.sin_family	= AF_INET;
	address.sin_port	= htons(_Port);

	int nRetries = 0;

	while(connect(sock, (sockaddr*)&address, sizeof(address)))
	{
		nRetries++;
		if(nRetries < 3)
		{
			LogFile("Client: Failed to connect to host, should retry...");
			closesocket(sock);
			return false;
		}
		else
		{
			LogFile(CStrF("Client: Failed to connect to host, retrying (%d of %d)", nRetries, 3));
			MRTC_SystemInfo::OS_Sleep(500);
		}
	}

	uint32 NonBlocking = 1;
	if(ioctlsocket(sock, FIONBIO, &NonBlocking))
	{
		LogFile("Client: Failed to set non-blocking on socket");
		closesocket(sock);
		return false;
	}

	m_ClientCacheFilename = CStrF("XWC_0x%.8X_0x%.4X.cache", _Address, _Port);

	m_ClientSocket	= sock;
	return true;
}

void MRTC_TaskManager::Client_SetBlocking(bool _bBlocking)
{
	if (m_ClientSocket != INVALID_SOCKET)
	{
		uint32 NonBlocking = _bBlocking == false;
		ioctlsocket(m_ClientSocket, FIONBIO, &NonBlocking);
	}
}

void MRTC_TaskManager::Client_Stop()
{
	if(m_ClientSocket != INVALID_SOCKET)
	{

		linger l;
		l.l_onoff = 1;
		l.l_linger = 5;
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));

		closesocket(m_ClientSocket);
		m_ClientSocket	= INVALID_SOCKET;
	}
}

TPtr<TaskPacket> MRTC_TaskManager::Client_ReadPacket()
{
	TPtr<TaskPacket> spPacket;

	switch(m_ClientPacketState)
	{
	case PACKET_STATE_NEWPACKET:
		{
			uint32 PacketType;
			int nRet = recv(m_ClientSocket, (char*)&PacketType, 4, 0);
			if(nRet == -1)
			{
				int ErrorCode = WSAGetLastError();
				if(ErrorCode == WSAENOBUFS)
				{
					// No resources, do nothing
					break;
				}
				else if (ErrorCode != WSAEWOULDBLOCK)
				{
					LogFile(CStrF("Socket error %d when trying to fetch packet type", ErrorCode));
					closesocket(m_ClientSocket);
					m_ClientSocket	= INVALID_SOCKET;
					break;
				}
			}
			else if(nRet == 0)
			{
				closesocket(m_ClientSocket);
				m_ClientSocket	= INVALID_SOCKET;
				break;
			}
			else
			{
				m_ClientPartialPacket.m_Type	= PacketType;
				m_ClientPacketState	= PACKET_STATE_PACKETSIZE;
			}
			break;
		}

	case PACKET_STATE_PACKETSIZE:
		{
			uint32 PacketSize;
			int nRet = recv(m_ClientSocket, (char*)&PacketSize, 4, 0);
			if(nRet == -1)
			{
				int ErrorCode = WSAGetLastError();
				if(ErrorCode == WSAENOBUFS)
				{
					// No resources, do nothing
					break;
				}
				else if (ErrorCode != WSAEWOULDBLOCK)
				{
					LogFile(CStrF("Socket error %d when trying to fetch packet size", ErrorCode));
					closesocket(m_ClientSocket);
					m_ClientSocket	= INVALID_SOCKET;
					break;
				}
			}
			else if(nRet == 0)
			{
				closesocket(m_ClientSocket);
				m_ClientSocket	= INVALID_SOCKET;
				break;
			}
			else
			{
				m_ClientPartialPacket.m_Size	= PacketSize;
				m_ClientPacketState	= PACKET_STATE_PACKETDATA;

				m_ClientPartialPacket.m_lData.SetLen(PacketSize);
			}
			break;
		}

	case PACKET_STATE_PACKETDATA:
		{
			int nData = m_ClientPartialPacket.m_Size - m_ClientPartialPacket.m_iPacketPos;
			if(nData > 0)
			{
				int nRet = recv(m_ClientSocket, (char*)m_ClientPartialPacket.m_lData.GetBasePtr() + m_ClientPartialPacket.m_iPacketPos, nData, 0);
				if(nRet == -1)
				{
					int ErrorCode = WSAGetLastError();
					if(ErrorCode == WSAENOBUFS)
					{
						// No resources, do nothing
						break;
					}
					else if (ErrorCode != WSAEWOULDBLOCK)
					{
						LogFile(CStrF("Socket error %d when trying to fetch packet data at offset %d", ErrorCode, m_ClientPartialPacket.m_iPacketPos));
						closesocket(m_ClientSocket);
						m_ClientSocket	= INVALID_SOCKET;
						break;
					}
				}
				else if(nRet == 0)
				{
					closesocket(m_ClientSocket);
					m_ClientSocket	= INVALID_SOCKET;
					break;
				}
				else
				{
					m_ClientPartialPacket.m_iPacketPos	+= nRet;
				}
			}

			if(m_ClientPartialPacket.m_iPacketPos == m_ClientPartialPacket.m_Size)
			{
				spPacket = MNew(TaskPacket);
				*spPacket	= m_ClientPartialPacket;
				m_ClientPartialPacket.Clear();
				m_ClientPacketState	= PACKET_STATE_NEWPACKET;
			}
			break;
		}
	}

	return spPacket;
}

void MRTC_TaskManager::Client_SendData(void* _pData, int _Size)
{
	int iPos = 0;
	while(1)
	{
		int nSize = send(m_ClientSocket, (const char*)_pData + iPos, _Size - iPos, 0);
		if(nSize > 0)
		{
			iPos	+= nSize;
			if(iPos == _Size)
				break;
		}
		else if(nSize == 0)
		{
			// Connection close from other side
			closesocket(m_ClientSocket);
			m_ClientSocket	= INVALID_SOCKET;
			break;
		}
		else
		{
			int SocketError = WSAGetLastError();
			if((SocketError == WSAEWOULDBLOCK) || (SocketError == WSAENOBUFS))
			{
				// socket full
				MRTC_SystemInfo::OS_Sleep(10);
			}
			else
			{
				// Lost connection
				closesocket(m_ClientSocket);
				m_ClientSocket	= INVALID_SOCKET;
				break;
			}
		}
	}
}

void MRTC_TaskManager::Client_SendPacket(int _Type, void* _pData, int _Size)
{
	char aHeader[8];
	memcpy(aHeader + 0, &_Type, 4);
	memcpy(aHeader + 4, &_Size, 4);
	Client_SendData(aHeader, 8);
	if(_Size > 0)
		Client_SendData(_pData, _Size);
}

void MRTC_TaskManager::Coord_SendData(void* _pData, int _Size)
{
	int iPos = 0;
	while(1)
	{
		int nSize = send(m_CoordinatorSocket, (const char*)_pData + iPos, _Size - iPos, 0);
		if(nSize > 0)
		{
			iPos	+= nSize;
			if(iPos == _Size)
				break;
		}
		else if(nSize == 0)
		{
			// Connection has been closed from other side
			closesocket(m_CoordinatorSocket);
			m_CoordinatorSocket	= INVALID_SOCKET;
			break;
		}
		else
		{
			int SocketError = WSAGetLastError();
			if((SocketError == WSAEWOULDBLOCK) || (SocketError == WSAENOBUFS))
			{
				// socket is full
				MRTC_SystemInfo::OS_Sleep(10);
			}
			else
			{
				LogFile(CStrF("Socket error %d when trying to send coordinator data", SocketError));
				// Lost connection
				closesocket(m_CoordinatorSocket);
				m_CoordinatorSocket	= INVALID_SOCKET;
				break;
			}
		}
	}
}

void MRTC_TaskManager::Coord_SendPacket(int _Type, void* _pData, int _Size)
{
	char aHeader[8];
	memcpy(aHeader + 0, &_Type, 4);
	memcpy(aHeader + 4, &_Size, 4);
	Coord_SendData(aHeader, 8);
	if(_Size > 0)
		Coord_SendData(_pData, _Size);
}

void MRTC_TaskManager::Host_SendData(spMRTC_TaskClient _spClient, void* _pData, int32 _Size)
{
	int iPos = 0;
	while(1)
	{
		int nSize = send(_spClient->m_Socket, (const char*)_pData + iPos, _Size - iPos, 0);
		if(nSize > 0)
		{
			iPos	+= nSize;
			if(iPos == _Size)
				break;
		}
		else if(nSize == 0)
		{
			// Connection has been closed from other side
			if(_spClient->m_spTask && _spClient->m_spTask->m_nFailures < 3)
			{
				LogFile(CStrF("ERROR: Lost connection to client '%s(%d)', adding assigned task to pending queue.", _spClient->m_HostName.GetStr(), _spClient->m_Socket));
				AddPendingRemoteTask(_spClient->m_spTask);
			}
			_spClient->Close();
			break;
		}
		else
		{
			int error = WSAGetLastError();
			if((error == WSAEWOULDBLOCK) || (error == WSAENOBUFS))
			{
				// socket is full
				MRTC_SystemInfo::OS_Sleep(10);
			}
			else
			{
				// Lost connection
				if(_spClient->m_spTask && _spClient->m_spTask->m_nFailures < 3)
				{
					LogFile(CStrF("ERROR: Unknown error occured on client '%s(%d)', adding assigned task to pending queue.", _spClient->m_HostName.GetStr(), _spClient->m_Socket));
					AddPendingRemoteTask(_spClient->m_spTask);
				}
				_spClient->Close();
				break;
			}
		}
	}
}

void MRTC_TaskManager::Host_SendPacket(spMRTC_TaskClient _spClient, int32 _Type, TArray<uint8>& _lData, int32* _pParams, int32 _nParams)
{
	Host_SendPacket(_spClient, _Type, _lData.GetBasePtr(), _lData.Len(), _pParams, _nParams);
}

void MRTC_TaskManager::Host_SendPacket(spMRTC_TaskClient _spClient, int32 _Type, void* _pData, int32 _Size, int32* _pParams, int32 _nParams)
{
	char aHeader[8];
	int32 Size = _Size;
	if(_pParams && _nParams > 0)
		Size += sizeof(int32) * _nParams;
	memcpy(aHeader, &_Type, 4);
	memcpy(aHeader + 4, &Size, 4);
	Host_SendData(_spClient, aHeader, 8);
	if(_pParams && _nParams > 0)
		Host_SendData(_spClient, _pParams, sizeof(int32) * _nParams);

	if(_Size > 0 )
		Host_SendData(_spClient, _pData, _Size);
}

void MRTC_TaskManager::ReleaseFileLock()
{
/*
	if(m_spFileLock)
	{
		m_spFileLock->Close();
		m_spFileLock	= NULL;
	}
*/
	if(m_hFileLock)
	{
		MRTC_SystemInfo::OS_MutexUnlock(m_hFileLock);
		MRTC_SystemInfo::OS_MutexClose(m_hFileLock);
		m_hFileLock = NULL;
	}
}

void MRTC_TaskManager::AcquireFileLock(CStr _Name)
{
	// Release whatever lock we already own
	ReleaseFileLock();
	m_hFileLock	= MRTC_SystemInfo::OS_MutexOpen(_Name.GetStr());
	MRTC_SystemInfo::OS_MutexLock(m_hFileLock);
/*
	while(1)
	{
		M_TRY
		{
			m_spFileLock = MNew(CCFile);
			m_spFileLock->Open(_Name + ".lock", CFILE_WRITE);
			break;
		}
		M_CATCH(
		catch(CCException)
		{
		}
		)
		m_spFileLock = NULL;
		MRTC_SystemInfo::OS_Sleep(100);
	}
*/
}

void MRTC_TaskManager::Client_UpdateNet()
{
	TPtr<TaskPacket> spPacket = Client_ReadPacket();
	if(spPacket == 0)
		return;

	switch(spPacket->m_Type)
	{
	case PACKET_TYPE_LOG:
		{
			// Does nothing, dummy traffic.
			break;
		}

	case PACKET_TYPE_PROGRESS:
		{
			// Does nothing
			break;
		}

	case PACKET_TYPE_TASKNAME:
		{
			// The runtime classname
			CStr TaskName;
			TaskName.Capture((char*)spPacket->m_lData.GetBasePtr(), spPacket->m_lData.Len());
			m_spClientTask = MRTC_TaskInstance::CreateTaskInstance(TaskName, MNew(MRTC_TaskArgRemoteTask));
			break;
		}

	case PACKET_TYPE_DATACHECKSUM:
		{
			// Load data from disk and check if it's the correct checksum
			MRTC_TaskArgRemoteTask* pArg = TDynamicCast<MRTC_TaskArgRemoteTask>((MRTC_TaskBaseArg*)m_spClientTask->m_spTaskArg);
			uint32 Everything = 0xffffffff;
//			bool bCleanCache = false;
			TArray<int32> lNeededData;

			AcquireFileLock(m_ClientCacheFilename);

			uint8* pData = spPacket->m_lData.GetBasePtr();
			int32 Packets = *((uint32*)pData); pData += 4;
			pArg->m_llTaskData.SetLen(Packets);

			if(CDiskUtil::FileExists(m_ClientCacheFilename))
			{
				Everything = 0;
				for(int i = 0; i < Packets; i++)
				{
					CMD5Digest HostMd5;
					memcpy(&HostMd5, pData, sizeof(HostMd5)); pData += sizeof(HostMd5);

					CStr ClientCacheFilename = m_ClientCacheFilename;
					if(i > 0)
						ClientCacheFilename += CStrF("%d", i);

					bool bNeedDownload = true;
					if(CDiskUtil::FileExists(ClientCacheFilename))
					{
						CMD5Digest md5;
						if(CDiskUtil::FileExists(ClientCacheFilename + ".md5"))
						{
							TArray<uint8> lMD5 = CDiskUtil::ReadFileToArray(ClientCacheFilename + ".md5", CFILE_READ);
							if(lMD5.Len() == sizeof(CMD5Digest))
								memcpy(&md5, lMD5.GetBasePtr(), sizeof(CMD5Digest));
							else
								md5 = GetFileMD5(m_ClientCacheFilename);
						}
						else
							md5 = GetFileMD5(m_ClientCacheFilename);

						if(!memcmp(&HostMd5, &md5, sizeof(md5)))
						{
							pArg->m_llTaskData[i]	= CDiskUtil::ReadFileToArray(ClientCacheFilename, CFILE_READ);
							bNeedDownload = false;
						}
					}

					if(bNeedDownload)
						lNeededData.Add(i);
				}
			}

//			if(bCleanCache)
//			{
//				if(CDiskUtil::FileExists(m_ClientCacheFilename))
//					CDiskUtil::DelFile(m_ClientCacheFilename);
//				if(CDiskUtil::FileExists(m_ClientCacheFilename + ".md5"))
//					CDiskUtil::DelFile(m_ClientCacheFilename + ".md5");
//			}

			if(lNeededData.Len() > 0)
			{
				Everything = lNeededData.Len();
				lNeededData.Insert(0, Everything);
				Client_SendPacket(PACKET_TYPE_REQUESTDATA, lNeededData.GetBasePtr(), lNeededData.ListSize());
			}
			else
				Client_SendPacket(PACKET_TYPE_REQUESTDATA, &Everything, sizeof(Everything));
			break;
		}

	case PACKET_TYPE_TASKDATA:
		{
			// data for a job
			MRTC_TaskArgRemoteTask* pArg = TDynamicCast<MRTC_TaskArgRemoteTask>((MRTC_TaskBaseArg*)m_spClientTask->m_spTaskArg);
			uint8* pData = spPacket->m_lData.GetBasePtr();
			int32 iPacket = *((uint32*)pData); pData += 4;
			pArg->m_llTaskData[iPacket].SetLen(spPacket->m_lData.Len() - 4);
			memcpy(pArg->m_llTaskData[iPacket].GetBasePtr(), pData, pArg->m_llTaskData[iPacket].Len());

			CStr ClientCacheFilename = m_ClientCacheFilename;
			if(iPacket > 0)
				ClientCacheFilename += CStrF("%d", iPacket);

			CDiskUtil::WriteFileFromArray(ClientCacheFilename, CFILE_WRITE, pArg->m_llTaskData[iPacket]);
			{
				CMD5 Md5;
				Md5.f_AddData(pArg->m_llTaskData[iPacket].GetBasePtr(), pArg->m_llTaskData[iPacket].Len());
				CMD5Digest md5(Md5);
				TArray<uint8> lMD5;
				lMD5.SetLen(sizeof(CMD5Digest));
				memcpy(lMD5.GetBasePtr(), &md5, sizeof(CMD5Digest));
				CDiskUtil::WriteFileFromArray(ClientCacheFilename + ".md5", CFILE_WRITE, lMD5);
			}
			break;
		}

	case PACKET_TYPE_TASKPARAM:
		{
			ReleaseFileLock();
			// Parameters for a job
			MRTC_TaskArgRemoteTask* pRemoteArg = TDynamicCast<MRTC_TaskArgRemoteTask>((MRTC_TaskBaseArg*)m_spClientTask->m_spTaskArg);
			pRemoteArg->m_lParamData	= spPacket->m_lData;
			break;
		}

	case PACKET_TYPE_TASKEXECUTE:
		{
			ReleaseFileLock();
			// Execute command, all data has been recieved
			MRTC_TaskArgRemoteTask* pRemoteArg = TDynamicCast<MRTC_TaskArgRemoteTask>((MRTC_TaskBaseArg*)m_spClientTask->m_spTaskArg);

			spMRTC_TaskInstance spReturnTask = MRTC_TaskInstance::CreateTaskInstance("MRTC_RemoteTaskReturn", m_spClientTask->m_spTaskArg);
			spReturnTask->DependOn(m_spClientTask);
			AddTask(spReturnTask);
			pRemoteArg->Create();

			AddTask(m_spClientTask);
			// Free the client task
			m_spClientTask	= 0;
			break;
		}

	case PACKET_TYPE_RESULT:
		{
			// Does nothing
			break;
		}
	}
}

bool MRTC_TaskManager::Client_IsAlive()
{
	return m_ClientSocket != INVALID_SOCKET;
}

void MRTC_TaskManager::Client_Log(CStr _Msg)
{
	if(_Msg.Len() > 0)
		Client_SendPacket(PACKET_TYPE_LOG, _Msg.GetStr(), _Msg.Len() + 1);
}

spMRTC_TaskClient MRTC_TaskManager::Host_GetFreeClient()
{
	spMRTC_TaskClient spClient;

	for(int i = 0; i < m_lspActiveClients.Len(); i++)
	{
		if(m_lspActiveClients[i] == 0)
			continue;

		if(m_lspActiveClients[i]->m_spTask == 0)
		{
			spClient	= m_lspActiveClients[i];
			break;
		}
	}

	return spClient;
}

int MRTC_TaskManager::Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int _Flags)
{
	spMRTC_RemoteTaskInstance spRemoteInstance = MNew(MRTC_RemoteTaskInstance);
	spRemoteInstance->Create(_TaskName, _lParam, _lData, _pTaskInstance, _pArg, _pTask, _pfnOnDistribute, _RamSize, _Flags);
	_pTaskInstance->DependOn(spRemoteInstance);
	return AddTask(spRemoteInstance);
}

int MRTC_TaskManager::Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<uint8> _lData, CMD5Digest _Checksum, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize, int _Flags)
{
	spMRTC_RemoteTaskInstance spRemoteInstance = MNew(MRTC_RemoteTaskInstance);
	spRemoteInstance->Create(_TaskName, _lParam, _lData, _Checksum, _pTaskInstance, _pArg, _pTask, _pfnOnDistribute, _RamSize, _Flags);
	_pTaskInstance->DependOn(spRemoteInstance);
	return AddTask(spRemoteInstance);
}

int MRTC_TaskManager::Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _llData, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize /* = 0 */, int _Flags /* = 0 */)
{
	spMRTC_RemoteTaskInstance spRemoteInstance = MNew(MRTC_RemoteTaskInstance);
	spRemoteInstance->Create(_TaskName, _lParam, _llData, _pTaskInstance, _pArg, _pTask, _pfnOnDistribute, _RamSize, _Flags);
	_pTaskInstance->DependOn(spRemoteInstance);
	return AddTask(spRemoteInstance);
}

int MRTC_TaskManager::Distribute(CStr _TaskName, TArray<uint8> _lParam, TArray<TArray<uint8> > _llData, TArray<CMD5Digest> _lChecksum, MRTC_TaskInstance* _pTaskInstance, MRTC_TaskBaseArg* _pArg, MRTC_RemoteTaskBase* _pTask, MRTC_RemoteTaskBase::OnDistribute _pfnOnDistribute, uint64 _RamSize /* = 0 */, int _Flags /* = 0 */)
{
	spMRTC_RemoteTaskInstance spRemoteInstance = MNew(MRTC_RemoteTaskInstance);
	spRemoteInstance->Create(_TaskName, _lParam, _llData, _lChecksum, _pTaskInstance, _pArg, _pTask, _pfnOnDistribute, _RamSize, _Flags);
	_pTaskInstance->DependOn(spRemoteInstance);
	return AddTask(spRemoteInstance);
}

int MRTC_TaskManager::GetMaxWorkingThreads()
{
	M_LOCK(m_Lock);
	return m_nMaxWorkingThreads;
}

int MRTC_TaskManager::GetActiveWorkingThreads()
{
	M_LOCK(m_Lock);
	return m_nActiveWorkingThreads;
}

void MRTC_TaskManager::IncrementActiveWorkingThreads(int32 _Count)
{
	M_LOCK(m_Lock);
	MRTC_SystemInfo::Atomic_Add(&m_nActiveWorkingThreads, _Count);
}

void MRTC_TaskManager::SetMaxWorkingThreads(int _Count)
{
	M_LOCK(m_Lock);
	m_nMaxWorkingThreads	= _Count;
	ThreadContainer.Create(_Count);
}
