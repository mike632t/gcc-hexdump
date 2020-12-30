/*
 * hexdump.c
 *
 * Copyright(C) 2020 - MT
 * 
 * Prints  the contents of one or more files in hex and (optionally) octal.
 *
 * Avoids using 'getopt' or 'argparse' to maximize portability.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *
 * 03 Mar 13   0.1   - Initial version - MEJT
 * 16 Mar 13         - Fixed  problems with printing ASCII characters  from
 *                     the buffer - MEJT
 *                   - Modified   format  strings and added  a  check   for
 *                     characters   values greater than  127  to   maintain
 *                     compatibility with other compiler versions - MEJT
 * 04 Apr 18   0.2   - ASCII output now optional (option '-a') - MEJT
 *                   - Can now print bytes in in hex (the default) or octal
 *                     (option '-b') - MEJT
 * 06 Apr 18   0.3   - Tidied up the code so it now reads the data into the
 *                     buffer before it is printed - MEJT
 *                   - Moved the code to display the contents of the buffer
 *                     into a separate routine - MEJT
 *                   - Modified  buffer display routine to allow the  ASCII
 *                     file contents to be shown below the hex/octal output
 *                     (option '-c') - MEJT
 *                   - Fixed bug that would cause data bytes that had their
 *                     most  significant bit set to be displayed as  signed
 *                     integers when converted to hex/octal - MEJT
 * 07 Apr 18         - Added  output file to output routines parameters for
 *                     'compatibility' with fprintf - MEJT
 * 12 Mar 19         - Rewrote command line parsing to remove dependency on
 *                     'getopt' and added debug macro - MEJT
 *                   - Optionally prints the filename before dumping a file
 *                     (option '-h') - MEJT
 *                   - Fixed bug that stopped more than one file from being 
 *                     processed (reuse of count) - MEJT
 * 17 Mar 19   0.4   - Updated command line argument parsing code to handle
 *                     '--help' and '--version' - MEJT
 * 22 Mar 19         - Modified  code slightly to get it to allow it to  be 
 *                     compiled with Microsoft Quick C - MEJT
 *                   - Open file in binary mode - MEJT
 * 09 Jul 20   0.5   - Uses fread() to read directly into the buffer - MT
 * 10 Jul 20         - Only  prints the number of ASCII characters  in  the
 *                     buffer - MT
 *                   - Calculates  the number of spaces needed before ASCII
 *                     text (instead of using a loop to print spaces) - MT
 * 11 Jul 20   0.6   - Now shows correct address - MT
 *                   - Checks that the path is not a directory - MT
 * 21 Sep 20   0.7   - Fixed  bug in argument parser caused by an undefined
 *                     value for i_abort on the first pass through the loop
 *                     (only affected Tru64 UNIX) - MT
 *                   - Added type prefixes to vairable names - MT
 *                   - Can  now  parse  Microsoft/DEC  style  command  line
 *                     options (allows partial completion) - MT
 *                   
 * To Do:            - Check that source is a valid file...
 *                   - Default to copying standard input to standard output
 *                     if no arguments are specified on the command line.
 */
 
#define NAME         "gcc-hexdump"
#define VERSION      "0.7"
#define BUILD        "0023"
#define AUTHOR       "MT"
#define COPYRIGHT    (__DATE__ +7) /* Extract copyright year from date */
  
#define true         1
#define false        0
 
#define BUFFER_SIZE  16
 
#include <stdio.h>
#include <stdlib.h>     /* exit */
#include <string.h>
#include <stdarg.h>
#include <ctype.h>      /* isprint */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>   /* stat */
 
char b_aflag, b_bflag, b_cflag, b_hflag = false;
 
void v_version() { /* Display version information */
   fprintf(stderr, "%s: Version %s\n", NAME, VERSION);
   fprintf(stdout, "Copyright(C) %s %s\n", COPYRIGHT, AUTHOR);
   fprintf(stdout, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
   fprintf(stdout, "This is free software: you are free to change and redistribute it.\n");
   fprintf(stdout, "There is NO WARRANTY, to the extent permitted by law.\n");
   exit(0);
}
 
#if defined(VMS) || defined(MSDOS) || defined (WIN32) /* Use DEC/Microsoft command line options */
   void v_about() { /* Display help text */
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
   void v_about() { /* Display help text */
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
 
void v_error(const char *s_fmt, ...) { /* Print formatted error message */
   va_list t_args;
   va_start(t_args, s_fmt);
   fprintf(stderr, "%s : ", NAME);
   vfprintf(stderr, s_fmt, t_args);
   va_end(t_args);
}
  
int i_isfile(char *s_name) {
   struct stat t_file_d;
      stat(s_name, &t_file_d);
   return ((t_file_d.st_mode & S_IFMT) == S_IFREG);
}
 
int i_isdir(char *s_name) {
   struct stat t_file_d;
   stat(s_name, &t_file_d);
   return ((t_file_d.st_mode & S_IFMT) == S_IFDIR);
}
 #include <stdarg.h>

int i_fprintbuf (FILE *_file, int _address, int _size, char _buffer[]) {
   int i_count;
   fprintf(_file, "%07X: ", _address); /* Print address */
   for (i_count = 0; i_count < _size; i_count++) { /* Print buffer */
      if (b_bflag) /* Print bytes using octal */
         fprintf(_file, "%03o ", (unsigned char) _buffer[i_count]);
      else /* Otherwise print byte using hex (default) */
         fprintf(_file, "%02X ", (unsigned char) _buffer[i_count]);
   }
   for (i_count = 0; i_count < _size; i_count++) { /* Replace non printing*/
      if (!(isprint(_buffer[i_count])&&_buffer[i_count]<127)) {
         if (b_cflag) 
            _buffer[i_count] = ' ';
         else
            _buffer[i_count] = '.';
      }
   }
   if (b_aflag) { /* Print ASCII characters on same line */
      fprintf(_file, "%*s", (3 + b_bflag) * (BUFFER_SIZE - _size), ""); /* Print required number of spaces - but being a bit naughty here... */
      fprintf(_file, "%.*s", _size, _buffer); /* Print the number of characters in the buffer */
   }
   if (b_cflag) { /* Print ASCII characters underneath hex/octal values */
      fprintf(_file, "\n%07X: ", _address); /* Print address again ? */
      for (i_count = 0; i_count < _size; i_count++) {
         if (b_bflag) /* Display bytes in octal */
            fprintf(_file, " %c  ", _buffer[i_count]);
         else /* Otherwise print byte in hex (default) */
            fprintf(_file, " %c ", _buffer[i_count]);
      }
   }
   fprintf(_file, "\n");/* Print newline */
   return(fflush(_file)); /* Return status from fflush() */
}
 
int main(int argc, char **argv) {
   FILE *h_file;
   char c_buffer[BUFFER_SIZE];
   int i_bytes; /* Number of bytes read from file */
   int i_size;  /* Number of bytes read into the buffer */
   int i_count, i_index, i_status;
 
#if defined(VMS) || defined(MSDOS) || defined (WIN32) /* Parse DEC/Microsoft style command line options */
   for (i_count = 1; i_count < argc; i_count++) {
      if (argv[i_count][0] == '/') {
         for (i_index = 0; argv[i_count][i_index]; i_index++) /* Convert option to uppercase */
            if (argv[i_count][i_index] >= 'a' && argv[i_count][i_index] <= 'z')
               argv[i_count][i_index] = argv[i_count][i_index] - 32;
         if (!strncmp(argv[i_count], "/VERSION", i_index)) {
            v_version(); /* Display version information */
         } else if (!strncmp(argv[i_count], "/ALPHANUMERIC", i_index)) {
            b_aflag = true; b_cflag = false; 
         } else if (!strncmp(argv[i_count], "/CHARACTERS", i_index)) {
            b_cflag = true; b_aflag = false; 
         } else if (!strncmp(argv[i_count], "/OCTAL", i_index)) {
            b_bflag = true; 
         } else if (!strncmp(argv[i_count], "/HEADER", i_index)) {
            if (strlen(argv[i_count]) < 4) { /* Check option is not ambigious */
               v_error("option '%s' is ambiguous; please specify '/HEADER' or '/HELP'.\n", argv[i_count]);
               exit(-1);
            }
            b_hflag = true;
         } else if (!strncmp(argv[i_count], "/HELP", i_index)) {
            v_about();
         } else if (!strncmp(argv[i_count], "/?", i_index)) {
            v_about();
         } else { /* If we get here then the we have an invalid option */
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
   
   for (i_count = 1; i_count < argc && (b_abort != true); i_count++) {
      if (argv[i_count][0] == '-') {
         i_index = 1;
         while (argv[i_count][i_index] != 0) {
            switch (argv[i_count][i_index]) {
            case 'a': /* Print ASCII characters on same line */
                b_aflag = true; b_cflag = false; break;
            case 'b': /* Display bytes in octal */
                b_bflag = true; break;
            case 'c': /* Print ASCII characters underneath hex/octal values */
                b_cflag = true; b_aflag = false; break;
            case 'h': /* Print filenames */
                b_hflag = true; break;
            case '?': /* Display help */      
               v_about();               
            case '-': /* '--' terminates command line processing */
               i_index = strlen(argv[i_count]);
               if (i_index == 2)
                 b_abort = true; /* '--' terminates command line processing */
               else
                  if (!strncmp(argv[i_count], "--version", i_index)) {
                     v_version(); /* Display version information */
                  } else if (!strncmp(argv[i_count], "--alphanumeric", i_index)) {
                     b_aflag = true; b_cflag = false;
                  } else if (!strncmp(argv[i_count], "--octal", i_index)) {
                     b_bflag = true;
                  } else if (!strncmp(argv[i_count], "--characters", i_index)) {
                     b_cflag = true; b_aflag = false;
                  } else if (!strncmp(argv[i_count], "--filenames", i_index)) {
                     b_hflag = true;
                  } else if (!strncmp(argv[i_count], "--help", i_index)) {
                     v_about();
                  } else { /* If we get here then the we have an invalid long option */
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
   for (i_count = 1; i_count < argc; i_count++) { /* Dump files */
      i_bytes = 0; /* Reset byte count */
      if (i_isdir(argv[i_count])) {
         fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i_count], strerror(21));            
      }
      else {
         if ((h_file = fopen(argv[i_count], "rb")) != NULL) {
            while((i_size = fread(c_buffer, 1, BUFFER_SIZE, h_file)) > 0 ){ 
               if (i_bytes == 0) {
                  if (b_hflag) fprintf(stdout, "%s:\n", argv[i_count]); /* Optionally print filename */
               }
               i_fprintbuf (stdout, i_bytes, i_size, c_buffer); /* Print buffer */
               i_bytes += i_size;
            }
            fclose(h_file);
         }
         else {
            i_status = errno;
            fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i_count], strerror(i_status));      
         }
      }
   }
   exit (0);
}
