/* unicap
 *
 * Copyright (C) 2004-2008 Arne Caspari ( arne@unicap-imaging.org )
 *
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef   	AVI_H_
# define   	AVI_H_

#include <sys/types.h>
#include <linux/types.h>

typedef struct
{
      __u16 left;
      __u16 top;
      __u16 right;
      __u16 bottom;
} avi_rect_t;

typedef struct
{
      __u32 biSize;
      __u32 biWidth;
      __u32 biHeight;
      __u16 biPlanes;
      __u16 biBitCount;
      __u32 biCompression;
      __u32 biSizeImage;
      __u32 biXPelsPerMeter;
      __u32 biYPelsPerMeter;
      __u32 biClrUsed;
      __u32 biClrImportant;
} avi_bitmapinfohdr_t;


typedef struct
{
      __u32 dwMicroSecPerFrame; // frame display rate (or 0)
      __u32 dwMaxBytesPerSec; // max. transfer rate
      __u32 dwPaddingGranularity; // pad to multiples of this
// size;
      __u32 dwFlags; // the ever-present flags
      __u32 dwTotalFrames; // # frames in file
      __u32 dwInitialFrames;
      __u32 dwStreams;
      __u32 dwSuggestedBufferSize;
      __u32 dwWidth;
      __u32 dwHeight;
      __u32 dwReserved[4];
} avi_main_header_t;

typedef struct {
      __u32 fccType;
      __u32 fccHandler;
      __u32 dwFlags;
      __u16 wPriority;
      __u16 wLanguage;
      __u32 dwInitialFrames;
      __u32 dwScale;
      __u32 dwRate; /* dwRate / dwScale == samples/second */
      __u32 dwStart;
      __u32 dwLength; /* In units above... */
      __u32 dwSuggestedBufferSize;
      __u32 dwQuality;
      __u32 dwSampleSize;
      avi_rect_t rcFrame;
} avi_stream_header_t;

typedef struct {
      __u32 dwFourCC;
      __u32 dwSize;
} avi_chunk_t;

typedef struct {
      __u32 dwList;
      __u32 dwSize;
      __u32 dwFourCC;
} avi_list_t;

typedef struct {
      __u32 dwBufferSize;
      __u32 dwPtr;
      __u8 *bData;
} avi_buffer_t;

/* typedef struct _aviindex_chunk { */
/*       __u32 fcc; */
/*       __u32 cb; */
/*       __u16 wLongsPerEntry; */
/*       __u8 bIndexSubType; */
/*       __u8 bIndexType; */
/*       __u32 nEntriesInUse; */
/*       __u32 dwChunkId; */
/*       __u32 dwReserved[3]; */
/*       struct _aviindex_entry { */
/* 	    __u32 adw[wLongsPerEntry]; */
/*       } aIndex [ ]; */
/* } avi_index_chunk_t; */

typedef struct {
    __u32 dwFourCC;
    __u32 dwFlags;
    __u32 dwChunkOffset;
    __u32 dwChunkLength;
} avi_index_entry_t;


#endif 	    /* !AVI_H_ */
