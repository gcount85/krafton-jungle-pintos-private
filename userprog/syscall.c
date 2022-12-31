#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

/******* P2 syscall: syscall interface를 위한 헤더 추가 - 시작 *******/
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/kernel/stdio.h"
#include "userprog/process.h" // exec 헤더
#include "threads/palloc.h"	  // EXEC PALLOG GET PAGE 헤더
#include <string.h>			  // strcpy를 위한?

/******* P2 syscall: syscall interface를 위한 헤더 추가 - 끝 *******/

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

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

/***************** P2 syscall: 매크로 추가 *****************/
#define STDIN 1	 // P2 sys call: FDT의 FD 0 매크로 선언
#define STDOUT 2 // P2 sys call: FDT의 FD 1 매크로 선언
/***************** P2 syscall: 매크로 추가 - 끝 *****************/

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

	/********* P2 syscall: filesys_lock 초기화를 위한 코드 추가 *********/
	lock_init(&filesys_lock);
	/********* P2 syscall: filesys_lock 초기화를 위한 코드 추가 - 끝 *********/
}

/******* P2 syscall: TODO The main system call interface & body *******/
/* ***** 중요한 점 ******
 * 1. syscall-nr.h 에서 번호 확인
 * 2. 유저가 준 포인터 에러처리; (void *)0 같은 포인터 넘겨주는 경우 등
 * (2번의 경우 포인터를 받는 함수들만 check_address 만들어서 확인하기) */
void syscall_handler(struct intr_frame *f)
{
	// TODO: Your implementation goes here.
	// 	`%rdi`, `%rsi`, `%rdx`, `%r10`, `%r8`, and `%r9`
	switch (f->R.rax)
	{
	case SYS_HALT: /* Halt the operating system. */
		halt();
		break;
	case SYS_EXIT: /* Terminate this process. */
		exit(f->R.rdi);
		break;
	case SYS_FORK: /* Clone current process. */
		fork(f->R.rdi);
		break;
	case SYS_EXEC: /* Switch current process. */
		f->R.rax = exec(f->R.rdi);
		break;
	case SYS_WAIT: /* Wait for a child process to die. */
		f->R.rax = wait(f->R.rdi);
		break;
	case SYS_CREATE: /* Create a file. */
		f->R.rax = create(f->R.rdi, f->R.rsi);
		break;
	case SYS_REMOVE: /* Delete a file. */
		f->R.rax = remove(f->R.rdi);
		break;
	case SYS_OPEN: /* Open a file. */
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE: /* Obtain a file's size. */
		f->R.rax = filesize(f->R.rdi);
		break;
	case SYS_READ: /* Read from a file. */
		f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_WRITE: /* Write to a file. */
		f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_SEEK: /* Change position in a file. */
		seek(f->R.rdi, f->R.rsi);
		break;
	case SYS_TELL: /* Report current position in a file. */
		f->R.rax = tell(f->R.rdi);
		break;
	case SYS_CLOSE:
		close(f->R.rdi);
		break;
	default:
		exit(-1);
	}
	// printf("system call!\n");
}

// P2 sys call: 유저가 준 포인터가 유효한 지 확인
void check_address(void *addr)
{
	struct thread *cur = thread_current();
	if (!addr || !is_user_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
	{
		exit(-1);
	}
}

/* P2 syscall: sys call body
 * 다 만들고 헤더에 추가하기
 *  */
void halt(void)
{
	power_off();
}

int fork(const char *thread_name)
{
	tid_t tid;
	tid = process_fork(thread_name, &thread_current()->tf);

	return tid;
	
	// if fork 성공(자식 프로세스에서 리턴값 0): return 자식의 pid
	// if fork 실패: return TID_ERROR

	// thread_exit();
}

void exit(int status)
{
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

int exec(const char *cmd_line)
{
	check_address(cmd_line);

	/********* P2 syscall: 추가 코드 - 시작 *********/
	char *fn_copy;
	tid_t tid;

	fn_copy = palloc_get_page(PAL_ZERO);
	if (fn_copy == NULL)
		return TID_ERROR; // or return -1?
	strlcpy(fn_copy, cmd_line, PGSIZE);

	tid = process_exec(fn_copy); // line 196

	if (tid < 0)
		exit(-1);
	else
		return tid;

	/********* P2 syscall: 추가 코드 - 끝 *********/
}

int wait(tid_t pid)
{
	// return process_wait(pid);
	thread_exit();
}

// P2 sys call; `initial_size`를 가진 파일을 만듦
bool create(const char *file, unsigned initial_size)
{
	check_address(file);

	lock_acquire(&filesys_lock);
	bool bool_created = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return bool_created;
}

// P2 sys call; `file`이라는 이름을 가진 파일 삭제
bool remove(const char *file)
{
	check_address(file);

	lock_acquire(&filesys_lock);
	bool bool_removed = filesys_remove(file);
	lock_release(&filesys_lock);
	return bool_removed;
}

// P2 sys call: `file` 안의 path에 있는 file open + fd 반환
int open(const char *file)
{
	check_address(file);

	struct thread *cur = thread_current();

	/* 반복문으로 비어있는 곳 찾기(next_fd 필드 사용X) */
	int i = 2;
	// while (true) // 비어 있는 fdt 엔트리를 찾을 때까지 검색
	while (i < OPEN_MAX) // 비어 있는 fdt 엔트리를 찾을 때까지 검색
	{
		if (cur->fdt[i] != 0)
		{
			continue;
		}
		else
		{
			lock_acquire(&filesys_lock);
			struct file *f = filesys_open(file);
			lock_release(&filesys_lock);
			if (f == NULL)
			{
				return -1;
			}
			cur->fdt[i] = f;
			return i;
		}
		i++;
	}
	return -1;

}

// P2 sys call: `fd`로 open 되어 있는 파일의 바이트 사이즈 반환
int filesize(int fd)
{
	struct thread *cur = thread_current();
	struct file *f = cur->fdt[fd];

	if (fd < 2 || f == 0)
	{
		return -1;
	}

	return file_length(f);
}

// P2 sys call: fd`로 open 되어 있는 파일로부터 `size` 바이트를 읽어내 `buffer`에 담음 + 실제로 읽어낸 바이트 반환
int read(int fd, void *buffer, unsigned size)
{
	check_address(buffer);
	check_address((uint8_t *)buffer + size - 1); // 버퍼 끝 사이즈도 확인하기 - 추가

	struct thread *cur = thread_current();
	struct file *f = cur->fdt[fd];

	if (fd < 0 || f == NULL || f == STDOUT)
	{
		return -1;
	}

	if (f == STDIN)
	{
		return input_getc();
	}

	return file_read(f, buffer, size); // 0 반환 → 파일의 끝을 의미
}

// P2 sys call: `buffer`에 담긴 `size` 바이트를 open file `fd`에 쓰기 + 실제로 쓰인 바이트의 숫자를 반환
int write(int fd, const void *buffer, unsigned size)
{
	check_address(buffer);

	struct thread *cur = thread_current();
	struct file *f = cur->fdt[fd];
	off_t byte_written;

	if (fd < 0 || f == NULL || f == STDIN) // 에러조건문 추가
	{
		return -1;
	}

	// 파일의 끝인 경우 에러 (proj 3에서 필요할 수 있는 에러 처리)
	// if (read(fd, buffer, size) == 0)
	// {
	// 	return -1;
	// }

	if (f == STDOUT)
	{
		putbuf(buffer, size);
		return size;
	}

	lock_acquire(&filesys_lock);
	byte_written = file_write(f, buffer, size); // 내 원래 코드
	lock_release(&filesys_lock);
	return byte_written;
}

// P2 sys call: 파일의 위치(offset)를 이동하는 함수. open file `fd`에서 읽히거나 적혀야 하는 다음 바이트를 `position` 위치로 바꿈
void seek(int fd, unsigned new_position)
{
	struct thread *cur = thread_current();

	if (fd < 2 || cur->fdt[fd] == 0)
	{
		return;
	}
	lock_acquire(&filesys_lock);
	file_seek(cur->fdt[fd], new_position);
	lock_release(&filesys_lock);
}

// P2 sys call: open file `fd`에서 읽히거나 적혀야 하는 다음 바이트의 포지션을 반환
unsigned tell(int fd)
{
	struct thread *cur = thread_current();

	if (fd < 2 || cur->fdt[fd] == 0)
	{
		return;
	}

	return file_tell(cur->fdt[fd]);
}

// 러닝 스레드의 열린 파일 모두 닫음
void close(int fd)
{
	struct thread *cur = thread_current();
	struct file *f = cur->fdt[fd];
	if (fd < 2 || f == 0)
	{
		return;
	}
	lock_acquire(&filesys_lock);
	file_close(f);
	lock_release(&filesys_lock);
	cur->fdt[fd] = 0; // 이거 f = 0; 으로 하면 에러남! 조심!
}
/******* P2 syscall: TODO The main system call interface & body - 끝 *******/

/****** P2 syscall: kaist pdf에서 복붙한 코드 - 시작 - unsure ******/
// /* Reads a byte at user virtual address UADDR.
// UADDR must be below PHYS_BASE.
// Returns the byte value if successful, -1 if a segfault
// occurred. */
// static int get_user(const uint8_t *uaddr)
// {
// 	int result;
// 	asm("movl $1f, %0; movzbl %1, %0; 1:"
// 		: "=&a"(result)
// 		: "m"(*uaddr));
// 	return result;
// }

// /* Writes BYTE to user address UDST.
// UDST must be below PHYS_BASE.
// Returns true if successful, false if a segfault occurred.*/
// static bool put_user(uint8_t *udst, uint8_t byte)
// {
// 	int error_code;
// 	asm("movl $1f, %0; movb %b2, %1; 1:"
// 		: "=&a"(error_code), "=m"(*udst)
// 		: "q"(byte));
// 	return error_code != -1;
// }
/********************* pdf에서 복붙한 코드: unsure - 끝 *****************/