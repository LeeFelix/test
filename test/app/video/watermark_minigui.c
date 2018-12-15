#if USE_WATERMARK

#include "watermark_minigui.h"
#include <minigui/minigui.h>
#include <minigui/gdi.h>

#include "ui_resolution.h"

//argb(ตอ -> ธ฿)
static Uint32 RGB888_PALETTE_TABLE[PALETTE_TABLE_LEN] =
{
    0xffffffff, 0xd7ffffff, 0xafffffff, 0x87ffffff, 0x5fffffff, 0x00ffffff,
    0xffd7ffff, 0xd7d7ffff, 0xafd7ffff, 0x87d7ffff, 0x5fd7ffff, 0x00d7ffff,
    0xffafffff, 0xd7afffff, 0xafafffff, 0x87afffff, 0x5fafffff, 0x00afffff,
    0xff87ffff, 0xd787ffff, 0xaf87ffff, 0x8787ffff, 0x5f87ffff, 0x0087ffff,
    0xff5fffff, 0xd75fffff, 0xaf5fffff, 0x875fffff, 0x5f5fffff, 0x005fffff,
    0xff00ffff, 0xd700ffff, 0xaf00ffff, 0x8700ffff, 0x5f00ffff, 0x0000ffff,
    0xffffd7ff, 0xd7ffd7ff, 0xafffd7ff, 0x87ffd7ff, 0x5fffd7ff, 0x00ffd7ff,
    0xffd7d7ff, 0xd7d7d7ff, 0xafd7d7ff, 0x87d7d7ff, 0x5fd7d7ff, 0x00d7d7ff,
    0xffafd7ff, 0xd7afd7ff, 0xafafd7ff, 0x87afd7ff, 0x5fafd7ff, 0x00afd7ff,
    0xff87d7ff, 0xd787d7ff, 0xaf87d7ff, 0x8787d7ff, 0x5f87d7ff, 0x0087d7ff, //10
    0xff5fd7ff, 0xd75fd7ff, 0xaf5fd7ff, 0x875fd7ff, 0x5f5fd7ff, 0x005fd7ff,
    0xff00d7ff, 0xd700d7ff, 0xaf00d7ff, 0x8700d7ff, 0x5f00d7ff, 0x0000d7ff,
    0xffffafff, 0xd7ffafff, 0xafffafff, 0x87ffafff, 0x5fffafff, 0x00ffafff,
    0xffd7afff, 0xd7d7afff, 0xafd7afff, 0x87d7afff, 0x5fd7afff, 0x00d7afff,
    0xffafafff, 0xd7afafff, 0xafafafff, 0x87afafff, 0x5fafafff, 0x00afafff,
    0xff87afff, 0xd787afff, 0xaf87afff, 0x8787afff, 0x5f87afff, 0x0087afff,
    0xff5fafff, 0xd75fafff, 0xaf5fafff, 0x875fafff, 0x5f5fafff, 0x005fafff,
    0xff00afff, 0xd700afff, 0xaf00afff, 0x8700afff, 0x5f00afff, 0x0000afff,
    0xffff87ff, 0xd7ff87ff, 0xafff87ff, 0x87ff87ff, 0x5fff87ff, 0x00ff87ff,
    0xffd787ff, 0xd7d787ff, 0xafd787ff, 0x87d787ff, 0x5fd787ff, 0x00d787ff, //20
    0xffaf87ff, 0xd7af87ff, 0xafaf87ff, 0x87af87ff, 0x5faf87ff, 0x00af87ff,
    0xff8787ff, 0xd78787ff, 0xaf8787ff, 0x878787ff, 0x5f8787ff, 0x008787ff,
    0xff5f87ff, 0xd75f87ff, 0xaf5f87ff, 0x875f87ff, 0x5f5f87ff, 0x005f87ff,
    0xff0087ff, 0xd70087ff, 0xaf0087ff, 0x870087ff, 0x5f0087ff, 0x000087ff,
    0xffff5fff, 0xd7ff5fff, 0xafff5fff, 0x87ff5fff, 0x5fff5fff, 0x00ff5fff,
    0xffd75fff, 0xd7d75fff, 0xafd75fff, 0x87d75fff, 0x5fd75fff, 0x00d75fff,
    0xffaf5fff, 0xd7af5fff, 0xafaf5fff, 0x87af5fff, 0x5faf5fff, 0x00af5fff,
    0xff875fff, 0xd7875fff, 0xaf875fff, 0x87875fff, 0x5f875fff, 0x00875fff,
    0xff5f5fff, 0xd75f5fff, 0xaf5f5fff, 0x875f5fff, 0x5f5f5fff, 0x005f5fff,
    0xff005fff, 0xd7005fff, 0xaf005fff, 0x87005fff, 0x5f005fff, 0x00005fff, //30
    0xffff00ff, 0xd7ff00ff, 0xafff00ff, 0x87ff00ff, 0x5fff00ff, 0x00ff00ff,
    0xffd700ff, 0xd7d700ff, 0xafd700ff, 0x87d700ff, 0x5fd700ff, 0x00d700ff,
    0xffaf00ff, 0xd7af00ff, 0xafaf00ff, 0x87af00ff, 0x5faf00ff, 0x00af00ff,
    0xff8700ff, 0xd78700ff, 0xaf8700ff, 0x878700ff, 0x5f8700ff, 0x008700ff,
    0xff5f00ff, 0xd75f00ff, 0xaf5f00ff, 0x875f00ff, 0x5f5f00ff, 0x005f00ff,
    0xff0000ff, 0xd70000ff, 0xaf0000ff, 0x870000ff, 0x5f0000ff, 0x000000ff,
    0xeeeeeeff, 0xe4e4e4ff, 0xdadadaff, 0xd0d0d0ff, 0xc6c6c6ff, 0xbcbcbcff,
    0xb2b2b2ff, 0xa8a8a8ff, 0x9e9e9eff, 0x949494ff, 0x8a8a8aff, 0x767676ff,
    0x6c6c6cff, 0x626262ff, 0x585858ff, 0x4e4e4eff, 0x444444ff, 0x3a3a3aff,
    0x303030ff, 0x262626ff, 0x1c1c1cff, 0x121212ff, 0x080808ff, 0x000080ff, //40
    0x008000ff, 0x008080ff, 0x800000ff, 0x800080ff, 0x808000ff, 0xc0c0c0ff,
    0x808080ff, 0xffffccff, 0xccccccff, 0x99ccccff, 0x9999ccff, 0x3366ccff,
    0x0033ccff, 0x3300ccff, 0x00ffccff, 0xffffff00
};

//yuva(ตอ -> ธ฿)
Uint32 YUV444_PALETTE_TABLE[PALETTE_TABLE_LEN] =
{
    0xff8080eb, 0xff826ee9, 0xff835de6, 0xff854be4, 0xff863ae1, 0xff8a10db,
    0xff908ed2, 0xff927cd0, 0xff936acd, 0xff9559cb, 0xff9647c9, 0xff9a1ec3,
    0xffa09bba, 0xffa28ab7, 0xffa378b5, 0xffa566b2, 0xffa655b0, 0xffaa2baa,
    0xffb0a9a1, 0xffb1979f, 0xffb3859c, 0xffb5749a, 0xffb66297, 0xffba3991,
    0xffc0b689, 0xffc1a586, 0xffc39384, 0xffc58181, 0xffc6707f, 0xffca4679,
    0xffe6d64e, 0xffe7c54c, 0xffe9b349, 0xffeba247, 0xffec9044, 0xfff0663f,
    0xff6e84e4, 0xff7072e1, 0xff7261df, 0xff734fdc, 0xff753eda, 0xff7914d4,
    0xff7e92cb, 0xff8080c9, 0xff826ec6, 0xff835dc4, 0xff854bc1, 0xff8922bb,
    0xff8e9fb3, 0xff908eb0, 0xff927cae, 0xff936aab, 0xff9559a9, 0xff992fa3,
    0xff9ead9a, 0xffa09b98, 0xffa28a95, 0xffa37893, 0xffa56690, 0xffa93d8a, //10
    0xffaeba81, 0xffb0a97f, 0xffb1977c, 0xffb3857a, 0xffb57477, 0xffb94a72,
    0xffd4da47, 0xffd6c945, 0xffd7b742, 0xffd9a640, 0xffdb943d, 0xffde6a37,
    0xff5d88dc, 0xff5e76da, 0xff6065d7, 0xff6253d5, 0xff6342d2, 0xff6718cd,
    0xff6d96c4, 0xff6e84c1, 0xff7072bf, 0xff7261bc, 0xff734fba, 0xff7726b4,
    0xff7da3ab, 0xff7e92a9, 0xff8080a6, 0xff826ea4, 0xff835da1, 0xff87339b,
    0xff8db193, 0xff8e9f90, 0xff908e8e, 0xff927c8b, 0xff936a89, 0xff974183,
    0xff9dbe7a, 0xff9ead78, 0xffa09b75, 0xffa28a73, 0xffa37870, 0xffa74e6a,
    0xffc3de40, 0xffc4cd3d, 0xffc6bb3b, 0xffc7aa38, 0xffc99836, 0xffcd6e30,
    0xff4b8cd5, 0xff4d7bd3, 0xff4f69d0, 0xff5057ce, 0xff5246cb, 0xff561cc5,
    0xff5b9abd, 0xff5d88ba, 0xff5e76b8, 0xff6065b5, 0xff6253b3, 0xff662aad, //20
    0xff6ba7a4, 0xff6d96a1, 0xff6e849f, 0xff70729d, 0xff72619a, 0xff753794,
    0xff7bb58b, 0xff7da389, 0xff7e9286, 0xff808084, 0xff826e81, 0xff85457c,
    0xff8bc273, 0xff8db170, 0xff8e9f6e, 0xff908e6b, 0xff927c69, 0xff955263,
    0xffb1e238, 0xffb3d136, 0xffb4bf34, 0xffb6ae31, 0xffb79c2f, 0xffbb7229,
    0xff3a90ce, 0xff3b7fcb, 0xff3d6dc9, 0xff3f5bc6, 0xff404ac4, 0xff4420be,
    0xff4a9eb5, 0xff4b8cb3, 0xff4d7bb0, 0xff4f69ae, 0xff5057ab, 0xff542ea5,
    0xff5aab9d, 0xff5b9a9a, 0xff5d8898, 0xff5e7695, 0xff606593, 0xff643b8d,
    0xff6ab984, 0xff6ba782, 0xff6d967f, 0xff6e847d, 0xff70727a, 0xff744974,
    0xff7ac66c, 0xff7bb569, 0xff7da367, 0xff7e9264, 0xff808062, 0xff84565c,
    0xff9fe631, 0xffa1d52f, 0xffa3c32c, 0xffa4b22a, 0xffa6a027, 0xffaa7621,
    0xff109abc, 0xff1288ba, 0xff1377b7, 0xff1565b5, 0xff1653b3, 0xff1a2aad,
    0xff20a7a4, 0xff2296a1, 0xff23849f, 0xff25729c, 0xff26619a, 0xff2a3794,
    0xff30b58b, 0xff32a389, 0xff339286, 0xff358084, 0xff366e81, 0xff3a457b,
    0xff40c273, 0xff41b170, 0xff439f6e, 0xff458e6b, 0xff467c69, 0xff4a5263,
    0xff50d05a, 0xff51be58, 0xff53ad55, 0xff559b53, 0xff568a50, 0xff5a604a,
    0xff76f020, 0xff77de1d, 0xff79cd1b, 0xff7bbb18, 0xff7caa16, 0xff808010,
    0xff8080dc, 0xff8080d4, 0xff8080cb, 0xff8080c3, 0xff8080ba, 0xff8080b1,
    0xff8080a9, 0xff8080a0, 0xff808098, 0xff80808f, 0xff808087, 0xff808075,
    0xff80806d, 0xff808064, 0xff80805c, 0xff808053, 0xff80804a, 0xff808042,
    0xff808039, 0xff808031, 0xff808028, 0xff80801f, 0xff808017, 0xffb87327,
    0xff4d555f, 0xff854876, 0xff7bb818, 0xffb3ab2f, 0xff488d67, 0xff8080b5,
    0xff80807e, 0xff6a85e2, 0xff8080bf, 0xff826abc, 0xff967b9d, 0xffaf5f77,
    0xffc55a55, 0xffd88238, 0xff7415d2, 0x008080eb
};

extern BITMAP watermark_bmap[7];
static HWND hWnd = 0;
static HDC hdc = 0;

void watermark_minigui_setHWND(HWND wnd)
{
	hWnd = wnd;
}

void watermark_minigui_setHDC(HDC dc)
{
	hdc = dc;
}

//Match an RGB value to a particular palette index
static Uint8 find_color(Uint32 *pal, Uint32 len, Uint8 r, Uint8 g, Uint8 b)
{
    /* Do colorspace distance matching */
    unsigned int smallest;
    unsigned int distance;
    int rd, gd, bd;
    Uint8 rp, gp, bp;
    int i;
    Uint8 pixel=0;

    smallest = ~0;
    for (i = 0; i < len; ++i) {
        bp = (pal[i] & 0xff000000) >> 24;
        gp = (pal[i] & 0x00ff0000) >> 16;
        rp = (pal[i] & 0x0000ff00) >> 8;

        rd = rp - r;
        gd = gp - g;
        bd = bp - b;

        distance = (rd*rd)+(gd*gd)+(bd*bd);
        if ( distance < smallest ) {
            pixel = i;
            if ( distance == 0 ) { /* Perfect match! */
                break;
            }
            smallest = distance;
        }
    }
    return(pixel);
}

void get_logo_data(Uint8* dst, Uint32 logoIndex, WATERMARK_RECT rect, Uint32 flag, Uint16 rgb565_pixel)
{
    int i, j;
    gal_pixel pixel;
    RGB rgb = {0};

    HDC hdc = BeginPaint(hWnd);

    if (dst == NULL){
        printf("%s error, dst buf is NULL\n", __func__);
        EndPaint(hWnd, hdc);
        return;
    }

    if(watermark_bmap[logoIndex].bmBits == NULL){
        printf("%s error, watermark_bmap is NULL, bmpIndex = %d\n", __func__, logoIndex);
        EndPaint(hWnd, hdc);
        return;
    }

    for(j=0; j<watermark_bmap[logoIndex].bmHeight; j++)
        for(i=0; i<watermark_bmap[logoIndex].bmWidth; i++){
            pixel = GetPixelInBitmap (&watermark_bmap[logoIndex], i, j);

            if(flag && ((Uint16)pixel == rgb565_pixel)) {
                    dst[i] = PALETTE_TABLE_LEN - 1;
            } else {
                Pixel2RGBAs(hdc, &pixel, &rgb, 1);
                if(rgb.a == 0){
                    dst[i + j * rect.width] = PALETTE_TABLE_LEN - 1;
                }
                else
                    dst[i + j * rect.width] = find_color(RGB888_PALETTE_TABLE, PALETTE_TABLE_LEN, rgb.r, rgb.g, rgb.b);
            }
        }

    EndPaint(hWnd, hdc);
}

static void get_rectBmp_data(Uint8* dst, BITMAP* bmp, Uint16 textColor, WATERMARK_RECT rect)
{
    int i, j;
    gal_pixel pixel;
    RGB rgb = {0};
    Uint8 textIndex;

    if (dst == NULL){
        printf("%s error, dst buf is NULL\n", __func__);
        return;
    }

    rgb.r = (textColor>>8)&0x00f8;
    rgb.g = (textColor>>3)&0x00fc;
    rgb.b = (textColor<<3)&0x00f8;

    textIndex = find_color(RGB888_PALETTE_TABLE, PALETTE_TABLE_LEN, rgb.r, rgb.g, rgb.b);

    for(j=0; j<bmp->bmHeight; j++)
        for(i=0; i<bmp->bmWidth; i++){
            pixel = GetPixelInBitmap (bmp, i, j);
            if((Uint16)pixel == textColor)
                dst[i + j * rect.width] = textIndex;
            else
                dst[i + j * rect.width] = PALETTE_TABLE_LEN - 1;
        }

}

void update_rectBmp(WATERMARK_RECT srcRect, WATERMARK_RECT dstRect, Uint8* dst)
{
    Uint32 x, y;
    Uint32 imagesize, pitch;
    BITMAP bmp = {0}, scaledBmp = {0};

    Uint16 textColor = GetTextColor(hdc);

    bmp.bmBits = NULL;
    scaledBmp.bmBits = NULL;

    if (dst == NULL){
        printf("%s error, dst buf is NULL\n", __func__);
        return;
    }

    if(!GetBitmapFromDC(hdc, srcRect.x, srcRect.y, srcRect.width, srcRect.height, &bmp))
    {
        printf("%s get time watermark error\n", __func__);
        goto free_ret;
    }

    if((bmp.bmWidth != dstRect.w) || (bmp.bmHeight != dstRect.h)){
        imagesize = GetBoxSize (hdc, dstRect.w, dstRect.h, &pitch);

        if(imagesize <= 0)
           goto free_ret;

        scaledBmp.bmType = bmp.bmType;
        scaledBmp.bmBytesPerPixel = bmp.bmBytesPerPixel;
        scaledBmp.bmWidth = dstRect.w;
        scaledBmp.bmHeight = dstRect.h;
        scaledBmp.bmPitch = pitch;
        scaledBmp.bmBits = malloc (imagesize);

        if (scaledBmp.bmBits == NULL)
            goto free_ret;

        ScaleBitmap (&scaledBmp, &bmp);
        get_rectBmp_data(dst, &scaledBmp, textColor, dstRect);

    } else {
        get_rectBmp_data(dst, &bmp, textColor, dstRect);
    }

free_ret:
    if(bmp.bmBits != NULL){
        free(bmp.bmBits);
        bmp.bmBits = NULL;
    }

    if(bmp.bmAlphaMask != NULL){
        free(bmp.bmAlphaMask);
        bmp.bmAlphaMask = NULL;
    }

    if(scaledBmp.bmBits != NULL){
        free(scaledBmp.bmBits);
        scaledBmp.bmBits = NULL;
    }

    if(scaledBmp.bmAlphaMask != NULL){
        free(scaledBmp.bmAlphaMask);
        scaledBmp.bmAlphaMask = NULL;
    }
}
#endif