#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define PHYS_CONTIG_MAX_PAGES 4096

SYSCALL_DEFINE4(phys_contig,
		const void __user *, vaddr,
		size_t, len,
		u8 __user *, out,
		size_t, out_len)
{
	unsigned long uaddr = (unsigned long)vaddr;
	unsigned long start = uaddr & PAGE_MASK;
	unsigned long end, npages, nbits, nbytes;
	struct page **pages;
	u8 *kbuf;
	long ret;

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

	if (copy_to_user(out, kbuf, nbytes))
		ret = -EFAULT;
	else
		ret = 0;

	kfree(kbuf);
out_free_pages:
	kfree(pages);
	return ret;
}
