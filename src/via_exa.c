/*
 * Copyright 1998-2008 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 * Copyright 2006 Thomas Hellström. All Rights Reserved.
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
 * 2D acceleration functions for the VIA/S3G UniChrome IGPs.
 *
 * Mostly rewritten, and modified for EXA support, by Thomas Hellström.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xarch.h>
#include "xaalocal.h"
#include "xaarop.h"
#include "miline.h"

#include "via_driver.h"
#include "via_regs.h"
#include "via_dmabuffer.h"

#ifdef X_HAVE_XAAGETROP
#define VIAACCELPATTERNROP(vRop) (XAAGetPatternROP(vRop) << 24)
#define VIAACCELCOPYROP(vRop) (XAAGetCopyROP(vRop) << 24)
#else
#define VIAACCELPATTERNROP(vRop) (XAAPatternROP[vRop] << 24)
#define VIAACCELCOPYROP(vRop) (XAACopyROP[vRop] << 24)
#endif

enum VIA_2D_Regs {
	GECMD,
	GEMODE,
	GESTATUS,
	SRCPOS,
	DSTPOS,
	LINE_K1K2,
	LINE_XY,
	LINE_ERROR,
	DIMENSION,
	PATADDR,
	FGCOLOR,
	DSTCOLORKEY,
	BGCOLOR,
	SRCCOLORKEY,
	CLIPTL,
	CLIPBR,
	OFFSET,
	KEYCONTROL,
	SRCBASE,
	DSTBASE,
	PITCH,
	MONOPAT0,
	MONOPAT1,
	COLORPAT,
	MONOPATFGC,
	MONOPATBGC
};

/* register offsets for old 2D core */
static const unsigned via_2d_regs[] = {
    [GECMD]             = VIA_REG_GECMD,
    [GEMODE]            = VIA_REG_GEMODE,
    [GESTATUS]          = VIA_REG_GESTATUS,
    [SRCPOS]            = VIA_REG_SRCPOS,
    [DSTPOS]            = VIA_REG_DSTPOS,
    [LINE_K1K2]         = VIA_REG_LINE_K1K2,
    [LINE_XY]           = VIA_REG_LINE_XY,
    [LINE_ERROR]        = VIA_REG_LINE_ERROR,
    [DIMENSION]         = VIA_REG_DIMENSION,
    [PATADDR]           = VIA_REG_PATADDR,
    [FGCOLOR]           = VIA_REG_FGCOLOR,
    [DSTCOLORKEY]       = VIA_REG_DSTCOLORKEY,
    [BGCOLOR]           = VIA_REG_BGCOLOR,
    [SRCCOLORKEY]       = VIA_REG_SRCCOLORKEY,
    [CLIPTL]            = VIA_REG_CLIPTL,
    [CLIPBR]            = VIA_REG_CLIPBR,
    [KEYCONTROL]        = VIA_REG_KEYCONTROL,
    [SRCBASE]           = VIA_REG_SRCBASE,
    [DSTBASE]           = VIA_REG_DSTBASE,
    [PITCH]             = VIA_REG_PITCH,
    [MONOPAT0]          = VIA_REG_MONOPAT0,
    [MONOPAT1]          = VIA_REG_MONOPAT1,
    [COLORPAT]          = VIA_REG_COLORPAT,
    [MONOPATFGC]        = VIA_REG_FGCOLOR,
    [MONOPATBGC]        = VIA_REG_BGCOLOR
};

/* register offsets for new 2D core (M1 in VT3353 == VX800) */
static const unsigned via_2d_regs_m1[] = {
    [GECMD]             = VIA_REG_GECMD_M1,
    [GEMODE]            = VIA_REG_GEMODE_M1,
    [GESTATUS]          = VIA_REG_GESTATUS_M1,
    [SRCPOS]            = VIA_REG_SRCPOS_M1,
    [DSTPOS]            = VIA_REG_DSTPOS_M1,
    [LINE_K1K2]         = VIA_REG_LINE_K1K2_M1,
    [LINE_XY]           = VIA_REG_LINE_XY_M1,
    [LINE_ERROR]        = VIA_REG_LINE_ERROR_M1,
    [DIMENSION]         = VIA_REG_DIMENSION_M1,
    [PATADDR]           = VIA_REG_PATADDR_M1,
    [FGCOLOR]           = VIA_REG_FGCOLOR_M1,
    [DSTCOLORKEY]       = VIA_REG_DSTCOLORKEY_M1,
    [BGCOLOR]           = VIA_REG_BGCOLOR_M1,
    [SRCCOLORKEY]       = VIA_REG_SRCCOLORKEY_M1,
    [CLIPTL]            = VIA_REG_CLIPTL_M1,
    [CLIPBR]            = VIA_REG_CLIPBR_M1,
    [KEYCONTROL]        = VIA_REG_KEYCONTROL_M1,
    [SRCBASE]           = VIA_REG_SRCBASE_M1,
    [DSTBASE]           = VIA_REG_DSTBASE_M1,
    [PITCH]             = VIA_REG_PITCH_M1,
    [MONOPAT0]          = VIA_REG_MONOPAT0_M1,
    [MONOPAT1]          = VIA_REG_MONOPAT1_M1,
    [COLORPAT]          = VIA_REG_COLORPAT_M1,
    [MONOPATFGC]        = VIA_REG_MONOPATFGC_M1,
    [MONOPATBGC]        = VIA_REG_MONOPATBGC_M1
};

#define VIA_REG(pVia, name)	(pVia)->TwodRegs[name]

/*
 * Use PCI MMIO to flush the command buffer when AGP DMA is not available.
 */
static void
viaDumpDMA(ViaCommandBuffer * buf)
{
    register CARD32 *bp = buf->buf;
    CARD32 *endp = bp + buf->pos;

    while (bp != endp) {
        if (((bp - buf->buf) & 3) == 0) {
            ErrorF("\n %04lx: ", (unsigned long)(bp - buf->buf));
        }
        ErrorF("0x%08x ", (unsigned)*bp++);
    }
    ErrorF("\n");
}

void
viaFlushPCI(ViaCommandBuffer * buf)
{
    register CARD32 *bp = buf->buf;
    CARD32 transSetting;
    CARD32 *endp = bp + buf->pos;
    unsigned loop = 0;
    register CARD32 offset = 0;
    register CARD32 value;
    VIAPtr pVia = VIAPTR(buf->pScrn);

    while (bp < endp) {
        if (*bp == HALCYON_HEADER2) {
            if (++bp == endp)
                return;
            VIASETREG(VIA_REG_TRANSET, transSetting = *bp++);
            while (bp < endp) {
                if ((transSetting != HC_ParaType_CmdVdata)
                    && ((*bp == HALCYON_HEADER2)
                        || (*bp & HALCYON_HEADER1MASK) == HALCYON_HEADER1))
                    break;
                VIASETREG(VIA_REG_TRANSPACE, *bp++);
            }
        } else if ((*bp & HALCYON_HEADER1MASK) == HALCYON_HEADER1) {

            while (bp < endp) {
                if (*bp == HALCYON_HEADER2)
                    break;
                if (offset == 0) {
                    /*
                     * Not doing this wait will probably stall the processor
                     * for an unacceptable amount of time in VIASETREG while
                     * other high priority interrupts may be pending.
                     */
                    switch (pVia->Chipset) {
                    case VIA_VX800:
                    case VIA_VX855:
                    case VIA_VX900:
                        while ((VIAGETREG(VIA_REG_STATUS) &
                                (VIA_CMD_RGTR_BUSY_H5 | VIA_2D_ENG_BUSY_H5)) &&
                                (loop++ < MAXLOOP)) ;
                        break;

                    case VIA_K8M890:
                    case VIA_P4M890:
                    case VIA_P4M900:
                        while ((VIAGETREG(VIA_REG_STATUS) &
                                (VIA_CMD_RGTR_BUSY | VIA_2D_ENG_BUSY)) &&
                                (loop++ < MAXLOOP)) ;
                        break;

                    default:
                        while (!(VIAGETREG(VIA_REG_STATUS) & VIA_VR_QUEUE_EMPTY) &&
                                (loop++ < MAXLOOP)) ;
                        while ((VIAGETREG(VIA_REG_STATUS) &
                                (VIA_CMD_RGTR_BUSY | VIA_2D_ENG_BUSY)) &&
                                (loop++ < MAXLOOP)) ;
                    }
                }
                offset = (*bp++ & 0x0FFFFFFF) << 2;
                value = *bp++;
                VIASETREG(offset, value);
            }
        } else {
            ErrorF("Command stream parser error.\n");
        }
    }
    buf->pos = 0;
    buf->mode = 0;
    buf->has3dState = FALSE;
}

#ifdef XF86DRI
/*
 * Flush the command buffer using DRM. If in PCI mode, we can bypass DRM,
 * but not for command buffers that contain 3D engine state, since then
 * the DRM command verifier will lose track of the 3D engine state.
 */
static void
viaFlushDRIEnabled(ViaCommandBuffer * cb)
{
    ScrnInfoPtr pScrn = cb->pScrn;
    VIAPtr pVia = VIAPTR(pScrn);
    char *tmp = (char *)cb->buf;
    int tmpSize;
    drm_via_cmdbuffer_t b;

    /* Align end of command buffer for AGP DMA. */
    OUT_RING_H1(0x2f8, 0x67676767);
    if (pVia->agpDMA && cb->mode == 2 && cb->rindex != HC_ParaType_CmdVdata
        && (cb->pos & 1)) {
        OUT_RING(HC_DUMMY);
    }

    tmpSize = cb->pos * sizeof(CARD32);
    if (pVia->agpDMA || (pVia->directRenderingType && cb->has3dState)) {
        cb->mode = 0;
        cb->has3dState = FALSE;
        while (tmpSize > 0) {
            b.size = (tmpSize > VIA_DMASIZE) ? VIA_DMASIZE : tmpSize;
            tmpSize -= b.size;
            b.buf = tmp;
            tmp += b.size;
            if (drmCommandWrite(pVia->drmFD, ((pVia->agpDMA)
                                              ? DRM_VIA_CMDBUFFER :
                                              DRM_VIA_PCICMD), &b, sizeof(b))) {
                ErrorF("DRM command buffer submission failed.\n");
                viaDumpDMA(cb);
                return;
            }
        }
        cb->pos = 0;
    } else {
        viaFlushPCI(cb);
    }
}
#endif

/*
 * Initialize a command buffer. Some fields are currently not used since they
 * are intended for Unichrome Pro group A video commands.
 */
int
viaSetupCBuffer(ScrnInfoPtr pScrn, ViaCommandBuffer * buf, unsigned size)
{
#ifdef XF86DRI
    VIAPtr pVia = VIAPTR(pScrn);
#endif

    buf->pScrn = pScrn;
    buf->bufSize = ((size == 0) ? VIA_DMASIZE : size) >> 2;
    buf->buf = (CARD32 *) calloc(buf->bufSize, sizeof(CARD32));
    if (!buf->buf)
        return BadAlloc;
    buf->waitFlags = 0;
    buf->pos = 0;
    buf->mode = 0;
    buf->header_start = 0;
    buf->rindex = 0;
    buf->has3dState = FALSE;
    buf->flushFunc = viaFlushPCI;
#ifdef XF86DRI
    if (pVia->directRenderingType == DRI_1) {
        buf->flushFunc = viaFlushDRIEnabled;
    }
#endif
    return Success;
}

/*
 * Free resources associated with a command buffer.
 */
void
viaTearDownCBuffer(ViaCommandBuffer * buf)
{
    if (buf && buf->buf)
        free(buf->buf);
    buf->buf = NULL;
}

/*
 * Leftover from VIA's code.
 */
static void
viaInitPCIe(VIAPtr pVia)
{
    VIASETREG(0x41c, 0x00100000);
    VIASETREG(0x420, 0x680A0000);
    VIASETREG(0x420, 0x02000000);
}

static void
viaInitAgp(VIAPtr pVia)
{
    VIASETREG(VIA_REG_TRANSET, 0x00100000);
    VIASETREG(VIA_REG_TRANSPACE, 0x00000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x00333004);
    VIASETREG(VIA_REG_TRANSPACE, 0x60000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x61000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x62000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x63000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x64000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x7D000000);

    VIASETREG(VIA_REG_TRANSET, 0xfe020000);
    VIASETREG(VIA_REG_TRANSPACE, 0x00000000);
}

/*
 * Initialize the virtual command queue. Header-2 commands can be put
 * in this queue for buffering. AFAIK it doesn't handle Header-1 
 * commands, which is really a pity, since it has to be idled before
 * issuing a Header-1 command.
 */
static void
viaEnableAgpVQ(VIAPtr pVia)
{
   CARD32
       vqStartAddr = pVia->VQStart,
       vqEndAddr = pVia->VQEnd,
       vqStartL = 0x50000000 | (vqStartAddr & 0xFFFFFF),
       vqEndL = 0x51000000 | (vqEndAddr & 0xFFFFFF),
       vqStartEndH = 0x52000000 | ((vqStartAddr & 0xFF000000) >> 24) |
       ((vqEndAddr & 0xFF000000) >> 16),
       vqLen = 0x53000000 | (VIA_VQ_SIZE >> 3);

    VIASETREG(VIA_REG_TRANSET, 0x00fe0000);
    VIASETREG(VIA_REG_TRANSPACE, 0x080003fe);
    VIASETREG(VIA_REG_TRANSPACE, 0x0a00027c);
    VIASETREG(VIA_REG_TRANSPACE, 0x0b000260);
    VIASETREG(VIA_REG_TRANSPACE, 0x0c000274);
    VIASETREG(VIA_REG_TRANSPACE, 0x0d000264);
    VIASETREG(VIA_REG_TRANSPACE, 0x0e000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x0f000020);
    VIASETREG(VIA_REG_TRANSPACE, 0x1000027e);
    VIASETREG(VIA_REG_TRANSPACE, 0x110002fe);
    VIASETREG(VIA_REG_TRANSPACE, 0x200f0060);
    VIASETREG(VIA_REG_TRANSPACE, 0x00000006);
    VIASETREG(VIA_REG_TRANSPACE, 0x40008c0f);
    VIASETREG(VIA_REG_TRANSPACE, 0x44000000);
    VIASETREG(VIA_REG_TRANSPACE, 0x45080c04);
    VIASETREG(VIA_REG_TRANSPACE, 0x46800408);

    VIASETREG(VIA_REG_TRANSPACE, vqStartEndH);
    VIASETREG(VIA_REG_TRANSPACE, vqStartL);
    VIASETREG(VIA_REG_TRANSPACE, vqEndL);
    VIASETREG(VIA_REG_TRANSPACE, vqLen);
}

static void
viaEnablePCIeVQ(VIAPtr pVia)
{
   CARD32
       vqStartAddr = pVia->VQStart,
       vqEndAddr = pVia->VQEnd,
       vqStartL = 0x70000000 | (vqStartAddr & 0xFFFFFF),
       vqEndL = 0x71000000 | (vqEndAddr & 0xFFFFFF),
       vqStartEndH = 0x72000000 | ((vqStartAddr & 0xFF000000) >> 24) |
       ((vqEndAddr & 0xFF000000) >> 16),
       vqLen = 0x73000000 | (VIA_VQ_SIZE >> 3);

    VIASETREG(0x41c, 0x00100000);
    VIASETREG(0x420, vqStartEndH);
    VIASETREG(0x420, vqStartL);
    VIASETREG(0x420, vqEndL);
    VIASETREG(0x420, vqLen);
    VIASETREG(0x420, 0x74301001);
    VIASETREG(0x420, 0x00000000);
}

/*
 * Disable the virtual command queue.
 */
void
viaDisableVQ(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);

    switch (pVia->Chipset) {
        case VIA_K8M890:
        case VIA_P4M900:
        case VIA_VX800:
        case VIA_VX855:
        case VIA_VX900:
            VIASETREG(0x41c, 0x00100000);
            VIASETREG(0x420, 0x74301000);
            break;
        default:
            VIASETREG(VIA_REG_TRANSET, 0x00fe0000);
            VIASETREG(VIA_REG_TRANSPACE, 0x00000004);
            VIASETREG(VIA_REG_TRANSPACE, 0x40008c0f);
            VIASETREG(VIA_REG_TRANSPACE, 0x44000000);
            VIASETREG(VIA_REG_TRANSPACE, 0x45080c04);
            VIASETREG(VIA_REG_TRANSPACE, 0x46800408);
            break;
    }
}

/*
 * Update our 2D state (TwoDContext) with a new mode.
 */
static Bool
viaAccelSetMode(int bpp, ViaTwodContext * tdc)
{
    switch (bpp) {
        case 16:
            tdc->mode = VIA_GEM_16bpp;
            tdc->bytesPPShift = 1;
            return TRUE;
        case 32:
            tdc->mode = VIA_GEM_32bpp;
            tdc->bytesPPShift = 2;
            return TRUE;
        case 8:
            tdc->mode = VIA_GEM_8bpp;
            tdc->bytesPPShift = 0;
            return TRUE;
        default:
            tdc->bytesPPShift = 0;
            return FALSE;
    }
}

/*
 * Initialize the 2D engine and set the 2D context mode to the
 * current screen depth. Also enable the virtual queue.
 */
void
viaInitialize2DEngine(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;
    int i;

    /* Initialize the 2D engine registers to reset the 2D engine. */
    for (i = 0x04; i <= 0x40; i += 4) {
        VIASETREG(i, 0x0);
    }

    if (pVia->Chipset == VIA_VX800 ||
        pVia->Chipset == VIA_VX855 ||
        pVia->Chipset == VIA_VX900) {
        for (i = 0x44; i <= 0x5c; i += 4) {
            VIASETREG(i, 0x0);
        }
    }

    if (pVia->Chipset == VIA_VX900)
    {
        /*410 redefine 0x30 34 38*/
        VIASETREG(0x60, 0x0); /*already useable here*/
    }

    /* Make the VIA_REG() macro magic work */
    switch (pVia->Chipset) {
    case VIA_VX800:
    case VIA_VX855:
    case VIA_VX900:
        pVia->TwodRegs = via_2d_regs_m1;
        break;
    default:
        pVia->TwodRegs = via_2d_regs;
        break;
    }

    switch (pVia->Chipset) {
        case VIA_K8M890:
        case VIA_P4M900:
        case VIA_VX800:
        case VIA_VX855:
        case VIA_VX900:
            viaInitPCIe(pVia);
            break;
        default:
            viaInitAgp(pVia);
            break;
    }

    if (pVia->VQStart != 0) {
        switch (pVia->Chipset) {
            case VIA_K8M890:
            case VIA_P4M900:
            case VIA_VX800:
            case VIA_VX855:
            case VIA_VX900:
                viaEnablePCIeVQ(pVia);
                break;
            default:
                viaEnableAgpVQ(pVia);
                break;
        }
    } else {
        viaDisableVQ(pScrn);
    }

    viaAccelSetMode(pScrn->bitsPerPixel, tdc);
}

/*
 * Wait for acceleration engines idle. An expensive way to sync.
 */
void
viaAccelSync(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    int loop = 0;

    mem_barrier();

    switch (pVia->Chipset) {
        case VIA_VX800:
        case VIA_VX855:
        case VIA_VX900:
            while ((VIAGETREG(VIA_REG_STATUS) &
                    (VIA_CMD_RGTR_BUSY_H5 | VIA_2D_ENG_BUSY_H5 | VIA_3D_ENG_BUSY_H5))
                   && (loop++ < MAXLOOP)) ;
            break;
        case VIA_P4M890:
        case VIA_K8M890:
        case VIA_P4M900:
            while ((VIAGETREG(VIA_REG_STATUS) &
                    (VIA_CMD_RGTR_BUSY | VIA_2D_ENG_BUSY | VIA_3D_ENG_BUSY))
                   && (loop++ < MAXLOOP)) ;
            break;
        default:
            while (!(VIAGETREG(VIA_REG_STATUS) & VIA_VR_QUEUE_EMPTY)
                   && (loop++ < MAXLOOP)) ;

            while ((VIAGETREG(VIA_REG_STATUS) &
                    (VIA_CMD_RGTR_BUSY | VIA_2D_ENG_BUSY | VIA_3D_ENG_BUSY))
                   && (loop++ < MAXLOOP)) ;
            break;
    }
}

/*
 * Switch 2D state clipping on.
 */
static void
viaSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2)
{
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;

    tdc->clipping = TRUE;
    tdc->clipX1 = (x1 & 0xFFFF);
    tdc->clipY1 = y1;
    tdc->clipX2 = (x2 & 0xFFFF);
    tdc->clipY2 = y2;
}

/*
 * This is a small helper to wrap around a PITCH register write
 * to deal with the subtle differences of M1 and old 2D engine
 */
static void
viaPitchHelper(VIAPtr pVia, unsigned dstPitch, unsigned srcPitch)
{
    unsigned val = (dstPitch >> 3) << 16 | (srcPitch >> 3);
    RING_VARS;

    if (pVia->Chipset != VIA_VX800 &&
        pVia->Chipset != VIA_VX855 &&
        pVia->Chipset != VIA_VX900) {
        val |= VIA_PITCH_ENABLE;
    }
    OUT_RING_H1(VIA_REG(pVia, PITCH), val);
}

/*
 * Emit clipping borders to the command buffer and update the 2D context
 * current command with clipping info.
 */
static int
viaAccelClippingHelper(VIAPtr pVia, int refY)
{
    ViaTwodContext *tdc = &pVia->td;

    RING_VARS;

    if (tdc->clipping) {
        refY = (refY < tdc->clipY1) ? refY : tdc->clipY1;
        tdc->cmd |= VIA_GEC_CLIP_ENABLE;
        BEGIN_RING(4);
        OUT_RING_H1(VIA_REG(pVia, CLIPTL),
                    ((tdc->clipY1 - refY) << 16) | tdc->clipX1);
        OUT_RING_H1(VIA_REG(pVia, CLIPBR),
		    ((tdc->clipY2 - refY) << 16) | tdc->clipX2);
    } else {
        tdc->cmd &= ~VIA_GEC_CLIP_ENABLE;
    }
    return refY;
}

/*
 * Emit a solid blit operation to the command buffer.
 */
void
viaAccelSolidHelper(VIAPtr pVia, int x, int y, int w, int h,
                    unsigned fbBase, CARD32 mode, unsigned pitch,
                    CARD32 fg, CARD32 cmd)
{
    RING_VARS;

    BEGIN_RING(14);
    OUT_RING_H1(VIA_REG(pVia, GEMODE), mode);
    OUT_RING_H1(VIA_REG(pVia, DSTBASE), fbBase >> 3);
    viaPitchHelper(pVia, pitch, 0);
    OUT_RING_H1(VIA_REG(pVia, DSTPOS), (y << 16) | (x & 0xFFFF));
    OUT_RING_H1(VIA_REG(pVia, DIMENSION), ((h - 1) << 16) | (w - 1));
    OUT_RING_H1(VIA_REG(pVia, MONOPATFGC), fg);
    OUT_RING_H1(VIA_REG(pVia, GECMD), cmd);
}

/*
 * Check if we can use a planeMask and update the 2D context accordingly.
 */
static Bool
viaAccelPlaneMaskHelper(ViaTwodContext * tdc, CARD32 planeMask)
{
    CARD32 modeMask = (1 << ((1 << tdc->bytesPPShift) << 3)) - 1;
    CARD32 curMask = 0x00000000;
    CARD32 curByteMask;
    int i;

    if ((planeMask & modeMask) != modeMask) {

        /* Masking doesn't work in 8bpp. */
        if (modeMask == 0xFF) {
            tdc->keyControl &= 0x0FFFFFFF;
            return FALSE;
        }

        /* Translate the bit planemask to a byte planemask. */
        for (i = 0; i < (1 << tdc->bytesPPShift); ++i) {
            curByteMask = (0xFF << (i << 3));

            if ((planeMask & curByteMask) == 0) {
                curMask |= (1 << i);
            } else if ((planeMask & curByteMask) != curByteMask) {
                tdc->keyControl &= 0x0FFFFFFF;
                return FALSE;
            }
        }
        ErrorF("DEBUG: planeMask 0x%08x, curMask 0%02x\n",
               (unsigned)planeMask, (unsigned)curMask);

        tdc->keyControl = (tdc->keyControl & 0x0FFFFFFF) | (curMask << 28);
    }

    return TRUE;
}

/*
 * Emit transparency state and color to the command buffer.
 */
static void
viaAccelTransparentHelper(VIAPtr pVia, CARD32 keyControl,
                          CARD32 transColor, Bool usePlaneMask)
{
    ViaTwodContext *tdc = &pVia->td;

    RING_VARS;

    tdc->keyControl &= ((usePlaneMask) ? 0xF0000000 : 0x00000000);
    tdc->keyControl |= (keyControl & 0x0FFFFFFF);
    BEGIN_RING(4);
    OUT_RING_H1(VIA_REG(pVia, KEYCONTROL), tdc->keyControl);
    if (keyControl) {
        OUT_RING_H1(VIA_REG(pVia, SRCCOLORKEY), transColor);
    }
}

/*
 * Emit a copy blit operation to the command buffer.
 */
static void
viaAccelCopyHelper(VIAPtr pVia, int xs, int ys, int xd, int yd,
                   int w, int h, unsigned srcFbBase, unsigned dstFbBase,
                   CARD32 mode, unsigned srcPitch, unsigned dstPitch,
                   CARD32 cmd)
{
    RING_VARS;

    if (cmd & VIA_GEC_DECY) {
        ys += h - 1;
        yd += h - 1;
    }

    if (cmd & VIA_GEC_DECX) {
        xs += w - 1;
        xd += w - 1;
    }

    BEGIN_RING(16);
    OUT_RING_H1(VIA_REG(pVia, GEMODE), mode);
    OUT_RING_H1(VIA_REG(pVia, SRCBASE), srcFbBase >> 3);
    OUT_RING_H1(VIA_REG(pVia, DSTBASE), dstFbBase >> 3);
    viaPitchHelper(pVia, dstPitch, srcPitch);
    OUT_RING_H1(VIA_REG(pVia, SRCPOS), (ys << 16) | (xs & 0xFFFF));
    OUT_RING_H1(VIA_REG(pVia, DSTPOS), (yd << 16) | (xd & 0xFFFF));
    OUT_RING_H1(VIA_REG(pVia, DIMENSION), ((h - 1) << 16) | (w - 1));
    OUT_RING_H1(VIA_REG(pVia, GECMD), cmd);
}

/*
 * Mark Sync using the 2D blitter for AGP. NoOp for PCI.
 * In the future one could even launch a NULL PCI DMA command
 * to have an interrupt generated, provided it is possible to
 * write to the PCI DMA engines from the AGP command stream.
 */
int
viaAccelMarkSync(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);

    RING_VARS;

    ++pVia->curMarker;

    /* Wrap around without affecting the sign bit. */
    pVia->curMarker &= 0x7FFFFFFF;

    if (pVia->agpDMA) {
        BEGIN_RING(2);
        OUT_RING_H1(VIA_REG(pVia, KEYCONTROL), 0x00);
        viaAccelSolidHelper(pVia, 0, 0, 1, 1, pVia->markerOffset,
                            VIA_GEM_32bpp, 4, pVia->curMarker,
                            (0xF0 << 24) | VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT);
        ADVANCE_RING;
    }
    return pVia->curMarker;
}

/*
 * Wait for the value to get blitted, or in the PCI case for engine idle.
 */
void
viaAccelWaitMarker(ScreenPtr pScreen, int marker)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    CARD32 uMarker = marker;

    if (pVia->agpDMA) {
        while ((pVia->lastMarkerRead - uMarker) > (1 << 24))
            pVia->lastMarkerRead = *pVia->markerBuf;
    } else {
        viaAccelSync(pScrn);
    }
}

/*
 * Check if we need to force upload of the whole 3D state (when other
 * clients or subsystems have touched the 3D engine). Also tell DRI
 * clients and subsystems that we have touched the 3D engine.
 */
static Bool
viaCheckUpload(ScrnInfoPtr pScrn, Via3DState * v3d)
{
    VIAPtr pVia = VIAPTR(pScrn);
    Bool forceUpload;

    forceUpload = (pVia->lastToUpload != v3d);
    pVia->lastToUpload = v3d;

#ifdef XF86DRI
    if (pVia->directRenderingType == DRI_1) {
        volatile drm_via_sarea_t *saPriv = (drm_via_sarea_t *)
                DRIGetSAREAPrivate(pScrn->pScreen);
        int myContext = DRIGetContext(pScrn->pScreen);

        forceUpload = forceUpload || (saPriv->ctxOwner != myContext);
        saPriv->ctxOwner = myContext;
    }
#endif
    return forceUpload;
}

static Bool
viaOrder(CARD32 val, CARD32 * shift)
{
    *shift = 0;

    while (val > (1 << *shift))
        (*shift)++;
    return (val == (1 << *shift));
}

/*
 * Exa functions. It is assumed that EXA does not exceed the blitter limits.
 */
static Bool
viaExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;

    RING_VARS;

    if (exaGetPixmapPitch(pPixmap) & 7)
        return FALSE;

    if (!viaAccelSetMode(pPixmap->drawable.depth, tdc))
        return FALSE;

    if (!viaAccelPlaneMaskHelper(tdc, planeMask))
        return FALSE;
    viaAccelTransparentHelper(pVia, 0x0, 0x0, TRUE);

    tdc->cmd = VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT | VIAACCELPATTERNROP(alu);

    tdc->fgColor = fg;

    return TRUE;
}

static void
viaExaSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;
    CARD32 dstPitch, dstOffset;

    RING_VARS;

    int w = x2 - x1, h = y2 - y1;

    dstPitch = exaGetPixmapPitch(pPixmap);
    dstOffset = exaGetPixmapOffset(pPixmap);

    viaAccelSolidHelper(pVia, x1, y1, w, h, dstOffset,
                        tdc->mode, dstPitch, tdc->fgColor, tdc->cmd);
    ADVANCE_RING;
}

static void
viaExaDoneSolidCopy(PixmapPtr pPixmap)
{
}

static Bool
viaExaPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir,
                  int ydir, int alu, Pixel planeMask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;

    RING_VARS;

    if (pSrcPixmap->drawable.bitsPerPixel != pDstPixmap->drawable.bitsPerPixel)
        return FALSE;

    if ((tdc->srcPitch = exaGetPixmapPitch(pSrcPixmap)) & 3)
        return FALSE;

    if (exaGetPixmapPitch(pDstPixmap) & 7)
        return FALSE;

    tdc->srcOffset = exaGetPixmapOffset(pSrcPixmap);

    tdc->cmd = VIA_GEC_BLT | VIAACCELCOPYROP(alu);
    if (xdir < 0)
        tdc->cmd |= VIA_GEC_DECX;
    if (ydir < 0)
        tdc->cmd |= VIA_GEC_DECY;

    if (!viaAccelSetMode(pDstPixmap->drawable.bitsPerPixel, tdc))
        return FALSE;

    if (!viaAccelPlaneMaskHelper(tdc, planeMask))
        return FALSE;
    viaAccelTransparentHelper(pVia, 0x0, 0x0, TRUE);

    return TRUE;
}

static void
viaExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
           int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;
    CARD32 srcOffset = tdc->srcOffset;
    CARD32 dstOffset = exaGetPixmapOffset(pDstPixmap);

    RING_VARS;

    if (!width || !height)
        return;

    viaAccelCopyHelper(pVia, srcX, srcY, dstX, dstY, width, height,
                       srcOffset, dstOffset, tdc->mode, tdc->srcPitch,
                       exaGetPixmapPitch(pDstPixmap), tdc->cmd);
    ADVANCE_RING;
}

#ifdef VIA_DEBUG_COMPOSITE
static void
viaExaCompositePictDesc(PicturePtr pict, char *string, int n)
{
    char format[20];
    char size[20];

    if (!pict) {
        snprintf(string, n, "None");
        return;
    }

    switch (pict->format) {
        case PICT_x8r8g8b8:
            snprintf(format, 20, "RGB8888");
            break;
        case PICT_a8r8g8b8:
            snprintf(format, 20, "ARGB8888");
            break;
        case PICT_r5g6b5:
            snprintf(format, 20, "RGB565  ");
            break;
        case PICT_x1r5g5b5:
            snprintf(format, 20, "RGB555  ");
            break;
        case PICT_a8:
            snprintf(format, 20, "A8      ");
            break;
        case PICT_a1:
            snprintf(format, 20, "A1      ");
            break;
        default:
            snprintf(format, 20, "0x%x", (int)pict->format);
            break;
    }

    snprintf(size, 20, "%dx%d%s", pict->pDrawable->width,
             pict->pDrawable->height, pict->repeat ? " R" : "");

    snprintf(string, n, "0x%lx: fmt %s (%s)", (long)pict->pDrawable, format,
             size);
}

static void
viaExaPrintComposite(CARD8 op,
                     PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst)
{
    char sop[20];
    char srcdesc[40], maskdesc[40], dstdesc[40];

    switch (op) {
        case PictOpSrc:
            sprintf(sop, "Src");
            break;
        case PictOpOver:
            sprintf(sop, "Over");
            break;
        default:
            sprintf(sop, "0x%x", (int)op);
            break;
    }

    viaExaCompositePictDesc(pSrc, srcdesc, 40);
    viaExaCompositePictDesc(pMask, maskdesc, 40);
    viaExaCompositePictDesc(pDst, dstdesc, 40);

    ErrorF("Composite fallback: op %s, \n"
           "                    src  %s, \n"
           "                    mask %s, \n"
           "                    dst  %s, \n", sop, srcdesc, maskdesc, dstdesc);
}
#endif /* VIA_DEBUG_COMPOSITE */

/*
 * Helper for bitdepth expansion.
 */
static CARD32
viaBitExpandHelper(CARD32 pixel, CARD32 bits)
{
    CARD32 component, mask, tmp;

    component = pixel & ((1 << bits) - 1);
    mask = (1 << (8 - bits)) - 1;
    tmp = component << (8 - bits);
    return ((component & 1) ? (tmp | mask) : tmp);
}

/*
 * Extract the components from a pixel of the given format to an argb8888 pixel.
 * This is used to extract data from one-pixel repeat pixmaps.
 * Assumes little endian.
 */
static void
viaPixelARGB8888(unsigned format, void *pixelP, CARD32 * argb8888)
{
    CARD32 bits, shift, pixel, bpp;

    bpp = PICT_FORMAT_BPP(format);

    if (bpp <= 8) {
        pixel = *((CARD8 *) pixelP);
    } else if (bpp <= 16) {
        pixel = *((CARD16 *) pixelP);
    } else {
        pixel = *((CARD32 *) pixelP);
    }

    switch (PICT_FORMAT_TYPE(format)) {
        case PICT_TYPE_A:
            bits = PICT_FORMAT_A(format);
            *argb8888 = viaBitExpandHelper(pixel, bits) << 24;
            return;
        case PICT_TYPE_ARGB:
            shift = 0;
            bits = PICT_FORMAT_B(format);
            *argb8888 = viaBitExpandHelper(pixel, bits);
            shift += bits;
            bits = PICT_FORMAT_G(format);
            *argb8888 |= viaBitExpandHelper(pixel >> shift, bits) << 8;
            shift += bits;
            bits = PICT_FORMAT_R(format);
            *argb8888 |= viaBitExpandHelper(pixel >> shift, bits) << 16;
            shift += bits;
            bits = PICT_FORMAT_A(format);
            *argb8888 |= ((bits) ? viaBitExpandHelper(pixel >> shift,
                                                      bits) : 0xFF) << 24;
            return;
        case PICT_TYPE_ABGR:
            shift = 0;
            bits = PICT_FORMAT_B(format);
            *argb8888 = viaBitExpandHelper(pixel, bits) << 16;
            shift += bits;
            bits = PICT_FORMAT_G(format);
            *argb8888 |= viaBitExpandHelper(pixel >> shift, bits) << 8;
            shift += bits;
            bits = PICT_FORMAT_R(format);
            *argb8888 |= viaBitExpandHelper(pixel >> shift, bits);
            shift += bits;
            bits = PICT_FORMAT_A(format);
            *argb8888 |= ((bits) ? viaBitExpandHelper(pixel >> shift,
                                                      bits) : 0xFF) << 24;
            return;
        default:
            break;
    }
    return;
}

/*
 * Check if the above function will work.
 */
static Bool
viaExpandablePixel(int format)
{
    int formatType = PICT_FORMAT_TYPE(format);

    return (formatType == PICT_TYPE_A ||
            formatType == PICT_TYPE_ABGR || formatType == PICT_TYPE_ARGB);
}

#ifdef XF86DRI

static int
viaAccelDMADownload(ScrnInfoPtr pScrn, unsigned long fbOffset,
                    unsigned srcPitch, unsigned char *dst,
                    unsigned dstPitch, unsigned w, unsigned h)
{
    VIAPtr pVia = VIAPTR(pScrn);
    drm_via_dmablit_t blit[2], *curBlit;
    unsigned char *sysAligned = NULL;
    Bool doSync[2], useBounceBuffer;
    unsigned pitch, numLines[2];
    int curBuf, err, i, ret, blitHeight;

    ret = 0;

    useBounceBuffer = (((unsigned long)dst & 15) || (dstPitch & 15));
    doSync[0] = FALSE;
    doSync[1] = FALSE;
    curBuf = 1;
    blitHeight = h;
    pitch = dstPitch;
    if (useBounceBuffer) {
        pitch = ALIGN_TO(dstPitch, 16);
        blitHeight = VIA_DMA_DL_SIZE / pitch;
    }

    while (doSync[0] || doSync[1] || h != 0) {
        curBuf = 1 - curBuf;
        curBlit = &blit[curBuf];
        if (doSync[curBuf]) {

            do {
                err = drmCommandWrite(pVia->drmFD, DRM_VIA_BLIT_SYNC,
                                      &curBlit->sync, sizeof(curBlit->sync));
            } while (err == -EAGAIN);

            if (err)
                return err;

            doSync[curBuf] = FALSE;
            if (useBounceBuffer) {
                for (i = 0; i < numLines[curBuf]; ++i) {
                    memcpy(dst, curBlit->mem_addr, w);
                    dst += dstPitch;
                    curBlit->mem_addr += pitch;
                }
            }
        }

        if (h == 0)
            continue;

        curBlit->num_lines = (h > blitHeight) ? blitHeight : h;
        h -= curBlit->num_lines;
        numLines[curBuf] = curBlit->num_lines;

        sysAligned =
                (unsigned char *)pVia->dBounce + (curBuf * VIA_DMA_DL_SIZE);
        sysAligned = (unsigned char *)
                ALIGN_TO((unsigned long)sysAligned, 16);

        curBlit->mem_addr = (useBounceBuffer) ? sysAligned : dst;
        curBlit->line_length = w;
        curBlit->mem_stride = pitch;
        curBlit->fb_addr = fbOffset;
        curBlit->fb_stride = srcPitch;
        curBlit->to_fb = 0;
        fbOffset += curBlit->num_lines * srcPitch;

        do {
            err = drmCommandWriteRead(pVia->drmFD, DRM_VIA_DMA_BLIT, curBlit,
                                      sizeof(*curBlit));
        } while (err == -EAGAIN);

        if (err) {
            ret = err;
            h = 0;
            continue;
        }

        doSync[curBuf] = TRUE;
    }

    return ret;
}

/*
 * Use PCI DMA if we can. If the system alignments don't match, we're using
 * an aligned bounce buffer for pipelined PCI DMA and memcpy.
 * Throughput for large transfers is around 65 MB/s.
 */
static Bool
viaExaDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
                         char *dst, int dst_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    unsigned srcPitch = exaGetPixmapPitch(pSrc);
    unsigned wBytes = (pSrc->drawable.bitsPerPixel * w + 7) >> 3;
    unsigned srcOffset;
    char *bounceAligned = NULL;
    unsigned totSize;

    if (!w || !h)
        return TRUE;

    srcOffset = x * pSrc->drawable.bitsPerPixel;
    if (srcOffset & 3)
        return FALSE;
    srcOffset = exaGetPixmapOffset(pSrc) + y * srcPitch + (srcOffset >> 3);

    totSize = wBytes * h;

    exaWaitSync(pScrn->pScreen);
    if (totSize < VIA_MIN_DOWNLOAD) {
        bounceAligned = (char *)pVia->FBBase + srcOffset;
        while (h--) {
            memcpy(dst, bounceAligned, wBytes);
            dst += dst_pitch;
            bounceAligned += srcPitch;
        }
        return TRUE;
    }

    if (!pVia->directRenderingType)
        return FALSE;

    if ((srcPitch & 3) || (srcOffset & 3)) {
        ErrorF("VIA EXA download src_pitch misaligned\n");
        return FALSE;
    }

    if (viaAccelDMADownload(pScrn, srcOffset, srcPitch, (unsigned char *)dst,
                            dst_pitch, wBytes, h))
        return FALSE;

    return TRUE;
}

/*
 * Upload to framebuffer memory using memcpy to AGP pipelined with a
 * 3D engine texture operation from AGP to framebuffer. The AGP buffers (2)
 * should be kept rather small for optimal pipelining.
 */
static Bool
viaExaTexUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src,
                        int src_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    unsigned dstPitch = exaGetPixmapPitch(pDst);
    unsigned wBytes = (w * pDst->drawable.bitsPerPixel + 7) >> 3;
    unsigned dstOffset;
    CARD32 texWidth, texHeight, texPitch;
    int format;
    char *dst, *texAddr;
    int i, sync[2], yOffs, bufH, bufOffs, height;
    Bool buf;
    Via3DState *v3d = &pVia->v3d;

    if (!w || !h)
        return TRUE;

    if (wBytes * h < VIA_MIN_TEX_UPLOAD) {
        dstOffset = x * pDst->drawable.bitsPerPixel;
        if (dstOffset & 3)
            return FALSE;
        dst = (char *)pVia->FBBase + (exaGetPixmapOffset(pDst) + y * dstPitch
                                      + (dstOffset >> 3));
        exaWaitSync(pScrn->pScreen);

        while (h--) {
            memcpy(dst, src, wBytes);
            dst += dstPitch;
            src += src_pitch;
        }
        return TRUE;
    }

    if (!pVia->texAGPBuffer->ptr)
        return FALSE;

    switch (pDst->drawable.bitsPerPixel) {
        case 32:
            format = PICT_a8r8g8b8;
            break;
        case 16:
            format = PICT_r5g6b5;
            break;
        default:
            return FALSE;
    }

    dstOffset = exaGetPixmapOffset(pDst);

    if (pVia->nPOT[0]) {
        texPitch = ALIGN_TO(wBytes, 32);
        height = VIA_AGP_UPL_SIZE / texPitch;
    } else {
        viaOrder(wBytes, &texPitch);
        if (texPitch < 3)
            texPitch = 3;
        height = VIA_AGP_UPL_SIZE >> texPitch;
        texPitch = 1 << texPitch;
    }

    if (height > 1024)
        height = 1024;
    viaOrder(w, &texWidth);
    texWidth = 1 << texWidth;

    texHeight = height << 1;
    bufOffs = texPitch * height;
    texAddr = (char *) drm_bo_map(pScrn, pVia->texAGPBuffer);

    v3d->setDestination(v3d, dstOffset, dstPitch, format);
    v3d->setDrawing(v3d, 0x0c, 0xFFFFFFFF, 0x000000FF, 0x00);
    v3d->setFlags(v3d, 1, TRUE, TRUE, FALSE);
    if (!v3d->setTexture(v3d, 0, (unsigned long) texAddr, texPitch,
                         pVia->nPOT[0], texWidth, texHeight, format,
                         via_single, via_single, via_src, TRUE))
        return FALSE;

    v3d->emitState(v3d, &pVia->cb, viaCheckUpload(pScrn, v3d));
    v3d->emitClipRect(v3d, &pVia->cb, 0, 0, pDst->drawable.width,
                      pDst->drawable.height);

    buf = 1;
    yOffs = 0;
    sync[0] = -1;
    sync[1] = -1;

    while (h) {
        buf = (buf) ? 0 : 1;
        bufH = (h > height) ? height : h;
        dst = texAddr + ((buf) ? bufOffs : 0);

        if (sync[buf] >= 0)
            viaAccelWaitMarker(pScrn->pScreen, sync[buf]);

        for (i = 0; i < bufH; ++i) {
            memcpy(dst, src, wBytes);
            dst += texPitch;
            src += src_pitch;
        }

        v3d->emitQuad(v3d, &pVia->cb, x, y + yOffs, 0, (buf) ? height : 0, 0,
                      0, w, bufH);

        sync[buf] = viaAccelMarkSync(pScrn->pScreen);

        h -= bufH;
        yOffs += bufH;
    }

    if (sync[buf] >= 0)
        viaAccelWaitMarker(pScrn->pScreen, sync[buf]);

    return TRUE;
}

/*
 * I'm not sure PCI DMA upload is necessary. Seems buggy for widths below 65,
 * and I'd guess that in most situations CPU direct writes are faster.
 * Use DMA only when alignments match. At least it saves some CPU cycles.
 */
static Bool
viaExaUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src,
                     int src_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    drm_via_dmablit_t blit;
    unsigned dstPitch = exaGetPixmapPitch(pDst);
    unsigned wBytes = (w * pDst->drawable.bitsPerPixel + 7) >> 3;
    unsigned dstOffset;
    char *dst;
    int err;

    dstOffset = x * pDst->drawable.bitsPerPixel;
    if (dstOffset & 3)
        return FALSE;
    dstOffset = exaGetPixmapOffset(pDst) + y * dstPitch + (dstOffset >> 3);

    if (wBytes * h < VIA_MIN_UPLOAD || wBytes < 65) {
        dst = (char *)pVia->FBBase + dstOffset;

        exaWaitSync(pScrn->pScreen);
        while (h--) {
            memcpy(dst, src, wBytes);
            dst += dstPitch;
            src += src_pitch;
        }
        return TRUE;
    }

    if (!pVia->directRenderingType)
        return FALSE;

    if (((unsigned long)src & 15) || (src_pitch & 15))
        return FALSE;

    if ((dstPitch & 3) || (dstOffset & 3))
        return FALSE;

    blit.line_length = wBytes;
    blit.num_lines = h;
    blit.fb_addr = dstOffset;
    blit.fb_stride = dstPitch;
    blit.mem_addr = (unsigned char *)src;
    blit.mem_stride = src_pitch;
    blit.to_fb = 1;

    exaWaitSync(pScrn->pScreen);
    while (-EAGAIN == (err = drmCommandWriteRead(pVia->drmFD, DRM_VIA_DMA_BLIT,
                                                 &blit, sizeof(blit)))) ;
    if (err < 0)
        return FALSE;

    while (-EAGAIN == (err = drmCommandWrite(pVia->drmFD, DRM_VIA_BLIT_SYNC,
                                             &blit.sync, sizeof(blit.sync)))) ;
    return (err == 0);
}

#endif /* XF86DRI */

static Bool
viaExaCheckComposite(int op, PicturePtr pSrcPicture,
                     PicturePtr pMaskPicture, PicturePtr pDstPicture)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPicture->pDrawable->pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    Via3DState *v3d = &pVia->v3d;

    /* Reject small composites early. They are done much faster in software. */
    if (!pSrcPicture->repeat &&
        pSrcPicture->pDrawable->width *
        pSrcPicture->pDrawable->height < VIA_MIN_COMPOSITE)
        return FALSE;

    if (pMaskPicture &&
        !pMaskPicture->repeat &&
        pMaskPicture->pDrawable->width *
        pMaskPicture->pDrawable->height < VIA_MIN_COMPOSITE)
        return FALSE;

    if (pMaskPicture && pMaskPicture->repeat != RepeatNormal)
        return FALSE;

    if (pMaskPicture && pMaskPicture->componentAlpha) {
#ifdef VIA_DEBUG_COMPOSITE
        ErrorF("Component Alpha operation\n");
#endif
        return FALSE;
    }

    if (!v3d->opSupported(op)) {
#ifdef VIA_DEBUG_COMPOSITE
#warning Composite verbose debug turned on.
        ErrorF("Operator not supported\n");
        viaExaPrintComposite(op, pSrcPicture, pMaskPicture, pDstPicture);
#endif
        return FALSE;
    }

    /*
     * FIXME: A8 destination formats are currently not supported and do not
     * seem supported by the hardware, although there are some leftover
     * register settings apparent in the via_3d_reg.h file. We need to fix this
     * (if important), by using component ARGB8888 operations with bitmask.
     */

    if (!v3d->dstSupported(pDstPicture->format)) {
#ifdef VIA_DEBUG_COMPOSITE
        ErrorF("Destination format not supported:\n");
        viaExaPrintComposite(op, pSrcPicture, pMaskPicture, pDstPicture);
#endif
        return FALSE;
    }

    if (v3d->texSupported(pSrcPicture->format)) {
        if (pMaskPicture && (PICT_FORMAT_A(pMaskPicture->format) == 0 ||
                             !v3d->texSupported(pMaskPicture->format))) {
#ifdef VIA_DEBUG_COMPOSITE
            ErrorF("Mask format not supported:\n");
            viaExaPrintComposite(op, pSrcPicture, pMaskPicture, pDstPicture);
#endif
            return FALSE;
        }
        return TRUE;
    }
#ifdef VIA_DEBUG_COMPOSITE
    ErrorF("Src format not supported:\n");
    viaExaPrintComposite(op, pSrcPicture, pMaskPicture, pDstPicture);
#endif
    return FALSE;
}

static Bool
viaIsAGP(VIAPtr pVia, PixmapPtr pPix, unsigned long *offset)
{
#ifdef XF86DRI
    unsigned long offs;

    if (pVia->directRenderingType && !pVia->IsPCI) {
        offs = ((unsigned long)pPix->devPrivate.ptr
                - (unsigned long)pVia->agpMappedAddr);

        if ((offs - pVia->scratchOffset) < pVia->agpSize) {
            *offset = offs + pVia->agpAddr;
            return TRUE;
        }
    }
#endif
    return FALSE;
}

static Bool
viaExaIsOffscreen(PixmapPtr pPix)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);

    return ((unsigned long)pPix->devPrivate.ptr -
            (unsigned long)pVia->FBBase) < pVia->videoRambytes;
}

static Bool
viaExaPrepareComposite(int op, PicturePtr pSrcPicture,
                       PicturePtr pMaskPicture, PicturePtr pDstPicture,
                       PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
    CARD32 height, width;
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    Via3DState *v3d = &pVia->v3d;
    int curTex = 0;
    ViaTexBlendingModes srcMode;
    Bool isAGP;
    unsigned long offset;

    /* Workaround: EXA crash with new libcairo2 on a VIA VX800 (#298) */
    /* TODO Add real source only pictures */
    if (!pSrc) {
	    ErrorF("pSrc is NULL\n");
	    return FALSE;
	}

    v3d->setDestination(v3d, exaGetPixmapOffset(pDst),
                        exaGetPixmapPitch(pDst), pDstPicture->format);
    v3d->setCompositeOperator(v3d, op);
    v3d->setDrawing(v3d, 0x0c, 0xFFFFFFFF, 0x000000FF, 0xFF);

    viaOrder(pSrc->drawable.width, &width);
    viaOrder(pSrc->drawable.height, &height);

    /*
     * For one-pixel repeat mask pictures we avoid using multitexturing by
     * modifying the src's texture blending equation and feed the pixel
     * value as a constant alpha for the src's texture. Multitexturing on the
     * Unichromes seems somewhat slow, so this speeds up translucent windows.
     */

    srcMode = via_src;
    pVia->maskP = NULL;
    if (pMaskPicture &&
        (pMaskPicture->pDrawable->height == 1) &&
        (pMaskPicture->pDrawable->width == 1) &&
        pMaskPicture->repeat && viaExpandablePixel(pMaskPicture->format)) {
        pVia->maskP = pMask->devPrivate.ptr;
        pVia->maskFormat = pMaskPicture->format;
        pVia->componentAlpha = pMaskPicture->componentAlpha;
        srcMode = ((pMaskPicture->componentAlpha)
                   ? via_src_onepix_comp_mask : via_src_onepix_mask);
    }

    /*
     * One-Pixel repeat src pictures go as solid color instead of textures.
     * Speeds up window shadows.
     */

    pVia->srcP = NULL;
    if (pSrcPicture && pSrcPicture->repeat
        && (pSrcPicture->pDrawable->height == 1)
        && (pSrcPicture->pDrawable->width == 1)
        && viaExpandablePixel(pSrcPicture->format)) {
        pVia->srcP = pSrc->devPrivate.ptr;
        pVia->srcFormat = pSrcPicture->format;
    }

    /* Exa should be smart enough to eliminate this IN operation. */
    if (pVia->srcP && pVia->maskP) {
        ErrorF("Bad one-pixel IN composite operation. "
               "EXA needs to be smarter.\n");
        return FALSE;
    }

    if (!pVia->srcP) {
        offset = exaGetPixmapOffset(pSrc);
        isAGP = viaIsAGP(pVia, pSrc, &offset);
        if (!isAGP && !viaExaIsOffscreen(pSrc))
            return FALSE;
        if (!v3d->setTexture(v3d, curTex, offset,
                             exaGetPixmapPitch(pSrc), pVia->nPOT[curTex],
                             1 << width, 1 << height, pSrcPicture->format,
                             via_repeat, via_repeat, srcMode, isAGP)) {
            return FALSE;
        }
        curTex++;
    }

    if (pMaskPicture && !pVia->maskP) {
        offset = exaGetPixmapOffset(pMask);
        isAGP = viaIsAGP(pVia, pMask, &offset);
        if (!isAGP && !viaExaIsOffscreen(pMask))
            return FALSE;
        viaOrder(pMask->drawable.width, &width);
        viaOrder(pMask->drawable.height, &height);
        if (!v3d->setTexture(v3d, curTex, offset,
                             exaGetPixmapPitch(pMask), pVia->nPOT[curTex],
                             1 << width, 1 << height, pMaskPicture->format,
                             via_repeat, via_repeat,
                             ((pMaskPicture->componentAlpha)
                              ? via_comp_mask : via_mask), isAGP)) {
            return FALSE;
        }
        curTex++;
    }

    v3d->setFlags(v3d, curTex, FALSE, TRUE, TRUE);
    v3d->emitState(v3d, &pVia->cb, viaCheckUpload(pScrn, v3d));
    v3d->emitClipRect(v3d, &pVia->cb, 0, 0, pDst->drawable.width,
                      pDst->drawable.height);

    return TRUE;
}

static void
viaExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
                int dstX, int dstY, int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    Via3DState *v3d = &pVia->v3d;
    CARD32 col;

    if (pVia->maskP) {
        viaPixelARGB8888(pVia->maskFormat, pVia->maskP, &col);
        v3d->setTexBlendCol(v3d, 0, pVia->componentAlpha, col);
    }
    if (pVia->srcP) {
        viaPixelARGB8888(pVia->srcFormat, pVia->srcP, &col);
        v3d->setDrawing(v3d, 0x0c, 0xFFFFFFFF, col & 0x00FFFFFF, col >> 24);
        srcX = maskX;
        srcY = maskY;
    }

    if (pVia->maskP || pVia->srcP)
        v3d->emitState(v3d, &pVia->cb, viaCheckUpload(pScrn, v3d));

    v3d->emitQuad(v3d, &pVia->cb, dstX, dstY, srcX, srcY, maskX, maskY,
                  width, height);
}


static ExaDriverPtr
viaInitExa(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    ExaDriverPtr pExa = exaDriverAlloc();

    memset(pExa, 0, sizeof(*pExa));

    if (!pExa)
        return NULL;

    pExa->exa_major = EXA_VERSION_MAJOR;
    pExa->exa_minor = EXA_VERSION_MINOR;
    pExa->memoryBase = pVia->FBBase;
    pExa->memorySize = pVia->FBFreeEnd;
    pExa->offScreenBase = pScrn->virtualY * pVia->Bpl;
    pExa->pixmapOffsetAlign = 32;
    pExa->pixmapPitchAlign = 16;
    pExa->flags = EXA_OFFSCREEN_PIXMAPS |
            (pVia->nPOT[1] ? 0 : EXA_OFFSCREEN_ALIGN_POT);
    pExa->maxX = 2047;
    pExa->maxY = 2047;
    pExa->WaitMarker = viaAccelWaitMarker;
    pExa->MarkSync = viaAccelMarkSync;
    pExa->PrepareSolid = viaExaPrepareSolid;
    pExa->Solid = viaExaSolid;
    pExa->DoneSolid = viaExaDoneSolidCopy;
    pExa->PrepareCopy = viaExaPrepareCopy;
    pExa->Copy = viaExaCopy;
    pExa->DoneCopy = viaExaDoneSolidCopy;

#ifdef XF86DRI
    if (pVia->directRenderingType == DRI_1) {
#ifdef linux
        if ((pVia->drmVerMajor > 2) ||
            ((pVia->drmVerMajor == 2) && (pVia->drmVerMinor >= 7))) {
            pExa->DownloadFromScreen = viaExaDownloadFromScreen;
        }
#endif /* linux */
        switch (pVia->Chipset) {
            case VIA_K8M800:
            case VIA_KM400:
                pExa->UploadToScreen = viaExaTexUploadToScreen;
                break;
            default:
                pExa->UploadToScreen = NULL;
                break;
        }
    }
#endif /* XF86DRI */

    if (!pVia->noComposite) {
        pExa->CheckComposite = viaExaCheckComposite;
        pExa->PrepareComposite = viaExaPrepareComposite;
        pExa->Composite = viaExaComposite;
        pExa->DoneComposite = viaExaDoneSolidCopy;
    } else {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "[EXA] Disabling EXA accelerated composite.\n");
    }

    if (!exaDriverInit(pScreen, pExa)) {
        free(pExa);
        return NULL;
    }

    viaInit3DState(&pVia->v3d);
    return pExa;
}


/*
 * Acceleration initialization function. Sets up offscreen memory disposition,
 * and initializes engines and acceleration method.
 */
Bool
UMSAccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
    Bool nPOTSupported, ret;

    /*  HW Limitation are described here:
     *
     *  1. H2/H5/H6 2D source and destination:
     *     Pitch: (1 << 14) - 1 = 16383
     *     Dimension: (1 << 12) = 4096
     *     X, Y position: (1 << 12) - 1 = 4095.
     *
     *  2. H2 3D engine Render target:
     *     Pitch: (1 << 14) - 1 = 16383
     *     Clip Rectangle: 0 - 2047
     *
     *  3. H5/H6 3D engine Render target:
     *     Pitch: ((1 << 10) - 1)*32 = 32736
     *     Clip Rectangle: Color Window, 12bits. As Spec saied: 0 - 2048
     *                     Scissor is the same as color window.
     * */
    pVia->VQStart = 0;
    pVia->vq_bo = drm_bo_alloc(pScrn, VIA_VQ_SIZE, 16, TTM_PL_FLAG_VRAM);
    if (pVia->vq_bo) {
        pVia->VQStart = pVia->vq_bo->offset;
        pVia->VQEnd = pVia->vq_bo->offset + pVia->vq_bo->size;
    }

    viaInitialize2DEngine(pScrn);

    VIAInitialize3DEngine(pScrn);

    if (Success != viaSetupCBuffer(pScrn, &pVia->cb, 0))
        pVia->NoAccel = TRUE;

    /* Sync marker space. */
    pVia->exa_sync_bo = drm_bo_alloc(pScrn, 32, 32, TTM_PL_FLAG_VRAM);
    if (!pVia->exa_sync_bo)
        return FALSE;
    pVia->markerOffset = pVia->exa_sync_bo->offset;
    pVia->markerOffset = (pVia->markerOffset + 31) & ~31;
    pVia->markerBuf = (CARD32 *) drm_bo_map(pScrn, pVia->exa_sync_bo);
    *pVia->markerBuf = 0;
    pVia->curMarker = 0;
    pVia->lastMarkerRead = 0;

    /*
     * nPOT textures. DRM versions below 2.11.0 don't allow them.
     * Also some CLE266 hardware may not allow nPOT textures for
     * texture engine 1. We need to figure that out.
     */

    nPOTSupported = TRUE;
#ifdef XF86DRI
    nPOTSupported = ((!pVia->directRenderingType) ||
                     (pVia->drmVerMajor > 2) ||
                     ((pVia->drmVerMajor == 2) && (pVia->drmVerMinor >= 11)));
#endif
    pVia->nPOT[0] = nPOTSupported;
    pVia->nPOT[1] = nPOTSupported;

#ifdef XF86DRI
    pVia->dBounce = NULL;
    pVia->scratchAddr = NULL;
#endif /* XF86DRI */
    if (pVia->useEXA) {
        pVia->exaDriverPtr = viaInitExa(pScreen);
        if (!pVia->exaDriverPtr) {

            /*
             * Docs recommend turning off also Xv here, but we handle this
             * case with the old linear offscreen FB manager
             */
            pVia->NoAccel = TRUE;
            return FALSE;
        }
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "[EXA] Enabled EXA acceleration.\n");
        return TRUE;
    }
    return viaInitXAA(pScreen);
}

void
UMSAccelSetup(ScrnInfoPtr pScrn)
{
	ScreenPtr pScreen = pScrn->pScreen;
	VIAPtr pVia = VIAPTR(pScrn);

#ifdef XF86DRI
	if (pVia->directRenderingType == DRI_1) {
		if (!VIADRIFinishScreenInit(pScreen)) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering disabled\n");
			pVia->directRenderingType = DRI_NONE;
		} else
			xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering enabled\n");
	}
#endif

	if (pVia->NoAccel)
		memset(pVia->FBBase, 0x00, pVia->videoRambytes);
	else
		viaFinishInitAccel(pScreen);
}

/*
 * Free the used acceleration resources.
 */
void
viaExitAccel(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);

    viaAccelSync(pScrn);
    viaTearDownCBuffer(&pVia->cb);

    if (pVia->useEXA) {
#ifdef XF86DRI
        if (pVia->directRenderingType == DRI_1) {
            if (pVia->texAGPBuffer) {
                drm_bo_free(pScrn, pVia->texAGPBuffer);
                pVia->texAGPBuffer = NULL;
            }

            if (pVia->scratchBuffer) {
                drm_bo_free(pScrn, pVia->scratchBuffer);
                pVia->scratchBuffer = NULL;
            }
        }
        if (pVia->dBounce)
            free(pVia->dBounce);
#endif /* XF86DRI */
        if (pVia->scratchBuffer) {
            drm_bo_free(pScrn, pVia->scratchBuffer);
            pVia->scratchBuffer = NULL;
        }
        if (pVia->vq_bo) {
            drm_bo_unmap(pScrn, pVia->vq_bo);
            drm_bo_free(pScrn, pVia->vq_bo);
        }
        if (pVia->exa_sync_bo) {
            drm_bo_unmap(pScrn, pVia->exa_sync_bo);
            drm_bo_free(pScrn, pVia->exa_sync_bo);
        }
        if (pVia->exaDriverPtr) {
            exaDriverFini(pScreen);
        }
        free(pVia->exaDriverPtr);
        pVia->exaDriverPtr = NULL;
        return;
    }
    if (pVia->AccelInfoRec) {
        XAADestroyInfoRec(pVia->AccelInfoRec);
        pVia->AccelInfoRec = NULL;
    }
}

/*
 * Allocate a command buffer and  buffers for accelerated upload, download,
 * and EXA scratch area. The scratch area resides primarily in AGP memory,
 * but reverts to FB if AGP is not available.
 */
void
viaFinishInitAccel(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr pVia = VIAPTR(pScrn);
#ifdef XF86DRI
    int size, ret;

    if (pVia->directRenderingType && pVia->useEXA) {

        pVia->dBounce = calloc(VIA_DMA_DL_SIZE * 2, 1);

        if (!pVia->IsPCI) {

            /* Allocate upload and scratch space. */
            if (pVia->exaDriverPtr->UploadToScreen == viaExaTexUploadToScreen) {
                size = VIA_AGP_UPL_SIZE * 2;

                pVia->texAGPBuffer = drm_bo_alloc(pScrn, size, 32, TTM_PL_FLAG_TT);
                if (pVia->texAGPBuffer) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                               "Allocated %u kiB of AGP memory for "
                               "system-to-framebuffer transfer.\n",
                               size / 1024);
                    pVia->texAGPBuffer->offset = (pVia->texAGPBuffer->offset + 31) & ~31;
                }
            }

            size = pVia->exaScratchSize * 1024;
            pVia->scratchBuffer = drm_bo_alloc(pScrn, size, 32, TTM_PL_FLAG_TT);
            if (pVia->scratchBuffer) {
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                           "Allocated %u kiB of AGP memory for "
                           "EXA scratch area.\n", size / 1024);
                pVia->scratchOffset =
                        (pVia->scratchBuffer->offset + 31) & ~31;
                pVia->scratchAddr = drm_bo_map(pScrn, pVia->scratchBuffer);
            }
        }
    }
#endif /* XF86DRI */
    if (!pVia->scratchAddr && pVia->useEXA) {
        size = pVia->exaScratchSize * 1024 + 32;
        pVia->scratchBuffer = drm_bo_alloc(pScrn, size, 32, TTM_PL_FLAG_SYSTEM);

        if (pVia->scratchBuffer) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "Allocated %u kiB of framebuffer memory for "
                       "EXA scratch area.\n", pVia->exaScratchSize);
            pVia->scratchOffset = pVia->scratchBuffer->offset;
            pVia->scratchAddr = drm_bo_map(pScrn, pVia->scratchBuffer);
        }
    }
}

/*
 * DGA accelerated functions go here and let them be independent of
 * acceleration method.
 */
void
viaAccelBlitRect(ScrnInfoPtr pScrn, int srcx, int srcy, int w, int h,
                 int dstx, int dsty)
{
    VIAPtr pVia = VIAPTR(pScrn);
    ViaTwodContext *tdc = &pVia->td;
    unsigned dstOffset = pScrn->fbOffset + dsty * pVia->Bpl;
    unsigned srcOffset = pScrn->fbOffset + srcy * pVia->Bpl;

    RING_VARS;

    if (!w || !h)
        return;

    if (!pVia->NoAccel) {

        int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
        int ydir = (srcy < dsty) ? -1 : 1;
        CARD32 cmd = VIA_GEC_BLT | VIAACCELCOPYROP(GXcopy);

        if (xdir < 0)
            cmd |= VIA_GEC_DECX;
        if (ydir < 0)
            cmd |= VIA_GEC_DECY;

        viaAccelSetMode(pScrn->bitsPerPixel, tdc);
        viaAccelTransparentHelper(pVia, 0x0, 0x0, FALSE);
        viaAccelCopyHelper(pVia, srcx, 0, dstx, 0, w, h, srcOffset, dstOffset,
                           tdc->mode, pVia->Bpl, pVia->Bpl, cmd);
        pVia->accelMarker = viaAccelMarkSync(pScrn->pScreen);
        ADVANCE_RING;
    }
}

void
viaAccelFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h,
                 unsigned long color)
{
    VIAPtr pVia = VIAPTR(pScrn);
    unsigned dstBase = pScrn->fbOffset + y * pVia->Bpl;
    ViaTwodContext *tdc = &pVia->td;
    CARD32 cmd = VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT |
            VIAACCELPATTERNROP(GXcopy);
    RING_VARS;

    if (!w || !h)
        return;

    if (!pVia->NoAccel) {
        viaAccelSetMode(pScrn->bitsPerPixel, tdc);
        viaAccelTransparentHelper(pVia, 0x0, 0x0, FALSE);
        viaAccelSolidHelper(pVia, x, 0, w, h, dstBase, tdc->mode,
                            pVia->Bpl, color, cmd);
        pVia->accelMarker = viaAccelMarkSync(pScrn->pScreen);
        ADVANCE_RING;
    }
}

void
viaAccelFillPixmap(ScrnInfoPtr pScrn,
                   unsigned long offset,
                   unsigned long pitch,
                   int depth, int x, int y, int w, int h, unsigned long color)
{
    VIAPtr pVia = VIAPTR(pScrn);
    unsigned dstBase = offset + y * pitch;
    ViaTwodContext *tdc = &pVia->td;
    CARD32 cmd = VIA_GEC_BLT | VIA_GEC_FIXCOLOR_PAT |
            VIAACCELPATTERNROP(GXcopy);
    RING_VARS;

    if (!w || !h)
        return;

    if (!pVia->NoAccel) {
        viaAccelSetMode(depth, tdc);
        viaAccelTransparentHelper(pVia, 0x0, 0x0, FALSE);
        viaAccelSolidHelper(pVia, x, 0, w, h, dstBase, tdc->mode,
                            pitch, color, cmd);
        pVia->accelMarker = viaAccelMarkSync(pScrn->pScreen);
        ADVANCE_RING;
    }
}

void
viaAccelSyncMarker(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);

    viaAccelWaitMarker(pScrn->pScreen, pVia->accelMarker);
}

void
viaAccelTextureBlit(ScrnInfoPtr pScrn, unsigned long srcOffset,
                    unsigned srcPitch, unsigned w, unsigned h, unsigned srcX,
                    unsigned srcY, unsigned srcFormat, unsigned long dstOffset,
                    unsigned dstPitch, unsigned dstX, unsigned dstY,
                    unsigned dstFormat, int rotate)
{
    VIAPtr pVia = VIAPTR(pScrn);
    CARD32 wOrder, hOrder;
    Via3DState *v3d = &pVia->v3d;

    viaOrder(w, &wOrder);
    viaOrder(h, &hOrder);

    v3d->setDestination(v3d, dstOffset, dstPitch, dstFormat);
    v3d->setDrawing(v3d, 0x0c, 0xFFFFFFFF, 0x000000FF, 0x00);
    v3d->setFlags(v3d, 1, TRUE, TRUE, FALSE);
    v3d->setTexture(v3d, 0, srcOffset, srcPitch, TRUE,
                    1 << wOrder, 1 << hOrder, srcFormat,
                    via_single, via_single, via_src, FALSE);
    v3d->emitState(v3d, &pVia->cb, viaCheckUpload(pScrn, v3d));
    v3d->emitClipRect(v3d, &pVia->cb, dstX, dstY, w, h);
    v3d->emitQuad(v3d, &pVia->cb, dstX, dstY, srcX, srcY, 0, 0, w, h);
}