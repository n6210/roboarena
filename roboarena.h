/*
	File: roboarena.h
	RoboArena - game for programmers

	Created: 2015.09.30
	Author: Taddy G
	E-mail: fotonix@pm.me
	License: GPL
	Copyright (2015) by Taddy G
*/

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>

#define ARENA_W		500
#define ARENA_H		ARENA_W
#define RESULT_COLW	300
#define STATUS_H	100

#define U 0
#define R 1
#define D 2
#define L 3

#define DIS_ANIM		0x00
#define ENA_MOVE_ANIM	0x01
#define OT_ANIM			0x02
#define ENA_ROTATE_ANIM	0x04

#define FALSE	0x0
#define TRUE 	0x1
#define HIDDEN	FALSE
#define VISIBLE	TRUE

#define SPRITE_SIZE	50

#define FSPRITE		1
#define MAX_ROBOTS	4
#define MAX_ROCKETS	20
#define MAX_BOMBS	20
#define MAX_EXPLOSION 30
#define MAX_RADAR	MAX_ROBOTS
#define MAX_SHIELD	MAX_ROBOTS

#define MAX_LIFE		4
#define MAX_ROBO_BOMBS	5

#define TOTAL_SPRITES	(FSPRITE + MAX_ROBOTS + MAX_ROCKETS + MAX_BOMBS + MAX_EXPLOSION + MAX_RADAR + MAX_SHIELD)

#define ROCKET_RANGE	(25 * 8)
#define ROBOT_RANGE		SPRITE_SIZE
#define RADAR_RANGE		(25 * 9)

#define TS_WALL_COLL	(-1)
#define TS_MOVE_DONE	0
#define TS_AT_DEST		1
#define TS_ROBO_COLL	2

#define	PKT_STEP		1
#define	PKT_ROCKET		5
#define	PKT_BOMB		50
#define	PKT_ROBOT		100
#define	PKT_ROBOT2		10

#define max(a, b) ((a > b) ? (a) : (b))

typedef enum { ROB = 1, ROC, BOM, EXP, RAD, SHI } SpriteType;
typedef enum { C_NOOP, C_TURN_L, C_TURN_R, C_STEP, C_ROCKET, C_RADAR, C_BOMB } rCom;
typedef enum { EV_NULL = 0, EV_WALL, EV_ROCKET, EV_ROBOT } rEvent;

struct compileErr {
	char *err_file;
	char *err_msg;
	int err_ln;
};

struct SpriteState {
	int x;
	int y;
	int dx;
	int dy;
	int dir;
	int radar;
	int bomb;
	int echowith;
	int life;
	int acnt;
	int arot;
	int scnt;
	int points;
	int owner;
	int visible;
	int shield;
	int shcnt;
	rEvent event;
	SDL_Surface *img;
// RO
	SpriteType typ;
	int mx;
	int my;
	int maxph;
	int aph;
	int step;
	int adiv;
	int aen;
};

struct BaseLocations {
	int x;
	int y;
};

struct Config {
	int bg_mus_play;
	int fx_play;
	char *work_dir;
};

struct Status {
	unsigned int CPUclk;
	unsigned int maxclk;
	int stop;
	int tt_end;
	int timeStart;
	struct Config *conf;
	struct compileErr *comErr;
};

extern struct SpriteState sprite[];
extern char *rComS[];
extern char *rEventS[];

extern SDL_Surface *screen;
extern SDL_Surface *bombicon;

extern TTF_Font *font1;
extern TTF_Font *font2;

extern int rc;
extern int rob[];

int brainInit(char *hdir, char *flist[], struct compileErr *ce);
rCom askRoboBrain(int robn, rEvent event, char *dis, int rnd, int dbg);

int putText(int x, int y, SDL_Color color, TTF_Font *font, const char *fmt, ...) __attribute__ ((format (printf, 5, 6)));

int drawResults(int x, int y, int dbg, int drive, int stop);
void drawStatus(struct Status *s);
void printCodes(char *line, int reset);
void showMsg(char *msg1, char *msg2, char *msg3, char *msg4, Uint8 r, Uint8 g, Uint8 b);
void showWinner(int, int);
void showGoodbye(void);

SDL_Surface * LoadSprite(char *name, Uint8 alpha);

