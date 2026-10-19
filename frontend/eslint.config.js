import js from '@eslint/js'
import pluginVue from 'eslint-plugin-vue'
import pluginVueA11y from 'eslint-plugin-vuejs-accessibility'
import globals from 'globals'

export default [
  {
    ignores: ['dist/**', 'node_modules/**', 'playwright-report/**', 'test-results/**']
  },
  js.configs.recommended,
  ...pluginVue.configs['flat/recommended'],
  {
    files: ['src/**/*.{js,vue}', 'tests/**/*.js', 'e2e/**/*.js', 'scripts/**/*.mjs', '*.config.js'],
    languageOptions: {
      ecmaVersion: 'latest',
      sourceType: 'module',
      globals: {
        ...globals.browser,
        ...globals.node
      }
    },
    rules: {
      'no-unused-vars': ['error', { argsIgnorePattern: '^_', caughtErrorsIgnorePattern: '^_' }],
      'vue/attributes-order': 'off',
      'vue/html-indent': ['error', 2],
      'vue/html-self-closing': 'off',
      'vue/max-attributes-per-line': 'off',
      'vue/multi-word-component-names': 'off',
      'vue/no-v-html': 'error',
      'vue/singleline-html-element-content-newline': 'off'
    }
  },
  {
    // Front-End Checklist `accessibility-testing`: catch a11y regressions at
    // lint time. Kept at warn level so the existing codebase does not fail
    // `npm run lint`; promote to error once the remaining hits are cleared.
    files: ['src/**/*.vue'],
    plugins: { 'vuejs-accessibility': pluginVueA11y },
    rules: {
      'vuejs-accessibility/alt-text': 'warn',
      'vuejs-accessibility/anchor-has-content': 'warn',
      'vuejs-accessibility/aria-props': 'warn',
      'vuejs-accessibility/aria-role': 'warn',
      'vuejs-accessibility/click-events-have-key-events': 'warn',
      'vuejs-accessibility/form-control-has-label': 'warn',
      'vuejs-accessibility/label-has-for': 'warn',
      'vuejs-accessibility/no-static-element-interactions': 'warn'
    }
  }
]
