#include <linux/kernel.h>       
#include <linux/syscalls.h>     
#include <linux/sched.h>        
#include <linux/sched/signal.h> 
#include <linux/errno.h>        
#include <linux/types.h>        

asmlinkage long sys_hello(void) {
 printk("Hello, World!\n");
 return 0;
}


#define	GETPID		0x01	// 00000001
#define	PIPE	    0x02	// 00000010
#define	KILL		0x04	// 00000100

asmlinkage long sys_set_ban(int ban_getpid, int ban_pipe, int ban_kill){
	if(ban_getpid < 0 || ban_pipe < 0 || ban_kill < 0)
		return -EINVAL;
		
	if(current_euid().val != 0)
		return -EPERM;
		
	current-> banned_signals = 0;
	if(ban_getpid > 0)
		current-> banned_signals |= GETPID;
	if(ban_pipe> 0)
		current-> banned_signals |= PIPE;
	if(ban_kill > 0)
		current-> banned_signals |= KILL;
			
	return 0;

}

asmlinkage long sys_get_ban(char ban){
	unsigned char sig = 0;

	switch(ban){
		case 'g': sig = GETPID; break;
		case 'p': sig = PIPE; break;
		case 'k': sig = KILL; break;
		default: return -EINVAL;		
	}


	return (current -> banned_signals & sig) ? 1 : 0;
	

}
asmlinkage long sys_check_ban(pid_t pid, char ban){
	unsigned char sig = 0;
	
	switch(ban){
		case 'g': sig = GETPID; break;
		case 'p': sig = PIPE; break;
		case 'k': sig = KILL; break;
		default: return -EINVAL;		
	}	

	struct task_struct *other = find_task_by_vpid(pid);
	if(!other)
		return -ESRCH;
	
	if(current->banned_signals & sig)
		return -EPERM;
	
    
	return (other -> banned_signals & sig) ? 1 : 0;
}
	
asmlinkage long sys_flip_ban_branch(int height, char ban){
	if(height <= 0)
		return -EINVAL;
	
	unsigned char sig = 0;
	switch(ban){
		case 'g': sig = GETPID; break;
		case 'p': sig = PIPE; break;
		case 'k': sig = KILL; break;
		default: return -EINVAL;		
	}
	
	if(current->banned_signals & sig)
		return -EPERM;
	
	struct task_struct *parent = current -> real_parent;
	int count = 0;
	while(parent && height > 0){
		if(!(parent -> banned_signals & sig)){
			parent -> banned_signals |= sig;
			count++;	
		}
		else 
			parent -> banned_signals &= ~sig;
		
		if(parent == parent -> real_parent){
			break;
		}

		parent = parent -> real_parent;
		height--;
	}
	
	return count;
}
