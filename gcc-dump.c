/*
 * dump.c
 *
 * Copyright(C) 2023 - MT
 *
 * Prints the contents of one or more files in hexadecimal (with the option
 * to incude the corisponding ASCII characters in the output).
 *
 * Deliberatly avoids using 'getopt' or 'argparse' to maximize portability.
 *
 * This  program is free software: you can redistribute it and/or modify it
 * under  the terms of the GNU General Public License as published  by  the
 * Free  Software Foundation, either version 3 of the License, or (at  your
 * option) any later version.
 *
 * This  program  is distributed in the hope that it will  be  useful,  but
 * WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You  should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>. 
 *
 * 30 Jul 23   0.1   - Initial version - MT
 * 04 Aug 23         - Checks that the path is not a directory - MT
 * 10 Aug 23         - Fixed very silly error with true/false values! - MT
 *                   - Do not use stdbool.h as this isn;t available on some
 *                     platforms - MT
 *
 */

#define  NAME        "gcc-unload"
#define  VERSION     "0.1"
#define  BUILD       "0003"
#define  AUTHOR      "MT"
#define  COPYRIGHT   (__DATE__ + 7) /* Extract copyright year from date */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>   /* isprint */
#include <errno.h>

#include <sys/stat.h>
 
#define  BUFFER_SIZE 16

#define  false       0
#define  true        !false

char b_aflag, b_bflag, b_cflag, b_hflag;
unsigned char a_buffer[BUFFER_SIZE];

void v_version() /* Display version information */
{
   fprintf(stderr, "%s: Version %s\n", NAME, VERSION);
   fprintf(stdout, "Copyright(C) %s %s\n", COPYRIGHT, AUTHOR);
   fprintf(stdout, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
   fprintf(stdout, "This is free software: you are free to change and redistribute it.\n");
   fprintf(stdout, "There is NO WARRANTY, to the extent permitted by law.\n");
   exit(0);
}
 
#if defined(VMS) || defined(MSDOS) || defined (WIN32) /* Use DEC/Microsoft command line options */
void v_about() /* Display help text */
{
   fprintf(stdout, "Usage: %s [OPTION]... [FILE]...\n", NAME);
   fprintf(stdout, "Dump FILE(s) contents in hexadecimal or octal.\n\n");
   fprintf(stdout, "  /alphanumeric            display alpha numeric characters \n");
   fprintf(stdout, "  /characters              print characters under hex\n");
   fprintf(stdout, "  /octal                   display bytes in octal\n");
   fprintf(stdout, "  /header                  print filenames\n");
   fprintf(stdout, "  /version                 output version information and exit\n");
   fprintf(stdout, "  /?, /help                display this help and exit\n");
   exit(0);
}
#else
void v_about() /* Display help text */
{
   fprintf(stdout, "Usage: %s [OPTION]... [FILE]...\n", NAME);
   fprintf(stdout, "Dump FILE(s) contents in hexadecimal or octal.\n\n");
   fprintf(stdout, "  -a, --alphanumeric       display alpha numeric characters \n");
   fprintf(stdout, "  -b, --octal              display bytes in octal\n");
   fprintf(stdout, "  -c, --characters         print characters under hex\n");
   fprintf(stdout, "  -f, --filenames          print filenames\n");
   fprintf(stdout, "  -?, --help               display this help and exit\n");
   fprintf(stdout, "      --version            output version information and exit\n");
   exit(0);
}
#endif
 
void v_error(const char *s_fmt, ...) /* Print formatted error message */
{
   va_list t_args;
   va_start(t_args, s_fmt);
   fprintf(stderr, "%s : ", NAME);
   vfprintf(stderr, s_fmt, t_args);
   va_end(t_args);
}

int i_isfile(char *s_name) /* Return true if path is a file */
{
   struct stat t_file_d;
   stat(s_name, &t_file_d);
   return (S_ISREG(t_file_d.st_mode));
}

int i_isdir(char *s_name) /* Return true if path is a directory */
{
   struct stat t_file_d;
   stat(s_name, &t_file_d);
   return (S_ISDIR(t_file_d.st_mode));
}

void v_dump_hex(FILE *h_file, int i_address) /* Display a file using hexadecimal starting at the specified address */
{
   int i_count;
   int i_bytes = 0; /* Number of bytes read from file into buffer */ 

   while ((i_bytes = fread(a_buffer, 1, BUFFER_SIZE, h_file)) > 0)
   {
      if (b_bflag) 
         printf("%06o", i_address); /* Print address in octal */
      else 
         printf("%04X", i_address); /* Otherwise print address in hex */
      for (i_count = 0; i_count < i_bytes; i_count++) {
         if (b_bflag) 
            printf("%03o ", (unsigned char) a_buffer[i_count]); /* Print bytes using octal */
         else 
         {
            if (!(i_count % 4))
               printf(" "); /* Space out bytes in groups of four */
            printf("%02X", (unsigned char) a_buffer[i_count]); /* Otherwise print bytes using hex (default) */
         }
      }
      for (i_count = 0; i_count < i_bytes; i_count++) /* Replace non printing characters */
      { 
         if (!(isprint(a_buffer[i_count]) && a_buffer[i_count] < 127))
         {
            if (b_cflag) 
               a_buffer[i_count] = ' ';
            else
               a_buffer[i_count] = '.';
         }
      }
      if (b_aflag) /* Print ASCII characters on same line */
      {
         printf(" %*s", 4 - ((i_bytes - 1) / 4) + 2  * (BUFFER_SIZE - i_bytes), ""); /* Print required number of spaces */
         printf("%.*s", i_bytes, a_buffer); /* Print the number of characters in the buffer */
      }
      i_address += i_bytes;
      printf("\n");/* Print newline */
   }
}


int main(int argc, char **argv)
{
   FILE *h_file;
   int i_count, i_index;

#if defined(VMS) || defined(MSDOS) || defined (WIN32) /* Parse DEC/Microsoft style command line options */
   for (i_count = 1; i_count < argc; i_count++) 
   {
      if (argv[i_count][0] == '/')
      {
         for (i_index = 0; argv[i_count][i_index]; i_index++) /* Convert option to uppercase */
            if (argv[i_count][i_index] >= 'a' && argv[i_count][i_index] <= 'z')
               argv[i_count][i_index] = argv[i_count][i_index] - 32;
         if (!strncmp(argv[i_count], "/VERSION", i_index))
         {
            v_version(); /* Display version information */
         }
         else if (!strncmp(argv[i_count], "/ALPHANUMERIC", i_index))
         {
            b_aflag = true; b_cflag = false;
         }
         else if (!strncmp(argv[i_count], "/OCTAL", i_index))
         {
            b_bflag = true;
         }
         else if (!strncmp(argv[i_count], "/CHARACTERS", i_index))
         {
            b_cflag = true; b_aflag = false;
         }
         else if (!strncmp(argv[i_count], "/HEADER", i_index))
         {
            if (strlen(argv[i_count]) < 4) /* Check option is not ambigious */
            {
               v_error("option '%s' is ambiguous; please specify '/HEADER' or '/HELP'.\n", argv[i_count]);
               exit(-1);
            }
            b_hflag = true;
         }
         else if (!strncmp(argv[i_count], "/HELP", i_index))
            v_about();
         else if (!strncmp(argv[i_count], "/?", i_index))
            v_about();
         else 
         { /* If we get here then the we have an invalid option */
            v_error("invalid option %s\nTry '%s /help' for more information.\n", argv[i_count] , NAME);
            exit(-1);
         }
         if (argv[i_count][1] != 0) {
            for (i_index = i_count; i_index < argc - 1; i_index++) argv[i_index] = argv[i_index + 1];
            argc--; i_count--;
         }
      }
   }
#else /* Parse UNIX style command line options */
   char b_abort = false; /* Stop processing command line */
   for (i_count = 1; i_count < argc && (b_abort != true); i_count++)
   {
      if (argv[i_count][0] == '-')
      {
         i_index = 1;
         while (argv[i_count][i_index] != 0)
         {
            switch (argv[i_count][i_index])
            {
            case 'a': /* Print ASCII characters on same line */
                b_aflag = true; b_cflag = false; break;
            case 'b': /* Display bytes in octal */
                b_bflag = true; break;
            case 'c': /* Print ASCII characters underneath hex/octal values */
                b_cflag = true; b_aflag = false; break;
            case 'f': /* Print filenames */
                b_hflag = true; break;
            case '?': /* Display help */
               v_about();
            case '-': /* '--' terminates command line processing */
               i_index = strlen(argv[i_count]);
               if (i_index == 2)
                 b_abort = true; /* '--' terminates command line processing */
               else
                  if (!strncmp(argv[i_count], "--version", i_index))
                  {
                     v_version(); /* Display version information */
                  }
                  else if (!strncmp(argv[i_count], "--alphanumeric", i_index))
                  {
                     b_aflag = true; b_cflag = false;
                  }
                  else if (!strncmp(argv[i_count], "--octal", i_index))
                  {
                     b_bflag = true;
                  }
                  else if (!strncmp(argv[i_count], "--characters", i_index))
                  {
                     b_cflag = true; b_aflag = false;
                  }
                  else if (!strncmp(argv[i_count], "--filenames", i_index))
                     b_hflag = true;
                  else if (!strncmp(argv[i_count], "--help", i_index))
                     v_about();
                  else
                  { /* If we get here then the we have an invalid long option */
                     v_error("%s: invalid option %s\nTry '%s --help' for more information.\n", argv[i_count][i_index] , NAME);
                     exit(-1);
                  }
               i_index--; /* Leave index pointing at end of string (so argv[i_count][i_index] = 0) */
               break;
            default: /* If we get here the single letter option is unknown */
               v_error("unknown option -- %c\nTry '%s --help' for more information.\n", argv[i_count][i_index] , NAME);
               exit(-1);
            }
            i_index++; /* Parse next letter in options */
         }
         if (argv[i_count][1] != 0) {
            for (i_index = i_count; i_index < argc - 1; i_index++) argv[i_index] = argv[i_index + 1];
            argc--; i_count--;
         }
      }
   }
#endif

   for (i_count = 1; i_count < argc; i_count++) /* Dump files */
   {
      if (!i_isdir(argv[i_count])) /* Check that input files isn't a directory! */
      {
         if ((h_file = fopen(argv[i_count], "rb")) != NULL) 
         {
            if (b_hflag) fprintf(stdout, "%s:\n", argv[i_count]); /* Optionally print filename */
            v_dump_hex(h_file, 0x0100);
            fclose(h_file);
         }
         else
         {
#if defined(VMS) /* Use VAX-C extension (avoids potential ACCVIO) */
            v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno, vaxc$errno)); 
#else
            v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno));
#endif
         }
      }
      else
         v_error("Cannot open %s: Can't read from a directory\n", argv[i_count], argv[i_count]);
   }
   exit (0);
}
