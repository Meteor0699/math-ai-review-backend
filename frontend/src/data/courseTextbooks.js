const textbookMap = {
  advanced_math: [
    {
      title: '高等数学 上册',
      subtitle: '同济版',
      publisher: '高等教育出版社',
      coverUrl: '/textbook-covers/tongji-advanced-math-1.svg'
    },
    {
      title: '高等数学 下册',
      subtitle: '同济版',
      publisher: '高等教育出版社',
      coverUrl: '/textbook-covers/tongji-advanced-math-2.svg'
    }
  ],
  linear_algebra: [
    {
      title: '工程数学 线性代数',
      subtitle: '同济版',
      publisher: '高等教育出版社',
      coverUrl: '/textbook-covers/engineering-math-linear-algebra.svg'
    },
    {
      title: '线性代数',
      subtitle: '中南财经政法大学 经管类',
      publisher: '经管类课程参考',
      coverUrl: '/textbook-covers/znufe-linear-algebra.svg'
    }
  ],
  probability_statistics: [
    {
      title: '概率论与数理统计',
      subtitle: '中南财经政法大学 经管类',
      publisher: '经管类课程参考',
      coverUrl: '/textbook-covers/znufe-probability-statistics.svg'
    },
    {
      title: '概率论与数理统计',
      subtitle: '同济/高校数学参考',
      publisher: '高等教育出版社',
      coverUrl: '/textbook-covers/tongji-probability-statistics.svg'
    }
  ]
}

function normalizeCourseCode(course) {
  return course?.courseCode || course?.code || ''
}

export function getCourseTextbooks(course) {
  return textbookMap[normalizeCourseCode(course)] || []
}

export function getPrimaryCourseCover(course) {
  return course?.coverUrl || getCourseTextbooks(course)[0]?.coverUrl || ''
}

export { textbookMap }
