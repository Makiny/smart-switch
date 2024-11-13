<!--@author kaiyin-->

<template>
  <div class="page-header">
    <h3 class="public-page-title">智能开关</h3>
  </div>

  <div class="top-container">

    <div class="common-list">
      <div class="common-list-item" @click="goToEnergyStatistics">
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

    <SettingsGroup
        :items="[
        {
          label: '开关',
          type: 'switch',
          value: this.swStatus,
          onClick: () => switchAction()
        }
      ]"
    />

    <SettingsGroup
        :items="[
        {
          label: '网络设置',
          value: this.connectedSsid,
          onClick: () => goToWifiConfig(),
          type: 'text',
        },
        {
          label: '功能设置',
          value: '',
          onClick: () => goToDeviceSettings(),
          type: 'text',
        }
      ]"
    />
    <SettingsGroup
        :items="[
        {
          label: '设备信息',
          value: '',
          onClick: () => goToDeviceInfo(),
          type: 'text',
        }
      ]"
    />

  </div>
</template>
<script>
import '../styles/page-common-styles.css';
import './SettingsGroup.vue'
import SettingsGroup from "./SettingsGroup.vue";

import GaugeChart from './GaugeChart.vue';

import powerIcon from '@/assets/icons/power.svg'; // 导入图标

import {startGetDeviceStatus, stopGetDeviceStatus, getDeviceStatusOnce, deviceControl} from '../api/apiService.js';

export default {
  components: {
    SettingsGroup,
    GaugeChart
  },

  data() {
    return {
      todayUsage: 0,
      monthUsage: 0,
      currentPower: 0,

      connectedSsid: '',

      swStatus: false,
      isSwitching: false,
    }
  },

  methods: {
    async switchAction() {
      if(this.isSwitching) {
        return;
      }

      this.isSwitching = true;
      let swStatus = this.swStatus ? 0 : 1;
      const rsp = await deviceControl(swStatus);
      this.isSwitching = false;

      if (rsp && rsp.status !== 'OK') {
        return;
      }

      this.swStatus = swStatus === 1;
    },
    goToDeviceInfo() {
      this.$router.push('/device-info');
    },
    goToWifiConfig() {
      this.$router.push('/wifi');
    },
    goToDeviceSettings() {
      this.$router.push('/settings')
    },
    goToEnergyStatistics() {
      this.$router.push('/energy-statistics')
    },
    handleRealtimeDataUpdate(data) {
      this.swStatus = (data.sw_status || 0) === 1;

      this.todayUsage = data.eng_today_usage || 0;
      this.currentPower = data.power || 0;
      this.monthUsage = data.eng_month_usage || 0;

      if(data.wifi_con) {
        this.connectedSsid = data.wifi_con.ssid || '';
        if(this.connectedSsid === '') {
          this.connectedSsid = '未连接'
          return;
        }
        let connectedWifi = {};
        connectedWifi.ssid = data.wifi_con.ssid;
        connectedWifi.bssid = data.wifi_con.bssid;
        connectedWifi.rssi = data.wifi_con.rssi;
        connectedWifi.auth = data.wifi_con.auth;
        connectedWifi.connected = 1;
        this.$store.dispatch('updateConnectedWifi', connectedWifi);
      }
    }
  },

  mounted() {

    let connectedWifi = this.$store.getters.getConnectedWifi;
    if(!connectedWifi) {
      getDeviceStatusOnce(this.handleRealtimeDataUpdate,
          {"query": ["sw_status", "wifi_con", "eng_today_usage", "eng_month_usage", "power"]});
    } else {
      this.connectedSsid = connectedWifi.ssid;
      getDeviceStatusOnce(this.handleRealtimeDataUpdate,
          {"query": ["sw_status", "eng_today_usage", "eng_month_usage", "power"]});
    }

    startGetDeviceStatus(1000,
        this.handleRealtimeDataUpdate,
        {"query": ["sw_status","eng_today_usage", "eng_month_usage", "power"]});
  },

  beforeDestroy() {
    stopGetDeviceStatus();
  },

  beforeRouteLeave(to, from, next) {
    stopGetDeviceStatus();
    next();
  },
};
</script>

<style scoped>

.gauge-chart-list {
  flex: 1;
  padding: 0 2% 0 2%;
  display: flex;
  justify-content: space-around;
}

.common-list-item {
  display: flex;
}

ul {
  list-style: none;
  padding: 0;
}

</style>
