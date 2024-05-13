/*
 * file: html.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "html.h"
#include "macro.h"

/*
 * @default HTML values
 */
#define DEFAULT_HTML_FILE_NAME "index.html"
#define DEFAULT_HTML_TITLE     "Document"

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
  {HTML_TAG_NONE, NULL, NULL},
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

  (*html)->file_name = DEFAULT_HTML_FILE_NAME;
  (*html)->title = DEFAULT_HTML_TITLE;
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
  *unit = malloc (sizeof (HTMLUnit));

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
  char template[1000];

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
html_from_md (MD   *md,
              char *file_name,
              char *title)
{
  uint i = 0;
  MDUnit *unit = NULL;
  HTML *html = NULL;

  html_init (&html, md->n_lines);

  /* custom file_name */
  if (file_name != NULL)
    html->file_name = file_name;

  if (title != NULL)
    html->title = title;

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
