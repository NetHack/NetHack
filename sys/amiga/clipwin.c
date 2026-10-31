static USHORT Palette[] = {
    0x0AAA, /* color #0 */
    0x0000, /* color #1 */
    0x0FFF, /* color #2 */
    0x058B, /* color #3 */
    0x000F, /* color #4 */
    0x0F0F, /* color #5 */
    0x00FF, /* color #6 */
    0x0FFF  /* color #7 */
#define PaletteColorCount 8
};

#define PALETTE Palette

static SHORT ClipBorderVectors1[] = { 0, 0, 76, 0, 76, 11, 0, 11, 0, 0 };
static struct Border ClipBorder1 = {
    -1, -1,             /* XY origin relative to container TopLeft */
    3, 0, JAM1,         /* front pen, back pen and drawmode */
    5,                  /* number of XY vectors */
    ClipBorderVectors1, /* pointer to XY vectors */
    NULL                /* next border in list */
};

static struct IntuiText ClipIText1 = {
    4,        0, JAM1, /* front and back text pens, drawmode and fill byte */
    15,       1,       /* XY origin relative to container TopLeft */
    NULL,              /* font pointer or NULL for default */
    "Cancel",          /* pointer to text */
    NULL               /* next IntuiText structure */
};

static struct Gadget ClipCancel = {
    NULL,                /* next gadget */
    240, 59,             /* origin XY of hit box relative to window TopLeft */
    75, 10,              /* hit box width and height */
    NULL,                /* gadget flags */
    RELVERIFY,           /* activation flags */
    BOOLGADGET,          /* gadget type flags */
    (APTR) &ClipBorder1, /* gadget border or image to be rendered */
    NULL,                /* alternate imagery for selection */
    &ClipIText1,         /* first IntuiText structure */
    NULL,                /* gadget mutual-exclude long word */
    NULL,                /* SpecialInfo structure */
    GADCANCEL,           /* user-definable data */
    NULL                 /* pointer to user-definable data */
};

static SHORT ClipBorderVectors2[] = { 0, 0, 78, 0, 78, 11, 0, 11, 0, 0 };
static struct Border ClipBorder2 = {
    -1, -1,             /* XY origin relative to container TopLeft */
    3, 0, JAM1,         /* front pen, back pen and drawmode */
    5,                  /* number of XY vectors */
    ClipBorderVectors2, /* pointer to XY vectors */
    NULL                /* next border in list */
};

static struct IntuiText ClipIText2 = {
    4,      0, JAM1, /* front and back text pens, drawmode and fill byte */
    24,     1,       /* XY origin relative to container TopLeft */
    NULL,            /* font pointer or NULL for default */
    "Okay",          /* pointer to text */
    NULL             /* next IntuiText structure */
};

static struct Gadget ClipOkay = {
    &ClipCancel,         /* next gadget */
    17, 60,              /* origin XY of hit box relative to window TopLeft */
    77, 10,              /* hit box width and height */
    NULL,                /* gadget flags */
    RELVERIFY,           /* activation flags */
    BOOLGADGET,          /* gadget type flags */
    (APTR) &ClipBorder2, /* gadget border or image to be rendered */
    NULL,                /* alternate imagery for selection */
    &ClipIText2,         /* first IntuiText structure */
    NULL,                /* gadget mutual-exclude long word */
    NULL,                /* SpecialInfo structure */
    GADOKAY,             /* user-definable data */
    NULL                 /* pointer to user-definable data */
};

static struct PropInfo ClipClipXCLIPSInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    24504, -1,            /* horizontal and vertical pot values */
    10922, -1,            /* horizontal and vertical body values */
};

static struct Image ClipImage1 = {
    43,     0,      /* XY origin relative to container TopLeft */
    24,     3,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

static struct IntuiText ClipIText3 = {
    3, 0, JAM1,       /* front and back text pens, drawmode and fill byte */
    -116, -1,         /* XY origin relative to container TopLeft */
    NULL,             /* font pointer or NULL for default */
    "X Clip Border:", /* pointer to text */
    NULL              /* next IntuiText structure */
};

static struct Gadget ClipXCLIP = {
    &ClipOkay, /* next gadget */
    134, 37,   /* origin XY of hit box relative to window TopLeft */
    -199, 7,   /* hit box width and height */
    GRELWIDTH, /* gadget flags */
    RELVERIFY + GADGIMMEDIATE,  /* activation flags */
    PROPGADGET,                 /* gadget type flags */
    (APTR) &ClipImage1,         /* gadget border or image to be rendered */
    NULL,                       /* alternate imagery for selection */
    &ClipIText3,                /* first IntuiText structure */
    NULL,                       /* gadget mutual-exclude long word */
    (APTR) &ClipClipXCLIPSInfo, /* SpecialInfo structure */
    XCLIP,                      /* user-definable data */
    NULL                        /* pointer to user-definable data */
};

static struct PropInfo ClipClipYCLIPSInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    13106, -1,            /* horizontal and vertical pot values */
    10922, -1,            /* horizontal and vertical body values */
};

static struct Image ClipImage2 = {
    22,     0,      /* XY origin relative to container TopLeft */
    24,     3,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

static struct IntuiText ClipIText4 = {
    3, 0, JAM1,       /* front and back text pens, drawmode and fill byte */
    -116, -1,         /* XY origin relative to container TopLeft */
    NULL,             /* font pointer or NULL for default */
    "Y Clip Border:", /* pointer to text */
    NULL              /* next IntuiText structure */
};

static struct Gadget ClipYCLIP = {
    &ClipXCLIP, /* next gadget */
    134, 46,    /* origin XY of hit box relative to window TopLeft */
    -199, 7,    /* hit box width and height */
    GRELWIDTH,  /* gadget flags */
    RELVERIFY + GADGIMMEDIATE,  /* activation flags */
    PROPGADGET,                 /* gadget type flags */
    (APTR) &ClipImage2,         /* gadget border or image to be rendered */
    NULL,                       /* alternate imagery for selection */
    &ClipIText4,                /* first IntuiText structure */
    NULL,                       /* gadget mutual-exclude long word */
    (APTR) &ClipClipYCLIPSInfo, /* SpecialInfo structure */
    YCLIP,                      /* user-definable data */
    NULL                        /* pointer to user-definable data */
};

static struct PropInfo ClipClipXSIZESInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    26212, -1,            /* horizontal and vertical pot values */
    10922, -1,            /* horizontal and vertical body values */
};

static struct Image ClipImage3 = {
    45,     0,      /* XY origin relative to container TopLeft */
    24,     3,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

static struct IntuiText ClipIText5 = {
    3, 0, JAM1,        /* front and back text pens, drawmode and fill byte */
    -124, -1,          /* XY origin relative to container TopLeft */
    NULL,              /* font pointer or NULL for default */
    "X Scale Factor:", /* pointer to text */
    NULL               /* next IntuiText structure */
};

static struct Gadget ClipXSIZE = {
    &ClipYCLIP, /* next gadget */
    134, 15,    /* origin XY of hit box relative to window TopLeft */
    -199, 7,    /* hit box width and height */
    GRELWIDTH,  /* gadget flags */
    RELVERIFY + GADGIMMEDIATE,  /* activation flags */
    PROPGADGET,                 /* gadget type flags */
    (APTR) &ClipImage3,         /* gadget border or image to be rendered */
    NULL,                       /* alternate imagery for selection */
    &ClipIText5,                /* first IntuiText structure */
    NULL,                       /* gadget mutual-exclude long word */
    (APTR) &ClipClipXSIZESInfo, /* SpecialInfo structure */
    XSIZE,                      /* user-definable data */
    NULL                        /* pointer to user-definable data */
};

static struct PropInfo ClipClipYSIZESInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    -25937, -1,           /* horizontal and vertical pot values */
    10922, -1,            /* horizontal and vertical body values */
};

static struct Image ClipImage4 = {
    69,     0,      /* XY origin relative to container TopLeft */
    24,     3,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

static struct IntuiText ClipIText6 = {
    3, 0, JAM1,        /* front and back text pens, drawmode and fill byte */
    -124, -1,          /* XY origin relative to container TopLeft */
    NULL,              /* font pointer or NULL for default */
    "Y Scale Factor:", /* pointer to text */
    NULL               /* next IntuiText structure */
};

static struct Gadget ClipYSIZE = {
    &ClipXSIZE, /* next gadget */
    134, 24,    /* origin XY of hit box relative to window TopLeft */
    -199, 7,    /* hit box width and height */
    GRELWIDTH,  /* gadget flags */
    RELVERIFY + GADGIMMEDIATE,  /* activation flags */
    PROPGADGET,                 /* gadget type flags */
    (APTR) &ClipImage4,         /* gadget border or image to be rendered */
    NULL,                       /* alternate imagery for selection */
    &ClipIText6,                /* first IntuiText structure */
    NULL,                       /* gadget mutual-exclude long word */
    (APTR) &ClipClipYSIZESInfo, /* SpecialInfo structure */
    YSIZE,                      /* user-definable data */
    NULL                        /* pointer to user-definable data */
};

#define ClipGadgetList1 ClipYSIZE

static struct NewWindow ClipNewWindowStructure1 = {
    114, 16, /* window XY origin relative to TopLeft of screen */
    346, 76, /* window width and height */
    0, 1,    /* detail and block pens */
    NEWSIZE + MOUSEMOVE + GADGETDOWN + GADGETUP + CLOSEWINDOW + ACTIVEWINDOW
        + VANILLAKEY + INTUITICKS, /* IDCMP flags */
    WINDOWSIZING + WINDOWDRAG + WINDOWDEPTH + WINDOWCLOSE + ACTIVATE
        + NOCAREREFRESH,        /* other window flags */
    &ClipYSIZE,                 /* first gadget in gadget list */
    NULL,                       /* custom CHECKMARK imagery */
    "Edit Clipping Parameters", /* window title */
    NULL,                       /* custom screen pointer */
    NULL,                       /* custom bitmap */
    350,
    76,          /* minimum width and height */
    -1, -1,      /* maximum width and height */
    CUSTOMSCREEN /* destination screen type */
};

/* end of PowerWindows source generation */
