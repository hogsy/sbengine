
#ifndef __MCONTENTCONTEXT_H_INCLUDED
#define __MCONTENTCONTEXT_H_INCLUDED

enum
{
	CONTENT_VOID = 0,

	CONTENT_SAVEGAMES,
	CONTENT_DOWNLOADABLE,

	CONTENT_MAX,

	CONTENT_OK = 0,
	CONTENT_ERR_PENDING,		// Request is pending
	CONTENT_ERR_UNKNOWN,		// This error only occurs when the code breaks (paths that should not be taken has been taken)
	CONTENT_ERR_INVALID_HANDLE,
	CONTENT_ERR_CONTAINER_NOT_FOUND,
	CONTENT_ERR_INSTANCE_NOT_FOUND,
	CONTENT_ERR_UNSUPPORTED,	// Request type is not valid for this function (non-enumeration for enumeration call)
	CONTENT_ERR_GENERIC_READ,	// Error while reading
	CONTENT_ERR_GENERIC_WRITE,	// Error while writing
};

class CContentQueryData
{
public:
	CStr m_Str;
	uint32 m_Data;
};

class CContentContext
{
public:
	virtual void Create() pure;
	virtual void Destroy() pure;
	virtual int StorageSelect(uint _iUser, uint _SaveSize) pure;

	virtual int EnumerateContainers(uint _iUser) pure;
	virtual int ContainerCreate(uint _iUser, CStr _Container) pure;
	virtual int ContainerDelete(uint _iUser, CStr _Container) pure;

	virtual int ContainerMount(uint _iUser, CStr _Container) pure;			// Used by downloadable content
	virtual int ContainerQueryPath(uint _iUser, CStr _Container) pure;		// Used by downloadable content
	virtual int ContainerQuerySize(uint _iUser, CStr _Container) pure;		// Used by downloadable content

	virtual int EnumerateInstances(uint _iUser, CStr _Container) pure;
	virtual int InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance) pure;
	virtual int InstanceDelete(uint _iUser, CStr _Container, CStr _Instance) pure;
	virtual int InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen) pure;
	virtual int InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len) pure;

	virtual int GetRequestStatus(int _iRequest) pure;
	virtual int BlockOnRequest(int _iRequest) pure;

	virtual int GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData) pure;	// Returns the enumeration data if the request was an enumeration.
	virtual int PeekRequestData(int _iRequest, CContentQueryData* _pRet) pure;		// Returns the request data if available without removing from context
	virtual int GetRequestData(int _iRequest, CContentQueryData* _pRet) pure;		// Returns the request data if available and removes it from the context
};

#endif
