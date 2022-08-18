#ifndef __SERDESHEADER_H_
#define __SERDESHEADER_H_
#include <asm/io.h>
struct init_seq {
	unsigned int addr;
	unsigned int value;
};

void serdes_multi_mode_init(void __iomem * serdes_base, struct init_seq *seq , int len);
#endif
