#pragma once

#include <Arduino.h>

// Limit streamed JSON without buffering the whole response. The underlying
// stream supplies the timeout; this wrapper must not add another wait.
class LimitedStream : public Stream {
public:
    LimitedStream(Stream &source, size_t limit) : m_source(source), m_left(limit) { setTimeout(0); }
    int available() override { return m_left ? m_source.available() : 0; }
    int read() override {
        if (!m_left) {
            m_exceeded = true;
            return -1;
        }
        char c;
        if (m_source.readBytes(&c, 1) != 1) return -1;
        --m_left;
        return static_cast<uint8_t>(c);
    }
    int peek() override { return m_left ? m_source.peek() : -1; }
    size_t write(uint8_t) override { return 0; }
    bool exceeded() const { return m_exceeded; }

private:
    Stream &m_source;
    size_t m_left;
    bool m_exceeded = false;
};
