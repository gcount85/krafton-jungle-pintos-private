#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
/********* P2 syscall: 추가한 헤더 - 시작 *********/
#include "threads/malloc.h"
#include "lib/user/syscall.h"

/********* P2 syscall: 추가한 헤더 - 끝 *********/

#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup(void);
static bool load(const char *file_name, struct intr_frame *if_);
static void initd(void *f_name);
static void __do_fork(void *);

/* General process initializer for initd and other process. */
static void process_init(void)
{
    struct thread *current = thread_current();
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE.
 * === process_excute */
tid_t process_create_initd(const char *file_name)
{
    char *fn_copy;
    tid_t tid;

    /* Make a copy of FILE_NAME.
     * Otherwise there's a race between the caller and load(). */
    fn_copy = palloc_get_page(0);
    if (fn_copy == NULL)
        return TID_ERROR;
    strlcpy(fn_copy, file_name, PGSIZE);

    /****************** P2 arg passing: added - 시작 *************************/
    // P2 arg passing: filename 파싱하기 (파싱 예시: ls –a → ls)
    char *save_ptr;
    file_name = strtok_r(file_name, " ", &save_ptr);
    /****************** P2 arg passing: added - 끝 *************************/

    /* Create a new thread to execute FILE_NAME. */
    // 인수; 스레드 이름(문자열), 스레드 우선순위,
    // 생성된 스레드가 실행할 함수 포인터, 그 함수에게 넘겨줄 인수
    tid = thread_create(file_name, PRI_DEFAULT, initd, fn_copy);

    if (tid == TID_ERROR)
        palloc_free_page(fn_copy);
    return tid;
}

/* A thread function that launches first user process. */
static void initd(void *f_name)
{
#ifdef VM
    supplemental_page_table_init(&thread_current()->spt);
#endif

    process_init();

    if (process_exec(f_name) < 0)
        PANIC("Fail to launch initd\n");

    NOT_REACHED();
}

/****************** P2 arg passing: added - 시작 *************************/
/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
tid_t process_fork(const char *name, struct intr_frame *if_ UNUSED)
{
    /* Clone current thread to new thread.*/
    struct thread *parent = thread_current();
    memcpy(&parent->parent_if, if_, sizeof(struct intr_frame));

    tid_t tid = thread_create(name, PRI_DEFAULT, __do_fork, parent);
    if (tid == TID_ERROR)
    {
        return TID_ERROR;
    }

    sema_down(&parent->sema_for_fork); // sema up이 do fork에서 이루어질때까지
                                       // 다음 수행은 이루어지지 않음
    if (get_exit_status(parent, tid) == -1)
    {
        return TID_ERROR;
    }

    return tid;
}
/****************** P2 arg passing: added - 끝 *************************/

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
static bool duplicate_pte(uint64_t *pte, void *va, void *aux)
{
    struct thread *current = thread_current();
    struct thread *parent = (struct thread *)aux;
    void *parent_page;
    void *newpage;
    bool writable;

    /********* P2 syscall: added - 시작 *********/
    /* 1. TODO: If the parent_page is kernel page, then return immediately. */
    if (is_kernel_vaddr(va)) // 커널 공간인 경우 바로 복사하도록 반환
    {
        return true;
    }

    /* 2. Resolve VA from the parent's page map level 4. */
    parent_page = pml4_get_page(parent->pml4, va);
    if (parent_page == NULL)
    {
        return false;
    }

    /* 3. TODO: Allocate new PAL_USER page for the child and set result to
     *    TODO: NEWPAGE. */
    newpage = palloc_get_page(PAL_ZERO);
    if (newpage == NULL)
    {
        return false;
    }

    /* 4. TODO: Duplicate parent's page to the new page and
     *    TODO: check whether parent's page is writable or not (set WRITABLE
     *    TODO: according to the result). */
    memcpy(newpage, parent_page, PGSIZE);
    writable = is_writable(pte);

    /* 5. Add new page to child's page table at address VA with WRITABLE
     *    permission. */
    if (!pml4_set_page(current->pml4, va, newpage, writable))
    {
        return false;
    }

    /********* P2 syscall: added - 끝 *********/
    return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */
static void  __do_fork(void *aux)
{
    struct intr_frame if_;                        // 부모로부터 복사된 값을 받을 자식의 if
    struct thread *parent = (struct thread *)aux; // 부모
    struct thread *current = thread_current();    // 자식

    /********* P2 syscall: added - 시작 *********/
    /* TODO: somehow pass the parent_if. (i.e. process_fork()'s if_) */
    struct intr_frame *parent_if = &parent->parent_if;
    bool succ = true;

    /* 1. Read the cpu context to local stack. */
    memcpy(&if_, parent_if, sizeof(struct intr_frame));
    /********* P2 syscall: added - 끝 *********/

    /* 2. Duplicate PT */
    current->pml4 = pml4_create();
    if (current->pml4 == NULL)
        goto error;

    process_activate(current);
#ifdef VM
    supplemental_page_table_init(&current->spt);
    if (!supplemental_page_table_copy(&current->spt, &parent->spt))
        goto error;
#else
    if (!pml4_for_each(parent->pml4, duplicate_pte, parent))
        goto error;
#endif

    /********* P2 syscall: added - 시작 *********/
    /* TODO: Your code goes here.
     * TODO: Hint) To duplicate the file object, use `file_duplicate`
     * TODO:       in include/filesys/file.h. Note that parent should not return
     * TODO:       from the fork() until this function successfully duplicates
     * TODO:       the resources of parent.*/

    // +++ multi-oom을 통과하기 위함
    if (parent->next_fd == OPEN_MAX)
    {
        goto error;
    }

    // 부모의 fdt를 복사하여 자식의 fdt로 넣음
    for (int i = 0; i < OPEN_MAX; i++)
    {
        struct file *file = parent->fdt[i];

        if (file == NULL) // 열린 파일이 없는 경우
        {
            continue;
        }

        if (file == 1 || file == 2) // 스탠다드 입출력 파일인 경우
        {
            current->fdt[i] = file;
        }
        else // 평범한 파일인 경우
        {
            current->fdt[i] = file_duplicate(file);
        }
    }
    current->next_fd = parent->next_fd;
    if_.R.rax = 0; // 성공의 의미로 0 반환 
    sema_up(&parent->sema_for_fork); // 여기서 부모 깨우기! 

    /********* P2 syscall: added - 끝 *********/

    /* Finally, switch to the newly created process. */
    if (succ)
        do_iret(&if_);
error:
    /********* P2 syscall: added - 시작 *********/
    set_exit_status(parent, current->tid, TID_ERROR);
    sema_up(&parent->sema_for_fork); // 에러일 때도 깨워야 함 
    exit(TID_ERROR);

    /********* P2 syscall: added - 끝 *********/
}

/* Switch the current execution context to the f_name.
 * Returns -1 on fail.
 * === start_process */
int process_exec(void *f_name)
{

    char *file_name = f_name;
    bool success;

    /* We cannot use the intr_frame in the thread structure.
     * This is because when current thread rescheduled,
     * it stores the execution information to the member. */
    struct intr_frame _if;
    _if.ds = _if.es = _if.ss = SEL_UDSEG;
    _if.cs = SEL_UCSEG;
    _if.eflags = FLAG_IF | FLAG_MBS;

    /* We first kill the current context */
    process_cleanup();

    /* And then load the binary */
    success = load(file_name, &_if);

    /* If load failed, quit. */
    if (!success)
    {
        palloc_free_page(file_name); // ++ 자리 옮김
        return -1;
    }

    /* Start switched process. */
    do_iret(&_if);
    NOT_REACHED();
}

/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 *cd
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */

/********* P2 syscall: modified - 시작 *********/
int process_wait(tid_t child_tid UNUSED)
{
    /* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
     * XXX:       to add infinite loop here before
     * XXX:       implementing the process_wait. */

    struct thread *curr = thread_current();
    struct child_info_t *info = NULL;
    if (!(info = get_child_info(curr, child_tid)))
    {
        return -1;
    }
    sema_down(&info->sema_for_wait); // 이 시점에서 자식 프로세스 종료될 때까지 호출자
                                     // 블락, child가 exit을 부를 때 sema_up()
    int child_exit_status = get_exit_status(curr, child_tid);
    delete_child(curr, child_tid);

    return child_exit_status;
}

/* Exit the process. This function is called by thread_exit (). */
void process_exit(void)
{
    struct thread *cur = thread_current();
    /* TODO: Your code goes here.
     * TODO: Implement process termination message (see
     * TODO: project2/process_termination.html).
     * TODO: We recommend you to implement process resource cleanup here. */

    // 모든 열린 파일 close
    for (int fd = 0; fd < OPEN_MAX, fd++;)
    {
        close(fd);
    }

    /* Deallocate File Descriptor Table */
    palloc_free_multiple(cur->fdt, FDT_PAGES);
    file_close(cur->running_f);
    struct child_info_t *info = get_child_info(cur->parent, cur->tid);
    sema_up(&info->sema_for_wait); // 블락 되어있던 wait 호출자 프로세스가 깨어남

    process_cleanup();
}
/********* P2 syscall: modified - 끝 *********/

/* Free the current process's resources. */
static void process_cleanup(void)
{
    struct thread *curr = thread_current();

#ifdef VM
    supplemental_page_table_kill(&curr->spt);
#endif

    uint64_t *pml4;
    /* Destroy the current process's page directory and switch back
     * to the kernel-only page directory. */
    pml4 = curr->pml4;
    if (pml4 != NULL)
    {
        /* Correct ordering here is crucial.  We must set
         * cur->pagedir to NULL before switching page directories,
         * so that a timer interrupt can't switch back to the
         * process page directory.  We must activate the base page
         * directory before destroying the process's page
         * directory, or our active page directory will be one
         * that's been freed (and cleared). */
        curr->pml4 = NULL;
        pml4_activate(NULL);
        pml4_destroy(pml4);
    }
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
void process_activate(struct thread *next)
{
    /* Activate thread's page tables. */
    pml4_activate(next->pml4);

    /* Set thread's kernel stack for use in processing interrupts. */
    tss_update(next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL 0           /* Ignore. */
#define PT_LOAD 1           /* Loadable segment. */
#define PT_DYNAMIC 2        /* Dynamic linking info. */
#define PT_INTERP 3         /* Name of dynamic loader. */
#define PT_NOTE 4           /* Auxiliary info. */
#define PT_SHLIB 5          /* Reserved. */
#define PT_PHDR 6           /* Program header table. */
#define PT_STACK 0x6474e551 /* Stack segment. */

#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr
{
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ELF64_PHDR
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

/* Abbreviations */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack(struct intr_frame *if_);
static bool validate_segment(const struct Phdr *, struct file *);
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */
static bool load(const char *file_name, struct intr_frame *if_)
{

    struct thread *t = thread_current();
    struct ELF ehdr;
    struct file *file = NULL;
    off_t file_ofs;
    bool success = false;
    int i;

    // **************** P2 arg passing: 파일명 파싱 ***************************
    // //
    char *token, *save_ptr;
    char *argv[64]; // char형 포인터는 string으로 받는다
    int argc = 0;

    for (token = strtok_r(file_name, " ", &save_ptr); token != NULL; token = strtok_r(NULL, " ", &save_ptr))
    {
        argv[argc] = token;
        argc++;
    }
    // **************** P2 arg passing: 파일명 파싱 - 끝
    // ************************ //

    /* 메모리 할당; Allocate and activate page directory. */
    t->pml4 = pml4_create();
    if (t->pml4 == NULL)
        goto done;
    process_activate(thread_current());

    /* Open executable file. */

    file = filesys_open(file_name);
    if (file == NULL)
    {
        printf("load: %s: open failed\n", file_name);
        goto done;
    }

    /********* P2 syscall: 파일 권한 코드 추가 - 시작 *********/
    file_deny_write(file); // 파일의 쓰기 권한 닫음
    t->running_f = file;   // 실행중인 파일로 올림
    /********* P2 syscall: 파일 권한 코드 추가 - 끝 *********/

    /* Read and verify executable header. */
    if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr || memcmp(ehdr.e_ident, "\177ELF\2\1\1", 7) || ehdr.e_type != 2 || ehdr.e_machine != 0x3E // amd64
        || ehdr.e_version != 1 || ehdr.e_phentsize != sizeof(struct Phdr) || ehdr.e_phnum > 1024)
    {
        printf("load: %s: error loading executable\n", file_name);
        goto done;
    }

    /* Read program headers. */
    file_ofs = ehdr.e_phoff;
    for (i = 0; i < ehdr.e_phnum; i++)
    {
        struct Phdr phdr;

        if (file_ofs < 0 || file_ofs > file_length(file))
            goto done;
        file_seek(file, file_ofs);

        if (file_read(file, &phdr, sizeof phdr) != sizeof phdr)
            goto done;
        file_ofs += sizeof phdr;
        switch (phdr.p_type)
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
            /* Ignore this segment. */
            break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
            goto done;
        case PT_LOAD:
            if (validate_segment(&phdr, file))
            {
                bool writable = (phdr.p_flags & PF_W) != 0;
                uint64_t file_page = phdr.p_offset & ~PGMASK;
                uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
                uint64_t page_offset = phdr.p_vaddr & PGMASK;
                uint32_t read_bytes, zero_bytes;
                if (phdr.p_filesz > 0)
                {
                    /* Normal segment.
                     * Read initial part from disk and zero the rest. */
                    read_bytes = page_offset + phdr.p_filesz;
                    zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) - read_bytes);
                }
                else
                {
                    /* Entirely zero.
                     * Don't read anything from disk. */
                    read_bytes = 0;
                    zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
                }

                if (!load_segment(file, file_page, (void *)mem_page, read_bytes, zero_bytes, writable))
                {
                    goto done;
                }
            }
            else
            {
                goto done;
            }
            break;
        }
    }

    /* Set up stack. */
    if (!setup_stack(if_))
    {
        goto done;
    }

    /* Start address. */
    if_->rip = ehdr.e_entry;

    /**************** P2 arg passing: args passing ****************/
    /* TODO : Implement argument passing(see project2 / argument_passing.html).
     * argv: 프로그램 이름과 인자가 저장되어 있는 메모리 공간
     * count: 인자의 개수
     * rsp: 스택 포인터를 가리키는 주소 */
    stack_arguments(argc, argv, if_);
    // hex_dump((uintptr_t)if_->rsp, if_->rsp, USER_STACK - if_->rsp, true);

    /**************** P2 arg passing: args passing - end ****************/

    success = true;

done:
    /* We arrive here whether the load is successful or not. */
    // file_close(file);
    return success;
}

/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool validate_segment(const struct Phdr *phdr, struct file *file)
{
    /* p_offset and p_vaddr must have the same page offset. */
    if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
        return false;

    /* p_offset must point within FILE. */
    if (phdr->p_offset > (uint64_t)file_length(file))
        return false;

    /* p_memsz must be at least as big as p_filesz. */
    if (phdr->p_memsz < phdr->p_filesz)
        return false;

    /* The segment must not be empty. */
    if (phdr->p_memsz == 0)
        return false;

    /* The virtual memory region must both start and end within the
       user address space range. */
    if (!is_user_vaddr((void *)phdr->p_vaddr))
        return false;
    if (!is_user_vaddr((void *)(phdr->p_vaddr + phdr->p_memsz)))
        return false;

    /* The region cannot "wrap around" across the kernel virtual
       address space. */
    if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
        return false;

    /* Disallow mapping page 0.
       Not only is it a bad idea to map page 0, but if we allowed
       it then user code that passed a null pointer to system calls
       could quite likely panic the kernel by way of null pointer
       assertions in memcpy(), etc. */
    if (phdr->p_vaddr < PGSIZE)
        return false;

    /* It's okay. */
    return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page(void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(upage) == 0);
    ASSERT(ofs % PGSIZE == 0);

    file_seek(file, ofs);
    while (read_bytes > 0 || zero_bytes > 0)
    {
        /* Do calculate how to fill this page.
         * We will read PAGE_READ_BYTES bytes from FILE
         * and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* Get a page of memory. */
        uint8_t *kpage = palloc_get_page(PAL_USER);
        if (kpage == NULL)
            return false;

        /* Load this page. */
        if (file_read(file, kpage, page_read_bytes) != (int)page_read_bytes)
        {
            palloc_free_page(kpage);
            return false;
        }
        memset(kpage + page_read_bytes, 0, page_zero_bytes);

        /* Add the page to the process's address space. */
        if (!install_page(upage, kpage, writable))
        {
            printf("fail\n");
            palloc_free_page(kpage);
            return false;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        upage += PGSIZE;
    }
    return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK
 * VM을 만들기 전의 setup_stack!!!!!! */
static bool setup_stack(struct intr_frame *if_)
{
    uint8_t *kpage;
    bool success = false;

    // 페이지 할당
    kpage = palloc_get_page(PAL_USER | PAL_ZERO);
    if (kpage != NULL)
    {
        // 페이지 테이블 세팅
        success = install_page(((uint8_t *)USER_STACK) - PGSIZE, kpage, true);
        if (success)
        {
            if_->rsp = USER_STACK; // 스택 포인터 셋업
        }
        else
        {
            palloc_free_page(kpage);
        }
    }
    return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool install_page(void *upage, void *kpage, bool writable)
{
    struct thread *t = thread_current();

    /* Verify that there's not already a page at that virtual
     * address, then map our page there. */
    return (pml4_get_page(t->pml4, upage) == NULL && pml4_set_page(t->pml4, upage, kpage, writable));
}
#else

/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails.
 * ************ VM에서 사용하기 위해 #ifndef 블럭에서 복사한 것 */
bool install_page(void *upage, void *kpage, bool writable)
{
    struct thread *t = thread_current();

    /* Verify that there's not already a page at that virtual
     * address, then map our page there. */
    return (pml4_get_page(t->pml4, upage) == NULL && pml4_set_page(t->pml4, upage, kpage, writable));
}

/* 페이지 폴트가 났을 때 호출됨.
 * aux로 전달받은 file_info를 통해
 * 세그먼트를 읽어낼 파일을 찾고 + 세그먼트를 메모리로 올려야 함.
 * (VM이 없던 기존의 load_segment 코드를 수정함) */
static bool lazy_load_segment(struct page *page, struct file_info *file_info)
{

    /******************* P3: added *******************/
    /* TODO: Load the segment from the file */
    /* TODO: This called when the first page fault occurs on address VA. */
    /* TODO: VA is available when calling this function. */

    struct file *file = file_info->file;
    size_t offset = file_info->file_offset;
    size_t page_read_bytes = file_info->page_read_bytes;
    size_t page_zero_bytes = file_info->page_zero_bytes;
    bool writable = page->writable;

    file_seek(file, offset);

    /* Fetch a page of memory. */
    uint8_t *kpage = page->frame;
    if (kpage == NULL)
    {
        return false;
    }

    /* Load this page. */
    if (file_read(file, kpage, page_read_bytes) != (int)page_read_bytes)
    {
        palloc_free_page(kpage);
        return false;
    }
    memset(kpage + page_read_bytes, 0, page_zero_bytes); // zero the rest

    return true;
    /******************* P3: added - end *******************/
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(upage) == 0);
    ASSERT(ofs % PGSIZE == 0);

    while (read_bytes > 0 || zero_bytes > 0)
    {
        /* Do calculate how to fill this page.
         * We will read PAGE_READ_BYTES bytes from FILE
         * and zero the final PAGE_ZERO_BYTES bytes.
         * 페이지 크기보다 read_byte가 작다면 나머지를 0으로 채워야 하기 때문 */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /******************* P3: added *******************/
        /* TODO: Set up aux to pass information to the lazy_load_segment.
         * 페이지의 file, load_status, read_byts, zero_bytes, offset 필드 값? */
        struct file_info *file_info = (struct file_info *)calloc(1, sizeof(struct file_info));
        if (file_info == NULL)
        {
            return false;
        }
        file_info->file = file;
        file_info->file_offset = ofs;
        file_info->page_read_bytes = page_read_bytes;
        file_info->page_zero_bytes = page_zero_bytes;

        /******************* P3: added - end *******************/

        if (!vm_alloc_page_with_initializer(VM_ANON, upage, writable, lazy_load_segment, file_info))
        {
            return false;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        upage += PGSIZE;
    }
    return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool setup_stack(struct intr_frame *if_)
{

    bool success = false;
    void *stack_bottom = (void *)(((uint8_t *)USER_STACK) - PGSIZE); // == va, 유저 어드레스 확인

    /******************* P3: added *******************/
    /* TODO: Map the stack on stack_bottom and claim the page immediately.
     * TODO: If success, set the rsp accordingly.
     * TODO: You should mark the page is stack. */
    /* TODO: Your code goes here */

    if (vm_claim_page(stack_bottom))
    {
        struct page *stack_page = spt_find_page(&thread_current()->spt, stack_bottom);
        stack_page->vm_marker = VM_MARKER_0_STACK;
        if_->rsp = USER_STACK; // 스택 포인터 셋업
        success = true;
    }

    /******************* P3: added - end *******************/

    return success;
}

#endif /* VM */

/****************** P2: added ******************/
// P2 arg passing: 프로그램 이름과 명령행 인자를 유저 스택에 저장하는 함수
void stack_arguments(int argc, char **argv, struct intr_frame *if_)
{
    // argv: 프로그램 이름과 인자가 저장되어 있는 메모리 공간,
    // count: 인자의 개수, rsp: 스택 포인터를 가리키는 주소(**rsp는 스택에 있는
    // 데이터)
    // === push 하면서 여유 공간 있는지 확인 ?
    // 혜지코드
    char *argv_addr[128]; // array that saves addresses of arguments

    /* Pushing arguments into user stack (right -> left) */
    for (int i = argc - 1; i >= 0; i--)
    {
        int argv_len = strlen(argv[i]) + 1;
        if_->rsp -= argv_len; // including NULL
        memcpy(if_->rsp, argv[i], argv_len);
        argv_addr[i] = (char *)if_->rsp; // save addr of argument for later
    }

    /* Word-align padding */
    /* Stack pointer(address) must be multiples of 8 */
    while (if_->rsp % 8 != 0)
    {
        if_->rsp--;
        *(uint8_t *)if_->rsp = 0;
    }

    /* Addresses of each arguments */
    for (int i = argc; i >= 0; i--)
    {
        if_->rsp = if_->rsp - 8;
        if (i == argc)
            memset(if_->rsp, 0, sizeof(char **));
        else
            memcpy(if_->rsp, &argv_addr[i], sizeof(char **));
    }

    if_->R.rdi = argc;     /* Set %rdi to argc */
    if_->R.rsi = if_->rsp; /* Point %rsi to the address of argv[0] */

    /* Return Address */
    if_->rsp -= 8;
    memset(if_->rsp, 0, sizeof(void *));
}

/****************** P2: added - end ******************/
