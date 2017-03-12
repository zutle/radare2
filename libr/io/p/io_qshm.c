/* Copyright (c) 2017, zutle, All rights reversed. */

#include <r_io.h>
#include <r_lib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

// TODO: #ifdef __UNIX__ 

typedef struct {
	int fd;
	ut8* buf;
	ut32 size;
	int rw;
} RIOQemuShm;
//#define RIOQEMUSHM_FD(x) (((RIOQemuShm*)x)->fd)

#define QSHM_PREFIX "qshm://"
#define QSHM_PREFIX_LEN 7

static int qshm__write(RIO *io, RIODesc *fd, const ut8 *buf, int count) {
	RIOQemuShm *qshm;
	if (!fd || !fd->data)
		return -1;
	qshm = fd->data;
	if (!qshm->buf || !(qshm->rw & R_IO_WRITE) || (io->off + count > qshm->size))
		return -1;
	memcpy (qshm->buf + io->off, buf, count);
	return count;
}

static int qshm__read(RIO *io, RIODesc *fd, ut8 *buf, int count) {
	RIOQemuShm *qshm;
	if (!fd || !fd->data)
		return -1;
	qshm = fd->data;
	if (io->off > qshm->size)
		return -1;
	if (io->off + count >= qshm->size)
		count = qshm->size - io->off;
	memcpy (buf, qshm->buf + io->off, count);
	return count;
}

static ut64 qshm__lseek(RIO *io, RIODesc *fd, ut64 offset, int whence) {
	RIOQemuShm *qshm = fd->data;
	switch (whence) {
	case SEEK_SET:
		io->off = offset;
		break;
	case SEEK_CUR:		
		if ((io->off + (int)offset) > qshm->size)
			io->off = qshm->size;
		else
			io->off += (int)offset;
		break;
	case SEEK_END:
		io->off = qshm->size;
		break;
	default:
		break;
	}
	return io->off;
}

static int qshm__close(RIODesc *fd) {
	RIOQemuShm *qshm;
	if (!fd || !fd->data)
		return -1;
	qshm = fd->data;
	munmap (qshm->buf, qshm->size);
	close (qshm->fd);
	free (fd->data);
	fd->data = NULL;
	return 0;
}

static RIODesc *qshm__open(RIO *io, const char *pathname, int rw, int mode) {
	RIOQemuShm *qshm;
	const char *filename;
	struct stat sb = { 0 };
	
	if (strncmp (pathname, QSHM_PREFIX, QSHM_PREFIX_LEN))
		return NULL;
	qshm = R_NEW0 (RIOQemuShm);
	if (!qshm)
		return NULL;

	filename = pathname + QSHM_PREFIX_LEN;

	if (rw & R_IO_WRITE)
		qshm->fd = open(filename, O_RDWR, mode);
	else
		qshm->fd = open(filename, O_RDONLY, mode);

	if (-1 == qshm->fd) {
		eprintf ("Failed to connect to Qemu shared memory (%d)\n", errno);
		goto fail;
	}
	if (-1 == fstat(qshm->fd, &sb)) {
		eprintf ("Failed to obtain the size of the Qemu shared memory (%d)\n", errno);
		goto fail;
	}

	qshm->size = sb.st_size;
	qshm->rw = rw;

	if (rw & R_IO_WRITE)
		qshm->buf = mmap(NULL, qshm->size, PROT_READ | PROT_WRITE, MAP_SHARED, qshm->fd, 0);
	else
		qshm->buf = mmap(NULL, qshm->size, PROT_READ, MAP_SHARED, qshm->fd, 0);

	if (MAP_FAILED == qshm->buf) {
		eprintf ("Failed to map Qemu shared memory into memory (%d)\n", errno);
		goto fail;
	}

	eprintf ("Connected to Qemu shared memory: %s\n", filename);
	return r_io_desc_new (&r_io_plugin_qshm, qshm->fd, pathname, rw, mode, qshm); 

fail:
	if (-1 != qshm->fd)
		close (qshm->fd);
	free (qshm);
	return NULL;
}

static int qshm__system(RIO *io, RIODesc *fd, const char *cmd) {
	eprintf ("system command (%s)\n", cmd);
	return true;
}

static bool qshm__plugin_open(RIO *io, const char *pathname, bool many) {
	return (!strncmp (pathname, QSHM_PREFIX, QSHM_PREFIX_LEN));
}

static int qshm__init(RIO *io) {
	return true;
}

RIOPlugin r_io_plugin_qshm = {
	.name = "qshm",
	.desc = "qemu shared memory resources (qshm://key)",
	.license = "LGPL3",
	.author = "zutle",
	.version = "0.1.0",
	.open = qshm__open,
	.close = qshm__close,
	.read = qshm__read,
	.write = qshm__write,
	.check = qshm__plugin_open,
	.lseek = qshm__lseek,
	.init = qshm__init,
	.system = qshm__system,
//	.isdbg = true,
};

#ifndef CORELIB
struct r_lib_struct_t radare_plugin = {
	.type = R_LIB_TYPE_IO,
	.data = &r_io_plugin_qshm,
	.version = R2_VERSION
}
#endif
