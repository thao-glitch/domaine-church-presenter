import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  base: '/domaine-church-presenter/',
  plugins: [react()],
  build: {
    outDir: '../docs',
    emptyOutDir: true,
    assetsDir: 'assets',
    chunkSizeWarningLimit: 1200
  },
  server: {
    port: 5173
  }
});