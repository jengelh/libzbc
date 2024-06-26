/* SPDX-License-Identifier: BSD-2-Clause */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * This file is part of libzbc.
 *
 * Copyright (C) 2009-2014, HGST, Inc. All rights reserved.
 * Copyright (C) 2016, Western Digital. All rights reserved.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 *          Christoph Hellwig (hch@infradead.org)
 */
#ifndef __LIBZBC_INTERNAL_H__
#define __LIBZBC_INTERNAL_H__

#include "config.h"
#include "libzbc/zbc.h"
#include "zbc_private.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

/*
 * Memory page size information.
 */
#define PAGE_SIZE	(sysconf(_SC_PAGESIZE))
#define PAGE_MASK	(PAGE_SIZE - 1)

/**
 * Backend driver descriptor.
 */
struct zbc_drv {

	/**
	 * Driver flag.
	 */
	unsigned int	flag;

	/**
	 * Open device.
	 */
	int		(*zbd_open)(const char *, int, struct zbc_device **);

	/**
	 * Close device.
	 */
	int		(*zbd_close)(struct zbc_device *);

	/**
	 * Get or set XMR device configuration parameters.
	 */
	int		(*zbd_dev_control)(struct zbc_device *,
					   struct zbc_zd_dev_control *, bool);

	/**
	 * Report a device zone information.
	 */
	int		(*zbd_report_zones)(struct zbc_device *, uint64_t,
					    enum zbc_zone_reporting_options,
					    struct zbc_zone *, unsigned int *);

	/**
	 * Execute a zone operation.
	 */
	int		(*zbd_zone_op)(struct zbc_device *, uint64_t,
				       unsigned int, enum zbc_zone_op,
				       unsigned int);

	/**
	 * Report zone domain information.
	 */
	int		(*zbd_report_domains)(struct zbc_device *, uint64_t,
					      enum zbc_domain_report_options,
					      struct zbc_zone_domain *,
					      unsigned int);
	/**
	 * Report zone realm configuration.
	 */
	int		(*zbd_report_realms)(struct zbc_device *, uint64_t,
					     enum zbc_realm_report_options,
					     struct zbc_zone_realm *,
					     unsigned int *);

	/**
	 * Activate zones as a CMR or SMR type or query
	 * about the possible results of such activation.
	 */
	int		(*zbd_zone_query_actv)(struct zbc_device *, bool, bool,
					       bool, bool, uint64_t,
					       unsigned int, unsigned int,
					       struct zbc_actv_res *,
					       uint32_t *);

	/**
	 * Vector read from a ZBC device.
	 */
	ssize_t		(*zbd_preadv)(struct zbc_device *,
				      const struct iovec *, int, uint64_t);

	/**
	 * Vector write to a ZBC device.
	 */
	ssize_t		(*zbd_pwritev)(struct zbc_device *,
				       const struct iovec *, int, uint64_t);

	/**
	 * Flush to a ZBC device cache.
	 */
	int		(*zbd_flush)(struct zbc_device *);

	/**
	 * Zoned Block Device statistics (optional).
	 */
	int		(*zbd_get_stats)(struct zbc_device *,
					 struct zbc_zoned_blk_dev_stats *);
};

/**
 * Device descriptor.
 */
struct zbc_device {

	/**
	 * Device file path.
	 */
	char			*zbd_filename;

	/**
	 * Device file descriptor.
	 */
	int			zbd_fd;

	/**
	 * File descriptor used for SG_IO. For block devices, this
	 * may be different from zbd_fd.
	 */
	int			zbd_sg_fd;

	/**
	 * Device operations.
	 */
	struct zbc_drv		*zbd_drv;

	/**
	 * Device info.
	 */
	struct zbc_device_info	zbd_info;

	/**
	 * Device open flags.
	 */
	unsigned int		zbd_o_flags;

	/**
	 * Device backend driver flags.
	 */
	unsigned int		zbd_drv_flags;

	/**
	 * Report zone buffer size alignment.
	 */
	size_t			zbd_report_bufsz_mask;

	/**
	 * Report zone buffer size minimum size.
	 */
	size_t			zbd_report_bufsz_min;

};

/**
 * Per-thread local zbc_errno handling.
 */
extern __thread struct zbc_err_ext zerrno;

static inline void zbc_set_errno(enum zbc_sk sk, enum zbc_asc_ascq asc_ascq)
{
	zerrno.sk = sk;
	zerrno.asc_ascq = asc_ascq;
}

static inline void zbc_clear_errno(void)
{
	memset(&zerrno, 0, sizeof(zerrno));
}

/**
 * Test if a device is zoned.
 */
#define zbc_dev_model(dev)		((dev)->zbd_info.zbd_model)
#define zbc_dev_is_zoned(dev)		(zbc_dev_model(dev) == ZBC_DM_HOST_MANAGED || \
					 zbc_dev_model(dev) == ZBC_DM_HOST_AWARE)
/*
 * Zone Domains device property checks.
 */
#define zbc_dev_is_zdr(dev)		(zbc_dev_is_zoned(dev) && \
					 (((dev)->zbd_info.zbd_flags & \
					   ZBC_ZONE_DOMAINS_SUPPORT) || \
					 ((dev)->zbd_info.zbd_flags & \
					   ZBC_ZONE_REALMS_SUPPORT)))
#define zbc_supp_report_realms(dev)	((dev)->zbd_info.zbd_flags & \
					 ZBC_REPORT_REALMS_SUPPORT)
#define zbc_supp_za_control(dev)	((dev)->zbd_info.zbd_flags & \
					 ZBC_ZA_CONTROL_SUPPORT)
#define zbc_supp_nozsrc(dev)		((dev)->zbd_info.zbd_flags & \
					 ZBC_ZA_NOZSRC_SUPPORT)
#define zbc_supp_conv_zone(dev)		((dev)->zbd_info.zbd_flags & \
					 ZBC_CONV_ZONE_SUPPORT)
#define zbc_supp_seq_req_zone(dev)	((dev)->zbd_info.zbd_flags & \
					 ZBC_SEQ_REQ_ZONE_SUPPORT)
#define zbc_supp_seq_pref_zone(dev)	((dev)->zbd_info.zbd_flags & \
					 ZBC_SEQ_PREF_ZONE_SUPPORT)
#define zbc_supp_sobr_zone(dev)		((dev)->zbd_info.zbd_flags & \
					 ZBC_SOBR_ZONE_SUPPORT)

/**
 * Device open access mode and allowed drivers mask.
 */
#define ZBC_O_MODE_MASK		(O_RDONLY | O_WRONLY | O_RDWR)
#define ZBC_O_DMODE_MASK	(ZBC_O_MODE_MASK | O_DIRECT)
#define ZBC_O_DRV_MASK		(ZBC_O_DRV_SCSI | ZBC_O_DRV_ATA)
#define ZBC_O_TEST_DRV_MASK	(ZBC_O_DRV_SCSI | ZBC_O_DRV_ATA)

/**
 * Test if a device is in test mode.
 */
#ifdef HAVE_DEVTEST
#define zbc_test_mode(dev)	((dev)->zbd_o_flags & ZBC_O_DEVTEST)
#else
#define zbc_test_mode(dev)	(false)
#endif

/**
 * ZAC (ATA) device driver (uses SG_IO).
 */
extern struct zbc_drv zbc_ata_drv;

/**
 * ZBC (SCSI) device driver (uses SG_IO).
 */
extern struct zbc_drv zbc_scsi_drv;

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * REPORT ZONES reporting option mask.
 */
#define zbc_rz_ro_mask(ro)		((ro) & 0x3f)

/**
 * Logical block to sector conversion.
 */
#define zbc_dev_sect2lba(dev, sect)	(sect ? zbc_sect2lba(&(dev)->zbd_info, sect) : sect)
#define zbc_dev_lba2sect(dev, lba)	(lba ? zbc_lba2sect(&(dev)->zbd_info, lba) : lba)

/**
 * Check sector alignment to logical block.
 */
#define zbc_dev_sect_laligned(dev, sect)	\
	((((sect) << 9) & ((dev)->zbd_info.zbd_lblock_size - 1)) == 0)

/**
 * Check sector alignment to physical block.
 */
#define zbc_dev_sect_paligned(dev, sect)	\
	((((sect) << 9) & ((dev)->zbd_info.zbd_pblock_size - 1)) == 0)

/**
 * Count total size of vector buffers.
 */
static inline size_t zbc_iov_count(const struct iovec *iov, int iovcnt)
{
	size_t count = 0;
	int i;

	for (i = 0; i < iovcnt; i++)
		count += iov[i].iov_len;

	return count;
}

/**
 * The ATA backend driver may use the SCSI backend I/O functions.
 */
ssize_t zbc_scsi_preadv(struct zbc_device *dev,
			const struct iovec *iov, int iovcnt, uint64_t offset);
ssize_t zbc_scsi_pwritev(struct zbc_device *dev,
			 const struct iovec *iov, int iovcnt, uint64_t offset);
int zbc_scsi_flush(struct zbc_device *dev);

/**
 * Get device capacity information of an ATA device.
 */
int zbc_ata_get_capacity(struct zbc_device *dev);

/**
 * Log levels.
 */
enum {
	ZBC_LOG_NONE = 0,	/* Disable all messages */
	ZBC_LOG_WARNING,	/* Critical errors (invalid drive,...) */
	ZBC_LOG_ERROR,		/* Normal errors (I/O errors etc) */
	ZBC_LOG_INFO,		/* Informational */
	ZBC_LOG_DEBUG,		/* Debug-level messages */
	ZBC_LOG_MAX
};

/**
 * Library log level.
 */
extern int zbc_log_level;

#define zbc_print(stream, format, args...)		\
	do {						\
		fprintf((stream), format, ## args);     \
		fflush(stream);                         \
	} while (0)

/**
 * Log level controlled messages.
 */
#define zbc_print_level(l, stream, format, args...)		\
	do {							\
		if ((l) <= zbc_log_level)			\
			zbc_print((stream), "(libzbc/%d) "	\
				  format,			\
				  getpid(), ## args);		\
	} while (0)

#define zbc_warning(format, args...)	\
	zbc_print_level(ZBC_LOG_WARNING, stderr, "[WARNING] " format, ##args)

#define zbc_error(format, args...)	\
	zbc_print_level(ZBC_LOG_ERROR, stderr, "[ERROR] " format, ##args)

#define zbc_info(format, args...)	\
	zbc_print_level(ZBC_LOG_INFO, stdout, format, ##args)

#define zbc_debug(format, args...)	\
	zbc_print_level(ZBC_LOG_DEBUG, stdout, format, ##args)

#define zbc_panic(format, args...)	\
	do {						\
		zbc_print_level(ZBC_LOG_ERROR,		\
				stderr,			\
				"[PANIC] " format,      \
				##args);                \
		assert(0);                              \
	} while (0)

#define zbc_assert(cond)					\
	do {							\
		if (!(cond))					\
			zbc_panic("Condition %s failed\n",	\
				  # cond);			\
	} while (0)

#endif

/* __LIBZBC_INTERNAL_H__ */
