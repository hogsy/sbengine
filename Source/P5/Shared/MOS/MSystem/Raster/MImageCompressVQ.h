/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Image compression using Vector Quantization (VQ)
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		030506:		Added comments
\*_____________________________________________________________________________________________*/
#ifndef _VQ_
#define _VQ_
#ifndef	PLATFORM_CONSOLE

#include <math.h>

class VQPixel {

public:
	float r, g, b, a;

	VQPixel();
	VQPixel( float r, float g, float b, float a );
	VQPixel( unsigned char r, unsigned char g, unsigned char b, unsigned char a );

	void Zero();


	inline VQPixel operator+( const VQPixel &v) const {
		return VQPixel( r + v.r, g + v.g, b + v.b, a + v.a );
	}

	inline VQPixel operator-( const VQPixel &v) const {
		return VQPixel( r - v.r, g - v.g, b - v.b, a - v.a );
	}

	inline float operator*( const VQPixel &v ) const {
		return r * v.r  +  g * v.g  +  b * v.b  +  a * v.a;
	}

	inline VQPixel operator*( const float &t ) const {
		return VQPixel( t * r, t * g, t * b, t * a );
	}

	inline const VQPixel & operator=( const unsigned int &t ) {
		r = t & 255;
		g = (t>>8) & 255;
		b = (t>>16) & 255;
		a = (t>>24) & 255;
		return *this;
	}

	inline const int operator == ( const VQPixel &v ) const {
		uint32 p0 = (int32(r)&255) | ((int32(g)&255)<< 8) | ((int32(b)&255)<< 16) | ((int32(a)&255)<< 24);
		uint32 p1 = (int32(v.r)&255) | ((int32(v.g)&255)<< 8) | ((int32(v.b)&255)<< 16) | ((int32(v.a)&255)<< 24);
		return p0 == p1;
	}

	inline bool AlmostEqual( const VQPixel &v, fp32 _range ) const {
		if( fabsf( r - v.r ) > _range ) return false;
		if( fabsf( g - v.g ) > _range ) return false;
		if( fabsf( b - v.b ) > _range ) return false;
		if( fabsf( a - v.a ) > _range ) return false;
		return true;
	}

	inline void Validate() {
		if( r < 0.f ) r = 0.f;
		else if( r > 255.f) r = 255.0f;
		if( g < 0.f ) g = 0.f;
		else if( g > 255.f) g = 255.0f;
		if( b < 0.f ) b = 0.f;
		else if( b > 255.f) b = 255.0f;
		if( a < 0.f ) a = 0.f;
		else if( a > 255.f) a = 255.0f;
	}
};




class VQVector {

public:
	VQPixel c[4];

	VQVector() {};
	VQVector( const VQPixel &v1, const VQPixel &v2, const VQPixel &v3, const VQPixel &v4 );

	void Zero();

	inline VQVector operator+( const VQVector &v ) const { // add
		return VQVector( c[0] + v.c[0], c[1] + v.c[1], c[2] + v.c[2], c[3] + v.c[3] );
	}

	inline VQVector operator-( const VQVector &v ) const { // subtract
		return VQVector( c[0] - v.c[0], c[1] - v.c[1], c[2] - v.c[2], c[3] - v.c[3] );
	}

	inline float operator*( const VQVector &v ) const { // multiply
		return c[0] * v.c[0] + c[1] * v.c[1] + c[2] * v.c[2] + c[3] * v.c[3];
	}

	inline VQVector operator*( float t ) const { // multiply with scalar
		return VQVector( c[0] * t, c[1] * t, c[2] * t, c[3] * t );
	}

	inline float operator~() const{ // length of vector squared
		return *this * *this;
	}

	inline int operator == ( const VQVector &_v ) const
	{
		for( int32 i = 0; i < 4; i++ )
			if( !(c[i] == _v.c[i]) )
				return false;
		return true;
	}

	inline bool AlmostEqual( const VQVector &_v, fp32 _range ) const
	{
		for( int32 i = 0; i < 4; i++ )
			if( !c[i].AlmostEqual( _v.c[i], _range ) )
				return false;
		return true;
	}

	inline void RandomModify( const fp32 _Step ) {
		for( int32 i = 0; i < 4; i++ )
		{
			float v[4];
			for( int32 j = 0; j < 4; j++ )
			{
				v[j] = _Step * ( MRTC_GETRAND()->GenRand1Inclusive_fp32() - 0.5f );
			}
			c[i] = c[i] + VQPixel( v[0], v[1], v[2], v[3] );
			c[i].Validate();
		}
	}

	inline void Validate() {
		for( int32 i = 0; i < 4; i++ )
			c[i].Validate();
	}

};





class TSVQ {

private:
	int			n, num;				// number of vectors in (sub)set, and leaf enumeration
	VQVector	*x;					// (sub)set of vectors

	VQVector	y;					// centroid of set
	float		r;					// distortion

	TSVQ		*u, *v;				// children


	static VQVector *vptr;


	int Leaf();						// is subtree a leaf

	TSVQ *MaxDist();
	void CreateNode( int n, VQVector *x );

	void Expand();

	void GetCodebook();
	TSVQ *Search( VQVector *q );

	int Enumerate( int num );

	int	TwiddleImage(int width, int height, VQVector *vim );

public:

	TSVQ();
	~TSVQ();
	TSVQ( int n, VQVector *set );
	TSVQ( int w, int h, int d, unsigned char *image );

	void GetCodebook( unsigned char *codebook );
	void GetImage( unsigned char *vqimage, unsigned char *rawimage, int w, int h, int d );

	void ConvertImage( VQVector *x, unsigned char *im, int w, int h, int d );
	void ConvertImage( unsigned char *im, VQVector *x, int w, int h, int d );

	void GLA_iter( unsigned char *vqimage, unsigned char *image, int w, int h, int d );
};

#endif	// PLATFORM_CONSOLE
#endif
