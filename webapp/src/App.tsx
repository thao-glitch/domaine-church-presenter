import { useState } from 'react';
import type { ReactNode } from 'react';
import { AuthProvider, useAuth } from './auth';
import { ToastProvider } from './components/toast';
import { saveConfig, resolveConfig } from './config';
import { useHash, navigate, Link } from './lib/router';
import { Button, Avatar, RoleBadge, Spinner } from './components/ui';
import { Icon } from './components/icons';
import { LoginPage } from './pages/LoginPage';
import { DashboardPage } from './pages/DashboardPage';
import { MembersPage } from './pages/MembersPage';
import { SchedulePage } from './pages/SchedulePage';
import { MediaPage } from './pages/MediaPage';
import { ChatPage } from './pages/ChatPage';
import { SessionsPage } from './pages/SessionsPage';
import { PresenterPage } from './pages/PresenterPage';
import { StageViewPage } from './pages/StageViewPage';
import { ProfilePage } from './pages/ProfilePage';

export function App() {
  return (
    <ToastProvider>
      <AuthProvider>
        <Root />
      </AuthProvider>
    </ToastProvider>
  );
}

function Root() {
  const { configuring, ready, user } = useAuth();
  if (configuring) return <SetupScreen />;
  if (!ready) return <Splash />;
  if (!user) return <LoginPage />;
  const h = window.location.hash.replace(/^#/, '');
  if (h.startsWith('/stage-view/')) return <StageViewPage id={h.split('/')[2]} key={h.split('/')[2]} />;
  return <Shell />;
}

function Splash() {
  return (
    <div className="splash">
      <div className="splash-logo">DC</div>
      <Spinner label="Loading Domaine Church…" />
    </div>
  );
}

function SetupScreen() {
  const [cfg, setCfg] = useState(() => ({ ...resolveConfig() }));
  const [saved, setSaved] = useState(false);

  const save = () => {
    saveConfig({ ...cfg, name: cfg.name || 'Domaine Church' });
    setSaved(true);
    setTimeout(() => window.location.reload(), 700);
  };

  return (
    <div className="setup-wrap">
      <div className="card setup-card">
        <header className="card-head">
          <h2 className="card-title">First-time setup</h2>
        </header>
        <p className="muted">
          This app connects to your church's <b>Supabase</b> project (login, members,
          services, media, chat) and a <b>LiveKit</b> server (online video sessions).
        </p>
        <label className="field">
          <span className="field-label">App name</span>
          <input value={cfg.name} onChange={(e) => setCfg({ ...cfg, name: e.target.value })} />
        </label>
        <label className="field">
          <span className="field-label">Supabase project URL</span>
          <input value={cfg.supabaseUrl} onChange={(e) => setCfg({ ...cfg, supabaseUrl: e.target.value })} />
        </label>
        <label className="field">
          <span className="field-label">Supabase anon (public) key</span>
          <input value={cfg.supabaseKey} onChange={(e) => setCfg({ ...cfg, supabaseKey: e.target.value })} />
        </label>
        <label className="field">
          <span className="field-label">LiveKit server URL</span>
          <input value={cfg.livekitUrl} onChange={(e) => setCfg({ ...cfg, livekitUrl: e.target.value })} />
        </label>
        <label className="field">
          <span className="field-label">LiveKit token endpoint (Supabase Edge Function)</span>
          <input value={cfg.livekitTokenUrl} onChange={(e) => setCfg({ ...cfg, livekitTokenUrl: e.target.value })} />
        </label>
        <div className="card-actions">
          <Button onClick={save}>Save and reload</Button>
        </div>
        {saved && (
          <p className="ok">
            Saved — reloading…<br />
            <b>Next screen:</b> create your church account (email + password) on the
            login page, then you're in.
          </p>
        )}
        <div className="setup-note">
          Get URL + key from Supabase → <i>Project Settings → API</i>. Deploy the token
          function by following <b>docs/supabase/LIVEKIT.md</b>. Values only ever live in
          your own browser.
        </div>
      </div>
    </div>
  );
}

interface NavItem {
  hash: string;
  label: string;
  icon: (p: { size?: number }) => ReactNode;
  show: boolean;
}

function Shell() {
  const hash = useHash();
  const { roleLabel, canEdit, canPresent, signOut, user, profile, refreshProfile } = useAuth();
  const missingProfile = !!user && !profile;

  const nav: NavItem[] = [
    { hash: '/dashboard', label: 'Dashboard', icon: Icon.Home, show: true },
    { hash: '/members', label: 'Members', icon: Icon.Users, show: true },
    { hash: '/schedule', label: 'Services & Events', icon: Icon.Calendar, show: true },
    { hash: '/media', label: 'Media', icon: Icon.Upload, show: true },
    { hash: '/chat', label: 'Chat', icon: Icon.Chat, show: true },
    { hash: '/sessions', label: 'Online Sessions', icon: Icon.Video, show: true },
    { hash: '/stage', label: 'Stage (Presenter)', icon: Icon.Stage, show: canPresent },
    { hash: '/profile', label: 'My profile', icon: Icon.Users, show: true }
  ];
  const visible = nav.filter((n) => n.show);
  const active = visible.find((n) => hash.startsWith(n.hash) || (n.hash === '/dashboard' && hash === '/'))
    ?? visible[0];

  return (
    <div className="app">
      <Sidebar items={visible} active={active.hash} />
      <div className="main">
        <header className="topbar">
          <button className="icon-btn menu-btn" onClick={() => document.body.classList.toggle('nav-open')}>
            <Icon.Menu />
          </button>
          <div className="topbar-title">{active.label}</div>
          <div className="topbar-spacer" />
          {missingProfile && (
            <Button variant="soft" onClick={() => refreshProfile()}>Attach role to my account</Button>
          )}
          <RoleBadge role={roleLabel} />
          <Link to="/profile" className="topbar-user">
            <Avatar name={profile?.full_name || user?.email || '?'} size={30} />
            <span className="topbar-name">{profile?.full_name || user?.email?.split('@')[0]}</span>
          </Link>
          <button className="icon-btn" onClick={signOut} title="Log out"><Icon.Logout /></button>
        </header>
        <main className="content">{renderPage(hash)}</main>
      </div>
    </div>
  );
}

function Sidebar({ items, active }: { items: NavItem[]; active: string }) {
  return (
    <aside className="sidebar">
      <div className="sidebar-brand">
        <div className="splash-logo small">DC</div>
        <div>
          <div className="brand-name">Domaine Church</div>
          <div className="brand-sub">Family App</div>
        </div>
      </div>
      <nav className="side-nav">
        {items.map((it) => (
          <Link key={it.hash} to={it.hash}
            className={`side-item ${active === it.hash ? 'active' : ''}`}>
            {it.icon({ size: 19 })}
            <span>{it.label}</span>
          </Link>
        ))}
      </nav>
      <div className="side-foot">
        <div className="side-foot-text">Log in shows what your role can manage.</div>
        <Button variant="ghost" className="w-full"><Icon.Users size={16} /> Community</Button>
      </div>
    </aside>
  );
}

function renderPage(hash: string): ReactNode {
  if (hash.startsWith('/stage')) return <PresenterPage />;
  if (hash.startsWith('/members')) return <MembersPage />;
  if (hash.startsWith('/schedule')) return <SchedulePage />;
  if (hash.startsWith('/media')) return <MediaPage />;
  if (hash.startsWith('/chat')) return <ChatPage />;
  if (hash.startsWith('/sessions')) return <SessionsPage />;
  if (hash.startsWith('/profile')) return <ProfilePage />;
  if (hash.startsWith('/dashboard')) return <DashboardPage />;
  navigate('/dashboard');
  return <Splash />;
}