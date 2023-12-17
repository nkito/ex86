OUTPUT_FORMAT("ihex")
ENTRY(_start)

SECTIONS
{
	. = SEGMENT_START ("text-segment", 0x000);

	.text : {
		__stext = .;
		*(.text) *(.text.*)
		__etext = .;
	}
	__ltext = __etext - __stext;

	.data : {
		__sdata = .;

		*(.rodata) *(.rodata.*)
		*(.data) *(.data.*)
		__edata = .;
	}

	.bss (NOLOAD) : {
		 __sbss = .;
                *(.bss) *(.bss.*)
                __ebss = .;
	}

	__ldata = __edata - __sdata;
	__lbss0 = __ebss - __sbss;
	__lbss1 = __lbss0 + 1;
	__lbss = __lbss1 / 2;

	. = SEGMENT_START ("text-segment", 0xfff0);

	.poweron : {
		*(.poweron);
	}
}
