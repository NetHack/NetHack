#define INTEGER 257
#define A_DUNGEON 258
#define BRANCH 259
#define CHBRANCH 260
#define LEVEL 261
#define RNDLEVEL 262
#define CHLEVEL 263
#define RNDCHLEVEL 264
#define UP_OR_DOWN 265
#define PROTOFILE 266
#define DESCRIPTION 267
#define DESCRIPTOR 268
#define LEVELDESC 269
#define ALIGNMENT 270
#define LEVALIGN 271
#define ENTRY 272
#define STAIR 273
#define NO_UP 274
#define NO_DOWN 275
#define PORTAL 276
#define STRING 277
typedef union
{
	int	i;
	char*	str;
} YYSTYPE;
extern YYSTYPE yylval;
