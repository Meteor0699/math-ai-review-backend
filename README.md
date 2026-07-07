# 数学 AI 复习助手

高校数学课程 AI 辅助复习 Web 平台，面向全校在校生，提供课程知识结构化梳理、题库练习、AI 智能讲解和往年试题管理等一站式复习服务。

## 项目简介

数学 AI 复习助手旨在帮助高校学生高效复习数学公共基础课程。平台将课程内容按章节和重点知识进行结构化组织，提供丰富的题库资源，并接入大模型实现 AI 智能题目讲解，让学生能够随时获得个性化学习指导。

第一版覆盖三门数学公共基础课程：**高等数学**、**线性代数**、**概率论与数理统计**。

## 功能介绍

### 学生端

| 功能模块 | 说明 |
|----------|------|
| 用户登录 | 支持学生和管理员两种角色登录，JWT 鉴权 |
| 课程浏览 | 展示课程列表，查看课程详情 |
| 章节浏览 | 按课程查看章节结构，逐层深入 |
| 重点知识 | 每章展示重点知识，包括核心内容、常用公式、高频题型、易错点、复习建议 |
| 题库练习 | 按课程、章节、题型、难度筛选题目 |
| 题目详情 | 查看题干、标准答案、普通解析 |
| AI 讲解 | 调用大模型 API 生成详细解题讲解，首次生成后缓存 |
| 往年试题 | 浏览和下载往年考试试卷 |

### 管理员端

| 功能模块 | 说明 |
|----------|------|
| 课程管理 | 课程的新增、编辑、删除（逻辑删除） |
| 章节管理 | 章节的新增、编辑、删除，支持按课程筛选 |
| 重点知识管理 | 知识点的新增、编辑、删除，含考核频率标注 |
| 题库管理 | 支持选择题、填空题、计算题、证明题、判断题的录入和维护 |
| 试卷管理 | 试卷信息管理及文件上传 |
| 用户管理 | 用户列表查看（预留） |

## 技术栈

| 层级 | 技术 |
|------|------|
| 前端 | Vue 3 + Vite + Element Plus + Vue Router + Pinia + Axios |
| 后端 | C++17 + Drogon（HTTP 框架） |
| 数据库 | MySQL 8.0 |
| 鉴权 | 自研 JWT（HMAC-SHA256） |
| 密码 | SHA256 + Salt 哈希 |
| AI | 阿里云百炼（兼容 OpenAI Chat Completions 协议） |
| 文件存储 | 服务器本地目录，数据库保存相对路径 |

## 项目目录结构

```
math-ai-review-backend/
├── main.cc                          # 后端入口
├── CMakeLists.txt                   # CMake 构建配置
├── config.json                      # 后端配置（数据库、JWT、上传目录等）
├── .env.example                     # AI 环境变量模板
├── .gitignore
│
├── controllers/                     # 控制器层（HTTP 路由处理）
│   ├── AuthController.cc/h          # 登录认证
│   ├── CourseController.cc/h        # 课程
│   ├── ChapterController.cc/h       # 章节
│   ├── KnowledgeController.cc/h     # 重点知识
│   ├── QuestionController.cc/h      # 题库
│   ├── PaperController.cc/h         # 试卷管理
│   └── AiController.cc/h            # AI 讲解
│
├── services/                        # 服务层（业务逻辑）
│   ├── AuthService.cc/h
│   ├── CourseService.cc/h
│   ├── ChapterService.cc/h
│   ├── KnowledgeService.cc/h
│   ├── QuestionService.cc/h
│   ├── AiService.cc/h
│   └── ServicePlaceholders.h
│
├── dao/                             # 数据访问层（SQL 操作）
│   ├── UserDao.cc/h
│   ├── CourseDao.cc/h
│   ├── ChapterDao.cc/h
│   ├── KnowledgeDao.cc/h
│   ├── QuestionDao.cc/h
│   └── AiExplanationDao.cc/h
│
├── models/                          # 数据模型
│   ├── User.h
│   └── ModelPlaceholders.h
│
├── clients/                         # 外部 API 客户端
│   └── AiClient.cc/h                # 大模型 API 调用
│
├── filters/                         # 中间件/过滤器
│   └── AdminAuthFilter.cc/h         # 管理员权限校验
│
├── utils/                           # 工具类
│   ├── JwtUtil.cc/h                 # JWT 生成与验证
│   ├── PasswordUtil.cc/h            # 密码哈希
│   ├── JsonResponse.h               # 统一 JSON 响应
│   ├── EnvLoader.cc/h               # 环境变量加载
│   ├── ErrorHandler.cc/h            # 错误处理
│   └── DbClientProvider.h           # 数据库客户端提供者
│
├── database/                        # 数据库脚本
│   ├── schema.sql                   # 建表语句
│   └── test_data.sql                # 测试数据
│
├── scripts/                         # 辅助脚本
│   ├── build-msys.bat / .ps1        # 编译脚本
│   ├── run-msys.bat / .ps1          # 启动脚本
│   └── init-db.bat / .ps1           # 数据库初始化脚本
│
└── frontend/                        # 前端工程
    ├── package.json
    ├── vite.config.js
    ├── index.html
    └── src/
        ├── main.js                  # 前端入口
        ├── App.vue
        ├── api/                     # API 封装
        │   ├── request.js           # Axios 实例（拦截器、错误处理）
        │   ├── auth.js              # 认证接口
        │   ├── course.js            # 课程接口
        │   └── admin.js             # 管理后台接口
        ├── stores/
        │   └── user.js              # Pinia 用户状态管理
        ├── router/
        │   └── index.js             # 路由配置 + 导航守卫
        ├── layouts/
        │   ├── StudentLayout.vue    # 学生端布局
        │   └── AdminLayout.vue      # 管理员端布局
        └── views/
            ├── Login.vue            # 登录页
            ├── student/             # 学生端页面
            │   ├── Home.vue         # 首页
            │   ├── CourseList.vue   # 课程列表
            │   ├── CourseDetail.vue # 课程详情（含章节列表）
            │   ├── ChapterDetail.vue# 章节详情（含知识点列表）
            │   ├── KnowledgeDetail.vue  # 知识点详情
            │   ├── KnowledgeList.vue    # 知识点列表
            │   ├── QuestionList.vue     # 题库列表
            │   ├── QuestionDetail.vue   # 题目详情（答案、解析、AI 讲解）
            │   └── PaperList.vue        # 往年试卷列表
            └── admin/               # 管理员端页面
                ├── AdminHome.vue    # 后台首页
                ├── CourseManage.vue # 课程管理
                ├── ChapterManage.vue# 章节管理
                ├── KnowledgeManage.vue # 知识点管理
                ├── QuestionManage.vue  # 题库管理
                ├── PaperManage.vue     # 试卷管理
                └── UserManage.vue      # 用户管理
```

## 数据库初始化

### 1. 创建数据库与表结构

```powershell
mysql -u root -p < "D:\solo proj\math-ai-review-backend\database\schema.sql"
```

### 2. 导入测试数据

```powershell
mysql -u root -p < "D:\solo proj\math-ai-review-backend\database\test_data.sql"
```

测试数据包含：
- 3 个用户（admin / student / student2）
- 3 门课程（高等数学 / 线性代数 / 概率论与数理统计）
- 10 个章节
- 7 个重点知识点
- 9 道题目

### 3. 数据库表结构概览

| 表名 | 说明 |
|------|------|
| `user` | 用户表（含角色和状态） |
| `course` | 课程表 |
| `chapter` | 章节表（关联课程，支持父子层级） |
| `knowledge_point` | 重点知识表（含核心内容、公式、高频题型、易错点、复习建议） |
| `question` | 题库表（含题干、答案、解析、来源、年份） |
| `question_option` | 选择题选项表 |
| `exam_paper` | 试卷表（含文件路径） |
| `ai_explanation` | AI 讲解缓存表 |

## 后端启动方法

### 环境要求

- C++17 编译器（GCC 13+ / MSVC 2022+）
- CMake 3.16+
- MySQL 8.0+
- Drogon 框架
- OpenSSL
- jsoncpp

### 1. 安装依赖（Windows + MSYS2/UCRT64）

```powershell
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
pacman -S mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-drogon
pacman -S mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-jsoncpp
```

### 2. 配置数据库连接

编辑 `config.json`，修改 `db_clients` 中的数据库连接信息：

```json
"db_clients": [
  {
    "host": "127.0.0.1",
    "port": 3306,
    "dbname": "math_ai_review",
    "user": "root",
    "passwd": "你的MySQL密码"
  }
]
```

同时修改 `custom_config` 中的 JWT 密钥：

```json
"custom_config": {
  "jwt_secret": "请替换为随机字符串",
  "jwt_expire_hours": 24,
  "upload_dir": "./uploads/papers"
}
```

### 3. 编译

```powershell
cd "D:\solo proj\math-ai-review-backend"
.\scripts\build-msys.bat
```

或手动编译：

```powershell
mkdir build-msys
cd build-msys
cmake .. -G "MSYS Makefiles"
make -j4
```

### 4. 启动

```powershell
.\scripts\run-msys.bat
```

或手动启动：

```powershell
cd "D:\solo proj\math-ai-review-backend\build-msys"
./math-ai-review-backend.exe
```

后端默认运行在 `http://localhost:8080`。

## 前端启动方法

### 环境要求

- Node.js 18+

### 1. 安装依赖

```powershell
cd "D:\solo proj\math-ai-review-backend\frontend"
npm install
```

### 2. 启动开发服务器

```powershell
npm run dev
```

前端默认运行在 `http://localhost:5173`，API 请求通过 Vite 代理自动转发到后端 `http://localhost:8080`。

### 3. 生产构建

```powershell
npm run build
```

构建产物输出到 `frontend/dist/` 目录。

## 环境变量配置

AI 讲解功能需要配置大模型 API 密钥。从 `.env.example` 复制并编辑：

```powershell
cd "D:\solo proj\math-ai-review-backend"
copy .env.example .env
```

编辑 `.env` 文件：

```env
# OpenAI-compatible chat completions endpoint
# 阿里云百炼兼容模式示例
AI_API_BASE_URL=https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
AI_API_KEY=你的阿里云百炼API Key
AI_MODEL=qwen-plus
```

> **注意**：`.env` 文件已在 `.gitignore` 中排除，请勿将真实 API Key 提交到仓库。

## 测试账号

| 用户名 | 密码 | 角色 | 说明 |
|--------|------|------|------|
| `admin` | `123456` | 管理员 | 可访问管理后台 `/admin` |
| `student` | `123456` | 学生 | 可访问学生端全部功能 |
| `student2` | `123456` | 学生 | 可访问学生端全部功能 |

## API 接口说明

### 统一响应格式

所有接口返回统一的 JSON 格式：

```json
{
  "code": 200,
  "message": "success",
  "data": {}
}
```

### 认证接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/auth/login` | 用户登录，返回 token 和用户信息 |
| GET | `/api/v1/auth/me` | 获取当前用户信息（需 Bearer Token） |

### 学生端接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/courses` | 获取课程列表 |
| GET | `/api/courses/{courseId}` | 获取课程详情 |
| GET | `/api/courses/{courseId}/chapters` | 获取课程章节列表 |
| GET | `/api/chapters/{chapterId}` | 获取章节详情 |
| GET | `/api/chapters/{chapterId}/knowledge-points` | 获取章节重点知识 |
| GET | `/api/knowledge-points/{knowledgeId}` | 获取知识点详情 |
| GET | `/api/questions` | 获取题库列表（支持筛选） |
| GET | `/api/questions/{questionId}` | 获取题目详情 |
| GET | `/api/questions/{questionId}/answer` | 获取题目答案和解析 |
| POST | `/api/questions/{questionId}/ai-explanation` | 获取 AI 讲解 |
| GET | `/api/papers` | 获取试卷列表 |
| GET | `/api/papers/{paperId}` | 获取试卷详情 |

### 管理员端接口（均需 Bearer Token + admin 角色）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/courses` | 新增课程 |
| PUT | `/api/admin/courses/{id}` | 编辑课程 |
| DELETE | `/api/admin/courses/{id}` | 删除课程（逻辑删除） |
| POST | `/api/admin/chapters` | 新增章节 |
| PUT | `/api/admin/chapters/{id}` | 编辑章节 |
| DELETE | `/api/admin/chapters/{id}` | 删除章节（逻辑删除） |
| POST | `/api/admin/knowledge-points` | 新增重点知识 |
| PUT | `/api/admin/knowledge-points/{id}` | 编辑重点知识 |
| DELETE | `/api/admin/knowledge-points/{id}` | 删除重点知识（逻辑删除） |
| POST | `/api/admin/questions` | 新增题目 |
| PUT | `/api/admin/questions/{id}` | 编辑题目 |
| DELETE | `/api/admin/questions/{id}` | 删除题目（逻辑删除） |
| POST | `/api/admin/questions/{id}/options` | 新增题目选项 |
| PUT | `/api/admin/questions/{id}/options` | 替换题目全部选项 |
| DELETE | `/api/admin/questions/{id}/options/{optionId}` | 删除题目选项 |
| POST | `/api/admin/papers/upload` | 上传试卷文件 |
| PUT | `/api/admin/papers/{id}` | 编辑试卷信息 |
| DELETE | `/api/admin/papers/{id}` | 删除试卷（逻辑删除） |

### 题库筛选参数

`GET /api/questions` 支持以下 query 参数：

| 参数 | 类型 | 说明 |
|------|------|------|
| `courseId` | int | 按课程筛选 |
| `chapterId` | int | 按章节筛选 |
| `questionType` | string | 题型：`single_choice` / `blank` / `calculation` / `proof` / `judge` |
| `difficulty` | string | 难度：`basic` / `medium` / `hard` |
| `source` | string | 按来源筛选 |
| `year` | int | 按年份筛选 |
| `keyword` | string | 关键词搜索 |
| `page` | int | 页码（默认 1） |
| `pageSize` | int | 每页数量（默认 10） |

### 登录示例

```bash
curl -X POST "http://localhost:8080/api/auth/login" \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"admin\",\"password\":\"123456\"}"
```

返回：

```json
{
  "code": 200,
  "message": "success",
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "user": {
      "id": 1,
      "username": "admin",
      "realName": "管理员",
      "role": "admin"
    }
  }
}
```

### 管理员接口调用示例

```bash
# 新增课程
curl -X POST "http://localhost:8080/api/admin/courses" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"name\":\"测试课程\",\"code\":\"test\",\"description\":\"测试\",\"sortOrder\":10,\"status\":1}"

# 上传试卷
curl -X POST "http://localhost:8080/api/admin/papers/upload" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -F "file=@/path/to/exam.pdf" \
  -F "title=2024年期末考试" \
  -F "courseId=1" \
  -F "examYear=2024"
```

## 第一版功能边界

### 已实现

- 用户登录（JWT 鉴权 + 角色区分）
- 管理员后台（课程、章节、重点知识、题库、试卷的 CRUD）
- 课程展示（课程列表 + 课程详情）
- 章节展示（章节列表 + 章节详情）
- 重点知识展示（核心内容、常用公式、高频题型、易错点、复习建议）
- 题库展示（按课程/章节/题型/难度筛选 + 分页）
- 题目详情展示（题干 + 选项 + 标准答案 + 普通解析）
- AI 题目讲解（接入大模型 + 结果缓存）
- 往年试题文件管理（上传 + 下载 + 信息维护）

### 第一版暂不做

- 在线提交答案
- 自动判分
- 错题本
- 拍照搜题
- OCR 自动拆题
- 小程序端
- 企业微信端
- 排行榜
- 班级管理
- 教师端

## 后期扩展计划

### 功能扩展

- **在线作答与自动判分**：学生在线提交答案，系统自动批改并给出分数
- **错题本**：自动收集错题，支持错题重做和针对性复习
- **拍照搜题**：上传题目图片，OCR 识别后匹配题库
- **OCR 自动拆题**：上传试卷图片，自动识别并拆分为题库条目
- **排行榜**：按课程展示学习进度和答题正确率排行
- **班级管理**：支持按班级组织学生，批量导入学生账号
- **教师端**：教师查看班级学习数据，发布复习任务

### 多端扩展

- **小程序端**：基于 Taro 框架开发微信小程序，复用现有 API
- **企业微信接入**：通过企业微信工作台集成，方便师生使用
- **统一身份认证**：对接学校统一身份认证系统（CAS/OAuth），实现单点登录

### 课程扩展

- 支持添加更多课程（如离散数学、数值分析等）
- 题库持续扩充，支持社区贡献

### 技术优化

- 前端性能优化（懒加载、虚拟滚动）
- 后端引入 Redis 缓存，提升高频查询性能
- 数据库读写分离，支撑更大并发
- 容器化部署（Docker + Docker Compose）
- CI/CD 自动化构建与部署