#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);
typedef int pid_t; // P2 syscall: 추가한 부분

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface
 * P2 syscall TODO: body 구현하기
 ***** 중요한 점 ******
 * 1. syscall-nr.h 에서 번호 확인
 * 2. 유저가 준 포인터 에러처리; (void *)0 같은 포인터 넘겨주는 경우 등 */
void syscall_handler(struct intr_frame *f UNUSED)
{
	// TODO: Your implementation goes here.
	
	switch (f->R.rax)
	{
	case SYS_HALT: /* Halt the operating system. */
		halt();
		break;
	case SYS_EXIT: /* Terminate this process. */
		exit(status);
		break;
	case SYS_FORK: /* Clone current process. */
		fork();
		break;
	case SYS_EXEC: /* Switch current process. */
		exec(cmd_line);
		break;
	case SYS_WAIT: /* Wait for a child process to die. */
		break;
	case SYS_CREATE: /* Create a file. */
		break;
	case SYS_REMOVE: /* Delete a file. */
		break;
	case SYS_OPEN: /* Open a file. */
		break;
	case SYS_FILESIZE: /* Obtain a file's size. */
		filesize();
		break;
	case SYS_READ: /* Read from a file. */
		read();
		break;
	case SYS_WRITE: /* Write to a file. */
		write();
		break;
	case SYS_SEEK: /* Change position in a file. */
		seek();
		break;
	case SYS_TELL: /* Report current position in a file. */
		tell();
		break;
	case SYS_CLOSE:
		close();
		break;
	default:
		exit(-1);
	}
	printf("system call!\n");
}

/* P2 syscall: sys call body
 * 다 만들고 헤더에 추가하기*/
void halt(void)
{
	power_off();
}

void exit(int status)
{
	struct thread *cur = thread_current();
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}
pid_t exec(const char *cmd_line)
{
	power_off();
}

int wait(pid_t pid)
{
}

/********************* pdf에서 복붙한 코드: unsure *****************/

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. */
static int get_user(const uint8_t *uaddr)
{
	int result;
	asm("movl $1f, %0; movzbl %1, %0; 1:"
		: "=&a"(result)
		: "m"(*uaddr));
	return result;
}

/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred.*/
static bool put_user(uint8_t *udst, uint8_t byte)
{
	int error_code;
	asm("movl $1f, %0; movb %b2, %1; 1:"
		: "=&a"(error_code), "=m"(*udst)
		: "q"(byte));
	return error_code != -1;
}
/********************* pdf에서 복붙한 코드: unsure - 끝 *****************/