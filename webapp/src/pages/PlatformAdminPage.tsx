import { useEffect, useMemo, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Badge, Spinner, Empty } from '../components/ui';
import {
  fetchAdminChurches, fetchAllProfiles, deleteChurch, deleteProfile, setProfileRole,
  type Church, type AdminProfile
} from '../lib/api';
import { ROLES } from '../roles';
import { useToast } from '../components/toast';

export function PlatformAdminPage() {
  const { isAdmin, enterChurch, profile } = useAuth();
  const toast = useToast();
  const [churches, setChurches] = useState<Church[] | null>(null);
  const [people, setPeople] = useState<AdminProfile[] | null>(null);

  function load() {
    fetchAdminChurches().then(setChurches).catch((e) => toast(String(e), 'error'));
    fetchAllProfiles().then(setPeople).catch((e) => toast(String(e), 'error'));
  }

  useEffect(() => { if (isAdmin) load(); /* eslint-disable-next-line react-hooks/exhaustive-deps */ }, [isAdmin]);

  const byChurch = useMemo(() => {
    const map = new Map<string, number>();
    (people || []).forEach((p) => {
      if (p.church_id) map.set(p.church_id, (map.get(p.church_id) || 0) + 1);
    });
    return map;
  }, [people]);

  const churchName = (id: string | null) => {
    if (!id) return '—';
    return (churches || []).find((c) => c.id === id)?.name || 'Unknown church';
  };

  async function removeChurch(c: Church) {
    if (!confirm(`Delete "${c.name}"? Its members' links stay, but the church app disappears.`)) return;
    try {
      await deleteChurch(c.id);
      toast(`${c.name} deleted.`, 'success');
      load();
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  async function changeRole(p: AdminProfile, next: string) {
    try {
      await setProfileRole(p.id, next);
      toast(`${p.full_name || p.email} is now ${next}.`, 'success');
      setPeople((prev) => (prev || []).map((x) => (x.id === p.id ? { ...x, role: next } : x)));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  async function removeUser(p: AdminProfile) {
    if (!confirm(`Remove the account for ${p.email}? They will no longer be able to sign in.`)) return;
    try {
      await deleteProfile(p.id);
      toast(`${p.email} removed.`, 'success');
      setPeople((prev) => (prev || []).filter((x) => x.id !== p.id));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  if (!isAdmin) {
    return <Empty title="Platform admins only" sub="This area is restricted to the overall administrator." />;
  }

  return (
    <div className="stack">
      <Card title="All church apps">
        <div className="muted small">
          {churches ? `${churches.length} church${churches.length === 1 ? '' : 'es'}` : '…'} ·
          {' '}{people ? `${people.length} account${people.length === 1 ? '' : 's'}` : '…'} registered.
        </div>
      </Card>

      <Card title="Churches">
        {!churches ? <Spinner /> : churches.length === 0 ? (
          <Empty title="No churches yet" sub="Churches appear here as they register from the sign-up screen." />
        ) : (
          <div className="list">
            {churches.map((c) => (
              <div key={c.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{c.name} {c.city ? <span className="muted small">· {c.city}</span> : null}</div>
                  <div className="muted small">
                    {byChurch.get(c.id) || 0} account{(byChurch.get(c.id) || 0) === 1 ? '' : 's'} ·
                    {' '}owner {c.owner_name || c.owner_email || '—'}
                  </div>
                </div>
                <Button variant="soft" onClick={() => enterChurch(c.id)}>Manage this church</Button>
                <Button variant="ghost" onClick={() => removeChurch(c)}>Delete</Button>
              </div>
            ))}
          </div>
        )}
      </Card>

      <Card title="All accounts">
        {!people ? <Spinner /> : people.length === 0 ? (
          <Empty title="No accounts yet" sub="People appear here after they create an account." />
        ) : (
          <div className="list">
            {people.map((p) => (
              <div key={p.id} className="list-row">
                <div className="list-main">
                  <div className="list-title">{p.full_name || p.email}</div>
                  <div className="muted small">{p.email} · {churchName(p.church_id)}</div>
                </div>
                {p.email === profile?.email ? (
                  <Badge tone="success">{p.role} (you)</Badge>
                ) : (
                  <>
                    <select className="role-select" defaultValue={p.role} onChange={(e) => changeRole(p, e.target.value)}>
                      {ROLES.map((r) => <option key={r.name} value={r.name}>{r.name}</option>)}
                    </select>
                    <Button variant="ghost" onClick={() => removeUser(p)}>Remove</Button>
                  </>
                )}
              </div>
            ))}
          </div>
        )}
      </Card>
    </div>
  );
}