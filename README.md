# MoodCanvasQt · 心象日记

程序运行录屏链接：https://disk.pku.edu.cn/link/AA5AA0FEEA42544596AD17DB0FD70DCDB6 提取码：DBNy 有效期限：2026-09-01 20:08
注：这是6月6日展示录制的链接，实际后续又修改了部分功能，具体参考README.md和作业报告.pdf

[![License](https://img.shields.io/badge/license-CC%20BY--NC--SA%204.0-lightgrey)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.11-green)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-blue)](https://isocpp.org/)
[![Windows](https://img.shields.io/badge/platform-Windows-0078d7)]()

> **把文字情绪，转化为可视化的心象风景**  
> 一款基于 Qt Widgets 的桌面端情绪记录与疗愈应用。每天只需写下一段日记，系统便会通过大模型解析情绪，并将其映射为动态的「心象画布」——颜色、流动速度、光点与关键词星座，都随着你的心情一同呼吸。

MoodCanvasQt 不提供诊断，也不强调社交，而是用温和的视觉反馈和私密的本地存储，帮助你看见自己的情绪流动。它适合那些希望以低门槛、有趣的方式记录心情，并在周报与时间长廊中回看自身情绪模式的年轻用户。

---

## ✨ 特性一览

| 模块 | 功能描述 |
|------|----------|
| **今日心象输入** | 支持文字输入、表情选择、情境标签；调用大模型输出情绪类别、愉悦度/激活度、关键词、触发因素、安慰语与 CBT 小建议。 |
| **动态心象画布** | `MoodCanvasWidget` 使用 `QPainter` 自绘渐变背景、色带、极光线、星光点、关键词气泡与日记摘录，随情绪向量实时变化。 |
| **今日植物** | 根据当天最后一条记录自动切换休眠、开花、新叶、卷叶、雨中生长等状态，以温柔的植物语言回应情绪。 |
| **时间长廊** | 月历视图按日期显示心象主色；点击日期查看当天全部记录，并支持删除单条记录。 |
| **每周情绪报告** | 统计本周情绪分布（环形图）、高频关键词、触发因素与愉悦度/激活度趋势，自动生成自然语言小结与下周建议，支持导出 PNG / PDF。 |
| **匿名树洞** | 分享最近一次心象的模糊心情到云端树洞，支持查看实时心象列表与发送回应；树洞内容经过句子级情绪着色，视觉上更温暖。 |
| **设置 / 隐私** | 配置主题（浅色/深色/跟随系统）、大模型 API、云端地址，支持连接测试与本地 JSON 数据导出。 |

---

## 🚀 快速开始

### 环境要求

- Windows 10/11 + Qt 6.x MinGW 套件  
- C++17 编译器  
- 可访问的 OpenAI 兼容大模型 API（如 DeepSeek、智谱等）  

### 构建与运行

#### 方式一：使用 Qt Creator

1. 用 Qt Creator 打开 `MoodCanvasQt.pro`。
2. 选择 Qt 6 MinGW Kit，构建 Release 或 Debug。
3. 先启动兼容的云端服务（如需使用登录/树洞），再运行客户端。
4. 在登录页注册或登录账号。
5. 进入「设置 / 隐私」，配置大模型 API Key、接口地址和模型名称，并测试连接。

#### 方式二：命令行构建（Windows）

```powershell
cd "项目路径\MoodCanvasQt"
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
qmake MoodCanvasQt.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -j4
.\release\MoodCanvasQt.exe
```

项目内附带了便捷脚本 `build_and_run.bat` 和 `run_client.ps1`，请根据你的 Qt 安装路径修改其中的目录。

---

## ⚙️ 大模型配置

在「设置 / 隐私」中填写以下信息：

| 配置项 | 说明 |
|--------|------|
| API Key | 服务商提供的有效密钥（支持 `sk-` 开头） |
| 接口地址 | 例如 `https://api.deepseek.com` |
| 模型名称 | 例如 `deepseek-chat` |

大模型返回的结构化 JSON 包含以下核心字段（用于驱动心象画布与报告）：

```json
{
  "emotion": "开心",
  "emotion_weights": {"开心": 0.7, "平静": 0.2, ...},
  "valence": 0.6,
  "arousal": 0.8,
  "keywords": ["作业", "完成", "轻松"],
  "comfort_text": "今天你完成了重要的一步，可以允许自己慢一点。",
  "cbt_hint": "试着把「我必须完美」换成「我已经尽力了」。",
  "colors": ["#FFD700", "#FF8C00", "#FFA07A"]
}
```

---

## ☁️ 云端服务约定

客户端默认请求以下 REST API（需另行提供后端实现，本项目未包含服务端代码）：

| 方法 | 路径 | 用途 |
|------|------|------|
| `GET` | `/api/health` | 健康检查 |
| `POST` | `/api/auth/register` | 注册 |
| `POST` | `/api/auth/login` | 登录 |
| `GET` | `/api/treehole/posts` | 获取树洞帖子 |
| `POST` | `/api/treehole/posts` | 发布树洞心象 |
| `POST` | `/api/treehole/posts/{id}/replies` | 回复树洞 |

登录成功后，客户端会保存 token，并在后续请求中附加 `Authorization: Bearer <token>`。

---

## 📂 项目结构

```text
MoodCanvasQt/
├── MoodCanvasQt.pro          # qmake 工程
├── main.cpp                  # 程序入口
├── mainwindow.*              # 首页及导航
├── llmclient.*               # 大模型 API 请求与解析
├── datastore.*               # 本地 JSON 存储与设置
├── moodentry.*               # 记录数据结构
├── moodcanvaswidget.*        # 心象画布自绘（QPainter）
├── resultdialog.*            # 结果弹窗
├── timelinewindow.*          # 时间长廊
├── weeklyreportwindow.*      # 周报与导出
├── weeklyscenerywidget.*     # 周报场景
├── plantwidget.*             # 今日植物
├── settingsdialog.*          # 设置/隐私
├── logindialog.*             # 登录/注册
├── userservice.*             # 用户账号逻辑
├── cloudapiclient.*          # 云端 REST 客户端
├── cloudtreehole.*           # 树洞同步与发布
├── treeholeemotion.*         # 树洞句子情绪着色
├── toolboxdialog.*           # 匿名树洞界面
├── thememanager.*            # 浅色/深色/跟随系统
├── resources.qrc             # 资源文件
├── build_and_run.bat         # Windows 编译运行脚本
└── run_client.ps1            # Windows 启动脚本
```

---

## 🔐 数据与隐私

| 数据类型 | 存储位置 / 流向 |
|----------|-----------------|
| 日记正文与分析结果 | 本机 Qt 应用数据目录下的 `mood_entries.json` |
| API、云端地址、会话信息 | 本机 `mood_settings.json` |
| 导出备份 | 用户手动选择路径的 JSON 文件 |
| 大模型分析 | 仅在生成心象、周报文字或树洞分析时发送必要文本 |
| 树洞内容 | 发送至云端服务；只展示昵称、模糊心情与情绪色，不暴露原始日记 |

> **重要提醒**：MoodCanvasQt **不是医疗诊断工具**。所有建议与反馈仅供自我觉察和情绪调节参考，不能替代专业心理咨询或医疗服务。



## 🛠️ 技术栈

- **客户端框架**：Qt 6 Widgets / C++17
- **网络**：Qt Network（OpenAI 兼容 API）
- **异步**：Qt Concurrent / 定时器
- **图形**：`QPainter` 自定义绘制（画布、图表、植物）
- **存储**：本地 JSON（Qt 标准应用数据目录）
- **构建**：qmake + MinGW
- **主题**：动态切换浅色/深色/跟随系统

---

## ❓ 常见问题

| 问题 | 解决方案 |
|------|----------|
| 启动后无法登录 | 确认云端服务已启动，且「设置/隐私」中的云端地址正确。 |
| 生成心象提示“未配置 API” | 当前版本已移除离线模型，必须配置有效的大模型 API Key。 |
| 大模型测试失败 | 检查 API Key、接口地址、模型名称、网络连接及 Qt/OpenSSL 环境。 |
| 编译时提示 `Permission denied` | 关闭正在运行的 `MoodCanvasQt.exe` 后重新构建。 |
| 今日植物显示“未找到图片” | 确认 `images/` 资源目录与 `resources.qrc` 中的图片文件已随项目提交。 |
| 树洞无法刷新或发布 | 检查云端接口可用性及当前账号登录状态（token 是否有效）。 |


## 🙏 致谢

感谢大模型 API 服务商，老师，助教以及所有参与 MoodCanvasQt 设计、开发与测试的小组成员。

---

**心象日记 · 让每一种情绪，都被温柔地看见。**  
欢迎 Star ⭐，也欢迎提出建议与反馈！
