/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	This file is a derivative work for use in Oxygen Engine.
*	It's based on the Genesis Plus GX source code, original license see below.
*/

/*
**
** software implementation of Yamaha FM sound generator (YM2612/YM3438)
**
** Original code (MAME fm.c)
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta)
**
** Additional code & fixes by Eke-Eke for Genesis Plus GX
**
*/

#pragma once

#include <rmxbase.h>

namespace soundemulation
{
	class YM2612
	{
	public:
		void init();
		void config(unsigned char dac_bits);
		void resetChip();
		void update(int *buffer, int length);
		void write(unsigned int a, unsigned int v);

	private:
		struct FM_SLOT	/* struct describing a single operator (SLOT) */
		{
			int32   *DT;        /* detune          :dt_tab[DT]      */
			uint8   KSR;        /* key scale rate  :3-KSR           */
			uint32  ar;         /* attack rate                      */
			uint32  d1r;        /* decay rate                       */
			uint32  d2r;        /* sustain rate                     */
			uint32  rr;         /* release rate                     */
			uint8   ksr;        /* key scale rate  :kcode>>(3-KSR)  */
			uint32  mul;        /* multiple        :ML_TABLE[ML]    */

			/* Phase Generator */
			uint32  phase;      /* phase counter */
			int32   Incr;       /* phase step */

			/* Envelope Generator */
			uint8   state;      /* phase type */
			uint32  tl;         /* total level: TL << 3 */
			int32   volume;     /* envelope counter */
			uint32  sl;         /* sustain level:sl_table[SL] */
			uint32  vol_out;    /* current output from EG circuit (without AM from LFO) */

			uint8  eg_sh_ar;    /*  (attack state)  */
			uint8  eg_sel_ar;   /*  (attack state)  */
			uint8  eg_sh_d1r;   /*  (decay state)   */
			uint8  eg_sel_d1r;  /*  (decay state)   */
			uint8  eg_sh_d2r;   /*  (sustain state) */
			uint8  eg_sel_d2r;  /*  (sustain state) */
			uint8  eg_sh_rr;    /*  (release state) */
			uint8  eg_sel_rr;   /*  (release state) */

			uint8  ssg;         /* SSG-EG waveform  */
			uint8  ssgn;        /* SSG-EG negated output  */

			uint8  key;         /* 0=last key was KEY OFF, 1=KEY ON */

			/* LFO */
			uint32  AMmask;     /* AM enable flag */
		};

		struct FM_CH
		{
			FM_SLOT  SLOT[4];     /* four SLOTs (operators) */

			uint8   ALGO;         /* algorithm */
			uint8   FB;           /* feedback shift */
			int32   op1_out[2];   /* op1 output for feedback */

			int32   *connect1;    /* SLOT1 output pointer */
			int32   *connect3;    /* SLOT3 output pointer */
			int32   *connect2;    /* SLOT2 output pointer */
			int32   *connect4;    /* SLOT4 output pointer */

			int32   *mem_connect; /* where to put the delayed sample (MEM) */
			int32   mem_value;    /* delayed sample (MEM) value */

			int32   pms;          /* channel PMS */
			uint8   ams;          /* channel AMS */

			uint32  fc;           /* fnum,blk */
			uint8   kcode;        /* key code */
			uint32  block_fnum;   /* blk/fnum value (for LFO PM calculations) */
		};

		struct FM_ST
		{
			uint16  address;        /* address register     */
			uint8   status;         /* status flag          */
			uint32  mode;           /* mode  CSM / 3SLOT    */
			uint8   fn_h;           /* freq latch           */
			int32   TA;             /* timer a value        */
			int32   TAL;            /* timer a base         */
			int32   TAC;            /* timer a counter      */
			int32   TB;             /* timer b value        */
			int32   TBL;            /* timer b base         */
			int32   TBC;            /* timer b counter      */
			int32   dt_tab[8][32];  /* DeTune table         */
		};

		/* OPN 3slot struct */
		struct FM_3SLOT
		{
			uint32  fc[3];          /* fnum3,blk3: calculated */
			uint8   fn_h;           /* freq3 latch */
			uint8   kcode[3];       /* key code */
			uint32  block_fnum[3];  /* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
			uint8   key_csm;        /* CSM mode Key-ON flag */
		};

		struct FM_OPN	/* OPN/A/B common state */
		{
			FM_ST  ST;                  /* general state */
			FM_3SLOT SL3;               /* 3 slot mode state */
			unsigned int pan[6 * 2];      /* fm channels output masks (0xffffffff = enable) */

										  /* EG */
			uint32  eg_cnt;             /* global envelope generator counter */
			uint32  eg_timer;           /* global envelope generator counter works at frequency = chipclock/144/3 */

										/* LFO */
			uint8   lfo_cnt;            /* current LFO phase (out of 128) */
			uint32  lfo_timer;          /* current LFO phase runs at LFO frequency */
			uint32  lfo_timer_overflow; /* LFO timer overflows every N samples (depends on LFO frequency) */
			uint32  LFO_AM;             /* current LFO AM step */
			uint32  LFO_PM;             /* current LFO PM step */
		};

	private:
		void FM_KEYON(FM_CH *CH, int s);
		void FM_KEYOFF(FM_CH *CH, int s);
		void FM_KEYON_CSM(FM_CH *CH, int s);
		void FM_KEYOFF_CSM(FM_CH *CH, int s);
		void CSMKeyControll(FM_CH *CH);
		void INTERNAL_TIMER_A();
		void INTERNAL_TIMER_B(int step);
		void set_timers(int v);
		void setup_connection(FM_CH *CH, int ch);
		void set_det_mul(FM_CH *CH, FM_SLOT *SLOT, int v);
		void set_tl(FM_SLOT *SLOT, int v);
		void set_ar_ksr(FM_CH *CH, FM_SLOT *SLOT, int v);
		void set_dr(FM_SLOT *SLOT, int v);
		void set_sr(FM_SLOT *SLOT, int v);
		void set_sl_rr(FM_SLOT *SLOT, int v);
		void advance_lfo();
		void advance_eg_channels(FM_CH *CH, unsigned int eg_cnt);
		void update_ssg_eg_channels(FM_CH *CH);
		void update_phase_lfo_slot(FM_SLOT *SLOT, int32 pms, uint32 block_fnum);
		void update_phase_lfo_channel(FM_CH *CH);
		void refresh_fc_eg_slot(FM_SLOT *SLOT, unsigned int fc, unsigned int kc);
		void refresh_fc_eg_chan(FM_CH *CH);
		void chan_calc(FM_CH *CH, int num);
		void OPNWriteMode(int r, int v);
		void OPNWriteReg(int r, int v);
		static void reset_channels(FM_CH *CH, int num);
		void init_tables();

	private:
		FM_CH   mChannels[6];  /* channel state */
		uint8   dacen;  /* DAC mode  */
		int32   dacout; /* DAC output */
		FM_OPN  OPN;    /* OPN state */

		int32  m2, c1, c2;   /* Phase Modulation input for operators 2,3,4 */
		int32  mem;        /* one sample delay memory */
		int32  out_fm[8];  /* outputs of working channels */
		uint32 bitmask;    /* working channels output bitmasking (DAC quantization) */
	};
}
