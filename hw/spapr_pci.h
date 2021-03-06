/*
 * QEMU SPAPR PCI BUS definitions
 *
 * Copyright (c) 2011 Alexey Kardashevskiy <aik@au1.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#if !defined(__HW_SPAPR_H__)
#error Please include spapr.h before this file!
#endif

#if !defined(__HW_SPAPR_PCI_H__)
#define __HW_SPAPR_PCI_H__

#include "hw/pci.h"
#include "hw/pci_host.h"
#include "hw/xics.h"

#define SPAPR_MSIX_MAX_DEVS 32

typedef struct sPAPRPHBState {
    PCIHostState host_state;

    uint64_t buid;
    char *busname;
    char *dtbusname;

    MemoryRegion memspace, iospace;
    target_phys_addr_t mem_win_addr, mem_win_size, io_win_addr, io_win_size;
    target_phys_addr_t msi_win_addr;
    MemoryRegion memwindow, iowindow, msiwindow;

    uint32_t dma_liobn;
    uint64_t dma_window_start;
    uint64_t dma_window_size;
    DMAContext *dma;

    struct {
        uint32_t irq;
    } lsi_table[PCI_NUM_PINS];

    struct {
        uint32_t config_addr;
        uint32_t irq;
        int nvec;
    } msi_table[SPAPR_MSIX_MAX_DEVS];

    QLIST_ENTRY(sPAPRPHBState) list;
} sPAPRPHBState;

static inline qemu_irq spapr_phb_lsi_qirq(struct sPAPRPHBState *phb, int pin)
{
    return xics_get_qirq(spapr->icp, phb->lsi_table[pin].irq);
}

#define SPAPR_PCI_MEM_WIN_BUS_OFFSET 0x80000000ULL
#define SPAPR_PCI_IO_WIN_SIZE        0x10000

void spapr_create_phb(sPAPREnvironment *spapr,
                      const char *busname, uint64_t buid,
                      uint64_t mem_win_addr, uint64_t mem_win_size,
                      uint64_t io_win_addr, uint64_t msi_win_addr);

int spapr_populate_pci_dt(sPAPRPHBState *phb,
                          uint32_t xics_phandle,
                          void *fdt);

void spapr_pci_rtas_init(void);

#endif /* __HW_SPAPR_PCI_H__ */
