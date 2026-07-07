<template>
  <div class="question-manage">
    <div class="page-header">
      <h2>题库管理</h2>
      <el-button type="primary" @click="openDialog(null)">
        <el-icon><Plus /></el-icon>
        新增题目
      </el-button>
    </div>

    <!-- 筛选 -->
    <el-form inline style="margin-bottom: 16px">
      <el-form-item label="课程">
        <el-select v-model="filterCourseId" placeholder="全部" clearable @change="onCourseFilter">
          <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
        </el-select>
      </el-form-item>
      <el-form-item label="章节">
        <el-select v-model="filterChapterId" placeholder="全部" clearable @change="fetchList">
          <el-option v-for="ch in chapterList" :key="ch.id" :label="ch.title" :value="ch.id" />
        </el-select>
      </el-form-item>
      <el-form-item label="题型">
        <el-select v-model="filterType" placeholder="全部" clearable @change="fetchList">
          <el-option label="选择题" value="single_choice" />
          <el-option label="填空题" value="blank" />
          <el-option label="计算题" value="calculation" />
          <el-option label="证明题" value="proof" />
          <el-option label="判断题" value="judge" />
        </el-select>
      </el-form-item>
      <el-form-item label="难度">
        <el-select v-model="filterDifficulty" placeholder="全部" clearable @change="fetchList">
          <el-option label="简单" value="basic" />
          <el-option label="中等" value="medium" />
          <el-option label="困难" value="hard" />
        </el-select>
      </el-form-item>
      <el-form-item>
        <el-button @click="resetFilter">重置</el-button>
      </el-form-item>
    </el-form>

    <el-table :data="tableData" v-loading="loading" border stripe>
      <el-table-column prop="id" label="ID" width="60" />
      <el-table-column prop="courseName" label="课程" width="100" />
      <el-table-column prop="chapterName" label="章节" width="100" />
      <el-table-column prop="content" label="题干" min-width="200" show-overflow-tooltip />
      <el-table-column prop="questionType" label="题型" width="80">
        <template #default="{ row }">
          {{ typeLabel(row.questionType) }}
        </template>
      </el-table-column>
      <el-table-column prop="difficulty" label="难度" width="80">
        <template #default="{ row }">
          <el-tag :type="diffTagType(row.difficulty)" size="small">{{ diffLabel(row.difficulty) }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="source" label="来源" width="120" show-overflow-tooltip />
      <el-table-column prop="sourceYear" label="年份" width="70" />
      <el-table-column prop="fromExamPaper" label="往年试题" width="90">
        <template #default="{ row }">
          <el-tag :type="row.fromExamPaper ? 'warning' : 'info'" size="small">{{ row.fromExamPaper ? '是' : '否' }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button size="small" text type="primary" @click="openDialog(row)">编辑</el-button>
          <el-button size="small" text type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 新增/编辑对话框 -->
    <el-dialog
      v-model="dialogVisible"
      :title="formMode === 'add' ? '新增题目' : '编辑题目'"
      width="750px"
      :close-on-click-modal="false"
    >
      <el-form ref="formRef" :model="form" :rules="rules" label-width="110px">
        <el-row :gutter="16">
          <el-col :span="12">
            <el-form-item label="课程" prop="courseId">
              <el-select v-model="form.courseId" placeholder="请选择" style="width: 100%" @change="onFormCourseChange">
                <el-option v-for="c in courseList" :key="c.id" :label="c.courseName" :value="c.id" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="章节" prop="chapterId">
              <el-select v-model="form.chapterId" placeholder="请选择" style="width: 100%">
                <el-option v-for="ch in formChapterList" :key="ch.id" :label="ch.title" :value="ch.id" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>
        <el-row :gutter="16">
          <el-col :span="8">
            <el-form-item label="题型" prop="questionType">
              <el-select v-model="form.questionType" placeholder="请选择" style="width: 100%">
                <el-option label="选择题" value="single_choice" />
                <el-option label="填空题" value="blank" />
                <el-option label="计算题" value="calculation" />
                <el-option label="证明题" value="proof" />
                <el-option label="判断题" value="judge" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="难度" prop="difficulty">
              <el-select v-model="form.difficulty" placeholder="请选择" style="width: 100%">
                <el-option label="简单" value="basic" />
                <el-option label="中等" value="medium" />
                <el-option label="困难" value="hard" />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="8">
            <el-form-item label="年份">
              <el-input v-model="form.sourceYear" placeholder="如 2024" />
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item label="题干" prop="content">
          <el-input v-model="form.content" type="textarea" :rows="4" placeholder="请输入题目内容" />
        </el-form-item>
        <el-form-item label="标准答案" prop="standardAnswer">
          <el-input v-model="form.standardAnswer" type="textarea" :rows="3" placeholder="请输入标准答案" />
        </el-form-item>
        <el-form-item label="解析">
          <el-input v-model="form.normalExplanation" type="textarea" :rows="3" placeholder="请输入题目解析（可选）" />
        </el-form-item>
        <el-form-item label="来源">
          <el-input v-model="form.source" placeholder="如 2024年期末试卷" />
        </el-form-item>
        <el-form-item label="是否往年试题">
          <el-switch v-model="form.fromExamPaper" />
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
import {
  adminGetQuestionList, adminCreateQuestion, adminUpdateQuestion, adminDeleteQuestion,
  adminGetCourseList, adminGetChapterList
} from '../../api/admin'
import { ElMessage, ElMessageBox } from 'element-plus'

const tableData = ref([])
const courseList = ref([])
const chapterList = ref([])
const formChapterList = ref([])
const loading = ref(false)
const dialogVisible = ref(false)
const formMode = ref('add')
const editingId = ref(null)
const submitting = ref(false)
const formRef = ref(null)

const filterCourseId = ref(null)
const filterChapterId = ref(null)
const filterType = ref(null)
const filterDifficulty = ref(null)

const typeMap = {
  single_choice: '选择题', blank: '填空题', calculation: '计算题', proof: '证明题', judge: '判断题'
}
const diffMap = { basic: '简单', medium: '中等', hard: '困难' }

function typeLabel(val) { return typeMap[val] || val }
function diffLabel(val) { return diffMap[val] || val }

const form = reactive({
  courseId: null, chapterId: null, questionType: '', difficulty: '',
  content: '', standardAnswer: '', normalExplanation: '', source: '', sourceYear: '', fromExamPaper: false
})
const rules = {
  courseId: [{ required: true, message: '请选择课程', trigger: 'change' }],
  chapterId: [{ required: true, message: '请选择章节', trigger: 'change' }],
  questionType: [{ required: true, message: '请选择题型', trigger: 'change' }],
  difficulty: [{ required: true, message: '请选择难度', trigger: 'change' }],
  content: [{ required: true, message: '请输入题干', trigger: 'blur' }],
  standardAnswer: [{ required: true, message: '请输入标准答案', trigger: 'blur' }]
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

async function loadChapters(courseId) {
  try {
    const res = await adminGetChapterList({ courseId, page: 1, pageSize: 100 })
    if (res.code === 200) return res.data?.items || []
  } catch { return [] }
  return []
}

async function onCourseFilter() {
  filterChapterId.value = null
  chapterList.value = filterCourseId.value ? await loadChapters(filterCourseId.value) : []
  fetchList()
}

async function onFormCourseChange() {
  form.chapterId = null
  formChapterList.value = form.courseId ? await loadChapters(form.courseId) : []
}

async function fetchList() {
  loading.value = true
  try {
    const params = { page: 1, pageSize: 100 }
    if (filterCourseId.value) params.courseId = filterCourseId.value
    if (filterChapterId.value) params.chapterId = filterChapterId.value
    if (filterType.value) params.questionType = filterType.value
    if (filterDifficulty.value) params.difficulty = filterDifficulty.value
    const res = await adminGetQuestionList(params)
    if (res.code === 200) tableData.value = res.data?.items || []
  } catch { ElMessage.error('加载题目列表失败') }
  finally { loading.value = false }
}

function resetFilter() {
  filterCourseId.value = null
  filterChapterId.value = null
  filterType.value = null
  filterDifficulty.value = null
  chapterList.value = []
  fetchList()
}

function openDialog(row) {
  formRef.value?.resetFields()
  formChapterList.value = []
  if (row) {
    formMode.value = 'edit'
    editingId.value = row.id
    form.courseId = row.courseId
    form.chapterId = row.chapterId
    form.questionType = row.questionType
    form.difficulty = row.difficulty
    form.content = row.content
    form.standardAnswer = row.standardAnswer
    form.normalExplanation = row.normalExplanation || ''
    form.source = row.source || ''
    form.sourceYear = row.sourceYear ? String(row.sourceYear) : ''
    form.fromExamPaper = !!row.fromExamPaper
    if (row.courseId) {
      loadChapters(row.courseId).then(items => { formChapterList.value = items })
    }
  } else {
    formMode.value = 'add'
    editingId.value = null
    Object.assign(form, {
      courseId: null, chapterId: null, questionType: '', difficulty: '',
      content: '', standardAnswer: '', normalExplanation: '', source: '', sourceYear: '', fromExamPaper: false
    })
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
      chapterId: form.chapterId,
      questionType: form.questionType,
      difficulty: form.difficulty,
      content: form.content,
      standardAnswer: form.standardAnswer,
      normalExplanation: form.normalExplanation,
      source: form.source,
      sourceYear: form.sourceYear ? parseInt(form.sourceYear) : null,
      fromExamPaper: form.fromExamPaper
    }
    let res
    if (formMode.value === 'add') {
      res = await adminCreateQuestion(data)
    } else {
      res = await adminUpdateQuestion(editingId.value, data)
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
    await ElMessageBox.confirm('确定删除该题目吗？', '删除确认', { type: 'warning' })
    const res = await adminDeleteQuestion(row.id)
    if (res.code === 200) {
      ElMessage.success('删除成功')
      fetchList()
    } else {
      ElMessage.error(res.message || '删除失败')
    }
  } catch { /* 取消 */ }
}

function diffTagType(diff) {
  const map = { basic: 'success', medium: 'warning', hard: 'danger' }
  return map[diff] || 'info'
}
</script>

<style scoped>
.question-manage { max-width: 1400px; }
.page-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; }
.page-header h2 { font-size: 20px; color: #303133; }
</style>