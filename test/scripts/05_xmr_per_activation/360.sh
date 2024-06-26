#!/bin/bash
#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# This file is part of libzbc.
#
# Copyright (C) 2009-2014, HGST, Inc. All rights reserved.
# Copyright (C) 2023, Western Digital. All rights reserved.

. scripts/zbc_test_lib.sh

zbc_test_init $0 "READ an INACTIVE zone (type=${test_zone_type:-${ZT_SEQ}})" $*

# Get drive information
zbc_test_get_device_info

expected_sk="Data-protect"
expected_asc="Zone-is-inactive"

# Search target zone
zbc_test_search_zone_cond_or_NA ${ZC_INACTIVE}

# Start testing
zbc_test_run ${bin_path}/zbc_test_read_zone -v ${device} ${target_slba} ${lblk_per_pblk}

# Check result
zbc_test_get_sk_ascq
if [[ ${unrestricted_read} -eq 0 && ${target_type} == @(${ZT_RESTRICT_READ_INACTIVE}) ]]; then
    zbc_test_check_sk_ascq "zone_type=${target_type} URSWRZ=${unrestricted_read}"
else
    zbc_test_check_no_sk_ascq "zone_type=${target_type} URSWRZ=${unrestricted_read}"
fi
