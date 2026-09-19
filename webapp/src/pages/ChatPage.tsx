import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useAuth } from '../auth';
import {
  ensureChannels, fetchChannels, fetchMessages, sendMessage, subscribeMessages,
  dmKey, roleKey, fetchMembers, type Channel, type Message, type Member
} from '../lib/api';
import { Avatar, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import { timeAgo } from '../utils';
import { useToast } from '../components/toast';
import { roleDef } from '../roles';

interface ChatItem { key: string; label: string; kind: 'channel' | 'role' | 'dm'; sub?: string; }

export function ChatPage() {
  const { user, profile } = useAuth();
  const toast = useToast();
  const [items, setItems] = useState<ChatItem[]>([]);
  const [active, setActive] = useState<string>('general');
  const [messages, setMessages] = useState<Message[] | null>(null);
  const [draft, setDraft] = useState('');
  const scrollRef = useRef<HTMLDivElement>(null);

  const me = user?.email || '';
  const myName = profile?.full_name || me.split('@')[0] || 'Anonymous';

  useEffect(() => {
    let alive = true;
    ensureChannels().then(async () => {
      const chans: Channel[] = await fetchChannels().catch(() => []);
      const mems: Member[] = await fetchMembers().catch(() => []);
      if (!alive) return;
      const list: ChatItem[] = [];
      for (const c of chans.filter((x) => x.kind !== 'dm')) list.push({ key: c.key, label: c.name, kind: 'channel' });
      const role = profile?.role || 'Member';
      if (!chans.some((c) => c.key === roleKey(role))) {
        list.push({ key: roleKey(role), label: `${role}s`, kind: 'role', sub: 'Your ministry group' });
      }
      const others = mems.filter((m) => (m.email || '') !== me && m.email);
      for (const m of others) {
        list.push({ key: dmKey(me, m.email || ''), label: m.full_name || m.email, kind: 'dm', sub: m.role });
      }
      setItems(list);
    }).catch(() => { if (alive) setItems([{ key: 'general', label: 'General', kind: 'channel' }]); });
    return () => { alive = false; };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const loadMessages = useCallback(async (key: string) => {
    try {
      setMessages(await fetchMessages(key));
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
      setMessages([]);
    }
  }, [toast]);

  useEffect(() => {
    setMessages(null);
    loadMessages(active);
    const off = subscribeMessages(active, (m) => {
      setMessages((prev) => {
        const list = prev || [];
        if (list.some((x) => x.id === m.id)) return prev;
        return [...list, m];
      });
    });
    return () => off();
  }, [active, loadMessages]);

  useEffect(() => {
    const el = scrollRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [messages]);

  async function send() {
    const body = draft.trim();
    if (!body) return;
    setDraft('');
    try {
      await sendMessage(active, body, me, myName);
    } catch (e) {
      toast(e instanceof Error ? e.message : String(e), 'error');
    }
  }

  return (
    <div className="chat">
      <aside className="chat-side">
        <div className="chat-side-title">Channels</div>
        {items.length === 0 ? <Spinner /> : items.map((it) => (
          <button key={it.key}
            className={`chat-item ${active === it.key ? 'active' : ''}`}
            onClick={() => setActive(it.key)}>
            <span className={`chat-dot ${it.kind}`} />
            <span className="chat-item-label">{it.label}{it.sub && <em>{it.sub}</em>}</span>
          </button>
        ))}
      </aside>

      <section className="chat-main">
        <header className="chat-head">
          <span className="chat-head-title">{items.find((i) => i.key === active)?.label || 'Chat'}</span>
          <span className="muted small">live · private to this conversation</span>
        </header>
        <div className="chat-messages" ref={scrollRef}>
          {!messages ? <Spinner /> : messages.length === 0 ? (
            <Empty title="No messages yet" sub="Send the first message to start the conversation." />
          ) : messages.map((m) => {
            const own = m.sender === me;
            return (
              <div key={m.id} className={`bubble-wrap ${own ? 'own' : ''}`}>
                <Avatar name={m.sender_name || m.sender} size={30} />
                <div className={`bubble ${own ? 'own' : ''}`}>
                  <div className="bubble-meta">
                    <span className="bubble-name">{own ? 'You' : m.sender_name || m.sender}</span>
                    <span className="bubble-time">{timeAgo(m.created_at)}</span>
                  </div>
                  <div className="bubble-body">{m.body}</div>
                </div>
              </div>
            );
          })}
        </div>
        <footer className="chat-input">
          <input value={draft} placeholder={`Message ${items.find((i) => i.key === active)?.label || 'chat'}…`}
            onChange={(e) => setDraft(e.target.value)}
            onKeyDown={(e) => { if (e.key === 'Enter') send(); }} />
          <button className="btn btn-primary" onClick={send} disabled={!draft.trim()}><Icon.Send size={16} /></button>
        </footer>
      </section>
    </div>
  );
}