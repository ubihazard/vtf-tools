#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
  #include <direct.h>
  #include <vcruntime_string.h>
#else
  #include <sys/stat.h>
#endif
#include "VTFLib/VTFLib.h"

#define VTF_HDR_SIZE 80
#define VTF_HDR_OFF_VER_MAJ 4
#define VTF_HDR_OFF_VER_MIN 8
#define VTF_HDR_OFF_SIZE 12
#define VTF_HDR_OFF_WIDTH 16
#define VTF_HDR_OFF_HEIGHT 18
#define VTF_HDR_OFF_FLAGS 20
#define VTF_HDR_OFF_FRAMES 24
#define VTF_HDR_OFF_START 26
#define VTF_HDR_OFF_REFL 32
#define VTF_HDR_OFF_FORMAT 52
#define VTF_HDR_OFF_MIPS 56
#define VTF_HDR_OFF_THUMB_FMT 57
#define VTF_HDR_OFF_THUMB_W 61
#define VTF_HDR_OFF_THUMB_H 62
#define VTF_HDR_OFF_DEPTH 63
#define VTF_HDR_OFF_RES 68

#define re_cast reinterpret_cast

#define strieq(s1, s2) (stricmp(s1, s2) == 0)

#ifndef _WIN32
  #define max(a, b) ((a) > (b) ? (a) : (b))
  #define min(a, b) ((a) < (b) ? (a) : (b))
  #define mkdir(path) mkdir(path, S_IRWXU)
#endif

static inline unsigned
vtf_get_real_size (VTFLib::CVTFFile const& vtf)
{
  auto const hdr_size = vtf.GetHeaderData()->HeaderSize;
  if (hdr_size < VTF_HDR_SIZE) {
    auto const diff = VTF_HDR_SIZE - hdr_size;
    return vtf.GetSize() + diff;
  }
  return vtf.GetSize();
}

static inline void
vtf_force_72 (VTFLib::CVTFFile const& vtf, void* const vtf_buf, bool const resize)
{
  // Force VTF version 7.2 header: 80 bytes
  // without resource block
  auto const vtf_size = vtf.GetSize();
  auto const hdr_size = vtf.GetHeaderData()->HeaderSize;
  auto const thumb_size = vtf.GetThumbnailDataSize();
  auto const img_size = vtf.GetImageDataSize();
  if (hdr_size < VTF_HDR_SIZE) {
    // VTF version 7.1 or below: smaller header size,
    // need to expand
    auto const diff = VTF_HDR_SIZE - hdr_size;
    if (resize) {
      memmove((char*)vtf_buf + VTF_HDR_SIZE
      , (char*)vtf_buf + hdr_size
      , thumb_size + img_size);
    }
    memset((char*)vtf_buf + VTF_HDR_SIZE - diff, 0, diff);
    *(short*)((char*)vtf_buf + VTF_HDR_OFF_DEPTH) = 1;
  } else if (vtf.GetHeaderData()->ResourceCount > 0) {
    // VTF version 7.3 or above: bigger header size,
    // drop the resource block
    auto const res_size = hdr_size - VTF_HDR_SIZE;
    if (resize) {
      memmove((char*)vtf_buf + VTF_HDR_SIZE
      , (char*)vtf_buf + hdr_size
      , thumb_size + img_size);
    }
  }

  *(int*)((char*)vtf_buf + VTF_HDR_OFF_SIZE) = VTF_HDR_SIZE;
  *(int*)((char*)vtf_buf + VTF_HDR_OFF_VER_MAJ) = 7;
  *(int*)((char*)vtf_buf + VTF_HDR_OFF_VER_MIN) = 2;
  *(int*)((char*)vtf_buf + VTF_HDR_OFF_RES) = 0;
}
