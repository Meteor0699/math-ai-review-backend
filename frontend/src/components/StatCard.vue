<template>
  <div class="stat-card">
    <span class="stat-card__icon" :class="tone">
      <el-icon><component :is="icon" /></el-icon>
    </span>
    <div>
      <span class="stat-card__label">{{ label }}</span>
      <strong>{{ value }}</strong>
      <small v-if="hint">{{ hint }}</small>
    </div>
  </div>
</template>

<script setup>
defineProps({
  label: { type: String, required: true },
  value: { type: [String, Number], default: 0 },
  hint: { type: String, default: '' },
  icon: { type: String, default: 'DataLine' },
  tone: { type: String, default: 'primary' }
})
</script>

<style scoped>
.stat-card {
  display: flex;
  align-items: center;
  gap: var(--space-4);
  min-height: 104px;
  padding: var(--space-5);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  background: var(--color-surface);
}

.stat-card__icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  flex: 0 0 44px;
  border-radius: var(--radius-md);
  color: var(--color-primary);
  background: var(--color-primary-light);
  font-size: 21px;
}

.stat-card__icon.success {
  color: #15803d;
  background: var(--color-success-light);
}

.stat-card__icon.warning {
  color: #b45309;
  background: var(--color-warning-light);
}

.stat-card__icon.danger {
  color: #dc2626;
  background: var(--color-danger-light);
}

.stat-card__label,
.stat-card small {
  display: block;
  color: var(--color-text-secondary);
  font-size: var(--text-xs);
}

.stat-card strong {
  display: block;
  margin: 2px 0;
  font-size: 24px;
  line-height: 1.25;
}
</style>
