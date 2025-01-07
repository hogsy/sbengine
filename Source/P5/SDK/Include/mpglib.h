
#ifndef	__MPGLIB_H_INCLUDED
#define	__MPGLIB_H_INCLUDED


struct buffer2 {
        unsigned char *pnt;
		long size;
		long pos;
        struct buffer2 *next;
        struct buffer2 *prev;
		long inuse;
};

struct framebuf {
	struct buf *buf;
	long pos;
	struct frame *next;
	struct frame *prev;
};

struct mpstr {
	struct buffer2 *head,*tail;
	int bsize;
	int framesize;
    int fsizeold;
	struct frame fr;
	unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned long header;
	int bsnum;
	real synth_buffs[2][2][0x110];
    int  synth_bo;
	int bitindex;
	unsigned char *wordpointer;
	unsigned char *pcm_sample;
};

#ifndef BOOL
#define BOOL int
#endif

#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1


#ifdef __cplusplus
extern "C" {
#endif
  
BOOL InitMP3(struct mpstr *mp);
int decodeMP3(struct mpstr *mp, struct buffer2 *mbuffer,
     char *outmemory,int outmemsize,int *done);
void ExitMP3(struct mpstr *mp);

#ifdef __cplusplus
}
#endif


#endif	// __MPGLIB_H_INCLUDED
