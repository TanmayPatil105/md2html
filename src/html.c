/* html.c
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


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "html.h"
#include "macro.h"
#include "syntax.h"

/*
 * utility macros
 */

#define FWRITE_STR(str, file) \
        fwrite (str, sizeof (char), strlen (str), file);

/*
 * @default HTML values
 */
#define __DEFAULT_HTML_FILE_NAME__ "index.html"
#define __DEFAULT_HTML_TITLE__     "Document"

typedef struct {
  HTMLTag key;
  char *start_tag;
  char *end_tag;
} html_tags;

/*
 * Keep order in-sync with html.h
 */
static html_tags tags[] = {
  {HTML_TAG_H1, "<h1>", "</h1>"},
  {HTML_TAG_H2, "<h2>", "</h2>"},
  {HTML_TAG_H3, "<h3>", "</h3>"},
  {HTML_TAG_LI, "<li>", "</li>"},
  {HTML_TAG_BLOCKQUOTE, "<blockquote><q>", "</q></blockquote>"},
  {HTML_TAG_CODE_BLOCK, "<pre>", "</pre>"},

  /* FIXME: Treat as <p> */
  {HTML_TAG_NONE, NULL, NULL},
  {HTML_TAG_NEWLINE, NULL, NULL},
};

/*
 * html_init
 * @html
 *
 * allocates memory to HTML object
 */
static void
html_init (HTML **html,
           uint  n_lines)
{
  *html = malloc (sizeof (HTML));

  // malloc fails
  if (*html == NULL)
    {
      // panic
      return;
    }

  (*html)->file_name = strdup (__DEFAULT_HTML_FILE_NAME__);
  (*html)->title = NULL;

  (*html)->stylesheet = NULL;
  (*html)->document = true;
  (*html)->n_lines = n_lines;
  (*html)->html = malloc (n_lines * sizeof (HTMLUnit));
}

static HTMLTag
find_html_tag (UnitType type)
{
  switch (type)
    {
      case UNIT_TYPE_H1:
        return HTML_TAG_H1;
      case UNIT_TYPE_H2:
       return HTML_TAG_H2;
      case UNIT_TYPE_H3:
       return HTML_TAG_H3;
      case UNIT_TYPE_BULLET:
        return HTML_TAG_LI;
      case UNIT_TYPE_QUOTE:
        return HTML_TAG_BLOCKQUOTE;
      case UNIT_TYPE_NONE:
        return HTML_TAG_NEWLINE;
      case UNIT_TYPE_CODE_BLOCK:
        return HTML_TAG_CODE_BLOCK;
      default:
       return HTML_TAG_NONE;
    }

  return HTML_TAG_NONE;
}

/*
 * html_unit_init
 * @html_unit
 * @md_unit
 *
 * allocates memory to HTMLUnit object
 */
static void
html_unit_init (HTMLUnit **unit,
                MDUnit   **md_unit)
{
  *unit = (HTMLUnit *) malloc (sizeof (HTMLUnit));
  memset(*unit, 0, sizeof(HTMLUnit));

  // malloc fails
  if (*unit == NULL)
    {
      // panic
      return;
    }

  (*unit)->tag = find_html_tag ((*md_unit)->type);

  /* strdup is expensive;
   * refer to the same string */
  if ((*md_unit)->content != NULL)
    (*unit)->content = (*md_unit)->content;

  (*unit)->uri = (*md_unit)->uri;
  (*unit)->lang = (*md_unit)->lang;

  /* move forward */
  *md_unit = (*md_unit)->next;
}

/*
 * init_template
 * @file: HTMLFile
 *
 * inject template HTML code in a file
 */
static void
init_template (HTMLFile *file,
               HTML     *html)
{
  HTMLUnit *unit = NULL;

  if (html->title == NULL)
    {
      if (html->html != NULL && (unit = html->html[0]) && unit->tag == HTML_TAG_H1)
        html->title = strdup (unit->content);
      else
        html->title = strdup (__DEFAULT_HTML_TITLE__);
    }

  fprintf (file,
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "\t<meta charset=\"UTF-8\">\n"
    "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");

  if (html->stylesheet)
    {
      fprintf (file,
               "\t<link rel=\"stylesheet\" href=\"%s\">\n",
               html->stylesheet);
    }

  fprintf (file,
    "\t<title>%s</title>\n"
    "</head>\n"
    "<body>\n", html->title);
}

static void
final_template (HTMLFile *file)
{
  fprintf (file,
    "</body>\n"
    "</html>\n");
}

/*
 * html_from_md
 * @md: markdown doc
 * @file_name: custom file name
 *
 * converts markdown doc into html
 * also takes *ownership* of content
 */
HTML*
html_from_md (MD     *md,
              Params *params)
{
  uint i = 0;
  MDUnit *unit = NULL;
  HTML *html = NULL;

  html_init (&html, md->n_lines);

  /* css */
  if (params->css_file)
    html->stylesheet = strdup (params->css_file);

  html->document = params->document;

  /* custom file_name */
  if (params->o_file != NULL)
    html->file_name = strdup (params->o_file);

  if (params->title != NULL)
    html->title = strdup (params->title);

  unit = md->elements;
  while (unit != NULL)
    {
      HTMLUnit *html_unit = NULL;

      html_unit_init (&html_unit, &unit);

      html->html[i++] = html_unit;
    }

  return html;
}

/*
 * html_free
 * @HTML
 *
 * frees up memory
 */
void
html_free (HTML *html)
{
  /* free HTML units */
  for (uint i = 0; i < html->n_lines; i++)
    {
      HTMLUnit *unit = html->html[i];

      if (unit->content != NULL)
        free (unit->content);

      free (unit);
    }

  if (html->title != NULL)
    free (html->title);
  if (html->file_name != NULL)
    free (html->file_name);

  free (html->html);
  free (html);
}

static inline bool
tag_is_heading (HTMLTag tag)
{
  return tag == HTML_TAG_H1 ||
         tag == HTML_TAG_H2 ||
         tag == HTML_TAG_H3 ||
         tag == HTML_TAG_BLOCKQUOTE;
}


static inline bool
tag_is_code_block (HTMLTag tag)
{
  return tag == HTML_TAG_CODE_BLOCK;
}

static inline bool
tag_is_list (HTMLTag tag)
{
  return tag == HTML_TAG_LI;
}

static char *
format_text (char *content)
{
  char *replaced = NULL;
  char *ptr = NULL;
  size_t size = 256;
  size_t len = 0;

  replaced = malloc (sizeof (char) * size);
  ptr = content;

  while (*ptr)
    {
      if (*ptr == '*' && *(ptr + 1) == '*' && *(ptr + 2) == '*')
        {
          int offset = 3;
          char *start = ptr + offset;
          char *end = strstr (start, "***");
          if (end)
            {
              int tag_len = 14;
              sprintf (replaced + len, "<b><i>%.*s</i></b>", (int)(end - start), start);
              len += end - start + tag_len;
              ptr = end + offset;

              continue;
            }
        }
      else if (*ptr == '*' && *(ptr + 1) == '*')
        {
          int offset = 2;
          char *start = ptr + offset;
          char *end = strstr (start, "**");
          if (end)
            {
              int tag_len = 7;
              sprintf (replaced + len, "<b>%.*s</b>", (int)(end - start), start);
              len += end - start + tag_len;
              ptr = end + offset;

              continue;
            }
        }
      else if (*ptr == '*')
        {
          int offset = 1;
          char *start = ptr + offset;
          char *end = strchr (start, '*');
          if (end)
            {
              int tag_len = 7;
              sprintf (replaced + len, "<i>%.*s</i>", (int)(end - start), start);
              len += end - start + tag_len;
              ptr = end + offset;

              continue;
            }
        }
      else if (*ptr == '!' && *(ptr + 1) == '[')
        {
          char *alt_start = ptr + 2;
          char *alt_end = strstr (alt_start, "](");

          if (alt_end)
            {
              char *src_start = alt_end + 2;
              char *src_end = strchr (src_start, ')');

              if (src_end)
                {
                  int tag_len = 19;
                  sprintf (replaced + len, "<img src=\"%.*s\" alt=\"%.*s\">",
                                          (int) (src_end - src_start), src_start,
                                          (int) (alt_end - alt_start), alt_start);
                  len += (src_end - alt_start - 2) + tag_len;
                  ptr = src_end + 1;

                  continue;
                }
            }
        }
      else if (*ptr == '[')
        {
          char *anc_start = ptr + 1;
          char *anc_end = strstr (anc_start, "](");

          if (anc_end)
            {
              char *href_start = anc_end + 2;
              char *href_end = strchr (href_start, ')');

              if (href_end)
                {
                  int tag_len = 15;
                  sprintf (replaced + len, "<a href=\"%.*s\">%.*s</a>",
                                          (int) (href_end - href_start), href_start,
                                          (int) (anc_end - anc_start), anc_start);
                  len += (href_end - anc_start - 2) + tag_len;
                  ptr = href_end + 1;

                  continue;
                }
            }
        }

      replaced[len++] = *ptr++;
      if (len >= size)
        {
          size <<= 1;
          replaced = realloc (replaced, size);
        }
    }

  replaced[len] = '\0';
  /* give ownership */
  return replaced;
}

static void
syntax_highlight_block (char     *codeblk,
                        Lang      lang,
                        HTMLFile *file)
{
  char *highlighted = NULL;

  highlighted = syntax_highlight (codeblk, lang);

  if (highlighted != NULL)
    {
      FWRITE_STR (highlighted, file);
      free (highlighted);
    }
}

static void
flush_content (HTMLFile *file,
               HTMLUnit *unit)
{
  if (tags[unit->tag].start_tag)
    FWRITE_STR (tags[unit->tag].start_tag, file);

  if (unit->content)
    {
      if (unit->tag == HTML_TAG_CODE_BLOCK)
        {
          if (unit->lang == LANG_NONE || unit->lang == LANG_HTML)
            {
              FWRITE_STR (unit->content, file);
            }
          else
            {
              syntax_highlight_block (unit->content, unit->lang, file);
            }
        }
      else
        {
          char *replaced = NULL;

          replaced = format_text (unit->content);
          FWRITE_STR (replaced, file);

          free (replaced);
        }
    }

  if (tags[unit->tag].end_tag)
    FWRITE_STR (tags[unit->tag].end_tag, file);
}

static void
pre_format (HTMLFile *file,
            HTML     *html,
            int       index)
{
  HTMLUnit *unit = NULL;

  unit = html->html[index];

  if (unit->tag == HTML_TAG_LI &&
      (index == 0 || html->html[index - 1]->tag != HTML_TAG_LI))
    {
      UL_TOP_LEVEL_START (file);
    }

  if (unit->tag != HTML_TAG_NEWLINE)
    {
      if (html->document && !tag_is_code_block (unit->tag))
        INSERT_TABSPACE (file);

      if (unit->tag == HTML_TAG_LI)
        INSERT_TABSPACE (file);
    }
}

static void
post_format (HTMLFile *file,
             HTML     *html,
             uint      index)
{
  HTMLUnit *unit = NULL;

  unit = html->html[index];

  /*
   * Do not add <br>
   *  1. after a heading
   *  2. inside a code block
   *  3. if previous element was a heading
   *
   */
  if (!(tag_is_heading (unit->tag)  ||
        tag_is_code_block (unit->tag) ||
        tag_is_list (unit->tag) ||
       ((index != 0) && unit->tag == HTML_TAG_NEWLINE && tag_is_heading (html->html[index - 1]->tag))))
    INSERT_LINEBREAK (file);

  if (unit->tag == HTML_TAG_LI &&
      (index == html->n_lines - 1 || html->html[index + 1]->tag != HTML_TAG_LI))
    {
      UL_TOP_LEVEL_END (file);
    }

  INSERT_NEWLINE (file);
}

/*
 * flush_html
 * @html: HTML doc
 *
 * flushed HTML doc into a html file
 */
void
flush_html (HTML *html)
{
  HTMLFile *file = NULL;

  file = fopen (html->file_name, "w+");

  if (html->document)
    init_template (file, html);

  for (uint i = 0; i < html->n_lines; i++)
    {
      HTMLUnit *unit = NULL;

      unit = html->html[i];

      pre_format (file, html, i);

      flush_content (file, unit);

      post_format (file, html, i);
    }

  if (html->document)
    final_template (file);

  fclose (file);
}
