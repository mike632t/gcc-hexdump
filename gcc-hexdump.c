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
 *                   
 * To Do:            - Check that source is a valid file...
 *                   - Default to copying standard input to standard output
 *                     if no arguments are specified on the command line.
 */
 
#define NAME         "gcc-hexdump"
#define VERSION      "0.6"
#define BUILD        "0021"
#define AUTHOR       "MT"
#define COPYRIGHT    (__DATE__ +7) /* Extract copyright year from date */
 
#define true         1
#define false        0
 
#define BUFFER_SIZE  16
 
#include <stdio.h>
#include <stdlib.h>     /* exit */
#include <string.h>
#include <ctype.h>      /* isprint */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>   /* stat */
 
char _aflag, _bflag, _cflag, _hflag = 0;
 
void about() { /* Display help text */
   fprintf(stdout, "Usage: %s [OPTION]... [FILE]...\n", NAME);
   fprintf(stdout, "Dump FILE(s) contents in hexadecimal or octal.\n\n");
   fprintf(stdout, "  -a,                      display alpha numeric characters \n");
   fprintf(stdout, "  -b,                      display bytes in octal\n");
   fprintf(stdout, "  -c,                      print characters under hex\n");
   fprintf(stdout, "  -h                       print filenames\n");
   fprintf(stdout, "      --help               display this help and exit\n");
   fprintf(stdout, "      --version            output version information and exit\n");
   exit(0);
}
 
void version() { /* Display version information */
   fprintf(stderr, "%s: Version %s\n", NAME, VERSION);
   fprintf(stdout, "Copyright(C) %s %s\n", COPYRIGHT, AUTHOR);
   fprintf(stdout, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
   fprintf(stdout, "This is free software: you are free to change and redistribute it.\n");
   fprintf(stdout, "There is NO WARRANTY, to the extent permitted by law.\n");
   exit(0);
}
 
int isfile(const char *_name) {
   struct stat _file_d;
   stat(_name, &_file_d);
   return ((_file_d.st_mode & S_IFMT) == S_IFREG);
}
 
int isdir(const char *_name) {
   struct stat _file_d;
   stat(_name, &_file_d);
   return ((_file_d.st_mode & S_IFMT) == S_IFDIR);   
}
 
int fprintbuf (FILE *_file, int _address, int _size, char _buffer[]) {
   int counter;
   fprintf(_file, "%07X: ", _address); /* Print address */
   for (counter = 0; counter < _size; counter++) { /* Print buffer */
      if (_bflag) /* Print bytes using octal */
         fprintf(_file, "%03o ", (unsigned char) _buffer[counter]);
      else /* Otherwise print byte using hex (default) */
         fprintf(_file, "%02X ", (unsigned char) _buffer[counter]);
   }
   for (counter = 0; counter < _size; counter++) { /* Replace non printing*/
      if (!(isprint(_buffer[counter])&&_buffer[counter]<127)) {
         if (_cflag) 
            _buffer[counter] = ' ';
         else
            _buffer[counter] = '.';
      }
   }
   if (_aflag) { /* Print ASCII characters on same line */
      fprintf(_file, "%*s", (3 + _bflag) * (BUFFER_SIZE - _size), ""); /* Print required number of spaces - but being a bit naughty here... */
      fprintf(_file, "%.*s", _size, _buffer); /* Print the number of characters in the buffer */
   }
   if (_cflag) { /* Print ASCII characters underneath hex/octal values */
      fprintf(_file, "\n%07X: ", _address); /* Print address again ? */
      for (counter = 0; counter < _size; counter++) {
         if (_bflag) /* Display bytes in octal */
            fprintf(_file, " %c  ", _buffer[counter]);
         else /* Otherwise print byte in hex (default) */
            fprintf(_file, " %c ", _buffer[counter]);
      }
   }
   fprintf(_file, "\n");/* Print newline */
   return(fflush(_file)); /* Return status from fflush() */
}
 
int main(int argc, char **argv) {
   FILE *file;
   char _buffer[BUFFER_SIZE];
   int _bytes; /* Number of bytes read from file */
   int _size;  /* Number of bytes read into the buffer */
   int _abort; /* Stop processing command line */
   int _count, _index, _status;
 
   /* Parse command line */
   for (_count = 1; _count < argc && (_abort != true); _count++) {
      if (argv[_count][0] == '-') {
         _index = 1;
         while (argv[_count][_index] != 0) {
            switch (argv[_count][_index]) {
            case 'a': /* Print ASCII characters on same line */
                _aflag = 1; _cflag = 0; break;
            case 'b': /* Display bytes in octal */
                _bflag = 1; break;
            case 'c': /* Print ASCII characters underneath hex/octal values */
                _cflag = 1; _aflag = 0; break;
            case 'h': /* Print filenames */
                _hflag = 1; break;
            case '?': /* Display help */      
               about();               
            case '-': /* '--' terminates command line processing */
               _index = strlen(argv[_count]);
               if (_index == 2) 
                 _abort = true; /* '--' terminates command line processing */
               else
                  if (!strncmp(argv[_count], "--version", _index)) {
                     version(); /* Display version information */
                  }
                  else if (!strncmp(argv[_count], "--show-filenames", _index)) {
                     _hflag = true;
                  }
                  else if (!strncmp(argv[_count], "--help", _index)) {
                     about();
                  }
                  else { /* If we get here then the we have an invalid long option */
                     fprintf(stderr, "%s: invalid option %s\n", argv[0], argv[_count]);
                     fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                     exit(-1);
               }
               _index--;
               break;
            default: /* If we get here the single letter option is unknown */
               fprintf(stderr, "%s: unknown option -- %c\n", argv[0], argv[_count][_index]);
               fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
               exit(-1);
            }
            _index++;
         }
         if (argv[_count][1] != 0) {
            for (_index = _count; _index < argc - 1; _index++) argv[_index] = argv[_index + 1];
            argc--; _count--;
         }
      }
   }
 
   /* Dump files */
   for (_count = 1; _count < argc; _count++) {
      _bytes = 0; /* Reset byte count */
      if (isdir(argv[_count])) {
         fprintf(stderr, "%s: %s: %s\n", argv[0], argv[_count], strerror(21));            
      }
      else {
         if ((file = fopen(argv[_count], "rb")) != NULL) {
            while((_size = fread(_buffer, 1, BUFFER_SIZE, file)) > 0 ){ 
               if (_bytes == 0) {
                  if (_hflag) fprintf(stdout, "%s:\n", argv[_count]); /* Optionally print filename */
               }
               fprintbuf (stdout, _bytes, _size, _buffer); /* Print buffer */
               _bytes += _size;
            }
            fclose(file);
         }
         else {
            _status = errno;
            fprintf(stderr, "%s: %s: %s\n", argv[0], argv[_count], strerror(_status));      
         }
      }
   }
   exit (0);
}
