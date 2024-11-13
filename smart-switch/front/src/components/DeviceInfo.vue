<!--@author kaiyin-->

<template>
  <!-- 页头部分 -->
  <div class="page-header">
    <button @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">设备信息</h3>
  </div>

  <!-- 页面内容部分 -->
  <div class="top-container">
    <SettingsGroup
        :items="[
            { label: '设备型号', value: this.model, type: 'custom', customElement: 'custom-text' },
      ]"
    >
    </SettingsGroup>

    <SettingsGroup
        :items="[
            { label: '硬件版本', value: this.hardwareVersion, type: 'custom', customElement: 'custom-text' },
            { label: '固件版本', value: this.firmwareVersion, type: 'custom', customElement: 'custom-text' }
      ]"
    >
    </SettingsGroup>

    <SettingsGroup
        :items="[
            { label: '运行时长', value: this.runtime, type: 'custom', customElement: 'custom-text' },
      ]"
    >
    </SettingsGroup>
  </div>
</template>

<script>
import '../styles/page-common-styles.css';
import '../styles/loading-icon.css';

import SettingsGroup from "./SettingsGroup.vue";

import {getDeviceInfo} from '../api/apiService.js';
export default {
  components: {
    SettingsGroup,
  },
  data() {
    return {
      model: "未知",
      hardwareVersion: "未知",
      firmwareVersion: "未知",
      runtime: "未知",
    };
  },
  methods: {
    handleBack() {
      this.$router.go(-1); // 返回上一级页面
    },
    async queryDeviceInfo() {
      const data = await getDeviceInfo();
      if(!data) {
        return;
      }
      this.model = data.model;
      this.hardwareVersion = data.hw_v;
      this.firmwareVersion = data.fw_v;
      this.runtime = (data.runtime / 86400).toFixed(0) + ' 天';
    }
  },
  mounted() {
    this.queryDeviceInfo();
  }
};
</script>

<style scoped>

</style>
