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

/* creates empty task with only complete function to be used as callback from other thread */
bool Task::init(const CompleteCallback &c, Ref *t, TaskGroup *g) {
	addRef(t);
	if (c) {
		_complete.push_back(c);
	}
	_group = g;
	return true;
}

bool Task::init(CompleteCallback &&c, Ref *t, TaskGroup *g) {
	addRef(t);
	if (c) {
		_complete.emplace_back(sp::move(c));
	}
	_group = g;
	return true;
}

/* creates regular async task without initialization phase */
bool Task::init(const ExecuteCallback &e, const CompleteCallback &c, Ref *t, TaskGroup *g) {
	addRef(t);
	if (e) {
		_execute.push_back(e);
	}
	if (c) {
		_complete.push_back(c);
	}
	_group = g;
	return true;
}

bool Task::init(ExecuteCallback &&e, CompleteCallback &&c, Ref *t, TaskGroup *g) {
	addRef(t);
	if (e) {
		_execute.emplace_back(sp::move(e));
	}
	if (c) {
		_complete.emplace_back(sp::move(c));
	}
	_group = g;
	return true;
}

/* creates regular async task with initialization phase */
bool Task::init(const PrepareCallback &p, const ExecuteCallback &e, const CompleteCallback &c, Ref *t, TaskGroup *g) {
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
	_group = g;
	return true;
}

bool Task::init(PrepareCallback &&p, ExecuteCallback &&e, CompleteCallback &&c, Ref *t, TaskGroup *g) {
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
	_group = g;
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

void Task::run() const {
	if (_state == TaskState::Initial) {
		prepare();
	}
	if (_state == TaskState::Prepared) {
		execute();
	}
	handleCompleted();
}

bool Task::prepare() const {
	if (_state == TaskState::Initial) {
		if (!_prepare.empty()) {
			for (auto i : _prepare) {
				if (i && !i(*this)) {
					_state = TaskState::ExecutedFailed;
					return false;
				}
			}
		}
		if (_group) {
			_group->handleAdded(this);
		}
		_state = TaskState::Prepared;
		return true;
	} else {
		log::warn("thread::Task", "Task::prepare was called on the task, that was already prepared");
	}
	return false;
}

/** called on worker thread */
bool Task::execute() const {
	if (_state == TaskState::Prepared) {
		if (!_execute.empty()) {
			for (auto i : _execute) {
				if (i && !i(*this)) {
					_state = TaskState::ExecutedFailed;
					return false;
				}
			}
		}
		_state = TaskState::ExecutedSuccessful;
		return true;
	} else {
		log::warn("thread::Task", "Task::execute was called on the task, that is not in TaskState::Prepared");
	}
	return false;
}

/** called on dispatcher thread when request is completed */
void Task::handleCompleted() const {
	switch (_state) {
	case TaskState::ExecutedSuccessful:
	case TaskState::ExecutedFailed:
		if (!_complete.empty()) {
			for (auto i : _complete) {
				i(*this, isSuccessful());
			}
		}
		if (_group) {
			_group->handlePerformed(this);
		}
		break;
	default:
		log::warn("thread::Task", "Task::handleComplete was called on the task, that is not in TaskState::ExecutedSuccessful or TaskState::ExecutedFailed");
		break;
	}
}

void Task::cancel() const {
	if (_state == TaskState::Prepared) {
		_state = TaskState::ExecutedFailed;
		handleCompleted();
	} else {
		log::warn("thread::Task", "Task::cancel was called on the task, that is not in TaskState::Prepared");
	}
}

}
