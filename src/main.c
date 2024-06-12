/*
 * @file: main.c
 */

#include <stdio.h>
#include "params.h"
#include "md.h"
#include "html.h"


void
print_usage (char *binary)
{
  printf("Usage: %s -i MD_FILE\n"
  "Generate equivalent HTML of md\n"
  "\n"
  "Mandatory arguments:\n"
  "  -i, --input                input markdown file"
  "\n"
  "Optional arguments:\n"
  "  -o, --output               name of output HTML doc\n"
  "  -t, --title                title of output HTML doc\n"
  "  -h, --help                 display this message\n", binary);
}

int
main (int   argc,
      char *argv[])
{
  Params *params = NULL;
  MDFile *file = NULL;
  MD *md = NULL;
  HTML *html = NULL;

  params = params_parse (argc, argv);

  if (params->error != NULL)
    {
      fprintf (stderr, "%s: %s\n",
               argv[0], params->error);
      return 1;
    }

  if (params->help)
    {
      print_usage (argv[0]);
      return 0;
    }

  file = fopen (params->i_file, "r");
  if (file == NULL)
    {
      fprintf (stderr, "%s: %s: No such file or directory\n",
               argv[0], params->i_file);
      return 1;
    }

  md = parse_md (file);

  html = html_from_md (md, params);
  flush_html (html);

  /* free */
  html_free (html);
  md_free (md);
  params_free (params);
  fclose (file);

  return 0;
}
