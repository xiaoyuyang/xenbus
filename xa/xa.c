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
#include<xen/events.h>
#include<xen/grant_table.h>
#include"xa.h"

MODULE_LICENSE("Dual BSD/GPL");

struct vm_struct *area;

//watch com and read command
int command_read(unsigned pos, unsigned len)
{
	char *str = (char *)vmalloc(len + 1);
	if(NULL == str){
		printk(KERN_ALERT "[xa]: vmalloc failed.\n");
		return -1;
	}

	memcpy(str, area->addr + pos, len);
	str[len] = '\0';
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
	grant_ref_t gnt_ref;
	int domid;
	struct gnttab_map_grant_ref op;
	
	
	//get the domain id of svm
	ret = xenbus_scanf(XBT_NIL, "/vrv", "svm", "%d", &domid);
	if(ret < 0){
		printk(KERN_ALERT "[xa]:can not get the value of s_domid and gnt_ref from /vrv/svm.");
		return -1;
	}
	
	//get the grant table reference
	ret = xenbus_scanf(XBT_NIL, "/vrv/svm", "grant_table", "%u", &gnt_ref);
	if(ret < 0){
		printk(KERN_ALERT "[xa]:can not get the value of gnt_ref from /vrv/svm/grant_table.");
		return -1;
	}
	printk(KERN_ALERT "[xa]: svm domid is:%d, gnt_ref is:%u.\n", domid, gnt_ref);
	
	area = xen_alloc_vm_area(PAGE_SIZE);
	
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
	printk(KERN_ALERT "[xa]: map grant op status is %d", op.status);

	/* op.handle is needed in the unmap op
	 * Stuff the handle in an unused field
	 */
	area->phys_addr = (unsigned long)op.handle;

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

irqreturn_t evh_handler(int irq, void *dev_id)
{
	xa_map_ring();
	xa_watch();
}

int xa_set_evtchn(void)
{
	int ret, remote_domid;
	struct evtchn_alloc_unbound alloc_unbound;

	//read svm domid
	ret = xenbus_scanf(XBT_NIL, "/vrv", "svm", "%d", &remote_domid);
	//if(ret < 1)

	//alloc evtchn and call hyprecall
	alloc_unbound.dom = DOMID_SELF;
	alloc_unbound.remote_dom = remote_domid;
	ret = HYPERVISOR_event_channel_op(EVTCHNOP_alloc_unbound, &alloc_unbound);
	if(0 != ret){
		printk(KERN_ALERT "alloc unbound evtchn failed.\n");
		goto fail;
	}
	printk(KERN_ALERT "alloc unbound evtchn %d.\n", alloc_unbound.port);

	//bind evtchn to irqhandler
	ret = bind_evtchn_to_irqhandler(alloc_unbound.port, evh_handler, IRQF_SAMPLE_RANDOM, "/vrv/svm", NULL);
	if(ret < 0){
		printk(KERN_ALERT "bind evtchn to irqhandler failed.\n");
		goto fail;
	}
	return alloc_unbound.port;

fail:
	return -1;
}

//xa_ioctl
static int xa_ioctl(struct inode *inodp, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int r;
	switch(cmd){
		case XA_DOMAIN_NUMBER:
			r = xa_domain_id();
			break; 
		case XA_SET_EVTCHN:
			r = xa_set_evtchn();
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

int xa_unmap_ring(void)
{
	struct gnttab_unmap_grant_ref op = {
		.host_addr = (unsigned long)area->addr,
		.handle = (grant_handle_t)area->phys_addr,
	};
	
	HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref, &op, 1);
	if (op.status == GNTST_okay)
		xen_free_vm_area(area);
	else
		printk(KERN_ALERT "can not unmap the page.\n");
	
	return op.status;
}

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

	//check the right to access xenstore
	ret = xenbus_exists(XBT_NIL, "/vrv/svm/grant_table", "com");
	if(0 == ret){
		printk(KERN_ALERT "[xa]: do not have the right of xenstore!\n");
		return -1;
	}

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
	xa_unmap_ring();
	unregister_xenstore_notifier(&xenstore_notifier);
	misc_deregister(&xa_dev);
	printk(KERN_ALERT "[xa]: module removed.\n");
	
	return;
}

module_init(xa_init);
module_exit(xa_exit);
