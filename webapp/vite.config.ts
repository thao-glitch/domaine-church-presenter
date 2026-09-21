import { fileURLToPath } from 'node:url';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const page = (p: string) => fileURLToPath(new URL(p, import.meta.url));

export default defineConfig({
  base: '/domaine-church-presenter/',
  plugins: [react()],
  build: {
    outDir: '../docs',
    emptyOutDir: true,
    assetsDir: 'assets',
    chunkSizeWarningLimit: 1200,
    rollupOptions: {
      input: {
        launcher: page('./index.html'),
        members: page('./members/index.html'),
        church: page('./church/index.html'),
        admin: page('./admin/index.html')
      }
    }
  },
  server: {
    port: 5173
  }
});
