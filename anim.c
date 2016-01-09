
/*
**
**  $VER: anim.c 1.12 (12.11.97)
**  anim.datatype 1.12
**
*/

#define DEBUG 1
#include <aros/debug.h>

struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"

LONG LoadILBMBody( struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *data, ULONG len )
{
    const BYTE *in = (const BYTE *)data;
    //const BYTE *end = in + len;
    // The mask plane is interleaved after the bitmap planes, so we need to count
    // it as another plane when reading.
    int nplanes = bmh->bmh_Depth + (bmh->bmh_Masking == mskHasMask);
    int pitch = bm->BytesPerRow;
    int out = 0;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (int y = 0; y < bmh->bmh_Height; ++y)
    {
        for (int p = 0; p < nplanes; ++p)
        {
            if (p < bmh->bmh_Depth)
            {
                // Read data into bitplane
                if (bmh->bmh_Compression == cmpNone)
                {
                    memcpy(&bm->Planes[p][out], in, pitch);
                    in += pitch;
                }
                else for (int ofs = 0; ofs < pitch;)
                {
                    if (*in >= 0)
                    {
                        memcpy(&bm->Planes[p][out + ofs], in + 1, *in + 1);
                        ofs += *in + 1;
                        in += *in + 2;
                    }
                    else
                    {
                        memset(&bm->Planes[p][out + ofs], in[1], -*in + 1);
                        ofs += -*in + 1;
                        in += 2;
                    }
                }
            }
            else
            {
                // This is the mask plane. Skip over it.
                if (bmh->bmh_Compression == cmpNone)
                {
                    in += pitch;
                }
                else for (int ofs = 0; ofs < pitch;)
                {
                    if (*in >= 0)
                    {
                        ofs += *in + 1;
                        in += *in + 2;
                    }
                    else
                    {
                        ofs += -*in + 1;
                        in += 2;
                    }
                }
            }
        }
        out += pitch;
    }

    return 0;
}

LONG unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

#if defined(COMMENTED_OUT)
LONG unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

#endif
LONG unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    // ILBMs are only padded to 16 pixel widths, so what happens when the image
    // needs to be padded to 32 pixels for long data but isn't? The spec doesn't say.
    const ULONG *lists = (const ULONG *)((IPTR)dlta + 4);
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 15) / 32;
    int pitch = bm->BytesPerRow;
    const ULONG xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFFFF : 0x00;
    ULONG opptr;
    const ULONG *data;
    ULONG *pixels;
    ULONG *stop;
    const UBYTE *ops;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    D(bug("[anim.datatype] %s: dlta @ 0x%p\n", __PRETTY_FUNCTION__, dlta));
    D(bug("[anim.datatype] %s: lists @ 0x%p\n", __PRETTY_FUNCTION__, lists));
    D(bug("[anim.datatype] %s: xormask %04x\n", __PRETTY_FUNCTION__, xormask));

    for (int p = 0; p < bm->Depth; ++p)
    {
        D(bug("[anim.datatype] %s:   plane #%d\n", __PRETTY_FUNCTION__, p));
        opptr = AROS_BE2LONG(lists[p]);
        if (opptr == 0)
        { // No ops for this plane.
                continue;
        }
        data = (const ULONG *)((const UBYTE *)dlta + AROS_BE2LONG(lists[p + 8]));
        ops = (const UBYTE *)dlta + opptr;
        for (int x = 0; x < numcols; ++x)
        {
            pixels = (ULONG *)bm->Planes[p] + x;
            stop = (ULONG *)((UBYTE *)pixels + GetBitMapAttr( bm, BMA_HEIGHT) * pitch);
            UBYTE opcount = *ops++;
            while (opcount-- > 0)
            {
                UBYTE op = *ops++;
                if (op & 0x80)
                { // Uniq op: copy data literally
                    UBYTE cnt = op & 0x7F;
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = AROS_LONG2BE((AROS_BE2LONG(*pixels) & xormask) ^ AROS_BE2LONG(*data));
                            pixels = (ULONG *)((UBYTE *)pixels + pitch);
                        }
                        data++;
                    }
                }
                else if (op == 0)
                { // Same op: copy one byte to several rows
                    UBYTE cnt = *ops++;
                    ULONG fill = AROS_BE2LONG(*data++);
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = AROS_LONG2BE((AROS_BE2LONG(*pixels) & xormask) ^ fill);
                            pixels = (ULONG *)((UBYTE *)pixels + pitch);
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixels = (ULONG *)((UBYTE *)pixels + op * pitch);
                }
            }
        }
    }

    return 0;
}

LONG unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 15) / 16;
    int pitch = bm->BytesPerRow / 2;
    const UWORD xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFFFF : 0x00;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (int p = 0; p < bm->Depth; ++p)
    {
        ULONG opptr = AROS_BE2LONG(lists[p]);
        if (opptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *data = (const UWORD *)((const UBYTE *)dlta + AROS_BE2LONG(lists[p + 8]));
        const UBYTE *ops = (const UBYTE *)dlta + opptr;
        for (int x = 0; x < numcols; ++x)
        {
            UWORD *pixels = (UWORD *)bm->Planes[p] + x;
            UWORD *stop = pixels + GetBitMapAttr( bm, BMA_HEIGHT) * pitch;
            UBYTE opcount = *ops++;
            while (opcount-- > 0)
            {
                UBYTE op = *ops++;
                if (op & 0x80)
                { // Uniq op: copy data literally
                    UBYTE cnt = op & 0x7F;
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = (*pixels & xormask) ^ *data;
                            pixels += pitch;
                        }
                        data++;
                    }
                }
                else if (op == 0)
                { // Same op: copy one byte to several rows
                    UBYTE cnt = *ops++;
                    UWORD fill = *data++;
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = (*pixels & xormask) ^ fill;
                            pixels += pitch;
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixels += op * pitch;
                }
            }
        }
    }

    return 0;
}

static const UWORD *Do8short(UWORD *pixel, UWORD *stop, const UWORD *ops, UWORD xormask, int pitch)
{
    UWORD opcount = AROS_BE2WORD(*ops++);

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    while (opcount-- > 0)
    {
        UWORD op = AROS_BE2WORD(*ops++);
        if (op & 0x8000)
        { // Uniq op: copy data literally
            UWORD cnt = op & 0x7FFF;
            while (cnt-- > 0)
            {
                if (pixel < stop)
                {
                    *pixel = (*pixel & xormask) ^ *ops;
                    pixel += pitch;
                }
                ops++;
            }
        }
        else if (op == 0)
        { // Same op: copy one byte to several rows
            UWORD cnt = AROS_BE2WORD(*ops++);
            UWORD fill = *ops++;
            while (cnt-- > 0)
            {
                if (pixel < stop)
                {
                    *pixel = (*pixel & xormask) ^ fill;
                    pixel += pitch;
                }
            }
        }
        else
        { // Skip op: Skip some rows
            pixel += op * pitch;
        }
    }
    return ops;
}

LONG unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 31) / 32;
    int pitch = bm->BytesPerRow;
    BOOL lastisshort = (GetBitMapAttr( bm, BMA_WIDTH) & 16) != 0;
    const UWORD xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFF : 0x00;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (int p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if (ptr == 0)
        { // No ops for this plane.
            continue;
        }
        const ULONG *ops = (const ULONG *)dlta + ptr;
        for (int x = 0; x < numcols; ++x)
        {
            ULONG *pixel = (ULONG *)(bm->Planes[p] + x);
            ULONG *stop = (ULONG *)((UBYTE *)pixel + GetBitMapAttr( bm, BMA_HEIGHT) * pitch);
            if (x == numcols - 1 && lastisshort)
            {
                    Do8short((UWORD *)pixel, (UWORD *)stop, (UWORD *)ops, xormask, pitch / 2);
                    continue;
            }
            ULONG opcount = AROS_BE2LONG(*ops++);
            while (opcount-- > 0)
            {
                ULONG op = AROS_BE2LONG(*ops++);
                if (op & 0x80000000)
                { // Uniq op: copy data literally
                    ULONG cnt = op & 0x7FFFFFFF;
                    while (cnt-- > 0)
                    {
                        if (pixel < stop)
                        {
                            *pixel = (*pixel & xormask) ^ *ops;
                            pixel = (ULONG *)((UBYTE *)pixel + pitch);
                        }
                        ops++;
                    }
                }
                else if (op == 0)
                { // Same op: copy one byte to several rows
                    ULONG cnt = AROS_BE2LONG(*ops++);
                    ULONG fill = *ops++;
                    while (cnt-- > 0)
                    {
                        if (pixel < stop)
                        {
                            *pixel = (*pixel & xormask) ^ fill;
                            pixel = (ULONG *)((UBYTE *)pixel + pitch);
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixel = (ULONG *)((UBYTE *)pixel + op * pitch);
                }
            }
        }
    }

    return 0;
}

LONG unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 15) / 16;
    int pitch = bm->BytesPerRow / 2;
    const UWORD xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFF : 0x00;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (int p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if (ptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *ops = (const UWORD *)dlta + ptr;
        for (int x = 0; x < numcols; ++x)
        {
            UWORD *pixel = (UWORD *)(bm->Planes[p] + x);
            UWORD *stop = pixel + GetBitMapAttr( bm, BMA_HEIGHT) * pitch;
            ops = Do8short(pixel, stop, ops, xormask, pitch);
        }
    }

    return 0;
}