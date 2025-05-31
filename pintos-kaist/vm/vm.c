/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page(물리 메모리가 할당되지 않은 대기중인 페이지) object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 마지막 void aux file_load_aux로 바꿔보기
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, struct file_load_aux *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *new_page = malloc(sizeof(struct page));
		bool (*initializer)(struct page *, enum vm_type, void *kva) = NULL;

		if (new_page == NULL){
			return false;
		}
		switch(type){
		case VM_ANON:{
			initializer = anon_initializer;
			break;
		}
		case VM_FILE:{
			initializer = file_backed_initializer;
			break;
		}
		default:
		free(new_page);
		ASSERT(false);
		}

		uninit_new(new_page,upage,init,type,NULL,initializer);
		new_page->writable = writable;

		/* TODO: Insert the page into the spt. */


		if (!spt_insert_page(spt, new_page)){
			printf("페이지 이미 할당되었음\n");
			return false;
		}
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page tmp;
	struct hash_elem *tmp_e;
	/* TODO: Fill this function. */
	// 해쉬요소를 찾으려면 key만 넘겨주면 됨.

	tmp.va = va;
	tmp_e = hash_find(&spt->s_pt,&tmp.hash_elem);

	

	return tmp_e != NULL? hash_entry(tmp_e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
// [*]3-o, 해쉬테이블에 요소 추가
// 같은 키를 가진 요소가 없으면 요소에 추가하고 NULL 반환,
// 있으면 삽입하지 않고 이미 있던 요소를 반환
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	lock_acquire(&spt->spt_lock);
	struct hash_elem *tmp =  hash_insert(&spt->s_pt, &page->hash_elem);
	lock_release(&spt->spt_lock);

	if (tmp == NULL){
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	lock_acquire(&spt->spt_lock);
	struct hash_elem *tmp = hash_delete(&spt->s_pt, &page->hash_elem);
	if (tmp != NULL){
		vm_dealloc_page (page);
	}
	lock_release(&spt->spt_lock);
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// 접근 유효범위(사용자 영역 등 검증)

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* claim = 자원을 확보하는 것
	 page claim이란,
	빈 프레임 하나를 가져오고
	그 프레임을 이 페이지에 연결하고
	페이지 테이블에 이 연결을 등록해서
	이제 CPU가 그 가상주소로 접근하면 실제 
	물리 메모리(프레임)를 바라보도록 만든다는 의미
	 */ 

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
// [*]3-o, 해쉬테이블 초기화
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->s_pt,page_hash,page_less,NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
