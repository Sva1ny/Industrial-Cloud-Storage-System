# ☁️ Industrial Cloud Storage System

企业级云盘存储系统，基于 C++ 微服务架构，支持文件上传下载、在线预览、全文检索、分享与回收站管理，覆盖 **Qt6 桌面客户端** 和 **Web 端** 双端访问。

## 架构概览

```
┌──────────────────────────┐
│   Qt6 桌面客户端          │
│  (macOS / C++17 / Qt6)   │
└─────────┬────────────────┘
          │ HTTP / RPC
┌─────────▼────────────────┐
│    wfrest API 网关         │
│  (RESTful HTTP 接口层)      │
└─────────┬────────────────┘
          │ srpc RPC
┌─────────▼────────────────┐
│     微服务集群              │
│  ┌──────┬──────┬──────┐   │
│  │用户  │文件  │搜索  │   │
│  │服务  │服务  │服务  │   │
│  └──┬───┴──┬───┴──┬───┘   │
│     │      │      │        │
│  Consul 服务注册与发现       │
└─────────┬────────────────┘
          │
┌─────────▼────────────────┐
│     MySQL · Aliyun OSS    │
│  (元数据)  (对象存储)       │
│     RabbitMQ · 异步任务     │
│  (消息队列 · 索引构建)       │
└───────────────────────────┘
```

## 技术栈

| 层 | 技术 |
|---|---|
| **语言** | C++17, JavaScript |
| **RPC 框架** | Sogou srpc (基于 Workflow) |
| **HTTP 框架** | wfrest |
| **序列化** | Protobuf |
| **客户端** | Qt6 (Widgets, Network, Multimedia) |
| **数据库** | MySQL |
| **对象存储** | Aliyun OSS SDK |
| **消息队列** | RabbitMQ (SimpleAmqpClient) |
| **服务发现** | Consul (ppconsul) |
| **搜索** | 自研倒排索引 + Jieba 中文分词 |
| **认证** | JWT |
| **服务端工具** | TinyXML2, nlohmann/json, OpenSSL |

## 功能特性

### 文件管理
- 文件上传 / 下载（支持断点续传）
- 目录树导航 + 面包屑路径
- 网格 / 列表双视图切换
- 多选批量操作（批量下载、删除、分享）
- 拖拽上传
- 新建文件夹

### 文件分类
- 全部 / 最近 / 文档 / 图片 / 视频 分类筛选
- 收藏夹
- 回收站

### 搜索
- 按文件名搜索
- **全文搜索**：自研倒排索引引擎，支持 PDF、Office 文档、TXT 等格式的内容检索
- 搜索关键词高亮 + 片段预览
- Jieba 中文分词

### 文件预览（Qt 客户端）
- 图片预览 (JPG, PNG, GIF, WebP)
- 视频预览 (macOS AVKit)
- PDF 预览
- 文本预览
- 文件历史版本

### 安全
- JWT 令牌认证
- SQL 注入防护
- OSS 访问凭证隔离

## 项目结构

```
├── server          # 服务端主程序入口
├── client/         # Qt6 桌面客户端
│   ├── src/
│   │   ├── pages/          # 页面 (登录/注册/首页/分享/传输列表/回收站)
│   │   ├── preview/        # 文件预览 (图片/视频/PDF/文本)
│   │   ├── network/        # 网络请求管理
│   │   ├── models/         # 数据模型
│   │   └── styles/         # 主题管理
│   └── CMakeLists.txt
├── backend/        # 后端微服务
│   ├── src/
│   │   ├── handler/        # 请求处理 (认证/文件/搜索/分享/收藏/回收站)
│   │   ├── service/        # 服务抽象层
│   │   ├── search/         # 倒排索引 + 分词 + 文本提取
│   │   ├── model/          # 数据模型
│   │   ├── middleware/     # 中间件 (认证)
│   │   └── util/           # 工具类
│   └── sql/                # 数据库迁移脚本
├── static/         # 静态 Web 前端 (Bootstrap)
├── web/            # Vanilla JS SPA 前端
├── lib/            # 第三方依赖库
└── docker-compose.yml
```

## 快速开始

### 环境要求
- C++17 编译器 (Clang / GCC)
- CMake ≥ 3.15
- Qt6 (Widgets, Network, Multimedia)
- MySQL
- RabbitMQ
- Consul
- 依赖库: workflow, srpc, wfrest, protobuf, ppconsul, SimpleAmqpClient, Aliyun OSS SDK, Jieba, TinyXML2

### 编译服务端

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 编译客户端

```bash
cd client && mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 配置

配置文件 `Config.h` 已加入 `.gitignore`，请通过环境变量设置：

```bash
export OSS_ENDPOINT=oss-cn-shanghai.aliyuncs.com
export OSS_ACCESS_KEY_ID=your_key
export OSS_ACCESS_KEY_SECRET=your_secret
export OSS_BUCKET=your_bucket
export MYSQL_URL=mysql://root:password@localhost/cloud_disk
export RABBITMQ_URL=amqp://guest:guest@localhost:5672/%2f
export JWT_SECRET=your_secret
```

### Docker 部署

```bash
docker-compose up -d
```
