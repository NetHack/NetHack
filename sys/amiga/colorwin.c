SHORT Col_BorderVectors1[] = { 0, 0, 59, 0, 59, 12, 0, 12, 0, 0 };
struct Border Col_Border1 = {
    -1, -1,             /* XY origin relative to container TopLeft */
    3, 0, JAM1,         /* front pen, back pen and drawmode */
    5,                  /* number of XY vectors */
    Col_BorderVectors1, /* pointer to XY vectors */
    NULL                /* next border in list */
};

struct IntuiText Col_IText1 = {
    7,      0, JAM1, /* front and back text pens, drawmode and fill byte */
    13,     1,       /* XY origin relative to container TopLeft */
    NULL,            /* font pointer or NULL for default */
    "Save",          /* pointer to text */
    NULL             /* next IntuiText structure */
};

struct Gadget Col_Save = {
    NULL,                /* next gadget */
    9, 77,               /* origin XY of hit box relative to window TopLeft */
    58, 11,              /* hit box width and height */
    NULL,                /* gadget flags */
    RELVERIFY,           /* activation flags */
    BOOLGADGET,          /* gadget type flags */
    (APTR) &Col_Border1, /* gadget border or image to be rendered */
    NULL,                /* alternate imagery for selection */
    &Col_IText1,         /* first IntuiText structure */
    NULL,                /* gadget mutual-exclude long word */
    NULL,                /* SpecialInfo structure */
    GADCOLSAVE,          /* user-definable data */
    NULL                 /* pointer to user-definable data */
};

SHORT Col_BorderVectors2[] = { 0, 0, 59, 0, 59, 12, 0, 12, 0, 0 };
struct Border Col_Border2 = {
    -1, -1,             /* XY origin relative to container TopLeft */
    3, 0, JAM1,         /* front pen, back pen and drawmode */
    5,                  /* number of XY vectors */
    Col_BorderVectors2, /* pointer to XY vectors */
    NULL                /* next border in list */
};

struct IntuiText Col_IText2 = {
    7,     0, JAM1, /* front and back text pens, drawmode and fill byte */
    17,    1,       /* XY origin relative to container TopLeft */
    NULL,           /* font pointer or NULL for default */
    "Use",          /* pointer to text */
    NULL            /* next IntuiText structure */
};

struct Gadget Col_Okay = {
    &Col_Save,           /* next gadget */
    128, 77,             /* origin XY of hit box relative to window TopLeft */
    58, 11,              /* hit box width and height */
    NULL,                /* gadget flags */
    RELVERIFY,           /* activation flags */
    BOOLGADGET,          /* gadget type flags */
    (APTR) &Col_Border2, /* gadget border or image to be rendered */
    NULL,                /* alternate imagery for selection */
    &Col_IText2,         /* first IntuiText structure */
    NULL,                /* gadget mutual-exclude long word */
    NULL,                /* SpecialInfo structure */
    GADCOLOKAY,          /* user-definable data */
    NULL                 /* pointer to user-definable data */
};

SHORT Col_BorderVectors3[] = { 0, 0, 59, 0, 59, 12, 0, 12, 0, 0 };
struct Border Col_Border3 = {
    -1, -1,             /* XY origin relative to container TopLeft */
    3, 0, JAM1,         /* front pen, back pen and drawmode */
    5,                  /* number of XY vectors */
    Col_BorderVectors3, /* pointer to XY vectors */
    NULL                /* next border in list */
};

struct IntuiText Col_IText3 = {
    7,        0, JAM1, /* front and back text pens, drawmode and fill byte */
    6,        1,       /* XY origin relative to container TopLeft */
    NULL,              /* font pointer or NULL for default */
    "Cancel",          /* pointer to text */
    NULL               /* next IntuiText structure */
};

struct Gadget Col_Cancel = {
    &Col_Okay,           /* next gadget */
    244, 77,             /* origin XY of hit box relative to window TopLeft */
    58, 11,              /* hit box width and height */
    NULL,                /* gadget flags */
    RELVERIFY,           /* activation flags */
    BOOLGADGET,          /* gadget type flags */
    (APTR) &Col_Border3, /* gadget border or image to be rendered */
    NULL,                /* alternate imagery for selection */
    &Col_IText3,         /* first IntuiText structure */
    NULL,                /* gadget mutual-exclude long word */
    NULL,                /* SpecialInfo structure */
    GADCOLCANCEL,        /* user-definable data */
    NULL                 /* pointer to user-definable data */
};

struct PropInfo Col_Col_RedPenSInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    0, 0,                 /* horizontal and vertical pot values */
    -1, -1,               /* horizontal and vertical body values */
};

struct Image Col_Image1 = {
    0,      0,      /* XY origin relative to container TopLeft */
    263,    7,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

struct Gadget Col_RedPen = {
    &Col_Cancel, /* next gadget */
    32, 12,      /* origin XY of hit box relative to window TopLeft */
    271, 11,     /* hit box width and height */
    NULL,        /* gadget flags */
    RELVERIFY + GADGIMMEDIATE + FOLLOWMOUSE, /* activation flags */
    PROPGADGET,                              /* gadget type flags */
    (APTR) &Col_Image1,          /* gadget border or image to be rendered */
    NULL,                        /* alternate imagery for selection */
    NULL,                        /* first IntuiText structure */
    NULL,                        /* gadget mutual-exclude long word */
    (APTR) &Col_Col_RedPenSInfo, /* SpecialInfo structure */
    GADREDPEN,                   /* user-definable data */
    NULL                         /* pointer to user-definable data */
};

struct PropInfo Col_Col_GreenPenSInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    0, 0,                 /* horizontal and vertical pot values */
    -1, -1,               /* horizontal and vertical body values */
};

struct Image Col_Image2 = {
    0,      0,      /* XY origin relative to container TopLeft */
    263,    7,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

struct Gadget Col_GreenPen = {
    &Col_RedPen, /* next gadget */
    32, 24,      /* origin XY of hit box relative to window TopLeft */
    271, 11,     /* hit box width and height */
    NULL,        /* gadget flags */
    RELVERIFY + GADGIMMEDIATE + FOLLOWMOUSE, /* activation flags */
    PROPGADGET,                              /* gadget type flags */
    (APTR) &Col_Image2,            /* gadget border or image to be rendered */
    NULL,                          /* alternate imagery for selection */
    NULL,                          /* first IntuiText structure */
    NULL,                          /* gadget mutual-exclude long word */
    (APTR) &Col_Col_GreenPenSInfo, /* SpecialInfo structure */
    GADGREENPEN,                   /* user-definable data */
    NULL                           /* pointer to user-definable data */
};

struct PropInfo Col_Col_BluePenSInfo = {
    AUTOKNOB + FREEHORIZ, /* PropInfo flags */
    0, 0,                 /* horizontal and vertical pot values */
    -1, -1,               /* horizontal and vertical body values */
};

struct Image Col_Image3 = {
    0,      0,      /* XY origin relative to container TopLeft */
    263,    7,      /* Image width and height in pixels */
    0,              /* number of bitplanes in Image */
    NULL,           /* pointer to ImageData */
    0x0000, 0x0000, /* PlanePick and PlaneOnOff */
    NULL            /* next Image structure */
};

struct Gadget Col_BluePen = {
    &Col_GreenPen, /* next gadget */
    32, 36,        /* origin XY of hit box relative to window TopLeft */
    271, 11,       /* hit box width and height */
    NULL,          /* gadget flags */
    RELVERIFY + GADGIMMEDIATE + FOLLOWMOUSE, /* activation flags */
    PROPGADGET,                              /* gadget type flags */
    (APTR) &Col_Image3,           /* gadget border or image to be rendered */
    NULL,                         /* alternate imagery for selection */
    NULL,                         /* first IntuiText structure */
    NULL,                         /* gadget mutual-exclude long word */
    (APTR) &Col_Col_BluePenSInfo, /* SpecialInfo structure */
    GADBLUEPEN,                   /* user-definable data */
    NULL                          /* pointer to user-definable data */
};

#define Col_GadgetList1 Col_BluePen

struct IntuiText Col_IText6 = {
    3,    0,  JAM1, /* front and back text pens, drawmode and fill byte */
    17,   38,       /* XY origin relative to container TopLeft */
    NULL,           /* font pointer or NULL for default */
    "B",            /* pointer to text */
    NULL            /* next IntuiText structure */
};

struct IntuiText Col_IText5 = {
    4,          0,
    JAM1,           /* front and back text pens, drawmode and fill byte */
    16,         26, /* XY origin relative to container TopLeft */
    NULL,           /* font pointer or NULL for default */
    "G",            /* pointer to text */
    &Col_IText6     /* next IntuiText structure */
};

struct IntuiText Col_IText4 = {
    7,          0,
    JAM1,           /* front and back text pens, drawmode and fill byte */
    16,         14, /* XY origin relative to container TopLeft */
    NULL,           /* font pointer or NULL for default */
    "R",            /* pointer to text */
    &Col_IText5     /* next IntuiText structure */
};

#define Col_IntuiTextList1 Col_IText4

struct NewWindow Col_NewWindowStructure1 = {
    175, 45, /* window XY origin relative to TopLeft of screen */
    312, 93, /* window width and height */
    0, 1,    /* detail and block pens */
    MOUSEBUTTONS + MOUSEMOVE + GADGETDOWN + GADGETUP + CLOSEWINDOW
        + VANILLAKEY + INTUITICKS, /* IDCMP flags */
    WINDOWDRAG + WINDOWDEPTH + WINDOWCLOSE + ACTIVATE
        + NOCAREREFRESH,  /* other window flags */
    &Col_BluePen,         /* first gadget in gadget list */
    NULL,                 /* custom CHECKMARK imagery */
    "Edit Screen Colors", /* window title */
    NULL,                 /* custom screen pointer */
    NULL,                 /* custom bitmap */
    5,
    5,           /* minimum width and height */
    -1, -1,      /* maximum width and height */
    CUSTOMSCREEN /* destination screen type */
};

/* end of PowerWindows source generation */
