import { createRouter, createWebHistory } from 'vue-router';
import Home from './components/Home.vue';
import WifiList from './components/WifiList.vue';
import WifiConnect from './components/WifiConnect.vue';
import DeviceSettings from './components/DeviceSettings.vue'
import DeviceSubSettings from './components/DeviceSubSettings.vue';
import SwitchSetting from "./components/SwitchSetting.vue";
import ResetToFactory from "./components/DeviceReset.vue";
import EnergyStatistics from "./components/EnergyStatistics.vue";
import DeviceInfo from "./components/DeviceInfo.vue";

const routes = [
    { path: '/', component: Home },
    { path: '/wifi', component: WifiList },
    { path: '/connect/:ssid', component: WifiConnect, props: true },
    { path: '/settings', component: DeviceSettings },
    { path: '/settings/switch', component: SwitchSetting },
    { path: '/sub-settings/:type', name: 'DeviceSubSettings', component: DeviceSubSettings},
    { path: '/settings/reset', component: ResetToFactory },
    { path: '/energy-statistics', component: EnergyStatistics },
    { path: '/device-info', component: DeviceInfo },
];

const router = createRouter({
    history: createWebHistory(),
    routes,
});

export default router;
