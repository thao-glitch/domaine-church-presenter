import { useEffect, useRef, useState } from 'react';
import { fetchSlides, getStage, type Slide, type StageState } from '../lib/api';
import { navigate } from '../lib/router';
import { useAuth } from '../auth';
import { Button } from '../components/ui';

export function StageViewPage({ id }: { id: string }) {
  const { user } = useAuth();
  const [slides, setSlides] = useState<Slide[]>([]);
  const [liveIdx, setLiveIdx] = useState(0);
  const [hasLive, setHasLive] = useState(false);
  const [err, setErr] = useState('');
  const loadedSet = useRef<string | null>(null);
  const cancelledRef = useRef(false);

  useEffect(() => {
    cancelledRef.current = false;
    async function tick() {
      try {
        const st: StageState = await getStage();
        if (cancelledRef.current) return;
        if (st.set_id === id) {
          setHasLive(true);
          setLiveIdx(st.slide_idx ?? 0);
          if (loadedSet.current !== st.set_id) {
            loadedSet.current = st.set_id;
            setSlides(await fetchSlides(st.set_id));
          }
        } else {
          setHasLive(false);
        }
        setErr('');
      } catch (e) {
        setErr(e instanceof Error ? e.message : String(e));
      }
    }
    tick();
    const timer = window.setInterval(tick, 1500);
    return () => { cancelledRef.current = true; clearInterval(timer); };
  }, [id]);

  const slide = slides[liveIdx];
  const slideCount = slides.length;

  function fullscreen() {
    document.documentElement.requestFullscreen?.().catch(() => { /* noop */ });
  }

  return (
    <div className="stage-screen" style={{ width: '100vw', height: '100vh', display: 'flex', flexDirection: 'column' }}>
      <div className="stage-screen-exit">
        <Button variant="soft" onClick={fullscreen}>Full screen</Button>
        {window.self !== window.top && <span className="muted small">embedded</span>}
        <span className="stage-screen-spacer" />
        <span className="muted small">{user?.email}</span>
        <Button variant="ghost" onClick={() => navigate('/stage')}>← Control panel</Button>
      </div>

      {!hasLive && (
        <div className="stage-screen-center">
          <div className="stage-wait">{err ? `Connection issue: ${err}` : 'No live presentation right now'}</div>
          <div className="stage-wait-sub">Waiting for the presenter to go live…</div>
        </div>
      )}

      {hasLive && !slide && (
        <div className="stage-screen-center">
          <div className="stage-wait">Presentation not found or loading…</div>
        </div>
      )}

      {hasLive && slide && (
        <div className="stage-screen-slide" style={{ background: slide.bg || '#0b0f1a' }}>
          <div className="stage-screen-text">{slide.text || ' '}</div>
          <div className="stage-screen-foot">Domaine Church</div>
          <div className="stage-screen-count">{liveIdx + 1} / {slideCount || '·'}</div>
        </div>
      )}
    </div>
  );
}