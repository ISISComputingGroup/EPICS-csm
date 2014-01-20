TOP=..
include $(TOP)/configure/CONFIG

LIBRARY_IOC = csmbase
csmbase_SRCS = csmbase.c
csmbase_LIBS += $(EPICS_BASE_IOC_LIBS)

INC = csmbase.h

DOCS += html latex man

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

$(COMMON_DIR)/csmbase.h: csmbase.c $(COMMON_DIR)/hgen.pl
	$(PERL) $(COMMON_DIR)/hgen.pl -c -h $@ $<

$(COMMON_DIR)/hgen.pl: ../hgen.pl
	$(CP) $< $@

$(COMMON_DIR)/%: csmbase.c ../Doxyfile.external
#	$(RMDIR) $@
#	$(MKDIR) $@
#	(echo "OUTPUT_DIRECTORY=$(COMMON_DIR)"; echo "INPUT=.."; echo QUIET=YES) | cat ../Doxyfile.external - | doxygen -

$(INSTALL_DOC)/%: $(COMMON_DIR)/%
	@echo "Installing doc $@"
#	-$(MKDIR) -p $(INSTALL_DOC)
#	$(CP) $< $(INSTALL_DOC)

.PRECIOUS: $(DOCS:%=$(COMMON_DIR)/%)
