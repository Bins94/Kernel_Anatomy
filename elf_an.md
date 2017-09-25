## binelf_load
### bprm->init
```  
int prepare_binprm(struct linux_binprm *bprm)
{
	int retval;

	bprm_fill_uid(bprm);

	/* fill in binprm security blob */
	retval = security_bprm_set_creds(bprm);
	if (retval)
		return retval;
	bprm->cred_prepared = 1;

	memset(bprm->buf, 0, BINPRM_BUF_SIZE);
    /* 调用 vfs_read 加载文件头到 rpbm->buf */
	return kernel_read(bprm->file, 0, bprm->buf, BINPRM_BUF_SIZE); 
}
```  
### load_elf_binary（4.9）
```  
static int load_elf_binary(struct linux_binprm *bprm)
{
	struct file *interpreter = NULL; /* to shut gcc up */
 	unsigned long load_addr = 0, load_bias = 0;
	int load_addr_set = 0;
	char * elf_interpreter = NULL;
	unsigned long error;
	struct elf_phdr *elf_ppnt, *elf_phdata, *interp_elf_phdata = NULL;
	unsigned long elf_bss, elf_brk;
	int retval, i;
	unsigned long elf_entry;
	unsigned long interp_load_addr = 0;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long reloc_func_desc __maybe_unused = 0;
	int executable_stack = EXSTACK_DEFAULT;
	struct pt_regs *regs = current_pt_regs();
	struct {
		struct elfhdr elf_ex;
		struct elfhdr interp_elf_ex;
	} *loc;
	struct arch_elf_state arch_state = INIT_ARCH_ELF_STATE;

	loc = kmalloc(sizeof(*loc), GFP_KERNEL);
	if (!loc) {
		retval = -ENOMEM;
		goto out_ret;
	}
	
	/* Get the exec-header */
	loc->elf_ex = *((struct elfhdr *)bprm->buf); /*这个 buf 是读取 ELF 文件头获得的，在 prepare_binprm 初始化*/

	retval = -ENOEXEC;
	/* First of all, some simple consistency checks */
	if (memcmp(loc->elf_ex.e_ident, ELFMAG, SELFMAG) != 0) /*ELF 魔数，ELFMAG="\177ELF"，SELFMAG=4*/
		goto out;

    /* ELF类型检查 */
	if (loc->elf_ex.e_type != ET_EXEC && loc->elf_ex.e_type != ET_DYN)
		goto out;
    /* 检查e_machine是否与相应架构符合 */
	if (!elf_check_arch(&loc->elf_ex))
		goto out;
    /* 文件操作的确认 */
	if (!bprm->file->f_op->mmap)
		goto out;

    /*申请空间，将elf_phdr*elf_ex->e_phnum读取，从bprm转移出来*/
	elf_phdata = load_elf_phdrs(&loc->elf_ex, bprm->file);
	if (!elf_phdata)
		goto out;

	elf_ppnt = elf_phdata;
	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

        /* 这个循环专门处理解释器加载，解释器加载早于其他段 */
	for (i = 0; i < loc->elf_ex.e_phnum; i++) {
		if (elf_ppnt->p_type == PT_INTERP) {   /* 用于解释器的段 */
			/* This is the program interpreter used for
			 * shared libraries - for now assume that this
			 * is an a.out format binary
			 */
			retval = -ENOEXEC;
            /* 检查文件大小 */
			if (elf_ppnt->p_filesz > PATH_MAX || 
			    elf_ppnt->p_filesz < 2)
				goto out_free_ph;

			retval = -ENOMEM;
			elf_interpreter = kmalloc(elf_ppnt->p_filesz,
						  GFP_KERNEL);
			if (!elf_interpreter)
				goto out_free_ph;

            /* 读取解释器的段 */
			retval = kernel_read(bprm->file, elf_ppnt->p_offset,
					     elf_interpreter,
					     elf_ppnt->p_filesz);
			/* 检查读取长度 */
                        if (retval != elf_ppnt->p_filesz) {
				if (retval >= 0)
					retval = -EIO;
				goto out_free_interp;
			}
			/* 检查字符串结尾，这是解释器的路径，objdump -s 可看到 */
			retval = -ENOEXEC;
			if (elf_interpreter[elf_ppnt->p_filesz - 1] != '\0')
				goto out_free_interp;
            /*解释器本身也是一个ELF，也需要读取ELF的流程*/
			interpreter = open_exec(elf_interpreter);
			retval = PTR_ERR(interpreter);
			if (IS_ERR(interpreter))
				goto out_free_interp;

			/*
			 * If the binary is not readable then enforce
			 * mm->dumpable = 0 regardless of the interpreter's
			 * permissions.
			 */
			would_dump(bprm, interpreter);

			/* 读取解释器文件头 */
			retval = kernel_read(interpreter, 0,
					     (void *)&loc->interp_elf_ex,
					     sizeof(loc->interp_elf_ex));
			if (retval != sizeof(loc->interp_elf_ex)) {
				if (retval >= 0)
					retval = -EIO;
				goto out_free_dentry;
			}

			break;
		}
		elf_ppnt++;
	}

	elf_ppnt = elf_phdata;     /*重新遍历*/
	for (i = 0; i < loc->elf_ex.e_phnum; i++, elf_ppnt++)
		switch (elf_ppnt->p_type) {
		case PT_GNU_STACK:            /*处理栈段*/
			if (elf_ppnt->p_flags & PF_X)        /*p_flags决定了能否执行*/
				executable_stack = EXSTACK_ENABLE_X;
			else
				executable_stack = EXSTACK_DISABLE_X;
			break;

		case PT_LOPROC ... PT_HIPROC:   /*保留地址，空实现*/
			retval = arch_elf_pt_proc(&loc->elf_ex, elf_ppnt,
						  bprm->file, false,
						  &arch_state);
			if (retval)
				goto out_free_dentry;
			break;
		}

	/* 解释器的检查，类似前面读取 ELF 的那些检查 */
	if (elf_interpreter) {
		retval = -ELIBBAD;
		/* ELF 魔数的检查 */
		if (memcmp(loc->interp_elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
			goto out_free_dentry;
		/* 检查e_machine */
		if (!elf_check_arch(&loc->interp_elf_ex))
			goto out_free_dentry;

		/* 读取解释器的ELF头部 */
		interp_elf_phdata = load_elf_phdrs(&loc->interp_elf_ex,
						   interpreter);
		if (!interp_elf_phdata)
			goto out_free_dentry;

		/* PT_LOPROC..PT_HIPROC 部分是空实现 */
		elf_ppnt = interp_elf_phdata;
		for (i = 0; i < loc->interp_elf_ex.e_phnum; i++, elf_ppnt++)
			switch (elf_ppnt->p_type) {
			case PT_LOPROC ... PT_HIPROC:
				retval = arch_elf_pt_proc(&loc->interp_elf_ex,
							  elf_ppnt, interpreter,
							  true, &arch_state);
				if (retval)
					goto out_free_dentry;
				break;
			}
	}

	/*空实现*/
	retval = arch_check_elf(&loc->elf_ex,
				!!interpreter, &loc->interp_elf_ex,
				&arch_state);
	if (retval)
		goto out_free_dentry;

	/* 更新bprm，一些继承自父进程的被刷新 */
	retval = flush_old_exec(bprm);
	if (retval)
		goto out_free_dentry;

	/* Do this immediately, since STACK_TOP as used in setup_arg_pages
	   may depend on the personality.  */
        /*这个字段似乎是用于兼容UNIX的设计，这里有很多用途，和栈的可执行和随机化有关*/
	SET_PERSONALITY2(loc->elf_ex, &arch_state);
	if (elf_read_implies_exec(loc->elf_ex, executable_stack))
		current->personality |= READ_IMPLIES_EXEC;

	if (!(current->personality & ADDR_NO_RANDOMIZE) && randomize_va_space)
		current->flags |= PF_RANDOMIZE;

	setup_new_exec(bprm);
	install_exec_creds(bprm);

	/* 设置一些执行参数，注意这里用了随机化栈顶 */
	retval = setup_arg_pages(bprm, randomize_stack_top(STACK_TOP),
				 executable_stack);
	if (retval < 0)
		goto out_free_dentry;
	
	current->mm->start_stack = bprm->p;     /*初始化栈起点，栈加入第一个随机化*/

	/* Now we do a little grungy work by mmapping the ELF image into
	   the correct location in memory. */
        /* 到这里，我们开始正式将 ELF 镜像映射进内存 */
	for(i = 0, elf_ppnt = elf_phdata;
	    i < loc->elf_ex.e_phnum; i++, elf_ppnt++) {
		int elf_prot = 0, elf_flags;
		unsigned long k, vaddr;
		unsigned long total_size = 0;

		if (elf_ppnt->p_type != PT_LOAD)
			continue;

		if (unlikely (elf_brk > elf_bss)) {
			unsigned long nbyte;
	            
			/* There was a PT_LOAD segment with p_memsz > p_filesz
			   before this one. Map anonymous pages, if needed,
			   and clear the area.  */
			retval = set_brk(elf_bss + load_bias,
					 elf_brk + load_bias);
			if (retval)
				goto out_free_dentry;
			nbyte = ELF_PAGEOFFSET(elf_bss);
			if (nbyte) {
				nbyte = ELF_MIN_ALIGN - nbyte;
				if (nbyte > elf_brk - elf_bss)
					nbyte = elf_brk - elf_bss;
				if (clear_user((void __user *)elf_bss +
							load_bias, nbyte)) {
					/*
					 * This bss-zeroing can fail if the ELF
					 * file specifies odd protections. So
					 * we don't check the return value
					 */
				}
			}
		}

                /*一些映射内存性质的设置*/
		if (elf_ppnt->p_flags & PF_R)
			elf_prot |= PROT_READ;
		if (elf_ppnt->p_flags & PF_W)
			elf_prot |= PROT_WRITE;
		if (elf_ppnt->p_flags & PF_X)
			elf_prot |= PROT_EXEC;

		elf_flags = MAP_PRIVATE | MAP_DENYWRITE | MAP_EXECUTABLE;

                /*读取段虚拟地址*/
		vaddr = elf_ppnt->p_vaddr;
                /* 处理可执行段 */
		if (loc->elf_ex.e_type == ET_EXEC || load_addr_set) {
                        /* 可执行段 */
			elf_flags |= MAP_FIXED;
		} else if (loc->elf_ex.e_type == ET_DYN) {
			/* 动态库类型段 */
            /* 这个变量是下文会用到的映射基址， ELF_ET_DYN_BASE=(TASK_SIZE / 3 * 2) */
            /* 调用 elf_map 时会加上相应的地址 */
			load_bias = ELF_ET_DYN_BASE - vaddr;  
			/* 根据标志确定随机化映射基址与否 */
            if (current->flags & PF_RANDOMIZE)
				load_bias += arch_mmap_rnd();
                         /* 内存地址对齐 */
			load_bias = ELF_PAGESTART(load_bias);
			total_size = total_mapping_size(elf_phdata,
							loc->elf_ex.e_phnum);
			if (!total_size) {
				retval = -EINVAL;
				goto out_free_dentry;
			}
		}
        /* 映射相应段 */
		error = elf_map(bprm->file, load_bias + vaddr, elf_ppnt,
				elf_prot, elf_flags, total_size);
        /* 映射完的地址检查 */
		if (BAD_ADDR(error)) {
			retval = IS_ERR((void *)error) ?
				PTR_ERR((void*)error) : -EINVAL;
			goto out_free_dentry;
		}

		if (!load_addr_set) {
			load_addr_set = 1;
			load_addr = (elf_ppnt->p_vaddr - elf_ppnt->p_offset);
			if (loc->elf_ex.e_type == ET_DYN) {
				load_bias += error -
				             ELF_PAGESTART(load_bias + vaddr);
				load_addr += load_bias;
				reloc_func_desc = load_bias;
			}
		}
		k = elf_ppnt->p_vaddr;
		if (k < start_code)
			start_code = k;
		if (start_data < k)
			start_data = k;

		/* 检查内存和文件大小，防止溢出 */
		if (BAD_ADDR(k) || elf_ppnt->p_filesz > elf_ppnt->p_memsz ||
		    elf_ppnt->p_memsz > TASK_SIZE ||
		    TASK_SIZE - elf_ppnt->p_memsz < k) {
			/* set_brk can never work. Avoid overflows. */
			retval = -EINVAL;
			goto out_free_dentry;
		}
                /* 长度检查通过，将k指向段末 */
		k = elf_ppnt->p_vaddr + elf_ppnt->p_filesz;

		if (k > elf_bss)
			elf_bss = k;
		if ((elf_ppnt->p_flags & PF_X) && end_code < k)
			end_code = k;
		if (end_data < k)
			end_data = k;
		k = elf_ppnt->p_vaddr + elf_ppnt->p_memsz;
		/* 指向堆末尾 */
                if (k > elf_brk)
			elf_brk = k;
	}

        /* 一些偏移更新 */
	loc->elf_ex.e_entry += load_bias;
	elf_bss += load_bias;
	elf_brk += load_bias; 
	start_code += load_bias;
	end_code += load_bias;
	start_data += load_bias;
	end_data += load_bias;

	/* Calling set_brk effectively mmaps the pages that we need
	 * for the bss and break sections.  We must do this before
	 * mapping in the interpreter, to make sure it doesn't wind
	 * up getting placed where the bss needs to go.
	 */
	retval = set_brk(elf_bss, elf_brk);
	if (retval)
		goto out_free_dentry;
	if (likely(elf_bss != elf_brk) && unlikely(padzero(elf_bss))) {
		retval = -EFAULT; /* Nobody gets to see this, but.. */
		goto out_free_dentry;
	}

    /* 若不需要解释器，直接用 ELF 格式里的 entry */
	if (elf_interpreter) {
		unsigned long interp_map_addr = 0;
    /* 获取进程入口，像加载其他 ELF 一样加载解释器，但有单独的函数 */
		elf_entry = load_elf_interp(&loc->interp_elf_ex,
					    interpreter,
					    &interp_map_addr,
					    load_bias, interp_elf_phdata);
		if (!IS_ERR((void *)elf_entry)) {
			/*
			 * load_elf_interp() returns relocation
			 * adjustment
			 */
			interp_load_addr = elf_entry;
			elf_entry += loc->interp_elf_ex.e_entry;
		}
		if (BAD_ADDR(elf_entry)) {
			retval = IS_ERR((void *)elf_entry) ?
					(int)elf_entry : -EINVAL;
			goto out_free_dentry;
		}
		reloc_func_desc = interp_load_addr;

		allow_write_access(interpreter);
		fput(interpreter);
		kfree(elf_interpreter);
	} else {
		elf_entry = loc->elf_ex.e_entry;
		if (BAD_ADDR(elf_entry)) {
			retval = -EINVAL;
			goto out_free_dentry;
		}
	}

	kfree(interp_elf_phdata);
	kfree(elf_phdata);

	set_binfmt(&elf_format);

#ifdef ARCH_HAS_SETUP_ADDITIONAL_PAGES
	retval = arch_setup_additional_pages(bprm, !!elf_interpreter);
	if (retval < 0)
		goto out;
#endif /* ARCH_HAS_SETUP_ADDITIONAL_PAGES */

	retval = create_elf_tables(bprm, &loc->elf_ex,
			  load_addr, interp_load_addr);
	if (retval < 0)
		goto out;
	/* N.B. passed_fileno might not be initialized? */
        /* 更新一些 current 的参数 */
	current->mm->end_code = end_code;
	current->mm->start_code = start_code;
	current->mm->start_data = start_data;
	current->mm->end_data = end_data;
	current->mm->start_stack = bprm->p;

        /* 堆增长基址随机化，randomize_va_space在proc中有接口 */
	if ((current->flags & PF_RANDOMIZE) && (randomize_va_space > 1)) {
		current->mm->brk = current->mm->start_brk =
			arch_randomize_brk(current->mm);
#ifdef compat_brk_randomized
		current->brk_randomized = 1;
#endif
	}
        /* 将 page-0 映射为只读，一些应用依赖这一特性，man personality可查看 */
	if (current->personality & MMAP_PAGE_ZERO) {
		error = vm_mmap(NULL, 0, PAGE_SIZE, PROT_READ | PROT_EXEC,
				MAP_FIXED | MAP_PRIVATE, 0);
	}

#ifdef ELF_PLAT_INIT
	/*
	 * The ABI may specify that certain registers be set up in special
	 * ways (on i386 %edx is the address of a DT_FINI function, for
	 * example.  In addition, it may also specify (eg, PowerPC64 ELF)
	 * that the e_entry field is the address of the function descriptor
	 * for the startup routine, rather than the address of the startup
	 * routine itself.  This macro performs whatever initialization to
	 * the regs structure is required as well as any relocations to the
	 * function descriptor entries when executing dynamically links apps.
	 */
	ELF_PLAT_INIT(regs, reloc_func_desc);
#endif
     /* 作为进程开始调度 */
	start_thread(regs, elf_entry, bprm->p);
	retval = 0;
out:
	kfree(loc);     /* 释放临时结构体 */
out_ret:
	return retval;

	/* error cleanup */
out_free_dentry:
	......
out_free_interp:
	......
out_free_ph:
	......
}
```  
