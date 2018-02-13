#ifndef MODULE
#define MODULE		
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h> //for ioremap

MODULE_LICENSE("GPL");	//mod license



int init_module(void) {
	printk("MODULE INSTALLED\n");  //dmseg print
	
	unsigned long *GPFSEL0 = (unsigned long *) ioremap(0x3F200000, 4096); //remap GPFSEL0 to address

	*GPFSEL0 = *GPFSEL0 | 0x9240;		//mask the bits to turn bcms to output

	GPFSEL0 = (GPFSEL0 + 0x001C / 4);	//move to address to set LEDs on
		
	*GPFSEL0 = *GPFSEL0 | 0x003C;		//turn LEDs on with bit mask

	

	
	
	return 0;
}
void cleanup_module(void) {
	printk("MODULE REMOVED\n");
	unsigned long *GPFSEL0 = (unsigned long *) ioremap(0x3F200028, 4096);		//address to GPFCLR0
	*GPFSEL0 = *GPFSEL0 | 0x003C;		//turn LEDs off with bit mask
}
