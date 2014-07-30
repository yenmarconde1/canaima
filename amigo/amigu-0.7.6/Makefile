PYTHON=`which python`
BUILDIR=$(CURDIR)/debian/amigu
PROJECT=amigu
VERSION=0.7.6

all:
	@echo "make source - Create source package"
	@echo "make install - Install on local system"
	@echo "make buildrpm - Generate a rpm package"
	@echo "make builddeb - Generate a deb package"
	@echo "make clean - Get rid of scratch and byte files"

source:
	$(PYTHON) setup.py sdist $(COMPILE)

install:
	$(PYTHON) setup.py install --root $(DESTDIR) $(COMPILE)

clean:
	$(PYTHON) setup.py clean
	rm -rf build/ dist/ MANIFEST
	find . -name '*.pyc' -delete
