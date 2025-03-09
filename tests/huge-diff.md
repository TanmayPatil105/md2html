```diff
diff --git a/docs/acrostic-editing.md b/docs/acrostic-editing.md
new file mode 100644
index 00000000..078e30ae
--- /dev/null
+++ b/docs/acrostic-editing.md
@@ -0,0 +1,279 @@
+# Editing the Acrostic Grid
+
+**Status:** Approved | January 2025
+
+**Author:** Jonathan Blandford
+
+**Reviewer:** Tanmay Patil
+
+# Overview
+
+Acrostic puzzles have some unique constraints that need to be
+satisfied before they're a _well formed_ puzzle. The editor lets you
+edit parts of the constraints putting the puzzle in _partially formed_
+or _error_ states. Changing some of the constraints can be
+particularly destructive. It's not clear what grid editing actions
+should exist and what's an acceptable level of destruction. In
+addition, it's not clear what the correct behavior for the `_fix()`
+functions — and the UI — should be.
+
+# Problem Statement
+
+Acrostics have the following constraints:
+
+1. The quote string can be read from the grid (and vice versa)
+2. The first letter of each answer spell out the source string
+3. Every cell in the grid is used exactly once for an answer, and has
+   a 1:1 mapping between a clue and a cell.
+
+We introduce the concept of the acrostic being in a _well formed_,
+_partially formed_, or an _error state_ in the editor.
+
+When all the contraints are met and the puzzle is complete, it's in a
+well-formed state.  When constraints are all possible to be met, but
+we don't have a full set of answers, the puzzle is in a partially
+formed state. When we have contradictions in the set of characters
+then we're in an error state.
+
+We don't have the APIs currently to handle the partially formed and
+error states. Consequentially the puzzle is hard to work with in the
+editor.
+
+## Existing fix functions
+
+`IpuzAcrostic` has the following fix functions:
+
+* **fix_quote():** Syncs between the grid and to the quote
+  string. Takes an argument to indicate direction of
+  synchronization. Will (potentially) resize the board and clears all
+  quote strings.
+* **fix_source():** Syncs between the clues and the source
+  string. Takes an argument to indicate direction of
+  synchronization. Changes the number of clues in the puzzle and
+  clears them.
+* **set_answers():** Takes a list of answers that satisfies the
+  constraints set by the quote and source strings. It will set the
+  clues. Unlike the `quote_str` and `source_str`, the answers are
+  stored as the clue cells and aren't kept as a lookaside value.
+* **fix_labels():** Updates the labels of every cell to match that of
+  the clue.
+
+# Proposal
+
+**tl;dr:** Never let the puzzle get into an error state in the editor
+as the result of a user action.
+
+## Supported User Actions
+
+We want to support the following user actions in the editor:
+
+* Edit the quote string
+* Edit the source string
+* Edit and write individual answers
+* Use the autofill functionality on either a portion of the answer space, or the full puzzle.
+
+### Edit the quote string
+
+_The user edits the quote string_
+
+Editing the quote string will change the size and shape of the grid,
+as well as the valid characters for the grid. It will also
+(potentially) invalidate any clues that currently exists. It's
+possible to make minor changes to a quote and keep the puzzle largely
+the same. Alternatively, it's also possible to put the puzzle into an
+error state invalidating everything.
+
+When the user changes the quote string, we should see if the source
+string still works with the original quote. If so we are in a
+partially formed state, otherwise we're in an error state. We can then
+choose the actions in one of the following two options:
+
+**Option 1: Partially Formed**
+
+1. Take a snapshot of all existing answers in the puzzle.
+1. Recreate the grid with the new quote string
+1. Go through each answer serially as they exists, and see if it can
+   be used with the quote letters. Add back the ones that work. Add
+   any clues as well.
+1. Update the puzzle labels to not include clues
+1. Write the puzzle to the undo stack.
+
+**Option 2: Error State**
+
+1. Possibly warn the user about a destructive change?
+1. Recreate the grid with the new quote string
+1. Clear the source string
+1. Clear all answers
+1. Update the puzzle labels to not include clues
+1. Write the puzzle to the undo stack.
+
+That would make the callback look something like:
+
+```C
+static void
+quote_string_changed_cb (quote_str)
+{
+  answer_list = snapshot_answers ();
+  set_quote_str (quote_str);
+  fix_quote_str ();
+  state = check_acrostic_state ();
+  if (state == ERROR)
+    {
+       set_source_str ("");
+       fix_source_str ();
+       // state should be partial now, by definition
+    }
+  else if (state == PARTIAL)
+    {
+       for (guint i = 0; i < answer_list->len; i++)
+         {
+           // This call will fail if there aren't enough letters
+           set_answer (i, answer_list[i]);
+         }
+       // Recheck to see if all the letters are used
+       state = check_acrostic_state ();
+    }
+
+  if (state == WELL_FORMED)
+    fix_labels (CLUES);
+  else
+    fix_labels (NO_CLUES);
+
+  push_change ();
+}
+```
+
+### Edit the source string
+
+_The user edits the source string_
+
+We should make setting of a source to be secondary to the quote, and
+prevent the user from writing one that's not a subset of its
+characters.
+
+Editing the source string will change the number of answers, or
+as well as the valid characters for the grid. It will also
+(potentially) invalidate any clues that currently exists.
+
+1. Take a snapshot of all existing answers in the puzzle.
+1. Recreate the answers with the new first letters
+1. Go through each answer serially as they exists, and see if it can
+   be used with the new initial letters and quote letters. Add back
+   the ones that work.
+1. Write the answers to the puzzle
+1. Update the puzzle labels to not include clues
+1. Write the puzzle to the undo stack.
+
+That would make the callback look something like:
+
+```C
+static void
+source_string_changed_cb (source_str)
+{
+  answer_list = snapshot_answers ();
+  set_source_str (quote_str);
+  fix_grid ();
+  state = check_acrostic_state ();
+  if (state == ERROR)
+    {
+       // We shouldn't let you set a source_str that's invalid
+       g_assert_not_reached();
+    }
+  else if (state == PARTIAL)
+    {
+       for (guint i = 0; i < answer_list->len; i++)
+         {
+           set_answer (i, answer_list[i]);
+         }
+       fix_labels (NO_CLUES);
+    }
+  else // Well formed puzzle
+    {
+      fix_labels (NO_CLUES);
+    }
+  push_change ();
+}
+```
+
+### Edit an answer
+
+This is a little simpler from the perspective of the puzzle, though
+perhaps a more complex API. The one thing we do is update the
+answer. One challenge is that we don't want to have the puzzle in an error state, which means the answer widget should only emit
+
+```C
+static void
+source_answer_changed_cb (index, new_answer)
+{
+  set_answer (index, new_answer);
+  state = check_acrostic_state ();
+
+  if (state == ERROR)
+    g_assert_not_reached();
+  else if (state == PARTIAL)
+    fix_labels (NO_CLUES);
+  else // Well formed puzzle
+    fix_labels (NO_CLUES);
+  push_change ();
+}
+```
+
+
+## Libipuz changes
+
+We need to be able to represent the puzzle in libipuz when it's in a
+partially-formed state. This is for two reasons. First, we need to be
+able to push the puzzle to the undo stack when it's in this form. We
+have to capture those changes when they occur. Second, the user may
+want to save an acrostic while in the middle of creating it.
+
+As a convention, I propose we don't update the labels in the editor to
+include the clue numbers when we're in a partially formed state. That
+will avoid the numbers bouncing around when editing, and give a visual
+indication that it's not done. We may want to write them out when
+saving.
+
+To implement this we should:
+
+* Change `set_answers()` to accept a partial list of answers, or
+  potentially just one answer.
+* Change `fix_labels()` to take an argument about whether it should
+  map the clues, or just include a cell number.
+
+> **NOTE:** It's possible to manually set the puzzle into an _error
+> state_ and save it through raw calls to the library, or through
+> manually editing the file. The editor shouldn't allow that, and
+> should fix up the puzzle as best as possible when loading from disk.
+
+## Post push
+
+Once a new acrostic puzzle has been pushed, we need to prepare it for
+updating widgets. One important thing to do is to recalculate the
+Charset of characters in the puzzle, and the Charset of the current
+set of answers. If they're identical, then the puzzle is well
+formed. That will be useful to pass to the various `update()`
+functions.
+
+# Actions
+
+- [ ] Create widgets for editing an acrostic grid.
+    - [ ] Widget for `quote_str`
+    - [ ] Widget for `source_str`
+    - [ ] Widget for `answers`. The autofill fill feature will be
+          embedded in the answer widget.
+- [ ] Setup callbacks for widgets
+- [ ] Post push propagation and validation
+- [X] _(libipuz)_ Change `fix_labels()` to take an enum indicating how to take the
+- [ ] _(libipuz)_ Add an `set_answer()` function equivalent for just one answer.
+- [X] _(libipuz)_ Add `ipuz_charset_subset()`
+
+
+# Other Thoughts
+
+One other long-standing action is to make `IpuzAcrostic` inherit from
+`IpuzGrid`. We don't really use any of the functions from
+`IpuzCrossword` other than `fix_all()` and `fix_style)_`.
+
+It's good to not refactor too much at once so we will do that as a
+separate action. Bbut care should be taken in the implementation to
+make sure that we don't make that task harder.
diff --git a/src/acrostic-generator.c b/src/acrostic-generator.c
index a45561e9..776ad045 100644
--- a/src/acrostic-generator.c
+++ b/src/acrostic-generator.c
@@ -1,6 +1,6 @@
 /* acrostic-generator.c
  *
- * Copyright 2023 Tanmay Patil <tanmaynpatil105@gmail.com>
+ * Copyright 2023-2024 Tanmay Patil <tanmaynpatil105@gmail.com>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
@@ -38,6 +38,7 @@ enum
   PROP_SOURCE_STR,
   PROP_CHARSET,
   PROP_WORD_LIST,
+  PROP_SKIP_TABLE,
   N_PROPS
 };
 
@@ -47,7 +48,6 @@ static GParamSpec *obj_props[N_PROPS] = {NULL, };
 struct _AcrosticGenerator
 {
   GObject parent_object;
-  guint64 counter;
   guint32 random_seed;
   IpuzCharset *charset;
 
@@ -59,6 +59,7 @@ struct _AcrosticGenerator
 
   /* Word List */
   WordList *word_list;
+  GHashTable *skip_table;
 
   /* Word size */
   guint max_word_size;
@@ -67,6 +68,12 @@ struct _AcrosticGenerator
   /* Grid */
   guint n_rows;
   guint n_columns;
+
+  GCancellable *cancellable;
+
+  gint *count;
+  GMutex *results_mutex;
+  GArray *results;
 };
 
 static void acrostic_generator_init         (AcrosticGenerator      *self);
@@ -80,28 +87,39 @@ static void acrostic_generator_get_property (GObject                *object,
                                              GValue                 *value,
                                              GParamSpec             *pspec);
 static void acrostic_generator_finalize     (GObject                *object);
+static void acrostic_generator_set_run_info (PuzzleTask             *self,
+                                             gint                   *count,
+                                             GMutex                 *results_mutex,
+                                             GArray                 *results);
+static void acrostic_generator_run          (GTask                  *task,
+                                             GObject                *source_object,
+                                             gpointer                task_data,
+                                             GCancellable           *cancellable);
 
 
-G_DEFINE_TYPE (AcrosticGenerator, acrostic_generator, G_TYPE_OBJECT);
+G_DEFINE_TYPE (AcrosticGenerator, acrostic_generator, PUZZLE_TYPE_TASK);
 
 static void
 acrostic_generator_init (AcrosticGenerator *self)
 {
   self->max_word_size = MAX_WORD_SIZE;
   self->min_word_size = MIN_WORD_SIZE;
-  self->counter = 0;
 }
 
 static void
 acrostic_generator_class_init (AcrosticGeneratorClass *klass)
 {
   GObjectClass *object_class;
+  PuzzleTaskClass *task_class;
 
   object_class = G_OBJECT_CLASS (klass);
+  task_class = PUZZLE_TASK_CLASS (klass);
 
   object_class->set_property = acrostic_generator_set_property;
   object_class->get_property = acrostic_generator_get_property;
   object_class->finalize = acrostic_generator_finalize;
+  task_class->run = acrostic_generator_run;
+  task_class->set_run_info = acrostic_generator_set_run_info;
 
   obj_props[PROP_RANDOM_SEED] = g_param_spec_uint ("random-seed",
                                                    "Random seed",
@@ -149,6 +167,12 @@ acrostic_generator_class_init (AcrosticGeneratorClass *klass)
                                                    WORD_TYPE_LIST,
                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
 
+  obj_props[PROP_SKIP_TABLE] = g_param_spec_boxed ("skip-table",
+                                                   NULL, NULL,
+                                                   G_TYPE_HASH_TABLE,
+                                                   G_PARAM_READWRITE);
+
+
   g_object_class_install_properties (object_class, N_PROPS, obj_props);
 }
 
@@ -190,6 +214,11 @@ acrostic_generator_set_property (GObject      *object,
       g_clear_object (&self->word_list);
       self->word_list = g_value_dup_object (value);
       break;
+    case PROP_SKIP_TABLE:
+      if (self->skip_table)
+        g_hash_table_destroy (self->skip_table);
+      self->skip_table = g_value_get_boxed (value);
+      break;
     default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       break;
@@ -231,6 +260,9 @@ acrostic_generator_get_property (GObject    *object,
     case PROP_WORD_LIST:
       g_value_set_object (value, self->word_list);
       break;
+    case PROP_SKIP_TABLE:
+      g_value_set_boxed (value, self->skip_table);
+      break;
     default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
       break;
@@ -247,7 +279,9 @@ acrostic_generator_finalize (GObject *object)
   g_clear_pointer (&self->quote_str, g_free);
   g_clear_pointer (&self->source_str, g_free);
   g_clear_pointer (&self->clues, g_array_unref);
+  g_clear_object (&self->cancellable);
   g_clear_object (&self->word_list);
+  g_clear_pointer (&self->skip_table, g_hash_table_unref);
   ipuz_charset_unref (self->charset);
 
   G_OBJECT_CLASS (acrostic_generator_parent_class)->finalize (object);
@@ -261,6 +295,7 @@ acrostic_generator_new (void)
   GObject *object;
   IpuzCharsetBuilder *builder;
   IpuzCharset *charset;
+  GHashTable *skip_table;
   g_autoptr (WordList) word_list = NULL;
   GRand *g_rand;
   guint32 random_seed;
@@ -273,11 +308,14 @@ acrostic_generator_new (void)
   charset = ipuz_charset_builder_build (builder);
   builder = NULL;
   word_list = word_list_new ();
+  skip_table = g_hash_table_new_full (g_str_hash, g_str_equal,
+                                      g_free, NULL);
 
   object = g_object_new (ACROSTIC_TYPE_GENERATOR,
                          "random-seed", random_seed,
                          "charset", charset,
                          "word-list", word_list,
+                         "skip-table", skip_table,
                          NULL);
 
   g_rand_free (g_rand);
@@ -388,6 +426,9 @@ acrostic_generator_set_text (AcrosticGenerator *self,
   /* normalised quote and source strings */
   gchar *n_quote_str, *n_source_str;
 
+  g_return_val_if_fail (quote_str != NULL, FALSE);
+  g_return_val_if_fail (source_str != NULL, FALSE);
+
   n_quote_str = normalise_input (quote_str, self->charset);
   n_source_str = normalise_input (source_str, self->charset);
 
@@ -419,6 +460,7 @@ acrostic_generator_set_seed (AcrosticGenerator *self,
   self->random_seed = seed;
 }
 
+/* Answer entry */
 typedef struct {
   gunichar leading_char;
   const gchar *letters;
@@ -457,6 +499,30 @@ generate_random_lookup (guint size)
   return array;
 }
 
+static gboolean
+add_or_skip_word (AcrosticGenerator *self,
+                  const gchar       *word)
+{
+  if (word == NULL)
+    return FALSE;
+
+  if (g_hash_table_contains (self->skip_table, word))
+    return TRUE;
+
+  g_hash_table_insert (self->skip_table, g_strdup (word), GINT_TO_POINTER (TRUE));
+  return FALSE;
+}
+
+static void
+clean_up_word (AcrosticGenerator *self,
+               const gchar       *word)
+{
+  if (word == NULL)
+    return;
+
+  g_hash_table_remove (self->skip_table, word);
+}
+
 static gchar *
 generate_filter (gunichar leading_char,
                  guint    len,
@@ -524,6 +590,9 @@ acrostic_generator_helper (AcrosticGenerator  *self,
   g_autofree gchar *filter = NULL;
   g_autoptr (GArray) rand_offset = NULL;
 
+  if (g_cancellable_is_cancelled (self->cancellable))
+    return FALSE;
+
   if (index == clues->len)
     return TRUE;
 
@@ -540,20 +609,27 @@ acrostic_generator_helper (AcrosticGenerator  *self,
   for (guint i = 0; i < word_list_get_n_items (self->word_list); i++)
     {
       const gchar *word;
-      self->counter++;
+
+      g_atomic_int_inc (self->count);
+
       word = word_list_get_word (self->word_list, g_array_index (rand_offset, gushort, i));
 
       clue_entry->letters = word;
 
       /* Give some progress */
-      if (self->counter % 1000000 == 0)
+      if (*self->count % 1000000 == 0)
         dump_clues (clues);
 
       if (ipuz_charset_builder_remove_text (remaining_letters, word + 1))
         {
-          if (acrostic_generator_helper (self, clues, index + 1, remaining_letters))
+          if (!add_or_skip_word (self, word) &&
+               acrostic_generator_helper (self, clues, index + 1, remaining_letters))
             return TRUE;
 
+          if (g_cancellable_is_cancelled (self->cancellable))
+            break;
+
+          clean_up_word (self, word);
           ipuz_charset_builder_add_text (remaining_letters, word + 1);
           clue_entry->letters = NULL;
           word_list_set_filter (self->word_list, filter, WORD_LIST_MATCH);
@@ -591,14 +667,14 @@ initialize_clue_entries (const char *text)
   return clues;
 }
 
-static void
+static gboolean
 generate_random_lengths (GArray *clues,
                          guint   number,
                          guint   min_word_size,
                          guint   max_word_size)
 {
   if ((clues->len * max_word_size) < number)
-    return;
+    return FALSE;
 
   guint sum = 0;
 
@@ -616,8 +692,7 @@ generate_random_lengths (GArray *clues,
       clue_entry->word_length = len;
     }
 
-  if (sum != number)
-    generate_random_lengths (clues, number, min_word_size, max_word_size);
+  return sum == number;
 }
 
 static guint
@@ -633,10 +708,32 @@ minimise_max_word_size (guint quote_len,
   return MIN (max_word_size, minimised);
 }
 
-void
-acrostic_generator_run (AcrosticGenerator *self)
+static GArray *
+clue_entry_get_answers (GArray *clues)
 {
-  guint max_word_size;
+  GArray *answers = NULL;
+
+  g_return_val_if_fail (clues != NULL, NULL);
+  g_return_val_if_fail (clues->len > 0, NULL);
+
+  answers = g_array_new (FALSE, FALSE, sizeof(gchar *));
+
+  for (guint i = 0; i < clues->len; i++)
+    {
+      ClueEntry clue;
+
+      clue = g_array_index (clues, ClueEntry, i);
+
+      g_array_append_val (answers, clue.letters);
+    }
+
+  return answers;
+}
+
+static void
+acrostic_generator_run_helper (AcrosticGenerator *self)
+{
+  guint max_word_size, quote_length;
   GArray *clues;
   IpuzCharsetBuilder *quote_builder, *source_builder;
   g_autoptr (IpuzCharset) quote_charset = NULL;
@@ -654,14 +751,16 @@ acrostic_generator_run (AcrosticGenerator *self)
   srand(self->random_seed);
 
   clues = initialize_clue_entries (self->source_str);
+  quote_length = ipuz_charset_get_total_count (quote_charset);
 
-  max_word_size = minimise_max_word_size (ipuz_charset_get_total_count (quote_charset),
+  max_word_size = minimise_max_word_size (quote_length,
                                           ipuz_charset_get_total_count (source_charset),
                                           self->min_word_size, self->max_word_size);
 
-  generate_random_lengths (clues, ipuz_charset_get_total_count (quote_charset),
-                           self->min_word_size,
-                           max_word_size);
+
+  while (!generate_random_lengths (clues, quote_length,
+                                   self->min_word_size,
+                                   max_word_size));
 
   quote_builder = ipuz_charset_builder_new ();
   ipuz_charset_builder_add_text (quote_builder, self->quote_str);
@@ -679,7 +778,7 @@ acrostic_generator_run (AcrosticGenerator *self)
 
   if (!acrostic_generator_helper (self, clues, 0, quote_builder))
     {
-      g_print ("Didn't find a clue: counter (%lu)\n", self->counter);
+      g_print ("Didn't find a clue: counter (%ls)\n", self->count);
       dump_clues (clues);
       g_clear_pointer (&clues, g_array_unref);
       ipuz_charset_unref (ipuz_charset_builder_build (quote_builder));
@@ -692,5 +791,56 @@ acrostic_generator_run (AcrosticGenerator *self)
       ipuz_charset_unref (ipuz_charset_builder_build (quote_builder));
     }
 
-  self->clues = clues;
+  if (clues != NULL)
+    {
+      g_mutex_lock (self->results_mutex);
+
+      if (!g_cancellable_is_cancelled (self->cancellable))
+        {
+          GArray *answers;
+
+          answers = clue_entry_get_answers (clues);
+
+          g_array_append_val (self->results, answers);
+        }
+
+      g_mutex_unlock (self->results_mutex);
+    }
+}
+
+void
+acrostic_generator_run (GTask        *task,
+                        GObject      *source_object,
+                        gpointer      task_data,
+                        GCancellable *cancellable)
+{
+  AcrosticGenerator *self = ACROSTIC_GENERATOR (task_data);
+
+  g_return_if_fail (ACROSTIC_IS_GENERATOR (self));
+
+  self->cancellable = g_object_ref (cancellable);
+
+  acrostic_generator_run_helper (self);
+
+  g_task_return_pointer (task, NULL, NULL);
+}
+
+static void
+acrostic_generator_set_run_info (PuzzleTask *self,
+                                 gint       *count,
+                                 GMutex     *results_mutex,
+                                 GArray     *results)
+{
+  ACROSTIC_GENERATOR (self)->count = count;
+  ACROSTIC_GENERATOR (self)->results_mutex = results_mutex;
+  ACROSTIC_GENERATOR (self)->results = results;
+}
+
+void
+acrostic_generator_set_word_resource (AcrosticGenerator *self,
+                                      WordListResource  *resource)
+{
+  g_return_if_fail (ACROSTIC_IS_GENERATOR (self));
+
+  word_list_set_resource (self->word_list, resource);
 }
diff --git a/src/acrostic-generator.h b/src/acrostic-generator.h
index da56aec9..77fd1812 100644
--- a/src/acrostic-generator.h
+++ b/src/acrostic-generator.h
@@ -24,6 +24,8 @@
 
 #include <libipuz/libipuz.h>
 #include <glib-object.h>
+#include "puzzle-task.h"
+#include "word-list-resources.h"
 
 G_BEGIN_DECLS
 
@@ -40,7 +42,7 @@ GQuark acrostic_generator_error_quark (void);
 
 
 #define ACROSTIC_TYPE_GENERATOR (acrostic_generator_get_type())
-G_DECLARE_FINAL_TYPE (AcrosticGenerator, acrostic_generator, ACROSTIC, GENERATOR, GObject);
+G_DECLARE_FINAL_TYPE (AcrosticGenerator, acrostic_generator, ACROSTIC, GENERATOR, PuzzleTask);
 
 
 AcrosticGenerator      *acrostic_generator_new               (void);
@@ -54,7 +56,8 @@ gboolean                acrostic_generator_set_text          (AcrosticGenerator
                                                               GError           **error);
 void                    acrostic_generator_set_seed          (AcrosticGenerator *self,
                                                               guint32            seed);
-void                    acrostic_generator_run               (AcrosticGenerator *self);
+void                    acrostic_generator_set_word_resource (AcrosticGenerator *self,
+                                                              WordListResource  *resource);
 
 
 
diff --git a/src/crosswords.gresource.xml.in b/src/crosswords.gresource.xml.in
index 83a15b4c..b759803d 100644
--- a/src/crosswords.gresource.xml.in
+++ b/src/crosswords.gresource.xml.in
@@ -8,6 +8,8 @@
 
     <file>cell-preview.ui</file>
     <file>edit-autofill-details.ui</file>
+    <file>edit-acrostic-answer.ui</file>
+    <file>edit-acrostic-answers.ui</file>
     <file>edit-acrostic-details.ui</file>
     <file>edit-bars.ui</file>
     <file>edit-cell.ui</file>
diff --git a/src/edit-acrostic-answer.c b/src/edit-acrostic-answer.c
new file mode 100644
index 00000000..7edaf8ea
--- /dev/null
+++ b/src/edit-acrostic-answer.c
@@ -0,0 +1,88 @@
+/* edit-acrostic-answer.c
+ *
+ * Copyright 2024 Tanmay Patil <tanmaynpatil105@gmail.com>
+ *
+ * This program is free software: you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation, either version 3 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program.  If not, see <http://www.gnu.org/licenses/>.
+ */
+
+#include "crosswords-config.h"
+#include <glib/gi18n-lib.h>
+#include "edit-acrostic-answer.h"
+
+struct _EditAcrosticAnswer
+{
+  GtkWidget parent_instance;
+
+  GtkWidget *answer;
+  GtkWidget *check_button;
+};
+
+G_DEFINE_TYPE (EditAcrosticAnswer, edit_acrostic_answer, GTK_TYPE_WIDGET);
+
+static void edit_acrostic_answer_init       (EditAcrosticAnswer      *self);
+static void edit_acrostic_answer_class_init (EditAcrosticAnswerClass *klass);
+static void edit_acrostic_answer_dispose    (GObject                  *object);
+
+static void
+edit_acrostic_answer_init (EditAcrosticAnswer *self)
+{
+  gtk_widget_init_template (GTK_WIDGET (self));
+}
+
+static void
+edit_acrostic_answer_class_init (EditAcrosticAnswerClass *klass)
+{
+  GObjectClass *object_class = G_OBJECT_CLASS (klass);
+  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
+
+  object_class->dispose = edit_acrostic_answer_dispose;
+
+  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Crosswords/edit-acrostic-answer.ui");
+
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswer, answer);
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswer, check_button);
+
+  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
+}
+
+static void
+edit_acrostic_answer_dispose (GObject *object)
+{
+  GtkWidget *child;
+
+  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
+    gtk_widget_unparent (child);
+
+  G_OBJECT_CLASS (edit_acrostic_answer_parent_class)->dispose (object);
+}
+
+GtkWidget *
+edit_acrostic_answer_new (const char *str)
+{
+  EditAcrosticAnswer *answer;
+
+  answer = g_object_new (EDIT_TYPE_ACROSTIC_ANSWER, NULL);
+
+  if (str != NULL)
+    edit_acrostic_answer_set_str (answer, str);
+
+  return GTK_WIDGET (answer);
+}
+
+void
+edit_acrostic_answer_set_str (EditAcrosticAnswer *self,
+                              const char         *str)
+{
+  gtk_label_set_text (GTK_LABEL (self->answer), str);
+}
diff --git a/src/edit-acrostic-answer.h b/src/edit-acrostic-answer.h
new file mode 100644
index 00000000..975c4b06
--- /dev/null
+++ b/src/edit-acrostic-answer.h
@@ -0,0 +1,34 @@
+/* edit-acrostic-answer.h
+ *
+ * Copyright 2024 Tanmay Patil <tanmaynpatil105@gmail.com>
+ *
+ * This program is free software: you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation, either version 3 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program.  If not, see <http://www.gnu.org/licenses/>.
+ */
+
+#pragma once
+
+#include <gtk/gtk.h>
+#include <adwaita.h>
+#include <libipuz/libipuz.h>
+
+G_BEGIN_DECLS
+
+#define EDIT_TYPE_ACROSTIC_ANSWER (edit_acrostic_answer_get_type())
+G_DECLARE_FINAL_TYPE (EditAcrosticAnswer, edit_acrostic_answer, EDIT, ACROSTIC_ANSWER, GtkWidget);
+
+GtkWidget *edit_acrostic_answer_new     (const char         *str);
+void       edit_acrostic_answer_set_str (EditAcrosticAnswer *self,
+                                         const char         *str);
+
+G_END_DECLS
diff --git a/src/edit-acrostic-answer.ui b/src/edit-acrostic-answer.ui
new file mode 100644
index 00000000..53b08a17
--- /dev/null
+++ b/src/edit-acrostic-answer.ui
@@ -0,0 +1,23 @@
+<?xml version="1.0" encoding="UTF-8"?>
+<interface>
+  <requires lib="gtk" version="4.0"/>
+  <requires lib="Adw" version="1.0"/>
+  <template class="EditAcrosticAnswer" parent="GtkWidget">
+    <child>
+      <object class="GtkBox">
+        <property name="orientation">horizontal</property>
+        <property name="spacing">6</property>
+        <child>
+          <object class="GtkCheckButton" id="check_button">
+            <property name="valign">center</property>
+          </object>
+        </child>
+        <child>
+          <object class="GtkLabel" id="answer">
+            <property name="use-underline">True</property>
+          </object>
+        </child>
+      </object>
+    </child>
+  </template>
+</interface>
diff --git a/src/edit-acrostic-answers.c b/src/edit-acrostic-answers.c
new file mode 100644
index 00000000..199e5aa3
--- /dev/null
+++ b/src/edit-acrostic-answers.c
@@ -0,0 +1,252 @@
+/* edit-acrostic-answers.c
+ *
+ * Copyright 2024 Jonathan Blandford
+ *
+ * This program is free software: you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation, either version 3 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more answers.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program.  If not, see <http://www.gnu.org/licenses/>.
+ *
+ * SPDX-License-Identifier: GPL-3.0-or-later
+ */
+
+#include "crosswords-config.h"
+#include <glib/gi18n-lib.h>
+#include <libipuz/libipuz.h>
+#include <adwaita.h>
+#include "crosswords-misc.h"
+#include "edit-acrostic-answer.h"
+#include "edit-acrostic-answers.h"
+#include "acrostic-generator.h"
+#include "edit-entry-row.h"
+
+
+enum
+{
+  PREV_ANSWER,
+  NEXT_ANSWER,
+  START_SOLVE,
+  N_SIGNALS
+};
+
+static guint obj_signals[N_SIGNALS] = { 0 };
+
+
+struct _EditAcrosticAnswers
+{
+  GtkWidget parent_instance;
+
+  gint curr_answer;
+  GtkStringList *answers_model;
+
+  GtkWidget *answers_list_sw;
+  GtkWidget *answers_word_list_view;
+  GtkWidget *words_tried_text;
+  GtkWidget *solve_button;
+};
+
+
+static void     edit_acrostic_answers_init           (EditAcrosticAnswers      *self);
+static void     edit_acrostic_answers_class_init     (EditAcrosticAnswersClass *klass);
+static void     edit_acrostic_answers_dispose        (GObject                  *object);
+static void     start_solve_cb                       (EditAcrosticAnswers      *self,
+                                                      GtkButton                *solve_button);
+static void     setup_listitem_cb                    (GtkListItemFactory       *factory,
+                                                      GtkListItem              *list_item);
+static void     bind_listitem_cb                     (GtkListItemFactory       *factory,
+                                                      GtkListItem              *list_item);
+
+
+G_DEFINE_TYPE (EditAcrosticAnswers, edit_acrostic_answers, GTK_TYPE_WIDGET);
+
+
+static void
+edit_acrostic_answers_init (EditAcrosticAnswers *self)
+{
+  GtkSelectionModel *selection_model;
+  g_autoptr (GtkListItemFactory) factory = NULL;
+
+  gtk_widget_init_template (GTK_WIDGET (self));
+
+  self->curr_answer = -1;
+
+  self->answers_model = gtk_string_list_new (NULL);
+
+  selection_model =
+    GTK_SELECTION_MODEL (gtk_no_selection_new (G_LIST_MODEL (self->answers_model)));
+
+  factory = gtk_signal_list_item_factory_new ();
+
+  g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem_cb), NULL);
+  g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem_cb), NULL);
+
+  g_object_set (self->answers_word_list_view,
+                "model", selection_model,
+                "factory", factory,
+                NULL);
+}
+
+static void
+edit_acrostic_answers_class_init (EditAcrosticAnswersClass *klass)
+{
+  GObjectClass *object_class = G_OBJECT_CLASS (klass);
+  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
+
+  object_class->dispose = edit_acrostic_answers_dispose;
+
+  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Crosswords/edit-acrostic-answers.ui");
+
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswers, answers_list_sw);
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswers, answers_word_list_view);
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswers, words_tried_text);
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticAnswers, solve_button);
+
+  gtk_widget_class_bind_template_callback (widget_class, start_solve_cb);
+
+  obj_signals [START_SOLVE] =
+    g_signal_new ("start-solve",
+                  EDIT_TYPE_ACROSTIC_ANSWERS,
+                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
+                  0,
+                  NULL, NULL,
+                  NULL,
+                  G_TYPE_NONE, 0);
+
+  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
+}
+
+static void
+edit_acrostic_answers_dispose (GObject *object)
+{
+  GtkWidget *child;
+
+  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
+    gtk_widget_unparent (child);
+
+  G_OBJECT_CLASS (edit_acrostic_answers_parent_class)->dispose (object);
+}
+
+static void
+setup_listitem_cb (GtkListItemFactory       *factory,
+                   GtkListItem              *list_item)
+{
+  GtkWidget *row;
+
+  row = edit_acrostic_answer_new (NULL);
+
+  gtk_list_item_set_child (list_item, row);
+}
+
+static void
+bind_listitem_cb (GtkListItemFactory       *factory,
+                  GtkListItem              *list_item)
+{
+  GtkWidget *acrostic_answer;
+  GtkStringObject *object;
+  const char *answer;
+
+  acrostic_answer= gtk_list_item_get_child (list_item);
+
+  object = gtk_list_item_get_item (list_item);
+  answer = gtk_string_object_get_string (object);
+
+  edit_acrostic_answer_set_str (EDIT_ACROSTIC_ANSWER (acrostic_answer),
+                                answer);
+}
+
+static void
+update_warnings (EditAcrosticAnswers *self)
+{
+  g_autoptr (GError) error = NULL;
+  g_autoptr (AcrosticGenerator) acrostic_generator = NULL;
+  const gchar *quote_str, *source_str;
+
+  acrostic_generator = acrostic_generator_new ();
+
+  if (acrostic_generator_set_text (acrostic_generator, quote_str, source_str, &error))
+    {
+      gtk_widget_set_sensitive	(self->solve_button, TRUE);
+    }
+  else
+    {
+      gtk_widget_set_sensitive	(self->solve_button, FALSE);
+    }
+}
+
+static void
+start_solve_cb (EditAcrosticAnswers *self,
+                GtkButton           *solve_button)
+{
+  g_signal_emit (self, obj_signals[START_SOLVE], 0);
+
+  gtk_button_set_label (solve_button, "Finding...");
+  gtk_widget_set_sensitive (GTK_WIDGET (solve_button), FALSE);
+}
+
+void
+edit_acrostic_answers_commit_changes (EditAcrosticAnswers *self)
+{
+}
+
+static void
+update_count (EditAcrosticAnswers *self,
+              guint                count)
+{
+  const gchar *words_tried;
+
+  words_tried = g_strdup_printf (_("%d words tried"), count);
+
+  gtk_widget_set_visible (self->words_tried_text, TRUE);
+  gtk_label_set_text (GTK_LABEL (self->words_tried_text), words_tried);
+
+  gtk_button_set_label (GTK_BUTTON (self->solve_button), "Generate");
+  gtk_widget_set_sensitive (self->solve_button, TRUE);
+}
+
+static void
+update_answers (EditAcrosticAnswers *self,
+                GridState           *state)
+{
+  guint n_answers;
+
+  /* clear the old list model */
+  n_answers = g_list_model_get_n_items (G_LIST_MODEL (self->answers_model));
+  gtk_string_list_splice (self->answers_model, 0, n_answers, NULL);
+
+  /* quick and dirty implementation to display something */
+  n_answers = ipuz_clues_get_n_clues (IPUZ_CLUES (state->xword),
+                                      IPUZ_CLUE_DIRECTION_CLUES);
+  for (guint i = 0; i < n_answers; i++)
+    {
+      g_autofree gchar *answer = NULL;
+      IpuzClueId clue_id = {
+        .index = i,
+        .direction = IPUZ_CLUE_DIRECTION_CLUES,
+      };
+
+      answer = ipuz_clues_get_clue_string_by_id (IPUZ_CLUES (state->xword), &clue_id);
+      gtk_string_list_append (self->answers_model, answer);
+    }
+}
+
+
+void
+edit_acrostic_answers_update (EditAcrosticAnswers *self,
+                               GridState           *state,
+                               TaskRunner          *runner,
+                               WordListResource    *resource)
+{
+  g_return_if_fail (EDIT_IS_ACROSTIC_ANSWERS (self));
+
+  update_answers (self, state);
+  
+}
+
diff --git a/src/edit-acrostic-answers.h b/src/edit-acrostic-answers.h
new file mode 100644
index 00000000..866c7964
--- /dev/null
+++ b/src/edit-acrostic-answers.h
@@ -0,0 +1,41 @@
+/* edit-acrostic-autofill.h
+ *
+ * Copyright 2025 Jonathan Blandford
+ *
+ * This program is free software: you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation, either version 3 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program.  If not, see <http://www.gnu.org/licenses/>.
+ */
+
+#pragma once
+
+#include <gtk/gtk.h>
+#include <libipuz/libipuz.h>
+#include "grid-state.h"
+#include "task-runner.h"
+#include "word-list-resource.h"
+
+
+G_BEGIN_DECLS
+
+
+#define EDIT_TYPE_ACROSTIC_ANSWERS (edit_acrostic_answers_get_type())
+G_DECLARE_FINAL_TYPE (EditAcrosticAnswers, edit_acrostic_answers, EDIT, ACROSTIC_ANSWERS, GtkWidget);
+
+
+void edit_acrostic_answers_update           (EditAcrosticAnswers *self,
+                                             GridState           *grid_state,
+                                             TaskRunner          *runner,
+                                             WordListResource    *resource);
+
+
+G_END_DECLS
diff --git a/src/edit-acrostic-answers.ui b/src/edit-acrostic-answers.ui
new file mode 100644
index 00000000..e9b1b441
--- /dev/null
+++ b/src/edit-acrostic-answers.ui
@@ -0,0 +1,74 @@
+<?xml version="1.0" encoding="UTF-8"?>
+<interface>
+  <requires lib="gtk" version="4.0"/>
+  <template class="EditAcrosticAnswers" parent="GtkWidget">
+    <property name="hexpand">true</property>
+    <property name="vexpand">true</property>
+    <child>
+      <object class="GtkBox" id="answers_list_sw">
+        <property name="orientation">vertical</property>
+        <property name="spacing">8</property>
+        <property name="vexpand">true</property>
+        <property name="hexpand">true</property>
+        <property name="valign">fill</property>
+        <property name="margin-top">20</property>
+        <property name="homogeneous">false</property>
+        <child>
+          <object class="AdwPreferencesGroup">
+            <property name="title" translatable="yes">Answers</property>
+            <property name="vexpand">true</property>
+            <child>
+              <object class="GtkScrolledWindow">
+                <property name="has-frame">false</property>
+                <property name="hexpand">true</property>
+                <property name="vexpand">true</property>
+                <property name="hscrollbar-policy">never</property>
+                <style>
+                  <class name="adw-list-box" />
+                </style>
+                <child>
+                  <object class="GtkListView" id="answers_word_list_view">
+                    <property name="visible">true</property>
+                  </object>
+                </child>
+              </object>
+            </child>
+          </object>
+        </child>
+        <child>
+          <object class="GtkBox">
+            <property name="orientation">horizontal</property>
+            <property name="spacing">8</property>
+            <property name="hexpand">false</property>
+            <property name="valign">fill</property>
+            <property name="margin-top">20</property>
+            <property name="homogeneous">true</property>
+            <property name="width-request">325</property>
+            <child>
+              <object class="GtkLabel" id="words_tried_text">
+                <property name="visible">false</property>
+                <property name="hexpand">true</property>
+                <property name="halign">start</property>
+                <property name="width-request">150</property>
+                <property name="tooltip-text">words tried</property>
+              </object>
+            </child>
+            <child>
+              <object class="GtkButton" id="solve_button">
+                <property name="label" translatable="yes">Generate</property>
+                <property name="tooltip-text">Generate answers</property>
+                <property name="can-shrink">true</property>
+                <property name="halign">end</property>
+                <property name="width-request">175</property>
+                <signal name="clicked" handler="start_solve_cb" swapped="true" />
+                <style>
+                  <class name="suggested-action"/>
+                </style>
+              </object>
+            </child>
+          </object>
+        </child>
+      </object>
+    </child>
+  </template>
+</interface>
diff --git a/src/edit-acrostic-details.c b/src/edit-acrostic-details.c
index c1589ef6..3fb86e58 100644
--- a/src/edit-acrostic-details.c
+++ b/src/edit-acrostic-details.c
@@ -23,18 +23,38 @@
 #include <libipuz/libipuz.h>
 #include <adwaita.h>
 #include "crosswords-misc.h"
+#include "edit-acrostic-answer.h"
 #include "edit-acrostic-details.h"
+#include "acrostic-generator.h"
+#include "edit-entry-row.h"
+
+
+enum
+{
+  NORMALIZED_QUOTE_STR_CHANGED,
+  NORMALIZED_SOURCE_STR_CHANGED,
+  N_SIGNALS
+};
+
+static guint obj_signals[N_SIGNALS] = { 0 };
 
 
 struct _EditAcrosticDetails
 {
   GtkWidget parent_instance;
+
+  GtkWidget *quote_row;
+  GtkWidget *source_row;
 };
 
 
 static void     edit_acrostic_details_init           (EditAcrosticDetails      *self);
 static void     edit_acrostic_details_class_init     (EditAcrosticDetailsClass *klass);
 static void     edit_acrostic_details_dispose        (GObject                  *object);
+static void     quote_row_applied_cb                 (EditAcrosticDetails      *self,
+                                                      EditEntryRow             *entry_row);
+static void     source_row_applied_cb                (EditAcrosticDetails      *self,
+                                                      EditEntryRow             *entry_row);
 
 
 G_DEFINE_TYPE (EditAcrosticDetails, edit_acrostic_details, GTK_TYPE_WIDGET);
@@ -56,6 +76,32 @@ edit_acrostic_details_class_init (EditAcrosticDetailsClass *klass)
 
   gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Crosswords/edit-acrostic-details.ui");
 
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticDetails, quote_row);
+  gtk_widget_class_bind_template_child (widget_class, EditAcrosticDetails, source_row);
+
+  gtk_widget_class_bind_template_callback (widget_class, quote_row_applied_cb);
+  gtk_widget_class_bind_template_callback (widget_class, source_row_applied_cb);
+
+  obj_signals [NORMALIZED_QUOTE_STR_CHANGED] =
+    g_signal_new ("normalized-quote-str-changed",
+                  EDIT_TYPE_ACROSTIC_DETAILS,
+                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
+                  0,
+                  NULL, NULL,
+                  NULL,
+                  G_TYPE_NONE, 1,
+                  G_TYPE_STRING);
+
+  obj_signals [NORMALIZED_SOURCE_STR_CHANGED] =
+    g_signal_new ("normalized-source-str-changed",
+                  EDIT_TYPE_ACROSTIC_DETAILS,
+                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
+                  0,
+                  NULL, NULL,
+                  NULL,
+                  G_TYPE_NONE, 1,
+                  G_TYPE_STRING);
+
   gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
 }
 
@@ -70,14 +116,74 @@ edit_acrostic_details_dispose (GObject *object)
   G_OBJECT_CLASS (edit_acrostic_details_parent_class)->dispose (object);
 }
 
-void
-edit_acrostic_details_update (EditAcrosticDetails *self,
-                              GridState           *grid_state)
+static void
+update_warnings (EditAcrosticDetails *self)
+{
+  g_autoptr (GError) error = NULL;
+  g_autoptr (AcrosticGenerator) acrostic_generator = NULL;
+  const gchar *quote_str, *source_str;
+
+  quote_str = gtk_editable_get_text (GTK_EDITABLE (self->quote_row));
+  source_str = gtk_editable_get_text (GTK_EDITABLE (self->source_row));
+
+  acrostic_generator =  acrostic_generator_new ();
+
+  if (acrostic_generator_set_text (acrostic_generator, quote_str, source_str, &error))
+    {
+      edit_entry_row_clear_error (EDIT_ENTRY_ROW (self->source_row));
+    }
+  else
+    {
+      edit_entry_row_set_error (EDIT_ENTRY_ROW (self->source_row), error->message);
+    }
+}
+
+static void
+quote_row_applied_cb (EditAcrosticDetails *self,
+                      EditEntryRow        *entry_row)
+{
+  const gchar *quote_str;
+
+  quote_str = gtk_editable_get_text (GTK_EDITABLE (self->quote_row));
+
+  update_warnings (self);
+  g_signal_emit (self, obj_signals[NORMALIZED_QUOTE_STR_CHANGED], 0, quote_str);
+}
+
+static void
+source_row_applied_cb (EditAcrosticDetails *self,
+                       EditEntryRow        *entry_row)
 {
+  const gchar *source_str;
+
+  source_str = gtk_editable_get_text (GTK_EDITABLE (self->source_row));
+
+  update_warnings (self);
+  g_signal_emit (self, obj_signals[NORMALIZED_SOURCE_STR_CHANGED], 0, source_str);
 }
 
 void
 edit_acrostic_details_commit_changes (EditAcrosticDetails *self)
 {
+  edit_entry_row_commit (EDIT_ENTRY_ROW (self->quote_row));
+  edit_entry_row_commit (EDIT_ENTRY_ROW (self->source_row));
+}
+
+void
+edit_acrostic_details_update (EditAcrosticDetails *self,
+                              GridState           *grid_state)
+{
+  const gchar *quote_str, *source_str;
+
+  quote_str = ipuz_acrostic_get_quote (IPUZ_ACROSTIC (grid_state->xword));
+  source_str = ipuz_acrostic_get_source (IPUZ_ACROSTIC (grid_state->xword));
+
+  /* Update quote row */
+  edit_entry_row_set_text (EDIT_ENTRY_ROW (self->quote_row), quote_str);
+  edit_entry_row_commit (EDIT_ENTRY_ROW (self->quote_row));
+
+  /* Update source row */
+  edit_entry_row_set_text (EDIT_ENTRY_ROW (self->source_row), source_str);
+  edit_entry_row_commit (EDIT_ENTRY_ROW (self->source_row));
 }
 
diff --git a/src/edit-acrostic-details.h b/src/edit-acrostic-details.h
index 5510c108..25e2e96b 100644
--- a/src/edit-acrostic-details.h
+++ b/src/edit-acrostic-details.h
@@ -20,6 +20,8 @@
 
 #include <gtk/gtk.h>
 #include <libipuz/libipuz.h>
+#include "task-runner.h"
+#include "word-list-resource.h"
 
 
 G_BEGIN_DECLS
@@ -29,9 +31,9 @@ G_BEGIN_DECLS
 G_DECLARE_FINAL_TYPE (EditAcrosticDetails, edit_acrostic_details, EDIT, ACROSTIC_DETAILS, GtkWidget);
 
 
-void edit_acrostic_details_update           (EditAcrosticDetails *self,
-                                             GridState           *grid_state);
-void edit_acrostic_details_commit_changes   (EditAcrosticDetails *self);
+void edit_acrostic_details_update         (EditAcrosticDetails *self,
+                                           GridState           *grid_state);
+void edit_acrostic_details_commit_changes (EditAcrosticDetails *self);
 
 
 G_END_DECLS
diff --git a/src/edit-acrostic-details.ui b/src/edit-acrostic-details.ui
index 1dfc0569..334cbbdf 100644
--- a/src/edit-acrostic-details.ui
+++ b/src/edit-acrostic-details.ui
@@ -2,19 +2,27 @@
 <interface>
   <requires lib="gtk" version="4.0"/>
   <template class="EditAcrosticDetails" parent="GtkWidget">
-    <property name="hexpand">true</property>
     <property name="hexpand">true</property>
     <child>
-      <object class="GtkBox">
-        <property name="orientation">vertical</property>
-        <property name="spacing">2</property>
+      <object class="AdwPreferencesGroup">
+        <property name="title" translatable="yes">Acrostic Details</property>
+        <property name="hexpand">true</property>
+        <child>
+          <object class="EditEntryRow" id="quote_row">
+            <property name="title" translatable="yes">_Quote</property>
+            <property name="use-underline">true</property>
+            <property name="activates-default">true</property>
+            <property name="show-apply-button">true</property>
+            <signal name="apply" handler="quote_row_applied_cb" swapped="true" />
+          </object>
+        </child>
         <child>
-          <object class="GtkLabel">
-            <property name="hexpand">true</property>
-            <property name="vexpand">true</property>
-            <property name="halign">center</property>
-            <property name="valign">center</property>
-            <property name="label">[Acrostic Details]</property>
+          <object class="EditEntryRow" id="source_row">
+            <property name="title" translatable="yes">_Author / _Work</property>
+            <property name="use-underline">true</property>
+            <property name="activates-default">true</property>
+            <property name="show-apply-button">true</property>
+            <signal name="apply" handler="source_row_applied_cb" swapped="true" />
           </object>
         </child>
       </object>
diff --git a/src/edit-app.c b/src/edit-app.c
index d5348ee8..89957c8c 100644
--- a/src/edit-app.c
+++ b/src/edit-app.c
@@ -27,6 +27,8 @@
 #include "charset-entry-buffer.h"
 #include "crosswords-credits.h"
 #include "crosswords-init.h"
+#include "edit-acrostic-answer.h"
+#include "edit-acrostic-answers.h"
 #include "edit-acrostic-details.h"
 #include "edit-app.h"
 #include "edit-bars.h"
@@ -169,8 +171,10 @@ edit_app_startup (GApplication *app)
   /* FIXME(refactor): clean up the crosswords-init / edit-init mess */
   g_type_ensure (CELL_TYPE_PREVIEW);
   g_type_ensure (CHARSET_TYPE_ENTRY_BUFFER);
-  g_type_ensure (EDIT_TYPE_AUTOFILL_DETAILS);
+  g_type_ensure (EDIT_TYPE_ACROSTIC_ANSWER);
+  g_type_ensure (EDIT_TYPE_ACROSTIC_ANSWERS);
   g_type_ensure (EDIT_TYPE_ACROSTIC_DETAILS);
+  g_type_ensure (EDIT_TYPE_AUTOFILL_DETAILS);
   g_type_ensure (EDIT_TYPE_BARS);
   g_type_ensure (EDIT_TYPE_CELL);
   g_type_ensure (EDIT_TYPE_GREETER);
diff --git a/src/edit-clue-list.c b/src/edit-clue-list.c
index 47587d50..03efab70 100644
--- a/src/edit-clue-list.c
+++ b/src/edit-clue-list.c
@@ -88,7 +88,7 @@ edit_clue_list_class_init (EditClueListClass *klass)
                   NULL,
                   G_TYPE_NONE, 1,
                   IPUZ_TYPE_CLUE_ID);
-  
+
   gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
 }
 
@@ -123,6 +123,46 @@ void
 edit_clue_list_update (EditClueList *self,
                        GridState    *clues_state)
 {
-  play_clue_list_update (PLAY_CLUE_LIST (self->primary_clues), clues_state, ZOOM_NORMAL);
-  play_clue_list_update (PLAY_CLUE_LIST (self->secondary_clues), clues_state, ZOOM_NORMAL);
+  guint n_clues;
+  gboolean primary_visible = FALSE;
+  gboolean secondary_visible = FALSE;
+  IpuzClueDirection direction;
+
+  n_clues = ipuz_clues_get_n_clue_sets (IPUZ_CLUES (clues_state->xword));
+
+  if (n_clues == 0)
+    {
+      g_warning ("Puzzle without clues");
+    }
+
+  if (n_clues >= 1)
+    {
+      primary_visible = TRUE;
+      direction = ipuz_clues_clue_set_get_dir (IPUZ_CLUES (clues_state->xword),
+                                               0);
+      g_object_set (self->primary_clues,
+                    "direction", direction,
+                    NULL);
+      play_clue_list_update (PLAY_CLUE_LIST (self->primary_clues), clues_state, ZOOM_NORMAL);
+    }
+
+  if (n_clues >= 2)
+    {
+      secondary_visible = TRUE;
+      direction = ipuz_clues_clue_set_get_dir (IPUZ_CLUES (clues_state->xword),
+                                               1);
+      g_object_set (self->secondary_clues,
+                    "direction", direction,
+                    NULL);
+      play_clue_list_update (PLAY_CLUE_LIST (self->secondary_clues), clues_state, ZOOM_NORMAL);
+    }
+
+  if (n_clues >= 3)
+    {
+      g_warning ("Puzzle has more than two clue sets. This isn't supported yet");
+    }
+
+  /* Update visibility */
+  gtk_widget_set_visible (self->primary_clues, primary_visible);
+  gtk_widget_set_visible (self->secondary_clues, secondary_visible);
 }
diff --git a/src/edit-clue-list.ui b/src/edit-clue-list.ui
index 657abc03..98277c2d 100644
--- a/src/edit-clue-list.ui
+++ b/src/edit-clue-list.ui
@@ -17,7 +17,6 @@
             <property name="hexpand">true</property>
             <property name="mode">edit</property>
             <property name="direction">across</property>
-            <property name="width-request">250</property>
             <signal name="clue-selected" handler="clue_selected_cb" swapped="true"/>
           </object>
         </child>
@@ -27,7 +26,6 @@
             <property name="hexpand">true</property>
             <property name="mode">edit</property>
             <property name="direction">down</property>
-            <property name="width-request">250</property>
             <signal name="clue-selected" handler="clue_selected_cb" swapped="true"/>
           </object>
         </child>
diff --git a/src/edit-entry-row.c b/src/edit-entry-row.c
index 8c5dfd9c..a4d60505 100644
--- a/src/edit-entry-row.c
+++ b/src/edit-entry-row.c
@@ -294,6 +294,12 @@ enumeration_insert_text_cb (GtkEditable *editable,
   g_signal_stop_emission_by_name (editable, "insert_text");
 }
 
+void
+edit_entry_row_clear_error (EditEntryRow *self)
+{
+  edit_entry_row_changed_cb (self);
+}
+
 void
 edit_entry_row_set_validation_mode (EditEntryRow               *self,
                                     EditEntryRowValidationMode  mode)
diff --git a/src/edit-entry-row.h b/src/edit-entry-row.h
index 2f7f798a..ffaec9e6 100644
--- a/src/edit-entry-row.h
+++ b/src/edit-entry-row.h
@@ -43,6 +43,7 @@ void edit_entry_row_set_warning         (EditEntryRow               *self,
 void edit_entry_row_set_error           (EditEntryRow               *self,
                                          const gchar                *message);
 void edit_entry_row_commit              (EditEntryRow               *self);
+void edit_entry_row_clear_error         (EditEntryRow               *self);
 void edit_entry_row_set_validation_mode (EditEntryRow               *self,
                                          EditEntryRowValidationMode  mode);
 
diff --git a/src/edit-greeter-acrostic.c b/src/edit-greeter-acrostic.c
index cd34a732..17436d47 100644
--- a/src/edit-greeter-acrostic.c
+++ b/src/edit-greeter-acrostic.c
@@ -334,5 +334,33 @@ static void
 source_row_applied_cb (EditGreeterAcrostic *self,
                        EditEntryRow        *entry_row)
 {
+  const gchar *source_str;
+
+  source_str = gtk_editable_get_text (GTK_EDITABLE (self->source_row));
+  ipuz_acrostic_set_source (IPUZ_ACROSTIC (self->puzzle), source_str);
+  ipuz_acrostic_fix_source (IPUZ_ACROSTIC (self->puzzle),
+                            IPUZ_ACROSTIC_SYNC_STRING_TO_PUZZLE);
+  ipuz_acrostic_initialize_answers (IPUZ_ACROSTIC (self->puzzle));
+
   update_warnings (self);
 }
+
+const gchar *
+edit_greeter_acrostic_get_quote_str (EditGreeterAcrostic *self)
+{
+  const gchar *quote_str;
+
+  quote_str = gtk_editable_get_text (GTK_EDITABLE (self->quote_row));
+
+  return quote_str;
+}
+
+const gchar *
+edit_greeter_acrostic_get_source_str (EditGreeterAcrostic *self)
+{
+  const gchar *source_str;
+
+  source_str = gtk_editable_get_text (GTK_EDITABLE (self->source_row));
+
+  return source_str;
+}
diff --git a/src/edit-greeter-acrostic.h b/src/edit-greeter-acrostic.h
index 92be65a6..768fae93 100644
--- a/src/edit-greeter-acrostic.h
+++ b/src/edit-greeter-acrostic.h
@@ -28,4 +28,7 @@ G_BEGIN_DECLS
 G_DECLARE_FINAL_TYPE (EditGreeterAcrostic, edit_greeter_acrostic, EDIT, GREETER_ACROSTIC, GtkWidget);
 
 
+const gchar* edit_greeter_acrostic_get_quote_str  (EditGreeterAcrostic *self);
+const gchar* edit_greeter_acrostic_get_source_str (EditGreeterAcrostic *self);
+
 G_END_DECLS
diff --git a/src/edit-greeter.c b/src/edit-greeter.c
index e591ce06..9111b3ff 100644
--- a/src/edit-greeter.c
+++ b/src/edit-greeter.c
@@ -23,6 +23,7 @@
 #include <libipuz/libipuz.h>
 #include "crosswords-misc.h"
 #include "edit-greeter.h"
+#include "edit-greeter-acrostic.h"
 #include "edit-greeter-details.h"
 #include "edit-window.h"
 #include "play-grid.h"
diff --git a/src/edit-histogram.c b/src/edit-histogram.c
index d59108b4..14f0aae4 100644
--- a/src/edit-histogram.c
+++ b/src/edit-histogram.c
@@ -385,3 +385,117 @@ edit_histogram_update (EditHistogram  *self,
   update_graph (self, info);
   update_summary (self, info);
 }
+
+static void
+update_acrostic_answers_graph (EditHistogram *self,
+                               GArray        *answers,
+                               guint         *total_len)
+{
+  g_autoptr (GHashTable) table = NULL;
+  GHashTableIter iter;
+  g_autoptr (GString) str = NULL;
+  guint r = 0, c = 0;
+  gpointer key, value;
+
+
+  table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
+                                 NULL, NULL);
+  str = g_string_new (NULL);
+
+  for (guint i = 0; i < answers->len; i++)
+    {
+      gchar *answer;
+      guint len;
+
+      answer = g_array_index (answers, gchar *, i);
+
+      len = g_utf8_strlen (answer, -1);
+      *total_len += len;
+
+      if (g_hash_table_contains (table, GINT_TO_POINTER (len)))
+        {
+          guint count;
+
+          count = GPOINTER_TO_UINT (g_hash_table_lookup (table, GINT_TO_POINTER (len)));
+
+          g_hash_table_replace (table, GINT_TO_POINTER (len), GINT_TO_POINTER (count + 1));
+        }
+      else
+        {
+          g_hash_table_insert (table, GINT_TO_POINTER (len), GINT_TO_POINTER (1));
+        }
+    }
+
+  resize_grid (self, g_hash_table_size (table));
+
+  g_hash_table_iter_init (&iter, table);
+
+  while (g_hash_table_iter_next (&iter, &key, &value))
+    {
+      guint answer_len, len_count;
+      GtkWidget *index_label;
+      GtkWidget *bar_label;
+
+      answer_len = GPOINTER_TO_UINT (key);
+      len_count = GPOINTER_TO_UINT (value);
+
+      index_label = gtk_grid_get_child_at (GTK_GRID (self->grid), c, r);
+      bar_label = gtk_grid_get_child_at (GTK_GRID (self->grid), c + 1, r);
+      gtk_widget_set_visible (index_label, TRUE);
+      gtk_widget_set_visible (bar_label, TRUE);
+
+      g_string_set_size (str, 0);
+      g_string_append_printf (str, "Answer length: %u", answer_len);
+      gtk_label_set_markup (GTK_LABEL (index_label),  str->str);
+
+      g_string_set_size (str, 0);
+      for (guint i = 0; i < len_count; i++)
+        {
+          g_string_append_printf (str, "█");
+        }
+
+      g_string_append_printf (str, " (%u)", len_count);
+      gtk_label_set_markup (GTK_LABEL (bar_label), str->str);
+
+      r++;
+      if (r >= self->max_rows)
+        {
+          r = 0;
+          c += 2;
+        }
+    }
+}
+
+void
+update_acrostic_answers_summary (EditHistogram *self,
+                                 guint          total_len,
+                                 guint          n_answers)
+{
+  g_autoptr (GString) summary_text = NULL;
+
+  summary_text = g_string_new (NULL);
+
+  if (n_answers > 0)
+    {
+      g_string_append_printf (summary_text,
+                              "Average answer length: %.2f",
+                              (float)total_len / n_answers);
+
+      gtk_label_set_markup (GTK_LABEL (self->summary_label), summary_text->str);
+      gtk_widget_set_visible (self->summary_label, TRUE);
+    }
+  else
+    {
+      gtk_widget_set_visible (self->summary_label, FALSE);
+    }
+}
+
+void
+edit_histogram_update_acrostic_answers (EditHistogram *self,
+                                        GArray        *answers)
+{
+  guint total_len = 0;
+
+  update_acrostic_answers_graph (self, answers, &total_len);
+  update_acrostic_answers_summary (self, total_len, answers->len);
+}
diff --git a/src/edit-histogram.h b/src/edit-histogram.h
index 704dd75f..d4a17c4a 100644
--- a/src/edit-histogram.h
+++ b/src/edit-histogram.h
@@ -29,6 +29,7 @@ typedef enum
 {
   EDIT_HISTOGRAM_MODE_LETTERS,
   EDIT_HISTOGRAM_MODE_CLUELEN,
+  EDIT_HISTOGRAM_MODE_ANSWERLEN,
 } EditHistogramMode;
 
 
@@ -36,8 +37,10 @@ typedef enum
 G_DECLARE_FINAL_TYPE (EditHistogram, edit_histogram, EDIT, HISTOGRAM, GtkWidget);
 
 
-void edit_histogram_update (EditHistogram  *self,
-                            IpuzPuzzleInfo *info);
+void edit_histogram_update                  (EditHistogram  *self,
+                                             IpuzPuzzleInfo *info);
+void edit_histogram_update_acrostic_answers (EditHistogram  *self,
+                                             GArray         *answers);
 
 
 G_END_DECLS
diff --git a/src/edit-state.c b/src/edit-state.c
index 27abc36e..96b31048 100644
--- a/src/edit-state.c
+++ b/src/edit-state.c
@@ -52,6 +52,10 @@ edit_state_new (IpuzPuzzle *puzzle)
 
   edit_state = g_new0 (EditState, 1);
   edit_state->stage = EDIT_STAGE_GRID;
+  if (IPUZ_IS_ACROSTIC (puzzle))
+    edit_state->grid_stage = EDIT_GRID_STAGE_ACROSTIC;
+  else
+    edit_state->grid_stage = EDIT_GRID_STAGE_CELL;
   edit_state->runner = task_runner_new ();
   edit_state->quirks = crosswords_quirks_new (puzzle);
   crosswords_quirks_set_symmetry (edit_state->quirks, symmetry);
@@ -61,22 +65,46 @@ edit_state_new (IpuzPuzzle *puzzle)
   if (puzzle)
     {
       edit_state->puzzle_kind = ipuz_puzzle_get_puzzle_kind (puzzle);
-      edit_state->grid_state =
-        grid_state_new (IPUZ_CROSSWORD (puzzle),
-                        edit_state->quirks,
-                        GRID_STATE_EDIT);
-      edit_state->clues_state =
-        grid_state_new (IPUZ_CROSSWORD (puzzle),
-                        edit_state->quirks,
-                        GRID_STATE_EDIT_BROWSE);
-      edit_state->style_state =
-        grid_state_new (IPUZ_CROSSWORD (puzzle),
-                        edit_state->quirks,
-                        GRID_STATE_SELECT);
-      edit_state->preview_state =
-        grid_state_new (IPUZ_CROSSWORD (puzzle),
-                        edit_state->quirks,
-                        GRID_STATE_VIEW);
+      if (IPUZ_IS_ACROSTIC (puzzle))
+        {
+          crosswords_quirks_set_focus_location (edit_state->quirks,
+                                                QUIRKS_FOCUS_LOCATION_CLUE_GRID);
+          edit_state->grid_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_VIEW);
+          edit_state->clues_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_EDIT_BROWSE);
+          edit_state->style_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_SELECT);
+          edit_state->preview_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_VIEW);
+        }
+      else
+        {
+          edit_state->grid_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_EDIT);
+          edit_state->clues_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_EDIT_BROWSE);
+          edit_state->style_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_SELECT);
+          edit_state->preview_state =
+            grid_state_new (IPUZ_CROSSWORD (puzzle),
+                            edit_state->quirks,
+                            GRID_STATE_VIEW);
+        }
 
 
       edit_state->preferred_direction = edit_state->grid_state->clue.direction;
diff --git a/src/edit-state.h b/src/edit-state.h
index 50c3e9af..d47803be 100644
--- a/src/edit-state.h
+++ b/src/edit-state.h
@@ -77,8 +77,10 @@ typedef struct
   /* Active widgets */
   guint grid_switcher_bar_enabled : 1;
   guint symmetry_widget_enabled : 1;
+  guint grid_info_widget_enabled: 1;
   guint letter_histogram_widget_enabled : 1;
   guint cluelen_histogram_widget_enabled : 1;
+  guint answerlen_histogram_widget_enabled : 1;
 } EditState;
 
 
diff --git a/src/edit-window-controls.c b/src/edit-window-controls.c
index 98ceda1e..ca478e2d 100644
--- a/src/edit-window-controls.c
+++ b/src/edit-window-controls.c
@@ -43,8 +43,11 @@
 
 
 #include "crosswords-config.h"
+#include "acrostic-generator.h"
 #include "cell-preview.h"
 #include "crosswords-misc.h"
+#include "edit-acrostic-answers.h"
+#include "edit-acrostic-details.h"
 #include "edit-autofill-details.h"
 #include "edit-bars.h"
 #include "edit-cell.h"
@@ -165,6 +168,10 @@ static void     edit_metadata_metadata_changed_cb         (EditWindow        *se
                                                            gchar             *value);
 static void     edit_metadata_showenumerations_changed_cb (EditWindow        *self,
                                                            gboolean           showenumerations);
+static void     edit_acrostic_details_quote_str_cb        (EditWindow        *self,
+                                                           gchar             *quote_str);
+static void     edit_acrostic_details_source_str_cb       (EditWindow        *self,
+                                                           gchar             *source_str);
 
 
 static void
@@ -238,6 +245,55 @@ update_color_rows (EditWindow *self)
                   NULL);
 }
 
+
+static void
+update_all_grid_acrostic (EditWindow *self)
+{
+  edit_grid_update (EDIT_GRID (self->main_grid), GRID_STATE (self));
+  edit_acrostic_details_update (EDIT_ACROSTIC_DETAILS (self->acrostic_details),
+                                GRID_STATE (self));
+  edit_acrostic_answers_update (EDIT_ACROSTIC_ANSWERS (self->acrostic_answers),
+                                GRID_STATE (self),
+                                self->edit_state->runner,
+                                self->resource);
+  edit_histogram_update (EDIT_HISTOGRAM (self->letter_histogram_widget), self->edit_state->info);
+  edit_histogram_update (EDIT_HISTOGRAM (self->answerlen_histogram_widget), self->edit_state->info);
+}
+
+static void
+update_all_grid_crossword (EditWindow *self)
+{
+  update_compact_autofill_visibility (self);
+  edit_grid_update (EDIT_GRID (self->main_grid), GRID_STATE (self));
+  cell_preview_update_from_cursor (CELL_PREVIEW (self->grid_preview),
+                                   GRID_STATE (self),
+                                   self->edit_state->sidebar_logo_config);
+  edit_cell_update (EDIT_CELL (self->cell_widget), GRID_STATE (self), self->edit_state->sidebar_logo_config);
+  edit_grid_info_update (EDIT_GRID_INFO (self->edit_grid_widget), GRID_STATE (self), self->edit_state->info);
+  edit_histogram_update (EDIT_HISTOGRAM (self->letter_histogram_widget), self->edit_state->info);
+  edit_histogram_update (EDIT_HISTOGRAM (self->cluelen_histogram_widget), self->edit_state->info);
+
+  if (GRID_STATE_HAS_SELECTABLE (GRID_STATE (self)))
+    {
+      gtk_stack_set_visible_child_name (GTK_STACK (self->grid_cell_stack), "autofill");
+      edit_autofill_details_update (EDIT_AUTOFILL_DETAILS (self->autofill_details),
+                                    GRID_STATE (self),
+                                    self->edit_state->runner,
+                                    self->resource);
+    }
+  else
+    {
+      gtk_stack_set_visible_child_name (GTK_STACK (self->grid_cell_stack), "word_list");
+      edit_autofill_details_update (EDIT_AUTOFILL_DETAILS (self->autofill_details_compact),
+                                    GRID_STATE (self),
+                                    self->edit_state->runner,
+                                    self->resource);
+      edit_word_list_update (EDIT_WORD_LIST (self->word_list_widget),
+                             GRID_STATE (self),
+                             self->resource);
+    }
+}
+
 static void
 update_all (EditWindow *self)
 {
@@ -257,35 +313,13 @@ update_all (EditWindow *self)
   switch (self->edit_state->stage)
     {
     case EDIT_STAGE_GRID:
-      update_compact_autofill_visibility (self);
-      edit_grid_update (EDIT_GRID (self->main_grid), GRID_STATE (self));
-      cell_preview_update_from_cursor (CELL_PREVIEW (self->grid_preview),
-                                       GRID_STATE (self),
-                                       self->edit_state->sidebar_logo_config);
-      edit_cell_update (EDIT_CELL (self->cell_widget), GRID_STATE (self), self->edit_state->sidebar_logo_config);
-      edit_grid_info_update (EDIT_GRID_INFO (self->edit_grid_widget), GRID_STATE (self), self->edit_state->info);
-      edit_histogram_update (EDIT_HISTOGRAM (self->letter_histogram_widget), self->edit_state->info);
-      edit_histogram_update (EDIT_HISTOGRAM (self->cluelen_histogram_widget), self->edit_state->info);
-
-      if (GRID_STATE_HAS_SELECTABLE (GRID_STATE (self)))
+      if (self->edit_state->puzzle_kind == IPUZ_PUZZLE_ACROSTIC)
         {
-          gtk_stack_set_visible_child_name (GTK_STACK (self->grid_cell_stack), "autofill");
-          edit_autofill_details_update (EDIT_AUTOFILL_DETAILS (self->autofill_details),
-                                        GRID_STATE (self),
-                                        self->edit_state->runner,
-                                        self->resource);
+          update_all_grid_crossword (self);
+          update_all_grid_acrostic (self);
         }
       else
-        {
-          gtk_stack_set_visible_child_name (GTK_STACK (self->grid_cell_stack), "word_list");
-          edit_autofill_details_update (EDIT_AUTOFILL_DETAILS (self->autofill_details_compact),
-                                        GRID_STATE (self),
-                                        self->edit_state->runner,
-                                        self->resource);
-          edit_word_list_update (EDIT_WORD_LIST (self->word_list_widget),
-                                 GRID_STATE (self),
-                                 self->resource);
-        }
+        update_all_grid_crossword (self);
       break;
     case EDIT_STAGE_CLUES:
       edit_grid_update (EDIT_GRID (self->main_grid), CLUES_STATE (self));
@@ -333,9 +367,11 @@ load_puzzle_kind (EditWindow *self)
   puzzle = puzzle_stack_get_puzzle (self->puzzle_stack);
   kind = ipuz_puzzle_get_puzzle_kind (puzzle);
 
+  self->edit_state->grid_info_widget_enabled = FALSE;
   self->edit_state->symmetry_widget_enabled = FALSE;
   self->edit_state->letter_histogram_widget_enabled = FALSE;
   self->edit_state->cluelen_histogram_widget_enabled = FALSE;
+  self->edit_state->answerlen_histogram_widget_enabled = FALSE;
 
   /* Figure out the default setup */
   switch (kind)
@@ -343,6 +379,7 @@ load_puzzle_kind (EditWindow *self)
     case IPUZ_PUZZLE_CROSSWORD:
     case IPUZ_PUZZLE_CRYPTIC:
     case IPUZ_PUZZLE_BARRED:
+      self->edit_state->grid_info_widget_enabled = TRUE;
       self->edit_state->symmetry_widget_enabled = TRUE;
       self->edit_state->letter_histogram_widget_enabled = TRUE;
       self->edit_state->cluelen_histogram_widget_enabled = TRUE;
@@ -350,6 +387,8 @@ load_puzzle_kind (EditWindow *self)
       break;
     case IPUZ_PUZZLE_ACROSTIC:
       grid_stage = "acrostic";
+      self->edit_state->letter_histogram_widget_enabled = TRUE;
+      self->edit_state->answerlen_histogram_widget_enabled = TRUE;
       break;
     default:
       g_assert_not_reached ();
@@ -368,8 +407,10 @@ load_puzzle_kind (EditWindow *self)
       edit_symmetry_set_symmetry (EDIT_SYMMETRY (self->symmetry_widget), symmetry, self->edit_state->sidebar_logo_config);
     }
 
+  gtk_widget_set_visible (self->grid_info_group, self->edit_state->grid_info_widget_enabled);
   gtk_widget_set_visible (self->letter_histogram_group, self->edit_state->letter_histogram_widget_enabled);
   gtk_widget_set_visible (self->cluelen_histogram_group, self->edit_state->cluelen_histogram_widget_enabled);
+  gtk_widget_set_visible (self->answerlen_histogram_group, self->edit_state->answerlen_histogram_widget_enabled);
 }
 
 static void
@@ -493,6 +534,32 @@ update_word_lists (EditWindow *self)
                                 self->edit_state->clue_selection_text);
 }
 
+/* It's a good idea to call fix_all() on puzzles when we get them. We
+ * are going to call them every iteration anyway — this will get it to
+ * a good state for the next iteration. */
+static void
+fixup_new_puzzle (IpuzPuzzle *puzzle)
+{
+
+  if (IPUZ_IS_ACROSTIC (puzzle))
+    {
+      /* We want to be a bit careful when loading an acrostic. Just
+       * assume that the quote and source strings _might_ be
+       * missing.*/
+      if (ipuz_acrostic_get_quote (IPUZ_ACROSTIC (puzzle)) == NULL)
+        ipuz_acrostic_fix_quote (IPUZ_ACROSTIC (puzzle),
+                                 IPUZ_ACROSTIC_SYNC_PUZZLE_TO_STRING);
+      if (ipuz_acrostic_get_source (IPUZ_ACROSTIC (puzzle)) == NULL)
+        ipuz_acrostic_fix_source (IPUZ_ACROSTIC (puzzle),
+                                  IPUZ_ACROSTIC_SYNC_PUZZLE_TO_STRING);
+    }
+  else if (IPUZ_IS_CROSSWORD (puzzle))
+    {
+      ipuz_crossword_fix_all (IPUZ_CROSSWORD (puzzle),
+                              NULL);
+    }
+}
+
 /* This is called when we clear the stack and push a new puzzle onto
  * it. */
 static void
@@ -504,11 +571,14 @@ pushed_puzzle_new (EditWindow *self)
 
   puzzle = puzzle_stack_get_puzzle (self->puzzle_stack);
 
+  fixup_new_puzzle (puzzle);
+
   g_clear_pointer (&self->edit_state, edit_state_free);
   self->edit_state = edit_state_new (puzzle);
 
   /* Update widgets */
   load_puzzle_kind (self);
+
   update_all (self);
 }
 
@@ -613,6 +683,11 @@ switch_grid_to_selectable (EditWindow *self)
 static void
 switch_grid_to_editable (EditWindow *self)
 {
+  /* The main grid is never editable if it's an acrostic
+   */
+  if (IPUZ_IS_ACROSTIC (GRID_STATE (self)->xword))
+    return;
+
   STATE_OP (self, grid, change_mode, GRID_STATE_EDIT);
   edit_grid_unset_anchor (EDIT_GRID (self->main_grid));
 
@@ -1496,6 +1571,354 @@ edit_metadata_showenumerations_changed_cb (EditWindow *self,
     }
 }
 
+static void
+snapshot_clue_func (IpuzClues         *clues,
+                    IpuzClueDirection  direction,
+                    IpuzClue          *clue,
+                    IpuzClueId        *clue_id,
+                    gpointer           user_data)
+{
+  gchar *answer;
+
+  answer = ipuz_clues_get_clue_string_by_id  (clues, clue_id);
+  g_array_append_val (user_data, answer);
+}
+
+static void
+clear_string (gchar **arr_element)
+{
+  g_free (*arr_element);
+  *arr_element = NULL;
+}
+
+static GArray *
+snapshot_acrostic_answers (IpuzAcrostic *acrostic)
+{
+  GArray *answers;
+
+  answers = g_array_new (FALSE, FALSE, sizeof (gchar *));
+  g_array_set_clear_func (answers, (GDestroyNotify) clear_string);
+  ipuz_clues_foreach_clue (IPUZ_CLUES (acrostic), snapshot_clue_func, answers);
+
+  return answers;
+}
+
+static void
+reapply_acrostic_answers_func (IpuzClues         *clues,
+                               IpuzClueDirection  direction,
+                               IpuzClue          *clue,
+                               IpuzClueId        *clue_id,
+                               gpointer           user_data)
+{
+  IpuzCellCoord coord;
+  GArray *answers = user_data;
+  gchar *answer;
+
+  /* Check to see if we've run out of answers to reapply */
+  if (clue_id->index >= answers->len)
+    return;
+
+  answer = g_array_index (answers, gchar *, clue_id->index);
+
+  /* short circuit invalid answers */
+  if (answer == NULL || answer[0] == '\0')
+    return;
+
+  if (ipuz_clue_get_first_coord (clue, &coord))
+    {
+      const gchar *sol;
+      IpuzCell *cell;
+
+      cell = ipuz_grid_get_cell (IPUZ_GRID (clues), &coord);
+      sol = ipuz_cell_get_solution (cell);
+
+      g_assert (sol != NULL);
+
+      /* system function, but we check sol and answer for NULL
+       * above */
+      if (strncmp (sol, answer, strlen (sol) == 0))
+        {
+          ipuz_acrostic_set_answer (IPUZ_ACROSTIC (clues),
+                                    clue_id->index,
+                                    answer);
+        }
+    }
+}
+
+static gboolean
+check_quote_contains_source (const gchar *quote,
+                             const gchar *source)
+{
+  IpuzCharsetBuilder *quote_b;
+  IpuzCharsetBuilder *source_b;
+  g_autoptr (IpuzCharset) quote_c = NULL;
+  g_autoptr (IpuzCharset) source_c = NULL;
+
+  quote_b = ipuz_charset_builder_new_from_text (quote);
+  source_b = ipuz_charset_builder_new_from_text (source);
+
+  quote_c = ipuz_charset_builder_build (quote_b);
+  source_c = ipuz_charset_builder_build (source_b);
+
+  return ipuz_charset_subset (quote_c, source_c);
+}
+
+
+/* Returns true, if the first letter of clue is answer */
+#if 0
+static gboolean
+check_clue_first_letter (IpuzAcrostic *self,
+                         IpuzClue     *clue,
+                         const gchar  *answer)
+{
+  IpuzCellCoord coord;
+
+  if (ipuz_clue_get_first_coord (clue, &coord))
+    {
+      const gchar *sol;
+      IpuzCell *cell;
+
+      cell = ipuz_grid_get_cell (IPUZ_GRID (self), &coord);
+      sol = ipuz_cell_get_solution (cell);
+
+      return g_str_equal (answer, sol);
+    }
+
+  return FALSE;
+}
+#endif
+
+
+static void
+edit_acrostic_details_quote_str_cb (EditWindow *self,
+                                    gchar      *quote_str)
+{
+  g_autoptr (GArray) old_answers = NULL;
+  IpuzAcrostic *acrostic;
+  g_autofree gchar *old_normalized_quote = NULL;
+  const gchar *normalized_quote;
+  const gchar *normalized_source;
+
+  acrostic = IPUZ_ACROSTIC (GRID_STATE (self)->xword);
+
+  /* Check if the quote string has changed. If it hasn't nothing else
+   * will change. */
+  if (g_strcmp0 (quote_str, ipuz_acrostic_get_quote (acrostic)) == 0)
+    return;
+
+  /* At least something has changed. We need to store our state. */
+  edit_state_save_to_stack (GTK_WIDGET (self), self->edit_state,
+                            self->puzzle_stack,
+                            TRUE);
+
+  /* snapshot the acrostic before we make changes
+   * Grab the normalized quote, set the quote, and then compare it to
+   * the the new one. We don't need to modify the grid if those two
+   * are the same. We could just be editing the whitespace. */
+  old_normalized_quote =
+    g_strdup (ipuz_acrostic_get_normalized_quote (acrostic));
+  normalized_source = ipuz_acrostic_get_normalized_source (acrostic);
+  old_answers = snapshot_acrostic_answers (acrostic);
+
+  /* set the new quote */
+  ipuz_acrostic_set_quote (acrostic, quote_str);
+  normalized_quote = ipuz_acrostic_get_normalized_quote (acrostic);
+
+  /* Check if the normalized quote changed. Eg., the quote_str could
+   * be different due to whitespace changing, but with an identical
+   * grid. In that instance, we bypass any grid changes. */
+  if (g_strcmp0 (old_normalized_quote, normalized_quote) == 0)
+    goto out;
+
+  ipuz_acrostic_fix_quote (acrostic, IPUZ_ACROSTIC_SYNC_STRING_TO_PUZZLE);
+
+  if (check_quote_contains_source (normalized_quote, normalized_source))
+    {
+      ipuz_acrostic_clear_answers (acrostic);
+      ipuz_acrostic_initialize_answers (acrostic);
+      ipuz_clues_foreach_clue (IPUZ_CLUES (acrostic),
+                               reapply_acrostic_answers_func, old_answers);
+    }
+  else
+    {
+      ipuz_acrostic_set_source (acrostic, "");
+      ipuz_acrostic_fix_source (acrostic, IPUZ_ACROSTIC_SYNC_STRING_TO_PUZZLE);
+    }
+
+  ipuz_acrostic_fix_labels (acrostic, IPUZ_ACROSTIC_LABEL_NUMBERS);
+
+ out:
+  puzzle_stack_push_change (self->puzzle_stack,
+                            STACK_CHANGE_GRID,
+                            IPUZ_PUZZLE (acrostic));
+}
+
+static void
+edit_acrostic_details_source_str_cb (EditWindow *self,
+                                     gchar      *source_str)
+{
+  g_autoptr (GArray) old_answers = NULL;
+  IpuzAcrostic *acrostic;
+  g_autofree gchar *old_normalized_source = NULL;
+  const gchar *normalized_source;
+
+  acrostic = IPUZ_ACROSTIC (GRID_STATE (self)->xword);
+
+  /* Check if the source string has changed. If it hasn't nothing else
+   * will change. */
+  if (g_strcmp0 (source_str, ipuz_acrostic_get_source (acrostic)) == 0)
+    return;
+
+  /* At least something has changed. We need to store our state. */
+  edit_state_save_to_stack (GTK_WIDGET (self), self->edit_state,
+                            self->puzzle_stack,
+                            TRUE);
+
+  /* snapshot the acrostic before we make changes*/
+  old_normalized_source = g_strdup (ipuz_acrostic_get_normalized_source (acrostic));
+  old_answers = snapshot_acrostic_answers (acrostic);
+
+  /* set the new quote */
+  ipuz_acrostic_set_source (acrostic, source_str);
+  normalized_source = ipuz_acrostic_get_normalized_source (acrostic);
+
+  /* Check if the normalized quote changed. Eg., the source_str could
+   * be different due to whitespace changing, but with an identical
+   * acrostic clue. In that instance, we bypass any changes. */
+  if (g_strcmp0 (old_normalized_source, normalized_source) == 0)
+    goto out;
+
+  ipuz_acrostic_fix_source (acrostic, IPUZ_ACROSTIC_SYNC_STRING_TO_PUZZLE);
+  ipuz_acrostic_clear_answers (acrostic);
+  ipuz_acrostic_initialize_answers (acrostic);
+  ipuz_clues_foreach_clue (IPUZ_CLUES (acrostic),
+                           reapply_acrostic_answers_func, old_answers);
+  ipuz_acrostic_fix_labels (acrostic, IPUZ_ACROSTIC_LABEL_NUMBERS);
+
+ out:
+  puzzle_stack_push_change (self->puzzle_stack,
+                            STACK_CHANGE_GRID,
+                            IPUZ_PUZZLE (acrostic));
+}
+
+#if 0
+static void
+configure_acrostic_generator (EditWindow        *self,
+                              AcrosticGenerator *generator)
+{
+  g_autoptr (GError) error = NULL;
+  const gchar *quote_str, *source_str;
+
+  quote_str = ipuz_acrostic_get_quote (IPUZ_ACROSTIC (self->edit_state->grid_state->xword));
+  source_str = ipuz_acrostic_get_source (IPUZ_ACROSTIC (self->edit_state->grid_state->xword));
+
+  acrostic_generator_set_word_resource (generator, self->resource);
+  g_assert (acrostic_generator_set_text (generator, quote_str, source_str, &error));
+}
+
+static void
+update_acrostic_details (EditWindow *self,
+                         gint        index)
+{
+  guint count;
+  guint n_results;
+  g_autoptr (GArray) answers = NULL;
+
+  n_results = task_runner_get_n_results (self->edit_state->runner);
+  count = task_runner_get_count (self->edit_state->runner);
+  answers = (GArray *) task_runner_get_result (self->edit_state->runner,
+                                               n_results - 1);
+
+  if (answers == NULL)
+    {
+      /* FIXME: Tell the user couldn't find any solution */
+      edit_acrostic_details_update (EDIT_ACROSTIC_DETAILS (self->acrostic_details),
+                                    NULL, count,
+                                    0, -1);
+      return;
+    }
+
+  /* Update puzzle */
+  if (!ipuz_acrostic_set_answers (IPUZ_ACROSTIC (self->edit_state->grid_state->xword), answers))
+    answers = NULL;
+  else
+    ipuz_acrostic_fix_labels (IPUZ_ACROSTIC (self->edit_state->grid_state->xword));
+
+  edit_acrostic_details_update (EDIT_ACROSTIC_DETAILS (self->acrostic_details),
+                                answers, count,
+                                n_results, index);
+
+  edit_histogram_update_acrostic_answers (EDIT_HISTOGRAM (self->answerlen_histogram_widget),
+                                          answers);
+}
+
+static void
+update_all_acrostic (EditWindow *self)
+{
+  guint n_results;
+
+  n_results = task_runner_get_n_results (self->edit_state->runner);
+
+  update_acrostic_details (self, n_results - 1);
+
+  if (self->edit_state->preview_window)
+    edit_preview_window_update (EDIT_PREVIEW_WINDOW (self->edit_state->preview_window),
+                                self->edit_state->preview_state);
+
+  sync_from_grid_state (self);
+
+
+  edit_word_list_update (EDIT_WORD_LIST (self->word_list_widget),
+                         GRID_STATE (self),
+                         self->resource);
+}
+
+static void
+acrostic_generator_finished_cb (EditWindow *self,
+                                AcrosticGenerator *generator)
+{
+  update_all_acrostic (self);
+}
+
+static gboolean
+update_acrostic_generator_timeout (EditWindow *self)
+{
+  if (task_runner_get_state (self->edit_state->runner) == TASK_RUNNER_RUNNING)
+    return TRUE;
+
+  self->edit_state->solver_timeout = 0;
+  return FALSE;
+}
+
+static void
+edit_acrostic_details_start_solve_cb (EditWindow *self)
+{
+  g_autoptr (PuzzleTask) puzzle_task = NULL;
+
+  if (self->edit_state->solver_timeout != 0)
+    {
+      /* We didn't stop the timer... ./~ */
+      g_warning ("Failed to clear the previous timeout");
+      g_clear_handle_id (&self->edit_state->solver_timeout, g_source_remove);
+    }
+
+  if (self->edit_state->solver_finished_handler == 0)
+    self->edit_state->solver_finished_handler =
+      g_signal_connect_swapped (self->edit_state->runner,
+                                "finished",
+                                G_CALLBACK (acrostic_generator_finished_cb), self);
+
+  puzzle_task = PUZZLE_TASK (acrostic_generator_new ());
+
+  configure_acrostic_generator (self, ACROSTIC_GENERATOR (puzzle_task));
+
+  task_runner_run (self->edit_state->runner, puzzle_task);
+
+  self->edit_state->solver_timeout =
+    g_timeout_add (1000, G_SOURCE_FUNC (update_acrostic_generator_timeout), self);
+}
+#endif
+
 /* Actions */
 /* Some actions are better done from this file rather than the ones in
    edit-window-controls.c. They can call the update functions from
@@ -1606,6 +2029,8 @@ _edit_window_class_controls_init (GtkWidgetClass *widget_class)
   gtk_widget_class_bind_template_callback (widget_class, style_background_color_changed_cb);
   gtk_widget_class_bind_template_callback (widget_class, edit_metadata_metadata_changed_cb);
   gtk_widget_class_bind_template_callback (widget_class, edit_metadata_showenumerations_changed_cb);
+  gtk_widget_class_bind_template_callback (widget_class, edit_acrostic_details_quote_str_cb);
+  gtk_widget_class_bind_template_callback (widget_class, edit_acrostic_details_source_str_cb);
 }
 
 void
diff --git a/src/edit-window-private.h b/src/edit-window-private.h
index 21eadf3d..11ca29d8 100644
--- a/src/edit-window-private.h
+++ b/src/edit-window-private.h
@@ -67,11 +67,15 @@ struct _EditWindow
   GtkWidget *word_list_widget;
   GtkWidget *autofill_details;
   GtkWidget *acrostic_details;
+  GtkWidget *acrostic_answers;
+  GtkWidget *grid_info_group;
   GtkWidget *edit_grid_widget;
   GtkWidget *letter_histogram_group;
   GtkWidget *letter_histogram_widget;
   GtkWidget *cluelen_histogram_group;
   GtkWidget *cluelen_histogram_widget;
+  GtkWidget *answerlen_histogram_group;
+  GtkWidget *answerlen_histogram_widget;
 
   /* Clues */
   GtkWidget *clue_details_widget;
diff --git a/src/edit-window.c b/src/edit-window.c
index 290614c5..269bc40c 100644
--- a/src/edit-window.c
+++ b/src/edit-window.c
@@ -134,11 +134,15 @@ edit_window_class_init (EditWindowClass *klass)
   gtk_widget_class_bind_template_child (widget_class, EditWindow, word_list_widget);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, autofill_details);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, acrostic_details);
+  gtk_widget_class_bind_template_child (widget_class, EditWindow, acrostic_answers);
+  gtk_widget_class_bind_template_child (widget_class, EditWindow, grid_info_group);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, edit_grid_widget);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, letter_histogram_group);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, letter_histogram_widget);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, cluelen_histogram_group);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, cluelen_histogram_widget);
+  gtk_widget_class_bind_template_child (widget_class, EditWindow, answerlen_histogram_widget);
+  gtk_widget_class_bind_template_child (widget_class, EditWindow, answerlen_histogram_group);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, clue_details_widget);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, clue_list_widget);
   gtk_widget_class_bind_template_child (widget_class, EditWindow, clue_suggestions);
diff --git a/src/edit-window.ui b/src/edit-window.ui
index 8967b95b..1164e363 100644
--- a/src/edit-window.ui
+++ b/src/edit-window.ui
@@ -143,13 +143,27 @@
                                         <property name="visible">false</property>
                                         <property name="icon-name">view-refresh-symbolic</property>
                                         <property name="child">
-                                          <object class="EditAcrosticDetails" id="acrostic_details">
-                                            <property name="halign">center</property>
-                                            <property name="valign">center</property>
+                                          <object class="GtkBox">
+                                            <property name="orientation">vertical</property>
+                                            <property name="spacing">8</property>
                                             <property name="margin-top">2</property>
                                             <property name="margin-bottom">2</property>
                                             <property name="margin-start">2</property>
                                             <property name="margin-end">2</property>
+                                            <child>
+                                              <object class="EditAcrosticDetails" id="acrostic_details">
+                                                <property name="halign">fill</property>
+                                                <property name="valign">fill</property>
+                                                <signal name="normalized-quote-str-changed" handler="edit_acrostic_details_quote_str_cb" swapped="true" />
+                                                <signal name="normalized-source-str-changed" handler="edit_acrostic_details_source_str_cb" swapped="true" />
+                                              </object>
+                                            </child>
+                                            <child>
+                                              <object class="EditAcrosticAnswers" id="acrostic_answers">
+                                                <property name="halign">fill</property>
+                                                <property name="valign">fill</property>
+                                              </object>
+                                            </child>
                                           </object>
                                         </property>
                                       </object>
@@ -424,7 +438,7 @@
                                         <property name="orientation">horizontal</property>
                                         <property name="spacing">8</property>
                                         <child>
-                                          <object class="AdwPreferencesGroup">
+                                          <object class="AdwPreferencesGroup" id="grid_info_group">
                                             <property name="title" translatable="true">Grid Information</property>
                                             <child>
                                               <object class="EditGridInfo" id="edit_grid_widget">
@@ -461,6 +475,19 @@
                                             </child>
                                           </object>
                                         </child>
+                                        <child>
+                                          <object class="AdwPreferencesGroup" id="answerlen_histogram_group">
+                                            <property name="title" translatable="true">Answer Length Distribution</property>
+                                            <child>
+                                              <object class="EditHistogram" id="answerlen_histogram_widget">
+                                                <property name="mode">answerlen</property>x
+                                                <property name="hexpand">true</property>
+                                                <property name="valign">fill</property>
+                                                <property name="vexpand">true</property>
+                                              </object>
+                                            </child>
+                                          </object>
+                                        </child>
                                       </object>
                                     </property>
                                   </object>
diff --git a/src/edit-word-list.c b/src/edit-word-list.c
index bcf6988a..2f2f5d13 100644
--- a/src/edit-word-list.c
+++ b/src/edit-word-list.c
@@ -76,7 +76,6 @@ static void down_word_activate_cb       (EditWordList      *self,
 static void show_multi_row_activated_cb (EditWordList      *self);
 static void edit_word_list_recalculate  (EditWordList      *self);
 
-
 static void
 setup_listitem_cb (GtkListItemFactory *factory,
                    GtkListItem        *list_item)
@@ -450,6 +449,8 @@ edit_word_list_update (EditWordList     *self,
       return;
     }
 
+  if (!IPUZ_IS_ACROSTIC (edit_state->xword))
+    {
   calculate_clue_word_pos (edit_state,
                            IPUZ_CLUE_DIRECTION_ACROSS,
                            &self->across_clue_string,
@@ -458,6 +459,15 @@ edit_word_list_update (EditWordList     *self,
                            IPUZ_CLUE_DIRECTION_DOWN,
                            &self->down_clue_string,
                            &self->down_pos);
+    }
+  else
+    {
+  calculate_clue_word_pos (edit_state,
+                           IPUZ_CLUE_DIRECTION_CLUES,
+                           &self->across_clue_string,
+                           &self->across_pos);
+
+    }
 
   edit_word_list_recalculate (self);
 }
diff --git a/src/meson.build b/src/meson.build
index 5e8de685..59b8c0e6 100644
--- a/src/meson.build
+++ b/src/meson.build
@@ -81,6 +81,8 @@ edit_sources = files (
   'basic-templates.c',
   'cell-preview.c',
   'charset-entry-buffer.c',
+  'edit-acrostic-answer.c',
+  'edit-acrostic-answers.c',
   'edit-acrostic-details.c',
   'edit-app.c',
   'edit-autofill-details.c',
@@ -124,6 +126,8 @@ edit_headers = files (
   'basic-templates.h',
   'cell-preview.h',
   'charset-entry-buffer.h',
+  'edit-acrostic-answer.h',
+  'edit-acrostic-answers.h',
   'edit-acrostic-details.h',
   'edit-app.h',
   'edit-autofill-details.h',
diff --git a/src/play-clue-list.c b/src/play-clue-list.c
index 77862d66..6897ca48 100644
--- a/src/play-clue-list.c
+++ b/src/play-clue-list.c
@@ -455,15 +455,13 @@ play_clue_list_update (PlayClueList *clue_list,
 
   g_return_if_fail (PLAY_IS_CLUE_LIST (clue_list));
 
-  /* We hide ourself if we don't have any clues to show. Maybe a
-   * little goofy */
+  /* We hide ourself if we don't have any clues to show. This can
+   * happen when editing acrostic puzzles, as the source string can
+   * easily be invalidated */
   clue_array = ipuz_clues_get_clues (IPUZ_CLUES (state->xword), clue_list->direction);
   if (clue_array == NULL ||
       clue_array->len == 0)
-    {
-      gtk_widget_set_visible (GTK_WIDGET (clue_list), FALSE);
-      return;
-    }
+    gtk_widget_set_visible (GTK_WIDGET (clue_list), FALSE);
   else
     gtk_widget_set_visible (GTK_WIDGET (clue_list), TRUE);
 
@@ -509,7 +507,7 @@ play_clue_list_update (PlayClueList *clue_list,
                 NULL);
 
   g_return_if_fail (end <= clue_array->len);
-  g_return_if_fail (end > start);
+  g_return_if_fail (end >= start);
 
   /* If we don't have enough items in our list, we add them.
    * If we have too many, we remove them. */
@@ -582,9 +580,10 @@ play_clue_list_update (PlayClueList *clue_list,
                             showenumerations, zoom_level);
     }
 
-  /* We check to see if there's a cursor. If not, clear the selection
-   * and block the handler */
-  if (! GRID_STATE_CURSOR_SET (state))
+  /* We check to see if there's a cursor or any rows at all. If not,
+   * clear the selection and block the handler */
+  if (clue_list->n_rows == 0 ||
+      ! GRID_STATE_CURSOR_SET (state))
     {
       if (clue_list->selected_id)
         {
diff --git a/src/play-clue-row.c b/src/play-clue-row.c
index b2f342ca..7847e536 100644
--- a/src/play-clue-row.c
+++ b/src/play-clue-row.c
@@ -447,7 +447,7 @@ play_clue_row_update (PlayClueRow *clue_row,
   gtk_label_set_markup (GTK_LABEL (clue_row->clue_label), clue_string);
 
   /* Display the acrostic clue grid */
-  if (IPUZ_IS_ACROSTIC (state->xword))
+  if (clue_row->mode == PLAY_CLUE_ROW_PLAY_ACROSTIC)
     {
       LayoutConfig layout_config;
 
diff --git a/src/style.css b/src/style.css
index 7e58e58e..7848875f 100644
--- a/src/style.css
+++ b/src/style.css
@@ -409,3 +409,7 @@ histogram label.bar {
     font-size: 7pt;
     color: @blue_1;
 }
+
+window.edit label.answer {
+  font-size: 16pt;
+}
```
