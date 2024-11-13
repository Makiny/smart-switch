<!--@author kaiyin-->

<template>
  <div class="date-picker-page">
    <div class="date-picker-header">
      <p class="date-picker-title">选择{{ viewMode === 'month' ? '月份' : '日期' }}</p>
    </div>
    <div class="date-picker-body">
      <div class="calendar-header">
        <button v-if="viewMode === 'month'" @click="prevYear" class="nav-button" :disabled="isPrevYearDisabled">‹</button>
        <button v-else @click="prevMonth" class="nav-button" :disabled="isPrevMonthDisabled">‹</button>
            {{ viewMode === 'month' ? currentYear : `${currentYear}-${currentMonthPadded}` }}
        <button v-if="viewMode === 'month'" @click="nextYear" class="nav-button" :disabled="isNextYearDisabled">›</button>
        <button v-else @click="nextMonth" class="nav-button" :disabled="isNextMonthDisabled">›</button>
      </div>

      <div class="calendar">
        <!-- 日期选择 -->
        <div v-if="viewMode === 'week'" class="calendar-grid">
          <div
              v-for="day in daysOfMonth"
              :key="day.date"
              class="calendar-day"
              @click="selectDate(day.date)"
              :class="{
              'disabled': isDisabled(day.date),
              'selected': isSelected(day.date)
            }"
          >
            {{ day.day }}
          </div>
        </div>
        <!-- 月份选择 -->
        <div v-else class="month-grid">
          <div
              v-for="month in monthsOfYear"
              :key="month.date"
              class="calendar-month"
              @click="selectMonth(month.date)"
              :class="{
              'disabled': isMonthDisabled(month.date),
              'selected': isMonthSelected(month.date)
            }"
          >
            <div>{{ month.number }}</div>
            <div class="consumption">{{ getMonthlyConsumption(month.date) }}度</div> <!-- 新增用电数据显示 -->
          </div>
        </div>
      </div>
    </div>
  </div>
</template>



<script>
export default {
  props: {
    minDate: {
      type: Number,
      default: null,
    },
    maxDate: {
      type: Number,
      default: null,
    },
    initDate: {
      type: Number,
      default: null,
    },
    viewMode: {
      type: String,
      default: 'date', // 可以是 'date' 或 'month'
    },
    consumptionData: {
      type: Array,
      default: () => [],
    },
  },
  data() {
    return {
      currentDate: new Date(),
    };
  },
  computed: {
    minDateObject() {
      return this.minDate ? new Date(this.minDate * 1000) : null;
    },
    maxDateObject() {
      return this.maxDate ? new Date(this.maxDate * 1000) : null;
    },
    initObject() {
      return this.initDate ? new Date(this.initDate * 1000) : null;
    },
    currentMonth() {
      return this.currentDate.getMonth();
    },
    currentMonthPadded() {
      // 将月份数字格式化为两位数
      return String(this.currentMonth + 1).padStart(2, '0');
    },
    currentYear() {
      return this.currentDate.getFullYear();
    },
    currentMonthName() {
      return this.currentDate.toLocaleString('default', { month: 'long' });
    },
    daysOfMonth() {
      const days = [];
      const firstDay = new Date(this.currentYear, this.currentMonth, 1).getDay();
      const lastDate = new Date(this.currentYear, this.currentMonth + 1, 0).getDate();

      // Fill days before the first day of the month
      for (let i = 0; i < firstDay; i++) {
        days.push({ day: '', date: null });
      }

      // Fill days of the current month
      for (let i = 1; i <= lastDate; i++) {
        days.push({ day: i, date: new Date(this.currentYear, this.currentMonth, i) });
      }

      return days;
    },
    isPrevMonthDisabled() {
      const prevMonthDate = new Date(this.currentYear, this.currentMonth - 1, 1);
      const nextMonthDate = new Date(this.currentYear, this.currentMonth, 0);
      return !this.isMonthWithinRange(prevMonthDate, nextMonthDate);
    },
    isNextMonthDisabled() {
      const nextMonthDate = new Date(this.currentYear, this.currentMonth + 1, 1);
      const nextMonthEndDate = new Date(this.currentYear, this.currentMonth + 2, 0);
      return !this.isMonthWithinRange(nextMonthDate, nextMonthEndDate);
    },

    monthsOfYear() {
      const months = [];
      for (let i = 0; i < 12; i++) {
        months.push({
          number: i + 1, // 使用阿拉伯数字表示月份
          date: new Date(this.currentYear, i, 1)
        });
      }
      return months;
    },

    isPrevYearDisabled() {
      const prevYearDate = new Date(this.currentYear - 1, 0, 1);
      return this.minDateObject && prevYearDate < new Date(this.minDateObject.getFullYear(), 0, 1);
    },

    isNextYearDisabled() {
      const nextYearDate = new Date(this.currentYear + 1, 0, 1);
      return this.maxDateObject && nextYearDate > new Date(this.maxDateObject.getFullYear(), 11, 31);
    },
  },
  methods: {
    prevMonth() {
      if (!this.isPrevMonthDisabled) {
        this.currentDate = new Date(this.currentDate.setMonth(this.currentDate.getMonth() - 1));
      }
    },
    nextMonth() {
      if (!this.isNextMonthDisabled) {
        this.currentDate = new Date(this.currentDate.setMonth(this.currentDate.getMonth() + 1));
      }
    },
    selectDate(date) {
      this.$emit('update:selectedDate', Math.floor(date.getTime() / 1000)); // Emit selected date as Unix timestamp
      this.$emit('close');
    },
    isDisabled(date) {
      if (this.minDateObject && date < this.minDateObject) return true;
      if (this.maxDateObject && date > this.maxDateObject) return true;
      return false;
    },
    isSelected(date) {
        return date && this.initObject && this.initObject.toDateString() === date.toDateString();
    },
    isMonthWithinRange(startDate, endDate) {
      if (this.minDateObject && startDate < this.minDateObject) startDate = this.minDateObject;
      if (this.maxDateObject && endDate > this.maxDateObject) endDate = this.maxDateObject;
      return startDate <= endDate;
    },


    selectMonth(monthDate) {
      this.$emit('update:selectedDate', Math.floor(monthDate.getTime() / 1000)); // Emit selected month as Unix timestamp
      this.$emit('close');
    },

    isMonthDisabled(monthDate) {
      if (this.minDateObject && monthDate < new Date(this.minDateObject.getFullYear(), this.minDateObject.getMonth(), 1)) return true;
      if (this.maxDateObject && monthDate > new Date(this.maxDateObject.getFullYear(), this.maxDateObject.getMonth(), 1)) return true;
      return false;
    },

    isMonthSelected(monthDate) {
      return this.initObject && this.initObject.getFullYear() === monthDate.getFullYear() && this.initObject.getMonth() === monthDate.getMonth();
    },

    prevYear() {
      if (!this.isPrevYearDisabled) {
        this.currentDate = new Date(this.currentDate.setFullYear(this.currentDate.getFullYear() - 1));
      }
    },

    nextYear() {
      if (!this.isNextYearDisabled) {
        this.currentDate = new Date(this.currentDate.setFullYear(this.currentDate.getFullYear() + 1));
      }
    },

    getMonthlyConsumption(monthDate) {
      const monthStart = new Date(monthDate.getFullYear(), monthDate.getMonth(), 1).getTime() / 1000;
      const monthEnd = new Date(monthDate.getFullYear(), monthDate.getMonth() + 1, 1).getTime() / 1000;

      const totalConsumption = this.consumptionData.reduce((acc, data) => {
        const timestamp = parseInt(data.time, 10);
        if (timestamp >= monthStart && timestamp < monthEnd) {
          return acc + data.data;
        }
        return acc;
      }, 0);

      return Math.floor(totalConsumption);
    },
  },
};
</script>

<style scoped>
.date-picker-page {
  /*display: flex;*/
  /*flex-direction: column;*/
  /*height: 100%;*/
}

.date-picker-header {
  padding: 10px;
}

.date-picker-title {
  margin: 0;
  text-align: center;
  font-weight: bold;
}

.date-picker-body {
  flex: 1;
  overflow-y: auto;
}

.calendar {
  display: flex;
  flex-direction: column;
  align-items: center;
}

.calendar-header {
  display: flex;
  justify-content: space-between;
  padding: 10px;
}

.nav-button {
  background: transparent;
  border: none;
  font-size: 20px;
  cursor: pointer;
}

.nav-button:disabled {
  cursor: not-allowed;
  color: #ccc;
}

.calendar-grid {
  display: grid;
  grid-template-columns: repeat(7, 1fr);
  column-gap: 10px; /* 增加按钮间的间距 */
  row-gap: 8px;
  margin: 5px 0 0 0;
}

.calendar-day {
  display: flex;
  justify-content: center;
  align-items: center;
  text-align: center;
  width: 40px;
  height: 40px;
  border-radius: 50%;
  cursor: pointer;
  font-size: 18px;
}

.calendar-day:hover {
  background: #ddd;
}

.calendar-day.disabled {
  color: #ccc;
  pointer-events: none;
}

.calendar-day.selected {
  background: #ed7669;
  color: white;
}

/* 新增的月份网格样式 */
.month-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 40px; /* 调整间距 */
  margin: 10px 0 0 0 ;
}

.calendar-month {
  display: flex;
  flex-direction: column; /* 垂直排列 */
  justify-content: center;
  align-items: center;
  /*text-align: center;*/
  width: 50px;
  height: 50px;
  border-radius: 50%;
  cursor: pointer;
  font-size: 18px;
}

.calendar-month:hover {
  background: #ddd;
}

.calendar-month.disabled {
  color: #ccc;
  pointer-events: none;
}

.calendar-month.selected {
  background: #ed7669;
  color: white;
}

.consumption {
  font-size: 12px;
  margin-top: -2px;
}


</style>
