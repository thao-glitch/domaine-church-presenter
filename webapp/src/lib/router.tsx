import { useEffect, useState, useCallback } from 'react';
import type { ReactNode } from 'react';

export function parseHash(): string {
  let h = window.location.hash.replace(/^#/, '');
  if (!h || h === '/') h = '/dashboard';
  return h;
}

export function useHash(): string {
  const [h, setH] = useState(parseHash);
  useEffect(() => {
    const on = () => setH(parseHash());
    window.addEventListener('hashchange', on);
    return () => window.removeEventListener('hashchange', on);
  }, []);
  return h;
}

export function navigate(to: string) {
  window.location.hash = to;
}

export interface RouteMatch {
  route: string;
  params: Record<string, string>;
}

export function matchRoutes(hash: string, routes: string[]): RouteMatch | null {
  const segs = hash.split('/').filter(Boolean);
  for (const r of routes) {
    const rs = r.split('/').filter(Boolean);
    if (rs.length !== segs.length) continue;
    const params: Record<string, string> = {};
    let ok = true;
    for (let i = 0; i < rs.length; i++) {
      if (rs[i].startsWith(':')) params[rs[i].slice(1)] = decodeURIComponent(segs[i]);
      else if (rs[i] !== segs[i]) { ok = false; break; }
    }
    if (ok) return { route: r, params };
  }
  return null;
}

export function Link({ to, children, className }: { to: string; children: ReactNode; className?: string }) {
  return (
    <a href={'#' + to} className={className} onClick={(e) => { e.preventDefault(); navigate(to); }}>
      {children}
    </a>
  );
}

export function useParamsFor(routes: string[]): Record<string, string> {
  const hash = useHash();
  const m = matchRoutes(hash, routes);
  return m ? m.params : {};
}