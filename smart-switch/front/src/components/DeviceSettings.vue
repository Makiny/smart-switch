<!--@author kaiyin-->

<template>
    <!-- 页头部分 -->
    <div class="page-header">
      <h3 class="public-page-title">功能设置</h3>
      <button @click="handleBack" class="public-back-button">返回</button>
    </div>

    <!-- 页面内容部分 -->
    <div class="top-container">
      <SettingsGroup
          :items="[
        {
          label: '开关模式',
          value: this.switchMode===1 ? '保持' : '延时关闭',
          onClick: () => this.navigateToSwitchSettings(),
          type: 'text'
        }
      ]"
      />

      <SettingsGroup
          :items="[
        {
          label: '功率保护',
          value: this.pwrPro,
          onClick: () => {},
          type: 'switch',
          onChange: this.handlePwrProChange
        },
        {
          label: '功率设置',
          value: `${this.pwrThr}W`,
          onClick: () => this.navigateToSubSettings('power-thr-setting'),
          type: 'text'
        }
      ]"
          :note="`开启后，负载功率超过 ${this.pwrThr}W 时断电保护。`"
      />

      <SettingsGroup
          :items="[
        {
          label: '温度保护',
          value: this.tmpPro,
          onClick: () => {},
          type: 'switch',
          onChange: this.handleTmpProChange
        },
        {
          label: '温度设置',
          value: `${this.tmpThr}℃`,
          onClick: () => this.navigateToSubSettings('tmp-thr-setting'),
          type: 'text'
        }
      ]"
          :note="`开启后，设备温度超过 ${this.tmpThr}℃ 时断电保护。`"
      />

      <SettingsGroup
          :items="[
        {
          label: '设备重置',
          value: '',
          onClick: () => this.navigateToReset(),
          type: 'text'
        }

      ]"
      />
    </div>
</template>

<script>
import '../styles/page-common-styles.css';

import SettingsGroup from './SettingsGroup.vue';

import {getConfig, updateConfig} from '../api/apiService.js';
export default {
  components: {
    SettingsGroup
  },
  data() {
    return {
      switchMode: 1,       // 开关模式
      switchDelayOffTime: 0,
      pwrPro: false,    // 功率保护
      pwrThr: 0,     // 功率设置
      tmpPro: false,
      tmpThr: 0,
    };
  },
  methods: {
    handleBack() {
      this.$router.push('/'); // 返回主页或之前的页面
    },

    navigateToSubSettings(type) {
      // 根据类型导航到不同的设置页面
      this.$router.push({ name: 'DeviceSubSettings', params: { type } });
    },

    navigateToSwitchSettings() {
      this.$router.push('/settings/switch')
    },

    navigateToReset() {
      this.$router.push('/settings/reset')
    },

    async handlePwrProChange(newValue) {
      this.pwrPro = newValue;

      this.$store.dispatch('updateDeviceSettings', {
        pwrPro: this.pwrPro,
      });

      let res = await updateConfig({
        pwr_pro: this.pwrPro ? 1 : 0,
      });
    },

    async handleTmpProChange(newValue) {
      this.tmpPro = newValue;

      this.$store.dispatch('updateDeviceSettings', {
        tmpPro: this.tmpPro,
      });

      let res = await updateConfig({
        tmp_pro: this.tmpPro ? 1 : 0,
      });
    },

    async getDeviceConfig() {
      let data = await getConfig();
      console.log(data)

      this.switchMode = data.sw_mode;

      this.switchDelayOffTime = data.sw_delay_time;

      this.pwrPro = data.pwr_pro === 1;
      this.pwrThr = data.pwr_pro_thr;

      this.tmpPro = data.tmp_pro === 1;
      this.tmpThr = data.tmp_pro_thr;

      this.$store.dispatch('updateDeviceSettings', {
        lastUpdated: new Date().toISOString(),

        switchMode: this.switchMode,
        switchDelayOffTime: this.switchDelayOffTime,
        pwrPro: this.pwrPro,
        pwrThr: this.pwrThr,
        tmpPro: this.tmpPro,
        tmpThr: this.tmpThr,
      });
    },
  },

  mounted() {
    const configLastUpdated = this.$store.getters.getLastUpdated;
    if(!configLastUpdated || (Date.now() - new Date(configLastUpdated).getTime() > 180000)) {
      this.getDeviceConfig();
      return;
    }

    let switchMode = this.$store.getters.getSwitchMode;
    if(switchMode) {
      this.switchMode = switchMode;
    }

    let switchDelayOffTime = this.$store.getters.getSwitchDelayOffTime;
    if(switchDelayOffTime) {
      this.switchDelayOffTime = switchDelayOffTime;
    }

    let pwrPro = this.$store.getters.getPwrPro;
    if(pwrPro) {
      this.pwrPro = pwrPro;
    }
    let pwrThr = this.$store.getters.getPwrThr;
    if(pwrThr) {
      this.pwrThr = pwrThr;
    }

    let tmpPro = this.$store.getters.getTmpPro;
    if(tmpPro) {
      this.tmpPro = tmpPro;
    }

    let tmpThr = this.$store.getters.getTmpThr;
    if(tmpThr) {
      this.tmpThr = tmpThr;
    }
  }
};
</script>

<style scoped>

</style>
