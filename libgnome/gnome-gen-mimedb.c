#define GEN_MIMEDB 1
#include "gnome-magic.c"

extern GnomeMagicEntry *gnome_magic_parse(const char *filename, int *nents);

int main(int argc, char *argv[])
{
  GnomeMagicEntry *ents = NULL;
  char *filename = NULL, *out_filename;
  int nents;

  char *outmem;
  int fd;

  gnomelib_init("gnome-gen-mimedb", VERSION);

  if(argc > 1) {
    if(argv[1][0] == '-') {
      printf("Usage: %s [filename]\n", argv[0]);
      return 1;
    } else if(g_file_exists(argv[1]))
      filename = argv[1];
  } else
    filename = gnome_config_file("mime-magic");

  if(!filename) {
    printf("Input file does not exist (or unspecified)...\n");
    printf("Usage: %s [filename]\n", argv[0]);
    return 1;
  }

  ents = gnome_magic_parse(filename, &nents);

  if(!nents) return 0;

  out_filename = g_copy_strings(filename, ".dat", NULL);
  fd = open(out_filename, O_RDWR|O_TRUNC|O_CREAT, 0644);
  if(fd < 0) return 1;

  if(write(fd, ents, nents * sizeof(GnomeMagicEntry))
     != (nents * sizeof(GnomeMagicEntry)))
    return 1;

  close(fd);

  return 0;
}
