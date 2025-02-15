/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "SPThreadTask.h"

namespace STAPPLER_VERSIONIZED stappler::thread {

Task::Task() { }
Task::~Task() { }

/* creates empty task with only complete function to be used as callback from other thread */
bool Task::init(const CompleteCallback &c, Ref *t) {
	addRef(t);
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

bool Task::init(CompleteCallback &&c, Ref *t) {
	addRef(t);
	if (c) {
		_complete.emplace_back(sp::move(c));
	}
	return true;
}

/* creates regular async task without initialization phase */
bool Task::init(const ExecuteCallback &e, const CompleteCallback &c, Ref *t) {
	addRef(t);
	if (e) {
		_execute.push_back(e);
	}
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

bool Task::init(ExecuteCallback &&e, CompleteCallback &&c, Ref *t) {
	addRef(t);
	if (e) {
		_execute.emplace_back(sp::move(e));
	}
	if (c) {
		_complete.emplace_back(sp::move(c));
	}
	return true;
}

/* creates regular async task with initialization phase */
bool Task::init(const PrepareCallback &p, const ExecuteCallback &e, const CompleteCallback &c, Ref *t) {
	addRef(t);
	if (p) {
		_prepare.push_back(p);
	}
	if (e) {
		_execute.push_back(e);
	}
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

bool Task::init(PrepareCallback &&p, ExecuteCallback &&e, CompleteCallback &&c, Ref *t) {
	addRef(t);
	if (p) {
		_prepare.emplace_back(sp::move(p));
	}
	if (e) {
		_execute.emplace_back(sp::move(e));
	}
	if (c) {
		_complete.emplace_back(sp::move(c));
	}
	return true;
}

/* adds one more function to be executed before task is added to queue, functions executed as FIFO */
void Task::addPrepareCallback(const PrepareCallback &cb) {
	if (cb) {
		_prepare.push_back(cb);
	}
}

void Task::addPrepareCallback(PrepareCallback &&cb) {
	if (cb) {
		_prepare.emplace_back(sp::move(cb));
	}
}

/* adds one more function to be executed in other thread, functions executed as FIFO */
void Task::addExecuteCallback(const ExecuteCallback &cb) {
	if (cb) {
		_execute.push_back(cb);
	}
}

void Task::addExecuteCallback(ExecuteCallback &&cb) {
	if (cb) {
		_execute.emplace_back(sp::move(cb));
	}
}

/* adds one more function to be executed when task is performed, functions executed as FIFO */
void Task::addCompleteCallback(const CompleteCallback &cb) {
	if (cb) {
		_complete.push_back(cb);
	}
}

void Task::addCompleteCallback(CompleteCallback &&cb) {
	if (cb) {
		_complete.emplace_back(sp::move(cb));
	}
}

bool Task::prepare() const {
	if (!_prepare.empty()) {
		for (auto i : _prepare) {
			if (i && !i(*this)) {
				return false;
			}
		}
	}
	return true;
}

/** called on worker thread */
bool Task::execute() {
	if (!_execute.empty()) {
		for (auto i : _execute) {
			if (i && !i(*this)) {
				return false;
			}
		}
	}
	return true;
}

/** called on UI thread when request is completed */
void Task::onComplete() {
	if (!_complete.empty()) {
		for (auto i : _complete) {
			i(*this, isSuccessful());
		}
	}
}

}
