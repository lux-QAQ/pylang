Address	Source Line	Assembly	CPU Time: Total	CPU Time: Self
0x42090	105	pushq  %r15	0.1%	0.024s
0x42092	105	pushq  %r14		
0x42094	105	pushq  %r13		
0x42096	105	pushq  %r12		
0x42098	105	pushq  %rbx		
0x42099	105	sub $0x10, %rsp		
0x4209d	105	mov %rsi, %rbx		
0x420a0	105	mov %rdi, %r14		
0x420a3	0	test $0x1, %r14b	0.1%	0.020s
0x420a7	0	jnz 0x4211c <Block 9>		
0x420a9	0	Block 2:		
0x420a9	0	mov %r14, %r15		
0x420ac	0	test $0x1, %bl		
0x420af	0	jz 0x4212b <Block 11>		
0x420b1	0	Block 3:		
0x420b1	0	movzxb  0x2286490(%rip), %eax		
0x420b8	0	test %al, %al		
0x420ba	0	jz 0x42230 <Block 33>		
0x420c0	0	Block 4:		
0x420c0	0	movq  0x8(%r14), %rax		
0x420c4	0	mov %r14, %r15		
0x420c7	0	cmpq  0x2286472(%rip), %rax		
0x420ce	0	jnz 0x4212b <Block 11>	0.1%	0.024s
0x420d0	0	Block 5:		
0x420d0	0	mov %rbx, %rdi		
0x420d3	0	sar $0x1, %rdi		
0x420d6	0	movq  0x30(%r14), %rax		
0x420da	0	movq  0x38(%r14), %rcx		
0x420de	0	sub %rax, %rcx		
0x420e1	0	sar $0x3, %rcx		
0x420e5	0	movq  0x48(%r14), %rdx	0.0%	0.012s
0x420e9	0	sub %rdx, %rcx		
0x420ec	0	mov %rbx, %rsi	0.0%	0.008s
0x420ef	0	sar $0x3f, %rsi		
0x420f3	0	and %rcx, %rsi		
0x420f6	0	add %rdi, %rsi		
0x420f9	0	mov %r14, %r15	0.1%	0.032s
0x420fc	0	js 0x4212b <Block 11>		
0x420fe	0	Block 6:		
0x420fe	0	mov %r14, %r15	0.1%	0.024s
0x42101	0	cmp %rcx, %rsi		
0x42104	0	jnl 0x4212b <Block 11>		
0x42106	0	Block 7:		
0x42106	0	lea (%rax,%rdx,8), %rax		
0x4210a	0	movq  (%rax,%rsi,8), %rax		
0x4210e	0	Block 8:		
0x4210e	122	add $0x10, %rsp	9.5%	2.539s
0x42112	122	popq  %rbx		
0x42113	122	popq  %r12		
0x42115	122	popq  %r13	0.0%	0.008s
0x42117	122	popq  %r14		
0x42119	122	popq  %r15		
0x4211b	122	retq  		
0x4211c	0	Block 9:		
0x4211c	0	movq  %r14, (%rsp)		



Source Line	Source	CPU Time: Total	CPU Time: Self
103	PYLANG_EXPORT_SUBSCR("list_getitem_i64", "obj", "obj,obj")		
104	py::PyObject *rt_list_getitem_i64(py::PyObject *list, py::PyObject *index)		
105	{	0.1%	0.024s
106	    py::PyObject *result = nullptr;		
107			
108	    if (py::rt::exact_list_index_hit(list, index, [&result](py::PyList *py_list, int64_t idx) {		
109	            result = py_list->unchecked_at(static_cast<size_t>(idx)).as_pyobject_raw();		
110	        })) {		
111	        return result;		
112	    }		
113			
114	    if (py::rt::exact_list_index(list, index, [&result](py::PyList *py_list, int64_t idx) {		
115	            result = py_list->unchecked_at(static_cast<size_t>(idx)).as_pyobject_raw();		
116	            return true;		
117	        })) {		
118	        return result;		
119	    }		
120	    // 回退到通用路径		
121	    return rt_unwrap(py::ensure_box(list)->getitem(py::ensure_box(index)));	1.4%	0s
122	}	9.6%	2.547s



Source Line	Source	CPU Time: Total	CPU Time: Self
77	PyObject *py_true()		
78	{	1.2%	0.315s
79	    static PyObject *value = nullptr;		
80	    if (!value) {	0.6%	0.155s
81	        // 纵深防御：确保分配在 program_arena		
82	        Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;		
83	        Arena::set_current(&ArenaManager::program_arena());		
84	        value = PyBool::create(true).unwrap();		
85	        if (saved) Arena::set_current(saved);		
86	    }		
87	    return value;	4.1%	1.084s
88	}		


Source Line	Source	CPU Time: Total	CPU Time: Self
77	PyObject *py_true()		
78	{	1.2%	0.315s
79	    static PyObject *value = nullptr;		
80	    if (!value) {	0.6%	0.155s
81	        // 纵深防御：确保分配在 program_arena		
82	        Arena *saved = Arena::has_current() ? &Arena::current() : nullptr;		
83	        Arena::set_current(&ArenaManager::program_arena());		
84	        value = PyBool::create(true).unwrap();		
85	        if (saved) Arena::set_current(saved);		
86	    }		
87	    return value;	4.1%	1.084s
88	}		