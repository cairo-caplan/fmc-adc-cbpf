
LINUX ?= /lib/modules/$(shell uname -r)/build
ZIO ?= $(src)/../zio
FMC_BUS ?= $(src)/../fmc-bus
SPEC_SW ?= $(src)/../spec-sw
SVEC_SW ?= $(src)/../svec-sw
CONFIG_FMC_ADC_SVEC ?= CONFIG_VME

KBUILD_EXTRA_SYMBOLS := \
	$(ZIO)/Module.symvers \
	$(FMC_BUS)/kernel/Module.symvers

GIT_VERSION = $(shell cd $(src); git describe --always --dirty --long --tags)

# For this CSM_VERSION, please see ohwr.org/csm documentation
ifdef CONFIG_CSM_VERSION
  ccflags-y += -D"CERN_SUPER_MODULE=MODULE_VERSION(\"$(CONFIG_CSM_VERSION)\")"
else
  ccflags-y += -DCERN_SUPER_MODULE=""
endif

ccflags-y += -DGIT_VERSION=\"$(GIT_VERSION)\" \
	-I$(ZIO)/include \
	-I$(FMC_BUS)/kernel/include \
	-I$(SPEC_SW)/kernel \
	-I$(SVEC_SW)/kernel \
	-I$(src)

ccflags-$(CONFIG_FMC_ADC_DEBUG) += -DDEBUG
ccflags-$(CONFIG_FMC_ADC_SVEC) += -DCONFIG_FMC_ADC_SVEC


subdirs-ccflags-y = $(ccflags-y)

obj-m := fmc-adc-100m14b.o
fmc-adc-100m14b-y =  fa-core.o
fmc-adc-100m14b-y += fa-zio-drv.o
fmc-adc-100m14b-y += fa-calibration.o
fmc-adc-100m14b-y += fa-regtable.o
fmc-adc-100m14b-y += fa-zio-trg.o
fmc-adc-100m14b-y += fa-irq.o
fmc-adc-100m14b-y += onewire.o
fmc-adc-100m14b-y += spi.o
fmc-adc-100m14b-y += fmc-util.o
fmc-adc-100m14b-y += fa-spec-core.o
fmc-adc-100m14b-y += fa-spec-regtable.o
fmc-adc-100m14b-y += fa-spec-dma.o
fmc-adc-100m14b-y += fa-spec-irq.o
fmc-adc-100m14b-$(CONFIG_FMC_ADC_SVEC) += fa-svec-core.o
fmc-adc-100m14b-$(CONFIG_FMC_ADC_SVEC) += fa-svec-regtable.o
fmc-adc-100m14b-$(CONFIG_FMC_ADC_SVEC) += fa-svec-dma.o
fmc-adc-100m14b-y += zio-helpers.o

all modules:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) modules

install modules_install:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) modules_install

# be able to run the "clean" rule even if $(LINUX) is not valid
clean:
	rm -rf *.o *~  .*.cmd *.ko *.mod.c .tmp_versions Module.symvers \
		Module.markers modules.order
