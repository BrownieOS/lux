
;; lux OS kernel
;; copyright (c) 2018 by Omar Mohammad

format elf
use32

section '.multiboot'

MULTIBOOT_MAGIC			= 0x1BADB002
MULTIBOOT_FLAGS			= 0x00000003	; page-align modules, E820 map
MULTIBOOT_CHECKSUM		= -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

multiboot_magic			dd MULTIBOOT_MAGIC
multiboot_flags			dd MULTIBOOT_FLAGS
multiboot_checksum		dd MULTIBOOT_CHECKSUM

saved_magic			dd 0
saved_structure			dd 0

section '.text'

; start:
; Start of kernel code
public start
start:
	cli
	cld

	lgdt [gdtr]
	jmp 0x08:.next

.next:
	mov dx, 0x10
	mov ss, dx
	mov ds, dx
	mov es, dx
	mov fs, dx
	mov gs, dx
	mov esp, stack_top

	; save multiboot information
	mov [saved_magic], eax
	mov [saved_structure], ebx

	mov al, 0xFF
	out 0x21, al
	out 0xA1, al

	; copy 16-bit VBE driver to low memory
	mov esi, vbe_driver
	mov edi, 0x1000
	mov ecx, end_vbe_driver - vbe_driver
	rep movsb

	; return to 16-bit mode to set VBE mode
	jmp 0x28:.next16

use16
.next16:
	mov ax, 0x30
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov eax, cr0
	and eax, not 1
	mov cr0, eax

	; pass the return address in EBX
	; the 16-bit program will return back to this point, in 32-bit mode
	mov ebx, .vbe_finish
	jmp 0x0000:0x1000

use32
.vbe_finish:
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov esp, stack_top

	; load IDT
	lidt [idtr]

	; reset EFLAGS
	push 0x00000002
	popfd

	; enable SSE
	mov eax, 0x600
	mov cr4, eax

	mov eax, cr0
	and eax, 0xFFFFFFFB
	or eax, 2
	mov cr0, eax

	; initialize the FPU
	finit
	fwait

	push ebx		; VBE mode info block

	mov ebx, [saved_structure]
	mov eax, [saved_magic]
	push ebx		; multiboot information
	push eax		; multiboot magic

	extrn kmain
	call kmain		; void kmain(mb_magic, mb_info, vbe_info);

	; we should never, EVER be here!
.halt:
	cli
	hlt
	jmp .halt


section '.rodata' align 16

; TRAMPOLINE CODE FOR SMP
; GOES IN RODATA BECAUSE WE TREAT IT LIKE DATA FOR TWO REASONS
; 1) We copy it to low memory
; 2) It's 16-bit code

public trampoline16
trampoline16:
use16
	xor ax, ax
	mov ds, ax
	mov es, ax

	; create GDT
	mov word[gdtr16.limit], end_of_gdt - gdt - 1
	mov dword[gdtr16.base], gdt

	lgdt [gdtr16.limit]

	; protected mode
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	jmp 0x08:0x1200

times 512 - ($-trampoline16) db 0

trampoline32:
use32
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov esp, ap_stack_top

	; load IDT
	lidt [idtr]

	; reset EFLAGS
	push 0x00000002
	popfd

	; enable SSE
	mov eax, 0x600
	mov cr4, eax

	mov eax, cr0
	and eax, 0xFFFFFFFB
	or eax, 2
	mov cr0, eax

	; initialize the FPU
	finit
	fwait

	; paging
	extrn page_directory
	mov eax, [page_directory]
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	or eax, 0x10000
	and eax, not 0x60000000
	mov cr0, eax

	; allocate temporary stack
	extrn kmalloc
	push 8192
	mov eax, kmalloc
	call eax

	mov esp, eax
	add esp, 8192

	extrn smp_kmain
	jmp 0x08:smp_kmain

; These pointers already exist below, but we need a copy in low memory for the APs
align 16
gdtr16:
	.limit		= 0x2000
	.base		= 0x2002

end_trampoline16:

public trampoline16_size
trampoline16_size:		dw end_trampoline16 - trampoline16

; gdt:
; Global Descriptor Table
align 16
gdt:
	; 0x00 -- null descriptor
	dq 0

	; 0x08 -- kernel code descriptor
	dw 0xFFFF				; limit low
	dw 0					; base low
	db 0					; base middle
	db 10011010b				; access
	db 11001111b				; flags and limit high
	db 0					; base high

	; 0x10 -- kernel data descriptor
	dw 0xFFFF
	dw 0
	db 0
	db 10010010b
	db 11001111b
	db 0

	; 0x18 -- user code descriptor
	dw 0xFFFF				; limit low
	dw 0					; base low
	db 0					; base middle
	db 11111010b				; access
	db 11001111b				; flags and limit high
	db 0					; base high

	; 0x20 -- user data descriptor
	dw 0xFFFF
	dw 0
	db 0
	db 11110010b
	db 11001111b
	db 0

	; 0x28 -- 16-bit code descriptor
	dw 0xFFFF
	dw 0
	db 0
	db 10011010b
	db 10001111b
	db 0

	; 0x30 -- 16-bit data descriptor
	dw 0xFFFF
	dw 0
	db 0
	db 10010010b
	db 10001111b
	db 0

	; 0x38 -- TSS descriptor
	dw 104
	dw 0
	db 0
	db 11101001b
	db 0
	db 0

public end_of_gdt
end_of_gdt:

; gdtr:
; GDT Pointer
align 16
gdtr:
	.size			dw end_of_gdt - gdt - 1
	.base			dd gdt

; idt:
; Interrupt Descriptor Table
align 16
public idt
idt:
	times 256 dw 0, 8, 0x8E00, 0
public end_of_idt
end_of_idt:

; idtr:
; IDT Pointer
align 16
idtr:
	.size			dw end_of_idt - idt - 1
	.base			dd idt

; VBE Driver
vbe_driver:			file "vbe.sys"
end_vbe_driver:

; Boot-time Font
public bootfont
bootfont:			file "kernel/asm_i386/cp437.bin"

section '.bss' align 16

align 16
stack_bottom:			times 32768*2 db 0
stack_top:

; Temporary stack for APs
align 16
ap_stack_bottom:		times 8192 db 0
ap_stack_top:


