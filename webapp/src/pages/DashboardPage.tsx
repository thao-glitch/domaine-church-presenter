import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Spinner, Empty, RoleBadge } from '../components/ui';
import { Link } from '../lib/router';
import {
  fetchServices, fetchEvents, fetchSessions, type Service, type ChurchEvent, type OnlineSession
} from '../lib/api';
import { todayISO, weekDay, formatDate, formatDateTime } from '../utils';
import { WEEKDAYS } from '../roles';
import { cn } from '../utils';

export function DashboardPage() {
  const { user, profile, canSchedule } = useAuth();
  const [services, setServices] = useState<Service[] | null>(null);
  const [events, setEvents] = useState<ChurchEvent[] | null>(null);
  const [sessions, setSessions] = useState<OnlineSession[] | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let live = true;
    Promise.allSettled([fetchServices(), fetchEvents(), fetchSessions()]).then((res) => {
      if (!live) return;
      const sv = res[0].status === 'fulfilled' ? res[0].value : null;
      const ev = res[1].status === 'fulfilled' ? res[1].value : null;
      const se = res[2].status === 'fulfilled' ? res[2].value : null;
      if (!sv && !ev && !se) setError('Could not load data. Check the connection in Setup.');
      setServices(sv); setEvents(ev); setSessions(se);
    });
    return () => { live = false; };
  }, []);

  const today = todayISO();
  const todayWd = WEEKDAYS[weekDay()];
  const todaysSvcs = (services || []).filter(
    (s) => (s.recurring && s.weekday === weekDay()) || (!s.recurring && s.date === today)
  ).sort((a, b) => a.start_time.localeCompare(b.start_time));
  const todaysEvts = (events || []).filter((e) => e.date === today)
    .sort((a, b) => a.start_time.localeCompare(b.start_time));
  const upcomingSessions = (sessions || []).filter((s) => new Date(s.starts_at) > new Date(Date.now() - 3600000))
    .slice(0, 4);

  return (
    <div className="stack">
      <Card title={`Good to see you${profile?.full_name ? ', ' + profile.full_name.split(' ')[0] : ''}`}>
        <p className="muted">
          You are logged in as <RoleBadge role={profile?.role || 'Member'} />. Here is what is happening at church today.
        </p>
      </Card>

      <div className="grid-2">
        <Card title={`Today · ${todayWd}, ${formatDate(today)}`}>
          {error && <Empty title="Offline data" sub={error} />}
          {!error && todaysSvcs.length === 0 && todaysEvts.length === 0 && (
            <Empty title="Nothing scheduled today" sub="Leaders can add services and events from Services & Events." />
          )}
          {(todaysSvcs.length > 0 || todaysEvts.length > 0) && (
            <ul className="tl">
              {[...todaysSvcs.map((s) => ({ time: s.start_time, label: s.name, extra: s.location })) as { time: string; label: string; extra?: string }[],
                ...todaysEvts.map((e) => ({ time: e.start_time || '·', label: e.title, extra: e.location })) as { time: string; label: string; extra?: string }[]
              ].sort((a, b) => a.time.localeCompare(b.time)).map((it, i) => (
                <li key={i} className="tl-row">
                  <span className="tl-time">{it.time}</span>
                  <span className="tl-label">{it.label}</span>
                  {it.extra && <span className="tl-extra">{it.extra}</span>}
                </li>
              ))}
            </ul>
          )}
          <div className="card-actions">
            <Link to="/schedule" className="btn btn-soft">See all services & events</Link>
          </div>
        </Card>

        <Card title="Online sessions" actions={canSchedule ? <Link to="/sessions" className="btn btn-soft">Schedule</Link> : undefined}>
          {!sessions && <Spinner />}
          {sessions && upcomingSessions.length === 0 && (
            <Empty title="No sessions coming up" sub="Any member can start a session from Online Sessions." />
          )}
          {sessions && upcomingSessions.map((s) => (
            <div key={s.id} className="session-card">
              <div className="session-title">{s.title}</div>
              <div className="muted small">{formatDateTime(s.starts_at)} · hosted by {s.host || '—'}</div>
              <Link to="/sessions" className="btn btn-primary sm">Join</Link>
            </div>
          ))}
        </Card>
      </div>

      <div className="grid-3">
        <Quick label="Members directory" desc="Everyone in the church family, sorted by role." to="/members" />
        <Quick label="Media & uploads" desc="Share files, notes, recordings with the whole church." to="/media" />
        <Quick label="Chat" desc="Talk with members in channels and privately." to="/chat" />
      </div>
    </div>
  );
}

function Quick({ label, desc, to }: { label: string; desc: string; to: string }) {
  return (
    <Link to={to} className="card quick-card">
      <div className="quick-title">{label}</div>
      <div className="muted small">{desc}</div>
    </Link>
  );
}