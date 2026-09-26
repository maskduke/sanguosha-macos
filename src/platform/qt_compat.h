#pragma once
#include <QtWidgets>
#include <QRegExp>
#include <QRandomGenerator>
#include <algorithm>
// Keep the legacy call boundary and 31-bit nonnegative range. The generator is
// per-thread, as the old Qt API was. Cross-version seed sequences are not promised.
inline QRandomGenerator &qsLegacyRandom() { thread_local QRandomGenerator rng(1); return rng; }
inline int qrand() { return int(qsLegacyRandom().generate() & 0x7fffffffu); }
inline void qsrand(uint seed) { qsLegacyRandom().seed(seed); }
template<class T> QSet<T> qToSet(const QList<T> &list) { return QSet<T>(list.cbegin(),list.cend()); }
template<class It> void qSort(It first, It last) { std::sort(first,last); }
template<class It,class Compare> void qSort(It first, It last, Compare compare) { std::sort(first,last,compare); }
template<class Container> void qSort(Container &c) { std::sort(c.begin(),c.end()); }
template<class It> void qStableSort(It first, It last) { std::stable_sort(first,last); }
template<class It,class Compare> void qStableSort(It first, It last, Compare compare) { std::stable_sort(first,last,compare); }
