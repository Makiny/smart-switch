<!--@author kaiyin-->

<template>
  <!-- 页头部分 -->
  <div class="page-header">
    <button :disabled="resetting" @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">设备重置</h3>
  </div>

  <!-- 页面内容部分 -->
  <div class="top-container">
    <div class="common-list">
      <div class="common-list-item" :class="{ disabled: resetting }" @click="showConfirm('resetNetwork')">重置网络</div>
    </div>
    <div class="group-note">
      重置后，需要重新配置网络。
    </div>

    <div class="common-list">
      <div class="common-list-item" :class="{ disabled: resetting }" @click="showConfirm('resetPlatform')">重置配对</div>
    </div>
    <div class="group-note">
      重置后，将清空设备网络配置，且会断开与所有智能家居平台的连接，并需要重新配置。
    </div>

    <div class="common-list">
      <div class="common-list-item" :class="{ disabled: resetting }" @click="showConfirm('resetToFactory')">恢复出厂设置</div>
    </div>
    <div class="group-note">
      恢复出厂设置将清空包括网络、用电统计、智能家居平台接入在内的所有数据。
    </div>

    <div v-if="showConfirmation" class="modal-overlay">
      <div class="modal">
        <p> {{ confirmActionText }} </p>
        <div class="modal-divider-line"></div>
        <div class="modal-footer">
          <div class="modal-button confirm" @click="confirmReset">确认</div>
          <div class="modal-divider"></div>
          <div class="modal-button cancel" @click="cancelReset">取消</div>
        </div>
      </div>
    </div>


    <div v-if="resetting" class="common-list">
      <div v-if="resetMode==='resetNetwork'" class="common-list-item">
        <div class="loading-container">
          正在重置网络
          <span class="loading-icon"></span>
        </div>
        <br>
        设备将自动重启并进入配置模式。请关闭此页面，待设备重启完成后，重新连接并配置网络。
      </div>
      <div v-if="resetMode==='resetPlatform'" class="common-list-item">
        <div class="loading-container">
          正在重置配对信息
          <span class="loading-icon"></span>
        </div>
        <br>
        设备将自动重启。请关闭此页面，待设备重启完成后，重新连接并配置设备。
      </div>
      <div v-if="resetMode==='resetToFactory'" class="common-list-item">
        <div class="loading-container">
          正在恢复出厂设置
          <span class="loading-icon"></span>
        </div>
        <br>
        设备将自动重启，记得一键三连喔~
      </div>
    </div>
  </div>
</template>

<script>
import '../styles/page-common-styles.css';
import '../styles/loading-icon.css';

import {resetDevice} from '../api/apiService.js';
export default {
  data() {
    return {
      resetting: false,
      resetMode: "",
      confirmAction: null, // 当前要执行的重置操作
      confirmActionText: "", // 用于显示在模态框中的操作名称
      showConfirmation: false, // 控制模态框显示
    };
  },
  methods: {
    handleBack() {
      this.$router.go(-1); // 返回上一级页面
    },
    showConfirm(action) {
      // 显示确认弹窗
      this.confirmAction = action;
      this.confirmActionText = action === "resetNetwork" ? "重置网络" : action === "resetPlatform" ? "重置配对" : "恢复出厂设置";
      this.showConfirmation = true;
    },
    async confirmReset() {
      this.showConfirmation = false;
      if (this.confirmAction === "resetNetwork") {
        await this.resetNetwork();
      } else if (this.confirmAction === "resetPlatform") {
        await this.resetPlatform();
      } else if (this.confirmAction === "resetToFactory") {
        await this.resetToFactory();
      }
    },
    cancelReset() {
      // 用户取消确认
      this.showConfirmation = false;
    },
    async resetNetwork() {
      this.resetMode = "resetNetwork"
      const rsp = await resetDevice("rstNet");
      if (rsp && rsp.status !== 'OK') {
        return;
      }
      this.resetting = true;
    },
    async resetPlatform() {
      this.resetMode = "resetPlatform"
      const rsp = await resetDevice("rstPlf");
      if (rsp && rsp.status !== 'OK') {
        return;
      }
      this.resetting = true;
    },
    async resetToFactory() {
      this.resetMode = "resetToFactory"
      const rsp = await resetDevice("rstFat");
      if (rsp && rsp.status !== 'OK') {
        return;
      }
      this.resetting = true;
    },
  },
};
</script>

<style scoped>

.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.4);
  display: flex;
  justify-content: center;
  align-items: center;
}

.modal {
  background-color: white;
  border-radius: 10px;
  width: 70%;
  text-align: center;
  overflow: hidden;
}

.modal p {
  margin-bottom: 5px;
  font-size: 16px;
}

.modal-footer {
  display: flex;
  height: 30%; /* 占据下三分之一的垂直空间 */
}

.modal-button {
  flex: 1; /* 每个区域占据一半的空间 */
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  height: 100%; /* 占满高度 */

  margin: 10px 0;

  font-size: 16px;
}

.modal-button.confirm {
  border-right: 1px solid #ccc; /* 右侧边框 */
}

.modal-divider {
  width: 1px; /* 线条宽度 */
  background-color: #ccc; /* 线条颜色 */
  height: 100%; /* 占满高度 */
}

.modal-divider-line {
  height: 1px; /* 分隔线的高度 */
  background-color: #ccc; /* 分隔线的颜色 */
  width: 100%; /* 让分隔线宽度占满模态框 */
  margin: 10px 0 0 0;
}

.loading-container {
  display: flex;
  align-items: center;
  justify-content: center
}

.loading-icon {
  margin-left: 5px;
  display: inline-block;
  width: 9px;
  height: 9px;
}

</style>
