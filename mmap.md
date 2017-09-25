# mmap kernel path


```  
unsigned long unmapped_area_topdown(struct vm_unmapped_area_info *info)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long length, low_limit, high_limit, gap_start, gap_end;

	/* 长度对齐 */
	length = info->length + info->align_mask;
	if (length < info->length)
		return -ENOMEM;

	/* 设置搜索区域 */
	gap_end = info->high_limit;
	if (gap_end < length)
		return -ENOMEM;
	high_limit = gap_end - length;

	if (info->low_limit > high_limit)
		return -ENOMEM;
	low_limit = info->low_limit + length;

        /* 到这里，high_limit 是 vm_start 的下界，low_limit 是 vm_end 的下界 */

	/* 大于 highest_vm_end 似乎是高端内存的部分 */
	gap_start = mm->highest_vm_end;
	if (gap_start <= high_limit)
		goto found_highest;

	/* 红黑树检查 */
	if (RB_EMPTY_ROOT(&mm->mm_rb))
		return -ENOMEM;
        /* 红黑树入口，若第一个长度则不足，说明内存不足 */
	vma = rb_entry(mm->mm_rb.rb_node, struct vm_area_struct, vm_rb);
	if (vma->rb_subtree_gap < length)
		return -ENOMEM;

	while (true) {
		/* 在红黑树里搜寻 */
		gap_start = vma->vm_prev ? vma->vm_prev->vm_end : 0;
		if (gap_start <= high_limit && vma->vm_rb.rb_right) {
			struct vm_area_struct *right =
				rb_entry(vma->vm_rb.rb_right,
					 struct vm_area_struct, vm_rb);
			if (right->rb_subtree_gap >= length) {
				vma = right;
				continue;
			}
		}
	/* 注意寻找结束的时候，是一个空隙刚好够大的区域，这个区域是gap_start~gap_end */

check_current:
		/* 这里需要保证，gap_end 要大于 low_limit 否则区域越过 mmap_base */
		gap_end = vma->vm_start;
		if (gap_end < low_limit)
			return -ENOMEM;
		/* 这里需要保证，gap_start 要小于 high_limit 否则区域越过 TASK_SIZE，并且空隙长度够 */
		if (gap_start <= high_limit && gap_end - gap_start >= length)
			goto found;

		/* Visit left subtree if it looks promising */
		if (vma->vm_rb.rb_left) {
			struct vm_area_struct *left =
				rb_entry(vma->vm_rb.rb_left,
					 struct vm_area_struct, vm_rb);
			if (left->rb_subtree_gap >= length) {
				vma = left;
				continue;
			}
		}

		/* Go back up the rbtree to find next candidate node */
		while (true) {
			struct rb_node *prev = &vma->vm_rb;
			if (!rb_parent(prev))
				return -ENOMEM;
			vma = rb_entry(rb_parent(prev),
				       struct vm_area_struct, vm_rb);
			if (prev == vma->vm_rb.rb_right) {
				gap_start = vma->vm_prev ?
					vma->vm_prev->vm_end : 0;
				goto check_current;
			}
		}
	}

found:
	/* We found a suitable gap. Clip it with the original high_limit. */
	if (gap_end > info->high_limit)
		gap_end = info->high_limit;

found_highest:
	/* 重新计算出 gap 的起始地址，包括一些对齐，这里的地址并不一定是 gap_starrt */
	gap_end -= info->length;
	gap_end -= (gap_end - info->align_offset) & info->align_mask;

	VM_BUG_ON(gap_end < info->low_limit);
	VM_BUG_ON(gap_end < gap_start);
	return gap_end;
}
```  
