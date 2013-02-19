#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/*
 * Error data structure.
 *      REPORT_NULL
 *      REPORT_ARGS
 *      REPORT_READ
 *      REPORT_ALLOC
 *      REPORT_OPEN
 *      REPORT_CLOSE
 */
 #define DEFINE_REPORTS                                      \
    NEW_REPORT(REPORT_NULL,  "No error.")                    \
    NEW_REPORT(REPORT_ARGS,  "Bad arguments.")               \
    NEW_REPORT(REPORT_READ,  "Reading error.")               \
    NEW_REPORT(REPORT_ALLOC, "Dynamic allocation fail.")     \
    NEW_REPORT(REPORT_OPEN,  "Unable to open a file.")       \
    NEW_REPORT(REPORT_CLOSE, "Unable to close a file.")

enum report
{
#define NEW_REPORT(s1, s2) s1,
    DEFINE_REPORTS
    REPORT_MASK
#undef NEW_REPORT
};

static const char *const description[REPORT_MASK] =
{
#define NEW_REPORT(s1, s2) s2,
    DEFINE_REPORTS
#undef NEW_REPORT
};

/*
 * Bitmap data structure (please refer to MSDN documentation).
 *      0x00 - ftype:        Header field.                           - "BM"
 *      0x02 - fsize:        Size of the BMP format (in bytes).      - /
 *      0x06 - freserved:    Reserved field (unused here).           - 0x00
 *      0x0a - foffset:      Starting address of the bitmap image.   - 0x36
 *      0x0e - iheadersize:  Size of this header.                    - 0x28
 *      0x12 - iwidth:       Bitmap width (in pixels).               - /
 *      0x16 - iheight:      Bitmap height (in pixels).              - /
 *      0x1a - iplanes:      Number of color planes.                 - 0x01
 *      0x1c - idepth:       Number of bits per pixel.               - 0x18
 *      0x1e - icompression: Compression method.                     - 0x01
 *      0x22 - isize:        Image size.                             - /
 *      0x26 - ihorizontal:  Horizontal resolution.                  - 0x00
 *      0x2a - ivertical:    Vertical resolution.                    - 0x00
 *      0x2e - icolors:      Number of colors.                       - 0x00
 *      0x32 - iimpcolors:   Number of important colors.             - 0x00
 */
struct bitmap
{
    char           ftype[2];
    uint32_t       fsize;
    uint32_t       freserved;
    uint32_t       foffset;
    uint32_t       iheadersize;
    int32_t        iwidth;
    int32_t        iheight;
    uint16_t       iplanes;
    uint16_t       idepth;
    uint32_t       icompression;
    uint32_t       isize;
    int32_t        ihorizontal;
    int32_t        ivertical;
    uint32_t       icolors;
    uint32_t       iimpcolors;
    unsigned char *data;
};

/*
 * Writes the data c of size n to the stream f. If a reading error occurs,
 * jump to the label l.
 */
#define BMP_WRITE(c, n, f, l)                   \
    do                                          \
    {                                           \
        if (fwrite((c), (n), 1, f) < 1)         \
        {                                       \
            res = REPORT_READ;                  \
            goto l;                             \
        }                                       \
    } while (0)

/*
 * Converts the position n in the image array data structure to an offset
 * in bytes, taking in account the number of bits per pixel p.
 */
#define BMP_CONVERT(p, n)                       \
    (n * ((p) / CHAR_BIT))

/*
 * Rounds n to the next multiple of 4.
 */
#define BMP_ROUND(n)                            \
    (((n + 3) / 4) * 4)

/*
 * Creates a new bitmap data structure p with a given width and height.
 * Return value:
 *      REPORT_NULL
 *      REPORT_ALLOC
 *      REPORT_ARGS
 */
static enum report create(struct bitmap *const p,
                          const uint32_t width,
                          const uint32_t height)
{
    enum report res = REPORT_NULL;

    if (p != NULL)
    {
        p->ftype[0]     = 'B';
        p->ftype[1]     = 'M';
        p->freserved    = UINT32_C(0x00);
        p->foffset      = UINT32_C(0x36);
        p->idepth       = UINT16_C(0x18);
        p->isize        = height * BMP_ROUND(BMP_CONVERT(p->idepth, width));
        p->fsize        = p->foffset + p->isize;
        p->iheadersize  = UINT32_C(0x28);
        p->iwidth       = width;
        p->iheight      = height;
        p->iplanes      = UINT16_C(0x01);
        p->icompression = UINT32_C(0x00);
        p->ihorizontal  = INT32_C(0x00);
        p->ivertical    = INT32_C(0x00);
        p->icolors      = UINT32_C(0x00);
        p->iimpcolors   = UINT32_C(0x00);
        p->data         = malloc(p->isize);

        if (p->data != NULL)
        {
            memset(p->data, 0xff, p->isize);
        }
        else
        {
            res = REPORT_ALLOC;
        }
    }
    else
    {
        res = REPORT_ARGS;
    }

    return res;
}

/*
 * Saves the bitmap data structure p on the stream dst. Avoid padding bytes
 * being also written.
 * /!\ This is definitive. Do not re-use the bitmap after such call.
 * Return value:
 *      REPORT_NULL
 *      REPORT_ARGS
 */
static enum report save(const struct bitmap *const p,
                        FILE *const dst)
{
    enum report res = REPORT_NULL;

    if (p != NULL && dst != NULL)
    {
        BMP_WRITE(&p->ftype,        sizeof p->ftype,        dst, exit);
        BMP_WRITE(&p->fsize,        sizeof p->fsize,        dst, exit);
        BMP_WRITE(&p->freserved,    sizeof p->freserved,    dst, exit);
        BMP_WRITE(&p->foffset,      sizeof p->foffset,      dst, exit);
        BMP_WRITE(&p->iheadersize,  sizeof p->iheadersize,  dst, exit);
        BMP_WRITE(&p->iwidth,       sizeof p->iwidth,       dst, exit);
        BMP_WRITE(&p->iheight,      sizeof p->iheight,      dst, exit);
        BMP_WRITE(&p->iplanes,      sizeof p->iplanes,      dst, exit);
        BMP_WRITE(&p->idepth,       sizeof p->idepth,       dst, exit);
        BMP_WRITE(&p->icompression, sizeof p->icompression, dst, exit);
        BMP_WRITE(&p->isize,        sizeof p->isize,        dst, exit);
        BMP_WRITE(&p->ihorizontal,  sizeof p->ihorizontal,  dst, exit);
        BMP_WRITE(&p->ivertical,    sizeof p->ivertical,    dst, exit);
        BMP_WRITE(&p->icolors,      sizeof p->icolors,      dst, exit);
        BMP_WRITE(&p->iimpcolors,   sizeof p->iimpcolors,   dst, exit);
        BMP_WRITE(p->data,          p->isize,               dst, exit);
        free(p->data);
    }
    else
    {
        res = REPORT_ARGS;
    }

exit:
    return res;
}

/*
 * Sets the pixel at column y and line x with the color given by the value of
 * red, green and blue in the bitmap p.
 * Return value:
 *      REPORT_NULL
 *      REPORT_ARGS
 */
static enum report putpixel(const struct bitmap *const p,
                            const uint32_t x,
                            const uint32_t y,
                            const unsigned char red,
                            const unsigned char green,
                            const unsigned char blue)
{
    enum report res = REPORT_NULL;

    if (p != NULL && x < (uint32_t)p->iwidth && y < (uint32_t)p->iheight)
    {
        unsigned char *const dst =
            p->data
            + BMP_CONVERT(p->idepth, x)
            + BMP_ROUND(BMP_CONVERT(p->idepth, p->iwidth)) * y;

        dst[0] = blue;
        dst[1] = green;
        dst[2] = red;
    }
    else
    {
        res = REPORT_ARGS;
    }

    return res;
}

/*
 * Draws a square of a given size at column x and line y in the bitmap p, with
 * the color incated by red, green and blue.
 * Return value:
 *      REPORT_NULL
 *      REPORT_ARGS
 */
static enum report draw(const struct bitmap *const p,
                        const uint32_t x,
                        const uint32_t y,
                        const uint32_t size,
                        const unsigned char red,
                        const unsigned char green,
                        const unsigned char blue)
{
    enum report res = REPORT_NULL;

    if (p != NULL)
    {
        uint32_t i;
        uint32_t j;

        for (i = UINT32_C(0); i < size; i++)
        {
            for (j = UINT32_C(0); j < size; j++)
            {
                res = putpixel(p, x + i, y + j, red, green, blue);
            }
        }
    }
    else
    {
        res = REPORT_ARGS;
    }

    return res;
}

/*
 * Performs saturated addition on op1 and op2.
 * Return value: 
 *     result of the operation.
 */
static unsigned char add(const unsigned char op1,
                         const unsigned char op2)
{
    unsigned char res = op1 + op2;

    if (res < op1 || res < op2)
    {
        res = UCHAR_MAX;
    }

    return res;
}

/*
 * Prints on the standard error stream stderr the description of the error
 * indicated by res.
 */
static void notice(const enum report res)
{
    if (res > 0 && res < REPORT_MASK)
    {
        fprintf(stderr, "Error: %s\n", description[res]);
    }
}

/*
 * This program generates avatars randomly. We assume a little-endian
 * architecture.
 * usage :
 *      ./gen image width height unit variation
 *      ./gen
 */
int main(int argc, char *argv[])
{
    enum report   error;
    FILE         *dst;
    int           res    = EXIT_SUCCESS;
    int32_t       width  = INT32_C(100);
    int32_t       height = INT32_C(100);
    int32_t       square = INT32_C(10);
    unsigned char maxvar = INT32_C(30);
    char         *path   = "default.bmp";

    if (argc > 5)
    {
        path   = argv[1];
        width  = atoi(argv[2]);
        height = atoi(argv[3]);
        square = atoi(argv[4]);
        maxvar = atoi(argv[5]);
    }

    srand((unsigned int)time(NULL));

    if ((dst = fopen(path, "wb")) != NULL)
    {
        struct bitmap src;

        if ((error = create(&src, width, height)) == REPORT_NULL)
        {
            unsigned char red   = rand();
            unsigned char green = rand();
            unsigned char blue  = rand();
            int32_t i;
            int32_t j;

            for (i = INT32_C(0); i < width; i += square)
            {
                for (j = INT32_C(0); j < height; j += square)
                {
                    unsigned char var = rand() % maxvar;

                    if ((error = draw(&src, i, j, square, add(red, var),
                                      add(blue, var), add(green, var)))
                         != REPORT_NULL)
                    {
                        notice(error);
                        res = EXIT_FAILURE;
                        goto exit;
                    }
                }
            }

            if ((error = save(&src, dst)) == REPORT_NULL)
            {
                if (fclose(dst) == EOF)
                {
                    notice(REPORT_CLOSE);
                    res = EXIT_FAILURE;
                }
            }
            else
            {
                notice(error);
                res = EXIT_FAILURE;
            }
        }
        else
        {
            notice(error);
            res = EXIT_FAILURE;
        }
    }
    else
    {
        notice(REPORT_OPEN);
        res = EXIT_FAILURE;
    }

exit:
    return res;
}
