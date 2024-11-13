<!--@author kaiyin-->

<template>
  <!-- 页头部分 -->
  <div class="page-header">
    <button @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">用电统计</h3>
  </div>

  <!-- 页面内容部分 -->
  <div class="top-container">
    <div class="common-list">
      <div class="common-list-item">
        <span class="gauge-chart-list">
          <GaugeChart
              :value="todayUsage"
              :max="10"
              label="今日用电·度"
          />
        </span>
        <span class="gauge-chart-list">
          <GaugeChart :value="monthUsage" :max="300" label="本月用电·度"
          />
        </span>
        <span class="gauge-chart-list">
          <GaugeChart :value="currentPower" :max="2500" label="实时功率·瓦"
          />
        </span>
      </div>
    </div>
    <div class="common-list">
      <div class="common-list-item period-style">
        <div class="option-item-list">
          <div
              class="option-item"
              :class="{ selected: selectedPeriod === 'week' }"
              @click="selectPeriod('week')">
              周
          </div>
          <div
              class="option-item"
              :class="{ selected: selectedPeriod === 'month' }"
              @click="selectPeriod('month')">
              月
          </div>
          <div
              class="option-item"
              :class="{ selected: selectedPeriod === 'year' }"
              @click="selectPeriod('year')">
              年
          </div>
        </div>
      </div>

      <div class="common-list-item period-style">
        <div class="stats-item-left">
          <span class="total-period">
              总电量
              <span class="stats-value" @click="openDatePicker">
                {{formattedDate}}
                <img
                    v-if="selectedPeriod !== 'year'"
                    src="@/assets/icons/menu-down.svg"
                    class="icon" :class="{ rotated: showDatePicker }"/>
              </span>
          </span>
        </div>
      </div>

      <div class="common-list-item period-style">
          <span class="energy-usage-value"> {{this.totalUsage.toFixed(1)}} </span>
          <span class="stats-label">度</span>
      </div>

      <div class="common-list-item ">
        <EnergyChart :data="chartData" :max-ticks-limit="6" :x-tick="selectedPeriod"/>
      </div>
    </div>
  </div>

  <!-- 日期选择器 -->
  <div v-show="showDatePicker" class="date-picker-container" ref="datePickerContainer">
    <DatePickerPage
        :min-date="this.minDate"
        :max-date="this.maxDate"
        :init-date="selectedDate"
        :view-mode="this.selectedPeriod"
        :consumption-data="this.powerData"
        @update:selectedDate="updateSelectedDate"
        @close="closeDatePicker"
    />
  </div>
</template>

<script>
import '../styles/page-common-styles.css';
import EnergyChart from './EnergyChart.vue';
import DatePickerPage from "./DatePickerPage.vue";

import GaugeChart from './GaugeChart.vue';

import {fetchEnergyRowData, getDeviceStatusOnce, startGetDeviceStatus, stopGetDeviceStatus} from '../api/apiService.js';

export default {
  components: {
    EnergyChart,
    DatePickerPage,
    GaugeChart,
  },
  data() {
    return {
      chartData: [],
      powerData: [],
      showDatePicker: false, // 控制日期选择器的显示
      selectedDate: 0,
      selectedPeriod: "week",
      minDate: 0,
      maxDate: 0,
      startDate: 0,
      endDate:0,

      todayUsage: 0,
      monthUsage: 0,
      currentPower: 0,

      totalUsage: 0,

      updateInterval: null, // 定时器引用
    };
  },
  computed: {
    formattedDate() {
      if (!this.selectedDate) return '';

      const startDate = new Date(this.startDate * 1000);
      const endDate = new Date(this.endDate * 1000);
      const startYear = startDate.getFullYear();
      const startMonth = String(startDate.getMonth() + 1).padStart(2, '0');
      const startDay = String(startDate.getDate()).padStart(2, '0');
      const endYear = endDate.getFullYear();
      const endMonth = String(endDate.getMonth() + 1).padStart(2, '0');
      const endDay = String(endDate.getDate()).padStart(2, '0');

      if (this.selectedPeriod === 'week') {
        // 判断是否跨月份
        if (startMonth === endMonth) {
          return `${startYear}.${startMonth}.${startDay} - ${startMonth}.${endDay}`;
        } else {
          return `${startYear}.${startMonth}.${startDay} - ${endMonth}.${endDay}`;
        }
      } else if (this.selectedPeriod === 'month') {
        return `${startYear}.${startMonth}`;
      } else if (this.selectedPeriod === 'year') {
        // 判断是否跨年
        if (startYear === endYear) {
          return `${startYear}.${startMonth} - ${endMonth}`;
        } else {
          return `${startYear}.${startMonth} - ${endYear}.${endMonth}`;
        }
      }
      return '';
    }
  },

  methods: {
    handleBack() {
      stopGetDeviceStatus();
      this.$router.go(-1); // 返回上一级页面
    },
    openDatePicker() {
      if (this.selectedPeriod === 'year') {
        return;
      }
      if(this.showDatePicker) {
        return;
      }
      this.showDatePicker = true; // 显示日期选择器
      setTimeout(() => {
        document.addEventListener('click', this.handleOutsideClick);
      }, 0);
    },
    closeDatePicker() {
      this.showDatePicker = false; // 隐藏日期选择器
      document.removeEventListener('click', this.handleOutsideClick);
    },
    handleOutsideClick(event) {
      const datePicker = this.$refs.datePickerContainer;
      if (datePicker && !datePicker.contains(event.target)) {
        this.closeDatePicker();
      }
    },
    updateSelectedDate(newDate) {
      this.selectedDate = newDate; // 更新父组件中的selectedDate
      this.updateDateRange();  // 更新日期范围
    },
    selectPeriod(period) {
      this.selectedPeriod = period;
      this.updateDateRange();
    },
    updateDateRange() {
      const date = new Date(this.selectedDate * 1000);

      if (this.selectedPeriod === 'week') {
        const startDate = new Date(this.selectedDate * 1000);
        const endDate = new Date(startDate);
        startDate.setDate(startDate.getDate() - 6);

        this.startDate = Math.floor(startDate.getTime() / 1000);
        this.endDate = Math.floor(endDate.getTime() / 1000);

        this.chartData = this.generateDayChartData(this.startDate, this.endDate);
      } else if (this.selectedPeriod === 'month') {
        const startDate = new Date(date.getFullYear(), date.getMonth(), 1);
        const endDate = new Date(date.getFullYear(), date.getMonth() + 1, 0);

        this.startDate = Math.floor(startDate.getTime() / 1000);
        this.endDate = Math.floor(endDate.getTime() / 1000);

        this.chartData = this.generateDayChartData(this.startDate, this.endDate);
      } else if (this.selectedPeriod === 'year') {
        // 年视图的逻辑
        const currentDate = new Date(); // 当前日期（包含时间）

        // 计算12个月前的日期
        const startDate = new Date(currentDate.getFullYear(), currentDate.getMonth() - 11, 1); // 当前月往前推11个月，即一整年的起始月
        const endDate = new Date(currentDate.getFullYear(), currentDate.getMonth() + 1, 0); // 当前月的最后一天

        this.startDate = Math.floor(startDate.getTime() / 1000);
        this.endDate = Math.floor(endDate.getTime() / 1000);

        this.chartData = this.generateMonthChartData();
      }

      // console.log(this.startDate);
      // console.log(this.endDate);
    },

    generateMonthChartData() {
      this.totalUsage = 0;
      const result = [];

      // 获取当前日期和时间
      let currentDate = new Date();

      // 初始化一个对象，用来存储每个月的电量
      const monthlyUsage = {};

      // 遍历 powerData，按月累加用电量
      this.powerData.forEach(item => {
        const itemDate = new Date(item.time * 1000);

        // 过滤掉不在范围内的日期（从当前日期往前推12个月）
        if (itemDate <= currentDate && itemDate >= new Date(currentDate.getFullYear(), currentDate.getMonth(), currentDate.getDate() - 365)) {
          // 按“年-月”作为键值
          const yearMonth = `${itemDate.getFullYear()}-${itemDate.getMonth() + 1}`;

          if (!monthlyUsage[yearMonth]) {
            monthlyUsage[yearMonth] = {
              // 该月的第一天作为timestamp
              time: new Date(itemDate.getFullYear(), itemDate.getMonth(), 1).getTime() / 1000,
              data: 0
            };
          }
          monthlyUsage[yearMonth].data += item.data; // 累加该月的用电量
        }
      });

      // 逐月遍历，确保每个月都有数据
      let month = currentDate.getMonth(); // 当前月
      let year = currentDate.getFullYear(); // 当前年

      // 遍历过去12个月
      for (let i = 0; i < 12; i++) {
        const yearMonth = `${year}-${month + 1}`;

        if (!monthlyUsage[yearMonth]) {
          // 如果该月没有数据，填充为0
          monthlyUsage[yearMonth] = {
            time: new Date(year, month, 1).getTime() / 1000,
            data: 0
          };
        }

        // 移动到上一个月
        month--;
        if (month < 0) {
          month = 11;
          year--;
        }
      }

      // 将每个月的数据按格式存入 result 数组
      Object.values(monthlyUsage).forEach(monthData => {
        result.push({
          time: monthData.time,
          data: monthData.data.toFixed(1)
        });
        this.totalUsage += monthData.data; // 更新总用电量
      });

      return result;
    },

    generateDayChartData(startDate, endDate) {
      // console.log(startDate, endDate);
      this.totalUsage = 0;
      const result = [];

      let currentDate = new Date(startDate * 1000);
      while (currentDate.getTime() / 1000 <= endDate) {
        const time = Math.floor(currentDate.getTime() / 1000);

        const dataForDay = this.powerData.find(item => {
          const itemDate = new Date(item.time * 1000);
          return itemDate.toDateString() === currentDate.toDateString();
        });

        result.push({
          time,
          data: dataForDay ? dataForDay.data : 0,
        });
        this.totalUsage += dataForDay ? dataForDay.data : 0;

        currentDate.setDate(currentDate.getDate() + 1);
      }

      return result;
    },

    rowDataConvert(rowData) {
      // 检查 data 是否存在且为数组，如果为空或不存在，返回空数组
      if (!rowData.data || !Array.isArray(rowData.data)) {
        return [];
      }

      // 获取当天的结束时间戳（即当天的最后一秒）
      const endOfToday = new Date();
      endOfToday.setHours(23, 59, 59, 999);
      const endOfTodayTimestamp = Math.floor(endOfToday.getTime() / 1000);

      return rowData.data.map(item => {
        const fullTimestamp = item.time;
        const dataValue = fullTimestamp <= endOfTodayTimestamp ? parseFloat((item.data / 100).toFixed(2)) : 0; // 超过当天的电量设为0

        return {
          time: fullTimestamp,
          data: dataValue,
        };
      });
    },
    setLimitDate() {
      const now = new Date();
      const twelveMonthsAgo = new Date(now.setMonth(now.getMonth() - 11));
      const startOfMonth = new Date(twelveMonthsAgo.getFullYear(), twelveMonthsAgo.getMonth(), 1);
      // 转换为 Unix 时间戳
      this.minDate = Math.floor(startOfMonth.getTime() / 1000);
      this.maxDate = Math.floor(Date.now() / 1000);
      this.selectedDate = this.maxDate;
    },

    async fetchRowData() {
      try {
        const data = await fetchEnergyRowData();

        // 处理 rowData 并更新页面相关内容
        this.setLimitDate();
        this.powerData = this.rowDataConvert(data);
        this.updateDateRange();

      } catch (error) {
        console.error('获取 rowData 失败:', error);
      }
    },

    handleRealtimeDataUpdate(data) {
      this.todayUsage = data.eng_today_usage || 0;
      this.currentPower = data.power || 0;
      this.monthUsage = data.eng_month_usage || 0;
    }
  },

  mounted() {
    this.fetchRowData();

    getDeviceStatusOnce(this.handleRealtimeDataUpdate,
        {"query": ["eng_today_usage", "eng_month_usage", "power"]});

    startGetDeviceStatus(1000,
        this.handleRealtimeDataUpdate,
        {"query":["eng_today_usage", "eng_month_usage", "power"]});
  },
  beforeDestroy() {
    stopGetDeviceStatus();
  }
};
</script>

<style scoped>
.common-list-item {
  display: flex;
}

.gauge-chart-list {
  flex: 1;
  display: flex;
  justify-content: space-around;
}

.energy-usage-value {
  font-size: 26px;
  font-weight: bold;
  font-family: "Arial Rounded MT Bold", "Lato", "proxima-nova", "Helvetica Neue", Arial, sans-serif; /* 圆润字体 */
}

.stats-item {
  font-size: 16px;
}

.stats-item-left {
  flex: 1;
  display: flex;
  justify-content: flex-start; /* 靠左对齐 */
}

.period-style {
  border-bottom: none;
  margin-bottom: -25px;
}

.option-item-list {
  margin: 5px 0 0 0 ;
  display: flex;
  justify-content: space-around; /* 使子元素在水平方向上均匀分布 */
  align-items: center;
  flex: 1;

  width: 100%;
  height: 30px;
  border-radius: 7px;

  background: #e3e3e3;
}

.option-item {
  display: flex; /* 添加flex布局 */
  justify-content: center; /* 水平方向居中 */
  text-align: center;
  align-items: center;

  font-size: 14px;
  width: 31.7%;
  height: 25px;

  border-radius: 5px;
}

.option-item.selected {
  background: #f9f9f9; /* 选中时的高亮颜色 */
}

.stats-label {
  font-size: 14px;
  color: #747474;
  margin-left: 3px;
  margin-top: 12px;
}

.vertical-line {
  height: 20px;
  width: 1px;
  background-color: #ddd;
  align-self: center;
  margin: 0 20px;
}

.chart-container {
  margin-top: 5px;
  padding: 1px;
}

.date-picker-container {
  position: fixed;
  bottom: 0;
  left: 0;
  width: 100%;
  height: 400px; /* 占据屏幕下半部分 */
  background-color: white;
  box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.2);
  z-index: 1000; /* 确保在最前面显示 */
}

.stats-value {
  display: inline-flex;
  align-items: center; /* 垂直方向居中 */
  justify-content: flex-start;
}

.total-period {
  font-size: 14px;
  color: #747474;
  display: flex;
  justify-content: space-between; /* 在同一行中分布左右两侧 */
  align-items: center; /* 垂直方向居中对齐 */
  width: 100%; /* 确保宽度填满父容器 */

  margin: 15px 2px 5px 2px;
}

.icon {
  width: 16px;
  height: 16px;
  margin-left: 1px;
  transition: transform 0.5s ease;
}

.icon.rotated {
  transform: rotate(180deg); /* 旋转图标 */
}

</style>
