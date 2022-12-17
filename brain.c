/*
	RoboArena - game for programmers

	File: brain.c
	Created: 2015.09.30
	Author: Taddy G
	E-mail: fotonix@pm.me
	License: GPL
	Copyright (2015) by Taddy G
*/

#include <stdio.h>
#include <stdlib.h>

#define _GNU_SOURCE
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "roboarena.h"

#define MAX_INS 27
#define MAX_ISIZE (1024 * 1) // Maximal 1k instuctions

char *rComS[] = {"C_NOOP", "C_TURN_L", "C_TURN_R", "C_STEP", "C_ROCKET", "C_RADAR", "C_BOMB"};
char *rEventS[] = {"EV_NULL", "EV_WALL", "EV_ROCKET", "EV_ROBOT", "EV_STEP"};

struct compileErr *cErr;
char err_msg[64];

struct Labels
{
	char label[32];
	int pc;
} lab[256], flab[256];

struct Instruction
{
	unsigned char oc;
	unsigned char cond;
	short param;
} code[MAX_ISIZE];

unsigned short lnr[MAX_ISIZE];

struct RoboCPU
{
	unsigned short *lnr;
	struct Instruction *code;
	int regs[16];
	int pc;
	int wall;
	int rocket;
	int robot;
	int step;
	int ret;
	int stepcnt;
	rEvent lastevent;
	int intr : 1;
} cpus[4];

typedef enum
{
	NOP = 0,
	TURNL,
	TURNR,
	STEP,
	ROCKET,
	RADAR,
	BOMB,
	SET,
	CPY,
	ADD,
	ADDR,
	SUB,
	SUBR,
	JMP,
	JMPZ,
	JMPNZ,
	DJMPZ,
	DJMPNZ,
	RET,
	INV,
	NEG,
	AND,
	ANDR,
	OR,
	ORR,
	SSTEP,
	RND
} OpCodes;

struct
{
	char iname[8];
	OpCodes oc;
} ilist[MAX_INS] = {
	{
		iname : "nop",
		oc : NOP,
	},

	{
		iname : "turnl",
		oc : TURNL,
	},
	{
		iname : "turnr",
		oc : TURNR,
	},
	{
		iname : "step",
		oc : STEP,
	},
	{
		iname : "rocket",
		oc : ROCKET,
	},
	{
		iname : "radar",
		oc : RADAR,
	},
	{
		iname : "bomb",
		oc : BOMB,
	},

	{
		iname : "set ",
		oc : SET,
	},
	{
		iname : "copy ",
		oc : CPY,
	},

	{
		iname : "add ",
		oc : ADD,
	},
	{
		iname : "addr ",
		oc : ADDR,
	},
	{
		iname : "sub ",
		oc : SUB,
	},
	{
		iname : "subr ",
		oc : SUBR,
	},

	{iname : "jmp ", oc : JMP},
	{
		iname : "jmpz ",
		oc : JMPZ,
	},
	{iname : "jmpnz ", oc : JMPNZ},
	{
		iname : "djmpz ",
		oc : DJMPZ,
	},
	{iname : "djmpnz ", oc : DJMPNZ},
	{
		iname : "ret",
		oc : RET,
	},

	{
		iname : "inv ",
		oc : INV,
	},
	{
		iname : "neg ",
		oc : NEG,
	},
	{
		iname : "and ",
		oc : AND,
	},
	{
		iname : "andr ",
		oc : ANDR,
	},
	{
		iname : "or ",
		oc : OR,
	},
	{
		iname : "orr ",
		oc : ORR,
	},

	{
		iname : "sstep ",
		oc : SSTEP,
	},
	{
		iname : "rnd ",
		oc : RND,
	},
};

typedef enum
{
	EI_WALL = 0,
	EI_ROCKET = 1,
	EI_ROBOT = 2,
	EI_STEP = 3
} EventIndex;

struct Events
{
	char label[6];
	int pc;
} events[4] = {
	{
		label : "WALL",
	},
	{
		label : "ROCKET",
	},
	{
		label : "ROBOT",
	},
	{
		label : "STEP",
	},
};

void resetCPU(int nr)
{
	struct RoboCPU *cpu = &cpus[nr];

	if (cpu->code)
		memset(cpu->regs, 0, sizeof(cpu->regs));

	if (cpu->lnr)
		memset(cpu->lnr, 0, sizeof(lnr));

	cpu->pc = 0;
	cpu->ret = -1;
	cpu->stepcnt = 0;
	cpu->intr = 0;
	cpu->lastevent = EV_NULL;
}

void initCPU(int nr, struct Instruction *cp, int pwall, int procket, int probot, int pstep)
{
	struct RoboCPU *cpu = &cpus[nr];

	if (cpu->code == NULL)
		cpu->code = malloc(sizeof(code));

	if (cpu->lnr == NULL)
		cpu->lnr = malloc(sizeof(lnr));

	resetCPU(nr);

	memcpy(cpu->code, cp, sizeof(code));
	memcpy(cpu->lnr, lnr, sizeof(lnr));

	cpu->wall = (pwall < MAX_ISIZE - 1) ? pwall : 0;
	cpu->rocket = (procket < MAX_ISIZE - 1) ? procket : 0;
	cpu->robot = (probot < MAX_ISIZE - 1) ? probot : 0;
	cpu->step = (pstep < MAX_ISIZE - 1) ? pstep : 0;
	// printf("InitCPU: wall=%d rocket=%d robot=%d step=%d\n", cpus->wall, cpus->rocket, cpus->robot, cpus->step);
}

int compile(char *fname)
{
	static char *tl = NULL;
	FILE *fp;
	char *line = NULL;
	int lsize;
	int i, x;
	int pc = 0;
	int maxlab = 0;
	int maxflab = 0;
	int linenumber = 0;

	if ((tl == NULL) && ((tl = malloc(4096)) == NULL))
		return (0);

	linenumber = 0;
	memset(code, 0, sizeof(code));
	for (i = 0; i < 4; i++)
		events[i].pc = 0;

	printf("Compile file: %s\n", fname);

	fp = fopen(fname, "r");
	if (!fp)
	{
		printf("Unable to open file: %s\n", fname);
		return (0);
	}

	while (fgets(tl, 4096, fp) != NULL)
	{
		char *l;

		lsize = strlen(tl);
		err_msg[0] = 0;
		linenumber++;

		if (lsize < 3)
			continue;

		if ((l = index(tl, '#')) != NULL)
			*l = 0;
		if ((l = index(tl, '\n')) != NULL)
			*l = 0;
		if ((l = index(tl, '\r')) != NULL)
			*l = 0;

		for (x = strlen(tl) - 1; x > 0; x--)
		{
			if ((tl[x] == ' ') || (tl[x] == '\t'))
				tl[x] = 0;
			else
				break;
		}

		line = tl;
		while ((*line == ' ') || (*line == '\t'))
			line++;

		if (strlen(line) > 1)
		{
			if (index(line, ':') != NULL)
			{
				if ((l = index(line, '$')) != NULL)
				{
					for (x = 0; x < 4; x++)
					{
						if (strncmp(l + 1, events[x].label, strlen(events[x].label)) == 0)
						{
							if (events[x].pc)
							{
								snprintf(err_msg, sizeof(err_msg), "Duplicated event label \" $%s \"", events[x].label);
								goto fail;
							}

							events[x].pc = pc;
							printf("Add event %s label = PC(%d)\n", events[x].label, pc);
							break;
						}
					}
					if (x == 4)
						printf("Compilation warning:\n\t Skipping unknown event label: \" %s \" in line %d\n", l, linenumber);
				}
				else
				{
					int lc;

					lab[maxlab].pc = pc;
					strncpy(lab[maxlab].label, strtok(line, ":"), 32);

					for (lc = 0; lc < maxlab; lc++)
					{
						if (strncmp(lab[lc].label, lab[maxlab].label, strlen(lab[maxlab].label)) == 0)
						{
							sprintf(err_msg, "Duplicated label %s", lab[maxlab].label);
							goto fail;
						}
					}

					printf("Add label [%s] = PC(%d) - (%d labels) - line %d\n", lab[maxlab].label, pc, maxlab, linenumber);

					if (maxflab)
					{
						for (x = 0; x < maxflab; x++)
						{
							if (strncmp(lab[maxlab].label, flab[x].label, strlen(lab[maxlab].label)) == 0)
							{
								int pc = flab[x].pc;

								code[pc].param = lab[maxlab].pc - pc;
								flab[x].label[0] = 0;
								// printf("OpCode: %02X%02X%04X - [%s] @ %02d <-MOD\n", code[pc].oc, code[pc].cond, code[pc].param, ilist[code[pc].oc].iname, pc);
								break;
							}
						}
					}
					maxlab++;
				}
			}
			else
				for (i = 0; i < MAX_INS; i++)
				{
					if (strncmp(line, ilist[i].iname, strlen(ilist[i].iname)) == 0)
					{
						code[pc].oc = ilist[i].oc;
						code[pc].cond = 0;
						code[pc].param = 0;
						lnr[pc] = linenumber;

						switch (code[pc].oc)
						{
						case SET:  // set R1, 5
						case CPY:  // cpy R1, R2
						case ADD:  // add R3, 7
						case ADDR: // addr R2, R1
						case SUB:  // sub R0, 3
						case SUBR: // subr R1, R7
						case AND:  // and R7, 2
						case OR:   // or R9, 1
						case ANDR: // and R7, R3
						case ORR:  // or R9, R1
						{
							char *s1 = index(line, 'R');
							char *s2 = NULL;
							int d;

							if (s1)
							{
								s2 = index(s1, ',');
								if (!s2)
								{
									sprintf(err_msg, "Missing \',\' after register");
									goto fail;
								}
							}
							sscanf(s1, "R%d", &d);
							if ((d < 0) || (d > 15))
							{
								sprintf(err_msg, "Unavailable register (allowed R0 - R15)");
								goto fail;
							}
							code[pc].cond = d;

							while ((*s2 == ' ') || (*s2 == ',') || (*s2 == '\t'))
								s2++;

							if ((code[pc].oc == CPY) || (code[pc].oc == ADDR) || (code[pc].oc == SUBR) || (code[pc].oc == ANDR) || (code[pc].oc == ORR))
							{
								s1 = index(s2, 'R');
								if (s1)
								{
									int reg;

									s1++;
									// reg = atoi(s1);
									sscanf(s1, "R%d", &reg);
									if ((reg < 0) || (reg > 15))
									{
										sprintf(err_msg, "Unavailable register (allowed R0 - R15)");
										goto fail;
									}

									code[pc].cond |= (reg << 4);
									// printf("%s R%d to R%d === ", ilist[i].iname, code[pc].cond & 0xF, code[pc].cond>>4);
								}
							}
							else
							{
								int d;
								sscanf(s2, "%i", &d);
								code[pc].param = d;
								// printf("%s R%d to %d === ", ilist[i].iname, code[pc].cond, code[pc].param);
							}

							break;
						}

						case INV: // inv R0 (R0 = !R0)
						case NEG: // neg R1 (R0 = ~R0)
						case RND: // rnd R7 (R7 = random())
						{
							char *s1 = index(line, 'R');

							if (s1)
							{
								int d;
								sscanf(s1, "R%d", &d);
								if ((d < 0) || (d > 15))
								{
									sprintf(err_msg, "Unavailable register (allowed R0 - R15)");
									goto fail;
								}

								code[pc].cond = d;
							}
							else
							{
								sprintf(err_msg, "Expected register as param");
								goto fail;
							}

							break;
						}

						case SSTEP:
						{
							char *s1 = index(line, ' ');
							;

							if (s1)
							{
								int d;
								sscanf(s1, "%i", &d);
								code[pc].param = d;
							}
							else
							{
								sprintf(err_msg, "Expected value as param");
								goto fail;
							}

							break;
						}

						case JMP: // jmp label
						{
							char *s2 = index(line, ' ');

							while ((*s2 == ' ') || (*s2 == '\t'))
								s2++;

							for (x = 0; x < maxlab; x++)
							{
								if (strncmp(lab[x].label, s2, strlen(lab[x].label)) == 0)
								{
									code[pc].param = lab[x].pc - pc;
									break;
								}
							}
							if (x == maxlab)
							{
								strcpy(flab[maxflab].label, s2);
								flab[maxflab].pc = pc;
								maxflab++;
							}
							break;
						}

						case JMPZ:	 // jmpz R2, label
						case JMPNZ:	 // jmpnz R3, label
						case DJMPZ:	 // sub R4, 1; jmpz label
						case DJMPNZ: // sub R4, 1; jmpnz label
						{
							char *s1 = index(line, 'R');
							char *s2;

							if (s1)
							{
								s2 = index(s1, ',');
								if (!s2)
								{
									sprintf(err_msg, "Expexted \',\' after register numebr");
									goto fail;
								}
								else
								{
									int d;

									sscanf(s1, "R%d", &d);
									if ((d < 0) || (d > 15))
									{
										sprintf(err_msg, "Unavailable register (allowed R0 - R15)");
										goto fail;
									}
									code[pc].cond = d;

									while ((*s2 == ' ') || (*s2 == ',') || (*s2 == '\t'))
										s2++;

									l = s2;

									for (x = 0; x < maxlab; x++)
									{
										if (strncmp(l, lab[x].label, strlen(l)) == 0)
										{
											code[pc].param = lab[x].pc - pc;
											break;
										}
									}

									if (x == maxlab)
									{
										strcpy(flab[maxflab].label, l);
										flab[maxflab].pc = pc;
										maxflab++;
									}
									// printf("%s R%s [%s] ==== ", ilist[i].iname, tmpstr, lab[x].label);
								}
							}
							else
							{
								sprintf(err_msg, "Expected register as param");
								goto fail;
							}

							break;
						}
						}

						// printf("OpCode: %02X%02X%04X - [%s] @ %02d (line: %d)\n", code[pc].oc, code[pc].cond, code[pc].param, ilist[i].iname, pc, lnr[pc]);

						pc++;

						if (pc >= MAX_ISIZE)
						{
							sprintf(err_msg, "Program too long - allowed maximal %d instructions", pc);
							goto fail;
						}

						break;
					}
				}

			if (i == MAX_INS)
			{
				sprintf(err_msg, "Unknown instruction \'%s\'", line);
				goto fail;
			}
		}
	}

	for (i = 0; i < maxflab; i++)
	{
		if (flab[i].label[0])
		{
			sprintf(err_msg, "Reference to undefined label \"%s\"", flab[i].label);
			goto fail;
		}
	}

	if (tl)
	{
		free(tl);
		tl = NULL;
	}

	fclose(fp);
	/*
		printf("Opcodes/ASM listing:\n");
		for (i = 0; i < pc; i++)
			printf("Line: %03d PC[%03d]: %02X%02X%04X - [%s]\n", lnr[i], i, code[i].oc, code[i].cond, code[i].param, ilist[code[i].oc].iname);

		if (0)
		{
			char tn[20];

			sprintf(tn, "%s.bin", fname);
			int f = creat(tn, S_IRUSR | S_IWUSR);
			if (f > 0)
			{
				int x ;

				x = write(f, &events, sizeof(events));
				x += write(f, &code, sizeof(struct Instruction) * pc);
				close(f);
				printf("Binary version saved: %d bytes\n", x);
			}
		}
	*/

	printf("Compile finished - %d instruction generated\n\n", pc);

	return (pc);

fail:
	if (cErr)
	{
		cErr->err_ln = linenumber;
		cErr->err_file = fname;
	}

	printf("Compilation error:\n\t %s in line %d\n", err_msg, linenumber);
	printf("Compilation failed at line %d\n\n", linenumber);

	if (tl)
	{
		free(tl);
		tl = NULL;
	}

	fclose(fp);

	return (-1);
}

rCom doRandRoboMove(int x, rEvent ev)
{
	rCom dat[] = {C_NOOP, C_TURN_L, C_TURN_R, C_STEP, C_ROCKET, C_RADAR};
	int r, ret = C_NOOP;

	if (ev == EV_NULL)
	{
		do
			r = rand() & 7;
		while (r >= C_RADAR);

		ret = dat[r];
	}
	else
	{
		if (ev == EV_WALL)
			ret = C_TURN_L;
		if ((ev == EV_ROCKET) || (ev == EV_ROBOT))
			ret = C_ROCKET;
	}

	printf("MGEN: ROBO-%d CMD=%s \n", x + 1, rComS[ret]);

	return (ret);
}

rCom askRoboBrain(int robn, rEvent event, char *dis, int rnd, int dbg)
{
	struct RoboCPU *cpu = &cpus[robn & 3];
	int ret = C_NOOP;
	int pc, r1, r2, val;

	if (!cpu->code)
	{
		if (rnd == 0)
			return ret;
		else
			doRandRoboMove(robn, event);
	}

	// printf("askRoboBrain: %d\n", robn);

	if ((event != cpu->lastevent) && event /* && !pc->intr*/)
	{
		// printf(">>> ROBO-%d: Event %s @PC=%d (lastevent=%s)\n", robn+1, rEventS[event], cpu->pc, rEventS[cpu->lastevent]);

		if (cpu->ret == -1)
			cpu->ret = cpu->pc;

		switch (event)
		{
		case EV_NULL:
			break;
		case EV_WALL:
			if (cpu->wall)
			{
				cpu->pc = cpu->wall;
				cpu->intr = 1;
				cpu->lastevent = event;
			}
			break;
		case EV_ROCKET:
			if (cpu->rocket)
			{
				cpu->pc = cpu->rocket;
				cpu->intr = 1;
				cpu->lastevent = event;
			}
			break;
		case EV_ROBOT:
			if (cpu->robot)
			{
				cpu->pc = cpu->robot;
				cpu->intr = 1;
				cpu->lastevent = event;
			}
			break;
		}
	}

	pc = cpu->pc;
	r1 = cpu->code[pc].cond & 0xF;
	r2 = (cpu->code[pc].cond >> 4) & 0xF;
	val = cpu->code[pc].param;

	switch (cpu->code[pc].oc)
	{
	case NOP:
		cpu->pc++;
		break;
	case SET:
		cpu->regs[r1] = val;
		cpu->pc++;
		break;
	case CPY:
		cpu->regs[r1] = cpu->regs[r2];
		cpu->pc++;
		break;
	case ADD:
		cpu->regs[r1] += val;
		cpu->pc++;
		break;
	case ADDR:
		cpu->regs[r1] += cpu->regs[r2];
		cpu->pc++;
		break;
	case SUB:
		cpu->regs[r1] -= val;
		cpu->pc++;
		break;
	case SUBR:
		cpu->regs[r1] -= cpu->regs[r2];
		cpu->pc++;
		break;
	case AND:
		cpu->regs[r1] &= val;
		cpu->pc++;
		break;
	case OR:
		cpu->regs[r1] |= val;
		cpu->pc++;
		break;
	case ANDR:
		cpu->regs[r1] &= cpu->regs[r2];
		cpu->pc++;
		break;
	case ORR:
		cpu->regs[r1] |= cpu->regs[r2];
		cpu->pc++;
		break;
	case JMP:
		cpu->pc += val;
		break;
	case JMPZ:
		if (cpu->regs[r1] == 0)
			cpu->pc += val;
		else
			cpu->pc++;
		break;
	case JMPNZ:
		if (cpu->regs[r1] != 0)
			cpu->pc += val;
		else
			cpu->pc++;
		break;
	case DJMPZ:
		--cpu->regs[r1];
		if (cpu->regs[r1] == 0)
			cpu->pc += val;
		else
			cpu->pc++;
		break;
	case DJMPNZ:
		--cpu->regs[r1];
		if (cpu->regs[r1] != 0)
			cpu->pc += val;
		else
			cpu->pc++;
		break;
	case RET:
		cpu->pc = cpu->ret;
		cpu->ret = -1;
		cpu->intr = 0;
		cpu->lastevent = EV_NULL;
		break;
	case INV:
		cpu->regs[r1] = !cpu->regs[r1];
		cpu->pc++;
		break;
	case NEG:
		cpu->regs[r1] = ~cpu->regs[r1];
		cpu->pc++;
		break;
	case RND:
		cpu->regs[r1] = rand();
		cpu->pc++;
		break;
	case TURNL:
		ret = C_TURN_L;
		cpu->pc++;
		break;
	case TURNR:
		ret = C_TURN_R;
		cpu->pc++;
		break;
	case STEP:
		ret = C_STEP;
		if (cpu->step && (cpu->stepcnt > 0) && !cpu->intr)
		{
			cpu->stepcnt--;
			if (!cpu->stepcnt)
			{
				cpu->intr = 1;
				cpu->ret = cpu->pc;
				cpu->pc = cpu->step;
				break;
			}
		}
		cpu->pc++;
		break;
	case SSTEP:
		cpu->stepcnt = val;
		cpu->pc++;
		break;
	case ROCKET:
		ret = C_ROCKET;
		cpu->pc++;
		break;
	case RADAR:
		ret = C_RADAR;
		cpu->pc++;
		break;
	case BOMB:
		ret = C_BOMB;
		cpu->pc++;
		break;
	default:
		printf("Execute error: Unknown instruction [%02X%02X%04X] @PC=%d\n", code[pc].oc, code[pc].cond, code[pc].param, cpu->pc);
		cpu->pc++;
	}
	if (cpu->pc >= MAX_ISIZE)
		cpu->pc = 0;
	// printf("Brain answered (pc=%d oc=%X)\n", pc, cpu->code[pc].oc);

	if (dis)
	{
		char buf[30] = "";

		switch (cpu->code[pc].oc)
		{
		case SET:
		case CPY:
		case ADD:
		case SUB:
		case AND:
		case OR:
			sprintf(buf, "R%d, %d", r1, val);
			break;
		case ADDR:
		case SUBR:
		case ANDR:
		case ORR:
			sprintf(buf, "R%d, R%d", r1, r2);
			break;
		case JMP:
			sprintf(buf, "[%03d]", cpu->pc);
			break;
		case JMPZ:
		case JMPNZ:
		case DJMPZ:
		case DJMPNZ:
			sprintf(buf, "R%d, [%03d]", r1, cpu->pc);
			break;
		case INV:
		case NEG:
		case RND:
			sprintf(buf, "R%d", r1);
			break;
		case SSTEP:
			sprintf(buf, "%d", val);
			break;
		}

		if (dbg)
			snprintf(dis, 60, "[%03d]: %s%s line: %d", pc, ilist[cpu->code[pc].oc].iname, buf, cpu->lnr[pc]);
		else
			snprintf(dis, 60, "[%03d]: %s%s", pc, ilist[cpu->code[pc].oc].iname, buf);

		// printf("ROBO-%d| %s\n", robn+1, dis);

		if (0)
		{
			int r;
			for (r = 0; r < 8; r++)
				printf("R%02d=%04d ", r, cpu->regs[r]);
			printf("\n");
			for (r = 8; r < 16; r++)
				printf("R%02d=%04d ", r, cpu->regs[r]);

			printf("\nPC=%03d RET=%03d SCNT=%d\n", cpu->pc, cpu->ret, cpu->stepcnt);
		}
	}

	return (ret);
}

int brainInit(char *hdir, char *flist[], struct compileErr *ce)
{
	int r, x = 0;
	char path[4096];

	if (ce)
	{
		cErr = ce;
		cErr->err_msg = err_msg;
	}
	else
		cErr = NULL;

	snprintf(path, sizeof(path), "%s/%s", hdir, flist[0]);
	r = compile(path);
	if (r > 0)
	{
		initCPU(0, code, events[EI_WALL].pc, events[EI_ROCKET].pc, events[EI_ROBOT].pc, events[EI_STEP].pc);
		x |= 1;
	}
	else if (r < 0)
		return (r);
	else
		resetCPU(0);

	snprintf(path, sizeof(path), "%s/%s", hdir, flist[1]);
	r = compile(path);
	if (r > 0)
	{
		initCPU(1, code, events[EI_WALL].pc, events[EI_ROCKET].pc, events[EI_ROBOT].pc, events[EI_STEP].pc);
		x |= 2;
	}
	else if (r < 0)
		return (r);
	else
		resetCPU(1);

	snprintf(path, sizeof(path), "%s/%s", hdir, flist[2]);
	r = compile(path);
	if (r > 0)
	{
		initCPU(2, code, events[EI_WALL].pc, events[EI_ROCKET].pc, events[EI_ROBOT].pc, events[EI_STEP].pc);
		x |= 4;
	}
	else if (r < 0)
		return (r);
	else
		resetCPU(2);

	snprintf(path, sizeof(path), "%s/%s", hdir, flist[3]);
	r = compile(path);
	if (r > 0)
	{
		initCPU(3, code, events[EI_WALL].pc, events[EI_ROCKET].pc, events[EI_ROBOT].pc, events[EI_STEP].pc);
		x |= 8;
	}
	else if (r < 0)
		return (r);
	else
		resetCPU(3);

	return (x);
}
