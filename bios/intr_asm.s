
// i186 is specified in this file because POPA and PUSHA are used in a error handling function

	.arch i186,jumps
	.code16 
	.att_syntax prefix 
	.text

.global _int0x00_default_handler
_int0x00_default_handler:
	movw $0x00, %cs:intr_num
	jmp _intr_default_handler

.global _int0x01_default_handler
_int0x01_default_handler:
	movw $0x01, %cs:intr_num
	jmp _intr_default_handler

.global _int0x02_default_handler
_int0x02_default_handler:
	movw $0x02, %cs:intr_num
	jmp _intr_default_handler

.global _int0x03_default_handler
_int0x03_default_handler:
	movw $0x03, %cs:intr_num
	jmp _intr_default_handler

.global _int0x04_default_handler
_int0x04_default_handler:
	movw $0x04, %cs:intr_num
	jmp _intr_default_handler

.global _int0x05_default_handler
_int0x05_default_handler:
	movw $0x05, %cs:intr_num
	jmp _intr_default_handler

.global _int0x06_default_handler
_int0x06_default_handler:
	movw $0x06, %cs:intr_num
	jmp _intr_default_handler

.global _int0x07_default_handler
_int0x07_default_handler:
	movw $0x07, %cs:intr_num
	jmp _intr_default_handler

.global _int0x08_default_handler
_int0x08_default_handler:
	movw $0x08, %cs:intr_num
	jmp _intr_default_handler

.global _int0x09_default_handler
_int0x09_default_handler:
	movw $0x09, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0a_default_handler
_int0x0a_default_handler:
	movw $0x0a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0b_default_handler
_int0x0b_default_handler:
	movw $0x0b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0c_default_handler
_int0x0c_default_handler:
	movw $0x0c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0d_default_handler
_int0x0d_default_handler:
	movw $0x0d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0e_default_handler
_int0x0e_default_handler:
	movw $0x0e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x0f_default_handler
_int0x0f_default_handler:
	movw $0x0f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x10_default_handler
_int0x10_default_handler:
	movw $0x10, %cs:intr_num
	jmp _intr_default_handler

.global _int0x11_default_handler
_int0x11_default_handler:
	movw $0x11, %cs:intr_num
	jmp _intr_default_handler

.global _int0x12_default_handler
_int0x12_default_handler:
	movw $0x12, %cs:intr_num
	jmp _intr_default_handler

.global _int0x13_default_handler
_int0x13_default_handler:
	movw $0x13, %cs:intr_num
	jmp _intr_default_handler

.global _int0x14_default_handler
_int0x14_default_handler:
	movw $0x14, %cs:intr_num
	jmp _intr_default_handler

.global _int0x15_default_handler
_int0x15_default_handler:
	movw $0x15, %cs:intr_num
	jmp _intr_default_handler

.global _int0x16_default_handler
_int0x16_default_handler:
	movw $0x16, %cs:intr_num
	jmp _intr_default_handler

.global _int0x17_default_handler
_int0x17_default_handler:
	movw $0x17, %cs:intr_num
	jmp _intr_default_handler

.global _int0x18_default_handler
_int0x18_default_handler:
	movw $0x18, %cs:intr_num
	jmp _intr_default_handler

.global _int0x19_default_handler
_int0x19_default_handler:
	movw $0x19, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1a_default_handler
_int0x1a_default_handler:
	movw $0x1a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1b_default_handler
_int0x1b_default_handler:
	movw $0x1b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1c_default_handler
_int0x1c_default_handler:
	movw $0x1c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1d_default_handler
_int0x1d_default_handler:
	movw $0x1d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1e_default_handler
_int0x1e_default_handler:
	movw $0x1e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x1f_default_handler
_int0x1f_default_handler:
	movw $0x1f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x20_default_handler
_int0x20_default_handler:
	movw $0x20, %cs:intr_num
	jmp _intr_default_handler

.global _int0x21_default_handler
_int0x21_default_handler:
	movw $0x21, %cs:intr_num
	jmp _intr_default_handler

.global _int0x22_default_handler
_int0x22_default_handler:
	movw $0x22, %cs:intr_num
	jmp _intr_default_handler

.global _int0x23_default_handler
_int0x23_default_handler:
	movw $0x23, %cs:intr_num
	jmp _intr_default_handler

.global _int0x24_default_handler
_int0x24_default_handler:
	movw $0x24, %cs:intr_num
	jmp _intr_default_handler

.global _int0x25_default_handler
_int0x25_default_handler:
	movw $0x25, %cs:intr_num
	jmp _intr_default_handler

.global _int0x26_default_handler
_int0x26_default_handler:
	movw $0x26, %cs:intr_num
	jmp _intr_default_handler

.global _int0x27_default_handler
_int0x27_default_handler:
	movw $0x27, %cs:intr_num
	jmp _intr_default_handler

.global _int0x28_default_handler
_int0x28_default_handler:
	movw $0x28, %cs:intr_num
	jmp _intr_default_handler

.global _int0x29_default_handler
_int0x29_default_handler:
	movw $0x29, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2a_default_handler
_int0x2a_default_handler:
	movw $0x2a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2b_default_handler
_int0x2b_default_handler:
	movw $0x2b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2c_default_handler
_int0x2c_default_handler:
	movw $0x2c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2d_default_handler
_int0x2d_default_handler:
	movw $0x2d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2e_default_handler
_int0x2e_default_handler:
	movw $0x2e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x2f_default_handler
_int0x2f_default_handler:
	movw $0x2f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x30_default_handler
_int0x30_default_handler:
	movw $0x30, %cs:intr_num
	jmp _intr_default_handler

.global _int0x31_default_handler
_int0x31_default_handler:
	movw $0x31, %cs:intr_num
	jmp _intr_default_handler

.global _int0x32_default_handler
_int0x32_default_handler:
	movw $0x32, %cs:intr_num
	jmp _intr_default_handler

.global _int0x33_default_handler
_int0x33_default_handler:
	movw $0x33, %cs:intr_num
	jmp _intr_default_handler

.global _int0x34_default_handler
_int0x34_default_handler:
	movw $0x34, %cs:intr_num
	jmp _intr_default_handler

.global _int0x35_default_handler
_int0x35_default_handler:
	movw $0x35, %cs:intr_num
	jmp _intr_default_handler

.global _int0x36_default_handler
_int0x36_default_handler:
	movw $0x36, %cs:intr_num
	jmp _intr_default_handler

.global _int0x37_default_handler
_int0x37_default_handler:
	movw $0x37, %cs:intr_num
	jmp _intr_default_handler

.global _int0x38_default_handler
_int0x38_default_handler:
	movw $0x38, %cs:intr_num
	jmp _intr_default_handler

.global _int0x39_default_handler
_int0x39_default_handler:
	movw $0x39, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3a_default_handler
_int0x3a_default_handler:
	movw $0x3a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3b_default_handler
_int0x3b_default_handler:
	movw $0x3b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3c_default_handler
_int0x3c_default_handler:
	movw $0x3c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3d_default_handler
_int0x3d_default_handler:
	movw $0x3d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3e_default_handler
_int0x3e_default_handler:
	movw $0x3e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x3f_default_handler
_int0x3f_default_handler:
	movw $0x3f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x40_default_handler
_int0x40_default_handler:
	movw $0x40, %cs:intr_num
	jmp _intr_default_handler

.global _int0x41_default_handler
_int0x41_default_handler:
	movw $0x41, %cs:intr_num
	jmp _intr_default_handler

.global _int0x42_default_handler
_int0x42_default_handler:
	movw $0x42, %cs:intr_num
	jmp _intr_default_handler

.global _int0x43_default_handler
_int0x43_default_handler:
	movw $0x43, %cs:intr_num
	jmp _intr_default_handler

.global _int0x44_default_handler
_int0x44_default_handler:
	movw $0x44, %cs:intr_num
	jmp _intr_default_handler

.global _int0x45_default_handler
_int0x45_default_handler:
	movw $0x45, %cs:intr_num
	jmp _intr_default_handler

.global _int0x46_default_handler
_int0x46_default_handler:
	movw $0x46, %cs:intr_num
	jmp _intr_default_handler

.global _int0x47_default_handler
_int0x47_default_handler:
	movw $0x47, %cs:intr_num
	jmp _intr_default_handler

.global _int0x48_default_handler
_int0x48_default_handler:
	movw $0x48, %cs:intr_num
	jmp _intr_default_handler

.global _int0x49_default_handler
_int0x49_default_handler:
	movw $0x49, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4a_default_handler
_int0x4a_default_handler:
	movw $0x4a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4b_default_handler
_int0x4b_default_handler:
	movw $0x4b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4c_default_handler
_int0x4c_default_handler:
	movw $0x4c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4d_default_handler
_int0x4d_default_handler:
	movw $0x4d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4e_default_handler
_int0x4e_default_handler:
	movw $0x4e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x4f_default_handler
_int0x4f_default_handler:
	movw $0x4f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x50_default_handler
_int0x50_default_handler:
	movw $0x50, %cs:intr_num
	jmp _intr_default_handler

.global _int0x51_default_handler
_int0x51_default_handler:
	movw $0x51, %cs:intr_num
	jmp _intr_default_handler

.global _int0x52_default_handler
_int0x52_default_handler:
	movw $0x52, %cs:intr_num
	jmp _intr_default_handler

.global _int0x53_default_handler
_int0x53_default_handler:
	movw $0x53, %cs:intr_num
	jmp _intr_default_handler

.global _int0x54_default_handler
_int0x54_default_handler:
	movw $0x54, %cs:intr_num
	jmp _intr_default_handler

.global _int0x55_default_handler
_int0x55_default_handler:
	movw $0x55, %cs:intr_num
	jmp _intr_default_handler

.global _int0x56_default_handler
_int0x56_default_handler:
	movw $0x56, %cs:intr_num
	jmp _intr_default_handler

.global _int0x57_default_handler
_int0x57_default_handler:
	movw $0x57, %cs:intr_num
	jmp _intr_default_handler

.global _int0x58_default_handler
_int0x58_default_handler:
	movw $0x58, %cs:intr_num
	jmp _intr_default_handler

.global _int0x59_default_handler
_int0x59_default_handler:
	movw $0x59, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5a_default_handler
_int0x5a_default_handler:
	movw $0x5a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5b_default_handler
_int0x5b_default_handler:
	movw $0x5b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5c_default_handler
_int0x5c_default_handler:
	movw $0x5c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5d_default_handler
_int0x5d_default_handler:
	movw $0x5d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5e_default_handler
_int0x5e_default_handler:
	movw $0x5e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x5f_default_handler
_int0x5f_default_handler:
	movw $0x5f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x60_default_handler
_int0x60_default_handler:
	movw $0x60, %cs:intr_num
	jmp _intr_default_handler

.global _int0x61_default_handler
_int0x61_default_handler:
	movw $0x61, %cs:intr_num
	jmp _intr_default_handler

.global _int0x62_default_handler
_int0x62_default_handler:
	movw $0x62, %cs:intr_num
	jmp _intr_default_handler

.global _int0x63_default_handler
_int0x63_default_handler:
	movw $0x63, %cs:intr_num
	jmp _intr_default_handler

.global _int0x64_default_handler
_int0x64_default_handler:
	movw $0x64, %cs:intr_num
	jmp _intr_default_handler

.global _int0x65_default_handler
_int0x65_default_handler:
	movw $0x65, %cs:intr_num
	jmp _intr_default_handler

.global _int0x66_default_handler
_int0x66_default_handler:
	movw $0x66, %cs:intr_num
	jmp _intr_default_handler

.global _int0x67_default_handler
_int0x67_default_handler:
	movw $0x67, %cs:intr_num
	jmp _intr_default_handler

.global _int0x68_default_handler
_int0x68_default_handler:
	movw $0x68, %cs:intr_num
	jmp _intr_default_handler

.global _int0x69_default_handler
_int0x69_default_handler:
	movw $0x69, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6a_default_handler
_int0x6a_default_handler:
	movw $0x6a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6b_default_handler
_int0x6b_default_handler:
	movw $0x6b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6c_default_handler
_int0x6c_default_handler:
	movw $0x6c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6d_default_handler
_int0x6d_default_handler:
	movw $0x6d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6e_default_handler
_int0x6e_default_handler:
	movw $0x6e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x6f_default_handler
_int0x6f_default_handler:
	movw $0x6f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x70_default_handler
_int0x70_default_handler:
	movw $0x70, %cs:intr_num
	jmp _intr_default_handler

.global _int0x71_default_handler
_int0x71_default_handler:
	movw $0x71, %cs:intr_num
	jmp _intr_default_handler

.global _int0x72_default_handler
_int0x72_default_handler:
	movw $0x72, %cs:intr_num
	jmp _intr_default_handler

.global _int0x73_default_handler
_int0x73_default_handler:
	movw $0x73, %cs:intr_num
	jmp _intr_default_handler

.global _int0x74_default_handler
_int0x74_default_handler:
	movw $0x74, %cs:intr_num
	jmp _intr_default_handler

.global _int0x75_default_handler
_int0x75_default_handler:
	movw $0x75, %cs:intr_num
	jmp _intr_default_handler

.global _int0x76_default_handler
_int0x76_default_handler:
	movw $0x76, %cs:intr_num
	jmp _intr_default_handler

.global _int0x77_default_handler
_int0x77_default_handler:
	movw $0x77, %cs:intr_num
	jmp _intr_default_handler

.global _int0x78_default_handler
_int0x78_default_handler:
	movw $0x78, %cs:intr_num
	jmp _intr_default_handler

.global _int0x79_default_handler
_int0x79_default_handler:
	movw $0x79, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7a_default_handler
_int0x7a_default_handler:
	movw $0x7a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7b_default_handler
_int0x7b_default_handler:
	movw $0x7b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7c_default_handler
_int0x7c_default_handler:
	movw $0x7c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7d_default_handler
_int0x7d_default_handler:
	movw $0x7d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7e_default_handler
_int0x7e_default_handler:
	movw $0x7e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x7f_default_handler
_int0x7f_default_handler:
	movw $0x7f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x80_default_handler
_int0x80_default_handler:
	movw $0x80, %cs:intr_num
	jmp _intr_default_handler

.global _int0x81_default_handler
_int0x81_default_handler:
	movw $0x81, %cs:intr_num
	jmp _intr_default_handler

.global _int0x82_default_handler
_int0x82_default_handler:
	movw $0x82, %cs:intr_num
	jmp _intr_default_handler

.global _int0x83_default_handler
_int0x83_default_handler:
	movw $0x83, %cs:intr_num
	jmp _intr_default_handler

.global _int0x84_default_handler
_int0x84_default_handler:
	movw $0x84, %cs:intr_num
	jmp _intr_default_handler

.global _int0x85_default_handler
_int0x85_default_handler:
	movw $0x85, %cs:intr_num
	jmp _intr_default_handler

.global _int0x86_default_handler
_int0x86_default_handler:
	movw $0x86, %cs:intr_num
	jmp _intr_default_handler

.global _int0x87_default_handler
_int0x87_default_handler:
	movw $0x87, %cs:intr_num
	jmp _intr_default_handler

.global _int0x88_default_handler
_int0x88_default_handler:
	movw $0x88, %cs:intr_num
	jmp _intr_default_handler

.global _int0x89_default_handler
_int0x89_default_handler:
	movw $0x89, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8a_default_handler
_int0x8a_default_handler:
	movw $0x8a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8b_default_handler
_int0x8b_default_handler:
	movw $0x8b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8c_default_handler
_int0x8c_default_handler:
	movw $0x8c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8d_default_handler
_int0x8d_default_handler:
	movw $0x8d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8e_default_handler
_int0x8e_default_handler:
	movw $0x8e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x8f_default_handler
_int0x8f_default_handler:
	movw $0x8f, %cs:intr_num
	jmp _intr_default_handler

.global _int0x90_default_handler
_int0x90_default_handler:
	movw $0x90, %cs:intr_num
	jmp _intr_default_handler

.global _int0x91_default_handler
_int0x91_default_handler:
	movw $0x91, %cs:intr_num
	jmp _intr_default_handler

.global _int0x92_default_handler
_int0x92_default_handler:
	movw $0x92, %cs:intr_num
	jmp _intr_default_handler

.global _int0x93_default_handler
_int0x93_default_handler:
	movw $0x93, %cs:intr_num
	jmp _intr_default_handler

.global _int0x94_default_handler
_int0x94_default_handler:
	movw $0x94, %cs:intr_num
	jmp _intr_default_handler

.global _int0x95_default_handler
_int0x95_default_handler:
	movw $0x95, %cs:intr_num
	jmp _intr_default_handler

.global _int0x96_default_handler
_int0x96_default_handler:
	movw $0x96, %cs:intr_num
	jmp _intr_default_handler

.global _int0x97_default_handler
_int0x97_default_handler:
	movw $0x97, %cs:intr_num
	jmp _intr_default_handler

.global _int0x98_default_handler
_int0x98_default_handler:
	movw $0x98, %cs:intr_num
	jmp _intr_default_handler

.global _int0x99_default_handler
_int0x99_default_handler:
	movw $0x99, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9a_default_handler
_int0x9a_default_handler:
	movw $0x9a, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9b_default_handler
_int0x9b_default_handler:
	movw $0x9b, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9c_default_handler
_int0x9c_default_handler:
	movw $0x9c, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9d_default_handler
_int0x9d_default_handler:
	movw $0x9d, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9e_default_handler
_int0x9e_default_handler:
	movw $0x9e, %cs:intr_num
	jmp _intr_default_handler

.global _int0x9f_default_handler
_int0x9f_default_handler:
	movw $0x9f, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa0_default_handler
_int0xa0_default_handler:
	movw $0xa0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa1_default_handler
_int0xa1_default_handler:
	movw $0xa1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa2_default_handler
_int0xa2_default_handler:
	movw $0xa2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa3_default_handler
_int0xa3_default_handler:
	movw $0xa3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa4_default_handler
_int0xa4_default_handler:
	movw $0xa4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa5_default_handler
_int0xa5_default_handler:
	movw $0xa5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa6_default_handler
_int0xa6_default_handler:
	movw $0xa6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa7_default_handler
_int0xa7_default_handler:
	movw $0xa7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa8_default_handler
_int0xa8_default_handler:
	movw $0xa8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xa9_default_handler
_int0xa9_default_handler:
	movw $0xa9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xaa_default_handler
_int0xaa_default_handler:
	movw $0xaa, %cs:intr_num
	jmp _intr_default_handler

.global _int0xab_default_handler
_int0xab_default_handler:
	movw $0xab, %cs:intr_num
	jmp _intr_default_handler

.global _int0xac_default_handler
_int0xac_default_handler:
	movw $0xac, %cs:intr_num
	jmp _intr_default_handler

.global _int0xad_default_handler
_int0xad_default_handler:
	movw $0xad, %cs:intr_num
	jmp _intr_default_handler

.global _int0xae_default_handler
_int0xae_default_handler:
	movw $0xae, %cs:intr_num
	jmp _intr_default_handler

.global _int0xaf_default_handler
_int0xaf_default_handler:
	movw $0xaf, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb0_default_handler
_int0xb0_default_handler:
	movw $0xb0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb1_default_handler
_int0xb1_default_handler:
	movw $0xb1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb2_default_handler
_int0xb2_default_handler:
	movw $0xb2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb3_default_handler
_int0xb3_default_handler:
	movw $0xb3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb4_default_handler
_int0xb4_default_handler:
	movw $0xb4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb5_default_handler
_int0xb5_default_handler:
	movw $0xb5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb6_default_handler
_int0xb6_default_handler:
	movw $0xb6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb7_default_handler
_int0xb7_default_handler:
	movw $0xb7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb8_default_handler
_int0xb8_default_handler:
	movw $0xb8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xb9_default_handler
_int0xb9_default_handler:
	movw $0xb9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xba_default_handler
_int0xba_default_handler:
	movw $0xba, %cs:intr_num
	jmp _intr_default_handler

.global _int0xbb_default_handler
_int0xbb_default_handler:
	movw $0xbb, %cs:intr_num
	jmp _intr_default_handler

.global _int0xbc_default_handler
_int0xbc_default_handler:
	movw $0xbc, %cs:intr_num
	jmp _intr_default_handler

.global _int0xbd_default_handler
_int0xbd_default_handler:
	movw $0xbd, %cs:intr_num
	jmp _intr_default_handler

.global _int0xbe_default_handler
_int0xbe_default_handler:
	movw $0xbe, %cs:intr_num
	jmp _intr_default_handler

.global _int0xbf_default_handler
_int0xbf_default_handler:
	movw $0xbf, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc0_default_handler
_int0xc0_default_handler:
	movw $0xc0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc1_default_handler
_int0xc1_default_handler:
	movw $0xc1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc2_default_handler
_int0xc2_default_handler:
	movw $0xc2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc3_default_handler
_int0xc3_default_handler:
	movw $0xc3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc4_default_handler
_int0xc4_default_handler:
	movw $0xc4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc5_default_handler
_int0xc5_default_handler:
	movw $0xc5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc6_default_handler
_int0xc6_default_handler:
	movw $0xc6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc7_default_handler
_int0xc7_default_handler:
	movw $0xc7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc8_default_handler
_int0xc8_default_handler:
	movw $0xc8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xc9_default_handler
_int0xc9_default_handler:
	movw $0xc9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xca_default_handler
_int0xca_default_handler:
	movw $0xca, %cs:intr_num
	jmp _intr_default_handler

.global _int0xcb_default_handler
_int0xcb_default_handler:
	movw $0xcb, %cs:intr_num
	jmp _intr_default_handler

.global _int0xcc_default_handler
_int0xcc_default_handler:
	movw $0xcc, %cs:intr_num
	jmp _intr_default_handler

.global _int0xcd_default_handler
_int0xcd_default_handler:
	movw $0xcd, %cs:intr_num
	jmp _intr_default_handler

.global _int0xce_default_handler
_int0xce_default_handler:
	movw $0xce, %cs:intr_num
	jmp _intr_default_handler

.global _int0xcf_default_handler
_int0xcf_default_handler:
	movw $0xcf, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd0_default_handler
_int0xd0_default_handler:
	movw $0xd0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd1_default_handler
_int0xd1_default_handler:
	movw $0xd1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd2_default_handler
_int0xd2_default_handler:
	movw $0xd2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd3_default_handler
_int0xd3_default_handler:
	movw $0xd3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd4_default_handler
_int0xd4_default_handler:
	movw $0xd4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd5_default_handler
_int0xd5_default_handler:
	movw $0xd5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd6_default_handler
_int0xd6_default_handler:
	movw $0xd6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd7_default_handler
_int0xd7_default_handler:
	movw $0xd7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd8_default_handler
_int0xd8_default_handler:
	movw $0xd8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xd9_default_handler
_int0xd9_default_handler:
	movw $0xd9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xda_default_handler
_int0xda_default_handler:
	movw $0xda, %cs:intr_num
	jmp _intr_default_handler

.global _int0xdb_default_handler
_int0xdb_default_handler:
	movw $0xdb, %cs:intr_num
	jmp _intr_default_handler

.global _int0xdc_default_handler
_int0xdc_default_handler:
	movw $0xdc, %cs:intr_num
	jmp _intr_default_handler

.global _int0xdd_default_handler
_int0xdd_default_handler:
	movw $0xdd, %cs:intr_num
	jmp _intr_default_handler

.global _int0xde_default_handler
_int0xde_default_handler:
	movw $0xde, %cs:intr_num
	jmp _intr_default_handler

.global _int0xdf_default_handler
_int0xdf_default_handler:
	movw $0xdf, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe0_default_handler
_int0xe0_default_handler:
	movw $0xe0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe1_default_handler
_int0xe1_default_handler:
	movw $0xe1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe2_default_handler
_int0xe2_default_handler:
	movw $0xe2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe3_default_handler
_int0xe3_default_handler:
	movw $0xe3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe4_default_handler
_int0xe4_default_handler:
	movw $0xe4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe5_default_handler
_int0xe5_default_handler:
	movw $0xe5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe6_default_handler
_int0xe6_default_handler:
	movw $0xe6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe7_default_handler
_int0xe7_default_handler:
	movw $0xe7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe8_default_handler
_int0xe8_default_handler:
	movw $0xe8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xe9_default_handler
_int0xe9_default_handler:
	movw $0xe9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xea_default_handler
_int0xea_default_handler:
	movw $0xea, %cs:intr_num
	jmp _intr_default_handler

.global _int0xeb_default_handler
_int0xeb_default_handler:
	movw $0xeb, %cs:intr_num
	jmp _intr_default_handler

.global _int0xec_default_handler
_int0xec_default_handler:
	movw $0xec, %cs:intr_num
	jmp _intr_default_handler

.global _int0xed_default_handler
_int0xed_default_handler:
	movw $0xed, %cs:intr_num
	jmp _intr_default_handler

.global _int0xee_default_handler
_int0xee_default_handler:
	movw $0xee, %cs:intr_num
	jmp _intr_default_handler

.global _int0xef_default_handler
_int0xef_default_handler:
	movw $0xef, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf0_default_handler
_int0xf0_default_handler:
	movw $0xf0, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf1_default_handler
_int0xf1_default_handler:
	movw $0xf1, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf2_default_handler
_int0xf2_default_handler:
	movw $0xf2, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf3_default_handler
_int0xf3_default_handler:
	movw $0xf3, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf4_default_handler
_int0xf4_default_handler:
	movw $0xf4, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf5_default_handler
_int0xf5_default_handler:
	movw $0xf5, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf6_default_handler
_int0xf6_default_handler:
	movw $0xf6, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf7_default_handler
_int0xf7_default_handler:
	movw $0xf7, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf8_default_handler
_int0xf8_default_handler:
	movw $0xf8, %cs:intr_num
	jmp _intr_default_handler

.global _int0xf9_default_handler
_int0xf9_default_handler:
	movw $0xf9, %cs:intr_num
	jmp _intr_default_handler

.global _int0xfa_default_handler
_int0xfa_default_handler:
	movw $0xfa, %cs:intr_num
	jmp _intr_default_handler

.global _int0xfb_default_handler
_int0xfb_default_handler:
	movw $0xfb, %cs:intr_num
	jmp _intr_default_handler

.global _int0xfc_default_handler
_int0xfc_default_handler:
	movw $0xfc, %cs:intr_num
	jmp _intr_default_handler

.global _int0xfd_default_handler
_int0xfd_default_handler:
	movw $0xfd, %cs:intr_num
	jmp _intr_default_handler

.global _int0xfe_default_handler
_int0xfe_default_handler:
	movw $0xfe, %cs:intr_num
	jmp _intr_default_handler

.global _int0xff_default_handler
_int0xff_default_handler:
	movw $0xff, %cs:intr_num
	jmp _intr_default_handler

.global _intr_default_handler
_intr_default_handler:
	push %ss
	push %ds
	push %es
	pusha
	pushf
	mov %ax, %di
	mov %sp, %si
	mov %ss, %ax
	mov %ax, %es
	mov %cs, %ax
	mov %ax, %ss
	mov %ax, %ds
	mov $0xffff, %ax
	mov %ax, %sp
	push	%dx
	push	%cx
	push	%bx
	push	%di
	push	%es
	push	%si
	call	intr_print_default_msg
	mov %es, %ax
	mov %ax, %ss
	mov %si, %sp
	popf
	popa
	pop %es
	pop %ds
	pop %ss
	iret

.global int_default_vector_table
int_default_vector_table:
.word _int0x00_default_handler
.word _int0x01_default_handler
.word _int0x02_default_handler
.word _int0x03_default_handler
.word _int0x04_default_handler
.word _int0x05_default_handler
.word _int0x06_default_handler
.word _int0x07_default_handler
.word _int0x08_default_handler
.word _int0x09_default_handler
.word _int0x0a_default_handler
.word _int0x0b_default_handler
.word _int0x0c_default_handler
.word _int0x0d_default_handler
.word _int0x0e_default_handler
.word _int0x0f_default_handler
.word _int0x10_default_handler
.word _int0x11_default_handler
.word _int0x12_default_handler
.word _int0x13_default_handler
.word _int0x14_default_handler
.word _int0x15_default_handler
.word _int0x16_default_handler
.word _int0x17_default_handler
.word _int0x18_default_handler
.word _int0x19_default_handler
.word _int0x1a_default_handler
.word _int0x1b_default_handler
.word _int0x1c_default_handler
.word _int0x1d_default_handler
.word _int0x1e_default_handler
.word _int0x1f_default_handler
.word _int0x20_default_handler
.word _int0x21_default_handler
.word _int0x22_default_handler
.word _int0x23_default_handler
.word _int0x24_default_handler
.word _int0x25_default_handler
.word _int0x26_default_handler
.word _int0x27_default_handler
.word _int0x28_default_handler
.word _int0x29_default_handler
.word _int0x2a_default_handler
.word _int0x2b_default_handler
.word _int0x2c_default_handler
.word _int0x2d_default_handler
.word _int0x2e_default_handler
.word _int0x2f_default_handler
.word _int0x30_default_handler
.word _int0x31_default_handler
.word _int0x32_default_handler
.word _int0x33_default_handler
.word _int0x34_default_handler
.word _int0x35_default_handler
.word _int0x36_default_handler
.word _int0x37_default_handler
.word _int0x38_default_handler
.word _int0x39_default_handler
.word _int0x3a_default_handler
.word _int0x3b_default_handler
.word _int0x3c_default_handler
.word _int0x3d_default_handler
.word _int0x3e_default_handler
.word _int0x3f_default_handler
.word _int0x40_default_handler
.word _int0x41_default_handler
.word _int0x42_default_handler
.word _int0x43_default_handler
.word _int0x44_default_handler
.word _int0x45_default_handler
.word _int0x46_default_handler
.word _int0x47_default_handler
.word _int0x48_default_handler
.word _int0x49_default_handler
.word _int0x4a_default_handler
.word _int0x4b_default_handler
.word _int0x4c_default_handler
.word _int0x4d_default_handler
.word _int0x4e_default_handler
.word _int0x4f_default_handler
.word _int0x50_default_handler
.word _int0x51_default_handler
.word _int0x52_default_handler
.word _int0x53_default_handler
.word _int0x54_default_handler
.word _int0x55_default_handler
.word _int0x56_default_handler
.word _int0x57_default_handler
.word _int0x58_default_handler
.word _int0x59_default_handler
.word _int0x5a_default_handler
.word _int0x5b_default_handler
.word _int0x5c_default_handler
.word _int0x5d_default_handler
.word _int0x5e_default_handler
.word _int0x5f_default_handler
.word _int0x60_default_handler
.word _int0x61_default_handler
.word _int0x62_default_handler
.word _int0x63_default_handler
.word _int0x64_default_handler
.word _int0x65_default_handler
.word _int0x66_default_handler
.word _int0x67_default_handler
.word _int0x68_default_handler
.word _int0x69_default_handler
.word _int0x6a_default_handler
.word _int0x6b_default_handler
.word _int0x6c_default_handler
.word _int0x6d_default_handler
.word _int0x6e_default_handler
.word _int0x6f_default_handler
.word _int0x70_default_handler
.word _int0x71_default_handler
.word _int0x72_default_handler
.word _int0x73_default_handler
.word _int0x74_default_handler
.word _int0x75_default_handler
.word _int0x76_default_handler
.word _int0x77_default_handler
.word _int0x78_default_handler
.word _int0x79_default_handler
.word _int0x7a_default_handler
.word _int0x7b_default_handler
.word _int0x7c_default_handler
.word _int0x7d_default_handler
.word _int0x7e_default_handler
.word _int0x7f_default_handler
.word _int0x80_default_handler
.word _int0x81_default_handler
.word _int0x82_default_handler
.word _int0x83_default_handler
.word _int0x84_default_handler
.word _int0x85_default_handler
.word _int0x86_default_handler
.word _int0x87_default_handler
.word _int0x88_default_handler
.word _int0x89_default_handler
.word _int0x8a_default_handler
.word _int0x8b_default_handler
.word _int0x8c_default_handler
.word _int0x8d_default_handler
.word _int0x8e_default_handler
.word _int0x8f_default_handler
.word _int0x90_default_handler
.word _int0x91_default_handler
.word _int0x92_default_handler
.word _int0x93_default_handler
.word _int0x94_default_handler
.word _int0x95_default_handler
.word _int0x96_default_handler
.word _int0x97_default_handler
.word _int0x98_default_handler
.word _int0x99_default_handler
.word _int0x9a_default_handler
.word _int0x9b_default_handler
.word _int0x9c_default_handler
.word _int0x9d_default_handler
.word _int0x9e_default_handler
.word _int0x9f_default_handler
.word _int0xa0_default_handler
.word _int0xa1_default_handler
.word _int0xa2_default_handler
.word _int0xa3_default_handler
.word _int0xa4_default_handler
.word _int0xa5_default_handler
.word _int0xa6_default_handler
.word _int0xa7_default_handler
.word _int0xa8_default_handler
.word _int0xa9_default_handler
.word _int0xaa_default_handler
.word _int0xab_default_handler
.word _int0xac_default_handler
.word _int0xad_default_handler
.word _int0xae_default_handler
.word _int0xaf_default_handler
.word _int0xb0_default_handler
.word _int0xb1_default_handler
.word _int0xb2_default_handler
.word _int0xb3_default_handler
.word _int0xb4_default_handler
.word _int0xb5_default_handler
.word _int0xb6_default_handler
.word _int0xb7_default_handler
.word _int0xb8_default_handler
.word _int0xb9_default_handler
.word _int0xba_default_handler
.word _int0xbb_default_handler
.word _int0xbc_default_handler
.word _int0xbd_default_handler
.word _int0xbe_default_handler
.word _int0xbf_default_handler
.word _int0xc0_default_handler
.word _int0xc1_default_handler
.word _int0xc2_default_handler
.word _int0xc3_default_handler
.word _int0xc4_default_handler
.word _int0xc5_default_handler
.word _int0xc6_default_handler
.word _int0xc7_default_handler
.word _int0xc8_default_handler
.word _int0xc9_default_handler
.word _int0xca_default_handler
.word _int0xcb_default_handler
.word _int0xcc_default_handler
.word _int0xcd_default_handler
.word _int0xce_default_handler
.word _int0xcf_default_handler
.word _int0xd0_default_handler
.word _int0xd1_default_handler
.word _int0xd2_default_handler
.word _int0xd3_default_handler
.word _int0xd4_default_handler
.word _int0xd5_default_handler
.word _int0xd6_default_handler
.word _int0xd7_default_handler
.word _int0xd8_default_handler
.word _int0xd9_default_handler
.word _int0xda_default_handler
.word _int0xdb_default_handler
.word _int0xdc_default_handler
.word _int0xdd_default_handler
.word _int0xde_default_handler
.word _int0xdf_default_handler
.word _int0xe0_default_handler
.word _int0xe1_default_handler
.word _int0xe2_default_handler
.word _int0xe3_default_handler
.word _int0xe4_default_handler
.word _int0xe5_default_handler
.word _int0xe6_default_handler
.word _int0xe7_default_handler
.word _int0xe8_default_handler
.word _int0xe9_default_handler
.word _int0xea_default_handler
.word _int0xeb_default_handler
.word _int0xec_default_handler
.word _int0xed_default_handler
.word _int0xee_default_handler
.word _int0xef_default_handler
.word _int0xf0_default_handler
.word _int0xf1_default_handler
.word _int0xf2_default_handler
.word _int0xf3_default_handler
.word _int0xf4_default_handler
.word _int0xf5_default_handler
.word _int0xf6_default_handler
.word _int0xf7_default_handler
.word _int0xf8_default_handler
.word _int0xf9_default_handler
.word _int0xfa_default_handler
.word _int0xfb_default_handler
.word _int0xfc_default_handler
.word _int0xfd_default_handler
.word _int0xfe_default_handler
.word _int0xff_default_handler
