#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/rotation.h>

static int DEV_DEGREE; //current device rotation
spinlock_t SET; //lock used when there is access to DEV_DEGREE

LIST_HEAD(waiters_list); //head pointer of waiters list exploiting embedded list_head waiters_list within rotation_entity
spinlock_t WAITERS; //lock used when there is access to waiters_list

LIST_HEAD(lock_held_list); //head pointer of granted list exploiting embedded list_head lock_held_list within rotation_entity
spinlock_t LOCK_HELD; //lock used when there is access to lock_held_list

static DECLARE_WAIT_QUEUE_HEAD(rotation_entity_wait_queue); //initialization of wait_queue_head_t

/* returns 1 if given degree is valid, 0 otherwise */
static int is_valid_degree(int degree)
{
	if (degree >= MIN_DEGREE && degree < MAX_DEGREE)
		return 1;
	else
		return 0;
}

/* returns 1 if given range is valid, 0 otherwise */
static int is_valid_range(int range)
{
	if (range > MIN_RANGE && range < MAX_RANGE)
		return 1;
	else
		return 0;
}

/* returns 1 if given two ranges are equal, 0 otherwise */
static int is_equal_range(int degree1, int range1, int degree2, int range2)
{
	if (degree1 == degree2 && range1 == range2)
		return 1;
	else
		return 0;
}

/* returns 1 if current device degree is within given range, 0 otherwise */
static int is_in_range(int dev_degree, int degree, int range)
{
	int diff;

	if (dev_degree - degree < 0)
		diff = degree - dev_degree;
	else
		diff = dev_degree - degree;

	/* when given degree is 330 and range is 60, given range expresses union of [270, 360) and [0, 30] */
	if (diff > MID_DEGREE)
		diff = MAX_DEGREE - diff;

	if (diff <= range)
		return 1;
	else
		return 0;
}

/* returns 1 if given two ranges are determined to be overlapped, 0 otherwise */
static int is_overlapped(int degree1, int range1, int degree2, int range2)
{
	int diff;

	if (degree1 - degree2 < 0)
		diff = degree2 - degree1;
	else
		diff = degree1 - degree2;

	/* when given degree is 330 and range is 60, given range expresses union of [270, 360) and [0, 30] */
	if (diff > MID_DEGREE)
		diff = MAX_DEGREE - diff;

	if (diff <= range1 + range2) //note that [30, 60] and [60, 90] is overlapping range
		return 1;
	else
		return 0;
}

/*
 * determine to assigning lock is possible
 * when it comes to assign new write lock, its range must not overlap with processes who have grabbed locks
 * when it comes to assign new read lock, its range must not overlap with a writers with holding locks
 * note that there is no problem range of read lock is overlapping with other readers
 * returns 1 if given range is overlapped with another range and lock is already held in that range, 0 otherwise
 */
static int is_overlapped_with_granted(struct rotation_entity *re, struct list_head *list, int rwid)
{
	struct rotation_entity *re_target;

	if (rwid == WRITER){
		list_for_each_entry(re_target, list, lock_held_list) {
		if (atomic_read(&re_target->lock_held) &&
		    is_overlapped(re->degree, re->range, re_target->degree, re_target->range))
			return 1;
		}
	}
	else {
		list_for_each_entry(re_target, list, lock_held_list) {
		if (atomic_read(&re_target->lock_held) && re_target->rwid == WRITER &&
		    is_overlapped(re->degree, re->range, re_target->degree, re_target->range))
			return 1;
		}

	}
	return 0;
}

/* returns 1 if there is a reader holding lock, whose range covers given current device rotation, 0 otherwise */
static int is_already_occupied_by_reader(int dev_degree, struct list_head *list)
{
	struct rotation_entity *re;
	list_for_each_entry(re, list, lock_held_list) {
		//find if there is a reader holding a lock and whose range covers current device degree
		if (atomic_read(&re->lock_held) && is_in_range(dev_degree, re->degree, re->range) && re->rwid == READER)
			return 1;
	}
	return 0;
}

/*
 * it must be distinguished between
 * the fact that there is a writer holding a lock and its range covers current device rotation
 * the fact that there exists a writer holding a lock
 * because in the first case, additive lock is not possible entirely in given situation with current device degree
 * returns 1 if there is a writer holding lock, whose range covers given current device rotation, 0 otherwise
 */
static int is_already_occupied_by_writer(int dev_degree, struct list_head *list)
{
	struct rotation_entity *re;
	list_for_each_entry(re, list, lock_held_list) {
		//find if there is a writer holding a lock and whose range covers current device degree
		if (atomic_read(&re->lock_held) && is_in_range(dev_degree, re->degree, re->range) && re->rwid == WRITER)
			return 1;
	}
	return 0;
}

/*
 * auxiliary function used to achieve prevention policy which avoids writer starvation
 * if there is a waiting writer whose range is covering current device degree
 * even though current degree is located in target ranges of readers, additive read lock aquisition is not allowed
 * returns 1 if there is a writer who has already waited for given current device rotation, 0 otherwise
 */
static int is_already_waited_by_writer(int dev_degree, struct list_head *list)
{
	struct rotation_entity *re;
	list_for_each_entry(re, list, waiters_list) {
		//find if there is a writer waiting for a lock and whose range covers current device degree
		if (!atomic_read(&re->lock_held) && is_in_range(dev_degree, re->degree, re->range) && re->rwid == WRITER)
			return 1;
	}
	return 0;
}

/*
 * takes rotation_entity struct as argument, simply assign locks to given rotation_entity
 * condition is already checked from assign_locks function
 */
static void assign_locks_proxy(struct rotation_entity *re)
{
	atomic_set(&re->lock_held, 1); //grab lock
	list_add_tail(&re->lock_held_list, &lock_held_list); //move this rotation entity to lock_held_list
	list_del(&re->waiters_list); //remove this rotation entity from waiters_list
	wake_up(&rotation_entity_wait_queue);
}

/* assign locks based on current device rotation, returns the total number of newly assigned locks */
static int assign_locks(int dev_degree)
{
	struct list_head *current_item, *next_item;
	int locks_assigned;
	locks_assigned = 0;

	if (is_already_occupied_by_writer(dev_degree, &lock_held_list)){
		return 0;
	}

	if(is_already_occupied_by_reader(dev_degree, &lock_held_list) &&
	   is_already_waited_by_writer(dev_degree, &waiters_list)){
		return 0;
	}

	if (!is_already_waited_by_writer(dev_degree, &waiters_list)) {
		list_for_each_safe(current_item, next_item, &waiters_list) {
			struct rotation_entity *re = list_entry(current_item, struct rotation_entity, waiters_list);
			if (is_in_range(dev_degree, re->degree, re->range) &&
			   !is_overlapped_with_granted(&re, &lock_held_list, READER)){
				assign_locks_proxy(re);
				locks_assigned++;
			}
		}
		return locks_assigned; //returns the number of newly assigned read locks
	}

	list_for_each_safe(current_item, next_item, &waiters_list) {
		struct rotation_entity *re = list_entry(current_item, struct rotation_entity, waiters_list);
		if (re->rwid == WRITER) {
			if (is_in_range(dev_degree, re->degree, re->range) &&
			   !is_overlapped_with_granted(&re, &lock_held_list, WRITER)){
				assign_locks_proxy(re);
				return 1;
			}
		}
		else {
			if (is_in_range(dev_degree, re->degree, re->range) &&
			   !is_overlapped_with_granted(&re, &lock_held_list, READER)){
				assign_locks_proxy(re);
				return 1;
			}

		}
	}
	return 0;
}

/* returns 1 if if the task with given pid is still running, 0 otherwise */
static int is_running_process(int pid){
	struct pid *pid_struct;
	struct task_struct *task;
	
	pid_struct = find_get_pid(pid);
	if(pid_struct == NULL)
		return 0;

	task = pid_task(pid_struct,PIDTYPE_PID);
	if(task == NULL)
		return 0;

	//determine whether given process is still running
	if((task->state & TASK_DEAD) != 0 || (task->state & TASK_WAKEKILL) != 0 ||
	    (task->exit_state & EXIT_ZOMBIE) != 0 || (task->exit_state & EXIT_DEAD) != 0)
		return 0;
	else
		return 1;
}

/*
 * process can hold the lock as long as it wished
 * and either eventually gives it up volutarily or
 * is forced to give it up when process dies
 */
static void release_when_process_dies(void) {
	struct list_head *current_item;
	struct list_head *next_item;

	list_for_each_safe(current_item, next_item, &lock_held_list) {
		struct rotation_entity *re = list_entry(current_item, struct rotation_entity, lock_held_list);
		if(!is_running_process(re->pid))
			list_del(current_item);
	}
}

/* 
 * system call number 380.
 * sets current device rotation in the kernel.
 * returns -1 on error, and the total number of processes awoken on success.
 */
asmlinkage int sys_set_rotation(int degree)
{
	struct list_head *current_item, *next_item;
	int locks_assigned;
	locks_assigned = 0; //the number of newly assigned locks; return value

	if (!is_valid_degree(degree))
		return -1;

	spin_lock(&SET);
	DEV_DEGREE = degree;
	spin_unlock(&SET);

	spin_lock(&LOCK_HELD);
	release_when_process_dies();
	spin_unlock(&LOCK_HELD);

	spin_lock(&SET);
	spin_lock(&WAITERS);
	spin_lock(&LOCK_HELD);
	locks_assigned = assign_locks(DEV_DEGREE);
	spin_unlock(&LOCK_HELD);
	spin_unlock(&WAITERS);
	spin_unlock(&SET);

	return locks_assigned;
}

/* 
 * system call number 381.
 * take a read lock using the given rotation range.
 * returns 0 on success, and -1 on failure.
 */
asmlinkage int sys_rotlock_read(int degree, int range)
{
	struct rotation_entity *re;

	if (!is_valid_degree(degree) || !is_valid_range(range))
		return -1; //invalid rotation value

	re = kmalloc(sizeof(struct rotation_entity), GFP_KERNEL);
	if (!re)
		return -1; //no memory

	DEFINE_WAIT(wait);

	//initialize rotation entity
	re->degree = degree;
	re->range = range;
	re->pid = current->pid;
	atomic_set(&re->lock_held, 0);
	INIT_LIST_HEAD(&re->waiters_list);
	INIT_LIST_HEAD(&re->lock_held_list);
	re->rwid = READER;

	spin_lock(&WAITERS);
	list_add_tail(&re->waiters_list, &waiters_list); //add to waiters_list
	spin_unlock(&WAITERS);

	spin_lock(&SET);
	spin_lock(&WAITERS);
	spin_lock(&LOCK_HELD);
	assign_locks(DEV_DEGREE);
	spin_unlock(&LOCK_HELD);
	spin_unlock(&WAITERS);
	spin_unlock(&SET);

	add_wait_queue(&rotation_entity_wait_queue, &wait);
	while(!atomic_read(&re->lock_held)) { //spinning until re->lock_held is 1
		prepare_to_wait(&rotation_entity_wait_queue, &wait, TASK_INTERRUPTIBLE); 
		schedule(); //calls scheduler, and resumes other process
		spin_lock(&SET);
		spin_lock(&WAITERS);
		spin_lock(&LOCK_HELD);
		assign_locks(DEV_DEGREE);
		spin_unlock(&LOCK_HELD);
		spin_unlock(&WAITERS);
		spin_unlock(&SET);
	}
	finish_wait(&rotation_entity_wait_queue, &wait);

	return 0;
}

/* 
 * system call number 382.
 * take a write lock using the given rotation range.
 * returns 0 on success, and -1 on failure.
 */
asmlinkage int sys_rotlock_write(int degree, int range)
{
	struct rotation_entity *re;
	
	DEFINE_WAIT(wait);

	if (!is_valid_degree(degree) || !is_valid_range(range))
		return -1; //invalid rotation value

	re = kmalloc(sizeof(struct rotation_entity), GFP_KERNEL);
	if (!re)
		return -1; //no memory
	
	//initialize rotation entity
	re->degree = degree;
	re->range = range;
	re->pid = current->pid;
	atomic_set(&re->lock_held, 0);
	INIT_LIST_HEAD(&re->waiters_list);
	INIT_LIST_HEAD(&re->lock_held_list);
	re->rwid = WRITER;

	spin_lock(&WAITERS);
	list_add_tail(&re->waiters_list, &waiters_list); //add to waiters_list
	spin_unlock(&WAITERS);

	spin_lock(&SET);
	spin_lock(&WAITERS);
	spin_lock(&LOCK_HELD);
	assign_locks(DEV_DEGREE);
	spin_unlock(&LOCK_HELD);
	spin_unlock(&WAITERS);
	spin_unlock(&SET);

	add_wait_queue(&rotation_entity_wait_queue, &wait);
	while(!atomic_read(&re->lock_held)) { //spinning until re->lock_held is 1
		prepare_to_wait(&rotation_entity_wait_queue, &wait, TASK_INTERRUPTIBLE);
		schedule(); //calls scheduler, and resumes other process
		spin_lock(&SET);
		spin_lock(&WAITERS);
		spin_lock(&LOCK_HELD);
		assign_locks(DEV_DEGREE);
		spin_unlock(&LOCK_HELD);
		spin_unlock(&WAITERS);
		spin_unlock(&SET);
	}
	finish_wait(&rotation_entity_wait_queue, &wait);

	return 0;
}

/* 
 * system call number 383.
 * release a read lock using the given rotation range.
 * returns 0 on success, and -1 on failure.
 */
asmlinkage int sys_rotunlock_read(int degree, int range)
{
	struct list_head *current_item, *next_item;
	struct rotation_entity *re = NULL;

	if (!is_valid_degree(degree) || !is_valid_range(range))
		return -1; //invalid rotation value

	spin_lock(&SET);
	spin_lock(&WAITERS);
	spin_lock(&LOCK_HELD);
	list_for_each_safe(current_item, next_item, &lock_held_list) {
		re = list_entry(current_item, struct rotation_entity, lock_held_list);
		/* 
		 * identified as pid, since a process cannot release other processes' lock
		 * processes and threads are managed as task struct
		 */
		if (is_equal_range(degree, range, re->degree, re->range) &&
		    re->rwid == READER && re->pid == current->pid) {
			list_del(current_item); //delete from lock_held_list
			kfree(re);
			assign_locks(DEV_DEGREE);
			spin_unlock(&LOCK_HELD);
			spin_unlock(&WAITERS);
			spin_unlock(&SET);
			return 0;
		}
	}
	spin_unlock(&WAITERS);
	spin_unlock(&LOCK_HELD);
	spin_unlock(&SET);

	return -1;
}

/* 
 * system call number 385.
 * release a write lock using the given rotation range.
 * returns 0 on success, and -1 on failure.
 */
asmlinkage int sys_rotunlock_write(int degree, int range)
{
	struct list_head *current_item, *next_item;
	struct rotation_entity *re = NULL;
	
	if (!is_valid_degree(degree) || !is_valid_range(range))
		return -1; //invalid rotation value

	spin_lock(&SET);
	spin_lock(&WAITERS);
	spin_lock(&LOCK_HELD);
	list_for_each_safe(current_item, next_item, &lock_held_list) {
		re = list_entry(current_item, struct rotation_entity, lock_held_list);	
		/* 
		 * identified as pid, since a process cannot release other processes' lock
		 * processes and threads are managed as task struct
		 */
		if (is_equal_range(degree, range, re->degree, re->range) &&
		    re->rwid == WRITER && re->pid == current->pid) {
			list_del(current_item); //delete from lock_held_list
			kfree(re);
			assign_locks(DEV_DEGREE);
			spin_unlock(&LOCK_HELD);
			spin_unlock(&WAITERS);
			spin_unlock(&SET);
			return 0;
		}
	}
	spin_unlock(&LOCK_HELD);
	spin_unlock(&WAITERS);
	spin_unlock(&SET);

	return -1;
}
