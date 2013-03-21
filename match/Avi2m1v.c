/* avi2m1v.c  - 
 * ���ļ�Ϊ��Ϊmpeg2enc.c�ĳ��Σ��ڴ˻����ϲ�����avi2mpg1����                    
 * ��֧�ֶ������avi�ļ���
*/



#include <windows.h>
#include <vfw.h>

#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "avi2mpg1.h"
#include "video.h"


/* ����˽�к��� */
static void init(void);
static void setparm(void);
static void initquantmat(void);

int avi2m1v(outputfile)

char *outputfile;
{
	/* �趨���� */
	setparm();

	/* ��ʼ���������� */
	initquantmat();

	/* ������ļ� */
	if (!(outfile=fopen(outputfile,"wb")))
		fprintf(stderr, "\nCouldn't create video stream file %s", outputfile);

	init();
	putseq();

	fclose(outfile);

#ifdef DEBUG  
  fclose(statfile);
#endif

  return 0;
}

void error(text)
char *text;
{
  putc('\n',stderr);
  fprintf(stderr,text);
  putc('\n',stderr);
  exit(1);
}

static void init()
{
  int i, size;
  static int block_count_tab[3] = {6,8,12};

  initbits();//��ʼ��64k�Ļ���
  init_fdct();//��ʼ��fdct�任�õ�������c[8][8] c2[8][8]
  init_idct();

  /* ��ͼ��Ĵ�С�������뵽������ӽ���16��32�ı���û��ʹ��fieldpic��prog_seqΪ1*/
  mb_width = (horizontal_size+15)/16;
  mb_height = prog_seq ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);
  mb_height2 = fieldpic ? mb_height>>1 : mb_height; /* for field pictures */
  width = 16*mb_width;
  height = 16*mb_height;
  /*mpeg1ʹ��420��ɫ�ȸ�ʽ*/
  chrom_width = (chroma_format==CHROMA444) ? width : width>>1;
  chrom_height = (chroma_format!=CHROMA420) ? height : height>>1;

  height2 = fieldpic ? height>>1 : height;
  width2 = fieldpic ? width<<1 : width;
  chrom_width2 = fieldpic ? chrom_width<<1 : chrom_width;
  
  block_count = block_count_tab[chroma_format-1];

  if (!(clp = (unsigned char *)malloc(1024)))
    error("clip table malloc failed\n");
  clp+= 384;
  for (i=-384; i<640; i++)
    clp[i] = (i<0) ? 0 : ((i>255) ? 255 : i);

  /*Ϊ����֡�����ڴ�*/
  for (i=0; i<3; i++)
  {
    size = (i==0) ? width*height : chrom_width*chrom_height;

    if (!(newrefframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(oldrefframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(auxframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(neworgframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(oldorgframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(auxorgframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
    if (!(predframe[i] = (unsigned char *)malloc(size)))
      error("malloc failed\n");
  }

  // ����Ϊͼ��Ķ�ȡ����U��V����
  if (!(Ubuffer = (unsigned char *)malloc(width*height)))
	error("malloc failed\n");

  if (!(Vbuffer = (unsigned char *)malloc(width*height)))
	error("malloc failed\n");

  if (!(Ubuffer422 = (unsigned char *)malloc(width*height)))
	error("malloc failed\n");

  if (!(Vbuffer422 = (unsigned char *)malloc(width*height)))
	error("malloc failed\n");

  if (!(TempFilterBuffer = (unsigned char *)malloc(width*height)))
	error("malloc failed\n");

  /*����mb_width*mb_height2���ṹ����ڴ�*/
  mbinfo = (struct mbinfo *)malloc(mb_width*mb_height2*sizeof(struct mbinfo));

  if (!mbinfo)
    error("malloc failed\n");
  /*����8*8���ݿ��ڴ�*/
  blocks =
    (short (*)[64])malloc(mb_width*mb_height2*block_count*sizeof(short [64]));

  if (!blocks)
    error("malloc failed\n");

#ifdef DEBUG
  if (statname[0]=='-')
    statfile = stdout;
  else if (!(statfile = fopen(statname,"w")))
  {
    sprintf(errortext,"Couldn't create statistics output file %s",statname);
    error(errortext);
  }
#endif
}

static void setparm()
{
  int i;
  int h,m,s,f;
  static double ratetab[8]=
    {24000.0/1001.0,24.0,25.0,30000.0/1001.0,30.0,50.0,60000.0/1001.0,60.0};
  extern int r,Xi,Xb,Xp,d0i,d0p,d0b; /* rate control */
  extern double avg_act; /* rate control */
  char line[256];
  char line1[256];
  char temp[256]="Encoded by AVI2MPG1 Version ";

  
  
  strcpy(tplref, "-");
  strcpy(statname, "nul:");
  i = (int)((float)masterPfi.dwRate/(float)masterPfi.dwScale*100.0);

  frame_rate_code = 0;
  frame_repl = 1;
  /*����֡������ֵ*/
  if(!forced_frame_rate)
  {
	if((i>950)&&(i<1010)) // 10 fps -> 30 fps
	{
		frame_rate_code = 5;
		frame_repl = 3;
	}
	if((i>1150)&&(i<1225)) // 12 fps -> 24 fps
	{
		frame_rate_code = 2;
		frame_repl = 2;
	}
	if((i>=12.25)&&(i<12.75)) // 12.5 fps -> 25 fps
	{
		frame_rate_code = 3;
		frame_repl = 2;
	}
	if((i>1450)&&(i<1550)) // 15 fps -> 30 fps
	{
		frame_rate_code = 5;
		frame_repl = 2;
	}
	if((i>2350)&&(i<2400))
		frame_rate_code = 1;
	if((i>=2400)&&(i<2450))
		frame_rate_code = 2;
	if((i>2450)&&(i<2550))
		frame_rate_code = 3;
	if((i>2950)&&(i<3000))
		frame_rate_code = 4;
	if((i>=3000)&&(i<3050))
		frame_rate_code = 5;
	if(frame_rate_code == 0)
	{
		fprintf(stderr, "\nsource frame rate must be 10, 12, 12.5, 15, 23.976, 24, 25, 29.97, or 30 FPS!");
		fprintf(stderr, "\nif you don not require audio, try using forced frame rate option (-f).\n");
	    exit(2);
	}
  }
  else
  {
	  frame_rate_code = forced_frame_rate;
  }
  /*vcd_parm������ܳ�����û��ʵ�֣������Կ���*/
  if(vcd_parm)
  {
	crop_size = 1;
	crop_horz = 352;
	if((frame_rate_code<2)||(frame_rate_code>3))
		crop_vert = 240;
	else
		crop_vert = 288;
  }


  inputtype = 1;

  nframes = (totalFrames * frame_repl) - 1;//����ܵ�֡��

  frame0 = 0;

  fieldpic = 0; // frame pictures only for mpeg1

  if(!crop_size)
  {
	  
	  horizontal_size = ((masterPfi.dwWidth + 15)>>4)<<4; // make it multiple of 16
	  vertical_size = ((masterPfi.dwHeight + 1)>>1)<<1;
  }
  else
  {
	  horizontal_size = ((crop_horz + 1)>>1)<<1; // make it even
	  vertical_size = ((crop_vert + 1)>>1)<<1;
  }

  /*�ó������������Ƶ���ı����ʣ�����system_byterateΪ�������ֽ���
   Ĭ��ֵΪ150��audio_bitrate_paramΪ��Ƶ���ı����ʣ�Ĭ��ֵΪ128
  */
  if(vcd_parm)
      bit_rate = 1040000;  
	  //bit_rate = 1152000;
  else if(video_only)
	  bit_rate = system_byterate_parm*8*1024;
  else
  {
      bit_rate = (float)(system_byterate_parm*8*1024) - (float)(audio_bitrate_parm*1024) - (float)(4*8*1024);
	  bit_rate = bit_rate * 0.9; 
	  if(bit_rate <= 81920)
	  {
		  fprintf(stderr, "\n specified parameters would result in video bitrate of less than 81920!");
		  fprintf(stderr, "\n Try adjusting audio bit-rate (-a), or use video only stream (-n)\n");
		  exit(3);
	  }
  }

  fprintf(stderr, " using bitrate of %u", (unsigned int)bit_rate);
  fprintf(stderr, ":\n");

  /*��֤ͼ��Ŀ��Ϊ16�ı�������λż��*/
  if((masterPfi.dwWidth != (unsigned)horizontal_size)||
     (masterPfi.dwHeight != (unsigned)vertical_size))
  {
	  fprintf(stderr, "*** WARNING: cropping video to %i x %i\n", horizontal_size, vertical_size);
  }

  /*����dwRate��dwScale������ȷ��frame_rate_code����frame_rep1,Ȼ�����ȷ���µ�֡��*/
  if(frame_repl!=1)
  {
	  if(frame_rate_code == 3)
		fprintf(stderr, "Source FPS of 12.5 will be upsampled to 25 FPS,");
	  else
		fprintf(stderr, "Source FPS of %u will be upsampled to %u FPS,", (i+50)/100, (i+50)/100*frame_repl);
	  fprintf(stderr, " New total frames = %u\n", nframes);
  }

  if(forced_frame_rate)
  {
	  fprintf(stderr, "\n Forcing frame rate of ");
	  switch (forced_frame_rate)
	  {
		case 1:
		  fprintf(stderr, "23.976");
		break;
		case 2:
		  fprintf(stderr, "24");
		break;
		case 3:
		  fprintf(stderr, "25");
		break;
		case 4:
		  fprintf(stderr, "29.97");
		break;
		case 5:
		  fprintf(stderr, "30");
		break;
	  }
	  fprintf(stderr, " FPS, video only stream.\n");
  }

  // bit_rate = 1856000;  
  low_delay = 0;     
  prog_seq = 1;
  chroma_format = 1; // 4:2:0
  display_horizontal_size = masterPfi.dwWidth;
  display_vertical_size = masterPfi.dwHeight;
  conceal_tab[0] = 0;
  conceal_tab[1] = 0;
  conceal_tab[2] = 0;
  topfirst = 0;
  prog_frame = 1;

  strcpy(id_string, "encoded by avi2mpg1 ver ");
  strcat(id_string, VERSION);

// ��ȡ�����ļ�

      GetPrivateProfileString("matrix","iqname","-",iqname,254,video_param_filename);
      GetPrivateProfileString("matrix","niqname","-",niqname,254,video_param_filename);
      GetPrivateProfileString("timecode","tc_of1stframe","00:00:00:00",line,254,video_param_filename); sscanf(line,"%d:%d:%d:%d",&h,&m,&s,&f);
      N=GetPrivateProfileInt("frameinfo","nrframesinGOP",15,video_param_filename);
      M=GetPrivateProfileInt("frameinfo","ipframedist",3,video_param_filename);
      aspectratio=GetPrivateProfileInt("image","aspectratio",8,video_param_filename);
      profile=GetPrivateProfileInt("image","prof_id",4,video_param_filename);
      video_format=GetPrivateProfileInt("image","vid_format",1,video_param_filename);
      level=GetPrivateProfileInt("image","lev_id",10,video_param_filename);
      vbv_buffer_size=GetPrivateProfileInt("buffer","vbv_buf",20,video_param_filename);
      wr_seqh2gop=GetPrivateProfileInt("flags","wr_seqh2gop",wr_seqh2gop,video_param_filename);
      constrparms=GetPrivateProfileInt("flags","constrparm",1,video_param_filename);
      color_primaries=GetPrivateProfileInt("color","col_primaries",5,video_param_filename);
      transfer_characteristics=GetPrivateProfileInt("color","trans_ch",5,video_param_filename);
      matrix_coefficients=GetPrivateProfileInt("color","matrix_coeff",5,video_param_filename);
      dc_prec=GetPrivateProfileInt("color","intradc_prec",0,video_param_filename);
      GetPrivateProfileString("frame","frame_pred_dct","1 1 1",line,254,video_param_filename); sscanf(line,"%d %d %d",
        frame_pred_dct_tab,frame_pred_dct_tab+1,frame_pred_dct_tab+2);
      GetPrivateProfileString("frame","q_scale_type","0 0 0",line,254,video_param_filename); sscanf(line,"%d %d %d",
        qscale_tab,qscale_tab+1,qscale_tab+2);
      GetPrivateProfileString("frame","intra_vlc_format","0 0 0",line,254,video_param_filename); sscanf(line,"%d %d %d",
        intravlc_tab,intravlc_tab+1,intravlc_tab+2);
      GetPrivateProfileString("frame","alternate_scan","0 0 0",line,254,video_param_filename); sscanf(line,"%d %d %d",
        altscan_tab,altscan_tab+1,altscan_tab+2);
      repeatfirst=GetPrivateProfileInt("flags","repeat_first_field",0,video_param_filename);
      P=GetPrivateProfileInt("slice","p_dist",0,video_param_filename);
      r=GetPrivateProfileInt("motion","rate_control",0,video_param_filename);
      GetPrivateProfileString("motion","avg_act","0",line,254,video_param_filename); sscanf(line,"%lf",&avg_act);
      Xi=GetPrivateProfileInt("motion","xi",0,video_param_filename);
      Xp=GetPrivateProfileInt("motion","xp",0,video_param_filename);
      Xb=GetPrivateProfileInt("motion","xb",0,video_param_filename);
      d0i=GetPrivateProfileInt("motion","d0i",0,video_param_filename);
      d0p=GetPrivateProfileInt("motion","d0p",0,video_param_filename);
      d0b=GetPrivateProfileInt("motion","d0b",0,video_param_filename);

      if (wr_seqh2gop<0)
          error("wr_seqh2gop must be 0 or positive");
      if (N<1)
          error("N must be positive");
      if (M<1)
          error("M must be positive");
      if (N%M != 0)
          error("N must be an integer multiple of M");

      motion_data = (struct motion_data *)malloc(M*sizeof(struct motion_data));
      if (!motion_data)
          error("malloc failed\n");

      i=0;
      GetPrivateProfileString("motion_data","p0","2 2 11 11",line,254,video_param_filename);
      sscanf(line,"%d %d %d %d", &motion_data[i].forw_hor_f_code, &motion_data[i].forw_vert_f_code,
        &motion_data[i].sxf, &motion_data[i].syf);

      for (i=1; i<M; i++)
	    {
        sprintf(line,"bf%u",i);
        GetPrivateProfileString("motion_data",line,"1 1 3 3",line1,254,video_param_filename);
        sscanf(line1,"%d %d %d %d", &motion_data[i].forw_hor_f_code, &motion_data[i].forw_vert_f_code,
          &motion_data[i].sxf, &motion_data[i].syf);

        sprintf(line,"bb%u",i);
        GetPrivateProfileString("motion_data",line,"1 1 3 3",line1,254,video_param_filename);
        sscanf(line1,"%d %d %d %d", &motion_data[i].back_hor_f_code, &motion_data[i].back_vert_f_code,
          &motion_data[i].sxb, &motion_data[i].syb);
	    }

	  /*�����ʹ�ò����ļ�*/
      if(!use_v_param_file)
  {
      P = 0;
      r = 0;
      avg_act = 0;
      Xi = 0;
      Xp = 0;
      Xb = 0;
      d0i = 0;
      d0p = 0;
      d0b = 0;

      motion_data = (struct motion_data *)malloc(M*sizeof(struct motion_data));
      if (!motion_data)
          error("malloc failed\n");

	  
      switch (m_search_size)//Ĭ��ֵΪ0
	  {
	  case 0:
			motion_data[0].forw_hor_f_code = motion_data[0].forw_vert_f_code = 2;
			motion_data[0].sxf = motion_data[0].syf = 11;

			motion_data[1].forw_hor_f_code = motion_data[1].forw_vert_f_code = 1;
			motion_data[1].sxf = motion_data[1].syf = 3;

			motion_data[1].back_hor_f_code = motion_data[1].back_vert_f_code = 1;
			motion_data[1].sxb = motion_data[1].syb = 7;

			motion_data[2].forw_hor_f_code = motion_data[2].forw_vert_f_code = 1;
			motion_data[2].sxf = motion_data[2].syf = 7;

			motion_data[2].back_hor_f_code = motion_data[2].back_vert_f_code = 1;
			motion_data[2].sxb = motion_data[2].syb = 3;
	  break;

	  case 1:
			motion_data[0].forw_hor_f_code = motion_data[0].forw_vert_f_code = 2;
			motion_data[0].sxf = motion_data[0].syf = 15;

			motion_data[1].forw_hor_f_code = motion_data[1].forw_vert_f_code = 1;
			motion_data[1].sxf = motion_data[1].syf = 7;

			motion_data[1].back_hor_f_code = motion_data[1].back_vert_f_code = 2;
			motion_data[1].sxb = motion_data[1].syb = 15;

			motion_data[2].forw_hor_f_code = motion_data[2].forw_vert_f_code = 2;
			motion_data[2].sxf = motion_data[2].syf = 15;

			motion_data[2].back_hor_f_code = motion_data[2].back_vert_f_code = 1;
			motion_data[2].sxb = motion_data[2].syb = 7;
	  break;

	  case 2:
			motion_data[0].forw_hor_f_code = motion_data[0].forw_vert_f_code = 3;
			motion_data[0].sxf = motion_data[0].syf = 31;

			motion_data[1].forw_hor_f_code = motion_data[1].forw_vert_f_code = 2;
			motion_data[1].sxf = motion_data[1].syf = 15;

			motion_data[1].back_hor_f_code = motion_data[1].back_vert_f_code = 3;
			motion_data[1].sxb = motion_data[1].syb = 31;

			motion_data[2].forw_hor_f_code = motion_data[2].forw_vert_f_code = 3;
			motion_data[2].sxf = motion_data[2].syf = 31;

			motion_data[2].back_hor_f_code = motion_data[2].back_vert_f_code = 2;
			motion_data[2].sxb = motion_data[2].syb = 15;
	  break;

	  case 3:
			motion_data[0].forw_hor_f_code = motion_data[0].forw_vert_f_code = 4;
			motion_data[0].sxf = motion_data[0].syf = 63;

			motion_data[1].forw_hor_f_code = motion_data[1].forw_vert_f_code = 3;
			motion_data[1].sxf = motion_data[1].syf = 31;

			motion_data[1].back_hor_f_code = motion_data[1].back_vert_f_code = 4;
			motion_data[1].sxb = motion_data[1].syb = 63;

			motion_data[2].forw_hor_f_code = motion_data[2].forw_vert_f_code = 4;
			motion_data[2].sxf = motion_data[2].syf = 63;

			motion_data[2].back_hor_f_code = motion_data[2].back_vert_f_code = 3;
			motion_data[2].sxb = motion_data[2].syb = 31;
	  break;

	  case 4:
			motion_data[0].forw_hor_f_code = motion_data[0].forw_vert_f_code = 4;
			motion_data[0].sxf = motion_data[0].syf = 63;

			motion_data[1].forw_hor_f_code = motion_data[1].forw_vert_f_code = 4;
			motion_data[1].sxf = motion_data[1].syf = 63;

			motion_data[1].back_hor_f_code = motion_data[1].back_vert_f_code = 4;
			motion_data[1].sxb = motion_data[1].syb = 63;

			motion_data[2].forw_hor_f_code = motion_data[2].forw_vert_f_code = 4;
			motion_data[2].sxf = motion_data[2].syf = 63;

			motion_data[2].back_hor_f_code = motion_data[2].back_vert_f_code = 4;
			motion_data[2].sxb = motion_data[2].syb = 63;
	  break;
	  }
  }

  if(vcd_parm)
  {
	vbv_buffer_size = 20;
	aspectratio = 14;
  }

  /* make flags boolean (x!=0 -> x=1) */
  fieldpic = !!fieldpic;//�Ƿ�ʹ�ó�ͼƬ���
  low_delay = !!low_delay;
  constrparms = !!constrparms; /* Լ��������־(MPEG-1) */
  prog_seq = !!prog_seq; /* ǰ������ */
  topfirst = !!topfirst;/* ����ʾ�ϰ벿�ֳ�ͼ�� */

  for (i=0; i<3; i++)
  {
	frame_pred_dct_tab[i] = !!frame_pred_dct_tab[i];/* ����֡Ԥ���֡DCT (I,P,B,current) */
    conceal_tab[i] = !!conceal_tab[i];              /* ʹ�������˶����� */
    qscale_tab[i] = !!qscale_tab[i];                /* ����/������������ */
    intravlc_tab[i] = !!intravlc_tab[i];            /* ֡��vlc��ʽ (I,P,B,current) */
    altscan_tab[i] = !!altscan_tab[i];              /* ����ɨ�跽ʽ(I,P,B,current) */
  }
  repeatfirst = !!repeatfirst;                      /* �ڵڶ���ͼ��֮���ظ���һ��*/
  prog_frame = !!prog_frame;                        /* ǰ��֡ */

  /* ȷ��MPEG���ض���������Ч��*/
  range_checks();

  frame_rate = ratetab[frame_rate_code-1];

  /* timecode -> frame number */
  tc0 = h;
  tc0 = 60*tc0 + m;
  tc0 = 60*tc0 + s;
  tc0 = (int)(frame_rate+0.5)*tc0 + f;

    /* MPEG-1 */
    if (constrparms)
    {
      if (horizontal_size>768
          || vertical_size>576
          || ((horizontal_size+15)/16)*((vertical_size+15)/16)>396
          || ((
		  
		  +15)/16)*((vertical_size+15)/16)*frame_rate>396*25.0
          || frame_rate>30.0)
      {
        if (!quiet)
          fprintf(stderr,"\n*** WARNING: VIDEO EXCEEDS CPB STANDARD!\n");
        constrparms = 0;
      }
    }
   /* constrparms��Լ��������־��quiet�����Ƿ���ʾ����*/
    if (constrparms)
    {
      for (i=0; i<M; i++)
      {
        if (motion_data[i].forw_hor_f_code>4)
        {
          if (!quiet)
            fprintf(stderr,"\n*** WARNING: VIDEO EXCEEDS CPB STANDARD!\n");
          constrparms = 0;
          break;
        }

        if (motion_data[i].forw_vert_f_code>4)
        {
          if (!quiet)
            fprintf(stderr,"\n*** WARNING: VIDEO EXCEEDS CPB STANDARD!\n");
          constrparms = 0;
          break;
        }

        if (i!=0)
        {
          if (motion_data[i].back_hor_f_code>4)
          {
            if (!quiet)
              fprintf(stderr,"\n*** WARNING: VIDEO EXCEEDS CPB STANDARD!\n");
            constrparms = 0;
            break;
          }

          if (motion_data[i].back_vert_f_code>4)
          {
            if (!quiet)
              fprintf(stderr,"\n*** WARNING: VIDEO EXCEEDS CPB STANDARD!\n");
            constrparms = 0;
            break;
          }
        }
      }
    }

	/*ǰ�����У���ɫ��ʽ��֡�ڱ�����dcϵ�����ȣ�
	����/������������֡��vlc��ʽ (I,P,B,current)
	����ɨ�跽ʽ(I,P,B,current)��Ҫ��Ϊ1*/
    if (!prog_seq)
    {
      if (!quiet)
        fprintf(stderr,"\nWarning: setting progressive_sequence = 1\n");
      prog_seq = 1;
    }

    if (chroma_format!=CHROMA420)
    {
      if (!quiet)
        fprintf(stderr,"\nWarning: setting chroma_format = 1 (4:2:0)\n");
      chroma_format = CHROMA420;
    }

    if (dc_prec!=0)
    {
      if (!quiet)
        fprintf(stderr,"\nWarning: setting intra_dc_precision = 0\n");
      dc_prec = 0;
    }

    for (i=0; i<3; i++)
      if (qscale_tab[i])
      {
        if (!quiet)
          fprintf(stderr,"\nWarning: setting qscale_tab[%d] = 0\n",i);
        qscale_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (intravlc_tab[i])
      {
        if (!quiet)
          fprintf(stderr,"\nWarning: setting intravlc_tab[%d] = 0\n",i);
        intravlc_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (altscan_tab[i])
      {
        if (!quiet)
          fprintf(stderr,"\nWarning: setting altscan_tab[%d] = 0\n",i);
        altscan_tab[i] = 0;
      }

  if (prog_seq && !prog_frame)
  {
    if (!quiet)
      fprintf(stderr,"\nWarning: setting progressive_frame = 1\n");
    prog_frame = 1;
  }

  if (prog_frame && fieldpic)
  {
    if (!quiet)
      fprintf(stderr,"\nWarning: setting field_pictures = 0\n");
    fieldpic = 0;
  }

  if (!prog_frame && repeatfirst)
  {
    if (!quiet)
      fprintf(stderr,"\nWarning: setting repeat_first_field = 0\n");
    repeatfirst = 0;
  }

  if (prog_frame)
  {
    for (i=0; i<3; i++)
      if (!frame_pred_dct_tab[i])
      {
        if (!quiet)
          fprintf(stderr,"\nWarning: setting frame_pred_frame_dct[%d] = 1\n",i);
        frame_pred_dct_tab[i] = 1;
      }
  }

  if (prog_seq && !repeatfirst && topfirst)
  {
    if (!quiet)
      fprintf(stderr,"\nWarning: setting top_field_first = 0\n");
    topfirst = 0;
  }

  /* search windows */
  for (i=0; i<M; i++)
  {
    if (motion_data[i].sxf > (4<<motion_data[i].forw_hor_f_code)-1)
    {
      if (!quiet)
        fprintf(stderr,
          "\nWarning: reducing forward horizontal search width to %d\n",
          (4<<motion_data[i].forw_hor_f_code)-1);
      motion_data[i].sxf = (4<<motion_data[i].forw_hor_f_code)-1;
    }

    if (motion_data[i].syf > (4<<motion_data[i].forw_vert_f_code)-1)
    {
      if (!quiet)
        fprintf(stderr,
          "\nWarning: reducing forward vertical search width to %d\n",
          (4<<motion_data[i].forw_vert_f_code)-1);
      motion_data[i].syf = (4<<motion_data[i].forw_vert_f_code)-1;
    }

    if (i!=0)
    {
      if (motion_data[i].sxb > (4<<motion_data[i].back_hor_f_code)-1)
      {
        if (!quiet)
          fprintf(stderr,
            "\nWarning: reducing backward horizontal search width to %d\n",
            (4<<motion_data[i].back_hor_f_code)-1);
        motion_data[i].sxb = (4<<motion_data[i].back_hor_f_code)-1;
      }

      if (motion_data[i].syb > (4<<motion_data[i].back_vert_f_code)-1)
      {
        if (!quiet)
          fprintf(stderr,
            "\nWarning: reducing backward vertical search width to %d\n",
            (4<<motion_data[i].back_vert_f_code)-1);
        motion_data[i].syb = (4<<motion_data[i].back_vert_f_code)-1;
      }
    }
  }

}

static void initquantmat()
{
  int i,v;
  FILE *fd;

  /*��ʼ�����������ݿ��ԴӶ���������л�ȡ��Ҳ�����Ǵ��ⲿ�ļ�����ȡ 
  intra_q[i] ֡�������� recip_intra_q[i]=(16384.0+x/2)/x
  inter_q[i]  ֡��������recip_inter_q[i]

  */
  if (iqname[0]=='-')
  {
    load_iquant = 0;
    for (i=0; i<64; i++)
	{
      intra_q[i] = default_intra_quantizer_matrix[i];
	  recip_intra_q[i] = (unsigned short)((16384.0 + (double) intra_q[i]/2.0)/intra_q[i]);
	}
  }
  else			
  {
    load_iquant = 1;
    if (!(fd = fopen(iqname,"r")))
    {
      sprintf(errortext,"Couldn't open quant intra matrix file %s",iqname);
      error(errortext);
    }

    for (i=0; i<64; i++)
    {
      fscanf(fd,"%d",&v);
      if (v<1 || v>255)
        error("invalid value in quant matrix");
      intra_q[i] = v;
	  recip_intra_q[i] = (unsigned short)((16384.0 + (double) intra_q[i]/2.0)/intra_q[i]);
    }

    fclose(fd);
  }

  if (niqname[0]=='-')
  {
    load_niquant = 0;
    for (i=0; i<64; i++)
	{
      inter_q[i] = 16;
	  recip_inter_q[i] = (unsigned short)((16384.0 + (double) inter_q[i]/2.0)/inter_q[i]);
	}
  }
  else
  {
    load_niquant = 1;
    if (!(fd = fopen(niqname,"r")))
    {
      sprintf(errortext,"Couldn't open quant non-intra matrix file %s",niqname);
      error(errortext);
    }

    for (i=0; i<64; i++)
    {
      fscanf(fd,"%d",&v);
      if (v<1 || v>255)
        error("invalid value in quant matrix");
      inter_q[i] = v;

	  recip_inter_q[i] = (unsigned short)((16384.0 + (double) inter_q[i]/2.0)/inter_q[i]);
    }

    fclose(fd);
  }
}
