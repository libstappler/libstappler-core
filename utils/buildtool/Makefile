# Copyright (c) 2025 Stappler LLC <admin@stappler.dev>
# Copyright (c) 2025 Stappler Team <admin@stappler.org>
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

LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

LOCAL_EXECUTABLE := stappler-build

LOCAL_MODULES_PATHS = \
	core/stappler-modules.mk

LOCAL_MODULES := \
	stappler_filesystem \
	stappler_data \
	stappler_makefile \
	stappler_network

LOCAL_SRCS_DIRS := src
LOCAL_INCLUDES_DIRS := src

LOCAL_MAIN := main.cpp

APPCONFIG_BUNDLE_NAME := org.stappler.stappler-build

STAPPLER_BUILD_ROOT ?= ../../../build/make

# Use build system from repo directly
include $(STAPPLER_BUILD_ROOT)/universal.mk
