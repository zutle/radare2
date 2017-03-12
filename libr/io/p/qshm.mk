OBJ_QSHM=io_qshm.o

STATIC_OBJ+=${OBJ_QSHM}
TARGET_QSHM=io_qshm.${EXT_SO}
ALL_TARGETS+=${TARGET_QSHM}

#ifeq (${WITHPIC},0)
#LINKFLAGS+=../../util/libr_util.a
#LINKFLAGS+=../../io/libr_io.a
#else
#LINKFLAGS+=-L../../util -lr_util
#LINKFLAGS+=-L.. -lr_io
#endif

${TARGET_QSHM}: ${OBJ_QSHM}
	${CC_LIB} $(call libname,io_qshm) ${CFLAGS} $(LDFLAGS) \
		-o ${TARGET_QSHM} ${OBJ_QSHM}
