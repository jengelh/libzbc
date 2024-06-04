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

zbc_test_init $0 "FINISH_ZONE full to full (type=${test_zone_type:-${ZT_SEQ}})" $*

expected_cond="${ZC_FULL}"

# Get drive information
zbc_test_get_device_info

# Search target LBA
zbc_test_search_wp_zone_cond_or_NA ${ZC_EMPTY}
target_lba=${target_slba}
if [[ ${target_type} == @(${ZT_SOBR}) ]]; then
    expected_sk="Illegal-request"
    expected_asc="Invalid-field-in-cdb"
fi
expected_cond="${ZC_FULL}"

# Specify post process
zbc_test_case_on_exit zbc_test_run ${bin_path}/zbc_test_reset_zone ${device} ${target_lba}

# Start testing
# First make the zone FULL by writing all of its blocks
zbc_test_run ${bin_path}/zbc_test_write_zone -v ${device} ${target_lba} ${target_size}
zbc_test_fail_exit_if_sk_ascq "Initial WRITE failed, zone_type=${target_type}"

# Now issue a FINISH command to the FULL zone
zbc_test_run ${bin_path}/zbc_test_finish_zone -v ${device} ${target_lba}

# Get SenseKey, ASC/ASCQ
zbc_test_get_sk_ascq

# Get target zone condition
zbc_test_get_target_zone_from_slba ${target_lba}

# Check result
if [[ ${target_type} != @(${ZT_SOBR}) ]]; then
    zbc_test_check_zone_cond "type=${target_type} cond=${target_cond}"
else
    zbc_test_check_sk_ascq_zone_cond "type=${target_type} cond=${target_cond}"
fi
