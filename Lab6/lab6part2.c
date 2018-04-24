#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kthread.h>	// for kthreads
#include <linux/sched.h>	// for task_struct
#include <linux/time.h>		// for using jiffies 
#define MSG_SIZE 40
#define CDEV_NAME "Lab6" 

MODULE_LICENSE("GPL");
unsigned long freq = 500; //for the wave
static struct task_struct *kthread1;
static int major;
static char buffer[MSG_SIZE];
int mydev_id; //char device id
unsigned long *start;
unsigned long *GPFSEL0; //speaker
unsigned long *GPFSEL1; //buttons
unsigned long *GPFSEL2;
unsigned long *GPFSET; //turn speaker on
unsigned long *GPFCLR; //clear event detect
unsigned long *GPEDS0; //event detect register
unsigned long *GPREN0; //rise edge detect
unsigned long *GPPUD; //pull down register
unsigned long *GPPUDClk0; //pull down register clock



int mydev_id;	// variable needed to identify the handler


int my_kthread(void *ptr) {
	while(1) {
		*GPFSET = *GPFSET | 0x0040;
		udelay(freq);
		*GPFCLR = *GPFCLR | 0x0040;		//turns speaker on
		udelay(freq);

		if(kthread_should_stop()) {
			do_exit(0);
		}
	}
	return 0;
}
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	// Whatever is in buffer will be placed into buffer, which will be copied into user space
	ssize_t dummy = copy_to_user(buffer, buffer, length);	// dummy will be 0 if successful

	// buffer should be protected (e.g. semaphore). Not implemented here, but you can add it.
	buffer[0] = '\0';	// "Clear" the message, in case the device is read again.
					// This way, the same message will not be read twice.
					// Also convenient for checking if there is nothing new, in user space.
	
	return length;
}
static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	ssize_t dummy;
	
	if(len > MSG_SIZE)
		return -EINVAL;
	
	// unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
	dummy = copy_from_user(buffer, buff, len);	// Transfers the data from user space to kernel space
	if(len == MSG_SIZE)
		buffer[len-1] = '\0';	// will ignore the last character received.
	else
		buffer[len] = '\0';
	//start checking for the values
	if(buffer[1] == 'A') {
		printk("Note 1\n");
		freq = 200;
	} else if (buffer[1] == 'B') {
		printk("Note 2\n");

		freq = 250;
	} else if (buffer[1] == 'C') {
		printk("Note 3\n");

		freq = 300;
	} else if (buffer[1] == 'D') {
		printk("Note 4\n");

		freq = 350;
	} else if (buffer[1] == 'E') {
		printk("Note 5\n");

		freq = 400;
	}
	
	return len;		// the number of bytes that were written to the Character Device.
}
static struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
};


static irqreturn_t button_isr(int irq, void *dev_id)
{
	// In general, you want to disable the interrupt while handling it.
	disable_irq_nosync(79);
	if(*GPEDS0 == 0x00010000) {
		freq = 200;
	}else if (*GPEDS0 == 0x00020000) {
		freq = 250;
	}else if (*GPEDS0 == 0x00040000) {		//checks which button is pressed and changes accordingly
		freq = 300;
	}else if (*GPEDS0 == 0x00080000) {
		freq = 350;
	} else if(*GPEDS0 == 0x00100000) {
		freq = 400;
	}
	// DO STUFF (whatever you need to do, based on the button that was pushed)
	// IMPORTANT: Clear the Event Detect status register before leaving.	
	//value will bet set inside event detect i.e. button that was pressed
	*GPEDS0 = *GPEDS0 | 0x001F0000;	
	enable_irq(79);		// re-enable interrupt
    return IRQ_HANDLED;
}

int init_module()
{

	int dummy = 0;
	char kthread[] = "my_thread";
	start = (unsigned long *) ioremap(0x3F200000, 4096);
	GPFSEL0 = start;
	GPFSEL1 = start + 1;	//set speaker to output and buttons to input
	GPFSEL2 = start + 2;

	GPFSET = (start + 0x001C / 4);
	GPFCLR = (start + 0x0028 / 4);	//used for speaker
	GPEDS0 = (start + 0x0040 / 4); //event detection
	GPREN0 = (start + 0x004C / 4); //rising edge detect (sync)
	GPPUD = (start + 0x0094 / 4); //pull down control
	GPPUDClk0 = (start + 0x0098 / 4);
	*GPFSEL0 = *GPFSEL0 | 0x00040000; //speaker set to output
	
	// Don't forget to configure all ports connected to the push buttons as inputs.
	*GPFSEL1 = *GPFSEL1 & 0xC003FFFF;
	*GPFSEL2 = *GPFSEL2 & 0xFFFFFFF8;

	// You need to configure the pull-downs for all those ports. There is
	// a specific sequence that needs to be followed to configure those pull-downs.
	// The sequence is described on page 101 of the BCM2835-ARM-Peripherals manual.
	// You can use  udelay(100);  for those 150 cycles mentioned in the manual.
	// It's not exactly 150 cycles, but it gives the necessary delay.
	// WiringPi makes it a lot simpler in user space, but it's good to know
	// how to do this "by hand".
	*GPPUD = *GPPUD | 0x00000001;
	udelay(100);
	*GPPUDClk0 = *GPPUDClk0 | 0x001F0000;
	udelay(100);
	*GPPUD = *GPPUD & 0xFFFFFFFE;
	*GPPUDClk0 = *GPPUDClk0 & 0xFFE0FFFF;
	
	
	// Enable (Async) Rising Edge detection for all 5 GPIO ports.	
	*GPREN0 = *GPREN0 | 0x001F0000;


	kthread1 = kthread_create(my_kthread, NULL, kthread);
	if(kthread1){	// true if kthread creation is successful
		// kthread is dormant after creation. Needs to be woken up
        	wake_up_process(kthread1);
    	}
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
     		printk("Registering the character device failed with %d\n", major);	//register and setup major for char dev
	     	return major;
	}
	printk("MAJOR: %d\n", major);	

	dummy = request_irq(79, button_isr, IRQF_SHARED, "Button_handler", &mydev_id);	//attach handler

	printk("Button Detection enabled.\n");
	return 0;
}

// Cleanup - undo whatever init_module did
void cleanup_module()
{
	kthread_stop(kthread1);
	// Good idea to clear the Event Detect status register here, just in case.
	*GPEDS0 = *GPEDS0 | 0x1F0000; 
	// Disable (Async) Rising Edge detection for all 5 GPIO ports.
	*GPREN0 = *GPREN0 | 0xFFE0FFFF;

	// Remove the interrupt handler; you need to provide the same identifier
    	free_irq(79, &mydev_id);
	
	unregister_chrdev(major, CDEV_NAME);
	printk("Button Detection disabled.\n");
}
