/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
/****************** P3: added ******************/
#include "userprog/process.h"
#include "include/threads/vaddr.h"
#include "include/threads/mmu.h"
/****************** P3: added ******************/

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
    vm_anon_init();
    vm_file_init();
#ifdef EFILESYS /* For project 4 */
    pagecache_init();
#endif
    register_inspect_intr();
    /* DO NOT MODIFY UPPER LINES. */
    /* TODO: Your code goes here. */

    /*********************** P3: added ***********************/
    hash_init(&frame_table, frame_hash, frame_less, NULL);

    /*********************** P3: added - end ***********************/
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type page_get_type(struct page *page)
{
    int ty = VM_TYPE(page->operations->type);
    switch (ty)
    {
    case VM_UNINIT:
        return VM_TYPE(page->uninit.type);
    default:
        return ty;
    }
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer.
 * If you want to create a *page,
 * do not create it directly and make it through this function or
 * `vm_alloc_page`.
 * 넘겨받은 `VM_TYPE`에 따른 적절한 이니셜라이저를 갖고 오고.
 * `uninit_new`를 그것과 함께 호출해라!!
 * */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable, vm_initializer *init, struct file_info *file_info)
{

    ASSERT(VM_TYPE(type) != VM_UNINIT);
    // ASSERT(upage != NULL);
    // ASSERT(init != NULL);
    // ASSERT(file_info != NULL); // 잘 통과함

    struct supplemental_page_table *spt = &thread_current()->spt;

    /* Check wheter the upage is already occupied or not. */
    if (spt_find_page(spt, upage) == NULL)
    {

        /******************* P3: added *******************/
        /* TODO: Create the page,
         * fetch the initialier according to the VM type,
         * TODO: and then create "uninit" page struct by calling uninit_new.
         * You should modify the field after calling the uninit_new. */
        struct page *new_page = (struct page *)calloc(1, sizeof(struct page));

        switch (type)
        {
        case VM_ANON:
            uninit_new(new_page, upage, init, type, file_info, anon_initializer);
            break;
        case VM_FILE:
            uninit_new(new_page, upage, init, type, file_info, file_backed_initializer);
            break;
        default:
            goto err;
        }

        // new_page 구조체의 기타 필드 초기화 (위치 확인)
        new_page->writable = writable;
        new_page->is_loaded = 0;
        /* TODO: Insert the page into the spt. */
        if (!spt_insert_page(spt, new_page)) // 이때 hash_elem이 초기화 됨
        {
            goto err;
        }

        return true;
        /******************* P3: added - end *******************/
    }

err:
    return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
    struct page *page = NULL;

    /* TODO: Fill this function. */
    page = page_lookup(va, spt);

    return page;
}

/* Insert PAGE into spt with validation.
This function should checks that
the virtual address does not exist in the given supplemental page table. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED, struct page *page UNUSED)
{
    int succ = false;

    /* TODO: Fill this function. */
    if (!(hash_insert(&spt->spt_hash_table, &page->hash_elem))) // 삽입시 NULL, otherwise page->hash_elem
    {
        succ = true;
    }

    return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
    vm_dealloc_page(page);
    return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *vm_get_victim(void)
{
    struct frame *victim = NULL;
    /* TODO: The policy for eviction is up to you. */

    return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *vm_evict_frame(void)
{
    struct frame *victim UNUSED = vm_get_victim();
    /* TODO: swap out the victim and return the evicted frame. */

    return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *vm_get_frame(void)
{
    struct frame *frame = NULL;

    /*********************** P3: added ***********************/
    /* TODO: Fill this function. */
    /* frame 할당 */
    frame = (struct frame *)calloc(1, sizeof(struct frame));
    if (!(frame))
    {
        PANIC("todo; vm_get_frame에서 frame 널이다"); // 만약 할당 실패시
                                                      // 임시방편; todo
    }

    /* frame 구조체 필드 초기화 */
    frame->kva = palloc_get_page(PAL_USER);
    if (!(frame->kva))
    {
        palloc_free_page(frame->kva);
        PANIC("todo"); // 만약 할당 실패시 임시방편; todo
    }

    /* frame 구조체 중 hash_elem 초기화를 위한 hash_insert 호출 */
    if (hash_insert(&frame_table, &frame->hash_elem))
    {
        PANIC("todo"); // 이미 frame_table에 존재하여 삽입 실패시 임시방편; todo
    }

    frame->page = NULL;
    /*********************** P3: added - end ***********************/

    ASSERT(frame != NULL);
    ASSERT(frame->page == NULL);
    return frame;
}

/* Growing the stack. */
static void vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
/* bool not_present;    True: not-present page, false: writing read-only page.
 * bool write;          True: access was write, false: access was read.
 * bool user;           True: access by user, false: access by kernel.
 * void* fault_addr;    Fault address. */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
    struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;

    /*********************** P3: added ***********************/
    /* TODO: Validate the fault */
    if (is_kernel_vaddr(addr))
    {
        return false;
    }

    if (!(page = spt_find_page(spt, addr))) // SPT에 존재하지 않는 페이지인 경우
    {
        return false;
    }

    if ((page->writable == 0) && (write == true)) // read-only 파일에 쓰기 접근인 경우
    {
        return false;
    }

    /* TODO: Your code goes here */

    // hash_replace(&spt->spt_hash_table, &page->hash_elem);

    /*********************** P3: added ***********************/
    return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
    destroy(page);
    free(page);
}

/* Claim the page that allocate on VA.
 * VA에 할당할 page 구조체 만들기 + va, frame 필드 초기화 */
bool vm_claim_page(void *va UNUSED)
{
    struct page *page = NULL;

    /*********************** P3: added ***********************/
    /* TODO: Fill this function */
    page = (struct page *)calloc(1, sizeof(struct page));
    if (!page)
    {
        return false;
    }
    page->frame = NULL;
    page->va = va;

    /*********************** P3: added - end ***********************/

    return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu.
 * physical frame 얻기 + page와 frame간 연결 + 페이지 테이블에 매핑 추가 +
 * swap_in */
static bool vm_do_claim_page(struct page *page)
{
    struct frame *frame = vm_get_frame();

    /* Set links */
    frame->page = page;
    page->frame = frame;

    /*********************** P3: added ***********************/
    /* TODO: Insert page table entry to map page's VA to frame's PA. */

    // 기존에 이미 PTE에 존재 할 수도 있는 페이지 매핑 정보 삭제 (확인필요)
    // pml4_clear_page(&thread_current()->pml4, page->va);

    if ((install_page(page->va, frame->kva, page->writable)))
    {
        // 여기까지는 들어옴
        return swap_in(page, frame->kva);
    }
    else
    {
        printf("vm_do_claim_page: install_page fail\n");
        return false;
    }
    printf("------------------vm_do_claim_page 함수 끝!\n");

    /*********************** P3: added - end ***********************/
}

/*********************** P3: added ***********************/
/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
    if (!spt)
    {
        return;
    }

    if (!(hash_init(&spt->spt_hash_table, page_hash, page_less, NULL)))
    {
        thread_exit();
    }
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED)
{
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
    /* TODO: Destroy all the supplemental_page_table hold by thread and
     * TODO: writeback all the modified contents to the storage. */
}

/* Returns the page containing the given virtual address,
 * or a null pointer if no such page exists. */
struct page *page_lookup(const void *va, struct supplemental_page_table *spt)
{
    struct page p;
    struct hash_elem *e;

    p.va = va;
    e = hash_find(&spt->spt_hash_table, &p.hash_elem);
    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Returns a hash value for page p. */
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page *p = hash_entry(p_, struct page, hash_elem);
    return hash_bytes(&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b.
 * a와 b가 같거나(이럴 수가 있나?), a 주소가 더 크면 false */
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct page *a = hash_entry(a_, struct page, hash_elem);
    const struct page *b = hash_entry(b_, struct page, hash_elem);

    return (a->va) < (b->va);
}

/* Returns a hash value for frame f. */
unsigned frame_hash(const struct hash_elem *f_, void *aux UNUSED)
{
    const struct frame *f = hash_entry(f_, struct frame, hash_elem);
    return hash_bytes(&f->kva, sizeof f->kva);
}

/* Returns true if frame a precedes frame b.
 * a와 b가 같거나(이럴 수가 있나?), a 주소가 더 크면 false */
bool frame_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct frame *a = hash_entry(a_, struct frame, hash_elem);
    const struct frame *b = hash_entry(b_, struct frame, hash_elem);

    return (a->kva) < (b->kva);
}
/*********************** P3: added - end ***********************/
