/*
 * unload.c
 *
 * Prints  the  contents of one or more files in eight bit Intel Hex  format
 * (the  same as the output produced by the  CP/M-80  'unload'  command), or
 * Motorola 'S' format.
 *
 * This  program is free software: you can redistribute it and/or modify  it
 * under  the  terms of the GNU General Public License as published  by  the
 * Free  Software  Foundation, either version 3 of the License, or (at  your
 * option) any later version.
 *
 * This  program  is  distributed in the hope that it will  be  useful,  but
 * WITHOUT   ANY   WARRANTY;   without  even   the   implied   warranty   of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You  should have received a copy of the GNU General Public License  along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 07 Apr 18  0.1   - Initial version - MEJT
 *                  - Added option to allow the use to select either Intel
 *                    Hex or Motorola S format output - MEJT
 *                  - Created  a  seperate  routine to flush the  buffer, as
 *                    it looks neater - MEJT
 * 08 Apr 18  0.2   - Included  the  record type in checksum for  Intel  Hex
 *                    format, this  is more correct but produces a  checksum
 *                    that is different from the original output produced by
 *                    the  CP/M-80 unload utility.  This difference does not
 *                    prvent  the  output  from working  with  the  original
 *                    CP/M-80  load utility - MEJT
 *
 * ToDo:            - Add  options  to allow the user to optionally  specify
 *                    the start address and for the motorola 'S' format  the
 *                    transfer address.

 */

#include <stdio.h>
#include <stdlib.h>           /* exit */
#include <errno.h>            /* errno */
#include <getopt.h>
#include <string.h>           /* strerror */

#define ENABLE_DEBUG 1        /* Enable debugging*/
#define DEBUG(code) do {if (ENABLE_DEBUG) {code;}} while (0)

#define BUFFER_SIZE 16        /* Number of data bytes in each record */

static int sflag = 0;

int main (int argc, char **argv) {
  FILE *file;
  char buf[BUFFER_SIZE]; /* Buffer for printable characters */
  int offset = 0x0100; /* Default offset for intel records */
  int address, count, next;
  int argi, opt;

  while ((opt = getopt (argc, argv, "is?")) != -1) {
    switch (opt) {
      case 'i': /* Intel Hex format */
        sflag = 0;
        offset = 0x0100;  /* Default offset for Intel on CP/M-80 */
        break;
      case 's': /* Motorola S record format */
        sflag = 1;
        offset = 0x0000;  /* Default offset for Motorola S format */
        break;
      case '?': /* An error occoured parsing the options.. */
      default:
        exit (1);
      }
  }

  int fprintbuf (FILE *output, int offset, int address, int count,
    char buf[]) {
    int counter;
    int checksum;
    int type;
    address += offset;
    if (sflag)  { /* Print buffer as a Motorola S record */
      if (count) type = 1; else type = 9;
      count += 3; /* Count includes checksum and address bytes */
      fprintf(output, "S%01d%02X%04X", type % 10, (unsigned char) count,
        (unsigned int) address);
      checksum = (unsigned char) count;
      checksum += (unsigned int) address / 0x0100;
      checksum += (unsigned int) address % 0x0100;
      count -= 3;
      for (counter = 0; counter < count; counter++) { /* Print buffer */
        fprintf(output, "%02X", (unsigned char) buf[counter]);
        checksum += (unsigned char) buf[counter];
      }
      checksum = ~checksum; /* Compute 1's compliment */
      fprintf(output, "%02X", (unsigned char) checksum); /* Checksum */
    }
    else { /* Print buffer as a Intel Hex record (default) */
      if (count) type = 0; else type = 1;
      fprintf(output, ":%02X%04X%02X", (unsigned char) count,
        (unsigned int) address, (unsigned char) type);
      checksum = (unsigned char) count;
      checksum += (unsigned char) type; /* Correct checksum includes type */
      checksum += (unsigned int) address / 0x0100;
      checksum += (unsigned int) address % 0x0100;
      for (counter = 0; counter < count; counter++) { /* Print buffer */
        fprintf(output, "%02X", (unsigned char) buf[counter]);
        checksum += (unsigned char) buf[counter];
      }
      checksum = ~checksum + 1; /* Compute 2's compliment  */
      fprintf(output, "%02X", (unsigned char) checksum); /* Checksum */
    }
    return(fprintf(output, "\n"));  /* Return status from fprintf */
  }

  int fflushbuf (FILE *output, int offset, int address, int count, char buf[]) {
    if (count != 0) { /* Not at the end of a buffer */
      fprintbuf (output, offset, address, count, buf);
    }
    if (sflag)
      fprintbuf (output, 0, offset, 0, buf); /* Start address record */
    else
      fprintbuf (output, 0, 0, 0, buf); /* Start address record */
      /* Note - An  address  of zero is used to maintain compatibility  with
       * 'unload.com' */
    return(fflush(output));
  }

  for (argi = optind; argi < argc; argi++) {
    if ((file = fopen(argv[argi], "r")) == NULL) {
      fprintf(stderr, "%s: %s : %s\n", argv[0], argv[argi],
        strerror(errno));
    }
    else { /* Dump the contents of the file */
      count = 0;
      address = 0;
      while ((next = fgetc(file)) != EOF) {
        buf[count] = next; /* Put next byte in buffer */
        count++;
        if (count == BUFFER_SIZE) { /* When we get to the end of a block */
          fprintbuf (stdout, offset, address, count, buf);
          address += count;
          count=0;
        }
      }
      fflushbuf (stdout, offset, address, count, buf);
      fclose(file);
    }
  }
  exit(0);
}
