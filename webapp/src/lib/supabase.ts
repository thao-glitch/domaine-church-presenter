import { createClient, SupabaseClient } from '@supabase/supabase-js';
import { config, isConfigured } from '../config';

let client: SupabaseClient | null = null;

// Bound every request so a slow/blocked network never leaves the app hanging forever.
const REQUEST_TIMEOUT = 90_000;
const boundedFetch: typeof fetch = (input, init) => {
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), REQUEST_TIMEOUT);
  if (init?.signal) {
    if (init.signal.aborted) ctrl.abort();
    else init.signal.addEventListener('abort', () => ctrl.abort(), { once: true });
  }
  return fetch(input, { ...init, signal: ctrl.signal })
    .finally(() => clearTimeout(timer))
    .catch((e) => {
      if (ctrl.signal.aborted) throw new Error('Request timed out — check your internet connection.');
      throw e;
    });
};

export function getClient(): SupabaseClient | null {
  if (!isConfigured('supabase')) return null;
  if (!client) {
    client = createClient(config.supabaseUrl, config.supabaseKey, {
      auth: { persistSession: true, autoRefreshToken: true },
      global: { fetch: boundedFetch },
      realtime: { params: { eventsPerSecond: 10 } }
    });
  }
  return client;
}

export { isConfigured };