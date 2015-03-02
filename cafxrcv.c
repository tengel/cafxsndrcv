/* cafxrcv
*  Receiving basic files from a Casio Algebra FX calculator
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

#include "comm.h"
#include "cafxrcv.h"


int main(int argc, char **argv)
{
	int ret;
	char *device = "/dev/ttyS0"; 
	
        // read command line arguments
        while((ret=getopt(argc, argv, "hVvd:"))!=EOF)
        {
	        switch(ret)
	        {
		        case 'h':
			        printf("cafxrcv %s \n", VERSION);
			        printf("Receive data from a Casio Algebra FX 2.0\n");    
				printf("usage: cafxrcv -hVvd<device> \n");
				printf("   -h			print help and exit\n");                                
				printf("   -V			print version information and exit\n");                                
				printf("   -v 			verbose mode\n");
				printf("   -d device            device where the Calculator ist connected to\n");
				exit(0);
			        break;
			case 'V':
	                        printf("cafxrcv %s \n", VERSION);
				exit(0);
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 'd':
				device = optarg;
				break;
			case '?':
				exit(1);
				break;
		}
	}
	rec_data(device);
	return 0;
}



int rec_data(char *device)
{
	int    fd=-1; 
	char mainheader[40];
	unsigned char fileheader[40];
	char endheader_rec[40];
	char endheader_ref[40] = {0x3a, 0x45, 0x4e, 0x44, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a};
	char ch=0x00;
	char chhex[5];
	int i;
	unsigned int filesize;
	int fd_diskfile;
	char filename[10] = "";
	char sys_cmd[255];
	int checksum=0;
	short int nextfile = FALSE;
	
	fd = opencom(device);
	if (fd<0) exit(0);

	printf("cafxrcv: waiting (%s)...\n", device);
	while (ch != 0x16) {
		ch = readchar(fd);
		if (ch != 0x16 && ch != 0x00) {
			hex(ch, &chhex[0]);
			printf("cafxrcv: 0x%s unexpected value. still waiting...\n",chhex);
			ch = 0x00;
		}
	}
	if (verbose)
		printf("cafxrcv: got 0x16. sending 0x13\n");
	ch = 0x13;
	write(fd, &ch, 1);
	for (i=0; i<=39; i++) {
		ch = readchar(fd);
		mainheader[i] = ch;
		if (i != 0) {
			checksum += ch;
		}
		if (verbose) {
			hex(ch, &chhex[0]);
			printf("cafxrcv: mainheader %d: %s \n", i, chhex);
		}
	}
	/* checksum = 256 - (sum of all bytes) % 256 */
	if (checksum % 256 != 0) {
		printf("cafxrcv: checksum error mainheader\n");
		exit(1);
	}
	for (i=0; i<=39; i++) {
		write(fd, &mainheader[i], 1);
	}
	if (verbose)
		printf("cafxrcv: waiting for 0x06...\n");
	ch = readchar(fd);
	if (ch != 0x6) {
		hex(ch, &chhex[0]);
		printf("cafxrcv: receive error: got: %s \n", chhex);
	}
	if (verbose)
		printf("cafxrcv: sending 0x06\n");
	ch = 0x6;
	write(fd, &ch, 1);
	checksum = 0;
	for (i = 0; i<=39; i++)  {
		ch = readchar(fd);
		fileheader[i] = ch;
		if (i != 0) {
			checksum += ch;
		}
		if (verbose) {
			hex(ch, &chhex[0]);
			printf("cafxrcv: fileheader %d: %s\n", i, chhex);
		}
	}
	if (checksum % 256 != 0) {
		printf("cafxrcv: checksum error fileheader\n");
		exit(1);
	}

	
	/* loop for more then one file */
	do
	{
		printf("cafxrcv: receiving ");
		nextfile = FALSE;
		if (verbose) {
			hex(fileheader[8], &chhex[0]);
			printf("cafxrcv: filesize high: %s \n", chhex);
			hex(fileheader[9], &chhex[0]);
			printf("cafxrcv: filesize low: %s \n", chhex);
		}
 		/* Calculating file size */	
		filesize = 0x00;
		filesize = fileheader[8];
		filesize = filesize<<8;
		filesize = filesize | fileheader[9];
		if (verbose)
			printf("cafxrcv: sending 0x06\n");
		ch = 0x06;
		write(fd, &ch, 1);
		printf(" %d byte ->", filesize);
		fd_diskfile = open("download.cafx", O_WRONLY | O_CREAT | O_TRUNC);
		if (fd_diskfile<0) {
			printf("cafxrcv: can't write output file download.cafx\n");
			exit(1);
		}
		checksum = 0;
		/* receiving data block */
		for (i = 1; i<=filesize+2; i++) {
			ch = readchar(fd);
			if (i != 1)
				checksum += ch;
			if (verbose) {
				hex(ch, &chhex[0]);
				printf("cafxrcv: data: %s ", chhex);
			}
			if (i>11 && i <= filesize) {
				hex(ch, &chhex[0]);
				write(fd_diskfile, &ch, 1);
				if (verbose)
					printf(" written");
			}
			if (verbose) {
				printf("\n");
			}
		}
		if (checksum % 256 != 0) {
			printf("cafxrcv: checksum error in Data!\n");
		}
		close(fd_diskfile);
		if (verbose)
			printf("cafxrcv: sending 0x6\n");
		ch = 0x6;
		write(fd, &ch, 1);
		/* receiving end-header (or next file-header)*/
		checksum = 0;
		for (i = 0; i<=39; i++) {
			ch = readchar(fd);
			endheader_rec[i] = ch;
			if (i != 0)
				checksum += ch;
			if (verbose) {
				hex(ch, &chhex[0]);
				printf("cafxrcv: endheader %d: %s \n", i, chhex);
			}
		}
		if (checksum % 256 != 0) {
			printf("cafxrcv: checksum error. \n");
			exit(1);
		}
		/* extracting filename from fileheader */ 
		for (i = 0; i<10; i++) {
			filename[i] = 0x0;
		}
		if (fileheader[11] != 0xff) {
			i = 11;
			while(fileheader[i] != 0xff) {
				filename[i-11] = fileheader[i];
				i++;
			}
		}
		else { 
			printf("cafxrcv: no valid filename transmitted\n");
			printf("cafxrcv: enter new filename: ");
			scanf("%s", filename);
			fflush(stdin);
		}
		printf(" %s\n", filename);
		strcpy(sys_cmd, "mv -i download.cafx ");
		strcat(sys_cmd, filename);
		strcat(sys_cmd, ".cafx");
		system(sys_cmd);
		/* if one or more files follow, the endheader is the fileheader
		   for the next file */
		for (i = 0; i<=39; i++)   /* use endheader as next fileheader */
			fileheader[i] = endheader_rec[i];
		for (i = 0; i<=39; i++) {
			/* check if fileheader was received */
			if (endheader_rec[i] != endheader_ref[i]) {
				nextfile = TRUE;
				break;
			}
		}
	}		
	while(nextfile);
	

	return(0);
}




