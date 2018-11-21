#ifndef __LINUX_ROTATION_H
#define __LINUX_ROTATION_H

#define MIN_DEGREE 0
#define MID_DEGREE 180
#define MAX_DEGREE 360

#define MIN_RANGE 0
#define MAX_RANGE 180

#define READER 0
#define WRITER 1

struct rotation_entity {
	int degree; //degree of this entity
	int range; //degree of this entity
	int rwid; //indicates this entity is READER or WRITER
	atomic_t lock_held; //indicates this entity is holding a lock or not
	pid_t pid; //pid of reader or writer
	struct list_head waiters_list; //embedded list_head of waiters
	struct list_head lock_held_list; //embedded list_head of readers or writers holding a lock 
};

#endif
