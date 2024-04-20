/*
 * @file: macro.h
 */

/*
 * @literals
 */
#define LINEBREAK "<br>"
#define NEWLINE   "\n"
#define TABSPACE  "\t"

#define INSERT_NEWLINE(file) \
        fwrite (NEWLINE, sizeof(char), 1, file);

#define INSERT_TABSPACE(file) \
        fwrite (TABSPACE, sizeof (char), 1, file);

#define INSERT_LINEBREAK(file) \
        fwrite (LINEBREAK, sizeof (char), 4, file);

#define UL_TOP_LEVEL_START(file)                               \
        fwrite (TABSPACE, sizeof (char), 1, file);             \
        fwrite ("<ul>", sizeof (char), strlen ("<ul>"), file); \
        fwrite (NEWLINE, sizeof (char), 1, file);

#define UL_TOP_LEVEL_END(file)                                   \
        fwrite (NEWLINE, sizeof (char), 1, file);                \
        fwrite (TABSPACE, sizeof (char), 1, file);               \
        fwrite ("</ul>", sizeof (char), strlen ("</ul>"), file);
