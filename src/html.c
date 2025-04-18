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

/* local reference */
static Footnotes *footnotes = NULL;

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

  footnotes = md->notes;

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

#define OFFSET 400

struct fmt {
  const char *start_pattern;
  const char *end_pattern; /* end_pattern is needed here because
                            * the ending underscores need a whitespace
                            * at the end to be valid */
  const char *start_tag;
  const char *end_tag;
} regx[] = {
  { "***", "***",  "<b><i>", "</i></b>" },
  { "**",  "**",   "<b>",    "</b>"     },
  { "*",   "*",    "<i>",    "</i>"     },
  { "___", "___ ", "<b><i>", "</i></b>" },
  { "__",  "__ ",  "<b>",    "</b>"     },
  { "_",   "_ ",   "<i>",    "</i>"     },
  { "`",   "`",    "<code>", "</code>"  },
};


static char *
format_text (char *content)
{
  char *replaced = NULL;
  char *ptr = NULL;
  size_t size = 256;
  size_t len = 0, write;
  size_t n_regex = sizeof (regx) / sizeof (regx[0]);

  replaced = malloc (sizeof (char) * size);
  ptr = content;

  while (*ptr)
    {
      bool in_regx = false;

      for (size_t i = 0; i < n_regex; i++)
        {
          int offset = strlen (regx[i].start_pattern);

          if (strncmp (ptr, regx[i].start_pattern, offset) == 0) {
              char *start, *end;

              start = ptr + offset;
              end = strstr (start, regx[i].end_pattern);

              if (end != NULL)
                {
                  write = sprintf (replaced + len, "%s%.*s%s",
                                                   regx[i].start_tag,
                                                   (int)(end - start), start,
                                                   regx[i].end_tag);

                  len += write;
                  ptr = end + offset;
                  in_regx = true;
                }
            }
        }

      if (in_regx)
        {
          continue;
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
                  write = sprintf (replaced + len, "<img src=\"%.*s\" alt=\"%.*s\">",
                                                   (int) (src_end - src_start), src_start,
                                                   (int) (alt_end - alt_start), alt_start);
                  len += write;
                  ptr = src_end + 1;

                  continue;
                }
            }
        }
      else if (*ptr == '[' && * (ptr + 1) == '^')
        {
          char *id_start = ptr + 2;
          char *id_end;

          id_end = strchr (id_start, ']');

          if (id_end)
            {
              char *id = NULL;
              int id_len;
              Reference *ref;

              id_len = id_end - id_start;
              ptr += id_len + 3;


              id = calloc (id_len + 1, sizeof (char));
              strncpy (id, id_start, id_len);

              ref = footnotes_get_ref (footnotes, id);
              if (ref != NULL)
                {
                  uuid_t uuid;
                  uuid_generate_random (uuid);

                  write = sprintf (replaced + len, "<a href=\"#fn-%s\" id=\"fnref-%s\">"
                                                    "<sup>%d</sup>"
                                                    "</a>",
                                                    ref->uuid, uuid, ref->index);

                  footnotes_add_referrer (footnotes, ref->index, uuid);
                }
              else
                {
                  write = sprintf (replaced + len, "<a href=\"#\"><sup>?</sup></a>");
                }

              free (id);
              len += write;
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
                  write = sprintf (replaced + len, "<a href=\"%.*s\">%.*s</a>",
                                          (int) (href_end - href_start), href_start,
                                          (int) (anc_end - anc_start), anc_start);
                  len += write;
                  ptr = href_end + 1;

                  continue;
                }
            }
        }

      replaced[len++] = *ptr++;
      if (len + OFFSET >= size)
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
flush_footnotes (HTMLFile *file)
{
  int n_refs;

  n_refs = footnotes_get_count (footnotes);

  if (n_refs == 0)
    return;

  FWRITE_STR ("\n\t<hr>\n", file);

  for (int i = 0; i < n_refs; i++)
    {
      Reference *ref;

      ref = footnotes_get_ref_from_index (footnotes, i);

      INSERT_TABSPACE (file);
      fprintf (file, "<p id=\"fn-%s\">"
                     "\t\t%d. %s",
                     ref->uuid, ref->index, ref->text);

      for (int j = 0; j < ref->n_referrers; j++)
        {
          if (j != 0)
            fprintf (file,
                     "<a href=\"#fnref-%s\">↩︎<sup>%d</sup></a>",
                     ref->referrers[j], j + 1);
          else
            fprintf (file,
                     "<a href=\"#fnref-%s\">↩︎</a>",
                     ref->referrers[j]);
        }

      fprintf (file, "</p>\n");

    }
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

  flush_footnotes (file);

  if (html->document)
    final_template (file);

  fclose (file);
}
