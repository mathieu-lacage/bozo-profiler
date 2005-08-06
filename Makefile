OUTPUT_DIR=bin

CFLAGS += -Ilibdebug -Ilibprofiler -DRUN_SELF_TESTS -Wall -Werror

LIBDEBUG_SRC = \
 libdebug/breakpoint.c \
 libdebug/symbol.c \
 libdebug/memory-reader.c \
 libdebug/elf32-parser.c \
 libdebug/elf32-debug.c \
 libdebug/user-ctx.c \
 libdebug/mmap-cache.c \
 libdebug/circular-buffer.c \
 libdebug/sllist.c \
 libdebug/utils.c \
 libdebug/utils-other.c \
 libdebug/load-map.c \
 libdebug/pthread-utils.c \
 libdebug/dwarf2-parser.c \
 libdebug/dwarf2-utils.c \
 libdebug/dwarf2-line.c \
 libdebug/timestamp.c \
 libdebug/x86-opcode.c \


LIBPROFILER_SRC= \
 libprofiler/manager.c \
 libprofiler/record.c \


TEST_SRC= \
 test/test.c \
 test/test-breakpoint.c \
 test/test-elf32.c \
 test/test-fn-address.c \
 test/test-thread-db.c \
 test/test-ld-brk.c \
 test/test-symbol.c \
 test/test-circular-buffer.c \
 test/test-dwarf2.c \
 test/test-performance.c \
 test/test-ti.c \
 test/test-profiler.c \
 test/test-circular-buffer.c \
 test/test-opcode.c \


READ_DUMP_SRC= \
 read-dump/read-dump.c \
 read-dump/outside-map.c \
 read-dump/dwarf2-cache.c \



#CC=$$HOME/bin/bin.linux2/insure
ifdef USE_DEBUGGING_LIBC
 LIBC_DIR=/usr/src/redhat/BUILD/glibc-20041021T0701
 LIBC_INSTALL_DIR=$(LIBC_DIR)/build/
 LDFLAGS +=-B$(LIBC_INSTALL_DIR) -B$(LIBC_INSTALL_DIR)/lib \
   -Wl,-rpath,$(LIBC_INSTALL_DIR)/lib,-dynamic-linker,$(LIBC_INSTALL_DIR)/lib/ld-linux.so.2 \
   -L$(LIBC_INSTALL_DIR) -lc 
 CFLAGS +=-B$(LIBC_INSTALL_DIR) -B$(LIBC_INSTALL_DIR)/lib -gdwarf-2
else
 CFLAGS +=-gdwarf-2 -O3
endif

LIBDEBUG_TARGET=$(OUTPUT_DIR)/libdebug/libdebug.a
LIBDEBUG_PIC_TARGET=$(OUTPUT_DIR)/libdebug/libdebug-pic.a
LIBPROFILER_TARGET=$(OUTPUT_DIR)/libprofiler/libprofiler.so
READ_DUMP_TARGET=$(OUTPUT_DIR)/read-dump/read-dump

TEST_OBJ=$(addprefix $(OUTPUT_DIR)/,$(addsuffix .o,$(basename $(TEST_SRC))))
LIBDEBUG_OBJ=$(addprefix $(OUTPUT_DIR)/,$(addsuffix .o,$(basename $(LIBDEBUG_SRC))))
LIBDEBUG_PIC_OBJ=$(addprefix $(OUTPUT_DIR)/,$(addsuffix -pic.o,$(basename $(LIBDEBUG_SRC))))
LIBPROFILER_OBJ=$(addprefix $(OUTPUT_DIR)/,$(addsuffix .o,$(basename $(LIBPROFILER_SRC))))
READ_DUMP_OBJ=$(addprefix $(OUTPUT_DIR)/,$(addsuffix .o,$(basename $(READ_DUMP_SRC))))

OBJ_FILES=$(TEST_OBJ) $(LIBDEBUG_OBJ) $(LIBDEBUG_PIC_OBJ) $(LIBPROFILER_OBJ) $(READ_DUMP_OBJ)

OUTPUT_DIRS= \
 $(OUTPUT_DIR) \
 $(OUTPUT_DIR)/test \
 $(OUTPUT_DIR)/libdebug \
 $(OUTPUT_DIR)/read-dump \
 $(OUTPUT_DIR)/libprofiler \


ALL_TARGETS = \
 $(OUTPUT_DIRS) \
 $(LIBDEBUG_TARGET) \
 $(LIBDEBUG_PIC_TARGET) \
 $(LIBPROFILER_TARGET) \
 $(READ_DUMP_TARGET) \
 $(OUTPUT_DIR)/test/libtest-fn-address.so \
 $(OUTPUT_DIR)/test/libfoo.so \
 $(OUTPUT_DIR)/test/test \



all: $(ALL_TARGETS)

test:  $(ALL_TARGETS)

$(LIBDEBUG_TARGET): $(LIBDEBUG_OBJ)
$(LIBDEBUG_PIC_TARGET): $(LIBDEBUG_PIC_OBJ)
$(LIBPROFILER_TARGET): $(LIBPROFILER_OBJ) $(LIBDEBUG_PIC_OBJ)
$(READ_DUMP_TARGET): $(READ_DUMP_OBJ) $(LIBDEBUG_OBJ)

$(LIBPROFILER_TARGET): LDLIBS += -L$(OUTPUT_DIR)/libdebug -ldebug-pic -lpthread -ldl
$(READ_DUMP_TARGET): LDLIBS += -L$(OUTPUT_DIR)/libdebug -ldebug -ldl `pkg-config --libs glib-2.0`

$(OUTPUT_DIR)/read-dump/read-dump.o: CFLAGS += `pkg-config --cflags glib-2.0`
$(OUTPUT_DIR)/read-dump/dwarf2-cache.o: CFLAGS += `pkg-config --cflags glib-2.0`

$(OUTPUT_DIR)/test/test-profiler.o: CFLAGS += -finstrument-functions
$(OUTPUT_DIR)/test/fn-address.o: CFLAGS += -fPIC
$(OUTPUT_DIR)/test/libtest-fn-address.so: $(OUTPUT_DIR)/test/fn-address.o
$(OUTPUT_DIR)/test/foo.o: CFLAGS += -fPIC
$(OUTPUT_DIR)/test/libfoo.so: $(OUTPUT_DIR)/test/foo.o

TEST_LDFLAGS  = -L$(OUTPUT_DIR)/libprofiler -L$(OUTPUT_DIR)/libdebug -L$(OUTPUT_DIR)/test
TEST_LDFLAGS += -Wl,-rpath,$(OUTPUT_DIR)/libprofiler -Wl,-rpath,$(OUTPUT_DIR)/test
TEST_LDLIBS  += -ldebug -lprofiler -ltest-fn-address -lfoo -ldl -lpthread -lthread_db
$(OUTPUT_DIR)/test/test: LDLIBS += $(TEST_LDLIBS)
$(OUTPUT_DIR)/test/test: LDLIBS += $(TEST_LDFLAGS)
$(OUTPUT_DIR)/test/test: $(LIBDEBUG_TARGET) $(LIBPROFILER_TARGET) $(OUTPUT_DIR)/test/libfoo.so $(OUTPUT_DIR)/test/libtest-fn-address.so $(TEST_OBJ)




$(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(OUTPUT_DIR)/%-pic.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<
%.a:
	$(AR) cru $@ $^
%.so:
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUTPUT_DIRS):
	@[ -d $@ ] || mkdir -p $@;

clean:
	@for f in *~ .deps $(OBJ_FILES) $(ALL_TARGETS); do \
	  rm -f $$f >/dev/null 2>/dev/null; \
	done;

.PHONY: clean

.deps:
	@rm -f .deps; \
	for f in $(LIBDEBUG_SRC) $(LIBPROFILER_SRC) $(TEST_SRC) $(READ_DUMP_SRC); do \
	  OUTPUT_OBJ=`echo $$f|sed -e 's/\.c/\.o/'`; \
	  OUTPUT_PIC_OBJ=`echo $$f|sed -e 's/\.c/-pic\.o/'`; \
	  GLIB_CFLAGS=`pkg-config --cflags glib-2.0`; \
	  FLAGS="$(CFLAGS) $$GLIB_CFLAGS"; \
	  CC_COMMAND="gcc $$FLAGS -M -MT "$(OUTPUT_DIR)/$$OUTPUT_OBJ" $$f"; \
	  DEPS_LIST=`$$CC_COMMAND`; \
	  echo "$$DEPS_LIST" >> .deps; \
	  CC_COMMAND="gcc $$FLAGS -M -MT "$(OUTPUT_DIR)/$$OUTPUT_PIC_OBJ" $$f"; \
	  DEPS_LIST=`$$CC_COMMAND`; \
	  echo "$$DEPS_LIST" >> .deps; \
	done; \

-include .deps
