#ifndef _ASM_PLAT_FARADAY_H
#define _ASM_PLAT_FARADAY_H

#include <plat/nand.h>

void __init fttmr010_init(uint32_t hz);
void __init ftpwmtmr010_init(uint32_t hz);

void __init platform_audio_init(void);
#ifndef CONFIG_COMMON_CLK
void __init platform_clock_init(void);
#endif
void __init platform_pinmux_init(void);

#endif  /* _ASM_PLAT_FARADAY_H */
