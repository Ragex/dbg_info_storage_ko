#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/uaccess.h>


MODULE_LICENSE("GPL"); 
 
#define PROC_ENTRY_FILENAME "drv_dbg"
#define MESSAGE_LENGTH 1024

static char Message[MESSAGE_LENGTH];
char *emptyBuff = "File is empty\n";
static bool isEmpty = true;


static ssize_t
module_output(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
  int i;
  static bool finished = false;
  
  if(finished){
      finished = false;
      return 0;
  }
 
  if (isEmpty){
    for (i = 0; i < length && emptyBuff[i]; i++)
        put_user(emptyBuff[i], buffer + i);
  }
  else{
        for (i = 0; i < length && Message[i]; i++)
            put_user(Message[i], buffer + i);
  }
  
  finished = true;
  
  return i;             
}
 
 
static ssize_t
module_input(struct file *filp, const char *buffer, size_t length, loff_t * offset)
{
  int i, startData;
  static int dataEndIdx = 0;
  char *tmpBuff;
  
  if(isEmpty){
     if(length < MESSAGE_LENGTH) startData = 0;
     else                        startData = length - MESSAGE_LENGTH;
          
     for(i = startData; i < length; i++)
         get_user(Message[i-startData], buffer+i);
 
     Message[i] = '\0'; 
     dataEndIdx = i;
     
     isEmpty = false;
  }
  else{
       if((dataEndIdx + 1 + length) < MESSAGE_LENGTH){
          Message[dataEndIdx] = '\n';
          for(i = 0; i < length; i++)
              get_user(Message[dataEndIdx+1+i], buffer + i);
 
          Message[dataEndIdx+1+i] = '\0'; 
          dataEndIdx += (1+i);
       }
       else{
             tmpBuff = (char*)kmalloc(MESSAGE_LENGTH+length,GFP_ATOMIC);
             memcpy(tmpBuff, Message, sizeof(Message));
             tmpBuff[dataEndIdx] = '\n';
             
             for(i = 0; i < length; i++)
                 get_user(tmpBuff[dataEndIdx+1+i], buffer+i);
             
             memcpy(Message,tmpBuff+(sizeof(tmpBuff)-sizeof(Message)),sizeof(Message));
             
             dataEndIdx = MESSAGE_LENGTH-1;
             Message[dataEndIdx] = '\0';
             
             kfree(tmpBuff);
             
             return length;
       }           
  }
  
  return i;
}
 
 
int
module_open(struct inode *inode, struct file *file)
{
  try_module_get(THIS_MODULE);
  return 0;
}
 

int
module_close(struct inode *inode, struct file *file)
{
  module_put(THIS_MODULE);
  return 0;             // все нормально
}
 
 
static struct file_operations procFileOps = {
        .owner = THIS_MODULE,
        .read  = module_output,
        .write = module_input,
        .open  = module_open,
        .release = module_close,
};
 


int init_module()
{
  int err = 0;
 
  if (!proc_create(PROC_ENTRY_FILENAME,0666, NULL, &procFileOps)) {
    err = -ENOMEM;
    remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
    printk(KERN_INFO "Error: Could not initialize /proc/drv_dbg\n");
  }
 
  return err;
}
 
void cleanup_module()
{
  remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
}
