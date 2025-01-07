/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Force feedback envelopes
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/

#ifndef	__MINPUTENVELOPE_H_INCLUDED
#define	__MINPUTENVELOPE_H_INCLUDED

// -------------------------------------------------------------------
//  CInputEnvelopePoint
// -------------------------------------------------------------------
class CInputEnvelopePoint : public CReferenceCount
{
public:
	CInputEnvelopePoint();
	CInputEnvelopePoint( fp32 _fTime, fp32 _fValue );
	fp32	m_fValue;
	fp32	m_fTime;

	MACRO_OPERATOR_TPTR(CInputEnvelopePoint);
};

typedef TPtr<CInputEnvelopePoint> spCInputEnvelopePoint;

// -------------------------------------------------------------------
//  CInputEnvelopeChannel
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CInputEnvelopeChannel : public CReferenceCount
{
	friend class CInputEnvelope;
private:
	void AddPoint(const fp32 _fTime, const fp32 _fValue);
public:
	int32 m_ID;
	TArray<spCInputEnvelopePoint> m_lPoints;

	fp32 GetFeedbackForce(fp32 _fTime);
};

typedef TPtr<CInputEnvelopeChannel> spCInputEnvelopeChannel;

// -------------------------------------------------------------------
//  CInputEnvelope
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CInputEnvelope : public CReferenceCount
{
protected:
public:
	CInputEnvelope();
	TArray<spCInputEnvelopeChannel> m_lChannels;
	
	void Create(const CStr &_name);

	spCInputEnvelopeChannel GetChannel(int32 _Channel=0);
	fp32 GetFeedbackForce(fp32 _fTime, int32 _Channel=0);
	void AddPoint(const fp32 _fTime, const fp32 _fValue, int32 _Channel);

	CStr	m_Name;		// Name of envelope
	fp32		m_fEndTime;	// To speed up removal of envelopes

	MACRO_OPERATOR_TPTR(CInputEnvelope)
};

typedef TPtr<CInputEnvelope> spCInputEnvelope;

// -------------------------------------------------------------------
//  CInputEnvelopeInstance
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CInputEnvelopeInstance : public CReferenceCount
{
protected:
	CMTime	m_fStartTime;		// When was this envelope triggered
	fp32		m_fCeiling;			// Where does the envelopes ceiling lie

public:
	CInputEnvelope  *m_pEnvelope;
	
	CInputEnvelopeInstance( );
	CInputEnvelopeInstance(CInputEnvelope * );
	fp32 GetFeedbackForce(CMTime _fTime, int32 _Channel=0);
	bool IsActive( const CMTime _fTime );

	MACRO_OPERATOR_TPTR(CInputEnvelopeInstance)
};

typedef TPtr<CInputEnvelopeInstance> spCInputEnvelopeInstance;

// -------------------------------------------------------------------
//  CInputEnvelopeInstanceList
// -------------------------------------------------------------------
class CInputEnvelopeInstanceList : public CReferenceCount
{
protected:
	TArray<spCInputEnvelopeInstance> m_Instances;
public:
	CInputEnvelopeInstanceList();
	spCInputEnvelopeInstance AppendEnvelope(CInputEnvelope *_pEnvelope);
	spCInputEnvelopeInstance SetEnvelope(CInputEnvelope *_pEnvelope);
	void FlushEnvelopes();
	void RemoveEnvelope(CInputEnvelopeInstance *_pEnvelope );

	fp32 GetFeedbackForce(CMTime _fTime, int32 _Channel=0);
	void Update();

	MACRO_OPERATOR_TPTR(CInputEnvelopeInstanceList)
};

typedef TPtr<CInputEnvelopeInstanceList> spCInputEnvelopeInstanceList;

// -------------------------------------------------------------------
//  CPlayerInputEnvelopeInstanceList
// -------------------------------------------------------------------
class CPlayerInputEnvelopeInstanceList
{
protected:
	TArray<spCInputEnvelopeInstanceList>	m_PlayerLists;
public:
	CPlayerInputEnvelopeInstanceList();
	void Create();

	spCInputEnvelopeInstance AppendEnvelope(const int _index, CInputEnvelope *_pEnvelope);
	spCInputEnvelopeInstance SetEnvelope(const int _index, CInputEnvelope *_pEnvelope);
	void FlushEnvelopes( );
	void FlushEnvelopes( const int _index );
	void RemoveEnvelope( const int _index, CInputEnvelopeInstance *_pEnvelope);

	fp32 GetFeedbackForce( int _index, CMTime _fTime, int32 _Channel=0);
	void Update();
};

// -------------------------------------------------------------------
//  CInputEnvelopeList
// -------------------------------------------------------------------
class CInputEnvelopeList
{
protected:
	TArray<spCInputEnvelope>	m_Envelopes;
public:
	CInputEnvelopeList();
	void Create();

	spCInputEnvelope			FindEnvelope( const CStr &_name );
};

#endif	// __MINPUTENVELOPE_H_INCLUDED
