import './styles.css';

// Each app lives in its own deployed GitHub Pages project.
const PROJECTS = {
  members: 'https://thao-glitch.github.io/domaine-church-members/',
  church: 'https://thao-glitch.github.io/domaine-church-console/',
  admin: 'https://thao-glitch.github.io/domaine-church-admin/'
};

const APPS = [
  {
    href: PROJECTS.members,
    tag: 'Church Members',
    title: 'Church Members',
    desc: 'Feed, devotions, prayers, groups, giving, services, media and online sessions — for every member.',
    accent: 'linear-gradient(135deg, #6d5efc, #38bdf8)'
  },
  {
    href: PROJECTS.church,
    tag: 'Church Console',
    title: 'Church Admin',
    desc: 'Leaders manage members, content, service planning, attendance, groups and giving for their own church.',
    accent: 'linear-gradient(135deg, #f59e0b, #ef4444)'
  },
  {
    href: PROJECTS.admin,
    tag: 'Platform Console',
    title: 'Platform Admin',
    desc: 'Oversee every church, all accounts, publish global content and view cross-church analytics.',
    accent: 'linear-gradient(135deg, #10b981, #0ea5e9)'
  }
];

export function Launcher() {
  return (
    <div className="launcher">
      <header className="launcher-head">
        <div className="splash-logo">DC</div>
        <h1>Domaine Church</h1>
        <p className="muted">Three focused apps. Choose where you're going.</p>
      </header>
      <div className="launcher-grid">
        {APPS.map((a) => (
          <a key={a.href} className="launcher-card" href={a.href}>
            <div className="launcher-tag" style={{ background: a.accent }}>{a.tag}</div>
            <div className="launcher-title">{a.title}</div>
            <div className="launcher-desc">{a.desc}</div>
            <span className="launcher-go">Open →</span>
          </a>
        ))}
      </div>
      <footer className="launcher-foot muted small">
        Sign in once — your session works across all three apps.
      </footer>
    </div>
  );
}
