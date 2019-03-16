/*
 * hexdump.c
 *
 *  Copyright(C) 2019   MEJT
 * 
 *  Prints  the contents of one or more files in hex and (optionally)  ascii
 *  on standard output.
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
 *  03 Mar 13  0.1    - Initial version - MEJT
 *  16 Mar 13         - Fixed  problems with printing ASCII characters  from
 *                      the buffer - MEJT
 *                    - Modified   format  strings and added  a  check   for
 *                      characters   values greater than  127  to   maintain
 *                      compatability with other compiler versions - MEJT
 *  04 Apr 18  0.2    - ASCII output now optional (option '-a') - MEJT
 *                    - Can now print bytes in in hex (the default) or octal
 *                      (option '-b') - MEJT
 *  06 Apr 18  0.3    - Tidied up the code so it now reads the data into the
 *                      buffer before it is printed - MEJT
 *                    - Moved the code to display the contents of the buffer
 *                      into a seperate routine - MEJT
 *                    - Modified  buffer display routine to allow the  ASCII
 *                      file contents to be shown below the hex/octal output
 *                      (option '-c') - MEJT
 *                    - Fixed bug that would cause data bytes that had their
 *                      most  signifigent bit set to be displayed as  signed
 *                      integers when converted to hex/octal - MEJT
 *  07 Apr 18         - Added  output file to output routines parameters for
 *                      'compatibility' with fprintf - MEJT
 *  12 Mar 19         - Rewrote command line parsing to remove dependancy on
 *                      'getopt' and added debug macro - MEJT
 *                    - Optionally prints the filename before dumping a file
 *                      (option '-h') - MEJT
 *                    - Fixed bug that stopped more than one file from being 
 *                      processed (reuse of count) - MEJT
 */

#define VERSION       "0.1.A"
#define BUILD         "0012"
#define AUTHOR        "MEJT"
#define DATE          "12 Mar 19"
 
#define DEBUG(code)   do {if (ENABLE_DEBUG) {code;}} while(0)
#define ENABLE_DEBUG  0
 
#include <stdio.h> 
#include <stdlib.h>   /* exit */
#include <ctype.h>    /* isorint */
#include <errno.h>    /* errno */
#include <string.h>   /* strerror */

#define delimiter '-'

#define BLOCK 16      /* Number of bytes in a 'block' */
 
static int aflag, bflag, cflag, hflag = 0;
 
int main (int argc, char **argv) {
  FILE *file;
  char buf[BLOCK]; /* Buffer for printable characters */
  int address, counter, byte;
  int count, index;
  int flag = 1;
 
  for (index = 1; index < argc && flag; index++) {
    if (argv[index][0] == delimiter) {
      count = 1;
      while (argv[index][count] != 0) {
        switch (argv[index][count]) {
          case 'a': /* Print ASCII characters on same line */
            aflag = 1; cflag = 0; break;
          case 'b': /* Display bytes in octal */
            bflag = 1; break;
          case 'c': /* Print ASCII characters underneath hex/octal values */
            cflag = 1; aflag = 0; break;
          case 'h': /* Print filenames */
            hflag = 1; break;          
          case delimiter: /* '--' terminates command line processing */
            if (strlen(argv[index]) == 2) {flag = 0; break;}
          default: /* If we get here the option is invalid */
            fprintf(stderr, "%s: unknown option -- %c\n", argv[0], argv[index][count]);
            fprintf(stderr, "Usage: %s [-cab] [file...]\n", argv[0]);
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
 
  DEBUG(fprintf(stderr, "%s: Version %s (%s)\n", argv[0], VERSION, DATE));
 
  int fprintbuf (FILE *output, int address, int count, char buf[]) {
    int counter;
    fprintf(output, "%07X: ", address); /* Print address */
    for (counter = 0; counter < count; counter++) { /* Print buffer */
      if (bflag) /* Print bytes using octal */
        fprintf(output, "%03o ", (unsigned char) buf[counter]);
      else /* Otherwise print byte using hex (default) */
        fprintf(output, "%02X ", (unsigned char) buf[counter]);
    }
    for (counter = 0; counter < count; counter++) { /* Replace non printing*/
      if (!(isprint(buf[counter])&&buf[counter]<127)) {
        if (cflag) 
          buf[counter] = ' ';
        else
          buf[counter] = '.';
      }
    }
    if (aflag) { /* Print ASCII characters on same line */
      for (; counter < BLOCK; counter++) {
        if (bflag) 
          fprintf(output, "    "); /* Pad output with spaces */
        else 
          fprintf(output, "   "); /* Pad outout with spaces */
        buf[counter] = ' '; /* Pad buffer with spaces */
      }
      fprintf(output, "%s", buf); 
    }
    if (cflag) { /* Print ASCII characters underneath hex/octal values */
      fprintf(output, "\n%07X: ", address); /* Print address again ? */
      for (counter = 0; counter < count; counter++) {
        if (bflag) /* Display bytes in octal */
          fprintf(output, " %c  ", buf[counter]);
        else /* Otherwise print byte in hex (default) */
          fprintf(output, " %c ", buf[counter]);
      }
    }
    return(fprintf(output, "\n"));  /* Return status from fprintf */
  }

  buf[BLOCK] = '\0'; /* String terminator */
  for (count = 1; count < argc; count++) {
    if ((file = fopen(argv[count], "r")) == NULL) { /* Can't open file */
      fprintf(stdout, "%s: %s: Unable to open file\n", argv[0], argv[count]);
    } 
    else { /* Dump the contents of the file */
      if (hflag) fprintf(stdout, "%s:\n", argv[count]);
      counter = 0;
      address = 0;
      while ((byte = getc(file)) != EOF) {
        buf[counter] = byte; /* Put next byte in buffer */ 
        counter++;
        if (counter == BLOCK) { /* When we get to the end of a block */
          fprintbuf (stdout, address, counter, buf);
          address += counter;
          counter=0;
        } 
      } 
      if (counter != 0) fprintbuf (stdout, address, counter, buf); /* Flush output */
      fflush(stdout); 
      fclose(file); 
    }
  }
  exit(0);
}
