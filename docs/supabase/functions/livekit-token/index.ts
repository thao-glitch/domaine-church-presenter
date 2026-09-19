// LiveKit token minting for Domaine Church (Supabase Edge Function).
//
// Deploy in the Supabase dashboard: Edge Functions -> New function
// named "livekit-token", paste this file as index.ts, then set two
// secrets (Functions -> secret management):
//   LIVEKIT_API_KEY    = your LiveKit project API key
//   LIVEKIT_API_SECRET = your LiveKit project API secret
//
// The function returns { "token": "..." } for the requested room.
// HS256 JWT is implemented with WebCrypto so it needs no packages.

const apiKey = Deno.env.get('LIVEKIT_API_KEY') || '';
const apiSecret = Deno.env.get('LIVEKIT_API_SECRET') || '';

const enc = new TextEncoder();

function b64url(input: string): string {
  const bytes = enc.encode(input);
  let bin = '';
  bytes.forEach((b) => { bin += String.fromCharCode(b); });
  return btoa(bin).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

async function hmacSig(data: string): Promise<string> {
  const key = await crypto.subtle.importKey(
    'raw', enc.encode(apiSecret),
    { name: 'HMAC', hash: 'SHA-256' }, false, ['sign']);
  const sig = await crypto.subtle.sign('HMAC', key, enc.encode(data));
  return Array.from(new Uint8Array(sig))
    .map((b) => b.toString(16).padStart(2, '0')).join('');
}

export async function token(room: string, identity: string, name: string): Promise<string> {
  const now = Math.floor(Date.now() / 1000);
  const payload = {
    exp: now + 6 * 3600,
    iss: apiKey,
    nbf: now,
    sub: apiKey,
    name: name || identity,
    video: {
      room: room,
      roomJoin: true,
      canPublish: true,
      canSubscribe: true,
      canPublishData: true,
      canUpdateOwnMetadata: true
    }
  };
  const header = b64url(JSON.stringify({ alg: 'HS256', typ: 'JWT' }));
  const body = b64url(JSON.stringify(payload));
  const sig = await hmacSig(`${header}.${body}`);
  return `${header}.${body}.${sig}`;
}

Deno.serve(async (req) => {
  if (req.method !== 'POST') {
    return new Response(JSON.stringify({ error: 'POST only' }), {
      status: 405, headers: { 'Content-Type': 'application/json' }
    });
  }
  if (!apiKey || !apiSecret) {
    return new Response(JSON.stringify({ error: 'LiveKit secrets not configured' }), {
      status: 500, headers: { 'Content-Type': 'application/json' }
    });
  }
  try {
    const body = await req.json();
    const room = String(body?.room || '').trim();
    let identity = String(body?.identity || '').trim();
    const name = String(body?.name || '').trim();
    if (!room) {
      return new Response(JSON.stringify({ error: 'room is required' }), {
        status: 400, headers: { 'Content-Type': 'application/json' }
      });
    }
    if (!identity) identity = `member-${Math.floor(Math.random() * 1e8)}`;
    // Keep identities small and safe for LiveKit.
    identity = identity.slice(0, 60).replace(/[^a-zA-Z0-9._\-:@]/g, '-');
    const jwt = await token(room, identity, name);
    return new Response(JSON.stringify({ token: jwt }), {
      status: 200, headers: { 'Content-Type': 'application/json' }
    });
  } catch (e) {
    return new Response(JSON.stringify({ error: String(e) }), {
      status: 400, headers: { 'Content-Type': 'application/json' }
    });
  }
});