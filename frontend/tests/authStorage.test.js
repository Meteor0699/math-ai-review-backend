import assert from 'node:assert/strict'
import test from 'node:test'

import {
  clearAuthStorage,
  readToken,
  readUser,
  writeAuthStorage
} from '../src/utils/authStorage.js'

class MemoryStorage {
  constructor() {
    this.values = new Map()
  }

  getItem(key) {
    return this.values.has(key) ? this.values.get(key) : null
  }

  setItem(key, value) {
    this.values.set(key, String(value))
  }

  removeItem(key) {
    this.values.delete(key)
  }
}

test.beforeEach(() => {
  globalThis.localStorage = new MemoryStorage()
})

test('persists and restores authentication state', () => {
  writeAuthStorage('token-value', { id: 7, role: 'student' })
  assert.equal(readToken(), 'token-value')
  assert.deepEqual(readUser(), { id: 7, role: 'student' })
})

test('clears malformed user state instead of throwing', () => {
  localStorage.setItem('token', 'stale-token')
  localStorage.setItem('user', '{broken json')
  assert.equal(readUser(), null)
  assert.equal(readToken(), '')
})

test('clears authentication state explicitly', () => {
  writeAuthStorage('token-value', { role: 'admin' })
  clearAuthStorage()
  assert.equal(readToken(), '')
  assert.equal(readUser(), null)
})
