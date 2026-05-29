#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#define PHYS_CONTIG_MAX_PAGES 4096

SYSCALL_DEFINE4(phys_contig,
		const void __user *, vaddr,
		size_t, len,
		u8 __user *, out,
		size_t, out_len)
{
	unsigned long uaddr = (unsigned long)vaddr;
	unsigned long start = uaddr & PAGE_MASK;
	unsigned long end, npages, nbits, nbytes, i;
	struct page **pages;
	u8 *kbuf;
	long pinned, ret;

	if (len == 0)
		return -EINVAL;

	if (uaddr + len < uaddr)
		return -EINVAL;

	end = (uaddr + len - 1) & PAGE_MASK;
	npages = ((end - start) >> PAGE_SHIFT) + 1;

	if (npages > PHYS_CONTIG_MAX_PAGES)
		return -EINVAL;

	if (npages == 1)
		return 0;

	nbits = npages - 1;
	nbytes = (nbits + 7) >> 3;

	if (nbytes > out_len)
		return -EINVAL;

	pages = kmalloc_array(npages, sizeof(*pages), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	kbuf = kzalloc(nbytes, GFP_KERNEL);
	if (!kbuf) {
		ret = -ENOMEM;
		goto out_free_pages;
	}

	mmap_read_lock(current->mm);
	pinned = get_user_pages(start, npages, 0, pages);
	mmap_read_unlock(current->mm);

	if (pinned < 0) {
		ret = pinned;
		goto out_free_buf;
	}
	if (pinned != npages) {
		ret = -EFAULT;
		goto out_put;
	}

	for (i = 0; i < nbits; i++) {
		if (page_to_pfn(pages[i + 1]) == page_to_pfn(pages[i]) + 1)
			kbuf[i >> 3] |= 1u << (i & 7);
	}

	if (copy_to_user(out, kbuf, nbytes))
		ret = -EFAULT;
	else
		ret = 0;

out_put:
	while (pinned--)
		put_page(pages[pinned]);
out_free_buf:
	kfree(kbuf);
out_free_pages:
	kfree(pages);
	return ret;
}
