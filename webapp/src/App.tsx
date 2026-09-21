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
import { ChurchPage } from './pages/ChurchPage';
import { PlatformAdminPage } from './pages/PlatformAdminPage';
import { FeedPage } from './pages/FeedPage';
import { ContentPage } from './pages/ContentPage';
import { PlanningPage } from './pages/PlanningPage';
import { AttendancePage } from './pages/AttendancePage';
import { GroupsPage } from './pages/GroupsPage';
import { GivingPage } from './pages/GivingPage';
import { RecordingsPage } from './pages/RecordingsPage';

export type AppVariant = 'members' | 'church' | 'admin';

interface VariantMeta {
  title: string;
  brandSub: string;
  loginTitle: string;
}

const META: Record<AppVariant, VariantMeta> = {
  members: { title: 'Domaine Church', brandSub: 'Family App', loginTitle: 'Domaine Church — Family App' },
  church: { title: 'Church Admin', brandSub: 'Church Console', loginTitle: 'Church Admin Console' },
  admin: { title: 'Platform Admin', brandSub: 'Platform Console', loginTitle: 'Platform Admin' }
};

const BASE = import.meta.env.BASE_URL;

export function App({ variant }: { variant: AppVariant }) {
  return (
    <ToastProvider>
      <AuthProvider>
        <Root variant={variant} />
      </AuthProvider>
    </ToastProvider>
  );
}

function Root({ variant }: { variant: AppVariant }) {
  const { configuring, ready, user } = useAuth();
  if (configuring) return <SetupScreen />;
  if (!ready) return <Splash label={META[variant].title} />;
  if (!user) return <LoginPage />;
  const h = window.location.hash.replace(/^#/, '');
  if (h.startsWith('/stage-view/')) return <StageViewPage id={h.split('/')[2]} key={h.split('/')[2]} />;
  return <Gate variant={variant} />;
}

function Gate({ variant }: { variant: AppVariant }) {
  const { isAdmin, canManageUsers, previewChurchId } = useAuth();
  if (variant === 'admin' && !isAdmin) return <WrongApp variant={variant} />;
  if (variant === 'church' && !(canManageUsers || (isAdmin && previewChurchId))) return <WrongApp variant={variant} />;
  return <Shell variant={variant} />;
}

function Splash({ label = 'Domaine Church' }: { label?: string }) {
  return (
    <div className="splash">
      <div className="splash-logo">DC</div>
      <Spinner label={`Loading ${label}…`} />
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

function Shell({ variant }: { variant: AppVariant }) {
  const hash = useHash();
  const { roleLabel, canEdit, canPresent, isAdmin, previewChurchId, churchName, signOut, user, profile, refreshProfile, exitChurch } = useAuth();
  const missingProfile = !!user && !profile;

  const nav: NavItem[] = variant === 'admin'
    ? [
        { hash: '/platform', label: 'Platform Admin', icon: Icon.Shield, show: true },
        { hash: '/profile', label: 'My profile', icon: Icon.Users, show: true }
      ]
    : variant === 'church'
    ? [
        { hash: '/dashboard', label: 'Dashboard', icon: Icon.Home, show: true },
        { hash: '/members', label: 'Members', icon: Icon.Users, show: true },
        { hash: '/church', label: 'Church Settings', icon: Icon.Church, show: true },
        { hash: '/content', label: 'Content Studio', icon: Icon.Upload, show: true },
        { hash: '/planning', label: 'Planning', icon: Icon.Stage, show: true },
        { hash: '/attendance', label: 'Attendance', icon: Icon.Users, show: true },
        { hash: '/groups', label: 'Groups', icon: Icon.Users, show: true },
        { hash: '/schedule', label: 'Services & Events', icon: Icon.Calendar, show: true },
        { hash: '/giving', label: 'Giving', icon: Icon.Calendar, show: true },
        { hash: '/media', label: 'Media', icon: Icon.Upload, show: true },
        { hash: '/archive', label: 'Sermon Archive', icon: Icon.Video, show: true },
        { hash: '/chat', label: 'Chat', icon: Icon.Chat, show: true },
        { hash: '/sessions', label: 'Online Sessions', icon: Icon.Video, show: true },
        { hash: '/stage', label: 'Stage (Presenter)', icon: Icon.Stage, show: canPresent },
        { hash: '/feed', label: 'Preview Feed', icon: Icon.Chat, show: true },
        { hash: '/profile', label: 'My profile', icon: Icon.Users, show: true }
      ]
    : [
        { hash: '/dashboard', label: 'Dashboard', icon: Icon.Home, show: true },
        { hash: '/feed', label: 'Church Feed', icon: Icon.Chat, show: true },
        { hash: '/members', label: 'Members', icon: Icon.Users, show: true },
        { hash: '/groups', label: 'Groups', icon: Icon.Users, show: true },
        { hash: '/schedule', label: 'Services & Events', icon: Icon.Calendar, show: true },
        { hash: '/media', label: 'Media', icon: Icon.Upload, show: true },
        { hash: '/archive', label: 'Sermon Archive', icon: Icon.Video, show: true },
        { hash: '/giving', label: 'My Giving', icon: Icon.Calendar, show: true },
        { hash: '/chat', label: 'Chat', icon: Icon.Chat, show: true },
        { hash: '/sessions', label: 'Online Sessions', icon: Icon.Video, show: true },
        { hash: '/profile', label: 'My profile', icon: Icon.Users, show: true }
      ];

  const visible = nav.filter((n) => n.show);
  const active = visible.find((n) => hash.startsWith(n.hash) || (n.hash === '/dashboard' && hash === '/'))
    ?? visible[0];

  return (
    <div className={`app app-${variant}`}>
      <Sidebar variant={variant} items={visible} active={active.hash} churchName={churchName} />
      <div className="main">
        <header className="topbar">
          <button className="icon-btn menu-btn" onClick={() => document.body.classList.toggle('nav-open')}>
            <Icon.Menu />
          </button>
          <div className="topbar-title">{active.label}</div>
          <div className="topbar-spacer" />
          {missingProfile && variant !== 'admin' && (
            <Button variant="soft" onClick={() => refreshProfile()}>Attach role to my account</Button>
          )}
          <RoleBadge role={roleLabel} />
          <Link to="/profile" className="topbar-user">
            <Avatar name={profile?.full_name || user?.email || '?'} size={30} />
            <span className="topbar-name">{profile?.full_name || user?.email?.split('@')[0]}</span>
          </Link>
          <button className="icon-btn" onClick={signOut} title="Log out"><Icon.Logout /></button>
        </header>
        {previewChurchId && variant === 'church' && (
          <div className="preview-banner">
            <span>You are managing <b>{churchName || 'this church'}</b> as platform admin.</span>
            <Button variant="soft" onClick={exitChurch}>Exit to Platform Admin</Button>
          </div>
        )}
        <main className="content">{renderPage(variant, hash)}</main>
      </div>
    </div>
  );
}

function Sidebar({ variant, items, active, churchName }: { variant: AppVariant; items: NavItem[]; active: string; churchName: string | null }) {
  const brand = variant === 'admin' ? 'Platform' : variant === 'church' ? (churchName || 'Church Admin') : (churchName || 'Domaine Church');
  return (
    <aside className="sidebar">
      <div className="sidebar-brand">
        <div className="splash-logo small">{variant === 'admin' ? 'PA' : 'DC'}</div>
        <div>
          <div className="brand-name">{brand}</div>
          <div className="brand-sub">{META[variant].brandSub}</div>
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
        {variant === 'admin' ? (
          <>
            <div className="side-foot-text">Open another app:</div>
            <a className="side-item" href={`${BASE}church/`}><Icon.Church size={19} /><span>Church Console</span></a>
            <a className="side-item" href={`${BASE}members/`}><Icon.Users size={19} /><span>Family App</span></a>
          </>
        ) : variant === 'church' ? (
          <>
            <div className="side-foot-text">Switch app:</div>
            <a className="side-item" href={`${BASE}members/`}><Icon.Users size={19} /><span>Family App</span></a>
          </>
        ) : (
          <div className="side-foot-text">You're in the church family app.</div>
        )}
      </div>
    </aside>
  );
}

function WrongApp({ variant }: { variant: AppVariant }) {
  const { signOut, roleLabel } = useAuth();
  const isAdminApp = variant === 'admin';
  return (
    <div className="setup-wrap">
      <div className="card setup-card">
        <header className="card-head">
          <h2 className="card-title">{META[variant].title}</h2>
        </header>
        <p className="muted">
          This is the <b>{META[variant].loginTitle}</b>, but your account is not allowed here
          (role: <b>{roleLabel}</b>).
        </p>
        <div className="card-actions" style={{ flexWrap: 'wrap', gap: 8 }}>
          {isAdminApp ? (
            <>
              <Button onClick={() => { window.location.href = `${BASE}church/`; }}>Open Church Console</Button>
              <Button variant="soft" onClick={() => { window.location.href = `${BASE}members/`; }}>Open Family App</Button>
            </>
          ) : (
            <>
              <Button onClick={() => { window.location.href = `${BASE}members/`; }}>Go to Family App</Button>
              <Button variant="soft" onClick={() => { window.location.href = `${BASE}admin/`; }}>Open Platform Admin</Button>
            </>
          )}
          <Button variant="ghost" onClick={signOut}>Log out</Button>
        </div>
      </div>
    </div>
  );
}

function renderPage(variant: AppVariant, hash: string): ReactNode {
  if (variant === 'admin') {
    if (hash.startsWith('/profile')) return <ProfilePage />;
    if (!hash.startsWith('/platform')) { navigate('/platform'); return <Splash label="Platform Admin" />; }
    return <PlatformAdminPage />;
  }
  if (hash.startsWith('/stage')) return <PresenterPage />;
  if (hash.startsWith('/feed')) return <FeedPage />;
  if (hash.startsWith('/content')) return <ContentPage />;
  if (hash.startsWith('/planning')) return <PlanningPage />;
  if (hash.startsWith('/attendance')) return <AttendancePage />;
  if (hash.startsWith('/groups')) return <GroupsPage />;
  if (hash.startsWith('/giving')) return <GivingPage />;
  if (hash.startsWith('/archive')) return <RecordingsPage />;
  if (hash.startsWith('/members')) return <MembersPage />;
  if (hash.startsWith('/schedule')) return <SchedulePage />;
  if (hash.startsWith('/media')) return <MediaPage />;
  if (hash.startsWith('/chat')) return <ChatPage />;
  if (hash.startsWith('/sessions')) return <SessionsPage />;
  if (hash.startsWith('/church')) return <ChurchPage />;
  if (hash.startsWith('/platform')) return <PlatformAdminPage />;
  if (hash.startsWith('/profile')) return <ProfilePage />;
  if (hash.startsWith('/dashboard')) return <DashboardPage />;
  navigate('/dashboard');
  return <Splash />;
}
