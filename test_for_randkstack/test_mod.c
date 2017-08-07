/*
 * This proc module print the stack address which is randomized by PaX Team *
 * when CONFIG_PAX_RANDKSTACK is on. PaX randomize process' kernel stack by *
 * xor some bit of thread.sp0. It will be load when context switch or system*
 * occur. In this module, write/read operation will print the stack base add*
 * On 64-bit machine, we can found 4-bit(bit4-7) random.                    */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/ptrace.h>
#include <asm/thread_info.h>

#define MY_DEV_NAME "test"
#define DEBUG_FLAG "PROC_DEV"

extern unsigned long proc_test_sp_print;
static ssize_t proc_read (struct file *proc_file, char __user *proc_user, size_t n, loff_t *loff);
static ssize_t proc_write (struct file *proc_file, const char __user *proc_user, size_t n, loff_t *loff);
static int proc_open (struct inode *proc_inode, struct file *proc_file);
static struct file_operations a = {
                                .open = proc_open,
                                .read = proc_read,
                                .write = proc_write,
};


static int __init mod_init(void)
{
    struct proc_dir_entry *test_entry;
    const struct file_operations *proc_fops = &a;
    printk(DEBUG_FLAG":proc init start\n");

    test_entry = proc_create(MY_DEV_NAME, S_IRUGO|S_IWUGO, NULL, proc_fops);
    if(!test_entry)
       printk(DEBUG_FLAG":there is somethings wrong!\n");

    printk(DEBUG_FLAG":proc init over!\n");
    return 0;
}

static ssize_t proc_read (struct file *proc_file, char __user *proc_user, size_t n, loff_t *loff)
{
    unsigned long p = &p;
    printk("%s:thread.sp0: %p, p: %p\n", "PROC", current->thread.sp0, (void *)p);
    return 0;
}


static ssize_t proc_write (struct file *proc_file, const char __user *proc_user, size_t n, loff_t *loff)
{
    /* This is a simple way to get the current stack point */
    unsigned long p = &p;
    /* thread.sp0 is the base stack address( hight memory).
       It randomized by PaX                                */
    printk("%s:thread.sp0: %p, p: %p\n", "PROC", current->thread.sp0, (void *)p);
    return 0;
}

int proc_open (struct inode *proc_inode, struct file *proc_file)
{
    printk(DEBUG_FLAG":into open, cmdline:%s!\n", current->comm);
    printk("%s:thread.sp0: %p, task->stack: %p\n", "PROC", current->thread.sp0, current->stack + THREAD_SIZE);
    return 0;
}

module_init(mod_init);
