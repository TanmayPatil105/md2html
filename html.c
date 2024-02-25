/*
 * file: html.c
 */

#include <stdlib.h>
#include "html.h"


/*
 * @default HTML values
 */
#define DEFAULT_HTML_FILE_NAME "index.html"
#define DEFAULT_HTML_TITLE     "Document"


static void
html_init (HTML **html)
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
}

/*
 * init_template
 * @file: HTMLFile
 *
 * inject template HTML code in a file
 */
static void
init_template (HTMLFile *file)
{
  char *template =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "<title>Document</title>\n"
    "</head>\n"
    "<body>\n"
    //"\n"
    "</body>\n"
    "</html>\n";

  /* inject template HTML */
  fprintf (file, "%s", template);
}

HTML*
md_to_html (MD *md)
{
  HTML *html = NULL;

  html_init (&html);

  return html;
}

void
flush_html (HTML *html)
{
  HTMLFile *file = NULL;

  file = fopen (html->file_name, "w+");

  init_template (file);
}
