import request from './request'

export function login(username, password) {
  return request({
    url: '/auth/login',
    method: 'post',
    data: { username, password }
  })
}

export function register(data) {
  return request({
    url: '/auth/register',
    method: 'post',
    data
  })
}
