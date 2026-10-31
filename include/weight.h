/* NetHack 5.0	weight.h	$NHDT-Date: 1742427965 2025/03/19 23:46:05 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) Michael Allison, 2025. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WEIGHT_H
#define WEIGHT_H

/* weight-related constants and thresholds */
enum weight_constants {
    WT_ETHEREAL          =    0,
    WT_SPLASH_THRESHOLD  =    9,  /* weight needed to make splash in water */
    WT_WEIGHTCAP_STRCON  =   25,  /* str + con multiplied by this for conv to
                                   * carrying capacity in weight_cap() */
    WT_WEIGHTCAP_SPARE   =   50,  /* used in weight_cap calc */
    WT_JELLY             =   50,  /* weight of jelly body */
    WT_WOUNDEDLEG_REDUCT =  100,  /* wounded legs reduce carrcap by this */
    WT_TO_DMG            =  100,  /* divisor to convert weight to dmg amt */
    WT_IRON_BALL_INCR    =  160,  /* weight increment of heavy iron ball */
    WT_IRON_BALL_BASE    =  480,  /* base starting weight of iron ball */
    WT_NOISY_INV         =  500,  /* inv_weight() max for noisy fumbling */
    WT_NYMPH             =  600,  /* weight of nymph body */
    WT_TOOMUCH_DIAGONAL  =  600,  /* weight_cap threshold for diag squeeze */
    WT_ELF               =  800,  /* weight of elf body */
    WT_SQUEEZABLE_INV    =  850,  /* inv_weight() maximum for squeezing */
    MAX_CARR_CAP         = 1000,  /* max carrying capacity, so that
                                   * boulders can be heavier */
    WT_HUMAN             = 1450,  /* weight of human body */
    WT_BABY_DRAGON       = 1500,  /* weight of baby dragon body */
    WT_DRAGON            = 4500,  /* weight of dragon body */
};

#endif /* WEIGHT_H */


