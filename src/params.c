/* params.c
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
#include <stdlib.h>
#include <string.h>
#include "params.h"


void
params_init (Params **params)
{
  *params = (Params*) malloc (sizeof(Params));

  (*params)->help = false;
  (*params)->error = NULL;
}

Params *
params_parse (int   argc,
              char *argv[])
{
  Params *params = NULL;
  char *i_file = NULL;
  char *o_file = NULL;
  char *title = NULL;
  bool inline_css = false;
  char *i_css_file = NULL;
  bool help = false;
  char error[1000] = {};

  params_init (&params);

  for (int i = 1; i < argc; i++)
    {
      if ((strcmp (argv[i], "-h") == 0) ||
          (strcmp (argv[i], "--help") == 0))
        {
          help = true;
          break;
        }

      if ((strcmp (argv[i], "-i") == 0) ||
          (strcmp (argv[i], "--input") == 0))
        {
          if (argv[i + 1] != NULL)
            {
              i_file = strdup (argv[++i]);
            }
          else
            {
              sprintf (error, "operand missing after '%s'", argv[i]);
              break;
            }
        }

      if ((strcmp (argv[i], "-o") == 0) ||
          (strcmp (argv[i], "--output") == 0))
        {
          if (argv[i + 1] != NULL)
            {
              o_file = strdup (argv[++i]);
            }
          else
            {
              sprintf (error, "operand missing after '%s'", argv[i]);
              break;
            }
        }

      if ((strcmp (argv[i], "-t") == 0) ||
          (strcmp (argv[i], "--title") == 0))
        {
          if (argv[i + 1] != NULL)
            {
              title = strdup (argv[++i]);
            }
          else
            {
              sprintf (error, "operand missing after '%s'", argv[i]);
              break;
            }
        }

      if (strcmp (argv[i], "--css") == 0)
        {
          inline_css = true;

          if (argv[i + 1] != NULL)
            {
              i_css_file = strdup (argv[++i]);
            }
        }
    }

  if (help == false && i_file == NULL)
    sprintf (error, "missing input file");

  if (error[0] == '\0')
    {
      params->i_file = i_file;
      params->o_file = o_file;
      params->title = title;
      params->inline_css = inline_css;
      params->i_css_file = i_css_file;
      params->help = help;
    }
  else
    {
      params->error = strdup (error);
    }

  return params;
}

void params_free (Params *params)
{
  if (params->i_file != NULL)
    free (params->i_file);
  if (params->o_file != NULL)
    free (params->o_file);
  if (params->title != NULL)
    free (params->title);
  if (params->i_css_file != NULL)
    free (params->i_css_file);
  if (params->error != NULL)
    free (params->error);

  free (params);
}
