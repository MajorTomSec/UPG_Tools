#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct {
  uint32_t id; // only odd numbers
  uint32_t size;
  uint32_t offset; // offset in UPG file
  uint8_t md5[0x10];
} file_entry_t;

typedef struct {
  uint32_t magic; // NFWB
  uint32_t version; // 0x02 ?
  uint32_t unk1; // 0x226 / 0x21C
  uint8_t pad1[20];
  uint8_t sig[20]; // sha1 based sig ?
  uint32_t files_count;
  uint32_t header_size; // 0x180
  uint8_t pad2[128];
  uint32_t crc32; // of what
  file_entry_t entries[6];
  uint8_t pad3[0x18];
} UPG_header_t;

int main(int argc, char *argv[])
{
  FILE *f = NULL, *out = NULL;
  char *filename = NULL, *dir = NULL, *path = NULL;
  void *buf = NULL;
  UPG_header_t *header = NULL;
  int i, j;

  if (argc < 3) {
    printf("usage: %s [file.upg] [extract_dir]\n", argv[0]);
    return 1;
  }

  filename = argv[1];
  dir = argv[2];

  if (mkdir(dir, 0777) == -1 && errno != EEXIST) {
    perror("Could not create directory");
    return 1;
  }

  if ((f = fopen(filename, "rb")) == NULL) {
    perror("Could not open file");
    return 1;
  }

  header = malloc(sizeof(UPG_header_t));
  fread(header, sizeof(UPG_header_t), 1, f);

  printf("[+] %d files found.\n\n", header->files_count);

  for (i = 0; i < header->files_count; i++) {
    path = malloc(256 * sizeof(char));
    snprintf(path, 256, "%s/file_%X", dir, header->entries[i].offset);

    printf("[*] Extracting file_%X ", header->entries[i].offset);
    printf("(MD5: ");

    for (j = 0; j < 0x10; j++) {
      printf("%02X", header->entries[i].md5[j]);
    }
    printf(")...\n");

    if ((out = fopen(path, "wb+")) == NULL) {
      perror("Could not create file");
      continue;
    }

    buf = malloc(header->entries[i].size);
    fseek(f, header->entries[i].offset, SEEK_SET);

    fread(buf, header->entries[i].size, 1, f);
    fwrite(buf, header->entries[i].size, 1, out);

    free(buf);
    free(path);
    fclose(out);
  }

  printf("\n[+] Done. Exiting.\n");

  free(header);
  fclose(f);

  return 0;
}
