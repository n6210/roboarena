/*
	RoboArena - game for programmers

	File: main.c
	Created: 2015.09.22
	Author: Taddy G
	E-mail: fotonix@pm.me
	License: GPL
	Copyright (2015) by Taddy G
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>

#include "roboarena.h"

struct SpriteState sprite[TOTAL_SPRITES];
struct compileErr comErr;

SDL_Surface *screen = NULL;
SDL_Surface *bg = NULL;
SDL_Surface *robo[4] = {NULL, NULL, NULL, NULL};
SDL_Surface *rocket = NULL;
SDL_Surface *bomb[4] = {NULL, NULL, NULL, NULL};
SDL_Surface *bombicon = NULL;
SDL_Surface *explosion = NULL;
SDL_Surface *radar[2] = {NULL, NULL};
SDL_Surface *shield[2] = {NULL, NULL};

int fantom;
int rob[MAX_ROBOTS];
int roc[MAX_ROCKETS];
int bom[MAX_BOMBS];
int bum[MAX_EXPLOSION];
int rad[MAX_RADAR];
int shi[MAX_ROBOTS];

Mix_Music *bgmus = NULL;
Mix_Chunk *gaovsnd = NULL;
Mix_Chunk *explsnd = NULL;
Mix_Chunk *rocssnd = NULL;
Mix_Chunk *rocesnd = NULL;
Mix_Chunk *radpsnd = NULL;
Mix_Chunk *radrsnd = NULL;
Mix_Chunk *robrsnd = NULL;
Mix_Chunk *robmsnd = NULL;

// SDL_Thread *thread = NULL;
SDL_Event kbdEvent;
TTF_Font *font1 = NULL;
TTF_Font *font2 = NULL;

struct Status status;
unsigned int start = 0;
unsigned int cycle = 0;

int sprite_nr = FSPRITE;
int debug = 0;
int results_update = 1;
int winner = 0;
int rc = 0;
int drive = 0;
int ld = 4;
Uint8 *keystate;

struct Config cfg = {
	bg_mus_play : 0,
	fx_play : 0,
};

const struct BaseLocations basloc[4] = {
	{
		x : 0,
		y : 0,
	},
	{
		x : 450,
		y : 0,
	},
	{
		x : 450,
		y : 450,
	},
	{
		x : 0,
		y : 450,
	},
};

const char sdir[4] = {'U', 'R', 'D', 'L'}; // U, R, D, L
char *home_dir = NULL, *work_dir = NULL;
char *files_list[4] = {"test1.ras", "test2.ras", "test3.ras", "test4.ras"};

// ---------------------------------------------------------------------------------------------------------------
int isSpriteRobot(int i)
{
	return (sprite[i].typ == ROB);
}

int isSpriteShield(int i)
{
	return (sprite[i].typ == SHI);
}

int isSpriteRocket(int i)
{
	return (sprite[i].typ == ROC);
}

int isSpriteBomb(int i)
{
	return (sprite[i].typ == BOM);
}

int isSpriteRadar(int i)
{
	return (sprite[i].typ == RAD);
}

int isSpriteExplosion(int i)
{
	return (sprite[i].typ == EXP);
}

// ---------------------------------------------------------------------------------------------------------------
int getFreeRadar(void)
{
	int i;

	for (i = 0; i < MAX_RADAR; i++)
	{
		if (!sprite[rad[i]].visible)
		{
			sprite[rad[i]].img = radar[0];
			return (rad[i]);
		}
	}

	return (0);
}

int getFreeBomb(void)
{
	int i;

	for (i = 0; i < MAX_BOMBS; i++)
	{
		if (!sprite[bom[i]].visible)
			return (bom[i]);
	}

	return (0);
}

int getFreeShield(void)
{
	int i;

	for (i = 0; i < MAX_SHIELD; i++)
	{
		if (!sprite[shi[i]].visible)
		{
			sprite[shi[i]].img = shield[0];
			return (shi[i]);
		}
	}

	return (0);
}

// ---------------------------------------------------------------------------------------------------------------
int isSpriteRotating(int i)
{
	return (sprite[i].visible && sprite[i].aph && sprite[i].arot);
}

int isSpriteMoving(int i)
{
	return (sprite[i].visible && ((sprite[i].dx > 0) || (sprite[i].dy > 0)));
}

int isAnyRoboMoving(void)
{
	int i;

	for (i = 0; i < MAX_ROBOTS; i++)
	{
		if (isSpriteMoving(rob[i]) || isSpriteRotating(rob[i]))
			return (1);
	}

	return (0);
}

int isAnyRadarActive(void)
{
	int i;

	for (i = 0; i < MAX_ROBOTS; i++)
	{
		if (sprite[rob[i]].visible && sprite[rob[i]].radar)
		{
			// printf("RADAR in ROBO-%d radar=%d\n", i, sprite[rob[i]].radar);
			return (1);
		}
	}

	return (0);
}

void updateSpriteAcnt(int i)
{
	sprite[i].acnt = sprite[i].aph * sprite[i].dir;
}

void resetSprite(int i)
{
	sprite[i].dx = 0;
	sprite[i].dy = 0;
	sprite[i].radar = 0;
	sprite[i].bomb = 0;
	sprite[i].echowith = 0;
	sprite[i].arot = 0;
	sprite[i].scnt = 0;
	sprite[i].points = 0;
	sprite[i].shield = 0;
	sprite[i].shcnt = 0;
	sprite[i].visible = FALSE;
	sprite[i].event = EV_NULL;

	if (sprite[i].typ != ROB)
		sprite[i].owner = 0;

	updateSpriteAcnt(i); //!!
}

// ---------------------------------------------------------------------------------------------------------------
int detectRoboAhed(int i, int dir)
{
	int z, x1, y1, x2, y2, cx = 0, cy = 0;
	struct SpriteState *r = &sprite[i];

	switch (sprite[i].dir)
	{
	case U:
		cy = -5;
		break;
	case D:
		cy = 5;
		break;
	case R:
		cx = 5;
		break;
	case L:
		cx = -5;
		break;
	}

	x1 = r->x + cx;
	y1 = r->y + cy;

	for (z = 0; z < MAX_ROBOTS; z++)
	{
		int r = rob[z];

		if ((i != r) && sprite[r].visible)
		{
			x2 = sprite[r].x;
			y2 = sprite[r].y;

			if (!(((x1 + 45) < x2) || ((x2 + 45) < x1) || ((y1 + 45) < y2) || ((y2 + 45) < y1)))
			{
				return (1);
			}
		}
	}

	return (0);
}

void checkLocation(int i)
{
	int r;

	for (r = 0; r < MAX_ROBOTS; r++)
	{
		if ((rob[r] != i) && (basloc[r].x == sprite[i].x) && (basloc[r].y == sprite[i].y))
		{
			if (sprite[i].shield == 0)
			{
				int sh = getFreeShield();

				sprite[sh].owner = i;
				sprite[sh].x = sprite[i].x;
				sprite[sh].y = sprite[i].y;
				sprite[sh].dir = sprite[i].dir;
				sprite[sh].visible = TRUE;
				sprite[i].shield = (1 << r); // | (1 << (sprite[i].owner - 1));
				sprite[i].shcnt++;
				// printf("\n>>> ROBO-%d got backshield %d now (base of ROBO id=%d)\n", sprite[i].owner, sh, r+1);
			}
			else if ((sprite[i].shield & (1 << r)) == 0)
			{
				sprite[i].shield |= (1 << r);
				sprite[i].shcnt++;
			}
		}
	}
}

int checkSpriteCollision(int r, int i)
{
	struct SpriteState *s1, *s2;
	int cx = 8, cy = 8, cx2 = 8, cy2 = 8;

	if (!sprite[r].visible || !sprite[i].visible)
		return (0);

	if (isSpriteRocket(r))
	{
		switch (sprite[r].dir)
		{
		case U:
		case D:
			cx2 = 20;
			break;
		case R:
		case L:
			cy2 = 20;
			break;
		}
	}
	else if (isSpriteBomb(r))
		cx2 = cy = 13;

	if (isSpriteRocket(i))
	{
		switch (sprite[i].dir)
		{
		case U:
		case D:
			cx = 20;
			break;
		case R:
		case L:
			cy = 20;
			break;
		}
	}
	else if (isSpriteBomb(i))
		cx = cy = 13;

	s1 = &sprite[r];
	s2 = &sprite[i];

	return !(((s1->x + SPRITE_SIZE - cx) < (s2->x + cx2)) ||
			 ((s2->x + SPRITE_SIZE - cx2) < (s1->x + cx)) ||
			 ((s1->y + SPRITE_SIZE - cy) < (s2->y + cy2)) ||
			 ((s2->y + SPRITE_SIZE - cy2) < (s1->y + cy)));
}

// ---------------------------------------------------------------------------------------------------------------

int tryDoSpriteStep(int i)
{
	struct SpriteState *sp = &sprite[i];
	int dir = sprite[i].dir;

	if ((sp->dx <= 0) && (sp->dy <= 0))
		return (TS_AT_DEST);

	if (isSpriteRobot(i))
	{
		if (sprite[i].radar)
			return (TS_MOVE_DONE);

		if (isSpriteMoving(i) && detectRoboAhed(i, dir))
		{
			sprite[i].dx = 0;
			sprite[i].dy = 0;

			return (TS_ROBO_COLL);
		}
	}

	// if (isSpriteRocket(i)) printf("dir=%c x=%d mx=%d\n", sdir[dir], sp->x, sp->mx);

	switch (dir)
	{
	case L:
		if (sp->x >= sp->step)
		{
			if (sp->dx > 0)
			{
				sp->x -= sp->step;
				sp->dx -= sp->step;
				if (sp->dx <= 0)
					goto atdest;
			}
		}
		else
		{
			sp->dx = 0;
			return (TS_WALL_COLL);
		}
		break;

	case R:
		if (sp->x <= sp->mx)
		{
			if (sp->dx > 0)
			{
				sp->x += sp->step;
				sp->dx -= sp->step;
				if (sp->dx <= 0)
					goto atdest;
			}
		}
		else
		{
			sp->dx = 0;
			return (TS_WALL_COLL);
		}
		break;

	case U:
		if (sp->y >= sp->step)
		{
			if (sp->dy > 0)
			{
				sp->y -= sp->step;
				sp->dy -= sp->step;
				if (sp->dy <= 0)
					goto atdest;
			}
		}
		else
		{
			sp->dy = 0;
			return (TS_WALL_COLL);
		}
		break;

	case D:
		if (sp->y <= sp->my)
		{
			if (sp->dy > 0)
			{
				sp->y += sp->step;
				sp->dy -= sp->step;
				if (sp->dy <= 0)
					goto atdest;
			}
		}
		else
		{
			sp->dy = 0;
			return (TS_WALL_COLL);
		}
		break;
	}

	return TS_MOVE_DONE;

atdest:
	if (isSpriteRobot(i))
	{
		sprite[i].points += PKT_STEP;
		sprite[i].scnt++;
		results_update = TRUE; //!!
							   // printf("STEP=%d for %d\n", sprite[i].scnt, i);
	}

	return (TS_AT_DEST);
}

int getFreeRocket(void)
{
	int i;

	for (i = 0; i < MAX_ROCKETS; i++)
	{
		if (!sprite[roc[i]].visible)
			return (roc[i]);
	}

	return (0);
}

int getFreeExplosion(void)
{
	int i;

	for (i = 0; i < MAX_EXPLOSION; i++)
	{
		if (!sprite[bum[i]].visible)
		{
			sprite[bum[i]].owner = 0;
			return (bum[i]);
		}
	}

	return (0);
}

// ---------------------------------------------------------------------------------------------------------------

int rocLunchBySpriteID(int i)
{
	int dir;
	int rocn;

	if (!isSpriteRobot(i) || !sprite[i].visible)
		return (0);

	dir = sprite[i].dir;
	rocn = getFreeRocket();

	// if (sprite[robn].radar)	return (0);
	// printf("Rocket %d lunched by ROBO-%d on dir=%c (radar=%d)\n", rocn, robn, sdir[dir], sprite[robn].radar);

	if (rocn != 0)
	{
		sprite[rocn].visible = TRUE;
		sprite[rocn].x = sprite[i].x;
		sprite[rocn].y = sprite[i].y;
		sprite[rocn].dir = dir;
		sprite[rocn].owner = i;

		switch (dir)
		{
		case U:
		case D:
			sprite[rocn].dx = 0;
			sprite[rocn].dy = ROCKET_RANGE;
			break;
		case R:
		case L:
			sprite[rocn].dx = ROCKET_RANGE;
			sprite[rocn].dy = 0;
			break;
		}

		if (cfg.fx_play)
			Mix_PlayChannel(-1, rocssnd, 0);
	}
	else
		return (-1);

	return (0);
}

// ---------------------------------------------------------------------------------------------------------------

int startRadarByRobo(int robot_nr)
{
	int robn = rob[robot_nr];
	int radar = sprite[robn].radar;
	int dir;

	if (radar || !sprite[robn].visible)
		return (-1);

	radar = getFreeRadar();
	if (!radar)
		return (-1);

	dir = sprite[robn].dir;

	sprite[robn].radar = radar;
	sprite[radar].visible = TRUE;
	sprite[radar].x = sprite[robn].x;
	sprite[radar].y = sprite[robn].y;
	sprite[radar].dir = dir;
	sprite[radar].owner = robn;

	switch (dir)
	{
	case U:
	case D:
		sprite[radar].dx = 0;
		sprite[radar].dy = RADAR_RANGE;
		break;
	case R:
	case L:
		sprite[radar].dx = RADAR_RANGE;
		sprite[radar].dy = 0;
		break;
	}
	// printf("RADAR-%d started (%dx%d) dx=%d dy=%d owner=%d\n", radar, sprite[radar].x, sprite[radar].y, sprite[radar].dx, sprite[radar].dy, robn);

	if (cfg.fx_play)
		Mix_PlayChannel(-1, radpsnd, 0);

	return (0);
}

int putBombByRobo(int robot_nr)
{
	int robn = rob[robot_nr];
	int bomb_cnt, i, x, y;

	if (!sprite[robn].visible || !sprite[robn].bomb)
		return (-1);

	x = sprite[robn].x;
	y = sprite[robn].y;

	for (i = 0; i < MAX_BOMBS; i++)
	{
		if (sprite[bom[i]].visible && (sprite[bom[i]].x == x) && (sprite[bom[i]].y == y))
		{
			printf(">>> Other bomb is here x=%d y=%d\n", x, y);
			return (-1);
		}
	}

	bomb_cnt = getFreeBomb();
	if (!bomb_cnt)
		return (-1);

	sprite[robn].bomb--;
	results_update = TRUE;

	sprite[bomb_cnt].visible = TRUE;
	sprite[bomb_cnt].x = sprite[robn].x;
	sprite[bomb_cnt].y = sprite[robn].y;
	sprite[bomb_cnt].owner = robn;

	sprite[bomb_cnt].img = bomb[robot_nr];

	// if (cfg.fx_play)
	//		Mix_PlayChannel(-1, bompsnd, 0);
	// printf("BOMB-%d put (%dx%d) owner=%d\n", bomb, sprite[bomb].x, sprite[bomb].y, robn);

	return (0);
}

void calcSpriteSteps(int i, int dir)
{
	sprite[i].dir = dir;

	switch (dir)
	{
	case U:
	case D:
		sprite[i].dx = 0;
		sprite[i].dy = ROBOT_RANGE;
		break;
	case R:
	case L:
		sprite[i].dx = ROBOT_RANGE;
		sprite[i].dy = 0;
		break;
	}
	// printf("ROBO%d dx=%d dy=%d x=%d y=%d\n", i, sprite[i].dx, sprite[i].dy, sprite[i].x, sprite[i].y);
}

void checkAnyAlive(void)
{
	int x, alive, last_alive = (-1);

	for (x = alive = 0; x < MAX_ROBOTS; x++)
	{
		if (sprite[rob[x]].visible && sprite[rob[x]].life)
		{
			alive++;
			last_alive = x;
		}
	}

	if ((status.stop = (alive == 1)))
	{
		start = SDL_GetTicks();
		winner = last_alive + 1;
	}
}
// ---------------------------------------------------------------------------------------------------------------

int do_logic(void)
{
	int ret;
	int i;

	for (i = FSPRITE; i < sprite_nr; i++)
	{
		if (!sprite[i].visible)
			continue;
		else if (isSpriteExplosion(i))
		{
			int robo_own = sprite[i].owner;

			if ((robo_own) && (sprite[robo_own].typ != ROC))
			{
				sprite[i].x = sprite[robo_own].x;
				sprite[i].y = sprite[robo_own].y;
			}
		}
		if (isSpriteShield(i))
		{
			int r = sprite[i].owner;

			if (r && sprite[r].visible)
			{
				sprite[i].x = sprite[r].x;
				sprite[i].y = sprite[r].y;

				if (sprite[r].shcnt == 1)
					sprite[i].dir = sprite[r].dir;
				else
				{
					if ((cycle % sprite[i].adiv) == 0)
					{
						sprite[i].img = shield[1];
						sprite[i].dir++;
						if (sprite[i].dir > 5)
							sprite[i].dir = 0;
					}
				}
			}
			else
				sprite[i].visible = FALSE;
		}
		else if (isSpriteBomb(i))
		{
			int r;

			for (r = 0; r < MAX_ROBOTS; r++)
			{
				int robo_own = rob[r];
				int owner = sprite[i].owner;

				if (owner == robo_own)
					continue;
				else if (checkSpriteCollision(i, robo_own))
				{
					int expl = getFreeExplosion();

					if (expl > 0)
					{
						if (cfg.fx_play)
							Mix_PlayChannel(-1, explsnd, 0);

						sprite[i].visible = FALSE;
						sprite[i].owner = 0;
						sprite[expl].x = sprite[robo_own].x;
						sprite[expl].y = sprite[robo_own].y;
						sprite[expl].visible = TRUE;
						sprite[expl].owner = robo_own;
						sprite[owner].bomb++;

						if (sprite[robo_own].life > 0)
						{
							sprite[robo_own].life--;
							sprite[owner].points += PKT_BOMB;

							if (sprite[robo_own].life == 0)
							{
								int ra = sprite[robo_own].radar;

								if (ra)
									sprite[ra].visible = FALSE;
								sprite[robo_own].visible = FALSE;
								status.maxclk -= 500;
							}
						}

						checkAnyAlive();
						results_update = TRUE;
						printf(">>> BOMB-%d hit ROBO-%d\n", i, r);
					}
				}
			}
		}
		else if (isSpriteRocket(i))
		{
			int owner = sprite[i].owner;

			for (int r = 0; r < MAX_ROCKETS; r++)
			{
				int r2 = roc[r];

				if (!sprite[r2].visible)
					continue; // Double checked
				if (owner == sprite[r2].owner)
					continue;

				if (checkSpriteCollision(i, r2))
				{
					int e = getFreeExplosion();

					if (e > 0)
					{
						if (cfg.fx_play)
							Mix_PlayChannel(-1, rocesnd, 0);

						sprite[i].visible = FALSE;
						sprite[r2].visible = FALSE;
						sprite[e].x = sprite[r2].x;
						sprite[e].y = sprite[r2].y;
						sprite[e].visible = TRUE;
						sprite[e].owner = r2;
					}

					sprite[owner].points += PKT_ROCKET;
					results_update = TRUE; //!!!
					printf(">>> ROCKET id=%d hit ROCKET id=%d\n", i, r2);
				}
			}

			for (int r = 0; r < MAX_ROBOTS; r++)
			{
				int robo_own = rob[r];

				if (owner == robo_own)
					continue;

				if (checkSpriteCollision(i, robo_own))
				{
					int e = getFreeExplosion();

					if (sprite[robo_own].life > 0)
					{
						if (sprite[robo_own].shield)
						{
							if ((sprite[i].dir != sprite[robo_own].dir) && (sprite[robo_own].shcnt < 2))
							{
								sprite[robo_own].life--;
								sprite[owner].points += PKT_ROBOT;
							}
							else
								sprite[owner].points += PKT_ROBOT2;
						}
						else
						{
							sprite[robo_own].life--;
							sprite[owner].points += PKT_ROBOT;
						}

						if (sprite[robo_own].life == 0)
						{
							int ra = sprite[robo_own].radar;

							if (ra)
								sprite[ra].visible = FALSE;
							sprite[robo_own].visible = FALSE;
							status.maxclk -= 500;
						}
					}

					checkAnyAlive();
					results_update = TRUE;
					sprite[i].visible = FALSE;

					if (e > 0)
					{
						if (cfg.fx_play)
							Mix_PlayChannel(-1, explsnd, 0);

						sprite[e].x = sprite[robo_own].x;
						sprite[e].y = sprite[robo_own].y;
						sprite[e].visible = TRUE;
						sprite[e].owner = robo_own;
					}

					printf(">>> ROBO id=%d hit ROBO id=%d (life level: %d)!\n", sprite[i].owner, robo_own, sprite[robo_own].life);

					break;
				}
			}
		}
		else if (isSpriteRadar(i) && sprite[i].owner && !sprite[i].echowith)
		{
			int r;
			int direv[4] = {D, L, U, R}; // ORIG U, R, D, L -> D, L, U, R

			for (r = 0; r < MAX_ROBOTS; r++)
			{
				int r1 = rob[r];

				if ((sprite[i].owner == r1))
					continue;
				if (checkSpriteCollision(i, r1))
				{
					sprite[i].echowith = r1;
					// printf("RAD echo with %d\n", r1);
				}
			}

			if (!sprite[i].echowith)
			{
				for (r = 0; r < MAX_ROCKETS; r++)
				{
					int r1 = roc[r];

					if (!sprite[r1].visible)
						continue; // Double check to speed up
					if (sprite[i].owner == sprite[r1].owner)
						continue;
					if (sprite[i].dir != direv[sprite[r1].dir])
						continue;
					if (checkSpriteCollision(i, r1))
						sprite[i].echowith = r1;
				}
			}

			if (sprite[i].echowith)
			{
				sprite[i].dir = direv[sprite[i].dir];
				sprite[i].img = radar[1];

				if (sprite[i].dx)
				{
					sprite[i].dx = (RADAR_RANGE - SPRITE_SIZE) - sprite[i].dx;
				}
				if (sprite[i].dy)
				{
					sprite[i].dy = (RADAR_RANGE - SPRITE_SIZE) - sprite[i].dy;
				}

				if (cfg.fx_play)
					Mix_PlayChannel(-1, radrsnd, 0);

				// printf("RADAR ECHO with %d (%d - %d)\n", sprite[i].echowith, sprite[i].dx, sprite[i].dy);
			}
		}

		ret = tryDoSpriteStep(i);

		if (isSpriteRocket(i) && sprite[i].visible && (ret == TS_AT_DEST))
		{
			sprite[i].visible = FALSE;
			// printf("Rocket %d ret=%d\n", i, ret);
		}
		else if (isSpriteRadar(i))
		{
			if (sprite[i].echowith && !isSpriteMoving(i))
			{
				int e = sprite[i].echowith;
				int o = sprite[i].owner;

				sprite[i].visible = FALSE;
				sprite[i].echowith = 0;
				sprite[o].radar = 0;

				if (e)
				{
					// printf("RADAR-%d returned %d [%s detected]\n", i, e, isSpriteRobot(e)?"ROBO":(isSpriteRocket(e)?"ROCKET":"???"));
					if (isSpriteRobot(e))
					{
						if (isSpriteRobot(e))
							sprite[o].event = EV_ROBOT;
						else if (isSpriteRocket(e))
							sprite[o].event = EV_ROCKET;
					}
				}
			}

			if (ret != TS_MOVE_DONE)
			{
				sprite[i].visible = FALSE;
				sprite[sprite[i].owner].radar = 0;
			}
		}
		else if (isSpriteRobot(i))
		{
			if ((sprite[i].shcnt < 2) && (ret == TS_AT_DEST))
				checkLocation(i);

			if (!sprite[i].event)
			{
				switch (ret)
				{
				case TS_ROBO_COLL:
					// printf("ROBO-%d collision detected on dir=%c\n", i-1 , sdir[sprite[i].dir]);
					sprite[i].event = EV_ROBOT;
					break;

				case TS_WALL_COLL:
					// printf("ROBO-%d wall collision detected on dir=%c\n", i-1, sdir[sprite[i].dir]);
					sprite[i].event = EV_WALL;
					break;
				}
			}
		}
	}

	//------------------------------------------------------

	if (!isAnyRoboMoving() && !isAnyRadarActive() && !status.stop)
	{
		int ri;
		// Get next command from keyboard
		if ((drive != 0) && (ld != 4))
		{
			ri = rob[drive - 1];
			sprite[ri].dir = ld;
			updateSpriteAcnt(ri);
			calcSpriteSteps(ri, ld);
			ld = 4;
		}
		else
		{
			// printf(">> RC=%d | ", rc);
			ri = rob[rc];

			if (sprite[ri].life)
			{
				char disasm[60] = "";
				rCom instr;

				status.CPUclk++;

				instr = askRoboBrain(rc, sprite[ri].event, disasm, 0, debug);
				switch (instr)
				{
				case C_TURN_L:
					sprite[ri].dir = (sprite[ri].dir > 0) ? sprite[ri].dir - 1 : L;
					sprite[ri].arot = -sprite[ri].aph;
					if (cfg.fx_play)
						Mix_PlayChannel(-1, robrsnd, 0);
					break;
				case C_TURN_R:
					sprite[ri].dir = (sprite[ri].dir < L) ? sprite[ri].dir + 1 : U;
					sprite[ri].arot = sprite[ri].aph;
					if (cfg.fx_play)
						Mix_PlayChannel(-1, robrsnd, 0);
					break;
				case C_STEP:
					calcSpriteSteps(ri, sprite[ri].dir);
					if (cfg.fx_play)
						Mix_PlayChannel(-1, robmsnd, 0);
					break;
				case C_ROCKET:
					rocLunchBySpriteID(rob[rc]);
					break;
				case C_RADAR:
					startRadarByRobo(rc);
					break;
				case C_BOMB:
					putBombByRobo(rc);
					break;
				case C_NOOP:
					break;
				}

				// if (debug)	printf("ROBO-%d cmd=%s i=%d dir=%c radar=%d\n", rc+1, rComS[instr], i, sdir[sprite[i].dir], sprite[i].radar);
				// printf("DIS: %s\n", disasm);

				sprite[ri].event = EV_NULL;
				printCodes(disasm, 0);
			}

			do
			{
				if (++rc == MAX_ROBOTS)
					rc = 0;
			} while (sprite[rob[rc]].life == 0);

			if (debug)
				status.stop = 2;
		}
	}
	//------------------------------------------------------

	return 0;
}

// ---------------------------------------------------------------------------------------------------------------

void animate_all(void)
{
	static int wm;
	int i, dir;
	SDL_Rect sd, sp;

	if (results_update || (status.stop))
		results_update = drawResults(0, 0, debug, drive, status.stop);

	drawStatus(&status);

	for (i = FSPRITE; i < sprite_nr; i++)
	{
		if (sprite[i].visible)
		{
			SDL_Rect br;

			br.x = sprite[i].x;
			br.y = sprite[i].y;
			br.w = br.h = SPRITE_SIZE;
			SDL_BlitSurface(bg, &br, screen, &br);
		}
	}

	if (!status.stop)
		do_logic();
	cycle++;

	for (i = FSPRITE; i < sprite_nr; i++)
	{
		if (sprite[i].visible)
		{
			sd.x = sprite[i].x;
			sd.y = sprite[i].y;

			if (sprite[i].maxph > 0)
			{
				dir = sprite[i].acnt;

				if (sprite[i].aen)
				{
					if ((cycle % sprite[i].adiv) == 0)
					{
						if (sprite[i].aen & OT_ANIM)
						{
							if (++sprite[i].acnt > sprite[i].maxph)
							{
								sprite[i].visible = FALSE;
								sprite[i].acnt = 0;
								sprite[i].owner = 0;
							}
							// printf("OTA: Bum=%d vis=%d acnt=%d maxph=%d\n", i, sprite[i].visible, dir, sprite[i].maxph);
						}
						else if (sprite[i].aen == ENA_ROTATE_ANIM)
						{
							if (sprite[i].arot)
							{
								if (sprite[i].arot > 0)
								{
									sprite[i].acnt++;
									sprite[i].arot--;

									if (sprite[i].acnt > sprite[i].maxph)
										sprite[i].acnt = 0;
								}
								else
								{
									sprite[i].acnt--;
									sprite[i].arot++;

									if (sprite[i].acnt < 0)
										sprite[i].acnt = sprite[i].maxph;
								}
								// printf("i=%d arot=%d acnt=%d\n", i, sprite[i].arot, sprite[i].acnt);
							}
							// if ((sprite[i].arot == 0) && (sprite[i].acnt % 5)) { printf("i=%d arot=%d acnt=%d\n", i, sprite[i].arot, sprite[i].acnt); status.stop = 2;}
						}
						else
						{
							if (sprite[i].acnt < sprite[i].maxph)
								sprite[i].acnt++;
							else
								sprite[i].acnt = 0;
						}
					}
				}
				else
					dir = sprite[i].dir;

				sp.x = SPRITE_SIZE * dir;
				sp.y = 0;
				sp.w = sp.h = SPRITE_SIZE;
				SDL_BlitSurface(sprite[i].img, &sp, screen, &sd);
			}
			else
				SDL_BlitSurface(sprite[i].img, NULL, screen, &sd);
		}
	}

	if (winner)
	{
		if (cfg.fx_play && wm && !Mix_Playing(-1))
		{
			Mix_PlayChannel(-1, gaovsnd, 0);
			wm = 0;
		}

		showWinner(winner, (status.tt_end <= 0));

		if (debug)
			status.stop = 2;
	}
	else
	{
		int t = SDL_GetTicks();
		wm = 1;
		status.tt_end -= (t - status.timeStart);
		status.timeStart = t;

		if ((status.CPUclk >= status.maxclk) || (status.tt_end <= 0))
		{
			int r, lw = 0, mp = 0;

			for (r = 0; r < MAX_ROBOTS; r++)
			{
				if (!sprite[rob[r]].visible)
					continue;
				if (sprite[rob[r]].points >= mp)
				{
					if (sprite[rob[r]].points > mp)
					{
						mp = sprite[rob[r]].points;
						winner = r + 1;
						lw = r;
					}
					else
					{
						if (sprite[rob[r]].scnt > sprite[rob[lw]].scnt)
						{
							winner = r + 1;
							lw = r;
						}
					}
				}
			}
			status.stop = 2;
		}
	}

	SDL_Flip(screen);
}

// ---------------------------------------------------------------------------------------------------------------

int SetSprite(SDL_Surface *img, int visible, int iposx, int iposy, int step, int adiv, int anim_ena, int dir, SpriteType typ)
{
	if (sprite_nr >= TOTAL_SPRITES)
	{
		printf("Can not set sprite because number of available sprites is too low - set to fantom sprite #0\n");
		return (0);
	}

	sprite[sprite_nr].img = img;
	sprite[sprite_nr].visible = visible;
	sprite[sprite_nr].maxph = (img->w / img->h) - 1;
	sprite[sprite_nr].x = iposx;
	sprite[sprite_nr].y = iposy;
	sprite[sprite_nr].mx = ARENA_W - img->h - 1;
	sprite[sprite_nr].my = ARENA_H - img->h - 1;
	sprite[sprite_nr].step = step;
	sprite[sprite_nr].adiv = (adiv ? adiv : 1);
	sprite[sprite_nr].aen = anim_ena;
	sprite[sprite_nr].dir = dir;
	sprite[sprite_nr].typ = typ;

	if (typ == ROB)
	{
		if (sprite[sprite_nr].maxph > 3)
		{
			sprite[sprite_nr].aph = (img->w / img->h) / 4;
			updateSpriteAcnt(sprite_nr);
			// printf("ROB id=%d aph=%d acnt=%d maxph=%d\n", sprite_nr, sprite[sprite_nr].aph, sprite[sprite_nr].acnt, sprite[sprite_nr].maxph);
		}
		sprite[sprite_nr].life = MAX_LIFE;
		sprite[sprite_nr].bomb = MAX_ROBO_BOMBS;
	}

	return (sprite_nr++);
}

int resetGame(void)
{
	int i, r;

	SDL_BlitSurface(bg, NULL, screen, NULL);

	r = brainInit(work_dir, files_list, &comErr);
	if (r < 0)
		goto fail;

	for (i = FSPRITE; i < sprite_nr; i++)
		resetSprite(i);

	status.maxclk = 0;

	if (r & 1)
	{
		sprite[rob[0]].visible = TRUE;
		sprite[rob[0]].x = 0;
		sprite[rob[0]].y = 0;
		sprite[rob[0]].dir = D;
		sprite[rob[0]].life = MAX_LIFE;
		sprite[rob[0]].bomb = MAX_ROBO_BOMBS;
		updateSpriteAcnt(rob[0]);
		status.maxclk += 1000;
	}

	if (r & 2)
	{
		sprite[rob[1]].visible = TRUE;
		sprite[rob[1]].x = 450;
		sprite[rob[1]].y = 0;
		sprite[rob[1]].dir = L;
		sprite[rob[1]].life = MAX_LIFE;
		sprite[rob[1]].bomb = MAX_ROBO_BOMBS;
		updateSpriteAcnt(rob[1]);
		status.maxclk += 1000;
	}

	if (r & 4)
	{
		sprite[rob[2]].visible = TRUE;
		sprite[rob[2]].x = 450;
		sprite[rob[2]].y = 450;
		sprite[rob[2]].dir = U;
		sprite[rob[2]].life = MAX_LIFE;
		sprite[rob[2]].bomb = MAX_ROBO_BOMBS;
		updateSpriteAcnt(rob[2]);
		status.maxclk += 1000;
	}

	if (r & 8)
	{
		sprite[rob[3]].visible = TRUE;
		sprite[rob[3]].x = 0;
		sprite[rob[3]].y = 450;
		sprite[rob[3]].dir = R;
		sprite[rob[3]].life = MAX_LIFE;
		sprite[rob[3]].bomb = MAX_ROBO_BOMBS;
		updateSpriteAcnt(rob[3]);
		status.maxclk += 1000;
	}

	status.tt_end = 3 * 60 * 1000;
	status.timeStart = SDL_GetTicks();

	results_update = TRUE;
	status.CPUclk = rc = status.stop = 0;
	winner = 0;
	printCodes(NULL, 1);

	return (0);

fail:
	status.stop = 3;

	return (r);
}
char *getHomeDir(void)
{
	char *homedir;

	if ((homedir = getenv("HOME")) == NULL)
		homedir = getpwuid(getuid())->pw_dir;

	if (homedir == NULL)
	{
		printf("Unable to get home directory to read config file.\n");
		return (NULL);
	}

	return (homedir);
}

int readCFG(char *hdir)
{
	int x, f;

	char hd[4096] = "";

	strncpy(hd, hdir, sizeof(hd) - 15);
	strcat(hd, "/roboarena.cfg");
	f = open(hd, O_RDONLY);
	if (f > 0)
	{
		printf("Read config\n");
		x = read(f, &cfg, sizeof(cfg));
		close(f);

		if (x != sizeof(cfg))
		{
			printf("Config data are corrupted!\n");
			cfg.fx_play = 0;
			cfg.bg_mus_play = 0;

			return (-1);
		}
		else
			printf("FX is %s\nBGM is %s\n", cfg.fx_play ? "on" : "off", cfg.bg_mus_play ? "on" : "off");
	}
	else
		printf("Unable to read config file - set to defaults\n");

	return (0);
}

int saveCFG(const char *hdir)
{
	int f;
	char hd[4096] = "";

	strncpy(hd, hdir, sizeof(hd) - 15);
	strcat(hd, "/roboarena.cfg");
	f = creat(hd, S_IRUSR | S_IWUSR);
	if (f > 0)
	{
		size_t ret = 0;
		printf("Save config\n");
		ret = write(f, &cfg, sizeof(cfg));
		close(f);
		if (ret != sizeof(cfg))
		{
			printf("Error saving config file!");
		}
		return (0);
	}
	else
		printf("Unable to save config file: %s !\n", hdir);

	return (-1);
}

int prepareWorkDir(const char *hdir, char **wdir)
{
	DIR *dir;

	if (hdir == NULL)
		return (-1);
	if (*wdir == NULL)
	{
		*wdir = malloc(strlen(hdir) + 16);
		if (*wdir == NULL)
			return (-1);

		strcpy(*wdir, hdir);
	}
	strcat(*wdir, "/RoboArena");
	dir = opendir(*wdir);
	if (dir == NULL)
	{
		if (mkdir(*wdir, 0755))
		{
			printf("Cannot create required work directory: %s\n", *wdir);
			return (-1);
		}
	}
	else
		closedir(dir);

	return (0);
}

// ---------------------------------------------------------------------------------------------------------------

int main(int argc, char *args[])
{
	int i, fps;
	int progExit = 0;

	status.conf = &cfg;
	status.comErr = &comErr;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER /*SDL_INIT_EVERYTHING*/);

	SDL_WM_SetCaption("Robo Arena", NULL);
	SDL_WM_SetIcon(IMG_Load("res/icon.png"), NULL);

	if (TTF_Init() < 0)
	{
		printf("Unable to initialize TTF library\n");
		return (0);
	}

	if ((font1 = TTF_OpenFont("res/ubuntumono-b.ttf", 20)) == NULL)
	{
		printf("Font ubuntumono-b.ttf not found\n");
		return (0);
	}

	if ((font2 = TTF_OpenFont("res/tahoma.ttf", 14)) == NULL)
	{
		printf("Font tahoma.ttf not found\n");
		return (0);
	}

	if ((Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096) != -1))
	{
		if (((bgmus = Mix_LoadMUS("res/SpyHunter.ogg")) == NULL) ||
			((gaovsnd = Mix_LoadWAV("res/gameover.ogg")) == NULL) ||
			((explsnd = Mix_LoadWAV("res/explosion.ogg")) == NULL) ||
			((rocssnd = Mix_LoadWAV("res/rocket.ogg")) == NULL) ||
			((rocesnd = Mix_LoadWAV("res/rocketexp.ogg")) == NULL) ||
			((radpsnd = Mix_LoadWAV("res/radar.ogg")) == NULL) ||
			((radrsnd = Mix_LoadWAV("res/radar2.ogg")) == NULL) ||
			((robrsnd = Mix_LoadWAV("res/robotr.ogg")) == NULL) ||
			((robmsnd = Mix_LoadWAV("res/robotm.ogg")) == NULL))
		{
			printf("Loading of music/effects was not possible: %s\n", Mix_GetError());
			return (0);
		}
	}

	Mix_AllocateChannels(24);

	home_dir = getHomeDir();
	if (home_dir == NULL)
		return (0);

	readCFG(home_dir);
	prepareWorkDir(home_dir, &work_dir);

	Mix_VolumeMusic(MIX_MAX_VOLUME / 4);

	if (gaovsnd)
		Mix_VolumeChunk(gaovsnd, MIX_MAX_VOLUME);
	if (explsnd)
		Mix_VolumeChunk(explsnd, MIX_MAX_VOLUME / 6);
	if (rocssnd)
		Mix_VolumeChunk(rocssnd, MIX_MAX_VOLUME / 6);
	if (rocesnd)
		Mix_VolumeChunk(rocesnd, MIX_MAX_VOLUME / 6);
	if (radpsnd)
		Mix_VolumeChunk(radpsnd, MIX_MAX_VOLUME);
	if (radrsnd)
		Mix_VolumeChunk(radrsnd, MIX_MAX_VOLUME);
	if (robrsnd)
		Mix_VolumeChunk(robrsnd, MIX_MAX_VOLUME / 6);
	if (robmsnd)
		Mix_VolumeChunk(robmsnd, MIX_MAX_VOLUME / 7);

	//---------------------------------------------------------------------------------------------------
	screen = SDL_SetVideoMode(ARENA_W + RESULT_COLW, ARENA_H + STATUS_H, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWACCEL);
	if (screen == NULL)
	{
		printf("Unable to set proper video mode ans size\n");
		return (0);
	}

	SDL_SetColorKey(screen, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(screen->format, 0, 0, 0));
	SDL_SetAlpha(screen, SDL_SRCCOLORKEY | SDL_RLEACCEL | SDL_SRCALPHA, 255);

	memset(sprite, 0, sizeof(sprite));

	bg = LoadSprite("res/background2.png", 255);
	robo[0] = LoadSprite("res/robo1r.png", 255);
	robo[1] = LoadSprite("res/robo2r.png", 255);
	robo[2] = LoadSprite("res/robo3r.png", 255);
	robo[3] = LoadSprite("res/robo4r.png", 255);
	rocket = LoadSprite("res/rocket.png", 255);
	explosion = LoadSprite("res/explosion.png", 240);
	radar[0] = LoadSprite("res/radar1.png", 190);
	radar[1] = LoadSprite("res/radar2.png", 180);
	bombicon = LoadSprite("res/bombico.png", 200);
	bomb[0] = LoadSprite("res/bomb1.png", 255);
	bomb[1] = LoadSprite("res/bomb2.png", 255);
	bomb[2] = LoadSprite("res/bomb3.png", 255);
	bomb[3] = LoadSprite("res/bomb4.png", 255);
	shield[0] = LoadSprite("res/shields.png", 200);
	shield[1] = LoadSprite("res/shields2.png", 200);

	if (!bg || !robo[0] || !robo[1] || !robo[2] || !robo[3] ||
		!rocket || !explosion || !radar[0] || !radar[1] || !bombicon ||
		!bomb[0] || !bomb[1] || !bomb[2] || !bomb[3] || !shield[0] || !shield[1])
	{
		printf("Unable to load one of the spirits\n");
		return (0);
	}

	SDL_BlitSurface(bg, NULL, screen, NULL);

	if (cfg.bg_mus_play)
		Mix_PlayMusic(bgmus, -1);

	// fantom = SetSprite(bomb, HIDDEN, 250, 250, VISIBLE, DIS_MOVE_ANIM, D); // We need grab sprite idx = 0 !!!

	for (i = 0; i < MAX_BOMBS; i++)
		bom[i] = SetSprite(bomb[0], HIDDEN, 0, 0, 1, 25, ENA_MOVE_ANIM, 0, BOM);

	rob[0] = SetSprite(robo[0], VISIBLE, 0, 0, 1, 4, ENA_ROTATE_ANIM, D, ROB);
	rob[1] = SetSprite(robo[1], VISIBLE, 450, 0, 1, 4, ENA_ROTATE_ANIM, L, ROB);
	rob[2] = SetSprite(robo[2], VISIBLE, 450, 450, 1, 4, ENA_ROTATE_ANIM, U, ROB);
	rob[3] = SetSprite(robo[3], VISIBLE, 0, 450, 1, 4, ENA_ROTATE_ANIM, R, ROB);
	for (i = 0; i < MAX_ROBOTS; i++)
		sprite[rob[i]].owner = i + 1;

	for (i = 0; i < MAX_SHIELD; i++)
		shi[i] = SetSprite(shield[0], HIDDEN, 0, 0, 1, 5, DIS_ANIM, 0, SHI);

	for (i = 0; i < MAX_ROCKETS; i++)
		roc[i] = SetSprite(rocket, HIDDEN, 0, 0, 2, 2, DIS_ANIM, 0, ROC);

	for (i = 0; i < MAX_EXPLOSION; i++)
		bum[i] = SetSprite(explosion, HIDDEN, 0, 0, 1, 6, OT_ANIM, 0, EXP);

	for (i = 0; i < MAX_RADAR; i++)
		rad[i] = SetSprite(radar[0], HIDDEN, 0, 0, 4, 1, DIS_ANIM, 0, RAD); //!! speed z 3 na 4

	//---------------------------------------------------------------------------------------------------

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	keystate = SDL_GetKeyState(NULL);

	if (resetGame() < 0)
	{
		printf("No programs to run !\n");
		return (0);
	}

	// thread = SDL_CreateThread(logic_thread, NULL);

	while (progExit == 0)
	{
		fps = SDL_GetTicks();

		while (SDL_PollEvent(&kbdEvent))
		{
			if (kbdEvent.type == SDL_QUIT || keystate[SDLK_q])
				progExit = 1;
			else if (keystate[SDLK_RIGHT])
				ld = R;
			else if (keystate[SDLK_LEFT])
				ld = L;
			else if (keystate[SDLK_UP])
				ld = U;
			else if (keystate[SDLK_DOWN])
				ld = D;
			else if (keystate[SDLK_s])
			{
				if (!debug && !drive)
				{
					status.stop = (status.stop == 2) ? 0 : ((status.stop == 0) ? 2 : 1);

					if (status.stop && !Mix_PausedMusic())
						Mix_PauseMusic();
					else
						Mix_ResumeMusic();
				}
			}
			else if (keystate[SDLK_r])
				resetGame();
			else if (keystate[SDLK_f])
			{
				cfg.fx_play = !cfg.fx_play;
			}
			else if (keystate[SDLK_m])
			{
				if (!Mix_PlayingMusic())
				{
					Mix_PlayMusic(bgmus, -1);
					cfg.bg_mus_play = 1;
				}
				else
				{
					Mix_FadeOutMusic(300);
					cfg.bg_mus_play = 0;
				}
			}
			else if (keystate[SDLK_SPACE])
			{
				if (debug)
					status.stop = 0;
			}
			else if (keystate[SDLK_a])
			{
				drive = 0;
				results_update = TRUE;
				debug = FALSE;
				status.stop = 0;
			}
			else if (keystate[SDLK_d])
			{
				debug = !debug;
				// printf("Debug mode is %s\n", debug?"on":"off");
				if (!debug)
					status.stop = 0;
				results_update = TRUE;
			}
			else if (keystate[SDLK_w])
			{
				status.stop = 1;
				winner = 1;
				start = SDL_GetTicks();
			}
			else if (keystate[SDLK_n])
			{
				showMsg("Connecting game server: ", NULL, NULL, NULL, 100, 100, 100);
			}
			else if (debug)
			{
				if (keystate[SDLK_1])
				{
					drive = 1;
					results_update = TRUE;
				}
				else if (keystate[SDLK_2])
				{
					drive = 2;
					results_update = TRUE;
				}
				else if (keystate[SDLK_3])
				{
					drive = 3;
					results_update = TRUE;
				}
				else if (keystate[SDLK_4])
				{
					drive = 4;
					results_update = TRUE;
				}
			}
		}

		if ((status.stop == 1) && ((SDL_GetTicks() - start) > 10000))
		{
			resetGame();
		}

		animate_all();

		{
			int w = (SDL_GetTicks() - fps);
			// printf("%d\n", w);
			SDL_Delay(((w > 7) ? 0 : (7 - w)));
		}
	}

	showGoodbye();
	SDL_Flip(screen);
	// SDL_Delay(10000);

	saveCFG(home_dir);

	Mix_FadeOutMusic(100);
	SDL_Delay(100);
	Mix_HaltMusic();
	Mix_HaltChannel(-1);

	// Mix_CloseAudio();
	SDL_CloseAudio();
	Mix_FreeMusic(bgmus);

	TTF_CloseFont(font1);
	TTF_CloseFont(font2);

	SDL_FreeSurface(robo[0]);
	SDL_FreeSurface(robo[1]);
	SDL_FreeSurface(robo[2]);
	SDL_FreeSurface(robo[3]);
	SDL_FreeSurface(bomb[0]);
	SDL_FreeSurface(bomb[1]);
	SDL_FreeSurface(bomb[2]);
	SDL_FreeSurface(bomb[3]);
	SDL_FreeSurface(rocket);
	SDL_FreeSurface(explosion);
	SDL_FreeSurface(radar[0]);
	SDL_FreeSurface(radar[1]);
	SDL_FreeSurface(bg);

	// SDL_FreeSurface(bombicon);
	// SDL_KillThread(thread);

	TTF_Quit();
	SDL_Quit();

	return 0;
}
