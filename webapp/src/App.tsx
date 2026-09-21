import { useEffect } from 'react';
import type { ReactNode } from 'react';
import { AuthProvider, useAuth } from './auth';
import { ToastProvider } from './components/toast';
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
  members: { title: 'Church Members', brandSub: 'Church Members', loginTitle: 'Church Members' },
  church: { title: 'Church Admin', brandSub: 'Church Console', loginTitle: 'Church Admin Console' },
  admin: { title: 'Platform Admin', brandSub: 'Platform Console', loginTitle: 'Platform Admin' }
};

// Each app has its own deployed GitHub Pages project.
const PROJECTS: Record<AppVariant, string> = {
  members: 'https://thao-glitch.github.io/domaine-church-members/',
  church: 'https://thao-glitch.github.io/domaine-church-console/',
  admin: 'https://thao-glitch.github.io/domaine-church-admin/'
};

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
  const { ready, user } = useAuth();
  useEffect(() => {
    document.title = `${META[variant].title} — Domaine Church`;
  }, [variant]);
  if (!ready) return <Splash label={META[variant].title} />;
  if (!user) return <LoginPage appName={META[variant].title} appTag={META[variant].brandSub} />;
  const h = window.location.hash.replace(/^#/, '');
  if (h.startsWith('/stage-view/')) return <StageViewPage id={h.split('/')[2]} key={h.split('/')[2]} />;
  return <Gate variant={variant} />;
}

function Gate({ variant }: { variant: AppVariant }) {
  const { isAdmin, canManageUsers, previewChurchId, profile } = useAuth();
  const hasScope = !!(previewChurchId || profile?.church_id);
  if (variant === 'admin' && !isAdmin) return <WrongApp variant={variant} />;
  if (variant === 'church' && !(canManageUsers || (isAdmin && previewChurchId))) return <WrongApp variant={variant} />;
  if (variant !== 'admin' && isAdmin && !hasScope) return <AdminNoScope />;
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
  return (
    <div className="setup-wrap">
      <div className="card setup-card">
        <header className="card-head">
          <h2 className="card-title">First-time setup</h2>
        </header>
        <p className="muted">
          Missing configuration. See <b>docs/supabase/LIVEKIT.md</b> and set values in
          <code> webapp/src/config.ts </code> before building.
        </p>
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
            <a className="side-item" href={PROJECTS.church}><Icon.Church size={19} /><span>Church Console</span></a>
            <a className="side-item" href={PROJECTS.members}><Icon.Users size={19} /><span>Church Members</span></a>
          </>
        ) : variant === 'church' ? (
          <>
            <div className="side-foot-text">Switch app:</div>
            <a className="side-item" href={PROJECTS.members}><Icon.Users size={19} /><span>Church Members</span></a>
          </>
        ) : (
          <div className="side-foot-text">You're in the members app.</div>
        )}
      </div>
    </aside>
  );
}

function AdminNoScope() {
  const { signOut } = useAuth();
  return (
    <div className="setup-wrap">
      <div className="card setup-card">
        <header className="card-head">
          <h2 className="card-title">Platform Admin</h2>
        </header>
        <p className="muted">
          You're the overall administrator and oversee every church — you don't need to be a
          member of any. Go to the <b>Platform Admin</b> console to register churches, create
          accounts and church admins, or use <b>Manage this church</b> to preview any church.
        </p>
        <div className="card-actions" style={{ flexWrap: 'wrap', gap: 8 }}>
          <Button onClick={() => { window.location.href = PROJECTS.admin; }}>Open Platform Admin</Button>
          <Button variant="ghost" onClick={signOut}>Log out</Button>
        </div>
      </div>
    </div>
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
              <Button onClick={() => { window.location.href = PROJECTS.church; }}>Open Church Console</Button>
              <Button variant="soft" onClick={() => { window.location.href = PROJECTS.members; }}>Open Church Members</Button>
            </>
          ) : (
            <>
              <Button onClick={() => { window.location.href = PROJECTS.members; }}>Go to Church Members</Button>
              <Button variant="soft" onClick={() => { window.location.href = PROJECTS.admin; }}>Open Platform Admin</Button>
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
