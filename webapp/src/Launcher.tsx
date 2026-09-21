import './styles.css';

const BASE = import.meta.env.BASE_URL;

const APPS = [
  {
    href: `${BASE}members/`,
    tag: 'Family App',
    title: 'Church Family',
    desc: 'Sunday feed, devotions, prayers, groups, giving, services, media and online sessions — for every member.',
    accent: 'linear-gradient(135deg, #6d5efc, #38bdf8)'
  },
  {
    href: `${BASE}church/`,
    tag: 'Church Console',
    title: 'Church Admin',
    desc: 'Leaders manage members, content, service planning, attendance, groups and giving for their own church.',
    accent: 'linear-gradient(135deg, #f59e0b, #ef4444)'
  },
  {
    href: `${BASE}admin/`,
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
        <p className="muted">One church family, three focused apps. Choose where you're going.</p>
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
