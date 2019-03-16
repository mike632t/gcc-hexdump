/*
 *  load.c
 *
 *  Copyright(C) 2019   MEJT
 * 
 *  Reads  an eight bit Intel Hex or Motorola 'S' format file and writes the
 *  records to a binary file.
 *
 *  This  program is free software: you can redistribute it and/or modify it
 *  under  the terms of the GNU General Public License as published  by  the
 *  Free  Software Foundation, either version 3 of the License, or (at  your
 *  option) any later version.
 *
 *  This  program  is distributed in the hope that it will  be  useful,  but
 *  WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *  Public License for more details.
 *
 *  You  should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  03 Apr 18  0.1    - Initial version - MEJT
 *  14 Mar 19         - Tidied up the error handling, when there are no more
 *                      bytes returned from the input file then if the whole
 *                      file has not been read then it is assumed that there
 *                      was an error in the current record - MEJT
 *                    - Added  option to print verbose output for  debugging
 *                      and removed the other redundent options (for now) as
 *                      they have not yet been implemented - MEJT
 *  15 Mar 19         - Avoided a type error by explicitly coercing a string
 *                      constant to a string when assigning filename - MEJT
 *                    - Modified error message - MEJT
 *                    - Added line number to error message - MEJT
 *                    - Fixed bug in option parser - MEJT
 *                    - Added the capability to parse long options including
 *                      those that are only partly complete and used this to
 *                      add the ability to display the program version using
 *                      a macro to return the copyright year - MEJT
 * 
 *  ToDo:             - Actually  write  the contents of the buffer  to  the
 *                      output file!!
 *                    - Implement ability to parse 'S' format records
 *                    - Derive output filename from the input file name
 * 
 * References:
 *                    - https://stackoverflow.com/questions/41898659
 *                    - https://stackoverflow.com/questions/3437404 MIN/MAX
 */

#define VERSION       "0.1"
#define BUILD         "0005"
#define AUTHOR        "MEJT"
#define DATE          "14 Mar 19"
#define COPYRIGHT     (__DATE__ +7) /* Extract current year from date */
 
#include <stdio.h> 
#include <stdlib.h>   /* exit */
#include <errno.h>    /* errno */
#include <string.h>   /* strerror */


#define DEBUG(code)   do {if (__DEBUG__) {code;}} while(0)
#define __DEBUG__  0

/* 
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))   
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; }) 
*/

#define BUFFER_SIZE   0xFF /* Maximum size of record */
  
int verbose = 0;
 
unsigned char *fgetb (unsigned char *byte, FILE *file) {
  #undef __DEBUG__
  #define __DEBUG__ 0
  char nibble;
  int count;
  
  count = 2;
  *byte = 0;
  while (count != 0 && ((nibble = fgetc(file)) != EOF)) {
    count--;
    if (nibble >= '0' && nibble <= '9') nibble = nibble - '0';
    else if (nibble >= 'a' && nibble <='f') nibble = nibble - 'a' + 10;
    else if (nibble >= 'A' && nibble <='F') nibble = nibble - 'a' + 10;
    else return (NULL);
    *byte = (*byte << 4) | (nibble & 0x0F); 
    }
  DEBUG(fprintf(stderr, "(%02X)", *byte));
  return (byte);
}
 
unsigned char *fgetbufi (FILE *file, unsigned char *buf, int *address, int*bytes) {
  #undef __DEBUG__
  #define __DEBUG__ 1
  /* :10010000C3400220434F5059524947485420284386 */
  /* :CCAAAATTDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDSS */
  int count = 0;
  unsigned char  byte, checksum = 0;
  
  if ((byte = fgetc(file)) == EOF) fprintf(stderr, " EOF\n");
  if (!feof(file)) { 
    if (byte == ':') {
      if (verbose) fprintf(stdout, "%c", byte);
      while (fgetb(&byte, file) != NULL) {
        count++;
        checksum += byte;
        switch (count) {
          case 1: /* Get number of data bytes */
            *bytes = byte; 
            break;
          case 2: /* Clear the address and fall through... */
            *address = 0; 
          case 3: /* Add curent address byte to the address  */
            *address = (*address << 8) + byte; 
            break;
          case 4: /* Check record type? */
            break;
          default: /* Otherwise it is data (or the checksum) */
            buf[count - 5] = byte;
        }
        if (verbose) fprintf(stdout, "%02X", byte);
      }
      if (count != 0 && *bytes < count - 5) {
        if (verbose) fprintf(stdout, " (Invalid data)\n"); 
      }
      else if (*bytes > count - 5) {
        if (verbose) fprintf(stdout, " (Unexpected end of record)\n");
      }
      else if (checksum) {
        if (verbose) fprintf(stdout, " (Invalid checksum %02X)\n", checksum);
      }
      else {
        if (verbose) fprintf(stdout, " (Ok)\n");
        return(buf);
      }
    }
    else {
      if (verbose) fprintf(stdout, "(Invalid record)\n");
    }
  }
  return (NULL);
}

 
int main (int argc, char **argv) {

#undef __DEBUG__
#define __DEBUG__  0

  FILE *input, *output;
  char *filename;
  unsigned char buf[BUFFER_SIZE]; /* Buffer for printable characters */
  int errnum = 0;
  int line, length, address = 0;

  int count, index;
  int flag = 1;
 
  for (index = 1; index < argc && flag; index++) {
    if (argv[index][0] == '-') {
      count = 1;
      while (argv[index][count] != 0) {
        switch (argv[index][count]) { 
          case 'v': /* Print verbose output for debugging */
            verbose = 1;
            DEBUG(fprintf(stderr, "%s:%0d: argv[%0d][%0d]: '%c'\n", argv[0], __LINE__, index, count, argv[index][count]));
            break; 
          case 'b': /* Print verbose output for debugging */
            verbose = 0;
            DEBUG(fprintf(stderr, "%s:%0d: argv[%0d][%0d]: '%c'\n", argv[0], __LINE__, index, count, argv[index][count]));
            break;                
          case '-': /* Check for long options */
            DEBUG(fprintf(stderr, "%s:%0d: argv[%0d][%0d]: '%c'\n", argv[0], __LINE__, index, count, argv[index][count]));
            count = strlen(argv[index]);
            if (count == 2) 
              flag = 0; /* '--' terminates command line processing */
            else
              if (!strncmp(argv[index], "--version", count)) { /* Display version information */
                fprintf(stderr, "%s: Version %s\n", argv[0], VERSION);
                fprintf(stderr, "Copyright(C) %s %s\n", COPYRIGHT, AUTHOR);
                fprintf(stderr, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
                fprintf(stderr, "This is free software: you are free to change and redistribute it.\n");
                fprintf(stderr, "There is NO WARRANTY, to the extent permitted by law.\n");
                exit(0);
              }
              else if (!strncmp(argv[index], "--verbose", count)) 
                verbose = 1;
              else {
                fprintf(stderr, "%s: invalid option %s\n", argv[0], argv[index]);
                fprintf(stderr, "Usage: %s [-v] [--version] [file...]\n", argv[0]);
                exit(-1);
              }
            count--;
            break;
          default: /* If we get here the option is invalid */
            fprintf(stderr, "%s: unknown option -- %c\n", argv[0], argv[index][count]);
              fprintf(stderr, "Usage: %s [-v] [--verbose] [--version] [file...]\n", argv[0]);
            exit(-1);
        }
        count++;
      }
      if (argv[index][1] != 0) {
        for (count = index; count < argc - 1; count++) argv[count] = argv[count + 1];
        argc--; index--;
      }
    }
  }
 
  DEBUG(for (count = 1; count < argc; count++) fprintf(stderr, "argv[%d]: %s \n", count, argv[count]));
 
  filename = (char *) "a.out";
  for (count = 1; count < argc && !errnum ; count++) {
    if ((input = fopen(argv[count], "r")) == NULL) {
      errnum = errno;
      fprintf(stderr, "%s: %s: %s\n", argv[0], argv[count], strerror(errnum));
    }
    else {
      if ((output = fopen(filename, "wb")) == NULL) {
        errnum = errno;
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[count], strerror(errnum));
      }
      else { /* Dump the contents of the file */
        line = 0;
        while (fgetbufi(input, buf, &address, &length) != NULL) {
          ++line;
          /* fprintbuf(stdout, buf, &address, &length); */
        }
        if (!feof(input)) { /* Check if the whole file has been processed - if not there was an error in the file */
          fprintf(stderr, "%s: %s:%d: invalid record\n", argv[0], argv[count], ++line);
          exit (-1);
        }
        fclose (output); 
      }
      fclose (input); 
    }
  }
  exit(0);
}
