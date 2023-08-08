/*
 * load.c
 *
 * Copyright(C) 2023   MT
 *  
 * Reads an eight bit Intel Hex or Motorola 'S' format file and and creates
 * a binary file (in the same way the CP/M-80 'load' command).
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
 * 03 Apr 18   0.1   - Initial version - MT
 * 08 Apr 18   0.2   - Included  the record type in checksum for Intel  Hex
 *                     format, this is more correct but produces a checksum
 *                     that is different from the original output  produced
 *                     by the CP/M-80 unload utility.  This difference does
 *                     not prvent the output from working with the original 
 *                     CP/M-80  load utility - MT
 * 04 Aug 23         - Rewrote parser from scratch - MT
 *                   - Checks that the path is not a directory - MT
 * 06 Aug 23         - Automatically generates the output filename from the
 *                     input  filename  (by checking that the  filetype  is
 *                     '.hex', and replacing the '.hex' with '.com') - MT
 * 
 * ToDo:             - If  the address of the next record is  greater  than
 *                     the current offset then pad output with NOPs.
 *                   - Check if the output file exists.
 *                   - Add support for Motorola 'S' format.
 *                   - Derive output filename from the input file name.
 * 
 * :100100003EFF87D200003E0187DA0000AFC2000048
 * :100110003CCA00003DFA00003DF200003E7F3CE298
 * :100120000000E6FFEA00001132010E09CD0500C310
 * :0701300000004F4B0D0A24F3
 * :0000000000
 * 
 * :10010000C3400220434F5059524947485420284386
 * :06010100010203040506E3
 * :0601016001020303050A80
 * :060101C001020304050623
 * :00000001FF
 * 
 * :1001000006081110010E09C5CD2101C10520F8C94D
 * :1001100048656C6C6F20576F726C642021210D0A4A
 * :0B01200024C305002A0100694B42E9DE
 * :00010001FE
 * 
 */

#define  NAME        "gcc-load"
#define  VERSION     "0.1"
#define  BUILD       "0002"
#define  AUTHOR      "MT"
#define  COPYRIGHT   (__DATE__ + 7) /* Extract copyright year from date */

#define  DEBUG

#define  true        0
#define  false       !true
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#if defined(VMS)
#include <stat.h>
#else
#include <sys/stat.h>
#endif
#include "gcc-debug.h"


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
   fprintf(stdout, "Loads intel hexadecimal FILE(s) to binary FILE(s).\n\n");
   fprintf(stdout, "  /version                 output version information and exit\n");
   fprintf(stdout, "  /?, /help                display this help and exit\n");
   exit(0);
}
#else
void v_about() /* Display help text */
{
   fprintf(stdout, "Usage: %s [OPTION]... [FILE]...\n", NAME);
   fprintf(stdout, "Loads intel hexadecimal FILE(s) to binary FILE(s).\n\n");
   fprintf(stdout, "  -?, --help               display this help and exit\n");
   fprintf(stdout, "      --version            output version information and exit\n");
   exit(0);
}
#endif
 
void v_error(const char *s_fmt, ...) /* Print formatted error message */
{
   va_list t_args;
   va_start(t_args, s_fmt);
   fprintf(stderr, "%s: ", NAME);
   vfprintf(stderr, s_fmt, t_args);
   va_end(t_args);
}

int i_isfile(char *s_name) /* Return true if path is a file */
{
   struct stat t_file_d;
   stat(s_name, &t_file_d);
#if defined(VMS)
   return ((t_file_d.st_mode & S_IFMT) == S_IFREG);
#else
   return (S_ISREG(t_file_d.st_mode));
#endif
}

int i_isdir(char *s_name) /* Return true if path is a directory */
{
   struct stat t_file_d;
   stat(s_name, &t_file_d);
#if defined(VMS)
   return ((t_file_d.st_mode & S_IFMT) == S_IFDIR);
#else
   return (S_ISDIR(t_file_d.st_mode));
#endif
}

int i_read_hex(FILE *h_input, FILE *h_output,int i_offset) /* Read intel hexadecimal and print bytes */
{
   int i_last = '\n';
   int i_char;
   int i_bytes;
   int i_address;
   int i_type;
   int i_data;
   int i_checksum;
   int i_error = 0;
   int i_count = 0;

   while ((i_char = fgetc(h_input)) != EOF)
   {
      switch (i_count)
      {
         case 0: /* Check for start of record */
            if (((i_last == '\n') || (i_last == '\r')) && (i_char == ':')) /* Start of record */
            {
               fprintf (stdout, "%c", i_char);
               i_bytes = 0; /* Reset values */
               i_address = 0;
               i_type = 0;
               i_data = 0;
               i_checksum = 0;
               i_count++;
            }
            break;
         case 1: /* Get number of bytes in record */
         case 2:
            i_bytes <<= 4;
            if (i_char >= '0' && i_char <= '9') i_bytes |= ((i_char - '0') & 0x0F);
            else if (i_char >= 'a' && i_char <= 'f') i_bytes |= ((i_char - 'a' + 10) & 0x0F);
            else if (i_char >= 'A' && i_char <= 'F') i_bytes |= ((i_char - 'A' + 10) & 0x0F);
            else i_error++;
            if (i_count == 2) fprintf (stdout, "%02X", i_bytes);
            i_count++;
            break;
         case 3: /* Get address of bytes in record */
         case 4:
         case 5:
         case 6:
            i_address <<= 4;
            if (i_char >= '0' && i_char <= '9') i_address |= ((i_char - '0') & 0x0F);
            else if (i_char >= 'a' && i_char <= 'f') i_address |= ((i_char - 'a' + 10) & 0x0F);
            else if (i_char >= 'A' && i_char <= 'F') i_address |= ((i_char - 'A' + 10) & 0x0F);
            else i_error++;
            if (i_count == 6) fprintf (stdout, "%04X", i_address);
            i_count++;
            break;
         case 7: /* Get record type */
         case 8:
            i_type <<= 4;
            if (i_char >= '0' && i_char <= '9') i_type |= ((i_char - '0') & 0x0F);
            else if (i_char >= 'a' && i_char <= 'f') i_type |= ((i_char - 'a' + 10) & 0x0F);
            else if (i_char >= 'A' && i_char <= 'F') i_type |= ((i_char - 'A' + 10) & 0x0F);
            else i_error++;
            if (i_count == 8) fprintf (stdout, "%02X", i_type);
            i_checksum = i_bytes + i_type + (i_address / 256) + (i_address % 256);
            i_count++;
            break;
         default:
            if (i_char == '\r' || i_char == '\n')
            {
               i_count = 0; /* Read next record */
               if (i_bytes >= 0)
               {
                  i_error++;
                  fprintf (stdout, " - Error");
               }
               fprintf (stdout, "\n");
            }
            else
            {
               i_data <<= 4;
               if (i_char >= '0' && i_char <= '9') i_data |= ((i_char - '0') & 0x0F);
               else if (i_char >= 'a' && i_char <= 'f') i_data |= ((i_char - 'a' + 10) & 0x0F);
               else if (i_char >= 'A' && i_char <= 'F') i_data |= ((i_char - 'A' + 10) & 0x0F);
               else i_error++;
               i_count++;
               if (i_count % 2) 
               {
                  fprintf (stdout, "%02X", i_data);
                  if (i_bytes) 
                  {
                     fputc(i_data, h_output);
                     i_checksum += i_data;
                  }
                  else 
                  {
                     i_checksum = (~(i_checksum & 0xFF) + 1) & 0xFF;
                     if (i_checksum != i_data) 
                     {
                        i_error++;
                        fprintf (stdout, " - Error");
                     }
                     else
                        fprintf (stdout, " - Ok");
                  }
                  i_bytes--;
                  i_data = 0; /* Reset value before reading next byte */
               }
            }

      }
      if (i_last != '\n' || i_char != '\0') i_last= i_char; /* Ignore any leading NULL chracters at the start of each record */
   }
   return (i_char);
}

int main(int argc, char **argv)
{
   FILE *h_input, *h_output;
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
         if ((strlen(argv[i_count]) > 4)  && (!strcmp(argv[i_count] + strlen(argv[i_count]) - 4, ".hex"))) /* Check the filename ends in '.hex' */
         {
            if ((h_input = fopen(argv[i_count], "r")) != NULL) /* Open the input file */
            {
               if (argc > 2) fprintf(stdout, "%s\n", argv[i_count]); /* Print the files name if multiple files are being processed */
               strcpy(argv[i_count] + strlen(argv[i_count]) - 4, ".com"); /* Substitute '.com' for '.hex' in the file name */
               if ((h_output = fopen(argv[i_count], "wb")) != NULL) /* oOpen the output file */
               {
                  i_read_hex(h_input, h_output, 0x0100);
                  fclose(h_output);
               }
               else /* Can't open output file */
               {
#if defined(VMS) /* Use VAX-C extension (avoids potential ACCVIO) */
                  v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno, vaxc$errno)); 
#else
                  v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno));
#endif
               }
               fclose(h_input);
            }
            else /* Can't open input file */
            {
#if defined(VMS) /* Use VAX-C extension (avoids potential ACCVIO) */
               v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno, vaxc$errno)); 
#else
               v_error("Cannot open %s: %s\n", argv[i_count], strerror(errno));
#endif
            }
         }
         else
            v_error("Cannot open %s: Invalid filetype \n", argv[i_count], argv[i_count]);
      }
      else
         v_error("Cannot open %s: Can't read from a directory\n", argv[i_count], argv[i_count]);
   }
   exit (0);
}
