#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>

#include "sfx.h"

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;

#include "music_level.h"

#define READ_SONG_T(ch, pos) (song[(ch) * SONGLEN + (pos)])
#define READ_SONG_O(ch, pos) (song[4 * SONGLEN + (ch) * SONGLEN + (pos)])
#define READ_TRACK_N(tr, pos) (song[8 * SONGLEN + (tr) * TRACKLEN + (pos)])
#define READ_TRACK_C(tr, pos) (song[8 * SONGLEN + NUMTRACKS * TRACKLEN + (tr) * TRACKLEN + (pos)])

#define REVERBLENGTH 1024

#include "drums.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

enum {
	CHP_FREQDUP = 0x1,
	CHP_FREQDDN = 0x2,
	CHP_SUSTAIN = 0x3,
	CHP_RELEASE = 0x4,
	CHP_VIBRRATE = 0x5,
	CHP_WAVEFORM = 0x6,
	CHP_VOLDUP = 0x9,
	CHP_VOLDDN = 0xA,
	CHP_VIBRDEPTH = 0xB,
	CHP_VOL = 0xC,
	CHP_ECHO = 0xE
};

enum {
	WF_PULSE = 0,
	WF_TRI = 0x8,
	WF_NOISE = 0xF
};

static struct channel {
	u8	param[16];	// these get written by all commands; some of them are then read
	u16	phase;
	u16	freq;
	u16	vibrphase;
	u16	sustaintimer;
	u8	echonotes[5];
	u8	pad[3]; // makes the struct 32 bytes
} channel[4], initchannel[4] = {
	{
		.param[CHP_WAVEFORM] = WF_TRI,
		.param[CHP_SUSTAIN] = 0x0,
		.param[CHP_RELEASE] = 0x0,
		.param[CHP_VIBRRATE] = 0x8
	},
	{
		.param[CHP_WAVEFORM] = WF_PULSE + 7,
		.param[CHP_SUSTAIN] = 0x0,
		.param[CHP_RELEASE] = 0x0,
		.param[CHP_VIBRRATE] = 0x8
	},
	{
		.param[CHP_WAVEFORM] = WF_PULSE + 3,
		.param[CHP_SUSTAIN] = 0x3,
		.param[CHP_RELEASE] = 0x9,
		.param[CHP_VIBRRATE] = 0x8
	},
	{
		.param[CHP_WAVEFORM] = WF_NOISE,
		.param[CHP_SUSTAIN] = 0x1,
		.param[CHP_RELEASE] = 0x6,
		.param[CHP_VIBRRATE] = 0x0
	}
};

static struct player {
	u8	*drumptr;
	u16	songpos;
	u16	noise;
	u16	divtimer;
	u8	trackpos;
	u8	ttimer;
	s8	drumlevel;
	u8	drumvol;
} player, initplayer = {
	.noise = 1
};

static volatile u8 allow_advance;
static volatile u16 bleepfreq, bleepvolume;
static u16 bleepphase;

static u8 *drums[2] = {kickdata, snaredata};

static u8 shiftm1[16] = {
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000f, 0x001f, 0x003f, 0x007f
};
//#define SHIFTM1(x) ((1 << (x)) - 1)
#define SHIFTM1(x) (shiftm1[(x)])

static s16 nextsample() {
	int c;
	int acc;

	if(!player.divtimer) {
		if(!player.ttimer) {
			player.ttimer = SPEED;
			for(c = 0; c < 4; c++) {
				struct channel *ch = &channel[c];
				u8 note, cmd, trk, hi, lo, ivol = 0xf;

				trk = READ_SONG_T(c, player.songpos);
				note = READ_TRACK_N(trk, player.trackpos);
				cmd = READ_TRACK_C(trk, player.trackpos);
				if(note) note += READ_SONG_O(c, player.songpos);

#ifdef CMD_E_USED
				//ch->echonotes[3] = ch->echonotes[2];
				//ch->echonotes[2] = ch->echonotes[1];
				//ch->echonotes[1] = ch->echonotes[0];
				*((u32 *) &ch->echonotes[1]) = *((u32 *) &ch->echonotes[0]);
				ch->echonotes[0] = note;

				if(!note && ch->param[CHP_ECHO]) {
					note = ch->echonotes[3];
					ivol = ch->param[CHP_ECHO];
				}
#endif
				if(note) {
					ch->freq = pow(1.059463094, note + 60);
#ifdef CMD_1_USED
					ch->param[CHP_FREQDUP] = 0;
#endif
#ifdef CMD_2_USED
					ch->param[CHP_FREQDDN] = 0;
#endif
#ifdef CMD_9_USED
					ch->param[CHP_VOLDUP] = 0;
#endif
					ch->param[CHP_VOLDDN] = 0;
#ifdef CMD_B_USED
					ch->param[CHP_VIBRDEPTH] = 0;
#endif
					ch->param[CHP_VOL] = ivol << 4;
					ch->sustaintimer = SHIFTM1(ch->param[CHP_SUSTAIN]);
				}
				hi = cmd >> 4;
				lo = cmd & 15;
				ch->param[hi] = lo;
				if(0) {
				}
#ifdef CMD_A_USED
				else if(hi == CHP_VOLDDN) {
					ch->sustaintimer = 0;
				}
#endif
#ifdef CMD_B_USED
				else if(hi == CHP_VIBRDEPTH) {
					ch->vibrphase = 0;
				}
#endif
#ifdef CMD_C_USED
				else if(hi == CHP_VOL) {
					ch->param[CHP_VOL] = lo << 4;
				}
#endif
#ifdef CMD_D_USED
				else if(hi == 0xd) {
					player.drumptr = drums[lo >> 1];
					player.drumvol = lo & 1;
				}
#endif
			}
			player.trackpos++;
			player.trackpos %= TRACKLEN;
			if(!player.trackpos && allow_advance) {
				player.songpos++;
				if(player.songpos == SONGLEN) {
					player.songpos = 1;
				}
			}
		}
		player.ttimer--;
		for(c = 0; c < 4; c++) {
			struct channel *ch = &channel[c];
			s16 vol = ch->param[CHP_VOL];

#ifdef CMD_1_USED
			ch->freq += ch->param[CHP_FREQDUP];
#endif
#ifdef CMD_2_USED
			ch->freq -= ch->param[CHP_FREQDDN];
#endif
#ifdef CMD_9_USED
			vol += SHIFTM1(ch->param[CHP_VOLDUP]);
#endif
			vol -= SHIFTM1(ch->param[CHP_VOLDDN]);
			if(vol < 0) vol = 0;
#ifdef CMD_9_USED
			if(vol > 255) vol = 255;
#endif
			ch->param[CHP_VOL] = (u8) vol;
#ifdef CMD_B_USED
			ch->vibrphase += ch->param[CHP_VIBRRATE];
#endif
			if(ch->sustaintimer) {
				ch->sustaintimer--;
				if(!ch->sustaintimer) {
					ch->param[CHP_VOLDDN] = ch->param[CHP_RELEASE];
				}
			}
		}
		player.divtimer = 44100/TEMPO;
	}
	player.divtimer--;

	if(player.drumptr) {
		switch(*player.drumptr++) {
			case 0:
				if(player.drumlevel > -16) player.drumlevel--;
				break;
			case 1:
				if(player.drumlevel < 15) player.drumlevel++;
				break;
			default:
				player.drumptr = 0;
				break;
		}
	}
	acc = player.drumlevel << (9 - player.drumvol); // [-8192,8192]

	for(c = 0; c < 4; c++) {
		struct channel *ch = &channel[c];
		s8 sound;

		switch(ch->param[CHP_WAVEFORM]) {
			case 0x8:
				sound = ch->phase >> 10;
				if(sound & 0x20) {
					sound ^= 0x3f;
				}
				sound -= 16;
				break;
			case 0xF:
				sound = (player.noise & 15) >> (ch->freq >> 8);
				player.noise <<= 1;
				if(player.noise & 0x8000) player.noise ^= 1;
				if(player.noise & 0x4000) player.noise ^= 1;
				sound -= 8;
				break;
			default:
				sound = -8;
				if((ch->phase >> 12) > ch->param[CHP_WAVEFORM]) sound = 8;
				break;
		}

		acc += sound * ch->param[CHP_VOL]; // [-2048,2048]
		ch->phase +=
			ch->freq
#ifdef CMD_B_USED
			+ SHIFTM1(ch->param[CHP_VIBRDEPTH]) * sin((M_PI * 2 / 256) * ch->vibrphase)
#endif
			;
	}

	//acc /= 2;

	if(bleepvolume) {
		bleepphase += bleepfreq;
		acc += bleepvolume * 2 * ((bleepphase & 0x8000)? 1 : -1);
		bleepvolume--;
	}

	return acc;
}

static void playroutine(void *_dummy, uint8_t *_soundbuf, int _buflen) {
	int i;
	s16 *soundbuf = (s16 *) _soundbuf;
	int buflen = _buflen / 4;
	s16 dry;
	static s16 reverb[REVERBLENGTH];
	static int reverbpos;

	for(i = 0; i < buflen; i++) {
		dry = nextsample();
		soundbuf[i * 2 + 0] = dry + reverb[reverbpos];
		soundbuf[i * 2 + 1] = dry + reverb[(reverbpos + REVERBLENGTH - 100) % REVERBLENGTH];
		reverb[reverbpos] = dry >> 2;
		reverbpos = (reverbpos + 1) % REVERBLENGTH;
	}
}

static SDL_AudioSpec audiospec = {
	.freq = 44100,
	.format = AUDIO_S16,
	.samples = 1024,
	.callback = playroutine,
	.channels = 2
};

void sfx_init() {
	SDL_OpenAudio(&audiospec, 0);
}

void sfx_startsong(int num) {
	SDL_PauseAudio(1);
	memcpy(channel, initchannel, sizeof(channel));
	memcpy(&player, &initplayer, sizeof(player));
	allow_advance = 0;
	SDL_PauseAudio(0);
}

void sfx_advance() {
	allow_advance = 1;
}

void sfx_hitbase() {
	bleepfreq = 554 * 0x10000 / 44100;
	bleepvolume = 3000;
}

void sfx_hitbrick() {
	bleepfreq = 1108 * 0x10000 / 44100;
	bleepvolume = 3000;
}

void sfx_gameover() {
}

