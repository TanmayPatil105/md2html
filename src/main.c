/* main.c
 *
 * Copyright 2024 Tanmay Patil <tanmaynpatil105@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */


#include <stdio.h>
#include "params.h"
#include "md.h"
#include "html.h"


#define VERSION "0.1.0"

static void
print_version (char *binary)
{
  printf ("%s %s\n", binary, VERSION);
}

static void
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
  "  -h, --help                 display this message\n"
  "  -v, --version              output version information\n", binary);
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

  if (params->version)
    {
      print_version (argv[0]);
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
