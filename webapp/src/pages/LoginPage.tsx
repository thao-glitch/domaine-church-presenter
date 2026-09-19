import { useState } from 'react';
import { useAuth } from '../auth';
import { Button, Field } from '../components/ui';

export function LoginPage() {
  const { signIn, signUp, createProfile } = useAuth();
  const [mode, setMode] = useState<'in' | 'up'>('in');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [fullName, setFullName] = useState('');
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function submit() {
    setBusy(true);
    setError(null);
    try {
      if (mode === 'up') {
        const err = await signUp(email, password, fullName);
        if (err) setError(err);
        else {
          setMode('in');
          setError('Account created — check your inbox to confirm, then log in.');
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
          <p className="muted">One home for every member — by role, together.</p>
        </div>

        <div className="auth-tabs">
          <button className={mode === 'in' ? 'active' : ''} onClick={() => { setMode('in'); setError(null); }}>Log in</button>
          <button className={mode === 'up' ? 'active' : ''} onClick={() => { setMode('up'); setError(null); }}>Create account</button>
        </div>

        <div className="auth-form">
          {mode === 'up' && (
            <Field label="Full name">
              <input value={fullName} onChange={(e) => setFullName(e.target.value)} placeholder="e.g. Jane Mvula" />
            </Field>
          )}
          <Field label="Email">
            <input type="email" value={email} onChange={(e) => setEmail(e.target.value)} placeholder="you@example.com" />
          </Field>
          <Field label="Password">
            <input type="password" value={password} onChange={(e) => setPassword(e.target.value)} placeholder="••••••••" />
          </Field>
          {error && <div className="form-error">{error}</div>}
          <Button className="w-full" disabled={busy || !email || !password || (mode === 'up' && !fullName)} onClick={submit}>
            {busy ? 'Please wait…' : mode === 'in' ? 'Log in' : 'Create account'}
          </Button>
          <p className="auth-note muted">
            New accounts default to <b>Member</b>. Leadership assigns ministry roles after your first log in.
          </p>
        </div>
      </div>
    </div>
  );
}