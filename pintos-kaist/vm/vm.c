/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "../include/userprog/exception.h"
#include "../include/userprog/process.h"
#include "threads/vaddr.h"
#include <stdlib.h>


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
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
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

/* Create the pending page(물리 메모리가 할당되지 않은 대기중인 페이지) object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
// 마지막 void aux file_load_aux로 바꿔보기
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)
	struct supplemental_page_table *spt = &thread_current()->spt;
	/* Check wheter the upage is already occupied or not. */
	// printf("\taddr: %p, name: %s\n", upage, thread_current()->name);
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *new_page = malloc(sizeof(struct page));

		bool (*initializer)(struct page *, enum vm_type, void *kva) = NULL;
		if (new_page == NULL)
		{	
			// printf("sad\n");
			return false;
		}

		switch (VM_TYPE(type))
		{
		case VM_ANON:
		{
			initializer = anon_initializer;
			break;
		}
		case VM_FILE:
		{
			initializer = file_backed_initializer;
			break;
		}
		default:
			free(new_page);
			ASSERT(false);
		}
		uninit_new(new_page, upage, init, type, aux, initializer);
		new_page->writable = writable;

		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page(spt, new_page))
		{
			printf("생성한 uninit 페이지를 spt에 추가하는 과정에서 오류\n");
			return false;
		}

		return true;
	}
err:
	//printf("페이지가 이미 spt에 있음\n");
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page tmp;
	struct hash_elem *tmp_e;
	/* TODO: Fill this function. */
	// 해쉬요소를 찾으려면 key만 넘겨주면 됨.

	tmp.va = pg_round_down(va);
	tmp_e = hash_find(&spt->s_pt, &tmp.hash_elem);

	return tmp_e != NULL ? hash_entry(tmp_e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
// [*]3-o, 해쉬테이블에 요소 추가
// 같은 키를 가진 요소가 없으면 요소에 추가하고 NULL 반환,
// 있으면 삽입하지 않고 이미 있던 요소를 반환
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	int succ = false;
	/* TODO: Fill this function. */
	lock_acquire(&spt->spt_lock);
	struct hash_elem *tmp = hash_insert(&spt->s_pt, &page->hash_elem);
	lock_release(&spt->spt_lock);

	if (tmp == NULL)
	{
		succ = true;
	}
	return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	lock_acquire(&spt->spt_lock);
	struct hash_elem *tmp = hash_delete(&spt->s_pt, &page->hash_elem);
	if (tmp != NULL)
	{
		vm_dealloc_page(page);
	}
	lock_release(&spt->spt_lock);
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = malloc(sizeof(struct frame));
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = malloc(sizeof(struct frame));
	/* TODO: Fill this function. */
	frame->page = NULL;
	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);

	if (frame->kva == NULL)
	{
		// 추후 프레임 확보 알고리즘 구현
		// frame = vm_evict_frame();
		// 일단 지금은 return false로 처리
		printf("메모리 확보 실패\n");
		return false;
	}
	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
void vm_stack_growth(void *addr UNUSED)
{

	// void *target_stk_bot = (void *) ( addr);
	int i = 0; 
	// printf("sad\n");
	char *stack_bottom_growth = pg_round_down(addr);
	uintptr_t origin_stack_bot = thread_current () -> stack_bot;
	
	thread_current () -> stack_bot = (uintptr_t) stack_bottom_growth;
	while (origin_stack_bot > stack_bottom_growth)
	{	
		origin_stack_bot -= (1<<12);
		//printf("stack growth addr: %p, name: %s\n",origin_stack_bot, thread_current()->name);
		if (!vm_alloc_page(VM_ANON , origin_stack_bot, true))
		{
			printf("스택 확장 alloc 실패\n");
			thread_current () -> stack_bot = (origin_stack_bot + (1<<12));
			goto done;
		}
		
		if (!vm_claim_page(origin_stack_bot))
		{
			thread_current () -> stack_bot = (origin_stack_bot + (1<<12));
			printf("스택 확장 claim 실패\n");
			goto done;
		}
		thread_current () -> stack_bot = origin_stack_bot;
		//printf("reps: %d\n", i++);
	}
	
	//printf("asd\n");
	// return true;
done:
	return;
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	// 접근 유효범위 검증은 밖에서 했음. 여기서는 유저 영역에 대한 유효한 접근에 대해서만 처리.
	struct supplemental_page_table *spt = &thread_current()->spt;
	struct page *page = spt_find_page(spt, addr);
	// printf("addr: %p\n",addr);
	if (((uintptr_t)USER_STACK_LIMIT < (uintptr_t)addr) && ((uintptr_t)USER_STACK >= (uintptr_t)addr))
	{	
		if (!user)
		{
			// printf("커널모드에서 유효하지 않은 스택 주소 접근\n");
			sys_exit(-1);
		}

		// printf("stack_bot: %x\n", thread_current()->stack_bot);
		// printf("fault addr in stack: %p\n", addr);
		//printf("rsp: %x\n", f->rsp);
		// printf("stack_bot - fault addr: %p\n", ((thread_current()->stack_bot) - (uintptr_t)addr));
		// printf("rsp - addr: %p\n",  f->rsp - ((uintptr_t)addr));
		ptrdiff_t diff = (f->rsp) - ((uintptr_t)addr);

		if ((diff >= (1<<12)))
		{
			//printf("sad\n");
			sys_exit(-1);
		}
		else
		{	
			vm_stack_growth(addr);
			return true;
		}
	}

	if (page == NULL)
	{
		//printf("페이지 예약정보 없음\n");
		// printf("\taddr: %p\n", addr);
		// printf("\tthread:name %s\n", thread_current()->name);
		// printf("\tchildstack bot: %x\n", thread_current()->stack_bot); 	
		// printf("user?: %d\n", user);
		sys_exit(-1);
		// return false;
	}

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function */

	page = spt_find_page(&(thread_current()->spt), va);

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();
	struct thread *t = thread_current();

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
	// 여기서 다시

	if (swap_in(page, frame->kva))
	{
		if (pml4_get_page(t->pml4, page->va) == NULL && pml4_set_page(t->pml4, page->va, page->frame->kva, page->writable))
		{
			// printf("pml4 매핑 성공:page-> va = %p\n", page->va);
			return true;
		}
		else
		{
			printf("pml4 매핑 실패:page-> va = %p\n", page->va);
			palloc_free_page(page->frame->kva);
			return false;
		}
	}
	else
	{
		printf("파일 읽기 실패\n");
		return false;
	}
}

/* Initialize new supplemental page table */
// [*]3-o, 해쉬테이블 초기화
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->s_pt, page_hash, page_less, NULL);
	lock_init(&spt->spt_lock);
}

/* Copy supplemental page table from src to dst */
// 초기화 관련 함수들(예: 새로운 페이지 엔트리를 위한 uninit 페이지 할당, SPT를 초기화하는 함수 등)을 재사용
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	// dst (자식) , src (부모)
	// 복사하는 방식은 uninit 페이지 확보한 뒤, 바로 claim
	// claim의 의미; 물리페이지 할당 -> 프로세스의 페이지 테이블에 가상주소 <-> 할당한 물리프레임 매핑.

	struct hash_iterator i;
	hash_first(&i, src);
	/*
	enum vm_type type = page_get_type(src_page);
	얘는 uninit페이지가 향후 어떤 페이지로 초기화될지에 대한 정보
	*/

	while (hash_next(&i))
	{
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type type = src_page->operations->type;
		struct page *dst_page;
		switch (type)
		{
		case VM_UNINIT:{
			struct file_load_aux *uninit_aux = malloc(sizeof(struct file_load_aux));
			memcpy(uninit_aux, src_page->uninit.aux, sizeof(struct file_load_aux));
			//printf("\tcopy addr: %p\n", src_page->va);
			if (!vm_alloc_page_with_initializer(src_page->uninit.type, src_page->va, src_page->writable, src_page->uninit.init, uninit_aux))
			{
				printf("uninit copy 실패, 실패주소: %p\n", src_page->va);
				
				return false;
			}
			break;
		}
		case VM_FILE:{
			struct file_load_aux *copy_aux = malloc(sizeof(struct file_load_aux));
			copy_aux->file = src_page->file.file;
			copy_aux->ofs = src_page->file.ofs;
			copy_aux->page_read_bytes = src_page->file.page_read_bytes;
			copy_aux->page_zero_bytes = src_page->file.page_zero_bytes;
			
			if (!vm_alloc_page_with_initializer(VM_FILE, src_page->va, src_page->writable,NULL, copy_aux))
			{
				free(copy_aux);
				return false;
			}

			if (!vm_claim_page(src_page->va))
			{
				free(copy_aux);
				return false;
			}
			// memcpy 안쓰려고 했는데, 메모리에서 SPT를 통해 load하는 방식은 부모의 메모리상태를 반영하지 못해서 정확한 값 복사가안됨.
			dst_page = spt_find_page(dst, src_page->va);
			memcpy(dst_page->frame->kva, src_page->frame->kva, 1 << 12);
			break;
		}
		case VM_ANON:{

			if (!vm_alloc_page(VM_ANON, src_page->va, src_page->writable))
			{
				printf("복사중 할당 실패\n");
				return false;
			}

			if (!vm_claim_page(src_page->va))
			{
				printf("복사중 클레임 실패\n");
				return false;
			}
			// memcpy 안쓰려고 했는데, 메모리에서 SPT를 통해 load하는 방식은 부모의 메모리상태를 반영하지 못해서 정확한 값 복사가안됨.
			dst_page = spt_find_page(dst, src_page->va);
			memcpy(dst_page->frame->kva, src_page->frame->kva, 1 << 12);
			break;
		}
		}
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// supplemental_page_table_init(spt);
	//lock_acquire(&spt->spt_lock);
	hash_destroy(&spt->s_pt, spt_kill_destructor);
	//lock_release(&spt->spt_lock);

}



// [*] 3-o, 해쉬 함수 구현
// 가상주소를 읽어서 해싱

unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
	// 버퍼의 내용을 바이트 단위로 읽어서, 비트를 특정한 규칙에 따른 수학적 연산으로 섞어서, 64비트형 정수로 반환.
}

// 충돌시 버킷
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}

void spt_kill_destructor(struct hash_elem *h, void* aux UNUSED){
	struct page *killll = hash_entry(h, struct page, hash_elem);
	destroy(killll);
	free(killll);
}