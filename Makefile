#will be /usr/local/trafficserver in prod
BASEDIR=/usr/
INSTALLDIR=/usr/local/trafficserver/modules
#INSTALLDIR=/usr/libexec/trafficserver/

PATH=/usr/local/bin:/usr/bin:/bin::/usr/local/trafficserver/bin  #access to tsxs
CXXFLAGS="-O0 -g2 -L/usr/lib -L/usr/local/lib"
TSXS="$(BASEDIR)/bin/tsxs"
INSTALL=install

INCFLAGS=-I "./include" -I "/usr/include/mysql"
LIBLIST=-l re2 -l zmq -l config++

all:	banjax

banjax:
	$(TSXS) -o banjax.so $(INCFLAGS) $(LIBLIST) -C src/*.cpp

clean:
	rm -fr banjax.so

install:
	mkdir -p $(DESTDIR)$(INSTALLDIR)
	$(INSTALL) banjax.so $(DESTDIR)$(INSTALLDIR)
