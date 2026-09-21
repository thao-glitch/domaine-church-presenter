// Deploys a standalone app build to its own GitHub Pages project (repo).
// Usage: node scripts/deploy-pages.mjs <members|church|admin|all>
import { execSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, readdirSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dirname } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const owner = 'thao-glitch';

const APPS = {
  members: { repo: 'domaine-church-members', app: 'members' },
  church: { repo: 'domaine-church-console', app: 'church' },
  admin: { repo: 'domaine-church-admin', app: 'admin' }
};

const which = process.argv[2] || 'all';
const targets = which === 'all' ? Object.keys(APPS) : [which];
const missing = targets.filter((t) => !APPS[t]);
if (missing.length) {
  console.error(`Unknown app(s): ${missing.join(', ')} (pick members | church | admin | all)`);
  process.exit(1);
}

const run = (cmd, opts = {}) => execSync(cmd, { stdio: 'inherit', ...opts });

for (const t of targets) {
  const { repo, app } = APPS[t];
  const full = `${owner}/${repo}`;
  const dir = join(tmpdir(), `dc-deploy-${t}-${Date.now()}`);
  const base = join(root, `dist-${t}`);
  const htmlSrc = join(base, app);
  const assetsSrc = join(base, 'assets');

  console.log(`\n==> Deploying ${t} (${full})`);
  if (!repoExists(full)) {
    console.log(`    creating repo ${repo} …`);
    run(`gh repo create ${repo} --public --confirm`);
  }
  run(`git clone https://github.com/${full}.git "${dir}"`);
  run(`git -C "${dir}" checkout -B main`);
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    if (entry.name === '.git') continue;
    rmSync(join(dir, entry.name), { recursive: true, force: true });
  }
  for (const entry of readdirSync(htmlSrc)) {
    cpSync(join(htmlSrc, entry), join(dir, entry), { recursive: true });
  }
  if (existsSync(assetsSrc)) {
    mkdirSync(join(dir, 'assets'), { recursive: true });
    for (const entry of readdirSync(assetsSrc)) {
      cpSync(join(assetsSrc, entry), join(dir, 'assets', entry), { recursive: true });
    }
  }
  run(`git -C "${dir}" add -A`);
  run(`git -C "${dir}" commit -m "Deploy ${t} app" --allow-empty`);
  run(`git -C "${dir}" push -u origin main`);
  enablePages(full);
  rmSync(dir, { recursive: true, force: true });
  console.log(`    done: https://${owner}.github.io/${repo}/`);
}

function repoExists(full) {
  try {
    execSync(`gh repo view ${full} --json nameWithOwner`, { stdio: 'pipe' });
    return true;
  } catch {
    return false;
  }
}

function enablePages(full) {
  const opts = { stdio: 'pipe' };
  try {
    execSync(`gh api repos/${full}/pages -f "source[branch]=main" -f "source[path]=/"`, opts);
  } catch {
    try {
      execSync(`gh api repos/${full}/pages -X PATCH -f "source[branch]=main" -f "source[path]=/"`, opts);
    } catch {
      console.error('    (Pages already configured or must be enabled in the repo settings)');
    }
  }
}