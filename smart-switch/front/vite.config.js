// vite.config.js
import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import path from 'path';
export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, 'src'),
    },
  },

  server: {
    host: '0.0.0.0',  // 监听所有网络接口
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://192.168.4.1',
        changeOrigin: true,
        secure: false,
        configure: (proxy, options) => {
          proxy.on('proxyReq', (proxyReq, req, res) => {
            // 只保留必要的请求头
            proxyReq.removeHeader('User-Agent');
            proxyReq.removeHeader('Accept-Encoding');
            proxyReq.removeHeader('Connection');
            // 添加或保留的请求头
            proxyReq.setHeader('Content-Type', 'application/json');
          });
        }
      },
    },
  },

  build: {
    rollupOptions: {
      output: {
        // 指定 JavaScript 文件名
        entryFileNames: 'index.js',  // 入口 JS 文件名
        chunkFileNames: '[name].js',  // 按需加载的 chunk 文件名
        assetFileNames: (assetInfo) => {
          // 根据不同类型的文件设置不同的文件名
          if (assetInfo.name.endsWith('.css')) {
            return 'index.css';  // 指定 CSS 文件名
          }
          return '[name].[ext]';  // 对其他类型的文件保持默认命名
        },

        // 取消 assets 文件夹，将文件输出到 dist 根目录
        dir: 'dist',
      },
    },
  },
});
