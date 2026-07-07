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

export function getPaperList(params) {
  return request({
    url: '/papers',
    method: 'get',
    params
  })
}
