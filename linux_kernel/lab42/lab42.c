#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/parser.h>
#include <linux/smp_lock.h>
#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/vfs.h>
#include <linux/random.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/quotaops.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("support@ingben.com");
#define INGBENFS_MAGIC 0x19980122
static DEFINE_RWLOCK(file_systems_lock);


/*
 * * Anytime we make a file or directory in our filesystem we need to
 * * come up with an inode to represent it internally.  This is
 * * the function that does that job.  All that's really interesting
 * * is the "mode" parameter, which says whether this is a directory
 * * or file, and gives the permissions.
 * */
static struct inode *ingbenfs_make_inode(struct super_block *sb, int mode)
{
        struct inode *ret = new_inode(sb);
        if (ret) {
                ret->i_mode = mode;
                ret->i_uid = ret->i_gid = 0;
                //ret->i_blksize = PAGE_CACHE_SIZE;
                ret->i_blocks = 0;
                ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;
        }
        return ret;
}
/*
 * * The operations on our "files".
 * */
/*
 * * Open a file.  All we have to do here is to copy over a
 * * copy of the counter pointer so it's easier to get at.
 * */
static int ingbenfs_open(struct inode *inode, struct file *filp)
{
       // filp->private_data = inode->u.generic_ip;
        filp->private_data = inode->i_private;
        return 0;
}
#define TMPSIZE 20
/*
 * * Read a file.  Here we increment and read the counter, then pass it
 * * back to the caller.  The increment only happens if the read is done
 * * at the beginning of the file (offset = 0); otherwise we end up counting
 * * by twos.
 * */
static ssize_t ingbenfs_read_file(struct file *filp, char *buf,
                size_t count, loff_t *offset)
{
        atomic_t *counter = (atomic_t *) filp->private_data;
        int v, len;
        char tmp[TMPSIZE];
/*
 * * Encode the value, and figure out how much of it we can pass back.
 * */
        v = atomic_read(counter);
        if (*offset > 0)
                v -= 1;  /* the value returned when offset was zero */
        else
                atomic_inc(counter);
        len = snprintf(tmp, TMPSIZE, "%d\n", v);
        if (*offset > len)
                return 0;
        if (count > len - *offset)
                count = len - *offset;
/*
 * * Copy it back, increment the offset, and we're done.
 * */
        if (copy_to_user(buf, tmp + *offset, count))
                return -EFAULT;
        *offset += count;
        return count;
}
/*
 * * Write a file.
 * */
static ssize_t ingbenfs_write_file(struct file *filp, const char *buf,
                size_t count, loff_t *offset)
{
        atomic_t *counter = (atomic_t *) filp->private_data;
        char tmp[TMPSIZE];
/*
 * * Only write from the beginning.
 * */
        if (*offset != 0)
                return -EINVAL;
/*
 * * Read the value from the user.
 * */
        if (count >= TMPSIZE)
                return -EINVAL;
        memset(tmp, 0, TMPSIZE);
        if (copy_from_user(tmp, buf, count))
                return -EFAULT;
/*
 * * Store it in the counter and we are done.
 * */
        atomic_set(counter, simple_strtol(tmp, NULL, 10));
        return count;
}
/*
 * * Now we can put together our file operations structure.
 * */
static struct file_operations ingbenfs_file_ops = {
        .open        = ingbenfs_open,
        .read         = ingbenfs_read_file,
        .write  = ingbenfs_write_file,
};
/*
 * * Create a file mapping a name to a counter.
 * */
static struct dentry *ingbenfs_create_file (struct super_block *sb,
                struct dentry *dir, const char *name,
                atomic_t *counter)
{
        struct dentry *dentry;
        struct inode *inode;
        struct qstr qname;
/*
 * * Make a hashed version of the name to go with the dentry.
 * */
        qname.name = name;
        qname.len = strlen (name);
        qname.hash = full_name_hash(name, qname.len);
/*
 * * Now we can create our dentry and the inode to go with it.
 * */
        dentry = d_alloc(dir, &qname);
        if (! dentry)
                goto out;
        inode = ingbenfs_make_inode(sb, S_IFREG | 0644);
        if (! inode)
                goto out_dput;
        inode->i_fop = &ingbenfs_file_ops;
        //inode->u.generic_ip = counter;
        inode->i_private = counter;
/*
 * * Put it all into the dentry cache and we're done.
 * */
        d_add(dentry, inode);
        return dentry;
/*
 * * Then again, maybe it didn't work.
 * */
  out_dput:
        dput(dentry);
  out:
        return 0;
}
/*
 * * Create a directory which can be used to hold files.  This code is
 * * almost identical to the "create file" logic, except that we create
 * * the inode with a different mode, and use the libfs "simple" operations.
 * */
static struct dentry *ingbenfs_create_dir (struct super_block *sb,
                struct dentry *parent, const char *name)
{
        struct dentry *dentry;
        struct inode *inode;
        struct qstr qname;
        qname.name = name;
        qname.len = strlen (name);
        qname.hash = full_name_hash(name, qname.len);
        dentry = d_alloc(parent, &qname);
        if (! dentry)
                goto out;
        inode = ingbenfs_make_inode(sb, S_IFDIR | 0644);
        if (! inode)
                goto out_dput;
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;
        d_add(dentry, inode);
        return dentry;
  out_dput:
        dput(dentry);
  out:
        return 0;
}
/*
 * * OK, create the files that we export.
 * */
static atomic_t counter, subcounter;
static void ingbenfs_create_files (struct super_block *sb, struct dentry *root)
{
        struct dentry *subdir;
/*
 * * One counter in the top-level directory.
 * */
        atomic_set(&counter, 0);
        ingbenfs_create_file(sb, root, "counter", &counter);
/*
 * * And one in a subdirectory.
 * */
        atomic_set(&subcounter, 0);
        subdir = ingbenfs_create_dir(sb, root, "subdir");
        if (subdir)
                ingbenfs_create_file(sb, subdir, "subcounter", &subcounter);
}
/*
 * * Superblock stuff.  This is all boilerplate to give the vfs something
 * * that looks like a filesystem to work with.
 * */
/*
 * * Our superblock operations, both of which are generic kernel ops
 * * that we don't have to write ourselves.
 * */
static struct super_operations ingbenfs_s_ops = {
        .statfs                = simple_statfs,
        .drop_inode        = generic_delete_inode,
};



/*
 * * "Fill" a superblock with mundane stuff.
 * */
static int ingbenfs_fill_super (struct super_block *sb, void *data, int silent)
{
        struct inode *root;
        struct dentry *root_dentry;
/*
 * * Basic parameters.
 * */
        sb->s_blocksize = PAGE_CACHE_SIZE;
        sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
        sb->s_magic = INGBENFS_MAGIC;
        sb->s_op = &ingbenfs_s_ops;
/*
 * * We need to conjure up an inode to represent the root directory
 * * of this filesystem.  Its operations all come from libfs, so we
 * * don't have to mess with actually *doing* things inside this
 * * directory.
 * */
	printk(KERN_INFO "ingbenfs_fill_super is here\n");
        root = ingbenfs_make_inode (sb, S_IFDIR | 0755);
        if (! root)
                goto out;
        root->i_op = &simple_dir_inode_operations;
        root->i_fop = &simple_dir_operations;
/*
 * * Get a dentry to represent the directory in core.
 * */
        root_dentry = d_alloc_root(root);
        if (! root_dentry)
                goto out_iput;
        sb->s_root = root_dentry;
/*
 * * Make up the files which will be in this filesystem, and we're done.
 * */
        ingbenfs_create_files (sb, root_dentry);
        return 0;
        
  out_iput:
        iput(root);
  out:
        return -ENOMEM;
}


//static struct super_block *ingbenfs_get_super(struct file_system_type *fst,int flags, const char *devname, void *data)
static int ingbenfs_get_super(struct file_system_type *fst,int flags, const char *devname, void *data,struct vfsmount *mount)
{
	printk(KERN_INFO "mount from user\n");
        return get_sb_single(fst, flags, data, ingbenfs_fill_super,mount);
}


static struct file_system_type ingbenfs_type = {
        .owner                 = THIS_MODULE,
        .name                = "ingbenfs",
        .get_sb                = ingbenfs_get_super,
        .kill_sb        = kill_litter_super,
};


static int __init ingbenfs_init(void)
{
	 struct file_system_type * tmp;
/*	 write_lock(&file_systems_lock);
         tmp = &file_systems;
         while (*tmp) {
         //        if (fs == *tmp) {
           //             *tmp = fs->next;
             //           fs->next = NULL;
               //          write_unlock(&file_systems_lock);
                 //        return 0;
                // }
                 tmp = &(*tmp)->next;
         }
         write_unlock(&file_systems_lock);
*/
	tmp = get_fs_type("ext4");
        printk("filesystem name found = %s\n",tmp->name);

        
	printk("ingbenfs_init ok\n");
        return register_filesystem(&ingbenfs_type);
}
static void __exit ingbenfs_exit(void)
{
        unregister_filesystem(&ingbenfs_type);
	printk("ingbenfs_exit ok\n");
}
module_init(ingbenfs_init);
module_exit(ingbenfs_exit);


