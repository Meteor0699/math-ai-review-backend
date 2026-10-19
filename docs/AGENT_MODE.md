# Agent 模式设计（第一步：题目页常驻 AI 面板）

目标：把「点一次生成一段讲解」升级成**能持续对话、会调用工具、逐字输出**的 agent 面板。

## 1. 范围（第一步）

- 只在**题目详情页**内嵌常驻面板（先替换现有「AI 题目讲解」区）。
- 支持：多轮对话、流式输出、会话持久化（换设备/刷新可回看）、工具调用可视化。
- 不做：全局对话入口、跨题记忆、语音、图片输入。

## 2. 数据库（迁移 `V004__add_agent_conversations.sql`）

```sql
CREATE TABLE ai_conversation (
  id BIGINT NOT NULL AUTO_INCREMENT,
  user_id BIGINT NOT NULL,
  question_id BIGINT NOT NULL,
  message_count INT NOT NULL DEFAULT 0,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_ai_conversation_user_question (user_id, question_id),
  KEY idx_ai_conversation_user_updated (user_id, updated_at),
  CONSTRAINT fk_ai_conversation_user FOREIGN KEY (user_id) REFERENCES user (id) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_ai_conversation_question FOREIGN KEY (question_id) REFERENCES question (id) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE ai_message (
  id BIGINT NOT NULL AUTO_INCREMENT,
  conversation_id BIGINT NOT NULL,
  role ENUM('user','assistant','tool') NOT NULL,
  content MEDIUMTEXT DEFAULT NULL,
  tool_name VARCHAR(64) DEFAULT NULL,
  tool_args TEXT DEFAULT NULL,
  tool_result MEDIUMTEXT DEFAULT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_ai_message_conversation (conversation_id, id),
  CONSTRAINT fk_ai_message_conversation FOREIGN KEY (conversation_id) REFERENCES ai_conversation (id) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

约束：迁移必须幂等（`information_schema` + `PREPARE` 模式，见 `V002`），不得写死 `USE`，新库由 `schema.sql` 同步补齐。

## 3. HTTP 接口（都需 `UserAuthFilter`）

### `GET /api/v1/questions/{id}/agent/session`

返回该用户在该题上的会话与历史。不存在时返回空会话（不落库，首次发言才创建）。

```json
{ "code": 200, "message": "success", "data": {
  "conversationId": 12,
  "messages": [
    { "id": 1, "role": "user", "content": "这题为什么不能用洛必达？", "createdAt": "2026-10-13 16:00:00" },
    { "id": 2, "role": "tool", "toolName": "get_question_answer", "toolArgs": {...}, "toolResult": {...}, "createdAt": "..." },
    { "id": 3, "role": "assistant", "content": "...", "createdAt": "..." }
  ]
} }
```

### `POST /api/v1/questions/{id}/agent/chat` → `text/event-stream`

请求体：`{ "message": "学生这一轮说的话", "conversationId": 12 }`（`conversationId` 可省，服务端按 user+question 复用/创建）。

事件（每条 `event: <name>` + `data: <json>`，UTF-8，`\n\n` 结尾）：

| event | data | 说明 |
|---|---|---|
| `meta` | `{"conversationId":12,"userMessageId":34,"assistantMessageId":35}` | 首帧 |
| `delta` | `{"text":"洛必达需要"}` | 增量文本，可多次 |
| `tool` | `{"name":"search_questions","status":"running","args":{...}}` | 工具开始 |
| `tool` | `{"name":"search_questions","status":"done","result":{...},"ms":123}` | 工具结束（`result` 截断到 4KB） |
| `done` | `{"assistantMessageId":35,"usage":{"promptTokens":0,"completionTokens":0}}` | 正常结束 |
| `error` | `{"message":"...","code":"ai_unavailable"}` | 出错，随后连接关闭 |

要求：
- 响应头：`Content-Type: text/event-stream; charset=utf-8`、`Cache-Control: no-cache`、`X-Accel-Buffering: no`。
- 客户端断开时停止生成、把已生成内容落库（`role=assistant`），不抛异常。
- 并发上限沿用现有 `AiExecutor`（`AI_MAX_CONCURRENCY`）；队列满时返回 `event: error` + `code: busy`。
- 落库：`user` 消息先落，工具调用落 `role=tool`，最终 assistant 文本落 `role=assistant`。

## 4. 工具（模型可调用）

所有工具在该轮对话的 **orchestrator 线程**内同步执行（MySQL 同步客户端），不占用 `AiExecutor` 的模型并发位；必须只读、限流、结果截断。

| 名称 | 参数 | 返回（截断后） |
|---|---|---|
| `search_questions` | `keyword?, courseId?, chapterId?, difficulty?, questionType?, limit?`(≤20) | `{items:[{id,title,questionType,difficulty,courseName}], total}`，**不含答案** |
| `get_question_answer` | `questionId`(必填) | `{standardAnswer, normalExplanation}` |
| `list_knowledge_points` | `courseId?, chapterId?, limit?`(≤20) | `{items:[{id,title,chapterName,examFrequency}]}` |
| `list_my_wrong_questions` | `limit?`(≤20) | `{items:[{questionId,title,courseName,addedAt}]}`，只查当前登录用户 |
| `list_my_study_records` | `limit?`(≤20) | `{items:[{questionId,title,status,lastAttemptAt,attemptCount}]}`，只查当前登录用户 |
| `build_practice_set` | `courseId?, chapterId?, difficulty?, count?`(≤10) | `{items:[{id,title,difficulty,courseName}]}`，抽题不复用答案 |

安全：工具不接受 `userId` 参数（由服务端注入当前用户）；所有 SQL 走 `mathai::utils::mysql::quote`；路径/参数校验沿用 `utils/RequestValidation.h`。

## 5. 前端（`components/AgentPanel.vue`，嵌入题目详情页）

- 消息流：用户气泡 / assistant 正文（Markdown+KaTeX 走现有 `FormulaContent`）/ 工具调用卡片（名称 + 运行中/完成 + 结果摘要，可折叠）。
- 流式：`fetch` + `ReadableStream` 解析上面的 SSE，逐字落字；「停止」按钮用 `AbortController`。
- 状态：首帧未到 → 「思考中」；工具运行 → 卡片转圈；`done` → 结束；`error` → 失败提示 + 重试。
- 持久化：进入页面先 `GET .../agent/session` 恢复历史；发送后本地追加。
- 无障碍/响应式：输入框 `aria-label`，面板在 <980px 单列，沿用现有设计令牌（`--color-*`/`--space-*`）。
- 旧的「AI 题目讲解」一次性接口保留（缓存讲解仍有价值），面板作为新的主入口。

## 6. 架构约束（重要）

- 模型调用必须提交到 `AiExecutor`，**不得阻塞 Drogon IO 线程**；但 `AiExecutor` worker 只允许记录本轮结果并唤醒，**不得在 worker 上执行工具或等待下一轮**——否则 N 路并发工具轮会占满全部 worker 并自锁（每轮都在等排在等待者后面的请求）。
- 每轮的工具执行与续跑放在该轮独占的 **orchestrator 线程**（`generateTurn` 起的 detached thread）上，由它循环驱动，最多 5 轮；轮次预算用尽仍有工具待执行时直接收尾。
- 向 `ResponseStreamPtr` 写数据只能在事件循环线程做：任意工作线程用 `drogon::app().getLoop()->queueInLoop(...)` 投递。
- SSE 连接是长连接：为每帧设置合理刷新，避免占用线程；`number_of_threads` 目前为 4，长连接数量需有上限（同一用户同一题同时只允许 1 条流）。
