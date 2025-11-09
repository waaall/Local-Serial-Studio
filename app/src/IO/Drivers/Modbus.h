/*
 * Serial Studio
 * https://serial-studio.com/
 *
 * Copyright (C) 2020–2025 Alex Spataru
 *
 * This file is dual-licensed:
 *
 * - Under the GNU GPLv3 (or later) for builds that exclude Pro modules.
 * - Under the Serial Studio Commercial License for builds that include
 *   any Pro functionality.
 *
 * You must comply with the terms of one of these licenses, depending
 * on your use case.
 *
 * For GPL terms, see <https://www.gnu.org/licenses/gpl-3.0.html>
 * For commercial terms, see LICENSE_COMMERCIAL.md in the project root.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-SerialStudio-Commercial
 */

#pragma once

#include <QTimer>
#include <QObject>
#include <QString>
#include <QSettings>
#include <QByteArray>
#include <QVector>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QModbusClient>
#include <QModbusRtuSerialClient>
#include <QModbusTcpClient>
#include <QModbusReply>
#include <QModbusDataUnit>

#include "IO/HAL_Driver.h"

namespace IO
{
namespace Drivers
{
/**
 * @brief Modbus驱动类
 *
 * 用于通过Modbus RTU（串口）或Modbus TCP协议与设备通信
 * 支持定时轮询寄存器数据并转换为Serial Studio可用的数据流
 */
class Modbus : public HAL_Driver
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(quint8 modbusMode
             READ modbusMode
             WRITE setModbusMode
             NOTIFY modbusModeChanged)
  Q_PROPERTY(quint8 slaveAddress
             READ slaveAddress
             WRITE setSlaveAddress
             NOTIFY slaveAddressChanged)
  Q_PROPERTY(quint8 functionCode
             READ functionCode
             WRITE setFunctionCode
             NOTIFY functionCodeChanged)
  Q_PROPERTY(quint16 startAddress
             READ startAddress
             WRITE setStartAddress
             NOTIFY startAddressChanged)
  Q_PROPERTY(quint16 registerCount
             READ registerCount
             WRITE setRegisterCount
             NOTIFY registerCountChanged)
  Q_PROPERTY(quint16 pollInterval
             READ pollInterval
             WRITE setPollInterval
             NOTIFY pollIntervalChanged)
  Q_PROPERTY(QString tcpHost
             READ tcpHost
             WRITE setTcpHost
             NOTIFY tcpHostChanged)
  Q_PROPERTY(quint16 tcpPort
             READ tcpPort
             WRITE setTcpPort
             NOTIFY tcpPortChanged)
  Q_PROPERTY(quint8 serialPortIndex
             READ serialPortIndex
             WRITE setSerialPortIndex
             NOTIFY serialPortIndexChanged)
  Q_PROPERTY(qint32 baudRate
             READ baudRate
             WRITE setBaudRate
             NOTIFY baudRateChanged)
  Q_PROPERTY(quint8 parityIndex
             READ parityIndex
             WRITE setParity
             NOTIFY parityChanged)
  Q_PROPERTY(QStringList modeList
             READ modeList
             NOTIFY languageChanged)
  Q_PROPERTY(QStringList functionCodeList
             READ functionCodeList
             NOTIFY languageChanged)
  Q_PROPERTY(QStringList serialPortList
             READ serialPortList
             NOTIFY availablePortsChanged)
  Q_PROPERTY(QStringList parityList
             READ parityList
             NOTIFY languageChanged)
  Q_PROPERTY(QStringList baudRateList
             READ baudRateList
             CONSTANT)
  // clang-format on

signals:
  void languageChanged();
  void modbusModeChanged();
  void slaveAddressChanged();
  void functionCodeChanged();
  void startAddressChanged();
  void registerCountChanged();
  void pollIntervalChanged();
  void tcpHostChanged();
  void tcpPortChanged();
  void serialPortIndexChanged();
  void baudRateChanged();
  void parityChanged();
  void availablePortsChanged();
  void connectionError(const QString &error);

private:
  explicit Modbus();
  Modbus(Modbus &&) = delete;
  Modbus(const Modbus &) = delete;
  Modbus &operator=(Modbus &&) = delete;
  Modbus &operator=(const Modbus &) = delete;

  ~Modbus();

public:
  static Modbus &instance();

  // HAL_Driver接口实现
  void close() override;
  [[nodiscard]] bool isOpen() const override;
  [[nodiscard]] bool isReadable() const override;
  [[nodiscard]] bool isWritable() const override;
  [[nodiscard]] bool configurationOk() const override;
  [[nodiscard]] quint64 write(const QByteArray &data) override;
  [[nodiscard]] bool open(const QIODevice::OpenMode mode) override;

  // Modbus模式枚举
  enum ModbusMode
  {
    RTU = 0, // Modbus RTU (串口)
    TCP = 1  // Modbus TCP (网络)
  };
  Q_ENUM(ModbusMode)

  // 属性访问器
  [[nodiscard]] quint8 modbusMode() const;
  [[nodiscard]] quint8 slaveAddress() const;
  [[nodiscard]] quint8 functionCode() const;
  [[nodiscard]] quint16 startAddress() const;
  [[nodiscard]] quint16 registerCount() const;
  [[nodiscard]] quint16 pollInterval() const;
  [[nodiscard]] QString tcpHost() const;
  [[nodiscard]] quint16 tcpPort() const;
  [[nodiscard]] quint8 serialPortIndex() const;
  [[nodiscard]] qint32 baudRate() const;
  [[nodiscard]] quint8 parityIndex() const;

  // 列表访问器
  [[nodiscard]] QStringList modeList() const;
  [[nodiscard]] QStringList functionCodeList() const;
  [[nodiscard]] QStringList serialPortList() const;
  [[nodiscard]] QStringList parityList() const;
  [[nodiscard]] QStringList baudRateList() const;

public slots:
  void setupExternalConnections();
  void setModbusMode(const quint8 mode);
  void setSlaveAddress(const quint8 address);
  void setFunctionCode(const quint8 code);
  void setStartAddress(const quint16 address);
  void setRegisterCount(const quint16 count);
  void setPollInterval(const quint16 interval);
  void setTcpHost(const QString &host);
  void setTcpPort(const quint16 port);
  void setSerialPortIndex(const quint8 index);
  void setBaudRate(const qint32 rate);
  void setParity(const quint8 parityIndex);

private slots:
  void onPollTimer();
  void onReadReady();
  void onStateChanged(QModbusDevice::State state);
  void onErrorOccurred(QModbusDevice::Error error);
  void refreshSerialDevices();
  void processReply(QModbusReply *reply);

private:
  QVector<QSerialPortInfo> validPorts() const;
  void startPolling();
  void stopPolling();
  QByteArray formatModbusData(const QModbusDataUnit &data);

private:
  // Modbus客户端
  QModbusClient *m_modbusClient;

  // 配置参数
  quint8 m_modbusMode;        // RTU或TCP模式
  quint8 m_slaveAddress;      // 从站地址 (1-247)
  quint8 m_functionCode;      // 功能码 (1=读线圈, 3=读保持寄存器等)
  quint16 m_startAddress;     // 起始寄存器地址
  quint16 m_registerCount;    // 寄存器数量
  quint16 m_pollInterval;     // 轮询间隔(ms)

  // TCP配置
  QString m_tcpHost;
  quint16 m_tcpPort;

  // RTU串口配置
  quint8 m_serialPortIndex;
  qint32 m_baudRate;
  QSerialPort::Parity m_parity;
  quint8 m_parityIndex;

  // 定时器
  QTimer *m_pollTimer;

  // 串口列表
  QStringList m_deviceNames;
  QStringList m_deviceLocations;

  // 设置
  QSettings m_settings;
};
} // namespace Drivers
} // namespace IO
