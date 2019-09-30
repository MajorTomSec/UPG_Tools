#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>

#include "UPG.h"

#include "md5.h"
#include "crc32.c"

int g_nfiles = 0;

static struct option long_options[] =
{
  {"out", required_argument, NULL, 'o'},
  {"uboot", required_argument, NULL, 'u'},
  {"ubootdat", required_argument, NULL, 'U'},
  {"stbc", required_argument, NULL, 'S'},
  {"kernel", required_argument, NULL, 'k'},
  {"rootfs", required_argument, NULL, 'r'},
  {"boardfs", required_argument, NULL, 'b'},
  {"appdat", required_argument, NULL, 'a'},
  {"splash", required_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};

void print_help(char *exe)
{
  printf(
"usage: %s [options] -o out.upg\n"\
"Options:\n"\
  "\t[-o --out out.upg] \t\t- Output UPG file\n"\
  "\t[-u --uboot u-boot.bin] \t- U-Boot Bootloader\n"\
  "\t[-U --ubootdat u-boot.dat] \t- U-Boot data\n"\
  "\t[-S --stbc stbc.bin] \t\t- STBC Firmware\n"\
  "\t[-k --kernel kernel.bin] \t- Kernel Image\n"\
  "\t[-r --rootfs rootfs.img] \t- UBI rootfs image\n"\
  "\t[-b --boardfs boardfs.img] \t- UBI boardfs image\n"\
  "\t[-a --appdat appdat.img] \t- UBI appdat image\n"\
  "\t[-s --splash splash.bmp] \t- BMP splashscreen\n"\
  "\t[-h --help] \t\t\t- Print this message\n",\
  exe);

  exit(0);
}

void calcMD5(void *data, uint8_t *hash, size_t size)
{
  MD5_CTX ctx;

  MD5_Init(&ctx);
  MD5_Update(&ctx, data, size);
  MD5_Final(hash, &ctx);
}

void write_header(FILE *fp)
{
  size_t files_table_size = 0;
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

  files_table_size = sizeof(file_entry_t) * g_nfiles;
  files_table_size += (32 - (files_table_size % 32)); // padding

  header.files_count = g_nfiles;
  header.data_offset = sizeof(UPG_header_t) + files_table_size;
  header.crc32 = crc32(0, &header, sizeof(UPG_header_t));

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
  char *output = NULL;
  char *uboot = NULL, *ubootdat= NULL, *stbc = NULL, *kernel = NULL;
  char *rootfs = NULL, *boardfs = NULL, *appdat = NULL, *splash = NULL;

  FILE *out = NULL;
  void *header = NULL;
  file_entry_t *files_table = NULL;
  size_t offset = 0, files_table_size = 0;

  int c;
  while ((c = getopt_long(argc, argv, "o:u:U:S:k:r:b:a:s:h", long_options, NULL)) != -1) {
    switch (c) {
      case 'o':
        output = strdup(optarg);
        break;
      case 'u':
        uboot = strdup(optarg);
        g_nfiles++;
        break;
      case 'U':
        ubootdat = strdup(optarg);
        g_nfiles++;
        break;
      case 'S':
        stbc = strdup(optarg);
        g_nfiles++;
        break;
      case 'k':
        kernel = strdup(optarg);
        g_nfiles++;
        break;
      case 'r':
        rootfs = strdup(optarg);
        g_nfiles++;
        break;
      case 'b':
        boardfs = strdup(optarg);
        g_nfiles++;
        break;
      case 'a':
        appdat = strdup(optarg);
        g_nfiles++;
        break;
      case 's':
        splash = strdup(optarg);
        g_nfiles++;
        break;
      case 'h':
        print_help(argv[0]);
        break;
    }
  }

  if (output == NULL)
    print_help(argv[0]);

  if ((out = fopen(output, "wb+")) == NULL) {
    perror("Unable to create file.");
    return 1;
  }


  files_table_size = sizeof(file_entry_t) * g_nfiles;
  files_table_size += (32 - (files_table_size % 32));

  files_table = malloc(files_table_size);
  header = malloc(sizeof(UPG_header_t) + files_table_size);

  offset = sizeof(UPG_header_t) + files_table_size;
  memset(header, 0, offset);

  fwrite(header, offset, 1, out);

  printf("[*] Packing files...");

  c = 0;

  if (uboot)
    get_file_infos(&files_table[c++], uboot, FILE_UBOOT, &offset, out);
  if (ubootdat)
    get_file_infos(&files_table[c++], ubootdat, FILE_UBOOTDAT, &offset, out);
  if (stbc)
    get_file_infos(&files_table[c++], stbc, FILE_STBC, &offset, out);
  if (kernel)
    get_file_infos(&files_table[c++], kernel, FILE_KERNEL, &offset, out);
  if (rootfs)
    get_file_infos(&files_table[c++], rootfs, FILE_ROOTFS, &offset, out);
  if (boardfs)
    get_file_infos(&files_table[c++], boardfs, FILE_BOARDFS, &offset, out);
  if (appdat)
    get_file_infos(&files_table[c++], appdat, FILE_APPDAT, &offset, out);
  if (splash)
    get_file_infos(&files_table[c++], splash, FILE_SPLASH, &offset, out);

  fseek(out, sizeof(UPG_header_t), SEEK_SET);
  fwrite(&files_table[0], files_table_size, 1, out);
  printf("OK\n");

  printf("[*] Writing header...");

  write_header(out);
  printf("OK\n");

  fclose(out);
  free(files_table);
  free(header);
  printf("[+] All done. File saved as %s. Exiting.\n", output);

  return 0;
}
