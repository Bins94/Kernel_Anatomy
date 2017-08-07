# Touch kernel page-guard  
After kernel v4.9, a security feature has been add to upstream which called CONFIG_VMAP_STACK( Grsec/Pax implemented for a long time, call GRSECURITY_KSTACKOVERFLOW).
VMAP will insert guard page between vmap page after vmalloc. Guard page access will invoke a trap. I try to touch off this trap. The other is touch off a trap with a illegal address.

## Page guard  
```
static ssize_t proc_write (struct file *proc_file, const char __user *proc_user, size_t n, loff_t *loff)
{
    unsigned long stack_end ,stack_start, p, a;

    stack_start = (unsigned long)task_stack_page(current);
    stack_end = stack_start + THREAD_SIZE ;

    printk("Process kernel stack start at:%p, end at %p\n", stack_start, stack_end);
    p = stack_start - PAGE_SIZE + sizeof(unsigned long);
    *(unsigned long *)p = 100;
    dump_stack();

    return 0;
}
```
`p = stack_start - PAGE_SIZE + sizeof(unsigned long);`  
P point to the page guard near the start of stack( low memory).The access of this address will invoke a trap like:
```  
# echo habdhb > /proc/test
# dmesg
...
[   73.732229] PROC_DEV:into open!
[   73.732249] Process kernel stack start at:ffffc90000e54000, end at ffffc90000e58000
[   73.732254] BUG: stack guard page was hit at ffffc90000e53008 (stack is ffffc90000e54000..ffffc90000e57fff)
[   73.732258] kernel stack overflow (page fault): 0000 [#2] SMP
...
```  
The log com