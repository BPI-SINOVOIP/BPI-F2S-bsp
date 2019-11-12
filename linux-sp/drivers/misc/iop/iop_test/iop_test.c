

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <getopt.h>
#include "iop_ioctl.h"
 
int main(int argc, char** argv)
{
	int fd;
	int i,ret;
	int para;
	char data[3];	
	
	for(;;)
    {
		int c;   
		c = getopt(argc, argv, "abcd");		
        if(c==-1)
            break;
		            		
		switch(c){
			case 'a':
			    fd=open("/dev/sp_iop",O_RDWR);
				if(fd ==-1)
				{
					perror("update normal code fail");					
				}
				ret = ioctl(fd, IOCTL_VALGET, 1);
				if(ret ==-1)
				{
					perror("ioctl error ");	
				}
				perror("update normal code success");	    
				printf("close fd");
				close(fd);		
				break;	
			case 'b':
			    fd=open("/dev/sp_iop",O_RDWR);
				if(fd ==-1)
				{
					perror("update standby code fail");					
				}
				ret = ioctl(fd, IOCTL_VALGET, 2);
				if(ret ==-1)
				{
					perror("ioctl error ");	
				}
				perror("update standby code success");	    
				printf("close fd");
				close(fd);		
				break;			

			case 'c':
			    fd=open("/dev/sp_iop",O_RDWR);
				if(fd ==-1)
				{
					perror("get data fail");					
				}
				ret = ioctl(fd, IOCTL_VALGET, 3);
				if(ret ==-1)
				{
					perror("ioctl error ");	
				}
				perror("get data success");	    
				printf("close fd");
				close(fd);		
				break;	
			case 'd':
			    fd=open("/dev/sp_iop",O_RDWR);
				if(fd ==-1)
				{
					perror("set data fail");					
				}
				data[0] = 0x01;
				data[1] = 0x51;
				data[2] = 0x15;
				ret = write(fd, data, 3);
				if(ret ==-1)
				{
					perror("write error "); 
				}				 
				perror("set data success");	    
				printf("close fd");
				close(fd);		
				break;					
		}		
	}	
}  

	
