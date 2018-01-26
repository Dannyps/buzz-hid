
/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void print_hex(const char *s, unsigned long len){
	while(len-->0)
		printf("0x%02x ", (unsigned char) *s++);
	printf("\n");
}

void getRawName(unsigned long fd, char* buf){
	/* Get Raw Name */
	int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0){
		perror("HIDIOCGRAWNAME");
	}
}

int main(){
	int res;
	char buf[256];
	unsigned int fd = open("/dev/hidraw0", O_RDWR);

	while(1){
		char c[10];
		read(fd, c, 8);
		print_hex(c, 8);
		printf("done;\n");
	}
}
