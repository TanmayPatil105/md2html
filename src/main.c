/*
 * @file: main.c
 */

#include <stdio.h>
#include "md.h"
#include "html.h"


void
print_usage (char *binary)
{
  printf ("Usage: %s <md file_path> <output file name> <document title>\n", binary);
}

int
main (int   argc,
      char *argv[])
{
  MDFile *file = NULL;
  MD *md = NULL;
  HTML *html = NULL;
  char *file_name = NULL;

  if (argc < 2)
    {
      print_usage (argv[0]);
      return 1;
    }

  file = fopen (argv[1], "r");
  if (file == NULL)
    {
      printf ("No such file or directory.\n");
      print_usage (argv[0]);
      return 1;
    }

  md = parse_md (file);

  /* FIXME: handle args correctly */
  if (argc > 2)
    {
      file_name = argv[2];
    }

  html = html_from_md (md, file_name);
  flush_html (html);

  /* free */
  html_free (html);
  md_free (md);
  fclose (file);

  return 0;
}
