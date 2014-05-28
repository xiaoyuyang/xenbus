#include<linux/module.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/miscdevice.h>
#include<asm/page_types.h>
#include<asm/xen/page.h>
#include<xen/xenbus.h>
#include<xen/grant_table.h>
#include<xen/xen.h>
#include"svm.h"

MODULE_LICENSE("Dual BSD/GPL");

struct xenbus_device dev = {
	.devicetype = "svm",
	.nodename = "svm",
};


/* alloc page
 *
 *alloc the page to grant
 */
void *vaddr = NULL;
static int svm_alloc_page(void)
{
	int ret;

	vaddr = (void *)get_zeroed_page(GFP_NOIO | __GFP_HIGH);
	if(NULL == vaddr){
		printk(KERN_ALERT "[svm]: alloc page failed!\n");
	}

	ret = xenbus_exists(XBT_NIL, "/vrv/svm", "grant_table");
	if(0 == ret){
		printk(KERN_ALERT "[svm]: can not read /vrv/svm/grant_table.\n");
		return -1;
	}
	return 0;
}

static int svm_get_domid(void)
{
	int ret;
	xenbus_scanf(XBT_NIL, "domid", "", "%d", &ret);
	xenbus_printf(XBT_NIL, "/vrv", "svm", "%d", ret);
	return ret;
}

static int svm_grant_ring(int arg)
{
	int ret;
	struct xenbus_device dev;

	dev.otherend_id = arg;

	if(NULL == vaddr){
		printk(KERN_ALERT "[svm]: wrong vaddr.\n");
		return -1;
	}

	ret = xenbus_grant_ring(&dev, virt_to_mfn(vaddr));
	if(ret < 0)
		goto grant_ring_failed;
	
	xenbus_printf(XBT_NIL, "/vrv/svm", "grant_table", "%lu", virt_to_mfn(vaddr));

	return 0;

grant_ring_failed:
	free_page((unsigned long)vaddr);

	return -1;
}


static int svm_ioctl(struct inode *inodp, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int r;
	switch(cmd){
		case SVM_GET_DOMID:
			r = svm_get_domid();
			break;
		case SVM_ALLOC_PAGE:
			r = svm_alloc_page();
		case SVM_GRANT_RING:
			r = svm_grant_ring(arg);
			break;
		default:
			printk(KERN_ALERT "[svm]: wrong cmd");
			return -EINVAL;
	}

	return r;
}


ssize_t svm_write(struct file *filp, const char __user *src, size_t len, loff_t *offp)
{
	ssize_t ret;
	static unsigned int pos = 0;


	if(NULL == vaddr){
		printk(KERN_ALERT "[svm]: vaddr is NULL, can not write!");
		return -1;
	}

	if(pos + len > PAGE_SIZE)//overflow
		pos -= PAGE_SIZE;
	ret = copy_from_user(vaddr + pos, src, len);

	ret = xenbus_printf(XBT_NIL, "/vrv/svm/grant_table", "com", "pos:%u len:%u", pos, len);
	if(0 != ret){
		printk(KERN_ALERT "[svm]: write com failed.\n");
	}
	
	pos += len;
	*offp = pos;

	return ret;
}

struct file_operations svm_ops = {
	.owner = THIS_MODULE,
	.ioctl = svm_ioctl,
	.write = svm_write,
};

struct miscdevice svm_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "svm",
	.fops = &svm_ops,
};

static int __init svm_init(void)
{
	int ret;

	ret = xenbus_exists(XBT_NIL, "/vrv/svm/grant_table", "com");
	if(0 == ret){
		printk(KERN_ALERT "[svm]: can not read /vrv/svm/grant_table/com.\n");
		return -1;
	}

	ret = misc_register(&svm_dev);
	if(ret != 0){
		printk(KERN_ALERT "[svm]: misc register failed.\n");
		return -1; //FIXME: erron?
	}

	printk(KERN_ALERT "[svm]: module installed.\n");
	return 0;
}

static void __exit svm_exit(void)
{
	misc_deregister(&svm_dev);
	printk(KERN_ALERT "[svm]: module removed.\n");
	
	return;
}

module_init(svm_init);
module_exit(svm_exit);
