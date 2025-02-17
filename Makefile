# Copyright (c) 2024 Stappler LLC <admin@stappler.dev>
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

STAPPLER_ROOT ?= ..

LOCAL_LIBRARY := libstappler-core

# force to rebuild if this makefile changed
LOCAL_MAKEFILE := $(lastword $(MAKEFILE_LIST))

LOCAL_OUTDIR := stappler-build

LOCAL_MODULES_PATHS = \
	$(STAPPLER_ROOT)/core/stappler-modules.mk

LOCAL_MODULES ?= \
	stappler_core \
	stappler_backtrace \
	stappler_bitmap \
	stappler_crypto \
	stappler_data \
	stappler_filesystem \
	stappler_db \
	stappler_font \
	stappler_geom \
	stappler_idn \
	stappler_network \
	stappler_search \
	stappler_sql \
	stappler_tess \
	stappler_threads \
	stappler_vg \
	stappler_wasm \
	stappler_zip

LOCAL_EXPORT_MODULES ?= \
	stappler_bitmap \
	stappler_core \
	stappler_crypto \
	stappler_data \
	stappler_db \
	stappler_font \
	stappler_geom \
	stappler_network \
	stappler_search \
	stappler_tess \
	stappler_vg \
	stappler_wasm \
	stappler_zip

include $(STAPPLER_ROOT)/build/make/shared.mk
