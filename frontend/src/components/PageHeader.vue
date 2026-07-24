<template>
  <header class="page-heading">
    <div>
      <div v-if="$slots.breadcrumb" class="page-heading__breadcrumb">
        <slot name="breadcrumb" />
      </div>
      <span v-if="eyebrow" class="page-heading__eyebrow">{{ eyebrow }}</span>
      <h1>{{ title }}</h1>
      <p v-if="description">{{ description }}</p>
    </div>
    <div v-if="$slots.actions" class="page-heading__actions">
      <slot name="actions" />
    </div>
  </header>
</template>

<script setup>
defineProps({
  eyebrow: { type: String, default: '' },
  title: { type: String, required: true },
  description: { type: String, default: '' }
})
</script>

<style scoped>
.page-heading {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  gap: var(--space-5);
  margin-bottom: var(--space-6);
}

.page-heading h1 {
  font-size: var(--text-xl);
  font-weight: 750;
}

.page-heading__eyebrow {
  display: block;
  margin-bottom: var(--space-1);
  color: var(--color-primary);
  font-size: var(--text-xs);
  font-weight: 700;
}

.page-heading p {
  margin-top: var(--space-2);
  color: var(--color-text-secondary);
}

.page-heading__breadcrumb {
  margin-bottom: var(--space-3);
}

.page-heading__actions {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: var(--space-2);
}

@media (max-width: 640px) {
  .page-heading {
    align-items: flex-start;
    flex-direction: column;
    margin-bottom: var(--space-5);
  }

  .page-heading__actions {
    width: 100%;
    justify-content: flex-start;
  }
}
</style>
