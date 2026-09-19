import { useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Button, Field } from '../components/ui';
import { fetchChurches, registerChurch, type Church } from '../lib/api';

export function LoginPage() {
  const { signIn, signUp, createProfile } = useAuth();
  const [mode, setMode] = useState<'in' | 'up'>('in');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [fullName, setFullName] = useState('');
  const [churches, setChurches] = useState<Church[]>([]);
  const [churchesLoaded, setChurchesLoaded] = useState(false);
  const [churchChoice, setChurchChoice] = useState(''); // '' = none, 'new' = register
  const [newName, setNewName] = useState('');
  const [newCity, setNewCity] = useState('');
  const [newCountry, setNewCountry] = useState('');
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetchChurches().then(setChurches).catch(() => setChurches([])).finally(() => setChurchesLoaded(true));
  }, []);

  const registering = churchChoice === 'new';
  const canSubmit = !!email && !!password && (mode === 'in' || (mode === 'up' && !!fullName && (churchChoice === 'new' ? !!newName : !!churchChoice)));

  async function submit() {
    setBusy(true);
    setError(null);
    try {
      if (mode === 'up') {
        let cid = '';
        if (registering) {
          const church = await registerChurch({
            name: newName, city: newCity, country: newCountry,
            owner_email: email, owner_name: fullName
          });
          cid = church.id;
        } else {
          cid = churchChoice;
        }
        const err = await signUp(email, password, fullName, cid);
        if (err) setError(err);
        else {
          setMode('in');
          setError(registering
            ? `"${newName}" registered. Confirm your email if asked, then log in.`
            : 'Account created — confirm your email if asked, then log in.');
        }
      } else {
        const err = await signIn(email, password);
        if (err) setError(err);
        else createProfile();
      }
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="auth-wrap">
      <div className="auth-card">
        <div className="auth-brand">
          <div className="splash-logo big">DC</div>
          <h1>Domaine Church</h1>
          <p className="muted">One home for every church — every member is linked to their church.</p>
        </div>

        <div className="auth-tabs">
          <button className={mode === 'in' ? 'active' : ''} onClick={() => { setMode('in'); setError(null); }}>Log in</button>
          <button className={mode === 'up' ? 'active' : ''} onClick={() => { setMode('up'); setError(null); }}>Create account</button>
        </div>

        <div className="auth-form">
          {mode === 'up' && (
            <>
              <Field label="Full name">
                <input value={fullName} onChange={(e) => setFullName(e.target.value)} placeholder="e.g. Jane Mvula" />
              </Field>
              <Field label="Your church">
                <select value={churchChoice} onChange={(e) => { setChurchChoice(e.target.value); setError(null); }}>
                  <option value="">{churchesLoaded ? 'Choose your church…' : 'Loading churches…'}</option>
                  {churches.map((c) => (
                    <option key={c.id} value={c.id}>{c.name}{c.city ? ` · ${c.city}` : ''}{c.country ? `, ${c.country}` : ''}</option>
                  ))}
                  <option value="new">＋ Register my church</option>
                </select>
              </Field>
              {registering && (
                <div className="stack-s">
                  <Field label="Church name"><input value={newName} onChange={(e) => setNewName(e.target.value)} placeholder="e.g. Good Shepherd Assembly" /></Field>
                  <Field label="City"><input value={newCity} onChange={(e) => setNewCity(e.target.value)} placeholder="optional" /></Field>
                  <Field label="Country"><input value={newCountry} onChange={(e) => setNewCountry(e.target.value)} placeholder="optional" /></Field>
                </div>
              )}
            </>
          )}
          <Field label="Email">
            <input type="email" value={email} onChange={(e) => setEmail(e.target.value)} placeholder="you@example.com" />
          </Field>
          <Field label="Password">
            <input type="password" value={password} onChange={(e) => setPassword(e.target.value)} placeholder="••••••••" />
          </Field>
          {error && <div className="form-error">{error}</div>}
          <Button className="w-full" disabled={busy || !canSubmit} onClick={submit}>
            {busy ? 'Please wait…' : mode === 'in' ? 'Log in' : 'Create account'}
          </Button>
          <p className="auth-note muted">
            New accounts default to <b>Member</b>. Your church's leadership assigns ministry roles
            after your first log in — and everyone only sees their own church.
          </p>
        </div>
      </div>
    </div>
  );
}