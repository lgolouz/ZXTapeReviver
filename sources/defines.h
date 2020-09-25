#ifndef DEFINES_H
#define DEFINES_H

#include <QVector>

using QWavVectorType = float;
using QWavVector = QVector<QWavVectorType>;

template<typename T>
inline bool lessThanZero(T t) {
    return t < 0;
}

#endif // DEFINES_H
