/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize(struct page *page, void *kva);
static void uninit_destroy(struct page *page);

/* DO NOT MODIFY this struct
 * uninit page의 페이지 연산 구조체 */
static const struct page_operations uninit_ops = {
    .swap_in = uninit_initialize,
    .swap_out = NULL,
    .destroy = uninit_destroy,
    .type = VM_UNINIT,
};

/* DO NOT MODIFY this function
 * uninit page 구조체 필드를 초기화하는 함수 */
void uninit_new(struct page *page, void *va, vm_initializer *init, enum vm_type type, void *aux, bool (*initializer)(struct page *, enum vm_type, void *))
{
    // printf("---------------------------------uninit_new 들어왔음\n");

    ASSERT(page != NULL);
    // ASSERT(va != NULL);
    // ASSERT(init != NULL);
    // ASSERT(aux != NULL);
    // ASSERT(initializer != NULL);

    *page = (struct page){.operations = &uninit_ops,
                          .va = va,
                          .frame = NULL, /* no frame for now */
                          .uninit = (struct uninit_page){
                              .init = init,
                              .type = type,
                              .aux = aux,
                              .page_initializer = initializer,
                          }};
}

/* Initalize the page on first fault
 * uninit page를 anon, file, page_cache 중 하나로 바꿈! */
static bool uninit_initialize(struct page *page, void *kva)
{
    printf("uninit_initialize\n");

    struct uninit_page *uninit = &page->uninit;

    /* Fetch first, page_initialize may overwrite the values */
    vm_initializer *init = uninit->init; // lazy_load_segment()
    void *aux = uninit->aux;             // file_info

    /******************* P3: modified *******************/
    /* TODO: You may need to fix this function.
     * type에 맞게 이니셜라이저 실행(file_backed_initializer, anon_initializer)
     * + init(=lazy_load_segment) 호출 */

    return uninit->page_initializer(page, uninit->type, kva) && (init ? init(page, aux) : false); // 만약 초기화 실패하면?

    /******************* P3: modified - end *******************/
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. */
static void uninit_destroy(struct page *page)
{
    struct uninit_page *uninit UNUSED = &page->uninit;
    /* TODO: Fill this function.
     * TODO: If you don't have anything to do, just return. */
}
