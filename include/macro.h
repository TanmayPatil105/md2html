/* macro.h
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
