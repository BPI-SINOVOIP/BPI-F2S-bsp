#ifndef __FONT_API_H__
#define __FONT_API_H__

extern int FONT_init(void);
extern int FONT_SetSize(int size);
extern void FONT_GetString_Size(char *src_str, int len, int *w, int *h);
extern void FONT_GetString_ptr(char *src_str,
		int len,
		unsigned char *raw_ptr,
		int w,
		int h,
		int font_color,
		int font_bg_color,
		int pixel_len);

#endif /* __FONT_API_H__ */

