#include "line_framer.h"
namespace qs {
QList<QByteArray> LineFramer::feed(const QByteArray &bytes) {
    QList<QByteArray> frames;
    if (error_ != Error::None) return frames;
    qsizetype cursor = 0;
    while (cursor < bytes.size()) {
        const qsizetype lf = bytes.indexOf('\n', cursor);
        const qsizetype end = lf < 0 ? bytes.size() : lf;
        const qsizetype count = end - cursor;
        // Check before allocation, including when LF never arrives.
        if (count > MaxPayload - pending_.size()) {
            pending_.clear();
            error_ = Error::TooLarge;
            break;
        }
        pending_.append(bytes.constData() + cursor, count);
        if (lf < 0) break;
        frames.append(pending_);
        pending_.clear();
        cursor = lf + 1;
    }
    return frames;
}
bool LineFramer::finish() {
    if (error_ != Error::None) return false;
    if (!pending_.isEmpty()) {
        pending_.clear();
        error_ = Error::Truncated;
        return false;
    }
    return true;
}
void LineFramer::reset() { pending_.clear(); error_ = Error::None; }
}
