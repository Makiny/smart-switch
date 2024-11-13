/**
 * @author kaiyin
 */

import { createStore } from 'vuex';

export default createStore({
    state: {
        wifiConnectState: {
            selectedWifi: null,
            connectedWifi: null,
        },
        deviceSettings: {
            lastUpdated: null,
            switchMode: null,
            switchDelayOffTime: null,
            pwrPro: null,
            pwrThr: null,
            tmpPro: null,
            tmpThr: null,
        }
    },
    mutations: {
        // 更新 Wi-Fi 状态相关 mutations
        updateSelectedWifi(state, wifi) {
            state.wifiConnectState.selectedWifi = wifi;
        },
        clearSelectedWifi(state) {
            state.wifiConnectState.selectedWifi = null;
        },
        updateConnectedWifi(state, wifi) {
            state.wifiConnectState.connectedWifi = wifi;
        },
        clearConnectedWifi(state) {
            state.wifiConnectState.connectedWifi = null;
        },

        // 更新设备设置相关 mutations
        updateDeviceSettings(state, settings) {
            Object.keys(settings).forEach(key => {
                if (state.deviceSettings.hasOwnProperty(key)) {
                    state.deviceSettings[key] = settings[key];
                }
            });
        },
        resetDeviceSettings(state) {
            state.deviceSettings.switchMode = null;
            state.deviceSettings.switchDelayOffTime = null;
            state.deviceSettings.pwrPro = null;
            state.deviceSettings.pwrThr = null;
            state.deviceSettings.tmpPro = null;
            state.deviceSettings.tmpThr = null;
        }
    },
    actions: {
        // 触发 Wi-Fi 状态相关 actions
        updateSelectedWifi({ commit }, wifi) {
            commit('updateSelectedWifi', wifi);
        },
        clearSelectedWifi({ commit }) {
            commit('clearSelectedWifi');
        },
        updateConnectedWifi({ commit }, wifi) {
            commit('updateConnectedWifi', wifi);
        },
        clearConnectedWifi({ commit }) {
            commit('clearConnectedWifi');
        },

        // 触发设备设置相关 actions
        updateDeviceSettings({ commit }, settings) {
            commit('updateDeviceSettings', settings);
        },
        resetDeviceSettings({ commit }) {
            commit('resetDeviceSettings');
        }
    },
    getters: {
        // 获取 Wi-Fi 状态相关 getters
        getSelectedWifi: (state) => state.wifiConnectState.selectedWifi,
        getConnectedWifi: (state) => state.wifiConnectState.connectedWifi,

        // 获取设备设置相关 getters
        getLastUpdated: (state) => state.deviceSettings.lastUpdated,
        getSwitchMode: (state) => state.deviceSettings.switchMode,
        getSwitchDelayOffTime: (state) => state.deviceSettings.switchDelayOffTime,
        getPwrPro: (state) => state.deviceSettings.pwrPro,
        getPwrThr: (state) => state.deviceSettings.pwrThr,
        getTmpPro: (state) => state.deviceSettings.tmpPro,
        getTmpThr: (state) => state.deviceSettings.tmpThr,
    },
});
