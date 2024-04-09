/*
 * file: html.c
 */

#include <stdlib.h>
#include <string.h>
#include "html.h"


/*
 * @default HTML values
 */
#define DEFAULT_HTML_FILE_NAME "index.html"
#define DEFAULT_HTML_TITLE     "Document"

/*
 * @literals
 */
#define NEWLINE  "\n"
#define TABSPACE "\t"

typedef struct {
  HTMLTag key;
  char *start_tag;
  char *end_tag;
} html_tags;

static html_tags tags[] = {
  {HTML_TAG_H1, "<h1>", "</h1>"},
  {HTML_TAG_H2, "<h2>", "</h2>"},
  {HTML_TAG_H3, "<h3>", "</h3>"},
  {HTML_TAG_LI, "<li>", "</li>"},
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
  /* pass ownership of content */
  (*unit)->content = md_unit->content;
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
  char template[200];

  sprintf (template,
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "\t<meta charset=\"UTF-8\">\n"
    "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "\t<title>%s</title>\n"
    "</head>\n"
    "<body>\n", html->title);

  /* inject init template HTML */
  fprintf (file, "%s", template);
}

static void
final_template (HTMLFile *file)
{
  char template[20];

  sprintf (template,
    "</body>\n"
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
              char *file_name)
{
  uint i = 0;
  MDUnit *unit = NULL;
  HTML *html = NULL;

  html_init (&html, md->n_lines);

  /* custom file_name */
  if (file_name != NULL)
    html->file_name = file_name;

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

      if (unit->tag == HTML_TAG_LI && ( i == 0 || html->html[i-1]->tag != HTML_TAG_LI))
        {
          fwrite (TABSPACE, sizeof (char), 1, file);
          fwrite ("<ul>", sizeof (char), strlen ("<ul>"), file);
          fwrite (NEWLINE, sizeof (char), 1, file);
        }

      fwrite (TABSPACE, sizeof (char), 1, file);

      if (unit->tag == HTML_TAG_LI)
        fwrite (TABSPACE, sizeof (char), 1, file);

      if (tags[unit->tag].start_tag)
        fwrite (tags[unit->tag].start_tag, sizeof (char), strlen (tags[unit->tag].start_tag), file);

      // FIXME: do not print newlines
      if (unit->content)
        fwrite (unit->content, sizeof (char), strlen (unit->content), file);

      if (tags[unit->tag].end_tag)
        fwrite (tags[unit->tag].end_tag, sizeof (char), strlen (tags[unit->tag].end_tag), file);

      if (unit->uri && unit->tag == HTML_TAG_IMG)
        {
          char img[200];
          sprintf (img, "<img src=\"%s\">", unit->uri);
          fwrite (img, sizeof (char), strlen (img), file);
        }

      fwrite (NEWLINE, sizeof (char), 1, file);

      if (unit->tag == HTML_TAG_LI && ( i == html->n_lines - 1 || html->html[i+1]->tag != HTML_TAG_LI))
        {
          fwrite (TABSPACE, sizeof (char), 1, file);
          fwrite ("</ul>", sizeof (char), strlen ("</ul>"), file);
          fwrite (NEWLINE, sizeof (char), 1, file);
        }
    }

  final_template (file);

  fclose (file);
}
