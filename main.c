/*
 * @file: main.c
 */

#include <stdio.h>
#include "md.h"


void
print_usage (char *binary)
{
  printf ("Usage: %s <md file_path>\n", binary);
}

int
main (int   argc,
      char *argv[])
{
  /* debug */
  MDUnit *unit = NULL;
  MDFile *file = NULL;
  MD *md = NULL;

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

  /* debug */
  unit = md->elements;
  while (unit != NULL)
    {
      printf ("%s", unit->content);
      unit = unit->next;
    }

  /* free */
  fclose (file);

  return 0;
}
