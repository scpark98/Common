#include <math.h>
#include <stdlib.h>
#include "canny.h"


#define ABS(a)          (((a) > 0) ? (a) : -(a))
#define MAX2(a,b)		((a) > (b) ? (a) : (b))


void cannygrassfire3x3(unsigned char* output, const unsigned char* input,const unsigned short width ,const unsigned short height ,const short x, const short y,const int label,unsigned int *hp,unsigned char* hi)
{
	const unsigned short w = width;
	const unsigned short h = height;
	const unsigned int length = w*h;
	const unsigned char* I = input;
	unsigned char * O=output;

	const short offsets[9]=
	{
		-1-w,-w,1-w,
		-1,0,1,
		-1+w,w,1+w
	};
	unsigned int max=0;
	unsigned int p;
	unsigned short xx,yy;
	unsigned int htr=0;
	unsigned char flag=1;

	unsigned int *hisp=hp;
	unsigned char *i=hi;

	if(y<0 || x<0 || y >= h || x >= w )return;

	p=x+y*w;
	hisp[htr]=p;
	i[htr]=0;
	O[p]=label;
	xx=x;
	yy=y;
	while(flag)
	{
gexit_for:
		for(;i[htr]<9;i[htr]++)
		{
			if(I[offsets[i[htr]]+hisp[htr]] != 0 && O[offsets[i[htr]]+hisp[htr]] == 0)
			{
				p=offsets[i[htr]]+hisp[htr];
				O[p]=label;
				hisp[++htr]=p;
				i[htr]=0;
				goto gexit_for;
			}
		}
		if(!htr)
			flag=0;
		else
			htr--;
	}
}

void cannygrassfire(unsigned char* output, const unsigned char* input,const unsigned short width ,const unsigned short height)
{
	unsigned short x,y;
	const unsigned short w=width;
	const unsigned short h=height;
	const unsigned int size=h*w;
	unsigned int p,s=0;
	unsigned char* O=output;
	const unsigned char* I=input;
	unsigned int *hp;
	unsigned char *hi;

	hp=(unsigned int*)malloc(size);
	hi=(unsigned char*)malloc(size);


	for(y=0; y<h; y++)
	{
		for(x=0; x<w; x++)
		{
			p=s+x;
			O[p]=0;
			if(O[p] == 0 && I[p] == 255)
				cannygrassfire3x3(O,I,w,h,x,y,255,hp,hi);
		}
		s+=w;
	}

	free(hp);
	free(hi);
}


void nonmaximum_suppression(unsigned char *dst,const unsigned short* vec, const unsigned char *ang , const unsigned short sw, const unsigned short sh, const unsigned char lt, const unsigned char ht)
{
#define nonmax_local_err (0)
	unsigned short x,y;
	const unsigned short w=sw;
	const unsigned short h=sh;
	unsigned int s=0;
	const unsigned char lowthres=lt;
	const unsigned char highthres=ht;
	const unsigned char *A = ang;
	const unsigned short *V = vec;
	unsigned char *O = dst;
	unsigned int p;

	for(y=0; y<h; y++)
	{
		O[s]=0;
		O[s+(w-1)]=0;
		s+=w;
	}
	s=(h-1)*w;
	for(x=w-1;x;x--)
	{
		O[x]=0;
		O[x+s]=0;
	}
	s=0;
	for(y=1;y<h-1;y++)
	{
		s+=w;
		for(x=1;x<w-1;x++)
		{

			p=x+s;
#if 1
			if(V[p]>lowthres)
			{
				if(V[p]<highthres)
					switch (A[p])
				{
					case 1:
						if(V[p]+nonmax_local_err < MAX2(V[p+1],V[p-1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 2:
						if(V[p]+nonmax_local_err < MAX2(V[p+w-1],V[p-w+1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 3:
						if(V[p]+nonmax_local_err < MAX2(V[p+w],V[p-w]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 4:
						if(V[p]+nonmax_local_err < MAX2(V[p+w+1],V[p-w-1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
				}
				else
					switch (A[p])
				{
					case 1:
						if(V[p]+nonmax_local_err < MAX2(V[p+1],V[p-1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 2:
						if(V[p]+nonmax_local_err < MAX2(V[p+w-1],V[p-w+1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 3:
						if(V[p]+nonmax_local_err < MAX2(V[p+w],V[p-w]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 4:
						if(V[p]+nonmax_local_err < MAX2(V[p+w+1],V[p-w-1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
				}
			}
#else
			if(v>lowthres)
			{
				if(v<highthres)
					switch (A[p])
				{
					case 1:
						if(v+nonmax_local_err < MAX2(V[p+1],V[p-1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 2:
						if(v+nonmax_local_err < MAX2(V[p+w-1],V[p-w+1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 3:
						if(v+nonmax_local_err < MAX2(V[p+w],V[p-w]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
					case 4:
						if(v+nonmax_local_err < MAX2(V[p+w+1],V[p-w-1]) )
							O[p]=0;
						else
							//O[p]=V[p];
							O[p]=127;
						break;
				}
				else
					switch (A[p])
				{
					case 1:
						if(v+nonmax_local_err < MAX2(V[p+1],V[p-1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 2:
						if(v+nonmax_local_err < MAX2(V[p+w-1],V[p-w+1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 3:
						if(v+nonmax_local_err < MAX2(V[p+w],V[p-w]) )
							O[p]=0;
						else
							O[p]=255;
						break;
					case 4:
						if(v+nonmax_local_err < MAX2(V[p+w+1],V[p-w-1]) )
							O[p]=0;
						else
							O[p]=255;
						break;
				}
			}
#endif
			else
				O[p]=0;
		}
	}

}

void gaussian_bit(unsigned char* dst , const unsigned char *src,const short sw, const short sh)
{

	const unsigned short w=sw;
	const unsigned short h=sh;
	const unsigned char *I = src;
	short x,y;
	unsigned char *O = dst;
	unsigned int s=0;
	unsigned int d;
	unsigned int p;

	for(y=1;y<h-1;y++)
	{
		s+=w;
		for(x=w-1;x;x--)
		{
			p=s+x;
			d=I[p-w-1];
			d+=I[p-w]<<1;
			d+=I[p-w+1];
			d+=I[p-1]<<1;
			d+=I[p]<<2;
			d+=I[p+1]<<1;
			d+=I[p+w-1];
			d+=I[p+w]<<1;
			d+=I[p+w+1];
			O[p]=d>>4;
		}
	}
	s=0;
	for(y=1; y<h-1; y++)
	{
		s+=w;
		d=I[s-w];
		d+=I[s-w]<<1;
		d+=I[s-w+1];
		d+=I[s]<<1;
		d+=I[s]<<2;
		d+=I[s+1]<<1;
		d+=I[s+w];
		d+=I[s+w]<<1;
		d+=I[s+w+1];
		O[s]=d>>4;	

		p=s+w-1;
		d=I[p-w-1];
		d+=I[p-w]<<1;
		d+=I[p-w];
		d+=I[p-1]<<1;
		d+=I[p]<<2;
		d+=I[p]<<1;
		d+=I[p+w-1];
		d+=I[p+w]<<1;
		d+=I[p+w];

		O[p]=d>>4;
	}

	s=w*(h-1);
	for(x=w-1;x;x--)
	{
		d=I[x-1];
		d+=I[x]<<1;
		d+=I[x+1];
		d+=I[x-1]<<1;
		d+=I[x]<<2;
		d+=I[x+1]<<1;
		d+=I[x+w-1];
		d+=I[x+w]<<1;
		d+=I[x+w+1];
		O[x]=d>>4;

		p=s+x;
		d=I[p-w-1];
		d+=I[p-w]<<1;
		d+=I[p-w+1];
		d+=I[p-1]<<1;
		d+=I[p]<<2;
		d+=I[p+1]<<1;
		d+=I[p-1];
		d+=I[p]<<1;
		d+=I[p+1];
		O[p]=d>>4;
	}

	d=I[0];
	d+=I[0]<<1;
	d+=I[1];
	d+=I[0]<<1;
	d+=I[0]<<2;
	d+=I[1]<<1;
	d+=I[w];
	d+=I[w]<<1;
	d+=I[w+1];
	O[0]=d>>4;

	d=I[p-w-1];
	d+=I[p-w]<<1;
	d+=I[p-w];
	d+=I[p-1]<<1;
	d+=I[p]<<2;
	d+=I[p]<<1;
	d+=I[p-1];
	d+=I[p]<<1;
	d+=I[p];
	O[p]=d>>4;

	d=I[s-w];
	d+=I[s-w]<<1;
	d+=I[s-w+1];
	d+=I[s]<<1;
	d+=I[s]<<2;
	d+=I[s+1]<<1;
	d+=I[s];
	d+=I[s]<<1;
	d+=I[s+1];
	O[s]=d>>4;

	s=w-1;
	d=I[s-1];
	d+=I[s]<<1;
	d+=I[s];
	d+=I[s-1]<<1;
	d+=I[s]<<2;
	d+=I[s]<<1;
	d+=I[s+w-1];
	d+=I[s+w]<<1;
	d+=I[s+w];
	O[s]=d>>4;

}


void GradientAngle_vec(unsigned char* ang_dst,unsigned short* vec_dst, unsigned char* src, short width, short height)
{
	const unsigned short w=width;
	const unsigned short h=height;
	const unsigned char* I=src;
	unsigned short x,y;
	unsigned int p;
	unsigned int s=0;
	unsigned char* O=ang_dst;
	unsigned short* O2=vec_dst;
	short gx,gy;
	double seta;
	for(y=1;y<h-1;y++)
	{
		s+=w;
		for(x=1;x<w-1;x++)
		{
			p=s+x;

			gx =((I[p-w+1]+
				(I[p+1]<<1)+
				I[p+w+1]
			-I[p-w-1]
			-(I[p-1]<<1)
				-I[p+w-1]));

			gy =((I[p-w-1]+
				(I[p-w]<<1)+
				I[p-w+1]
			-I[p+w-1]
			-(I[p+w]<<1)
				-I[p+w+1]));

			O2[p]=ABS(gx)+ABS(gy);

			seta=atan2((double)(gy),(double)(gx));

			if(seta < - 2.7488935718910690836548129603696)
				O[p]=1;
			else if(seta < -1.9634954084936207740391521145497)
				O[p]=2;
			else if(seta < -1.1780972450961724644234912687298)
				O[p]=3;
			else if(seta < -0.39269908169872415480783042290994)
				O[p]=4;
			else if(seta < 0.39269908169872415480783042290994)
				O[p]=1;
			else if(seta < 1.1780972450961724644234912687298)
				O[p]=2;
			else if(seta < 1.9634954084936207740391521145497)
				O[p]=3;
			else if(seta < 2.7488935718910690836548129603696)
				O[p]=4;
			else
				O[p]=1;

		}
	}

}

void function_canny_main(unsigned char *dst, const unsigned char *src, const unsigned short width, const unsigned short height , const unsigned char lowthres, const unsigned char highthres)
{
	unsigned char *gaussian;
	unsigned char *seta;
	unsigned short *vec;
	unsigned char *nonmax;
	unsigned char *canny = dst;
	const unsigned short w = width;
	const unsigned short h = height;
	const unsigned int size = w*h;
	gaussian=(unsigned char*)malloc(size);
	seta=(unsigned char*)malloc(size);
	vec=(unsigned short*)malloc(size<<1);
	nonmax=(unsigned char*)malloc(size);

	gaussian_bit(gaussian,src,w,h);
	GradientAngle_vec(seta,vec,gaussian,w,h);
	nonmaximum_suppression(nonmax,vec,seta,w,h,lowthres,highthres);
	cannygrassfire(canny,nonmax,w,h);


	free(gaussian);
	free(seta);
	free(vec);
	free(nonmax);
}
