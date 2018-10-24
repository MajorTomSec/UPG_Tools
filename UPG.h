#ifndef __UPG_H
#define __UPG_H

#include <stdint.h>

enum FILE_ID {
  FILE_BOOTROM = 1,
  FILE_ROOTFS = 3,
  FILE_SPLASHSCREEN = 5,
  FILE_BOOTLOADER = 7,
  FILE_KERNEL = 9,
  FILE_BOARDFS = 12
};

typedef struct {
  uint32_t id;
  uint32_t size;
  uint32_t offset;
  uint8_t md5[0x10];
} file_entry_t;

typedef struct {
  file_entry_t entries[6];
  uint8_t pad[0x18];
} files_table_t;

typedef struct {
  uint8_t magic[4]; // NFWB
  uint32_t version_major;
  uint32_t version_minor;
  uint8_t pad1[20];
  uint32_t data_size; // File size - header (0xC0)
  uint8_t md5[0x10]; // File MD5 w/o header (0xC0 -> EOF)
  uint32_t files_count;
  uint32_t data_offset; // 0x180
  uint8_t pad2[128];
  uint32_t crc32; // crc32(header) with header.crc32 = 0
} UPG_header_t;

#endif // __UPG_H
