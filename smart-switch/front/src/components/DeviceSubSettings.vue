<!--@author kaiyin-->

<template>
  <!-- 页头部分 -->
  <div class="page-header">
    <button @click="handleBack" class="public-back-button">返回</button>
    <h3 class="public-page-title">{{ title }}</h3>
  </div>

  <!-- 页面内容部分 -->
  <div class="top-container">
    <div class="common-list">
      <div
          v-for="(option, index) in options"
          :key="index"
          class="common-list-item"
          @click="selectOption(option)"
      >
        <span v-if="option === selectedOption" class="checkmark-right"></span>
        {{ option }}
        <span v-if="pageIndex==='1'" >W</span>
        <span v-if="pageIndex==='2'" >℃</span>
      </div>
    </div>
    <span class="group-note" v-if="pageIndex==='1'" >当负载功率超过 {{selectedOption}}W 时断电保护。</span>
    <span class="group-note" v-if="pageIndex==='2'" >当设备温度超过 {{selectedOption}}℃ 时断电保护。</span>
  </div>
</template>

<script>
import '../styles/page-common-styles.css';
import {updateConfig} from "../api/apiService";
const PageIndex = {
  PWR_PRO_THR: '1',
  TMP_PRO_THR: '2',
};

export default {
  data() {
    return {
      title: '',
      options: [],
      pageIndex: '',
      selectedOption: null,
    };
  },
  mounted() {
    // 根据路由参数设置页面内容
    const { type } = this.$route.params;
    switch (type) {
      case 'power-thr-setting':
        this.pageIndex = PageIndex.PWR_PRO_THR;
        this.title = '功率设置';
        this.options = [35, 2200, 2300, 2400, 2500];

        let pwrThr = this.$store.getters.getPwrThr;
        if(pwrThr) {
          this.selectedOption = pwrThr;
        }
        break;
      case 'tmp-thr-setting':
        this.pageIndex = PageIndex.TMP_PRO_THR;
        this.title = '温度设置';
        this.options = [50, 60, 70, 80];

        let tmpThr = this.$store.getters.getTmpThr;

        if(tmpThr) {
          this.selectedOption = tmpThr;
        }
        break;
      default:
        break;
    }
  },
  methods: {
    handleBack() {
      this.$router.go(-1); // 返回上一级页面
    },
    async selectOption(option) {
      this.selectedOption = option;
      console.log("selectedOption = " + option);
      let res
      switch (this.pageIndex) {
        case PageIndex.PWR_PRO_THR:
          this.$store.dispatch('updateDeviceSettings', {
            pwrThr: option,
          });

          res = await updateConfig({
            pwr_pro_thr: option,
          });
          break;
        case PageIndex.TMP_PRO_THR:
          this.$store.dispatch('updateDeviceSettings', {
            tmpThr: option,
          });

          res = await updateConfig({
            tmp_pro_thr: option,
          });
          break
        default:
          break;
      }
    }
  }
};
</script>

<style scoped>

.common-list-item {
  position: relative;
}

</style>
