// GPL-3.0. ROM identity and data stay in the host game adapter.
#pragma once
#include "../app/src/main/cpp/pk_wide.h"
#include "../app/src/main/cpp/pk_wide_gb_profile.h"
#include "polished.h"
#include "polished_wide_tables.h"
static inline int pocket_wide_configure(uint32_t crc) {
    if (crc == PC_WIDE_CRC)
        return pk_wide_gb_configure_polished(pcWideConfig, pc_lz);
    return pk_wide_gb_configure(crc);
}
