#ifndef MODULE
#define MODULE
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");



int init_module(void) {
	printk("MODULE INSTALLED");
	
	unsigned long *GPFSEL0 = (unsigned long *) ioremap(0x3F200000, 4096);

	*GPFSEL0 = *GPFSEL0 | 0x9240;

	GPFSEL0 = (GPFSEL0 + 0x001C / 4);
	
	*GPFSEL0 = *GPFSEL0 | 0x003C;

	

	
	
	return 0;
}
void cleanup_module(void) {
	printk("MODULE REMOVED");
	unsigned long *GPFSEL0 = (unsigned long *) ioremap(0x3F200028, 4096);
	*GPFSEL0 = *GPFSEL0 | 0x003C;
}
