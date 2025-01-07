
#include "PCH.h"
#include "MImage.h"
#include "MTextureContainers.h"

#ifndef	PLATFORM_CONSOLE
#ifndef IMAGE_IO_NOVQ	
#include "MImageCompressVQ.h"

#define COMPRESS_VQ_CLUT

#undef LogFile
#define LogFile(_Msg) LogToSystemLog(_Msg)



VQPixel::VQPixel() {}

void VQPixel::Zero()
{
	r = g = b = a = 0.f;
}

VQPixel::VQPixel( float r, float g, float b, float a )
{
	this->r = r; this->g = g; this->b = b; this->a = a;
}

VQPixel::VQPixel( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
	this->r = (float)r; this->g = (float)g; this->b = (float)b; this->a = (float)a;
}



void VQVector::Zero()
{
	c[0].Zero();	c[1].Zero();	c[2].Zero();	c[3].Zero();
}

VQVector::VQVector( const VQPixel &v1, const VQPixel &v2, const VQPixel &v3, const VQPixel &v4 )
{
	c[0] = v1; c[1] = v2; c[2] = v3; c[3] = v4;
}




VQVector *TSVQ::vptr;

void TSVQ::ConvertImage( VQVector *x, unsigned char *im, int w, int h, int d )
{
	for( int j = 0; j < h; j++ )
	{
		for( int i = 0; i < w; i++ )
		{
			unsigned char r,g,b,a;

			if( d == 24 )
			{
				r = *im++;
				g = *im++;
				b = *im++;
				a = 255;
			}
			else if( d == 32 )
			{
				r = *im++;
				g = *im++;
				b = *im++;
				a = *im++;
			}

#ifdef COMPRESS_VQ_CLUT
			// 1x4 vector
			int xi = i >> 2;
			int yi = j * (w >> 2);
			x[xi + yi].c[ i & 3 ] = VQPixel(r,g,b,a);
#else
			// 2x2 vector
			int xi = i >> 1;
			int yi = (j >> 1) * (w >> 1);
			x[xi + yi].c[ (i & 1) + ((j & 1) << 1) ] = VQPixel(r,g,b,a);
#endif

		}
	}
}

void TSVQ::ConvertImage( unsigned char *im, VQVector *x, int w, int h, int d )
{
	int i,j;

	for( j = 0; j < h; j++ )
	{
		for( i = 0; i < w; i++ )
		{
			VQPixel p;
			unsigned char r,g,b,a;


#ifdef COMPRESS_VQ_CLUT
			// 1x4 vectors
			int xi = i >> 2;
			int yi = j * (w >> 2);
			p = x[xi + yi].c[ i & 3 ];
#else
			//	2x2 vectors
			int xi = i >> 1;
			int yi = (j >> 1) * (w >> 1);
			p = x[xi + yi].c[ (i & 1) + ((j & 1) << 1) ];
#endif

			r = (unsigned int)p.r;
			g = (unsigned int)p.g;
			b = (unsigned int)p.b;
			a = (unsigned int)p.a;

			if( d == 24 )
			{
				*im++ = r;
				*im++ = g;
				*im++ = b;
			}
			else if( d == 32 )
			{
				*im++ = r;
				*im++ = g;
				*im++ = b;
				*im++ = a;
			}
		}
	}
}



TSVQ::TSVQ() {}

TSVQ::~TSVQ()
{ // destructor
	if( !Leaf() )
	{ // delete both subtrees
		delete u;
		delete v;
	}
}

TSVQ::TSVQ( int n, VQVector *x )
{
	CreateNode( n, x );
}




TSVQ::TSVQ( int width, int height, int depth, unsigned char *image )
{
	int im_size = width	* height / 4;
	VQVector *vim = DNew(VQVector) VQVector[im_size];
	if( !vim ) Error_static( "TSVQ::TSVQ", "Out of memory" );

	ConvertImage( vim, image, width, height, depth );

#ifdef COMPRESS_VQ_CLUT
	TwiddleImage( width, height, vim );
#endif

	CreateNode( im_size, vim );

	for( int size = 1; size < 256; size++ )
	{ // codebook complete when size = 256

		 // select and expand leaf with maximum distortion

		TSVQ *node = MaxDist();
		node->Expand();
	}

	Enumerate( 0 );
	
	delete[] vim;
}

void TSVQ::CreateNode( int n, VQVector *x )
{
	int i;

	u = v = 0;
	this->n = n; this->x = x;

	// compute centroid -> y = E(x)
	for( i = 0, y.Zero(); i < n; i++ )
		y = y + x[i];

	y = y * (1.f / n);


	// compute distortion -> d = E(|y-x|^2)
	for( i = 0, r = 0.f; i < n; i++ )
		r += ~(y - x[i]);

	r /= n;
}


int TSVQ::Leaf()
{
	if( u || v ) { // children -> subtree is not a leaf
		return 0;
	}
	else { // no children -> subtree is a leaf
		return 1;
	}
}


TSVQ *TSVQ::MaxDist()
{ // return a pointer to leaf with maximum distortion

	if( Leaf() )
	{ // a leaf is the node with max distortion of subtree
		return this;
	}
	else
	{ // recursive definition, max dist is max of both subtrees max dist
		TSVQ *s, *t;

		s = u->MaxDist();
		t = v->MaxDist();

		if( s->r >= t->r )
		{
			return s;
		}
		else
		{
			return t;
		}
	}
}


int TSVQ::Enumerate( int num )
{ // enumerate leafs left to right
	if( Leaf() )
	{
		this->num = num;
		return num + 1;
	}
	else
	{
		return v->Enumerate( u->Enumerate(num) );
	}
}


void TSVQ::Expand()
{
	int i, un, vn;

	VQVector *ux = DNew(VQVector) VQVector[n];
	VQVector *vx = DNew(VQVector) VQVector[n];
	if( !ux || !vx ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );


	// General Loyd Algorithm

	VQVector uy = y, vy = x[0];		// initial codebook...

	for( i = 0; i < n; i++ )
	{
		if( ~(x[i] - y) < ~(vy - y) )
			vy = x[i];
	}

	vy = vy * (0.001f) + y * (1.f - 0.001f);	// ...and a slightly perturbed centroid

	VQVector ouy = uy, ovy = vy;	// "old" codebook, used to determine convergence

	do 
	{ // iterate until no change
		ouy = uy; ovy = vy;

		un = 0; vn = 0;

		for( i = 0; i < n; i++ )
		{ // partition set x into subsets ux and vx
			if( ~(x[i] - uy) <= ~(x[i] - vy) )
			{ // closest distance of x[i] is to centroid of u
				ux[un++] = x[i];
			}
			else
			{ // closest distance of x[i] is to centroid of v
				vx[vn++] = x[i];
			}
		}

		for( i = 0, uy.Zero(); i < un; i++ )
		{ // compute optimal codebook entry uy for set ux
			uy = uy + ux[i];
		}
		uy = uy * (1.f / un);

		for( i = 0, vy.Zero(); i < vn; i++ )
		{ // compute optimal codebook entry vy for set vx
			vy = vy + vx[i];
		}
		vy = vy * (1.f / vn);

	} while( ~(uy - ouy) + ~(vy - ovy) > 0.1f );
	
	// use in-place memory for subtrees, taking
	// advantage of the fast that subsets are completely disjunct
	// and only subsets of the leafs are used

	VQVector *pek = x;

	for( i = 0; i < un; i++ )
		*pek++ = ux[i];

	for( i = 0; i < vn; i++ )
		*pek++ = vx[i];
	
	delete[] ux;
	delete[] vx;

	ux = x; vx = &x[un];

	// create subtrees

	u = DNew(TSVQ) TSVQ( un, ux );
	v = DNew(TSVQ) TSVQ( vn, vx );

	if( !u ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );
	if( !v ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );
}


void TSVQ::GetCodebook( unsigned char *codebook )
{
	VQVector book[256];

	vptr = book;
	GetCodebook();

	for( int i = 0; i < 256; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			*codebook++ = (unsigned char)book[i].c[j].r;
			*codebook++ = (unsigned char)book[i].c[j].g;
			*codebook++ = (unsigned char)book[i].c[j].b;
			*codebook++ = (unsigned char)book[i].c[j].a;
		}
	}

}

void TSVQ::GetCodebook()
{
	if( Leaf() )
	{
		*vptr++ = y;
	}
	else
	{ // left to right leaf extraction
		u->GetCodebook();
		v->GetCodebook();
	}
}


void TSVQ::GetImage( unsigned char *vqimage, unsigned char *rawimage, int w, int h, int d )
{
	int i, n = w / 2 * h / 2;

	VQVector *image = DNew(VQVector) VQVector[n];
	if( !image ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );

	ConvertImage( image, rawimage, w, h, d );
#ifdef COMPRESS_VQ_CLUT
	TwiddleImage( w, h, image );
#endif

	for( i = 0; i < n; i++ )
	{
		int entry = Search( &image[i] )->num;
		*vqimage++ = entry;
	}

	delete[] image;
}


TSVQ *TSVQ::Search( VQVector *q )
{
	if( Leaf() )
	{
		return this;
	}
	else
	{
		if( ~(*q - u->y) <= ~(*q - v->y) )
		{ // left subtree
			return u->Search( q );
		}
		else
		{ // right subtree
			return v->Search( q );
		}
	}
}

void TSVQ::GLA_iter( unsigned char *_vqimage, unsigned char *image, int w, int h, int d )
{ // General Loyd Algorithm - can be used as postprocess


	int i,j,k;
	int num[256];							// number of vectors for each codebook entry
	VQVector book[256];						// input codebook
	VQVector cb[256];						// new codebook
	VQVector *vim = DNew(VQVector) VQVector[w*h/4];	// uncompressed image in VQVector format
	fp4 dist[256];

	if( !vim ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );

	ConvertImage( vim, image, w, h, d );
	TwiddleImage( w, h, vim );

	w >>= 1; h >>= 1;						// 2x2 pixels for each entry

	fp4 olddist = 999999999.f;
	fp4 newdist;


	while( 1 )
	{
		unsigned char *im = _vqimage;
		unsigned char *vqimage = _vqimage;

		newdist = 0.f;

		for( i = 0; i < 256; i++ )
		{ // convert codebook into VQVectors
			num[i] = 0;

			cb[i].Zero();
			dist[ i ] = 0.f;

			for( int j = 0; j < 4; j++ )
			{
				book[i].c[j].r = *im++;
				book[i].c[j].g = *im++;
				book[i].c[j].b = *im++;
				book[i].c[j].a = *im++;
			}
		}

		for( i = 0, j = w*h; i < j; i++ )
		{
			VQVector &v = vim[i];
			int iMin = 0;
			float fMin = ~(v - book[iMin]);

			fp4 diff;
			for( k = 0; k < 256; k++ )
			{ // find best match in codebook
				diff = ~(v - book[k]);
				if( diff < fMin )
				{
					iMin = k;
					fMin = diff;
				}
			}

			num[iMin] += 1;
			cb[iMin] = cb[iMin] + v; // book[iMin];

			dist[iMin] = fMin;
			newdist += dist[iMin];
			*im++ = iMin;	// output new codebook entry (best match index)
		}
 
		// expand unused book nodes
		for( i = 0; i < 256; i++ )
		{
			if( num[i] )
			{
				book[i] = cb[i] * (1.f / num[i] );
			}
			else
			{
				// find node with highest distortion
				int32 iMax = 0;
				for( j = 0; j < 256; j++ )
					if( dist[j] > dist[iMax] )
						iMax = j;

				// take iMax and randomize
				book[i] = book[iMax];
				book[i].RandomModify( 1.0f );
				dist[iMax] = M_Sqrt(dist[iMax]); 
			}
		}

		// modify equal nodes
		for( i = 0; i < 256; i++ )
			for( j = i+1; j < 256; j++ )
				if( book[i] == book[j] )
					book[j].RandomModify( 1.0f );


		for( i = 0; i < 256; i++ )
		{ // output new codebook!
			book[i].Validate();
			for( j = 0; j < 4; j++ )
			{
				*vqimage++ = (unsigned char)book[i].c[j].r;
				*vqimage++ = (unsigned char)book[i].c[j].g;
				*vqimage++ = (unsigned char)book[i].c[j].b;
				*vqimage++ = (unsigned char)book[i].c[j].a;
			}
		}

		if( newdist + 0.0001f > olddist )
			break; 
		olddist = newdist;
	}

	
	delete[] vim;
}


#endif	// IMAGE_IO_NOVQ



void CImage::ClutVQ(CTexture *_pTexture)
{
	if( !( (GetMemModel() & IMAGE_MEM_COMPRESSED) && (GetMemModel() & IMAGE_MEM_COMPRESSTYPE_VQ)))
	{ // cant conjure up a clut for the VQ image codebook unless it is a truecolor VQ image!
		return;
	}

	void *image = LockCompressed();

	int i;
	unsigned char clut[256*4];

	for( i = 0; i < 256; i++ )
		((int *)clut)[i] = ((int *)image)[i*4];

	Unlock();
	
	m_spPalette = MNew(CImagePalette);
	m_spPalette->SetPalette((CPixel32 *)clut, 0, 256);

	ClutVQ( clut ); // transform truecolor code book into palette code book associated with clut

	_pTexture->m_LargestMap.SetPalette(m_spPalette);

}

void CImage::ClutVQ( unsigned char *clut )
{ // transform truecolor code book into palette code book associated with clut

	void *image = LockCompressed();

	unsigned char *sptr = (unsigned char *)image;
	unsigned char *dptr = (unsigned char *)image;

	for( int i = 0; i < 256*4; i++ )
	{
		unsigned char min = 0;
		VQPixel u, v;

		u.r = *sptr++; u.g = *sptr++; u.b = *sptr++; u.a = *sptr++;
		v.r = clut[0]; v.g = clut[1]; v.b = clut[2]; v.a = clut[3];

		float min_dist = (u - v) * (u - v);

		for( int j = 1; j < 256; j++ )
		{ // find closest match in clut, naive search
			v.r = clut[(j<<2) + 0]; v.g = clut[(j<<2) + 1]; v.b = clut[(j<<2) + 2]; v.a = clut[(j<<2) + 3];

			float dist = (u - v) * (u - v);

			if( dist < min_dist )
			{ // v is closest match so far!
				min_dist = dist;
				min = j;
			}
		}

		*dptr++ = min;
	}

	// copy clut after palette code book
	memcpy( (unsigned char *)image + 256 * 4, clut, 256 * 4);

	Unlock();
}



void CImage::Compress_VQ(fp4 _Quality, CImage* _pDestImg, CTexture *_pTexture)
{

#ifdef IMAGE_IO_NOVQ
	Error("Compress_VQ", "VQ support disabled in this build.");
#else
	if (!CanCompress(IMAGE_MEM_COMPRESSTYPE_VQ))
		Error("Compress", "Image format not supported by VQ compressed format.");

	int h = GetHeight(), w = GetWidth(), d = 24;

	if( (GetFormat() & IMAGE_FORMAT_BGRA8) || (GetFormat() & IMAGE_FORMAT_BGRX8 ))
	{ // depth is 32 bit
		d = 32;
	}
	else if( GetFormat() & IMAGE_FORMAT_BGR8 )
	{ // depth is 24 bit
		d = 24;
	}
	else
	{ // not gonna bother supporting all crappy formats!
		Duplicate( _pDestImg );
		return;
	}

	if( w == 0 || h == 0 )
	{
		LogFile( "..invalid size, aborting" );
		return;
	}

	if( (w * h) > 256 * 256 )
	{
		int AllocSize = w*h/4 + 256*16;
		_pDestImg->Destroy();
		_pDestImg->m_pBitmap = DNew(uint8) uint8[ 1 ]; // AllocSize ];
		if (!_pDestImg->m_pBitmap) MemError("Compress_VQ");
		_pDestImg->m_AllocSize = 1; // AllocSize;
		_pDestImg->m_Width = GetWidth();
		_pDestImg->m_Height = GetHeight();
		_pDestImg->m_Format = GetFormat();
		_pDestImg->m_Memmodel = GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_VQ;
		_pDestImg->m_Memmodel &= ~IMAGE_MEM_LOCKABLE;
		return;
	}


	if( (w < 4) || (h < 4) || ((w & 15) != 0) || ((h & 15) != 0) || (w * h < 256 * 4) /*|| ((w * h) > 256 * 256)*/ )
	{ // absurd, but code actually tries to compress shitty small 1x1 textures! (mipmap etc)
		// Convert to Indexed mode
//		LogFile( CStr( "       ignored" ).Str() );
		if(_pTexture->m_LargestMap.GetPalette() == NULL)
		{
			Duplicate( _pDestImg );
			if(d == 24)
			{
				// Create a palette for the texture
				_pDestImg->SetPalette(MNew(CImagePalette));
				// Quantize the texture
				CDefaultQuantize *pQuantizer = _pDestImg->GetPalette()->GetQuantizer();
				pQuantizer->Begin();
				pQuantizer->Include(_pDestImg);
				pQuantizer->End();
				spCImage spQuant = pQuantizer->Quantize(_pDestImg);
				spQuant->Duplicate(_pDestImg);
				_pTexture->m_LargestMap.SetPalette(_pDestImg->GetPalette());
			}
		}
		else
			RemapFromClut(_pTexture->m_LargestMap.GetPalette(), _pDestImg);
		return;
	}


	LogFile( CStrF( "VQ Compress %s ( %d x %d ) %s", _pTexture->m_Name, w, h, _Quality > 0.5f ? "GLA" : "" ).Str() );

	void *image = Lock();

	if( image == 0 )
	{ // Error, may be better to just return and ignore...
		Error( "CompressVQ", "Failed to lock texture!" );
	}

	if( GetFormat() & IMAGE_FORMAT_BGRX8 )
	{ // make sure unused alpha bits wont fuckup the vector quantisation
		unsigned char *_image = (unsigned char *)image;
		int num = h * w * 4;
		for( int i = 3; i < num; i+=4 )
			_image[i] = 255;
	}

	// Create tree shaped codebook
	TSVQ tsvq( w, h, d, (unsigned char *)image );

	int AllocSize = ( w*h/4 + 256*16 );

	_pDestImg->Destroy();
	_pDestImg->m_pBitmap = DNew(uint8) uint8[AllocSize];
	if (!_pDestImg->m_pBitmap) MemError("Compress_VQ");
	_pDestImg->m_AllocSize = AllocSize;
	_pDestImg->m_Width = GetWidth();
	_pDestImg->m_Height = GetHeight();
	_pDestImg->m_Format = GetFormat();
	_pDestImg->m_Memmodel = GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_VQ;
	_pDestImg->m_Memmodel &= ~IMAGE_MEM_LOCKABLE;

	tsvq.GetCodebook( (unsigned char *)_pDestImg->m_pBitmap );
	tsvq.GetImage((unsigned char *)_pDestImg->m_pBitmap + (256 * 16), (unsigned char *)image, w, h, d);

	if( _Quality > 0.5f )
		tsvq.GLA_iter( (unsigned char *)_pDestImg->m_pBitmap, (unsigned char *)image, w, h, d );

	Unlock();

	_pDestImg->UpdateFormat();

#ifdef COMPRESS_VQ_CLUT
	if( _pTexture->m_LargestMap.GetPalette() == 0 )
		_pDestImg->ClutVQ(_pTexture);	// Calc clut and store in m_spPal for the mipmaps
	else
		_pDestImg->ClutVQ((uint8 *)_pTexture->m_LargestMap.GetPalette()->GetPalettePtr());	// Use m_spPal
#endif


#endif
} 


void CImage::Decompress_VQ(CImage* _pDestImg)
{
#ifdef IMAGE_IO_NOVQ
	Error("Decompress_VQ", "VQ support disabled in this build.");
#else

	Error("Decompress_VQ", "Decompression of VQ textures not permitted other than by hardware!!.");

#endif
}

void CImage::RemapFromClut(spCImagePalette _spPal, CImage *_pDestImg)
{
	// Dead slow algo that remaps a true color image from a palette.
	// Shouldn't take too long though, because it's only called on small textures

	_pDestImg->Create(GetWidth(), GetHeight(), IMAGE_FORMAT_CLUT8, GetMemModel(), _spPal);
	spCImage pSrcImage = Convert(IMAGE_FORMAT_BGRA8, IMAGE_CONVERT_RGBA);

	uint8 *clut = (uint8 *)_spPal->GetPalettePtr();
	uint8* pSrcMem;
	uint8* pDestMem;
	MLock(pSrcImage, pSrcMem);
	MLock(_pDestImg, pDestMem);

	for(int y = 0; y < GetHeight(); y++)
	{
		for(int w = 0; w < GetWidth(); w++)
		{
			// Find closest match
			uint8 min = 0;
			VQPixel u, v;

			u.r = *pSrcMem++; u.g = *pSrcMem++; u.b = *pSrcMem++; u.a = *pSrcMem++;
			v.r = clut[0]; v.g = clut[1]; v.b = clut[2]; v.a = clut[3];

			float min_dist = (u - v) * (u - v);

			for( int j = 1; j < 256; j++ )
			{
				v.r = clut[(j<<2) + 0]; v.g = clut[(j<<2) + 1]; v.b = clut[(j<<2) + 2]; v.a = clut[(j<<2) + 3];

				float dist = (u - v) * (u - v);

				if( dist < min_dist )
				{ // v is closest match so far!
					min_dist = dist;
					min = j;
				}
			}

			pDestMem[w] = min;
		}
		pSrcMem += pSrcImage->GetModulo() - (GetWidth() << 2);
		pDestMem += _pDestImg->GetModulo();
	}

	MUnlock(_pDestImg);
	MUnlock(this);

}






// WELL FUCK ME SIDEWAYS! This is the most obscure VQ image compression implementation
// ever written due to PS pixel memory formats


// -------------------------------------------------------------------------------------------
//
//



int	TSVQ::TwiddleImage(int width, int height, VQVector *vim )
{
	static int lut[] = {
		// even column
		0, 36, 8,  44,
		1, 37, 9,  45,
		2, 38, 10, 46,
		3, 39, 11, 47,
		4, 32, 12, 40,
		5, 33, 13, 41,
		6, 34, 14, 42,
		7, 35, 15, 43,

		16, 52, 24, 60,
		17, 53, 25, 61,
		18, 54, 26, 62,
		19, 55, 27, 63, 
		20, 48, 28, 56,
		21, 49, 29, 57,
		22, 50, 30, 58,
		23, 51, 31, 59,

		// odd column
		4, 32, 12, 40,
		5, 33, 13, 41,
		6, 34, 14, 42,
		7, 35, 15, 43,
		0, 36, 8,  44,
		1, 37, 9,  45,
		2, 38, 10, 46,
		3, 39, 11, 47,

		20, 48, 28, 56,
		21, 49, 29, 57,
		22, 50, 30, 58,
		23, 51, 31, 59,
		16, 52, 24, 60,
		17, 53, 25, 61,
		18, 54, 26, 62,
		19, 55, 27, 63
	};



	unsigned int *pin = DNew(unsigned int) unsigned int[width * height];
	if( !pin ) Error_static( "TSVQ::TwiddleImage", "Out of memory" );

	ConvertImage( (unsigned char *)pin, vim, width, height, 32 );

	int i,j,k;

	for( j = 0; j < height/2; j++ )
	{
		for( i = 0; i < width/2; i++ )
		{
			int column_index = ((i & 7) + ((j & 1) << 3) + ((j >> 1) & 1) * 16) << 2;
		
			for( k = 0; k < 4; k++ )
			{
				int index = lut[column_index++];

				int x = ((i & ~7) << 1) + (index & 15);
				int y = ((j & ~1) << 1) + (index >> 4);

				vim[j*width/2 + i].c[k] = pin[ x + y * width ];
			}
		}
	}

	delete[] pin;

	return 0;
}

#endif	// PLATFORM_CONSOLE
