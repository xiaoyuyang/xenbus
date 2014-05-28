/* get domain info from a domU guest os
 *
 * “svm” stand for security virtual machine 
 */
#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/notifier.h>
#include<linux/errno.h>
#include<linux/miscdevice.h>
#include<linux/vmalloc.h>
#include<asm/xen/hypercall.h>
#include<xen/xenbus.h>
#include<xen/xen.h>
#include<xen/grant_table.h>
#include"xa.h"

MODULE_LICENSE("Dual BSD/GPL");
void *vaddr;

//watch com and read command
int command_read(unsigned pos, unsigned len)
{
	char *str = (char *)vmalloc(len + 1);
	if(NULL == str){
		printk(KERN_ALERT "[xa]: vmalloc failed.\n");
		return -1;
	}

	memcpy(str, vaddr + pos, len);
	str[len] = '\n';
	printk(KERN_ALERT "[xa]: command is %s.\n", str);
	vfree(str);
	
	return 0;
}

static void command_watch(struct xenbus_watch *watch, const char **vec, unsigned int len)
{
	unsigned int pos;
	int ret;

	ret = xenbus_scanf(XBT_NIL, "/vrv/svm/grant_table", "com", "pos:%u len:%u", &pos, &len);
	if(ret < 0){
		return ;
	}

	printk(KERN_ALERT "[xa]: new command is pos:%u, len%u.\n", pos, len);

	if(0 != len)
		ret = command_read(pos, len);
	
	return ;
}

static struct xenbus_watch target_watch = {
	.node = "/vrv/svm/grant_table/com",
	.callback = command_watch,
};

static int xa_init_watcher(struct notifier_block * notifier, unsigned long event, void * data)
{
	int ret;

	ret = register_xenbus_watch(&target_watch);
	if(ret){
		printk(KERN_ERR "[xa]: falied to register xenstore watcher.\n");
	}

	return NOTIFY_DONE;
}

static struct notifier_block xenstore_notifier = {
	.notifier_call = xa_init_watcher,
};

static int xa_watch(void)
{
	//register the notifier	
	register_xenstore_notifier(&xenstore_notifier);
	
	return 0;

}


/* xa_map_ring
 *
 * read domid and gnt_ref
 * map the page shared by svm to current dom
 */

static int xa_map_ring(void)
{
	int ret;
	int gnt_ref;
	int domid;
	struct gnttab_map_grant_ref op;
	struct vm_struct *area;
	
	ret = xenbus_exists(XBT_NIL, "/vrv/svm/grant_table", "com");
	if(0 == ret){
		printk(KERN_ALERT "[xa]: can not read /vrv/svm/grant_table!\n");
		return -1;
	}
	
	//get the domain id of svm
	ret = xenbus_scanf(XBT_NIL, "/vrv", "svm", "%d", &domid);
	if(ret < 0){
		printk(KERN_ALERT "[xa]:can not get the value of s_domid and gnt_ref from /vrv/svm.");
		return -1;
	}
	
	//get the grant table reference
	ret = xenbus_scanf(XBT_NIL, "/vrv/svm", "grant_table", "%d", &gnt_ref);
	if(ret < 0){
		printk(KERN_ALERT "[xa]:can not get the value of gnt_ref from /vrv/svm/grant_table.");
		return -1;
	}

	area = xen_alloc_vm_area(PAGE_SIZE);
	vaddr = area->addr;
	
	//map
	op.host_addr = (unsigned long)area->addr;
	op.flags     = GNTMAP_host_map;
	op.ref       = gnt_ref;
	op.dom       = domid;


	if (HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1))
        BUG();

	if (op.status != GNTST_okay) {
		printk(KERN_ALERT "[xa]: map grant table failed! status is %d", op.status);
	}	
	
	return op.status;
}


/* dom ID
 *
 * get the id of current dom
 * called by ioctl
 */

static int xa_domain_id(void)
{
	int domid, ret;

	ret = xenbus_scanf(XBT_NIL, "domid", "", "%d", &domid);
	if(ret < 0){
		printk(KERN_ALERT "[xa]:can not read the domid.\n");
		return -1;
	}
	printk(KERN_ALERT "[xa]:domid is %d.\n", domid);
	
	return domid;

}

//xa_ioctl
static int xa_ioctl(struct inode *inodp, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int r;
	switch(cmd){
		case XA_DOMAIN_NUMBER:
			r = xa_domain_id();
			break; 
		case XA_MAP_RING:
			r = xa_map_ring();
			break;
		case XA_WATCH:
			r = xa_watch();
			break;
		default:
			printk(KERN_ALERT "[xa]: unrecognized cmd!\n");
			r = -EINVAL;
	}

	return r;
}



//module init and exit
static struct file_operations xa_ops = {
	.owner = THIS_MODULE,
	.ioctl = xa_ioctl,
};

struct miscdevice xa_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.fops  = &xa_ops,
	.name  = "xa",
};

static int __init xa_init(void)
{
	int ret;
	
	ret = misc_register(&xa_dev);
	if(ret != 0){
		printk(KERN_ALERT "[xa]: misc register failed.\n");
		return -1; //FIXME: erron?
	}

	printk(KERN_ALERT "[xa]: module installed.\n");
	
	return 0;
	
}

static void __exit xa_exit(void)
{
	unregister_xenstore_notifier(&xenstore_notifier);

	misc_deregister(&xa_dev);
	printk(KERN_ALERT "[xa]: module removed.\n");
	
	return;
}

module_init(xa_init);
module_exit(xa_exit);
