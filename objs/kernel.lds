OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start);
SECTIONS
{
	. = 0x80030000;
	start = .;
	stext = .;
	.text : 
	{
		*(.text.entry)
		*(.text .text.*)
	}
	etext = .;

	sdata = .;
	.data : 
	{ 
		*(.data .data.*) 
	}
	edata = .;

	srodata = .;
	.rodata : {
		*(.rodata .rodata.*)
	}
	erodata = .;
	
	sbss = .;
	.bss  : 
	{ 
		*(.bss .bss.*) 
	}
	ebss = .;
	end = .;
}
