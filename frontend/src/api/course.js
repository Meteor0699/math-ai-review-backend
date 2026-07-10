import request from './request'

export function getCourseList(params) {
  return request({
    url: '/courses',
    method: 'get',
    params
  })
}

export function getCourseDetail(courseId) {
  return request({
    url: `/courses/${courseId}`,
    method: 'get'
  })
}

export function getChapters(courseId, params) {
  return request({
    url: `/courses/${courseId}/chapters`,
    method: 'get',
    params
  })
}

export function getKnowledgePoints(chapterId, params) {
  return request({
    url: `/chapters/${chapterId}/knowledge-points`,
    method: 'get',
    params
  })
}

export function getQuestionList(params) {
  return request({
    url: '/questions',
    method: 'get',
    params
  })
}

export function getQuestionDetail(questionId) {
  return request({
    url: `/questions/${questionId}`,
    method: 'get'
  })
}

export function getQuestionAnswer(questionId) {
  return request({
    url: `/questions/${questionId}/answer`,
    method: 'get'
  })
}

export function getAiExplanation(questionId) {
  return request({
    url: `/questions/${questionId}/ai-explanation`,
    method: 'post'
  })
}

export function askAiFollowUp(questionId, question) {
  return request({
    url: `/questions/${questionId}/ai-follow-up`,
    method: 'post',
    data: { question }
  })
}

export function getStudyState(questionId) {
  return request({
    url: `/questions/${questionId}/study-state`,
    method: 'get'
  })
}

export function recordQuestionStudy(questionId, data = {}) {
  return request({
    url: `/questions/${questionId}/study-record`,
    method: 'post',
    data
  })
}

export function addWrongQuestion(questionId, data = {}) {
  return request({
    url: `/questions/${questionId}/wrong-book`,
    method: 'post',
    data
  })
}

export function removeWrongQuestion(questionId) {
  return request({
    url: `/questions/${questionId}/wrong-book`,
    method: 'delete'
  })
}

export function getQuestionRecords(params) {
  return request({
    url: '/my/question-records',
    method: 'get',
    params
  })
}

export function getWrongQuestions(params) {
  return request({
    url: '/my/wrong-questions',
    method: 'get',
    params
  })
}

export function getPaperList(params) {
  return request({
    url: '/papers',
    method: 'get',
    params
  })
}

export function getMyPaperList(params) {
  return request({
    url: '/my/papers',
    method: 'get',
    params
  })
}

export function uploadMyPaper(formData) {
  return request({
    url: '/my/papers/upload',
    method: 'post',
    data: formData,
    timeout: 120000,
    headers: { 'Content-Type': 'multipart/form-data' }
  })
}
