<!--@author kaiyin-->

<template>
  <div class="page-header">
    <button @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">选择设备要连接的Wi-Fi</h3>
  </div>
  <p class="public-page-title-note">
    可用Wi-Fi
    <span class="loading-icon" v-if="isLoading"></span>
  </p>
  <div class="top-container">
    <div class="common-list">
      <div class="common-list-item" v-for="network in networks" :key="network.ssid" @click="selectNetwork(network)" >
        <span v-if="connectedWifi && connectedWifi.ssid === network.ssid" class="checkmark-left"></span>
        <span class="wifi-ssid">{{ network.ssid }}</span>
        <span :class="getSignalClass(network)" class="signal-icon"></span>
      </div>
    </div>
  </div>
</template>

<script>
import '../styles/page-common-styles.css';

import { EventBus } from '@/eventBus';

import {getWifiList} from '../api/apiService.js';
export default {
  data() {
    return {
      isLoading: false,
      networks: [],
      connectedWifi: null, // 保存已连接的 Wi-Fi
      selectedWifi: null,
    };
  },

  methods: {
    async fetchNetworks() {
      this.isLoading = true;

      this.connectedWifi = this.$store.getters.getConnectedWifi;
      if (this.connectedWifi != null) {
        this.networks.push(this.connectedWifi);
      }
      try {
        const data = await getWifiList();
        this.isLoading = false;

        let connectedWifi = null;
        if(this.connectedWifi) {
          connectedWifi = data.find(network => network.bssid === this.connectedWifi.bssid);
          this.networks = data.filter(network => network.bssid !== this.connectedWifi.bssid);
        } else {
          this.networks = data;
        }
        this.networks.sort((a, b) => b.rssi - a.rssi);

        if (connectedWifi) {
          this.connectedWifi = connectedWifi;
          this.$store.dispatch('updateConnectedWifi', this.connectedWifi);
          this.networks.unshift(this.connectedWifi);
        }
      } catch (error) {
        console.error('获取网络错误:', error);
        this.networks = [];  // 可以在此设置错误状态提示
        this.isLoading = false;
      }
    },

    selectNetwork(network) {
      if(this.connectedWifi && network.bssid === this.connectedWifi.bssid) {
        return;
      }
      this.$store.dispatch('updateSelectedWifi', network);
      this.$router.push(`/connect/${network.ssid}`); // 导航到WifiConnect页面
    },
    handleBack() {
      this.$router.push('/');
    },
    getSignalClass(network) {
      const { rssi, auth } = network;
      if (rssi > -60) {
        return auth ? 'signal-strong-lock' : 'signal-strong';
      } else if (rssi > -70) {
        return auth ? 'signal-medium-lock' : 'signal-medium';
      } else if (rssi > -80) {
        return auth ? 'signal-weak-lock' : 'signal-weak';
      } else {
        return auth ? 'signal-very-weak-lock' : 'signal-very-weak';
      }
    },

    updateConnectedWifi(network) {
      this.connectedWifi = this.$store.getters.getSelectedWifi;
      this.connectedWifi.connected = 1;
      this.$store.dispatch('updateConnectedWifi', this.connectedWifi);
    },
  },
  mounted() {
    this.fetchNetworks();
    EventBus.on('wifi-connected', this.updateConnectedWifi);
  },
  beforeDestroy() {
    // 移除事件监听器
    EventBus.off('wifi-connected', this.updateConnectedWifi);
  },
};
</script>

<style scoped>

.public-page-title-note {
  /*margin-top: 60px;*/
}

.top-container {
  margin-top: 0;
}

.loading-icon {
  width: 10px;
  height: 10px;
  margin-left: 5px;
}

.wifi-ssid {
  flex-grow: 1;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.signal-icon {
  flex-shrink: 0;
  width: 16px;
  height: 16px;
  background-size: contain;
  background-repeat: no-repeat;
  margin-left: 10px;
}

.common-list-item {
  margin-left: 40px;
  display: flex;
  align-items: center;
  position: relative;
}

.checkmark-left {
  left: -30px; /* 根据需要调整位置 */
}

.signal-very-weak-lock {
  background-image: url('@/assets/icons/wifi-strength-1-lock.svg');
}
.signal-very-weak {
  background-image: url('@/assets/icons/wifi-strength-1.svg');
}

.signal-weak-lock {
  background-image: url('@/assets/icons/wifi-strength-2-lock.svg');
}
.signal-weak {
  background-image: url('@/assets/icons/wifi-strength-2.svg');
}

.signal-medium-lock {
  background-image: url('@/assets/icons/wifi-strength-3-lock.svg');
}
.signal-medium {
  background-image: url('@/assets/icons/wifi-strength-3.svg');
}

.signal-strong-lock {
  background-image: url('@/assets/icons/wifi-strength-4-lock.svg');
}
.signal-strong {
  background-image: url('@/assets/icons/wifi-strength-4.svg');
}

</style>
