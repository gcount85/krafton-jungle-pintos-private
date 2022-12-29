#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
// P2 syscall: syscall interface를 위한 헤더 추가 - 시작
#include "include/filesys/file.h"
#include "include/filesys/filesys.h"
#include "include/lib/kernel/stdio.h"

// P2 syscall: syscall interface를 위한 헤더 추가 - 끝

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
#define STDIN 1  // P2 sys call: FDT의 FD 0 매크로 선언
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
		exit(-777);
	}
	// printf("system call!\n");
}

// P2 sys call: 유저가 준 포인터가 유효한 지 확인
void check_address(void *addr)
{
	struct thread *cur = thread_current();
	if (!addr || !is_user_vaddr(addr) || pml4_get_page(cur->pml4, addr) == NULL)
	{
		exit(-666);
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
	thread_exit();
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
	thread_exit();
}

int wait(tid_t pid)
{
	thread_exit();
}

// P2 sys call; `initial_size`를 가진 파일을 만듦
bool create(const char *file, unsigned initial_size)
{
	check_address(file);

	if (filesys_create(file, initial_size))
	{
		return true;
	}
	else
	{
		return false;
	}
}

// P2 sys call; `file`이라는 이름을 가진 파일 삭제
bool remove(const char *file)
{
	check_address(file);

	if (filesys_remove(file))
	{
		return true;
	}
	else
	{
		return false;
	}
}

// P2 sys call: `file` 안의 path에 있는 file open + fd 반환
int open(const char *file)
{
	check_address(file);

	struct thread *cur = thread_current();

	/* 반복문으로 비어있는 곳 찾기(next_fd 필드 사용X) */
	int i = 2;
	while (true) // 비어 있는 fdt 엔트리를 찾을 때까지 검색
	{
		if (cur->fdt[i] == 0)
		{
			cur->fdt[i] = filesys_open(file);
			return i;
		}

		if (i == OPEN_MAX)
		{
			return -1;
		}

		i++;
	}

	/* next_fd를 사용하기 - 시작 */
	// int openfd = cur->next_fd;

	// if (openfd == OPEN_MAX)
	// {
	// 	return;
	// }

	// cur->fdt[openfd] = filesys_open(file);
	// cur->next_fd = openfd + 1;

	// return openfd;
	/* next_fd를 사용하기 - 끝 */
}

// P2 sys call: `fd`로 open 되어 있는 파일의 바이트 사이즈 반환
int filesize(int fd)
{
	struct thread *cur = thread_current();

	if (fd < 2 || cur->fdt[fd] == 0)
	{
		return -1;
	}

	return file_length(cur->fdt[fd]);
}

// P2 sys call: fd`로 open 되어 있는 파일로부터 `size` 바이트를 읽어내 `buffer`에 담음 + 실제로 읽어낸 바이트 반환
int read(int fd, void *buffer, unsigned size)
{
	check_address(buffer);

	struct thread *cur = thread_current();

	if (fd < 0 || cur->fdt[fd] == NULL || cur->fdt[fd] == STDOUT)
	{
		return -1;
	}

	if (cur->fdt[fd] == STDIN)
	{
		return input_getc();
	}

	return file_read(cur->fdt[fd], buffer, size); // 0 반환 → 파일의 끝을 의미
}

// P2 sys call: `buffer`에 담긴 `size` 바이트를 open file `fd`에 쓰기 + 실제로 쓰인 바이트의 숫자를 반환
int write(int fd, const void *buffer, unsigned size)
{
	check_address(buffer);

	struct thread *cur = thread_current();

	if (fd < 0 || cur->fdt[fd] == NULL || cur->fdt[fd] == STDIN) // 에러조건문 추가 
	{
		return -1;
	}

	if (read(fd, buffer, size) == 0) // 파일의 끝인 경우 에러
	{
		return -1;
	}

	if (cur->fdt[fd] == STDOUT)
	{
		putbuf(buffer, size);
		return size;
	}

	return file_write(cur->fdt[fd], buffer, size);
}

// P2 sys call: 파일의 위치(offset)를 이동하는 함수. open file `fd`에서 읽히거나 적혀야 하는 다음 바이트를 `position` 위치로 바꿈
void seek(int fd, unsigned new_position)
{
	struct thread *cur = thread_current();

	if (fd < 2 || cur->fdt[fd] == 0)
	{
		return;
	}

	file_seek(cur->fdt[fd], new_position);
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

void close(int fd)
{
	struct thread *cur = thread_current();
	if (fd < 2 || cur->fdt[fd] == 0)
	{
		return;
	}
	file_close(cur->fdt[fd]);
	cur->fdt[fd] = 0;
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