-include $(TOP_DIR)/config.mk
ifeq ($(KERNEL_SCHED), RR) 
	SUB_MODULES += rr
else
	SUB_MODULES += linux0.11
endif
-include $(MODULE)