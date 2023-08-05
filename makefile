# makefile
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Note - Shell commands must start with a tab character at the beginning
# of each line NOT spaces..!
#
#  30 Jul 23   0.1   - Initial version - MT
#   4 Aug 23         - Added backup files to tar archive - MT
#
PROJECT	=  gcc-hexdump

SOURCE	=  $(wildcard *.c) 	# Compile all source files 
INCLUDE	=  $(wildcard *.h) 	# Automatically get all include files 
BACKUP	=  $(wildcard *.c.[0-9])
OBJECT	=  $(SOURCE:.c=.o)
PROGRAM	=  $(SOURCE:.c=)

FILES	=  $(SOURCE) $(BACKUP) $(INCLUDE) LICENSE README.md makefile .gitignore .gitattributes
LANG	=  LANG_$(shell (echo $$LANG | cut -f 1 -d '_'))
UNAME	=  $(shell uname)

#LIBS	= 
FLAGS	=  -fcommon -Wall -pedantic -std=gnu99
#FLAGS	+= -Wno-comment -Wno-deprecated-declarations -Wno-builtin-macro-redefined
FLAGS	+= -D $(LANG)

# Operating system specific settings
ifeq ($(UNAME), NetBSD) # NetBSD..
LIBS	+=  -lcompat
#FLAGS	+=  -I /usr/X11R7/include/ -L /usr/X11R7/lib/ -R /usr/X11R7/lib
endif

# Compiler specific settings
ifeq ($(CC), cc)
LIBS	+=  -no-pie
FLAGS	+=  -ansi
endif

ifdef DEBUG
FLAGS	+=  -g
endif

make:$(PROGRAM) $(OBJECT)

all:clean $(PROGRAM) $(OBJECT)

# Compile sources
%.o : %.c 
	@$(CC) $(FLAGS) -c $<

# Link object file and display execuitable file to indecate progress
# and validate that it was created
%: %.o 
	@$(CC) $(FLAGS) -o $@ $<
	@ls --color $@  

clean:
	@rm -f $(OBJECT) # -v
	@rm -f $(PROGRAM) # -v
	
backup: clean
	@echo "$(PROJECT)-`date +'%Y%m%d%H%M'`.tar.gz"; tar -czpf ..\/$(PROJECT)-`date +'%Y%m%d%H%M'`.tar.gz $(FILES)	
