<template>
  <div class="chapter-manage">
    <div class="page-header">
      <h2>章节管理</h2>
      <el-button type="primary" @click="openDialog(null)">
        <el-icon><Plus /></el-icon>
        新增章节
      </el-button>
    </div>

    <!-- 筛选 -->
    <el-form inline style="margin-bottom: 16px">
      <el-form-item label="所属课程">
        <el-select v-model="filterCourseId" placeholder="全部课程" clearable @change="fetchList">
          <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
        </el-select>
      </el-form-item>
    </el-form>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="60" />
      <el-table-column prop="courseName" label="所属课程" width="140" />
      <el-table-column prop="courseId" label="课程ID" width="80" />
      <el-table-column prop="title" label="章节标题" min-width="180" show-overflow-tooltip />
      <el-table-column prop="description" label="章节描述" min-width="200" show-overflow-tooltip />
      <el-table-column prop="sortOrder" label="排序" width="80" />
      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openDialog(row)">编辑</el-button>
          <el-button size="small" text type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <el-dialog
      v-model="dialogVisible"
      :title="formMode === 'add' ? '新增章节' : '编辑章节'"
      width="500px"
      :close-on-click-modal="false"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="80px">
        <el-form-item label="所属课程" prop="courseId">
          <el-select v-model="form.courseId" placeholder="请选择课程" style="width: 100%">
            <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="章节标题" prop="title">
          <el-input v-model="form.title" placeholder="请输入章节标题" />
        </el-form-item>
        <el-form-item label="章节描述">
          <el-input v-model="form.description" type="textarea" :rows="2" placeholder="请输入章节描述" />
        </el-form-item>
        <el-form-item label="排序">
          <el-input-number v-model="form.sortOrder" :min="0" :max="999" />
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
import { adminGetChapterList, adminCreateChapter, adminUpdateChapter, adminDeleteChapter } from '../../api/admin'
import { adminGetCourseList } from '../../api/admin'
import { ElMessage, ElMessageBox } from 'element-plus'

const tableData = ref([])
const courseList = ref([])
const loading = ref(false)
const dialogVisible = ref(false)
const formMode = ref('add')
const editingId = ref(null)
const submitting = ref(false)
const filterCourseId = ref(null)
const formRef = ref(null)

const form = reactive({ courseId: null, title: '', description: '', sortOrder: 0 })
const rules = {
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  title: [{ required: true, message: '请输入章节标题', trigger: 'blur' }]
}

onMounted(async () => {
  await loadCourses()
  fetchList()
})

async function loadCourses() {
  try {
    const res = await adminGetCourseList({ page: 1, pageSize: 100 })
    if (res.code === 200) courseList.value = res.data?.items || []
  } catch { /* 忽略 */ }
}

async function fetchList() {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 100 }
    if (filterCourseId.value) params.courseId = filterCourseId.value
    const res = await adminGetChapterList(params)
    if (res.code === 200) tableData.value = res.data?.items || []
  } catch { ElMessage.error('加载章节列表失败') }
  finally { loading.value = false }
}

function openDialog(row) {
  formRef.value?.resetFields()
  if (row) {
    formMode.value = 'edit'
    editingId.value = row.id
    form.courseId = row.courseId
    form.title = row.title
    form.description = row.description || ''
    form.sortOrder = row.sortOrder || 0
  } else {
    formMode.value = 'add'
    editingId.value = null
    form.courseId = null
    form.title = ''
    form.description = ''
    form.sortOrder = 0
  }
  dialogVisible.value = true
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return
  submitting.value = true
  try {
    const data = {
      courseId: form.courseId,
      title: form.title,
      description: form.description,
      sortOrder: form.sortOrder
    }
    let res
    if (formMode.value === 'add') {
      res = await adminCreateChapter(data)
    } else {
      res = await adminUpdateChapter(editingId.value, data)
    }
    if (res.code === 200) {
      ElMessage.success(formMode.value === 'add' ? '新增成功' : '编辑成功')
      dialogVisible.value = false
      fetchList()
    } else {
      ElMessage.error(res.message || '操作失败')
    }
  } catch { ElMessage.error('操作失败') }
  finally { submitting.value = false }
}

async function handleDelete(row) {
  try {
    await ElMessageBox.confirm(`确定删除章节「${row.title}」吗？`, '删除确认', { type: 'warning' })
    const res = await adminDeleteChapter(row.id)
    if (res.code === 200) {
      ElMessage.success('删除成功')
      fetchList()
    } else {
      ElMessage.error(res.message || '删除失败')
    }
  } catch { /* 取消 */ }
}
</script>

<style scoped>
.chapter-manage { max-width: 1200px; }
.page-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; }
.page-header h2 { font-size: 20px; color: #303133; }
</style>