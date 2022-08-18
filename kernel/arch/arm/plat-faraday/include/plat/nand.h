#ifndef __PLAT_NAND_H
#define __PLAT_NAND_H

/**
 * struct nand_flash_dev - NAND Flash Device ID Structure
 * @name:	Identify the device type
 * @id:		device ID code
 * @pagesize:	Pagesize in bytes. Either 256 or 512 or 0
 *		If the pagesize is 0, then the real pagesize
 *		and the eraseize are determined from the
 *		extended id bytes in the chip
 * @erasesize:	Size of an erase block in the flash device.
 * @chipsize:	Total chipsize in Mega Bytes
 * @options:	Bitfield to store chip relevant options
 * @dataecc:	ECC bits in each data region
 * @spareecc:	ECC bits in spare region
 */
struct platform_nand_flash {
	char *name;
	unsigned long pagesize;
	unsigned long chipsize;
	unsigned long erasesize;
	unsigned long addrcycles;
	unsigned long dataecc;
	unsigned long spareecc;
};

#endif
