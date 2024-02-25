# Copyright (c) 2023 Stappler LLC <admin@stappler.dev>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# current dir
STAPPLER_MODULE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

STAPPLER_VERSION_NUMBER := 1
STAPPLER_VERSION_BUILD := 2

STAPPLER_CRYPTO_DEFAULT ?= openssl

include $(STAPPLER_MODULE_DIR)/core/core.mk
include $(STAPPLER_MODULE_DIR)/bitmap/bitmap.mk
include $(STAPPLER_MODULE_DIR)/data/data.mk
include $(STAPPLER_MODULE_DIR)/db/db.mk
include $(STAPPLER_MODULE_DIR)/filesystem/filesystem.mk
include $(STAPPLER_MODULE_DIR)/threads/threads.mk
include $(STAPPLER_MODULE_DIR)/idn/idn.mk
include $(STAPPLER_MODULE_DIR)/crypto/crypto.mk
include $(STAPPLER_MODULE_DIR)/network/network.mk
include $(STAPPLER_MODULE_DIR)/search/search.mk
include $(STAPPLER_MODULE_DIR)/sql/sql.mk
include $(STAPPLER_MODULE_DIR)/threads/threads.mk
include $(STAPPLER_MODULE_DIR)/geom/geom.mk
include $(STAPPLER_MODULE_DIR)/font/font.mk
include $(STAPPLER_MODULE_DIR)/tess/tess.mk
include $(STAPPLER_MODULE_DIR)/vg/vg.mk
include $(STAPPLER_MODULE_DIR)/zip/zip.mk
include $(STAPPLER_MODULE_DIR)/backtrace.mk
