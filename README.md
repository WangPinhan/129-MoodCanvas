# MoodCanvasQt · 心象日记

把文字情绪，转化为可视化的心象风景。

MoodCanvasQt 是一款基于 Qt Widgets 的桌面端情绪记录应用。用户写下一段日记后，程序会调用 OpenAI 兼容的大模型接口进行中文情绪分析，并把分析结果映射为一张动态的“心象画布”：颜色、流动速度、关键词星座、极光线条与光点都会随情绪向量变化。

项目强调低门槛记录、温柔反馈和隐私可控。日记正文与分析结果默认保存在本机 JSON 文件中；只有在启用大模型分析或云端树洞时，才会向配置的 API 服务发送必要数据。

> 说明：当前客户端主流程为 API-only 情绪分析，已移除本地训练模型回退；生成心象前需要配置有效的大模型 API Key。

---

## 当前实现

| 模块 | 当前代码中的实现 |
|------|------------------|
| 登录 / 注册 | 启动时进入 `LoginDialog`，通过云端 REST API 完成注册、登录与会话保存。 |
| 今日心象输入 | 首页输入日记文本，调用大模型生成情绪类别、情绪权重、愉悦度、激活度、关键词、安慰语与 CBT 小建议。 |
| 动态心象画布 | `MoodCanvasWidget` 使用 `QPainter` 自绘渐变背景、色带、极光线、星光点、关键词气泡与日记摘录。 |
| 今日植物 | `PlantWidget` 根据当天最后一条记录切换休眠、开花、新叶、卷叶、雨中生长等状态。 |
| 时间长廊 | 月历视图按日期显示心象主色；点击日期查看当天全部记录，并支持删除单条记录。 |
| 每周情绪报告 | 统计本周情绪分布、高频关键词、触发因素与周内变化，调用大模型生成周报文字，支持导出 PNG / PDF。 |
| 匿名树洞 | 分享最近一次心象的模糊心情到云端树洞，支持查看实时心象列表与发送回应。 |
| 树洞情感着色 | `TreeholeEmotionAnalyzer` 对树洞心情和回复进行句子级情绪分析，并用本地色彩算法渲染卡片。 |
| 设置 / 隐私 | 配置主题、大模型 API、云端地址，支持连接测试与本地 JSON 数据导出。 |

---

## 技术栈

| 方向 | 技术 |
|------|------|
| 客户端 | Qt 6 Widgets / C++17 |
| 构建 | qmake + MinGW |
| 网络 | Qt Network，调用 OpenAI 兼容 Chat Completions 接口 |
| 异步任务 | Qt Concurrent / 定时同步 |
| 可视化 | `QPainter` 自定义绘制心象画布、周报图表与植物卡片 |
| 本地存储 | Qt 标准应用数据目录下的 JSON 文件 |
| 云端交互 | REST API：登录注册、树洞帖子与回复同步 |

当前 `.pro` 工程包含的 Qt 模块：

```pro
QT += widgets network concurrent
CONFIG += c++17
```

---

## 项目结构

```text
MoodCanvasQt/
├── MoodCanvasQt.pro          # qmake 工程文件
├── main.cpp                  # 程序入口：主题初始化 -> 登录 -> 主窗口
├── mainwindow.*              # 首页输入、生成心象、导航入口
├── llmclient.*               # OpenAI 兼容大模型请求与 JSON 解析
├── datastore.*               # 本地日记、设置、云端会话配置读写
├── moodentry.*               # 情绪记录数据结构与 JSON 序列化
├── moodcanvaswidget.*        # 动态心象画布自绘
├── resultdialog.*            # 分析结果与保存弹窗
├── timelinewindow.*          # 时间长廊（月历与记录详情）
├── weeklyreportwindow.*      # 每周报告、图表、PNG/PDF 导出
├── weeklyscenerywidget.*     # 周报视觉场景
├── plantwidget.*             # 今日植物状态
├── settingsdialog.*          # API、云端、主题、导出设置
├── logindialog.*             # 登录 / 注册界面
├── userservice.*             # 云端账号逻辑
├── cloudapiclient.*          # 云端 REST 客户端
├── cloudtreehole.*           # 树洞帖子轮询与发布
├── treeholeemotion.*         # 树洞句子情绪分析与 HTML 着色
├── toolboxdialog.*           # 匿名树洞界面
├── thememanager.*            # 浅色 / 深色 / 跟随系统主题
├── resources.qrc             # 图片资源声明
├── build_and_run.bat         # Windows 编译并运行脚本
└── run_client.ps1            # Windows 客户端启动脚本
```

`treeholenetwork.*` 是早期 TCP 树洞方案文件，当前 `.pro` 未接入主流程。

---

## 环境要求

- Windows + Qt 6.x MinGW 套件
- C++17 编译器
- 可用的大模型 API Key
- 登录 / 树洞所需的云端服务，默认地址为 `http://127.0.0.1:8765`

本项目脚本中使用的本机示例路径为：

```text
C:\Qt\6.11.1\mingw_64
C:\Qt\Tools\mingw1310_64\bin
```

如果你的 Qt 安装位置不同，请修改 `build_and_run.bat` 或 `run_client.ps1` 中的路径。

---

## 快速开始

### 方式一：Qt Creator

1. 使用 Qt Creator 打开 `MoodCanvasQt.pro`。
2. 选择 Qt 6 MinGW Kit。
3. 构建 Release 或 Debug。
4. 先启动兼容的云端服务，再运行客户端。
5. 在登录页注册或登录账号。
6. 进入“设置 / 隐私”，配置大模型 API Key、接口地址和模型名称。

### 方式二：命令行构建

```powershell
cd "项目路径\MoodCanvasQt"
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
qmake MoodCanvasQt.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -j4
.\release\MoodCanvasQt.exe
```

也可以在 CMD 中运行：

```cmd
build_and_run.bat
```

### 启动脚本

```powershell
.\run_client.ps1
```

该脚本会先尝试检查 `http://127.0.0.1:8765/api/health`，再启动 `release\MoodCanvasQt.exe`。如果提示找不到可执行文件，请先完成构建。

---

## 大模型配置

在“设置 / 隐私”中填写：

| 配置项 | 默认值 / 说明 |
|--------|---------------|
| API Key | 支持 `sk-` 开头或服务商提供的有效密钥 |
| 接口地址 | `https://api.deepseek.com` |
| 模型名称 | `deepseek-chat` |

也可以通过环境变量提供 Key：

```powershell
$env:DEEPSEEK_API_KEY = "sk-xxxxxx"
# 或
$env:MOODCANVAS_API_KEY = "sk-xxxxxx"
```

大模型返回内容会被解析为结构化 JSON，核心字段包括：

- `emotion`：开心、愤怒、难过、平静、焦虑、疲惫之一
- `emotion_weights`：多维情绪权重
- `valence`：愉悦度，范围 `-1` 到 `1`
- `arousal`：激活度，范围 `0` 到 `1`
- `keywords`：关键词
- `comfort_text`：结合日记内容生成的安慰语
- `cbt_hint`：一句温柔、可执行的小建议
- `colors`：用于心象画布的颜色组

---

## 云端服务约定

当前客户端已经实现云端登录与树洞 REST 客户端，但本次代码包未包含 `server/` 后端目录。若要完整使用登录和匿名树洞，需要另行提供或补齐兼容服务。

客户端默认请求以下接口：

| 方法 | 路径 | 用途 |
|------|------|------|
| `GET` | `/api/health` | 健康检查 |
| `POST` | `/api/auth/register` | 注册账号 |
| `POST` | `/api/auth/login` | 登录账号 |
| `GET` | `/api/treehole/posts` | 获取树洞帖子 |
| `POST` | `/api/treehole/posts` | 发布树洞心象 |
| `POST` | `/api/treehole/posts/{id}/replies` | 回复树洞心象 |

登录成功后，客户端会保存云端返回的 token、用户名和昵称，并在树洞相关请求中使用 `Authorization: Bearer <token>`。

---

## 数据与隐私

| 数据类型 | 存储位置 / 流向 |
|----------|-----------------|
| 日记正文与分析结果 | 本机 Qt 应用数据目录下的 `mood_entries.json` |
| API、云端地址、会话信息 | 本机 Qt 应用数据目录下的 `mood_settings.json` |
| 导出的备份 | 用户在设置页手动选择的 JSON 文件路径 |
| 大模型分析 | 仅在生成心象、周报文字或树洞句子分析时，向配置的 API 服务发送必要文本 |
| 树洞内容 | 发送到所配置的云端服务；树洞只展示昵称、模糊心情、情绪色与回复，不展示原始日记全文 |

MoodCanvasQt 不是医疗诊断工具。项目提供的是情绪记录、可视化回看和轻量化自我调节建议，不替代专业心理咨询或医疗服务。

---

## 使用流程

1. 启动云端服务，并确认 `/api/health` 可访问。
2. 构建并启动客户端。
3. 在登录页注册或登录账号。
4. 在“设置 / 隐私”中配置大模型 API，并点击“测试大模型连接”。
5. 首页写下今天的心情，点击“记录心情并生成心象”。
6. 在结果弹窗中查看分析结果和动态心象，确认后保存。
7. 进入“时间长廊”回看月度记录，或进入“每周报告”查看统计与导出。
8. 已保存心象后，可进入“匿名树洞”分享模糊心情并回应他人。

---

## 常见问题

| 问题 | 处理方式 |
|------|----------|
| 启动后无法登录 | 确认云端服务已启动，且“设置 / 隐私”中的云端地址正确。 |
| 点击生成心象提示未配置 API | 当前版本不再离线分析，请配置有效的大模型 API Key。 |
| 大模型测试失败 | 检查 API Key、接口地址、模型名称、网络连接与 Qt/OpenSSL 环境。 |
| PowerShell 脚本提示路径错误 | 修改脚本中的 Qt / MinGW 安装路径。 |
| 编译时提示 `Permission denied` | 关闭正在运行的 `MoodCanvasQt.exe` 后重新构建。 |
| 今日植物显示“未找到图片” | 确认 `images/` 资源目录与 `resources.qrc` 中声明的图片文件已随项目提交。 |
| 树洞无法发布或刷新 | 确认云端接口可用，并且当前账号已登录且 token 有效。 |

---

## 项目亮点

- 情绪向量到视觉参数的自定义映射：颜色、速度、关键词气泡、光点密度与文字摘要共同组成心象画布。
- 面向中文日记的大模型结构化 Prompt：要求输出可解析 JSON，便于后续统计、可视化和报告生成。
- 温柔但不诊断的反馈语气：安慰语、CBT 小建议、周报文字和树洞回应都避免医疗化判断。
- 本地优先的数据设计：日记默认保存在本机，云端仅承担登录与树洞互动。
- 统一的治愈系桌面 UI：渐变背景、圆角卡片、主题切换、今日植物、月度场景与周报风景。

---

## 许可证

本项目仅供课程学习、展示与交流使用。若项目中包含第三方图片、字体、图标、模型接口或云端服务，请遵守对应资源与服务商的授权协议。

---

MoodCanvasQt · 让每一种情绪，都被温柔地看见。
