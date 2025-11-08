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

#include "IO/Drivers/Modbus.h"
#include "Misc/Translator.h"
#include "Misc/TimerEvents.h"
#include "Misc/Utilities.h"

#include <QMessageBox>

//------------------------------------------------------------------------------
// 构造函数、析构函数和单例访问
//------------------------------------------------------------------------------

/**
 * 构造函数 - 初始化Modbus驱动
 */
IO::Drivers::Modbus::Modbus()
  : m_modbusClient(nullptr)
  , m_modbusMode(ModbusMode::RTU)
  , m_slaveAddress(1)
  , m_functionCode(3) // 默认使用功能码3 (读保持寄存器)
  , m_startAddress(0)
  , m_registerCount(10)
  , m_pollInterval(1000) // 默认1秒轮询一次
  , m_tcpHost("127.0.0.1")
  , m_tcpPort(502)
  , m_serialPortIndex(0)
  , m_baudRate(9600)
  , m_parity(QSerialPort::NoParity)
  , m_parityIndex(0)
{
  // 创建轮询定时器
  m_pollTimer = new QTimer(this);
  connect(m_pollTimer, &QTimer::timeout, this, &Modbus::onPollTimer);

  // 从设置中恢复配置
  m_modbusMode = m_settings.value("Modbus_Mode", ModbusMode::RTU).toUInt();
  m_slaveAddress = m_settings.value("Modbus_SlaveAddr", 1).toUInt();
  m_functionCode = m_settings.value("Modbus_FuncCode", 3).toUInt();
  m_startAddress = m_settings.value("Modbus_StartAddr", 0).toUInt();
  m_registerCount = m_settings.value("Modbus_RegCount", 10).toUInt();
  m_pollInterval = m_settings.value("Modbus_PollInterval", 1000).toUInt();
  m_tcpHost = m_settings.value("Modbus_TcpHost", "127.0.0.1").toString();
  m_tcpPort = m_settings.value("Modbus_TcpPort", 502).toUInt();
  m_baudRate = m_settings.value("Modbus_BaudRate", 9600).toInt();
  m_parityIndex = m_settings.value("Modbus_Parity", 0).toUInt();

  // 设置奇偶校验
  setParity(m_parityIndex);
}

/**
 * 析构函数 - 清理资源
 */
IO::Drivers::Modbus::~Modbus()
{
  close();

  if (m_pollTimer)
  {
    m_pollTimer->stop();
    m_pollTimer->deleteLater();
  }
}

/**
 * 返回单例实例
 */
IO::Drivers::Modbus &IO::Drivers::Modbus::instance()
{
  static Modbus singleton;
  return singleton;
}

//------------------------------------------------------------------------------
// HAL_Driver接口实现
//------------------------------------------------------------------------------

/**
 * 关闭Modbus连接
 */
void IO::Drivers::Modbus::close()
{
  // 停止轮询
  stopPolling();

  // 断开信号连接
  if (m_modbusClient)
  {
    disconnect(m_modbusClient, &QModbusClient::stateChanged, this,
               &Modbus::onStateChanged);
    disconnect(m_modbusClient, &QModbusClient::errorOccurred, this,
               &Modbus::onErrorOccurred);

    // 断开并删除客户端
    if (m_modbusClient->state() == QModbusDevice::ConnectedState)
      m_modbusClient->disconnectDevice();

    m_modbusClient->deleteLater();
    m_modbusClient = nullptr;
  }

  Q_EMIT configurationChanged();
}

/**
 * 检查Modbus连接是否打开
 */
bool IO::Drivers::Modbus::isOpen() const
{
  if (m_modbusClient)
    return m_modbusClient->state() == QModbusDevice::ConnectedState;

  return false;
}

/**
 * 检查是否可读
 */
bool IO::Drivers::Modbus::isReadable() const
{
  return isOpen();
}

/**
 * 检查是否可写
 */
bool IO::Drivers::Modbus::isWritable() const
{
  return isOpen();
}

/**
 * 检查配置是否完整有效
 */
bool IO::Drivers::Modbus::configurationOk() const
{
  // RTU模式需要串口索引大于0
  if (m_modbusMode == ModbusMode::RTU)
    return m_serialPortIndex > 0;

  // TCP模式需要有效的主机地址
  return !m_tcpHost.isEmpty();
}

/**
 * 写入数据到Modbus设备
 *
 * @note Modbus驱动主要用于读取，写入功能可根据需要扩展
 * @param data 要写入的数据
 * @return 写入的字节数
 */
quint64 IO::Drivers::Modbus::write(const QByteArray &data)
{
  Q_UNUSED(data);

  // TODO: 实现Modbus写入功能（功能码5, 6, 15, 16等）
  // 这里可以解析data并写入到Modbus寄存器
  qWarning() << "Modbus write not implemented yet";

  return 0;
}

/**
 * 打开Modbus连接
 *
 * @param mode 打开模式（读/写）
 * @return 成功返回true
 */
bool IO::Drivers::Modbus::open(const QIODevice::OpenMode mode)
{
  Q_UNUSED(mode);

  // 先关闭现有连接
  close();

  // 根据模式创建客户端
  if (m_modbusMode == ModbusMode::RTU)
  {
    // RTU模式 - 使用串口
    auto ports = serialPortList();
    if (m_serialPortIndex >= 1 && m_serialPortIndex < ports.count())
    {
      auto rtuClient = new QModbusRtuSerialClient(this);
      m_modbusClient = rtuClient;

      // 获取串口信息
      auto validPortList = validPorts();
      auto portInfo = validPortList.at(m_serialPortIndex - 1);

      // 配置串口参数
      rtuClient->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                        portInfo.portName());
      rtuClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                        m_baudRate);
      rtuClient->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                        static_cast<int>(m_parity));
      rtuClient->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                        QSerialPort::Data8);
      rtuClient->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                        QSerialPort::OneStop);

      // 设置超时
      rtuClient->setTimeout(1000);
      rtuClient->setNumberOfRetries(3);
    }
    else
    {
      Misc::Utilities::showMessageBox(tr("Modbus RTU配置错误"),
                                      tr("请选择有效的串口"),
                                      QMessageBox::Warning);
      return false;
    }
  }
  else if (m_modbusMode == ModbusMode::TCP)
  {
    // TCP模式 - 使用网络
    auto tcpClient = new QModbusTcpClient(this);
    m_modbusClient = tcpClient;

    // 配置TCP参数
    tcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                      m_tcpHost);
    tcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter,
                                      m_tcpPort);

    // 设置超时
    tcpClient->setTimeout(3000);
    tcpClient->setNumberOfRetries(3);
  }

  if (!m_modbusClient)
    return false;

  // 连接信号
  connect(m_modbusClient, &QModbusClient::stateChanged, this,
          &Modbus::onStateChanged);
  connect(m_modbusClient, &QModbusClient::errorOccurred, this,
          &Modbus::onErrorOccurred);

  // 尝试连接
  if (!m_modbusClient->connectDevice())
  {
    QString error = m_modbusClient->errorString();
    Misc::Utilities::showMessageBox(tr("Modbus连接失败"), error,
                                    QMessageBox::Critical);
    close();
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// 属性访问器
//------------------------------------------------------------------------------

quint8 IO::Drivers::Modbus::modbusMode() const
{
  return m_modbusMode;
}

quint8 IO::Drivers::Modbus::slaveAddress() const
{
  return m_slaveAddress;
}

quint8 IO::Drivers::Modbus::functionCode() const
{
  return m_functionCode;
}

quint16 IO::Drivers::Modbus::startAddress() const
{
  return m_startAddress;
}

quint16 IO::Drivers::Modbus::registerCount() const
{
  return m_registerCount;
}

quint16 IO::Drivers::Modbus::pollInterval() const
{
  return m_pollInterval;
}

QString IO::Drivers::Modbus::tcpHost() const
{
  return m_tcpHost;
}

quint16 IO::Drivers::Modbus::tcpPort() const
{
  return m_tcpPort;
}

quint8 IO::Drivers::Modbus::serialPortIndex() const
{
  return m_serialPortIndex;
}

qint32 IO::Drivers::Modbus::baudRate() const
{
  return m_baudRate;
}

quint8 IO::Drivers::Modbus::parityIndex() const
{
  return m_parityIndex;
}

//------------------------------------------------------------------------------
// 列表访问器
//------------------------------------------------------------------------------

/**
 * 返回Modbus模式列表
 */
QStringList IO::Drivers::Modbus::modeList() const
{
  QStringList list;
  list.append(tr("Modbus RTU (串口)"));
  list.append(tr("Modbus TCP (网络)"));
  return list;
}

/**
 * 返回功能码列表
 */
QStringList IO::Drivers::Modbus::functionCodeList() const
{
  QStringList list;
  list.append(tr("01 - 读线圈"));
  list.append(tr("02 - 读离散输入"));
  list.append(tr("03 - 读保持寄存器"));
  list.append(tr("04 - 读输入寄存器"));
  return list;
}

/**
 * 返回串口列表
 */
QStringList IO::Drivers::Modbus::serialPortList() const
{
  if (m_deviceNames.count() > 0)
    return m_deviceNames;

  return QStringList{tr("选择串口")};
}

/**
 * 返回奇偶校验列表
 */
QStringList IO::Drivers::Modbus::parityList() const
{
  QStringList list;
  list.append(tr("无"));
  list.append(tr("偶"));
  list.append(tr("奇"));
  return list;
}

/**
 * 返回波特率列表
 */
QStringList IO::Drivers::Modbus::baudRateList() const
{
  return QStringList{"1200",   "2400",   "4800",   "9600",
                     "19200",  "38400",  "57600",  "115200",
                     "230400", "460800", "921600"};
}

//------------------------------------------------------------------------------
// 属性设置器
//------------------------------------------------------------------------------

/**
 * 设置外部连接
 */
void IO::Drivers::Modbus::setupExternalConnections()
{
  // 每秒刷新串口列表
  connect(&Misc::TimerEvents::instance(), &Misc::TimerEvents::timeout1Hz, this,
          &Modbus::refreshSerialDevices);

  // 语言变化时更新列表
  connect(&Misc::Translator::instance(), &Misc::Translator::languageChanged,
          this, &Modbus::languageChanged);
}

/**
 * 设置Modbus模式
 */
void IO::Drivers::Modbus::setModbusMode(const quint8 mode)
{
  if (m_modbusMode != mode)
  {
    m_modbusMode = mode;
    m_settings.setValue("Modbus_Mode", mode);
    Q_EMIT modbusModeChanged();
    Q_EMIT configurationChanged();
  }
}

/**
 * 设置从站地址
 */
void IO::Drivers::Modbus::setSlaveAddress(const quint8 address)
{
  if (m_slaveAddress != address && address >= 1 && address <= 247)
  {
    m_slaveAddress = address;
    m_settings.setValue("Modbus_SlaveAddr", address);
    Q_EMIT slaveAddressChanged();
  }
}

/**
 * 设置功能码
 */
void IO::Drivers::Modbus::setFunctionCode(const quint8 code)
{
  if (m_functionCode != code)
  {
    m_functionCode = code + 1; // 列表索引转换为实际功能码
    m_settings.setValue("Modbus_FuncCode", m_functionCode);
    Q_EMIT functionCodeChanged();
  }
}

/**
 * 设置起始地址
 */
void IO::Drivers::Modbus::setStartAddress(const quint16 address)
{
  if (m_startAddress != address)
  {
    m_startAddress = address;
    m_settings.setValue("Modbus_StartAddr", address);
    Q_EMIT startAddressChanged();
  }
}

/**
 * 设置寄存器数量
 */
void IO::Drivers::Modbus::setRegisterCount(const quint16 count)
{
  if (m_registerCount != count && count > 0 && count <= 125)
  {
    m_registerCount = count;
    m_settings.setValue("Modbus_RegCount", count);
    Q_EMIT registerCountChanged();
  }
}

/**
 * 设置轮询间隔
 */
void IO::Drivers::Modbus::setPollInterval(const quint16 interval)
{
  if (m_pollInterval != interval && interval >= 100)
  {
    m_pollInterval = interval;
    m_settings.setValue("Modbus_PollInterval", interval);
    Q_EMIT pollIntervalChanged();

    // 如果正在轮询，重新启动定时器
    if (m_pollTimer->isActive())
    {
      m_pollTimer->stop();
      m_pollTimer->start(m_pollInterval);
    }
  }
}

/**
 * 设置TCP主机地址
 */
void IO::Drivers::Modbus::setTcpHost(const QString &host)
{
  if (m_tcpHost != host)
  {
    m_tcpHost = host;
    m_settings.setValue("Modbus_TcpHost", host);
    Q_EMIT tcpHostChanged();
    Q_EMIT configurationChanged();
  }
}

/**
 * 设置TCP端口
 */
void IO::Drivers::Modbus::setTcpPort(const quint16 port)
{
  if (m_tcpPort != port)
  {
    m_tcpPort = port;
    m_settings.setValue("Modbus_TcpPort", port);
    Q_EMIT tcpPortChanged();
  }
}

/**
 * 设置串口索引
 */
void IO::Drivers::Modbus::setSerialPortIndex(const quint8 index)
{
  auto ports = serialPortList();
  if (index < ports.count())
  {
    m_serialPortIndex = index;
    Q_EMIT serialPortIndexChanged();
    Q_EMIT configurationChanged();
  }
}

/**
 * 设置波特率
 */
void IO::Drivers::Modbus::setBaudRate(const qint32 rate)
{
  if (m_baudRate != rate && rate > 0)
  {
    m_baudRate = rate;
    m_settings.setValue("Modbus_BaudRate", rate);
    Q_EMIT baudRateChanged();
  }
}

/**
 * 设置奇偶校验
 */
void IO::Drivers::Modbus::setParity(const quint8 parityIndex)
{
  if (parityIndex < parityList().count())
  {
    m_parityIndex = parityIndex;
    m_settings.setValue("Modbus_Parity", parityIndex);

    // 转换为QSerialPort枚举
    switch (parityIndex)
    {
      case 0:
        m_parity = QSerialPort::NoParity;
        break;
      case 1:
        m_parity = QSerialPort::EvenParity;
        break;
      case 2:
        m_parity = QSerialPort::OddParity;
        break;
    }

    Q_EMIT parityChanged();
  }
}

//------------------------------------------------------------------------------
// 私有槽函数
//------------------------------------------------------------------------------

/**
 * 轮询定时器触发 - 读取Modbus寄存器
 */
void IO::Drivers::Modbus::onPollTimer()
{
  if (!isOpen() || !m_modbusClient)
    return;

  // 根据功能码确定寄存器类型
  QModbusDataUnit::RegisterType registerType;
  switch (m_functionCode)
  {
    case 1:
      registerType = QModbusDataUnit::Coils;
      break;
    case 2:
      registerType = QModbusDataUnit::DiscreteInputs;
      break;
    case 3:
      registerType = QModbusDataUnit::HoldingRegisters;
      break;
    case 4:
      registerType = QModbusDataUnit::InputRegisters;
      break;
    default:
      registerType = QModbusDataUnit::HoldingRegisters;
  }

  // 创建读取请求
  QModbusDataUnit readUnit(registerType, m_startAddress, m_registerCount);

  // 发送读取请求
  auto *reply = m_modbusClient->sendReadRequest(readUnit, m_slaveAddress);
  if (reply)
  {
    if (!reply->isFinished())
    {
      // 请求完成时处理数据
      connect(reply, &QModbusReply::finished, this, &Modbus::onReadReady);
    }
    else
    {
      // 立即处理（同步响应）
      delete reply;
    }
  }
  else
  {
    qWarning() << "Modbus read request failed:"
               << m_modbusClient->errorString();
  }
}

/**
 * Modbus读取完成
 */
void IO::Drivers::Modbus::onReadReady()
{
  auto reply = qobject_cast<QModbusReply *>(sender());
  if (!reply)
    return;

  if (reply->error() == QModbusDevice::NoError)
  {
    const QModbusDataUnit unit = reply->result();

    // 将Modbus数据格式化为字节流
    QByteArray data = formatModbusData(unit);

    // 发射数据接收信号
    Q_EMIT dataReceived(data);
  }
  else
  {
    qWarning() << "Modbus read error:" << reply->errorString();
  }

  reply->deleteLater();
}

/**
 * Modbus状态变化
 */
void IO::Drivers::Modbus::onStateChanged(QModbusDevice::State state)
{
  if (state == QModbusDevice::ConnectedState)
  {
    // 连接成功，开始轮询
    startPolling();
    Q_EMIT configurationChanged();
  }
  else if (state == QModbusDevice::UnconnectedState)
  {
    // 断开连接，停止轮询
    stopPolling();
    Q_EMIT configurationChanged();
  }
}

/**
 * Modbus错误处理
 */
void IO::Drivers::Modbus::onErrorOccurred(QModbusDevice::Error error)
{
  if (error != QModbusDevice::NoError && m_modbusClient)
  {
    QString errorMsg = m_modbusClient->errorString();
    qWarning() << "Modbus error:" << errorMsg;
    Q_EMIT connectionError(errorMsg);
  }
}

/**
 * 刷新串口设备列表
 */
void IO::Drivers::Modbus::refreshSerialDevices()
{
  // 创建设备列表
  QStringList names;
  QStringList locations;
  names.append(tr("选择串口"));
  locations.append("/dev/null");

  // 获取有效串口
  auto portList = validPorts();
  for (const auto &info : portList)
  {
    if (!info.isNull())
    {
#ifdef Q_OS_WIN
      names.append(info.portName() + "  " + info.description());
#else
      names.append(info.portName());
#endif
      locations.append(info.systemLocation());
    }
  }

  // 仅在列表变化时更新
  if (m_deviceNames != names)
  {
    m_deviceNames = names;
    m_deviceLocations = locations;
    Q_EMIT availablePortsChanged();
  }
}

//------------------------------------------------------------------------------
// 私有辅助函数
//------------------------------------------------------------------------------

/**
 * 获取有效串口列表
 */
QVector<QSerialPortInfo> IO::Drivers::Modbus::validPorts() const
{
  QVector<QSerialPortInfo> ports;

  for (const auto &info : QSerialPortInfo::availablePorts())
  {
    if (!info.isNull())
    {
#ifdef Q_OS_MACOS
      // macOS上过滤掉tty设备，只保留cu设备
      if (info.portName().toLower().startsWith("tty."))
        continue;
#endif
      ports.append(info);
    }
  }

  return ports;
}

/**
 * 开始轮询
 */
void IO::Drivers::Modbus::startPolling()
{
  if (!m_pollTimer->isActive())
  {
    m_pollTimer->start(m_pollInterval);
    qDebug() << "Modbus polling started, interval:" << m_pollInterval << "ms";
  }
}

/**
 * 停止轮询
 */
void IO::Drivers::Modbus::stopPolling()
{
  if (m_pollTimer->isActive())
  {
    m_pollTimer->stop();
    qDebug() << "Modbus polling stopped";
  }
}

/**
 * 格式化Modbus数据为字节流
 *
 * 将Modbus寄存器数据转换为CSV格式，便于Serial Studio处理
 *
 * @param data Modbus数据单元
 * @return 格式化后的字节流
 */
QByteArray IO::Drivers::Modbus::formatModbusData(const QModbusDataUnit &data)
{
  QByteArray result;

  // 根据寄存器类型格式化数据
  if (data.registerType() == QModbusDataUnit::Coils
      || data.registerType() == QModbusDataUnit::DiscreteInputs)
  {
    // 线圈/离散输入 - 布尔值
    QStringList values;
    for (int i = 0; i < data.valueCount(); ++i)
      values.append(QString::number(data.value(i)));

    result = values.join(",").toUtf8();
  }
  else
  {
    // 保持寄存器/输入寄存器 - 16位整数
    QStringList values;
    for (int i = 0; i < data.valueCount(); ++i)
      values.append(QString::number(data.value(i)));

    result = values.join(",").toUtf8();
  }

  // 添加换行符
  result.append("\n");

  return result;
}
