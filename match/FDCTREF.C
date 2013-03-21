/* fdctref.c, ǰ����ɢ���ұ任��˫����      */


extern unsigned int doublePrecision;
#include <math.h>

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.14159265358979323846
# endif
#endif

/* ����ȫ�ֺ��� */
void init_fdct(void);
void fdct(short *block);

/*����˽�б���*/
static double c[8][8];     /*���ø�������ʱ��ת��ϵ��*/
static short   c2[8][8];   /*������������ʱ��ת��ϵ��*/


/* �ص��ɳ���������ת����˫��������ʱ�����ľ�����Ϣ*/
#pragma warning( disable : 4244 )

void init_fdct()
{
	int i, j;
	double s;
	double sTmp;

	for (i=0; i<8; i++)
	{
		s = (i==0) ? sqrt(0.125) : 0.5;

		for (j=0; j<8; j++)
		{
			c[i][j] = s * cos((PI/8.0)*i*(j+0.5));
			/* �����������ݲ�����14λ*/
			sTmp = c[i][j] * 16384;
			c2[i][j] = floor(sTmp+0.499999);
		}
	}
}

/*�ú����������DCT�任*/


void fdct(block)
short *block;
{
	int        i, j, k;
	short*     pBlock;
	short*     pC;
	long       s2;
	short      tmp2[64];
	short*     pTmp2;
	double     s;
	double     tmp[64];


	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
		{
		/*
		* ������õ���MMX���������������������������������������ٴ�������
		* 
		*/
		if (!doublePrecision)
		{
			s2 = 0;
			pC = (short*)c2 + (j*8);
			pBlock = block + (8*i);

			__asm {
			mov         eax, pBlock       ;
			movq        mm0, [eax]        ; //����0-3�����ַmm0
			movq        mm1, [eax+8]      ; //����4-7�����ַmm1
			mov         eax, pC           ;
			movq        mm3, [eax]        ;
			movq        mm4, [eax+8]      ;
			pmaddwd     mm0, mm3          ;
			pmaddwd     mm1, mm4          ;

			movd        eax, mm0          ; 
			psrlq       mm0, 0x20         ;
			movd        ebx, mm0          ; 
			add         eax, ebx

			movd        ebx, mm1          ; 
			add         eax, ebx
			psrlq       mm1, 0x20         ;
			movd        ebx, mm1          ; 
			add         eax, ebx

			sar         eax, 14           ;  // ����������14 bits
			jnc         NO_ROUNDING1      ; // �������뵽���������
			inc         eax
NO_ROUNDING1:
            mov         s2, eax
			emms
			}
			tmp2[8*j+i] = s2;
		}
		else
		/* ������õ��Ǹ�������ķ���*/
		{
			s = 0.0;
			for (k=0; k<8; k++)
				s += c[j][k] * block[8*i+k];
			tmp[8*i+j] = s;
		}
    }

	for (j=0; j<8; j++)
		for (i=0; i<8; i++)
		{
			if (!doublePrecision)
			{
				s2 = 0;
				pC = (short*)c2 + (i*8);
				pTmp2 = tmp2 + (j*8);

				__asm
				{
				mov         eax, pTmp2        ;
				movq        mm0, [eax]        ; 
				movq        mm1, [eax+8]      ; 
				mov         eax, pC           ;
				movq        mm3, [eax]        ;
				movq        mm4, [eax+8]      ;
				pmaddwd     mm0, mm3          ;
				pmaddwd     mm1, mm4          ;

				movd        eax, mm0          ; 
				psrlq       mm0, 0x20         ;
				movd        ebx, mm0          ; 
				add         eax, ebx

				movd        ebx, mm1          ; 
				add         eax, ebx
				psrlq       mm1, 0x20         ;
				movd        ebx, mm1          ; 
				add         eax, ebx
				sar         eax, 14           ; 
				jnc         NO_ROUNDING2      ; 
				inc         eax
NO_ROUNDING2:
				mov         s2, eax
				emms
			}
			block[8*i+j] = s2;
		}
		else
		{
			s = 0.0;
			for (k=0; k<8; k++)
				s += c[i][k] * tmp[8*k+j];
			block[8*i+j] = (short)floor(s+0.499999);
		}
	}
}

#pragma warning( default : 4244 )