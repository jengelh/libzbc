#!/bin/bash
#
# This file is part of libzbc.
#
# Copyright (C) 2018, Western Digital. All rights reserved.
#
# This software is distributed under the terms of the BSD 2-clause license,
# "as is," without technical support, and WITHOUT ANY WARRANTY, without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. You should have received a copy of the BSD 2-clause license along
# with libzbc. If not, see  <http://opensource.org/licenses/BSD-2-Clause>.

. scripts/zbc_test_lib.sh

zbc_test_init $0 "Run ZBC test on device after activating back to CONV or SOBR" $*

ZBC_TEST_LOG_PATH_BASE=${2}/allcmr2

# Get information
zbc_test_get_device_info
zbc_test_get_zone_realm_info

# Find the first SWR or SWP realm that can be activated as CONV or SOBR
zbc_test_search_realm_by_type_and_actv_or_NA "${ZT_SEQ}" "conv" "NOFAULTY"

#XXX Should compute the number of zones currently not CONV or SOBR that could be converted
# Find the total number of realms to reactivate as CONV or SOBR
actv_realms=${nr_actv_as_seq_realms}
if [ ${actv_realms} -eq 0 ]; then
    # This should not happen because we found one just above
    zbc_test_fail_exit "WARNING: No realms can be activated CONV or SOBR"
fi

if [ $(expr "${realm_num}" + "${actv_realms}") -gt ${nr_realms} ]; then
    actv_realms=$(expr "${nr_realms}" - "${realm_num}")
fi

# Use the same computation as 04.020, to reconvert back the same realms
nr=$(( actv_realms/2 ))
if [ ${nr} -eq 0 ]; then
    nr=1
fi

# If activation size is restricted, stay within the limit
if [ ${max_act} != "unlimited" ]; then
    zbc_test_calc_nr_realm_zones ${realm_num} ${nr}
    while [ ${nr_seq_zones} -gt ${max_act} ]; do
	nr=$(( ${nr} / 2 ))
	if [ ${nr} -eq 0 ]; then
	    zbc_test_print_not_applicable \
		"Cannot activate non-sequential zones (max_activate=${max_act})"
	fi
	zbc_test_calc_nr_realm_zones ${realm_num} ${nr}
    done
fi

# Activate the realms to the configuration for the run we invoke below
zbc_test_run ${bin_path}/zbc_test_reset_zone -v ${device} -1
zbc_test_run ${bin_path}/zbc_test_zone_activate -v \
			${device} ${realm_num} ${nr} ${cmr_type}
if [ $? -ne 0 ]; then
    printf "\n${0}: %s ${realm_num} ${nr} ${cmr_type}\n" \
	"Failed to activate device realms to intended test configuration"
    exit
fi

# Pass the batch_mode flag through to the run we invoke below
arg_b=""
if [ ${ZBC_TEST_BATCH_MODE} -ne 0 ] ; then
    arg_b="-b"
fi

arg_a=""
if [ ${ZBC_TEST_FORCE_ATA} ]; then
    arg_a="-a"
fi

arg_w=""
if [ ${ZBC_ACCEPT_ANY_FAIL} ]; then
    arg_w="-w"
fi

arg_l=""
if [ ${RUN_ACTIVATIONS_ONLY} ]; then
    arg_l="-l"
fi

# Specify post-processing to occur when script exits
zbc_test_case_on_exit zbc_test_run ${bin_path}/zbc_test_reset_zone ${device} -1

# Start ZBC test
zbc_test_meta_run ./zbc_xmr_test.sh ${arg_a} ${arg_b} ${arg_w} ${arg_l} -n ${eexec_list} ${cskip_list} ${device}
if [ $? -ne 0 ]; then
    sk="04.030 fail -- log path ${ZBC_TEST_LOG_PATH_BASE}"
    asc="child test of 04.030 failed $?"
fi

# Check result
zbc_test_check_no_sk_ascq
