<template>
  <!-- renderMathContent escapes all source HTML before KaTeX renders with trust disabled. -->
  <!-- eslint-disable-next-line vue/no-v-html -->
  <div class="formula-content" v-html="safeContent"></div>
</template>

<script setup>
import { computed } from 'vue'
import { renderMathContent } from '../utils/renderMathContent'

const props = defineProps({
  content: { type: String, default: '' }
})

const safeContent = computed(() => renderMathContent(props.content))
</script>

<style scoped>
.formula-content {
  max-width: 100%;
  overflow-wrap: anywhere;
  color: var(--color-text-primary);
  line-height: 1.85;
}

.formula-content :deep(p) {
  margin: 8px 0;
}

.formula-content :deep(.katex-display) {
  max-width: 100%;
  margin: 14px 0;
  overflow-x: auto;
  overflow-y: hidden;
  padding-bottom: 4px;
}

.formula-content :deep(code) {
  padding: 2px 5px;
  border-radius: 4px;
  color: #475569;
  background: var(--color-surface-muted);
}
</style>
