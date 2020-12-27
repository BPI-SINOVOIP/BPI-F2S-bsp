// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2015 Google, Inc
 */

#include <common.h>
#include <bmp_layout.h>
#include <dm.h>
#include <mapmem.h>
#include <splash.h>
#include <video.h>
#include <watchdog.h>
#include <asm/unaligned.h>

//#define debug printf

#define DISP_SWAP16(x)		((((x) & 0x00ff) << 8) | \
				  ((x) >> 8) \
				)
#define DISP_SWAP32(x)		((((x) & 0x000000ff) << 24) | \
				 (((x) & 0x0000ff00) <<  8) | \
				 (((x) & 0x00ff0000) >>  8) | \
				 (((x) & 0xff000000) >> 24)   \
				)

#define VIDEO_CMAP_OFFSET 16

#ifdef CONFIG_VIDEO_BMP_RLE8
#define BMP_RLE8_ESCAPE		0
#define BMP_RLE8_EOL		0
#define BMP_RLE8_EOBMP		1
#define BMP_RLE8_DELTA		2

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
static void draw_unencoded_bitmap(uchar **fbp, uchar *bmap, u32 *cmap,
#else
static void draw_unencoded_bitmap(ushort **fbp, uchar *bmap, ushort *cmap,
#endif
				  int cnt)
{
	while (cnt > 0) {
		#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
		//*(*fbp)++ = *bmap++;
		*(*fbp) = (*bmap) + VIDEO_CMAP_OFFSET;
		(*fbp)++;
		bmap++;	
		#else
		*(*fbp)++ = cmap[*bmap++];
		#endif
		cnt--;
	}
}

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
static void draw_encoded_bitmap(uchar **fbp, uchar col, int cnt)
#else
static void draw_encoded_bitmap(ushort **fbp, ushort col, int cnt)
#endif
{
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	uchar *fb = *fbp;
#else	
	ushort *fb = *fbp;
#endif

	while (cnt > 0) {
		#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
		*fb++ = col + VIDEO_CMAP_OFFSET;
		#else
		*fb++ = col;
		#endif
		cnt--;
	}
	*fbp = fb;
}

static void video_display_rle8_bitmap(struct udevice *dev,
	#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
				      struct bmp_image *bmp, u32 *cmap,
	#else				      
				      struct bmp_image *bmp, ushort *cmap,
	#endif      
				      uchar *fb, int x_off, int y_off)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	uchar *bmap;
	ulong width, height;
	ulong cnt, runlen;
	int x, y;
	int decode = 1;

	debug("%s\n", __func__);
	width = get_unaligned_le32(&bmp->header.width);
	height = get_unaligned_le32(&bmp->header.height);
	bmap = (uchar *)bmp + get_unaligned_le32(&bmp->header.data_offset);

	x = 0;
	y = height - 1;

	while (decode) {
		if (bmap[0] == BMP_RLE8_ESCAPE) {
			switch (bmap[1]) {
			case BMP_RLE8_EOL:
				/* end of line */
				bmap += 2;
				x = 0;
				y--;
		#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
				/* 8bpix, 1-byte per pixel, width should *1 */
				fb -= (width * 1 + priv->line_length);	
		#else
				/* 16bpix, 2-byte per pixel, width should *2 */
				fb -= (width * 2 + priv->line_length);
		#endif
				break;
			case BMP_RLE8_EOBMP:
				/* end of bitmap */
				decode = 0;
				break;
			case BMP_RLE8_DELTA:
				/* delta run */
				x += bmap[2];
				y -= bmap[3];
		#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
				/* 8bpix, 1-byte per pixel, x should *1 */
				fb = (uchar *)(priv->fb + (y + y_off - 1)
					* priv->line_length + (x + x_off) * 1);
		#else
				/* 16bpix, 2-byte per pixel, x should *2 */
				fb = (uchar *)(priv->fb + (y + y_off - 1)
					* priv->line_length + (x + x_off) * 2);
		#endif
				bmap += 4;
				break;
			default:
				/* unencoded run */
				runlen = bmap[1];
				bmap += 2;
				if (y < height) {
					if (x < width) {
						if (x + runlen > width)
							cnt = width - x;
						else
							cnt = runlen;
						draw_unencoded_bitmap(
			#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
							(uchar **)&fb,
			#else	
							(ushort **)&fb,
			#endif
							bmap, cmap, cnt);
					}
					x += runlen;
				}
				bmap += runlen;
				if (runlen & 1)
					bmap++;
			}
		} else {
			/* encoded run */
			if (y < height) {
				runlen = bmap[0];
				if (x < width) {
					/* aggregate the same code */
					while (bmap[0] == 0xff &&
					       bmap[2] != BMP_RLE8_ESCAPE &&
					       bmap[1] == bmap[3]) {
						runlen += bmap[2];
						bmap += 2;
					}
					if (x + runlen > width)
						cnt = width - x;
					else
						cnt = runlen;
			#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
					draw_encoded_bitmap((uchar **)&fb,
			#else
					draw_encoded_bitmap((ushort **)&fb,
			#endif
						cmap[bmap[1]], cnt);
				}
				x += runlen;
			}
			bmap += 2;
		}
	}
}
#endif

__weak void fb_put_byte(uchar **fb, uchar **from)
{
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	*(*fb) = *(*from) + VIDEO_CMAP_OFFSET;
	(*fb)++;
	(*from)++;
#else
	*(*fb)++ = *(*from)++;
#endif
}

#if defined(CONFIG_BMP_16BPP)
__weak void fb_put_word(uchar **fb, uchar **from)
{
	*(*fb)++ = *(*from)++;
	*(*fb)++ = *(*from)++;
}
#endif /* CONFIG_BMP_16BPP */

/**
 * video_splash_align_axis() - Align a single coordinate
 *
 *- if a coordinate is 0x7fff then the image will be centred in
 *  that direction
 *- if a coordinate is -ve then it will be offset to the
 *  left/top of the centre by that many pixels
 *- if a coordinate is positive it will be used unchnaged.
 *
 * @axis:	Input and output coordinate
 * @panel_size:	Size of panel in pixels for that axis
 * @picture_size:	Size of bitmap in pixels for that axis
 */
static void video_splash_align_axis(int *axis, unsigned long panel_size,
				    unsigned long picture_size)
{
	unsigned long panel_picture_delta = panel_size - picture_size;
	unsigned long axis_alignment;

	if (*axis == BMP_ALIGN_CENTER)
		axis_alignment = panel_picture_delta / 2;
	else if (*axis < 0)
		axis_alignment = panel_picture_delta + *axis + 1;
	else
		return;

	*axis = max(0, (int)axis_alignment);
}

static void video_set_cmap(struct udevice *dev,
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
			   struct bmp_color_table_entry *cte, unsigned colours, int color_used, int pal_sel)
#else
			   struct bmp_color_table_entry *cte, unsigned colours)
#endif
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
	int i;
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	u32 *cmap = priv->cmap;
#else
	ushort *cmap = priv->cmap;
#endif

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	debug("%s: colours=%d, color_used=%d\n", __func__, colours, color_used);
	
	if (pal_sel) {
		debug("set grey scale palette! \n");
	}
	else {
		debug("load bmp palette! \n");
	}
#else
	debug("%s: colours=%d\n", __func__, colours);
#endif

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	cmap += VIDEO_CMAP_OFFSET;
	for (i = VIDEO_CMAP_OFFSET; i < colours; ++i) {
		if (pal_sel) {
			//Palette is ARGB8888 with 256 grey scale
			if (i < VIDEO_CMAP_OFFSET) {
				;//*cmap = 0x000000ff;
			} else if (i > 235) {
				*cmap = 0xffffffff;
			} else {
				*cmap = (((u32)(0xff) << 0) |
						((u32)((u8)(1164*(i-16)/1000)) << 8) |
						((u32)((u8)(1164*(i-16)/1000)) << 16) |
						((u32)((u8)(1164*(i-16)/1000)) << 24));
			}
		}
		else {
			//Palette is ARGB8888 with 256 color 
			if (i >= (VIDEO_CMAP_OFFSET + color_used))
				*cmap = 0x000000ff;
			else
				*cmap = (((u32)(0xff) << 0) |
						((u32)(cte->red) << 8) |
						((u32)(cte->green) << 16) |
						((u32)(cte->blue) << 24));
		}
		cmap++;
		cte++;
	}
	flush_cache((ulong)priv->cmap, 1024);

	#if 0 //dump palette for debug
	cmap = priv->cmap;
	debug("dump palette! \n");
	for (i = 0; i < colours; ++i) {
		debug("%d : 0x%08x \n", i, *cmap);
		cmap++;
	}
	#endif
#else	
	for (i = 0; i < colours; ++i) {
		*cmap = ((cte->red   << 8) & 0xf800) |
			((cte->green << 3) & 0x07e0) |
			((cte->blue  >> 3) & 0x001f);
		cmap++;
		cte++;
	}
#endif
}

int video_bmp_display(struct udevice *dev, ulong bmp_image, int x, int y,
		      bool align)
{
	struct video_priv *priv = dev_get_uclass_priv(dev);
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	u32 *cmap_base = NULL;
#else
	ushort *cmap_base = NULL;
#endif
	int i, j;
	uchar *fb;
	struct bmp_image *bmp = map_sysmem(bmp_image, 0);
	uchar *bmap;
	ushort padded_width;
	unsigned long width, height, byte_width = 0;
	unsigned long pwidth = priv->xsize;
	unsigned colours, bpix, bmp_bpix;
	struct bmp_color_table_entry *palette;
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)	
	int hdr_size, color_used, data_offset;
#else
	int hdr_size;
#endif

	if (!bmp || !(bmp->header.signature[0] == 'B' &&
	    bmp->header.signature[1] == 'M')) {
		printf("Error: no valid bmp image at %lx\n", bmp_image);

		return -EINVAL;
	}

	width = get_unaligned_le32(&bmp->header.width);
	height = get_unaligned_le32(&bmp->header.height);
	bmp_bpix = get_unaligned_le16(&bmp->header.bit_count);
	hdr_size = get_unaligned_le16(&bmp->header.size);
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	data_offset = get_unaligned_le16(&bmp->header.data_offset);
	color_used = get_unaligned_le16(&bmp->header.colors_used);
	if( (bmp_bpix == 8) && (color_used == 0) && (data_offset > 54) )
		color_used = 0x100 ;//(data_offset - 54) / 4;
	debug("data_offset=%d, hdr_size=%d, bmp_bpix=%d, color_used=%d\n", data_offset, hdr_size, bmp_bpix, color_used);
#else
	debug("hdr_size=%d, bmp_bpix=%d\n", hdr_size, bmp_bpix);
#endif
	palette = (void *)bmp + 14 + hdr_size;

	colours = 1 << bmp_bpix;

	bpix = VNBITS(priv->bpix);
	
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	debug("bmp_bpix: %d , bpix %d \n",bmp_bpix, bpix);
#endif

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	if (bpix != 8 && bpix != 16 && bpix != 32) {
#else
	if (bpix != 1 && bpix != 8 && bpix != 16 && bpix != 32) {
#endif
		printf("Error: %d bit/pixel mode, but BMP has %d bit/pixel\n",
		       bpix, bmp_bpix);

		return -EINVAL;
	}


#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	/*
	 * We support displaying 8bpp/24bpp/32bpp BMPs transfer to SP7021 display engine
	 * and SP7021 display engine can select 8bpp(palette=ARGB),RGB565,ARGB8888
	 */
	if (bpix != bmp_bpix &&
			!(bmp_bpix == 8 && bpix == 16) &&
			!(bmp_bpix == 8 && bpix == 32) &&
			!(bmp_bpix == 24 && bpix == 8) &&
			!(bmp_bpix == 24 && bpix == 16) &&
			!(bmp_bpix == 24 && bpix == 32) &&
			!(bmp_bpix == 32 && bpix == 8) &&
			!(bmp_bpix == 32 && bpix == 16) &&
			!(bmp_bpix == 32 && bpix == 32)) {
#else
	/*
	 * We support displaying 8bpp and 24bpp BMPs on 16bpp LCDs
	 * and displaying 24bpp BMPs on 32bpp LCDs
	 */
	if (bpix != bmp_bpix &&
	    !(bmp_bpix == 8 && bpix == 16) &&
	    !(bmp_bpix == 24 && bpix == 16) &&
	    !(bmp_bpix == 24 && bpix == 32)) {
#endif 	
		printf("Error: %d bit/pixel mode, but BMP has %d bit/pixel\n",
		       bpix, get_unaligned_le16(&bmp->header.bit_count));
		return -EPERM;
	}

	debug("Display-bmp: %d x %d  with %d colours, display %d\n",
	      (int)width, (int)height, (int)colours, 1 << bpix);

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
	if (bmp_bpix == 8)
		video_set_cmap(dev, palette, colours, color_used, 0);

	if (((bmp_bpix == 32) || (bmp_bpix == 24)) && (bpix == 8)) {
		colours = 0x100;
		video_set_cmap(dev, palette, colours, color_used, 1);
	}
#else
	if (bmp_bpix == 8)
		video_set_cmap(dev, palette, colours);
#endif

	padded_width = (width & 0x3 ? (width & ~0x3) + 4 : width);

	if (align) {
		video_splash_align_axis(&x, priv->xsize, width);
		video_splash_align_axis(&y, priv->ysize, height);
	}

	if ((x + width) > pwidth)
		width = pwidth - x;
	if ((y + height) > priv->ysize)
		height = priv->ysize - y;

	bmap = (uchar *)bmp + get_unaligned_le32(&bmp->header.data_offset);
	fb = (uchar *)(priv->fb +
		(y + height - 1) * priv->line_length + x * bpix / 8);

	switch (bmp_bpix) {
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
#else
	case 1:
#endif
	case 8: {
		cmap_base = priv->cmap;
#ifdef CONFIG_VIDEO_BMP_RLE8
		u32 compression = get_unaligned_le32(&bmp->header.compression);
		debug("compressed %d %d\n", compression, BMP_BI_RLE8);
		if (compression == BMP_BI_RLE8) {
			#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
			if (bpix != 8) {
				printf("Error: only support 8 bpix");
			#else
			if (bpix != 16) {
				/* TODO implement render code for bpix != 16 */
				printf("Error: only support 16 bpix");
			#endif
				return -EPROTONOSUPPORT;
			}
			video_display_rle8_bitmap(dev, bmp, cmap_base, fb, x,
						  y);
			break;
		}
#endif

#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
		if (bpix == 8)
			byte_width = width;
		else if (bpix == 16)
			byte_width = width * 2;
		else if (bpix == 24)
			;//SP7021 didn't support this setting
		else if (bpix == 32)
			byte_width = width * 4;
			
		for (i = 0; i < height; ++i) {
			WATCHDOG_RESET();
			for (j = 0; j < width; j++) {
				if (bpix == 8) {
					//fmt is 8bpp(palette=ARGB)
					fb_put_byte(&fb, &bmap);
				} else if (bpix == 16) {
					//fmt is RGB565
					*(uint16_t *)fb = ((uint16_t)((cmap_base[*bmap+VIDEO_CMAP_OFFSET] & 0x0000f800) >> 0 |
                                    (cmap_base[*bmap+VIDEO_CMAP_OFFSET] & 0x00fc0000) >> 13 |
                                    (cmap_base[*bmap+VIDEO_CMAP_OFFSET] & 0xf8000000) >> 27));
					bmap++;
					fb += sizeof(uint16_t) / sizeof(*fb);
				} else if (bpix == 24) {
					;//SP7021 didn't support this setting
				} else if (bpix == 32) {
					//fmt is ARGB8888
					*(uint32_t *)fb = DISP_SWAP32((uint32_t)cmap_base[*bmap+VIDEO_CMAP_OFFSET]);
					bmap++;
					fb += sizeof(uint32_t) / sizeof(*fb);
				}

			}
			bmap += (padded_width - width);
			fb -= byte_width + priv->line_length;
		}			
#else
		if (bpix != 16)
			byte_width = width;
		else
			byte_width = width * 2;

		for (i = 0; i < height; ++i) {
			WATCHDOG_RESET();
			for (j = 0; j < width; j++) {
				if (bpix != 16) {
					fb_put_byte(&fb, &bmap);
				} else {
					*(uint16_t *)fb = cmap_base[*bmap];
					bmap++;
					fb += sizeof(uint16_t) / sizeof(*fb);
				}
			}
			bmap += (padded_width - width);
			fb -= byte_width + priv->line_length;
		}
#endif
		break;
	}
#if defined(CONFIG_BMP_16BPP)
	case 16:
		for (i = 0; i < height; ++i) {
			WATCHDOG_RESET();
			for (j = 0; j < width; j++)
				fb_put_word(&fb, &bmap);

			bmap += (padded_width - width) * 2;
			fb -= width * 2 + priv->line_length;
		}
		break;
#endif /* CONFIG_BMP_16BPP */
#if defined(CONFIG_BMP_24BPP)
	case 24:
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
		if (bpix == 8) {
			debug("BMP RGB888 to OSD 8bpp \n");
		} else if (bpix == 16) {
			debug("BMP RGB888 to OSD 16bpp \n");
		} else if (bpix == 24) {
			debug("BMP RGB888 to OSD 24bpp \n");
			debug("SP7021 not support! \n");
		} else { //bpix == 32	
			debug("BMP RGB888 to OSD 32bpp \n");
		}

		for (i = 0; i < height; ++i) {
			for (j = 0; j < width; j++) {
				if (bpix == 8) {
					/* 8bpp (palette=ARGB) format */
					#if 1
					unsigned int tmp_val24;
					tmp_val24 = ((299 * bmap[2] + 587 * bmap[1] + 114 * bmap[0])/1000);
					if (tmp_val24 < VIDEO_CMAP_OFFSET)
						*(uchar *)fb = (uchar)VIDEO_CMAP_OFFSET;
					else if (tmp_val24 > 0xff)
						*(uchar *)fb = (uchar)0xff;
					else 
						*(uchar *)fb = (uchar)tmp_val24;
					#else
					*(uchar *)fb = (uchar)((299 * bmap[2] + 587 * bmap[1] + 114 * bmap[0])/1000);
					#endif
					bmap += 3;
					fb ++;
				} else if (bpix == 16) {
					/* 16bit 565RGB format */
					*(u16 *)fb = ((bmap[2] >> 3) << 11) |
						((bmap[1] >> 2) << 5) |
						(bmap[0] >> 3);
					bmap += 3;
					fb += 2;
				} else if (bpix == 24) {
					;//SP7021 didn't support
				} else { //bpix == 32
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = 0xff;
				}
			}
			fb -= priv->line_length + width * (bpix / 8);
			bmap += (padded_width - width) * 3;
		}
#else
		for (i = 0; i < height; ++i) {
			for (j = 0; j < width; j++) {
				if (bpix == 16) {
					/* 16bit 555RGB format */
					*(u16 *)fb = ((bmap[2] >> 3) << 10) |
						((bmap[1] >> 3) << 5) |
						(bmap[0] >> 3);
					bmap += 3;
					fb += 2;
				} else {
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = 0;
				}
			}
			fb -= priv->line_length + width * (bpix / 8);
			bmap += (padded_width - width) * 3;
		}
#endif	
		break;
#endif /* CONFIG_BMP_24BPP */
#if defined(CONFIG_BMP_32BPP)
	case 32:
#if defined(CONFIG_VIDEO_SP7021) || defined(CONFIG_VIDEO_I143)
		for (i = 0; i < height; ++i) {
			for (j = 0; j < width; j++) {
				if (bpix == 8) {
					#if 1 //deal with upper/lower limit
					unsigned int tmp_val32;
					tmp_val32 = ((299 * bmap[2] + 587 * bmap[1] + 114 * bmap[0])/1000);
					if (tmp_val32 < VIDEO_CMAP_OFFSET)
						*(uchar *)fb = (uchar)VIDEO_CMAP_OFFSET;
					else if (tmp_val32 > 0xff)
						*(uchar *)fb = (uchar)0xff;
					else 
						*(uchar *)fb = (uchar)tmp_val32;
					#else 
					*(uchar *)fb = (uchar)((299 * bmap[2] + 587 * bmap[1] + 114 * bmap[0])/1000);
					#endif
					bmap += 4;
					fb ++;
				} else if (bpix == 16) {
					*(u16 *)fb = ((bmap[2] >> 3) << 11) |
						((bmap[1] >> 2) << 5) |
						(bmap[0] >> 3);
					bmap += 4;
					fb += 2;
				} else if (bpix == 24) {
					;//SP7021 didn't support
				} else { //bpix == 32
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
					*(fb++) = *(bmap++);
				}
			}
			fb -= priv->line_length + width * (bpix / 8);
			bmap += (padded_width - width) * 4;
		}		
#else
		for (i = 0; i < height; ++i) {
			for (j = 0; j < width; j++) {
				*(fb++) = *(bmap++);
				*(fb++) = *(bmap++);
				*(fb++) = *(bmap++);
				*(fb++) = *(bmap++);
			}
			fb -= priv->line_length + width * (bpix / 8);
		}
#endif
		break;
#endif /* CONFIG_BMP_32BPP */
	default:
		break;
	};

	video_sync(dev, false);

	return 0;
}

