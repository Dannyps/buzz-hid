#include <stdbool.h>
/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>


#define BIT(n) (0x01<<(n))

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
#include <signal.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & BIT(0) ? '1' : '0'), \
  (byte & BIT(1) ? '1' : '0'), \
  (byte & BIT(2) ? '1' : '0'), \
  (byte & BIT(3) ? '1' : '0'), \
  (byte & BIT(4) ? '1' : '0'), \
  (byte & BIT(5) ? '1' : '0'), \
  (byte & BIT(6) ? '1' : '0'), \
  (byte & BIT(7) ? '1' : '0')

typedef struct{
	bool light;
	bool buzzBtn, blueBtn, orangeBtn, greenBtn, yellowBtn;
} buzz_controller;

typedef struct{
	buzz_controller controller[4];
} buzz_hub;

void print_hex(const char *s, unsigned long len){
	while(len-->0)
		printf("0x%02x ", (unsigned char) *s++);
	printf("\n");
}

void printBuzzButtons(char* c){
	for(int i = 2; i<5; i++){
		printf("byte %d "BYTE_TO_BINARY_PATTERN" ", i, BYTE_TO_BINARY(c[i]));
	}
	printf("\n");
}

bool updateStructByRead(buzz_hub* hub, char* c){

	hub->controller[0].blueBtn 		= c[2] & BIT(4);
	hub->controller[0].orangeBtn 	= c[2] & BIT(3);
	hub->controller[0].greenBtn 	= c[2] & BIT(2);
	hub->controller[0].yellowBtn 	= c[2] & BIT(1);
	hub->controller[0].buzzBtn 		= c[2] & BIT(0);

	hub->controller[1].blueBtn 		= c[3] & BIT(1);
	hub->controller[1].orangeBtn 	= c[3] & BIT(0);
	hub->controller[1].greenBtn 	= c[2] & BIT(7);
	hub->controller[1].yellowBtn 	= c[2] & BIT(6);
	hub->controller[1].buzzBtn 		= c[2] & BIT(5);

	hub->controller[2].blueBtn 		= c[3] & BIT(6);
	hub->controller[2].orangeBtn 	= c[3] & BIT(5);
	hub->controller[2].greenBtn 	= c[3] & BIT(4);
	hub->controller[2].yellowBtn 	= c[3] & BIT(3);
	hub->controller[2].buzzBtn 		= c[3] & BIT(2);

	hub->controller[3].blueBtn 		= c[4] & BIT(3);
	hub->controller[3].orangeBtn 	= c[4] & BIT(2);
	hub->controller[3].greenBtn 	= c[4] & BIT(1);
	hub->controller[3].yellowBtn 	= c[4] & BIT(0);
	hub->controller[3].buzzBtn 		= c[3] & BIT(7);

	if(hub->controller[0].yellowBtn && hub->controller[1].yellowBtn && hub->controller[2].yellowBtn && hub->controller[3].yellowBtn){
		printf("pressed yellow on all controlers!\n");
	}

	for(int i = 0; i < 4; i++){
		hub->controller[i].light = hub->controller[i].buzzBtn;
	}
	return true;
}

void printHUB(buzz_hub* hub){
	for(int i=0;i<4;i++){
		buzz_controller c= hub->controller[i];
		printf(" 		     ^\n		   / %d \\\n		 / ", i);
		if(c.buzzBtn)
			printf("BUZZ");
		else
			printf("    ");
		printf("	 \\\n		/---------\\\n		|  ");
		if(c.blueBtn)
			printf("BLUE");
		else
			printf("    ");
		printf("   |\n		|  ");
		if(c.orangeBtn)
			printf("ORANGE");
		else
			printf("      ");
		printf(" |\n		|  ");
		if(c.greenBtn)
			printf("GREEN");
		else
			printf("     ");
		printf("  |\n		|  ");
		if(c.yellowBtn)
			printf("YELLOW");
		else
			printf("      ");
		printf(" |\n		-----------\n\n");
	}
}

void getRawName(unsigned long fd, char* buf){
	/* Get Raw Name */
	int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0){
		perror("HIDIOCGRAWNAME");
	}
}

int runBashCMD(char* cmd){
	if(fork()==0){
		execlp("bash", "bash", "-c", cmd, NULL);
	}
	return 0;
}

int updateLights(buzz_hub* hub){
	unsigned char buf[7];
	memset(buf, 0, 2);
	memset(buf+6, 0, 1);
	for(int i = 0; i<4; i++){
		if(hub->controller[i].light)
			buf[2+i]=0xff;
		else
			buf[2+i]=0x00;
	}

	char cmd[64];
	sprintf(cmd, "echo -e \"\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\" > /dev/hidraw0", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
	runBashCMD(cmd);
	return 0;
}

void sig_handler( int a){
	printf("=>  got signal %d\n", a);
}

int main(){
	signal(SIGPIPE, &sig_handler);
	buzz_hub hub;
	int fd = open("/dev/hidraw0", O_RDWR);
	if(fd==-1){
		fprintf(stderr, "The Buzz controller is not connected!. Exiting.\n");
		exit(-1);
	}
	char buf[256];
	getRawName(fd, buf);
	printf("Raw name is %s.\n", buf);

	while(1){
		char c[5];
		updateLights(&hub);
		int ret = read(fd, c, 5);
		if(ret==-1){
			fprintf(stderr, "The Buzz controller was disconnected. Exiting.\n");
			exit(-1);
		}
		//printf("got %d from read\n", ret);
		//printBuzzButtons(c);
		updateStructByRead(&hub, c);
		if(hub.controller[0].greenBtn || hub.controller[1].greenBtn || hub.controller[2].greenBtn || hub.controller[3].greenBtn){
			runBashCMD("echo \"ola\" > /dev/tcp/192.168.1.4/9100");
		}
		printHUB(&hub);
		//printf("done;\n");
	}
}
