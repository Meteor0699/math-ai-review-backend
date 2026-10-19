import { onBeforeUnmount, ref } from 'vue'

/**
 * 默认断点：1400px。
 *
 * 管理端最宽的一张表（题库管理）需要约 1060px 列宽，而管理端内容区在
 * 视口宽度 W 下只有 W - 232px（侧边栏）- 56px（左右内边距）- 滚动条 的
 * 可用宽度，即从约 1400px 起完整列集才放得下；低于该宽度就需要收起
 * 低优先级列并取消操作列固定，否则表头/表体会横向溢出。
 */
export const COMPACT_QUERY = '(max-width: 1400px)'

/**
 * 跟踪一个媒体查询，返回响应式布尔 ref。
 *
 * - 传入 query（默认 1400px 紧凑断点），命中时返回 true；
 * - 监听 matchMedia 的 change 事件实时更新；
 * - 组件卸载（onBeforeUnmount）时移除监听，避免内存泄漏；
 * - SSR 安全：无 window / matchMedia 时始终返回 false。
 *
 * @param {string} query 媒体查询字符串
 * @returns {import('vue').Ref<boolean>}
 */
export function useIsCompact(query = COMPACT_QUERY) {
  const isCompact = ref(false)

  if (typeof window === 'undefined' || typeof window.matchMedia !== 'function') {
    return isCompact
  }

  const mediaQuery = window.matchMedia(query)
  isCompact.value = mediaQuery.matches

  const handleChange = (event) => {
    isCompact.value = event.matches
  }

  // 现代浏览器用 addEventListener；旧版 Safari 只有已废弃的 addListener。
  if (typeof mediaQuery.addEventListener === 'function') {
    mediaQuery.addEventListener('change', handleChange)
    onBeforeUnmount(() => mediaQuery.removeEventListener('change', handleChange))
  } else if (typeof mediaQuery.addListener === 'function') {
    mediaQuery.addListener(handleChange)
    onBeforeUnmount(() => mediaQuery.removeListener(handleChange))
  }

  return isCompact
}

export default useIsCompact
