#define CHAR 257
#define INTEGER 258
#define BOOLEAN 259
#define PERCENT 260
#define SPERCENT 261
#define MINUS_INTEGER 262
#define PLUS_INTEGER 263
#define MAZE_GRID_ID 264
#define SOLID_FILL_ID 265
#define MINES_ID 266
#define ROGUELEV_ID 267
#define MESSAGE_ID 268
#define MAZE_ID 269
#define LEVEL_ID 270
#define LEV_INIT_ID 271
#define GEOMETRY_ID 272
#define NOMAP_ID 273
#define OBJECT_ID 274
#define COBJECT_ID 275
#define MONSTER_ID 276
#define TRAP_ID 277
#define DOOR_ID 278
#define DRAWBRIDGE_ID 279
#define object_ID 280
#define monster_ID 281
#define terrain_ID 282
#define MAZEWALK_ID 283
#define WALLIFY_ID 284
#define REGION_ID 285
#define FILLING 286
#define IRREGULAR 287
#define JOINED 288
#define ALTAR_ID 289
#define LADDER_ID 290
#define STAIR_ID 291
#define NON_DIGGABLE_ID 292
#define NON_PASSWALL_ID 293
#define ROOM_ID 294
#define PORTAL_ID 295
#define TELEPRT_ID 296
#define BRANCH_ID 297
#define LEV 298
#define MINERALIZE_ID 299
#define CORRIDOR_ID 300
#define GOLD_ID 301
#define ENGRAVING_ID 302
#define FOUNTAIN_ID 303
#define POOL_ID 304
#define SINK_ID 305
#define NONE 306
#define RAND_CORRIDOR_ID 307
#define DOOR_STATE 308
#define LIGHT_STATE 309
#define CURSE_TYPE 310
#define ENGRAVING_TYPE 311
#define DIRECTION 312
#define RANDOM_TYPE 313
#define RANDOM_TYPE_BRACKET 314
#define A_REGISTER 315
#define ALIGNMENT 316
#define LEFT_OR_RIGHT 317
#define CENTER 318
#define TOP_OR_BOT 319
#define ALTAR_TYPE 320
#define UP_OR_DOWN 321
#define SUBROOM_ID 322
#define NAME_ID 323
#define FLAGS_ID 324
#define FLAG_TYPE 325
#define MON_ATTITUDE 326
#define MON_ALERTNESS 327
#define MON_APPEARANCE 328
#define ROOMDOOR_ID 329
#define IF_ID 330
#define ELSE_ID 331
#define TERRAIN_ID 332
#define HORIZ_OR_VERT 333
#define REPLACE_TERRAIN_ID 334
#define EXIT_ID 335
#define SHUFFLE_ID 336
#define QUANTITY_ID 337
#define BURIED_ID 338
#define LOOP_ID 339
#define FOR_ID 340
#define TO_ID 341
#define SWITCH_ID 342
#define CASE_ID 343
#define BREAK_ID 344
#define DEFAULT_ID 345
#define ERODED_ID 346
#define TRAPPED_STATE 347
#define RECHARGED_ID 348
#define INVIS_ID 349
#define GREASED_ID 350
#define FEMALE_ID 351
#define CANCELLED_ID 352
#define REVIVED_ID 353
#define AVENGE_ID 354
#define FLEEING_ID 355
#define BLINDED_ID 356
#define PARALYZED_ID 357
#define STUNNED_ID 358
#define CONFUSED_ID 359
#define SEENTRAPS_ID 360
#define ALL_ID 361
#define MONTYPE_ID 362
#define GRAVE_ID 363
#define ERODEPROOF_ID 364
#define FUNCTION_ID 365
#define MSG_OUTPUT_TYPE 366
#define COMPARE_TYPE 367
#define UNKNOWN_TYPE 368
#define rect_ID 369
#define fillrect_ID 370
#define line_ID 371
#define randline_ID 372
#define grow_ID 373
#define selection_ID 374
#define flood_ID 375
#define rndcoord_ID 376
#define circle_ID 377
#define ellipse_ID 378
#define filter_ID 379
#define complement_ID 380
#define gradient_ID 381
#define GRADIENT_TYPE 382
#define LIMITED 383
#define HUMIDITY_TYPE 384
#define STRING 385
#define MAP_ID 386
#define NQSTRING 387
#define VARSTRING 388
#define CFUNC 389
#define CFUNC_INT 390
#define CFUNC_STR 391
#define CFUNC_COORD 392
#define CFUNC_REGION 393
#define VARSTRING_INT 394
#define VARSTRING_INT_ARRAY 395
#define VARSTRING_STRING 396
#define VARSTRING_STRING_ARRAY 397
#define VARSTRING_VAR 398
#define VARSTRING_VAR_ARRAY 399
#define VARSTRING_COORD 400
#define VARSTRING_COORD_ARRAY 401
#define VARSTRING_REGION 402
#define VARSTRING_REGION_ARRAY 403
#define VARSTRING_MAPCHAR 404
#define VARSTRING_MAPCHAR_ARRAY 405
#define VARSTRING_MONST 406
#define VARSTRING_MONST_ARRAY 407
#define VARSTRING_OBJ 408
#define VARSTRING_OBJ_ARRAY 409
#define VARSTRING_SEL 410
#define VARSTRING_SEL_ARRAY 411
#define METHOD_INT 412
#define METHOD_INT_ARRAY 413
#define METHOD_STRING 414
#define METHOD_STRING_ARRAY 415
#define METHOD_VAR 416
#define METHOD_VAR_ARRAY 417
#define METHOD_COORD 418
#define METHOD_COORD_ARRAY 419
#define METHOD_REGION 420
#define METHOD_REGION_ARRAY 421
#define METHOD_MAPCHAR 422
#define METHOD_MAPCHAR_ARRAY 423
#define METHOD_MONST 424
#define METHOD_MONST_ARRAY 425
#define METHOD_OBJ 426
#define METHOD_OBJ_ARRAY 427
#define METHOD_SEL 428
#define METHOD_SEL_ARRAY 429
#define DICE 430
typedef union
{
	long	i;
	char*	map;
	struct {
		long room;
		long wall;
		long door;
	} corpos;
    struct {
	long area;
	long x1;
	long y1;
	long x2;
	long y2;
    } lregn;
    struct {
	long x;
	long y;
    } crd;
    struct {
	long ter;
	long lit;
    } terr;
    struct {
	long height;
	long width;
    } sze;
    struct {
	long die;
	long num;
    } dice;
    struct {
	long cfunc;
	char *varstr;
    } meth;
} YYSTYPE;
extern YYSTYPE yylval;
