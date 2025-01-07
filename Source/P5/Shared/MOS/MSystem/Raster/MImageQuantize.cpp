#include "PCH.h"

#ifndef PLATFORM_CONSOLE
#include "MImage.h"

// -------------------------------------------------------------------
//  COctreeQuantize
// -------------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(COctreeQuantize);


inline OctNode* COctreeQuantize::InsertColor(OctNode* Node, uint8 r, uint8 g, uint8 b)
{
	MAUTOSTRIP(COctreeQuantize_InsertColor, NULL);
	// Add color to this node.
	Node->AddColor(r,g,b);

	// Have we reached a leaf or gone past the reduction level?
	if (!Node->IsLeaf() && Node->GetLevel()<LeafLevel) {

		// Where shall we go next?
		int Child=Node->FindChild(r,g,b);

		OctNode* SubTree;

		if (Node->GetChild(Child)==NULL) {
			bool Leaf=(Node->GetLevel()+1>=LeafLevel);
			if (Leaf) NrLeafs++;

			SubTree=DNew(OctNode) OctNode(Node->GetLevel()+1,Leaf);
			if (SubTree==NULL) MemError("InsertColor");
			Node->IncChildren();
			Node->SetChild(SubTree,Child);

			if (Node->GetNrChildren()>=2 && !Node->GetReduceable()) {
				Node->SetReduceable(TRUE);
				Reduceables[Node->GetLevel()].Add(Node);
			}

		}
		else
			SubTree=Node->GetChild(Child);

		return InsertColor(SubTree,r,g,b);
	}
	else
		return Node;
}

inline void COctreeQuantize::DeleteSubTree(OctNode* Node)
{
	MAUTOSTRIP(COctreeQuantize_DeleteSubTree, MAUTOSTRIP_VOID);
	if (Node==NULL)
		return;

	if (!Node->IsLeaf()) {

		int i;
		for (i=0; i<8; i++) {
			if (Node->GetChild(i)!=NULL) {
				DeleteSubTree(Node->GetChild(i));
				Node->SetChild(NULL,i);
				Node->DecChildren();
			}
		}

	}
	else
		NrLeafs--;

	delete Node;
}

inline void COctreeQuantize::ReduceTree()
{
	MAUTOSTRIP(COctreeQuantize_ReduceTree, MAUTOSTRIP_VOID);
	uint8 ReductionLevel=LeafLevel-1;

	// Find reduction level.
	while (Reduceables[ReductionLevel].Len()==0 && ReductionLevel>0)
		ReductionLevel--;

	if (Reduceables[ReductionLevel].Len()==0)
		Error("ReduceTree","All reduceable lists are empty.");

	// Search list and get the element with the brightest
	// average color.
	uint8 r,g,b;
	fp32 Val;
	fp32 Min=1000000.0;
	int Element;
	int i;
	for (i=0; i<Reduceables[ReductionLevel].Len(); i++) 
	{
		(Reduceables[ReductionLevel])[i]->GetAverageRGB(r,g,b);

		// Calculate the square of the square distance between
		// the colors (r,g,b) and (255,255,255).
		Val=(fp32)r*(fp32)r+(fp32)g*(fp32)g+(fp32)b*(fp32)b-
			510*((fp32)r+(fp32)g+(fp32)b);

		if (Val<Min) {
			Min=Val;
			Element=i;
		}
	}

	// Remove the node from the list.
	OctNode& Node=*(Reduceables[ReductionLevel])[Element];
	Reduceables[ReductionLevel].Del(Element);

	// Delete the children of the node to reduce.
	for (i=0; i<8; i++) {
		if (Node.GetChild(i)!=NULL) {
			DeleteSubTree(Node.GetChild(i));
			Node.SetChild(NULL,i);
			Node.DecChildren();
		}
	}

	// Mark node as leaf and as not reduceable.
	Node.SetReduceable(FALSE);
	Node.SetLeaf(TRUE);
	NrLeafs++;

	// Update leaf level.
	LeafLevel=ReductionLevel+1;
}

void COctreeQuantize::CreatePalette(OctNode* Node)
{
	MAUTOSTRIP(COctreeQuantize_CreatePalette, MAUTOSTRIP_VOID);
	if (Node==NULL)
		return;

	if (Node->IsLeaf() || Node->GetLevel()==LeafLevel) {

		Node->SetPaletteIndex(Index);

		uint8 r,g,b;
		Node->GetAverageRGB(r,g,b);

		if (Index>=(1 << CONST_OCTQUANT_DESTBPP)) Error("CreatePalette","Too many colors in octree.");

		Palette[Index++]=CPixel32(r,g,b,255);
	}
	else {
		int i;
		for (i=0; i<8; i++)
			CreatePalette(Node->GetChild(i));
	}
}

// MapColor can only map colors that has an exact match in the tree.
inline uint8 COctreeQuantize::MapColor(OctNode* Node,uint8 r,uint8 g,uint8 b)
{
	MAUTOSTRIP(COctreeQuantize_MapColor, 0);
	// Same as last time?
	if (LastR==r && LastG==g && LastB==b)
		return LastIndex;

	// Have we reached a leaf?
	while (Node!=NULL && !Node->IsLeaf() && Node->GetLevel()!=LeafLevel)
		Node=Node->GetChild( Node->FindChild(r,g,b) );

	if (Node==NULL) Error("MapColor","Color not represented in octree.");

	LastR=r;
	LastG=g;
	LastB=b;

	return LastIndex=Node->GetPaletteIndex();
}

// MapColor2 can map all colors, even those that hasn't got an exact
// match in the tree. If can't find an exact match it will
// call FindBestMatch which will recursively find the best
// match in the tree.
inline uint8 COctreeQuantize::MapColor2(OctNode* Node,uint8 r,uint8 g,uint8 b)
{
	MAUTOSTRIP(COctreeQuantize_MapColor2, 0);
	// Same as last time?
	if (LastR==r && LastG==g && LastB==b)
		return LastIndex;

	if (Node==NULL) Error("MapColor2","Color not represented in octree.");

	LastR=r;
	LastG=g;
	LastB=b;

	int Child;
	while (!Node->IsLeaf() && Node->GetLevel()!=LeafLevel) {
		Child=Node->FindChild(r,g,b);
		if (Node->GetChild(Child)==NULL)
			return LastIndex=FindBestMatch(Node,r,g,b);

		Node=Node->GetChild(Child);
	}

	return LastIndex=Node->GetPaletteIndex();
}


inline uint8 COctreeQuantize::FindBestMatch(OctNode* Node,uint8 r,uint8 g,uint8 b)
{
	MAUTOSTRIP(COctreeQuantize_FindBestMatch, 0);
	// Have we reached a leaf?
	if (Node->IsLeaf() || Node->GetLevel()==LeafLevel)
		return Node->GetPaletteIndex();
	else {
		// Choose the child whose subtree contains the color
		// with the least square "distance" to the (r,g,b) color.
		uint8 r2,g2,b2;
		uint32 MinDist=500000,Dist;
		uint8 Ind,BestInd;

		int i;
		for (i=0; i<8; i++) {
			if (Node->GetChild(i)!=NULL) {

				Ind=FindBestMatch(Node->GetChild(i),r,g,b);
				r2=Palette[Ind].GetR();
				g2=Palette[Ind].GetG();
				b2=Palette[Ind].GetB();

				Dist=(r2-r)*(r2-r)+(g2-g)*(g2-g)+(b2-b)*(b2-b);

				if (Dist<MinDist) {
					MinDist=Dist;
					BestInd=Ind;
				}
			}
		}

		return BestInd;
	}
}

uint8 COctreeQuantize::GetIndex(CPixel32 col)
{
	MAUTOSTRIP(COctreeQuantize_GetIndex, 0);
	return MapColor2(Tree, col.GetR(), col.GetG(), col.GetB());
};

void COctreeQuantize::GetIndices(CPixel32* colors, uint8* dest, int NrColors)
{
	MAUTOSTRIP(COctreeQuantize_GetIndices, MAUTOSTRIP_VOID);
	for (int i=0; i<NrColors; i++)
		dest[i]=MapColor2(Tree,colors[i].GetR(),colors[i].GetG(),colors[i].GetB());
}

void COctreeQuantize::BuildFromPalette(CPixel32* Pal)
{
	MAUTOSTRIP(COctreeQuantize_BuildFromPalette, MAUTOSTRIP_VOID);
	// Init tree etc.
	Begin();

	AllColorsInTree=FALSE;	// All colors might not have an exact match.

	// Copy palette.
	memcpy(Palette,Pal,(1 << CONST_OCTQUANT_DESTBPP)*sizeof(CPixel32));

	// Insert palette colors into the octree.
	OctNode* Node;
	uint16 i;
	for (i=0; i<(1 << CONST_OCTQUANT_DESTBPP); i++) {
		Node=InsertColor(Tree,Palette[i].GetR(),Palette[i].GetG(),Palette[i].GetB());
		Node->SetPaletteIndex(i);
	}
}

void COctreeQuantize::BuildFromPalette(spCImagePalette spPal)
{
	MAUTOSTRIP(COctreeQuantize_BuildFromPalette_1, MAUTOSTRIP_VOID);
	Begin();
	AllColorsInTree = FALSE;

	spPal->GetPalette((CPixel32*) &Palette, 0, 256);
	mspPal = spPal;

	// Insert palette colors into the octree.
	OctNode* Node;
	uint16 i;
	for (i=0; i<(1 << CONST_OCTQUANT_DESTBPP); i++) {
		Node=InsertColor(Tree,Palette[i].GetR(),Palette[i].GetG(),Palette[i].GetB());
		Node->SetPaletteIndex(i);
	}
}

void COctreeQuantize::Begin() 
{
	MAUTOSTRIP(COctreeQuantize_Begin, MAUTOSTRIP_VOID);
	LeafLevel=CONST_OCTQUANT_DESTBPP;
	NrLeafs=0;

	if (Tree!=NULL) delete Tree;

	int i;
	for (i=0; i<CONST_OCTQUANT_DESTBPP; i++)
		Reduceables[i].Clear();

	Tree=DNew(OctNode) OctNode(0,FALSE);
	if (Tree==NULL) MemError("Begin");

	AllColorsInTree=TRUE;	// All colors will have an exact match.
}

void COctreeQuantize::Include(CImage* _pImage)
{
	MAUTOSTRIP(COctreeQuantize_Include, MAUTOSTRIP_VOID);
	int Format = _pImage->GetFormat();
	int Width = _pImage->GetWidth();
	int Height = _pImage->GetHeight();
	spCImage spTmpSrc;
	CImage* pSrc = NULL;

	if ((Format != IMAGE_FORMAT_CLUT8) &&
		(Format != IMAGE_FORMAT_BGR8))
	{
		spTmpSrc = MNew(CImage);
		if (spTmpSrc == NULL) MemError("Include");
		spTmpSrc->Create(Width, Height, IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
		try { CImage::Convert(_pImage, spTmpSrc); }
		catch (CCException) { Error("Include", "Unable to convert source to RGB24."); }
		Format = spTmpSrc->GetFormat();
		pSrc = spTmpSrc;
	}
	else
	{
		pSrc = _pImage;
	}

	switch (Format) 
	{
	case IMAGE_FORMAT_BGR8: 
		{
			uint8* Scanline=NULL;
			try {
				Scanline=DNew(uint8) uint8[Width*3];
				if (Scanline==NULL) MemError("Include");

				int k=(1 << CONST_OCTQUANT_DESTBPP);

				int i,j;
				for (i=0; i<Height; i++) {

					pSrc->GetRAWData(CPnt(0,i),Width*3,Scanline);

					for (j=0; j<Width; j++) {
						InsertColor(Tree,*(Scanline+j*3+2),*(Scanline+j*3+1),*(Scanline+j*3));
						if (NrLeafs>k)
							ReduceTree();
					}
				}
			}
			catch (...) 
			{
				if (Scanline!=NULL) delete[] Scanline;
				throw;
			}
			delete[] Scanline;
		}
		break;

	case IMAGE_FORMAT_CLUT8: 
		{
			CPixel32 Pal[256];
			pSrc->GetPalette()->GetPalette(Pal,0,256);

			int k = (1 << CONST_OCTQUANT_DESTBPP);

			for (int i=0; i<256; i++) 
			{
				InsertColor(Tree,Pal[i].GetR(),Pal[i].GetG(),Pal[i].GetB());
				if (NrLeafs>k) ReduceTree();
			}
		}
		break;

	default:
		Error("Include","Can only quantize 8 or 24-bit images.");

	}	// end switch

}

void COctreeQuantize::End()
{
	MAUTOSTRIP(COctreeQuantize_End, MAUTOSTRIP_VOID);
	memset(Palette,0,(1 << CONST_OCTQUANT_DESTBPP)*sizeof(CPixel32));

	Index=0;
	CreatePalette(Tree);
}

spCImage COctreeQuantize::Quantize(CImage* _pSrcImage, eDither Dither)
{
	MAUTOSTRIP(COctreeQuantize_Quantize, NULL);
	if (!_pSrcImage) Error("Quantize", "pSrcImage == NULL");

	spCImage spDestImage = NULL;
	uint8* Scanline = NULL;
	uint8* Scanline2 = NULL;
	spCImage spTmpSrc;
	CImage* pSrc = NULL;

	LastR=-1;

	try 
	{
		int Format=_pSrcImage->GetFormat();
		int Width=_pSrcImage->GetWidth();
		int Height=_pSrcImage->GetHeight();
		int MemModel=_pSrcImage->GetMemModel();

		if ((_pSrcImage->GetFormat() != IMAGE_FORMAT_CLUT8) &&
			(_pSrcImage->GetFormat() != IMAGE_FORMAT_BGR8))
		{
			spTmpSrc = MNew(CImage);
			if (spTmpSrc == NULL) MemError("Quantize");
			spTmpSrc->Create(Width, Height, IMAGE_FORMAT_BGR8, MemModel);
			try { CImage::Convert(_pSrcImage, spTmpSrc); }
			catch (CCException) { Error("Quantize", "Unable to convert source to RGB24."); }
			Format = spTmpSrc->GetFormat();
			pSrc = spTmpSrc;
		}
		else
		{
			pSrc = _pSrcImage;
		}

		// Create the destination image.
		if (mspPal == NULL) mspPal = MNew(CImagePalette);
		if (mspPal == NULL) MemError("Quantize");
		mspPal->SetPalette(Palette,0,256);

		spDestImage = MNew(CImage);
		if (spDestImage == NULL) MemError("Quantize");
		spDestImage->Create(Width, Height, IMAGE_FORMAT_CLUT8, MemModel, mspPal);

		switch (Format) 
		{
		case IMAGE_FORMAT_BGR8: 
			{
				if (Dither==NO_DITHERING) 
				{
					Scanline=DNew(uint8) uint8[Width*3];
					if (Scanline==NULL) MemError("Quantize");

					// Map all colors to an entry in the new palette.
					int i,j;
					for (i=0; i<Height; i++) 
					{
						pSrc->GetRAWData(CPnt(0,i),Width*3,Scanline);

						if (AllColorsInTree) 
						{
							for (j=0; j<Width; j++) 
								*(Scanline+j)=
									MapColor(Tree,*(Scanline+j*3+2),*(Scanline+j*3+1),*(Scanline+j*3));
						}
						else 
						{
							for (j=0; j<Width; j++) 
								*(Scanline+j)=
									MapColor2(Tree,*(Scanline+j*3+2),*(Scanline+j*3+1),*(Scanline+j*3));
						}

						spDestImage->SetRAWData(CPnt(0,i),Width,Scanline);
					}
				}
				else 
				{

					Scanline=DNew(uint8) uint8[Width*3*sizeof(fp32)];
					Scanline2=DNew(uint8) uint8[Width*3*sizeof(fp32)];
					if (Scanline==NULL || Scanline2==NULL) MemError("Quantize");

					// Map all colors to an entry in the new palette
					// and dither the image at the same time.
					int i,j;

					pSrc->GetRAWData(CPnt(0,0),Width*3,Scanline);

					for (j=Width-1; j>=0; j--) 
					{
						*((fp32*)Scanline+j*3+2)=(fp32)*(Scanline+j*3+2);
						*((fp32*)Scanline+j*3+1)=(fp32)*(Scanline+j*3+1);
						*((fp32*)Scanline+j*3)=(fp32)*(Scanline+j*3);
					}

					for (i=0; i<Height; i++) 
					{

						if (i+1<Height) 
						{
							pSrc->GetRAWData(CPnt(0,i+1),Width*3,Scanline2);
							for (j=Width-1; j>=0; j--) 
							{
								*((fp32*)Scanline2+j*3+2)=(fp32)*(Scanline2+j*3+2);
								*((fp32*)Scanline2+j*3+1)=(fp32)*(Scanline2+j*3+1);
								*((fp32*)Scanline2+j*3)=(fp32)*(Scanline2+j*3);
							}
						}

						fp32 r,g,b;
						fp32 ErrorR,ErrorG,ErrorB;
						for (j=0; j<Width; j++) 
						{
							r=*((fp32*)Scanline+j*3+2);
							g=*((fp32*)Scanline+j*3+1);
							b=*((fp32*)Scanline+j*3);

							r=Min(Max(r,0.0f),255.0f);
							g=Min(Max(g,0.0f),255.0f);
							b=Min(Max(b,0.0f),255.0f);

							*(Scanline+j)=MapColor2(Tree,(uint8)(r+0.5f),(uint8)(g+0.5f),(uint8)(b+0.5f));

							ErrorR=*((fp32*)Scanline+j*3+2)-(fp32)(Palette[*(Scanline+j)].GetR());
							ErrorG=*((fp32*)Scanline+j*3+1)-(fp32)(Palette[*(Scanline+j)].GetG());
							ErrorB=*((fp32*)Scanline+j*3)-(fp32)(Palette[*(Scanline+j)].GetB());


							if (j+1<Width) 
							{
								*((fp32*)Scanline+(j+1)*3+2)+=(ErrorR*7)/16;
								*((fp32*)Scanline+(j+1)*3+1)+=(ErrorG*7)/16;
								*((fp32*)Scanline+(j+1)*3)+=(ErrorB*7)/16;
							}

							if (i+1<Height) 
							{
								*((fp32*)Scanline2+j*3+2)+=(ErrorR*5)/16;
								*((fp32*)Scanline2+j*3+1)+=(ErrorG*5)/16;
								*((fp32*)Scanline2+j*3)+=(ErrorB*5)/16;
							}

							if (i+1<Height && j+1<Width) 
							{
								*((fp32*)Scanline2+(j+1)*3+2)+=(ErrorR)/16;
								*((fp32*)Scanline2+(j+1)*3+1)+=(ErrorG)/16;
								*((fp32*)Scanline2+(j+1)*3)+=(ErrorB)/16;
							}

						}

						spDestImage->SetRAWData(CPnt(0,i),Width,Scanline);

						memcpy(Scanline,Scanline2,Width*3*sizeof(fp32));
					}

				}

				break;
			}

		case IMAGE_FORMAT_CLUT8: 
			{
				if (Dither==NO_DITHERING) 
				{

					CPixel32 Pal[256];
					pSrc->GetPalette()->GetPalette(Pal,0,256);

					uint8 IndexTable[256];

					// Setup index to index conversion table.
					if (AllColorsInTree) 
					{
						for (int i=0; i<256; i++)
							IndexTable[i]=MapColor(Tree,Pal[i].GetR(),Pal[i].GetG(),Pal[i].GetB());
					}
					else 
					{
						for (int i=0; i<256; i++)
							IndexTable[i]=MapColor2(Tree,Pal[i].GetR(),Pal[i].GetG(),Pal[i].GetB());
					}

					Scanline=DNew(uint8) uint8[Width];
					if (Scanline==NULL) MemError("Quantize");

					// Remap all colors to an entry in the new palette
					// using the index conversion table.
					int i,j;
					for (i=0; i<Height; i++) 
					{
						pSrc->GetRAWData(CPnt(0,i),Width,Scanline);

						for (j=0; j<Width; j++)
							*(Scanline+j)=IndexTable[*(Scanline+j)];

						spDestImage->SetRAWData(CPnt(0,i),Width,Scanline);
					}

				}
				else 
				{
					CPixel32 Pal[256];
					pSrc->GetPalette()->GetPalette(Pal,0,256);

					Scanline=DNew(uint8) uint8[Width*3*sizeof(fp32)];
					Scanline2=DNew(uint8) uint8[Width*3*sizeof(fp32)];
					if (Scanline==NULL || Scanline2==NULL) MemError("Quantize");

					// Remap all colors to an entry in the new palette
					// and dither the image at the same time.
					int i,j;
					pSrc->GetRAWData(CPnt(0,0),Width,Scanline);

					int Index;
					for (j=Width-1; j>=0; j--) 
					{
						Index=*(Scanline+j);
						*((fp32*)Scanline+j*3+2)=Pal[Index].GetR();
						*((fp32*)Scanline+j*3+1)=Pal[Index].GetG();
						*((fp32*)Scanline+j*3)=Pal[Index].GetB();
					}

					for (i=0; i<Height; i++) 
					{

						if (i+1<Height) 
						{
							pSrc->GetRAWData(CPnt(0,i+1),Width,Scanline2);
							for (j=Width-1; j>=0; j--) 
							{
								Index=*(Scanline2+j);
								*((fp32*)Scanline2+j*3+2)=Pal[Index].GetR();
								*((fp32*)Scanline2+j*3+1)=Pal[Index].GetG();
								*((fp32*)Scanline2+j*3)=Pal[Index].GetB();
							}
						}

						fp32 r,g,b;
						fp32 ErrorR,ErrorG,ErrorB;

						for (j=0; j<Width; j++) 
						{

							r=*((fp32*)Scanline+j*3+2);
							g=*((fp32*)Scanline+j*3+1);
							b=*((fp32*)Scanline+j*3);

							r = Min(Max(r, 0.0f), 255.0f);
							g = Min(Max(g, 0.0f), 255.0f);
							b = Min(Max(b, 0.0f), 255.0f);

							*(Scanline+j)=MapColor2(Tree,(uint8)(r+0.5f),(uint8)(g+0.5f),(uint8)(b+0.5f));

							ErrorR=*((fp32*)Scanline+j*3+2)-(fp32)(Palette[*(Scanline+j)].GetR());
							ErrorG=*((fp32*)Scanline+j*3+1)-(fp32)(Palette[*(Scanline+j)].GetG());
							ErrorB=*((fp32*)Scanline+j*3)-(fp32)(Palette[*(Scanline+j)].GetB());


							if (j+1<Width) {
								*((fp32*)Scanline+(j+1)*3+2)+=(ErrorR*7)/16;
								*((fp32*)Scanline+(j+1)*3+1)+=(ErrorG*7)/16;
								*((fp32*)Scanline+(j+1)*3)+=(ErrorB*7)/16;
							}

							if (i+1<Height) {
								*((fp32*)Scanline2+j*3+2)+=(ErrorR*5)/16;
								*((fp32*)Scanline2+j*3+1)+=(ErrorG*5)/16;
								*((fp32*)Scanline2+j*3)+=(ErrorB*5)/16;
							}

							if (i+1<Height && j+1<Width) {
								*((fp32*)Scanline2+(j+1)*3+2)+=(ErrorR)/16;
								*((fp32*)Scanline2+(j+1)*3+1)+=(ErrorG)/16;
								*((fp32*)Scanline2+(j+1)*3)+=(ErrorB)/16;
							}

						}

						spDestImage->SetRAWData(CPnt(0,i),Width,Scanline);

						memcpy(Scanline,Scanline2,Width*3*sizeof(fp32));
					}

				}

				break;
			}

		default:
			Error("Quantize","Can only quantize 8 or 24-bit images.");

		}	// end switch

	}
	catch (...) {
		if (Scanline!=NULL) delete[] Scanline;
		if (Scanline2!=NULL) delete[] Scanline2;
		throw;
	}

	delete[] Scanline;
	if (Scanline2!=NULL) delete[] Scanline2;

	return spDestImage;
}


/***************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
|  CMinErrorQuantize
|___________________________________________________________________________________________________
\***************************************************************************************************/

extern "C"
{
	#include "../../../../SDK/NeuQuant/NEUQUANT.H"
}
class CMinErrorQuantize_WorkerNeuQuant : public CReferenceCount
{
private:
public:

	~CMinErrorQuantize_WorkerNeuQuant()
	{
	}
	CMinErrorQuantize_WorkerNeuQuant()
	{
	}

	TArray<CPixel32> m_lPalette;
	TArray<CPixel32> m_lPixels;
	TArray<CVec4Dfp64> m_lStartPalette;
	int m_nStartPalettes;

	void BuildFromPalette(CPixel32* _pPal)
	{
//		ConOutL("Warning building palette from other palette, this will not represent the number of pixels for each color");
		int CurLen = m_lPixels.Len();
		m_lPixels.SetLen(CurLen + (1 << CONST_OCTQUANT_DESTBPP));

		++m_nStartPalettes;
		for (int i = 0; i < (1 << CONST_OCTQUANT_DESTBPP); ++i)
		{
			m_lPixels[CurLen + i] = _pPal[i];
			m_lStartPalette[i] += CVec4Dfp64(_pPal[i].R(), _pPal[i].G(), _pPal[i].B(), _pPal[i].A());
		}

	}

	void BuildFromPalette(spCImagePalette _spPal)
	{
		if( m_lStartPalette.Len() == 0 )
		{
			Begin();
		}

		CPixel32* pPal = _spPal->GetPalettePtr();
		int nColors = _spPal->GetNumColors();
		int CurLen = m_lPixels.Len();
		m_lPixels.SetLen(CurLen + nColors);
		int iCurrentPal = 0;
		++m_nStartPalettes;
		for (int i = 0; i < nColors; ++i)
		{
			m_lPixels[CurLen + i] = pPal[i];
			m_lStartPalette[i] += CVec4Dfp64(pPal[i].R(), pPal[i].G(), pPal[i].B(), pPal[i].A());
		}
		for (int i = nColors; i < 256; ++i)
		{
			m_lStartPalette[i] += 128.0;
		}
	}

	void Begin()
	{
		m_lPixels.Clear();
		m_lStartPalette.SetLen(256);
		m_nStartPalettes = 0;
		for (int i = 0; i < 256; ++i)
		{
			m_lStartPalette[i] = 0.0;
		}
	}

	void Include(CImage* _pImage, int _Priority)
	{
		int Format = _pImage->GetFormat();
		int Width = _pImage->GetWidth();
		int Height = _pImage->GetHeight();
		spCImage spTmpSrc;
		CImage* pSrc = NULL;

		if (Format != IMAGE_FORMAT_BGRA8)
		{
			spTmpSrc = MNew(CImage);
			if (spTmpSrc == NULL) 
				MemError("Include");

			spTmpSrc->Create(Width, Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

			try 
			{ 
				CImage::Convert(_pImage, spTmpSrc, IMAGE_CONVERT_RGBA); 
			}
			catch (CCException) 
			{ 
				Error("Include", "Unable to convert source to RGBA32."); 
			}

			Format = spTmpSrc->GetFormat();
			pSrc = spTmpSrc;
		}
		else
		{
			pSrc = _pImage;
		}

		if (IsPow2(Width) && IsPow2(Height) && ! m_nStartPalettes)
		{
			spCImage spTmp = pSrc->Duplicate();
			while (spTmp->GetWidth() > 16 && spTmp->GetHeight() > 16)
			{
				spCImage spTmp2 = MNew(CImage);
				spTmp2->Create(spTmp->GetWidth() / 2, spTmp->GetHeight() / 2, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
				CImage::StretchHalf(spTmp, spTmp2);
				Swap(spTmp, spTmp2);
			}
			while (spTmp->GetWidth() > 16)
			{
				spCImage spTmp2 = MNew(CImage);
				spTmp2->Create(spTmp->GetWidth() / 2, spTmp->GetHeight(), IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
				CImage::StretchHalfX(spTmp, spTmp2);
				Swap(spTmp, spTmp2);
			}
			while (spTmp->GetHeight() > 16)
			{
				spCImage spTmp2 = MNew(CImage);
				spTmp2->Create(spTmp->GetWidth(), spTmp->GetHeight() / 2, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
				CImage::StretchHalfY(spTmp, spTmp2);
				Swap(spTmp, spTmp2);
			}
			int Pixels = spTmp->GetWidth() * spTmp->GetHeight();
			CPixel32 *pLock = (CPixel32 *)spTmp->Lock();
			for (int i = 0; i < Pixels; ++i)
			{
				m_lStartPalette[i] += CVec4Dfp64(pLock[i].R(), pLock[i].G(), pLock[i].B(), pLock[i].A());
			}
			spTmp->Unlock();
			for (int i = Pixels; i < 256; ++i)
			{
				m_lStartPalette[i] += 128.0;
			}
			++m_nStartPalettes;
		}

		CPixel32 *pLockSrc = (CPixel32 *)pSrc->Lock();
		CPixel32 *pCurrentPixelSrc;
		int ModuloSrc = pSrc->GetModulo();
		int CurLen = m_lPixels.Len();
		m_lPixels.SetLen(CurLen + ((ModuloSrc/4 * Height))*_Priority);
		CPixel32 *pCurrentPixel = m_lPixels.GetBasePtr() + CurLen;

		while (_Priority)
		{
			for (int y = 0; y < Height; ++y)
			{
				pCurrentPixelSrc = (CPixel32 *)(((uint8 *)pLockSrc + y * ModuloSrc));

				for (int x = 0; x < Width; ++x)
					pCurrentPixel[x] = pCurrentPixelSrc[x];

				pCurrentPixel+=Width;
			}
			--_Priority;
		}

		pSrc->Unlock();
	}

	void End()
	{
		while (m_lPixels.Len() < 16384)
		{
			int Len = m_lPixels.Len();
			m_lPixels.SetLen(Len*2);
			for (int i = 0; i < Len; ++i)
				m_lPixels[Len+i] = m_lPixels[i];
		}
		initnet((unsigned char *)m_lPixels.GetBasePtr(), m_lPixels.Len() * 4, 1);
		CPixel32 StartPal[256];
		for (int i = 0; i < 256; ++i)
		{
			StartPal[i] = CPixel32(
				Min(((int)(m_lStartPalette[i].k[0] / m_nStartPalettes + 0.5)), 255),
				Min(((int)(m_lStartPalette[i].k[1] / m_nStartPalettes + 0.5)), 255),
				Min(((int)(m_lStartPalette[i].k[2] / m_nStartPalettes + 0.5)), 255),
				Min(((int)(m_lStartPalette[i].k[3] / m_nStartPalettes + 0.5)), 255)
				);
		}
		learn((unsigned char *)StartPal);
		unbiasnet();
		m_lPalette.SetLen(256);
		for (int i = 0; i < m_lPalette.Len(); ++i)
		{
			m_lPalette[i] = CPixel32(0,0,0,0);
		}
		writecolourmap((unsigned char*)m_lPalette.GetBasePtr());
		inxbuild();
	}

	spCImage Quantize(CImage* _pSrcImage, eDither _Dither)
	{
		int Format = _pSrcImage->GetFormat();
		int Width = _pSrcImage->GetWidth();
		int Height = _pSrcImage->GetHeight();
		spCImage spTmpSrc;
		CImage* pSrc = NULL;

		if (Format != IMAGE_FORMAT_BGRA8)
		{
			spTmpSrc = MNew(CImage);
			if (spTmpSrc == NULL) 
				MemError("Include");

			spTmpSrc->Create(Width, Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

			try 
			{ 
				CImage::Convert(_pSrcImage, spTmpSrc, IMAGE_CONVERT_RGBA); 
			}
			catch (CCException) 
			{ 
				Error("Include", "Unable to convert source to RGBA32."); 
			}

			Format = spTmpSrc->GetFormat();
			pSrc = spTmpSrc;
		}
		else
		{
			pSrc = _pSrcImage;
		}

		spCImagePalette spPal = MNew(CImagePalette);
		spPal->SetPalette(m_lPalette.GetBasePtr(),0,m_lPalette.Len());

		spCImage spImg = MNew(CImage);
		spImg->Create(Width, Height, IMAGE_FORMAT_CLUT8, IMAGE_MEM_IMAGE);

		spImg->SetPalette(spPal);

		uint8 *pLock = (uint8 *)spImg->Lock();
		uint8 *pCurrentPixel;
		int Modulo = spImg->GetModulo();

		CPixel32 *pLockSrc = (CPixel32 *)pSrc->Lock();
		CPixel32 *pCurrentPixelSrc;
		int ModuloSrc = pSrc->GetModulo();

		for (int y = 0; y < Height; ++y)
		{
			pCurrentPixel = pLock + y * Modulo;
			pCurrentPixelSrc = (CPixel32 *)(((uint8 *)pLockSrc + y * ModuloSrc));
			for (int x = 0; x < Width; ++x)
			{
                pCurrentPixel[x] = GetIndex(pCurrentPixelSrc[x]);
			}
		}

		return spImg;
	}

	uint8 GetIndex(CPixel32 _Col)
	{
/*		int BestFit = 256*4;
		int iPal = 0;
		for (int i = 0; i < m_lPalette.Len(); ++i)
		{
			int Fit = Sqr(abs(m_lPalette[i].R() - _Col.R())) + Sqr(abs(m_lPalette[i].G() - _Col.G())) + Sqr(abs(m_lPalette[i].B() - _Col.B())) + Sqr(abs(m_lPalette[i].A() - _Col.A()));
			if (Fit < BestFit)
			{
				BestFit = Fit;
				iPal = i;
			}
		}
		return iPal;*/

		return inxsearch(_Col.B(), _Col.G(), _Col.R(), _Col.A());
	}

	void GetIndices(CPixel32* _Colors, uint8* _Dest, int _nColors)
	{
		for (int i = 0; i < _nColors; ++i)
			_Dest[i] = GetIndex(_Colors[i]);		
	}
};

typedef CMinErrorQuantize_WorkerNeuQuant CMinErrorQuantize_Worker;

void CMinErrorQuantize::Init()
{
	if (!m_pWorker)
		m_pWorker = MNew(CMinErrorQuantize_Worker);

}

CMinErrorQuantize::~CMinErrorQuantize()
{
	if (m_pWorker)
		delete (CMinErrorQuantize_Worker *)m_pWorker;
}

void CMinErrorQuantize::BuildFromPalette(CPixel32* Pal)
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->BuildFromPalette(Pal);
}

void CMinErrorQuantize::BuildFromPalette(spCImagePalette spPal)
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->BuildFromPalette(spPal);
}

void CMinErrorQuantize::Begin()
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->Begin();
}

void CMinErrorQuantize::Include(CImage* _pImage, int _Priority)
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->Include(_pImage, _Priority);
}

void CMinErrorQuantize::End()
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->End();
}

spCImage CMinErrorQuantize::Quantize(CImage* _pSrcImage,eDither Dither)
{
	Init();
	return ((CMinErrorQuantize_Worker*)m_pWorker)->Quantize(_pSrcImage,Dither);
}

uint8 CMinErrorQuantize::GetIndex(CPixel32 col)
{
	Init();
	return ((CMinErrorQuantize_Worker*)m_pWorker)->GetIndex(col);
}

void CMinErrorQuantize::GetIndices(CPixel32* colors, uint8* dest, int NrColors)
{
	Init();
	((CMinErrorQuantize_Worker*)m_pWorker)->GetIndices(colors, dest, NrColors);
}


#ifdef DEBUG_DISABLE_INTELP5
	#ifdef KEEP_CPU_INTELP5
		#undef KEEP_CPU_INTELP5
		#define CPU_INTELP5
	#endif
#endif

#endif
