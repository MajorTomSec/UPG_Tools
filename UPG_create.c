#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "UPG.h"

#include "md5.h"
#include "crc32.c"

void calcMD5(void *data, uint8_t *hash, size_t size)
{
  MD5_CTX ctx;

  MD5_Init(&ctx);
  MD5_Update(&ctx, data, size);
  MD5_Final(hash, &ctx);
}

void write_header(FILE *fp)
{
  UPG_header_t header = {0};
  void *buf = NULL;

  snprintf(&header.magic[0], 5, "NFWB");

  header.version_major = 0x2;
  header.version_minor = 0x21C;

  fseek(fp, 0, SEEK_END);
  header.data_size = ftell(fp) - sizeof(UPG_header_t);

  fseek(fp, sizeof(UPG_header_t), SEEK_SET);
  buf = malloc(header.data_size);
  fread(buf, header.data_size, 1, fp);
  calcMD5(buf, &header.md5[0], header.data_size);

  header.files_count = 6;
  header.data_offset = sizeof(UPG_header_t) + sizeof(files_table_t);
  header.crc32 = crc32(0, &header, sizeof(UPG_header_t));//, &header.crc32);

  fseek(fp, 0, SEEK_SET);
  fwrite(&header, sizeof(UPG_header_t), 1, fp);
}

void get_file_infos(file_entry_t *entry, const char *file, int file_id, size_t *offset, FILE *out)
{
  FILE *fp = NULL;
  void *buf = NULL;
  struct stat st;

  if (stat(file, &st) != 0) {
    return;
  }
  entry->id = file_id;
  entry->size = st.st_size;
  entry->offset = *offset;

  buf = malloc(st.st_size);

  fp = fopen(file, "rb");
  fread(buf, st.st_size, 1, fp);
  fclose(fp);

  calcMD5(buf, &entry->md5[0], st.st_size);

  fwrite(buf, st.st_size, 1, out);
  free(buf);

  *offset += st.st_size;
  while (*offset % 32 != 0) {
    fputc(0, out);
    *offset += 1;
  }
}

int main(int argc, char *argv[])
{
  FILE *out = NULL;
  void *header = NULL;
  files_table_t files_table = {0};
  size_t offset = 0;

  if (argc < 8) {
    printf("usage: %s [out.upg] [bootrom] [rootfs] [splash] [bootloader] [kernel] [boardfs]\n", argv[0]);
    return 1;
  }

  if ((out = fopen(argv[1], "wb+")) == NULL) {
    perror("Unable to create file");
    return 1;
  }

  header = malloc(sizeof(UPG_header_t) + sizeof(files_table_t));
  offset = (sizeof(UPG_header_t) + sizeof(files_table_t));
  memset(header, 0, offset);

  fwrite(header, offset, 1, out);

  get_file_infos(&files_table.entries[0], argv[2], FILE_BOOTROM, &offset, out);
  get_file_infos(&files_table.entries[1], argv[3], FILE_ROOTFS, &offset, out);
  get_file_infos(&files_table.entries[2], argv[4], FILE_SPLASHSCREEN, &offset, out);
  get_file_infos(&files_table.entries[3], argv[5], FILE_BOOTLOADER, &offset, out);
  get_file_infos(&files_table.entries[4], argv[6], FILE_KERNEL, &offset, out);
  get_file_infos(&files_table.entries[5], argv[7], FILE_BOARDFS, &offset, out);

  fseek(out, sizeof(UPG_header_t), SEEK_SET);
  fwrite(&files_table.entries[0], sizeof(files_table_t), 1, out);

  write_header(out);

  fclose(out);
  free(header);
  return 0;
}
