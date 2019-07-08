#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>

#define IMG_SZ 614400

int  main(void)
{
	struct stat buf;
	unsigned char binary[IMG_SZ];
	int fd;
	memset(binary, 0, IMG_SZ);
	
	if(stat("./u-boot.bin", &buf) != 0)
		return -1;
	//printf("File size %d\n", buf.st_size);
	
	fd = open("./u-boot.bin", O_RDWR);
	if(fd < 0)
	{
		printf("Open u-boot.bin file fail %d\n", fd);
		return -1;
	}
	read(fd, binary, buf.st_size);
	close(fd);
	
	fd = open("./u-boot_crc.bin", O_RDWR | O_CREAT, S_IRWXU);
	if(fd < 0)
	{
		printf("Open file fail %d\n", fd);
		return -1;
	}
	write(fd, binary, IMG_SZ);

	//printf("padding done\n");
	return 0;
} 
