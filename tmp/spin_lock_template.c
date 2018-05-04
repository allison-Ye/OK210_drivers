int xxx_count = 0;/*定义文件打开次数计数*/

static int xxx_open(struct inode *inode, struct file *filp)
{
...
spinlock(&xxx_lock);
if (xxx_count)/*已经打开*/
{
spin_unlock(&xxx_lock);
 return - EBUSY;
 }
 xxx_count++;/*增加使用计数*/
 spin_unlock(&xxx_lock);
 ...
 return 0; /* 成功 */
 }

 static int xxx_release(struct inode *inode, struct file *filp)
 {
 ...
 spinlock(&xxx_lock);
 xxx_count--; /*减少使用计数*/
 spin_unlock(&xxx_lock);

 return 0;
 }