/*
 * unload.c
 *
 * Copyright(C) 2023 - MT
 *
 * Lists the contents of one or more files in eight bit Intel Hex (the same
 * as the output produced by the  CP/M-80  'unload' command).
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
 *                   - Included  the record type field when calculating the
 *                     checksum value, this is more correct but results  in
 *                     a different a checksum value being generated for the
 *                     EOF  record than that used by the  original  CP/M-80
 *                     'unload' command.  I  don't know if this is due to a
 *                     bug in the CP/M-80 code or the documentation but the
 *                     original CP/M-80 'load' command accepts both - MT
 * 04 Aug 23         - Checks that the path is not a directory - MT
 * 10 Aug 23         - Fixed very silly error with true/false values! - MT
 *                     
 * ToDo:             - Add the support for the motorola 'S' format.
 *                   - Allow  the load address and the transfer address  to
 *                     specified on the command line?
 */

#define  NAME        "gcc-unload"
#define  VERSION     "0.1"
#define  BUILD       "0004"
#define  AUTHOR      "MT"
#define  COPYRIGHT   (__DATE__ + 7) /* Extract copyright year from date */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>   /* isprint */
#include <errno.h>
#include <sys/stat.h>

#define  BUFFER_SIZE 16

#define  false       0
#define  true        !false

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
   fprintf(stdout, "Dump FILE(s) using intel hexadecimal.\n\n");
   fprintf(stdout, "  /version                 output version information and exit\n");
   fprintf(stdout, "  /?, /help                display this help and exit\n");
   exit(0);
}
#else
void v_about() /* Display help text */
{
   fprintf(stdout, "Usage: %s [OPTION]... [FILE]...\n", NAME);
   fprintf(stdout, "Dump FILE(s) using intel hexadecimal.\n\n");
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

void v_dump_hex(FILE *h_file, int i_address) /* Display a file using intel hex starting at the specified address */
{
   int i_count;
   int i_bytes = 0; /* Number of bytes read from file into buffer */ 
   int i_type = 0;
   unsigned int i_checksum = 0;

   while ((i_bytes = fread(a_buffer, 1, BUFFER_SIZE, h_file)) > 0)
   {
      printf(":%02X%04X%02X", i_bytes, i_address, i_type); /* Print record length, address and record type */
      i_checksum = i_bytes + i_type + (i_address / 256) + (i_address % 256);
      for (i_count = 0; i_count < i_bytes; i_count++)
      {
         printf("%02X", a_buffer[i_count] & 0xFF);  /* Print each byte */
         i_checksum += a_buffer[i_count];
      }
      /* The checksum is the least significant byte of the the two's complement of the sum of all bytes values in the record */      
      printf("%02X\n", (~(i_checksum & 0xFF) + 1) & 0xFF); /* Print the checksum byte */
      i_address += i_bytes;
   }
   /* Print the end of file record - this could be shortened (a lot) but I've left it like this to show how the checksum is calculated */
   i_type = 1; /* EOF record type */
   i_checksum = i_type; /* Address and record length are both zero */
   printf(":%02X%04X%02X", 0, 0, i_type); /* Print record length, address and record type */
   printf("%02X\n", (~(i_checksum & 0xFF) + 1) & 0xFF); 
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
            v_version(); /* Display version information */
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
            case '?': /* Display help */
               v_about();
            case '-': /* '--' terminates command line processing */
               i_index = strlen(argv[i_count]);
               if (i_index == 2)
                 b_abort = true; /* '--' terminates command line processing */
               else
                  if (!strncmp(argv[i_count], "--version", i_index))
                     v_version(); /* Display version information */
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
