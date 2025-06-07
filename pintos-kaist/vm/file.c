/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "../include/userprog/process.h"


static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	// ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (addr) == 0);
	ASSERT (offset % PGSIZE == 0);
	uint32_t read_bytes, zero_bytes;

	size_t file__length = file_length(file);

	if (file__length >= length){
		read_bytes = length;
		zero_bytes = 0;
	}else{
		read_bytes = file__length;
		zero_bytes = length - file__length;
	}


	void* ret_addr = addr;

	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		/* 전달된 read 바이트가 pgsize(4KB)보다 크면, pgsize만큼만 읽고 다음 루프에서 처리
		작으면 전달된 read 바이트만큼만 읽고 나머지는 0으로 채우기 */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		off_t cur_ofs = offset;

		if (page_zero_bytes > zero_bytes)         // ★ 핵심
    	 	page_zero_bytes = zero_bytes;
		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		// [*]3-o, 페이지 예약 anon->file 로 변경(취소)
		// 파일 로드에 필요한 정보들 전달
		
		struct file_load_aux* load_aux = malloc(sizeof(struct file_load_aux));
		load_aux->file = file;
		load_aux->ofs = cur_ofs;
		load_aux->page_read_bytes = page_read_bytes;
		load_aux->page_zero_bytes = page_zero_bytes;

		// printf("\t page_read-bytes: %d\n", page_read_bytes);
		// printf("\t page_zero-bytes: %d\n", page_zero_bytes);
		// printf("\t cur_ofs %d\n", cur_ofs);
		// void *aux = load_aux;
		if (!vm_alloc_page_with_initializer (VM_FILE, addr,
					writable, lazy_load_segment, load_aux)){
					free(load_aux);
					return NULL;
					}


		/* Advance. */
		offset += page_read_bytes;
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
	}
	return ret_addr;

}

/* Do the munmap */
void
do_munmap (void *addr) {
	
}
