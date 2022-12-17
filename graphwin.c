/*
	RoboArena - game for programmers

	File: graphwin.c
	Created: 2015.09.22
	Author: Taddy G
	E-mail: fotonix@pm.me
	License: GPL
	Copyright (2015) by Taddy G
*/

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>

#include "roboarena.h"

//==========================================================================================
// Routines for right panel, text and windows
int putText(int x, int y, SDL_Color color, TTF_Font *font, const char *fmt, ...)
{
	SDL_Rect txt;
	SDL_Surface *text;

	if (font)
	{
		int h = 0;
		va_list params;
		char log_txt[512];

		va_start(params, fmt);
		vsnprintf(log_txt, sizeof(log_txt), fmt, params);
		va_end(params);

		txt.x = x;
		txt.y = y;
		text = TTF_RenderText_Solid(font, log_txt, color);

		if (text)
		{
			h = text->h;
			SDL_BlitSurface(text, NULL, screen, &txt);
			SDL_FreeSurface(text);
		}

		return (h);
	}

	return (0);
}

int drawResults(int x, int y, int dbg, int drv, int stop)
{

#define OFFSET_X	100
#define OFFSET_Y	25
#define R_SX		24
#define R_SY		30
#define R_SY2	R_SY - 10

	int i, j;
	SDL_Rect win, fillR0, fillR1, fillR2, bombIconR;
	SDL_Color textColor = {0, 0, 0};
	SDL_Color pointsColor = {255, 255, 255};

	Uint32 bg = SDL_MapRGB((screen)->format, 195, 165, 90);
	Uint32 gr = SDL_MapRGB((screen)->format, 0, 200, 0);
	Uint32 re = SDL_MapRGB((screen)->format, 200, 0, 0);

	Uint32 robc[4];

	robc[0] = SDL_MapRGB((screen)->format, 255, 255, 255);
	robc[1] = SDL_MapRGB((screen)->format, 255, 0, 0);
	robc[2] = SDL_MapRGB((screen)->format, 0, 255,0);
	robc[3] = SDL_MapRGB((screen)->format, 248, 161, 4);

	win.x = ARENA_W;
 	win.y = 0;
	win.w = RESULT_COLW;
	win.h = ARENA_H;
	SDL_FillRect(screen, &win, bg);

	if (drv)
		putText(win.x + 15, win.y, textColor, font1, "Keyboard drives ROBO-%d", drv);
	else
	if (dbg)
		putText(win.x + 15, win.y, textColor, font1, "DEBUG mode (spacebar = step)");
	else
		if (stop == 2)
		putText(win.x + 15, win.y, textColor, font1, "STOP - press S tu run");
	else
	if (stop == 3)
		putText(win.x + 15, win.y, textColor, font1, "STOP - COMPILATION ERROR");
	else
		putText(win.x + 15, win.y, textColor, font1, "AUTOPLAY mode");

	for (i = 0; i < MAX_ROBOTS; i++)
	{
		int idx = rob[i];
		Uint32 tgr = (!sprite[idx].visible) ? SDL_MapRGB((screen)->format, 10, 10, 10) : gr;

		fillR0.x = win.x + 10;
		fillR0.y = win.y + OFFSET_Y + (R_SY * i);
		fillR0.w = OFFSET_X - 10;
		fillR0.h = R_SY2;
		SDL_FillRect(screen, &fillR0, robc[i]);

		putText(win.x + 15, win.y + OFFSET_Y - 1 + (R_SY * i), textColor, font1, "ROBO-%d", i+1);

		fillR1.x = fillR0.x + OFFSET_X;
		fillR1.y = fillR0.y;
		fillR1.w = R_SX * sprite[idx].life;
		fillR1.h = R_SY2;
		SDL_FillRect(screen, &fillR1, tgr);

		if (MAX_LIFE - sprite[idx].life)
		{
			fillR2.x = fillR1.x + fillR1.w;
			fillR2.y = fillR1.y;
			fillR2.w = R_SX * (MAX_LIFE - sprite[idx].life);
			fillR2.h = R_SY2;
			SDL_FillRect(screen, &fillR2, re);
		}

		for (j = 0; j < sprite[idx].bomb; j++)
		{
			bombIconR.x = fillR1.x + 103 + (14 * j);
			bombIconR.y = fillR1.y - 3;
			SDL_BlitSurface(bombicon, NULL, screen, &bombIconR);
		}

		putText(fillR1.x + 5, fillR1.y - 1, pointsColor, font1, "%d", sprite[idx].points);
		//putText(fillR1.x + 65, fillR1.y - 1, pointsColor, font1, "S:%d", sprite[idx].scnt);
	}

	printCodes(NULL, 0);

	return 0;
}

void drawStatus(struct Status *s)
{
	SDL_Rect win;
	SDL_Color white = {255, 255, 255};
	SDL_Color red = {255, 0, 0};

	win.x = 0;
    win.y = ARENA_W;
	win.w = ARENA_W;
	win.h = STATUS_H;
	SDL_FillRect(screen, &win, SDL_MapRGB((screen)->format, 0, 0, 0));

	if (s->stop == 3)
	{
		putText(win.x + 5, win.y + 5, red, font1, "Compilation error !!!");
		putText(win.x + 5, win.y + 25, red, font1, "File: %s", s->comErr->err_file);
		putText(win.x + 5, win.y + 45, red, font1, "Line: %d", s->comErr->err_ln);
		putText(win.x + 5, win.y + 65, red, font2, "%s", s->comErr->err_msg);
	} else
	{
		putText(win.x + 5, win.y + 5, white, font1, "CPU cycles: %d/%d Time left: %d sec", s->CPUclk, s->maxclk, s->tt_end / 1000);
		putText(win.x + 5, win.y + 25, white, font1, "SFX: %s", s->conf->fx_play ? "on" : "off");
		putText(win.x + 5, win.y + 45, white, font1, "BGM: %s", s->conf->bg_mus_play ? "on" : "off");
	}
}

void printCodes(char *line, int reset)
{
#define LNR	17

	static char lines[LNR][64];
	static int lc[LNR];
	Uint32 bg = SDL_MapRGB((screen)->format, 0, 0, 0);
	SDL_Color textColor[4] = {{255, 255, 255}, {255, 0, 0}, {0, 255, 0}, {248, 161, 4}};
	SDL_Rect win;
	int ln;

	if (reset)
	{
		memset(lines, 0, sizeof(lines));
		memset(lc, 0, sizeof(lc));
	}

	if (line && strlen(line))
	{
		//printf("%s\n", line);
		for (ln = 1; ln < LNR; ln++)
		{
			strcpy(lines[ln-1], lines[ln]);
			lc[ln-1] = lc[ln];
		}

		memset(lines[LNR-1], 0, 60);
		strncpy(lines[LNR-1], line, 60);
		lc[LNR-1] = rc;
		line[0] = 0;
	}

	win.x = ARENA_W + 10;
	win.y = 145;
	win.w = 280;
	win.h = 280;
	SDL_FillRect(screen, &win, bg);

	for (ln = 0; ln < LNR; ln++)
	{
		if (strlen(lines[ln]))
			putText(win.x + 5, win.y + 2 + (16 * ln), textColor[lc[ln]], font2, "%s", lines[ln]);
	}
}

void showMsg(char *msg1, char *msg2, char *msg3, char *msg4, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_Rect win;
	SDL_Color color = {200, 200, 190};
	Uint32 bg = SDL_MapRGB((screen)->format, r, g, b);

	win.x = ARENA_W + 10;
	win.y = 430;
	win.w = 280;
	win.h = 65;
	SDL_FillRect(screen, &win, bg);

	if (msg1)
		putText(win.x + 5, win.y, color, font2, "%s", msg1);
	if (msg2)
		putText(win.x + 5, win.y + 15, color, font2, "%s", msg2);
	if (msg3)
		putText(win.x + 5, win.y + 30, color, font2, "%s", msg3);
	if (msg4)
		putText(win.x + 5, win.y + 45, color, font2, "%s", msg4);

	//printf("showMsg: %s %s\n", msg, s);
}

void showWinner(int nr, int tnr)
{
	static int d = R;
	static int x = 0;
	static int y = 110;
	int idx = rob[nr - 1];

	SDL_Rect fx, fx2, ip, is;
	SDL_Color textColor = {200, 0, 0};
	Uint32 bo = SDL_MapRGB((screen)->format, 10, 10, 10);
	Uint32 bg = SDL_MapRGB((screen)->format, 200, 200, 200);

	fx.x = 75;
	fx.y = 150;
	fx.w = 350;
	fx.h = 200;
	SDL_FillRect(screen, &fx, bo);

	fx2.x = fx.x + 25;
	fx2.y = fx.y + 25;
	fx2.w = fx.w - 50;
	fx2.h = fx.h - 50;
	SDL_FillRect(screen, &fx2, bg);

	ip.x = fx2.x + x - 5;
	ip.y = fx2.y + y - 5;
	ip.w = ip.h = 50;
	is.x = 50 * d * sprite[idx].aph;
	is.y = 0;
	is.w = is.h = 50;
	SDL_BlitSurface(sprite[idx].img, &is, screen, &ip);

	putText(fx2.x + 100, fx2.y + 10, textColor, font1, "%s OVER", (tnr == 0) ? "GAME" : "TIME IS");
	putText(fx2.x + 45,  fx2.y + 40, textColor, font1, "THE WINNER IS ROBO-%d", nr);
	putText(fx2.x + 45,  fx2.y + 70, textColor, font1, "TOTAL POINTS: %d", sprite[idx].points);
	putText(fx2.x + 45,  fx2.y + 90, textColor, font1, "STEPS:%d", sprite[idx].scnt);

	if ((d == R) && (++x > 260))
		d = U;
	if ((d == U) && (--y < 0))
		d = L;
	if ((d == L) && (--x <= 0))
		d = D;
	if ((d == D) && (++y > 110))
		d = R;
}

void showGoodbye(void)
{
	SDL_Rect fx, fx2;
	SDL_Color textColor = {200, 0, 0};
	Uint32 bg = SDL_MapRGB((screen)->format, 200, 200, 200);
	Uint32 bo = SDL_MapRGB((screen)->format, 250, 0, 0);

	fx.x = 25;
	fx.y = 150;
	fx.w = 450;
	fx.h = 200;
	SDL_FillRect(screen, &fx, bo);

	fx2.x = fx.x + 25;
	fx2.y = fx.y + 25;
	fx2.w = fx.w - 50;
	fx2.h = fx.h - 50;
	SDL_FillRect(screen, &fx2, bg);

	putText(fx2.x + 45, fx2.y + 60, textColor, font1, "GOODBYE, BRAVE ROBO PROGRAMMER !");
}

SDL_Surface * LoadSprite(char *name, Uint8 alpha)
{
	SDL_Surface *temp, *img;

	temp = IMG_Load(name);
	if (temp == NULL)
	{
		printf("ERROR: Problem with loading image: %s\n", name);
		return (NULL);
	}

	SDL_SetColorKey(temp, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB((temp)->format, 0, 0, 0));
	SDL_SetAlpha(temp, SDL_SRCCOLORKEY | SDL_RLEACCEL | SDL_SRCALPHA, alpha);
	img = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);

	return (img);
}


