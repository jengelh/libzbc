#!/bin/bash
#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# This file is part of libzbc.
#
# Copyright (C) 2009-2014, HGST, Inc. All rights reserved.
# Copyright (C) 2016, Western Digital. All rights reserved.

. scripts/zbc_test_lib.sh

zbc_test_init $0 "CLOSE_ZONE closed to closed" $*

expected_cond="${ZC_CLOSED}"

# Get drive information
zbc_test_get_device_info

# Search target LBA
zbc_test_search_seq_zone_cond_or_NA ${ZC_EMPTY}
target_lba=${target_slba}

# Specify post process
zbc_test_case_on_exit zbc_test_run ${bin_path}/zbc_test_reset_zone ${device} ${target_lba}

# Start testing
zbc_test_run ${bin_path}/zbc_test_write_zone -v ${device} ${target_lba} ${lblk_per_pblk}
zbc_test_fail_exit_if_sk_ascq "Initial WRITE failed, zone_type=${target_type}"

zbc_test_run ${bin_path}/zbc_test_close_zone -v ${device} ${target_lba}
zbc_test_fail_exit_if_sk_ascq "Zone CLOSE failed, zone_type=${target_type}"

zbc_test_run ${bin_path}/zbc_test_close_zone -v ${device} ${target_lba}

# Get SenseKey, ASC/ASCQ
zbc_test_get_sk_ascq

# Get target zone condition
zbc_test_get_target_zone_from_slba ${target_lba}

# Check result
zbc_test_check_zone_cond_wp $(( ${target_lba} + ${lblk_per_pblk} ))
