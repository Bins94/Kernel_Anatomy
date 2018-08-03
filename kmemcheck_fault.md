# kmemcheck_fault
调用路径：
__do_page_fault -> kmemcheck_fault -> v8086?/KERNEL_CS?/kmemcheck_pte_lookup/kmemcheck_access
kmemcheck_access 代码：
```  
enum kmemcheck_method {
	KMEMCHECK_READ,
	KMEMCHECK_WRITE,
};

static void kmemcheck_access(struct pt_regs *regs,
	unsigned long fallback_address, enum kmemcheck_method fallback_method)
{
	const uint8_t *insn;
	const uint8_t *insn_primary;
	unsigned int size;

	struct kmemcheck_context *data = this_cpu_ptr(&kmemcheck_context);

	if (data->busy) {
		kmemcheck_show_addr(fallback_address);
		kmemcheck_error_save_bug(regs);
		return;
	}

	data->busy = true;

	insn = (const uint8_t *) regs->ip;
	insn_primary = kmemcheck_opcode_get_primary(insn);

	/* 访问地址操作的指令 */
	kmemcheck_opcode_decode(insn, &size);

	/* 根据指令类型处理相应的访问内存 */
	switch (insn_primary[0]) {
#ifdef CONFIG_KMEMCHECK_BITOPS_OK
		/* AND, OR, XOR */
		/*
		 * Unfortunately, these instructions have to be excluded from
		 * our regular checking since they access only some (and not
		 * all) bits. This clears out "bogus" bitfield-access warnings.
		 */
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
		switch ((insn_primary[1] >> 3) & 7) {
			/* OR */
		case 1:
			/* AND */
		case 4:
			/* XOR */
		case 6:
			/* 上述都涉及写操作 */
			kmemcheck_write(regs, fallback_address, size);
			goto out;

			/* ADD */
		case 0:
			/* ADC */
		case 2:
			/* SBB */
		case 3:
			/* SUB */
		case 5:
			/* CMP */
		case 7:
			break;
		}
		break;
#endif

		/* MOVS, MOVSB, MOVSW, MOVSD */
	case 0xa4:
	case 0xa5:
		/*
		 * These instructions are special because they take two
		 * addresses, but we only get one page fault.
		 */
		/* 这些都涉及复制 */
		kmemcheck_copy(regs, regs->si, regs->di, size);
		goto out;

		/* CMPS, CMPSB, CMPSW, CMPSD */
	case 0xa6:
	case 0xa7:
		/* 比较类指令是读操作 */
		kmemcheck_read(regs, regs->si, size);
		kmemcheck_read(regs, regs->di, size);
		goto out;
	}

	/*
	 * If the opcode isn't special in any way, we use the data from the
	 * page fault handler to determine the address and type of memory
	 * access.
	 */
	/* page fault 传递进来的 error code 其中bit-0 是读写位设置 */
	switch (fallback_method) {
	case KMEMCHECK_READ:
		kmemcheck_read(regs, fallback_address, size);
		goto out;
	case KMEMCHECK_WRITE:
		kmemcheck_write(regs, fallback_address, size);
		goto out;
	}

out:
	data->busy = false;
}
```  
下面是 kmemcheck_read 的实现代码：
```  
```  
