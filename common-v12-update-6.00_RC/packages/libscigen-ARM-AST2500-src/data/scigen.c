#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "Types.h"
#include "dbgout.h"
#include "mmap.h"

//LPC Register Related MACROS
#define LPC_REG_BASE		0x1E789000
#define	ACPI_B7_B4			0x1AC
#define SWCR_07_04			0x184
#define	SWCR_0B_08			0x188			
#define SWCR_03_00			0x180
#define ACPI_B3_B0			0x1A8

#define	BMC_TRIG_SCI_EN		0x8
#define BMC_TRIG_WKUP_EN	0x8
#define BMC_TRIG_WKUP		0x6
#define	BMC_TRIG_WKUP_STAT	0x8
#define BMC_TRIG_SCI_STAT	0x8

/* Enable SCI Triggering */
int PDK_SCI_INIT(void)
{
	unsigned long value = 0;
	printf("!!--PDK_SCI_INIT--!!\n");
	mmap_read32( (LPC_REG_BASE + ACPI_B7_B4), &value);
	if (!(value & (1<<BMC_TRIG_SCI_EN)))
		mmap_write32((LPC_REG_BASE + ACPI_B7_B4), value|(1<<BMC_TRIG_SCI_EN));
	mmap_read32( (LPC_REG_BASE + SWCR_07_04), &value);
	if (!(value & (1<<BMC_TRIG_WKUP_EN)))
		mmap_write32((LPC_REG_BASE + SWCR_07_04), value|(1<<BMC_TRIG_WKUP_EN));

	return 0;
}

/* Trigger SCI Event */
int PDK_Trigger_SCI(void)
{
	printf("!!--PDK_Trigger_SCI--!!\n");
	mmap_write32((LPC_REG_BASE + SWCR_0B_08),(1<<BMC_TRIG_WKUP));
	usleep(1000);
	mmap_write32((LPC_REG_BASE + SWCR_03_00),(1<<BMC_TRIG_WKUP_STAT));
	mmap_write32((LPC_REG_BASE + ACPI_B3_B0),(1<<BMC_TRIG_SCI_STAT));
	return 0;
}

