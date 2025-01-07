/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Generic fixedpoint arithmetic class, not finished!
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Created File
\*_____________________________________________________________________________________________*/

#ifndef __MFIXED_H_INCLUDED
#define __MFIXED_H_INCLUDED

#undef double

template <int UpperSize, int LowerSize>
class TFixed
{
//	int64	m_Value;
	int32	m_UpperValue : UpperSize;
	uint32	m_LowerValue : LowerSize;

	M_INLINE TFixed<UpperSize,LowerSize>& SetValue( int64 _value )
	{
//		m_Value = _value;
		m_UpperValue = ( _value >> LowerSize ) & ( ( (uint32)1 << UpperSize ) - 1 );
		m_LowerValue = _value & ( ( (uint32)1 << LowerSize ) - 1 );

		return *this;
	}

	M_INLINE int64 GetValue( ) const
	{
		return ( (int64)m_UpperValue << LowerSize ) | ( (int64)m_LowerValue );
	}
public:
	M_INLINE TFixed<UpperSize,LowerSize>() : m_UpperValue( 0 ), m_LowerValue( 0 ) {}

	M_INLINE TFixed<UpperSize,LowerSize>( const int8 _value )   { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const int16 _value )  { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const int32 _value )  { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const int64 _value )  { SetValue( _value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const uint8 _value )  { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const uint16 _value ) { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const uint32 _value ) { SetValue( (int64)_value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const uint64 _value ) { SetValue( _value << LowerSize ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const float _value )  { SetValue( _value * ( ((int64)1) << LowerSize ) ); }
	M_INLINE TFixed<UpperSize,LowerSize>( const double _value ) { SetValue( _value * ( ((int64)1) << LowerSize ) ); }

	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 a = _a.GetValue(), b = _b.GetValue();
		return TFixed<UpperSize,LowerSize>(0).SetValue( a + b );
	}

	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 a = _a.GetValue(), b = _b.GetValue();
		return TFixed<UpperSize,LowerSize>(0).SetValue( a - b );
	}

	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 av = _a.GetValue(), bv = _b.GetValue();
		int64 a, b, c;

		a = ( av >> LowerSize ) * bv;
		b = av & ( ( ((int64)1) << LowerSize ) - 1 );
		c = av >> LowerSize;
		b = b * c + ( ( b * ( bv & ( ( ((int64)1) << LowerSize ) - 1 ) ) ) >> LowerSize );

		return TFixed<UpperSize,LowerSize>(0).SetValue( a + b );
	}

	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 av = _a.GetValue(), bv = _b.GetValue();
		int64 a, b;

		a = ( ( ( av & ( 0xffffffffffffffff ^ ( ( ((int64)1) << LowerSize ) - 1 ) ) ) / bv ) << LowerSize );
		b = ( ( ( av & ( ( ((int64)1) << LowerSize ) - 1 ) ) << LowerSize ) / bv );

		return TFixed<UpperSize,LowerSize>(0).SetValue( a + b );
	}

	M_INLINE TFixed<UpperSize,LowerSize>& operator += ( const TFixed<UpperSize,LowerSize>& _b )
	{
//		m_Value += _b.m_Value;
		SetValue( GetValue() + _b.GetValue() );
		return *this;
	}

	M_INLINE TFixed<UpperSize,LowerSize>& operator -= ( const TFixed<UpperSize,LowerSize>& _b )
	{
		SetValue( GetValue() - _b.GetValue() );
//		m_Value -= _b.m_Value;
		return *this;
	}

	M_INLINE TFixed<UpperSize,LowerSize>& operator *= ( const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 av = GetValue(), bv = _b.GetValue();
		int64 a, b, c;

		a = ( av >> LowerSize ) * bv;
		b = av & ( ( ((int64)1) << LowerSize ) - 1 );
		c = bv >> LowerSize;
		b = b * c + ( ( b * ( bv & ( ( ((int64)1) << LowerSize ) - 1 ) ) ) >> LowerSize );

		SetValue( a + b );

		return *this;
	}

	M_INLINE TFixed<UpperSize,LowerSize>& operator /= ( const TFixed<UpperSize,LowerSize>& _b )
	{
		int64 av = GetValue(), bv = _b.GetValue();
		int64 a, b;

		a = ( ( ( av & ( 0xffffffffffffffff ^ ( ( ((int64)1) << LowerSize ) - 1 ) ) ) / bv ) << LowerSize );
		b = ( ( ( av & ( ( ((int64)1) << LowerSize ) - 1 ) ) << LowerSize ) / bv );

		SetValue( a + b );

		return *this;
	}

	M_INLINE TFixed<UpperSize,LowerSize> operator ++ () { int64 v = GetValue() + 1; SetValue( v ); return TFixed<UpperSize,LowerSize>( 0 ).SetValue( v ); }
	M_INLINE TFixed<UpperSize,LowerSize> operator -- () { int64 v = GetValue() - 1; SetValue( v ); return TFixed<UpperSize,LowerSize>( 0 ).SetValue( v ); }
	M_INLINE TFixed<UpperSize,LowerSize> operator ++ (int _a) { int64 v = GetValue() + 1; SetValue( v ); return TFixed<UpperSize,LowerSize>( 0 ).SetValue( v - 1 ); }
	M_INLINE TFixed<UpperSize,LowerSize> operator -- (int _a) { int64 v = GetValue() - 1; SetValue( v ); return TFixed<UpperSize,LowerSize>( 0 ).SetValue( v + 1 ); }

	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() != _b.GetValue() ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() == _b.GetValue() ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() < _b.GetValue() ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() > _b.GetValue() ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() <= _b.GetValue() ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( _a.GetValue() >= _b.GetValue() ); }

#define OP_FIXED_CONST_PLUS( x )			M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_MINUS( x )			M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_MULTIPLICATION( x )	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_DIVISION( x )		M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_GT( x )				M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_LT( x )				M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_GE( x )				M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_LE( x )				M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_EQ( x )				M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
#define OP_FIXED_CONST_NE( x )				M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const x& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
#define OP_CONST_FIXED_PLUS( x )			M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
#define OP_CONST_FIXED_MINUS( x )			M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
#define OP_CONST_FIXED_MULTIPLICATION( x )	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
#define OP_CONST_FIXED_DIVISION( x )		M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
#define OP_CONST_FIXED_GT( x )				M_INLINE friend bool operator > ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
#define OP_CONST_FIXED_LT( x )				M_INLINE friend bool operator < ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
#define OP_CONST_FIXED_GE( x )				M_INLINE friend bool operator >= ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
#define OP_CONST_FIXED_LE( x )				M_INLINE friend bool operator <= ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
#define OP_CONST_FIXED_EQ( x )				M_INLINE friend bool operator == ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
#define OP_CONST_FIXED_NE( x )				M_INLINE friend bool operator != ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
#define OP_CONST_FIXED_ADD( x )				M_INLINE friend x operator += ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ); }
#define OP_CONST_FIXED_SUB( x )				M_INLINE friend x operator -= ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ); }
#define OP_CONST_FIXED_MULTIPLY( x )		M_INLINE friend x operator *= ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ); }
#define OP_CONST_FIXED_DIVIDE( x )			M_INLINE friend x operator /= ( const x& _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ); }
	
#define	OP_OVERLOADS( x )\
	OP_FIXED_CONST_PLUS( x )\
	OP_FIXED_CONST_MINUS( x )\
	OP_FIXED_CONST_MULTIPLICATION( x )\
	OP_FIXED_CONST_DIVISION( x )\
	OP_FIXED_CONST_EQ( x )\
	OP_FIXED_CONST_NE( x )\
	OP_FIXED_CONST_GT( x )\
	OP_FIXED_CONST_LT( x )\
	OP_FIXED_CONST_GE( x )\
	OP_FIXED_CONST_LE( x )\
	OP_CONST_FIXED_PLUS( x )\
	OP_CONST_FIXED_MINUS( x )\
	OP_CONST_FIXED_MULTIPLICATION( x )\
	OP_CONST_FIXED_DIVISION( x )\
	OP_CONST_FIXED_EQ( x )\
	OP_CONST_FIXED_NE( x )\
	OP_CONST_FIXED_GT( x )\
	OP_CONST_FIXED_LT( x )\
	OP_CONST_FIXED_GE( x )\
	OP_CONST_FIXED_LE( x )\
	OP_CONST_FIXED_ADD( x )\
	OP_CONST_FIXED_SUB( x )\
	OP_CONST_FIXED_MULTIPLY( x )\
	OP_CONST_FIXED_DIVIDE( x )

	OP_OVERLOADS( int8  )
	OP_OVERLOADS( int16 )
	OP_OVERLOADS( int32)
	OP_OVERLOADS( int64 )
	OP_OVERLOADS( uint8  )
	OP_OVERLOADS( uint16 )
	OP_OVERLOADS( uint32 )
	OP_OVERLOADS( uint64 )
	OP_OVERLOADS( float )
	OP_OVERLOADS( double )

/*
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a != TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator == ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a == TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator != ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }
	M_INLINE friend bool operator != ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) != _b; }
	M_INLINE friend bool operator == ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) == _b; }

	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a < TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator > ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a > TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator < ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }
	M_INLINE friend bool operator < ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) < _b; }
	M_INLINE friend bool operator > ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) > _b; }

	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const uint64& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const uint32& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const uint16& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a <= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator >= ( const TFixed<UpperSize,LowerSize>& _a, const uint8& _b ) { return _a >= TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend bool operator <= ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const uint8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const uint16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const uint32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const uint64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }
	M_INLINE friend bool operator <= ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) <= _b; }
	M_INLINE friend bool operator >= ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) >= _b; }

	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const float& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const double& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const int8& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const int16& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const int32& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a - TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a + TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a * TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const TFixed<UpperSize,LowerSize>& _a, const int64& _b ) { return _a / TFixed<UpperSize,LowerSize>( _b ); }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const int8& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const int16& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const int32& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const int64& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const float& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator - ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) - _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator + ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) + _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator * ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) * _b; }
	M_INLINE friend TFixed<UpperSize,LowerSize> operator / ( const double& _a, const TFixed<UpperSize,LowerSize>& _b ) { return TFixed<UpperSize,LowerSize>( _a ) / _b; }

	M_INLINE friend int8 operator += ( const int8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToInt8(); }
	M_INLINE friend int8 operator -= ( const int8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToInt8(); }
	M_INLINE friend int8 operator *= ( const int8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToInt8(); }
	M_INLINE friend int8 operator /= ( const int8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToInt8(); }
	M_INLINE friend uint8 operator += ( const uint8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToUint8(); }
	M_INLINE friend uint8 operator -= ( const uint8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToUint8(); }
	M_INLINE friend uint8 operator *= ( const uint8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToUint8(); }
	M_INLINE friend uint8 operator /= ( const uint8 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToUint8(); }
	M_INLINE friend int16 operator += ( const int16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToInt16(); }
	M_INLINE friend int16 operator -= ( const int16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToInt16(); }
	M_INLINE friend int16 operator *= ( const int16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToInt16(); }
	M_INLINE friend int16 operator /= ( const int16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToInt16(); }
	M_INLINE friend uint16 operator += ( const uint16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToUint16(); }
	M_INLINE friend uint16 operator -= ( const uint16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToUint16(); }
	M_INLINE friend uint16 operator *= ( const uint16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToUint16(); }
	M_INLINE friend uint16 operator /= ( const uint16 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToUint16(); }
	M_INLINE friend int32 operator += ( const int32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToInt32(); }
	M_INLINE friend int32 operator -= ( const int32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToInt32(); }
	M_INLINE friend int32 operator *= ( const int32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToInt32(); }
	M_INLINE friend int32 operator /= ( const int32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToInt32(); }
	M_INLINE friend uint32 operator += ( const uint32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToUint32(); }
	M_INLINE friend uint32 operator -= ( const uint32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToUint32(); }
	M_INLINE friend uint32 operator *= ( const uint32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToUint32(); }
	M_INLINE friend uint32 operator /= ( const uint32 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToUint32(); }
	M_INLINE friend int64 operator += ( const int64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToInt64(); }
	M_INLINE friend int64 operator -= ( const int64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToInt64(); }
	M_INLINE friend int64 operator *= ( const int64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToInt64(); }
	M_INLINE friend int64 operator /= ( const int64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToInt64(); }
	M_INLINE friend uint64 operator += ( const uint64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToUint64(); }
	M_INLINE friend uint64 operator -= ( const uint64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToUint64(); }
	M_INLINE friend uint64 operator *= ( const uint64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToUint64(); }
	M_INLINE friend uint64 operator /= ( const uint64 _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToUint64(); }
	M_INLINE friend float operator += ( const float _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToFloat(); }
	M_INLINE friend float operator -= ( const float _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToFloat(); }
	M_INLINE friend float operator *= ( const float _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToFloat(); }
	M_INLINE friend float operator /= ( const float _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToFloat(); }
	M_INLINE friend double operator += ( const double _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) + _b ).ToDouble(); }
	M_INLINE friend double operator -= ( const double _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) - _b ).ToDouble(); }
	M_INLINE friend double operator *= ( const double _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) * _b ).ToDouble(); }
	M_INLINE friend double operator /= ( const double _a, const TFixed<UpperSize,LowerSize>& _b ) { return ( TFixed<UpperSize,LowerSize>( _a ) / _b ).ToDouble(); }
*/
	M_INLINE TFixed<UpperSize,LowerSize> operator -() const { return TFixed<UpperSize,LowerSize>(0).SetValue( -GetValue() ); }
	M_INLINE operator int8 () const { return ToInt8(); }
	M_INLINE operator uint8 () const { return ToUint8(); }
	M_INLINE operator int16 () const { return ToInt16(); }
	M_INLINE operator uint16 () const { return ToUint16(); }
	M_INLINE operator int32 () const { return ToInt32(); }
	M_INLINE operator uint32 () const { return ToUint32(); }
	M_INLINE operator int64 () const { return ToInt64(); }
	M_INLINE operator uint64 () const { return ToUint64(); }
	M_INLINE operator float () const { return ToFloat(); }
	M_INLINE operator double () const { return ToDouble(); }
	M_INLINE operator bool () const { return GetValue() == 0; };

	M_INLINE float ToFloat() const
	{
		return (float)( GetValue() >> LowerSize ) + ( ( GetValue() & (((int64)1)<<LowerSize)-1) / (float)(((int64)1)<<LowerSize) );
	}

	M_INLINE double ToDouble() const
	{
		return (double)( GetValue() >> LowerSize ) + ( ( GetValue() & (((int64)1)<<LowerSize)-1) / (double)(((int64)1)<<LowerSize) );
	}

	M_INLINE int8 ToInt8() const { return (int8)( GetValue() >> LowerSize ); }
	M_INLINE uint8 ToUint8() const { return (uint8)( GetValue() >> LowerSize ); }
	M_INLINE int16 ToInt16() const { return (int16)( GetValue() >> LowerSize ); }
	M_INLINE uint16 ToUint16() const { return (uint16)( GetValue() >> LowerSize ); }
	M_INLINE int32 ToInt32() const { return (int32)( GetValue() >> LowerSize ); }
	M_INLINE uint32 ToUint32() const { return (uint32)( GetValue() >> LowerSize ); }
	M_INLINE int64 ToInt64() const { return (int64)( GetValue() >> LowerSize ); }
	M_INLINE uint64 ToUint64() const { return (uint64)( GetValue() >> LowerSize ); }
};

#endif //__MFIXED_H_INCLUDED
