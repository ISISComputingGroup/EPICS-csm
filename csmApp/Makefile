TOP=..
include $(TOP)/configure/CONFIG

LIBRARY_IOC = csmbase

csmbase_SRCS = csmbase.c
INC = csmbase.h

DOCS += html latex man

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

$(COMMON_DIR)/csmbase.h: csmbase.c $(COMMON_DIR)/hgen.pl
	PATH=$$PATH:$(COMMON_DIR) hgen.pl -c -h $@ $<

$(COMMON_DIR)/hgen.pl: ../hgen.pl
	$(CP) $< $@
	chmod +x $@

$(COMMON_DIR)/%: csmbase.c ../Doxyfile.external
	rm -rf $@
	mkdir $@
	(echo "OUTPUT_DIRECTORY=$(COMMON_DIR)"; echo "INPUT=.."; echo QUIET=YES) | cat ../Doxyfile.external - | doxygen -

$(INSTALL_DOC)/%: $(COMMON_DIR)/%
	@echo "Installing doc $@"
	test -d $(INSTALL_DOC) || mkdir -p $(INSTALL_DOC)
	cp -r $< $(INSTALL_DOC)

.PRECIOUS: $(DOCS:%=$(COMMON_DIR)/%)
