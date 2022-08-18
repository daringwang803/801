#ifndef __PLAT_SD_H
#define __PLAT_SD_H

#if defined(CONFIG_MACH_LEO) || defined(CONFIG_MACH_WIDEBAND001)
extern void platform_sdhci_wait_dll_lock(void);
#else
void platform_sdhci_wait_dll_lock(void) {}
#endif

#endif
