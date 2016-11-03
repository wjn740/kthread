#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>  // for threads
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <uapi/linux/in.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <net/sock.h>


#define DEFAULT_PORT 2325
#define WORKER_NAME kserver_worker


static struct task_struct *thread1;
static int sum;

struct kthread_server {
    struct socket *listen_socket;
    struct task_struct *worker;
}kserver;

int kthread_server_fn(void *t)
{
    int error;
    struct socket *socket;
    struct sockaddr_in sin, sin_send;
    struct socket workersocket;
    struct socket *pworkersocket = &workersocket;
    char buf[10] = {0};
    int len = 10;
    struct kvec iov = {buf, len};
    struct msghdr msg = { .msg_flags = MSG_NOSIGNAL};

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &kserver.listen_socket);
    if (error < 0) {
        printk(KERN_WARNING "socket create failed. Go Dead!\n");
        do_exit(0);
    }

    socket = kserver.listen_socket;
    kserver.listen_socket->sk->sk_reuse = 1;
    
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(DEFAULT_PORT);

    error = socket->ops->bind(socket, (struct sockaddr*)&sin, sizeof(sin));
    if (error < 0) {
        printk(KERN_ERR "Bind address failed\n");
        do_exit(1);
    }

    error = socket->ops->listen(socket, 5);
    if (error < 0) {
        printk(KERN_ERR "listen failed\n");
        do_exit(1);
    }

    //kserver.worker = kthread_run(kthread_worker_fn, NULL, WORKER_NAME);


        printk(KERN_INFO "create the worker socket.\n");
        error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &pworkersocket);
        if (error < 0) {
            printk(KERN_ERR "create workersocket failed\n");
            do_exit(1);
        }
    
    while(!kthread_should_stop()) {
        error = kernel_accept(socket, &pworkersocket, O_NONBLOCK);
	if (error == -EAGAIN) {
	    set_current_state(TASK_RUNNING);
	    schedule();
	    continue;
	}
        if (error < 0) {
            printk(KERN_ERR "accept failed, %d.\n", error);
            sock_release(pworkersocket);
            do_exit(1);
        }
        printk(KERN_INFO "client is online.\n");
	memset(buf,0,10);
        error = kernel_recvmsg(pworkersocket, &msg, &iov, 1, len-1, msg.msg_flags);
        if (error < 0) {
            printk(KERN_ERR "recvmsg failed. %d\n", error);
            sock_release(pworkersocket);
            do_exit(1);
        }
	if (buf[error-1] == '\n') {
        	printk(KERN_INFO "got it %#x.\n",buf[error-1]);
        	printk(KERN_INFO "%x,%x,%x,%x,%x,%x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	}
	*(short int *)&buf[error-2] = 0x0;
        printk(KERN_INFO "recv msg: %s.\n", buf);
        printk(KERN_INFO "%x,%x,%x,%x,%x,%x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	kernel_sock_shutdown(pworkersocket, SHUT_RDWR);
	set_current_state(TASK_RUNNING);
	schedule();
        sum++;
    }
    do_exit(0);
    return 0;
}

void *kthread_seq_start(struct seq_file *sfile, loff_t *pos)
{
    if (*pos == 0) {
        return sfile;
    }	
    return NULL;
}
void *kthread_seq_next(struct seq_file *sfile, void *v, loff_t *pos)
{
    ++(*pos);
    return NULL;
}
void kthread_seq_stop(struct seq_file *sfile, void *v)
{
    return;
}
int kthread_seq_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "the sum is %d\n", sum);
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

struct proc_dir_entry *entry;

int thread_init (void) {

    char name[20]={"Kthread_server"};

    entry = proc_create("apple7", 0, NULL, &kthread_proc_fsops);
    if (!entry)
        printk(KERN_WARNING "procfs interface create failed\n");

    printk(KERN_INFO "in init");
    thread1 = kthread_run(kthread_server_fn,NULL,name);

    return 0;
}



void thread_cleanup(void) {
    int ret;
    ret = kthread_stop(thread1);
    proc_remove(entry);
    kernel_sock_shutdown(kserver.listen_socket, SHUT_RDWR);
    if(!ret)
        printk(KERN_INFO "Thread stopped\n");

}
MODULE_LICENSE("GPL"); 
module_init(thread_init);
module_exit(thread_cleanup);

