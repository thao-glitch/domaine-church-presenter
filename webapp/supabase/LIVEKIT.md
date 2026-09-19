# LiveKit for Domaine Church

Online sessions (video meetings) need three things:

## 1. A LiveKit server (room hosting)

1. Go to https://livekit.io/cloud and create a free project.
2. Copy the **server URL** — it looks like `wss://project-xxxx-xxxx.livekit.cloud`.
3. Keep the **API key** and **API secret** handy (shown once).

## 2. The token function (deploys to Supabase)

The app asks Supabase for a short-lived meeting token before joining.

In the **Supabase dashboard**:

1. Open **Edge Functions → New function**, name it `livekit-token`.
2. Replace the generated file with the contents of
   `supabase/functions/livekit-token/index.ts` (in this repo).
3. Deploy, then open **Edge Functions → livekit-token → Secrets** and set:
   - `LIVEKIT_API_KEY` = your LiveKit API key
   - `LIVEKIT_API_SECRET` = your LiveKit API secret

Verify the URL is:
`https://<your-project>.supabase.co/functions/v1/livekit-token`

## 3. Tell the app about both

Open the app → **Setup screen** (first run) and enter:

- **LiveKit server URL** — e.g. `wss://project-xxxx-xxxx.livekit.cloud`
- **LiveKit token endpoint** — the function URL above

Save and reload. The **Online Sessions** tab will now let members join
meetings and leaders schedule them.

## Troubleshooting

- Meeting won't join? Check DevTools → Network for a non-200 from the function.
- "No token returned" — the function secrets are probably missing.
- Mic/camera prompts — the app needs permission for the site.