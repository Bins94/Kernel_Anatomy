/* The code modify from https://raw.githubusercontent.com/ilammy/ftrace-hook/master/ftrace_hook.c */
#include <linux/ftrace.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


#define HOOK_KF_NAME "vfs_open"

static unsigned long ft_address;
static struct ftrace_ops ft_ops;

static int (*real_vfs_open)(const struct path *path, struct file *file, const struct cred *cred);

static notrace int fh_vfs_open(const struct path *path, struct file *file, const struct cred *cred)
{
  int ret;

  real_vfs_open = (void*)(ft_address + MCOUNT_INSN_SIZE);
  printk("Hello vfs_open\n");
  ret = real_vfs_open(path, file, cred);
  return ret;
}

static notrace noinline void fh_callback(unsigned long ip, unsigned long parent_ip, struct ftrace_ops *ops, struct pt_regs *regs)
{
  regs->ip = (unsigned long)fh_vfs_open;
}

static int __init fh_init(void)
{
  int err;

  ft_address = kallsyms_lookup_name(HOOK_KF_NAME);
  if (!ft_address) {
    pr_debug("Symbol is not exit!\n");
    return -ENOENT;
  }

  ft_ops.func = fh_callback;
  ft_ops.flags = FTRACE_OPS_FL_SAVE_REGS
    | FTRACE_OPS_FL_RECURSION_SAFE
    | FTRACE_OPS_FL_IPMODIFY;

  err = ftrace_set_filter_ip(&ft_ops, ft_address, 0, 0);
  if (err) {
    pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
    return err;
  }

  err = register_ftrace_function(&ft_ops);
  if (err) {
    pr_debug("register_ftrace_function() failed: %d\n", err);
    ftrace_set_filter_ip(&ft_ops, ft_address, 1, 0);
    return err;
  }
  printk("ftrace load\n");
  return 0;
}

static void fh_exit(void)
{
  int err;
  err = unregister_ftrace_function(&ft_ops);
  if (err) {
    pr_debug("unregister_ftrace_function() failed: %d\n", err);
  }

  err = ftrace_set_filter_ip(&ft_ops, ft_address, 1, 0);
  if (err) {
    pr_debug("ftrace_set_filter_ip() failed: %d\n", err);
  }
  printk("ftrace remove\n");
}

module_init(fh_init);
module_exit(fh_exit);
MODULE_DESCRIPTION("Ftrace kernel function hook");
MODULE_LICENSE("GPL");
