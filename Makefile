#   BSD LICENSE
#
#   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = l2fwd

CFLAGS += -O3 -g
LDFLAGS += -lpcap -lyaml -lcheck_pic -lrt -lm
CFLAGS += $(WERROR_FLAGS)

# all source are stored in SRCS-y
SRCS-y := \
	src/dss/bloomfilter.c \
	src/dss/countmin.c \
	src/dss/hashmap.c \
	src/dss/hashmap_linear.c \
	src/dss/hashmap_cuckoo.c \
	src/dss/hashmap_cuckoo_bucket.c \
	src/dss/pqueue.c \
	src/bootstrap.c \
	src/console.c \
	src/experiment.c \
	src/main.c \
	src/memory.c \
	src/module.c \
	src/net.c \
	src/pkt.c \
	src/reporter.c \
	src/vendor/murmur3.c \
	src/modules/count_array.c \
	src/modules/super_spreader.c \
	src/modules/ring.c \
	src/modules/consumer.c \
	src/modules/randmod.c \
	src/modules/heavyhitter/countmin.c \
	src/modules/heavyhitter/cuckoo.c \
	src/modules/heavyhitter/cuckoo_bucket.c \
	src/modules/heavyhitter/cuckoo_local.c \
	src/modules/heavyhitter/cuckoo_local_ptr.c \
	src/modules/heavyhitter/hashmap.c \
	src/modules/heavyhitter/hashmap_linear.c \
	src/modules/heavyhitter/hashmap_linear_ptr.c \
	src/modules/heavyhitter/pqueue.c \
	src/modules/superspreader/hashmap.c \
	src/modules/superspreader/hashmap_linear.c \
	src/modules/superspreader/cuckoo_local.c \
	src/tests/test.c

ifdef TESTS
CFLAGS += -DIS_TEST_BUILD=1
SRCS-y += tests/test.c
LDFLAGS += -lcheck_pic -lrt -lm
APP = test
endif


.PHONY: check
check: clean install

include $(RTE_SDK)/mk/rte.extapp.mk
