/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

package org.stappler.core;

import android.content.res.Configuration;
import android.os.Build;

public class Application extends android.app.Application {
	public static Application s_application;

	private long _native = 0;

	@Override
	public void onCreate() {
		s_application = this;
		super.onCreate();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		handleConfigurationChanged(_native, newConfig);
	}

	@Override
	public void onLowMemory() {
		super.onLowMemory();
		handleLowMemory(_native);
	}

	@Override
	public void onTerminate() {
		super.onTerminate();
		_native = 0;
		s_application = null;
	}

	protected void setNative(long n) {
		_native = n;
	}

	protected boolean isEmulator() {
		return Build.PRODUCT.equals("sdk") || Build.PRODUCT.contains("_sdk") || Build.PRODUCT.contains("sdk_");
	}

	protected native void handleConfigurationChanged(long n, Configuration newConfig);
	protected native void handleLowMemory(long n);
}
