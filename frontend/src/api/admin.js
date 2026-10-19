import request from './request'

export function adminGetStats() {
  return request({ url: '/admin/stats', method: 'get' })
}

// ========== 课程管理 ==========
export function adminGetCourseList(params) {
  return request({ url: '/admin/courses', method: 'get', params })
}
export function adminGetCourseDetail(courseId) {
  return request({ url: `/admin/courses/${courseId}`, method: 'get' })
}
export function adminCreateCourse(data) {
  return request({ url: '/admin/courses', method: 'post', data })
}
export function adminUpdateCourse(courseId, data) {
  return request({ url: `/admin/courses/${courseId}`, method: 'put', data })
}
export function adminDeleteCourse(courseId) {
  return request({ url: `/admin/courses/${courseId}`, method: 'delete' })
}

// ========== 章节管理 ==========
export function adminGetChapterList(params) {
  return request({ url: '/admin/chapters', method: 'get', params })
}
export function adminGetChapterDetail(chapterId) {
  return request({ url: `/admin/chapters/${chapterId}`, method: 'get' })
}
export function adminCreateChapter(data) {
  return request({ url: '/admin/chapters', method: 'post', data })
}
export function adminUpdateChapter(chapterId, data) {
  return request({ url: `/admin/chapters/${chapterId}`, method: 'put', data })
}
export function adminDeleteChapter(chapterId) {
  return request({ url: `/admin/chapters/${chapterId}`, method: 'delete' })
}

// ========== 知识点管理 ==========
export function adminGetKnowledgeList(params) {
  return request({ url: '/admin/knowledge-points', method: 'get', params })
}
export function adminGetKnowledgeDetail(knowledgeId) {
  return request({ url: `/admin/knowledge-points/${knowledgeId}`, method: 'get' })
}
export function adminCreateKnowledge(data) {
  return request({ url: '/admin/knowledge-points', method: 'post', data })
}
export function adminUpdateKnowledge(knowledgeId, data) {
  return request({ url: `/admin/knowledge-points/${knowledgeId}`, method: 'put', data })
}
export function adminDeleteKnowledge(knowledgeId) {
  return request({ url: `/admin/knowledge-points/${knowledgeId}`, method: 'delete' })
}

// ========== 题库管理 ==========
export function adminGetQuestionList(params) {
  return request({ url: '/admin/questions', method: 'get', params })
}
export function adminGetQuestionDetail(questionId) {
  return request({ url: `/admin/questions/${questionId}`, method: 'get' })
}
export function adminCreateQuestion(data) {
  return request({ url: '/admin/questions', method: 'post', data })
}
export function adminUpdateQuestion(questionId, data) {
  return request({ url: `/admin/questions/${questionId}`, method: 'put', data })
}
export function adminDeleteQuestion(questionId) {
  return request({ url: `/admin/questions/${questionId}`, method: 'delete' })
}

// ========== 试卷管理 ==========
export function adminGetPaperList(params) {
  return request({ url: '/admin/papers', method: 'get', params })
}
export function adminGetPaperDetail(paperId) {
  return request({ url: `/admin/papers/${paperId}`, method: 'get' })
}
export function adminUploadPaper(formData) {
  return request({
    url: '/admin/papers/upload',
    method: 'post',
    data: formData,
    headers: { 'Content-Type': 'multipart/form-data' }
  })
}
export function adminUpdatePaper(paperId, data) {
  return request({ url: `/admin/papers/${paperId}`, method: 'put', data })
}
export function adminDeletePaper(paperId) {
  return request({ url: `/admin/papers/${paperId}`, method: 'delete' })
}

// ========== 用户管理 ==========
export function adminGetUserList(params) {
  return request({ url: '/admin/users', method: 'get', params })
}
export function adminCreateUser(data) {
  return request({ url: '/admin/users', method: 'post', data })
}
export function adminUpdateUser(userId, data) {
  return request({ url: `/admin/users/${userId}`, method: 'put', data })
}
export function adminDeleteUser(userId) {
  return request({ url: `/admin/users/${userId}`, method: 'delete' })
}
export function adminResetUserPassword(userId, password) {
  return request({ url: `/admin/users/${userId}/password`, method: 'put', data: { password } })
}
