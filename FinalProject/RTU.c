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
#define CDEV_NAME "Final" 

MODULE_LICENSE("GPL");

//static struct task_struct *kthread1;
static int major;
static char msg[MSG_SIZE];
int mydev_id; //char device id
unsigned long *start;
unsigned long *GPFSEL0; //speaker
unsigned long *GPFSEL1; //buttons
unsigned long *GPFSET; //turn speaker on
unsigned long *GPFCLR; //clear event detect
unsigned long *GPEDS0; //event detect register
unsigned long *GPREN0; //rise edge detect
unsigned long *GPPUD; //pull down register
unsigned long *GPPUDClk0; //pull down register clock



int mydev_id;	// variable needed to identify the handler


int my_kthread(void *ptr) {
	while(1) {
		msleep(250);
		printk("%s\n", msg);
	}
	return 0;
}
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) //when user reads char device
{
	// Whatever is in msg will be placed into buffer, which will be copied into user space
	ssize_t dummy;
	if(msg[0] != '\0') {
		dummy = copy_to_user(buffer, msg, length);
	}	
	// msg should be protected (e.g. semaphore). Not implemented here, but you can add it.
	msg[0] = '\0';	// "Clear" the message, in case the device is read again.
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
	dummy = copy_from_user(msg, buff, len);	// Transfers the data from user space to kernel space
	if(len == MSG_SIZE)
		msg[len-1] = '\0';	// will ignore the last character received.
	else
		msg[len] = '\0';
	//start checking for the values
	return len;		// the number of bytes that were written to the Character Device.
}
static irqreturn_t button_isr(int irq, void *dev_id)
{
	// In general, you want to disable the interrupt while handling it.
	disable_irq_nosync(79);
	if(*GPEDS0 == 0x1000) {
		sprintf(msg, "S1");
	} else if (*GPEDS0 == 0x2000){
		sprintf(msg, "S2");
	} else if (*GPEDS0 == 0x10000){
		sprintf(msg, "B1");
	} else if (*GPEDS0 == 0x20000) {
		sprintf(msg, "B2");
	}
	*GPEDS0 = *GPEDS0 | 0x00033000;		
	enable_irq(79);		// re-enable interrupt
    return IRQ_HANDLED;
}
static struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
};
int init_module()
{
	
	int dummy = 0;
//	char kthread[] = "my_thread";
	start = (unsigned long *) ioremap(0x3F200000, 4096);
	GPFSEL0 = start;
	GPFSEL1 = start + 1;	//buttons and switches to input
	GPFSET = (start + 0x001C / 4);
	GPFCLR = (start + 0x0028 / 4);	//used for LEDs
	
	*GPFSEL0 = *GPFSEL0 | 0x9240;
	*GPFSEL1 = *GPFSEL1 & 0xFF03F03F;


	GPEDS0 = (start + 0x0040 / 4); //event detection
	GPREN0 = (start + 0x004C / 4); //rising edge detect (sync)
	GPPUD = (start + 0x0094 / 4); //pull down control
	GPPUDClk0 = (start + 0x0098 / 4);
	
	// Don't forget to configure all ports connected to the push buttons as inputs.
	// You need to configure the pull-downs for all those ports. There is
	// a specific sequence that needs to be followed to configure those pull-downs.
	// The sequence is described on page 101 of the BCM2835-ARM-Peripherals manual.
	// You can use  udelay(100);  for those 150 cycles mentioned in the manual.
	// It's not exactly 150 cycles, but it gives the necessary delay.
	// WiringPi makes it a lot simpler in user space, but it's good to know
	// how to do this "by hand".
	*GPPUD = *GPPUD | 0x1;
	udelay(100);
	*GPPUDClk0 = *GPPUDClk0 | 0x00033000;
	udelay(100);
	*GPPUD = *GPPUD & 0x0;
	*GPPUDClk0 = *GPPUDClk0 & 0xFFFCCFFF;
	
	
	// Enable (Async) Rising Edge detection for all 5 GPIO ports.	
	*GPREN0 = *GPREN0 | 0x00033000;


/*
	kthread1 = kthread_create(my_kthread, NULL, kthread);
	if(kthread1){	// true if kthread creation is successful
		// kthread is dormant after creation. Needs to be woken up
        	wake_up_process(kthread1);
    	}
*/	
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
     		printk("Registering the character device failed with %d\n", major);	//register and setup major for char dev
	     	return major;
	}
	printk("Assigned major: %d\n", major);
	dummy = request_irq(79, button_isr, IRQF_SHARED, "Button_handler", &mydev_id);	//attach handler

	printk("Button Detection enabled.\n");
	return 0;
}

// Cleanup - undo whatever init_module did
void cleanup_module()
{
//	kthread_stop(kthread1);
	*GPFCLR = *GPFCLR | 0x003C;
	// Good idea to clear the Event Detect status register here, just in case.
	*GPEDS0 = *GPEDS0 | 0x00033000; 
	// Disable (Async) Rising Edge detection for all 5 GPIO ports.
	*GPREN0 = *GPREN0 | 0xFFFCCFFF;

	// Remove the interrupt handler; you need to provide the same identifier
    	free_irq(79, &mydev_id);
	unregister_chrdev(major, CDEV_NAME);
	printk("Button Detection disabled.\n");
}
