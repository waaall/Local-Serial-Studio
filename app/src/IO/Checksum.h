

#pragma once

#include <QMap>
#include <QStringList>

namespace IO
{
using ChecksumFunc = std::function<QByteArray(const char *, int)>;

[[nodiscard]] const QStringList &availableChecksums();
[[nodiscard]] const QMap<QString, ChecksumFunc> &checksumFunctionMap();
[[nodiscard]] QByteArray checksum(const QString &name, const QByteArray &data);
} // namespace IO
