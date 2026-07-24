<template>
  <div class="app-page paper-list">
    <div class="page-header">
      <div>
        <h2>试卷库</h2>
        <p>查看公开试卷，也可以上传自己的数学试卷，AI 审核通过后纳入试卷库。</p>
      </div>
      <el-button type="primary" @click="openUploadDialog">
        <el-icon><Upload /></el-icon>
        上传试卷
      </el-button>
    </div>

    <el-tabs v-model="activeTab" class="paper-tabs" @tab-change="loadPapers">
      <el-tab-pane label="公开试卷" name="public" />
      <el-tab-pane label="我的试卷" name="mine" />
    </el-tabs>

    <div v-loading="loading" class="paper-stack">
      <el-card
        v-for="paper in list"
        :key="paper.id"
        class="paper-card"
        shadow="hover"
      >
        <div class="paper-main">
          <div class="paper-icon">
            <el-icon><Document /></el-icon>
          </div>
          <div class="paper-info">
            <div class="paper-title">{{ paper.title }}</div>
            <div class="paper-meta">
              <el-tag size="small" effect="plain">{{ paper.examYear || '未知年份' }}</el-tag>
              <el-tag v-if="paper.visibility" size="small" :type="paper.visibility === 'public' ? 'success' : 'info'" effect="plain">
                {{ paper.visibility === 'public' ? '公开' : '私有' }}
              </el-tag>
              <el-tag v-if="paper.aiReviewStatus === 'approved'" size="small" type="primary" effect="plain">
                AI 已审核
              </el-tag>
              <span>{{ paper.courseName }}</span>
              <span v-if="paper.originalFilename">{{ paper.originalFilename }}</span>
            </div>
          </div>
          <el-button type="primary" plain size="small" @click="openPaper(paper)">
            查看/下载
          </el-button>
        </div>
      </el-card>

      <el-empty v-if="!loading && list.length === 0" description="暂无试卷" />
    </div>

    <el-dialog v-model="uploadDialogVisible" title="上传数学试卷" width="560px" :close-on-click-modal="false">
      <el-alert
        type="info"
        show-icon
        :closable="false"
        title="学生上传的文件会先经过 AI 审核。只有被识别为数学试卷、数学测试卷或数学练习卷后，才会进入试卷库。"
        class="upload-alert"
      />
      <el-form ref="uploadFormRef" :model="uploadForm" :rules="uploadRules" label-width="90px">
        <el-form-item label="课程" prop="courseId">
          <el-select v-model="uploadForm.courseId" placeholder="请选择课程" class="form-control">
            <el-option v-for="course in courseList" :key="course.id" :label="course.courseName" :value="course.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="标题" prop="title">
          <el-input v-model="uploadForm.title" placeholder="例如：高等数学期末模拟试卷" />
        </el-form-item>
        <el-form-item label="年份">
          <el-input v-model="uploadForm.examYear" placeholder="例如：2026" />
        </el-form-item>
        <el-form-item label="可见范围" prop="visibility">
          <el-radio-group v-model="uploadForm.visibility">
            <el-radio-button label="private">仅自己可见</el-radio-button>
            <el-radio-button label="public">公开给大家</el-radio-button>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="文件" prop="file">
          <el-upload
            class="upload-box"
            drag
            :auto-upload="false"
            :limit="1"
            :on-change="handleFileChange"
            :on-remove="handleFileRemove"
            :file-list="fileList"
            accept=".pdf,.doc,.docx,.txt"
          >
            <el-icon class="el-icon--upload"><UploadFilled /></el-icon>
            <div class="el-upload__text">拖拽文件到这里，或点击选择</div>
            <template #tip>
              <div class="el-upload__tip">支持 PDF、Word、TXT，最大 20MB。TXT 文件审核会读取正文片段。</div>
            </template>
          </el-upload>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="uploadDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="uploading" @click="submitUpload">
          {{ uploading ? 'AI 审核中...' : '提交审核并上传' }}
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getCourseList, getPaperList, getMyPaperList, uploadMyPaper } from '../../api/course'

const activeTab = ref('public')
const list = ref([])
const courseList = ref([])
const fileList = ref([])
const loading = ref(false)
const uploading = ref(false)
const uploadDialogVisible = ref(false)
const uploadFormRef = ref(null)

const uploadForm = reactive({
  courseId: null,
  title: '',
  examYear: '',
  visibility: 'private',
  file: null
})

const uploadRules = {
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  title: [{ required: true, message: '请输入试卷标题', trigger: 'blur' }],
  visibility: [{ required: true, message: '请选择可见范围', trigger: 'change' }],
  file: [{ required: true, message: '请选择试卷文件', trigger: 'change' }]
}

onMounted(async () => {
  await Promise.all([loadCourses(), loadPapers()])
})

async function loadCourses() {
  try {
    const res = await getCourseList({ page: 1, pageSize: 100 })
    courseList.value = res.data?.items || []
  } catch {
    courseList.value = []
  }
}

async function loadPapers() {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 50 }
    const res = activeTab.value === 'mine'
      ? await getMyPaperList(params)
      : await getPaperList(params)
    list.value = res.data?.items || []
  } catch {
    list.value = []
  } finally {
    loading.value = false
  }
}

function openUploadDialog() {
  uploadForm.courseId = null
  uploadForm.title = ''
  uploadForm.examYear = ''
  uploadForm.visibility = 'private'
  uploadForm.file = null
  fileList.value = []
  uploadDialogVisible.value = true
}

function handleFileChange(file, files) {
  uploadForm.file = file.raw
  fileList.value = files.slice(-1)
  uploadFormRef.value?.validateField('file')
}

function handleFileRemove() {
  uploadForm.file = null
  fileList.value = []
}

async function submitUpload() {
  const valid = await uploadFormRef.value.validate().catch(() => false)
  if (!valid) return

  uploading.value = true
  try {
    const formData = new FormData()
    formData.append('file', uploadForm.file)
    formData.append('courseId', uploadForm.courseId)
    formData.append('title', uploadForm.title)
    formData.append('visibility', uploadForm.visibility)
    if (uploadForm.examYear) formData.append('examYear', uploadForm.examYear)

    const res = await uploadMyPaper(formData)
    if (res.code === 200) {
      ElMessage.success('AI 审核通过，试卷已上传')
      uploadDialogVisible.value = false
      activeTab.value = uploadForm.visibility === 'public' ? 'public' : 'mine'
      await loadPapers()
    } else {
      ElMessage.error(res.message || '上传失败')
    }
  } catch {
    // request interceptor will show server message.
  } finally {
    uploading.value = false
  }
}

function openPaper(paper) {
  const url = paper.fileUrl || (paper.id ? `/api/papers/${paper.id}/download` : '')
  if (!url) {
    ElMessage.warning('该试卷暂无文件')
    return
  }
  window.open(url, '_blank')
}
</script>

<style scoped>
.paper-list {
  width: min(100%, 1040px);
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: var(--space-4);
  margin-bottom: var(--space-5);
}

.page-header h2 {
  color: var(--color-text-primary);
  font-size: var(--text-2xl);
}

.page-header p {
  margin-top: var(--space-1);
  color: var(--color-text-secondary);
  font-size: var(--text-sm);
}

.paper-tabs {
  margin-bottom: var(--space-3);
}

.paper-stack {
  display: grid;
  gap: var(--space-3);
}

.paper-card {
  transition: border-color var(--motion-fast) var(--ease-out), box-shadow var(--motion-fast) var(--ease-out);
}

.paper-card:hover {
  border-color: var(--color-primary-border);
  box-shadow: var(--shadow-sm);
}

.paper-main {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
}

.paper-icon {
  width: 42px;
  height: 42px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  background: var(--color-warning-light);
  color: var(--color-warning-dark);
  flex-shrink: 0;
}

.paper-info {
  flex: 1;
  min-width: 0;
}

.paper-title {
  font-size: var(--text-md);
  font-weight: 700;
  color: var(--color-text-primary);
  margin-bottom: var(--space-2);
}

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
  align-items: center;
  font-size: var(--text-xs);
  color: var(--color-text-tertiary);
}

.upload-box {
  width: 100%;
}

.upload-alert {
  margin-bottom: var(--space-4);
}

.form-control {
  width: 100%;
}

@media (max-width: 640px) {
  .page-header,
  .paper-main {
    align-items: flex-start;
  }

  .page-header {
    flex-direction: column;
  }

  .page-header .el-button,
  .paper-main > .el-button {
    width: 100%;
    min-height: 44px;
  }

  .paper-main {
    flex-wrap: wrap;
  }

  .paper-info {
    flex-basis: calc(100% - 58px);
  }
}
</style>
