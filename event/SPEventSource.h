/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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

#ifndef CORE_EVENT_SPEVENTSOURCE_H_
#define CORE_EVENT_SPEVENTSOURCE_H_

#include "SPEvent.h"
#include "SPEventBufferChain.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class Queue;

class Source : public Ref {
public:
	static constexpr size_t DataSize = 64;

	virtual ~Source() = default;

	Source();

	virtual Status read(uint8_t *, size_t &);
	virtual Status read(BufferChain &);

	virtual Status write(const uint8_t *, size_t &);
	virtual Status write(BufferChain &);

	virtual void close();

	virtual bool isOpen() const;
	virtual bool isEndOfStream() const;

	virtual void bind(Queue *);

	virtual void setError(ErrorFlags);

	const uint8_t *getData() const { return _data; }
	uint8_t *getData() { return _data; }

protected:
	alignas(void *) uint8_t _data[DataSize];
	Queue *_owner;
	ErrorFlags _errorFlags = ErrorFlags::None;
};

/*class Timer : public Source {
public:
	virtual ~Timer();

};

class Socket : public Source {
public:
	virtual ~Socket();

};*/


}

#endif /* CORE_EVENT_SPEVENTSOURCE_H_ */
