/*
 * @file: params.h
 */

#pragma once

#include <stdbool.h>


typedef struct Params {
  char *i_file;     /* input MD file */

  char *o_file;     /* output HTML file; defaults to index.html */
  char *title;      /* HTML doc title */

  bool  inline_css;  /* add inline css */
  char *i_css_file;  /* set inline css equal to contents of .css file */

  char *error;       /* error while parsing arguments */
} Params;


Params *params_parse (int     argc,
                      char   *argv[]);
void    params_free  (Params *params);
