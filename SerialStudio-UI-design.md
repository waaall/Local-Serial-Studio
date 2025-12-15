# Serial Studio UI设计架构与修改指南

## 1. 架构概览

Serial Studio采用**QML + C++混合架构**,实现了UI与业务逻辑的清晰分离:

```
┌─────────────────────────────────────────────────────────────┐
│                    QML层 (用户界面)                          │
│  ├─ main.qml (应用入口)                                     │
│  ├─ MainWindow/ (主窗口和面板)                              │
│  ├─ Widgets/ (可复用UI组件)                                 │
│  ├─ Dialogs/ (对话框)                                       │
│  └─ ProjectEditor/ (项目编辑器)                             │
└─────────────────────────────────────────────────────────────┘
                          ↕ (属性绑定 & 信号槽)
┌─────────────────────────────────────────────────────────────┐
│                   C++层 (业务逻辑)                           │
│  ├─ UI::Dashboard (数据中心单例)                            │
│  ├─ IO::Manager (设备管理单例)                              │
│  ├─ JSON::FrameBuilder (数据解析单例)                       │
│  ├─ Misc::ThemeManager (主题管理单例)                       │
│  └─ Widgets::* (数据可视化模型类)                           │
└─────────────────────────────────────────────────────────────┘
                          ↕
┌─────────────────────────────────────────────────────────────┐
│              硬件/数据层                                     │
│  Serial Port | Network | Bluetooth | MQTT | Audio           │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

1. **单例模式**: 所有C++业务逻辑模块都是单例,确保全局唯一访问点
2. **属性系统**: 通过`Q_PROPERTY`暴露C++数据给QML,实现自动双向绑定
3. **信号驱动**: C++通过信号通知QML更新UI,QML通过槽函数响应
4. **模块化**: UI组件高度模块化,每个Widget独立封装
5. **主题分离**: 所有颜色/样式通过ThemeManager统一管理

---

### UI流程简要

1. app/qml/main.qml 创建 MainWindow、ProjectEditor 和各类对话框；C++ 单例在 app/src/Misc/ModuleManager.cpp 里注册成 Cpp_* context，QML 直接绑定。

2. app/qml/MainWindow/MainWindow.qml 用 Panes.Toolbar + StackView（Console/Dashboard）+ Panes.Setup 组合界面，首帧到达后从 Console 自动切换到 Dashboard。

3. app/qml/MainWindow/Panes/Dashboard.qml 负责背景、操作面板、DashboardCanvas、Taskbar 和 StartMenu。

4. DashboardCanvas.qml 通过 SerialStudio.UI::TaskBar + WindowManager 管理浮动窗口，Instantiator 从 taskbar 模型生成窗口，不是简单的 Repeater。

5. WidgetDelegate.qml 创建 UI::DashboardWidget（C++ 模型），由它决定 widget 类型、颜色和 QML 路径，然后载入 app/qml/Widgets/Dashboard/*.qml。外部窗口、主题切换等也在此处理。

#### 数据链路：

```text
驱动 → IO::FrameReader → JSON::FrameBuilder → UI::Dashboard::hotpathRxFrame 置位 m_updateRequired → Misc::TimerEvents::uiTimeout 触发 Dashboard::updated() → QML 绑定刷新。
```

## 2. UI文件结构详解

### 2.1 目录组织

```
app/qml/
├── main.qml                         # 应用入口点,管理生命周期
├── MainWindow/
│   ├── MainWindow.qml               # 主窗口布局(工具栏+中央区+侧边栏)
│   └── Panes/                       # 主窗口子面板
│       ├── Toolbar.qml              # 顶部工具栏
│       ├── Console.qml              # 串口控制台
│       ├── Dashboard.qml            # 数据仪表板容器
│       ├── Setup.qml                # 设备设置面板
│       ├── Dashboard/               # 仪表板组件
│       │   ├── DashboardCanvas.qml  # 小部件画布
│       │   ├── WidgetDelegate.qml   # 小部件代理(加载器)
│       │   ├── Taskbar.qml          # 任务栏
│       │   └── StartMenu.qml        # 启动菜单
│       └── SetupPanes/              # 设置面板组件
│           ├── Hardware.qml         # 硬件配置容器(StackLayout)
│           └── Drivers/             # 各驱动配置页
│               ├── UART.qml
│               ├── Network.qml
│               ├── BluetoothLE.qml
│               ├── Audio.qml        # Pro
│               ├── Modbus.qml       # Pro
│               └── CANBus.qml       # Pro
├── Widgets/                         # 可复用UI组件库
│   ├── Dashboard/                   # 数据可视化小部件(13+种)
│   │   ├── Gauge.qml                # 仪表盘
│   │   ├── Plot.qml                 # 曲线图
│   │   ├── MultiPlot.qml            # 多曲线图
│   │   ├── GPS.qml                  # GPS地图
│   │   ├── DataGrid.qml             # 数据表格
│   │   └── ...
│   ├── SmartWindow.qml              # 可停靠窗口基类
│   ├── MiniWindow.qml               # 小部件窗口容器
│   ├── Pane.qml                     # 面板容器
│   └── ...
├── ProjectEditor/                   # 项目配置编辑器
│   ├── ProjectEditor.qml
│   ├── Sections/
│   └── Views/
├── Dialogs/                         # 对话框
│   ├── Settings.qml                 # 设置对话框
│   ├── About.qml
│   ├── CsvPlayer.qml
│   └── ...
└── DialogLoader.qml                 # 延迟加载对话框
```

### 2.2 主窗口布局结构

**MainWindow.qml** 的核心布局:

```qml
Widgets.SmartWindow {
  id: root

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // 1. 顶部工具栏
    Panes.Toolbar {
      id: toolbar
      Layout.fillWidth: true
    }

    // 2. 中央区域(StackView在Console和Dashboard间切换)
    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true

      StackView {
        id: stack
        Layout.fillWidth: true
        Layout.fillHeight: true

        initialItem: terminal

        data: [
          Panes.Console {id: terminal},      // 控制台页
          Panes.Dashboard {id: dashboard}    // 仪表板页
        ]
      }

      // 3. 右侧设置面板(可隐藏)
      Panes.Setup {
        id: setup
        Layout.fillHeight: true
        visible: toolbar.setupChecked
      }
    }
  }
}
```

**设计要点**:
- 使用StackView实现页面切换(避免同时渲染两个复杂页面)
- RowLayout允许侧边栏响应式显示/隐藏
- 通过`toolbar.setupChecked`控制setup面板可见性

---

## 3. C++与QML数据绑定机制

### 3.1 C++单例注册到QML

**在ModuleManager.cpp中**:

```cpp
void ModuleManager::initializeQmlInterface() {
  // 初始化所有单例实例
  auto dashboard = &UI::Dashboard::instance();
  auto ioManager = &IO::Manager::instance();
  auto frameBuilder = &JSON::FrameBuilder::instance();
  auto themeManager = &Misc::ThemeManager::instance();
  // ...

  // 注册为QML全局对象
  const auto c = m_engine.rootContext();
  c->setContextProperty("Cpp_UI_Dashboard", dashboard);
  c->setContextProperty("Cpp_IO_Manager", ioManager);
  c->setContextProperty("Cpp_JSON_FrameBuilder", frameBuilder);
  c->setContextProperty("Cpp_ThemeManager", themeManager);
  // ... 共25+个全局对象
}
```

**在QML中直接访问**:

```qml
// 读取属性
Text {
  text: Cpp_UI_Dashboard.totalWidgetCount
  color: Cpp_ThemeManager.colors["text"]
}

// 监听信号
Connections {
  target: Cpp_UI_Dashboard
  function onUpdated() {
    dashboard.loadLayout()
  }
}

// 调用方法
Button {
  onClicked: Cpp_IO_Manager.connectDevice()
}
```

### 3.2 QML类型注册

**在ModuleManager.cpp中**:

```cpp
void ModuleManager::registerQmlTypes() {
  // 注册小部件模型
  qmlRegisterType<Widgets::Gauge>("SerialStudio", 1, 0, "GaugeModel");
  qmlRegisterType<Widgets::Plot>("SerialStudio", 1, 0, "PlotModel");
  qmlRegisterType<Widgets::DataGrid>("SerialStudio", 1, 0, "DataGridWidget");
  // ...

  // 注册UI辅助类
  qmlRegisterType<UI::Taskbar>("SerialStudio.UI", 1, 0, "TaskBar");

  // 注册枚举单例
  qmlRegisterSingletonType<SerialStudio>(
    "SerialStudio", 1, 0, "SerialStudio",
    [](QQmlEngine *, QJSEngine *) -> QObject * {
      return new SerialStudio();
    });
}
```

**在QML中使用**:

```qml
import SerialStudio 1.0
import SerialStudio.UI as SS_UI

Item {
  GaugeModel {
    id: gaugeWidget
    // 自动继承Q_PROPERTY定义的属性
  }

  SS_UI.TaskBar {
    id: taskbar
  }
}
```

### 3.3 属性绑定示例

**C++端 (Dashboard.h)**:

```cpp
class Dashboard : public QObject {
  Q_OBJECT

  // 只读属性(CONSTANT)
  Q_PROPERTY(QString title READ title NOTIFY widgetCountChanged)

  // 可读写属性(READ + WRITE)
  Q_PROPERTY(int points READ points WRITE setPoints NOTIFY pointsChanged)

  // 可调用方法
  Q_INVOKABLE bool frameValid() const;
  Q_INVOKABLE int relativeIndex(const int widgetIndex);

signals:
  void updated();              // 通知UI更新
  void pointsChanged();        // 属性变化通知
  void widgetCountChanged();   // 属性变化通知

public slots:
  void setPoints(int points);  // 属性setter
};
```

**QML端**:

```qml
Item {
  // 属性绑定(自动同步)
  property int currentPoints: Cpp_UI_Dashboard.points

  // 修改属性
  Slider {
    value: Cpp_UI_Dashboard.points
    onValueChanged: Cpp_UI_Dashboard.points = value  // 触发setPoints()
  }

  // 响应信号
  Connections {
    target: Cpp_UI_Dashboard
    function onPointsChanged() {
      console.log("Points changed to:", Cpp_UI_Dashboard.points)
    }
  }

  // 调用方法
  Component.onCompleted: {
    if (Cpp_UI_Dashboard.frameValid()) {
      // ...
    }
  }
}
```

### 3.4 数据更新流程(以Gauge小部件为例)

```
1. 串口接收到数据
   ↓
2. IO::Manager 接收原始字节流
   ↓
3. FrameReader 检测帧边界
   ↓
4. JSON::FrameBuilder 解析为 JSON::Frame
   ↓
5. Dashboard::hotpathRxFrame(frame)
   ├─ 更新 m_datasetReferences (数据缓存)
   └─ 设置 m_updateRequired = true
   ↓
6. Misc::TimerEvents 按设定频率(默认60Hz,可设置1-240Hz)发出 uiTimeout 信号
   ↓
7. Dashboard 检测到 m_updateRequired = true
   └─ 发出 signal: updated()
   ↓
8. Gauge::updateData() 槽函数被调用
   ├─ 从 Dashboard 读取最新数据
   └─ 发出 signal: Gauge::updated()
   ↓
9. QML Gauge.qml 中的属性绑定自动刷新
   └─ onNormalizedValueChanged: control.requestPaint()
   ↓
10. Canvas 重绘仪表盘
```

**关键优化**:
- **节流机制**: 即使数据以高频率到达,UI按配置的刷新率更新(默认60Hz,可在设置中调整为1-240Hz),避免过度渲染
- **数据缓存**: `m_datasetReferences`使用指针避免数据复制
- **延迟更新**: 只设置标志位,在定时器触发时批量更新

**注**: 目前构造函数读取键名`uiRefreshRate`,而设置页调用`setFPS()`写入键名`uiTimerHz`;这两个键不一致,导致刷新率调节不会在重启后生效,除非手动同步设置键或修正代码键名。

---

## 4. 主题系统详解

### 4.1 主题架构

**ThemeManager** (C++单例) 管理所有主题:

```cpp
// 8个预定义主题 + 1个自动主题
enum Theme {
  Default = 0,   // 默认主题
  Light,         // 亮色
  Dark,          // 暗色
  Iron,          // 铁锈
  Rust,          // 锈色
  Gunmetal,      // 枪灰
  Midday,        // 正午
  Midnight,      // 午夜
  System         // 跟随系统
};
```

**每个主题是一个JSON文件** (`app/rcc/themes/*.json`):

```json
{
  "title": "Dark",
  "parameters": {
    "code-editor-theme": "dark",
    "start-icon": "qrc:/rcc/logo/start-dark.svg"
  },
  "translations": {
    "en_US": "Dark",
    "zh_CN": "深色",
    // ... 13种语言(es_MX, de_DE, fr_FR, it_IT, ja_JP, ko_KR, pl_PL, pt_BR, ru_RU, tr_TR, cs_CZ, uk_UA)
  },
  "colors": {
    // ===== Qt标准调色板(21个) =====
    "base": "#0e0e0e",
    "text": "#aaadb2",
    "accent": "#4b6cb7",
    "button": "#1c1c1c",
    "highlight": "#4b6cb7",
    "window": "#0e0e0e",
    "window_text": "#aaadb2",
    "button_text": "#aaadb2",
    "bright_text": "#ffffff",
    "tooltip_base": "#191b1d",
    "tooltip_text": "#c5c7cb",
    "link": "#4b6cb7",
    "link_visited": "#a45a7a",
    "alternate_base": "#161616",
    "placeholder_text": "#606060",
    "highlighted_text": "#ffffff",
    "mid": "#434548",
    "dark": "#26282a",
    "light": "#232527",
    "midlight": "#232527",
    "shadow": "#000000",

    // ===== 分组盒(GroupBox) - 3个 =====
    "groupbox_border": "#242628",
    "groupbox_background": "#141414",
    "groupbox_hard_border": "#242628",

    // ===== 面板(Pane) - 6个 =====
    "pane_background": "#0e0e0e",
    "pane_section_label": "#aaadb2",
    "pane_caption_bg_top": "#0b0b0c",
    "pane_caption_border": "#242628",
    "pane_caption_bg_bottom": "#111111",
    "pane_caption_foreground": "#aaadb2",

    // ===== 工具栏(Toolbar) - 10个 =====
    "setup_border": "#242628",
    "toolbar_top": "#000000",
    "titlebar_text": "#ffffff",
    "toolbar_text": "#aaadb2",
    "toolbar_bottom": "#050505",
    "toolbar_border": "#242628",
    "toolbar_separator": "#242628",
    "toolbar_checked_button_opacity": 0.7,
    "toolbar_checked_button_border": "#242628",
    "toolbar_checked_button_background": "#101010",

    // ===== 控制台(Console) - 4个 =====
    "console_text": "#98c379",
    "console_base": "#0e0e0e",
    "console_border": "#212325",
    "console_highlight": "#4b6cb7",

    // ===== Widget窗口 - 9个 =====
    "widget_text": "#b6b8bc",
    "widget_base": "#262626",
    "widget_button": "#191919",
    "widget_border": "#5B5E60",
    "widget_window": "#0e0e0e",
    "widget_highlight": "#4b6cb7",
    "widget_button_text": "#b6b8bc",
    "widget_highlighted_text": "#26282a",
    "widget_placeholder_text": "#727272",

    // ===== 窗口(Window) - 10个 =====
    "window_border": "#262626",
    "window_toolbar_background": "#121212",
    "window_caption_active_top": "#181818",
    "window_caption_active_text": "#ffffff",
    "window_caption_inactive_top": "#121212",
    "window_caption_inactive_text": "#727478",
    "window_caption_active_bottom": "#121212",
    "window_caption_inactive_bottom": "#121212",

    // ===== 任务栏(Taskbar) - 11个 =====
    "taskbar_top": "#1a1a1a",
    "taskbar_text": "#b6b8bc",
    "taskbar_bottom": "#0e0e0e",
    "taskbar_border": "#242628",
    "taskbar_separator": "#242628",
    "taskbar_highlight": "#4b6cb7",
    "taskbar_indicator_active": "#ffffff",
    "taskbar_indicator_inactive": "#6e7074",
    "taskbar_checked_button_top": "#161616",
    "taskbar_checked_button_border": "#0e0e0e",
    "taskbar_checked_button_bottom": "#18191a",

    // ===== 开始菜单(Start Menu) - 8个 =====
    "start_menu_text": "#c8cdd2",
    "start_menu_border": "#242628",
    "start_menu_highlight": "#3b5fb2",
    "start_menu_background": "#131415",
    "start_menu_gradient_top": "#202224",
    "start_menu_version_text": "#e8e9ec",
    "start_menu_gradient_bottom": "#18191b",
    "start_menu_highlighted_text": "#ffffff",

    // ===== 表格(Table) - 8个 =====
    "table_text": "#aaadb2",
    "table_cell_bg": "#0e0e0e",
    "table_fg_header": "#c8cdd2",
    "table_separator": "#242628",
    "table_bg_header_top": "#1a1a1a",
    "table_border_header": "#242628",
    "table_bg_header_bottom": "#161616",
    "table_separator_header": "#242628",

    // ===== 其他 =====
    "error": "#e06c75",
    "alarm": "#a02125",
    "dashboard_background": "#0a0a0a",
    "snap_indicator_border": "#1a83ca",
    "snap_indicator_background": "#401a83ca",

    // ===== 极坐标(Polar) - 3个 =====
    "polar_indicator": "#ea6654",
    "polar_background": "#0e0e0e",
    "polar_foreground": "#d2d2d2",

    // ===== 3D绘图(Plot3D) - 7个 =====
    "plot3d_x_axis": "#f46c7a",
    "plot3d_y_axis": "#99d95c",
    "plot3d_z_axis": "#5aaef2",
    "plot3d_axis_text": "#ffffff",
    "plot3d_grid_major": "#43484b",
    "plot3d_grid_minor": "#232323",
    "plot3d_background_inner": "#171717",
    "plot3d_background_outer": "#0f0f0f",

    // ===== 数据可视化颜色数组 =====
    "widget_colors": [
      "#4A90E2", "#50FA7B", "#FF5555", "#FFB86C",
      "#BD93F9", "#F1FA8C", "#FF79C6", "#8BE9FD",
      "#CFCFCF", "#FF6E6E", "#5AF78E"
    ]
  }
}
```

**颜色键位总计**: 70+个,分为10大类(Qt标准调色板、GroupBox、Pane、Toolbar、Console、Widget窗口、Window、Taskbar、Start Menu、Table) + 3D绘图、极坐标等专用颜色。

**提示**: 复制现有主题JSON文件时,务必保留所有颜色键位,否则部分UI组件会使用默认颜色导致视觉不一致。参考 `app/rcc/themes/dark.json` 获取完整模板。

### 4.2 QML中使用主题

**模式A: Page级调色板设置(影响所有子控件)**

```qml
Page {
  id: root

  // 设置Qt标准调色板
  palette.base: Cpp_ThemeManager.colors["base"]
  palette.text: Cpp_ThemeManager.colors["text"]
  palette.button: Cpp_ThemeManager.colors["button"]
  palette.highlight: Cpp_ThemeManager.colors["highlight"]
  palette.window: Cpp_ThemeManager.colors["window"]
  // ... 所有Qt Quick Controls自动继承

  Button {
    text: "我会自动使用palette.button颜色"
  }
}
```

**模式B: 直接颜色访问(自定义控件)**

```qml
Rectangle {
  color: Cpp_ThemeManager.colors["console_base"]
  border.color: Cpp_ThemeManager.colors["console_border"]

  Label {
    color: Cpp_ThemeManager.colors["toolbar_text"]
  }
}
```

**模式C: 响应主题变化**

```qml
Canvas {
  id: control

  onPaint: {
    const ctx = getContext("2d")
    ctx.fillStyle = Cpp_ThemeManager.colors["widget_base"]
    // ... 绘制
  }

  // 主题变化时自动重绘
  Connections {
    target: Cpp_ThemeManager
    function onThemeChanged() {
      control.requestPaint()
    }
  }
}
```

### 4.3 创建新主题

**步骤**:

1. 复制现有主题JSON:
   ```bash
   cp app/rcc/themes/dark.json app/rcc/themes/mytheme.json
   ```

2. 修改颜色值:
   ```json
   {
     "title": "MyTheme",
     "colors": {
       "base": "#yourcolor",
       // ... 修改所有颜色
     }
   }
   ```

3. 注册到资源文件 (`app/rcc/rcc.qrc`):
   ```xml
   <file>themes/mytheme.json</file>
   ```

4. **关键步骤**:在 `app/src/Misc/ThemeManager.cpp` 构造函数中添加到themes列表:
   ```cpp
   // ThemeManager::ThemeManager() 构造函数中
   const QStringList themes = {
       QStringLiteral("default"),
       QStringLiteral("light"),
       QStringLiteral("dark"),
       QStringLiteral("iron"),
       QStringLiteral("rust"),
       QStringLiteral("gunmetal"),
       QStringLiteral("midday"),
       QStringLiteral("midnight"),
       QStringLiteral("mytheme"),  // 添加新主题文件名(不带.json)
   };
   ```
   **注意**: 主题会按此列表顺序自动加载,无需手动调用`m_themes.insert()`。

5. 重新编译

---

## 5. 使用Qt Creator修改UI

### 5.1 Qt Creator打开项目

**推荐版本**: Qt Creator 13+ (配合Qt 6.9.2)

**打开方式**:
1. 启动Qt Creator
2. File → Open File or Project
3. 选择 `/Users/zhengxu/Desktop/some_code/Local-Serial-Studio/CMakeLists.txt`
4. 选择Kit (Qt 6.9.2 配置)
5. Configure Project

### 5.2 QML可视化编辑

**编辑QML文件**:

1. 在Qt Creator左侧项目树中找到 `app/qml/Widgets/Dashboard/Gauge.qml`
2. 双击打开文件
3. 切换到"Design"模式(左侧边栏图标或Ctrl+2)

**可视化编辑器功能**:
- **属性编辑器**: 右侧面板可直接修改属性(颜色、尺寸、字体等)
- **层次结构**: 查看QML元素树
- **状态管理**: 创建和管理UI状态
- **布局工具**: 拖放布局管理器(RowLayout, ColumnLayout)

**限制**:
- 复杂的自定义Canvas绘制无法可视化预览
- 需要在"Edit"模式下编写JavaScript逻辑

### 5.3 实时预览(qmlscene)

**单独预览QML文件**:

```bash
# 安装qmlscene (通常随Qt安装)
/path/to/Qt/6.9.2/macos/bin/qmlscene app/qml/Widgets/Pane.qml
```

**注意**: 需要确保QML文件不依赖C++单例,或者通过Mock对象模拟

### 5.4 热重载开发流程

**方法1: 使用qmlscene监视模式**

```bash
qmlscene --watch app/qml/MainWindow/MainWindow.qml
# 修改QML文件后自动重新加载
```

**方法2: 在主程序中启用QML调试**

在main.cpp中:
```cpp
QQmlDebuggingEnabler enabler;
```

然后在Qt Creator中:
1. Debug → Start Debugging → Attach to QML Port
2. 修改QML后保存,部分更改会实时反映

### 5.5 修改UI布局示例

**任务**: 在MainWindow添加一个新的侧边栏按钮

**步骤**:

1. 打开 `app/qml/MainWindow/Panes/Toolbar.qml`

2. 找到按钮列表区域:
   ```qml
   RowLayout {
     // 现有按钮
     ToolButton {
       id: setupButton
       text: qsTr("Setup")
     }
   }
   ```

3. 添加新按钮:
   ```qml
   ToolButton {
     id: myNewButton
     text: qsTr("My Feature")
     icon.source: "qrc:/rcc/icons/myicon.svg"
     checkable: true
     checked: false
     palette.buttonText: Cpp_ThemeManager.colors["toolbar_text"]

     onClicked: {
       // 调用C++方法或切换UI
       console.log("New button clicked!")
     }
   }
   ```

4. 保存并重新编译运行

**调试技巧**:
- 使用`console.log()`在QML中打印调试信息
- 在Qt Creator的"Application Output"窗口查看输出
- 使用Qt Creator的QML Profiler分析性能

---

## 6. 添加新UI组件(Widget)

### 6.1 创建新的数据可视化Widget

**场景**: 添加一个新的"温度计"Widget

**步骤1: 创建C++模型类**

文件: `app/src/UI/Widgets/Thermometer.h`

```cpp
#pragma once

#include <QQuickItem>

namespace Widgets
{
class Thermometer : public QQuickItem
{
  Q_OBJECT

  // 暴露给QML的属性
  Q_PROPERTY(QString units READ units CONSTANT)
  Q_PROPERTY(double value READ value NOTIFY updated)
  Q_PROPERTY(double minValue READ minValue CONSTANT)
  Q_PROPERTY(double maxValue READ maxValue CONSTANT)
  Q_PROPERTY(double normalizedValue READ normalizedValue NOTIFY updated)

signals:
  void updated();  // 数据更新信号

public:
  explicit Thermometer(QQuickItem *parent = nullptr);

  // 属性getter
  QString units() const { return m_units; }
  double value() const { return m_value; }
  double minValue() const { return m_minValue; }
  double maxValue() const { return m_maxValue; }
  double normalizedValue() const;

private slots:
  void updateData();  // 从Dashboard获取数据

private:
  int m_index;
  QString m_units;
  double m_value;
  double m_minValue;
  double m_maxValue;
};
}
```

文件: `app/src/UI/Widgets/Thermometer.cpp`

```cpp
#include "Thermometer.h"
#include "UI/Dashboard.h"

Widgets::Thermometer::Thermometer(QQuickItem *parent)
  : QQuickItem(parent)
  , m_index(0)
  , m_value(0)
  , m_minValue(0)
  , m_maxValue(100)
{
  // 读取配置
  if (VALIDATE_WIDGET(SerialStudio::DashboardBar, m_index))
  {
    const auto &dataset = GET_DATASET(SerialStudio::DashboardBar, m_index);
    m_units = dataset.units();
    m_minValue = dataset.min();
    m_maxValue = dataset.max();
  }

  // 连接到Dashboard的更新信号
  connect(&UI::Dashboard::instance(), &UI::Dashboard::updated, this,
          &Thermometer::updateData);
}

void Widgets::Thermometer::updateData()
{
  if (!isVisible())
    return;

  if (VALIDATE_WIDGET(SerialStudio::DashboardBar, m_index))
  {
    const auto &dataset = GET_DATASET(SerialStudio::DashboardBar, m_index);
    m_value = dataset.value().toDouble();
    Q_EMIT updated();
  }
}

double Widgets::Thermometer::normalizedValue() const
{
  const auto range = m_maxValue - m_minValue;
  if (range != 0)
    return (m_value - m_minValue) / range;
  return 0;
}
```

**步骤2: 注册到QML**

在 `app/src/Misc/ModuleManager.cpp` 的 `registerQmlTypes()` 中添加:

```cpp
qmlRegisterType<Widgets::Thermometer>("SerialStudio", 1, 0, "ThermometerModel");
```

**步骤3: 创建QML视图**

文件: `app/qml/Widgets/Dashboard/Thermometer.qml`

```qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import SerialStudio 1.0

Item {
  id: root

  // 必需属性(从DashboardWidget传入)
  required property color color
  required property var windowRoot
  required property ThermometerModel model

  // 响应式数据绑定
  property real normalizedValue: model.normalizedValue
  Behavior on normalizedValue {NumberAnimation{duration: 100}}

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8

    // 温度计视觉表示
    Rectangle {
      id: tube
      width: 60
      height: parent.height * 0.7
      color: "transparent"
      border.width: 2
      border.color: Cpp_ThemeManager.colors["widget_border"]
      radius: 30

      // 水银柱
      Rectangle {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.6
        height: parent.height * root.normalizedValue
        color: root.color
        radius: width / 2

        Behavior on height {NumberAnimation{duration: 300}}
      }
    }

    // 数值显示
    Label {
      text: model.value.toFixed(1) + " " + model.units
      color: Cpp_ThemeManager.colors["text"]
      font.pixelSize: 18
      font.bold: true
      horizontalAlignment: Text.AlignHCenter
      Layout.fillWidth: true
    }

    // 范围显示
    Label {
      text: qsTr("Range: %1 ~ %2").arg(model.minValue).arg(model.maxValue)
      color: Cpp_ThemeManager.colors["text"]
      font.pixelSize: 12
      opacity: 0.7
      horizontalAlignment: Text.AlignHCenter
      Layout.fillWidth: true
    }
  }

  // 响应主题变化
  Connections {
    target: Cpp_ThemeManager
    function onThemeChanged() {
      // 触发重绘或更新
    }
  }
}
```

**步骤4: 更新SerialStudio.h枚举**

在 `app/src/SerialStudio.h` 中添加新Widget类型:

```cpp
enum DashboardWidget {
  // ... 现有类型
  DashboardThermometer,  // 新增
  DashboardNoWidget,
};
```

**步骤5: 在SerialStudio.cpp中添加映射逻辑**

在 `app/src/SerialStudio.cpp` 的 `getDashboardWidgets()` 函数中添加:

```cpp
QList<SerialStudio::DashboardWidget>
SerialStudio::getDashboardWidgets(const JSON::Dataset &dataset)
{
  QList<DashboardWidget> list;
  const auto &widget = dataset.widget;

  // 添加新Widget类型的匹配
  if (widget == "thermometer")
    list.append(DashboardThermometer);

  // ... 其他已有Widget判断

  return list;
}
```

**步骤6: 关键步骤 - 在DashboardWidget.cpp中注册**

在 `app/src/UI/DashboardWidget.cpp` 的 `setWidgetIndex()` 函数的switch语句中添加case:

```cpp
void UI::DashboardWidget::setWidgetIndex(const int index)
{
  // ... 现有代码

  switch (widgetType())
  {
    // ... 现有cases

    case SerialStudio::DashboardThermometer:
      m_dbWidget = new Widgets::Thermometer(relativeIndex(), this);
      m_qmlPath = "qrc:/serial-studio.com/gui/qml/Widgets/Dashboard/Thermometer.qml";
      break;

    default:
      break;
  }

  // ... 现有代码
}
```

**注意**: 这一步至关重要,否则TaskBar/WindowManager不会为新Widget创建窗口。

**步骤7: 重新编译**

```bash
cd build
cmake --build . -j$(nproc)
```

**步骤8: 将QML文件加入构建资源**

如果新增了 `app/qml/Widgets/Dashboard/Thermometer.qml`，需在 `app/CMakeLists.txt` 的 `qt_add_qml_module` 文件列表中加入该 QML 路径，否则打包后运行会找不到新组件。

---

## 7. 修改现有Widget外观

### 7.1 修改Gauge Widget的颜色和样式

**文件**: `app/qml/Widgets/Dashboard/Gauge.qml`

**原始代码**(第200-250行):

```qml
Canvas {
  id: control

  onPaint: {
    const ctx = getContext("2d")

    // 绘制背景弧线
    ctx.strokeStyle = Cpp_ThemeManager.colors["widget_base"]
    ctx.lineWidth = 12
    ctx.arc(cx, cy, radius, startRad, endRad)
    ctx.stroke()

    // 绘制数值弧线
    ctx.strokeStyle = root.color
    ctx.lineWidth = 12
    ctx.arc(cx, cy, radius, startRad, valueRad)
    ctx.stroke()
  }
}
```

**修改1: 更改线宽和颜色**

```qml
onPaint: {
  const ctx = getContext("2d")

  // 更粗的背景线
  ctx.strokeStyle = Cpp_ThemeManager.colors["widget_border"]
  ctx.lineWidth = 20  // 从12改为20
  ctx.arc(cx, cy, radius, startRad, endRad)
  ctx.stroke()

  // 渐变色数值线
  const gradient = ctx.createLinearGradient(0, 0, control.width, 0)
  gradient.addColorStop(0, root.color)
  gradient.addColorStop(1, Qt.lighter(root.color, 1.5))
  ctx.strokeStyle = gradient
  ctx.lineWidth = 18
  ctx.arc(cx, cy, radius, startRad, valueRad)
  ctx.stroke()
}
```

**修改2: 添加警报视觉效果**

```qml
// 在Canvas下方添加
Rectangle {
  anchors.fill: parent
  color: "transparent"
  border.width: 3
  border.color: root.model.alarmTriggered ? "red" : "transparent"
  radius: width / 2

  // 闪烁动画
  SequentialAnimation on opacity {
    running: root.model.alarmTriggered
    loops: Animation.Infinite
    NumberAnimation { from: 1.0; to: 0.3; duration: 500 }
    NumberAnimation { from: 0.3; to: 1.0; duration: 500 }
  }
}
```

### 7.2 修改Plot Widget的轴标签

**文件**: `app/qml/Widgets/PlotWidget.qml` (Plot的基类)

**找到轴标签代码**(第300行左右):

```qml
Item {
  id: xAxisLabels

  Repeater {
    model: root.xAxis.tickCount

    Label {
      text: (root.xMin + index * root.xAxis.tickInterval).toFixed(1)
      color: Cpp_ThemeManager.colors["text"]
      font.pixelSize: 10
    }
  }
}
```

**修改: 更大字体和自定义格式**

```qml
Label {
  // 更大的字体
  font.pixelSize: 12
  font.bold: true

  // 自定义数字格式(科学计数法)
  text: {
    const value = root.xMin + index * root.xAxis.tickInterval
    if (Math.abs(value) > 1000)
      return value.toExponential(1)
    else
      return value.toFixed(2)
  }

  // 自定义颜色
  color: index === 0 ? "red" : Cpp_ThemeManager.colors["text"]
}
```

---

## 8. 使用其他工具修改UI

### 8.1 QML Scene Editor (独立工具)

**安装**:
```bash
# 通过Qt维护工具安装
# 或下载qmlscene工具
brew install qt@6  # macOS
```

**使用**:
```bash
qmlscene app/qml/Widgets/Dashboard/Gauge.qml
```

**优点**:
- 快速预览单个QML文件
- 无需完整编译项目
- 支持热重载

**缺点**:
- 无法访问C++单例(需要Mock)
- 不能调试复杂交互

### 8.2 Figma → QML 工作流

**推荐场景**: 设计复杂的静态UI布局

**步骤**:

1. 在Figma中设计UI界面
2. 使用插件导出为SVG/PNG资源
3. 在QML中使用`Image`组件加载:
   ```qml
   Image {
     source: "qrc:/rcc/images/my-design.svg"
     sourceSize: Qt.size(width, height)
   }
   ```
4. 在Image上叠加交互元素(Button, MouseArea等)

**工具**: Figma QML Exporter插件(第三方)

### 8.3 使用VS Code + QML扩展

**安装**:
1. 安装VS Code
2. 安装扩展: "QML" by bbenoist

**功能**:
- 语法高亮
- 代码补全
- 错误检查
- 格式化

**配置** (`.vscode/settings.json`):
```json
{
  "qml.executablePath": "/path/to/Qt/6.9.2/macos/bin/qml",
  "qml.lint.enabled": true
}
```

### 8.4 使用qmllint进行代码检查

```bash
# 安装(通常随Qt安装)
/path/to/Qt/6.9.2/macos/bin/qmllint app/qml/MainWindow/MainWindow.qml

# 检查所有QML文件
find app/qml -name "*.qml" -exec qmllint {} \;
```

**常见警告**:
- 未使用的导入
- 未定义的属性
- 类型不匹配

---

## 9. UI设计最佳实践

### 9.1 属性绑定性能优化

**❌ 避免复杂表达式绑定**:
```qml
Label {
  // 每次paint时都会重新计算
  text: Cpp_UI_Dashboard.datasets.filter(d => d.visible).map(d => d.value).join(", ")
}
```

**✅ 使用中间属性**:
```qml
Label {
  id: label

  Connections {
    target: Cpp_UI_Dashboard
    function onUpdated() {
      // 只在需要时计算
      const visible = Cpp_UI_Dashboard.datasets.filter(d => d.visible)
      label.text = visible.map(d => d.value).join(", ")
    }
  }
}
```

### 9.2 避免过度嵌套

**❌ 深层嵌套**:
```qml
Item {
  Item {
    Item {
      Item {
        Rectangle { /* 实际内容 */ }
      }
    }
  }
}
```

**✅ 扁平化**:
```qml
Rectangle {
  id: content
  // 直接定义
}
```

### 9.3 使用Loader延迟加载

**场景**: 对话框在需要时才加载

```qml
Loader {
  id: settingsDialog
  active: false
  source: "qrc:/qml/Dialogs/Settings.qml"

  function show() {
    active = true
    item.visible = true
  }
}

Button {
  onClicked: settingsDialog.show()
}
```

### 9.4 主题颜色统一

**❌ 硬编码颜色**:
```qml
Rectangle {
  color: "#1c1c1c"  // 主题切换时不变
}
```

**✅ 使用ThemeManager**:
```qml
Rectangle {
  color: Cpp_ThemeManager.colors["console_base"]
}
```

### 9.5 响应式设计

**使用Layout而非固定尺寸**:

```qml
// ❌ 固定尺寸
Rectangle {
  width: 800
  height: 600
}

// ✅ 响应式
Rectangle {
  Layout.fillWidth: true
  Layout.fillHeight: true
  Layout.minimumWidth: 400
  Layout.minimumHeight: 300
}
```

---

## 10. 常见UI修改场景

### 10.1 添加新的设置选项

**文件**: `app/qml/Dialogs/Settings.qml`

**在"Language"设置后添加新选项**:

```qml
// 找到现有的设置项
Widgets.Pane {
  title: qsTr("Language")

  ComboBox {
    model: Cpp_Misc_Translator.availableLanguages
    currentIndex: Cpp_Misc_Translator.language
  }
}

// 添加新设置项
Widgets.Pane {
  title: qsTr("Auto Connect")

  Switch {
    id: autoConnectSwitch
    checked: Cpp_IO_Manager.autoConnect  // 假设C++端添加了此属性

    onToggled: {
      Cpp_IO_Manager.autoConnect = checked
    }
  }

  Label {
    text: qsTr("Automatically connect to last device on startup")
    color: Cpp_ThemeManager.colors["text"]
    font.pixelSize: 12
    opacity: 0.7
    wrapMode: Label.WordWrap
  }
}
```

**对应C++端修改** (`app/src/IO/Manager.h`):

```cpp
class Manager : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)

signals:
  void autoConnectChanged();

public:
  bool autoConnect() const { return m_autoConnect; }
  void setAutoConnect(bool enabled) {
    if (m_autoConnect != enabled) {
      m_autoConnect = enabled;
      m_settings.setValue("AutoConnect", enabled);
      Q_EMIT autoConnectChanged();
    }
  }

private:
  bool m_autoConnect;
};
```

### 10.2 修改Dashboard的布局方式

**当前**: 使用MiniWindow自由拖放

**修改为**: 网格布局

**文件**: `app/qml/MainWindow/Panes/Dashboard/DashboardCanvas.qml`

**原始代码**:
```qml
Item {
  id: root

  Repeater {
    model: Cpp_UI_Dashboard.totalWidgetCount

    Widgets.MiniWindow {
      // 可拖动的窗口
      widgetIndex: index
      taskBar: taskbar
    }
  }
}
```

**修改为网格**:
```qml
GridView {
  id: grid
  anchors.fill: parent
  cellWidth: 300
  cellHeight: 250

  model: Cpp_UI_Dashboard.totalWidgetCount

  delegate: Item {
    width: grid.cellWidth
    height: grid.cellHeight

    WidgetDelegate {
      anchors.fill: parent
      anchors.margins: 4
      widgetIndex: index
    }
  }
}
```

### 10.2.1 Setup面板结构说明

**实际架构**:

`Setup.qml` → 加载 `SetupPanes/Hardware.qml` → `StackLayout`切换不同驱动配置页

```qml
// app/qml/MainWindow/Panes/Setup.qml
Widgets.Pane {
  title: qsTr("Device Setup")

  ColumnLayout {
    // 顶部:操作模式选择、CSV导出等

    // 底部:硬件配置
    SetupPanes.Hardware {  // 单一入口
      id: hardware
    }
  }
}

// app/qml/MainWindow/Panes/SetupPanes/Hardware.qml
Rectangle {
  StackLayout {
    currentIndex: Cpp_IO_Manager.busType  // 根据驱动类型切换

    Loader { sourceComponent: Drivers.UART {} }
    Loader { sourceComponent: Drivers.Network {} }
    Loader { sourceComponent: Drivers.BluetoothLE {} }
    Loader { source: "Drivers/Audio.qml" }      // Pro
    Loader { source: "Drivers/Modbus.qml" }     // Pro
    Loader { source: "Drivers/CANBus.qml" }     // Pro
  }
}
```

**注意**: 驱动配置页位于 `SetupPanes/Drivers/` 目录,而非直接在 `SetupPanes/` 下。

### 10.3 更改字体

**全局字体设置** (main.cpp):

```cpp
int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // 设置全局字体
  QFont font("Source Han Sans CN", 12);  // 思源黑体
  app.setFont(font);

  // 或从系统加载
  QFontDatabase::addApplicationFont(":/fonts/MyCustomFont.ttf");
  app.setFont(QFont("MyCustomFont"));

  // ...
}
```

**QML中覆盖特定元素**:
```qml
Label {
  font.family: "Courier New"  // 等宽字体
  font.pixelSize: 14
  font.bold: true
}
```

---

## 11. 调试技巧

### 11.1 QML调试输出

```qml
// 简单输出
console.log("Value:", model.value)

// 带时间戳
console.log(new Date().toISOString(), "Dashboard updated")

// 输出对象
console.log(JSON.stringify(Cpp_ThemeManager.colors, null, 2))
```

### 11.2 QML Profiler

**启动**:
1. Qt Creator → Analyze → QML Profiler
2. 运行应用
3. 查看性能热点(渲染时间、绑定更新频率等)

**优化目标**:
- 单帧渲染 < 16ms (60 FPS)
- 避免频繁的属性绑定触发

### 11.3 Inspector工具

**启用**:
在main.cpp中:
```cpp
#ifdef QT_DEBUG
  qputenv("QML_DISABLE_OPTIMIZER", "1");
  qputenv("QSG_VISUALIZE", "overdraw");  // 可视化过度绘制
#endif
```

**运行时**: 右键QML元素 → Inspect

---

## 12. 项目JSON配置与UI的关系

### 12.1 Project JSON结构

**文件示例**: `examples/HexadecimalADC/HexadecimalADC.json`

```json
{
  "title": "Hexadecimal ADC",
  "frameDetection": 2,  // StartAndEndDelimiter
  "startSequence": "/*",
  "finishSequence": "*/",
  "groups": [
    {
      "title": "Sensors",
      "widget": "multiplot",  // 对应 DashboardMultiPlot
      "datasets": [
        {
          "title": "Temperature",
          "units": "°C",
          "widget": "gauge",  // 对应 DashboardGauge
          "min": 0,
          "max": 100,
          "alarm": 80,
          "graph": true
        },
        {
          "title": "Pressure",
          "units": "kPa",
          "widget": "bar",  // 主Widget类型: bar/gauge/compass
          "plt": true,      // 额外创建折线图
          "min": 0,
          "max": 200
        }
      ]
    }
  ]
}
```

### 12.2 Widget字段映射

**在SerialStudio.cpp的getDashboardWidgets()中**:

```cpp
QList<SerialStudio::DashboardWidget>
SerialStudio::getDashboardWidgets(const JSON::Dataset &dataset)
{
  QList<DashboardWidget> list;
  const auto &widget = dataset.widget;

  // 主Widget类型(单值字符串匹配)
  if (widget == "compass")
    list.append(DashboardCompass);
  else if (widget == "bar")
    list.append(DashboardBar);
  else if (widget == "gauge")
    list.append(DashboardGauge);

  // 额外可视化(布尔字段控制)
  if (dataset.plt)   // "plt": true 创建折线图
    list.append(DashboardPlot);
  if (dataset.fft)   // "fft": true 创建FFT频谱图
    list.append(DashboardFFT);
  if (dataset.led)   // "led": true 创建LED面板
    list.append(DashboardLED);

  return list;
}
```

**重要**:
- `widget`字段只接受**单个值**: `"bar"`、`"gauge"`、`"compass"`(不能用逗号分隔)
- 要同时显示多种可视化,使用**布尔字段**: `"plt": true`、`"fft": true`、`"led": true`
- 例: `{"widget": "gauge", "plt": true, "fft": true}` 会创建仪表盘+折线图+FFT频谱图

### 12.3 ProjectEditor修改配置

**图形化编辑器**: `app/qml/ProjectEditor/ProjectEditor.qml`

**功能**:
- 添加/删除Group和Dataset
- 修改Widget类型
- 设置min/max/alarm值
- 配置数据解析器(JavaScript)

**保存**: 自动同步到JSON文件,Dashboard实时更新

---

## 13. 总结: UI修改速查表

| 任务 | 文件位置 | 工具 |
|------|---------|------|
| 修改主窗口布局 | `app/qml/MainWindow/MainWindow.qml` | Qt Creator Design模式 |
| 修改工具栏按钮 | `app/qml/MainWindow/Panes/Toolbar.qml` | Qt Creator |
| 修改主题颜色 | `app/rcc/themes/*.json` | 文本编辑器 |
| 添加新Widget | `app/src/UI/Widgets/*.{h,cpp}` + `app/qml/Widgets/Dashboard/*.qml` | Qt Creator + CMake |
| 修改Widget外观 | `app/qml/Widgets/Dashboard/*.qml` | Qt Creator |
| 调整字体 | `main.cpp` 或 QML `font.family` | Qt Creator |
| 修改设置对话框 | `app/qml/Dialogs/Settings.qml` | Qt Creator |
| 添加C++属性 | `app/src/*/xxx.h` (Q_PROPERTY) | Qt Creator C++编辑器 |
| 绑定新逻辑 | QML中使用 `Connections` 或 `onXxxChanged` | Qt Creator |
| 性能调优 | 使用QML Profiler | Qt Creator Profiler |

---

## 14. 常见问题

**Q: 修改QML后不生效?**
A: 确保重新编译,QML文件嵌入到资源文件(rcc)中。或使用qmlscene独立预览。

**Q: C++属性在QML中undefined?**
A: 检查是否在ModuleManager中注册了`setContextProperty`,并确保单例已初始化。

**Q: 主题颜色不存在?**
A: 检查主题JSON中是否定义了该颜色键,或使用标准Qt palette属性。

**Q: Widget不显示数据?**
A: 确保Dashboard已接收到数据帧,检查updateData()是否被调用,查看qDebug输出。

**Q: 如何调试QML性能问题?**
A: 使用QML Profiler,查找频繁的属性绑定更新或复杂的JavaScript计算。

---
