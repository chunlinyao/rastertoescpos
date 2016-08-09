
EXEC=rastertoescpos
CFLAGS = $(shell cups-config --cflags)
LDFLAGS = $(shell cups-config --ldflags)
LDLIBS = $(shell cups-config --image --libs)
CUPSDATADIR = $(shell cups-config --datadir)
PPDPATH=${CUPSDATADIR}/model
CUPSDIR = $(shell cups-config --serverbin)

all: rastertoescpos ppd

.PHONY: ppd clean install uninstall

$(EXEC): rastertoescpos.c

ppd:
	ppdc escpos.drv

install:
	install -s $(EXEC) $(CUPSDIR)/filter/
	install -m 644 escpos.drv $(CUPSDATADIR)/drv/
	install -m 644 escposmedia.h $(CUPSDATADIR)/ppdc/
	

uninstall:
	rm -f $(CUPSDIR)/filter/$(EXEC)
	rm -f $(CUPSDATADIR)/drv/escpos.drv
	rm -f $(CUPSDATADIR)/ppdc/escposmedia.h


clean:
	rm -f rastertoescpos
	rm -rf ppd
