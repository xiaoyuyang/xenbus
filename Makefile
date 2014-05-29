SUBDIRS = svm xa demo
.PHONY: build
build:
	$(MAKE) -C svm
	$(MAKE) -C xa
	$(MAKE) -C demo
