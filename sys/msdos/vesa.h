/* NetHack 3.6	vesa.h	$NHDT-Date: 1507161296 2017/10/04 23:54:56 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.0 $ */

/* VESA structures from the VESA BIOS Specification, retrieved 15 Jan 2016
 * from http://flint.cs.yale.edu/cs422/readings/hardware/vbe3.pdf
 * Part of rchason vesa port */

#ifndef VESA_H
#define VESA_H

#include <stdint.h>

struct VbeInfoBlock {
    uint8_t  VbeSignature[4];   /* VBE Signature */
    uint16_t VbeVersion;        /* VBE Version */
    uint32_t OemStringPtr;      /* VbeFarPtr to OEM String */
    uint32_t Capabilities;      /* Capabilities of graphics controller */
    uint32_t VideoModePtr;      /* VbeFarPtr to VideoModeList */
    uint16_t TotalMemory;       /* Number of 64kb memory blocks */
    /* Added for VBE 2.0+ */
    uint16_t OemSoftwareRev;    /* VBE implementation Software revision */
    uint32_t OemVendorNamePtr;  /* VbeFarPtr to Vendor Name String */
    uint32_t OemProductNamePtr; /* VbeFarPtr to Product Name String */
    uint32_t OemProductRevPtr;  /* VbeFarPtr to Product Revision String */
    uint8_t  Reserved[222];     /* Reserved for VBE implementation scratch area */
    uint8_t  OemData[256];      /* Data Area for OEM Strings */
} __attribute__((packed));

struct ModeInfoBlock {
    /* Mandatory information for all VBE revisions */
    uint16_t ModeAttributes;        /* mode attributes */
    uint8_t  WinAAttributes;        /* window A attributes */
    uint8_t  WinBAttributes;        /* window B attributes */
    uint16_t WinGranularity;        /* window granularity */
    uint16_t WinSize;               /* window size */
    uint16_t WinASegment;           /* window A start segment */
    uint16_t WinBSegment;           /* window B start segment */
    uint32_t WinFuncPtr;            /* real mode pointer to window function */
    uint16_t BytesPerScanLine;      /* bytes per scan line */
    /* Mandatory information for VBE 1.2 and above */
    uint16_t XResolution;           /* horizontal resolution in pixels or characters */
    uint16_t YResolution;           /* vertical resolution in pixels or characters */
    uint8_t  XCharSize;             /* character cell width in pixels */
    uint8_t  YCharSize;             /* character cell height in pixels */
    uint8_t  NumberOfPlanes;        /* number of memory planes */
    uint8_t  BitsPerPixel;          /* bits per pixel */
    uint8_t  NumberOfBanks;         /* number of banks */
    uint8_t  MemoryModel;           /* memory model type */
    uint8_t  BankSize;              /* bank size in KB */
    uint8_t  NumberOfImagePages;    /* number of images */
    uint8_t  Reserved1;             /* reserved for page function */
    /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
    uint8_t  RedMaskSize;           /* size of direct color red mask in bits */
    uint8_t  RedFieldPosition;      /* bit position of lsb of red mask */
    uint8_t  GreenMaskSize;         /* size of direct color green mask in bits */
    uint8_t  GreenFieldPosition;    /* bit position of lsb of green mask */
    uint8_t  BlueMaskSize;          /* size of direct color blue mask in bits */
    uint8_t  BlueFieldPosition;     /* bit position of lsb of blue mask */
    uint8_t  RsvdMaskSize;          /* size of direct color reserved mask in bits */
    uint8_t  RsvdFieldPosition;     /* bit position of lsb of reserved mask */
    uint8_t  DirectColorModeInfo;   /* direct color mode attributes */
    /* Mandatory information for VBE 2.0 and above */
    uint32_t PhysBasePtr;           /* physical address for flat memory frame buffer */
    uint32_t Reserved2;             /* Reserved - always set to 0 */
    uint16_t Reserved3;             /* Reserved - always set to 0 */
    /* Mandatory information for VBE 3.0 and above */
    uint16_t LinBytesPerScanLine;   /* bytes per scan line for linear modes */
    uint8_t  BnkNumberOfImagePages; /* number of images for banked modes */
    uint8_t  LinNumberOfImagePages; /* number of images for linear modes */
    uint8_t  LinRedMaskSize;        /* size of direct color red mask (linear modes) */
    uint8_t  LinRedFieldPosition;   /* bit position of lsb of red mask (linear modes) */
    uint8_t  LinGreenMaskSize;      /* size of direct color green mask  (linear modes) */
    uint8_t  LinGreenFieldPosition; /* bit position of lsb of green mask (linear modes) */
    uint8_t  LinBlueMaskSize;       /* size of direct color blue mask  (linear modes) */
    uint8_t  LinBlueFieldPosition;  /* bit position of lsb of blue mask (linear modes) */
    uint8_t  LinRsvdMaskSize;       /* size of direct color reserved mask (linear modes) */
    uint8_t  LinRsvdFieldPosition;  /* bit position of lsb of reserved mask (linear modes) */
    uint32_t MaxPixelClock;         /* maximum pixel clock (in Hz) for graphics mode */
    uint8_t  Reserved4[189];        /* remainder of ModeInfoBlock */
} __attribute__((packed));

#endif
