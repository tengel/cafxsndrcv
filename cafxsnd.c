/*  cafxsnd
*   Sends basic files to an Casio Algebra FX Calculator
*   Timo Engel (timo-e@freenet.de)  2002/04/01
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*
*
*/

#include<stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "cafxsnd.h"
#include "comm.h"



int main(int argc, char **argv)
{
	int ret;
	char *send_file; 
	char *device = "/dev/ttyS0";
	char missing_option = TRUE;
	
        /* read command line arguments */
        while((ret=getopt(argc, argv, "-s:hVvd:"))!=EOF)
        {
	        switch(ret)
	        {
		        case 'h':
			        printf("cafxsnd %s \n", VERSION);
			        printf("Send file to a Casio Algebra FX\n");    
				printf("usage: cafxsnd -hVvd<device> -s<file>\n");
				printf("   -s file		send file to calculator\n");
				printf("   -h			print help and exit\n");                                
				printf("   -V			print version information and exit\n");                                
				printf("   -v 			verbose mode\n");
				printf("   -d device            device where the Calculator ist connected to\n");
				exit(0);
			        break;
			case 'V':
	                        printf("cafxsnd %s \n", VERSION);
				exit(0);
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 's':
				send_file=(char*)malloc(strlen(optarg)+1);
                                if(!send_file) {
					perror("malloc");
					exit(1);
				}
				strcpy(send_file, optarg);
				missing_option = FALSE;
				break;
			case 'd':
				device=(char*)malloc(strlen(optarg)+1);
                                if(!device) {
					perror("malloc");
					exit(1);
				}
                                strcpy(device, optarg);
                                break;
			case '?':
				exit(1);
				break;
		}
	}
	if (missing_option) {
		printf("cafxsnd: no filename specified.\n");
		exit(1);
	}
	send_data(send_file, device);
	return 0;
}



int send_data(char *file, char *device)
{
	int fd_sendfile;
	int fd_dev;
	unsigned char ch;
	// original mainheader
/*	char mainheader_ref[40] = {0x3a, 0x4d, 0x44, 0x4c, 0x31, 0x5a, 0x58, 0x39, 0x34, 0x35, 0xff, 0x30, 0x33, 0x38, 0x34, 0x30, 0x30, 0x4e, 0x30, 0x2e, 0x33, 0x34, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x45}; */
	// mainheader wie bei casio->pc
	unsigned char mainheader_ref[40] = {0x3a, 0x4d, 0x44, 0x4c, 0x31, 0x5a, 0x58, 0x39, 0x34, 0x35, 0xff, 0x30, 0x33, 0x38, 0x34, 0x30, 0x30, 0x4e, 0x31, 0x2e, 0x30, 0x32, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x49}; 
	unsigned char mainheader_rec[40];
	unsigned char fileheader[40] =     {0x3a, 0x4d, 0x43, 0x53, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};
	unsigned char endheader[40] =      {0x3a, 0x45, 0x4e, 0x44, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a};
	unsigned int filesize = 0;																						
	struct stat filestatus;
	int i;				
	unsigned int checksum=0;
	
	fd_dev = opencom(device);
	
	printf("cafxsnd: sending %s -> %s ", file, device);
	fd_sendfile = open(file, O_RDONLY );
	if (fd_sendfile < 0) {
		perror("open");
		exit(1);
	}

	fstat(fd_sendfile, &filestatus);
	filesize = filestatus.st_size;
	printf("(%d Bytes)\n", filesize);
	/* calculating file size for fileheader */
	filesize += 11;
	fileheader[8] = (filesize & 0xff00)>>8;
	fileheader[9] = filesize & 0x00ff;
	filesize -= 11;
	if (verbose) {
		printf("fileheader[8]: %x\n", fileheader[8]);
		printf("fileheader[9]: %x\n", fileheader[9]);
	}
        /* put filename in fileheader */
	i = 0;
	while(file[i] != 0x00 && file[i] != '.') {
		fileheader[i+11] = file[i];
		i++;
	}

	/* start transfer */
	if (verbose)
		printf("cafxsnd: sending 0x16\n");
	ch = 0x16;
	write(fd_dev, &ch, 1); 
	
	ch = readchar(fd_dev);
	if (ch != 0x13) {
		printf("cafxsnd: transmission error. Expected: 0x13 Got: %x\n", ch);
		exit(1);
	}
	if (verbose)
		printf("Got 0x13\n");
	/* sending mainheader */
	if (verbose) {
		printf("cafxsnd: sending mainheader\n");
	}
	for (i = 0; i<=39; i++) {
		write(fd_dev, &mainheader_ref[i], 1);
		if (verbose) {
			printf("cafxsnd: mainheader_send(%d): %x\n", i, mainheader_ref[i]);
		}
	}
	/* receiving mainheader */
	for (i = 0; i<=39; i++) {
		mainheader_rec[i] = readchar(fd_dev);
		if (verbose) {
			printf("cafxsnd: mainheader_rec(%d): %x\n", i, mainheader_rec[i]);
		}
	}
	
	ch = 0x06;
	write(fd_dev, &ch, 1);
	
	ch = readchar(fd_dev);
	if (ch != 0x06) {
		printf("cafxsnd: transmission error. Expected: 0x06 Got: %x\n", ch);
		exit(1);
	}
	/* calculating checksum for fileheader */
	for (i = 1; i<=39; i++)
		checksum += fileheader[i];
	fileheader[39] = 256 - checksum % 256;
	if (verbose) {
		printf(" fileheader sum: %d \n", checksum);
	}


	/* sending fileheader */
	for (i = 0; i<=39; i++) {
		write(fd_dev, &fileheader[i], 1);
		if (verbose) {
			printf("cafxsnd: fileheader_send(%d): %x\n", i, fileheader[i]);
		}
	}

	ch = readchar(fd_dev);
	if (ch != 0x06) {
		printf("cafxsnd: transmission error. Expected: 0x06 Got: %x\n",ch);
		if (ch == 0x21) {
			printf("filename already on calc?\n");
		}
		exit(1);
	}
	if (verbose) {
		printf("got 0x06\ndata: 0x3a\n");
	}
	/* start data-block */
	checksum = 0;
	ch = 0x3a;
	write(fd_dev, &ch, 1);
	ch = 0x00;
	for (i = 0; i <= 9; i++) {
		usleep(1000);
		write(fd_dev, &ch, 1);
		if (verbose) {
			printf("data: %x\n", ch);
		}
	}
	/* read file -> send to com port */
	for (i = 1; i <= filesize; i++) {
		usleep(1000);
		read(fd_sendfile, &ch, 1);
		write(fd_dev, &ch, 1);
		checksum += ch;
		if (verbose) {
			printf("cafxsnd: Data(%d / %d): %x\n", i, filesize, ch);
		}
	}
	usleep(1000);
	ch = 0x00;
	write(fd_dev, &ch, 1);
	usleep(1000);
	ch = 256 - checksum % 256;
	write(fd_dev, &ch, 1);
	if (verbose) {
		printf("data: 0\ndata: %x (checksum)\n", ch);
	}
	
	if (verbose) {
		printf("waiting for 0x06...");
	}
	ch = readchar(fd_dev);
	if (ch != 0x06) {
		printf("cafxsnd: transmission error. Expected: 0x06 Got: %x\n",ch);
		exit(1);
	}
	if (verbose) {
		printf("got!\nsending end block:\n");
	}
	
	
	/* end-block */
	for (i = 0; i <= 39; i++) {
		usleep(1000);
		write(fd_dev, &endheader[i], 1);
		if (verbose) {
			printf("end-block: %x\n", endheader[i]);
		}
	}
	

	close(fd_sendfile);
	return 0;
}






