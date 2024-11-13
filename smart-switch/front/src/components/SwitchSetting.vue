<!--@author kaiyin-->

<template>
  <!-- 页头部分 -->
  <div class="page-header">
    <button @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">控制模式</h3>
  </div>

  <!-- 页面内容部分 -->
  <div class="top-container">
    <div class="common-list">
      <div
          v-for="(option, index) in ctrlModeOptions"
          :key="index"
          class="common-list-item"
          @click="selectCtrlModeOption(option)"
      >
        <span v-if="option === selectedCtrlModeName" class="checkmark-right"></span>
        {{ option }}
      </div>
    </div>
    <div class="group-note">
      <span v-if="selectedCtrlModeName==='保持'">普通开关</span>
      <span v-if="selectedCtrlModeName==='延时关闭'">开关打开后，延时 {{selectedDelayItem}} 秒自动关闭</span>
    </div>
    <div v-if="selectedCtrlModeName === '延时关闭'" class="common-list">
      <div
          v-for="(option, index) in delayItems"
          :key="index"
          class="common-list-item"
          @click="selectDelaItem(option)"
      >
        <span v-if="option === selectedDelayItem" class="checkmark-right"></span>
        {{ option }}
        秒
      </div>
    </div>

  </div>
</template>

<script>
import '../styles/page-common-styles.css';

import {updateConfig} from '../api/apiService.js';
import SettingsGroup from "./SettingsGroup.vue";

export default {
  components: {
    SettingsGroup,
  },
  data() {
    return {
      title: '',
      ctrlModeOptions: [],
      delayItems:[],
      selectedCtrlModeName: '',
      selectedDelayItem: 0,
    };
  },
  methods: {
    handleBack() {
      this.$router.go(-1); // 返回上一级页面
    },
    async selectCtrlModeOption(option) {
      this.selectedCtrlModeName = option;
      let value;
      switch (option) {
        case '保持':
          value = 1;
          break
        case '延时关闭':
          value = 2;
          break;
        default:
          value = 1;
      }

      this.$store.dispatch('updateDeviceSettings', {
        switchMode: value,
      });

      let res = await updateConfig({
        sw_mode: value,
      });
      console.log("selectedOption = " + option);
    },
    async selectDelaItem(option) {
      this.selectedDelayItem = option;

      this.$store.dispatch('updateDeviceSettings', {
        switchDelayOffTime: option,
      });

      let res = await updateConfig({
        sw_delay_time: option,
      });
      console.log("selectedDelayItem = " + option);
    },
  },
  created() {
    // 根据路由参数设置页面内容
    this.ctrlModeOptions = ['保持', '延时关闭'];
    this.delayItems = [3,5,10,30,60];
  },
  mounted() {
    let selectedCtrlModeName = this.$store.getters.getSwitchMode;
    if(selectedCtrlModeName) {
      this.selectedCtrlModeName = selectedCtrlModeName===1 ? '保持' : '延时关闭';
    }
    let selectedDelayItem = this.$store.getters.getSwitchDelayOffTime;
    if(selectedDelayItem) {
      this.selectedDelayItem = selectedDelayItem;
    }
  }
};
</script>

<style scoped>

.common-list-item {
  position: relative;
}

</style>
