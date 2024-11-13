<!--@author kaiyin-->

<template>
  <div class="gauge-chart">
    <svg
        viewBox="0 0 100 100"
        xmlns="http://www.w3.org/2000/svg"
        class="gauge-chart-svg"
    >
      <!-- 定义渐变 -->
      <defs>
        <linearGradient id="gaugeGradient" x1="0%" y1="100%" x2="100%" y2="50%">
          <stop offset="0%" style="stop-color:#2cc36b; stop-opacity:1" />
          <stop offset="50%" style="stop-color:#3498db; stop-opacity:1" />
          <stop offset="100%" style="stop-color:#f0f; stop-opacity:1" />
        </linearGradient>
      </defs>
      <!-- 背景圆环 -->
      <circle
          cx="50"
          cy="50"
          r="45"
          stroke="#E0E0E0"
          stroke-width="8"
          fill="none"
          :stroke-dasharray="backgroundDisplayLength + ' ' + backgroundRemainingLength"
          :stroke-dashoffset="backgroundOffset"
          stroke-linecap="round"
      />
      <!-- 前景圆环 -->
      <circle
          cx="50"
          cy="50"
          r="45"
          stroke="url(#gaugeGradient)"
          stroke-width="8"
          fill="none"
          :stroke-dasharray="displayLength + ' ' + remainingLength"
          :stroke-dashoffset="offset"
          stroke-linecap="round"
          :style="{ transition: 'stroke-dasharray 0.6s' }"
      >
      </circle>
      <!-- 显示值 -->
      <text
          x="50"
          y="55"
          text-anchor="middle"
          font-size="16"
          font-weight="bold"
          fill="#000"
      >
        {{ value.toFixed(1) }}
      </text>
      <!-- 标签文字 -->
      <text
          x="50"
          y="95"
          text-anchor="middle"
          font-size="14"
          fill="#747474"
      >
        {{ label }}
      </text>
    </svg>
  </div>
</template>

<script>
export default {
  props: {
    value: {
      type: Number,
      default: 0
    },
    max: {
      type: Number,
      default: 100
    },
    label: {
      type: String,
      default: ''
    }
  },
  data() {
    return {
      animatedValue: 0, // 初始值为 0 用于动画
    };
  },
  mounted() {
    setTimeout(() => {
      this.animatedValue = this.value;  // 更新值，触发动画
    }, 1);  // 设置稍长的延时以触发过渡效果
  },
  watch: {
    value(newValue) {
      this.animatedValue = newValue;
    }
  },
  computed: {
    // 圆的周长
    circleCircumference() {
      return 2 * Math.PI * 45; // 圆周长
    },
    // 显示的弧长，最多为240度（2/3圆周）
    displayLength() {
      const maxDisplayLength = this.circleCircumference * (2 / 3); // 240度对应的最大弧长
      const calculatedLength = this.circleCircumference * (this.animatedValue / this.max) * (2 / 3);
      return Math.min(calculatedLength, maxDisplayLength); // 确保不超过最大值
    },
    // 剩余的长度，用于使弧线之后的部分保持透明
    remainingLength() {
      return this.circleCircumference - this.displayLength;
    },
    // 控制前景弧线的偏移，从八点钟（210度）开始
    offset() {
       // 210度位置的起始偏移量
      return this.circleCircumference * (210 / 360);
    },
    // 背景弧线的显示长度，从8点钟到4点钟
    backgroundDisplayLength() {
      return this.circleCircumference * (240 / 360); // 240度的弧长
    },
    // 背景弧线剩余的长度
    backgroundRemainingLength() {
      return this.circleCircumference - this.backgroundDisplayLength;
    },
    // 背景弧线的偏移，从8点钟（240度）开始
    backgroundOffset() {
       // 240度位置的起始偏移量
      return this.circleCircumference * (210 / 360);
    }
  },
};
</script>

<style scoped>
.gauge-chart {
  width: 90px;
  height: 90px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.gauge-chart-svg {
  width: 100%;
  height: 100%;
}
</style>
