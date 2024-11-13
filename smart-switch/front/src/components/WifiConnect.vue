<!--@author kaiyin-->

<template>
  <div class="page-header">
    <button :disabled="isConnecting" @click="handleBack" class="public-back-button">返回</button>
    <button :disabled="password.length < 8 || isConnecting" @click="connect" class="public-save-button">连接</button>
    <h3 class="public-page-title">输入Wi-Fi密码</h3>
  </div>
  <p class="public-page-title-note">
    <span class="note-prefix">连接到</span>
    <span class="note-ssid">{{ ssid }}</span>
  </p>
  <div class="top-container">
    <div class="input-style">
      <input type="password" v-model="password" placeholder="输入密码" required />
    </div>
    <div class="connect-status-container">
      <div :class="statusClass" id="status">
        {{ statusMessage }}
        <span v-if="isConnecting" class="loading-icon"></span>
      </div>
    </div>
  </div>
</template>

<script>
import { useStore } from 'vuex';
import { onMounted } from 'vue';
import { EventBus } from '@/eventBus';

import {connectWifiByCred} from '../api/apiService.js';
export default {
  props: ['ssid'],
  data() {
    return {
      password: '',
      statusMessage: '',
      connectStatus: '',
      isConnecting: false,
      pollingInterval: null,
    };
  },
  setup() {
    const store = useStore();

    onMounted(() => {
      const selectedWifi = store.state.selectedWifi;
      if (selectedWifi) {
        // 处理选中的 Wi-Fi
      }
    });

    return {
      selectedWifi: store.state.selectedWifi
    };
  },
  computed: {
    statusClass() {
      if (this.connectStatus === 'connecting') {
        return 'status-info'; // 修改：连接过程中状态类设置为 `status-info`
      }
      if (this.connectStatus === 'connected') {
        return 'status-success'; // 修改：连接成功时状态类设置为 `status-success`
      }
      if (this.connectStatus === 'error') {
        return 'status-error'; // 修改：连接失败时状态类设置为 `status-error`
      }
      return ''; // 修改：默认状态下返回空字符串，避免不必要的样式
    },
  },
  methods: {
    async connect() {
      this.statusMessage = '连接中...';
      this.connectStatus = 'connecting';
      this.isConnecting = true;
      const rsp = await connectWifiByCred(this.ssid, this.password);
      if (rsp && rsp.status !== 'OK') {
        return;
      }

      // 开始轮询
      this.startPolling();
    },
    startPolling() {
      if (this.pollingInterval) {
        clearInterval(this.pollingInterval);
      }

      this.pollingInterval = setInterval(() => {
        fetch(`/api/wifi?action=status`, {
          method: 'POST'
        })
            .then((response) => response.json())
            .then((status) => {
                if (status.status === 'connected') {
                  clearInterval(this.pollingInterval);
                  this.statusMessage = '连接成功';
                  this.isConnecting = false;
                  this.connectStatus = 'connected';

                  this.onConnectSuccess();
                } else if (status.status === 'error') {
                  clearInterval(this.pollingInterval);
                  this.statusMessage = this.translateErrorReason(status.reason);
                  this.isConnecting = false;
                  this.connectStatus = 'error';
                }
            })
            .catch((error) => {
              console.error('获取配置信息状态错误:', error);
              this.statusMessage = '状态: 获取状态失败。';
            });
      }, 3000);
    },
    translateErrorReason(reason) {
      switch (reason) {
        case 'auth_error':
          return '密码错误，请重试';
        case 'ap_not_found':
          return '找不到 Wi-Fi，请确认网络是否存在';
        default:
          return '连接失败，请重试';
      }
    },
    onConnectSuccess() {
      EventBus.emit('wifi-connected', this.ssid);
      this.$router.push('/wifi');
    },
    handleBack() {
      this.$router.push('/wifi');
    },
  },
};
</script>

<style scoped>

.top-container {
  margin-top: 0;
  /*width: 100%;*/
}

.note-prefix {
  margin-right: 5px; /* 给“连接到”与SSID之间留一些空间 */
  flex-shrink: 0; /* 防止“连接到”被压缩 */
}

.note-ssid {
  overflow: hidden; /* 超出部分隐藏 */
  text-overflow: ellipsis; /* 超出部分显示为省略号 */
  white-space: nowrap; /* 禁止SSID文本换行 */
  display: inline-block;
}

/*确保前缀、ssid行高一样*/
.note-prefix, .note-ssid {
  vertical-align: middle;
  line-height: 1.5; /* 根据需要调整行高 */
}

input[type="password"] {
  width: calc(100% - 20px);
  padding: 10px 10px;
  margin-top: 0;
  border-radius: 10px;
  border: 0 solid #ddd;
  font-size: 16px;
}

.input-style {
  display: flex; /* 添加flex布局 */
  justify-content: center; /* 水平方向居中 */
}

/* 新增连接状态样式 */
.connect-status-container {
  margin: 5px 0 0 10px;
  font-size: 16px;
  color: #333;
  display: flex;
  align-items: center;
  width: 100%;
  text-align: center;
}

#status {
  margin-top: 1px;
  font-size: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.status-success {
  color: green;
}

.status-error {
  color: red;
}

.status-info {
  color: black;
}

</style>
