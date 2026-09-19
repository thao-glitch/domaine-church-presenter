import { useCallback, useEffect, useState } from 'react';
import { useAuth } from '../auth';
import { Card, Button, Modal, Field, Spinner, Empty } from '../components/ui';
import { Icon } from '../components/icons';
import {
  fetchSlideSets, createSlideSet, deleteSlideSet, fetchSlides,
  saveSlide, deleteSlide, getStage, setStage, type SlideSet, type Slide, type StageState
} from '../lib/api';
import { navigate } from '../lib/router';
import { useToast } from '../components/toast';

export function PresenterPage() {
  const { user, profile, canPresent } = useAuth();
  const toast = useToast();
  const [sets, setSets] = useState<SlideSet[] | null>(null);
  const [activeId, setActiveId] = useState<string | null>(null);
  const [slides, setSlides] = useState<Slide[]>([]);
  const [stage, setStageState] = useState<StageState | null>(null);
  const [idx, setIdx] = useState(0);
  const [text, setText] = useState('');
  const [bg, setBg] = useState('#0b0f1a');
  const [newTitle, setNewTitle] = useState('');
  const [showNew, setShowNew] = useState(false);

  const me = profile?.full_name || user?.email || '';

  const loadSets = useCallback(() => {
    fetchSlideSets().then(setSets).catch(() => setSets([]));
  }, []);

  useEffect(() => {
    loadSets();
    getStage().then(setStageState).catch(() => null);
  }, [loadSets]);

  async function selectSet(id: string) {
    setActiveId(id);
    const sl = await fetchSlides(id);
    setSlides(sl);
    setIdx(sl.length > 0 ? 0 : 0);
    setText(sl[0]?.text || '');
    setBg(sl[0]?.bg || '#0b0f1a');
  }

  const live = !!stage?.set_id && !!activeId && stage.set_id === activeId;

  async function refreshSlides(id: string) {
    const sl = await fetchSlides(id);
    setSlides(sl);
    if (sl[idx]) {
      setText(sl[idx].text);
      setBg(sl[idx].bg);
    }
  }

  async function onNewSet() {
    if (!newTitle.trim()) return;
    try {
      const s = await createSlideSet(newTitle.trim(), me);
      toast('Presentation created.', 'success');
      setShowNew(false); setNewTitle('');
      loadSets();
      await selectSet(s.id);
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function onAddSlide() {
    if (!activeId) return;
    try {
      await saveSlide({ set_id: activeId, idx: slides.length, text: '', bg: '#0b0f1a' });
      await refreshSlides(activeId);
      setIdx(slides.length);
      toast('Slide added.', 'success');
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function onSaveSlide() {
    if (!activeId || slides.length === 0) return;
    const s = slides[idx];
    try {
      await saveSlide({ id: s.id, set_id: activeId, text, bg });
      await refreshSlides(activeId);
      toast('Slide saved.', 'success');
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function onMove(dir: 1 | -1) {
    if (!activeId || slides.length === 0) return;
    const next = Math.max(0, Math.min(slides.length - 1, idx + dir));
    setIdx(next);
    if (slides[next]) { setText(slides[next].text); setBg(slides[next].bg); }
    if (live && stage.set_id === activeId) {
      try { await setStage(activeId, next); } catch { /* ignore */ }
    }
  }

  async function onGoLive() {
    if (!activeId || slides.length === 0) return;
    try {
      await setStage(activeId, idx);
      const st = await getStage();
      setStageState(st);
      toast('Now on the big screen.', 'success');
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function onStopLive() {
    if (!activeId) return;
    try {
      await setStage(null, 0);
      const st = await getStage();
      setStageState(st);
      toast('Stage cleared.', 'success');
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  async function onDeleteSet(s: SlideSet) {
    if (!confirm(`Delete "${s.title}" and all its slides?`)) return;
    try {
      await deleteSlideSet(s.id);
      toast('Presentation deleted.', 'success');
      if (activeId === s.id) { setActiveId(null); setSlides([]); }
      loadSets();
    } catch (e) { toast(e instanceof Error ? e.message : String(e), 'error'); }
  }

  if (!canPresent) {
    return <Card><Empty title="Stage control needs a presenting role" sub="Media, leadership and pastoral roles can run the stage." /></Card>;
  }

  return (
    <div className="stack">
      <Card title="Stage — live presentation"
        actions={<Button variant="ghost" onClick={() => navigate(`/stage-view/${activeId || ''}`)}>Open big-screen view <Icon.Stage size={15} /></Button>}>
        <p className="muted small">
          Build a presentation and push it to the big screen. The church TV just opens the
          <b> big-screen view</b> link in a browser and everyone follows along — modern, from any device.
          {live && <span className="ok"> ● LIVE now</span>}
        </p>
      </Card>

      {!sets ? <Spinner /> : (
        <div className="grid-stage">
          <Card title="Presentations" className="stage-sets"
            actions={<Button onClick={() => setShowNew(true)}><Icon.Plus size={15} /> New</Button>}>
            {sets.length === 0 && <Empty title="No presentations" sub="Create the first one." />}
            <div className="list">
              {sets.map((s) => (
                <div key={s.id} className={`list-row ${activeId === s.id ? 'selected' : ''}`}>
                  <button className="list-main" onClick={() => selectSet(s.id)}>
                    <div className="list-title">{s.title}</div>
                    <div className="muted small">by {s.owner}</div>
                  </button>
                  <div className="row-actions">
                    {activeId === s.id && <Button variant="soft" onClick={onGoLive}><Icon.Play size={14} /> Go live</Button>}
                    <button className="icon-btn danger" onClick={() => onDeleteSet(s)}><Icon.Trash size={16} /></button>
                  </div>
                </div>
              ))}
            </div>
            {stage?.set_id && <div className="muted small">Currently live: {sets.find((s) => s.id === stage.set_id)?.title || 'unknown set'} · slide { (stage.slide_idx ?? 0) + 1 }</div>}
          </Card>

          <Card title={activeId ? 'Slides' : 'Select a presentation'}
            className="stage-edit"
            actions={activeId && slides.length > 0 ? <Button variant="ghost" onClick={onAddSlide}><Icon.Plus size={15} /> Slide</Button> : undefined}>
            {!activeId ? <Empty title="Pick a presentation on the left" /> : (
              <>
                <div className="stage-thumb-row">
                  {slides.map((s, i) => (
                    <button key={s.id} className={`stage-thumb ${i === idx ? 'active' : ''}`}
                      onClick={() => { setIdx(i); setText(s.text); setBg(s.bg); }}>
                      <span className="stage-thumb-text">{s.text.split('\n')[0] || '—'}</span>
                      <span className="stage-thumb-idx">{i + 1}</span>
                    </button>
                  ))}
                  {slides.length === 0 && <Empty title="No slides yet" sub="Add the first slide." />}
                </div>

                <Field label={`Slide ${slides.length > 0 ? idx + 1 : 1} text`}>
                  <textarea rows={6} value={text} onChange={(e) => setText(e.target.value)}
                    placeholder="A verse or announcement…" />
                </Field>
                <div className="row-2">
                  <Field label="Background colour">
                    <input type="color" value={bg} onChange={(e) => setBg(e.target.value)} style={{ padding: 0, height: 38 }} />
                  </Field>
                  <Field label="&nbsp;">
                    <Button onClick={onSaveSlide} disabled={slides.length === 0}>Save slide</Button>
                  </Field>
                </div>

                <div className="stage-controls">
                  <Button variant="ghost" onClick={() => onMove(-1)} disabled={idx === 0}><ChevL /> Prev</Button>
                  <span className="muted small">Slide {idx + 1} of {slides.length}</span>
                  <Button variant="ghost" onClick={() => onMove(1)} disabled={idx >= slides.length - 1}>Next <ChevR /></Button>
                  <Button onClick={onGoLive} disabled={slides.length === 0}><Icon.Play size={15} /> Go live →</Button>
                  {live && <Button variant="danger" onClick={onStopLive}>Stop live</Button>}
                </div>
              </>
            )}
          </Card>
        </div>
      )}

      <Modal open={showNew} title="New presentation" onClose={() => setShowNew(false)}
        footer={<><Button variant="ghost" onClick={() => setShowNew(false)}>Cancel</Button><Button onClick={onNewSet} disabled={!newTitle.trim()}>Create</Button></>}>
        <Field label="Title">
          <input value={newTitle} onChange={(e) => setNewTitle(e.target.value)} placeholder="e.g. Sunday service" autoFocus />
        </Field>
      </Modal>
    </div>
  );
}

interface ChevProps { size?: number }
function ChevL({ size = 15 }: ChevProps) { return <span style={{ fontSize: size, lineHeight: 0 }}>‹</span>; }
function ChevR({ size = 15 }: ChevProps) { return <span style={{ fontSize: size, lineHeight: 0 }}>›</span>; }