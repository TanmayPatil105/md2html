# Diff

```diff
diff --git a/include/lang.h b/include/lang.h
index 8b0975e..cf23065 100644
--- a/include/lang.h
+++ b/include/lang.h
@@ -27,5 +27,6 @@
 typedef enum
 {
   LANG_C = 0,     /* C programming language */
+  LANG_DIFF,      /* DIFF */
   LANG_NONE,
 } Lang;
diff --git a/include/xml.h b/include/xml.h
index a6d0edc..ba96298 100644
--- a/include/xml.h
+++ b/include/xml.h
@@ -24,7 +24,7 @@
 #include <stddef.h>
 
 const char *xml_char_replace    (char c);
-void        xml_sanitize_strcpy (char   *dest,
+size_t      xml_sanitize_strcpy (char   *dest,
                                  char   *src,
                                  size_t  n);
 
diff --git a/src/md.c b/src/md.c
index ab080c4..8459a04 100644
--- a/src/md.c
+++ b/src/md.c
@@ -219,15 +219,18 @@ find_md_content (char    *line,
 static Lang
 find_code_block_lang (char *line)
 {
-  char *lang = NULL;
+  Lang lang = LANG_NONE;
+  char *str = NULL;
 
   /* We already know it's a codeblock start */
-  lang = line + 3;
+  str = line + 3;
 
-  if (strncmp (lang, "c", 1) == 0)
-    return LANG_C;
+  if (strncmp (str, "c", 1) == 0)
+    lang = LANG_C;
+  if (strncmp (str, "diff", 4) == 0)
+    lang = LANG_DIFF;
 
-  return LANG_NONE;
+  return lang;
 }
 
 /*
diff --git a/src/syntax.c b/src/syntax.c
index 1db76b7..8058810 100644
--- a/src/syntax.c
+++ b/src/syntax.c
@@ -451,6 +451,98 @@ highlight_keywords (char                 *codeblk,
   return highlighted;
 }
 
+static bool
+diff_keyword (char        *line,
+              const char **color)
+{
+  bool ret = false;
+
+  if (strncmp (line, "+", 1) == 0)
+    {
+      ret = true;
+      *color = "#228B22";
+    }
+  else if (strncmp (line, "-", 1) == 0)
+    {
+      ret = true;
+      *color = "#DC143C";
+    }
+	else if (strncmp (line, "@@", 2) == 0)
+    {
+      ret = true;
+      *color = "#4682B4";
+    }
+
+  return ret;
+}
+
+static char *
+highlight_diff (char *diffblk)
+{
+  size_t size = 1000, count = 0;
+  char *highlighted = NULL;
+  char *token = NULL;
+  char *cpy;
+  const char *strs[5] = {
+    "<font color=\"", NULL, "\">",
+    NULL,
+    "</font>"
+  };
+
+  highlighted = malloc (sizeof (char) * size);
+
+  diffblk += 1;
+
+  token = strtok (diffblk, "\n");
+
+  while (token != NULL)
+    {
+      size_t len;
+
+      len = strlen (token);
+
+      if (len + count + 28 >= size)
+        {
+          size <<= 1;
+
+          highlighted = realloc (highlighted, size);
+        }
+
+      if (diff_keyword (token, &strs[1]))
+        {
+          cpy = &highlighted[count];
+
+          for (int i = 0; i < 5; i++)
+            {
+              if (i != 3)
+                {
+                  cpy = stpcpy (cpy, strs[i]);
+                  count += strlen (strs[i]);
+                }
+              else
+                {
+                  size_t write;
+
+                  write = xml_sanitize_strcpy (cpy,
+                                                token, len);
+                  cpy += write;
+                  count += write;
+                }
+            }
+        }
+      else
+        {
+          count += xml_sanitize_strcpy (&highlighted[count], token, len);
+        }
+
+      highlighted[count++] = '\n';
+
+      token = strtok (NULL, "\n");
+    }
+
+  return highlighted;
+}
+
 char *
 syntax_highlight (char *codeblk,
                   Lang  lang)
@@ -465,6 +557,9 @@ syntax_highlight (char *codeblk,
         highlighted = highlight_keywords (codeblk,
                                           c_keywords, n_types);
         break;
+      case LANG_DIFF:
+        highlighted = highlight_diff (codeblk);
+        break;
       default:
         highlighted = NULL;
     }
diff --git a/src/xml.c b/src/xml.c
index de6b87e..418682d 100644
--- a/src/xml.c
+++ b/src/xml.c
@@ -75,11 +75,12 @@ xml_char_replace (char c)
   return ret;
 }
 
-void
+size_t
 xml_sanitize_strcpy (char   *dest,
                      char   *src,
                      size_t  n)
 {
+  size_t len = 0;
   size_t count = 0;
   char *ptr = dest;
 
@@ -92,15 +93,18 @@ xml_sanitize_strcpy (char   *dest,
       if (repl != NULL)
         {
           ptr = stpcpy (ptr, repl);
+          len += strlen (repl);
         }
       else
         {
           *ptr = *src;
-          ptr++;
+          ptr += 1;
+          len += 1;
         }
 
       src++;
       count++;
     }
-}
 
+  return len;
+}
```
