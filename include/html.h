/*
 * file: html.h
 */

#pragma once

#include <stdio.h>
#include "params.h"
#include "md.h"

/*
 * @HTMLFile
 */
typedef FILE HTMLFile;

typedef enum {
  HTML_TAG_H1,
  HTML_TAG_H2,
  HTML_TAG_H3,
  HTML_TAG_LI,
  HTML_TAG_IMG,
  HTML_TAG_BLOCKQUOTE,
  HTML_TAG_CODE_BLOCK_START,
  HTML_TAG_CODE_BLOCK_END,
  HTML_TAG_CODE_BLOCK_LINE,
  HTML_TAG_NONE,
  HTML_TAG_NEWLINE,
} HTMLTag;

typedef struct HTMLUnit {
  HTMLTag tag;
  char *content;
  char *uri;
} HTMLUnit;

typedef struct HTML {
  char *file_name;
  char *title;

  /* content */
  uint n_lines;
  HTMLUnit **html;
} HTML;


HTML *html_from_md (MD     *md,
                    Params *params);
void  html_free    (HTML *html);
void  flush_html   (HTML *html);
