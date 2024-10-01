#pragma once

#include "utils.h"
#include "interrupt.h"
#include "errno.h"
#include "pci.h"
#include "memory.h"

void disk_init();

#pragma pack(push,1)
struct PhysicalRegionDescriptor{
    void* address;
    u16 byteCount;
    u16 flags;       //0x8000=last one, 0x0000=not last
};
#pragma pack(pop)

static struct PhysicalRegionDescriptor* physicalRegionDescriptor;

typedef void (*disk_callback_t)(int, void*, void*);

void disk_interrupt(struct InterruptContext* ctx);
void disk_read_sectors(
                    unsigned firstSector,
                    unsigned numSectors,
                    disk_callback_t callback,
                    void* callback_data);