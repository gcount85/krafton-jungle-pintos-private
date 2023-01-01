#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/* A counting semaphore. */
struct semaphore
{
	unsigned value;		 /* 현재 공유 자원에 접근할 수 있는 사용자 수 */
	struct list waiters; /* 대기 상태인 스레드의 리스트 */
};

void sema_init(struct semaphore *, unsigned value);
void sema_down(struct semaphore *);
bool sema_try_down(struct semaphore *);
void sema_up(struct semaphore *);
void sema_self_test(void);


/* Lock. */
// 여기는 굳이 수정할 필요 없고 세마포어만 수정해도 충분
struct lock
{
	struct thread *holder;		/* Thread holding lock (for debugging). */
	struct semaphore semaphore; /* Binary semaphore controlling access. */
};

void lock_init(struct lock *);
void lock_acquire(struct lock *);
bool lock_try_acquire(struct lock *);
void lock_release(struct lock *);
bool lock_held_by_current_thread(const struct lock *);

/* Condition variable. */
struct condition
{
	struct list waiters; /* List of waiting threads. */
};

void cond_init(struct condition *);
void cond_wait(struct condition *, struct lock *);
void cond_signal(struct condition *, struct lock *);
void cond_broadcast(struct condition *, struct lock *);

/****************** P1 priority: 추가 - 시작 *************************/
void donate_priority(void);
void remove_with_lock(struct lock *lock);
void refresh_priority(void);
bool cmp_donor_priority(const struct list_elem *a, const struct list_elem *b, void *aux);
bool cmp_sema_priority(const struct list_elem *a,
					   const struct list_elem *b,
					   void *aux); // P1 priority
/****************** P1 priority: 추가 - 끝 *************************/

/* Optimization barrier.
 *
 * The compiler will not reorder operations across an
 * optimization barrier.  See "Optimization Barriers" in the
 * reference guide for more information.*/
#define barrier() asm volatile("" \
							   :  \
							   :  \
							   : "memory")

#endif /* threads/synch.h */
