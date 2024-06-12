/*
 * @file: params.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"


void
params_init (Params **params)
{
  *params = (Params*) malloc (sizeof(Params));

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
  char error[1000] = {};

  params_init (&params);

  for (int i = 1; i < argc; i++)
    {
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

  if (i_file == NULL)
    sprintf (error, "missing input file");

  if (error[0] == '\0')
    {
      params->i_file = i_file;
      params->o_file = o_file;
      params->title = title;
      params->inline_css = inline_css;
      params->i_css_file = i_css_file;
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
