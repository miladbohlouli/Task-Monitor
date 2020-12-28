#include <linux/init.h>   	
#include <linux/kernel.h>  	
#include <linux/module.h>  	
#include <linux/sched.h>   	
#include <linux/seq_file.h> 
#include <linux/rwlock.h>  	
#include <linux/proc_fs.h>  
#include <linux/types.h>
#include <linux/slab.h>


#define MODULE_NAME	"92522051-92521204"
#define Debug 1

/*********************************************************************
*
*	Struct I have created to collect all desired data in it.
*
*********************************************************************/

struct my_task_struct{
	long pid;
	long uid;
	u64 start_time;
	char* name;
	long size;
	long nice;
};

/********************************************************************
*	a list to collect all the processs' data, and finally 
*	sort them using bubble sort.
*
*
*********************************************************************/


struct my_task_struct *my_list[400];
int list_size;

/********************************************************************
*
*	For code readablity, I have seprated some functions to
*	generate the requred data and I use these functions in
*	Show function.
*
*********************************************************************/

char *get_proc_name(struct task_struct *current_task)
{
	return current_task->comm;
}

pid_t get_proc_pid(struct task_struct *current_task)
{
	return current_task->pid;
}

uid_t get_proc_uid(struct task_struct *current_task)
{
	uid_t cred_proc_uid = current_task->cred->uid.val; 
	return cred_proc_uid;
}

u64 get_proc_start_time(struct task_struct *current_task)
{
	return current_task->start_time; 
}

long get_proc_nice(struct task_struct *current_task)
{
	return (current_task)->static_prio - 120;	//This is the equation between niceness
}

/****************************************************************
*	This is the function to compare two strings, having
*	the maximum size of max_size.
*
*	char* string1:First String
*	char* string2:Second String
*	int size: Size
*
*	returns:
*	return<0 -> the first string is smaller
*	return>0 -> the second stirng is greater
*
*	to_lower(): gets a character data type and converts it
*	lower case.
*
*	returns : (char) the lower form of the input
*
******************************************************************/

static char to_lower(char c){
	int ascii;
	ascii = (int) c;
	if(ascii <= 90 && ascii >=41)
		c = 'a' + (c - 'A');
	return c;
}


int String_compare(const char* string1, const char* string2, int max_size)
{
	unsigned char c1, c2;
	while(max_size--)
	{
		c1 = *string1;
		c2 = *string2;
		
		string1++;
		string2++;

		if(!c1)
			break;
		if(!c2)
			break;
		if(c1 == c2)
			continue;

		c1 = to_lower(c1);
		c2 = to_lower(c2);

		if(c1 != c2)
			break;
	}
	return (int)c1 - (int)c2;
}


/******************************************************************************
*
*	This is the part of the code in witch all the data that have 
*	to be written in the /proc file, are written to the file. Note
*	that the data are written using seq_printf() function.
*	Actually in this section we set this function to handle the file
*	when it is opened.
*	
*******************************************************************************/

static int my_show(struct seq_file *m, void *v) {  
	struct my_task_struct *my_task;
	struct task_struct *task;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	long vma_start;
	long vma_end;
	long size;
	struct my_task_struct *temp;
	int i,j;
	
	list_size = 0;

	if(Debug)
		printk(KERN_INFO"Inside Show function.");	


	for_each_process(task)
	{
		size = 0;
		if(Debug)
			printk(KERN_INFO"The process pid: %ld", (long)get_proc_pid(task));
		my_task = kmalloc(sizeof(struct my_task_struct), GFP_KERNEL);
	  	
		my_task->nice = get_proc_nice(task);
	  	my_task->uid = (long)get_proc_uid(task);
	  	my_task->pid = (long)get_proc_pid(task);
	  	my_task->start_time = get_proc_start_time(task);
		my_task->name = get_proc_name(task);
	

		if ((mm = task->mm) != NULL) {
	        	vma = mm->mmap;
	        	while (vma != NULL) {

	        		vma_start = vma->vm_start;
			        vma_end = vma->vm_end;
				size += (vma_end - vma_start);
			        vma = vma->vm_next;
			}
		}
		my_task->size = size;
		my_list[list_size++] = my_task;
	}

	// Bubble Sort
	for(i = 0; i < list_size; i++){
		for(j = 1; j < list_size - i; j++){
			if(String_compare(my_list[j-1]->name, my_list[j]->name, 16)>0)
			{
				temp = my_list[j-1];
				my_list[j-1] = my_list[j];
				my_list[j] = temp;
			}
		}
	}


	seq_printf(m, "%-10s%-20s%-5s%-20s%-10s%-20s\n", "pid", "program_name", "nice", "start_time", "uid", "vm_size");

	for(i = 0; i < list_size; i++)
	{
		seq_printf(m,"%-10ld%-20s%-5ld%-20llu%-10ld%-20ld\n", my_list[i]->pid, my_list[i]->name, my_list[i]->nice, my_list[i]->start_time, my_list[i]->uid, my_list[i]->size);

		kfree(my_list[i]);
	}

	return 0;
}


static int my_open(struct inode *inode, struct file *file)
{
	if(Debug)
		printk(KERN_INFO"Inside the my_open function.");	
	return single_open(file, my_show, NULL);
};


/***********************************************************************
*	This is the part where some handler functions are linked	
*	with the file, for when the file in opened or read or any
*	other possible event.
*
************************************************************************/

static struct file_operations f_ops = {
	.owner   = THIS_MODULE,
	.open    = my_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};


/***********************************************************************
*
*	This part is fundamental for any module. In this function we 
*	define the parts of the functions, witch are compiled when
*	the module is loaded/unloaded.
*
************************************************************************/

static int __init module_entry(void)
{
	struct proc_dir_entry *entry;
	entry = proc_create(MODULE_NAME, 0, NULL, &f_ops);

	if(!entry)
	{
		printk(KERN_ERR " Not Enough Memory!!\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "Module %s loaded successfully \n",MODULE_NAME);
	return 0;
}

static void __exit cleanup_exit(void)
{
	remove_proc_entry(MODULE_NAME, NULL);
	printk(KERN_INFO "Module %s unloaded successfully \n",MODULE_NAME);
}


MODULE_AUTHOR("Milad Bohlouli - Pooya Ghahremanian");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tasks-Monitoring module");

static int __init module_entry(void);
static void __exit cleanup_exit(void);

module_init(module_entry);
module_exit(cleanup_exit);



