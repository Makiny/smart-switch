<!--@author kaiyin-->

<template>
  <div class="common-list">
    <div v-for="(item, index) in items" :key="index" @click="item.onClick">
      <div class="settings-row">
        <label class="settings-label">{{ item.label }}</label>
        <template v-if="item.type === 'switch'">
          <label class="switch">
            <input type="checkbox" v-model="item.value" @change="handleSwitchChange(item)" />
            <span class="slider round"></span>
          </label>
        </template>
        <template v-else>
          <span class="settings-value">{{ item.value }}</span>
          <span v-if="!item.customElement" class="arrow-container">
            <img src="@/assets/icons/chevron-right.svg" alt="Chevron Right" class="arrow-icon" />
          </span>
          <span v-if="item.customElement" class="custom-element">
            <slot :name="item.customElement"></slot>
          </span>
        </template>
      </div>
      <div v-if="index < items.length - 1" class="divider"></div>
    </div>
  </div>
  <div v-if="note" class="group-note">
    {{ note }}
  </div>
</template>

<script>
import '../styles/toggle-switch.css';
export default {
  name: 'SettingsGroup',
  props: {
    items: {
      type: Array,
      required: true,
      default: () => []
    },
    note: {
      type: String,
      required: false,
      default: ''
    }
  },
  methods: {
    handleSwitchChange(item) {
      if (item.onChange) {
        item.onChange(item.value);
      }
    }
  }
};
</script>

<style scoped>
.common-list {
  padding: 0 20px 0 20px;
}

.divider {
  border-bottom: 1px solid #ddd;
}

.settings-row {
  padding: 10px 0;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.settings-label {
  font-size: 17px;
  flex: 1;
}

.settings-value {
  font-size: 17px;
  color: #747474;
}

.arrow-container {
  display: flex;
  align-items: center;
}

.arrow-icon {
  width: 16px;
  height: 16px;
  margin-left: 10px;
}

.custom-element {
  display: flex;
  align-items: center;
}

</style>
