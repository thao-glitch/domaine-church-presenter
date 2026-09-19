import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Field, Badge, Spinner, Empty } from '../components/ui';
import { fetchChurch, saveChurch, fetchChurchProfiles, setProfileRole, type Church, type ChurchProfile } from '../lib/api';
import { ROLES } from '../roles';
import { useToast } from '../components/toast';
import { Avatar } from '../components/ui';

export function ChurchPage() {
  const { profile, churchName, churchId, canEdit } = useAuth();
  const toast = useToast();
  const [church, setChurch] = useState<Church | null>(null);
  const [list, setList] = useState<ChurchProfile[] | null>(null);
  const [d, setD] = useState<{ name: string; city: string; country: string; description: string }>({ name: '', city: '', country: '', description: '' });
  const [busy, setBusy] = useState(false);
  const isOwner = !!church && !!profile && church.owner_email === profile.email;

  useEffect(() => {
    if (!churchId) return;
    fetchChurch(churchId).then((c) => {
      if (!c) return;
      setChurch(c);
      setD({ name: c.name, city: c.city || '', country: c.country || '', description: c.description || '' });
    }).catch((e) => toast(String(e), 'error'));
    fetchChurchProfiles().then(setList).catch((e) => toast(String(e), 'error'));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [churchId]);

  async function saveInfo() {
    if (!church) return;
    setBusy(true);
    try {
      await saveChurch(church.id, { name: d.name, city: d.city || null, country: d.country || null, description: d.description || null });
      toast('Church details saved.', 'success');
      setChurch({ ...church, name: d.name, city: d.city || null, country: d.country || null, description: d.description || null });
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    } finally {
      setBusy(false);
    }
  }

  async function changeRole(userId: string, fullName: string | null, next: string) {
    try {
      await setProfileRole(userId, next);
      toast(`${fullName || 'Member'} is now ${next}.`, 'success');
      setList((prev) => (prev || []).map((p) => (p.id === userId ? { ...p, role: next } : p)));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  return (
    <div className="stack">
      <Card title="Your church">
        <div className="profile-top">
          <div className="splash-logo" style={{ width: 56, height: 56, fontSize: 20 }}>{churchName?.slice(0, 2).toUpperCase() || 'DC'}</div>
          <div>
            <div className="profile-name">{churchName || '…'}</div>
            <div className="muted">{church?.city ? `${church.city}${church.country ? `, ${church.country}` : ''}` : "Set your church's city and country below"}</div>
          </div>
        </div>
        <div className="spacer" />
        {isOwner ? (
          <>
            <Field label="Church name"><input value={d.name} onChange={(e) => setD({ ...d, name: e.target.value })} /></Field>
            <Field label="City"><input value={d.city} onChange={(e) => setD({ ...d, city: e.target.value })} /></Field>
            <Field label="Country"><input value={d.country} onChange={(e) => setD({ ...d, country: e.target.value })} /></Field>
            <Field label="About your church"><textarea rows={3} value={d.description} onChange={(e) => setD({ ...d, description: e.target.value })} /></Field>
            <div className="card-actions">
              <Button onClick={saveInfo} disabled={busy || !d.name.trim()}>{busy ? 'Saving…' : 'Save church details'}</Button>
            </div>
          </>
        ) : (
          <p className="muted">
            Owned by <b>{church?.owner_name || church?.owner_email || '…'}</b>. Ask your church's
            leadership to update details or assign roles below.
          </p>
        )}
      </Card>

      <Card title={canEdit ? 'Members & roles' : 'Members of this church'}>
        {!list ? <Spinner /> : list.length === 0 ? (
          <Empty title="No linked members yet" sub="As people create accounts and choose this church, they appear here." />
        ) : (
          <div className="list">
            {list.map((p) => (
              <div key={p.id} className="list-row">
                <Avatar name={p.full_name || p.email} size={34} />
                <div className="list-main">
                  <div className="list-title">{p.full_name || p.email}</div>
                  <div className="muted small">{p.email}</div>
                </div>
                {canEdit && p.id !== profile?.id ? (
                  <select className="role-select" defaultValue={p.role} onChange={(e) => changeRole(p.id, p.full_name, e.target.value)}>
                    {ROLES.map((r) => <option key={r.name} value={r.name}>{r.name}</option>)}
                  </select>
                ) : (
                  <Badge tone="success">{p.role}{p.id === profile?.id ? ' (you)' : ''}</Badge>
                )}
              </div>
            ))}
          </div>
        )}
      </Card>

      <Card title="Grow your church">
        <p className="muted small">
          Members join by visiting the same link and picking <b>{churchName || 'your church'}</b> when
          creating an account — their account is then linked to yours, so they only ever see your
          church's members, services, media and sessions.
        </p>
        <div className="spacer-s" />
        <input readOnly value={window.location.origin + window.location.pathname} className="invite-link" onFocus={(e) => e.currentTarget.select()} />
      </Card>
    </div>
  );
}