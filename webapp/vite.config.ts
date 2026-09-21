import { fileURLToPath } from 'node:url';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const page = (p: string) => fileURLToPath(new URL(p, import.meta.url));

// Each app is deployed to its own GitHub Pages repo (own "project").
const REPO_OF = {
  members: 'domaine-church-members',
  church: 'domaine-church-console',
  admin: 'domaine-church-admin'
} as const;

const app = (process.env.APP || '') as keyof typeof REPO_OF;

export default defineConfig({
  // Standalone apps get their own site path; the root build serves the
  // Members app directly in this repo at /domaine-church-presenter/.
  base: app ? `/${REPO_OF[app]}/` : '/domaine-church-presenter/',
  plugins: [react()],
  build: {
    outDir: app ? `dist-${app}` : '../docs',
    emptyOutDir: true,
    assetsDir: 'assets',
    chunkSizeWarningLimit: 1200,
    rollupOptions: app
      ? { input: { [app]: page(`./${app}/index.html`) } }
      : {
          input: {
            members: page('./index.html'),
            'members-sub': page('./members/index.html'),
            church: page('./church/index.html'),
            admin: page('./admin/index.html')
          }
        }
  },
  server: {
    port: 5173
  }
});