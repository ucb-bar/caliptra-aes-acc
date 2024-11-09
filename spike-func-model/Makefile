ifndef RISCV
$(error RISCV is unset)
else
$(info Running with RISCV=$(RISCV))
endif

PREFIX ?= $(RISCV)/
EXTENSION_NAME ?= crypto
SO := lib$(EXTENSION_NAME).so

CURRENT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
OPENSSL_BIN := $(shell which openssl)

default: $(SO)

$(SO): crypto.cc
	g++ -L $(RISCV)/lib -Wl,-rpath,$(RISCV)/lib -shared -o $@ -std=c++20 -I $(RISCV)/include -fPIC -O3 \
		-DEXTENSION_NAME=$(EXTENSION_NAME) \
		-DCUR_DIR=$(CURRENT_DIR) \
		-DOPENSSL_BIN=$(OPENSSL_BIN) \
		$^

.PHONY: install
install: $(SO)
	cp $(SO) $(RISCV)/lib

clean:
	rm -rf *.o $(SO)
