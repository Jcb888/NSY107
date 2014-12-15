#TARGET = tachesRoue
### Default Xenomai installation path
XENO ?= /usr/xenomai

XENOCONFIG=$(shell PATH=$(XENO):$(XENO)/bin:$(PATH) which xeno-config 2>/dev/null)

CC=$(shell $(XENOCONFIG) --cc) -g 

CFLAGS=$(shell $(XENOCONFIG) --xeno-cflags) $(MY_CFLAGS)

LDFLAGS=$(shell $(XENOCONFIG) --xeno-ldflags) $(MY_LDFLAGS) -lnative -lrtdk 

# This includes the library path of given Xenomai into the binary to make live
# easier for beginners if Xenomai's libs are not in any default search path.
LDFLAGS+=-Xlinker -rpath -Xlinker $(shell $(XENOCONFIG) --libdir)

all:: tachesRoue

clean::
	$(OBJS)

OBJS = tachesRoue.o libUtilRoue.o libmm32.o socketsTCP.o

tachesRoue: $(OBJS)
	$(CC) -o tachesRoue $(OBJS) $(CFLAGS) $(LDFLAGS)



