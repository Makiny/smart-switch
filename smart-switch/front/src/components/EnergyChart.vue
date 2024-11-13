<!--@author kaiyin-->

<template>
  <div class="chart-container">
    <canvas ref="chart"></canvas>
  </div>
</template>

<script>
import { Chart, registerables } from 'chart.js';
import 'chartjs-adapter-date-fns'; // 导入日期适配器

Chart.register(...registerables); // 注册所有必须的组件

export default {
  props: {
    data: {
      type: Array,
      required: true,
    },
    maxTicksLimit: 0,
    xTick: {
      type: String,
      default: 'day', // 默认周期为周
    },
  },
  mounted() {
    this.createChart();
  },
  watch: {
    // 监听 data 数据的变化
    data: {
      handler(newData) {
        this.updateChartData(newData); // 调用更新图表的方法
      },
      deep: true, // 深度监听
    },
    xTick: {
      handler() {
        this.updateChartXTick();
      },
    },
  },
  methods: {

    updateChartData(newData) {
      const labels = newData.map(item => new Date(item.time * 1000));
      const consumptionData = newData.map(item => item.data);

      // 更新图表数据
      this.chart.data.labels = labels;
      this.chart.data.datasets[0].data = consumptionData;

      // 刷新图表
      this.chart.update();
    },

    updateChartXTick() {
      if (!this.chart) return; // 如果没有图表实例，则不进行更新

      // 动态设置 timeUnit 和 displayFormat
      const timeUnit = this.xTick === 'year' ? 'month' : 'day';
      const displayFormat = this.xTick === 'year' ? 'MM' : 'MM.dd';
      const displayFormatLabel = this.xTick === 'year' ? 'yyyy.MM' : 'MM.dd';;

      // 更新图表的时间轴格式
      this.chart.options.scales.x.time.unit = timeUnit;
      this.chart.options.scales.x.time.displayFormats.month = displayFormat;
      this.chart.options.scales.x.time.displayFormats.day = displayFormat;
      this.chart.options.scales.x.time.tooltipFormat = displayFormatLabel;

      // 刷新图表
      this.chart.update();
    },

    createChart() {
      if (this.chart) {
        this.chart.destroy(); // 销毁之前的图表实例
      }
      const ctx = this.$refs.chart.getContext('2d');
      const labels = this.data.map(item => new Date(item.time * 1000)); // 使用 Unix 时间戳转换为 Date 对象
      const consumptionData = this.data.map(item => item.data);

      this.chart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: labels,
          datasets: [
            {
              label: '', // 这里将标签设置为空，去掉顶部提示信息
              data: consumptionData,
              backgroundColor: 'rgba(75, 192, 192, 0.4)',
              // borderColor: 'rgba(75, 192, 192, 0)',
              // borderWidth: 1,
              // barPercentage: 0.9, // 调整柱子的宽度
              // categoryPercentage: 0.8, // 调整柱子之间的间距
              hoverBackgroundColor: 'rgba(75, 192, 192, 0.8)',
            },
          ],
        },
        options: {
          responsive: true,
          maintainAspectRatio: false, // 禁用保持宽高比
          scales: {
            x: {
              type: 'time',
              time: {
                unit: 'day',
                displayFormats: {
                  month: 'MM.dd',
                  day: 'MM.dd',
                },
                tooltipFormat: 'MM.dd', // 显示日期的格式
              },
              grid: {
                display: false, // 隐藏网格线
              },
              ticks: {
                source: 'data',
                // autoSkip: false,
                maxTicksLimit: this.maxTicksLimit,
              },
              title: {
                display: false, // 隐藏横坐标标题
              },
            },
            y: {
              beginAtZero: false,
              grid: {
                color: '#ddd', // 网格线颜色
                // borderColor: 'transparent', // 去除左侧竖线
              },
              ticks: {
                maxTicksLimit: 5, // 设置纵坐标的最大刻度数量为5
                // stepSize: 10,     // 每个刻度之间的步长为10
              },
              title: {
                // 隐藏纵坐标标题
                display: false,
              },
              border: {
                display: false, // 隐藏 Y 轴的边框
              }
            },
          },
          plugins: {
            tooltip: {
              callbacks: {
                label: function (context) {
                  return ` ${context.raw} 度`;
                },
              },
              displayColors: false,
            },
            legend: {
              // 隐藏图例
              display: false,
            },
            title: {
              // 隐藏图表标题
              display: false,
            },
          },
        },
      });
    },
  },
};
</script>

<style scoped>
.chart-container {
  position: relative;
  width: 100%; /* 宽度自适应父容器 */
  height: 250px; /* 高度设置为固定值 */
}

canvas {
  width: 100%; /* 使 Canvas 填充父容器的宽度 */
  height: 100%; /* 使 Canvas 填充父容器的高度 */
}
</style>
