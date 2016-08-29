/*
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
 * via_tv.c
 *
 * Handles the initialization and management of TV output related
 * resources.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "via_driver.h"
#include <unistd.h>

/*
 *
 * TV specific code.
 *
 */
static void
ViaDisplayEnableDVO(ScrnInfoPtr pScrn, int port)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaDisplayEnableDVO, port: %d\n",
                     port));
    switch (port) {
    case VIA_DI_PORT_DVP0:
        ViaSeqMask(hwp, 0x1E, 0xC0, 0xC0);
        break;
    case VIA_DI_PORT_DVP1:
        ViaSeqMask(hwp, 0x1E, 0x30, 0x30);
        break;
    }
}

static void
viaTVSetDisplaySource(ScrnInfoPtr pScrn, CARD8 displaySource)
{

    vgaHWPtr hwp = VGAHWPTR(pScrn);
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 sr12, sr13, sr5a;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered viaTVSetDisplaySource.\n"));

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        sr5a = hwp->readSeq(hwp, 0x5A);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "SR5A: 0x%02X\n", sr5a));
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Setting 3C5.5A[0] to 0.\n"));
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
        /* 3C5.12[5] - FPD18 pin strapping
         *             0: DIP1 (Digital Interface Port 1) is used by
         *                a TMDS transmitter (DVI)
         *             1: DIP1 (Digital Interface Port 1) is used by
         *                a TV encoder */
        if (sr12 & 0x20) {
            viaDIP1SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DIP1 was not set up for "
                        "an external TV encoder use.\n");
        }

        break;
    case VIA_KM400:
    case VIA_K8M800:
    case VIA_PM800:
    case VIA_P4M800PRO:
        /* 3C5.13[3] - DVP0D8 pin strapping
         *             0: AGP pins are used for AGP
         *             1: AGP pins are used by FPDP
         *                (Flat Panel Display Port)
         * 3C5.12[6] - DVP0D6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - DVP0D5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - DVP0D4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP  (Flat Panel Display Port) */
        if ((sr12 & 0x40) && (sr12 & 0x20)) {
            viaDVP0SetDisplaySource(pScrn, displaySource);
        } else if ((sr13 & 0x08) && (~(sr12 & 0x10))) {
            viaDFPLowSetDisplaySource(pScrn, displaySource);
        } else if (sr13 & 0x08) {
            viaDVP1SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "None of the external ports were set up for "
                        "external TV encoder use.\n");
        }

        break;
    case VIA_P4M890:
    case VIA_K8M890:
    case VIA_P4M900:
        /* 3C5.12[6] - FPD6 pin strapping
         *             0: Disable DVP0 (Digital Video Port 0)
         *             1: Enable DVP0 (Digital Video Port 0)
         * 3C5.12[5] - FPD5 pin strapping
         *             0: DVP0 is used by a TMDS transmitter (DVI)
         *             1: DVP0 is used by a TV encoder
         * 3C5.12[4] - FPD4 pin strapping
         *             0: Dual 12-bit FPDP (Flat Panel Display Port)
         *             1: 24-bit FPDP  (Flat Panel Display Port) */
        if ((sr12 & 0x40) & (sr12 & 0x20) &(~(sr12 & 0x10))) {
            viaDVP0SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Unrecognized external TV encoder use.\n"
                        "Contact the developer for assistance.\n");
        }

        break;
    case VIA_CX700:
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        /* 3C5.13[6] - DVP1 DVP / capture port selection
         *             0: DVP1 is used as a DVP (Digital Video Port)
         *             1: DVP1 is used as a capture port
         */
        if (~(sr13 & 0x40)) {
            viaDVP1SetDisplaySource(pScrn, displaySource);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "DVP1 is not set up for external TV "
                        "encoder use.\n");
        }

        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Unrecognized IGP for "
                    "an external TV encoder use.\n");
        break;
    }

    if ((pVia->Chipset == VIA_CX700)
        || (pVia->Chipset == VIA_VX800)
        || (pVia->Chipset == VIA_VX855)
        || (pVia->Chipset == VIA_VX900)) {

        hwp->writeSeq(hwp, 0x5A, sr5a);
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Restoring 3C5.5A[0].\n"));
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting viaTVSetDisplaySource.\n"));
}

static void
ViaTVSave(ScrnInfoPtr pScrn)
{
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

    if (pBIOSInfo->TVSave)
        pBIOSInfo->TVSave(pScrn);
}

static void
ViaTVRestore(ScrnInfoPtr pScrn)
{
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

    if (pBIOSInfo->TVRestore)
        pBIOSInfo->TVRestore(pScrn);
}

static Bool
ViaTVDACSense(ScrnInfoPtr pScrn)
{
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

    if (pBIOSInfo->TVDACSense)
        return pBIOSInfo->TVDACSense(pScrn);
    return FALSE;
}

static void
ViaTVSetMode(xf86CrtcPtr crtc, DisplayModePtr mode)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    VIAPtr pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr pBIOSInfo = pVia->pBIOSInfo;

    if (pBIOSInfo->TVModeI2C)
        pBIOSInfo->TVModeI2C(pScrn, mode);

    if (pBIOSInfo->TVModeCrtc)
        pBIOSInfo->TVModeCrtc(crtc, mode);

    /* TV reset. */
    xf86I2CWriteByte(pBIOSInfo->TVI2CDev, 0x1D, 0x00);
    xf86I2CWriteByte(pBIOSInfo->TVI2CDev, 0x1D, 0x80);
}

static void
ViaTVPower(ScrnInfoPtr pScrn, Bool On)
{
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

#ifdef HAVE_DEBUG
    if (On)
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaTVPower: On.\n");
    else
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaTVPower: Off.\n");
#endif

    if (pBIOSInfo->TVPower)
        pBIOSInfo->TVPower(pScrn, On);
}

#ifdef HAVE_DEBUG
void
ViaTVPrintRegs(ScrnInfoPtr pScrn)
{
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

    if (pBIOSInfo->TVPrintRegs)
        pBIOSInfo->TVPrintRegs(pScrn);
}
#endif /* HAVE_DEBUG */

static void
via_tv_create_resources(xf86OutputPtr output)
{
}

#ifdef RANDR_12_INTERFACE
static Bool
via_tv_set_property(xf86OutputPtr output, Atom property,
                    RRPropertyValuePtr value)
{
    return TRUE;
}

static Bool
via_tv_get_property(xf86OutputPtr output, Atom property)
{
    return FALSE;
}
#endif

static void
via_tv_dpms(xf86OutputPtr output, int mode)
{
    ScrnInfoPtr pScrn = output->scrn;

    switch (mode) {
    case DPMSModeOn:
        ViaTVPower(pScrn, TRUE);
        break;

    case DPMSModeStandby:
    case DPMSModeSuspend:
    case DPMSModeOff:
        ViaTVPower(pScrn, FALSE);
        break;
    }
}

static void
via_tv_save(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn = output->scrn;

    ViaTVSave(pScrn);
}

static void
via_tv_restore(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn = output->scrn;

    ViaTVRestore(pScrn);
}

static int
via_tv_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    ScrnInfoPtr pScrn = output->scrn;
    int ret = MODE_OK;

    if (!ViaModeDotClockTranslate(pScrn, pMode))
        return MODE_NOCLOCK;

    return ret;
}

static Bool
via_tv_mode_fixup(xf86OutputPtr output, DisplayModePtr mode,
                  DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
via_tv_prepare(xf86OutputPtr output)
{
    via_tv_dpms(output, DPMSModeOff);
}

static void
via_tv_commit(xf86OutputPtr output)
{
    via_tv_dpms(output, DPMSModeOn);
}

static void
via_tv_mode_set(xf86OutputPtr output, DisplayModePtr mode,
                DisplayModePtr adjusted_mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    drmmode_crtc_private_ptr iga = output->crtc->driver_private;
    VIAPtr pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr pBIOSInfo = VIAPTR(pScrn)->pBIOSInfo;

    /* TV on FirstCrtc */
    if (output->crtc) {
        viaTVSetDisplaySource(pScrn, iga->index ? 0x01 : 0x00);
        ViaDisplayEnableDVO(pScrn, pBIOSInfo->TVDIPort);
        ViaTVSetMode(output->crtc, adjusted_mode);
    }

    pVia->FirstInit = FALSE;
}

static xf86OutputStatus
via_tv_detect(xf86OutputPtr output)
{
    xf86OutputStatus status = XF86OutputStatusDisconnected;
    ScrnInfoPtr pScrn = output->scrn;

    if (ViaTVDACSense(pScrn))
        status = XF86OutputStatusConnected;
    return status;
}

static DisplayModePtr
via_tv_get_modes(xf86OutputPtr output)
{
    DisplayModePtr modes = NULL, mode = NULL;
    ScrnInfoPtr pScrn = output->scrn;
    VIAPtr pVia = VIAPTR(pScrn);
    int i;

    for (i = 0; i < pVia->pBIOSInfo->TVNumModes; i++) {
        mode = xf86DuplicateMode(&pVia->pBIOSInfo->TVModes[i]);
        modes = xf86ModesAdd(modes, mode);
    }
    return modes;
}

static void
via_tv_destroy(xf86OutputPtr output)
{
}

static const xf86OutputFuncsRec via_tv_funcs = {
    .create_resources   = via_tv_create_resources,
#ifdef RANDR_12_INTERFACE
    .set_property       = via_tv_set_property,
#endif
#ifdef RANDR_13_INTERFACE
    .get_property       = via_tv_get_property,
#endif
    .dpms               = via_tv_dpms,
    .save               = via_tv_save,
    .restore            = via_tv_restore,
    .mode_valid         = via_tv_mode_valid,
    .mode_fixup         = via_tv_mode_fixup,
    .prepare            = via_tv_prepare,
    .commit             = via_tv_commit,
    .mode_set           = via_tv_mode_set,
    .detect             = via_tv_detect,
    .get_modes          = via_tv_get_modes,
    .destroy            = via_tv_destroy,
};

/*
 *
 */
Bool
via_tv_init(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr pBIOSInfo = pVia->pBIOSInfo;
    xf86OutputPtr output = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Entered via_tv_init.\n"));

    /* preset some pBIOSInfo TV related values -- move up */
    pBIOSInfo->TVEncoder = VIA_NONETV;
    pBIOSInfo->TVI2CDev = NULL;
    pBIOSInfo->TVSave = NULL;
    pBIOSInfo->TVRestore = NULL;
    pBIOSInfo->TVDACSense = NULL;
    pBIOSInfo->TVModeValid = NULL;
    pBIOSInfo->TVModeI2C = NULL;
    pBIOSInfo->TVModeCrtc = NULL;
    pBIOSInfo->TVPower = NULL;
    pBIOSInfo->TVModes = NULL;
    pBIOSInfo->TVPrintRegs = NULL;
    pBIOSInfo->LCDPower = NULL;
    pBIOSInfo->TVNumRegs = 0;

    /*
     * On an SK43G (KM400/Ch7011), false positive detections at a VT162x
     * chip were observed, so try to detect the Ch7011 first.
     */
    if (pVia->pI2CBus2 && xf86I2CProbeAddress(pVia->pI2CBus2, 0xEC))
        pBIOSInfo->TVI2CDev = ViaCH7xxxDetect(pScrn, pVia->pI2CBus2, 0xEC);
    else if (pVia->pI2CBus2 && xf86I2CProbeAddress(pVia->pI2CBus2, 0x40))
        pBIOSInfo->TVI2CDev = ViaVT162xDetect(pScrn, pVia->pI2CBus2, 0x40);
    else if (pVia->pI2CBus3 && xf86I2CProbeAddress(pVia->pI2CBus3, 0x40))
        pBIOSInfo->TVI2CDev = ViaVT162xDetect(pScrn, pVia->pI2CBus3, 0x40);
    else if (pVia->pI2CBus2 && xf86I2CProbeAddress(pVia->pI2CBus2, 0xEA))
        pBIOSInfo->TVI2CDev = ViaCH7xxxDetect(pScrn, pVia->pI2CBus2, 0xEA);
    else if (pVia->pI2CBus3 && xf86I2CProbeAddress(pVia->pI2CBus3, 0xEA))
        pBIOSInfo->TVI2CDev = ViaCH7xxxDetect(pScrn, pVia->pI2CBus3, 0xEA);

    if (!pBIOSInfo->TVI2CDev) {
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "Did not detect a TV encoder.\n");
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Exiting via_tv_init.\n"));

        return FALSE;
    }

    switch (pBIOSInfo->TVEncoder) {
        case VIA_VT1621:
        case VIA_VT1622:
        case VIA_VT1623:
        case VIA_VT1625:
            ViaVT162xInit(pScrn);
            break;
        case VIA_CH7011:
        case VIA_CH7019A:
        case VIA_CH7019B:
            ViaCH7xxxInit(pScrn);
            break;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Was not able to initialize a known TV encoder.\n");
            DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                                "Exiting via_tv_init.\n"));
            return FALSE;
            break;
    }

    if (!pBIOSInfo->TVSave || !pBIOSInfo->TVRestore
        || !pBIOSInfo->TVDACSense || !pBIOSInfo->TVModeValid
        || !pBIOSInfo->TVModeI2C || !pBIOSInfo->TVModeCrtc
        || !pBIOSInfo->TVPower || !pBIOSInfo->TVModes
        || !pBIOSInfo->TVPrintRegs) {

        xf86DestroyI2CDevRec(pBIOSInfo->TVI2CDev, TRUE);

        pBIOSInfo->TVI2CDev = NULL;
        pBIOSInfo->TVOutput = TVOUTPUT_NONE;
        pBIOSInfo->TVEncoder = VIA_NONETV;
        pBIOSInfo->TVI2CDev = NULL;
        pBIOSInfo->TVSave = NULL;
        pBIOSInfo->TVRestore = NULL;
        pBIOSInfo->TVDACSense = NULL;
        pBIOSInfo->TVModeValid = NULL;
        pBIOSInfo->TVModeI2C = NULL;
        pBIOSInfo->TVModeCrtc = NULL;
        pBIOSInfo->TVPower = NULL;
        pBIOSInfo->TVModes = NULL;
        pBIOSInfo->TVPrintRegs = NULL;
        pBIOSInfo->TVNumRegs = 0;

        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "TV encoder was not properly initialized.\n");
        DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                            "Exiting via_tv_init.\n"));
        return FALSE;
    }

    output = xf86OutputCreate(pScrn, &via_tv_funcs, "TV-1");
    pVia->FirstInit = TRUE;

    if (output) {
        /* Allow tv output on both crtcs, set bit 0 and 1. */
        output->possible_crtcs = 0x3;
    } else {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to register TV-1.\n");
    }

    pBIOSInfo->tv = output;
    /* Save now */
    pBIOSInfo->TVSave(pScrn);

#ifdef HAVE_DEBUG
    if (VIAPTR(pScrn)->PrintTVRegs)
        pBIOSInfo->TVPrintRegs(pScrn);
#endif

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Exiting via_tv_init.\n"));
    return TRUE;
}
