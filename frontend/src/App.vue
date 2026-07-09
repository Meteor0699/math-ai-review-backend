<template>
  <router-view />
</template>

<script setup>
</script>

<style>
:root {
  --study-primary: #2563eb;
  --study-primary-soft: #eaf2ff;
  --study-green: #16a34a;
  --study-orange: #f59e0b;
  --study-purple: #7c3aed;
  --study-text: #1f2937;
  --study-muted: #6b7280;
  --study-border: #e5e7eb;
  --study-bg: #f6f8fb;
  --el-color-primary: #2563eb;
  --el-border-radius-base: 6px;
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

html, body, #app {
  height: 100%;
  font-family: 'Helvetica Neue', Helvetica, 'PingFang SC', 'Hiragino Sans GB', 'Microsoft YaHei', Arial, sans-serif;
  color: var(--study-text);
  background: var(--study-bg);
}

body {
  letter-spacing: 0;
}

.el-card {
  border-radius: 8px;
  border-color: var(--study-border);
}

.el-button {
  border-radius: 6px;
}

.el-tag {
  border-radius: 4px;
}
</style>
