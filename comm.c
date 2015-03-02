/* Casio Algebra FX communication tool
   Timo Engel (timo-e@freenet.de)  2002/04/01

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.


*/
#include<stdio.h>
#include<termios.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#include "comm.h"


int verbose = FALSE;

char readchar(int fd)
{
	char ch;
	fd_set readfs;
	
	FD_SET(fd, &readfs); 
	select(fd+1, &readfs, NULL, NULL, NULL);
	if (FD_ISSET(fd, &readfs))
        {
		if (read(fd, &ch, 1)<0)
        	{
			printf("cafxcom: can't read descriptor %d\n", fd);
			exit(-1);
		}
	}
	return(ch);
}


int opencom(char file[50])
/* opens the serial port given as argument and returns
   the file desriptor.
*/
{
	int fd;
	struct termios portsettings;
	if (verbose)
		printf("cafxcom: open %s\n", file);
	fd = open(file, O_RDWR | O_NOCTTY);
	if (fd<0)
	{
		printf("cafxcom: can't open %s\n", file);
		exit(1);
	}
	/* parameters for the serial ports */
	portsettings.c_iflag = IGNBRK|IGNPAR|CSTOPB|IXOFF; 
	portsettings.c_oflag = 0;
	portsettings.c_cflag = CS8 | CREAD | CLOCAL;
	portsettings.c_lflag = 0; /* ECHO */
	portsettings.c_cc[VMIN] = 0;
	portsettings.c_cc[VTIME] = 0;
	cfsetospeed(&portsettings, B38400);
	cfsetispeed(&portsettings, B38400);
	if (tcsetattr(fd, TCSANOW, &portsettings)<0)
	{
		printf("cafxcom: can't open %s\n", file);
		exit(1);
	}
	if (verbose)
		printf("cafxcom: %s fd: %i\n", file, fd);
	return(fd);  
}



void hex(char ch, char *op)
/* returns a  8 bit value in hex
*/
{
  char nibble0, nibble1;
  
  nibble0 = ch & 15;
  nibble1 = (ch & 240) >> 4;
  if (nibble0 > 9) nibble0 += 87;
  else nibble0 += 48;
  if (nibble1 > 9) nibble1 += 87;
  else nibble1 += 48;

  op[0] = nibble1;
  op[1] = nibble0;
  op[2] = '\0';
}



