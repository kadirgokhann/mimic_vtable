mov   rax, [p]         ; load object
mov   rax, [rax]       ; load vptr
mov   rax, [rax+slot]  ; load function pointer from vtable slot
jmp   rax              ; indirect jump
