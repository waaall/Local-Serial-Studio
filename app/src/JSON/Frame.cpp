

#include "JSON/Frame.h"
#include "SerialStudio.h"

void JSON::finalize_frame(JSON::Frame &frame)
{
  frame.containsCommercialFeatures = SerialStudio::commercialCfg(frame.groups);

  int id = 1;
  for (auto &group : frame.groups)
  {
    for (auto &dataset : group.datasets)
      dataset.uniqueId = id++;
  }
}

void JSON::read_io_settings(QByteArray &frameStart, QByteArray &frameEnd,
                            QString &checksum, const QJsonObject &obj)
{
  // Obtain frame delimiters
  auto fEndStr = ss_jsr(obj, "frameEnd", "").toString();
  auto fStartStr = ss_jsr(obj, "frameStart", "").toString();
  auto isHex = ss_jsr(obj, "hexadecimalDelimiters", false).toBool();

  // Read checksum method
  checksum = ss_jsr(obj, "checksum", "").toString();

  // Convert hex + escape strings (e.g. "0A 0D", or "\\n0D") to raw bytes
  if (isHex)
  {
    QString resolvedEnd = SerialStudio::resolveEscapeSequences(fEndStr);
    QString resolvedStart = SerialStudio::resolveEscapeSequences(fStartStr);
    frameStart = QByteArray::fromHex(resolvedEnd.remove(' ').toUtf8());
    frameEnd = QByteArray::fromHex(resolvedStart.remove(' ').toUtf8());
  }

  // Resolve escape sequences (e.g. "\\n") and encode to UTF-8 bytes
  else
  {
    frameEnd = SerialStudio::resolveEscapeSequences(fEndStr).toUtf8();
    frameStart = SerialStudio::resolveEscapeSequences(fStartStr).toUtf8();
  }
}

QByteArray JSON::get_tx_bytes(const Action &action)
{
  QByteArray b;
  if (action.binaryData)
    b = SerialStudio::hexToBytes(action.txData);
  else
    b = SerialStudio::resolveEscapeSequences(action.txData).toUtf8();

  if (!action.eolSequence.isEmpty())
    b.append(SerialStudio::resolveEscapeSequences(action.eolSequence).toUtf8());

  return b;
}
