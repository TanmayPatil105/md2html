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
  {HTML_TAG_IMG, NULL, NULL},
  {HTML_TAG_BLOCKQUOTE, "<blockquote><q>", "</q></blockquote>"},
  {HTML_TAG_CODE_BLOCK_START, "<pre>", NULL},
  {HTML_TAG_CODE_BLOCK_END, NULL, "</pre>"},
  {HTML_TAG_CODE_BLOCK_LINE, NULL, NULL},
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
      case UNIT_TYPE_IMAGE:
        return HTML_TAG_IMG;
      case UNIT_TYPE_QUOTE:
        return HTML_TAG_BLOCKQUOTE;
      case UNIT_TYPE_NONE:
        return HTML_TAG_NEWLINE;
      case UNIT_TYPE_CODE_BLOCK_START:
        return HTML_TAG_CODE_BLOCK_START;
      case UNIT_TYPE_CODE_BLOCK_END:
        return HTML_TAG_CODE_BLOCK_END;
      case UNIT_TYPE_CODE_BLOCK_LINE:
        return HTML_TAG_CODE_BLOCK_LINE;
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
                MDUnit    *md_unit)
{
  *unit = (HTMLUnit *) malloc (sizeof (HTMLUnit));
  memset(*unit, 0, sizeof(HTMLUnit));

  // malloc fails
  if (*unit == NULL)
    {
      // panic
      return;
    }

  (*unit)->tag = find_html_tag (md_unit->type);
  if (md_unit->content != NULL)
    (*unit)->content = strdup (md_unit->content);
  (*unit)->uri = md_unit->uri;
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
  char template[1000];

  if (html->title == NULL)
    {
      if (html->html != NULL && (unit = html->html[0]) && unit->tag == HTML_TAG_H1)
        html->title = strdup (unit->content);
      else
        html->title = strdup (__DEFAULT_HTML_TITLE__);
    }

  sprintf (template,
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "\t<meta charset=\"UTF-8\">\n"
    "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "\t<title>%s</title>\n"
    "</head>\n"
    "<body>", html->title);

  /* inject init template HTML */
  fprintf (file, "%s", template);
}

static void
final_template (HTMLFile *file)
{
  char template[20];

  sprintf (template,
    "\n</body>\n"
    "</html>\n");

  /* inject final template HTML */
  fprintf (file, "%s", template);
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

  /* custom file_name */
  if (params->o_file != NULL)
    html->file_name = strdup (params->o_file);

  if (params->title != NULL)
    html->title = strdup (params->title);

  unit = md->elements;
  while (unit != NULL)
    {
      HTMLUnit *html_unit = NULL;

      html_unit_init (&html_unit, unit);

      html->html[i++] = html_unit;
      unit = unit->next;
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

static bool
tag_is_heading (HTMLTag tag)
{
  return tag == HTML_TAG_H1 ||
         tag == HTML_TAG_H2 ||
         tag == HTML_TAG_H3 ||
         tag == HTML_TAG_BLOCKQUOTE;
}


static bool
tag_is_code_block (HTMLTag tag)
{
  return tag == HTML_TAG_CODE_BLOCK_LINE ||
         tag == HTML_TAG_CODE_BLOCK_START ||
         tag == HTML_TAG_CODE_BLOCK_END;
}

void
insert_img_tag (HTMLFile *file,
                char     *uri)
{
  char img[200];
  sprintf (img, "<img src=\"%s\">", uri);
  fwrite (img, sizeof (char), strlen (img), file);
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

  init_template (file, html);

  for (uint i = 0; i < html->n_lines; i++)
    {
      HTMLUnit *unit = NULL;

      unit = html->html[i];

      /* print NEWLINE for formatting */
      INSERT_NEWLINE (file);

      if (unit->tag == HTML_TAG_LI &&
          (i == 0 ||
           html->html[i-1]->tag != HTML_TAG_LI))
        {
          UL_TOP_LEVEL_START (file);
        }

      if (unit->tag != HTML_TAG_CODE_BLOCK_LINE)
        INSERT_TABSPACE (file);

      if (unit->tag == HTML_TAG_LI)
        INSERT_TABSPACE (file);

      if (tags[unit->tag].start_tag)
        fwrite (tags[unit->tag].start_tag, sizeof (char), strlen (tags[unit->tag].start_tag), file);

      if (unit->content)
        fwrite (unit->content, sizeof (char), strlen (unit->content), file);

      if (tags[unit->tag].end_tag)
        fwrite (tags[unit->tag].end_tag, sizeof (char), strlen (tags[unit->tag].end_tag), file);

      if (unit->uri && unit->tag == HTML_TAG_IMG)
        insert_img_tag (file, unit->uri);

      if (!tag_is_heading (unit->tag) && !tag_is_code_block (unit->tag))
        INSERT_LINEBREAK (file);

      if (unit->tag == HTML_TAG_LI &&
          (i == html->n_lines - 1 ||
           html->html[i+1]->tag != HTML_TAG_LI))
        {
          UL_TOP_LEVEL_END (file);
        }
    }

  final_template (file);

  fclose (file);
}
