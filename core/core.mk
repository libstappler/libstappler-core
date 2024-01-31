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

MODULE_STAPPLER_CORE_PRECOMPILED_HEADERS := $(STAPPLER_MODULE_DIR)/core/SPCommon.h
MODULE_STAPPLER_CORE_SRCS_DIRS := $(STAPPLER_MODULE_DIR)/core $(STAPPLER_MODULE_DIR)/thirdparty
MODULE_STAPPLER_CORE_SRCS_OBJS :=
MODULE_STAPPLER_CORE_INCLUDES_DIRS := $(STAPPLER_MODULE_DIR)/core
MODULE_STAPPLER_CORE_INCLUDES_OBJS := $(STAPPLER_MODULE_DIR)/thirdparty $(OSTYPE_INCLUDE)
MODULE_STAPPLER_CORE_DEPENDS_ON :=
MODULE_STAPPLER_CORE_LIBS :=
MODULE_STAPPLER_CORE_FLAGS := \
	-DSTAPPLER_VERSION_NUMBER=$(STAPPLER_VERSION_NUMBER) \
	-DSTAPPLER_VERSION_BUILD=$(STAPPLER_VERSION_BUILD)

# module name resolution
MODULE_stappler_core := MODULE_STAPPLER_CORE

APR_LIB_PATH ?= /usr/local/lib/apache

MODULE_STAPPLER_APR_PRECOMPILED_HEADERS :=
MODULE_STAPPLER_APR_SRCS_DIRS :=
MODULE_STAPPLER_APR_SRCS_OBJS :=
MODULE_STAPPLER_APR_INCLUDES_DIRS :=
MODULE_STAPPLER_APR_INCLUDES_OBJS :=
MODULE_STAPPLER_APR_DEPENDS_ON :=
MODULE_STAPPLER_APR_LIBS := -L$(APR_LIB_PATH) -lapr-1

# module name resolution
MODULE_stappler_apr := MODULE_STAPPLER_APR
