#ifndef __PINCTRL_FTSCU010_H
#define __PINCTRL_FTSCU010_H

/**
 * struct ftscu010_pin: represent a single pin in ftscu010 pin controller
 * @offset: mux register offset of particular pin.
 * @functions: available pin functions. Assuming 2-bit per pin, there are
 *	       four functions selectable.
 */

struct ftscu010_pin {
	unsigned int offset;
	unsigned functions[8];
};

/**
 * struct ftscu010_pin_group: a group of pins for a pinmux function
 * @name: name of the pin group.
 * @npins: number of pins included in @pins
 * @pins: the pins included in this group.
 */
#define FTSCU010_PIN_MAX 32
#define FTSCU010_MODEX_MAX 19
#define FTSCU010_PIN_MAGIC_NUM_SIZE  4
struct ftscu010_pin_group {
	const char *name;
	unsigned npins;
	unsigned int *pins;
	unsigned int schmitt_trigger;
};

/**
 * struct ftscu010_pmx_function: pinmux function
 * @name: name of pinmux function.
 * @groups: pin groups that provide this function.
 * @ngroups: number of groups included in @groups.
 */
struct ftscu010_pmx_function {
	const char *name;
	const char * const *groups;
	const unsigned ngroups;
};

/**
 * struct ftscu010_pinctrl_soc_data: per-SoC configuration
 * @pins: arrary of pins affected by the pin controller.
 * @npins: number of pins included in @pins.
 * @functions: array of mux funtions supported on this SoC.
 * @nfunctions: number of functions included in @functions.
 * @groups: array of pin groups avaiable on this SoC.
 * @ngroups: number of groups included in @groups.
 * @map: array of ftscu010_pin affected by the pin controller. Required by
 * 	 ftscu010 pin controller for the setup of particular mux function.
 *	 Beware that the index of particular pin in arrary should be identical
 *	 to its pin number.
 */

struct ftscu010_pinctrl_soc_data {
	const struct pinctrl_pin_desc *pins;
	unsigned npins;
	const struct ftscu010_pmx_function *functions;
	unsigned nfunctions;
	struct ftscu010_pin_group *groups;
	unsigned ngroups;
	const struct ftscu010_pin *map;
};

struct ftscu010_pinctrl_hog_runtime {
	struct list_head node;
	char name[32];
	struct pinmux *pmx;
};

int ftscu010_pinctrl_probe(struct platform_device *pdev,
			   const struct ftscu010_pinctrl_soc_data *data);
int ftscu010_pinctrl_remove(struct platform_device *pdev);
int ftscu010_pinctrl_runtime(struct platform_device *pdev, unsigned int cmd, const char *name);

#endif /* __PINCTRL_FTSCU010_H */
