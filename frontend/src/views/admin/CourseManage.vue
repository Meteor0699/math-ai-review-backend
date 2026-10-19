<template>
  <div class="app-page management-page course-manage">
    <div class="page-header">
      <h2>课程管理</h2>
      <el-button type="primary" @click="openDialog(null)">
        <el-icon><Plus /></el-icon>
        新增课程
      </el-button>
    </div>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="70" />
      <el-table-column label="封面" width="92">
        <template #default="{ row }">
          <img v-if="row.coverUrl" class="cover-thumb" :src="row.coverUrl" :alt="row.courseName" />
          <span v-else class="cover-empty">无</span>
        </template>
      </el-table-column>
      <el-table-column prop="courseName" label="课程名称" min-width="150" />
      <el-table-column prop="courseCode" label="课程编号" width="150" />
      <el-table-column prop="courseDesc" label="课程描述" min-width="220" show-overflow-tooltip />
      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openDialog(row)">编辑</el-button>
          <el-button size="small" text type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog
      v-model="dialogVisible"
      :title="formMode === 'add' ? '新增课程' : '编辑课程'"
      width="560px"
      :close-on-click-modal="false"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="90px">
        <el-form-item label="课程名称" prop="name">
          <el-input v-model="form.name" placeholder="请输入课程名称" />
        </el-form-item>
        <el-form-item label="课程编号" prop="code">
          <el-input v-model="form.code" placeholder="请输入课程编号" />
        </el-form-item>
        <el-form-item label="封面地址">
          <el-input v-model="form.coverUrl" placeholder="/textbook-covers/engineering-math-linear-algebra.svg" />
        </el-form-item>
        <el-form-item label="课程描述" prop="description">
          <el-input v-model="form.description" type="textarea" :rows="3" placeholder="请输入课程描述" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, reactive } from 'vue'
import { adminGetCourseList, adminCreateCourse, adminUpdateCourse, adminDeleteCourse } from '../../api/admin'
import { ElMessage, ElMessageBox } from 'element-plus'

const tableData = ref([])
const loading = ref(false)
const dialogVisible = ref(false)
const formMode = ref('add')
const editingId = ref(null)
const submitting = ref(false)
const formRef = ref(null)

const form = reactive({ name: '', code: '', coverUrl: '', description: '' })
const rules = {
  name: [{ required: true, message: '请输入课程名称', trigger: 'blur' }],
  code: [{ required: true, message: '请输入课程编号', trigger: 'blur' }],
  description: [{ required: true, message: '请输入课程描述', trigger: 'blur' }]
}

onMounted(() => { fetchList() })

async function fetchList() {
  loading.value = true
  try {
    const res = await adminGetCourseList({ page: 1, pageSize: 100 })
    if (res.code === 200) tableData.value = res.data?.items || []
  } catch {
    ElMessage.error('加载课程列表失败')
  } finally {
    loading.value = false
  }
}

function openDialog(row) {
  formRef.value?.resetFields()
  if (row) {
    formMode.value = 'edit'
    editingId.value = row.id
    form.name = row.courseName
    form.code = row.courseCode
    form.coverUrl = row.coverUrl || ''
    form.description = row.courseDesc
  } else {
    formMode.value = 'add'
    editingId.value = null
    form.name = ''
    form.code = ''
    form.coverUrl = ''
    form.description = ''
  }
  dialogVisible.value = true
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return
  submitting.value = true
  try {
    const data = {
      name: form.name,
      code: form.code,
      coverUrl: form.coverUrl,
      description: form.description
    }
    const res = formMode.value === 'add'
      ? await adminCreateCourse(data)
      : await adminUpdateCourse(editingId.value, data)
    if (res.code === 200) {
      ElMessage.success(formMode.value === 'add' ? '新增成功' : '编辑成功')
      dialogVisible.value = false
      fetchList()
    } else {
      ElMessage.error(res.message || '操作失败')
    }
  } catch {
    ElMessage.error('操作失败')
  } finally {
    submitting.value = false
  }
}

async function handleDelete(row) {
  try {
    await ElMessageBox.confirm(`确定删除课程「${row.courseName}」吗？`, '删除确认', { type: 'warning' })
    const res = await adminDeleteCourse(row.id)
    if (res.code === 200) {
      ElMessage.success('删除成功')
      fetchList()
    } else {
      ElMessage.error(res.message || '删除失败')
    }
  } catch {
    // User cancelled.
  }
}
</script>

<style scoped>
.cover-thumb {
  display: block;
  width: 46px;
  height: 64px;
  object-fit: cover;
  border: 1px solid var(--color-border);
  border-radius: var(--radius-sm);
  background: var(--color-surface-muted);
}

.cover-empty {
  color: var(--color-text-tertiary);
  font-size: var(--text-xs);
}
</style>
