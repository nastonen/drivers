#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Niko Nastonen");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

#define MAXBYTES 128

/* driver context */
static struct drv_ctx {
        struct device *dev;
        int tx, rx, err, myword;
        u32 config1, config2;
        u64 config3;
        char secret[MAXBYTES];
} *ctx;

static int secret_open(struct inode *inode, struct file *filp)
{
        struct device *dev = ctx->dev;
        char *buf = kzalloc(PATH_MAX, GFP_KERNEL);

        if (unlikely(!buf))
                return -ENOMEM;

        dev_info(dev, "opening %s now, f_flags = 0x%x\n",
                        file_path(filp, buf, PATH_MAX), filp->f_flags);
        kfree(buf);

        return nonseekable_open(inode, filp);
}

static ssize_t secret_read(struct file *filp, char __user *ubuf, size_t count, loff_t *off)
{
        int ret = count;
        int secret_len = strnlen(ctx->secret, MAXBYTES);
        struct device *dev = ctx->dev;
        char tasknm[TASK_COMM_LEN];

        dev_info(dev, "%s wants to read (upto) %zu bytes\n", get_task_comm(tasknm, current), count);

        ret = -EINVAL;
        if (count < MAXBYTES) {
                dev_warn(dev, "request # of bytes (%zu) is < required size (%d), aborting read\n",
                                count, MAXBYTES);
                goto out_notok;
        }
        if (secret_len <= 0) {
                dev_warn(dev, "whoops, something's wrong, the 'secret' isn't available, aborting read\n");
                goto out_notok;
        }

        ret = -EFAULT;
        if (copy_to_user(ubuf, ctx->secret, secret_len)) {
                dev_warn(dev, "copy_to_user() failed\n");
                goto out_notok;
        }
        ret = secret_len;

        ctx->tx += secret_len;
        dev_info(dev, "%d bytes read, returning... (stats: tx=%d, rx=%d)\n",
                        secret_len, ctx->tx, ctx->rx);

out_notok:
        return ret;
}

static ssize_t secret_write(struct file *filp, const char __user *ubuf, size_t count, loff_t *off)
{
        int ret = count;
        void *kbuf = NULL;
        struct device *dev = ctx->dev;
        char tasknm[TASK_COMM_LEN];

        if (unlikely(count > MAXBYTES)) {
                dev_warn(dev, "count %zu exceeds max # of bytes allowed,\
                                aborting write\n", count);
                goto out_nomem;
        }

        dev_info(dev, "%s wants to write %zu bytes\n", get_task_comm(tasknm, current), count);

        ret = -ENOMEM;
        kbuf = kvzalloc(count, GFP_KERNEL);
        if (unlikely(!kbuf))
                goto out_nomem;

        ret = -EFAULT;
        if (copy_from_user(kbuf, ubuf, count)) {
                dev_warn(dev, "copy_from_user failed\n");
                goto out_cfu;
        }

        /* write our secret */
        strscpy(ctx->secret, kbuf, count);

        ctx->rx += count;
        ret = count;
        dev_info(dev, "%zu bytes written, returning... (stats: tx=%d, rx=%d)\n",
                        count, ctx->tx, ctx->rx);

out_cfu:
        kvfree(kbuf);
out_nomem:
        return ret;
}

static int secret_close(struct inode *inode, struct file *filp)
{
        return 0;
}

static const struct file_operations secret_fops = {
        .open = secret_open,
        .read = secret_read,
        .write = secret_write,
        .llseek = no_llseek,
        .release = secret_close
};

static struct miscdevice llkd_miscdev = {
        .minor = MISC_DYNAMIC_MINOR,    /* kernel automatically gives a minor # */
        .name = "secret",               /* /dev/secret will be created */
        .mode = 0666,                   /* permissions */
        .fops = &secret_fops            /* file operations */
};


static int __init miscdrv_secret_init(void)
{
        struct device *dev;

        int ret = misc_register(&llkd_miscdev);
        if (ret) {
                pr_notice("device registration failed\n");
                return ret;
        }

        dev = llkd_miscdev.this_device;
        ctx->dev = dev;

        dev_info(dev, "LLKD misc driver (major # 10) registered, minor = %d,\
                        dev node is /dev/%s\n", llkd_miscdev.minor, llkd_miscdev.name);

        ctx = devm_kzalloc(dev, sizeof(struct drv_ctx), GFP_KERNEL);
        if (unlikely(!ctx))
                return -ENOMEM;


        strscpy(ctx->secret, "initmsg", 8);
        dev_dbg(ctx->dev, "A sample print via the dev_dbg(): driver initialized\n");

        return 0;
}

static void __exit miscdrv_secret_exit(void)
{
        misc_deregister(&llkd_miscdev);
        dev_info(ctx->dev, "LLKD misc driver deregistered\n");
}

module_init(miscdrv_secret_init);
module_exit(miscdrv_secret_exit);

