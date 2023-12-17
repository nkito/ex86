        .arch i8086,jumps
        .code16
        .att_syntax prefix

	.section .poweron, "ax"

        .global reset_entry

reset_entry:
	ljmp	$0xf000, $0

