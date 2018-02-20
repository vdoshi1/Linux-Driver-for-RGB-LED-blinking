// Author : Vishwakumar Doshi && Nisarg Trivedi
//Subject : Embedded system Programming
//Course code: CSE 438


/*we will insert a module in the kernel which will be used to give an output to the pins of 
Gallileo on which the RGB led is attached. The Attached RGB will run in a pattern that is given in the question with a jump of 0.5s
and with a PWM value or duty cycle value given by the user. After the fucntion starts the code will wait for a mouse event which 
will initiaite a termintaion of the program. The mouse event here is a click of a mouse button (left or right)*/



#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define LEFT 272
#define RIGHT 273
#define CONFIG 10
#define NANOSECONDS_IN_500ms 500000000ul // converting 0.5 seconds to nanoseconds


struct input_event ie;            // Structure to store mouse event values
int tstp = 0;	//Thread stop variable, is set to 1 on detection of a mouse click

int fd,fLED,j;      // j is for loop variable and fd is file descriptor for /dev/input/mice file

int click,bytes;  // left == 1 when left click occurs and right == 1 when right click occurs. bytes holds the number of bytes read from mouse device file

const char *mDevice = "/dev/input/event3";//Storing path of mouse event. Change this event number if your machine shows different number for mouse event

int patn[7] = {4,2,1,6,5,3,7};          // Array to store pattern for LEDs. last 3 bits carry info about which pins to light

void* m_check(void* arg);

struct led_desc                      // Carries info about pin number and intensity from user to kernel space
{
	int PWM;
	int R;
	int G;
	int B;
} led_data; 


pthread_t pid;

int main(int argc, char** argv) 	// argv is a vector containing user input parameters , but in string format.  
{ 	
	int i = -1;
	
	if(argc != 5)                   // If there are fewer or more than 4 arguments, it prints error and terminates
		{
			printf("\n\nERROR: Only 4 inputs allowed  PWM R G B\n");
			goto TERMINATE;
		}

	led_data.PWM = atoi(argv[1]);       // Converts string into intergers (ASCII to integer)
	

	led_data.R = atoi(argv[2]);	
	

	led_data.G = atoi(argv[3]);
	

	led_data.B = atoi(argv[4]);
	


	fLED = open("/dev/RGBLed" , O_RDWR);
	if(fLED == -1) //Because open returns -1 on occurence of error
    {
    	printf("ERROR Opening LED Driver\n");
    	goto TERMINATE;
    }


    j = ioctl(fLED,CONFIG,(unsigned long)&led_data);         // CONFIG command initialises gpio pins according to pin numbers in struct sent as argument and sets on and off time 
    if(j < 0)												 // based on PWM 
    {
    	printf("ERROR in IOCTL\n");
    	goto TERMINATE;
    }


	fd = open(mDevice,O_RDWR);     // Opens mouse event file located in /dev/input/event3

    if(fd == -1) //Because open returns -1 on occurence of error
    {
    	printf("ERROR Opening %s\n", mDevice);
    	goto TERMINATE;
    }


   
   // Creating pthread which will run parallelly to detect a mouse click event using mcheck function
   pthread_create(&pid,NULL,m_check,NULL);
	
	
	while(1)							// Continuously changes pattern till mouse click is detected 
	{
		i = (i+1)%7;                   // Rolls over on 7 in pattern array
		
		j = write(fLED,(char*)&patn[i],sizeof(int));     // Sends pattern in kernel space  
		if(j<0)
			printf("ERROR in writing to LED\n");
				
					
	
		if(tstp == 1)                                          // this flag is 1 on mouse click and leds are turned off and program terminates
			{
				i = 0;
				j = write(fLED,(char*)&i,sizeof(int));    
				if(j<0)
					printf("ERROR in writing to LED\n");
				break;
			}	
	}

	 pthread_join(pid,NULL);                                 // Waits till mouse detection thread terminates

	 printf("Mouse-click detected! \n\n");

TERMINATE:
	 printf("System successfully terminated!\n\n");
	 printf("Good Bye!\n\n");


 return 0;
}

void* m_check(void* arg)                         // Checks for a mouse click event and return 1 on detection; returns 0 otherwise
{
	while(tstp == 0)
	{
		bytes = read(fd, &ie, sizeof(struct input_event)); 

	        if(bytes > 0)
	        {
	         click = ie.code;                                    // code contains 272 on left click and 273 on right click 

	         if ((click == LEFT) || (click == RIGHT)) 
	              tstp = 1;           
	         }
	 }

	 pthread_exit(NULL);
}

