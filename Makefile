DIR ?= .
CC ?= gcc
PROG ?= $(DIR)/9cc

SRCS := $(wildcard $(DIR)/nc_*.c)
OBJS := $(SRCS:%.c=%.o)
DEPS := $(SRCS:%.c=%.d)
CFLAGS += -Wall -Wextra -Wcast-qual -Wfloat-equal -Winit-self -Winline -Wlogical-op -Wpointer-arith -Wredundant-decls -Wwrite-strings -g3 -DDEBUG
LFLAGS +=

.PHONY:	all claen deps
all:	$(PROG)
deps:	$(DEPS)

clean:
	rm -rf $(DIR)/*.o $(DIR)/*.d $(PROG) nc_function.inc nc_def.inc

$(PROG):	$(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS)

%.o:	%.c nc_def.inc nc_function.inc
	$(CC) -MMD -MP -c $< $(CFLAGS)

%.d:	%.c
	$(CC) -MM $< -MF $@

nc_function.inc: $(SRCS)
	cat $(SRCS)|sed -ne '/GLOBAL/p'|sed -e 's/^GLOBAL \(.*\){.*$$/\1;/' > $@

nc_def.inc:	nc_def.h
	cat nc_def.h|sed -ne '/GLODEF/p'|sed -e 's/^GLODEF *\(struct\|enum\)\s*\([A-Za-z0-9_]*\).*$$/typedef \1 \2 \2;/' > $@

-include $(DEPS)
