import { createApp } from 'vue';
import App from './App.vue';
import router from './router';
import './styles/list-styles.css'
import './styles/loading-icon.css'

import store from './store';

const app = createApp(App);
app.use(router);
app.use(store);
app.mount('#app');
