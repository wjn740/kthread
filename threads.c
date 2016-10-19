#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>  // for threads
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/types.h>



static struct task_struct *thread1;
static int sum;


int thread_fn(void *t) {

unsigned long j0,j1;
int delay = 60*HZ;
j0 = jiffies;
j1 = j0 + delay;

while (time_before(jiffies, j1)){ 
if(kthread_should_stop()) {
do_exit(0);
}
  schedule();
}
printk(KERN_INFO "In thread1");
return 0;
}

void *kthread_seq_start(struct seq_file *sfile, loff_t *ops)
{
    return NULL;
}
void *kthread_seq_next(struct seq_file *sfile, void *v, loff_t *pos)
{
    return NULL;
}
void kthread_seq_stop(struct seq_file *sfile, void *v)
{
    return;
}
int kthread_seq_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "hello,world,%d", sum);
    return 0;
}

static struct seq_operations kthread_seq_ops = {
    .start = kthread_seq_start,
    .next  = kthread_seq_next,
    .stop  = kthread_seq_stop,
    .show  = kthread_seq_show
};

static int kthread_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &kthread_seq_ops);
}

struct file_operations kthread_proc_fsops = {
    .owner   = THIS_MODULE,
    .open    = kthread_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};

int thread_init (void) {
 
 char name[8]="thread1";
 struct proc_dir_entry *entry;

 entry = proc_create("apple7", 0, NULL, &kthread_proc_fsops);
 if (!entry)
     printk(KERN_WARNING "procfs interface create failed\n");


 printk(KERN_INFO "in init");
 thread1 = kthread_run(thread_fn,NULL,name);



 return 0;
}



void thread_cleanup(void) {
 int ret;
 ret = kthread_stop(thread1);
 if(!ret)
  printk(KERN_INFO "Thread stopped");

}
MODULE_LICENSE("GPL"); 
module_init(thread_init);
module_exit(thread_cleanup);

