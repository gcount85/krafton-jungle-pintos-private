#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
/************* P3: added headers *************/
#include "lib/kernel/hash.h"
/********** P3: added headers - end **********/

enum vm_type
{
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 (프로젝트3까지는 무시)*/
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type)&7)

/**************** P3: added ****************/
struct file_info
{
	struct file *file;		/* page의 가상주소와 맵핑된 파일 */
	size_t page_read_bytes; /* 가상페이지에 쓰여져 있는 데이터 크기 */
	size_t page_zero_bytes; /* 0으로 채울 남은 페이지의 바이트 */
	size_t file_offset;		/* 읽어야 할 파일 오프셋 */
};
/**************** P3: added - end ****************/

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page
{
	const struct page_operations *operations;
	void *va;			 /* Address in terms of user space */
	struct frame *frame; /* Back reference for frame */

	/**************** P3: added ****************/
	/* Your implementation */

	// 각각의 페이지들을 위한 추가 데이터들 (code 페이지들, DATA 페이지들, BSS 페이지들)
	// 데이터의 위치 (frame, disk, swap), 상응하는 커널 가상 주소를 담은 포인터(이건 frame에?)
	// active or inactive (present, or not?)

	bool writable;	/* True일 경우 해당 주소에 write 가능
					 False일 경우 해당 주소에 write 불가능 */
	bool is_loaded; /* 물리메모리의 탑재 여부를 알려주는 플래그 */

	struct file_info *file_info; /* file과 관련된 정보 */

	/* Memory Mapped File 에서 다룰 예정 */
	struct list_elem mmap_elem; /* mmap 리스트 element */

	/* Swapping 과제에서 다룰 예정 */
	size_t swap_slot; /* 스왑 슬롯 */

	/* ‘vm_entry들을 위한 자료구조’ 참고 */
	struct hash_elem hash_elem; /* Hash table element. */

	/**************** P3: added - end ****************/

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union
	{
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame" */
struct frame
{
	void *kva; // ≒ 물리 주소
	struct page *page;
	/**************** P3: added ****************/
	struct hash_elem hash_elem;

	/**************** P3: added - end ****************/
};

/**************** P3: added ****************/
struct hash frame_table;

/**************** P3: added - end ****************/

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations
{
	bool (*swap_in)(struct page *, void *);
	bool (*swap_out)(struct page *);
	void (*destroy)(struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in((page), v)
#define swap_out(page) (page)->operations->swap_out(page)
#define destroy(page)                \
	if ((page)->operations->destroy) \
	(page)->operations->destroy(page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
struct supplemental_page_table
{
	struct hash spt_hash_table;
};

#include "threads/thread.h"
void supplemental_page_table_init(struct supplemental_page_table *spt);
bool supplemental_page_table_copy(struct supplemental_page_table *dst,
								  struct supplemental_page_table *src);
void supplemental_page_table_kill(struct supplemental_page_table *spt);
struct page *spt_find_page(struct supplemental_page_table *spt,
						   void *va);
bool spt_insert_page(struct supplemental_page_table *spt, struct page *page);
void spt_remove_page(struct supplemental_page_table *spt, struct page *page);

void vm_init(void);
bool vm_try_handle_fault(struct intr_frame *f, void *addr, bool user,
						 bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, struct file_info *file_info);
void vm_dealloc_page(struct page *page);
bool vm_claim_page(void *va);
enum vm_type page_get_type(struct page *page);

/******************** P3: added ***********s*********/
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED);
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);
unsigned frame_hash(const struct hash_elem *f_, void *aux UNUSED);
struct page *page_lookup(const void *va, struct supplemental_page_table *spt);
/******************** P3: added - end ********************/

#endif /* VM_VM_H */
