// Copies the Supabase schema/LiveKit docs into the published docs/
// folder after every Vite build (emptyOutDir would otherwise delete them).
import { cpSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const out = join(root, '..', 'docs');

mkdirSync(join(out, 'supabase'), { recursive: true });

cpSync(join(root, 'supabase', 'schema.sql'), join(out, 'supabase', 'schema.sql'));
cpSync(join(root, 'supabase', 'LIVEKIT.md'), join(out, 'supabase', 'LIVEKIT.md'));
cpSync(join(root, 'supabase', 'functions'), join(out, 'supabase', 'functions'),
  { recursive: true });

console.log('Copied supabase schema + LiveKit docs into docs/.');