<template>
  <div class="paper-manage">
    <div class="page-header">
      <h2>试卷管理</h2>
      <el-button type="primary" @click="openUploadDialog">
        <el-icon><Upload /></el-icon>
        上传试卷
      </el-button>
    </div>

    <el-form inline style="margin-bottom: 16px">
      <el-form-item label="课程">
        <el-select v-model="filterCourseId" placeholder="全部" clearable @change="fetchList">
          <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
        </el-select>
      </el-form-item>
      <el-form-item label="年份">
        <el-input v-model="filterExamYear" placeholder="如 2024" clearable @change="fetchList" style="width: 120px" />
      </el-form-item>
    </el-form>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="60" />
      <el-table-column prop="title" label="试卷名称" min-width="200" show-overflow-tooltip />
      <el-table-column prop="courseName" label="课程" width="120" />
      <el-table-column prop="examYear" label="年份" width="80" />
      <el-table-column prop="filePath" label="文件名" min-width="200" show-overflow-tooltip />
      <el-table-column label="操作" width="220" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openEditDialog(row)">编辑</el-button>
          <el-button size="small" text type="success" @click="handleDownload(row)">下载</el-button>
          <el-button size="small" text type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 上传对话框 -->
    <el-dialog
      v-model="uploadVisible"
      title="上传试卷"
      width="500px"
      :close-on-click-modal="false"
    >
      <el-form ref="uploadFormRef" :model="uploadForm" :rules="uploadRules" label-width="80px">
        <el-form-item label="试卷名称" prop="title">
          <el-input v-model="uploadForm.title" placeholder="请输入试卷名称" />
        </el-form-item>
        <el-form-item label="课程" prop="courseId">
          <el-select v-model="uploadForm.courseId" placeholder="请选择" style="width: 100%">
            <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="年份" prop="examYear">
          <el-input v-model="uploadForm.examYear" placeholder="如 2024" />
        </el-form-item>
        <el-form-item label="试卷文件" prop="file">
          <el-upload
            ref="uploadRef"
            :auto-upload="false"
            :limit="1"
            :on-change="onFileChange"
            :on-remove="onFileRemove"
            accept=".pdf,.doc,.docx,.txt"
          >
            <el-button type="primary">
              <el-icon><Upload /></el-icon>
              选择文件
            </el-button>
            <template #tip>
              <div class="upload-tip">支持 PDF、Word、TXT 格式</div>
            </template>
          </el-upload>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="uploadVisible = false">取消</el-button>
        <el-button type="primary" :loading="uploading" @click="handleUpload">上传</el-button>
      </template>
    </el-dialog>

    <!-- 编辑对话框 -->
    <el-dialog
      v-model="editVisible"
      title="编辑试卷"
      width="500px"
      :close-on-click-modal="false"
    >
      <el-form ref="editFormRef" :model="editForm" :rules="editRules" label-width="80px">
        <el-form-item label="试卷名称" prop="title">
          <el-input v-model="editForm.title" placeholder="请输入试卷名称" />
        </el-form-item>
        <el-form-item label="课程" prop="courseId">
          <el-select v-model="editForm.courseId" placeholder="请选择" style="width: 100%">
            <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="年份" prop="examYear">
          <el-input v-model="editForm.examYear" placeholder="如 2024" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editVisible = false">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleEditSubmit">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, reactive } from 'vue'
import {
  adminGetPaperList, adminUploadPaper, adminUpdatePaper, adminDeletePaper,
  adminGetCourseList
} from '../../api/admin'
import { ElMessage, ElMessageBox } from 'element-plus'

const tableData = ref([])
const courseList = ref([])
const loading = ref(false)
const filterCourseId = ref(null)
const filterExamYear = ref('')

// 上传
const uploadVisible = ref(false)
const uploadFormRef = ref(null)
const uploadRef = ref(null)
const uploading = ref(false)
const uploadFile = ref(null)
const uploadForm = reactive({ title: '', courseId: null, examYear: '' })
const uploadRules = {
  title: [{ required: true, message: '请输入试卷名称', trigger: 'blur' }],
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  examYear: [{ required: true, message: '请输入年份', trigger: 'blur' }]
}

// 编辑
const editVisible = ref(false)
const editFormRef = ref(null)
const editingId = ref(null)
const submitting = ref(false)
const editForm = reactive({ title: '', courseId: null, examYear: '' })
const editRules = {
  title: [{ required: true, message: '请输入试卷名称', trigger: 'blur' }],
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  examYear: [{ required: true, message: '请输入年份', trigger: 'blur' }]
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
    if (filterExamYear.value) params.examYear = filterExamYear.value
    const res = await adminGetPaperList(params)
    if (res.code === 200) tableData.value = res.data?.items || []
  } catch { ElMessage.error('加载试卷列表失败') }
  finally { loading.value = false }
}

// ========== 上传 ==========
function openUploadDialog() {
  uploadFormRef.value?.resetFields()
  uploadForm.title = ''
  uploadForm.courseId = null
  uploadForm.examYear = ''
  uploadFile.value = null
  uploadRef.value?.clearFiles()
  uploadVisible.value = true
}

function onFileChange(file) {
  uploadFile.value = file.raw
}

function onFileRemove() {
  uploadFile.value = null
}

async function handleUpload() {
  const valid = await uploadFormRef.value.validate().catch(() => false)
  if (!valid) return
  if (!uploadFile.value) {
    ElMessage.warning('请选择文件')
    return
  }
  uploading.value = true
  try {
    const fd = new FormData()
    fd.append('file', uploadFile.value)
    fd.append('title', uploadForm.title)
    fd.append('courseId', uploadForm.courseId)
    fd.append('examYear', uploadForm.examYear)
    const res = await adminUploadPaper(fd)
    if (res.code === 200) {
      ElMessage.success('上传成功')
      uploadVisible.value = false
      fetchList()
    } else {
      ElMessage.error(res.message || '上传失败')
    }
  } catch { ElMessage.error('上传失败') }
  finally { uploading.value = false }
}

// ========== 编辑 ==========
function openEditDialog(row) {
  editFormRef.value?.resetFields()
  editingId.value = row.id
  editForm.title = row.title
  editForm.courseId = row.courseId
  editForm.examYear = row.examYear ? String(row.examYear) : ''
  editVisible.value = true
}

async function handleEditSubmit() {
  const valid = await editFormRef.value.validate().catch(() => false)
  if (!valid) return
  submitting.value = true
  try {
    const data = {
      title: editForm.title,
      courseId: editForm.courseId,
      examYear: editForm.examYear ? parseInt(editForm.examYear) : null
    }
    const res = await adminUpdatePaper(editingId.value, data)
    if (res.code === 200) {
      ElMessage.success('编辑成功')
      editVisible.value = false
      fetchList()
    } else {
      ElMessage.error(res.message || '编辑失败')
    }
  } catch { ElMessage.error('编辑失败') }
  finally { submitting.value = false }
}

// ========== 删除 ==========
async function handleDelete(row) {
  try {
    await ElMessageBox.confirm(`确定删除试卷「${row.title}」吗？`, '删除确认', { type: 'warning' })
    const res = await adminDeletePaper(row.id)
    if (res.code === 200) {
      ElMessage.success('删除成功')
      fetchList()
    } else {
      ElMessage.error(res.message || '删除失败')
    }
  } catch { /* 取消 */ }
}

// ========== 下载 ==========
function handleDownload(row) {
  const url = row.fileUrl || (row.id ? `/api/papers/${row.id}/download` : '')
  if (url) {
    window.open(url, '_blank')
  } else {
    ElMessage.warning('该试卷暂无文件')
  }
}
</script>

<style scoped>
.paper-manage { max-width: 1200px; }
.page-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; }
.page-header h2 { font-size: 20px; color: #303133; }
.upload-tip { font-size: 12px; color: #909399; margin-top: 4px; }
</style>
