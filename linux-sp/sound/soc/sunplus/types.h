#ifndef CYGONCE_HAL_TYPE_H
#define CYGONCE_HAL_TYPE_H

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;

typedef unsigned long long 	UINT64;
typedef unsigned int   		UINT32;
typedef unsigned short 		UINT16;
typedef unsigned char 		UINT8;

typedef long long 		INT64;
typedef int   			INT32;
typedef short 			INT16;
typedef signed char		INT8;

typedef unsigned char		__u8;
typedef unsigned short		__u16;
typedef unsigned int		__u32;
typedef unsigned long long	__u64;
typedef signed char		__s8;
typedef signed short		__s16;
typedef signed int		__s32;
typedef signed long long	__s64;

typedef UINT32              	FOURCC;

/* bcd */
#define	getBCD(p)	(((((UINT8 *)(p))[0])>>4)*10 + ((((UINT8 *)(p))[0])&0x0f))


/* unaligned accessing */
#define	getUINT16_bi(p)	((UINT16) (((UINT8 *)(p))[1] | ((UINT16)((UINT8 *)(p))[0]<<8)))
#define	getUINT16_li(p)	((UINT16) (((UINT8 *)(p))[0] | ((UINT16)((UINT8 *)(p))[1]<<8)))

#define	getUINT24_bi(p)	((UINT32) (((UINT8 *)(p))[2] | ((UINT32)((UINT8 *)(p))[1]<<8) | ((UINT32)((UINT8 *)(p))[0]<<16)))

#define	getUINT32_bi(p)	((UINT32) (((UINT8 *)(p))[3] | ((UINT32)((UINT8 *)(p))[2]<<8) | ((UINT32)((UINT8 *)(p))[1]<<16) | ((UINT32)((UINT8 *)(p))[0]<<24)))
#define	getUINT32_li(p)	((UINT32) (((UINT8 *)(p))[0] | ((UINT32)((UINT8 *)(p))[1]<<8) | ((UINT32)((UINT8 *)(p))[2]<<16) | ((UINT32)((UINT8 *)(p))[3]<<24)))

#define	getUINT64_bi(p)	((((UINT64) getUINT32_bi(p)) << 32) | ((UINT64) getUINT32_bi(p+4)))

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#define	BIG_ENDIAN  4321

/* endian-less taste */
#ifdef	BIG_ENDIAN
#define	getUINT16(p)	getUINT16_bi((BYTE *)p)
#define	getUINT32(p)	getUINT32_bi((BYTE *)p)
#else
#define	getUINT16(p)	getUINT16_li((BYTE *)p)
#define	getUINT32(p)	getUINT32_li((BYTE *)p)
#endif

#define	getBiUINT32(p)	getUINT32_li((BYTE *)p)
#define	getBiUINT16(p)	getUINT16_li((BYTE *)p)


#ifndef CYGPKG_UITRON
#ifndef  TRUE
#define  TRUE  1
#endif

#ifndef  FALSE
#define  FALSE 0
#endif
#endif


#define GET_LOW_NIBBLE(p)	((p) & 0x0F)

#ifndef NULL
#define NULL    ((void *)0)
#endif


#endif // CYGONCE_HAL_TYPE_H

