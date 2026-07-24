<template>
  <router-link :to="to" class="app-logo" :class="{ compact }" aria-label="数学 AI 复习助手首页">
    <span class="app-logo__mark">
      <el-icon><DataAnalysis /></el-icon>
    </span>
    <span v-if="!compact" class="app-logo__copy">
      <strong>数学 AI 复习助手</strong>
      <small>高校数学 · 高效复习</small>
    </span>
  </router-link>
</template>

<script setup>
defineProps({
  to: { type: String, default: '/home' },
  compact: { type: Boolean, default: false }
})
</script>

<style scoped>
.app-logo {
  display: inline-flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
  color: var(--color-text-primary);
  text-decoration: none;
}

.app-logo__mark {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 42px;
  height: 42px;
  flex: 0 0 42px;
  border-radius: 11px;
  color: #fff;
  background: var(--color-primary);
  box-shadow: 0 5px 8px rgba(37, 99, 235, 0.2);
  font-size: 22px;
}

.app-logo__copy {
  display: flex;
  min-width: 0;
  flex-direction: column;
}

.app-logo__copy strong {
  overflow: hidden;
  font-size: 16px;
  line-height: 1.3;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.app-logo__copy small {
  margin-top: 2px;
  color: var(--color-text-secondary);
  font-size: 11px;
  white-space: nowrap;
}

.app-logo.compact {
  justify-content: center;
}
</style>
