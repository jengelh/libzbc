/*
 * SPDX-License-Identifier: BSD-2-Clause
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright (c) 2023 Western Digital Corporation or its affiliates.
 *
 * This file is part of libzbc.
 *
 * Author: Dmitry Fomichev (dmitry.fomichev@wdc.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libzbc/zbc.h>
#include "zbc_private.h"

int main(int argc, char **argv)
{
	struct zbc_device *dev;
	struct zbc_device_info info;
	struct zbc_zd_dev_control ctl;
	unsigned int oflags;
	int i, ret = 1, nz = 0, max_activate = 0;
	bool upd = false, urswrz = false, set_nz = false;
	bool quiet = false, set_urswrz = false, set_max_activate = false;
	char *path;

	/* Check command line */
	if (argc < 2) {
usage:
		printf("Usage: %s [options] <dev>\n"
		       "Options:\n"
		       "  -v                        : Verbose mode\n"
		       "  -nz <num>                 : Set the default number of zones to activate\n"
		       "  -ur y|n                   : Enable or disable unrestricted reads\n"
		       "  -maxr <num>|\"unlimited\"   : Set the maximum number of realms to activate\n"
		       "  -q                        : Ouput only errors\n",
		       argv[0]);
		return 1;
	}

	/* Parse options */
	for (i = 1; i < (argc - 1); i++) {

		if (strcmp(argv[i], "-v") == 0) {
			zbc_set_log_level("debug");
		} else if (strcmp(argv[i], "-q") == 0) {
			quiet = true;
		} else if (strcmp(argv[i], "-nz") == 0) {
			if (i >= (argc - 1))
				goto usage;
			i++;

			nz = strtol(argv[i], NULL, 10);
			if (nz <= 0) {
				fprintf(stderr,
					"[TEST][ERROR],invalid -nz value\n");
				goto usage;
			}
			set_nz = true;
		} else if (strcmp(argv[i], "-maxr") == 0) {
			if (i >= (argc - 1))
				goto usage;
			i++;

			if (strcmp(argv[i], "unlimited") == 0) {
				max_activate = 0xfffe;
			} else {
				max_activate = strtol(argv[i], NULL, 10);
				if (max_activate <= 0) {
					fprintf(stderr,
						"[TEST][ERROR],invalid -maxr value\n");
					goto usage;
				}
			}
			set_max_activate = true;
		} else if (strcmp(argv[i], "-ur") == 0) {
			if (i >= (argc - 1))
				goto usage;
			i++;

			if (strcmp(argv[i], "y") == 0)
				urswrz = true;
			else if (strcmp(argv[i], "n") == 0)
				urswrz = false;
			else {
				fprintf(stderr,
					"[TEST][ERROR],-ur value must be y or n\n");
				goto usage;
			}
			set_urswrz = true;
		} else if (argv[i][0] == '-') {
			fprintf(stderr,
				"[TEST][ERROR],Unknown option \"%s\"\n",
				argv[i]);
			goto usage;
		} else {
			break;
		}

	}

	if (i != (argc - 1))
		goto usage;
	path = argv[i];

	/* Open device */
	oflags = ZBC_O_DEVTEST;
	oflags |= ZBC_O_DRV_ATA;
	if (!getenv("ZBC_TEST_FORCE_ATA"))
		oflags |= ZBC_O_DRV_SCSI;

	ret = zbc_open(path, oflags, &dev);
	if (ret != 0) {
		fprintf(stderr, "[TEST][ERROR],open device failed, err %d (%s) %s\n",
			ret, strerror(-ret), path);
		fprintf(stderr,
			"[TEST][ERROR][SENSE_KEY],open-device-failed\n");
		fprintf(stderr,
			"[TEST][ERROR][ASC_ASCQ],open-device-failed\n");
		return 1;
	}

	zbc_get_device_info(dev, &info);

	if (!zbc_device_is_zdr(&info)) {
		if (set_nz || set_urswrz || set_max_activate) {
			fprintf(stderr,
				"[TEST][ERROR],not a ZDR device\n");
			ret = 1;
		}
		goto out;
	}

	if (set_nz && !(info.zbd_flags & ZBC_ZA_CONTROL_SUPPORT)) {
		fprintf(stderr,
			"[TEST][ERROR],device doesn't support ZA control\n");
		ret = 1;
		goto out;
	}
	if (set_urswrz && !(info.zbd_flags & ZBC_URSWRZ_SET_SUPPORT)) {
		fprintf(stderr,
			"[TEST][ERROR],device doesn't support unrestricted read control\n");
		ret = 1;
		goto out;
	}
	if (set_max_activate && !(info.zbd_flags & ZBC_MAXACT_SET_SUPPORT)) {
		fprintf(stderr,
			"[TEST][ERROR],device doesn't support maximum activation control\n");
		ret = 1;
		goto out;
	}

	/* Query the device about persistent XMR settings */
	ret = zbc_zone_activation_ctl(dev, &ctl, false);
	if (ret != 0) {
		fprintf(stderr,
			"[TEST][ERROR],zbc_zone_activation_ctl get failed %d\n",
			ret);
		ret = 1;
		goto out;
	}

	if (set_nz) {
		ctl.zbt_nr_zones = nz;
		upd = true;
	}
	if (set_urswrz) {
		ctl.zbt_urswrz = urswrz ? 0x01 : 0x00;
		upd = true;
	}
	if (set_max_activate) {
		ctl.zbt_max_activate = max_activate;
		upd = true;
	}

	if (upd) {
		if (!set_nz)
			ctl.zbt_nr_zones = 0xffffffff;
		if (!set_urswrz)
			ctl.zbt_urswrz = 0xff;
		if (!set_max_activate)
			ctl.zbt_max_activate = 0xffff;

		/* Need to change some values, request the device to update */
		ret = zbc_zone_activation_ctl(dev, &ctl, true);
		if (ret != 0) {
			fprintf(stderr,
				"[TEST][ERROR],zbc_zone_activation_ctl set failed %d\n",
				ret);
			ret = 1;
			goto out;
		}

		/* Read back all the persistent XMR settings */
		ret = zbc_zone_activation_ctl(dev, &ctl, false);
		if (ret != 0) {
			fprintf(stderr,
				"[TEST][ERROR],zbc_zone_activation_ctl get failed %d\n",
				ret);
			ret = 1;
			goto out;
		}
	}

	if (!quiet)
		printf("[FSNOZ],%u\n[URSWRZ],%s\n[MAX_ACTIVATION],%u\n",
		       ctl.zbt_nr_zones, ctl.zbt_urswrz ? "Y" : "N",
		       ctl.zbt_max_activate);

out:
	if (ret != 0) {
		struct zbc_errno zbc_err;
		const char *sk_name;
		const char *ascq_name;

		zbc_errno(dev, &zbc_err);
		sk_name = zbc_sk_str(zbc_err.sk);
		ascq_name = zbc_asc_ascq_str(zbc_err.asc_ascq);

		printf("[TEST][ERROR][SENSE_KEY],%s\n", sk_name);
		printf("[TEST][ERROR][ASC_ASCQ],%s\n", ascq_name);
	}

	zbc_close(dev);

	return ret;
}

