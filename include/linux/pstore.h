/*
 * Persistent Storage - pstore.h
 *
 * Copyright (C) 2010 Intel Corporation <tony.luck@intel.com>
 *
 * This code is the generic layer to export data records from platform
 * level persistent storage via a file system.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _LINUX_PSTORE_H
#define _LINUX_PSTORE_H

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/kmsg_dump.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/types.h>

#ifdef OPLUS_FEATURE_DUMPDEVICE
#include <linux/pstore_ram.h>
#endif /* OPLUS_FEATURE_DUMPDEVICE */

struct module;

/* pstore record types (see fs/pstore/inode.c for filename templates) */
enum pstore_type_id {
	PSTORE_TYPE_DMESG	= 0,
	PSTORE_TYPE_MCE		= 1,
	PSTORE_TYPE_CONSOLE	= 2,
	PSTORE_TYPE_FTRACE	= 3,
	/* PPC64 partition types */
	PSTORE_TYPE_PPC_RTAS	= 4,
	PSTORE_TYPE_PPC_OF	= 5,
	PSTORE_TYPE_PPC_COMMON	= 6,
	PSTORE_TYPE_PMSG	= 7,
	PSTORE_TYPE_PPC_OPAL	= 8,
#ifdef OPLUS_FEATURE_DUMPDEVICE
	PSTORE_TYPE_DEVICE_INFO	= 9,
#endif
	PSTORE_TYPE_UNKNOWN	= 255
};

struct pstore_info;
/**
 * struct pstore_record - details of a pstore record entry
 * @psi:	pstore backend driver information
 * @type:	pstore record type
 * @id:		per-type unique identifier for record
 * @time:	timestamp of the record
 * @buf:	pointer to record contents
 * @size:	size of @buf
 * @ecc_notice_size:
 *		ECC information for @buf
 *
 * Valid for PSTORE_TYPE_DMESG @type:
 *
 * @count:	Oops count since boot
 * @reason:	kdump reason for notification
 * @part:	position in a multipart record
 * @compressed:	whether the buffer is compressed
 *
 */
struct pstore_record {
	struct pstore_info	*psi;
	enum pstore_type_id	type;
	u64			id;
	struct timespec64	time;
	char			*buf;
	ssize_t			size;
	ssize_t			ecc_notice_size;

	int			count;
	enum kmsg_dump_reason	reason;
	unsigned int		part;
	bool			compressed;
};

/**
 * struct pstore_info - backend pstore driver structure
 *
 * @owner:	module which is repsonsible for this backend driver
 * @name:	name of the backend driver
 *
 * @buf_lock:	semaphore to serialize access to @buf
 * @buf:	preallocated crash dump buffer
 * @bufsize:	size of @buf available for crash dump bytes (must match
 *		smallest number of bytes available for writing to a
 *		backend entry, since compressed bytes don't take kindly
 *		to being truncated)
 *
 * @read_mutex:	serializes @open, @read, @close, and @erase callbacks
 * @flags:	bitfield of frontends the backend can accept writes for
 * @data:	backend-private pointer passed back during callbacks
 *
 * Callbacks:
 *
 * @open:
 *	Notify backend that pstore is starting a full read of backend
 *	records. Followed by one or more @read calls, and a final @close.
 *
 *	@psi:	in: pointer to the struct pstore_info for the backend
 *
 *	Returns 0 on success, and non-zero on error.
 *
 * @close:
 *	Notify backend that pstore has finished a full read of backend
 *	records. Always preceded by an @open call and one or more @read
 *	calls.
 *
 *	@psi:	in: pointer to the struct pstore_info for the backend
 *
 *	Returns 0 on success, and non-zero on error. (Though pstore will
 *	ignore the error.)
 *
 * @read:
 *	Read next available backend record. Called after a successful
 *	@open.
 *
 *	@record:
 *		pointer to record to populate. @buf should be allocated
 *		by the backend and filled. At least @type and @id should
 *		be populated, since these are used when creating pstorefs
 *		file names.
 *
 *	Returns record size on success, zero when no more records are
 *	available, or negative on error.
 *
 * @write:
 *	A newly generated record needs to be written to backend storage.
 *
 *	@record:
 *		pointer to record metadata. When @type is PSTORE_TYPE_DMESG,
 *		@buf will be pointing to the preallocated @psi.buf, since
 *		memory allocation may be broken during an Oops. Regardless,
 *		@buf must be proccesed or copied before returning. The
 *		backend is also expected to write @id with something that
 *		can help identify this record to a future @erase callback.
 *		The @time field will be prepopulated with the current time,
 *		when available. The @size field will have the size of data
 *		in @buf.
 *
 *	Returns 0 on success, and non-zero on error.
 *
 * @write_user:
 *	Perform a frontend write to a backend record, using a specified
 *	buffer that is coming directly from userspace, instead of the
 *	@record @buf.
 *
 *	@record:	pointer to record metadata.
 *	@buf:		pointer to userspace contents to write to backend
 *
 *	Returns 0 on success, and non-zero on error.
 *
 * @erase:
 *	Delete a record from backend storage.  Different backends
 *	identify records differently, so entire original record is
 *	passed back to assist in identification of what the backend
 *	should remove from storage.
 *
 *	@record:	pointer to record metadata.
 *
 *	Returns 0 on success, and non-zero on error.
 *
 */
struct pstore_info {
	struct module	*owner;
	char		*name;

	struct semaphore buf_lock;
	char		*buf;
	size_t		bufsize;

	struct mutex	read_mutex;

	int		flags;
	void		*data;

	int		(*open)(struct pstore_info *psi);
	int		(*close)(struct pstore_info *psi);
	ssize_t		(*read)(struct pstore_record *record);
	int		(*write)(struct pstore_record *record);
	int		(*write_user)(struct pstore_record *record,
				      const char __user *buf);
	int		(*erase)(struct pstore_record *record);
};

/* Supported frontends */
#define PSTORE_FLAGS_DMESG	(1 << 0)
#define PSTORE_FLAGS_CONSOLE	(1 << 1)
#define PSTORE_FLAGS_FTRACE	(1 << 2)
#define PSTORE_FLAGS_PMSG	(1 << 3)

extern int pstore_register(struct pstore_info *);
extern void pstore_unregister(struct pstore_info *);

struct pstore_ftrace_record {
	unsigned long ip;
	unsigned long parent_ip;
	u64 ts;
};

/*
 * ftrace related stuff: Both backends and frontends need these so expose
 * them here.
 */

#if NR_CPUS <= 2 && defined(CONFIG_ARM_THUMB)
#define PSTORE_CPU_IN_IP 0x1
#elif NR_CPUS <= 4 && defined(CONFIG_ARM)
#define PSTORE_CPU_IN_IP 0x3
#endif

#define TS_CPU_SHIFT 8
#define TS_CPU_MASK (BIT(TS_CPU_SHIFT) - 1)

/*
 * If CPU number can be stored in IP, store it there, otherwise store it in
 * the time stamp. This means more timestamp resolution is available when
 * the CPU can be stored in the IP.
 */
#ifdef PSTORE_CPU_IN_IP
static inline void
pstore_ftrace_encode_cpu(struct pstore_ftrace_record *rec, unsigned int cpu)
{
	rec->ip |= cpu;
}

static inline unsigned int
pstore_ftrace_decode_cpu(struct pstore_ftrace_record *rec)
{
	return rec->ip & PSTORE_CPU_IN_IP;
}

static inline u64
pstore_ftrace_read_timestamp(struct pstore_ftrace_record *rec)
{
	return rec->ts;
}

static inline void
pstore_ftrace_write_timestamp(struct pstore_ftrace_record *rec, u64 val)
{
	rec->ts = val;
}
#else
static inline void
pstore_ftrace_encode_cpu(struct pstore_ftrace_record *rec, unsigned int cpu)
{
	rec->ts &= ~(TS_CPU_MASK);
	rec->ts |= cpu;
}

static inline unsigned int
pstore_ftrace_decode_cpu(struct pstore_ftrace_record *rec)
{
	return rec->ts & TS_CPU_MASK;
}

static inline u64
pstore_ftrace_read_timestamp(struct pstore_ftrace_record *rec)
{
	return rec->ts >> TS_CPU_SHIFT;
}

static inline void
pstore_ftrace_write_timestamp(struct pstore_ftrace_record *rec, u64 val)
{
	rec->ts = (rec->ts & TS_CPU_MASK) | (val << TS_CPU_SHIFT);
}
#endif

#ifdef OPLUS_FEATURE_DUMPDEVICE
/*move from ram.c*/
struct ramoops_context {
	struct persistent_ram_zone **dprzs;	/* Oops dump zones */
	struct persistent_ram_zone *cprz;	/* Console zone */
	struct persistent_ram_zone **fprzs;	/* Ftrace zones */
	struct persistent_ram_zone *mprz;	/* PMSG zone */
	struct persistent_ram_zone *dprz;
	phys_addr_t phys_addr;
	unsigned long size;
	unsigned int memtype;
	size_t record_size;
	size_t console_size;
	size_t ftrace_size;
	size_t pmsg_size;
	size_t device_info_size;
	int dump_oops;
	u32 flags;
	struct persistent_ram_ecc_info ecc_info;
	unsigned int max_dump_cnt;
	unsigned int dump_write_cnt;
	/* _read_cnt need clear on ramoops_pstore_open */
	unsigned int dump_read_cnt;
	unsigned int console_read_cnt;
	unsigned int max_ftrace_cnt;
	unsigned int ftrace_read_cnt;
	unsigned int pmsg_read_cnt;
	unsigned int device_info_read_cnt;
	struct pstore_info pstore;
};
#endif

#endif /*_LINUX_PSTORE_H*/
