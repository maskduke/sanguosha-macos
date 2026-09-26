#pragma once
#include <QByteArray>
#include <QList>

namespace qs {
// Payload excludes LF. Bytes (including CR) are preserved, never decoded here.
class LineFramer {
public:
    static constexpr qsizetype MaxPayload = 1024 * 1024;
    enum class Error { None, TooLarge, Truncated };
    QList<QByteArray> feed(const QByteArray &bytes);
    bool finish();
    void reset();
    Error error() const { return error_; }
    qsizetype bufferedBytes() const { return pending_.size(); }
private:
    QByteArray pending_;
    Error error_ = Error::None;
};
}
