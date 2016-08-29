/*
 * Copyright 2016 Kevin Brace
 * Copyright 2005-2016 The OpenChrome Project
 *                     [http://www.freedesktop.org/wiki/Openchrome]
 * Copyright 2004-2005 The Unichrome Project  [unichrome.sf.net]
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * via_outputs.c
 *
 * Everything to do with setting and changing xf86Outputs.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "via_driver.h"
#include <unistd.h>

/*
 * Modetable nonsense.
 *
 */
#include "via_mode.h"

/*
 * Sets IGA1 or IGA2 as the display output source for DIP1
 * (Digital Interface Port 1) interface for CLE266 only.
 */
void
viaDIP1SetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 temp = displaySource;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaDIP1SetDisplaySource.\n"));

    /* Set DIP1 display output source. */
    /* 3X5.93[7] - DIP1 (Digital Interface Port 1) Data Source Selection
     *             0: Primary Display (IGA1)
     *             1: Secondary Display (IGA2) */
    ViaCrtcMask(hwp, 0x93, temp << 7, 0x80);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "DIP1 Display Output Source: IGA%d\n",
                (temp & 0x01) + 1);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaDIP1SetDisplaySource.\n"));
}

/*
 * Sets IGA1 or IGA2 as the display output source for DVP0
 * (Digital Video Port) interface.
 */
void
viaDVP0SetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 temp = displaySource;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaDVP0SetDisplaySource.\n"));

    /* Set DVP0 display output source. */
    /* 3X5.96[4] - DVP0 Data Source Selection
     *             0: Primary Display
     *             1: Secondary Display */
    ViaCrtcMask(hwp, 0x96, temp << 4, 0x10);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "DVP0 Display Output Source: IGA%d\n",
                (temp & 0x01) + 1);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaDVP0SetDisplaySource.\n"));
}

/*
 * Sets IGA1 or IGA2 as the display output source for DVP1
 * (Digital Video Port) interface.
 */
void
viaDVP1SetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 temp = displaySource;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaDVP1SetDisplaySource.\n"));

    /* Set DVP1 display output source. */
    /* 3X5.9B[4] - DVP1 Data Source Selection
     *             0: Primary Display
     *             1: Secondary Display */
    ViaCrtcMask(hwp, 0x9B, temp << 4, 0x10);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "DVP1 Display Output Source: IGA%d\n",
                (temp & 0x01) + 1);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaDVP1SetDisplaySource.\n"));
}

/*
 * Sets IGA1 or IGA2 as the display output source for VIA Technologies
 * Chrome IGP DFP (Digital Flat Panel) Low interface.
 */
void
viaDFPLowSetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 temp = displaySource;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaDFPLowSetDisplaySource.\n"));

    /* Set DFP Low display output source. */
    /* 3X5.99[4] - DFP Low Data Source Selection
     *             0: Primary Display
     *             1: Secondary Display */
    ViaCrtcMask(hwp, 0x99, temp << 4, 0x10);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "DFP Low Display Output Source: IGA%d\n",
                (temp & 0x01) + 1);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaDFPLowSetDisplaySource.\n"));
}

/*
 * Reads off the VIA Technologies IGP pin strapping for
 * display detection purposes.
 */
void
viaProbePinStrapping(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13, sr5a;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaProbePinStrapping.\n"));

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "Probing VIA Technologies IGP pin strapping . . .\n");

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                    "Setting 3C5.5A[0] to 0.\n");
        ViaSeqMask(hwp, 0x5A, sr5a & 0xFE, 0x01);
    }

    sr12 = hwp->readSeq(hwp, 0x12);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR12: 0x%02X\n", sr12));
    sr13 = hwp->readSeq(hwp, 0x13);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "SR13: 0x%02X\n", sr13));

    switch (pVia->Chipset) {
    case VIA_CLE266:

        /* 3C5.12[4] - FPD17 pin strapping
         *             0: TMDS transmitter (DVI) / capture device
         *             1: Flat panel */
        if (sr12 & 0x10) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "A flat panel is connected to "
                        "flat panel interface.\n");

            /* 3C5.12[3:0] - FPD16-13 pin strapping
             *               0 ~ 15: Flat panel code defined
             *                       by VIA Technologies */
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Detected Flat Panel Type from "
                        "Strapping Pins: %d\n", sr12 & 0x0F);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "A TMDS transmitter (DVI) / capture device is "
                        "connected to flat panel interface.\n");
        }

        /* 3C5.12[5] - FPD18 pin strapping
         *             0: TMDS transmitter (DVI)
         *             1: TV encoder */
        if (sr12 & 0x20) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "A TMDS transmitter (DVI) is connected to "
                        "DVI port.\n");
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "A TV encoder is connected to "
                        "DVI port.\n");

            /* 3C5.13[4:3] - FPD21-20 pin strapping
             *               00: PAL
             *               01: NTSC
             *               10: PAL-N
             *               11: PAL-NC */
            if (sr13 & 0x04) {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "NTSC for the TV encoder.\n");
            } else {
                if (!(sr13 & 0x08)) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "PAL for the TV encoder.\n");
                } else {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "PAL%s for the TV encoder.\n",
                                sr13 & 0x04 ? "-NC" : "-N");
                }
            }

            /* 3C5.12[6] - FPD19 pin strapping
             *             0: 525 lines (NTSC)
             *             1: 625 lines (PAL) */
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "%s lines for the TV encoder.\n",
                        sr12 & 0x40 ? "625" : "525");
        }

        break;

    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:

        /* 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0) for
         *                DVI or TV out use
         *             1: Enable DVP0 (Digital Video Port 0) for
         *                DVI or TV out use */
        if (sr12 & 0x40) {

            /* 3C5.12[5] - DVP0D5 pin strapping
             *             0: TMDS transmitter (DVI)
             *             1: TV encoder */
            if (sr12 & 0x20) {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "A TV encoder is detected on "
                            "DVP0 (Digital Video Port 0).\n");
            } else {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "A TMDS transmitter (DVI) is detected on "
                            "DVP0 (Digital Video Port 0).\n");
            }
        }


        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *             (Flat Panel Display Port) */
        if (sr13 & 0x08) {

            /* 3C5.12[4] - DVP0D4 pin strapping
             *             0: Dual 12-bit FPDP (Flat Panel Display Port)
             *             1: 24-bit FPDP  (Flat Panel Display Port) */
            if (sr12 & 0x10) {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "24-bit FPDP (Flat Panel Display Port) "
                            "detected.\n");

                /* 3C5.12[3:0] - DVP0D3-0 pin strapping
                 *               0 ~ 15: Flat panel code defined
                 *                       by VIA Technologies */
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Detected Flat Panel Type from "
                            "Strapping Pins: %d\n", sr12 & 0x0F);
            } else {

                /* 3C5.12[6] - DVP0D6 pin strapping
                 *             0: Disable DVP0 (Digital Video Port 0) for
                 *                DVI or TV out use
                 *             1: Enable DVP0 (Digital Video Port 0) for
                 *                DVI or TV out use
                 * 3C5.12[5] - DVP0D5 pin strapping
                 *             0: TMDS transmitter (DVI)
                 *             1: TV encoder */
                if ((!(sr12 & 0x40)) && (!(sr12 & 0x20))) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "A TV encoder is connected to "
                                "FPDP (Flat Panel Display Port).\n");
                } else {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "Dual 12-bit FPDP (Flat Panel Display Port) "
                                "detected.\n");

                    /* 3C5.12[3:0] - DVP0D3-0 pin strapping
                     *               0 ~ 15: Flat panel code defined
                     *                       by VIA Technologies */
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "Detected Flat Panel Type from "
                                "Strapping Pins: %d\n", sr12 & 0x0F);
                }
            }
        }

        break;

    default:
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                    "Setting 3C5.5A[0] to 1.\n");
        ViaSeqMask(hwp, 0x5A, sr5a | 0x01, 0x01);

        sr12 = hwp->readSeq(hwp, 0x12);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR12: 0x%02X\n", sr12));
        sr13 = hwp->readSeq(hwp, 0x13);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR13: 0x%02X\n", sr13));

        /* 3C5.13[7:6] - Integrated LVDS / DVI Mode Select
         *               (DVP1D15-14 pin strapping)
         *               00: LVDS1 + LVDS2
         *               01: DVI + LVDS2
         *               10: Dual LVDS Channel (High Resolution Panel)
         *               11: One DVI only (decrease the clock jitter) */
        switch (sr13 & 0xC0) {
        case 0x00:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "LVDS1 + LVDS2 detected.\n");
            break;
        case 0x40:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Single Link DVI + LVDS2 detected.\n");
            break;
        case 0x80:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Dual Channel LVDS detected.\n");
            break;
        case 0xC0:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Single Link DVI detected.\n");
            break;
        default:
            break;
        }

        hwp->writeSeq(hwp, 0x5A, sr5a);

    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaProbePinStrapping.\n"));
}

void
viaOutputDetect(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr pBIOSInfo = pVia->pBIOSInfo;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaOutputDetect.\n"));

    pBIOSInfo->analog = NULL;

    /* Initialize the number of VGA connectors. */
    pVia->numberVGA = 0;

    /* Initialize the number of DVI connectors. */
    pVia->numberDVI = 0;

    /* Initialize the number of FP connectors. */
    pVia->numberFP = 0;

    /* Read off the VIA Technologies IGP pin strapping for
       display detection purposes. */
    viaProbePinStrapping(pScrn);

    /* VGA */
    via_analog_init(pScrn);

    /* TV */
    via_tv_init(pScrn);

    /* DVI */
    via_dvi_init(pScrn);

    /* LVDS */
    via_lvds_init(pScrn);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaOutputDetect.\n"));
}

#ifdef HAVE_DEBUG
/*
 * Returns:
 *   Bit[7] 2nd Path
 *   Bit[6] 1/0 MHS Enable/Disable
 *   Bit[5] 0 = Bypass Callback, 1 = Enable Callback
 *   Bit[4] 0 = Hot-Key Sequence Control (OEM Specific)
 *   Bit[3] LCD
 *   Bit[2] TV
 *   Bit[1] CRT
 *   Bit[0] DVI
 */
static CARD8
VIAGetActiveDisplay(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 tmp;

    tmp = (hwp->readCrtc(hwp, 0x3E) >> 4);
    tmp |= ((hwp->readCrtc(hwp, 0x3B) & 0x18) << 3);

    return tmp;
}
#endif /* HAVE_DEBUG */

/*
 *
 */
CARD32
ViaGetMemoryBandwidth(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                     "ViaGetMemoryBandwidth. Memory type: %d\n",
                     pVia->MemClk));

    switch (pVia->MemClk) {
        case VIA_MEM_SDR66:
        case VIA_MEM_SDR100:
        case VIA_MEM_SDR133:
            return VIA_BW_MIN;
        case VIA_MEM_DDR200:
            return VIA_BW_DDR200;
        case VIA_MEM_DDR266:
        case VIA_MEM_DDR333:
        case VIA_MEM_DDR400:
            return VIA_BW_DDR400;
        case VIA_MEM_DDR533:
        case VIA_MEM_DDR667:
            return VIA_BW_DDR667;
        case VIA_MEM_DDR800:
        case VIA_MEM_DDR1066:
            return VIA_BW_DDR1066;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "ViaBandwidthAllowed: Unknown memory type: %d\n",
                       pVia->MemClk);
            return VIA_BW_MIN;
    }
}

/*
 *
 * Some very common abstractions.
 *
 */

/*
 * Standard vga call really.
 * Needs to be called to reset the dotclock (after SR40:2/1 reset)
 */
void
ViaSetUseExternalClock(vgaHWPtr hwp)
{
    CARD8 data;

    DEBUG(xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO,
                     "ViaSetUseExternalClock\n"));

    data = hwp->readMiscOut(hwp);
    hwp->writeMiscOut(hwp, data | 0x0C);
}

/*
 *
 */
static void
ViaSetDotclock(ScrnInfoPtr pScrn, CARD32 clock, int base, int probase)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);

    DEBUG(xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO,
                     "ViaSetDotclock to 0x%06x\n", (unsigned)clock));

    if ((pVia->Chipset == VIA_CLE266) || (pVia->Chipset == VIA_KM400)) {
        hwp->writeSeq(hwp, base, clock >> 8);
        hwp->writeSeq(hwp, base+1, clock & 0xFF);
    } else {  /* unichrome pro */
        union pllparams pll;
        int dtz, dr, dn, dm;
        pll.packed = clock;
        dtz = pll.params.dtz;
        dr  = pll.params.dr;
        dn  = pll.params.dn;
        dm  = pll.params.dm;

        /* The VX855 and VX900 do not modify dm/dn, but earlier chipsets do. */
        if ((pVia->Chipset != VIA_VX855) && (pVia->Chipset != VIA_VX900)) {
            dm -= 2;
            dn -= 2;
        }

        hwp->writeSeq(hwp, probase, dm & 0xff);
        hwp->writeSeq(hwp, probase+1,
                      ((dm >> 8) & 0x03) | (dr << 2) | ((dtz & 1) << 7));
        hwp->writeSeq(hwp, probase+2, (dn & 0x7f) | ((dtz & 2) << 6));
    }
}

/*
 *
 */
void
ViaSetPrimaryDotclock(ScrnInfoPtr pScrn, CARD32 clock)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    ViaSetDotclock(pScrn, clock, 0x46, 0x44);

    ViaSeqMask(hwp, 0x40, 0x02, 0x02);
    ViaSeqMask(hwp, 0x40, 0x00, 0x02);
}

/*
 *
 */
void
ViaSetSecondaryDotclock(ScrnInfoPtr pScrn, CARD32 clock)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    ViaSetDotclock(pScrn, clock, 0x44, 0x4A);

    ViaSeqMask(hwp, 0x40, 0x04, 0x04);
    ViaSeqMask(hwp, 0x40, 0x00, 0x04);
}

/*
 *
 */
static void
ViaSetECKDotclock(ScrnInfoPtr pScrn, CARD32 clock)
{
    /* Does the non-pro chip have an ECK clock ? */
    ViaSetDotclock(pScrn, clock, 0, 0x47);
}

static CARD32
ViaComputeDotClock(unsigned clock)
{
    double fout, fref, err, minErr;
    CARD32 dr, dn, dm, maxdm, maxdn;
    CARD32 factual, best;

    fref = 14.31818e6;
    fout = (double)clock * 1.e3;

    factual = ~0;
    maxdm = 127;
    maxdn = 7;
    minErr = 1e10;
    best = 0;

    for (dr = 0; dr < 4; ++dr) {
        for (dn = (dr == 0) ? 2 : 1; dn <= maxdn; ++dn) {
            for (dm = 1; dm <= maxdm; ++dm) {
                factual = fref * dm;
                factual /= (dn << dr);
                err = fabs((double)factual / fout - 1.);
                if (err < minErr) {
                    minErr = err;
                    best = (dm & 127) | ((dn & 31) << 8) | (dr << 14);
                }
            }
        }
    }
    return best;
}

static CARD32
ViaComputeProDotClock(unsigned clock)
{
    double fvco, fout, err, minErr;
    CARD32 dr = 0, dn, dm, maxdm, maxdn;
    CARD32 factual;
    union pllparams bestClock;

    fout = (double)clock * 1.e3;

    factual = ~0;
    maxdm = factual / 14318000U;
    minErr = 1.e10;
    bestClock.packed = 0U;

    do {
        fvco = fout * (1 << dr);
    } while (fvco < 300.e6 && dr++ < 8);

    if (dr == 8) {
        return 0;
    }

    if (clock < 30000)
        maxdn = 8;
    else if (clock < 45000)
        maxdn = 7;
    else if (clock < 170000)
        maxdn = 6;
    else
        maxdn = 5;

    for (dn = 2; dn < maxdn; ++dn) {
        for (dm = 2; dm < maxdm; ++dm) {
            factual = 14318000U * dm;
            factual /= dn << dr;
            if ((err = fabs((double)factual / fout - 1.)) < 0.005) {
                if (err < minErr) {
                    minErr = err;
                    bestClock.params.dtz = 1;
                    bestClock.params.dr = dr;
                    bestClock.params.dn = dn;
                    bestClock.params.dm = dm;
                }
            }
        }
    }

    return bestClock.packed;
}

/*
 *
 */
CARD32
ViaModeDotClockTranslate(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    VIAPtr pVia = VIAPTR(pScrn);
    int i;

    if ((pVia->Chipset == VIA_CLE266) || (pVia->Chipset == VIA_KM400)) {
        CARD32 best1 = 0, best2;

        for (i = 0; ViaDotClocks[i].DotClock; i++)
            if (ViaDotClocks[i].DotClock == mode->Clock) {
                best1 = ViaDotClocks[i].UniChrome;
                break;
            }

        best2 = ViaComputeDotClock(mode->Clock);

        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                         "ViaComputeDotClock %d : %04x : %04x\n",
                         mode->Clock, (unsigned int)best1,
                         (unsigned int)best2));

        return best2;
    } else {
        for (i = 0; ViaDotClocks[i].DotClock; i++)
            if (ViaDotClocks[i].DotClock == mode->Clock)
                return ViaDotClocks[i].UniChromePro.packed;
        return ViaComputeProDotClock(mode->Clock);
    }

    return 0;
}
