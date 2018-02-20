/* ----------------------------------------------- LED DRIVER --------------------------------------------------
 
// Author : Vishwakumar Doshi && Nisarg Trivedi
//Subject : Embedded system Programming
//Course code: CSE 438

 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/timer.h>

#define DEVICE_NAME                  "RGBLed"  // Name of device in /dev
#define CYCLE_DURATIONns 20000000L  // PWM cycle duration in nanoseconds
#define NS_IN_HALFs 500000000L // converting 0.5 seconds to nanoseconds


#define CONFIG 10

ktime_t on_interval,off_interval,pattern_expiration; // Ktime variables to hold on time, off time and pattern delay used in hrtimer

static struct hrtimer light_timer,pattern_timer;   // hrtimer struct variables. light for pwm and pattern for 0.5 s calculation

int light_flag,pattern_flag,errno,stp,RL,GL,BL;            // flags used to indicate turning on,off of LED and to return with no restart

int gparray[14][4]  =  {	                                      // Lookup table to get gpio pin numbers to control corresponding IO pins on board
							{11,32,100,100},			  		  // Each row corresponds to a particular IO pin number
							{12,28,45,100},               		  // ROW 0 corresponds to IO0 .... ROW 13 corresponds to IO13 
							{13,34,77,100},             		  // Here 100 means IGNORE meaning their values don't affect behaviour of IO pin
 							{14,16,76,64},
							{6,36,100,100},
							{0,18,66,100},
							{1,20,68,100},
							{38,100,100,100},
							{40,100,100,100},
							{4,22,70,100},
							{10,26,74,100},
							{5,24,44,72},
							{15,42,100,100},
							{7,30,46,100}
						};

struct led_desc               // Led desciption structure containing info about pin numbers and intensity
{
	int PWM;
	int R;
	int G;
	int B;
}; 


/* per device structure */


struct led_dev 
{
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	struct led_desc *led_dptr;      /* Name of pointer to led description struct */
} *led_devp;


static dev_t led_dev_number;      /* Allotted device number */
struct class *led_dev_class;          /* Tie with the device model */
static struct device *led_dev_device;


int check_gpio64to79(int ret)       // Checks whether a gpio pin belongs to the range gpio64-gpio79 because these pins don't require direction file to be set
{
	if((ret>=64) && (ret<=79))
		return 0;
	else
		return 1;
}


/*
* Open RGBLed driver
*/
int led_driver_open(struct inode *inode, struct file *file)
{
	struct led_dev *led_devp;

	/* Get the per-device structure that contains this cdev */
	led_devp = container_of(inode->i_cdev, struct led_dev, cdev);


	
	file->private_data = led_devp;
	printk(KERN_ALERT"\n%s is opening \n", led_devp->name);
	return 0;
}

/*
 * Release RGBLed driver
 */
int led_driver_release(struct inode *inode, struct file *file)
{
	struct led_dev *led_devp = file->private_data;
	
	printk(KERN_ALERT"\n%s is closing\n", led_devp->name);
	
	return 0;
}



/*
 * Write to RGBLed driver
 */
ssize_t led_driver_write(struct file *file, const char *buff, size_t count, loff_t *ppos)
{
	int rl,gl,bl;             //  pins to light up 

	struct led_dev *led_devp = file->private_data;
	int *a = kmalloc(sizeof(int),GFP_KERNEL);
  
    copy_from_user(a,(int*)buff,sizeof(int));

    pattern_flag = 0;

  	light_flag = 1;               //Initially led will be on for on time

  	stp = 0;                      // Initially is 0 and will be 1 on mouse detection
  	
  	RL = led_devp->led_dptr->R;
  	GL = led_devp->led_dptr->G;
  	BL = led_devp->led_dptr->B;
  
  	rl = ((*a & 0x4) > 0);        // indicates which pins to light up
  	gl = ((*a & 0x2) > 0);        // Last 3 bits indicate pins to be light up
  	bl = ((*a & 0x1) > 0);

    kfree(a);
 
    hrtimer_start(&light_timer, on_interval, HRTIMER_MODE_REL);             // starts hrtimer
    hrtimer_start(&pattern_timer, pattern_expiration, HRTIMER_MODE_REL);

    while(pattern_flag == 0)                                              // lights up leds according to pattern till 0.5 s hasn't passed
  	{																	 // pattern_flag is 1 on elapsing of 0.5 s
  		if(rl == 1 && gl == 0 && bl == 0)
  		{
  			gpio_set_value(gparray[RL][0],light_flag);
  			gpio_set_value(gparray[GL][0],0);
  			gpio_set_value(gparray[BL][0],0);
  		}

  		if(rl == 0 && gl == 1 && bl == 0)
		{
			gpio_set_value(gparray[RL][0],0);
			gpio_set_value(gparray[GL][0],light_flag);
			gpio_set_value(gparray[BL][0],0);
		}

  		if(rl == 0 && gl == 0 && bl == 1)
		{
			gpio_set_value(gparray[RL][0],0);
			gpio_set_value(gparray[GL][0],0);
			gpio_set_value(gparray[BL][0],light_flag);
		}

		if(rl == 1 && gl == 1 && bl == 0)
		{
			gpio_set_value(gparray[RL][0],light_flag);
			gpio_set_value(gparray[GL][0],light_flag);
			gpio_set_value(gparray[BL][0],0);
		}

		if(rl == 1 && gl == 0 && bl == 1)
		{
			gpio_set_value(gparray[RL][0],light_flag);
			gpio_set_value(gparray[GL][0],0);
			gpio_set_value(gparray[BL][0],light_flag);
		}

		if(rl == 0 && gl == 1 && bl == 1)
		{
			gpio_set_value(gparray[RL][0],0);
			gpio_set_value(gparray[GL][0],light_flag);
			gpio_set_value(gparray[BL][0],light_flag);
		}

		if(rl == 1 && gl == 1 && bl == 1)
		{
			gpio_set_value(gparray[RL][0],light_flag);
			gpio_set_value(gparray[GL][0],light_flag);
			gpio_set_value(gparray[BL][0],light_flag);
		}

		if(rl == 0 && gl == 0 && bl == 0)                             // 0 is only passed when mouse click occurs
		{
			gpio_set_value(gparray[RL][0],0);
			gpio_set_value(gparray[GL][0],0);
			gpio_set_value(gparray[BL][0],0);
			stp = 1;
		}

 	 }

	 
	 return 0;
}


enum hrtimer_restart light_callback(struct hrtimer *timer_for_restart)       // is called on expiration of light timer
{   
	ktime_t advance,lcurrent = ktime_get();                                  // advance and lcurrent are local ktime variables 
   
	 	if(light_flag == 0)                                                   // If off then is now ON for on interval
	 	{
	 		light_flag = 1;
	 		advance = on_interval;
	 	}
	 	else if(light_flag == 1)                                             // If on then is now OFF for off interval
	 	{
	 		light_flag = 0;
	 		advance = off_interval;
	 	}
	 	
	 	if(stp == 1)                                                         // doesnt restart on mouse click
	 		return HRTIMER_NORESTART;

		hrtimer_forward(timer_for_restart,lcurrent,advance);                // forwards for ontime and off time accordingly
		 return HRTIMER_RESTART;                                            // Restarts and continues PWM till 0.5 s flag is not set
	
}


enum hrtimer_restart pattern_callback(struct hrtimer *timer_for_restart)
{
	pattern_flag = 1;                                                         // sets flag on pattern timer expiration
	return HRTIMER_NORESTART;
}


static long led_ioctl (struct file *file,unsigned int cmd, unsigned long arg) // Initialises GPIO pins based on numbers passed by user in struct on command CONFIG
{
	int R,G,B,PWM;
	
	struct led_dev *led_devp = file->private_data;                           // gets device pointer
	led_devp->led_dptr = kmalloc(sizeof(struct led_desc),GFP_KERNEL);
	copy_from_user(led_devp->led_dptr,(struct led_desc*)arg,sizeof(struct led_desc)); // copies struct in led_desc struct in device struct

	
	 if(cmd == CONFIG)
	 {
	 	int e,j;

	 	hrtimer_init(&light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);    // Initialises timer using monotonic clock in Relative mode
		light_timer.function = &light_callback;                           // passes function pointer

		hrtimer_init(&pattern_timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
		pattern_timer.function = &pattern_callback;
		
	 	// Extracts valus and puts in global variables

	 	PWM = led_devp->led_dptr->PWM;                                     
	 	if(!((PWM >= 0) && (PWM <= 100)))
	 	{
	 		printk(KERN_ALERT"ERROR: Invalid value for PWM!\nPlease choose integer value from 0-100\n");
	 		errno = EINVAL;
	 		return -1;
	 	}

	 	R = led_devp->led_dptr->R;
	 	if(!((R >= 0) && (R <= 13)))
	 	{
	 		printk(KERN_ALERT"ERROR: Invalid value for pin number of RED LED!\nPlease choose integer value from 0-13\n");
	 		errno = EINVAL;
	 		return -1;
	 	}

	 	G = led_devp->led_dptr->G;
	 	if(!((G >= 0) && (G <= 13)))
	 	{
	 		printk(KERN_ALERT"ERROR: Invalid value for pin number of GREEN LED!\nPlease choose integer value from 0-13\n");
	 		errno = EINVAL;
	 		return -1;
	 	}

	 	B = led_devp->led_dptr->B;
	 	if(!((B >= 0) && (B <= 13)))
	 	{
	 		printk(KERN_ALERT"ERROR: Invalid value for pin number of BLUE LED!\nPlease choose integer value from 0-13\n");
	 		errno = EINVAL;
	 		return -1;
	 	}

	 	on_interval = ktime_set(0,((unsigned long)PWM * CYCLE_DURATIONns)/100);               //Sets intervals for different delays
	 	off_interval = ktime_set(0,((100 - (unsigned long)PWM) * CYCLE_DURATIONns)/100);
	 	pattern_expiration = ktime_set(0,NS_IN_HALFs);
	 	

	 	
		 for(j=0;j<4;j++)            // Initialising pins
			{
				if((gparray[R][j] != 100) && check_gpio64to79(gparray[R][j])) // Initialises only if it's not to be IGNored ie value is not 100
				{
					e = gpio_request(gparray[R][j],"sysfs");     
					if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d request failed \n",gparray[R][j]);

					e = gpio_direction_output(gparray[R][j],0);				   // Sets direction only if it doesn't belong to the range gpio64-gpio79
					  if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d direction set failed \n",gparray[R][j]);	
				}
			

		            // Initialising G pin
		
				if((gparray[G][j] != 100) && check_gpio64to79(gparray[G][j])) // Initialises only if it's not to be IGNored ie value is not 100
				{
					 e = gpio_request(gparray[G][j],"sysfs");     
					if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d request failed \n",gparray[G][j]);
					                                                          // Sets direction only if it doesn't belong to the range gpio64-gpio79
					e = gpio_direction_output(gparray[G][j],0);
					  if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d direction set failed \n",gparray[G][j]);	
				}
		

		            // Initialising B pin
		
				if((gparray[B][j] != 100) && check_gpio64to79(gparray[B][j])) // Initialises only if it's not to be IGNored ie value is not 100
				{
					e = gpio_request(gparray[B][j],"sysfs");     
					if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d request failed \n",gparray[B][j]); 
					                                                          // Sets direction only if it doesn't belong to the range gpio64-gpio79
					e = gpio_direction_output(gparray[B][j],0);
					  if(e < 0) 
						 printk(KERN_ALERT"ERROR: gpio%d direction set failed \n",gparray[B][j]);	
				}
			}
		return 0;
	  }

	  else
	  {
	  	printk(KERN_ALERT"Invalid command. Only CONFIG allowed\n");
	  	errno = EINVAL;
	  	return -1;
	  }


}


/* File operations structure. Defined in linux/fs.h */
static struct file_operations led_fops = {
    .owner		= THIS_MODULE,            /* Owner */
    .open		= led_driver_open,        /* Open method */
    .release	= led_driver_release,     /* Release method */
    .write		= led_driver_write,       /* Write method */
    .unlocked_ioctl      = led_ioctl      /* Ioctl method */
};

/*
 * Driver Initialization
 */
int __init led_driver_init(void)
{
	int ret;
	

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&led_dev_number, 0,1, DEVICE_NAME) < 0) {
			printk(KERN_ALERT "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	led_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	led_devp = kmalloc(sizeof(struct led_dev), GFP_KERNEL);
	if (!led_devp) {
		printk(KERN_ALERT"Bad Kmalloc\n"); return -ENOMEM;
	}

	

	/* puts name of device */
	sprintf(led_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&led_devp->cdev, &led_fops);
	led_devp->cdev.owner = THIS_MODULE;
	
	/* Connect the major/minor number to the cdev */
	
	ret = cdev_add(&led_devp->cdev, MKDEV(MAJOR(led_dev_number),0), 1);

	if (ret) {
		printk(KERN_ALERT"Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	led_dev_device = device_create(led_dev_class, NULL, MKDEV(MAJOR(led_dev_number), 0), NULL, DEVICE_NAME);
	if(led_dev_device < 0)
		printk(KERN_ALERT"Device create failed!\n");
	
	
	printk(KERN_ALERT "LED driver initialized.\n");
	return 0;
}


/* Driver Exit */
void __exit led_driver_exit(void)
{
	int j;
	for(j = 0 ; j<4 ; j++)
	{
		gpio_free(gparray[RL][j]);
		gpio_free(gparray[GL][j]);
		gpio_free(gparray[BL][j]);
	}

	/* Release the major number */
	unregister_chrdev_region((led_dev_number), 1);

	/* Destroy device */
	device_destroy (led_dev_class, MKDEV(MAJOR(led_dev_number), 0));
	cdev_del(&led_devp->cdev);        // Delete cdev
	kfree(led_devp->led_dptr);        // Frees led desc struct
	kfree(led_devp);                  // Frees device struct
	
	/* Destroy driver_class */
	class_destroy(led_dev_class);

	printk(KERN_ALERT"LED driver removed.\n");
}



module_init(led_driver_init);
module_exit(led_driver_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Team 1");
